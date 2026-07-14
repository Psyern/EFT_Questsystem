//! Trigger-Verwaltung fuer Zonen-Objectives (CONTRACTS §6.5) — Singleton, server-only.
//! EIN Trigger pro ZoneId mit Refcount: mehrere Spieler/Objectives mit derselben Zone teilen sich
//! denselben DME_Tasks_ZoneTrigger; zerstoert wird erst, wenn der Refcount auf 0 faellt.
//!
//! ERFORDERLICHE API der Trigger-Klasse (Agent 11, Hooks/DME_Tasks_ZoneTrigger.c):
//!   class DME_Tasks_ZoneTrigger : Trigger {
//!       void SetZoneInfo(string zoneId, float radius);   // speichert ZoneId + Zylinder-Radius
//!       // OnEnterServerEvent/OnLeaveServerEvent (super zuerst!): insider.GetObject() → PlayerBase.Cast
//!       // + Null-Check; horizontale Distanz <= radius pruefen (Box ist nur die Grob-Huelle);
//!       // dann DME_Tasks_ZoneEvent bauen (SetEntered(true/false), m_DME_ZoneId, m_DME_PlayerUid =
//!       // identity.GetId(), m_DME_Position) → DME_Tasks_EventBus.GetInstance().Dispatch(evt).
//!   }
class DME_Tasks_ZoneTriggerManager {
	private static ref DME_Tasks_ZoneTriggerManager s_DME_Instance;

	private static const float DEFAULT_ZONE_RADIUS = 25.0;
	private static const float DEFAULT_ZONE_HEIGHT = 50.0;

	//! zoneId → Trigger + Refcount
	private ref map<string, ref DME_Tasks_ZoneTriggerEntry> m_DME_Triggers;
	//! uid → (questId → registrierte ZoneIds) — eigene Buchfuehrung, unabhaengig vom ObjectiveIndex,
	//! damit die Aufruf-Reihenfolge der Modul-Abos keine Rolle spielt.
	private ref map<string, ref map<string, ref array<string>>> m_DME_PlayerQuestZones;

	static DME_Tasks_ZoneTriggerManager GetInstance() {
		if (!s_DME_Instance) {
			s_DME_Instance = new DME_Tasks_ZoneTriggerManager();
		}
		return s_DME_Instance;
	}

	void DME_Tasks_ZoneTriggerManager() {
		m_DME_Triggers = new map<string, ref DME_Tasks_ZoneTriggerEntry>();
		m_DME_PlayerQuestZones = new map<string, ref map<string, ref array<string>>>();
	}

	//! Erzeugt/refcountet Trigger fuer jedes Zonen-Objective der Quest (Def.Zone gesetzt UND der
	//! zustaendige Handler indexiert unter ZONE_ENTER/ZONE_LEAVE). Idempotent (Release zuerst).
	void OnQuestActivated(string uid, string questId) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (uid == "" || questId == "") {
			return;
		}

		OnQuestDeactivated(uid, questId);

		DME_Tasks_QuestDef def = DME_Tasks_ConfigService.GetInstance().GetQuest(questId);
		if (!def) {
			return;
		}

		DME_Tasks_ActiveQuest active = DME_Tasks_QuestEngine.GetInstance().GetActiveQuest(uid, questId);

		DME_Tasks_ObjectiveRegistry registry = DME_Tasks_ObjectiveRegistry.GetInstance();
		array<int> eventTypes = new array<int>();

