//! AI-Adapter (CONTRACTS §6.8) — Erkennung: CfgPatches "DayZExpansion_AI";
//! Toggle: Settings.EnableIntegrationAI.
//!
//! Aufgabe: Kill-Credit fuer KI-Opfer. Expansion-AI-Einheiten sind PlayerBase-Ableitungen
//! (Script-Klasse eAIBase, verifiziert DayZExpansion/AI/.../eAIBase.c: "class eAIBase: PlayerBase";
//! Config-Klassen eAI_SurvivorM_*: SurvivorM_*). Der KillHook stempelt solche Opfer deshalb als
//! VictimIsPlayer=true / VictimCategory="PLAYER". Dieser Adapter korrigiert das ueber den
//! DME_Tasks_KillClassifier-Seam: Opfer OHNE Identity, das gegen eine der konfigurierbaren
//! Basisklassen matcht (Default ["eAIBase"]) → VictimIsAI=true, VictimCategory="AI".
//!
//! String-only-Klassenchecks (keine Hard-Refs auf Fremd-Mod-Typen):
//! - Script-Vererbung: string.ToType() (EnString.c:95) + Object.IsInherited(typename)
//!   (EnScript.c:23) — noetig, weil "eAIBase" NUR als Script-Klasse existiert (kein CfgPatches-/
//!   CfgVehicles-Eintrag dieses Namens).
//! - Config-Vererbung: Object.IsKindOf(string) (Object.c:517) als Fallback fuer Admin-Eintraege,
//!   die Config-Klassen benennen (z. B. eigene NPC-Basisklassen anderer AI-Mods).
//!
//! Faehigkeits-Grenzen (dokumentiert):
//! - Eine FRAKTIONS-Kategorie (z. B. "BANDIT_AI") ist ohne Referenz auf eAI-Gruppen-/Fraktions-
//!   Klassen nicht ableitbar — der Adapter setzt pauschal "AI". Server-spezifische Bruecken
//!   koennen praezisere Events selbst dispatchen (CONTRACTS §6.7: DME_Tasks_KillEvent mit eigener
//!   m_DME_VictimCategory via DME_Tasks_EventBus).
//! - Ausgeloggte Spieler-Koerper haben ebenfalls keine Identity; sie matchen aber nur, wenn ein
//!   Admin eine zu breite Basisklasse (z. B. "PlayerBase"/"SurvivorBase") konfiguriert — Default
//!   "eAIBase" ist davon nicht betroffen.

//! Eigene Adapter-Config $profile:DME_Tasks/Integrations/AIClasses.json (Member OHNE Prefix).
class DME_Tasks_AIClassesDef {
	int Version;
	string Comment;
	ref array<string> BaseClasses;

	void DME_Tasks_AIClassesDef() {
		Version = 1;
		Comment = "";
		BaseClasses = new array<string>();
	}
}

class DME_Tasks_AIAdapter : DME_Tasks_IntegrationBase {
	private static bool s_DME_Active = false;
	private static ref array<string> s_DME_BaseClasses = new array<string>();

	void DME_Tasks_AIAdapter() {
		m_DME_Name = "AI";
		m_DME_AddonPatchName = "DayZExpansion_AI";
	}

	override bool IsEnabled(DME_Tasks_Settings settings) {
		return settings.EnableIntegrationAI;
	}

	override void OnActivate() {
		DME_Tasks_IntegrationPaths.EnsureIntegrationsDir();
		LoadOrCreateClassesConfig();

		s_DME_Active = true;
		string classCount = s_DME_BaseClasses.Count().ToString();
		DME_Tasks_Log.Info("AIAdapter: Kill-Klassifikation aktiv (%1 Basisklassen, VictimCategory=\"AI\")", classCount);
	}

