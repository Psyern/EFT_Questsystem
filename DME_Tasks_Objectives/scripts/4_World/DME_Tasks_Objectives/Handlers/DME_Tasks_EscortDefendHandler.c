//! ESCORT / DEFEND (CONTRACTS §6.5, Agent 11) — indexiert unter ZONE_ENTER, ZONE_LEAVE, TIMER_TICK.
//!
//! DEFEND: Amount = Sekunden, die der Spieler IN der Zone (Def.Zone) verbringen muss.
//! ZONE_ENTER/ZONE_LEAVE pflegen ein In-Zone-Flag pro uid|questId|objectiveId (reiner Cache);
//! der 1-Hz-TIMER_TICK prueft die Position IMMER live gegen den Zonen-Zylinder und meldet nur
//! dann +1 — heilt verpasste Enter- UND Leave-Events (z. B. Reconnect/Teleport aus der Zone).
//! Flag-Cleanup via DME_Tasks_SpatialModule (Quest-Ende/Disconnect).
//!
//! ESCORT: benoetigt AI-Integration (Welle 4 kann diesen Handler modden) — bis dahin Warn-once
//! beim Indexieren und KEIN Progress.
class DME_Tasks_EscortDefendHandler : DME_Tasks_ObjectiveHandlerBase {
	//! uid|questId|objectiveId → Spieler steht aktuell in der Objective-Zone
	private ref map<string, bool> m_DME_InZone;
	private bool m_DME_EscortWarned;

	void DME_Tasks_EscortDefendHandler() {
		m_DME_InZone = new map<string, bool>();
		m_DME_EscortWarned = false;
	}

	override bool CanHandle(int objectiveType) {
		if (objectiveType == EDME_Tasks_ObjectiveType.ESCORT) {
			return true;
		}
		if (objectiveType == EDME_Tasks_ObjectiveType.DEFEND) {
			return true;
		}
		return false;
	}

	override void GetEventTypesFor(int objectiveType, array<int> outTypes) {
		if (objectiveType == EDME_Tasks_ObjectiveType.ESCORT) {
			WarnEscortOnce();
		}
		outTypes.Insert(EDME_Tasks_EventType.ZONE_ENTER);
		outTypes.Insert(EDME_Tasks_EventType.ZONE_LEAVE);
		outTypes.Insert(EDME_Tasks_EventType.TIMER_TICK);
	}

	override void OnEvent(DME_Tasks_ObjectiveRef objectiveRef, DME_Tasks_Event evt) {
		if (!objectiveRef || !objectiveRef.Def) {
			return;
		}
		if (!evt) {
			return;
		}

		DME_Tasks_ObjectiveDef def = objectiveRef.Def;
		int objectiveType = DME_Tasks_EnumUtil.ObjectiveTypeFromString(def.Type);
		if (objectiveType == EDME_Tasks_ObjectiveType.ESCORT) {
			//! Ohne AI-Integration kein Progress (Warn-once lief bereits beim Indexieren)
			return;
		}
		if (objectiveType != EDME_Tasks_ObjectiveType.DEFEND) {
			return;
		}
		if (!def.Zone) {
			return;
		}

		if (evt.m_DME_EventType == EDME_Tasks_EventType.ZONE_ENTER || evt.m_DME_EventType == EDME_Tasks_EventType.ZONE_LEAVE) {
			HandleZoneEvent(objectiveRef, evt);
			return;
		}

		if (evt.m_DME_EventType == EDME_Tasks_EventType.TIMER_TICK) {
			HandleTimerTick(objectiveRef, evt);
		}
	}

	// ==================================================================
	// State-Cleanup (via DME_Tasks_SpatialModule)
	// ==================================================================

	void ClearQuestState(string uid, string questId) {
		RemoveByPrefix(uid + "|" + questId + "|");
	}

	void ClearPlayerState(string uid) {
		RemoveByPrefix(uid + "|");
	}

	// ==================================================================
	// intern
	// ==================================================================

	private void HandleZoneEvent(DME_Tasks_ObjectiveRef objectiveRef, DME_Tasks_Event evt) {
		DME_Tasks_ZoneEvent zoneEvent = DME_Tasks_ZoneEvent.Cast(evt);
		if (!zoneEvent) {
			return;
		}

		DME_Tasks_ObjectiveDef def = objectiveRef.Def;
		string resolvedZoneId = DME_Tasks_SpatialUtil.ResolveZoneId(objectiveRef.QuestId, objectiveRef.ObjectiveId, def.Zone);
		if (zoneEvent.m_DME_ZoneId != resolvedZoneId) {
			return;
		}

		string stateKey = DME_Tasks_SpatialUtil.StateKey(objectiveRef);
		m_DME_InZone.Set(stateKey, zoneEvent.m_DME_Entered);
	}

	private void HandleTimerTick(DME_Tasks_ObjectiveRef objectiveRef, DME_Tasks_Event evt) {
		DME_Tasks_TimerEvent timerEvent = DME_Tasks_TimerEvent.Cast(evt);
		if (!timerEvent) {
			return;
		}

		DME_Tasks_ObjectiveDef def = objectiveRef.Def;
		if (!IsInTimeWindow(def.FromHour, def.ToHour)) {
			return;
		}

		string stateKey = DME_Tasks_SpatialUtil.StateKey(objectiveRef);

		//! Zylinder-Check bei JEDEM Tick (TimerEvent-Position = Spielerposition) — das In-Zone-Flag
		//! ist nur noch Cache; heilt verpasste Enter- UND Leave-Events (z. B. Reconnect/Teleport)
		bool inZone = DME_Tasks_SpatialUtil.IsPositionInZone(timerEvent.m_DME_Position, def.Zone);
		m_DME_InZone.Set(stateKey, inZone);
		if (!inZone) {
			return;
		}

		//! 1 Tick = 1 Sekunde in der Zone verteidigt
		ReportProgress(objectiveRef, 1);
	}

	private void RemoveByPrefix(string prefix) {
		for (int i = m_DME_InZone.Count() - 1; i >= 0; i--) {
			string key = m_DME_InZone.GetKey(i);
			if (key.IndexOf(prefix) == 0) {
				m_DME_InZone.RemoveElement(i);
			}
		}
	}

	private void WarnEscortOnce() {
		if (m_DME_EscortWarned) {
			return;
		}
		m_DME_EscortWarned = true;
		DME_Tasks_Log.Warn("EscortDefendHandler: ESCORT-Objectives benoetigen die AI-Integration (Welle 4) — bis dahin kein Progress");
	}
}
