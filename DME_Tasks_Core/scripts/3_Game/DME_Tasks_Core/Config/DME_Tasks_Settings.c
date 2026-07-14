//! JSON-Model fuer $profile:DME_Tasks/Settings.json — Member OHNE Prefix (Keys = JSON).
class DME_Tasks_Settings {
	int Version;
	float FlushIntervalSeconds;
	int BackupSlots;
	int DailyReplaceCost;
	int DailyQuestCount;
	int WeeklyQuestCount;
	bool EnableExpeditions;
	bool EnableDailyWeekly;
	bool EnableIntegrationMarket;
	bool EnableIntegrationAI;
	bool EnableIntegrationGroups;
	bool EnableIntegrationBoss;
	bool EnableIntegrationTerje;
	bool EnableIntegrationSeason;
	bool EnableIntegrationWebsite;

	void DME_Tasks_Settings() {
		Version = 1;
		FlushIntervalSeconds = DME_Tasks_Const.DEFAULT_FLUSH_INTERVAL;
		BackupSlots = DME_Tasks_Const.DEFAULT_BACKUP_SLOTS;
		DailyReplaceCost = 5000;
		DailyQuestCount = 3;
		WeeklyQuestCount = 2;
		EnableExpeditions = true;
		EnableDailyWeekly = true;
		EnableIntegrationMarket = false;
		EnableIntegrationAI = false;
		EnableIntegrationGroups = false;
		EnableIntegrationBoss = false;
		EnableIntegrationTerje = false;
		EnableIntegrationSeason = false;
		EnableIntegrationWebsite = false;
	}
}
