//! INTERACT / MARK / STASH / USE_ITEM / SIGNAL (CONTRACTS §6.5, Agent 11) —
//! indexiert unter ACTION_USED UND INTERACT (der Action-Hook waehlt den EventType:
//! mit Ziel-Objekt INTERACT, ohne ACTION_USED — ein Event traegt genau EINEN Typ,
//! daher kein Doppel-Zaehlen trotz doppelter Indexierung).
//!
//! Filter aus DME_Tasks_ObjectiveDef:
//! - ClassNames: gegen das Item in der Hand (fuer SIGNAL zusaetzlich gegen den Target-Typ —
//!   Signal-Items koennen in der Hand gehalten ODER als platziertes Ziel benutzt werden).
//! - TargetCategory: gegen den Target-Typ (exakt oder Config-Vererbung via IsKindOf).
//! - Zone: falls gesetzt, muss die Event-Position im Zonen-Zylinder liegen.
//! - FromHour/ToHour: In-Game-Zeitfenster.
class DME_Tasks_InteractHandler : DME_Tasks_ObjectiveHandlerBase {
	override bool CanHandle(int objectiveType) {
		if (objectiveType == EDME_Tasks_ObjectiveType.INTERACT) {
			return true;
		}
		if (objectiveType == EDME_Tasks_ObjectiveType.MARK) {
			return true;
		}
		if (objectiveType == EDME_Tasks_ObjectiveType.STASH) {
			return true;
		}
		if (objectiveType == EDME_Tasks_ObjectiveType.USE_ITEM) {
			return true;
		}
		if (objectiveType == EDME_Tasks_ObjectiveType.SIGNAL) {
			return true;
		}
		return false;
	}

	override void GetEventTypesFor(int objectiveType, array<int> outTypes) {
		outTypes.Insert(EDME_Tasks_EventType.ACTION_USED);
		outTypes.Insert(EDME_Tasks_EventType.INTERACT);
	}

	override void OnEvent(DME_Tasks_ObjectiveRef objectiveRef, DME_Tasks_Event evt) {
		if (!objectiveRef || !objectiveRef.Def) {
			return;
		}

		DME_Tasks_ActionEvent actionEvent = DME_Tasks_ActionEvent.Cast(evt);
		if (!actionEvent) {
			return;
		}

		DME_Tasks_ObjectiveDef def = objectiveRef.Def;
		if (!IsInTimeWindow(def.FromHour, def.ToHour)) {
			return;
		}

		int objectiveType = DME_Tasks_EnumUtil.ObjectiveTypeFromString(def.Type);

		//! ClassNames-Filter: Item in der Hand; SIGNAL matcht zusaetzlich Signal-Item als Ziel
		bool itemMatched = MatchesClassNames(actionEvent.m_DME_ItemInHandsType, def.ClassNames);
		if (!itemMatched && objectiveType == EDME_Tasks_ObjectiveType.SIGNAL) {
			itemMatched = MatchesClassNames(actionEvent.m_DME_TargetType, def.ClassNames);
		}
		if (!itemMatched) {
			return;
		}

		if (!MatchesTargetCategory(actionEvent.m_DME_TargetType, def.TargetCategory)) {
			return;
		}

		if (def.Zone) {
			if (!DME_Tasks_SpatialUtil.IsPositionInZone(actionEvent.m_DME_Position, def.Zone)) {
				return;
			}
		}

		ReportProgress(objectiveRef, 1);
	}

	//! Leere TargetCategory = kein Filter; sonst exakter Match oder Config-Vererbung (Game.c:1412).
	private bool MatchesTargetCategory(string targetType, string targetCategory) {
		if (targetCategory == "") {
			return true;
		}
		if (targetType == "") {
			return false;
		}
		if (targetType == targetCategory) {
			return true;
		}
		if (g_Game && g_Game.IsKindOf(targetType, targetCategory)) {
			return true;
		}
		return false;
	}
}
