class CfgPatches
{
	class DME_Tasks_UI
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DME_Tasks_Core"};
	};
};
class CfgMods
{
	class DME_Tasks_UI
	{
		dir = "DME_Tasks_UI";
		type = "mod";
		inputs = "DME_Tasks_UI/data/inputs.xml";
		name = "DME Tasks UI";
		author = "Psyern";
		credits = "Psyern, Deadmans Echo Community";
		version = "1.0.0";
		dependencies[] = {"Mission"};
		class defs
		{
			class missionScriptModule { value=""; files[]={"DME_Tasks_UI/scripts/5_Mission"}; };
		};
	};
};
