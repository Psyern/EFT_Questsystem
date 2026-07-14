class CfgPatches
{
	class DME_Tasks_Objectives
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DME_Tasks_Core"};
	};
};
class CfgMods
{
	class DME_Tasks_Objectives
	{
		dir = "DME_Tasks_Objectives";
		type = "mod";
		name = "DME Tasks Objectives";
		author = "Psyern";
		credits = "Psyern, Deadmans Echo Community";
		version = "1.0.0";
		//! CF ModStorage: item origin metadata is stored through CF (NOT raw ctx writes).
		//! ModStorage only registers a context while this is > 0 — do not remove or rename
		//! this CfgMods class, or all stored origin data is lost.
		storageVersion = 1;
		dependencies[] = {"World"};
		class defs
		{
			class worldScriptModule { value=""; files[]={"DME_Tasks_Objectives/scripts/4_World"}; };
		};
	};
};
