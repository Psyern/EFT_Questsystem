//! Sync-Projektionen fuer den Server→Client-Full-Sync (JSON via JsonSerializer, Member OHNE Prefix).
//! RPC_SyncTraders-Payload   = DME_Tasks_TraderSyncList (ggf. chunked: mehrere RPCs mit Teil-Listen — Client merged per TraderId).
//! RPC_SyncQuestDefs-Payload = DME_Tasks_QuestSyncChunk (pro Haendler, ggf. mehrere Chunks — Client merged per QuestId).
//! RPC_SyncPlayerState-Payload = DME_Tasks_PlayerState (direkt serialisiert, kein eigenes Modell).

//! Ein Haendler inkl. per-Spieler-Fortschritt (Rep/Umsatz/Loyalitaet).
class DME_Tasks_TraderSyncEntry {
	string TraderId;
	string DisplayName;
	string Faction;
	ref array<ref DME_Tasks_LoyaltyLevel> LoyaltyLevels;
	float Reputation;
	int Turnover;
	int LoyaltyLevel;

	void DME_Tasks_TraderSyncEntry() {
		TraderId = "";
		DisplayName = "";
		Faction = "";
		LoyaltyLevels = new array<ref DME_Tasks_LoyaltyLevel>();
		Reputation = 0.0;
		Turnover = 0;
		LoyaltyLevel = 1;
	}
}

//! Payload von RPC_SyncTraders.
class DME_Tasks_TraderSyncList {
	ref array<ref DME_Tasks_TraderSyncEntry> Traders;

	void DME_Tasks_TraderSyncList() {
		Traders = new array<ref DME_Tasks_TraderSyncEntry>();
	}
}

//! Eine Quest-Definition + serverseitig berechneter per-Spieler-State (EDME_Tasks_QuestState) fuer die UI.
class DME_Tasks_QuestSyncEntry {
	string QuestId;
	string TraderId;
	string Category;
	string Title;
	string Description;
	bool Repeatable;
	bool FailOnDeath;
	bool FailOnFactionChange;
	int TimeLimit;
	int State;
	string LockReasonKey;
	string LockReasonP1;
	string LockReasonP2;
	ref array<ref DME_Tasks_ObjectiveDef> Objectives;
	ref DME_Tasks_RewardDef Rewards;
	ref array<ref DME_Tasks_ChoiceDef> Choices;

	void DME_Tasks_QuestSyncEntry() {
		QuestId = "";
		TraderId = "";
		Category = "";
		Title = "";
		Description = "";
		Repeatable = false;
		FailOnDeath = false;
		FailOnFactionChange = false;
		TimeLimit = -1;
		State = EDME_Tasks_QuestState.LOCKED;
		LockReasonKey = "";
		LockReasonP1 = "";
		LockReasonP2 = "";
		Objectives = new array<ref DME_Tasks_ObjectiveDef>();
		Rewards = new DME_Tasks_RewardDef();
		Choices = new array<ref DME_Tasks_ChoiceDef>();
	}
}

//! Payload von RPC_SyncQuestDefs (ein Chunk pro RPC).
class DME_Tasks_QuestSyncChunk {
	string TraderId;
	ref array<ref DME_Tasks_QuestSyncEntry> Quests;

	void DME_Tasks_QuestSyncChunk() {
		TraderId = "";
		Quests = new array<ref DME_Tasks_QuestSyncEntry>();
	}
}
