//! Entscheidungs-Dialog (Fraktionskonflikt/Choice-Rewards) — Vollbild-Overlay als Kind des
//! Menue-Roots (choice_dialog.layout, initial versteckt). Eigener ScriptedWidgetEventHandler
//! via SetHandler (Muster Vanilla PlayerListEntryScriptedWidget.c).
//! Zeigt pro ChoiceDef einen Button mit DisplayName (max. 5 vorgebaute Buttons);
//! Klick sendet RPC_MakeChoice(questId, choiceId). Die Auszahlung selbst laeuft weiterhin
//! ueber RPC_ClaimReward (Server wertet die getroffene Entscheidung aus).
class DME_Tasks_ChoiceDialog : ScriptedWidgetEventHandler {
	//! Anzahl vorgebauter Choice-Buttons in choice_dialog.layout (choice_option_0..4).
	static const int OPTION_COUNT = 5;

	protected DME_Tasks_Menu m_DME_Menu;
	protected Widget m_DME_Root;
	protected TextWidget m_DME_QuestText;
	protected ButtonWidget m_DME_CancelButton;
	protected ref array<ButtonWidget> m_DME_OptionButtons;
	protected ref array<string> m_DME_OptionChoiceIds;
	protected string m_DME_QuestId;

	void DME_Tasks_ChoiceDialog(DME_Tasks_Menu menu, Widget parent) {
		m_DME_Menu = menu;
		m_DME_QuestId = "";
		m_DME_OptionButtons = new array<ButtonWidget>();
		m_DME_OptionChoiceIds = new array<string>();

		if (!g_Game || !parent) {
			return;
		}

		m_DME_Root = g_Game.GetWorkspace().CreateWidgets(DME_Tasks_UIConst.LAYOUT_CHOICE_DIALOG, parent);
		if (!m_DME_Root) {
			return;
		}

		m_DME_QuestText = TextWidget.Cast(m_DME_Root.FindAnyWidget("choice_quest_text"));
		m_DME_CancelButton = ButtonWidget.Cast(m_DME_Root.FindAnyWidget("choice_cancel_button"));

		for (int i = 0; i < OPTION_COUNT; i++) {
			ButtonWidget option = ButtonWidget.Cast(m_DME_Root.FindAnyWidget("choice_option_" + i.ToString()));
			m_DME_OptionButtons.Insert(option);
			m_DME_OptionChoiceIds.Insert("");
		}

		m_DME_Root.SetHandler(this);
	}

	//! Dialog fuer eine Quest oeffnen; ohne Choices oder falschem State passiert nichts.
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

		for (int i = 0; i < m_DME_OptionButtons.Count(); i++) {
			if (w != m_DME_OptionButtons.Get(i)) {
				continue;
			}
			string choiceId = m_DME_OptionChoiceIds.Get(i);
			if (choiceId != "" && m_DME_QuestId != "") {
				SendChoice(m_DME_QuestId, choiceId);
			}
			CloseDialog();
			return true;
		}

		return false;
	}

	// ------------------------------------------------------------------
	// Intern
	// ------------------------------------------------------------------

	//! Choice-Buttons aus ChoiceDef.DisplayName fuellen.
	//! false wenn Quest fehlt, nicht ABGABEBEREIT ist oder keine Choices hat.
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
		if (entry.State != EDME_Tasks_QuestState.READY_TO_TURN_IN) {
			return false;
		}
		if (!entry.Choices || entry.Choices.Count() == 0) {
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
			m_DME_OptionChoiceIds.Set(i, "");
		}

		for (i = 0; i < entry.Choices.Count(); i++) {
			if (used >= OPTION_COUNT) {
				break;
			}
			DME_Tasks_ChoiceDef choice = entry.Choices.Get(i);
			if (!choice || choice.ChoiceId == "") {
				continue;
			}

			ButtonWidget option = m_DME_OptionButtons.Get(used);
			if (!option) {
				continue;
			}
			string label = choice.DisplayName;
			if (label == "") {
				label = choice.ChoiceId;
			}
			option.SetText(label);
			option.Show(true);
			m_DME_OptionChoiceIds.Set(used, choice.ChoiceId);
			used++;
		}

		return used > 0;
	}

	protected void SendChoice(string questId, string choiceId) {
		if (!g_Game || g_Game.IsDedicatedServer()) {
			return;
		}
		GetRPCManager().SendRPC(DME_Tasks_Const.MOD_NAME, "RPC_MakeChoice", new Param2<string, string>(questId, choiceId), true);
	}
}
