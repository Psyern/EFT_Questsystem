//! "Operative Aufgabe ersetzen?"-Confirm (Daily/Weekly-Replace) — Vollbild-Overlay als Kind
//! des Menue-Roots (replace_dialog.layout, initial versteckt). Eigener
//! ScriptedWidgetEventHandler via SetHandler (Muster Vanilla PlayerListEntryScriptedWidget.c).
//! Die Replace-Kosten stehen in den Server-Settings und sind am Client NICHT verfuegbar —
//! der Dialog zeigt daher einen generischen Gebuehren-Hinweis (Layout-Text).
//! Bestaetigen sendet RPC_ReplaceDaily(questId); der Server validiert Kategorie & Kosten.
class DME_Tasks_ReplaceDialog : ScriptedWidgetEventHandler {
	protected DME_Tasks_Menu m_DME_Menu;
	protected Widget m_DME_Root;
	protected TextWidget m_DME_QuestText;
	protected ButtonWidget m_DME_ConfirmButton;
	protected ButtonWidget m_DME_CancelButton;
	protected string m_DME_QuestId;

	void DME_Tasks_ReplaceDialog(DME_Tasks_Menu menu, Widget parent) {
		m_DME_Menu = menu;
		m_DME_QuestId = "";

		if (!g_Game || !parent) {
			return;
		}

		m_DME_Root = g_Game.GetWorkspace().CreateWidgets(DME_Tasks_UIConst.LAYOUT_REPLACE_DIALOG, parent);
		if (!m_DME_Root) {
			return;
		}

		m_DME_QuestText = TextWidget.Cast(m_DME_Root.FindAnyWidget("replace_quest_text"));
		m_DME_ConfirmButton = ButtonWidget.Cast(m_DME_Root.FindAnyWidget("replace_confirm_button"));
		m_DME_CancelButton = ButtonWidget.Cast(m_DME_Root.FindAnyWidget("replace_cancel_button"));

		m_DME_Root.SetHandler(this);
	}

	//! Dialog fuer eine Quest oeffnen; nur fuer DAILY/WEEKLY in nicht-terminalem State.
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

	//! Live-Update bei Cache-Deltas; ungueltig geworden -> schliessen.
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

		if (w == m_DME_ConfirmButton) {
			if (m_DME_QuestId != "") {
				SendReplace(m_DME_QuestId);
			}
			CloseDialog();
			return true;
		}

		return false;
	}

	// ------------------------------------------------------------------
	// Intern
	// ------------------------------------------------------------------

	//! false wenn Quest fehlt, keine DAILY/WEEKLY ist oder terminal/gesperrt ist.
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
		if (entry.Category != "DAILY" && entry.Category != "WEEKLY") {
			return false;
		}

		bool stateOk = false;
		if (entry.State == EDME_Tasks_QuestState.AVAILABLE) {
			stateOk = true;
		}
		if (entry.State == EDME_Tasks_QuestState.ACTIVE || entry.State == EDME_Tasks_QuestState.READY_TO_TURN_IN) {
			stateOk = true;
		}
		if (!stateOk) {
			return false;
		}

		if (m_DME_QuestText) {
			string title = entry.Title;
			if (title == "") {
				title = entry.QuestId;
			}
			m_DME_QuestText.SetText(title);
		}

		return true;
	}

	protected void SendReplace(string questId) {
		if (!g_Game || g_Game.IsDedicatedServer()) {
			return;
		}
		GetRPCManager().SendRPC(DME_Tasks_Const.MOD_NAME, "RPC_ReplaceDaily", new Param1<string>(questId), true);
	}
}
