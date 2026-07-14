//! Engine-Death-Hook (CONTRACTS §6.5, Agent 12) — verdrahtet den Core-ExpeditionService mit dem
//! Engine-Death-Pfad und bootstrappt seine Invoker-Abos beim Server-Init.
//!
//! Pfad: Agent 9s PlayerBase-EEKilled-Hook ruft DME_Tasks_QuestEngine.GetInstance().OnPlayerDied(uid);
//! super failt zuerst alle FailOnDeath-Quests (Core-Basisimplementation), DANACH pflegt der
//! ExpeditionService den Expeditions-State und failt aktive, nicht extrahierte Expeditions-Quests
//! ("Tod in Expedition") unabhaengig von FailOnDeath.
//!
//! Bootstrap: OnServerInit (CoreModule.OnMissionStart, VOR jedem Spieler-Connect) erzeugt den
//! ExpeditionService — dessen Ctor abonniert s_DME_OnQuestActivated + s_DME_OnPlayerUnloaded.
//! Core selbst darf den Service nicht bootstrappen muessen (kein Objectives-Wissen in Core);
//! ohne Objectives-PBO gibt es ohnehin keine EXTRACT-Verarbeitung.
modded class DME_Tasks_QuestEngine {
	override void OnServerInit() {
		super.OnServerInit();

		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		DME_Tasks_ExpeditionService.GetInstance();
	}

	override void OnPlayerDied(string uid) {
		super.OnPlayerDied(uid);

		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		DME_Tasks_ExpeditionService.GetInstance().OnPlayerDied(uid);
	}
}
