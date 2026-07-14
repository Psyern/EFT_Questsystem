class DME_Tasks_TemplateDef
{
	int Version;
	string TemplateId;
	string TraderId;
	string ObjectiveType;
	ref array<string> TargetTypes;
	int MinimumAmount;
	int MaximumAmount;
	ref array<string> AllowedZones;
	int RewardTier;
	int TimeLimit;
	ref DME_Tasks_RewardDef Rewards;

	void DME_Tasks_TemplateDef()
	{
		Version = 1;
		TemplateId = "";
		TraderId = "";
		ObjectiveType = "";
		TargetTypes = new array<string>();
		MinimumAmount = 1;
		MaximumAmount = 1;
		AllowedZones = new array<string>();
		RewardTier = 0;
		TimeLimit = -1;
		Rewards = new DME_Tasks_RewardDef();
	}
}
