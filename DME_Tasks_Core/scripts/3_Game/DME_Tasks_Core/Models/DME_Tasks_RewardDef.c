class DME_Tasks_RewardDef
{
	int PlayerExperience;
	int Currency;
	float TraderReputation;
	ref array<ref DME_Tasks_RivalRep> RivalReputation;
	ref array<ref DME_Tasks_ItemReward> Items;
	int SkillPoints;
	int SeasonXp;

	void DME_Tasks_RewardDef()
	{
		PlayerExperience = 0;
		Currency = 0;
		TraderReputation = 0.0;
		RivalReputation = new array<ref DME_Tasks_RivalRep>();
		Items = new array<ref DME_Tasks_ItemReward>();
		SkillPoints = 0;
		SeasonXp = 0;
	}
}
