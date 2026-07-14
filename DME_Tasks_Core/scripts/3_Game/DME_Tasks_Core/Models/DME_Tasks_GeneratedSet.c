//! Persistenz-Model fuer generierte Daily-/Weekly-Sets (CONTRACTS §6.8):
//! $profile:DME_Tasks/Generated/<TodayKey>.json bzw. Generated/week_<WeekKey>.json.
//! JSON-Member OHNE Prefix (Keys = JSON). DateKey = TodayKey ("YYYY-MM-DD") bzw. WeekKey ("YYYY-Www").
class DME_Tasks_GeneratedSet
{
	int Version;
	string DateKey;
	ref array<ref DME_Tasks_QuestDef> Quests;

	void DME_Tasks_GeneratedSet()
	{
		Version = 1;
		DateKey = "";
		Quests = new array<ref DME_Tasks_QuestDef>();
	}
}
