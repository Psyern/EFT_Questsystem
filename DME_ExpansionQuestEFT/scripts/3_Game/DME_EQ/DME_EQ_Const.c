//! Konstanten + schlanker Logger fuer den DME Expansion-EFT Quest-Add-On.
class DME_EQ_Const
{
	static const string MOD_NAME = "DME_ExpansionQuestEFT";
	static const string PROFILE_DIR = "$profile:DME_ExpansionQuestEFT";
	static const string CONFIG_FILE = "$profile:DME_ExpansionQuestEFT/LoyaltyConfig.json";
	static const string SETTINGS_FILE = "$profile:DME_ExpansionQuestEFT/Settings.json";
	static const string EXAMPLE_CONFIG = "DME_ExpansionQuestEFT/_ServerProfile_Example/DME_ExpansionQuestEFT/LoyaltyConfig.json";
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
