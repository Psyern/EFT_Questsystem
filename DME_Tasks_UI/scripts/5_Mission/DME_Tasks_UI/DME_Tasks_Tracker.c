//! HUD-Quest-Tracker (client-only): zeigt bis zu 3 getrackte Quests (PlayerState.TrackedQuests)
//! als kompakte Bloecke am rechten Bildschirmrand — Titel + pro Objective Kurzzeile "current/required"
//! mit Mini-Fortschrittsbalken. Eigenstaendige Widget-Root (tracker.layout, Parent = Workspace),
//! lebt unabhaengig vom Menue. Reine Cache-Anzeige — kein Gameplay-Code, keine RPCs.
//! Das DME_Tasks_UIModule ruft DME_Tasks_Tracker.OnCacheUpdated() nach JEDEM Cache-Update
//! (Sync-Chunks, SyncComplete, ObjectiveProgress, QuestStateChanged, RewardGranted).
class DME_Tasks_Tracker {
	//! Vorgebaute Bloecke im tracker.layout (tracker_block_0..2) = DME_Tasks_Const.MAX_TRACKED_QUESTS.
	static const int BLOCK_COUNT = 3;
	//! Vorgebaute Objective-Zeilen pro Block (tracker_obj_row_<b>_0..3); weitere Objectives werden nicht angezeigt.
	static const int OBJ_ROWS_PER_BLOCK = 4;

	static const int COLOR_TITLE_DEFAULT = 0xFFEBEBEB;
	static const int COLOR_TITLE_READY = 0xFF8CD98C;
	static const int COLOR_TEXT_DEFAULT = 0xFFDCDCDC;
	static const int COLOR_TEXT_DONE = 0xFF8CD98C;
	static const int COLOR_BAR_DEFAULT = 0xFF568CBE;
	static const int COLOR_BAR_DONE = 0xFF60B460;

	private static ref DME_Tasks_Tracker s_DME_Instance;

	protected Widget m_DME_Root;
	protected ref array<Widget> m_DME_Blocks;
	protected ref array<TextWidget> m_DME_Titles;
	// Flach indexiert: blockIdx * OBJ_ROWS_PER_BLOCK + rowIdx
	protected ref array<Widget> m_DME_ObjRows;
	protected ref array<TextWidget> m_DME_ObjTexts;
	protected ref array<ProgressBarWidget> m_DME_ObjBars;

	void DME_Tasks_Tracker() {
		m_DME_Blocks = new array<Widget>();
		m_DME_Titles = new array<TextWidget>();
		m_DME_ObjRows = new array<Widget>();
		m_DME_ObjTexts = new array<TextWidget>();
		m_DME_ObjBars = new array<ProgressBarWidget>();
		CreateRoot();
	}

	//! Einstiegspunkt fuer das UIModule (Signatur verbindlich, CONTRACTS 6.6):
	//! baut die Anzeige aus dem ClientCache neu auf; erzeugt die Instanz lazy (client-only).
	static void OnCacheUpdated() {
		if (!g_Game || g_Game.IsDedicatedServer()) {
			return;
		}
		DME_Tasks_Tracker tracker = GetInstance();
		if (!tracker) {
			return;
		}
		tracker.Rebuild();
	}

	//! Lazy-Singleton; null auf Dedicated-Server.
	static DME_Tasks_Tracker GetInstance() {
		if (!g_Game || g_Game.IsDedicatedServer()) {
			return null;
		}
		if (!s_DME_Instance) {
			s_DME_Instance = new DME_Tasks_Tracker();
		}
		return s_DME_Instance;
	}

	// ------------------------------------------------------------------
	// Aufbau
	// ------------------------------------------------------------------

