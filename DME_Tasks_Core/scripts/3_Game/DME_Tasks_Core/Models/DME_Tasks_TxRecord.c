class DME_Tasks_TxRecord
{
	string TxId;
	string QuestId;
	int State;
	int CreatedAt;

	void DME_Tasks_TxRecord()
	{
		TxId = "";
		QuestId = "";
		State = EDME_Tasks_TxState.PENDING;
		CreatedAt = 0;
	}
}
