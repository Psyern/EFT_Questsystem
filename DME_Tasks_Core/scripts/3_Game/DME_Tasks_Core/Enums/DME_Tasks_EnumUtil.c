class DME_Tasks_EnumUtil
{
	// Unbekannte Strings liefern -1 und loggen eine Warnung (CF_Log direkt, da DME_Tasks_Log ggf. noch nicht geladen ist).

	static int ObjectiveTypeFromString(string s)
	{
		if (s == "KILL") return EDME_Tasks_ObjectiveType.KILL;
		if (s == "COLLECT") return EDME_Tasks_ObjectiveType.COLLECT;
		if (s == "HANDOVER") return EDME_Tasks_ObjectiveType.HANDOVER;
		if (s == "TRAVEL") return EDME_Tasks_ObjectiveType.TRAVEL;
		if (s == "DISCOVER") return EDME_Tasks_ObjectiveType.DISCOVER;
		if (s == "MARK") return EDME_Tasks_ObjectiveType.MARK;
		if (s == "STASH") return EDME_Tasks_ObjectiveType.STASH;
		if (s == "INTERACT") return EDME_Tasks_ObjectiveType.INTERACT;
		if (s == "SURVIVE") return EDME_Tasks_ObjectiveType.SURVIVE;
		if (s == "RETURN_TO_TRADER") return EDME_Tasks_ObjectiveType.RETURN_TO_TRADER;
		if (s == "CRAFT") return EDME_Tasks_ObjectiveType.CRAFT;
		if (s == "ESCORT") return EDME_Tasks_ObjectiveType.ESCORT;
		if (s == "DEFEND") return EDME_Tasks_ObjectiveType.DEFEND;
		if (s == "DELIVER") return EDME_Tasks_ObjectiveType.DELIVER;
		if (s == "USE_ITEM") return EDME_Tasks_ObjectiveType.USE_ITEM;
		if (s == "SIGNAL") return EDME_Tasks_ObjectiveType.SIGNAL;
		if (s == "GROUP") return EDME_Tasks_ObjectiveType.GROUP;
		if (s == "BOSS") return EDME_Tasks_ObjectiveType.BOSS;
		if (s == "EXTRACT") return EDME_Tasks_ObjectiveType.EXTRACT;
		CF_Log.Warn("[DME_Tasks] Unknown objective type string: '%1'", s);
		return -1;
	}

	static int CategoryFromString(string s)
	{
		if (s == "STORY") return EDME_Tasks_QuestCategory.STORY;
		if (s == "SIDE") return EDME_Tasks_QuestCategory.SIDE;
		if (s == "FACTION") return EDME_Tasks_QuestCategory.FACTION;
		if (s == "BOSS") return EDME_Tasks_QuestCategory.BOSS;
		if (s == "EXPEDITION") return EDME_Tasks_QuestCategory.EXPEDITION;
		if (s == "DAILY") return EDME_Tasks_QuestCategory.DAILY;
		if (s == "WEEKLY") return EDME_Tasks_QuestCategory.WEEKLY;
		CF_Log.Warn("[DME_Tasks] Unknown quest category string: '%1'", s);
		return -1;
	}

	static int FactionFromString(string s)
	{
		if (s == "NEUTRAL") return EDME_Tasks_Faction.NEUTRAL;
		if (s == "WEST") return EDME_Tasks_Faction.WEST;
		if (s == "EAST") return EDME_Tasks_Faction.EAST;
		if (s == "BANDITS") return EDME_Tasks_Faction.BANDITS;
		if (s == "MONSTERS") return EDME_Tasks_Faction.MONSTERS;
		if (s == "TRADE_UNION") return EDME_Tasks_Faction.TRADE_UNION;
		CF_Log.Warn("[DME_Tasks] Unknown faction string: '%1'", s);
		return -1;
	}

	static int OriginFromString(string s)
	{
		if (s == "UNKNOWN") return EDME_Tasks_OriginType.UNKNOWN;
		if (s == "WORLD_LOOT") return EDME_Tasks_OriginType.WORLD_LOOT;
		if (s == "INFECTED_DROP") return EDME_Tasks_OriginType.INFECTED_DROP;
		if (s == "AI_DROP") return EDME_Tasks_OriginType.AI_DROP;
		if (s == "BOSS_DROP") return EDME_Tasks_OriginType.BOSS_DROP;
		if (s == "EVENT_REWARD") return EDME_Tasks_OriginType.EVENT_REWARD;
		if (s == "QUEST_REWARD") return EDME_Tasks_OriginType.QUEST_REWARD;
		if (s == "TRADER_PURCHASE") return EDME_Tasks_OriginType.TRADER_PURCHASE;
		if (s == "PLAYER_TRADE") return EDME_Tasks_OriginType.PLAYER_TRADE;
		if (s == "ADMIN_SPAWN") return EDME_Tasks_OriginType.ADMIN_SPAWN;
		if (s == "CRAFTED") return EDME_Tasks_OriginType.CRAFTED;
		CF_Log.Warn("[DME_Tasks] Unknown origin type string: '%1'", s);
		return -1;
	}

	static string ObjectiveTypeToString(int t)
	{
		if (t == EDME_Tasks_ObjectiveType.KILL) return "KILL";
		if (t == EDME_Tasks_ObjectiveType.COLLECT) return "COLLECT";
		if (t == EDME_Tasks_ObjectiveType.HANDOVER) return "HANDOVER";
		if (t == EDME_Tasks_ObjectiveType.TRAVEL) return "TRAVEL";
		if (t == EDME_Tasks_ObjectiveType.DISCOVER) return "DISCOVER";
		if (t == EDME_Tasks_ObjectiveType.MARK) return "MARK";
		if (t == EDME_Tasks_ObjectiveType.STASH) return "STASH";
		if (t == EDME_Tasks_ObjectiveType.INTERACT) return "INTERACT";
		if (t == EDME_Tasks_ObjectiveType.SURVIVE) return "SURVIVE";
		if (t == EDME_Tasks_ObjectiveType.RETURN_TO_TRADER) return "RETURN_TO_TRADER";
		if (t == EDME_Tasks_ObjectiveType.CRAFT) return "CRAFT";
		if (t == EDME_Tasks_ObjectiveType.ESCORT) return "ESCORT";
		if (t == EDME_Tasks_ObjectiveType.DEFEND) return "DEFEND";
		if (t == EDME_Tasks_ObjectiveType.DELIVER) return "DELIVER";
		if (t == EDME_Tasks_ObjectiveType.USE_ITEM) return "USE_ITEM";
		if (t == EDME_Tasks_ObjectiveType.SIGNAL) return "SIGNAL";
		if (t == EDME_Tasks_ObjectiveType.GROUP) return "GROUP";
		if (t == EDME_Tasks_ObjectiveType.BOSS) return "BOSS";
		if (t == EDME_Tasks_ObjectiveType.EXTRACT) return "EXTRACT";
		CF_Log.Warn("[DME_Tasks] Unknown objective type value: %1", t.ToString());
		return "";
	}

	static string QuestStateToString(int s)
	{
		if (s == EDME_Tasks_QuestState.LOCKED) return "LOCKED";
		if (s == EDME_Tasks_QuestState.AVAILABLE) return "AVAILABLE";
		if (s == EDME_Tasks_QuestState.ACTIVE) return "ACTIVE";
		if (s == EDME_Tasks_QuestState.READY_TO_TURN_IN) return "READY_TO_TURN_IN";
		if (s == EDME_Tasks_QuestState.COMPLETED) return "COMPLETED";
		if (s == EDME_Tasks_QuestState.FAILED) return "FAILED";
		if (s == EDME_Tasks_QuestState.EXPIRED) return "EXPIRED";
		if (s == EDME_Tasks_QuestState.ABANDONED) return "ABANDONED";
		if (s == EDME_Tasks_QuestState.COOLDOWN) return "COOLDOWN";
		CF_Log.Warn("[DME_Tasks] Unknown quest state value: %1", s.ToString());
		return "";
	}

	// ------------------------------------------------------------------
	// Display keys ('#' rule): stringtable keys built mechanically from the
	// enum value name (STR_DME_TASKS_<AREA>_<VALUE>). Unknown values return
	// the raw input so the UI degrades to visible text instead of an empty label.
	// ------------------------------------------------------------------

	//! State int -> "#STR_DME_TASKS_STATE_<NAME>"; unknown -> "#STR_DME_TASKS_STATE_UNKNOWN".
	static string StateDisplayKey(int s)
	{
		string name = QuestStateToString(s);
		if (name == "") {
			return "#STR_DME_TASKS_STATE_UNKNOWN";
		}
		return "#STR_DME_TASKS_STATE_" + name;
	}

	//! Category name string (sync models carry the name) -> "#STR_DME_TASKS_CAT_<NAME>"; unknown -> raw input.
	static string CategoryDisplayKeyFromString(string category)
	{
		if (CategoryFromString(category) == -1) {
			return category;
		}
		return "#STR_DME_TASKS_CAT_" + category;
	}

	//! Faction name string (trader sync carries the name) -> "#STR_DME_TASKS_FACTION_<NAME>"; unknown -> raw input.
	static string FactionDisplayKeyFromString(string faction)
	{
		if (FactionFromString(faction) == -1) {
			return faction;
		}
		return "#STR_DME_TASKS_FACTION_" + faction;
	}

	//! Objective type name string (defs carry the name) -> "#STR_DME_TASKS_OBJ_<NAME>"; unknown -> raw input.
	static string ObjectiveDisplayKeyFromString(string objectiveType)
	{
		if (ObjectiveTypeFromString(objectiveType) == -1) {
			return objectiveType;
		}
		return "#STR_DME_TASKS_OBJ_" + objectiveType;
	}

	//! Origin name string -> "#STR_DME_TASKS_ORIGIN_<NAME>"; unknown -> raw input. (Future tooltips.)
	static string OriginDisplayKeyFromString(string origin)
	{
		if (OriginFromString(origin) == -1) {
			return origin;
		}
		return "#STR_DME_TASKS_ORIGIN_" + origin;
	}
}
