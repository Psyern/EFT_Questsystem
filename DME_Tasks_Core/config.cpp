class CfgPatches
{
	class DME_Tasks_Core
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data","JM_CF_Scripts"};
	};
};
class CfgMods
{
	class DME_Tasks_Core
	{
		dir = "DME_Tasks_Core";
		type = "mod";
		name = "DME Tasks Core";
		author = "Psyern";
		credits = "Psyern, Deadmans Echo Community";
		version = "1.0.0";
		dependencies[] = {"Game","World","Mission"};
		class defs
		{
			class gameScriptModule { value=""; files[]={"DME_Tasks_Core/scripts/3_Game"}; };
			class worldScriptModule { value=""; files[]={"DME_Tasks_Core/scripts/4_World"}; };
			class missionScriptModule { value=""; files[]={"DME_Tasks_Core/scripts/5_Mission"}; };
		};
	};
};
