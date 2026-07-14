class DME_Tasks_ActiveQuest
{
	string QuestId;
	int AcceptedAt;
	int State;
	ref array<ref DME_Tasks_ObjectiveProgress> Objectives;
	int ExpiresAt;
	ref DME_Tasks_ExpeditionState Expedition;

	void DME_Tasks_ActiveQuest()
	{
		QuestId = "";
		AcceptedAt = 0;
		State = EDME_Tasks_QuestState.ACTIVE;
		Objectives = new array<ref DME_Tasks_ObjectiveProgress>();
		ExpiresAt = -1;
		Expedition = null;
	}
}
