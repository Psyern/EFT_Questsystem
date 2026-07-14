//! TRAVEL / DISCOVER (CONTRACTS §6.5, Agent 11) — indexiert unter ZONE_ENTER.
//!
//! Semantik:
//! - Def.Zone gesetzt (Amount=1-Konvention: EINE Zone pro Objective): erster Besuch der Zone
//!   (ZoneId = Zone.ZoneId, Fallback "questId:objectiveId") → ReportProgress(+1), genau einmal.
//! - Def.RequiredZones nicht leer: mehrere Ziele — jede EINZIGARTIGE besuchte Zone aus der Liste
//!   → +1 (Amount sollte = RequiredZones.Count() sein). Besuchte Zonen werden pro
//!   uid|questId|objectiveId im Handler getrackt (Cleanup via DME_Tasks_SpatialModule).
//!   HINWEIS: RequiredZones-Eintraege muessen als ZoneId eines aktiven Zonen-Triggers existieren
//!   (der ZoneTriggerManager erzeugt Trigger nur fuer Objectives mit gesetztem Def.Zone) —
//!   z. B. ueber weitere Objectives derselben/anderer aktiver Quests mit identischer ZoneId.
//! - FromHour/ToHour wird gegen die In-Game-Stunde geprueft (IsInTimeWindow).
//!
//! MVP-Grenze: Der Besucht-Status lebt nur im Speicher — nach Server-Neustart/Reconnect kann eine
//! bereits gezaehlte Zone erneut zaehlen (persistiert ist nur der Zaehlerstand Current/Required).
class DME_Tasks_TravelHandler : DME_Tasks_ObjectiveHandlerBase {
	//! uid|questId|objectiveId → bereits gezaehlte ZoneIds
	private ref map<string, ref array<string>> m_DME_VisitedZones;

	void DME_Tasks_TravelHandler() {
		m_DME_VisitedZones = new map<string, ref array<string>>();
	}

	override bool CanHandle(int objectiveType) {
		if (objectiveType == EDME_Tasks_ObjectiveType.TRAVEL) {
			return true;
		}
		if (objectiveType == EDME_Tasks_ObjectiveType.DISCOVER) {
			return true;
		}
		return false;
	}

	override void GetEventTypesFor(int objectiveType, array<int> outTypes) {
		outTypes.Insert(EDME_Tasks_EventType.ZONE_ENTER);
	}

	override void OnEvent(DME_Tasks_ObjectiveRef objectiveRef, DME_Tasks_Event evt) {
		if (!objectiveRef || !objectiveRef.Def) {
			return;
		}

		DME_Tasks_ZoneEvent zoneEvent = DME_Tasks_ZoneEvent.Cast(evt);
		if (!zoneEvent) {
			return;
		}
		if (!zoneEvent.m_DME_Entered) {
			return;
		}
		if (zoneEvent.m_DME_ZoneId == "") {
			return;
		}

		DME_Tasks_ObjectiveDef def = objectiveRef.Def;
		if (!IsInTimeWindow(def.FromHour, def.ToHour)) {
			return;
		}

		bool matched = false;
		if (def.RequiredZones && def.RequiredZones.Count() > 0) {
			matched = def.RequiredZones.Find(zoneEvent.m_DME_ZoneId) != -1;
		} else if (def.Zone) {
			string resolvedZoneId = DME_Tasks_SpatialUtil.ResolveZoneId(objectiveRef.QuestId, objectiveRef.ObjectiveId, def.Zone);
			matched = zoneEvent.m_DME_ZoneId == resolvedZoneId;
		}
		if (!matched) {
			return;
		}

		//! Jede Zone zaehlt pro Objective nur EINMAL (Wiederbetreten zaehlt nicht erneut)
		string stateKey = DME_Tasks_SpatialUtil.StateKey(objectiveRef);
		array<string> visited = m_DME_VisitedZones.Get(stateKey);
		if (!visited) {
			visited = new array<string>();
			m_DME_VisitedZones.Set(stateKey, visited);
		}
		if (visited.Find(zoneEvent.m_DME_ZoneId) != -1) {
			return;
		}
		visited.Insert(zoneEvent.m_DME_ZoneId);

		ReportProgress(objectiveRef, 1);
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

	private void RemoveByPrefix(string prefix) {
		for (int i = m_DME_VisitedZones.Count() - 1; i >= 0; i--) {
			string key = m_DME_VisitedZones.GetKey(i);
			if (key.IndexOf(prefix) == 0) {
				m_DME_VisitedZones.RemoveElement(i);
			}
		}
	}
}
