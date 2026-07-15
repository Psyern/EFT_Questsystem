class CfgPatches
{
	class DME_ExpansionQuestEFT
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] =
		{
			"DZ_Data",
			"DayZExpansion_Core_Scripts",
			"DayZExpansion_Quests_Scripts",
			"DayZExpansion_Quests_GUI"
		};
	};
};
class CfgMods
{
	class DME_ExpansionQuestEFT
	{
		dir = "DME_ExpansionQuestEFT";
		type = "mod";
		name = "DME Expansion Quest EFT";
		author = "DME";
		dependencies[] = {"Game","World","Mission"};
		class defs
		{
			class gameScriptModule
			{
				value = "";
				files[] = {"DME_ExpansionQuestEFT/scripts/3_Game"};
			};
			class worldScriptModule
			{
				value = "";
				files[] = {"DME_ExpansionQuestEFT/scripts/4_World"};
			};
			class missionScriptModule
			{
				value = "";
				files[] = {"DME_ExpansionQuestEFT/scripts/5_Mission"};
			};
		};
	};
};
