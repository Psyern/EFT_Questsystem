class DME_Tasks_Paths {
	static string ProfileRoot() {
		return "$profile:DME_Tasks";
	}

	static string SettingsFile() {
		return ProfileRoot() + "/Settings.json";
	}

	static string TradersDir() {
		return ProfileRoot() + "/Traders";
	}

	static string TraderFile(string id) {
		return TradersDir() + "/" + id + ".json";
	}

	static string QuestsDir() {
		return ProfileRoot() + "/Quests";
	}

	static string QuestFile(string id) {
		return QuestsDir() + "/" + id + ".json";
	}

	static string DailyTemplatesDir() {
		return ProfileRoot() + "/DailyTemplates";
	}

	static string WeeklyTemplatesDir() {
		return ProfileRoot() + "/WeeklyTemplates";
	}

	static string GeneratedDir() {
		return ProfileRoot() + "/Generated";
	}

	static string GeneratedFile(string dateKey) {
		return GeneratedDir() + "/" + dateKey + ".json";
	}

	static string PlayersDir() {
		return ProfileRoot() + "/Players";
	}

	static string PlayerFile(string uid) {
		return PlayersDir() + "/" + uid + ".json";
	}

	static string PlayerTmpFile(string uid) {
		return PlayersDir() + "/" + uid + ".json.tmp";
	}

	static string LogsDir() {
		return ProfileRoot() + "/Logs";
	}

	static string BackupsDir() {
		return ProfileRoot() + "/Backups";
	}

	static string BackupFile(string uid, int slot) {
		return BackupsDir() + "/" + uid + "_" + slot.ToString() + ".json";
	}

	//! Seed-Quelle relativ zum Server-Arbeitsverzeichnis (Mod-Ordner im Server-Root).
	//! Nur gueltig, wenn der Mod-Ordner unter diesem Namen im Server-Root liegt —
	//! echtes Seeding (Welle 4) sollte Default-Content bevorzugt im Code einbetten.
	static string ExampleProfileRoot() {
		return "DME_Tarkov_Quest_System/_ServerProfile_Example/DME_Tasks";
	}

	static void EnsureDirectories() {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		EnsureDirectory(ProfileRoot());
		EnsureDirectory(TradersDir());
		EnsureDirectory(QuestsDir());
		EnsureDirectory(DailyTemplatesDir());
		EnsureDirectory(WeeklyTemplatesDir());
		EnsureDirectory(GeneratedDir());
		EnsureDirectory(PlayersDir());
		EnsureDirectory(LogsDir());
		EnsureDirectory(BackupsDir());
	}

	private static void EnsureDirectory(string path) {
		if (FileExist(path)) {
			return;
		}

		bool created = MakeDirectory(path);
		if (!created) {
			DME_Tasks_Log.Warn("Konnte Verzeichnis nicht anlegen: %1", path);
		}
	}
}
