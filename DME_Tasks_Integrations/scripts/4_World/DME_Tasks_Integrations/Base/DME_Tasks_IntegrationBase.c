//! Basisklasse aller Integrationsadapter (SPEC §7 Agent 18 + CONTRACTS §6.8).
//!
//! Verbindliche Adapter-Regeln:
//! - KEINE Referenz auf externe Mod-Klassen (eAI*/Expansion*/Terje*/... nur als String-Literale).
//! - Erkennung des Zielmods NUR via g_Game.ConfigIsExisting("CfgPatches <AddonName>")
//!   (verifiziert scripts - 1.29, Nutzung z. B. ModStructure.c:22).
//! - Aktiv NUR wenn IsAvailable() UND IsEnabled(settings) beide true sind (Aktivierung durch
//!   DME_Tasks_IntegrationsModule, server-only); sonst einmaliges Idle-Info-Log, KEINE Aktivitaet.
//! - Das Integrations-PBO kompiliert und laeuft OHNE installierte Fremd-Mods.
class DME_Tasks_IntegrationBase {
	protected string m_DME_Name;
	protected string m_DME_AddonPatchName;
	protected bool m_DME_Active;

	void DME_Tasks_IntegrationBase() {
		m_DME_Name = "";
		m_DME_AddonPatchName = "";
		m_DME_Active = false;
	}

	string GetName() {
		return m_DME_Name;
	}

	string GetAddonPatchName() {
		return m_DME_AddonPatchName;
	}

	bool IsActive() {
		return m_DME_Active;
	}

	void SetActive(bool active) {
		m_DME_Active = active;
	}

	//! Laufzeit-Erkennung des Zielmods. Leerer AddonPatchName = Adapter braucht keinen Fremd-Mod
	//! (Boss/Website) und ist immer verfuegbar — dann entscheidet allein der Settings-Toggle.
	bool IsAvailable() {
		if (m_DME_AddonPatchName == "") {
			return true;
		}
		if (!g_Game) {
			return false;
		}
		return g_Game.ConfigIsExisting("CfgPatches " + m_DME_AddonPatchName);
	}

	//! Settings-Toggle (EnableIntegration*) — jede Subklasse liest ihr Feld. Seam-Methode
	//! (wie DME_Tasks_ObjectiveHandlerBase): Overrides ersetzen die Basis komplett, kein super noetig.
	bool IsEnabled(DME_Tasks_Settings settings) {
		return false;
	}

	//! Aktivierung (Invoker-Abos, eigene Config laden, Export-Verzeichnisse anlegen) — wird vom
	//! IntegrationsModule NUR gerufen, wenn IsAvailable() und IsEnabled() beide true sind.
	void OnActivate() {
	}

	//! Einmaliges Idle-Log (Adapter bleibt safe no-op).
	void LogIdle(string reason) {
		DME_Tasks_Log.Info("Integration %1 idle — %2", m_DME_Name, reason);
	}
}

//! Pfad-Helfer des Integrations-PBO. Die Integrations-Unterordner sind NICHT Teil von
//! DME_Tasks_Paths.EnsureDirectories (Core kennt Integrations nicht) — die Adapter legen ihre
//! Verzeichnisse hier selbst an (MakeDirectory, server-guarded).
class DME_Tasks_IntegrationPaths {
	static string IntegrationsDir() {
		return DME_Tasks_Paths.ProfileRoot() + "/Integrations";
	}

	static string MarketUnlocksDir() {
		return IntegrationsDir() + "/MarketUnlocks";
	}

	static string MarketUnlocksFile(string uid) {
		return MarketUnlocksDir() + "/" + uid + ".json";
	}

	static string BossesFile() {
		return IntegrationsDir() + "/Bosses.json";
	}

	static string AIClassesFile() {
		return IntegrationsDir() + "/AIClasses.json";
	}

	static string PendingGrantsFile(string uid) {
		return IntegrationsDir() + "/PendingGrants_" + uid + ".json";
	}

	static string WebsiteDir() {
		return DME_Tasks_Paths.ProfileRoot() + "/Website";
	}

	static string WebsiteFile(string uid) {
		return WebsiteDir() + "/" + uid + ".json";
	}

	static void EnsureIntegrationsDir() {
		EnsureDirectory(IntegrationsDir());
	}

	static void EnsureMarketUnlocksDir() {
		EnsureDirectory(IntegrationsDir());
		EnsureDirectory(MarketUnlocksDir());
	}

	static void EnsureWebsiteDir() {
		EnsureDirectory(WebsiteDir());
	}

	private static void EnsureDirectory(string path) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (FileExist(path)) {
			return;
		}

		bool created = MakeDirectory(path);
		if (!created) {
			DME_Tasks_Log.Warn("Integrations: Verzeichnis nicht anlegbar: %1", path);
		}
	}
}
