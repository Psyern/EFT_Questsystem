//! Laedt Settings, Trader-, Quest- und Template-JSONs aus $profile:DME_Tasks (server-only).
//! Parse-Fehler scheitern nie hart: Warn-Log + Datei ueberspringen. Duplikate: erste Definition gewinnt.
class DME_Tasks_ConfigService {
	private static ref DME_Tasks_ConfigService s_DME_Instance;

	private ref DME_Tasks_Settings m_DME_Settings;
	private ref array<ref DME_Tasks_TraderDef> m_DME_Traders;
	private ref map<string, ref DME_Tasks_TraderDef> m_DME_TradersById;
	private ref array<ref DME_Tasks_QuestDef> m_DME_Quests;
	private ref map<string, ref DME_Tasks_QuestDef> m_DME_QuestsById;
	private ref array<ref DME_Tasks_TemplateDef> m_DME_DailyTemplates;
	private ref array<ref DME_Tasks_TemplateDef> m_DME_WeeklyTemplates;

	static DME_Tasks_ConfigService GetInstance() {
		if (!s_DME_Instance) {
			s_DME_Instance = new DME_Tasks_ConfigService();
		}
		return s_DME_Instance;
	}

	void DME_Tasks_ConfigService() {
		m_DME_Settings = new DME_Tasks_Settings();
		m_DME_Traders = new array<ref DME_Tasks_TraderDef>();
		m_DME_TradersById = new map<string, ref DME_Tasks_TraderDef>();
		m_DME_Quests = new array<ref DME_Tasks_QuestDef>();
		m_DME_QuestsById = new map<string, ref DME_Tasks_QuestDef>();
		m_DME_DailyTemplates = new array<ref DME_Tasks_TemplateDef>();
		m_DME_WeeklyTemplates = new array<ref DME_Tasks_TemplateDef>();
	}

	//! Server-Init (vom CoreModule nur auf dem Server gerufen).
	void LoadAll() {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		DME_Tasks_Paths.EnsureDirectories();
		LoadSettings();
		DME_Tasks_DefaultContent.SeedIfEmpty();
		LoadTraders();
		LoadQuests();
		LoadTemplates();

		string traderCount = m_DME_Traders.Count().ToString();
		string questCount = m_DME_Quests.Count().ToString();
		int templateTotal = m_DME_DailyTemplates.Count() + m_DME_WeeklyTemplates.Count();
		DME_Tasks_Log.Info("Konfiguration geladen: %1 Trader, %2 Quests, %3 Templates", traderCount, questCount, templateTotal.ToString());
	}

	DME_Tasks_Settings GetSettings() {
		return m_DME_Settings;
	}

	array<ref DME_Tasks_TraderDef> GetTraders() {
		return m_DME_Traders;
	}

	DME_Tasks_TraderDef GetTrader(string traderId) {
		return m_DME_TradersById.Get(traderId);
	}

	array<ref DME_Tasks_QuestDef> GetQuests() {
		return m_DME_Quests;
	}

	DME_Tasks_QuestDef GetQuest(string questId) {
		return m_DME_QuestsById.Get(questId);
	}

	array<ref DME_Tasks_QuestDef> GetQuestsForTrader(string traderId) {
		array<ref DME_Tasks_QuestDef> result = new array<ref DME_Tasks_QuestDef>();
		foreach (DME_Tasks_QuestDef questDef : m_DME_Quests) {
			if (questDef && questDef.TraderId == traderId) {
				result.Insert(questDef);
			}
		}
		return result;
	}

	//! Daily/Weekly-Generator speist generierte Quests hier ein.
	void RegisterRuntimeQuest(DME_Tasks_QuestDef def) {
		if (!def) {
			DME_Tasks_Log.Warn("RegisterRuntimeQuest: null-Definition ignoriert");
			return;
		}
		if (def.QuestId == "") {
			DME_Tasks_Log.Warn("RegisterRuntimeQuest: QuestId fehlt — ignoriert");
			return;
		}
		if (m_DME_QuestsById.Contains(def.QuestId)) {
			DME_Tasks_Log.Warn("RegisterRuntimeQuest: doppelte QuestId %1 — erste Definition gewinnt", def.QuestId);
			return;
		}

		m_DME_Quests.Insert(def);
		m_DME_QuestsById.Insert(def.QuestId, def);
	}

	array<ref DME_Tasks_TemplateDef> GetDailyTemplates() {
		return m_DME_DailyTemplates;
	}

	array<ref DME_Tasks_TemplateDef> GetWeeklyTemplates() {
		return m_DME_WeeklyTemplates;
	}

	// ------------------------------------------------------------------
	// intern
	// ------------------------------------------------------------------

	private void LoadSettings() {
		string settingsPath = DME_Tasks_Paths.SettingsFile();
		m_DME_Settings = new DME_Tasks_Settings();

		if (FileExist(settingsPath)) {
			string loadError;
			DME_Tasks_Settings loaded = new DME_Tasks_Settings();
			bool loadedOk = JsonFileLoader<DME_Tasks_Settings>.LoadFile(settingsPath, loaded, loadError);
			if (loadedOk && loaded) {
				m_DME_Settings = loaded;
				ValidateSettings();
			} else {
				DME_Tasks_Log.Warn("Settings.json unlesbar — Defaults aktiv: %1", loadError);
			}
		} else {
			string saveError;
			bool savedOk = JsonFileLoader<DME_Tasks_Settings>.SaveFile(settingsPath, m_DME_Settings, saveError);
			if (savedOk) {
				DME_Tasks_Log.Info("Settings.json mit Defaults angelegt: %1", settingsPath);
			} else {
				DME_Tasks_Log.Warn("Settings.json konnte nicht angelegt werden: %1", saveError);
			}
		}
	}

