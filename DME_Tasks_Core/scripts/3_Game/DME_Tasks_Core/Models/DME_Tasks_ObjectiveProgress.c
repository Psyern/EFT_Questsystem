class DME_Tasks_ObjectiveProgress
{
	string ObjectiveId;
	int Current;
	int Required;
	bool Done;

	void DME_Tasks_ObjectiveProgress()
	{
		ObjectiveId = "";
		Current = 0;
		Required = 1;
		Done = false;
	}
}
