//! Season-Adapter (CONTRACTS §6.8) — analog zum Terje-Adapter: Abo s_DME_OnGrantSeasonXp,
//! Export als PendingGrant (Type "SEASON_XP") nach
//! $profile:DME_Tasks/Integrations/PendingGrants_<uid>.json (Models + Store definiert in
//! Terje\DME_Tasks_TerjeAdapter.c — gemeinsame Datei, Type-Feld unterscheidet).
//! Toggle: Settings.EnableIntegrationSeason.
//!
//! Erkennung: CfgPatches "DME_SeasonPass" — der DME Season Pass ist ein hauseigener Mod ohne
//! lokal verifizierbare Quelle; stimmt der CfgPatches-Name des tatsaechlichen Season-Mods nicht,
//! bleibt der Adapter idle (Log nennt den erwarteten Namen) — dann hier anpassen.
//!
//! Faehigkeits-Grenzen: Season-XP direkt gutschreiben braeuchte Season-Pass-Klassen-Referenzen —
//! verboten (PBO kompiliert ohne Fremd-Mods). Auszahlung uebernimmt eine server-spezifische
//! Bruecke, die die PendingGrants-Datei liest.
class DME_Tasks_SeasonAdapter : DME_Tasks_IntegrationBase {
	void DME_Tasks_SeasonAdapter() {
		m_DME_Name = "Season";
		m_DME_AddonPatchName = "DME_SeasonPass";
	}

	override bool IsEnabled(DME_Tasks_Settings settings) {
		return settings.EnableIntegrationSeason;
	}

	override void OnActivate() {
		DME_Tasks_IntegrationPaths.EnsureIntegrationsDir();

		DME_Tasks_IntegrationEvents.s_DME_OnGrantSeasonXp.Insert(OnGrantSeasonXp);

		DME_Tasks_Log.Info("SeasonAdapter: Export-Bruecke aktiv — Season-XP-Grants nach PendingGrants_<uid>.json (Auszahlung braucht server-spezifische Season-Bruecke)");
	}

	// ------------------------------------------------------------------
	// Invoker-Abos (nur bei aktivem Adapter registriert)
	// ------------------------------------------------------------------

	private void OnGrantSeasonXp(string uid, int xp) {
		DME_Tasks_Log.Info("SeasonAdapter: Season-XP-Grant %1 fuer %2 — als PendingGrant exportiert", xp.ToString(), uid);
		DME_Tasks_PendingGrantsStore.Append(uid, "SEASON_XP", xp);
	}
}
