//! Prueft ALLE Prerequisites (DME_Tasks_Prereqs) einer Quest fuer einen Spieler — server-only.
//! Optionale Provider-Checks (Skill/Season/Boss) sind protected und default-erfuellt (+ einmaliges Warn,
//! wenn konfiguriert aber kein Provider aktiv) — Welle-4-Integrations ueberschreiben via `modded class DME_Tasks_PrereqEvaluator`.
//! CheckFaction laeuft seit Welle 4 (Agent 17) echt gegen DME_Tasks_FactionService (Default "NEUTRAL").
class DME_Tasks_PrereqEvaluator {
	private static ref DME_Tasks_PrereqEvaluator s_DME_Instance;

	//! Quest-Ids, die in irgendeinem UnlockDef.QuestIds vorkommen ("unlock-gated", CONTRACTS §6.2)
	private ref map<string, bool> m_DME_UnlockGatedQuests;
	private bool m_DME_WarnedSkillProvider;
	private bool m_DME_WarnedSeasonProvider;
	private bool m_DME_WarnedBossProvider;

	static DME_Tasks_PrereqEvaluator GetInstance() {
		if (!s_DME_Instance) {
			s_DME_Instance = new DME_Tasks_PrereqEvaluator();
		}
		return s_DME_Instance;
	}

	void DME_Tasks_PrereqEvaluator() {
		m_DME_UnlockGatedQuests = new map<string, bool>();
		m_DME_WarnedSkillProvider = false;
		m_DME_WarnedSeasonProvider = false;
		m_DME_WarnedBossProvider = false;
	}

	//! Von QuestEngine.OnServerInit gerufen (nach ConfigService.LoadAll + TaskGenerator):
	//! sammelt alle Quest-Ids aus QuestDef.Unlocks.QuestIds und ChoiceDef.Unlocks.QuestIds.
	void RebuildUnlockGatedSet() {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		m_DME_UnlockGatedQuests.Clear();

		array<ref DME_Tasks_QuestDef> quests = DME_Tasks_ConfigService.GetInstance().GetQuests();
		foreach (DME_Tasks_QuestDef def : quests) {
			if (!def) {
				continue;
			}
			CollectUnlockGated(def.Unlocks);
			foreach (DME_Tasks_ChoiceDef choice : def.Choices) {
				if (choice) {
					CollectUnlockGated(choice.Unlocks);
				}
			}
		}

		DME_Tasks_Log.Info("PrereqEvaluator: %1 unlock-gated Quests erfasst", m_DME_UnlockGatedQuests.Count().ToString());
	}

	bool IsAvailable(string uid, DME_Tasks_QuestDef def) {
		string unusedP1;
		string unusedP2;
		string reason = Evaluate(uid, def, unusedP1, unusedP2);
		return reason == "";
	}

	//! "" = available. Otherwise a stringtable key following the '#' rule (DME_Tasks_LocKeys
	//! constant, e.g. "#STR_DME_TASKS_LOCK_*") resolved on the client, with p1/p2 filling %1/%2.
	//! Server-only: never resolves the key itself.
	string GetLockReasonKey(string uid, DME_Tasks_QuestDef def, out string p1, out string p2) {
		return Evaluate(uid, def, p1, p2);
	}

	// ------------------------------------------------------------------
	// intern
	// ------------------------------------------------------------------

