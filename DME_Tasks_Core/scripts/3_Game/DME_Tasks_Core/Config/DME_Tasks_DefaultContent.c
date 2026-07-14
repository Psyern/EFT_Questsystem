//! Eingebetteter MVP-Default-Content (SPEC §7 Agent 19 + CONTRACTS §6.8) — ersetzt den Welle-0-Stub.
//!
//! SeedIfEmpty() prueft die vier Content-Gruppen UNABHAENGIG voneinander (Traders / Quests /
//! DailyTemplates / WeeklyTemplates): nur leere Ordner werden befuellt — vorhandener Admin-Content
//! wird NIE ueberschrieben. Die programmatisch gebauten Defs sind 1:1 identisch mit den Seed-JSONs
//! unter <MOD_ROOT>/_ServerProfile_Example/DME_Tasks (ExampleProfileRoot ist zur Laufzeit
//! unzuverlaessig, CONTRACTS §6.1 — deshalb Code-Einbettung). Geschrieben wird via
//! JsonFileLoader<T>.SaveFile in die $profile-Pfade (DME_Tasks_Paths), Log pro Datei; danach laedt
//! ConfigService.LoadAll die Dateien normal (SeedIfEmpty laeuft dort VOR LoadTraders/LoadQuests/
//! LoadTemplates — verifiziert in DME_Tasks_ConfigService.LoadAll).
//!
//! MVP-Kette (west_001..west_006, verkettet via Prerequisites.RequiredCompletedQuests):
//! 001 DISCOVER Lager → 002 KILL 5 Infizierte in der Zone → 003 COLLECT 3 Munitionskisten
//! (FoundInWorld) → 004 COLLECT+HANDOVER (Teilabgabe) → 005 KILL Kommandant (BossId-Fallback:
//! ZmbM_PatrolNormal_Base) + Beweisstueck → 006 RETURN_TO_TRADER + AK74-Reward + Market-Unlock.
//! west_004: ReferencesObjective verweist IMMER auf ein Objective DERSELBEN Quest
//! (HandoverProcessor loest questId-intern auf) — deshalb traegt west_004 ein eigenes
//! COLLECT-Objective; beim Annehmen zaehlt der Initial-Recount bereits gehaltene Kisten sofort.
//!
//! Zonen-Koordinaten: Chernarus-Beispielwerte (Myshkino-Militaerlager, ca. X 2035 / Z 7295) —
//! Serverbetreiber passen sie in den $profile-JSONs an die eigene Karte an.
class DME_Tasks_DefaultContent {
	static void SeedIfEmpty() {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		if (!HasJsonFiles(DME_Tasks_Paths.TradersDir())) {
			SeedTraders();
		}
		if (!HasJsonFiles(DME_Tasks_Paths.QuestsDir())) {
			SeedQuests();
		}
		if (!HasJsonFiles(DME_Tasks_Paths.DailyTemplatesDir())) {
			SeedDailyTemplates();
		}
		if (!HasJsonFiles(DME_Tasks_Paths.WeeklyTemplatesDir())) {
			SeedWeeklyTemplates();
		}
	}

	// ==================================================================
	// Gruppen-Seeding
	// ==================================================================

	private static void SeedTraders() {
		DME_Tasks_Log.Info("Kein Trader-Content gefunden — Default-Content wird angelegt");
		SaveTrader(BuildQuartermaster());
	}

	private static void SeedQuests() {
		DME_Tasks_Log.Info("Kein Quest-Content gefunden — MVP-Kette west_001..west_006 wird angelegt");
		SaveQuest(BuildWest001());
		SaveQuest(BuildWest002());
		SaveQuest(BuildWest003());
		SaveQuest(BuildWest004());
		SaveQuest(BuildWest005());
		SaveQuest(BuildWest006());
	}

	private static void SeedDailyTemplates() {
		DME_Tasks_Log.Info("Keine Daily-Templates gefunden — Default-Template wird angelegt");
		string path = DME_Tasks_Paths.DailyTemplatesDir() + "/daily_kill_infected.json";
		SaveTemplate(path, BuildDailyKillInfected());
	}

	private static void SeedWeeklyTemplates() {
		DME_Tasks_Log.Info("Keine Weekly-Templates gefunden — Default-Template wird angelegt");
		string path = DME_Tasks_Paths.WeeklyTemplatesDir() + "/weekly_boss.json";
		SaveTemplate(path, BuildWeeklyBoss());
	}

	// ==================================================================
	// Trader (identisch zu _ServerProfile_Example/DME_Tasks/Traders/west_quartermaster.json)
	// ==================================================================

