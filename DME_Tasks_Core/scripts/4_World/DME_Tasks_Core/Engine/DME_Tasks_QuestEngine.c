//! Serverautoritative Quest-Engine (CONTRACTS §3.4 + §6.2) — Singleton, ausschliesslich Dedicated Server.
//! UID-Konvention (quellcode-verifiziert): ueberall `identity.GetId()` (gehashte SteamID, DB-/log-sicher,
//! gameplay.c:367). GetPlainId() (Steam64) ist beim Disconnect NICHT verfuegbar — vanilla missionServer.c:673
//! ruft PlayerDisconnected mit identity.GetId(), CF setzt CF_EventPlayerDisconnectedArgs.UID daraus.
//! Alle State-Uebergaenge laufen ueber DME_Tasks_QuestLifecycle; Belohnung NUR via ClaimReward.
class DME_Tasks_QuestEngine {
	private static ref DME_Tasks_QuestEngine s_DME_Instance;

	private static const float UPDATE_TICK_SECONDS = 1.0;
	//! QuestDef hat kein Cooldown-Feld — Default fuer Repeatable-Quests (Abandon/Complete); Welle 4 kann verfeinern.
	private static const int REPEATABLE_COOLDOWN_SECONDS = 3600;

	private float m_DME_UpdateAccumulator;
	private ref array<string> m_DME_OnlineUids;
	private ref array<string> m_DME_ExpiredScratch;

	static DME_Tasks_QuestEngine GetInstance() {
		if (!s_DME_Instance) {
			s_DME_Instance = new DME_Tasks_QuestEngine();
		}
		return s_DME_Instance;
	}

	void DME_Tasks_QuestEngine() {
		m_DME_UpdateAccumulator = 0.0;
		m_DME_OnlineUids = new array<string>();
		m_DME_ExpiredScratch = new array<string>();
	}

	// ==================================================================
	// Lifecycle (vom CoreModule)
	// ==================================================================

	void OnServerInit() {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		DME_Tasks_ConfigService configService = DME_Tasks_ConfigService.GetInstance();
		configService.LoadAll();

		DME_Tasks_Settings settings = configService.GetSettings();
		if (settings && settings.EnableDailyWeekly) {
			DME_Tasks_TaskGenerator.GetInstance().GenerateForToday();
		}

		//! Nach LoadAll + Generator, damit auch Runtime-Quests erfasst sind (§6.2 unlock-gated Set)
		DME_Tasks_PrereqEvaluator.GetInstance().RebuildUnlockGatedSet();

		DME_Tasks_Log.Info("QuestEngine initialisiert");
	}