	private void ValidateSettings() {
		if (m_DME_Settings.FlushIntervalSeconds <= 0) {
			DME_Tasks_Log.Warn("Settings: FlushIntervalSeconds ungueltig — Default %1 aktiv", DME_Tasks_Const.DEFAULT_FLUSH_INTERVAL.ToString());
			m_DME_Settings.FlushIntervalSeconds = DME_Tasks_Const.DEFAULT_FLUSH_INTERVAL;
		}
		if (m_DME_Settings.BackupSlots < 0) {
			DME_Tasks_Log.Warn("Settings: BackupSlots ungueltig — Default %1 aktiv", DME_Tasks_Const.DEFAULT_BACKUP_SLOTS.ToString());
			m_DME_Settings.BackupSlots = DME_Tasks_Const.DEFAULT_BACKUP_SLOTS;
		}
	}

	private void LoadTraders() {
		m_DME_Traders.Clear();
		m_DME_TradersById.Clear();

		string tradersDir = DME_Tasks_Paths.TradersDir();
		array<string> fileNames = EnumerateJsonFiles(tradersDir);
		foreach (string fileName : fileNames) {
			string filePath = tradersDir + "/" + fileName;
			LoadTraderFile(filePath);
		}
	}

	private void LoadTraderFile(string filePath) {
		string loadError;
		DME_Tasks_TraderDef def = new DME_Tasks_TraderDef();
		bool loadedOk = JsonFileLoader<DME_Tasks_TraderDef>.LoadFile(filePath, def, loadError);
		if (!loadedOk || !def) {
			DME_Tasks_Log.Warn("Trader-Datei uebersprungen (%1): %2", filePath, loadError);
			return;
		}
		if (def.TraderId == "") {
			DME_Tasks_Log.Warn("Trader-Datei uebersprungen (%1): Pflichtfeld TraderId fehlt", filePath);
			return;
		}
		if (m_DME_TradersById.Contains(def.TraderId)) {
			DME_Tasks_Log.Warn("Doppelte TraderId %1 in %2 — erste Definition gewinnt", def.TraderId, filePath);
			return;
		}

		m_DME_Traders.Insert(def);
		m_DME_TradersById.Insert(def.TraderId, def);
	}

	private void LoadQuests() {
		m_DME_Quests.Clear();
		m_DME_QuestsById.Clear();

		string questsDir = DME_Tasks_Paths.QuestsDir();
		array<string> fileNames = EnumerateJsonFiles(questsDir);
		foreach (string fileName : fileNames) {
			string filePath = questsDir + "/" + fileName;
			LoadQuestFile(filePath);
		}
	}

	private void LoadQuestFile(string filePath) {
		string loadError;
		DME_Tasks_QuestDef def = new DME_Tasks_QuestDef();
		bool loadedOk = JsonFileLoader<DME_Tasks_QuestDef>.LoadFile(filePath, def, loadError);
		if (!loadedOk || !def) {
			DME_Tasks_Log.Warn("Quest-Datei uebersprungen (%1): %2", filePath, loadError);
			return;
		}
		if (def.QuestId == "") {
			DME_Tasks_Log.Warn("Quest-Datei uebersprungen (%1): Pflichtfeld QuestId fehlt", filePath);
			return;
		}
		if (m_DME_QuestsById.Contains(def.QuestId)) {
			DME_Tasks_Log.Warn("Doppelte QuestId %1 in %2 — erste Definition gewinnt", def.QuestId, filePath);
			return;
		}
		if (def.TraderId == "") {
			DME_Tasks_Log.Warn("Quest %1: TraderId leer — Quest ist keinem Haendler zugeordnet", def.QuestId);
		}

		m_DME_Quests.Insert(def);
		m_DME_QuestsById.Insert(def.QuestId, def);
	}

	private void LoadTemplates() {
		m_DME_DailyTemplates.Clear();
		m_DME_WeeklyTemplates.Clear();

		LoadTemplatesFrom(DME_Tasks_Paths.DailyTemplatesDir(), m_DME_DailyTemplates);
		LoadTemplatesFrom(DME_Tasks_Paths.WeeklyTemplatesDir(), m_DME_WeeklyTemplates);
	}

	private void LoadTemplatesFrom(string templatesDir, array<ref DME_Tasks_TemplateDef> target) {
		array<string> fileNames = EnumerateJsonFiles(templatesDir);
		foreach (string fileName : fileNames) {
			string filePath = templatesDir + "/" + fileName;
			string loadError;
			DME_Tasks_TemplateDef def = new DME_Tasks_TemplateDef();
			bool loadedOk = JsonFileLoader<DME_Tasks_TemplateDef>.LoadFile(filePath, def, loadError);
			if (!loadedOk || !def) {
				DME_Tasks_Log.Warn("Template-Datei uebersprungen (%1): %2", filePath, loadError);
				continue;
			}
			if (def.TemplateId == "") {
				DME_Tasks_Log.Warn("Template-Datei uebersprungen (%1): Pflichtfeld TemplateId fehlt", filePath);
				continue;
			}

			target.Insert(def);
		}
	}

	//! Liefert nur Dateinamen (ohne Pfad), Muster "<dir>/*.json".
	private array<string> EnumerateJsonFiles(string dir) {
		array<string> result = new array<string>();

		string fileName;
		FileAttr fileAttr;
		string pattern = dir + "/*.json";
		FindFileHandle handle = FindFile(pattern, fileName, fileAttr, FindFileFlags.DIRECTORIES);
		if (!handle) {
			return result;
		}

		if (fileName != "") {
			result.Insert(fileName);
		}
		while (FindNextFile(handle, fileName, fileAttr)) {
			if (fileName != "") {
				result.Insert(fileName);
			}
		}
		CloseFindFile(handle);

		return result;
	}
}