	//! Runs all prerequisite checks in order and returns "" (available) or the DME_Tasks_LocKeys
	//! constant ("#STR_DME_TASKS_..." stringtable key) for the first failing check. p1/p2 carry the
	//! %1/%2 placeholder values (only LOCK_LEVEL_REQUIRED and LOCK_TRADER_LEVEL_REQUIRED use p1).
	protected string Evaluate(string uid, DME_Tasks_QuestDef def, out string p1, out string p2) {
		p1 = "";
		p2 = "";

		if (!def) {
			return DME_Tasks_LocKeys.NOTIF_UNKNOWN_QUEST;
		}

		DME_Tasks_PlayerState state = DME_Tasks_PlayerStore.GetInstance().Get(uid);
		if (!state) {
			return DME_Tasks_LocKeys.LOCK_STATE_NOT_LOADED;
		}

		if (!def.Repeatable && HasCompleted(state, def.QuestId)) {
			return DME_Tasks_LocKeys.LOCK_ALREADY_COMPLETED;
		}

		int cooldownUntil;
		if (state.Cooldowns.Find(def.QuestId, cooldownUntil)) {
			int now = DME_Tasks_TimeUtil.NowEpoch();
			if (now < cooldownUntil) {
				return DME_Tasks_LocKeys.LOCK_COOLDOWN;
			}
		}

		//! Unlock-Gating (§6.2): unlock-gated Quests sind nur mit Freischaltung verfuegbar
		if (m_DME_UnlockGatedQuests.Contains(def.QuestId)) {
			if (!DME_Tasks_UnlockLedger.GetInstance().IsQuestUnlocked(uid, def.QuestId)) {
				return DME_Tasks_LocKeys.LOCK_NOT_UNLOCKED;
			}
		}

		DME_Tasks_Prereqs prereqs = def.Prerequisites;
		if (!prereqs) {
			return "";
		}

		if (prereqs.MinimumPlayerLevel > 0 && state.PlayerLevel < prereqs.MinimumPlayerLevel) {
			p1 = prereqs.MinimumPlayerLevel.ToString();
			return DME_Tasks_LocKeys.LOCK_LEVEL_REQUIRED;
		}

		if (!CheckFaction(uid, prereqs)) {
			return DME_Tasks_LocKeys.LOCK_WRONG_FACTION;
		}

		if (prereqs.MinimumTraderReputation > 0) {
			float reputation = DME_Tasks_TraderService.GetInstance().GetReputation(uid, def.TraderId);
			if (reputation < prereqs.MinimumTraderReputation) {
				return DME_Tasks_LocKeys.LOCK_REPUTATION_LOW;
			}
		}

		if (prereqs.RequiredTraderLevel > 0) {
			int loyaltyLevel = DME_Tasks_TraderService.GetInstance().GetLoyaltyLevel(uid, def.TraderId);
			if (loyaltyLevel < prereqs.RequiredTraderLevel) {
				p1 = prereqs.RequiredTraderLevel.ToString();
				return DME_Tasks_LocKeys.LOCK_TRADER_LEVEL_REQUIRED;
			}
		}

		foreach (string requiredQuestId : prereqs.RequiredCompletedQuests) {
			if (requiredQuestId == "") {
				continue;
			}
			if (!HasCompleted(state, requiredQuestId)) {
				return DME_Tasks_LocKeys.LOCK_PREV_QUEST;
			}
		}

		foreach (string blockedQuestId : prereqs.BlockedByCompletedQuests) {
			if (blockedQuestId == "") {
				continue;
			}
			if (HasCompleted(state, blockedQuestId)) {
				return DME_Tasks_LocKeys.LOCK_BLOCKED_QUESTLINE;
			}
		}

		//! Agent 17 (§6.8/Konzept §14): Eintraege "questId:choiceId" gegen PlayerState.Decisions pruefen
		foreach (string requiredDecision : prereqs.RequiredDecisions) {
			if (requiredDecision == "") {
				continue;
			}
			if (state.Decisions.Find(requiredDecision) == -1) {
				return DME_Tasks_LocKeys.LOCK_DECISION_MISSING;
			}
		}

		foreach (string blockedDecision : prereqs.BlockedByDecisions) {
			if (blockedDecision == "") {
				continue;
			}
			if (state.Decisions.Find(blockedDecision) != -1) {
				return DME_Tasks_LocKeys.LOCK_BLOCKED_DECISION;
			}
		}

		if (!CheckSkillRequirement(uid, prereqs)) {
			return DME_Tasks_LocKeys.LOCK_SKILL;
		}

		if (!CheckSeasonLevel(uid, prereqs)) {
			return DME_Tasks_LocKeys.LOCK_SEASON_LEVEL;
		}

		if (!CheckBossProgress(uid, prereqs)) {
			return DME_Tasks_LocKeys.LOCK_BOSS_PROGRESS;
		}

		if (prereqs.RequiredItem != "" && !HasRequiredItem(uid, prereqs.RequiredItem)) {
			return DME_Tasks_LocKeys.LOCK_ITEM_MISSING;
		}

		if (!CheckTimeWindow(prereqs.FromHour, prereqs.ToHour)) {
			return DME_Tasks_LocKeys.LOCK_TIME_WINDOW;
		}

		return "";
	}

	protected bool HasCompleted(DME_Tasks_PlayerState state, string questId) {
		foreach (DME_Tasks_CompletedQuest completed : state.CompletedQuests) {
			if (completed && completed.QuestId == questId) {
				return true;
			}
		}
		return false;
	}

