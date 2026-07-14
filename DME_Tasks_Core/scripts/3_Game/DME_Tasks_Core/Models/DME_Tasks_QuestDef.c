class DME_Tasks_QuestDef
{
	int Version;
	string QuestId;
	string TraderId;
	string Category;
	string Title;
	string Description;
	bool Repeatable;
	bool FailOnDeath;
	bool FailOnFactionChange;
	int TimeLimit;
	ref DME_Tasks_Prereqs Prerequisites;
	ref array<ref DME_Tasks_ObjectiveDef> Objectives;
	ref DME_Tasks_RewardDef Rewards;
	ref DME_Tasks_UnlockDef Unlocks;
	ref array<ref DME_Tasks_ChoiceDef> Choices;

	void DME_Tasks_QuestDef()
	{
		Version = 1;
		QuestId = "";
		TraderId = "";
		Category = "";
		Title = "";
		Description = "";
		Repeatable = false;
		FailOnDeath = false;
		FailOnFactionChange = false;
		TimeLimit = -1;
		Prerequisites = new DME_Tasks_Prereqs();
		Objectives = new array<ref DME_Tasks_ObjectiveDef>();
		Rewards = new DME_Tasks_RewardDef();
		Unlocks = new DME_Tasks_UnlockDef();
		Choices = new array<ref DME_Tasks_ChoiceDef>();
	}
}
