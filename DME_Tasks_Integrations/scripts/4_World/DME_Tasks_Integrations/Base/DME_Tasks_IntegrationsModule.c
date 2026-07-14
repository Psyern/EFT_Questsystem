//! CF-Modul des Integrations-PBO (SPEC §7 Agent 18 + CONTRACTS §6.8) — instanziiert bei
//! MissionStart (server-only) alle Adapter, prueft Erkennung (CfgPatches) + Settings-Toggle
//! und aktiviert bzw. loggt idle. Die Adapter-Instanzen werden hier gehalten (ref-Array),
//! damit ihre ScriptInvoker-Abos ueber die gesamte Mission gueltig bleiben.
//!
//! Reihenfolge: CF dispatcht Modul-Events in Registrierungsreihenfolge = Compile-Reihenfolge
//! der PBOs (Core → Objectives → Integrations, via requiredAddons). DME_Tasks_CoreModule hat
//! beim Eintreffen unseres OnMissionStart also bereits QuestEngine.OnServerInit() und damit
//! ConfigService.LoadAll() ausgefuehrt — die Settings-Toggles sind geladen.
[CF_RegisterModule(DME_Tasks_IntegrationsModule)]
class DME_Tasks_IntegrationsModule : CF_ModuleWorld {
	private ref array<ref DME_Tasks_IntegrationBase> m_DME_Adapters;

	void DME_Tasks_IntegrationsModule() {
		m_DME_Adapters = new array<ref DME_Tasks_IntegrationBase>();
	}

	override void OnInit() {
		super.OnInit();

		EnableMissionStart();
	}

	override void OnMissionStart(Class sender, CF_EventArgs args) {
		super.OnMissionStart(sender, args);

		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		ActivateAdapters();
	}

	// ------------------------------------------------------------------
	// intern
	// ------------------------------------------------------------------

	private void ActivateAdapters() {
		m_DME_Adapters.Clear();
		m_DME_Adapters.Insert(new DME_Tasks_MarketAdapter());
		m_DME_Adapters.Insert(new DME_Tasks_AIAdapter());
		m_DME_Adapters.Insert(new DME_Tasks_BossAdapter());
		m_DME_Adapters.Insert(new DME_Tasks_GroupsAdapter());
		m_DME_Adapters.Insert(new DME_Tasks_TerjeAdapter());
		m_DME_Adapters.Insert(new DME_Tasks_SeasonAdapter());
		m_DME_Adapters.Insert(new DME_Tasks_WebsiteAdapter());

		DME_Tasks_Settings settings = DME_Tasks_ConfigService.GetInstance().GetSettings();
		if (!settings) {
			DME_Tasks_Log.Warn("IntegrationsModule: keine Settings verfuegbar — alle Adapter bleiben idle");
			return;
		}

		int activeCount = 0;
		foreach (DME_Tasks_IntegrationBase adapter : m_DME_Adapters) {
			bool available = adapter.IsAvailable();
			bool enabled = adapter.IsEnabled(settings);

			if (available && enabled) {
				adapter.SetActive(true);
				adapter.OnActivate();
				DME_Tasks_Log.Info("Integration %1 AKTIV (%2, Toggle an)", adapter.GetName(), DescribeDetection(adapter));
				activeCount++;
			} else if (!enabled) {
				adapter.LogIdle("Settings-Toggle aus (" + DescribeDetection(adapter) + ")");
			} else {
				adapter.LogIdle("Zielmod nicht erkannt (CfgPatches " + adapter.GetAddonPatchName() + " fehlt)");
			}
		}

		string activeStr = activeCount.ToString();
		string totalStr = m_DME_Adapters.Count().ToString();
		DME_Tasks_Log.Info("IntegrationsModule: %1 von %2 Adaptern aktiv", activeStr, totalStr);
	}

	private string DescribeDetection(DME_Tasks_IntegrationBase adapter) {
		if (adapter.GetAddonPatchName() == "") {
			return "kein Fremd-Mod noetig";
		}
		if (adapter.IsAvailable()) {
			return "CfgPatches " + adapter.GetAddonPatchName() + " erkannt";
		}
		return "CfgPatches " + adapter.GetAddonPatchName() + " fehlt";
	}
}
