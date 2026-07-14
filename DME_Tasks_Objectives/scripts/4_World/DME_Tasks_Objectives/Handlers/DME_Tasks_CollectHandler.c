//! COLLECT-Handler (CONTRACTS §6.5, Agent 10) — Besitz-Ziel.
//! Bei jedem ITEM_ACQUIRED/ITEM_LOST des Spielers wird das Inventar neu gezaehlt
//! (EnumerateInventory LEVELORDER via DME_Tasks_OriginService.CollectMatchingItems: ClassNames
//! exakt/IsKindOf + Origin-Regeln nur wenn definiert) und der Fortschritt ABSOLUT via
//! SetObjectiveProgress gesetzt (Engine klemmt auf [0..Required], setzt/entzieht Done und
//! handhabt READY_TO_TURN_IN inkl. Ruecknahme, wenn Items wieder abgelegt werden).
//!
//! Referenz-Konsistenz: verweist ein HANDOVER/DELIVER-Objective derselben Quest via
//! ReferencesObjective auf dieses COLLECT-Objective, zaehlen bereits abgegebene Items
//! (Progress.Current des Geschwister-Objectives) weiterhin als "gesammelt" — sonst wuerde
//! die Abgabe (ObjectDelete -> ITEM_LOST -> Neuzaehlung) das Sammel-Ziel wieder entwerten.
class DME_Tasks_CollectHandler : DME_Tasks_ObjectiveHandlerBase {
	override bool CanHandle(int objectiveType) {
		return objectiveType == EDME_Tasks_ObjectiveType.COLLECT;
	}

	override void GetEventTypesFor(int objectiveType, array<int> outTypes) {
		outTypes.Insert(EDME_Tasks_EventType.ITEM_ACQUIRED);
		outTypes.Insert(EDME_Tasks_EventType.ITEM_LOST);
	}

	override void OnEvent(DME_Tasks_ObjectiveRef objectiveRef, DME_Tasks_Event evt) {
		if (!objectiveRef || !objectiveRef.Def) {
			return;
		}

		DME_Tasks_ItemEvent itemEvent = DME_Tasks_ItemEvent.Cast(evt);
		if (!itemEvent) {
			return;
		}

		//! Cheap-Filter: nur neu zaehlen, wenn der bewegte Item-Typ das Objective ueberhaupt betrifft.
		//! Leerer ItemType = expliziter Recount-Request (z. B. Initial-Zaehlung bei Quest-Aktivierung
		//! oder Origin-Umstempelung) → Filter ueberspringen und immer neu zaehlen.
		if (itemEvent.m_DME_ItemType != "" && !MatchesClassNames(itemEvent.m_DME_ItemType, objectiveRef.Def.ClassNames)) {
			return;
		}

		DME_Tasks_QuestEngine engine = DME_Tasks_QuestEngine.GetInstance();

		PlayerBase player = engine.FindPlayerByUid(objectiveRef.PlayerUid);
		if (!player) {
			return;
		}

		DME_Tasks_ActiveQuest active = engine.GetActiveQuest(objectiveRef.PlayerUid, objectiveRef.QuestId);
		int acceptedAt = 0;
		if (active) {
			acceptedAt = active.AcceptedAt;
		}

		array<ItemBase> matched = new array<ItemBase>();
		DME_Tasks_OriginService.CollectMatchingItems(player, objectiveRef.Def, objectiveRef.PlayerUid, acceptedAt, matched);

		int count = matched.Count();
		count = count + GetHandedOverCount(engine, objectiveRef, active);

		engine.SetObjectiveProgress(objectiveRef.PlayerUid, objectiveRef.QuestId, objectiveRef.ObjectiveId, count);
	}

	// ==================================================================
	// intern
	// ==================================================================

	//! Summe der bereits abgegebenen Items aller HANDOVER/DELIVER-Objectives derselben Quest,
	//! die via ReferencesObjective auf dieses COLLECT-Objective zeigen.
	private int GetHandedOverCount(DME_Tasks_QuestEngine engine, DME_Tasks_ObjectiveRef objectiveRef, DME_Tasks_ActiveQuest active) {
		if (!active) {
			return 0;
		}

		DME_Tasks_QuestDef questDef = engine.GetQuestDef(objectiveRef.QuestId);
		if (!questDef) {
			return 0;
		}

		int total = 0;
		foreach (DME_Tasks_ObjectiveDef siblingDef : questDef.Objectives) {
			if (!siblingDef) {
				continue;
			}
			if (siblingDef.ReferencesObjective != objectiveRef.ObjectiveId) {
				continue;
			}

			int siblingType = DME_Tasks_EnumUtil.ObjectiveTypeFromString(siblingDef.Type);
			if (siblingType != EDME_Tasks_ObjectiveType.HANDOVER && siblingType != EDME_Tasks_ObjectiveType.DELIVER) {
				continue;
			}

			foreach (DME_Tasks_ObjectiveProgress siblingProgress : active.Objectives) {
				if (siblingProgress && siblingProgress.ObjectiveId == siblingDef.ObjectiveId) {
					total = total + siblingProgress.Current;
				}
			}
		}
		return total;
	}
}
