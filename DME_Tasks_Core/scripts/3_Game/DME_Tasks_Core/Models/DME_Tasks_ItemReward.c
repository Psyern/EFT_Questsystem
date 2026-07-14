class DME_Tasks_ItemReward
{
	string ClassName;
	int Amount;
	ref array<string> Attachments;

	void DME_Tasks_ItemReward()
	{
		ClassName = "";
		Amount = 1;
		Attachments = new array<string>();
	}
}
