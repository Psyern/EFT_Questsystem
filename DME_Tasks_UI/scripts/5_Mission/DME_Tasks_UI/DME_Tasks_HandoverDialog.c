//! Bestaetigungsdialog "Items abgeben?" — Vollbild-Overlay als Kind des Menue-Roots
//! (handover_dialog.layout, initial versteckt; Show/Hide). Eigener ScriptedWidgetEventHandler
//! via SetHandler (Muster Vanilla PlayerListEntryScriptedWidget.c).
//! Bietet bei mehreren offenen HANDOVER/DELIVER-Objectives eine Auswahl (max. 6 vorgebaute
//! Buttons); Klick auf eine Option IST die Bestaetigung und sendet
//! RPC_HandoverItems(questId, objectiveId). Der Server durchsucht das Inventar.
class DME_Tasks_HandoverDialog : ScriptedWidgetEventHandler {
	//! Anzahl vorgebauter Options-Buttons in handover_dialog.layout (handover_option_0..5).
	static const int OPTION_COUNT = 6;

	protected DME_Tasks_Menu m_DME_Menu;
	protected Widget m_DME_Root;
	protected TextWidget m_DME_QuestText;
	protected ButtonWidget m_DME_CancelButton;
	protected ref array<ButtonWidget> m_DME_OptionButtons;
	protected ref array<string> m_DME_OptionObjectiveIds;
	protected string m_DME_QuestId;

	void DME_Tasks_HandoverDialog(DME_Tasks_Menu menu, Widget parent) {
		m_DME_Menu = menu;
		m_DME_QuestId = "";
		m_DME_OptionButtons = new array<ButtonWidget>();
		m_DME_OptionObjectiveIds = new array<string>();

		if (!g_Game || !parent) {
			return;
		}

		m_DME_Root = g_Game.GetWorkspace().CreateWidgets(DME_Tasks_UIConst.LAYOUT_HANDOVER_DIALOG, parent);
		if (!m_DME_Root) {
			return;
		}

		m_DME_QuestText = TextWidget.Cast(m_DME_Root.FindAnyWidget("handover_quest_text"));
		m_DME_CancelButton = ButtonWidget.Cast(m_DME_Root.FindAnyWidget("handover_cancel_button"));

		for (int i = 0; i < OPTION_COUNT; i++) {
			ButtonWidget option = ButtonWidget.Cast(m_DME_Root.FindAnyWidget("handover_option_" + i.ToString()));
			m_DME_OptionButtons.Insert(option);
			m_DME_OptionObjectiveIds.Insert("");
		}

		m_DME_Root.SetHandler(this);
	}

	//! Dialog fuer eine Quest oeffnen; ohne offene Abgabe-Ziele passiert nichts.
	void Open(string questId) {
		if (!m_DME_Root) {
			return;
		}
		m_DME_QuestId = questId;
		if (!Populate()) {
			m_DME_QuestId = "";
			return;
		}
		m_DME_Root.Show(true);
	}

	bool IsOpen() {
		if (!m_DME_Root) {
			return false;
		}
		return m_DME_Root.IsVisible();
	}

	void CloseDialog() {
		m_DME_QuestId = "";
		if (m_DME_Root) {
			m_DME_Root.Show(false);
		}
	}

	//! Live-Update bei Cache-Deltas: Optionen neu fuellen; ungueltig geworden -> schliessen.
	void RefreshFromCache() {
		if (!IsOpen()) {
			return;
		}
		if (!Populate()) {
			CloseDialog();
		}
	}

	void Destroy() {
		if (m_DME_Root) {
			m_DME_Root.Unlink();
			m_DME_Root = null;
		}
	}

	override bool OnClick(Widget w, int x, int y, int button) {
		if (w == m_DME_CancelButton) {
			CloseDialog();
			return true;
		}

		for (int i = 0; i < m_DME_OptionButtons.Count(); i++) {
			if (w != m_DME_OptionButtons.Get(i)) {
				continue;
			}
			string objectiveId = m_DME_OptionObjectiveIds.Get(i);
			if (objectiveId != "" && m_DME_QuestId != "") {
				SendHandover(m_DME_QuestId, objectiveId);
			}
			CloseDialog();
			return true;
		}

		return false;
	}

	// ------------------------------------------------------------------
	// Intern
	// ------------------------------------------------------------------

	//! Options-Buttons aus den offenen HANDOVER/DELIVER-Objectives fuellen.
	//! false wenn Quest fehlt, nicht mehr AKTIV ist oder keine offene Abgabe existiert.
	protected bool Populate() {
		if (!m_DME_Menu || m_DME_QuestId == "") {
			return false;
		}
		DME_Tasks_ClientCache cache = m_DME_Menu.GetCache();
		if (!cache) {
			return false;
		}
		DME_Tasks_QuestSyncEntry entry = cache.GetQuest(m_DME_QuestId);
		if (!entry) {
			return false;
		}
		if (entry.State != EDME_Tasks_QuestState.ACTIVE) {
			return false;
		}

		if (m_DME_QuestText) {
			string title = entry.Title;
			if (title == "") {
				title = entry.QuestId;
			}
			m_DME_QuestText.SetText(title);
		}

		int used = 0;
		int i;
		for (i = 0; i < m_DME_OptionButtons.Count(); i++) {
			ButtonWidget resetButton = m_DME_OptionButtons.Get(i);
			if (resetButton) {
				resetButton.Show(false);
			}
			m_DME_OptionObjectiveIds.Set(i, "");
		}

		if (!entry.Objectives) {
			return false;
		}

		for (i = 0; i < entry.Objectives.Count(); i++) {
			if (used >= OPTION_COUNT) {
				break;
			}
			DME_Tasks_ObjectiveDef def = entry.Objectives.Get(i);
			if (!def) {
				continue;
			}
			if (def.Type != "HANDOVER" && def.Type != "DELIVER") {
				continue;
			}

			int required = def.Amount;
			int current = 0;
			DME_Tasks_ObjectiveProgress progress = cache.FindProgress(m_DME_QuestId, def.ObjectiveId);
			if (progress) {
				if (progress.Done) {
					continue;
				}
				current = progress.Current;
				required = progress.Required;
			}

			ButtonWidget option = m_DME_OptionButtons.Get(used);
			if (!option) {
				continue;
			}
			string label = DME_Tasks_UITextUtil.ObjectiveText(def) + "  [" + current.ToString() + "/" + required.ToString() + "]";
			option.SetText(label);
			option.Show(true);
			m_DME_OptionObjectiveIds.Set(used, def.ObjectiveId);
			used++;
		}

		return used > 0;
	}

	protected void SendHandover(string questId, string objectiveId) {
		if (!g_Game || g_Game.IsDedicatedServer()) {
			return;
		}
		GetRPCManager().SendRPC(DME_Tasks_Const.MOD_NAME, "RPC_HandoverItems", new Param2<string, string>(questId, objectiveId), true);
	}
}