	private static DME_Tasks_TraderDef BuildQuartermaster() {
		DME_Tasks_TraderDef def = new DME_Tasks_TraderDef();
		def.TraderId = "west_quartermaster";
		def.DisplayName = "#STR_DME_TASKS_SEED_TRADER_WEST";
		def.Faction = "WEST";

		DME_Tasks_LoyaltyLevel level1 = new DME_Tasks_LoyaltyLevel();
		level1.Level = 1;
		level1.RequiredPlayerLevel = 0;
		level1.RequiredReputation = 0.0;
		level1.RequiredTurnover = 0;
		def.LoyaltyLevels.Insert(level1);

		DME_Tasks_LoyaltyLevel level2 = new DME_Tasks_LoyaltyLevel();
		level2.Level = 2;
		level2.RequiredPlayerLevel = 10;
		level2.RequiredReputation = 0.2;
		level2.RequiredTurnover = 250000;
		def.LoyaltyLevels.Insert(level2);

		DME_Tasks_LoyaltyLevel level3 = new DME_Tasks_LoyaltyLevel();
		level3.Level = 3;
		level3.RequiredPlayerLevel = 20;
		level3.RequiredReputation = 0.45;
		level3.RequiredTurnover = 1000000;
		def.LoyaltyLevels.Insert(level3);

		return def;
	}

	// ==================================================================
	// Quests (identisch zu _ServerProfile_Example/DME_Tasks/Quests/west_00X.json)
	// ==================================================================

	private static DME_Tasks_QuestDef BuildWest001() {
		DME_Tasks_QuestDef def = new DME_Tasks_QuestDef();
		def.QuestId = "west_001";
		def.TraderId = "west_quartermaster";
		def.Category = "STORY";
		def.Title = "#STR_DME_TASKS_SEED_W001_TITLE";
		def.Description = "#STR_DME_TASKS_SEED_W001_DESC";

		DME_Tasks_ObjectiveDef discover = new DME_Tasks_ObjectiveDef();
		discover.ObjectiveId = "discover_camp";
		discover.Type = "DISCOVER";
		discover.Amount = 1;
		discover.Zone = BuildCampZone();
		def.Objectives.Insert(discover);

		def.Rewards.PlayerExperience = 1500;
		def.Rewards.Currency = 5000;
		def.Rewards.TraderReputation = 0.02;

		return def;
	}

	private static DME_Tasks_QuestDef BuildWest002() {
		DME_Tasks_QuestDef def = new DME_Tasks_QuestDef();
		def.QuestId = "west_002";
		def.TraderId = "west_quartermaster";
		def.Category = "STORY";
		def.Title = "#STR_DME_TASKS_SEED_W002_TITLE";
		def.Description = "#STR_DME_TASKS_SEED_W002_DESC";
		def.Prerequisites.RequiredCompletedQuests.Insert("west_001");

		DME_Tasks_ObjectiveDef kill = new DME_Tasks_ObjectiveDef();
		kill.ObjectiveId = "kill_infected_camp";
		kill.Type = "KILL";
		kill.Amount = 5;
		kill.TargetCategory = "INFECTED";
		kill.RequiredZones.Insert("mil_camp_west");
		kill.Zone = BuildCampZone();
		def.Objectives.Insert(kill);

		def.Rewards.PlayerExperience = 2500;
		def.Rewards.Currency = 7500;
		def.Rewards.TraderReputation = 0.02;

		return def;
	}

	private static DME_Tasks_QuestDef BuildWest003() {
		DME_Tasks_QuestDef def = new DME_Tasks_QuestDef();
		def.QuestId = "west_003";
		def.TraderId = "west_quartermaster";
		def.Category = "STORY";
		def.Title = "#STR_DME_TASKS_SEED_W003_TITLE";
		def.Description = "#STR_DME_TASKS_SEED_W003_DESC";
		def.Prerequisites.RequiredCompletedQuests.Insert("west_002");

		DME_Tasks_ObjectiveDef collect = new DME_Tasks_ObjectiveDef();
		collect.ObjectiveId = "collect_ammoboxes";
		collect.Type = "COLLECT";
		collect.Amount = 3;
		AddAmmoBoxClassNames(collect.ClassNames);
		collect.FoundInWorldRequired = true;
		AddFoundInWorldOrigins(collect.AllowedOrigins);
		def.Objectives.Insert(collect);

		def.Rewards.PlayerExperience = 3000;
		def.Rewards.Currency = 10000;
		def.Rewards.TraderReputation = 0.03;

		return def;
	}

