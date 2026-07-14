//! Handler-Registry (CONTRACTS §3.6 + §6.5) — Singleton; key = EDME_Tasks_ObjectiveType.
//! Alle Handler sind Singletons: RegisterAll erzeugt pro Handler-Klasse genau EINE Instanz,
//! die Registry haelt die einzige starke Referenz.
class DME_Tasks_ObjectiveRegistry {
	private static ref DME_Tasks_ObjectiveRegistry s_DME_Instance;

	private ref map<int, ref DME_Tasks_ObjectiveHandlerBase> m_DME_Handlers;
	private bool m_DME_Registered;

	static DME_Tasks_ObjectiveRegistry GetInstance() {
		if (!s_DME_Instance) {
			s_DME_Instance = new DME_Tasks_ObjectiveRegistry();
		}
		return s_DME_Instance;
	}

	void DME_Tasks_ObjectiveRegistry() {
		m_DME_Handlers = new map<int, ref DME_Tasks_ObjectiveHandlerBase>();
		m_DME_Registered = false;
	}

	//! Registriert einen Handler fuer alle uebergebenen EDME_Tasks_ObjectiveType-Werte.
	void Register(DME_Tasks_ObjectiveHandlerBase handler, array<int> objectiveTypes) {
		if (!handler || !objectiveTypes) {
			return;
		}

		foreach (int objectiveType : objectiveTypes) {
			DME_Tasks_ObjectiveHandlerBase existing = m_DME_Handlers.Get(objectiveType);
			if (existing && existing != handler) {
				DME_Tasks_Log.Warn("ObjectiveRegistry: Handler fuer Typ %1 wird ersetzt", DME_Tasks_EnumUtil.ObjectiveTypeToString(objectiveType));
			}
			m_DME_Handlers.Set(objectiveType, handler);
		}
	}

	//! null, wenn kein Handler fuer den ObjectiveType registriert ist.
	DME_Tasks_ObjectiveHandlerBase GetHandler(int objectiveType) {
		return m_DME_Handlers.Get(objectiveType);
	}

	//! Verdrahtet EXAKT die 11 Handler-Klassen aus CONTRACTS §6.5 (Agenten 9-12).
	//! Idempotent; wird vom DME_Tasks_ObjectivesModule in OnMissionStart gerufen (server-only).
	void RegisterAll() {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (m_DME_Registered) {
			return;
		}
		m_DME_Registered = true;

		array<int> killTypes = {EDME_Tasks_ObjectiveType.KILL, EDME_Tasks_ObjectiveType.BOSS};
		Register(new DME_Tasks_KillHandler(), killTypes);

		array<int> collectTypes = {EDME_Tasks_ObjectiveType.COLLECT};
		Register(new DME_Tasks_CollectHandler(), collectTypes);

		array<int> handoverTypes = {EDME_Tasks_ObjectiveType.HANDOVER, EDME_Tasks_ObjectiveType.DELIVER};
		Register(new DME_Tasks_HandoverHandler(), handoverTypes);

		array<int> travelTypes = {EDME_Tasks_ObjectiveType.TRAVEL, EDME_Tasks_ObjectiveType.DISCOVER};
		Register(new DME_Tasks_TravelHandler(), travelTypes);

		array<int> returnTypes = {EDME_Tasks_ObjectiveType.RETURN_TO_TRADER};
		Register(new DME_Tasks_ReturnHandler(), returnTypes);

		array<int> interactTypes = {EDME_Tasks_ObjectiveType.INTERACT, EDME_Tasks_ObjectiveType.MARK, EDME_Tasks_ObjectiveType.STASH, EDME_Tasks_ObjectiveType.USE_ITEM, EDME_Tasks_ObjectiveType.SIGNAL};
		Register(new DME_Tasks_InteractHandler(), interactTypes);

		array<int> surviveTypes = {EDME_Tasks_ObjectiveType.SURVIVE};
		Register(new DME_Tasks_SurviveHandler(), surviveTypes);

		array<int> craftTypes = {EDME_Tasks_ObjectiveType.CRAFT};
		Register(new DME_Tasks_CraftHandler(), craftTypes);

		array<int> escortTypes = {EDME_Tasks_ObjectiveType.ESCORT, EDME_Tasks_ObjectiveType.DEFEND};
		Register(new DME_Tasks_EscortDefendHandler(), escortTypes);

		array<int> groupTypes = {EDME_Tasks_ObjectiveType.GROUP};
		Register(new DME_Tasks_GroupHandler(), groupTypes);

		array<int> extractTypes = {EDME_Tasks_ObjectiveType.EXTRACT};
		Register(new DME_Tasks_ExtractHandler(), extractTypes);

		DME_Tasks_Log.Info("ObjectiveRegistry: %1 Objective-Typen verdrahtet", m_DME_Handlers.Count().ToString());
	}
}
