//! CRAFT (CONTRACTS §6.5 — Handler-Tabelle: Agent 11) — indexiert unter CRAFT.
//!
//! CraftEvents erzeugt der Craft-Hook (Hooks/DME_Tasks_CraftHook.c, modded RecipeBase.SpawnItems):
//! ein Event pro gespawntem Ergebnis-Item (m_DME_ResultType = Klassenname). Filter:
//! - ClassNames gegen den Ergebnis-Typ (exakt oder Config-Vererbung).
//! - Zone falls gesetzt (Event-Position = Spielerposition beim Craften).
//! - FromHour/ToHour In-Game-Zeitfenster.
class DME_Tasks_CraftHandler : DME_Tasks_ObjectiveHandlerBase {
	override bool CanHandle(int objectiveType) {
		return objectiveType == EDME_Tasks_ObjectiveType.CRAFT;
	}

	override void GetEventTypesFor(int objectiveType, array<int> outTypes) {
		outTypes.Insert(EDME_Tasks_EventType.CRAFT);
	}

	override void OnEvent(DME_Tasks_ObjectiveRef objectiveRef, DME_Tasks_Event evt) {
		if (!objectiveRef || !objectiveRef.Def) {
			return;
		}

		DME_Tasks_CraftEvent craftEvent = DME_Tasks_CraftEvent.Cast(evt);
		if (!craftEvent) {
			return;
		}

		DME_Tasks_ObjectiveDef def = objectiveRef.Def;
		if (!IsInTimeWindow(def.FromHour, def.ToHour)) {
			return;
		}

		if (!MatchesClassNames(craftEvent.m_DME_ResultType, def.ClassNames)) {
			return;
		}

		if (def.Zone) {
			if (!DME_Tasks_SpatialUtil.IsPositionInZone(craftEvent.m_DME_Position, def.Zone)) {
				return;
			}
		}

		ReportProgress(objectiveRef, 1);
	}
}
