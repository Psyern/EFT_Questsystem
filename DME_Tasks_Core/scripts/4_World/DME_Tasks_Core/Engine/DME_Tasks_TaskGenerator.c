//! Deterministischer LCG (CONTRACTS §6.8) — KEIN Math.Random: gleicher Seed liefert auf jedem
//! Server und nach jedem Neustart dieselbe Sequenz. Parameter wie C-rand (1103515245 / 12345);
//! der 32-Bit-Overflow ist in EnScript definiert (Wrap-around) und Teil des Determinismus.
//! Wertebereich pro Zug: 0..32767 — fuer Template-Auswahl/Amounts/Zonen mehr als ausreichend.
class DME_Tasks_SeededRng {
	private int m_DME_State;

	void DME_Tasks_SeededRng(int seed) {
		m_DME_State = seed;
	}

	//! Deterministischer Wert in [minInclusive, maxExclusive); ungueltiger Bereich → minInclusive.
	int NextInt(int minInclusive, int maxExclusive) {
		int range = maxExclusive - minInclusive;
		if (range <= 0) {
			return minInclusive;
		}

		m_DME_State = m_DME_State * 1103515245 + 12345;
		int value = (m_DME_State >> 16) & 0x7FFF;
		return minInclusive + (value % range);
	}

	//! Simpler String-Hash (CONTRACTS §6.8: "Summe char*31") — bewusst eigene Implementation statt
	//! string.Hash(), damit der Seed engine-unabhaengig reproduzierbar bleibt.
	static int HashString(string s) {
		int hash = 0;
		for (int i = 0; i < s.Length(); i++) {
			string ch = s.Get(i);
			hash = hash * 31 + ch.ToAscii();
		}
		return hash;
	}
}

//! Daily/Weekly-Generator (CONTRACTS §6.8, Agent 16) — Singleton, ausschliesslich Dedicated Server.
//!
//! RESTART-STABILITAET: Generierte Quests werden als DME_Tasks_GeneratedSet nach
//! $profile:DME_Tasks/Generated/<TodayKey>.json bzw. Generated/week_<WeekKey>.json geschrieben.
//! Existiert die Datei, wird sie geladen und 1:1 registriert (nach Neustart identische Quests);
//! sonst wird deterministisch aus den Daily-/Weekly-Templates generiert und gespeichert.
//! Es gibt KEINE Zeit-/Zufallsabhaengigkeit ausser TodayKey()/WeekKey() (UTC, via CF_Date).
//!
//! SEEDS (alle via DME_Tasks_SeededRng.HashString):
//! - Template-Auswahl:            HashString(dateKey)
//! - Quest-Inhalt (Amount/Zone):  HashString(dateKey + TemplateId)
//! - Ersatzquest (ReplaceDaily):  HashString(TodayKey) + (HashString(uid) & 0x7FFFFFFF) + Anzahl bisheriger Replaces heute
//!
//! TEMPLATE→QUESTDEF-MAPPING:
//! - QuestId "daily_<TodayKey>_<n>" / "weekly_<WeekKey>_<n>" (n 1-basiert); Category "DAILY"/"WEEKLY";
//!   TraderId/TimeLimit direkt aus dem Template; Repeatable=false.
//! - ObjectiveType → Objectives[0].Type; TargetTypes → ClassNames. AUSNAHME KILL/BOSS: die
//!   Kategorie-Schluesselwoerter INFECTED/PLAYER/ANIMAL/AI wandern nach TargetCategory, denn der
//!   KillHandler matcht ClassNames gegen den Opfer-KLASSENNAMEN und Kategorien gegen VictimCategory
//!   (Kategorie in ClassNames wuerde nie Fortschritt liefern).
//! - Amount: deterministisch zwischen MinimumAmount..MaximumAmount (inklusive).
//! - AllowedZones: ERSTE GEWUERFELTE Zone; "ANY" (oder leere Liste) = keine Zonen-Bedingung.
//!   TRAVEL/DISCOVER: Zone → RequiredZones (Fortschritt braucht einen aktiven Zonen-Trigger mit
//!   dieser ZoneId — siehe TravelHandler-Header). Alle anderen Typen: Zone nur informativ in der
//!   Beschreibung, denn harte Zonen-Filter brauchen Zone-GEOMETRIE (Def.Zone), die Templates nicht
//!   tragen (RequiredZones ohne Def.Zone ist z. B. beim KillHandler eine dokumentierte
//!   Fehlkonfiguration ohne jeden Fortschritt — solche Quests waeren unabschliessbar).
//! - Rewards: Template.Rewards, wenn dort irgendein Feld gesetzt ist; sonst Tier-Formel
//!   XP = 1000 * RewardTier, Currency = 5000 * RewardTier, TraderReputation = 0.01 * RewardTier.
//!
//! REPLACEDAILY (per Spieler): Quest muss heutige DAILY und beim Spieler nicht COMPLETED sein;
//! die ersetzte Quest bekommt einen Cooldown bis Mitternacht UTC (persistiert via
//! PlayerState.Cooldowns → ueberlebt Neustarts); die Ersatzquest (Id-Suffix "_r<uidHash>") wird
//! registriert UND in das heutige Generated-Set geschrieben (restart-stabil). Die Kosten werden als
//! DME_Tasks_IntegrationEvents.s_DME_OnGrantCurrency.Invoke(uid, -Settings.DailyReplaceCost)
//! gemeldet — die DURCHSETZUNG braucht eine Market-/Currency-Integration (ohne Abonnent nur Log).
//!
//! AUFRAEUMEN: pro Gruppe (Daily/Weekly) bleiben die 7 lexikographisch neuesten Generated-Dateien
//! erhalten (Keys sind fix formatiert → Stringsortierung = Zeitsortierung); aktuelle Keys werden
//! nie geloescht. Datei-Enumeration = EnumerateJsonFiles-Muster aus DME_Tasks_ConfigService (§6.1).
//!
//! BEKANNTE MVP-GRENZEN (dokumentiert):
//! - ConfigService hat kein Unregister: nach einem Tageswechsel bei laufendem Server bleiben die
//!   Vortages-Dailies bis zum Neustart registriert/sichtbar.
//! - Ersatzquests sind technisch fuer alle Spieler sichtbar (keine per-Spieler-Definitionen);
//!   der Tagesend-Cooldown der ersetzten Quest gilt nur fuer den ausloesenden Spieler.
class DME_Tasks_TaskGenerator {
	private static ref DME_Tasks_TaskGenerator s_DME_Instance;

