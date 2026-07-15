//! UI-Reskin (LIZENZ-SAUBER): DayZ-Expansion steht unter CC BY-NC-ND (NoDerivatives) — deshalb werden
//! WEDER Expansion-Layouts kopiert/weiterverbreitet NOCH per GetLayoutFile umgelenkt. Stattdessen faerben
//! wir die vorhandenen Stock-Widgets zur Laufzeit auf unsere EFT/Tarkov-Palette um (modded OnShow, super
//! zuerst). Nutzt ausschliesslich die auto-gebundenen protected Member der Basis -> existieren garantiert,
//! kein Null-Deref. Nur Client (das Menue laeuft client-seitig).
modded class ExpansionQuestMenu
{
	override void OnShow()
	{
		super.OnShow();
		DME_EQ_Restyle();
	}

	//! Panels -> Gunmetal, Aktions-Buttons -> Messing, Leisten -> Void. Alles null-sicher.
	private void DME_EQ_Restyle()
	{
		int panel = ARGB(235, 22, 25, 30);    //! Gunmetal  #16191E
		int deep  = ARGB(245, 14, 16, 19);    //! Void      #0E1013
		int brass = ARGB(255, 201, 162, 39);  //! Messing   #C9A227

		DME_EQ_SetColor(QuestListPanel, panel);
		DME_EQ_SetColor(QuestDetailsPanel, panel);
		DME_EQ_SetColor(DefaultPanel, panel);
		DME_EQ_SetColor(RewardPanel, panel);
		DME_EQ_SetColor(ObjectivePanel, panel);
		DME_EQ_SetColor(QuestItemsPanel, panel);
		DME_EQ_SetColor(Reputation, panel);
		DME_EQ_SetColor(FactionReputation, panel);
		DME_EQ_SetColor(ButtonsPanel, deep);
		DME_EQ_SetColor(MenuButtonsPanel, deep);
		DME_EQ_SetColor(AcceptBackground, brass);
		DME_EQ_SetColor(CompleteBackground, brass);
		DME_EQ_SetColor(CancelBackground, deep);
		DME_EQ_SetColor(ShareBackground, deep);
		DME_EQ_SetColor(CloseBackground, deep);
		DME_EQ_SetColor(HideHudBackground, deep);
		DME_EQ_SetColor(BackBackground, deep);
	}

	private void DME_EQ_SetColor(Widget w, int color)
	{
		if (w)
			w.SetColor(color);
	}
}
