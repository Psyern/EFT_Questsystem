//! Terje-Adapter (CONTRACTS §6.8) — Erkennung: CfgPatches "TerjeCore" (verifiziert lokal:
//! TerjeMods-master-main/TerjeCore/config.cpp); Toggle: Settings.EnableIntegrationTerje.
//!
//! Faehigkeits-Grenzen (ehrlich dokumentiert): Skillpunkte DIREKT gutschreiben braeuchte
//! Terje-Klassen-Referenzen — verboten (PBO muss ohne Fremd-Mods kompilieren). Der Adapter
//! abonniert deshalb s_DME_OnGrantSkillPoints und exportiert die Grants nach
//! $profile:DME_Tasks/Integrations/PendingGrants_<uid>.json (letzte N Grants, Read-Modify-Write).
//! Eine server-spezifische Bruecke (Companion-PBO mit Terje-requiredAddons) liest die Datei
//! und zahlt aus.
//!
//! HINWEIS: Die PendingGrants-Models + der Store hier werden AUCH vom Season-Adapter genutzt
//! (gemeinsame Datei je Spieler, Type-Feld unterscheidet "SKILL_POINTS"/"SEASON_XP") —
//! Klassen sind global, daher genau EINE Definition (hier).

//! Export-Models (JSON-Member OHNE Prefix — Keys = Datei).
class DME_Tasks_PendingGrant {
	string Type;
	int Amount;
	int GrantedAt;

	void DME_Tasks_PendingGrant() {
		Type = "";
		Amount = 0;
		GrantedAt = 0;
	}
}

class DME_Tasks_PendingGrantsExport {
	int Version;
	string Uid;
	ref array<ref DME_Tasks_PendingGrant> Grants;

	void DME_Tasks_PendingGrantsExport() {
		Version = 1;
		Uid = "";
		Grants = new array<ref DME_Tasks_PendingGrant>();
	}
}

//! Append-artiger, idempotent gehaltener Grant-Export: Datei je Spieler, gedeckelt auf die
//! letzten MAX_GRANTS Eintraege (CONTRACTS §6.8 "einfach: letzte N Grants"). Single-threaded
//! Script-VM → Read-Modify-Write je Grant ist race-frei, auch wenn Terje- UND Season-Adapter
//! in dieselbe Datei schreiben.
class DME_Tasks_PendingGrantsStore {
	private static const int MAX_GRANTS = 50;

	static void Append(string uid, string grantType, int amount) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (uid == "" || grantType == "" || amount == 0) {
			return;
		}

		DME_Tasks_IntegrationPaths.EnsureIntegrationsDir();
		string path = DME_Tasks_IntegrationPaths.PendingGrantsFile(uid);

		DME_Tasks_PendingGrantsExport data = new DME_Tasks_PendingGrantsExport();
		if (FileExist(path)) {
			string loadError;
			DME_Tasks_PendingGrantsExport loaded = new DME_Tasks_PendingGrantsExport();
			bool loadedOk = JsonFileLoader<DME_Tasks_PendingGrantsExport>.LoadFile(path, loaded, loadError);
			if (loadedOk && loaded) {
				data = loaded;
			} else {
				DME_Tasks_Log.Warn("PendingGrants: %1 unlesbar — Datei wird neu aufgebaut: %2", path, loadError);
			}
		}
		data.Uid = uid;

		DME_Tasks_PendingGrant grant = new DME_Tasks_PendingGrant();
		grant.Type = grantType;
		grant.Amount = amount;
		grant.GrantedAt = DME_Tasks_TimeUtil.NowEpoch();
		data.Grants.Insert(grant);

		while (data.Grants.Count() > MAX_GRANTS) {
			data.Grants.RemoveOrdered(0);
		}

		string saveError;
		bool savedOk = JsonFileLoader<DME_Tasks_PendingGrantsExport>.SaveFile(path, data, saveError);
		if (!savedOk) {
			DME_Tasks_Log.Warn("PendingGrants: Export fehlgeschlagen (%1): %2", path, saveError);
		}
	}
}

class DME_Tasks_TerjeAdapter : DME_Tasks_IntegrationBase {
	void DME_Tasks_TerjeAdapter() {
		m_DME_Name = "Terje";
		m_DME_AddonPatchName = "TerjeCore";
	}

	override bool IsEnabled(DME_Tasks_Settings settings) {
		return settings.EnableIntegrationTerje;
	}

	override void OnActivate() {
		DME_Tasks_IntegrationPaths.EnsureIntegrationsDir();

		DME_Tasks_IntegrationEvents.s_DME_OnGrantSkillPoints.Insert(OnGrantSkillPoints);

		DME_Tasks_Log.Info("TerjeAdapter: Export-Bruecke aktiv — Skill-Grants nach PendingGrants_<uid>.json (Auszahlung braucht server-spezifische Terje-Bruecke)");
	}

	// ------------------------------------------------------------------
	// Invoker-Abos (nur bei aktivem Adapter registriert)
	// ------------------------------------------------------------------

	private void OnGrantSkillPoints(string uid, int points) {
		DME_Tasks_Log.Info("TerjeAdapter: Skill-Grant %1 fuer %2 — als PendingGrant exportiert", points.ToString(), uid);
		DME_Tasks_PendingGrantsStore.Append(uid, "SKILL_POINTS", points);
	}
}