		foreach (DME_Tasks_ObjectiveDef objectiveDef : def.Objectives) {
			if (!objectiveDef) {
				continue;
			}
			if (!objectiveDef.Zone) {
				continue;
			}
			if (active && IsObjectiveDone(active, objectiveDef.ObjectiveId)) {
				continue;
			}

			int objectiveType = DME_Tasks_EnumUtil.ObjectiveTypeFromString(objectiveDef.Type);
			if (objectiveType == -1) {
				continue;
			}

			DME_Tasks_ObjectiveHandlerBase handler = registry.GetHandler(objectiveType);
			if (!handler) {
				continue;
			}

			eventTypes.Clear();
			handler.GetEventTypesFor(objectiveType, eventTypes);

			bool wantsEnter = eventTypes.Find(EDME_Tasks_EventType.ZONE_ENTER) != -1;
			bool wantsLeave = eventTypes.Find(EDME_Tasks_EventType.ZONE_LEAVE) != -1;
			if (!wantsEnter && !wantsLeave) {
				continue;
			}

			RegisterZoneObjective(uid, questId, objectiveDef);
		}
	}

	//! Gibt alle Zonen-Registrierungen der Quest frei (Refcount; Trigger-Zerstoerung bei 0).
	void OnQuestDeactivated(string uid, string questId) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		map<string, ref array<string>> byQuest = m_DME_PlayerQuestZones.Get(uid);
		if (!byQuest) {
			return;
		}

		array<string> zoneIds = byQuest.Get(questId);
		if (zoneIds) {
			foreach (string zoneId : zoneIds) {
				ReleaseZone(zoneId);
			}
			byQuest.Remove(questId);
		}

		if (byQuest.Count() == 0) {
			m_DME_PlayerQuestZones.Remove(uid);
		}
	}

	//! Disconnect: gibt ALLE Zonen-Registrierungen des Spielers frei.
	void OnPlayerRemoved(string uid) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		//! Stale-Insider-Cleanup: Uid aus dem Zylinder-Tracking ALLER aktiven Trigger entfernen
		//! (ohne Leave-Dispatch — der Spieler-State wird beim Disconnect ohnehin komplett abgeraeumt)
		for (int t = m_DME_Triggers.Count() - 1; t >= 0; t--) {
			DME_Tasks_ZoneTriggerEntry triggerEntry = m_DME_Triggers.GetElement(t);
			if (triggerEntry && triggerEntry.m_DME_Trigger) {
				triggerEntry.m_DME_Trigger.RemoveInsider(uid);
			}
		}

		map<string, ref array<string>> byQuest = m_DME_PlayerQuestZones.Get(uid);
		if (!byQuest) {
			return;
		}

		for (int i = byQuest.Count() - 1; i >= 0; i--) {
			array<string> zoneIds = byQuest.GetElement(i);
			if (!zoneIds) {
				continue;
			}
			foreach (string zoneId : zoneIds) {
				ReleaseZone(zoneId);
			}
		}

		m_DME_PlayerQuestZones.Remove(uid);
	}

	//! Registriert genau EIN Zonen-Objective (intern von OnQuestActivated genutzt; public, damit
	//! Handler/Hooks der Agenten 9-12 bei Bedarf gezielt Zonen anfordern koennen).
	//! ZoneId = Def.Zone.ZoneId, Fallback "questId:objectiveId" (CONTRACTS §6.5).
	void RegisterZoneObjective(string uid, string questId, DME_Tasks_ObjectiveDef objectiveDef) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!objectiveDef || !objectiveDef.Zone) {
			return;
		}

		string zoneId = ResolveZoneId(questId, objectiveDef);
		bool acquired = AcquireZone(zoneId, objectiveDef.Zone);
		if (!acquired) {
			return;
		}

		map<string, ref array<string>> byQuest = m_DME_PlayerQuestZones.Get(uid);
		if (!byQuest) {
			byQuest = new map<string, ref array<string>>();
			m_DME_PlayerQuestZones.Set(uid, byQuest);
		}

		array<string> zoneIds = byQuest.Get(questId);
		if (!zoneIds) {
			zoneIds = new array<string>();
			byQuest.Set(questId, zoneIds);
		}

		zoneIds.Insert(zoneId);
	}

	// ==================================================================
	// intern
	// ==================================================================

	private string ResolveZoneId(string questId, DME_Tasks_ObjectiveDef objectiveDef) {
		string zoneId = objectiveDef.Zone.ZoneId;
		if (zoneId == "") {
			zoneId = questId + ":" + objectiveDef.ObjectiveId;
		}
		return zoneId;
	}

	private bool AcquireZone(string zoneId, DME_Tasks_ZoneVolume zone) {
		DME_Tasks_ZoneTriggerEntry entry = m_DME_Triggers.Get(zoneId);
		if (entry) {
			entry.m_DME_RefCount = entry.m_DME_RefCount + 1;
			return true;
		}

		DME_Tasks_ZoneTrigger trigger = CreateTrigger(zoneId, zone);
		if (!trigger) {
			return false;
		}

		entry = new DME_Tasks_ZoneTriggerEntry();
		entry.m_DME_Trigger = trigger;
		entry.m_DME_RefCount = 1;
		m_DME_Triggers.Set(zoneId, entry);
		return true;
	}

	private void ReleaseZone(string zoneId) {
		DME_Tasks_ZoneTriggerEntry entry = m_DME_Triggers.Get(zoneId);
		if (!entry) {
			return;
		}

		entry.m_DME_RefCount = entry.m_DME_RefCount - 1;
		if (entry.m_DME_RefCount > 0) {
			return;
		}

		if (entry.m_DME_Trigger && g_Game) {
			g_Game.ObjectDelete(entry.m_DME_Trigger);
		}
		entry.m_DME_Trigger = null;
		m_DME_Triggers.Remove(zoneId);
		DME_Tasks_Log.Debug("ZoneTriggerManager: Zone %1 entfernt", zoneId);
	}

	//! CONTRACTS §6.3-Muster (EffectArea/Hologram): Script-Trigger sind per Klassennamen spawnbar,
	//! KEIN CfgVehicles-Eintrag noetig. Zylinder-Zone: Box-Huelle [-r,-h/2,-r]..[r,h/2,r] um das
	//! Zentrum; die exakte Radius-Pruefung macht der Trigger selbst in OnEnter/OnLeaveServerEvent.
	private DME_Tasks_ZoneTrigger CreateTrigger(string zoneId, DME_Tasks_ZoneVolume zone) {
		if (!g_Game) {
			return null;
		}

		float radius = zone.Radius;
		if (radius <= 0) {
			radius = DEFAULT_ZONE_RADIUS;
		}
		float height = zone.Height;
		if (height <= 0) {
			height = DEFAULT_ZONE_HEIGHT;
		}
		float halfHeight = height * 0.5;

		vector pos = Vector(zone.CenterX, zone.CenterY, zone.CenterZ);
		Object created = g_Game.CreateObjectEx("DME_Tasks_ZoneTrigger", pos, ECE_NONE);

		DME_Tasks_ZoneTrigger trigger;
		if (!Class.CastTo(trigger, created)) {
			if (created) {
				g_Game.ObjectDelete(created);
			}
			DME_Tasks_Log.Error("ZoneTriggerManager: Trigger fuer Zone %1 konnte nicht erzeugt werden", zoneId);
			return null;
		}

		vector mins = Vector(-radius, -halfHeight, -radius);
		vector maxs = Vector(radius, halfHeight, radius);
		trigger.SetExtents(mins, maxs);
		trigger.SetZoneInfo(zoneId, radius);

		DME_Tasks_Log.Debug("ZoneTriggerManager: Zone %1 aktiv (r=%2)", zoneId, radius.ToString());
		return trigger;
	}

	private bool IsObjectiveDone(DME_Tasks_ActiveQuest active, string objectiveId) {
		foreach (DME_Tasks_ObjectiveProgress progress : active.Objectives) {
			if (progress && progress.ObjectiveId == objectiveId) {
				return progress.Done;
			}
		}
		return false;
	}
}

//! Interner Refcount-Eintrag des ZoneTriggerManagers.
class DME_Tasks_ZoneTriggerEntry {
	DME_Tasks_ZoneTrigger m_DME_Trigger;	// Entity — engine-owned, bewusst OHNE ref
	int m_DME_RefCount;

	void DME_Tasks_ZoneTriggerEntry() {
		m_DME_Trigger = null;
		m_DME_RefCount = 0;
	}
}
