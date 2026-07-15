//! UI-Reskin: lenkt die Expansion-Quest-UI auf unsere EFT-Layouts um. Da das Menu per String-Reflection
//! gespawnt wird ("ExpansionQuestMenu".ToType().Spawn()), greift dieser modded-Override transparent —
//! ohne Call-Site-Patch. Die Layouts sind 1:1-Kopien der Originale mit umgefaerbtem Gunmetal-Theme:
//! ALLE Widgetnamen + Binding_Names + der Controller-Typ bleiben unveraendert -> kein Null-Deref.
//! (Quests ist Pflicht-requiredAddon -> EXPANSIONUI/EXPANSIONMODQUESTS sind hier immer definiert.)

modded class ExpansionQuestMenu
{
	override string GetLayoutFile()
	{
		return DME_EQ_Const.LAYOUT_QUEST_MENU;
	}
}

modded class ExpansionQuestMenuListEntry
{
	override string GetLayoutFile()
	{
		return DME_EQ_Const.LAYOUT_LIST_ENTRY;
	}
}

modded class ExpansionQuestHUDObjective
{
	override string GetLayoutFile()
	{
		return DME_EQ_Const.LAYOUT_HUD_OBJECTIVE;
	}
}
