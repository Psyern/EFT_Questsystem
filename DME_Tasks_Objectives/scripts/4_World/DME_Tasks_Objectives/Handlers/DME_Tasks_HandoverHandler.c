//! HANDOVER/DELIVER-Handler (CONTRACTS §6.5, Agent 10).
//! - HANDOVER: laeuft NICHT ueber den EventBus — die Abgabe stoesst der HandoverItems-Engine-Override
//!   (Hooks/DME_Tasks_EngineHandoverHook.c) an. Der HANDOVER-EventType-Bucket ist ein Dummy,
//!   der das Objective im Active-Objective-Index sichtbar haelt (niemand feuert HANDOVER-Events).
//! - DELIVER: zusaetzlich unter ZONE_ENTER indexiert — der ZoneTriggerManager erzeugt automatisch
//!   einen Trigger (Def.Zone!); beim Betreten des Abgabeorts laeuft dieselbe Abgabe-Pipeline,
//!   still wenn keine passenden Items im Inventar sind.
class DME_Tasks_HandoverHandler : DME_Tasks_ObjectiveHandlerBase {
	override bool CanHandle(int objectiveType) {
		if (objectiveType == EDME_Tasks_ObjectiveType.HANDOVER) {
			return true;
		}
		if (objectiveType == EDME_Tasks_ObjectiveType.DELIVER) {
			return true;
		}
		return false;
	}

	override void GetEventTypesFor(int objectiveType, array<int> outTypes) {
		outTypes.Insert(EDME_Tasks_EventType.HANDOVER);
		if (objectiveType == EDME_Tasks_ObjectiveType.DELIVER) {
			outTypes.Insert(EDME_Tasks_EventType.ZONE_ENTER);
		}
	}

	override void OnEvent(DME_Tasks_ObjectiveRef objectiveRef, DME_Tasks_Event evt) {
		if (!objectiveRef || !objectiveRef.Def) {
			return;
		}

		//! Nur der DELIVER-Zonen-Pfad reagiert auf Events (HANDOVER-Bucket ist ein Dummy)
		DME_Tasks_ZoneEvent zoneEvent = DME_Tasks_ZoneEvent.Cast(evt);
		if (!zoneEvent) {
			return;
		}
		if (!zoneEvent.m_DME_Entered) {
			return;
		}

		int objectiveType = DME_Tasks_EnumUtil.ObjectiveTypeFromString(objectiveRef.Def.Type);
		if (objectiveType != EDME_Tasks_ObjectiveType.DELIVER) {
			return;
		}

		if (!objectiveRef.Def.Zone) {
			return;
		}

		//! ZONE_ENTER-Events erreichen ALLE Zonen-Refs des Spielers — nur die eigene Zone zaehlt
		//! (ZoneId-Aufloesung identisch zum ZoneTriggerManager: Def.Zone.ZoneId, Fallback quest:objective)
		string expectedZoneId = objectiveRef.Def.Zone.ZoneId;
		if (expectedZoneId == "") {
			expectedZoneId = objectiveRef.QuestId + ":" + objectiveRef.ObjectiveId;
		}
		if (zoneEvent.m_DME_ZoneId != expectedZoneId) {
			return;
		}

		DME_Tasks_HandoverProcessor.ProcessHandover(objectiveRef.PlayerUid, objectiveRef.QuestId, objectiveRef.ObjectiveId, false);
	}
}

//! Gemeinsame Abgabe-Pipeline (CONTRACTS §6.5) — genutzt vom HandoverItems-Engine-Override
//! (RPC-Pfad) und vom DELIVER-Zonen-Pfad oben. Ausschliesslich Dedicated Server.
//!
//! Ablauf: Quest aktiv & Objective HANDOVER/DELIVER & nicht Done -> Zonen-Check (falls Def.Zone) ->
//! ReferencesObjective-Aufloesung (Match-Regeln aus dem referenzierten COLLECT-Objective) ->
//! Inventar-Scan -> partielle Abgabe (AllowPartialHandover) -> sichere Entfernung via
//! g_Game.ObjectDelete -> AddObjectiveProgress(abgegeben) -> SOFORT PlayerStore.SaveNow ->
//! RPC_Notification mit benoetigt/abgegeben/verbleibend.
//! Keine Doppel-Abgabe moeglich: entfernt wird hoechstens der noch offene Rest (Required - Current);
//! Done-Objectives brechen frueh ab.
class DME_Tasks_HandoverProcessor {
	private static const float DEFAULT_ZONE_RADIUS = 25.0;
	private static const float DEFAULT_ZONE_HEIGHT = 50.0;

	//! notifyWhenNoItems: true fuer den RPC-Pfad; false fuer den automatischen DELIVER-Zonen-Pfad
	//! (kein Notification-Spam beim Betreten ohne passende Items).
	static void ProcessHandover(string uid, string questId, string objectiveId, bool notifyWhenNoItems) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (uid == "" || questId == "" || objectiveId == "") {
			return;
		}

		DME_Tasks_QuestEngine engine = DME_Tasks_QuestEngine.GetInstance();

		DME_Tasks_ActiveQuest active = engine.GetActiveQuest(uid, questId);
		if (!active) {
			return;
		}
		if (active.State != EDME_Tasks_QuestState.ACTIVE && active.State != EDME_Tasks_QuestState.READY_TO_TURN_IN) {
			return;
		}

		DME_Tasks_ObjectiveDef objectiveDef = engine.GetObjectiveDef(questId, objectiveId);
		if (!objectiveDef) {
			return;
		}

