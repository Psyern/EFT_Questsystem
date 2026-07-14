//! Client-seitige Re-Publikation der Server→Client-RPCs (UI abonniert diese Invoker).
class DME_Tasks_ClientEvents {
	static ref ScriptInvoker s_DME_OnSyncTraders = new ScriptInvoker();       // (string json)
	static ref ScriptInvoker s_DME_OnSyncQuestDefs = new ScriptInvoker();     // (string traderId, string json)
	static ref ScriptInvoker s_DME_OnSyncPlayerState = new ScriptInvoker();   // (string json)
	static ref ScriptInvoker s_DME_OnSyncComplete = new ScriptInvoker();      // ()
	static ref ScriptInvoker s_DME_OnObjectiveProgress = new ScriptInvoker(); // (string questId, string objectiveId, int current)
	static ref ScriptInvoker s_DME_OnQuestStateChanged = new ScriptInvoker(); // (string questId, int state)
	static ref ScriptInvoker s_DME_OnRewardGranted = new ScriptInvoker();     // (string json)
	static ref ScriptInvoker s_DME_OnNotification = new ScriptInvoker();      // (string titleRaw, string msgKey, string p1, string p2, string p3, int type) — '#' rule: keys resolve client-side
}
