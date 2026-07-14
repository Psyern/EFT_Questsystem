class DME_Tasks_Prereqs
{
	int MinimumPlayerLevel;
	string RequiredFaction;
	float MinimumTraderReputation;
	int RequiredTraderLevel;
	ref array<string> RequiredCompletedQuests;
	ref array<string> BlockedByCompletedQuests;
	ref array<string> RequiredDecisions;
	ref array<string> BlockedByDecisions;
	ref array<ref DME_Tasks_SkillReq> RequiredSkills;
	int RequiredSeasonLevel;
	string RequiredBossProgress;
	string RequiredItem;
	int FromHour;
	int ToHour;

	void DME_Tasks_Prereqs()
	{
		MinimumPlayerLevel = 0;
		RequiredFaction = "";
		MinimumTraderReputation = 0.0;
		RequiredTraderLevel = 0;
		RequiredCompletedQuests = new array<string>();
		BlockedByCompletedQuests = new array<string>();
		RequiredDecisions = new array<string>();
		BlockedByDecisions = new array<string>();
		RequiredSkills = new array<ref DME_Tasks_SkillReq>();
		RequiredSeasonLevel = 0;
		RequiredBossProgress = "";
		RequiredItem = "";
		FromHour = -1;
		ToHour = -1;
	}
}