	protected void CreateRoot() {
		if (!g_Game || g_Game.IsDedicatedServer()) {
			return;
		}
		if (m_DME_Root) {
			return;
		}

		WorkspaceWidget workspace = g_Game.GetWorkspace();
		if (!workspace) {
			return;
		}

		m_DME_Root = workspace.CreateWidgets(DME_Tasks_UIConst.LAYOUT_TRACKER, null);
		if (!m_DME_Root) {
			DME_Tasks_Log.Warn("Tracker: Layout %1 konnte nicht geladen werden", DME_Tasks_UIConst.LAYOUT_TRACKER);
			return;
		}

		for (int b = 0; b < BLOCK_COUNT; b++) {
			string blockIdxText = b.ToString();
			Widget block = m_DME_Root.FindAnyWidget("tracker_block_" + blockIdxText);
			m_DME_Blocks.Insert(block);
			TextWidget title = TextWidget.Cast(m_DME_Root.FindAnyWidget("tracker_title_" + blockIdxText));
			m_DME_Titles.Insert(title);

			for (int o = 0; o < OBJ_ROWS_PER_BLOCK; o++) {
				string rowSuffix = blockIdxText + "_" + o.ToString();
				Widget row = m_DME_Root.FindAnyWidget("tracker_obj_row_" + rowSuffix);
				m_DME_ObjRows.Insert(row);
				TextWidget objText = TextWidget.Cast(m_DME_Root.FindAnyWidget("tracker_obj_text_" + rowSuffix));
				m_DME_ObjTexts.Insert(objText);
				ProgressBarWidget objBar = ProgressBarWidget.Cast(m_DME_Root.FindAnyWidget("tracker_obj_bar_" + rowSuffix));
				m_DME_ObjBars.Insert(objBar);
			}
		}

		m_DME_Root.Show(false);
	}

	// ------------------------------------------------------------------
	// Anzeige aus dem Cache neu aufbauen
	// ------------------------------------------------------------------

	void Rebuild() {
		if (!g_Game || g_Game.IsDedicatedServer()) {
			return;
		}
		if (!m_DME_Root) {
			CreateRoot();
		}
		if (!m_DME_Root) {
			return;
		}

		DME_Tasks_ClientCache cache = null;
		DME_Tasks_UIModule module = DME_Tasks_UIModule.GetInstance();
		if (module) {
			cache = module.GetCache();
		}

		// Anzuzeigende Quests: getrackt UND (noch) aktiv — abgeschlossene/abgebrochene verschwinden.
		array<string> displayIds = new array<string>();
		if (cache && cache.PlayerState && cache.PlayerState.TrackedQuests) {
			for (int i = 0; i < cache.PlayerState.TrackedQuests.Count(); i++) {
				if (displayIds.Count() >= BLOCK_COUNT) {
					break;
				}
				string questId = cache.PlayerState.TrackedQuests.Get(i);
				if (questId == "") {
					continue;
				}
				if (displayIds.Find(questId) > -1) {
					continue;
				}
				if (!cache.FindActiveQuest(questId)) {
					continue;
				}
				displayIds.Insert(questId);
			}
		}

		if (displayIds.Count() == 0) {
			m_DME_Root.Show(false);
			return;
		}

		m_DME_Root.Show(true);

		for (int b = 0; b < BLOCK_COUNT; b++) {
			Widget block = null;
			if (b < m_DME_Blocks.Count()) {
				block = m_DME_Blocks.Get(b);
			}
			if (!block) {
				continue;
			}
			if (b < displayIds.Count()) {
				FillBlock(b, cache, displayIds.Get(b));
				block.Show(true);
			} else {
				block.Show(false);
			}
		}
	}

	// ------------------------------------------------------------------
	// Intern
	// ------------------------------------------------------------------

