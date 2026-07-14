//! Idempotente Reward-Auszahlung nach Konzept §13 (server-only).
//! Ablauf (CONTRACTS §6.2): TxId = uid + ":" + questId + ":claim";
//! bereits COMPLETED -> true (Doppel-RPC-sicher); sonst PENDING-Reservierung + SaveNow,
//! dann auszahlen, dann COMPLETED + SaveNow. Crash zwischen Reservierung und COMPLETED:
//! naechster Claim findet den PENDING-Record und zahlt erneut aus (at-least-once).
//!
//! Design-Entscheidungen (dokumentiert):
//! - Choice ERSETZT Quest-Rewards: bei nicht-leerer choiceId werden ChoiceDef.Rewards
//!   statt QuestDef.Rewards ausgezahlt; zusaetzlich werden ChoiceDef.ReputationEffects angewendet.
//!   Unbekannte choiceId -> Warn + Standard-Quest-Rewards.
//! - Level-Up simpel: PlayerLevel = 1 + Experience / XP_PER_LEVEL (lokale Konstante 10000,
//!   bewusst NICHT in DME_Tasks_Const); Level sinkt nie.
//! - Currency laeuft NUR ueber DME_Tasks_IntegrationEvents.s_DME_OnGrantCurrency (kein Item-Spawn);
//!   ohne Abonnent bleibt es beim Info-Log (§6.2).
//! - player == null (offline): Items werden NICHT gespawnt (Warn), alle uebrigen Rewards
//!   (XP/Currency/Rep/Skill/SeasonXp) werden trotzdem ausgezahlt.
//! - ItemReward.Amount > 1 spawnt Amount separate Instanzen (kein Stacking).
//! - Volles Inventar: Fallback g_Game.CreateObjectEx(className, Spielerposition, ECE_PLACE_ON_SURFACE).
class DME_Tasks_RewardService {
	private static ref DME_Tasks_RewardService s_DME_Instance;

	//! Simples Level-Up: 10000 XP pro Level (Konzept §13; bewusst lokal statt DME_Tasks_Const).
	private static const int XP_PER_LEVEL = 10000;

	static DME_Tasks_RewardService GetInstance() {
		if (!s_DME_Instance) {
			s_DME_Instance = new DME_Tasks_RewardService();
		}
		return s_DME_Instance;
	}

