//! Statische Engine-Invoker (CONTRACTS §3.4 + §6.2) — Entkopplungs-Seam zwischen Core und Objectives/Integrations.
//! Objectives-PBO abonniert diese fuer den Active-Objective-Index; OriginService stempelt Reward-Items.
class DME_Tasks_EngineEvents {
	static ref ScriptInvoker s_DME_OnQuestActivated = new ScriptInvoker();     // (string uid, string questId)
	static ref ScriptInvoker s_DME_OnQuestDeactivated = new ScriptInvoker();   // (string uid, string questId) — Abandon/Complete/Fail/Expire → Index-Cleanup
	static ref ScriptInvoker s_DME_OnPlayerLoaded = new ScriptInvoker();       // (string uid) — nach Connect+Load (Index-Rebuild aktiver Quests)
	static ref ScriptInvoker s_DME_OnPlayerUnloaded = new ScriptInvoker();     // (string uid) — Disconnect
	static ref ScriptInvoker s_DME_OnRewardItemSpawned = new ScriptInvoker();  // (EntityAI item, string uid, string questId) — §6.2: RewardService feuert pro gespawntem Item
}
