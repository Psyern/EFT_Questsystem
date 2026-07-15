//! Konstanten + schlanker Logger fuer den DME Expansion-EFT Quest-Add-On.
class DME_EQ_Const
{
	static const string MOD_NAME = "DME_ExpansionQuestEFT";
	static const string PROFILE_DIR = "$profile:DME_ExpansionQuestEFT";
	static const string CONFIG_FILE = "$profile:DME_ExpansionQuestEFT/LoyaltyConfig.json";
	static const string SETTINGS_FILE = "$profile:DME_ExpansionQuestEFT/Settings.json";
	static const string EXAMPLE_CONFIG = "DME_ExpansionQuestEFT/_ServerProfile_Example/DME_ExpansionQuestEFT/LoyaltyConfig.json";

	//! Eigene EFT-Layouts (Reskin). Muster: Expansion-Layouts 1:1 kopiert + umgefaerbt, Widgetnamen erhalten.
	static const string LAYOUT_QUEST_MENU = "DME_ExpansionQuestEFT/GUI/layouts/dme_eq/quest_menu.layout";
	static const string LAYOUT_LIST_ENTRY = "DME_ExpansionQuestEFT/GUI/layouts/dme_eq/quest_menu_list_entry.layout";
	static const string LAYOUT_HUD_OBJECTIVE = "DME_ExpansionQuestEFT/GUI/layouts/dme_eq/quest_hud_objective.layout";
}

class DME_EQ_Log
{
	static void Info(string msg)
	{
		Print("[DME_EQ] " + msg);
	}

	static void Warn(string msg)
	{
		Print("[DME_EQ][WARN] " + msg);
	}
}
