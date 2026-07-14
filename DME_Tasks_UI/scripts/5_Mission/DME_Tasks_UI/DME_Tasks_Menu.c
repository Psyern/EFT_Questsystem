//! Vollbild-Menue "DME Tasks" (Vanilla-Stack: UIScriptedMenu + FindAnyWidget, KEIN Dabs/Expansion-MVC).
//! Shell: Tab-Leiste (nur Tasks aktiv), Haendler-Auswahl, Header, Kategorie-Filter, Content-Panels.
//! Zeigt AUSSCHLIESSLICH Cache-Daten an; Anfragen laufen als RPCs ueber das UIModule.
//! Oeffentliche API fuer Agenten 14/15: Refresh(), GetSelectedTraderId(), GetSelectedQuestId(),
//! GetCategoryFilter(), ShowDetail(questId), ShowList(), GetCache(), GetTaskListPanel(),
//! GetTaskListContent(), GetTaskDetailPanel(), GetHeaderPanel().
//! Agent 14 (implementiert): OnRefreshTaskList() baut die Task-Liste (Toolbar + Zeilen aus
//! task_list_entry.layout), OnRefreshDetail() fuellt Detail-Panel + offene Dialoge; dazu
//! ToggleShowCompletedLocked(), OpenHandoverDialog(), OpenChoiceDialog(), OpenReplaceDialog().
class DME_Tasks_Menu : UIScriptedMenu {
	protected Widget m_DME_HeaderPanel;
	protected Widget m_DME_HeaderRoot;
	protected Widget m_DME_TaskListPanel;
	protected Widget m_DME_TaskListContent;
	protected Widget m_DME_TaskDetailPanel;
	protected Widget m_DME_SyncOverlayPanel;
	protected TextWidget m_DME_SyncOverlayText;

	protected ButtonWidget m_DME_TabBuyButton;
	protected ButtonWidget m_DME_TabSellButton;
	protected ButtonWidget m_DME_TabTasksButton;
	protected ButtonWidget m_DME_TabServicesButton;
	protected ButtonWidget m_DME_CloseButton;

	protected TextWidget m_DME_HeaderTraderName;
	protected TextWidget m_DME_HeaderFactionText;
	protected TextWidget m_DME_HeaderReputationValue;
	protected TextWidget m_DME_HeaderLoyaltyValue;
	protected TextWidget m_DME_HeaderTurnoverValue;

	protected ref array<ButtonWidget> m_DME_TraderSlots;
	protected ref array<string> m_DME_TraderSlotIds;
	protected ref array<ButtonWidget> m_DME_FilterButtons;
	protected ref array<int> m_DME_FilterValues;

	protected string m_DME_SelectedTraderId;
	protected string m_DME_SelectedQuestId;
	protected int m_DME_CategoryFilter;
	protected float m_DME_ToggleGuard;

	// --- Agent 14: Task-Liste, Detail, Dialoge ---
	protected ref array<ref DME_Tasks_TaskListEntry> m_DME_ListEntries;
	protected ref DME_Tasks_TaskListEntry m_DME_ListToolbar;
	protected ref DME_Tasks_TaskDetail m_DME_TaskDetail;
	protected ref DME_Tasks_HandoverDialog m_DME_HandoverDialog;
	protected ref DME_Tasks_ChoiceDialog m_DME_ChoiceDialog;
	protected ref DME_Tasks_ReplaceDialog m_DME_ReplaceDialog;
	protected bool m_DME_ShowCompletedLocked;

	void DME_Tasks_Menu() {
		m_DME_TraderSlots = new array<ButtonWidget>();
		m_DME_TraderSlotIds = new array<string>();
		m_DME_FilterButtons = new array<ButtonWidget>();
		m_DME_FilterValues = new array<int>();
		m_DME_SelectedTraderId = "";
		m_DME_SelectedQuestId = "";
		m_DME_CategoryFilter = DME_Tasks_UIConst.FILTER_ALL;
		m_DME_ToggleGuard = 0.0;
		m_DME_ListEntries = new array<ref DME_Tasks_TaskListEntry>();
		m_DME_ShowCompletedLocked = false;
	}

	// ------------------------------------------------------------------
	// Lifecycle
	// ------------------------------------------------------------------