	//! Vom modded DME_Tasks_KillClassifier gerufen (server-only Pfad — DispatchKill ist geguarded).
	static void ClassifyVictim(EntityAI victim, DME_Tasks_KillEvent evt) {
		if (!s_DME_Active) {
			return;
		}
		if (!victim || !evt) {
			return;
		}
		if (evt.m_DME_VictimIsAI) {
			return;
		}

		//! KI im eAI-Muster ist immer Man-abgeleitet; echte Spieler haben eine Identity.
		Man victimMan = Man.Cast(victim);
		if (!victimMan) {
			return;
		}
		if (victimMan.GetIdentity()) {
			return;
		}

		if (!MatchesAnyBaseClass(victim)) {
			return;
		}

		evt.m_DME_VictimIsAI = true;
		evt.m_DME_VictimIsPlayer = false;
		//! Fraktion ohne Refs nicht ableitbar (siehe Header) — "AI" ersetzt die leere bzw. die
		//! vom KillHook fuer PlayerBase-Opfer gesetzte "PLAYER"-Kategorie.
		if (evt.m_DME_VictimCategory == "" || evt.m_DME_VictimCategory == "PLAYER") {
			evt.m_DME_VictimCategory = "AI";
		}
	}

	// ------------------------------------------------------------------
	// intern
	// ------------------------------------------------------------------

	private static bool MatchesAnyBaseClass(EntityAI victim) {
		foreach (string baseClass : s_DME_BaseClasses) {
			if (baseClass == "") {
				continue;
			}

			typename baseType = baseClass.ToType();
			if (baseType && victim.IsInherited(baseType)) {
				return true;
			}
			if (victim.IsKindOf(baseClass)) {
				return true;
			}
		}
		return false;
	}

	private void LoadOrCreateClassesConfig() {
		string path = DME_Tasks_IntegrationPaths.AIClassesFile();

		if (!FileExist(path)) {
			DME_Tasks_AIClassesDef defaults = new DME_Tasks_AIClassesDef();
			defaults.Comment = "Basisklassen (Script- oder Config-Klasse), deren Ableitungen ohne Identity als KI-Kill zaehlen. Default: eAIBase (DayZ Expansion AI).";
			defaults.BaseClasses.Insert("eAIBase");

			string saveError;
			bool savedOk = JsonFileLoader<DME_Tasks_AIClassesDef>.SaveFile(path, defaults, saveError);
			if (savedOk) {
				DME_Tasks_Log.Info("AIAdapter: Default-Config angelegt: %1", path);
			} else {
				DME_Tasks_Log.Warn("AIAdapter: Default-Config nicht anlegbar (%1): %2", path, saveError);
			}

			ApplyClassesConfig(defaults);
			return;
		}

		string loadError;
		DME_Tasks_AIClassesDef loaded = new DME_Tasks_AIClassesDef();
		bool loadedOk = JsonFileLoader<DME_Tasks_AIClassesDef>.LoadFile(path, loaded, loadError);
		if (!loadedOk || !loaded) {
			DME_Tasks_Log.Warn("AIAdapter: AIClasses.json unlesbar — Default [eAIBase] aktiv: %1", loadError);
			loaded = new DME_Tasks_AIClassesDef();
			loaded.BaseClasses.Insert("eAIBase");
		}
		if (loaded.BaseClasses.Count() == 0) {
			DME_Tasks_Log.Warn("AIAdapter: AIClasses.json ohne BaseClasses — Default [eAIBase] aktiv");
			loaded.BaseClasses.Insert("eAIBase");
		}

		ApplyClassesConfig(loaded);
	}

	private void ApplyClassesConfig(DME_Tasks_AIClassesDef def) {
		s_DME_BaseClasses.Clear();
		foreach (string baseClass : def.BaseClasses) {
			if (baseClass != "") {
				s_DME_BaseClasses.Insert(baseClass);
			}
		}
	}
}

//! Klassifikator-Kette (CONTRACTS §6.8): super ZUERST, dann eigene Anreicherung.
//! Kompiliert immer mit; ohne aktiven Adapter ist ClassifyVictim ein no-op (s_DME_Active-Guard).
modded class DME_Tasks_KillClassifier {
	override void Classify(EntityAI victim, DME_Tasks_KillEvent evt) {
		super.Classify(victim, evt);

		DME_Tasks_AIAdapter.ClassifyVictim(victim, evt);
	}
}
