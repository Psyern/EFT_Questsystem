//! UI-Konstanten des DME-Tasks-Menues (Menue-ID, Layout-Pfade, Filter-Werte).
class DME_Tasks_UIConst {
	//! Eigene Menue-ID (CONTRACTS 6.6) — registriert via modded MissionGameplay.CreateScriptedMenu.
	static const int MENU_DME_TASKS = 731000;

	//! Input-Name aus DME_Tasks_UI/data/inputs.xml (Default-Belegung F4).
	static const string INPUT_TOGGLE_MENU = "UADMETasksMenu";

	// Layout-Pfade (Welle 3; task_list_entry/task_detail/handover/choice/replace = Agent 14, tracker/notification = Agent 15)
	static const string LAYOUT_MENU_ROOT = "DME_Tasks_UI/gui/layouts/menu_root.layout";
	static const string LAYOUT_HEADER = "DME_Tasks_UI/gui/layouts/header.layout";
	static const string LAYOUT_TASK_LIST_ENTRY = "DME_Tasks_UI/gui/layouts/task_list_entry.layout";
	static const string LAYOUT_TASK_DETAIL = "DME_Tasks_UI/gui/layouts/task_detail.layout";
	static const string LAYOUT_HANDOVER_DIALOG = "DME_Tasks_UI/gui/layouts/handover_dialog.layout";
	static const string LAYOUT_CHOICE_DIALOG = "DME_Tasks_UI/gui/layouts/choice_dialog.layout";
	static const string LAYOUT_REPLACE_DIALOG = "DME_Tasks_UI/gui/layouts/replace_dialog.layout";
	static const string LAYOUT_TRACKER = "DME_Tasks_UI/gui/layouts/tracker.layout";
	static const string LAYOUT_NOTIFICATION = "DME_Tasks_UI/gui/layouts/notification.layout";

	//! Kategorie-Filter-Werte fuer DME_Tasks_Menu.GetCategoryFilter():
	//! -1 = alle; 0..6 = EDME_Tasks_QuestCategory (STORY..WEEKLY);
	//! 100 = nur abgeschlossene Quests (State COMPLETED); 101 = nur fehlgeschlagene (State FAILED).
	static const int FILTER_ALL = -1;
	static const int FILTER_COMPLETED = 100;
	static const int FILTER_FAILED = 101;

	//! Anzahl vorgebauter Haendler-Buttons in menu_root.layout (trader_slot_0 .. trader_slot_9).
	static const int TRADER_SLOT_COUNT = 10;
}