	//! Behalte pro Gruppe (Daily/Weekly) die 7 neuesten Generated-Dateien.
	private static const int KEEP_GENERATED_FILES = 7;
	private static const int SECONDS_PER_DAY = 86400;

	private ref DME_Tasks_GeneratedSet m_DME_TodaySet;
	private ref DME_Tasks_GeneratedSet m_DME_WeekSet;

	static DME_Tasks_TaskGenerator GetInstance() {
		if (!s_DME_Instance) {
			s_DME_Instance = new DME_Tasks_TaskGenerator();
		}
		return s_DME_Instance;
	}

	void DME_Tasks_TaskGenerator() {
		m_DME_TodaySet = null;
		m_DME_WeekSet = null;
	}

	// ==================================================================
	// Oeffentliche API (Signaturen wie Welle-1-Stub — Engine ruft sie bereits)
	// ==================================================================

	//! Laedt (restart-stabil) oder generiert (deterministisch) das heutige Daily- und das
	//! Wochen-Set, registriert alle Quests via ConfigService.RegisterRuntimeQuest und raeumt
	//! alte Generated-Dateien auf. Idempotent — bereits registrierte QuestIds werden uebersprungen.
	void GenerateForToday() {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		DME_Tasks_ConfigService configService = DME_Tasks_ConfigService.GetInstance();
		DME_Tasks_Settings settings = configService.GetSettings();
		if (!settings || !settings.EnableDailyWeekly) {
			DME_Tasks_Log.Info("TaskGenerator: Daily/Weekly deaktiviert (Settings.EnableDailyWeekly)");
			return;
		}

		string todayKey = DME_Tasks_TimeUtil.TodayKey();
		string weekKey = DME_Tasks_TimeUtil.WeekKey();
		string dailyPath = DME_Tasks_Paths.GeneratedFile(todayKey);
		string weeklyPath = DME_Tasks_Paths.GeneratedFile("week_" + weekKey);

		m_DME_TodaySet = LoadOrGenerateSet(dailyPath, todayKey, configService.GetDailyTemplates(), settings.DailyQuestCount, "daily_" + todayKey, "DAILY", DME_Tasks_LocKeys.GEN_DAILY_TITLE);
		m_DME_WeekSet = LoadOrGenerateSet(weeklyPath, weekKey, configService.GetWeeklyTemplates(), settings.WeeklyQuestCount, "weekly_" + weekKey, "WEEKLY", DME_Tasks_LocKeys.GEN_WEEKLY_TITLE);

		int registeredDaily = RegisterSet(m_DME_TodaySet);
		int registeredWeekly = RegisterSet(m_DME_WeekSet);
		DME_Tasks_Log.Info("TaskGenerator: %1 Daily- und %2 Weekly-Quests registriert (%3)", registeredDaily.ToString(), registeredWeekly.ToString(), todayKey);

		CleanupGeneratedFiles(todayKey, weekKey);
	}