	protected void FillBlock(int blockIdx, DME_Tasks_ClientCache cache, string questId) {
		DME_Tasks_QuestSyncEntry quest = cache.GetQuest(questId);
		DME_Tasks_ActiveQuest active = cache.FindActiveQuest(questId);

		TextWidget title = null;
		if (blockIdx < m_DME_Titles.Count()) {
			title = m_DME_Titles.Get(blockIdx);
		}
		if (title) {
			string titleText = questId;
			if (quest && quest.Title != "") {
				titleText = quest.Title;
			}
			title.SetText(titleText);

			bool readyToTurnIn = false;
			if (active && active.State == EDME_Tasks_QuestState.READY_TO_TURN_IN) {
				readyToTurnIn = true;
			}
			if (readyToTurnIn) {
				title.SetColor(COLOR_TITLE_READY);
			} else {
				title.SetColor(COLOR_TITLE_DEFAULT);
			}
		}

		int rowIdx = 0;
		bool usedDefs = false;

		// Bevorzugt: Objective-Definitionen aus der QuestSyncEntry (stabile Reihenfolge, Typ-Label).
		if (quest && quest.Objectives) {
			usedDefs = true;
			for (int i = 0; i < quest.Objectives.Count(); i++) {
				if (rowIdx >= OBJ_ROWS_PER_BLOCK) {
					break;
				}
				DME_Tasks_ObjectiveDef def = quest.Objectives.Get(i);
				if (!def) {
					continue;
				}

				int required = def.Amount;
				int current = 0;
				bool done = false;
				DME_Tasks_ObjectiveProgress progress = cache.FindProgress(questId, def.ObjectiveId);
				if (progress) {
					current = progress.Current;
					required = progress.Required;
					done = progress.Done;
				}

				string label = def.Type;
				if (label == "") {
					label = "ZIEL";
				}
				FillRow(blockIdx, rowIdx, label, current, required, done);
				rowIdx++;
			}
		}

		// Fallback: nur Fortschritts-Eintraege aus dem ActiveQuest (Quest-Def noch nicht gesynct).
		if (!usedDefs && active && active.Objectives) {
			for (int j = 0; j < active.Objectives.Count(); j++) {
				if (rowIdx >= OBJ_ROWS_PER_BLOCK) {
					break;
				}
				DME_Tasks_ObjectiveProgress fallbackProgress = active.Objectives.Get(j);
				if (!fallbackProgress) {
					continue;
				}
				FillRow(blockIdx, rowIdx, "ZIEL", fallbackProgress.Current, fallbackProgress.Required, fallbackProgress.Done);
				rowIdx++;
			}
		}

		// Ungenutzte Zeilen verstecken.
		for (int r = rowIdx; r < OBJ_ROWS_PER_BLOCK; r++) {
			int hideFlat = blockIdx * OBJ_ROWS_PER_BLOCK + r;
			Widget hideRow = null;
			if (hideFlat < m_DME_ObjRows.Count()) {
				hideRow = m_DME_ObjRows.Get(hideFlat);
			}
			if (hideRow) {
				hideRow.Show(false);
			}
		}
	}

	protected void FillRow(int blockIdx, int rowIdx, string label, int current, int required, bool done) {
		int flat = blockIdx * OBJ_ROWS_PER_BLOCK + rowIdx;

		Widget row = null;
		if (flat < m_DME_ObjRows.Count()) {
			row = m_DME_ObjRows.Get(flat);
		}
		if (!row) {
			return;
		}
		row.Show(true);

		TextWidget objText = null;
		if (flat < m_DME_ObjTexts.Count()) {
			objText = m_DME_ObjTexts.Get(flat);
		}
		if (objText) {
			string line = label + "  " + current.ToString() + "/" + required.ToString();
			objText.SetText(line);
			if (done) {
				objText.SetColor(COLOR_TEXT_DONE);
			} else {
				objText.SetColor(COLOR_TEXT_DEFAULT);
			}
		}

		ProgressBarWidget objBar = null;
		if (flat < m_DME_ObjBars.Count()) {
			objBar = m_DME_ObjBars.Get(flat);
		}
		if (objBar) {
			float fraction = 0.0;
			if (done) {
				fraction = 1.0;
			} else {
				if (required > 0) {
					float currentF = current;
					float requiredF = required;
					fraction = currentF / requiredF;
				}
				if (fraction < 0.0) {
					fraction = 0.0;
				}
				if (fraction > 1.0) {
					fraction = 1.0;
				}
			}
			// ProgressBarWidget: Default-Wertebereich 0..100 (verifiziert: EnWidgets.c SimpleProgressBarWidget + NijinsCautionHPBar).
			objBar.SetCurrent(fraction * 100.0);
			if (done) {
				objBar.SetColor(COLOR_BAR_DONE);
			} else {
				objBar.SetColor(COLOR_BAR_DEFAULT);
			}
		}
	}
}
