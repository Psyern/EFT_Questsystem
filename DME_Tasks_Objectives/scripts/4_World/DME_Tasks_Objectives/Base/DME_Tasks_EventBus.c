//! Zentraler Event-Bus (SPEC §5.5 + CONTRACTS §6.5) — Singleton, ausschliesslich Dedicated Server.
//! Dispatch holt via ObjectiveIndex NUR die Refs des betroffenen Spielers fuer den EventType des
//! Events (kein globaler Scan, kein Polling) und ruft je Ref den zustaendigen Handler.
class DME_Tasks_EventBus {
	private static ref DME_Tasks_EventBus s_DME_Instance;

	static DME_Tasks_EventBus GetInstance() {
		if (!s_DME_Instance) {
			s_DME_Instance = new DME_Tasks_EventBus();
		}
		return s_DME_Instance;
	}

	//! evt.m_DME_PlayerUid und evt.m_DME_EventType MUESSEN vom Erzeuger gesetzt sein
	//! (EventType setzen die Event-Ctors bzw. SetEntered/SetAcquired).
	void Dispatch(DME_Tasks_Event evt) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!evt) {
			return;
		}
		if (evt.m_DME_PlayerUid == "" || evt.m_DME_EventType == -1) {
			return;
		}

		//! Snapshot — Handler duerfen waehrend der Iteration Index-Mutationen ausloesen (FailQuest etc.)
		array<ref DME_Tasks_ObjectiveRef> refs = DME_Tasks_ObjectiveIndex.GetInstance().GetRefs(evt.m_DME_PlayerUid, evt.m_DME_EventType);
		if (refs.Count() == 0) {
			return;
		}

		DME_Tasks_ObjectiveRegistry registry = DME_Tasks_ObjectiveRegistry.GetInstance();
		foreach (DME_Tasks_ObjectiveRef objectiveRef : refs) {
			if (!objectiveRef || !objectiveRef.Def) {
				continue;
			}

			int objectiveType = DME_Tasks_EnumUtil.ObjectiveTypeFromString(objectiveRef.Def.Type);
			if (objectiveType == -1) {
				continue;
			}

			DME_Tasks_ObjectiveHandlerBase handler = registry.GetHandler(objectiveType);
			if (!handler) {
				continue;
			}

			handler.OnEvent(objectiveRef, evt);
		}
	}
}