	//! Ersetzt eine heutige Daily-Quest fuer EINEN Spieler (Semantik siehe Klassen-Header).
	//! true = ersetzt (die Engine benachrichtigt/synct den Aufrufer zusaetzlich selbst).
	bool ReplaceDaily(string uid, string questId) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return false;
		}
		if (uid == "" || questId == "") {
			return false;
		}

		DME_Tasks_ConfigService configService = DME_Tasks_ConfigService.GetInstance();
		DME_Tasks_Settings settings = configService.GetSettings();
		if (!settings || !settings.EnableDailyWeekly) {
			DME_Tasks_Log.Info("ReplaceDaily: Daily/Weekly deaktiviert (Settings.EnableDailyWeekly)");
			return false;
		}

		string todayKey = DME_Tasks_TimeUtil.TodayKey();

		//! Heutiges Set sicherstellen (deckt Tageswechsel bei laufendem Server ab)
		bool needsGenerate = false;
		if (!m_DME_TodaySet) {
			needsGenerate = true;
		} else if (m_DME_TodaySet.DateKey != todayKey) {
			needsGenerate = true;
		}
		if (needsGenerate) {
			GenerateForToday();
		}
		if (!m_DME_TodaySet) {
			DME_Tasks_Log.Warn("ReplaceDaily: heutiges Generated-Set fehlt");
			return false;
		}

		DME_Tasks_QuestDef def = configService.GetQuest(questId);
		if (!def) {
			DME_Tasks_Log.Warn("ReplaceDaily: unbekannte Quest %1", questId);
			return false;
		}
		if (def.Category != "DAILY") {
			DME_Tasks_Log.Info("ReplaceDaily: %1 ist keine DAILY-Quest", questId);
			return false;
		}
		string dailyPrefix = "daily_" + todayKey + "_";
		if (questId.IndexOf(dailyPrefix) != 0) {
			DME_Tasks_Log.Info("ReplaceDaily: %1 ist keine heutige Tagesquest", questId);
			return false;
		}

		DME_Tasks_QuestEngine engine = DME_Tasks_QuestEngine.GetInstance();
		DME_Tasks_PlayerState state = engine.GetPlayerState(uid);
		if (!state) {
			DME_Tasks_Log.Warn("ReplaceDaily: PlayerState fuer %1 nicht geladen", uid);
			return false;
		}

		if (HasCompletedQuest(state, questId)) {
			DME_Tasks_Log.Info("ReplaceDaily: %1 wurde von %2 bereits abgeschlossen", questId, uid);
			return false;
		}

		int now = DME_Tasks_TimeUtil.NowEpoch();
		int existingCooldown;
		if (state.Cooldowns.Find(questId, existingCooldown)) {
			if (now < existingCooldown) {
				DME_Tasks_Log.Info("ReplaceDaily: %1 wurde von %2 heute bereits ersetzt", questId, uid);
				return false;
			}
		}

		array<DME_Tasks_TemplateDef> validTemplates = new array<DME_Tasks_TemplateDef>();
		CollectValidTemplates(configService.GetDailyTemplates(), validTemplates);
		if (validTemplates.Count() == 0) {
			DME_Tasks_Log.Warn("ReplaceDaily: keine gueltigen Daily-Templates vorhanden");
			return false;
		}

		//! Ersatzquest deterministisch (CONTRACTS §6.8): Seed + uid-Hash + Anzahl bisheriger Replaces
		int uidHash = DME_Tasks_SeededRng.HashString(uid) & 0x7FFFFFFF;
		int replaceCount = CountCooldownsWithPrefix(state, dailyPrefix);
		int replaceSeed = DME_Tasks_SeededRng.HashString(todayKey) + uidHash + replaceCount;
		DME_Tasks_SeededRng rng = new DME_Tasks_SeededRng(replaceSeed);

		string newQuestId = questId + "_r" + uidHash.ToString();
		if (configService.GetQuest(newQuestId)) {
			DME_Tasks_Log.Warn("ReplaceDaily: Ersatzquest %1 existiert bereits — Abbruch", newQuestId);
			return false;
		}

		int templateIndex = rng.NextInt(0, validTemplates.Count());
		DME_Tasks_TemplateDef templateDef = validTemplates.Get(templateIndex);
		DME_Tasks_QuestDef newDef = BuildQuestFromTemplate(templateDef, rng, newQuestId, "DAILY", DME_Tasks_LocKeys.GEN_DAILY_TITLE);

		//! Registrieren + restart-stabil im heutigen Set persistieren
		configService.RegisterRuntimeQuest(newDef);
		m_DME_TodaySet.Quests.Insert(newDef);
		SaveSet(m_DME_TodaySet, DME_Tasks_Paths.GeneratedFile(todayKey));

		//! Laufende Quest beenden (Index-/Tracking-Cleanup uebernimmt der Fail-Pfad der Engine)
		if (engine.GetActiveQuest(uid, questId)) {
			engine.FailQuest(uid, questId, DME_Tasks_LocKeys.NOTIF_REASON_REPLACED);
		}

		//! Tagesend-Cooldown (Mitternacht UTC) — persistiert via PlayerState.Cooldowns
		int cooldownUntil = UtcMidnightEpoch();
		state.Cooldowns.Set(questId, cooldownUntil);
		DME_Tasks_PlayerStore.GetInstance().SaveNow(uid);

		//! Kosten via Invoker (§6.8) — Durchsetzung braucht Market-Integration; ohne Abonnent nur Log
		int cost = settings.DailyReplaceCost;
		if (cost > 0) {
			DME_Tasks_IntegrationEvents.s_DME_OnGrantCurrency.Invoke(uid, -cost);
			DME_Tasks_Log.Info("ReplaceDaily: Kosten %1 fuer %2 via s_DME_OnGrantCurrency gemeldet (Durchsetzung erfordert Market-Integration)", cost.ToString(), uid);
		}

		//! Kein eigener Re-Sync: QuestEngine.ReplaceDaily ruft bei true bereits RequestSync(sender)
		//! (ein Full-Sync ist gechunkt und teuer — nur EINE Stelle synct).

		DME_Tasks_Log.Info("ReplaceDaily: %1 durch %2 ersetzt (Spieler %3)", questId, newQuestId, uid);
		return true;
	}

	// ==================================================================
	// Set-Erzeugung
	// ==================================================================

	private DME_Tasks_GeneratedSet LoadOrGenerateSet(string filePath, string dateKey, array<ref DME_Tasks_TemplateDef> templates, int questCount, string idPrefix, string category, string titlePrefix) {
		DME_Tasks_GeneratedSet existing = LoadSet(filePath, dateKey);
		if (existing) {
			DME_Tasks_Log.Info("Generated-Set %1 geladen (%2 Quests, restart-stabil)", filePath, existing.Quests.Count().ToString());
			return existing;
		}

		DME_Tasks_GeneratedSet generated = GenerateSet(dateKey, templates, questCount, idPrefix, category, titlePrefix);
		SaveSet(generated, filePath);
		return generated;
	}

	private DME_Tasks_GeneratedSet LoadSet(string filePath, string expectedDateKey) {
		if (!FileExist(filePath)) {
			return null;
		}

		string loadError;
		DME_Tasks_GeneratedSet loadedSet = new DME_Tasks_GeneratedSet();
		bool loadedOk = JsonFileLoader<DME_Tasks_GeneratedSet>.LoadFile(filePath, loadedSet, loadError);
		if (!loadedOk || !loadedSet) {
			DME_Tasks_Log.Warn("Generated-Set unlesbar (%1): %2 — wird deterministisch neu generiert", filePath, loadError);
			return null;
		}
		if (loadedSet.DateKey != expectedDateKey) {
			DME_Tasks_Log.Warn("Generated-Set %1 traegt falschen DateKey %2 — wird neu generiert", filePath, loadedSet.DateKey);
			return null;
		}
		return loadedSet;
	}

	private bool SaveSet(DME_Tasks_GeneratedSet generatedSet, string filePath) {
		string saveError;
		bool savedOk = JsonFileLoader<DME_Tasks_GeneratedSet>.SaveFile(filePath, generatedSet, saveError);
		if (!savedOk) {
			DME_Tasks_Log.Warn("Generated-Set konnte nicht gespeichert werden (%1): %2", filePath, saveError);
		}
		return savedOk;
	}

	private DME_Tasks_GeneratedSet GenerateSet(string dateKey, array<ref DME_Tasks_TemplateDef> templates, int questCount, string idPrefix, string category, string titlePrefix) {
		DME_Tasks_GeneratedSet generatedSet = new DME_Tasks_GeneratedSet();
		generatedSet.DateKey = dateKey;

		if (questCount <= 0) {
			return generatedSet;
		}

		array<DME_Tasks_TemplateDef> validTemplates = new array<DME_Tasks_TemplateDef>();
		CollectValidTemplates(templates, validTemplates);
		if (validTemplates.Count() == 0) {
			DME_Tasks_Log.Info("Keine gueltigen Templates fuer %1 — Set bleibt leer", idPrefix);
			return generatedSet;
		}

		int selectionSeed = DME_Tasks_SeededRng.HashString(dateKey);
		DME_Tasks_SeededRng selectionRng = new DME_Tasks_SeededRng(selectionSeed);
		array<DME_Tasks_TemplateDef> picked = new array<DME_Tasks_TemplateDef>();
		PickTemplates(validTemplates, questCount, selectionRng, picked);

		for (int i = 0; i < picked.Count(); i++) {
			DME_Tasks_TemplateDef templateDef = picked.Get(i);
			int questNumber = i + 1;
			string questId = idPrefix + "_" + questNumber.ToString();
			int questSeed = DME_Tasks_SeededRng.HashString(dateKey + templateDef.TemplateId);
			DME_Tasks_SeededRng questRng = new DME_Tasks_SeededRng(questSeed);
			DME_Tasks_QuestDef questDef = BuildQuestFromTemplate(templateDef, questRng, questId, category, titlePrefix);
			generatedSet.Quests.Insert(questDef);
		}
		return generatedSet;
	}

	//! Nur Templates mit gueltiger TemplateId/TraderId/ObjectiveType (deterministischer Filter —
	//! haengt ausschliesslich vom geladenen Config-Stand ab).
	private void CollectValidTemplates(array<ref DME_Tasks_TemplateDef> source, array<DME_Tasks_TemplateDef> target) {
		if (!source) {
			return;
		}

		foreach (DME_Tasks_TemplateDef templateDef : source) {
			if (!templateDef) {
				continue;
			}
			if (templateDef.TemplateId == "") {
				continue;
			}
			if (templateDef.TraderId == "") {
				DME_Tasks_Log.Warn("Template %1 uebersprungen: TraderId fehlt (Quest waere keinem Haendler zugeordnet)", templateDef.TemplateId);
				continue;
			}
			int objectiveType = DME_Tasks_EnumUtil.ObjectiveTypeFromString(templateDef.ObjectiveType);
			if (objectiveType == -1) {
				DME_Tasks_Log.Warn("Template %1 uebersprungen: unbekannter ObjectiveType %2", templateDef.TemplateId, templateDef.ObjectiveType);
				continue;
			}
			target.Insert(templateDef);
		}
	}

	//! Auswahl ohne Wiederholung, bis der Pool erschoepft ist (dann Wieder-Auffuellung: mehr Quests
	//! als Templates erzeugt bewusst inhaltsgleiche Duplikate, da der Quest-Seed nur aus
	//! dateKey + TemplateId besteht).
	private void PickTemplates(array<DME_Tasks_TemplateDef> pool, int count, DME_Tasks_SeededRng rng, array<DME_Tasks_TemplateDef> picked) {
		if (pool.Count() == 0 || count <= 0) {
			return;
		}

		array<int> indices = new array<int>();
		FillIndexPool(indices, pool.Count());

		while (picked.Count() < count) {
			if (indices.Count() == 0) {
				FillIndexPool(indices, pool.Count());
			}
			int pickIndex = rng.NextInt(0, indices.Count());
			int templateIndex = indices.Get(pickIndex);
			indices.Remove(pickIndex);
			picked.Insert(pool.Get(templateIndex));
		}
	}

	private void FillIndexPool(array<int> indices, int poolSize) {
		for (int i = 0; i < poolSize; i++) {
			indices.Insert(i);
		}
	}

	// ==================================================================
	// Template → QuestDef (Mapping-Regeln siehe Klassen-Header)
	// ==================================================================

	private DME_Tasks_QuestDef BuildQuestFromTemplate(DME_Tasks_TemplateDef templateDef, DME_Tasks_SeededRng rng, string questId, string category, string titlePrefix) {
		DME_Tasks_QuestDef def = new DME_Tasks_QuestDef();
		def.QuestId = questId;
		def.TraderId = templateDef.TraderId;
		def.Category = category;
		def.Repeatable = false;
		def.TimeLimit = templateDef.TimeLimit;

		DME_Tasks_ObjectiveDef objective = new DME_Tasks_ObjectiveDef();
		objective.ObjectiveId = "obj_1";
		objective.Type = templateDef.ObjectiveType;

		//! Amount deterministisch zwischen MinimumAmount..MaximumAmount (inklusive)
		int minAmount = templateDef.MinimumAmount;
		if (minAmount < 1) {
			minAmount = 1;
		}
		int maxAmount = templateDef.MaximumAmount;
		if (maxAmount < minAmount) {
			maxAmount = minAmount;
		}
		int amount = rng.NextInt(minAmount, maxAmount + 1);
		objective.Amount = amount;

		//! TargetTypes → ClassNames; Kategorie-Schluesselwoerter bei KILL/BOSS → TargetCategory
		bool isKillType = false;
		if (templateDef.ObjectiveType == "KILL" || templateDef.ObjectiveType == "BOSS") {
			isKillType = true;
		}
		if (templateDef.TargetTypes) {
			foreach (string targetType : templateDef.TargetTypes) {
				if (targetType == "") {
					continue;
				}
				if (isKillType && IsKillCategory(targetType)) {
					if (objective.TargetCategory == "") {
						objective.TargetCategory = targetType;
					} else if (objective.TargetCategory != targetType) {
						DME_Tasks_Log.Warn("Template %1: mehrere Ziel-Kategorien — nur %2 wird verwendet", templateDef.TemplateId, objective.TargetCategory);
					}
					continue;
				}
				objective.ClassNames.Insert(targetType);
			}
		}

		//! Zone: erste gewuerfelte; "ANY" = keine Zonen-Bedingung (typabhaengige Anwendung, s. Header)
		string zoneName = "";
		if (templateDef.AllowedZones && templateDef.AllowedZones.Count() > 0) {
			int zoneIndex = rng.NextInt(0, templateDef.AllowedZones.Count());
			zoneName = templateDef.AllowedZones.Get(zoneIndex);
		}
		if (zoneName == "ANY") {
			zoneName = "";
		}
		if (zoneName != "") {
			bool zoneEnforceable = false;
			if (templateDef.ObjectiveType == "TRAVEL" || templateDef.ObjectiveType == "DISCOVER") {
				zoneEnforceable = true;
			}
			if (zoneEnforceable) {
				objective.RequiredZones.Insert(zoneName);
			} else {
				DME_Tasks_Log.Debug("Template %1: Zone %2 nur informativ (Typ %3 braucht Zone-Geometrie fuer harte Filter)", templateDef.TemplateId, zoneName, templateDef.ObjectiveType);
			}
		}

		if (templateDef.ObjectiveType == "RETURN_TO_TRADER") {
			objective.TraderId = templateDef.TraderId;
		}

		def.Objectives.Insert(objective);
		def.Rewards = BuildRewards(templateDef);

		//! '#' rule: generated quests carry static stringtable keys (titlePrefix = GEN_*_TITLE from the
		//! caller). The client renders the localized objective sentence, zone and time limit from the
		//! ObjectiveDef itself, so no server-composed prose is needed.
		def.Title = titlePrefix;
		if (category == "WEEKLY") {
			def.Description = DME_Tasks_LocKeys.GEN_WEEKLY_DESC;
		} else {
			def.Description = DME_Tasks_LocKeys.GEN_DAILY_DESC;
		}

		return def;
	}

	//! Template.Rewards, wenn dort irgendein Feld gesetzt ist (tiefe Kopie gegen Aliasing);
	//! sonst Tier-Formel: XP = 1000 * Tier, Currency = 5000 * Tier, TraderReputation = 0.01 * Tier.
	private DME_Tasks_RewardDef BuildRewards(DME_Tasks_TemplateDef templateDef) {
		DME_Tasks_RewardDef rewards = new DME_Tasks_RewardDef();

		DME_Tasks_RewardDef custom = templateDef.Rewards;
		if (custom && HasCustomRewards(custom)) {
			rewards.PlayerExperience = custom.PlayerExperience;
			rewards.Currency = custom.Currency;
			rewards.TraderReputation = custom.TraderReputation;
			rewards.SkillPoints = custom.SkillPoints;
			rewards.SeasonXp = custom.SeasonXp;
			if (custom.RivalReputation) {
				foreach (DME_Tasks_RivalRep rival : custom.RivalReputation) {
					if (!rival) {
						continue;
					}
					DME_Tasks_RivalRep rivalCopy = new DME_Tasks_RivalRep();
					rivalCopy.TraderId = rival.TraderId;
					rivalCopy.Delta = rival.Delta;
					rewards.RivalReputation.Insert(rivalCopy);
				}
			}
			if (custom.Items) {
				foreach (DME_Tasks_ItemReward item : custom.Items) {
					if (!item) {
						continue;
					}
					DME_Tasks_ItemReward itemCopy = new DME_Tasks_ItemReward();
					itemCopy.ClassName = item.ClassName;
					itemCopy.Amount = item.Amount;
					if (item.Attachments) {
						foreach (string attachment : item.Attachments) {
							itemCopy.Attachments.Insert(attachment);
						}
					}
					rewards.Items.Insert(itemCopy);
				}
			}
			return rewards;
		}

		int tier = templateDef.RewardTier;
		if (tier < 0) {
			tier = 0;
		}
		rewards.PlayerExperience = 1000 * tier;
		rewards.Currency = 5000 * tier;
		rewards.TraderReputation = 0.01 * tier;
		return rewards;
	}

	private bool HasCustomRewards(DME_Tasks_RewardDef rewards) {
		if (rewards.PlayerExperience != 0) {
			return true;
		}
		if (rewards.Currency != 0) {
			return true;
		}
		if (rewards.TraderReputation != 0.0) {
			return true;
		}
		if (rewards.SkillPoints != 0) {
			return true;
		}
		if (rewards.SeasonXp != 0) {
			return true;
		}
		if (rewards.RivalReputation && rewards.RivalReputation.Count() > 0) {
			return true;
		}
		if (rewards.Items && rewards.Items.Count() > 0) {
			return true;
		}
		return false;
	}

	//! Kategorien, die der KillHandler gegen VictimCategory matcht (nicht gegen Klassennamen).
	private bool IsKillCategory(string targetType) {
		if (targetType == "INFECTED") {
			return true;
		}
		if (targetType == "PLAYER") {
			return true;
		}
		if (targetType == "ANIMAL") {
			return true;
		}
		return targetType == "AI";
	}

	// ==================================================================
	// Registrierung & Aufraeumen
	// ==================================================================

	//! Registriert alle Quests des Sets (bereits registrierte QuestIds werden still uebersprungen —
	//! GenerateForToday kann mehrfach laufen, z. B. Init + Tageswechsel im ReplaceDaily-Pfad).
	private int RegisterSet(DME_Tasks_GeneratedSet generatedSet) {
		if (!generatedSet) {
			return 0;
		}

		DME_Tasks_ConfigService configService = DME_Tasks_ConfigService.GetInstance();
		int registered = 0;
		foreach (DME_Tasks_QuestDef def : generatedSet.Quests) {
			if (!def) {
				continue;
			}
			if (def.QuestId == "") {
				continue;
			}
			if (configService.GetQuest(def.QuestId)) {
				continue;
			}
			configService.RegisterRuntimeQuest(def);
			registered++;
		}
		return registered;
	}

	//! Aufraeum-Politik siehe Klassen-Header (7 neueste pro Gruppe, aktuelle Keys nie loeschen).
	private void CleanupGeneratedFiles(string todayKey, string weekKey) {
		array<string> fileNames = EnumerateJsonFiles(DME_Tasks_Paths.GeneratedDir());
		array<string> dailyFiles = new array<string>();
		array<string> weeklyFiles = new array<string>();
		foreach (string fileName : fileNames) {
			if (fileName.IndexOf("week_") == 0) {
				weeklyFiles.Insert(fileName);
			} else {
				dailyFiles.Insert(fileName);
			}
		}

		DeleteOldGenerated(dailyFiles, todayKey + ".json");
		DeleteOldGenerated(weeklyFiles, "week_" + weekKey + ".json");
	}

	private void DeleteOldGenerated(array<string> fileNames, string currentFileName) {
		if (fileNames.Count() <= KEEP_GENERATED_FILES) {
			return;
		}

		//! Absteigend sortiert: lexikographisch groesste = neueste (Keys fix formatiert)
		fileNames.Sort(true);
		for (int i = KEEP_GENERATED_FILES; i < fileNames.Count(); i++) {
			string fileName = fileNames.Get(i);
			if (fileName == currentFileName) {
				continue;
			}
			string filePath = DME_Tasks_Paths.GeneratedDir() + "/" + fileName;
			bool deleted = DeleteFile(filePath);
			if (deleted) {
				DME_Tasks_Log.Debug("Alte Generated-Datei geloescht: %1", filePath);
			} else {
				DME_Tasks_Log.Warn("Alte Generated-Datei konnte nicht geloescht werden: %1", filePath);
			}
		}
	}

	//! Kopie des EnumerateJsonFiles-Musters aus DME_Tasks_ConfigService (CONTRACTS §6.1):
	//! FindFile → erster Treffer schon im out-Param → FindNextFile-Schleife → CloseFindFile.
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

	// ==================================================================
	// Replace-Helfer
	// ==================================================================

	private bool HasCompletedQuest(DME_Tasks_PlayerState state, string questId) {
		foreach (DME_Tasks_CompletedQuest completed : state.CompletedQuests) {
			if (completed && completed.QuestId == questId) {
				return true;
			}
		}
		return false;
	}

	//! Anzahl bisheriger Replaces heute: Dailies sind nicht repeatable — Cooldown-Eintraege mit dem
	//! Tages-Prefix stammen ausschliesslich aus ReplaceDaily.
	private int CountCooldownsWithPrefix(DME_Tasks_PlayerState state, string prefix) {
		int count = 0;
		for (int i = 0; i < state.Cooldowns.Count(); i++) {
			string key = state.Cooldowns.GetKey(i);
			if (key.IndexOf(prefix) == 0) {
				count++;
			}
		}
		return count;
	}

	//! Naechste Mitternacht UTC als Epoch: CF_Date.Now(true) → Sekunden seit Tagesbeginn vom
	//! Now-Epoch abziehen + 86400 (CF_Date-API: GetHours/GetMinutes/GetSeconds/DateToEpoch).
	private int UtcMidnightEpoch() {
		CF_Date now = CF_Date.Now(true);
		int secondsToday = now.GetHours() * 3600 + now.GetMinutes() * 60 + now.GetSeconds();
		int nowEpoch = now.DateToEpoch();
		return nowEpoch - secondsToday + SECONDS_PER_DAY;
	}

	//! Voll-Re-Sync fuer den betroffenen Spieler (RequestSync braucht PlayerIdentity —
	//! FindPlayerByUid → GetIdentity → RequestSync, mit Null-Checks; offline: kein Sync noetig).
}
