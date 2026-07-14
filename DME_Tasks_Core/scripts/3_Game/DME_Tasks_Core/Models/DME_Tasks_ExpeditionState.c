class DME_Tasks_ExpeditionState
{
	string SessionId;
	string QuestId;
	int StartedAt;
	bool EnteredZone;
	bool Extracted;
	bool PlayerDied;
	bool DisconnectedInCombat;
	int ExtractStartedAt;

	void DME_Tasks_ExpeditionState()
	{
		SessionId = "";
		QuestId = "";
		StartedAt = 0;
		EnteredZone = false;
		Extracted = false;
		PlayerDied = false;
		DisconnectedInCombat = false;
		ExtractStartedAt = -1;
	}
}