	override Widget Init() {
		super.Init();

		if (!g_Game) {
			return null;
		}

		layoutRoot = g_Game.GetWorkspace().CreateWidgets(DME_Tasks_UIConst.LAYOUT_MENU_ROOT);
		if (!layoutRoot) {
			return null;
		}

		m_DME_HeaderPanel = layoutRoot.FindAnyWidget("header_panel");
		m_DME_TaskListPanel = layoutRoot.FindAnyWidget("task_list_panel");
		m_DME_TaskListContent = layoutRoot.FindAnyWidget("task_list_content");
		m_DME_TaskDetailPanel = layoutRoot.FindAnyWidget("task_detail_panel");
		m_DME_SyncOverlayPanel = layoutRoot.FindAnyWidget("sync_overlay_panel");
		m_DME_SyncOverlayText = TextWidget.Cast(layoutRoot.FindAnyWidget("sync_overlay_text"));

		m_DME_TabBuyButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("tab_buy_button"));
		m_DME_TabSellButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("tab_sell_button"));
		m_DME_TabTasksButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("tab_tasks_button"));
		m_DME_TabServicesButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("tab_services_button"));
		m_DME_CloseButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("close_button"));

		// MVP: nur der Tasks-Tab ist aktiv.
		SetButtonEnabled(m_DME_TabBuyButton, false);
		SetButtonEnabled(m_DME_TabSellButton, false);
		SetButtonEnabled(m_DME_TabServicesButton, false);

		// Header-Layout in das Host-Panel laden.
		if (m_DME_HeaderPanel) {
			m_DME_HeaderRoot = g_Game.GetWorkspace().CreateWidgets(DME_Tasks_UIConst.LAYOUT_HEADER, m_DME_HeaderPanel);
		}
		if (m_DME_HeaderRoot) {
			m_DME_HeaderTraderName = TextWidget.Cast(m_DME_HeaderRoot.FindAnyWidget("header_trader_name"));
			m_DME_HeaderFactionText = TextWidget.Cast(m_DME_HeaderRoot.FindAnyWidget("header_faction_text"));
			m_DME_HeaderReputationValue = TextWidget.Cast(m_DME_HeaderRoot.FindAnyWidget("header_reputation_value"));
			m_DME_HeaderLoyaltyValue = TextWidget.Cast(m_DME_HeaderRoot.FindAnyWidget("header_loyalty_value"));
			m_DME_HeaderTurnoverValue = TextWidget.Cast(m_DME_HeaderRoot.FindAnyWidget("header_turnover_value"));
		}

		// Vorgebaute Haendler-Slots einsammeln (trader_slot_0 .. trader_slot_9).
		for (int slotIdx = 0; slotIdx < DME_Tasks_UIConst.TRADER_SLOT_COUNT; slotIdx++) {
			string slotName = "trader_slot_" + slotIdx.ToString();
			ButtonWidget slot = ButtonWidget.Cast(layoutRoot.FindAnyWidget(slotName));
			m_DME_TraderSlots.Insert(slot);
			m_DME_TraderSlotIds.Insert("");
		}

		// Kategorie-Filter-Buttons (parallel zu m_DME_FilterValues).
		AddFilterButton("filter_all_button", DME_Tasks_UIConst.FILTER_ALL);
		AddFilterButton("filter_story_button", EDME_Tasks_QuestCategory.STORY);
		AddFilterButton("filter_side_button", EDME_Tasks_QuestCategory.SIDE);
		AddFilterButton("filter_faction_button", EDME_Tasks_QuestCategory.FACTION);
		AddFilterButton("filter_boss_button", EDME_Tasks_QuestCategory.BOSS);
		AddFilterButton("filter_expedition_button", EDME_Tasks_QuestCategory.EXPEDITION);
		AddFilterButton("filter_daily_button", EDME_Tasks_QuestCategory.DAILY);
		AddFilterButton("filter_weekly_button", EDME_Tasks_QuestCategory.WEEKLY);
		AddFilterButton("filter_completed_button", DME_Tasks_UIConst.FILTER_COMPLETED);
		AddFilterButton("filter_failed_button", DME_Tasks_UIConst.FILTER_FAILED);

		// Agent 14: Detail-Ansicht + Dialoge instanzieren (Detail in das task_detail_panel,
		// Dialog-Overlays als Kinder des Menue-Roots, initial versteckt).
		InitTaskViews();

		ShowList();
		Refresh();

		return layoutRoot;
	}

	override void OnShow() {
		super.OnShow();

		if (!g_Game || g_Game.IsDedicatedServer()) {
			return;
		}

		//! UIScriptedMenu sperrt Custom-Inputs (ChangeGameFocus) — eigenen Menue-Toggle
		//! reaktivieren, damit F4 das offene Menue schliessen kann (Muster ExpansionBookMenu).
		UAInput toggleInput = GetUApi().GetInputByName(DME_Tasks_UIConst.INPUT_TOGGLE_MENU);
		if (toggleInput) {
			toggleInput.ForceDisable(false);
		}
		GetUApi().UpdateControls();

		m_DME_ToggleGuard = 0.3;

		DME_Tasks_UIModule module = DME_Tasks_UIModule.GetInstance();
		if (module) {
			module.SetActiveMenu(this);
			module.RequestSyncFromServer();
		}

		Refresh();
	}

	override void OnHide() {
		super.OnHide();

		CloseAllDialogs();

		DME_Tasks_UIModule module = DME_Tasks_UIModule.GetInstance();
		if (module && module.GetActiveMenu() == this) {
			module.SetActiveMenu(null);
		}
	}

	override void Update(float timeslice) {
		super.Update(timeslice);

		if (m_DME_ToggleGuard > 0.0) {
			m_DME_ToggleGuard = m_DME_ToggleGuard - timeslice;
		}

		if (GetUApi().GetInputByID(UAUIBack).LocalPress()) {
			Close();
			return;
		}

		if (m_DME_ToggleGuard <= 0.0) {
			UAInput toggleInput = GetUApi().GetInputByName(DME_Tasks_UIConst.INPUT_TOGGLE_MENU);
			if (toggleInput && toggleInput.LocalPress()) {
				Close();
			}
		}
	}

	// ------------------------------------------------------------------
	// Klicks
	// ------------------------------------------------------------------

	override bool OnClick(Widget w, int x, int y, int button) {
		bool handled = super.OnClick(w, x, y, button);
		if (handled) {
			return true;
		}
		if (!w) {
			return false;
		}

		if (w == m_DME_CloseButton) {
			Close();
			return true;
		}

		if (w == m_DME_TabTasksButton) {
			ShowList();
			Refresh();
			return true;
		}

		int i;
		for (i = 0; i < m_DME_TraderSlots.Count(); i++) {
			if (w == m_DME_TraderSlots.Get(i)) {
				string traderId = m_DME_TraderSlotIds.Get(i);
				if (traderId != "") {
					m_DME_SelectedTraderId = traderId;
					ShowList();
					Refresh();
				}
				return true;
			}
		}

		for (i = 0; i < m_DME_FilterButtons.Count(); i++) {
			if (w == m_DME_FilterButtons.Get(i)) {
				m_DME_CategoryFilter = m_DME_FilterValues.Get(i);
				ShowList();
				Refresh();
				return true;
			}
		}

		return false;
	}

	// ------------------------------------------------------------------
	// Oeffentliche Menue-API (Agenten 14/15)
	// ------------------------------------------------------------------

	//! Baut den sichtbaren Shell-Bereich aus dem Cache neu auf (Sync-Overlay, Haendler-Slots,
	//! Header, Filter-Markierung) und ruft danach die Agent-14-Hooks auf.
	override void Refresh() {
		super.Refresh();

		DME_Tasks_ClientCache cache = GetCache();
		if (!cache) {
			return;
		}

		UpdateSyncOverlay(cache);
		UpdateTraderSlots(cache);
		UpdateHeader(cache);
		UpdateFilterHighlight();

		OnRefreshTaskList();
		OnRefreshDetail();
	}

	//! Aktuell gewaehlter Haendler ("" solange kein Sync/kein Haendler).
	string GetSelectedTraderId() {
		return m_DME_SelectedTraderId;
	}

	//! Aktuell im Detail angezeigte Quest ("" in der Listenansicht).
	string GetSelectedQuestId() {
		return m_DME_SelectedQuestId;
	}

	//! Filter-Wert: -1 alle; 0..6 EDME_Tasks_QuestCategory; 100/101 siehe DME_Tasks_UIConst.
	int GetCategoryFilter() {
		return m_DME_CategoryFilter;
	}

	//! Paging Liste -> Detail (Panels Show(true/false)); Agent 14 fuellt task_detail_panel.
	void ShowDetail(string questId) {
		m_DME_SelectedQuestId = questId;
		if (m_DME_TaskListPanel) {
			m_DME_TaskListPanel.Show(false);
		}
		if (m_DME_TaskDetailPanel) {
			m_DME_TaskDetailPanel.Show(true);
		}
		OnRefreshDetail();
	}

	//! Paging Detail -> Liste. Aufrufer muessen danach Refresh() rufen (baut u.a. die Task-Liste).
	void ShowList() {
		m_DME_SelectedQuestId = "";
		if (m_DME_TaskDetailPanel) {
			m_DME_TaskDetailPanel.Show(false);
		}
		if (m_DME_TaskListPanel) {
			m_DME_TaskListPanel.Show(true);
		}
	}

	//! Client-Cache (null-sicher; null nur wenn das UIModule fehlt).
	DME_Tasks_ClientCache GetCache() {
		DME_Tasks_UIModule module = DME_Tasks_UIModule.GetInstance();
		if (!module) {
			return null;
		}
		return module.GetCache();
	}

	//! Listen-Panel (Agent 14).
	Widget GetTaskListPanel() {
		return m_DME_TaskListPanel;
	}

	//! WrapSpacer im ScrollWidget — Ziel-Parent fuer task_list_entry.layout-Instanzen (Agent 14):
	//! g_Game.GetWorkspace().CreateWidgets(DME_Tasks_UIConst.LAYOUT_TASK_LIST_ENTRY, GetTaskListContent())
	Widget GetTaskListContent() {
		return m_DME_TaskListContent;
	}

	//! Detail-Panel, initial versteckt (Agent 14).
	Widget GetTaskDetailPanel() {
		return m_DME_TaskDetailPanel;
	}

	//! Header-Host-Panel; enthaelt das instanzierte header.layout (Agent 15: Portrait etc.).
	Widget GetHeaderPanel() {
		return m_DME_HeaderPanel;
	}

	// ------------------------------------------------------------------
	// Oeffentliche API fuer die Agent-14-Controller (Entry/Detail/Dialoge)
	// ------------------------------------------------------------------

	//! Toolbar-Zeile der Liste: "Abgeschlossene/Gesperrte anzeigen"-Toggle umschalten.
	void ToggleShowCompletedLocked() {
		m_DME_ShowCompletedLocked = !m_DME_ShowCompletedLocked;

		//! Rebuild deferren: der Aufrufer ist der OnClick-Handler der Toolbar-Zeile —
		//! ein synchroner Rebuild wuerde das klickende Widget + Handler-Objekt mitten
		//! im eigenen Event-Dispatch zerstoeren (Use-after-free).
		ScriptCallQueue queue = null;
		if (g_Game) {
			queue = g_Game.GetCallQueue(CALL_CATEGORY_GUI);
		}
		if (queue) {
			queue.CallLater(OnRefreshTaskList, 1, false);
		} else {
			OnRefreshTaskList();
		}
	}

	bool GetShowCompletedLocked() {
		return m_DME_ShowCompletedLocked;
	}

	//! "Items abgeben?"-Dialog (RPC_HandoverItems) oeffnen.
	void OpenHandoverDialog(string questId) {
		if (m_DME_HandoverDialog) {
			m_DME_HandoverDialog.Open(questId);
		}
	}

	//! Entscheidungs-Dialog (RPC_MakeChoice) oeffnen.
	void OpenChoiceDialog(string questId) {
		if (m_DME_ChoiceDialog) {
			m_DME_ChoiceDialog.Open(questId);
		}
	}

	//! "Operative Aufgabe ersetzen?"-Dialog (RPC_ReplaceDaily) oeffnen.
	void OpenReplaceDialog(string questId) {
		if (m_DME_ReplaceDialog) {
			m_DME_ReplaceDialog.Open(questId);
		}
	}

	// ------------------------------------------------------------------
	// Hooks (Agent 14): Task-Liste + Detail aus dem Cache aufbauen
	// ------------------------------------------------------------------

	//! Wird nach jedem Refresh() gerufen — baut die Task-Liste neu auf:
	//! Filter (Haendler + Kategorie; ABGESCHLOSSEN/FEHLGESCHLAGEN filtern nach State),
	//! Toolbar-Zeile (Zaehler "N AUFGABEN" + Show-completed/locked-Toggle), danach die
	//! Quest-Zeilen sortiert: AKTIV/ABGABEBEREIT zuerst, dann VERFUEGBAR, dann Rest.
	protected void OnRefreshTaskList() {
		if (!m_DME_TaskListContent) {
			return;
		}
		DME_Tasks_ClientCache cache = GetCache();
		if (!cache) {
			return;
		}

		ClearListEntries();

		array<ref DME_Tasks_QuestSyncEntry> filtered = BuildFilteredQuestList(cache);

		m_DME_ListToolbar = new DME_Tasks_TaskListEntry(this, m_DME_TaskListContent);
		m_DME_ListToolbar.SetToolbar(filtered.Count(), m_DME_ShowCompletedLocked);

		for (int i = 0; i < filtered.Count(); i++) {
			DME_Tasks_QuestSyncEntry entry = filtered.Get(i);
			if (!entry) {
				continue;
			}
			DME_Tasks_TaskListEntry row = new DME_Tasks_TaskListEntry(this, m_DME_TaskListContent);
			row.SetQuest(entry, cache);
			m_DME_ListEntries.Insert(row);
		}
	}

	//! Wird nach jedem Refresh() und bei ShowDetail() gerufen — fuellt das Detail-Panel und
	//! aktualisiert offene Dialoge aus dem Cache (Live-Updates via RPC_ObjectiveProgress/
	//! RPC_QuestStateChanged: Fortschrittsbalken + Button-States).
	protected void OnRefreshDetail() {
		if (m_DME_TaskDetail) {
			m_DME_TaskDetail.RefreshFromCache();
		}
		if (m_DME_HandoverDialog) {
			m_DME_HandoverDialog.RefreshFromCache();
		}
		if (m_DME_ChoiceDialog) {
			m_DME_ChoiceDialog.RefreshFromCache();
		}
		if (m_DME_ReplaceDialog) {
			m_DME_ReplaceDialog.RefreshFromCache();
		}
	}

	// ------------------------------------------------------------------
	// Intern (Agent 14: Listen-/Detail-/Dialog-Verwaltung)
	// ------------------------------------------------------------------

	//! Detail-View in das task_detail_panel, Dialoge als versteckte Overlays unter den Menue-Root.
	protected void InitTaskViews() {
		if (m_DME_TaskDetailPanel) {
			m_DME_TaskDetail = new DME_Tasks_TaskDetail(this, m_DME_TaskDetailPanel);
		}
		if (layoutRoot) {
			m_DME_HandoverDialog = new DME_Tasks_HandoverDialog(this, layoutRoot);
			m_DME_ChoiceDialog = new DME_Tasks_ChoiceDialog(this, layoutRoot);
			m_DME_ReplaceDialog = new DME_Tasks_ReplaceDialog(this, layoutRoot);
		}
	}

	protected void CloseAllDialogs() {
		if (m_DME_HandoverDialog) {
			m_DME_HandoverDialog.CloseDialog();
		}
		if (m_DME_ChoiceDialog) {
			m_DME_ChoiceDialog.CloseDialog();
		}
		if (m_DME_ReplaceDialog) {
			m_DME_ReplaceDialog.CloseDialog();
		}
	}

	//! Alle Listen-Zeilen (inkl. Toolbar) zerstoeren.
	protected void ClearListEntries() {
		if (m_DME_ListToolbar) {
			m_DME_ListToolbar.Destroy();
			m_DME_ListToolbar = null;
		}
		for (int i = 0; i < m_DME_ListEntries.Count(); i++) {
			DME_Tasks_TaskListEntry row = m_DME_ListEntries.Get(i);
			if (row) {
				row.Destroy();
			}
		}
		m_DME_ListEntries.Clear();
	}

	//! Quests des gewaehlten Haendlers filtern und sortieren:
	//! Bucket 1 = ABGABEBEREIT, Bucket 2 = AKTIV, Bucket 3 = VERFUEGBAR, Bucket 4 = Rest.
	protected array<ref DME_Tasks_QuestSyncEntry> BuildFilteredQuestList(DME_Tasks_ClientCache cache) {
		array<ref DME_Tasks_QuestSyncEntry> result = new array<ref DME_Tasks_QuestSyncEntry>();
		if (m_DME_SelectedTraderId == "") {
			return result;
		}

		array<ref DME_Tasks_QuestSyncEntry> all = cache.GetQuestsForTrader(m_DME_SelectedTraderId);
		array<ref DME_Tasks_QuestSyncEntry> readyBucket = new array<ref DME_Tasks_QuestSyncEntry>();
		array<ref DME_Tasks_QuestSyncEntry> activeBucket = new array<ref DME_Tasks_QuestSyncEntry>();
		array<ref DME_Tasks_QuestSyncEntry> availableBucket = new array<ref DME_Tasks_QuestSyncEntry>();
		array<ref DME_Tasks_QuestSyncEntry> restBucket = new array<ref DME_Tasks_QuestSyncEntry>();

		int i;
		for (i = 0; i < all.Count(); i++) {
			DME_Tasks_QuestSyncEntry entry = all.Get(i);
			if (!entry) {
				continue;
			}
			if (!PassesFilter(entry)) {
				continue;
			}
			if (entry.State == EDME_Tasks_QuestState.READY_TO_TURN_IN) {
				readyBucket.Insert(entry);
			} else if (entry.State == EDME_Tasks_QuestState.ACTIVE) {
				activeBucket.Insert(entry);
			} else if (entry.State == EDME_Tasks_QuestState.AVAILABLE) {
				availableBucket.Insert(entry);
			} else {
				restBucket.Insert(entry);
			}
		}

		for (i = 0; i < readyBucket.Count(); i++) {
			result.Insert(readyBucket.Get(i));
		}
		for (i = 0; i < activeBucket.Count(); i++) {
			result.Insert(activeBucket.Get(i));
		}
		for (i = 0; i < availableBucket.Count(); i++) {
			result.Insert(availableBucket.Get(i));
		}
		for (i = 0; i < restBucket.Count(); i++) {
			result.Insert(restBucket.Get(i));
		}

		return result;
	}

	//! Filterlogik: FILTER_COMPLETED/FILTER_FAILED filtern nach State statt Category;
	//! Kategorie-Filter (0..6) matcht EDME_Tasks_QuestCategory; ohne "Show completed/locked"
	//! werden LOCKED/COMPLETED/FAILED/EXPIRED/ABANDONED ausgeblendet (COOLDOWN bleibt sichtbar).
	protected bool PassesFilter(DME_Tasks_QuestSyncEntry entry) {
		if (m_DME_CategoryFilter == DME_Tasks_UIConst.FILTER_COMPLETED) {
			return entry.State == EDME_Tasks_QuestState.COMPLETED;
		}
		if (m_DME_CategoryFilter == DME_Tasks_UIConst.FILTER_FAILED) {
			return entry.State == EDME_Tasks_QuestState.FAILED;
		}

		if (m_DME_CategoryFilter != DME_Tasks_UIConst.FILTER_ALL) {
			int category = DME_Tasks_EnumUtil.CategoryFromString(entry.Category);
			if (category != m_DME_CategoryFilter) {
				return false;
			}
		}

		if (!m_DME_ShowCompletedLocked) {
			int state = entry.State;
			if (state == EDME_Tasks_QuestState.LOCKED) {
				return false;
			}
			if (state == EDME_Tasks_QuestState.COMPLETED) {
				return false;
			}
			if (state == EDME_Tasks_QuestState.FAILED) {
				return false;
			}
			if (state == EDME_Tasks_QuestState.EXPIRED) {
				return false;
			}
			if (state == EDME_Tasks_QuestState.ABANDONED) {
				return false;
			}
		}

		return true;
	}

	// ------------------------------------------------------------------
	// Intern (Shell-Rendering aus dem Cache)
	// ------------------------------------------------------------------

	protected void SetButtonEnabled(ButtonWidget buttonWidget, bool enabled) {
		if (!buttonWidget) {
			return;
		}
		buttonWidget.Enable(enabled);
		if (enabled) {
			buttonWidget.SetAlpha(1.0);
		} else {
			buttonWidget.SetAlpha(0.35);
		}
	}

	protected void AddFilterButton(string widgetName, int filterValue) {
		if (!layoutRoot) {
			return;
		}
		ButtonWidget filterButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget(widgetName));
		if (!filterButton) {
			return;
		}
		m_DME_FilterButtons.Insert(filterButton);
		m_DME_FilterValues.Insert(filterValue);
	}

	protected void UpdateSyncOverlay(DME_Tasks_ClientCache cache) {
		if (!m_DME_SyncOverlayPanel) {
			return;
		}
		m_DME_SyncOverlayPanel.Show(!cache.SyncComplete);
	}

	protected void UpdateTraderSlots(DME_Tasks_ClientCache cache) {
		array<string> traderIds = cache.GetTraderIds();

		// Auswahl validieren bzw. auf ersten Haendler setzen.
		if (traderIds.Count() > 0) {
			bool selectionValid = false;
			if (m_DME_SelectedTraderId != "" && traderIds.Find(m_DME_SelectedTraderId) > -1) {
				selectionValid = true;
			}
			if (!selectionValid) {
				m_DME_SelectedTraderId = traderIds.Get(0);
			}
		} else {
			m_DME_SelectedTraderId = "";
		}

		for (int i = 0; i < m_DME_TraderSlots.Count(); i++) {
			ButtonWidget slot = m_DME_TraderSlots.Get(i);
			if (!slot) {
				m_DME_TraderSlotIds.Set(i, "");
				continue;
			}

			if (i < traderIds.Count()) {
				string traderId = traderIds.Get(i);
				m_DME_TraderSlotIds.Set(i, traderId);

				string label = traderId;
				DME_Tasks_TraderSyncEntry trader = cache.GetTrader(traderId);
				if (trader && trader.DisplayName != "") {
					label = trader.DisplayName;
				}
				slot.SetText(label);
				slot.Show(true);

				if (traderId == m_DME_SelectedTraderId) {
					slot.SetAlpha(1.0);
				} else {
					slot.SetAlpha(0.55);
				}
			} else {
				m_DME_TraderSlotIds.Set(i, "");
				slot.Show(false);
			}
		}
	}

	protected void UpdateHeader(DME_Tasks_ClientCache cache) {
		DME_Tasks_TraderSyncEntry trader = null;
		if (m_DME_SelectedTraderId != "") {
			trader = cache.GetTrader(m_DME_SelectedTraderId);
		}

		if (m_DME_HeaderTraderName) {
			if (trader) {
				m_DME_HeaderTraderName.SetText(trader.DisplayName);
			} else {
				m_DME_HeaderTraderName.SetText("KEIN HAENDLER");
			}
		}

		if (m_DME_HeaderFactionText) {
			if (trader) {
				m_DME_HeaderFactionText.SetText(trader.Faction);
			} else {
				m_DME_HeaderFactionText.SetText("");
			}
		}

		if (m_DME_HeaderReputationValue) {
			if (trader) {
				m_DME_HeaderReputationValue.SetText(trader.Reputation.ToString());
			} else {
				m_DME_HeaderReputationValue.SetText("-");
			}
		}

		if (m_DME_HeaderLoyaltyValue) {
			if (trader) {
				m_DME_HeaderLoyaltyValue.SetText(trader.LoyaltyLevel.ToString());
			} else {
				m_DME_HeaderLoyaltyValue.SetText("-");
			}
		}

		if (m_DME_HeaderTurnoverValue) {
			if (trader) {
				m_DME_HeaderTurnoverValue.SetText(trader.Turnover.ToString());
			} else {
				m_DME_HeaderTurnoverValue.SetText("-");
			}
		}
	}

	protected void UpdateFilterHighlight() {
		for (int i = 0; i < m_DME_FilterButtons.Count(); i++) {
			ButtonWidget filterButton = m_DME_FilterButtons.Get(i);
			if (!filterButton) {
				continue;
			}
			if (m_DME_FilterValues.Get(i) == m_DME_CategoryFilter) {
				filterButton.SetAlpha(1.0);
			} else {
				filterButton.SetAlpha(0.55);
			}
		}
	}
}
