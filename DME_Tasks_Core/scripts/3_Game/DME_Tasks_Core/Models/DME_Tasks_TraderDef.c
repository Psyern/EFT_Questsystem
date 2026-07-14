class DME_Tasks_TraderDef
{
	int Version;
	string TraderId;
	string DisplayName;
	string Faction;
	ref array<ref DME_Tasks_LoyaltyLevel> LoyaltyLevels;

	void DME_Tasks_TraderDef()
	{
		Version = 1;
		TraderId = "";
		DisplayName = "";
		Faction = "";
		LoyaltyLevels = new array<ref DME_Tasks_LoyaltyLevel>();
	}
}
