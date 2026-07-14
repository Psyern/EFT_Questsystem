//! SURVIVE (CONTRACTS §6.5, Agent 11) — indexiert unter TIMER_TICK und PLAYER_DIED.
//!
//! Amount = zu ueberlebende SEKUNDEN. Das DME_Tasks_ObjectivesModule dispatcht 1-Hz-TimerEvents
//! NUR fuer Spieler mit TIMER_TICK-Refs — jeder Tick meldet +1 via ReportProgress (die Engine
//! klemmt auf Required und setzt Done). KEIN Frame-Loop, kein eigener Zustand im Handler:
//! der Zaehlerstand lebt ausschliesslich in DME_Tasks_ObjectiveProgress (persistiert Reconnects).
//! PLAYER_DIED (Death-Hook der Welle 2) → Fortschritts-Reset via SetObjectiveProgress(0).
class DME_Tasks_SurviveHandler : DME_Tasks_ObjectiveHandlerBase {
	override bool CanHandle(int objectiveType) {
		return objectiveType == EDME_Tasks_ObjectiveType.SURVIVE;
	}

	override void GetEventTypesFor(int objectiveType, array<int> outTypes) {
		outTypes.Insert(EDME_Tasks_EventType.TIMER_TICK);
		outTypes.Insert(EDME_Tasks_EventType.PLAYER_DIED);
	}

	override void OnEvent(DME_Tasks_ObjectiveRef objectiveRef, DME_Tasks_Event evt) {
		if (!objectiveRef || !objectiveRef.Def) {
			return;
		}
		if (!evt) {
			return;
		}

		if (evt.m_DME_EventType == EDME_Tasks_EventType.PLAYER_DIED) {
			//! Tod setzt die Ueberlebenszeit zurueck (einziger erlaubter Nicht-ReportProgress-Pfad)
			DME_Tasks_QuestEngine.GetInstance().SetObjectiveProgress(objectiveRef.PlayerUid, objectiveRef.QuestId, objectiveRef.ObjectiveId, 0);
			return;
		}

		if (evt.m_DME_EventType != EDME_Tasks_EventType.TIMER_TICK) {
			return;
		}

		DME_Tasks_TimerEvent timerEvent = DME_Tasks_TimerEvent.Cast(evt);
		if (!timerEvent) {
			return;
		}

		//! 1 Tick = 1 Sekunde ueberlebt
		ReportProgress(objectiveRef, 1);
	}
}