		int objectiveType = DME_Tasks_EnumUtil.ObjectiveTypeFromString(objectiveDef.Type);
		if (objectiveType != EDME_Tasks_ObjectiveType.HANDOVER && objectiveType != EDME_Tasks_ObjectiveType.DELIVER) {
			return;
		}

		DME_Tasks_ObjectiveProgress progress = FindProgress(active, objectiveId);
		if (!progress || progress.Done) {
			return;
		}

		int remaining = progress.Required - progress.Current;
		if (remaining <= 0) {
			return;
		}

		PlayerBase player = engine.FindPlayerByUid(uid);
		if (!player) {
			return;
		}

		string title = GetTitle(engine, questId);

		//! Abgabeort pruefen, falls das Objective eine Zone definiert (§6.5)
		if (objectiveDef.Zone && !IsPlayerInZone(player, objectiveDef.Zone)) {
			engine.NotifyPlayer(uid, title, "Du bist nicht am Abgabeort", EDME_Tasks_NotificationType.WARNING);
			return;
		}

		//! ReferencesObjective: HANDOVER/DELIVER zieht ClassNames/Origin-Regeln aus dem
		//! referenzierten (COLLECT-)Objective derselben Quest
		DME_Tasks_ObjectiveDef matchDef = objectiveDef;
		if (objectiveDef.ReferencesObjective != "") {
			DME_Tasks_ObjectiveDef referenced = engine.GetObjectiveDef(questId, objectiveDef.ReferencesObjective);
			if (referenced) {
				matchDef = referenced;
			} else {
				DME_Tasks_Log.Warn("HandoverProcessor: ReferencesObjective %1 in Quest %2 unbekannt — nutze eigene Regeln", objectiveDef.ReferencesObjective, questId);
			}
		}

		array<ItemBase> matched = new array<ItemBase>();
		DME_Tasks_OriginService.CollectMatchingItems(player, matchDef, uid, active.AcceptedAt, matched);

		if (matched.Count() == 0) {
			if (notifyWhenNoItems) {
				engine.NotifyPlayer(uid, title, "Keine passenden Gegenstaende im Inventar", EDME_Tasks_NotificationType.WARNING);
			}
			return;
		}

		if (!objectiveDef.AllowPartialHandover && matched.Count() < remaining) {
			string partialMessage = "Teilabgabe nicht erlaubt — benoetigt: " + remaining.ToString() + ", vorhanden: " + matched.Count().ToString();
			engine.NotifyPlayer(uid, title, partialMessage, EDME_Tasks_NotificationType.WARNING);
			return;
		}

		int toRemove = matched.Count();
		if (toRemove > remaining) {
			toRemove = remaining;
		}

		//! Sichere Entfernung: ObjectDelete je Item; Null-Check schuetzt gegen bereits (mit einem
		//! Parent-Container) entfernte Eintraege
		int removed = 0;
		for (int i = 0; i < matched.Count(); i++) {
			if (removed >= toRemove) {
				break;
			}
			ItemBase item = matched.Get(i);
			if (!item) {
				continue;
			}
			g_Game.ObjectDelete(item);
			removed++;
		}

		if (removed == 0) {
			return;
		}

		engine.AddObjectiveProgress(uid, questId, objectiveId, removed);
		DME_Tasks_PlayerStore.GetInstance().SaveNow(uid);

		int newRemaining = remaining - removed;
		if (newRemaining < 0) {
			newRemaining = 0;
		}
		string message = "Abgegeben: " + removed.ToString() + " — Benoetigt: " + progress.Required.ToString() + " — Verbleibend: " + newRemaining.ToString();
		engine.NotifyPlayer(uid, title, message, EDME_Tasks_NotificationType.SUCCESS);

		DME_Tasks_Log.Info("Handover: %1 hat %2 Item(s) abgegeben (%3)", uid, removed.ToString(), questId + "/" + objectiveId);
	}

	// ==================================================================
	// intern
	// ==================================================================

	private static DME_Tasks_ObjectiveProgress FindProgress(DME_Tasks_ActiveQuest active, string objectiveId) {
		foreach (DME_Tasks_ObjectiveProgress progress : active.Objectives) {
			if (progress && progress.ObjectiveId == objectiveId) {
				return progress;
			}
		}
		return null;
	}

	private static string GetTitle(DME_Tasks_QuestEngine engine, string questId) {
		DME_Tasks_QuestDef def = engine.GetQuestDef(questId);
		if (def && def.Title != "") {
			return def.Title;
		}
		return questId;
	}

	//! Zylinder-Check wie beim ZoneTrigger: horizontale Distanz <= Radius, vertikal symmetrisch
	//! um CenterY (Defaults identisch zum ZoneTriggerManager: Radius 25 m, Hoehe 50 m).
	private static bool IsPlayerInZone(PlayerBase player, DME_Tasks_ZoneVolume zone) {
		float radius = zone.Radius;
		if (radius <= 0) {
			radius = DEFAULT_ZONE_RADIUS;
		}
		float height = zone.Height;
		if (height <= 0) {
			height = DEFAULT_ZONE_HEIGHT;
		}

		vector playerPos = player.GetPosition();

		float dx = playerPos[0] - zone.CenterX;
		float dz = playerPos[2] - zone.CenterZ;
		float horizontalSq = dx * dx + dz * dz;
		if (horizontalSq > radius * radius) {
			return false;
		}

		float dy = playerPos[1] - zone.CenterY;
		float halfHeight = height * 0.5;
		if (dy < -halfHeight || dy > halfHeight) {
			return false;
		}

		return true;
	}
}