	private static DME_Tasks_QuestDef BuildWest004() {
		DME_Tasks_QuestDef def = new DME_Tasks_QuestDef();
		def.QuestId = "west_004";
		def.TraderId = "west_quartermaster";
		def.Category = "STORY";
		def.Title = "#STR_DME_TASKS_SEED_W004_TITLE";
		def.Description = "#STR_DME_TASKS_SEED_W004_DESC";
		def.Prerequisites.RequiredCompletedQuests.Insert("west_003");

		DME_Tasks_ObjectiveDef collect = new DME_Tasks_ObjectiveDef();
		collect.ObjectiveId = "collect_ammoboxes";
		collect.Type = "COLLECT";
		collect.Amount = 3;
		AddAmmoBoxClassNames(collect.ClassNames);
		collect.FoundInWorldRequired = true;
		AddFoundInWorldOrigins(collect.AllowedOrigins);
		def.Objectives.Insert(collect);

		DME_Tasks_ObjectiveDef handover = new DME_Tasks_ObjectiveDef();
		handover.ObjectiveId = "handover_ammoboxes";
		handover.Type = "HANDOVER";
		handover.Amount = 3;
		handover.ReferencesObjective = "collect_ammoboxes";
		handover.AllowPartialHandover = true;
		def.Objectives.Insert(handover);

		def.Rewards.PlayerExperience = 3500;
		def.Rewards.Currency = 12000;
		def.Rewards.TraderReputation = 0.03;

		return def;
	}

	private static DME_Tasks_QuestDef BuildWest005() {
		DME_Tasks_QuestDef def = new DME_Tasks_QuestDef();
		def.QuestId = "west_005";
		def.TraderId = "west_quartermaster";
		def.Category = "STORY";
		def.Title = "#STR_DME_TASKS_SEED_W005_TITLE";
		def.Description = "#STR_DME_TASKS_SEED_W005_DESC";
		def.Prerequisites.MinimumTraderReputation = 0.05;
		def.Prerequisites.RequiredCompletedQuests.Insert("west_004");

		DME_Tasks_ObjectiveDef kill = new DME_Tasks_ObjectiveDef();
		kill.ObjectiveId = "kill_commander";
		kill.Type = "KILL";
		kill.Amount = 1;
		kill.BossId = "camp_commander";
		kill.ClassNames.Insert("ZmbM_PatrolNormal_Base");
		def.Objectives.Insert(kill);

		DME_Tasks_ObjectiveDef collect = new DME_Tasks_ObjectiveDef();
		collect.ObjectiveId = "collect_proof";
		collect.Type = "COLLECT";
		collect.Amount = 1;
		collect.ClassNames.Insert("GPSReceiver");
		collect.FoundInWorldRequired = true;
		def.Objectives.Insert(collect);

		DME_Tasks_ObjectiveDef handover = new DME_Tasks_ObjectiveDef();
		handover.ObjectiveId = "handover_proof";
		handover.Type = "HANDOVER";
		handover.Amount = 1;
		handover.ReferencesObjective = "collect_proof";
		handover.AllowPartialHandover = false;
		def.Objectives.Insert(handover);

		def.Rewards.PlayerExperience = 5000;
		def.Rewards.Currency = 20000;
		def.Rewards.TraderReputation = 0.04;

		return def;
	}

	private static DME_Tasks_QuestDef BuildWest006() {
		DME_Tasks_QuestDef def = new DME_Tasks_QuestDef();
		def.QuestId = "west_006";
		def.TraderId = "west_quartermaster";
		def.Category = "STORY";
		def.Title = "#STR_DME_TASKS_SEED_W006_TITLE";
		def.Description = "#STR_DME_TASKS_SEED_W006_DESC";
		def.Prerequisites.MinimumTraderReputation = 0.08;
		def.Prerequisites.RequiredCompletedQuests.Insert("west_005");

		DME_Tasks_ObjectiveDef report = new DME_Tasks_ObjectiveDef();
		report.ObjectiveId = "report_back";
		report.Type = "RETURN_TO_TRADER";
		report.Amount = 1;
		report.TraderId = "west_quartermaster";
		report.MustBeAlive = true;
		def.Objectives.Insert(report);

		def.Rewards.PlayerExperience = 8500;
		def.Rewards.Currency = 45000;
		def.Rewards.TraderReputation = 0.05;

		DME_Tasks_ItemReward ak = new DME_Tasks_ItemReward();
		ak.ClassName = "AK74";
		ak.Amount = 1;
		ak.Attachments.Insert("AK74_WoodBttstck");
		ak.Attachments.Insert("AK74_Hndgrd");
		ak.Attachments.Insert("Mag_AK74_30Rnd");
		def.Rewards.Items.Insert(ak);

		def.Unlocks.MarketItems.Insert("AK74");

		return def;
	}

	// ==================================================================
	// Templates (identisch zu _ServerProfile_Example/DME_Tasks/*Templates/*.json)
	// ==================================================================

