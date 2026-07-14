//! Ein Eintrag (Zeile) der Task-Liste — reine Anzeige aus Cache-Daten.
//!
//! Klick-Muster (quellcode-verifiziert, Vanilla 1.29 PlayerListEntryScriptedWidget.c):
//! Der Entry-Controller erbt von ScriptedWidgetEventHandler, erzeugt sein Layout via
//! g_Game.GetWorkspace().CreateWidgets(<template>, parent) und setzt m_DME_Root.SetHandler(this).
//! Dadurch laufen alle Widget-Events des Eintrags (Root ist eine full-row ButtonWidget)
//! beim Entry selbst auf — OnClick vergleicht das Widget und delegiert an das Menue
//! (menu.ShowDetail bzw. menu.ToggleShowCompletedLocked). Dieses Muster ist robuster als
//! ein zentrales Menue-OnClick, weil dynamische Eintraege keine Widget-Vergleichslisten
//! im Menue benoetigen und beim Rebuild einfach mit dem Widget verschwinden.
//!
//! Sonderfall "Toolbar-Zeile": Die erste Zeile der Liste ist KEINE Quest, sondern der
//! "Abgeschlossene/Gesperrte anzeigen"-Toggle + Zaehler "N AUFGABEN" (SetToolbar) —
//! ein eigenes Layout dafuer war Agent 14 nicht zugewiesen, daher wird das Zeilen-Template
//! wiederverwendet.
class DME_Tasks_TaskListEntry : ScriptedWidgetEventHandler {
	protected DME_Tasks_Menu m_DME_Menu;
	protected Widget m_DME_Root;
	protected Widget m_DME_StatePanel;
	protected TextWidget m_DME_StateText;
	protected TextWidget m_DME_TitleText;
	protected TextWidget m_DME_InfoText;
	protected TextWidget m_DME_ProgressText;
	protected string m_DME_QuestId;
	protected bool m_DME_IsToolbar;

	void DME_Tasks_TaskListEntry(DME_Tasks_Menu menu, Widget parent) {
		m_DME_Menu = menu;
		m_DME_QuestId = "";
		m_DME_IsToolbar = false;

		if (!g_Game || !parent) {
			return;
		}

		m_DME_Root = g_Game.GetWorkspace().CreateWidgets(DME_Tasks_UIConst.LAYOUT_TASK_LIST_ENTRY, parent);
		if (!m_DME_Root) {
			return;
		}

		m_DME_StatePanel = m_DME_Root.FindAnyWidget("entry_state_panel");
		m_DME_StateText = TextWidget.Cast(m_DME_Root.FindAnyWidget("entry_state_text"));
		m_DME_TitleText = TextWidget.Cast(m_DME_Root.FindAnyWidget("entry_title_text"));
		m_DME_InfoText = TextWidget.Cast(m_DME_Root.FindAnyWidget("entry_info_text"));
		m_DME_ProgressText = TextWidget.Cast(m_DME_Root.FindAnyWidget("entry_progress_text"));

		m_DME_Root.SetHandler(this);
	}

	//! Zeile als Quest-Eintrag fuellen (State-Chip, Titel, Kategorie/Ort, Fortschritts-Kurztext).
	void SetQuest(DME_Tasks_QuestSyncEntry entry, DME_Tasks_ClientCache cache) {
		if (!entry) {
			return;
		}

		m_DME_IsToolbar = false;
		m_DME_QuestId = entry.QuestId;

		if (m_DME_StateText) {
			m_DME_StateText.SetText(DME_Tasks_UITextUtil.StateLabel(entry.State));
		}
		if (m_DME_StatePanel) {
			m_DME_StatePanel.SetColor(DME_Tasks_UITextUtil.StateColor(entry.State));
		}

		if (m_DME_TitleText) {
			string title = entry.Title;
			if (title == "") {
				title = entry.QuestId;
			}
			m_DME_TitleText.SetText(title);
		}

		if (m_DME_InfoText) {
			string info = DME_Tasks_UITextUtil.CategoryLabel(entry.Category);
			string location = DME_Tasks_UITextUtil.QuestLocationLabel(entry);
			if (location != "") {
				info = info + " | " + location;
			}
			m_DME_InfoText.SetText(info);
		}

		if (m_DME_ProgressText) {
			m_DME_ProgressText.SetText(BuildProgressShortText(entry, cache));
		}
	}

	//! Zeile als Listen-Toolbar fuellen: Zaehler "N AUFGABEN" + Toggle-Status; Klick schaltet um.
	void SetToolbar(int taskCount, bool showCompletedLocked) {
		m_DME_IsToolbar = true;
		m_DME_QuestId = "";

		if (m_DME_Root) {
			m_DME_Root.SetColor(ARGB(255, 26, 30, 41));
		}
		if (m_DME_StateText) {
			m_DME_StateText.SetText(DME_Tasks_LocKeys.MENU_FILTER);
		}
		if (m_DME_StatePanel) {
			m_DME_StatePanel.SetColor(ARGB(255, 60, 66, 82));
		}
		if (m_DME_TitleText) {
			m_DME_TitleText.SetText(DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_TASK_COUNT, taskCount.ToString()));
		}
		if (m_DME_InfoText) {
			if (showCompletedLocked) {
				m_DME_InfoText.SetText(DME_Tasks_LocKeys.MENU_SHOW_HIDDEN_ON);
			} else {
				m_DME_InfoText.SetText(DME_Tasks_LocKeys.MENU_SHOW_HIDDEN_OFF);
			}
		}
		if (m_DME_ProgressText) {
			m_DME_ProgressText.SetText(DME_Tasks_LocKeys.MENU_TOGGLE);
		}
	}

	string GetQuestId() {
		return m_DME_QuestId;
	}

	//! Widget-Baum zerstoeren (beim Listen-Rebuild); Unlink zerstoert Root + alle Kinder.
	void Destroy() {
		if (m_DME_Root) {
			m_DME_Root.Unlink();
			m_DME_Root = null;
		}
	}

	override bool OnClick(Widget w, int x, int y, int button) {
		if (!m_DME_Root || !m_DME_Menu) {
			return false;
		}
		if (w != m_DME_Root) {
			return false;
		}

		if (m_DME_IsToolbar) {
			m_DME_Menu.ToggleShowCompletedLocked();
			return true;
		}

		if (m_DME_QuestId != "") {
			m_DME_Menu.ShowDetail(m_DME_QuestId);
			return true;
		}

		return false;
	}

	//! Kurztext rechts: "n/m ZIELE" fuer laufende Quests, sonst leer.
	protected string BuildProgressShortText(DME_Tasks_QuestSyncEntry entry, DME_Tasks_ClientCache cache) {
		bool running = false;
		if (entry.State == EDME_Tasks_QuestState.ACTIVE || entry.State == EDME_Tasks_QuestState.READY_TO_TURN_IN) {
			running = true;
		}
		if (!running || !cache) {
			return "";
		}

		DME_Tasks_ActiveQuest active = cache.FindActiveQuest(entry.QuestId);
		if (!active || !active.Objectives) {
			return "";
		}

		int total = active.Objectives.Count();
		int done = 0;
		for (int i = 0; i < total; i++) {
			DME_Tasks_ObjectiveProgress progress = active.Objectives.Get(i);
			if (progress && progress.Done) {
				done++;
			}
		}
		if (total == 0) {
			return "";
		}
		return DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJECTIVES_SHORT, done.ToString(), total.ToString());
	}
}
