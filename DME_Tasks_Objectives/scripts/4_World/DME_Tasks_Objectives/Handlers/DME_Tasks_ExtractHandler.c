//! EXTRACT-Objective-Handler (CONTRACTS §6.5, Agent 12) — Extraktionspunkt = Def.Zone.
//! Den Trigger erzeugt der ZoneTriggerManager automatisch (Objective ist unter ZONE_ENTER/
//! ZONE_LEAVE indexiert); der Handler registriert selbst nichts.
//!
//! Ablauf:
//! - ZONE_ENTER (eigene Zone) → Expedition.ExtractStartedAt = NowEpoch + Notification
//!   "Extraktion laeuft (N s)" (N = ObjectiveDef.Amount = Extraktionszeit in SEKUNDEN, min 1).
//! - ZONE_LEAVE → ExtractStartedAt = -1 + Notification (Abbruch — failt NICHT).
//! - TIMER_TICK (1 Hz vom ObjectivesModule) → wenn ExtractStartedAt > 0 und
//!   NowEpoch - ExtractStartedAt >= Extraktionszeit → ExpeditionService.OnExtracted +
//!   ReportProgress(voll) → Objective Done (Required = Amount, vom Engine-Accept geklemmt auf min 1).
//!
//! ActiveQuest.Expedition legt der Core-ExpeditionService bei Quest-Aktivierung an — kann null
//! sein (Service nicht gebootstrapped / Quest ohne EXTRACT-Objective) → ueberall null-checken.
class DME_Tasks_ExtractHandler : DME_Tasks_ObjectiveHandlerBase {
	private static const int MIN_EXTRACT_SECONDS = 1;

	override bool CanHandle(int objectiveType) {
		return objectiveType == EDME_Tasks_ObjectiveType.EXTRACT;
	}

	override void GetEventTypesFor(int objectiveType, array<int> outTypes) {
		outTypes.Insert(EDME_Tasks_EventType.ZONE_ENTER);
		outTypes.Insert(EDME_Tasks_EventType.ZONE_LEAVE);
		outTypes.Insert(EDME_Tasks_EventType.TIMER_TICK);
	}

	override void OnEvent(DME_Tasks_ObjectiveRef objectiveRef, DME_Tasks_Event evt) {
		if (!objectiveRef || !objectiveRef.Def || !evt) {
			return;
		}

		if (evt.m_DME_EventType == EDME_Tasks_EventType.TIMER_TICK) {
			DME_Tasks_TimerEvent timerEvent = DME_Tasks_TimerEvent.Cast(evt);
			if (!timerEvent) {
				return;
			}
			HandleTimerTick(objectiveRef, timerEvent);
			return;
		}

		DME_Tasks_ZoneEvent zoneEvent = DME_Tasks_ZoneEvent.Cast(evt);
		if (!zoneEvent) {
			return;
		}

		//! Nur die EIGENE Extraktionszone zaehlt (Spieler kann weitere Zonen-Objectives haben)
		string zoneId = ResolveZoneId(objectiveRef);
		if (zoneId == "" || zoneEvent.m_DME_ZoneId != zoneId) {
			return;
		}

		if (zoneEvent.m_DME_Entered) {
			HandleZoneEnter(objectiveRef, zoneId);
		} else {
			HandleZoneLeave(objectiveRef, zoneId);
		}
	}

	// ==================================================================
	// intern
	// ==================================================================

	private void HandleZoneEnter(DME_Tasks_ObjectiveRef objectiveRef, string zoneId) {
		DME_Tasks_QuestEngine engine = DME_Tasks_QuestEngine.GetInstance();
		DME_Tasks_ActiveQuest active = engine.GetActiveQuest(objectiveRef.PlayerUid, objectiveRef.QuestId);
		if (!active) {
			return;
		}
		DME_Tasks_ExpeditionState session = active.Expedition;
		if (!session) {
			return;
		}

		DME_Tasks_ExpeditionService.GetInstance().OnZoneEnter(objectiveRef.PlayerUid, objectiveRef.QuestId, zoneId);

		if (session.Extracted) {
			return;
		}
		if (session.ExtractStartedAt > 0) {
			return;
		}

		session.ExtractStartedAt = DME_Tasks_TimeUtil.NowEpoch();
		DME_Tasks_PlayerStore.GetInstance().MarkDirty(objectiveRef.PlayerUid);

		int extractSeconds = GetExtractSeconds(objectiveRef.Def);
		engine.NotifyPlayer(objectiveRef.PlayerUid, GetQuestTitle(objectiveRef.QuestId), DME_Tasks_LocKeys.NOTIF_EXTRACT_RUNNING, EDME_Tasks_NotificationType.PROGRESS, extractSeconds.ToString());
	}