	//! true = jetzt oder bereits frueher vollstaendig ausgezahlt.
	bool ClaimQuestReward(PlayerBase player, string uid, DME_Tasks_QuestDef quest, string choiceId) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return false;
		}
		if (!quest || uid == "") {
			DME_Tasks_Log.Error("RewardService.ClaimQuestReward: quest null oder uid leer");
			return false;
		}

		DME_Tasks_PlayerState state = DME_Tasks_PlayerStore.GetInstance().Get(uid);
		if (!state) {
			state = DME_Tasks_PlayerStore.GetInstance().LoadOrCreate(uid);
		}
		if (!state) {
			DME_Tasks_Log.Error("RewardService.ClaimQuestReward: kein PlayerState fuer %1", uid);
			return false;
		}

		// Wiederholungszaehler in der TxId: TimesCompleted wird von der Engine erst NACH
		// erfolgreichem Claim inkrementiert — Doppel-RPCs/Crash-Retries desselben Claims
		// treffen dieselbe TxId (idempotent), der naechste Repeatable-Durchlauf eine neue.
		int claimCycle = 0;
		DME_Tasks_CompletedQuest completedEntry = FindCompletedEntry(state, quest.QuestId);
		if (completedEntry) {
			claimCycle = completedEntry.TimesCompleted;
		}
		string txId = uid + ":" + quest.QuestId + ":claim:" + claimCycle.ToString();
		if (DME_Tasks_Transaction.HasCompleted(state, txId)) {
			DME_Tasks_Log.Info("Reward fuer %1 bereits ausgezahlt (Tx %2) — Doppel-RPC ignoriert", quest.QuestId, txId);
			return true;
		}

		DME_Tasks_TxRecord tx = DME_Tasks_Transaction.BeginOrGet(state, txId, quest.QuestId);
		if (!tx) {
			return false;
		}
		// Reservierung persistieren, BEVOR ausgezahlt wird (Konzept §13).
		DME_Tasks_PlayerStore.GetInstance().SaveNow(uid);

		// Effektive Rewards bestimmen: Choice ERSETZT Quest-Rewards (siehe Kopf-Doku).
		DME_Tasks_RewardDef rewards = quest.Rewards;
		DME_Tasks_ChoiceDef choice = null;
		if (choiceId != "") {
			choice = FindChoice(quest, choiceId);
			if (choice) {
				rewards = choice.Rewards;
			} else {
				DME_Tasks_Log.Warn("ClaimQuestReward: Choice %1 nicht in Quest %2 — Standard-Rewards", choiceId, quest.QuestId);
			}
		}

		PayoutRewards(player, uid, state, quest, rewards);
		if (choice) {
			ApplyReputationEffects(uid, choice.ReputationEffects);
		}

		DME_Tasks_Transaction.Complete(state, txId);
		DME_Tasks_PlayerStore.GetInstance().SaveNow(uid);

		DME_Tasks_Log.Info("Reward fuer Quest %1 an %2 ausgezahlt (Tx %3)", quest.QuestId, uid, txId);
		return true;
	}

	// ------------------------------------------------------------------
	// intern
	// ------------------------------------------------------------------

	private DME_Tasks_CompletedQuest FindCompletedEntry(DME_Tasks_PlayerState state, string questId) {
		if (!state.CompletedQuests) {
			return null;
		}
		foreach (DME_Tasks_CompletedQuest completed : state.CompletedQuests) {
			if (completed && completed.QuestId == questId) {
				return completed;
			}
		}
		return null;
	}

	private DME_Tasks_ChoiceDef FindChoice(DME_Tasks_QuestDef quest, string choiceId) {
		if (!quest.Choices) {
			return null;
		}
		foreach (DME_Tasks_ChoiceDef choiceDef : quest.Choices) {
			if (choiceDef && choiceDef.ChoiceId == choiceId) {
				return choiceDef;
			}
		}
		return null;
	}

	private void PayoutRewards(PlayerBase player, string uid, DME_Tasks_PlayerState state, DME_Tasks_QuestDef quest, DME_Tasks_RewardDef rewards) {
		if (!rewards) {
			DME_Tasks_Log.Warn("PayoutRewards: Quest %1 ohne RewardDef — nichts auszuzahlen", quest.QuestId);
			return;
		}

		if (rewards.PlayerExperience > 0) {
			GrantExperience(state, rewards.PlayerExperience);
		}

		if (rewards.Currency > 0) {
			DME_Tasks_IntegrationEvents.s_DME_OnGrantCurrency.Invoke(uid, rewards.Currency);
			DME_Tasks_Log.Info("Currency-Reward %1 fuer %2 via Invoker publiziert (Market-Adapter zahlt aus)", rewards.Currency.ToString(), uid);
		}

		if (rewards.TraderReputation != 0) {
			if (quest.TraderId != "") {
				DME_Tasks_TraderService.GetInstance().AddReputation(uid, quest.TraderId, rewards.TraderReputation);
			} else {
				DME_Tasks_Log.Warn("PayoutRewards: Quest %1 hat TraderReputation aber keine TraderId", quest.QuestId);
			}
		}

		ApplyReputationEffects(uid, rewards.RivalReputation);

		if (rewards.Items && rewards.Items.Count() > 0) {
			if (player) {
				SpawnItemRewards(player, uid, quest.QuestId, rewards.Items);
			} else {
				DME_Tasks_Log.Warn("PayoutRewards: Spieler %1 offline — Item-Rewards fuer %2 NICHT gespawnt, Rest wird ausgezahlt", uid, quest.QuestId);
			}
		}

		if (rewards.SkillPoints > 0) {
			DME_Tasks_IntegrationEvents.s_DME_OnGrantSkillPoints.Invoke(uid, rewards.SkillPoints);
		}

		if (rewards.SeasonXp > 0) {
			DME_Tasks_IntegrationEvents.s_DME_OnGrantSeasonXp.Invoke(uid, rewards.SeasonXp);
		}
	}

	//! Experience gutschreiben + simples Level-Up (Level sinkt nie).
	private void GrantExperience(DME_Tasks_PlayerState state, int experience) {
		state.Experience = state.Experience + experience;
		int newLevel = 1 + state.Experience / XP_PER_LEVEL;
		if (newLevel > state.PlayerLevel) {
			state.PlayerLevel = newLevel;
		}
	}

	private void ApplyReputationEffects(string uid, array<ref DME_Tasks_RivalRep> effects) {
		if (!effects) {
			return;
		}
		foreach (DME_Tasks_RivalRep rival : effects) {
			if (!rival || rival.TraderId == "") {
				continue;
			}
			DME_Tasks_TraderService.GetInstance().AddReputation(uid, rival.TraderId, rival.Delta);
		}
	}

	private void SpawnItemRewards(PlayerBase player, string uid, string questId, array<ref DME_Tasks_ItemReward> items) {
		foreach (DME_Tasks_ItemReward itemReward : items) {
			if (!itemReward || itemReward.ClassName == "") {
				continue;
			}
			int amount = itemReward.Amount;
			if (amount < 1) {
				amount = 1;
			}
			for (int i = 0; i < amount; i++) {
				SpawnSingleItem(player, uid, questId, itemReward);
			}
		}
	}

	//! Erst ins Inventar (GameInventory.CreateInInventory, verifiziert Inventory.c:876);
	//! volles Inventar -> Boden an Spielerposition (CreateObjectEx + ECE_PLACE_ON_SURFACE, Game.c:702).
	private void SpawnSingleItem(PlayerBase player, string uid, string questId, DME_Tasks_ItemReward itemReward) {
		string className = itemReward.ClassName;

		EntityAI item = null;
		GameInventory inventory = player.GetInventory();
		if (inventory) {
			item = inventory.CreateInInventory(className);
		}

		if (!item) {
			vector spawnPos = player.GetPosition();
			Object obj = g_Game.CreateObjectEx(className, spawnPos, ECE_PLACE_ON_SURFACE);
			item = EntityAI.Cast(obj);
			if (!item) {
				if (obj) {
					g_Game.ObjectDelete(obj);
				}
				DME_Tasks_Log.Error("SpawnSingleItem: %1 konnte nicht gespawnt werden (Quest %2)", className, questId);
				return;
			}
			DME_Tasks_Log.Info("Inventar voll — Reward %1 am Boden bei %2 gespawnt", className, uid);
		}

		SpawnAttachments(item, itemReward);

		// Welle-2-OriginService abonniert und stempelt Origin = QUEST_REWARD (CONTRACTS §6.2).
		DME_Tasks_EngineEvents.s_DME_OnRewardItemSpawned.Invoke(item, uid, questId);
	}

	private void SpawnAttachments(EntityAI item, DME_Tasks_ItemReward itemReward) {
		if (!itemReward.Attachments) {
			return;
		}
		GameInventory itemInventory = item.GetInventory();
		if (!itemInventory) {
			if (itemReward.Attachments.Count() > 0) {
				DME_Tasks_Log.Warn("SpawnAttachments: %1 hat kein Inventar — Attachments uebersprungen", itemReward.ClassName);
			}
			return;
		}
		foreach (string attachmentClass : itemReward.Attachments) {
			if (attachmentClass == "") {
				continue;
			}
			EntityAI attachment = itemInventory.CreateAttachment(attachmentClass);
			if (!attachment) {
				DME_Tasks_Log.Warn("SpawnAttachments: Attachment %1 passt nicht an %2", attachmentClass, itemReward.ClassName);
			}
		}
	}
}
