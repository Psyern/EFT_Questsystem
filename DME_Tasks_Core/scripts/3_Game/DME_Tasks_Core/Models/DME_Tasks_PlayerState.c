class DME_Tasks_PlayerState
{
	int Version;
	string SteamId;
	int PlayerLevel;
	int Experience;
	ref map<string, ref DME_Tasks_TraderProgress> TraderProgress;
	ref array<ref DME_Tasks_ActiveQuest> ActiveQuests;
	ref array<ref DME_Tasks_CompletedQuest> CompletedQuests;
	ref map<string, int> Cooldowns;
	ref array<string> Decisions;
	ref array<string> TrackedQuests;
	ref array<ref DME_Tasks_TxRecord> Transactions;

	void DME_Tasks_PlayerState()
	{
		Version = 1;
		SteamId = "";
		PlayerLevel = 1;
		Experience = 0;
		TraderProgress = new map<string, ref DME_Tasks_TraderProgress>();
		ActiveQuests = new array<ref DME_Tasks_ActiveQuest>();
		CompletedQuests = new array<ref DME_Tasks_CompletedQuest>();
		Cooldowns = new map<string, int>();
		Decisions = new array<string>();
		TrackedQuests = new array<string>();
		Transactions = new array<ref DME_Tasks_TxRecord>();
	}
}
