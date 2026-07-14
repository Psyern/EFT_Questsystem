//! Active-Objective-Index (SPEC §5.5 + CONTRACTS §6.5) — Performance-Kern, Singleton, server-only.
//! Datenstruktur: playerUid → (EDME_Tasks_EventType → ObjectiveRefs). Ein Objective kann unter
//! MEHREREN EventTypes indexiert sein (dieselbe DME_Tasks_ObjectiveRef-Instanz in mehreren Buckets).
//! Befuellt/geleert ausschliesslich ueber die DME_Tasks_EngineEvents-Abos des DME_Tasks_ObjectivesModule.
class DME_Tasks_ObjectiveIndex {
	private static ref DME_Tasks_ObjectiveIndex s_DME_Instance;

	private ref map<string, ref map<int, ref array<ref DME_Tasks_ObjectiveRef>>> m_DME_Index;

	static DME_Tasks_ObjectiveIndex GetInstance() {
		if (!s_DME_Instance) {
			s_DME_Instance = new DME_Tasks_ObjectiveIndex();
		}
		return s_DME_Instance;
	}

	void DME_Tasks_ObjectiveIndex() {
		m_DME_Index = new map<string, ref map<int, ref array<ref DME_Tasks_ObjectiveRef>>>();
	}

	//! Sortiert alle NICHT erledigten Objectives der aktiven Quest ein. Idempotent (RemoveQuest zuerst).
	//! EventTypes je Objective liefert der zustaendige Handler (Registry → GetEventTypesFor).
	void AddQuest(string uid, string questId) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (uid == "" || questId == "") {
			return;
		}

		RemoveQuest(uid, questId);

		DME_Tasks_QuestDef def = DME_Tasks_ConfigService.GetInstance().GetQuest(questId);
		if (!def) {
			DME_Tasks_Log.Warn("ObjectiveIndex.AddQuest: QuestDef %1 unbekannt", questId);
			return;
		}

		DME_Tasks_ActiveQuest active = DME_Tasks_QuestEngine.GetInstance().GetActiveQuest(uid, questId);
		if (!active) {
			DME_Tasks_Log.Warn("ObjectiveIndex.AddQuest: Quest %1 fuer %2 nicht aktiv", questId, uid);
			return;
		}

		DME_Tasks_ObjectiveRegistry registry = DME_Tasks_ObjectiveRegistry.GetInstance();
		array<int> eventTypes = new array<int>();

		foreach (DME_Tasks_ObjectiveDef objectiveDef : def.Objectives) {
			if (!objectiveDef) {
				continue;
			}
			if (IsObjectiveDone(active, objectiveDef.ObjectiveId)) {
				continue;
			}

			int objectiveType = DME_Tasks_EnumUtil.ObjectiveTypeFromString(objectiveDef.Type);
			if (objectiveType == -1) {
				continue;
			}

			DME_Tasks_ObjectiveHandlerBase handler = registry.GetHandler(objectiveType);
			if (!handler) {
				DME_Tasks_Log.Warn("ObjectiveIndex.AddQuest: kein Handler fuer Typ %1 (%2)", objectiveDef.Type, questId);
				continue;
			}

			eventTypes.Clear();
			handler.GetEventTypesFor(objectiveType, eventTypes);
			if (eventTypes.Count() == 0) {
				continue;
			}

			DME_Tasks_ObjectiveRef objectiveRef = new DME_Tasks_ObjectiveRef(uid, questId, objectiveDef.ObjectiveId, objectiveDef);
			foreach (int eventType : eventTypes) {
				InsertRef(uid, eventType, objectiveRef);
			}
		}
	}

	//! Entfernt alle Refs der Quest aus allen EventType-Buckets des Spielers.
	void RemoveQuest(string uid, string questId) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		map<int, ref array<ref DME_Tasks_ObjectiveRef>> byEvent = m_DME_Index.Get(uid);
		if (!byEvent) {
			return;
		}

		for (int mapIndex = byEvent.Count() - 1; mapIndex >= 0; mapIndex--) {
			array<ref DME_Tasks_ObjectiveRef> refs = byEvent.GetElement(mapIndex);
			if (!refs) {
				byEvent.RemoveElement(mapIndex);
				continue;
			}

			for (int i = refs.Count() - 1; i >= 0; i--) {
				DME_Tasks_ObjectiveRef objectiveRef = refs.Get(i);
				if (objectiveRef && objectiveRef.QuestId == questId) {
					refs.RemoveOrdered(i);
				}
			}

			if (refs.Count() == 0) {
				byEvent.RemoveElement(mapIndex);
			}
		}

		if (byEvent.Count() == 0) {
			m_DME_Index.Remove(uid);
		}
	}

	//! Kompletter Index-Drop eines Spielers (Disconnect).
	void RemovePlayer(string uid) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		m_DME_Index.Remove(uid);
	}

	//! Nie null — leeres Array. Liefert einen SNAPSHOT (frische Kopie), damit Handler waehrend der
	//! Dispatch-Iteration gefahrlos Index-Mutationen ausloesen duerfen (z. B. FailQuest → RemoveQuest).
	array<ref DME_Tasks_ObjectiveRef> GetRefs(string uid, int eventType) {
		array<ref DME_Tasks_ObjectiveRef> result = new array<ref DME_Tasks_ObjectiveRef>();

		map<int, ref array<ref DME_Tasks_ObjectiveRef>> byEvent = m_DME_Index.Get(uid);
		if (!byEvent) {
			return result;
		}

		array<ref DME_Tasks_ObjectiveRef> refs = byEvent.Get(eventType);
		if (!refs) {
			return result;
		}

		foreach (DME_Tasks_ObjectiveRef objectiveRef : refs) {
			result.Insert(objectiveRef);
		}
		return result;
	}

	//! Allokationsfreier Cheap-Guard (z. B. Item-Hook, Timer-Tick): Anzahl Refs fuer (uid, eventType).
	int CountRefs(string uid, int eventType) {
		map<int, ref array<ref DME_Tasks_ObjectiveRef>> byEvent = m_DME_Index.Get(uid);
		if (!byEvent) {
			return 0;
		}

		array<ref DME_Tasks_ObjectiveRef> refs = byEvent.Get(eventType);
		if (!refs) {
			return 0;
		}
		return refs.Count();
	}

	// ==================================================================
	// intern
	// ==================================================================

	private void InsertRef(string uid, int eventType, DME_Tasks_ObjectiveRef objectiveRef) {
		map<int, ref array<ref DME_Tasks_ObjectiveRef>> byEvent = m_DME_Index.Get(uid);
		if (!byEvent) {
			byEvent = new map<int, ref array<ref DME_Tasks_ObjectiveRef>>();
			m_DME_Index.Set(uid, byEvent);
		}

		array<ref DME_Tasks_ObjectiveRef> refs = byEvent.Get(eventType);
		if (!refs) {
			refs = new array<ref DME_Tasks_ObjectiveRef>();
			byEvent.Set(eventType, refs);
		}

		refs.Insert(objectiveRef);
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