	protected void CollectUnlockGated(DME_Tasks_UnlockDef unlocks) {
		if (!unlocks || !unlocks.QuestIds) {
			return;
		}
		foreach (string questId : unlocks.QuestIds) {
			if (questId != "") {
				m_DME_UnlockGatedQuests.Set(questId, true);
			}
		}
	}

	//! Tageszeit-Fenster via In-Game-Welt-Zeit (World.GetDate, verifiziert scripts - 1.29\3_Game\DayZ\Global\World.c:33).
	//! FromHour > ToHour = Fenster ueber Mitternacht (z.B. 22 → 4). -1 = kein Fenster.
	protected bool CheckTimeWindow(int fromHour, int toHour) {
		if (fromHour < 0 || toHour < 0) {
			return true;
		}
		if (fromHour == toHour) {
			return true;
		}
		if (!g_Game) {
			return true;
		}

		World world = g_Game.GetWorld();
		if (!world) {
			return true;
		}

		int year;
		int month;
		int day;
		int hour;
		int minute;
		world.GetDate(year, month, day, hour, minute);

		if (fromHour < toHour) {
			return hour >= fromHour && hour < toHour;
		}
		return hour >= fromHour || hour < toHour;
	}

	//! Inventar-Check nur wenn Spieler online (SPEC §4.4); offline → Check uebersprungen.
	protected bool HasRequiredItem(string uid, string requiredItem) {
		PlayerBase player = DME_Tasks_QuestEngine.GetInstance().FindPlayerByUid(uid);
		if (!player) {
			return true;
		}

		GameInventory inventory = player.GetInventory();
		if (!inventory) {
			return false;
		}

		array<EntityAI> items = new array<EntityAI>();
		inventory.EnumerateInventory(InventoryTraversalType.LEVELORDER, items);

		string wanted = requiredItem;
		wanted.ToLower();
		foreach (EntityAI item : items) {
			if (!item) {
				continue;
			}
			string typeName = item.GetType();
			typeName.ToLower();
			if (typeName == wanted) {
				return true;
			}
		}
		return false;
	}

	// ------------------------------------------------------------------
	// Optionale Provider-Checks (§6.2) — Default erfuellt + einmaliges Warn.
	// Welle-4-Integrations ueberschreiben via `modded class DME_Tasks_PrereqEvaluator` (override + super-Logik beachten).
	// ------------------------------------------------------------------

	protected bool CheckSkillRequirement(string uid, DME_Tasks_Prereqs prereqs) {
		if (!prereqs.RequiredSkills || prereqs.RequiredSkills.Count() == 0) {
			return true;
		}
		if (!m_DME_WarnedSkillProvider) {
			m_DME_WarnedSkillProvider = true;
			DME_Tasks_Log.Warn("Quest verlangt RequiredSkills, aber kein Skill-Provider aktiv — Check gilt als erfuellt");
		}
		return true;
	}

	protected bool CheckSeasonLevel(string uid, DME_Tasks_Prereqs prereqs) {
		if (prereqs.RequiredSeasonLevel <= 0) {
			return true;
		}
		if (!m_DME_WarnedSeasonProvider) {
			m_DME_WarnedSeasonProvider = true;
			DME_Tasks_Log.Warn("Quest verlangt RequiredSeasonLevel, aber kein Season-Provider aktiv — Check gilt als erfuellt");
		}
		return true;
	}

	protected bool CheckBossProgress(string uid, DME_Tasks_Prereqs prereqs) {
		if (prereqs.RequiredBossProgress == "") {
			return true;
		}
		if (!m_DME_WarnedBossProvider) {
			m_DME_WarnedBossProvider = true;
			DME_Tasks_Log.Warn("Quest verlangt RequiredBossProgress, aber kein Boss-Provider aktiv — Check gilt als erfuellt");
		}
		return true;
	}

	//! Agent 17 (§6.8): echter Check gegen den FactionService (Default "NEUTRAL"; Integrations
	//! liefern die Quelle via modded DME_Tasks_FactionService) — Warn-once-Pfad entfaellt.
	protected bool CheckFaction(string uid, DME_Tasks_Prereqs prereqs) {
		if (prereqs.RequiredFaction == "") {
			return true;
		}
		string faction = DME_Tasks_FactionService.GetInstance().GetFaction(uid);
		return faction == prereqs.RequiredFaction;
	}
}
