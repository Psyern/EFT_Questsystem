//! RETURN_TO_TRADER (CONTRACTS §6.5, Agent 11) — indexiert unter ZONE_ENTER.
//!
//! Zwei Betriebsarten laut Handler-Tabelle:
//! - Def.Zone GESETZT: der ZoneTriggerManager erzeugt den Trigger; das Betreten der Zone
//!   (MustBeAlive ist beim Enter trivially erfuellt — der Trigger dispatcht nur lebende Spieler)
//!   vervollstaendigt das Objective komplett (ReportProgress mit Required-Delta).
//! - Def.Zone NICHT gesetzt: es entsteht kein Trigger und hier kommt nie ein Event an —
//!   das Objective wird stattdessen im RequestTurnIn-Override auto-vervollstaendigt
//!   (Hooks/DME_Tasks_EngineTurnInHook.c).
class DME_Tasks_ReturnHandler : DME_Tasks_ObjectiveHandlerBase {
	override bool CanHandle(int objectiveType) {
		return objectiveType == EDME_Tasks_ObjectiveType.RETURN_TO_TRADER;
	}

	override void GetEventTypesFor(int objectiveType, array<int> outTypes) {
		outTypes.Insert(EDME_Tasks_EventType.ZONE_ENTER);
	}

	override void OnEvent(DME_Tasks_ObjectiveRef objectiveRef, DME_Tasks_Event evt) {
		if (!objectiveRef || !objectiveRef.Def) {
			return;
		}

		DME_Tasks_ObjectiveDef def = objectiveRef.Def;
		if (!def.Zone) {
			return;
		}

		DME_Tasks_ZoneEvent zoneEvent = DME_Tasks_ZoneEvent.Cast(evt);
		if (!zoneEvent) {
			return;
		}
		if (!zoneEvent.m_DME_Entered) {
			return;
		}

		string resolvedZoneId = DME_Tasks_SpatialUtil.ResolveZoneId(objectiveRef.QuestId, objectiveRef.ObjectiveId, def.Zone);
		if (zoneEvent.m_DME_ZoneId != resolvedZoneId) {
			return;
		}

		if (!IsInTimeWindow(def.FromHour, def.ToHour)) {
			return;
		}

		//! Rueckkehr ist binaer — Objective in einem Schritt vervollstaendigen (Engine klemmt auf Required)
		int required = def.Amount;
		if (required < 1) {
			required = 1;
		}
		ReportProgress(objectiveRef, required);
	}
}
