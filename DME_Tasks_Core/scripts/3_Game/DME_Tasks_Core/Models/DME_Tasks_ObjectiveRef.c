// Laufzeit-Index-Eintrag — wird NICHT persistiert.
class DME_Tasks_ObjectiveRef
{
	string PlayerUid;
	string QuestId;
	string ObjectiveId;
	DME_Tasks_ObjectiveDef Def;	// bewusst OHNE ref — Ownership liegt beim ConfigService

	void DME_Tasks_ObjectiveRef(string playerUid, string questId, string objectiveId, DME_Tasks_ObjectiveDef def)
	{
		PlayerUid = playerUid;
		QuestId = questId;
		ObjectiveId = objectiveId;
		Def = def;
	}
}