	private void HandleZoneLeave(DME_Tasks_ObjectiveRef objectiveRef, string zoneId) {
		DME_Tasks_QuestEngine engine = DME_Tasks_QuestEngine.GetInstance();
		DME_Tasks_ActiveQuest active = engine.GetActiveQuest(objectiveRef.PlayerUid, objectiveRef.QuestId);
		if (!active) {
			return;
		}
		DME_Tasks_ExpeditionState session = active.Expedition;
		if (!session) {
			return;
		}

		DME_Tasks_ExpeditionService.GetInstance().OnZoneLeave(objectiveRef.PlayerUid, objectiveRef.QuestId, zoneId);

		if (session.Extracted) {
			return;
		}
		if (session.ExtractStartedAt <= 0) {
			return;
		}

		session.ExtractStartedAt = -1;
		DME_Tasks_PlayerStore.GetInstance().MarkDirty(objectiveRef.PlayerUid);
		engine.NotifyPlayer(objectiveRef.PlayerUid, GetQuestTitle(objectiveRef.QuestId), DME_Tasks_LocKeys.NOTIF_EXTRACT_ABORTED, EDME_Tasks_NotificationType.WARNING);
	}

	private void HandleTimerTick(DME_Tasks_ObjectiveRef objectiveRef, DME_Tasks_TimerEvent timerEvent) {
		DME_Tasks_QuestEngine engine = DME_Tasks_QuestEngine.GetInstance();
		DME_Tasks_ActiveQuest active = engine.GetActiveQuest(objectiveRef.PlayerUid, objectiveRef.QuestId);
		if (!active) {
			return;
		}
		DME_Tasks_ExpeditionState session = active.Expedition;
		if (!session) {
			return;
		}
		if (session.Extracted) {
			return;
		}
		if (session.ExtractStartedAt <= 0) {
			return;
		}

		int extractSeconds = GetExtractSeconds(objectiveRef.Def);
		int elapsed = timerEvent.m_DME_NowEpoch - session.ExtractStartedAt;
		if (elapsed < extractSeconds) {
			return;
		}

		//! Defense-in-Depth: Live-Position pruefen (TimerEvent-Position = Spielerposition) —
		//! faengt stale ExtractStartedAt-Werte ab, deren Leave-Event verloren ging.
		if (!DME_Tasks_SpatialUtil.IsPositionInZone(timerEvent.m_DME_Position, objectiveRef.Def.Zone)) {
			session.ExtractStartedAt = -1;
			DME_Tasks_PlayerStore.GetInstance().MarkDirty(objectiveRef.PlayerUid);
			return;
		}

		//! Handler pflegt ExtractStartedAt; Extracted + MarkDirty macht der Service
		session.ExtractStartedAt = -1;
		DME_Tasks_ExpeditionService.GetInstance().OnExtracted(objectiveRef.PlayerUid, objectiveRef.QuestId);
		engine.NotifyPlayer(objectiveRef.PlayerUid, GetQuestTitle(objectiveRef.QuestId), DME_Tasks_LocKeys.NOTIF_EXTRACT_SUCCESS, EDME_Tasks_NotificationType.SUCCESS);

		//! Voller Fortschritt: Required = max(1, Amount) = extractSeconds → Objective Done
		ReportProgress(objectiveRef, extractSeconds);
	}

	//! Konvention (SPEC §7 Agent 12): ObjectiveDef.Amount = Extraktionszeit in Sekunden, min 1.
	private int GetExtractSeconds(DME_Tasks_ObjectiveDef def) {
		int seconds = def.Amount;
		if (seconds < MIN_EXTRACT_SECONDS) {
			seconds = MIN_EXTRACT_SECONDS;
		}
		return seconds;
	}

	//! MUSS die ZoneId-Regel des ZoneTriggerManagers spiegeln (CONTRACTS §6.5):
	//! Def.Zone.ZoneId, Fallback "questId:objectiveId". Ohne Zone: "" (kein Trigger, kein Match).
	private string ResolveZoneId(DME_Tasks_ObjectiveRef objectiveRef) {
		if (!objectiveRef.Def.Zone) {
			return "";
		}
		string zoneId = objectiveRef.Def.Zone.ZoneId;
		if (zoneId == "") {
			zoneId = objectiveRef.QuestId + ":" + objectiveRef.ObjectiveId;
		}
		return zoneId;
	}

	private string GetQuestTitle(string questId) {
		DME_Tasks_QuestDef def = DME_Tasks_QuestEngine.GetInstance().GetQuestDef(questId);
		if (def && def.Title != "") {
			return def.Title;
		}
		return questId;
	}
}
