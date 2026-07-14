//! Client-Hook: Keybind (UADMETasksMenu, Default F4) oeffnet das DME-Tasks-Menue und
//! registriert die eigene Menue-ID im CreateScriptedMenu-Fabrikpfad (Vanilla-Muster
//! missionGameplay.c OnUpdate / missionBase.c CreateScriptedMenu).
modded class MissionGameplay {
	override void OnUpdate(float timeslice) {
		super.OnUpdate(timeslice);

		if (!g_Game || g_Game.IsDedicatedServer()) {
			return;
		}

		UAInput toggleInput = GetUApi().GetInputByName(DME_Tasks_UIConst.INPUT_TOGGLE_MENU);
		if (!toggleInput) {
			return;
		}
		if (!toggleInput.LocalPress()) {
			return;
		}

		UIManager uiManager = g_Game.GetUIManager();
		if (!uiManager) {
			return;
		}
		// Nur oeffnen, wenn kein anderes Menue offen ist und der Spieler existiert.
		if (uiManager.GetMenu() != null) {
			return;
		}
		if (!g_Game.GetPlayer()) {
			return;
		}

		uiManager.EnterScriptedMenu(DME_Tasks_UIConst.MENU_DME_TASKS, null);
	}

	override UIScriptedMenu CreateScriptedMenu(int id) {
		UIScriptedMenu menu = super.CreateScriptedMenu(id);
		if (menu) {
			return menu;
		}

		if (id == DME_Tasks_UIConst.MENU_DME_TASKS) {
			menu = new DME_Tasks_Menu();
			menu.SetID(id);
		}

		return menu;
	}
}