	private static DME_Tasks_TemplateDef BuildDailyKillInfected() {
		DME_Tasks_TemplateDef def = new DME_Tasks_TemplateDef();
		def.TemplateId = "daily_kill_infected";
		def.TraderId = "west_quartermaster";
		def.ObjectiveType = "KILL";
		def.TargetTypes.Insert("INFECTED");
		def.MinimumAmount = 10;
		def.MaximumAmount = 40;
		def.AllowedZones.Insert("ANY");
		def.AllowedZones.Insert("MILITARY");
		def.AllowedZones.Insert("CITY");
		def.RewardTier = 2;
		def.TimeLimit = 86400;
		//! Rewards bleiben leer (Ctor-Defaults) → Generator nutzt die Tier-Formel
		return def;
	}

	private static DME_Tasks_TemplateDef BuildWeeklyBoss() {
		DME_Tasks_TemplateDef def = new DME_Tasks_TemplateDef();
		def.TemplateId = "weekly_boss";
		def.TraderId = "west_quartermaster";
		def.ObjectiveType = "BOSS";
		def.TargetTypes.Insert("ZmbM_PatrolNormal_Base");
		def.MinimumAmount = 1;
		def.MaximumAmount = 3;
		def.AllowedZones.Insert("ANY");
		def.RewardTier = 5;
		def.TimeLimit = 604800;

		def.Rewards.PlayerExperience = 6000;
		def.Rewards.Currency = 30000;
		def.Rewards.TraderReputation = 0.05;

		DME_Tasks_ItemReward ammo = new DME_Tasks_ItemReward();
		ammo.ClassName = "Ammo_545x39";
		ammo.Amount = 2;
		def.Rewards.Items.Insert(ammo);

		return def;
	}

	// ==================================================================
	// Gemeinsame Bausteine
	// ==================================================================

	//! Beispiel-Zone der MVP-Kette (west_001 + west_002) — jede Verwendung bekommt eine EIGENE
	//! Instanz (kein Aliasing zwischen QuestDefs).
	private static DME_Tasks_ZoneVolume BuildCampZone() {
		DME_Tasks_ZoneVolume zone = new DME_Tasks_ZoneVolume();
		zone.ZoneId = "mil_camp_west";
		zone.CenterX = 2035.0;
		zone.CenterY = 290.0;
		zone.CenterZ = 7295.0;
		zone.Radius = 150.0;
		zone.Height = 120.0;
		return zone;
	}

	private static void AddAmmoBoxClassNames(array<string> target) {
		target.Insert("AmmoBox_545x39_20Rnd");
		target.Insert("AmmoBox_545x39Tracer_20Rnd");
	}

	private static void AddFoundInWorldOrigins(array<string> target) {
		target.Insert("WORLD_LOOT");
		target.Insert("INFECTED_DROP");
		target.Insert("AI_DROP");
		target.Insert("BOSS_DROP");
	}

	// ==================================================================
	// Schreiben & Pruefen
	// ==================================================================

	private static void SaveTrader(DME_Tasks_TraderDef def) {
		string path = DME_Tasks_Paths.TraderFile(def.TraderId);
		string saveError;
		bool savedOk = JsonFileLoader<DME_Tasks_TraderDef>.SaveFile(path, def, saveError);
		if (savedOk) {
			DME_Tasks_Log.Info("Default-Content angelegt: %1", path);
		} else {
			DME_Tasks_Log.Warn("Default-Content konnte nicht geschrieben werden (%1): %2", path, saveError);
		}
	}

	private static void SaveQuest(DME_Tasks_QuestDef def) {
		string path = DME_Tasks_Paths.QuestFile(def.QuestId);
		string saveError;
		bool savedOk = JsonFileLoader<DME_Tasks_QuestDef>.SaveFile(path, def, saveError);
		if (savedOk) {
			DME_Tasks_Log.Info("Default-Content angelegt: %1", path);
		} else {
			DME_Tasks_Log.Warn("Default-Content konnte nicht geschrieben werden (%1): %2", path, saveError);
		}
	}

	private static void SaveTemplate(string path, DME_Tasks_TemplateDef def) {
		string saveError;
		bool savedOk = JsonFileLoader<DME_Tasks_TemplateDef>.SaveFile(path, def, saveError);
		if (savedOk) {
			DME_Tasks_Log.Info("Default-Content angelegt: %1", path);
		} else {
			DME_Tasks_Log.Warn("Default-Content konnte nicht geschrieben werden (%1): %2", path, saveError);
		}
	}

	private static bool HasJsonFiles(string dir) {
		string fileName;
		FileAttr fileAttr;
		string pattern = dir + "/*.json";
		FindFileHandle handle = FindFile(pattern, fileName, fileAttr, FindFileFlags.DIRECTORIES);
		if (!handle) {
			return false;
		}

		bool found = false;
		if (fileName != "") {
			found = true;
		}
		CloseFindFile(handle);

		return found;
	}
}
