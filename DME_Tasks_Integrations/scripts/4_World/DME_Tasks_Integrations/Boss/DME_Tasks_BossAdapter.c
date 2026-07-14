//! Boss-Adapter (CONTRACTS §6.8) — KEIN Fremd-Mod-Check (AddonPatchName leer): Bosse sind
//! Server-Custom-Klassen (Zombies/Tiere/KI beliebiger Mods); allein der Settings-Toggle
//! EnableIntegrationBoss entscheidet ueber die Aktivierung.
//!
//! Aufgabe: Klassennamen → BossId-Mapping aus eigener Config
//! $profile:DME_Tasks/Integrations/Bosses.json. Der Classifier-Mod setzt m_DME_BossId am
//! DME_Tasks_KillEvent (Seam DME_Tasks_KillClassifier, super ZUERST); der DME_Tasks_KillHandler
//! matcht die BossId dann exakt gegen ObjectiveDef.BossId (BOSS-/KILL-Objectives).
//!
//! ClassName-Match (String-only, keine Hard-Refs):
//! 1. exakter Typ-Vergleich (m_DME_VictimType == ClassName),
//! 2. Config-Vererbung via Object.IsKindOf(string) (Object.c:517),
//! 3. Script-Vererbung via string.ToType() + Object.IsInherited(typename) — fuer Bosse, die nur
//!    als Script-Klasse existieren.
//! Fehlende Config-Datei → Default-Datei mit leerem Mapping + Beispiel im Comment-Feld.

//! Config-Models (JSON-Member OHNE Prefix — Keys = Datei).
class DME_Tasks_BossMapEntry {
	string ClassName;
	string BossId;

	void DME_Tasks_BossMapEntry() {
		ClassName = "";
		BossId = "";
	}
}

class DME_Tasks_BossMapDef {
	int Version;
	string Comment;
	ref array<ref DME_Tasks_BossMapEntry> Bosses;

	void DME_Tasks_BossMapDef() {
		Version = 1;
		Comment = "";
		Bosses = new array<ref DME_Tasks_BossMapEntry>();
	}
}

class DME_Tasks_BossAdapter : DME_Tasks_IntegrationBase {
	private static bool s_DME_Active = false;
	private static ref array<ref DME_Tasks_BossMapEntry> s_DME_Entries = new array<ref DME_Tasks_BossMapEntry>();

	void DME_Tasks_BossAdapter() {
		m_DME_Name = "Boss";
		m_DME_AddonPatchName = "";
	}

	override bool IsEnabled(DME_Tasks_Settings settings) {
		return settings.EnableIntegrationBoss;
	}

	override void OnActivate() {
		DME_Tasks_IntegrationPaths.EnsureIntegrationsDir();
		LoadOrCreateBossMap();

		s_DME_Active = true;
		string entryCount = s_DME_Entries.Count().ToString();
		DME_Tasks_Log.Info("BossAdapter: Kill-Klassifikation aktiv (%1 Boss-Mappings aus Bosses.json)", entryCount);
	}

	//! Vom modded DME_Tasks_KillClassifier gerufen (server-only Pfad — DispatchKill ist geguarded).
	static void ClassifyVictim(EntityAI victim, DME_Tasks_KillEvent evt) {
		if (!s_DME_Active) {
			return;
		}
		if (!victim || !evt) {
			return;
		}
		if (evt.m_DME_BossId != "") {
			return;
		}

		foreach (DME_Tasks_BossMapEntry entry : s_DME_Entries) {
			if (!entry) {
				continue;
			}
			if (MatchesEntry(victim, evt.m_DME_VictimType, entry.ClassName)) {
				evt.m_DME_BossId = entry.BossId;
				return;
			}
		}
	}

	// ------------------------------------------------------------------
	// intern
	// ------------------------------------------------------------------

	private static bool MatchesEntry(EntityAI victim, string victimType, string className) {
		if (className == "") {
			return false;
		}
		if (victimType == className) {
			return true;
		}
		if (victim.IsKindOf(className)) {
			return true;
		}

		typename baseType = className.ToType();
		if (baseType && victim.IsInherited(baseType)) {
			return true;
		}
		return false;
	}

	private void LoadOrCreateBossMap() {
		string path = DME_Tasks_IntegrationPaths.BossesFile();

		if (!FileExist(path)) {
			DME_Tasks_BossMapDef defaults = new DME_Tasks_BossMapDef();
			defaults.Comment = "Boss-Mapping: Bosses-Eintraege wie {\"ClassName\": \"ZmbM_CommanderBoss\", \"BossId\": \"camp_commander\"} — ClassName matcht exakt oder per Config-/Script-Vererbung; BossId referenziert ObjectiveDef.BossId der Quest.";

			string saveError;
			bool savedOk = JsonFileLoader<DME_Tasks_BossMapDef>.SaveFile(path, defaults, saveError);
			if (savedOk) {
				DME_Tasks_Log.Info("BossAdapter: Default-Config (leeres Mapping) angelegt: %1", path);
			} else {
				DME_Tasks_Log.Warn("BossAdapter: Default-Config nicht anlegbar (%1): %2", path, saveError);
			}

			ApplyBossMap(defaults);
			return;
		}

		string loadError;
		DME_Tasks_BossMapDef loaded = new DME_Tasks_BossMapDef();
		bool loadedOk = JsonFileLoader<DME_Tasks_BossMapDef>.LoadFile(path, loaded, loadError);
		if (!loadedOk || !loaded) {
			DME_Tasks_Log.Warn("BossAdapter: Bosses.json unlesbar — leeres Mapping aktiv: %1", loadError);
			loaded = new DME_Tasks_BossMapDef();
		}

		ApplyBossMap(loaded);
	}

	private void ApplyBossMap(DME_Tasks_BossMapDef def) {
		s_DME_Entries.Clear();
		foreach (DME_Tasks_BossMapEntry entry : def.Bosses) {
			if (!entry) {
				continue;
			}
			if (entry.ClassName == "" || entry.BossId == "") {
				DME_Tasks_Log.Warn("BossAdapter: Bosses.json-Eintrag ohne ClassName/BossId uebersprungen");
				continue;
			}
			s_DME_Entries.Insert(entry);
		}
	}
}

//! Klassifikator-Kette (CONTRACTS §6.8): super ZUERST — laeuft NACH dem AI-Adapter-Glied,
//! beide Anreicherungen (VictimIsAI + BossId) kombinieren sich am selben Event.
modded class DME_Tasks_KillClassifier {
	override void Classify(EntityAI victim, DME_Tasks_KillEvent evt) {
		super.Classify(victim, evt);

		DME_Tasks_BossAdapter.ClassifyVictim(victim, evt);
	}
}
