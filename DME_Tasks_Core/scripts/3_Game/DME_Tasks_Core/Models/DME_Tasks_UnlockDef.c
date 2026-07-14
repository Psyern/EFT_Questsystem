class DME_Tasks_UnlockDef
{
	ref array<string> QuestIds;
	ref array<string> MarketItems;
	ref array<string> Keys;
	string BossAccess;

	void DME_Tasks_UnlockDef()
	{
		QuestIds = new array<string>();
		MarketItems = new array<string>();
		Keys = new array<string>();
		BossAccess = "";
	}
}
