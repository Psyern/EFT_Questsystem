//! Statische Integrations-Invoker (CONTRACTS §3.4 + §6.2) — Welle-4-Adapter (Market/Terje/Season/Website) abonnieren hier.
//! Ohne Abonnent: Aufrufer loggt Info, kein Fehler (README dokumentiert das).
class DME_Tasks_IntegrationEvents {
	static ref ScriptInvoker s_DME_OnGrantSkillPoints = new ScriptInvoker();   // (string uid, int points)
	static ref ScriptInvoker s_DME_OnGrantSeasonXp = new ScriptInvoker();      // (string uid, int xp)
	static ref ScriptInvoker s_DME_OnUnlocksChanged = new ScriptInvoker();     // (string uid)
	static ref ScriptInvoker s_DME_OnQuestCompleted = new ScriptInvoker();     // (string uid, string questId) — Website-Export etc.
	static ref ScriptInvoker s_DME_OnGrantCurrency = new ScriptInvoker();      // (string uid, int amount) — §6.2: Currency-Rewards laufen NUR hierueber
}
