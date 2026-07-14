//! Client-UI-Modul: haelt den DME_Tasks_ClientCache, abonniert ALLE DME_Tasks_ClientEvents-Invoker
//! und aktualisiert ein offenes Menue (SetActiveMenu/GetActiveMenu) sowie den HUD-Tracker live.
//! Erwartete Welle-3-Seams (Agent 15, parallel):
//!   DME_Tasks_Notifier: static void Show(string title, string message, int type)
//!   DME_Tasks_Tracker:  static void OnCacheUpdated()
[CF_RegisterModule(DME_Tasks_UIModule)]
class DME_Tasks_UIModule : CF_ModuleWorld {
	private ref DME_Tasks_ClientCache m_DME_Cache;
	private DME_Tasks_Menu m_DME_ActiveMenu;

	override void OnInit() {
		super.OnInit();

		m_DME_Cache = new DME_Tasks_ClientCache();

		DME_Tasks_ClientEvents.s_DME_OnSyncTraders.Insert(OnSyncTraders);
		DME_Tasks_ClientEvents.s_DME_OnSyncQuestDefs.Insert(OnSyncQuestDefs);
		DME_Tasks_ClientEvents.s_DME_OnSyncPlayerState.Insert(OnSyncPlayerState);
		DME_Tasks_ClientEvents.s_DME_OnSyncComplete.Insert(OnSyncComplete);
		DME_Tasks_ClientEvents.s_DME_OnObjectiveProgress.Insert(OnObjectiveProgress);
		DME_Tasks_ClientEvents.s_DME_OnQuestStateChanged.Insert(OnQuestStateChanged);
		DME_Tasks_ClientEvents.s_DME_OnRewardGranted.Insert(OnRewardGranted);
		DME_Tasks_ClientEvents.s_DME_OnNotification.Insert(OnNotification);
	}

	//! Statischer Zugriff (null-sicher verwenden!).
	static DME_Tasks_UIModule GetInstance() {
		return CF_Modules<DME_Tasks_UIModule>.Get();
	}

	//! Der Cache existiert immer (lazy-Fallback falls vor OnInit abgefragt).
	DME_Tasks_ClientCache GetCache() {
		if (!m_DME_Cache) {
			m_DME_Cache = new DME_Tasks_ClientCache();
		}
		return m_DME_Cache;
	}

	//! Vom Menue in OnShow(this)/OnHide(null) gesetzt — Invoker-Abos refreshen das offene Menue live.
	void SetActiveMenu(DME_Tasks_Menu menu) {
		m_DME_ActiveMenu = menu;
	}

	DME_Tasks_Menu GetActiveMenu() {
		return m_DME_ActiveMenu;
	}

	//! Fordert einen Full-Sync an; setzt SyncComplete zurueck (Menue zeigt "Synchronisiere...").
	void RequestSyncFromServer() {
		if (!g_Game || g_Game.IsDedicatedServer()) {
			return;
		}
		GetCache().SyncComplete = false;
		GetRPCManager().SendRPC(DME_Tasks_Const.MOD_NAME, "RPC_RequestSync", new Param1<int>(0), true);
	}

	// ------------------------------------------------------------------
	// ClientEvents-Handler (alle client-only; delegieren an den Cache)
	// ------------------------------------------------------------------

	private void OnSyncTraders(string json) {
		if (!g_Game || g_Game.IsDedicatedServer()) {
			return;
		}
		GetCache().ApplyTraderJson(json);
		NotifyCacheUpdated();
	}

	private void OnSyncQuestDefs(string traderId, string json) {
		if (!g_Game || g_Game.IsDedicatedServer()) {
			return;
		}
		GetCache().ApplyQuestChunkJson(traderId, json);
		NotifyCacheUpdated();
	}

	private void OnSyncPlayerState(string json) {
		if (!g_Game || g_Game.IsDedicatedServer()) {
			return;
		}
		GetCache().ApplyPlayerStateJson(json);
		NotifyCacheUpdated();
	}

	private void OnSyncComplete() {
		if (!g_Game || g_Game.IsDedicatedServer()) {
			return;
		}
		GetCache().SyncComplete = true;
		NotifyCacheUpdated();
	}

	private void OnObjectiveProgress(string questId, string objectiveId, int current) {
		if (!g_Game || g_Game.IsDedicatedServer()) {
			return;
		}
		GetCache().ApplyObjectiveProgress(questId, objectiveId, current);
		NotifyCacheUpdated();
	}

	private void OnQuestStateChanged(string questId, int state) {
		if (!g_Game || g_Game.IsDedicatedServer()) {
			return;
		}
		GetCache().ApplyQuestStateChanged(questId, state);
		NotifyCacheUpdated();
	}

	//! Reward-Payload (JSON) wird hier nicht geparst — Toast-Darstellung macht Agent 15
	//! (eigenes Abo auf s_DME_OnRewardGranted); hier nur Anzeige-Refresh.
	private void OnRewardGranted(string json) {
		if (!g_Game || g_Game.IsDedicatedServer()) {
			return;
		}
		NotifyCacheUpdated();
	}

	private void OnNotification(string title, string message, int type) {
		if (!g_Game || g_Game.IsDedicatedServer()) {
			return;
		}
		DME_Tasks_Notifier.Show(title, message, type);
	}

	// ------------------------------------------------------------------
	// Intern
	// ------------------------------------------------------------------

	private void NotifyCacheUpdated() {
		if (m_DME_ActiveMenu) {
			m_DME_ActiveMenu.Refresh();
		}
		DME_Tasks_Tracker.OnCacheUpdated();
	}
}
