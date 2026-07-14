class CfgPatches
{
	class DME_Tasks_Integrations
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DME_Tasks_Core","DME_Tasks_Objectives"};
	};
};
class CfgMods
{
	class DME_Tasks_Integrations
	{
		dir = "DME_Tasks_Integrations";
		type = "mod";
		name = "DME Tasks Integrations";
		author = "Psyern";
		credits = "Psyern, Deadmans Echo Community";
		version = "1.0.0";
		dependencies[] = {"World"};
		class defs
		{
			class worldScriptModule { value=""; files[]={"DME_Tasks_Integrations/scripts/4_World"}; };
		};
	};
};
