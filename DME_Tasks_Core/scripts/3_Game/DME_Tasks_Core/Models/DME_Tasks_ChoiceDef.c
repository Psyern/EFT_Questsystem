class DME_Tasks_ChoiceDef
{
	string ChoiceId;
	string DisplayName;
	ref DME_Tasks_RewardDef Rewards;
	ref DME_Tasks_UnlockDef Unlocks;
	ref array<ref DME_Tasks_RivalRep> ReputationEffects;

	void DME_Tasks_ChoiceDef()
	{
		ChoiceId = "";
		DisplayName = "";
		Rewards = new DME_Tasks_RewardDef();
		Unlocks = new DME_Tasks_UnlockDef();
		ReputationEffects = new array<ref DME_Tasks_RivalRep>();
	}
}
