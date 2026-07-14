//! Market-Adapter (CONTRACTS §6.8) — Erkennung: CfgPatches "DayZExpansion_Market";
//! Toggle: Settings.EnableIntegrationMarket.
//!
//! Faehigkeits-Grenzen (ehrlich dokumentiert, siehe auch README):
//! - Ein DIREKTER Zugriff auf den Expansion-Market (per-Spieler-Sortiment freischalten,
//!   Currency auszahlen) ist ohne Hard-Referenz auf Expansion-Klassen nicht moeglich —
//!   und Hard-Refs sind fuer dieses PBO verboten (muss ohne Fremd-Mods kompilieren).
//! - Dieser Adapter ist deshalb eine GENERISCHE EXPORT-BRUECKE: er schreibt je Spieler
//!   $profile:DME_Tasks/Integrations/MarketUnlocks/<uid>.json mit der aktuellen Liste der
//!   freigeschalteten Market-Items (Quelle: DME_Tasks_UnlockLedger). Eine server-spezifische
//!   Companion-Logik (z. B. kleines Bridge-PBO mit Expansion-requiredAddons) kann die Datei
//!   lesen und im Market anwenden.
//! - s_DME_OnGrantCurrency wird abonniert und NUR geloggt — die Auszahlung braucht dieselbe
//!   server-spezifische Bruecke.

//! Export-Model (JSON-Member OHNE Prefix — Keys = Datei).
class DME_Tasks_MarketUnlockExport {
	int Version;
	string Uid;
	ref array<string> MarketItems;

	void DME_Tasks_MarketUnlockExport() {
		Version = 1;
		Uid = "";
		MarketItems = new array<string>();
	}
}

class DME_Tasks_MarketAdapter : DME_Tasks_IntegrationBase {
	void DME_Tasks_MarketAdapter() {
		m_DME_Name = "Market";
		m_DME_AddonPatchName = "DayZExpansion_Market";
	}

	override bool IsEnabled(DME_Tasks_Settings settings) {
		return settings.EnableIntegrationMarket;
	}

	override void OnActivate() {
		DME_Tasks_IntegrationPaths.EnsureMarketUnlocksDir();

		DME_Tasks_IntegrationEvents.s_DME_OnUnlocksChanged.Insert(OnUnlocksChanged);
		DME_Tasks_IntegrationEvents.s_DME_OnQuestCompleted.Insert(OnQuestCompleted);
		DME_Tasks_IntegrationEvents.s_DME_OnGrantCurrency.Insert(OnGrantCurrency);

		DME_Tasks_Log.Info("MarketAdapter: Export-Bruecke aktiv — MarketUnlocks/<uid>.json; Currency-Grants werden nur geloggt (server-spezifische Bruecke noetig)");
	}

	// ------------------------------------------------------------------
	// Invoker-Abos (nur bei aktivem Adapter registriert)
	// ------------------------------------------------------------------

	private void OnUnlocksChanged(string uid) {
		ExportUnlocks(uid);
	}

	private void OnQuestCompleted(string uid, string questId) {
		ExportUnlocks(uid);
	}

	//! Currency-Rewards laufen laut CONTRACTS §6.2 NUR ueber diesen Invoker. Ohne Market-Bruecke
	//! koennen wir nichts auszahlen — Log haelt die Betraege fuer Server-Admins nachvollziehbar.
	private void OnGrantCurrency(string uid, int amount) {
		DME_Tasks_Log.Info("MarketAdapter: Currency-Grant %1 fuer %2 — keine Auszahlung ohne server-spezifische Market-Bruecke", amount.ToString(), uid);
	}

	// ------------------------------------------------------------------
	// Export
	// ------------------------------------------------------------------

	private void ExportUnlocks(string uid) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (uid == "") {
			return;
		}

		DME_Tasks_MarketUnlockExport data = new DME_Tasks_MarketUnlockExport();
		data.Uid = uid;
		data.MarketItems = DME_Tasks_UnlockLedger.GetInstance().GetUnlockedMarketItems(uid);

		DME_Tasks_IntegrationPaths.EnsureMarketUnlocksDir();
		string path = DME_Tasks_IntegrationPaths.MarketUnlocksFile(uid);
		string saveError;
		bool savedOk = JsonFileLoader<DME_Tasks_MarketUnlockExport>.SaveFile(path, data, saveError);
		if (!savedOk) {
			DME_Tasks_Log.Warn("MarketAdapter: Export fehlgeschlagen (%1): %2", path, saveError);
		}
	}
}