	void OnPlayerConnect(PlayerBase player, PlayerIdentity identity) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!identity) {
			return;
		}

		string uid = identity.GetId();
		if (uid == "") {
			return;
		}

		DME_Tasks_PlayerState state = DME_Tasks_PlayerStore.GetInstance().LoadOrCreate(uid);
		if (!state) {
			DME_Tasks_Log.Error("OnPlayerConnect: LoadOrCreate fuer %1 fehlgeschlagen", uid);
			return;
		}

		if (state.SteamId == "") {
			state.SteamId = uid;
			DME_Tasks_PlayerStore.GetInstance().MarkDirty(uid);
		}

		if (m_DME_OnlineUids.Find(uid) == -1) {
			m_DME_OnlineUids.Insert(uid);
		}

		DME_Tasks_EngineEvents.s_DME_OnPlayerLoaded.Invoke(uid);

		//! Index-Rebuild: jede aktive Quest erneut ankuendigen (Objectives-PBO sortiert Refs ein)
		foreach (DME_Tasks_ActiveQuest active : state.ActiveQuests) {
			if (!active) {
				continue;
			}
			DME_Tasks_EngineEvents.s_DME_OnQuestActivated.Invoke(uid, active.QuestId);
		}

		DME_Tasks_Log.Info("Spieler %1 geladen (%2 aktive Quests)", uid, state.ActiveQuests.Count().ToString());
	}

	void OnPlayerDisconnect(string uid) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (uid == "") {
			return;
		}

		DME_Tasks_EngineEvents.s_DME_OnPlayerUnloaded.Invoke(uid);
		m_DME_OnlineUids.RemoveItem(uid);
		DME_Tasks_PlayerStore.GetInstance().Unload(uid);
		DME_Tasks_UnlockLedger.GetInstance().RemoveFor(uid);
	}

	//! 1-Sekunden-Akku: Expiry-Check + PlayerStore-Flush laufen NICHT per Frame.
	void OnUpdate(float deltaTime) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		m_DME_UpdateAccumulator += deltaTime;
		if (m_DME_UpdateAccumulator < UPDATE_TICK_SECONDS) {
			return;
		}

		float elapsed = m_DME_UpdateAccumulator;
		m_DME_UpdateAccumulator = 0.0;

		CheckExpiredQuests();
		DME_Tasks_PlayerStore.GetInstance().OnUpdate(elapsed);
	}

	//! §6.2: Welle-2-Death-Hook + ExpeditionService rufen dies; failt alle aktiven Quests mit FailOnDeath.
	void OnPlayerDied(string uid) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		DME_Tasks_PlayerState state = GetPlayerState(uid);
		if (!state) {
			return;
		}

		array<string> toFail = new array<string>();
		foreach (DME_Tasks_ActiveQuest active : state.ActiveQuests) {
			if (!active) {
				continue;
			}
			DME_Tasks_QuestDef def = GetQuestDef(active.QuestId);
			if (def && def.FailOnDeath) {
				toFail.Insert(active.QuestId);
			}
		}

		foreach (string failQuestId : toFail) {
			FailQuest(uid, failQuestId, "Du bist gestorben");
		}
	}

	// ==================================================================
	// RPC-Delegation (vom CoreModule; sender != null bereits geprueft)
	// ==================================================================

	//! Full-Sync: Traders (chunked pro Haendler) → QuestDefs (chunked pro Haendler/50) → PlayerState → SyncComplete.
	void RequestSync(PlayerIdentity sender) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!sender) {
			return;
		}

		string uid = sender.GetId();
		DME_Tasks_PlayerState state = GetPlayerState(uid);
		if (!state) {
			DME_Tasks_Log.Warn("RequestSync: PlayerState fuer %1 nicht geladen", uid);
			return;
		}

		SyncTraders(sender, state);
		SyncQuestDefs(sender, uid, state);
		SyncPlayerState(sender, state);
		DME_Tasks_RPC.SendSyncComplete(sender);
	}

	void AcceptQuest(PlayerIdentity sender, string questId) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!sender) {
			return;
		}

		string uid = sender.GetId();
		DME_Tasks_PlayerState state = GetPlayerState(uid);
		if (!state) {
			DME_Tasks_Log.Warn("AcceptQuest: PlayerState fuer %1 nicht geladen", uid);
			return;
		}

		DME_Tasks_QuestDef def = GetQuestDef(questId);
		if (!def) {
			NotifyPlayer(uid, "Quest", "Unbekannte Quest", EDME_Tasks_NotificationType.WARNING);
			return;
		}

		if (FindActiveQuest(state, questId)) {
			NotifyPlayer(uid, def.Title, "Quest ist bereits aktiv", EDME_Tasks_NotificationType.WARNING);
			return;
		}

		//! Deckt auch ab: nicht-repeatable bereits abgeschlossen, Cooldown, Unlock-Gating, alle Prereqs
		DME_Tasks_PrereqEvaluator evaluator = DME_Tasks_PrereqEvaluator.GetInstance();
		if (!evaluator.IsAvailable(uid, def)) {
			string lockReason = evaluator.GetLockReason(uid, def);
			NotifyPlayer(uid, def.Title, lockReason, EDME_Tasks_NotificationType.WARNING);
			return;
		}

		int now = DME_Tasks_TimeUtil.NowEpoch();

		DME_Tasks_ActiveQuest active = new DME_Tasks_ActiveQuest();
		active.QuestId = questId;
		active.State = EDME_Tasks_QuestState.ACTIVE;
		active.AcceptedAt = now;
		if (def.TimeLimit > 0) {
			active.ExpiresAt = now + def.TimeLimit;
		} else {
			active.ExpiresAt = -1;
		}

		foreach (DME_Tasks_ObjectiveDef objectiveDef : def.Objectives) {
			if (!objectiveDef) {
				continue;
			}
			DME_Tasks_ObjectiveProgress progress = new DME_Tasks_ObjectiveProgress();
			progress.ObjectiveId = objectiveDef.ObjectiveId;
			int required = objectiveDef.Amount;
			if (required < 1) {
				required = 1;
			}
			progress.Required = required;
			progress.Current = 0;
			progress.Done = false;
			active.Objectives.Insert(progress);
		}

		state.ActiveQuests.Insert(active);

		DME_Tasks_EngineEvents.s_DME_OnQuestActivated.Invoke(uid, questId);
		DME_Tasks_RPC.SendQuestStateChanged(sender, questId, EDME_Tasks_QuestState.ACTIVE);
		DME_Tasks_PlayerStore.GetInstance().MarkDirty(uid);
		NotifyPlayer(uid, def.Title, "Quest angenommen", EDME_Tasks_NotificationType.SUCCESS);
	}

	void AbandonQuest(PlayerIdentity sender, string questId) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!sender) {
			return;
		}

		string uid = sender.GetId();
		bool terminated = TerminateQuest(uid, questId, EDME_Tasks_QuestState.ABANDONED, "");
		if (!terminated) {
			NotifyPlayer(uid, "Quest", "Quest ist nicht aktiv", EDME_Tasks_NotificationType.WARNING);
			return;
		}

		DME_Tasks_QuestDef def = GetQuestDef(questId);
		if (def && def.Repeatable) {
			DME_Tasks_PlayerState state = GetPlayerState(uid);
			if (state) {
				int now = DME_Tasks_TimeUtil.NowEpoch();
				SetCooldown(state, questId, now);
			}
		}
	}

	//! §6.2: nur Validierung, idempotent — KEINE Transition (READY_TO_TURN_IN setzt die Engine automatisch,
	//! die COMPLETED-Transition macht erst ClaimReward).
	void RequestTurnIn(PlayerIdentity sender, string questId) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!sender) {
			return;
		}

		string uid = sender.GetId();
		DME_Tasks_ActiveQuest active = GetActiveQuest(uid, questId);
		if (!active) {
			NotifyPlayer(uid, "Quest", "Quest ist nicht aktiv", EDME_Tasks_NotificationType.WARNING);
			return;
		}

		string title = GetQuestTitle(questId);
		bool allDone = AreAllObjectivesDone(active);
		if (active.State == EDME_Tasks_QuestState.READY_TO_TURN_IN && allDone) {
			NotifyPlayer(uid, title, "Bereit zur Abgabe — Belohnung kann abgeholt werden", EDME_Tasks_NotificationType.SUCCESS);
		} else {
			NotifyPlayer(uid, title, "Noch nicht alle Ziele erfuellt", EDME_Tasks_NotificationType.WARNING);
		}
	}

	//! Validiert die Abgabe-Anfrage; die eigentliche Inventar-Logik lebt im Objectives-PBO (Welle 2)
	//! und ueberschreibt diese Methode via `modded class DME_Tasks_QuestEngine` (super zuerst — super
	//! macht NUR Validierung + Log, keine Transition; bei Rueckgabe ueber Notification informiert).
	void HandoverItems(PlayerIdentity sender, string questId, string objectiveId) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!sender) {
			return;
		}

		string uid = sender.GetId();
		DME_Tasks_ActiveQuest active = GetActiveQuest(uid, questId);
		if (!active) {
			NotifyPlayer(uid, "Quest", "Quest ist nicht aktiv", EDME_Tasks_NotificationType.WARNING);
			return;
		}

		DME_Tasks_ObjectiveDef objectiveDef = GetObjectiveDef(questId, objectiveId);
		if (!objectiveDef) {
			DME_Tasks_Log.Warn("HandoverItems: Objective %1/%2 unbekannt", questId, objectiveId);
			return;
		}

		DME_Tasks_ObjectiveProgress progress = FindObjectiveProgress(active, objectiveId);
		if (!progress || progress.Done) {
			NotifyPlayer(uid, GetQuestTitle(questId), "Nichts abzugeben", EDME_Tasks_NotificationType.INFO);
			return;
		}

		DME_Tasks_Log.Debug("HandoverItems angefordert: %1 %2/%3 (Handler folgt in Welle 2)", uid, questId, objectiveId);
	}

	void ClaimReward(PlayerIdentity sender, string questId) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!sender) {
			return;
		}

		string uid = sender.GetId();
		DME_Tasks_PlayerState state = GetPlayerState(uid);
		if (!state) {
			return;
		}

		DME_Tasks_ActiveQuest active = FindActiveQuest(state, questId);
		if (!active) {
			NotifyPlayer(uid, "Quest", "Quest ist nicht aktiv", EDME_Tasks_NotificationType.WARNING);
			return;
		}

		if (active.State != EDME_Tasks_QuestState.READY_TO_TURN_IN) {
			NotifyPlayer(uid, GetQuestTitle(questId), "Quest ist noch nicht bereit zur Abgabe", EDME_Tasks_NotificationType.WARNING);
			return;
		}

		DME_Tasks_QuestDef def = GetQuestDef(questId);
		if (!def) {
			DME_Tasks_Log.Error("ClaimReward: QuestDef %1 fehlt", questId);
			return;
		}

		PlayerBase player = FindPlayerByUid(uid);
		if (!player) {
			DME_Tasks_Log.Warn("ClaimReward: Spieler %1 nicht online", uid);
			return;
		}

		//! Agent 17 (§6.8): gespeicherte Decision "questId:choiceId" zur choiceId aufloesen — der
		//! RewardService zahlt dann Choice-Rewards + ReputationEffects statt der Standard-Rewards
		//! aus ("" = keine Entscheidung getroffen -> Standard-Quest-Rewards).
		string choiceId = DME_Tasks_ChoiceService.GetInstance().GetChoiceIdFor(state, questId);

		//! Idempotente Auszahlung (TxRecord im RewardService); true = jetzt oder frueher vollstaendig ausgezahlt
		bool claimed = DME_Tasks_RewardService.GetInstance().ClaimQuestReward(player, uid, def, choiceId);
		if (!claimed) {
			NotifyPlayer(uid, def.Title, "Belohnung konnte nicht ausgezahlt werden — bitte erneut versuchen", EDME_Tasks_NotificationType.ERROR);
			return;
		}

		bool applied = DME_Tasks_QuestLifecycle.ApplyState(active, EDME_Tasks_QuestState.COMPLETED);
		if (!applied) {
			return;
		}

		int now = DME_Tasks_TimeUtil.NowEpoch();
		DME_Tasks_CompletedQuest completedEntry = FindCompletedEntry(state, questId);
		if (completedEntry) {
			completedEntry.CompletedAt = now;
			completedEntry.TimesCompleted = completedEntry.TimesCompleted + 1;
		} else {
			completedEntry = new DME_Tasks_CompletedQuest();
			completedEntry.QuestId = questId;
			completedEntry.CompletedAt = now;
			completedEntry.TimesCompleted = 1;
			state.CompletedQuests.Insert(completedEntry);
		}

		RemoveActiveQuest(state, questId);
		state.TrackedQuests.RemoveItem(questId);
		if (def.Repeatable) {
			SetCooldown(state, questId, now);
		}

		DME_Tasks_UnlockLedger.GetInstance().ApplyUnlocks(uid, def.Unlocks);
		DME_Tasks_IntegrationEvents.s_DME_OnQuestCompleted.Invoke(uid, questId);
		DME_Tasks_EngineEvents.s_DME_OnQuestDeactivated.Invoke(uid, questId);

		DME_Tasks_RPC.SendQuestStateChanged(sender, questId, EDME_Tasks_QuestState.COMPLETED);

		//! Reward-Zusammenfassung fuer die UI (SPEC §5.2 RPC_RewardGranted; Payload = DME_Tasks_RewardDef-JSON,
		//! das Format erwartet der Client-Notifier). Choice ersetzt Standard-Rewards (CONTRACTS §6.4).
		DME_Tasks_RewardDef grantedRewards = def.Rewards;
		if (choiceId != "" && def.Choices) {
			foreach (DME_Tasks_ChoiceDef choiceDef : def.Choices) {
				if (choiceDef && choiceDef.ChoiceId == choiceId && choiceDef.Rewards) {
					grantedRewards = choiceDef.Rewards;
					break;
				}
			}
		}
		if (grantedRewards) {
			string rewardJson = DME_Tasks_Json<DME_Tasks_RewardDef>.ToJson(grantedRewards, "RewardGranted " + questId);
			if (rewardJson != "") {
				DME_Tasks_RPC.SendRewardGranted(sender, rewardJson);
			}
		}

		NotifyPlayer(uid, def.Title, "Quest abgeschlossen — Belohnung erhalten", EDME_Tasks_NotificationType.SUCCESS);

		//! §6.2: Verfuegbarkeiten neu berechnen (Unlocks koennen neue Quests freischalten) — Full-Re-Sync
		RequestSync(sender);
		DME_Tasks_PlayerStore.GetInstance().SaveNow(uid);
	}

	void SetTracked(PlayerIdentity sender, string csvQuestIds) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!sender) {
			return;
		}

		string uid = sender.GetId();
		DME_Tasks_PlayerState state = GetPlayerState(uid);
		if (!state) {
			return;
		}

		array<string> parts = new array<string>();
		csvQuestIds.Split(",", parts);

		array<string> accepted = new array<string>();
		foreach (string part : parts) {
			string trimmed = part.Trim();
			if (trimmed == "") {
				continue;
			}
			if (accepted.Find(trimmed) != -1) {
				continue;
			}
			if (!FindActiveQuest(state, trimmed)) {
				continue;
			}
			if (accepted.Count() >= DME_Tasks_Const.MAX_TRACKED_QUESTS) {
				break;
			}
			accepted.Insert(trimmed);
		}

		state.TrackedQuests.Clear();
		foreach (string trackedId : accepted) {
			state.TrackedQuests.Insert(trackedId);
		}

		DME_Tasks_PlayerStore.GetInstance().MarkDirty(uid);
	}

	void ReplaceDaily(PlayerIdentity sender, string questId) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!sender) {
			return;
		}

		string uid = sender.GetId();
		bool replaced = DME_Tasks_TaskGenerator.GetInstance().ReplaceDaily(uid, questId);
		if (replaced) {
			RequestSync(sender);
			NotifyPlayer(uid, "Tagesauftrag", "Auftrag ersetzt", EDME_Tasks_NotificationType.SUCCESS);
		} else {
			NotifyPlayer(uid, "Tagesauftrag", "Ersetzen derzeit nicht moeglich", EDME_Tasks_NotificationType.WARNING);
		}
	}

	void MakeChoice(PlayerIdentity sender, string questId, string choiceId) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!sender) {
			return;
		}

		string uid = sender.GetId();
		//! Agent 17 (§6.8): Notifications (Erfolg + spezifische Fehlgruende) sendet der ChoiceService
		//! selbst — hier keine zusaetzliche Notification (sonst doppelt). Sync-Refresh, damit der Client
		//! neu freigeschaltete/blockierte Quests (RequiredDecisions/BlockedByDecisions) sofort sieht.
		bool appliedChoice = DME_Tasks_ChoiceService.GetInstance().ApplyChoice(uid, questId, choiceId);
		if (appliedChoice) {
			RequestSync(sender);
		}
	}

	// ==================================================================
	// API fuer Objective-Handler (Objectives-PBO) & Services
	// ==================================================================

	DME_Tasks_PlayerState GetPlayerState(string uid) {
		return DME_Tasks_PlayerStore.GetInstance().Get(uid);
	}

	DME_Tasks_ActiveQuest GetActiveQuest(string uid, string questId) {
		DME_Tasks_PlayerState state = GetPlayerState(uid);
		if (!state) {
			return null;
		}
		return FindActiveQuest(state, questId);
	}

	DME_Tasks_QuestDef GetQuestDef(string questId) {
		return DME_Tasks_ConfigService.GetInstance().GetQuest(questId);
	}

	DME_Tasks_ObjectiveDef GetObjectiveDef(string questId, string objectiveId) {
		DME_Tasks_QuestDef def = GetQuestDef(questId);
		if (!def) {
			return null;
		}
		foreach (DME_Tasks_ObjectiveDef objectiveDef : def.Objectives) {
			if (objectiveDef && objectiveDef.ObjectiveId == objectiveId) {
				return objectiveDef;
			}
		}
		return null;
	}

	//! Klemmt auf [0..Required], setzt Done, prueft READY_TO_TURN_IN, sendet Delta-RPC, Dirty.
	void AddObjectiveProgress(string uid, string questId, string objectiveId, int delta) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		DME_Tasks_ActiveQuest active = GetActiveQuest(uid, questId);
		if (!active) {
			return;
		}

		DME_Tasks_ObjectiveProgress progress = FindObjectiveProgress(active, objectiveId);
		if (!progress) {
			DME_Tasks_Log.Warn("AddObjectiveProgress: Objective %1/%2 nicht gefunden", questId, objectiveId);
			return;
		}

		int newCurrent = progress.Current + delta;
		ApplyObjectiveProgress(uid, active, progress, newCurrent);
	}

	void SetObjectiveProgress(string uid, string questId, string objectiveId, int current) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		DME_Tasks_ActiveQuest active = GetActiveQuest(uid, questId);
		if (!active) {
			return;
		}

		DME_Tasks_ObjectiveProgress progress = FindObjectiveProgress(active, objectiveId);
		if (!progress) {
			DME_Tasks_Log.Warn("SetObjectiveProgress: Objective %1/%2 nicht gefunden", questId, objectiveId);
			return;
		}

		ApplyObjectiveProgress(uid, active, progress, current);
	}

	void FailQuest(string uid, string questId, string reason) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		TerminateQuest(uid, questId, EDME_Tasks_QuestState.FAILED, reason);
	}

	void NotifyPlayer(string uid, string title, string message, int type) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		PlayerIdentity identity = FindIdentityByUid(uid);
		if (!identity) {
			return;
		}
		DME_Tasks_RPC.SendNotification(identity, title, message, type);
	}

	//! Suche ueber die Online-Liste (g_Game.GetPlayers, verifiziert Game.c:947) — NIE Welt-Scan.
	PlayerBase FindPlayerByUid(string uid) {
		if (!g_Game) {
			return null;
		}
		if (uid == "") {
			return null;
		}

		array<Man> players = new array<Man>();
		g_Game.GetPlayers(players);
		foreach (Man man : players) {
			if (!man) {
				continue;
			}
			PlayerIdentity identity = man.GetIdentity();
			if (!identity) {
				continue;
			}
			if (identity.GetId() != uid) {
				continue;
			}
			PlayerBase player = PlayerBase.Cast(man);
			if (player) {
				return player;
			}
		}
		return null;
	}

	// ==================================================================
	// intern
	// ==================================================================

	private PlayerIdentity FindIdentityByUid(string uid) {
		PlayerBase player = FindPlayerByUid(uid);
		if (!player) {
			return null;
		}
		PlayerIdentity identity = player.GetIdentity();
		if (!identity) {
			return null;
		}
		return identity;
	}

	private void ApplyObjectiveProgress(string uid, DME_Tasks_ActiveQuest active, DME_Tasks_ObjectiveProgress progress, int newCurrent) {
		if (active.State != EDME_Tasks_QuestState.ACTIVE && active.State != EDME_Tasks_QuestState.READY_TO_TURN_IN) {
			return;
		}

		if (newCurrent < 0) {
			newCurrent = 0;
		}
		if (newCurrent > progress.Required) {
			newCurrent = progress.Required;
		}
		if (newCurrent == progress.Current) {
			return;
		}

		progress.Current = newCurrent;
		bool isDone = newCurrent >= progress.Required;
		progress.Done = isDone;

		PlayerIdentity identity = FindIdentityByUid(uid);
		if (identity) {
			DME_Tasks_RPC.SendObjectiveProgress(identity, active.QuestId, progress.ObjectiveId, newCurrent);
		}

		bool allDone = AreAllObjectivesDone(active);
		if (allDone && active.State == EDME_Tasks_QuestState.ACTIVE) {
			bool readyApplied = DME_Tasks_QuestLifecycle.ApplyState(active, EDME_Tasks_QuestState.READY_TO_TURN_IN);
			if (readyApplied) {
				if (identity) {
					DME_Tasks_RPC.SendQuestStateChanged(identity, active.QuestId, EDME_Tasks_QuestState.READY_TO_TURN_IN);
				}
				NotifyPlayer(uid, GetQuestTitle(active.QuestId), "Alle Ziele erfuellt — Quest beim Haendler abgeben", EDME_Tasks_NotificationType.SUCCESS);
			}
		} else if (!allDone && active.State == EDME_Tasks_QuestState.READY_TO_TURN_IN) {
			bool revertApplied = DME_Tasks_QuestLifecycle.ApplyState(active, EDME_Tasks_QuestState.ACTIVE);
			if (revertApplied && identity) {
				DME_Tasks_RPC.SendQuestStateChanged(identity, active.QuestId, EDME_Tasks_QuestState.ACTIVE);
			}
		}

		DME_Tasks_PlayerStore.GetInstance().MarkDirty(uid);
	}

	//! Gemeinsamer Fail-Pfad fuer FAILED / EXPIRED / ABANDONED.
	private bool TerminateQuest(string uid, string questId, int terminalState, string reason) {
		DME_Tasks_PlayerState state = GetPlayerState(uid);
		if (!state) {
			return false;
		}

		DME_Tasks_ActiveQuest active = FindActiveQuest(state, questId);
		if (!active) {
			return false;
		}

		bool applied = DME_Tasks_QuestLifecycle.ApplyState(active, terminalState);
		if (!applied) {
			return false;
		}

		RemoveActiveQuest(state, questId);
		state.TrackedQuests.RemoveItem(questId);

		DME_Tasks_EngineEvents.s_DME_OnQuestDeactivated.Invoke(uid, questId);

		PlayerIdentity identity = FindIdentityByUid(uid);
		if (identity) {
			DME_Tasks_RPC.SendQuestStateChanged(identity, questId, terminalState);
		}

		string title = GetQuestTitle(questId);
		if (terminalState == EDME_Tasks_QuestState.FAILED) {
			string failMessage = "Quest fehlgeschlagen";
			if (reason != "") {
				failMessage = failMessage + ": " + reason;
			}
			NotifyPlayer(uid, title, failMessage, EDME_Tasks_NotificationType.ERROR);
		} else if (terminalState == EDME_Tasks_QuestState.EXPIRED) {
			NotifyPlayer(uid, title, "Zeitlimit abgelaufen — Quest fehlgeschlagen", EDME_Tasks_NotificationType.WARNING);
		} else if (terminalState == EDME_Tasks_QuestState.ABANDONED) {
			NotifyPlayer(uid, title, "Quest abgebrochen", EDME_Tasks_NotificationType.INFO);
		}

		DME_Tasks_PlayerStore.GetInstance().MarkDirty(uid);
		return true;
	}

	//! 1-Hz-Sweep ueber die GELADENEN Online-Spieler: ExpiresAt > 0 && Now >= ExpiresAt → EXPIRED (Fail-Pfad).
	private void CheckExpiredQuests() {
		if (m_DME_OnlineUids.Count() == 0) {
			return;
		}

		int now = DME_Tasks_TimeUtil.NowEpoch();
		for (int i = 0; i < m_DME_OnlineUids.Count(); i++) {
			string uid = m_DME_OnlineUids.Get(i);
			DME_Tasks_PlayerState state = DME_Tasks_PlayerStore.GetInstance().Get(uid);
			if (!state) {
				continue;
			}

			m_DME_ExpiredScratch.Clear();
			foreach (DME_Tasks_ActiveQuest active : state.ActiveQuests) {
				if (!active) {
					continue;
				}
				if (active.ExpiresAt > 0 && now >= active.ExpiresAt) {
					m_DME_ExpiredScratch.Insert(active.QuestId);
				}
			}

			foreach (string expiredQuestId : m_DME_ExpiredScratch) {
				TerminateQuest(uid, expiredQuestId, EDME_Tasks_QuestState.EXPIRED, "");
			}
		}
	}

	private void SetCooldown(DME_Tasks_PlayerState state, string questId, int now) {
		int until = now + REPEATABLE_COOLDOWN_SECONDS;
		state.Cooldowns.Set(questId, until);
	}

	private DME_Tasks_ActiveQuest FindActiveQuest(DME_Tasks_PlayerState state, string questId) {
		foreach (DME_Tasks_ActiveQuest active : state.ActiveQuests) {
			if (active && active.QuestId == questId) {
				return active;
			}
		}
		return null;
	}

	private DME_Tasks_ObjectiveProgress FindObjectiveProgress(DME_Tasks_ActiveQuest active, string objectiveId) {
		foreach (DME_Tasks_ObjectiveProgress progress : active.Objectives) {
			if (progress && progress.ObjectiveId == objectiveId) {
				return progress;
			}
		}
		return null;
	}

	private DME_Tasks_CompletedQuest FindCompletedEntry(DME_Tasks_PlayerState state, string questId) {
		foreach (DME_Tasks_CompletedQuest completed : state.CompletedQuests) {
			if (completed && completed.QuestId == questId) {
				return completed;
			}
		}
		return null;
	}

	private bool HasCompleted(DME_Tasks_PlayerState state, string questId) {
		DME_Tasks_CompletedQuest completed = FindCompletedEntry(state, questId);
		return completed != null;
	}

	private void RemoveActiveQuest(DME_Tasks_PlayerState state, string questId) {
		for (int i = state.ActiveQuests.Count() - 1; i >= 0; i--) {
			DME_Tasks_ActiveQuest active = state.ActiveQuests.Get(i);
			if (active && active.QuestId == questId) {
				state.ActiveQuests.RemoveOrdered(i);
			}
		}
	}

	private bool AreAllObjectivesDone(DME_Tasks_ActiveQuest active) {
		if (active.Objectives.Count() == 0) {
			return false;
		}
		foreach (DME_Tasks_ObjectiveProgress progress : active.Objectives) {
			if (!progress || !progress.Done) {
				return false;
			}
		}
		return true;
	}

	private string GetQuestTitle(string questId) {
		DME_Tasks_QuestDef def = GetQuestDef(questId);
		if (def && def.Title != "") {
			return def.Title;
		}
		return questId;
	}

	// ------------------------------------------------------------------
	// Full-Sync-Aufbau (RequestSync)
	// ------------------------------------------------------------------

	private void SyncTraders(PlayerIdentity sender, DME_Tasks_PlayerState state) {
		array<ref DME_Tasks_TraderDef> traders = DME_Tasks_ConfigService.GetInstance().GetTraders();

		DME_Tasks_TraderSyncList fullList = new DME_Tasks_TraderSyncList();
		foreach (DME_Tasks_TraderDef traderDef : traders) {
			if (!traderDef) {
				continue;
			}
			DME_Tasks_TraderSyncEntry entry = BuildTraderEntry(traderDef, state);
			fullList.Traders.Insert(entry);
		}

		string json = DME_Tasks_Json<DME_Tasks_TraderSyncList>.ToJson(fullList, "TraderSyncList");
		if (json == "") {
			return;
		}

		bool fitsInOne = json.Length() <= DME_Tasks_Const.RPC_CHUNK_MAX_BYTES;
		if (fitsInOne || fullList.Traders.Count() <= 1) {
			DME_Tasks_RPC.SendSyncTraders(sender, json);
			return;
		}

		//! Chunking-Protokoll (SPEC §5.2): > 2 KB → eine RPC pro Haendler
		foreach (DME_Tasks_TraderSyncEntry chunkEntry : fullList.Traders) {
			DME_Tasks_TraderSyncList chunk = new DME_Tasks_TraderSyncList();
			chunk.Traders.Insert(chunkEntry);
			string chunkJson = DME_Tasks_Json<DME_Tasks_TraderSyncList>.ToJson(chunk, "TraderSyncList-Chunk");
			if (chunkJson != "") {
				DME_Tasks_RPC.SendSyncTraders(sender, chunkJson);
			}
		}
	}

	private DME_Tasks_TraderSyncEntry BuildTraderEntry(DME_Tasks_TraderDef traderDef, DME_Tasks_PlayerState state) {
		DME_Tasks_TraderSyncEntry entry = new DME_Tasks_TraderSyncEntry();
		entry.TraderId = traderDef.TraderId;
		entry.DisplayName = traderDef.DisplayName;
		entry.Faction = traderDef.Faction;
		entry.LoyaltyLevels = traderDef.LoyaltyLevels;

		DME_Tasks_TraderProgress traderProgress = state.TraderProgress.Get(traderDef.TraderId);
		if (traderProgress) {
			entry.Reputation = traderProgress.Reputation;
			entry.Turnover = traderProgress.Turnover;
			entry.LoyaltyLevel = traderProgress.LoyaltyLevel;
		}
		return entry;
	}

	private void SyncQuestDefs(PlayerIdentity sender, string uid, DME_Tasks_PlayerState state) {
		array<ref DME_Tasks_TraderDef> traders = DME_Tasks_ConfigService.GetInstance().GetTraders();
		foreach (DME_Tasks_TraderDef traderDef : traders) {
			if (!traderDef) {
				continue;
			}
			SyncQuestDefsForTrader(sender, uid, state, traderDef.TraderId);
		}
	}

	//! Chunking: max QUESTS_PER_SYNC_CHUNK pro RPC; ist der Chunk-JSON > RPC_CHUNK_MAX_BYTES,
	//! wird die Chunk-Groesse halbiert (bis minimal 1 Quest pro RPC).
	private void SyncQuestDefsForTrader(PlayerIdentity sender, string uid, DME_Tasks_PlayerState state, string traderId) {
		array<ref DME_Tasks_QuestDef> quests = DME_Tasks_ConfigService.GetInstance().GetQuestsForTrader(traderId);
		if (quests.Count() == 0) {
			return;
		}

		array<ref DME_Tasks_QuestSyncEntry> entries = new array<ref DME_Tasks_QuestSyncEntry>();
		foreach (DME_Tasks_QuestDef def : quests) {
			if (!def) {
				continue;
			}
			DME_Tasks_QuestSyncEntry entry = BuildQuestEntry(uid, state, def);
			entries.Insert(entry);
		}

		int index = 0;
		int total = entries.Count();
		while (index < total) {
			int take = DME_Tasks_Const.QUESTS_PER_SYNC_CHUNK;
			int remaining = total - index;
			if (take > remaining) {
				take = remaining;
			}

			string json = "";
			while (take >= 1) {
				DME_Tasks_QuestSyncChunk chunk = BuildQuestChunk(traderId, entries, index, take);
				json = DME_Tasks_Json<DME_Tasks_QuestSyncChunk>.ToJson(chunk, "QuestSyncChunk " + traderId);
				if (json == "") {
					return;
				}
				bool fits = json.Length() <= DME_Tasks_Const.RPC_CHUNK_MAX_BYTES;
				if (fits || take == 1) {
					break;
				}
				take = take / 2;
			}

			DME_Tasks_RPC.SendSyncQuestDefs(sender, traderId, json);
			index += take;
		}
	}

	private DME_Tasks_QuestSyncChunk BuildQuestChunk(string traderId, array<ref DME_Tasks_QuestSyncEntry> entries, int startIndex, int count) {
		DME_Tasks_QuestSyncChunk chunk = new DME_Tasks_QuestSyncChunk();
		chunk.TraderId = traderId;
		for (int i = 0; i < count; i++) {
			DME_Tasks_QuestSyncEntry entry = entries.Get(startIndex + i);
			chunk.Quests.Insert(entry);
		}
		return chunk;
	}

	private DME_Tasks_QuestSyncEntry BuildQuestEntry(string uid, DME_Tasks_PlayerState state, DME_Tasks_QuestDef def) {
		DME_Tasks_QuestSyncEntry entry = new DME_Tasks_QuestSyncEntry();
		entry.QuestId = def.QuestId;
		entry.TraderId = def.TraderId;
		entry.Category = def.Category;
		entry.Title = def.Title;
		entry.Description = def.Description;
		entry.Repeatable = def.Repeatable;
		entry.FailOnDeath = def.FailOnDeath;
		entry.FailOnFactionChange = def.FailOnFactionChange;
		entry.TimeLimit = def.TimeLimit;
		entry.Objectives = def.Objectives;
		entry.Rewards = def.Rewards;
		entry.Choices = def.Choices;

		entry.State = ComputeQuestStateFor(uid, state, def);
		if (entry.State == EDME_Tasks_QuestState.LOCKED) {
			entry.LockReason = DME_Tasks_PrereqEvaluator.GetInstance().GetLockReason(uid, def);
		}
		return entry;
	}

	//! Berechneter per-Spieler-State fuer die Def-Projektion (ACTIVE/READY direkt, sonst COMPLETED/COOLDOWN/AVAILABLE/LOCKED).
	private int ComputeQuestStateFor(string uid, DME_Tasks_PlayerState state, DME_Tasks_QuestDef def) {
		DME_Tasks_ActiveQuest active = FindActiveQuest(state, def.QuestId);
		if (active) {
			return active.State;
		}

		if (!def.Repeatable && HasCompleted(state, def.QuestId)) {
			return EDME_Tasks_QuestState.COMPLETED;
		}

		int cooldownUntil;
		if (state.Cooldowns.Find(def.QuestId, cooldownUntil)) {
			int now = DME_Tasks_TimeUtil.NowEpoch();
			if (now < cooldownUntil) {
				return EDME_Tasks_QuestState.COOLDOWN;
			}
		}

		if (DME_Tasks_PrereqEvaluator.GetInstance().IsAvailable(uid, def)) {
			return EDME_Tasks_QuestState.AVAILABLE;
		}
		return EDME_Tasks_QuestState.LOCKED;
	}

	private void SyncPlayerState(PlayerIdentity sender, DME_Tasks_PlayerState state) {
		string json = DME_Tasks_Json<DME_Tasks_PlayerState>.ToJson(state, "PlayerState");
		if (json != "") {
			DME_Tasks_RPC.SendSyncPlayerState(sender, json);
		}
	}
}
