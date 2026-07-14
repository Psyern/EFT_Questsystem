//! Website-Adapter (CONTRACTS §6.8 + Konzept §22 "Webseite") — KEIN Fremd-Mod noetig
//! (AddonPatchName leer, immer aktivierbar); Toggle: Settings.EnableIntegrationWebsite.
//!
//! Schreibt bei Quest-Abschluss und Unlock-Aenderung eine PlayerState-Projektion nach
//! $profile:DME_Tasks/Website/<uid>.json (JsonFileLoader). Quelle: DME_Tasks_PlayerStore.Get(uid)
//! — null (Spieler nicht geladen) → Skip mit Debug-Log; plus DME_Tasks_UnlockLedger fuer die
//! freigeschalteten Market-Items. Ein Web-Backend liest die Dateien aus dem Server-Profil.

//! Export-Models (JSON-Member OHNE Prefix — Keys = Datei).
class DME_Tasks_WebsiteObjectiveExport {
	string ObjectiveId;
	int Current;
	int Required;
	bool Done;

	void DME_Tasks_WebsiteObjectiveExport() {
		ObjectiveId = "";
		Current = 0;
		Required = 1;
		Done = false;
	}
}

class DME_Tasks_WebsiteActiveQuestExport {
	string QuestId;
	int State;
	ref array<ref DME_Tasks_WebsiteObjectiveExport> Objectives;

	void DME_Tasks_WebsiteActiveQuestExport() {
		QuestId = "";
		State = 0;
		Objectives = new array<ref DME_Tasks_WebsiteObjectiveExport>();
	}
}

class DME_Tasks_WebsiteCompletedQuestExport {
	string QuestId;
	int CompletedAt;
	int TimesCompleted;

	void DME_Tasks_WebsiteCompletedQuestExport() {
		QuestId = "";
		CompletedAt = 0;
		TimesCompleted = 0;
	}
}

class DME_Tasks_WebsiteTraderExport {
	string TraderId;
	float Reputation;
	int Turnover;
	int LoyaltyLevel;

	void DME_Tasks_WebsiteTraderExport() {
		TraderId = "";
		Reputation = 0.0;
		Turnover = 0;
		LoyaltyLevel = 1;
	}
}

class DME_Tasks_WebsiteExport {
	int Version;
	string Uid;
	int UpdatedAt;
	int PlayerLevel;
	int Experience;
	ref array<ref DME_Tasks_WebsiteActiveQuestExport> ActiveQuests;
	ref array<ref DME_Tasks_WebsiteCompletedQuestExport> CompletedQuests;
	ref array<ref DME_Tasks_WebsiteTraderExport> TraderRep;
	ref array<string> UnlockedItems;
	ref array<string> Decisions;

	void DME_Tasks_WebsiteExport() {
		Version = 1;
		Uid = "";
		UpdatedAt = 0;
		PlayerLevel = 1;
		Experience = 0;
		ActiveQuests = new array<ref DME_Tasks_WebsiteActiveQuestExport>();
		CompletedQuests = new array<ref DME_Tasks_WebsiteCompletedQuestExport>();
		TraderRep = new array<ref DME_Tasks_WebsiteTraderExport>();
		UnlockedItems = new array<string>();
		Decisions = new array<string>();
	}
}

class DME_Tasks_WebsiteAdapter : DME_Tasks_IntegrationBase {
	void DME_Tasks_WebsiteAdapter() {
		m_DME_Name = "Website";
		m_DME_AddonPatchName = "";
	}

	override bool IsEnabled(DME_Tasks_Settings settings) {
		return settings.EnableIntegrationWebsite;
	}

	override void OnActivate() {
		DME_Tasks_IntegrationPaths.EnsureWebsiteDir();

		DME_Tasks_IntegrationEvents.s_DME_OnQuestCompleted.Insert(OnQuestCompleted);
		DME_Tasks_IntegrationEvents.s_DME_OnUnlocksChanged.Insert(OnUnlocksChanged);

		DME_Tasks_Log.Info("WebsiteAdapter: Export aktiv — Website/<uid>.json bei Quest-Abschluss und Unlock-Aenderung");
	}

	// ------------------------------------------------------------------
	// Invoker-Abos (nur bei aktivem Adapter registriert)
	// ------------------------------------------------------------------

	private void OnQuestCompleted(string uid, string questId) {
		ExportPlayer(uid);
	}

	private void OnUnlocksChanged(string uid) {
		ExportPlayer(uid);
	}

	// ------------------------------------------------------------------
	// Export
	// ------------------------------------------------------------------

	private void ExportPlayer(string uid) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (uid == "") {
			return;
		}

		DME_Tasks_PlayerState state = DME_Tasks_PlayerStore.GetInstance().Get(uid);
		if (!state) {
			DME_Tasks_Log.Debug("WebsiteAdapter: kein geladener PlayerState fuer %1 — Export uebersprungen", uid);
			return;
		}

		DME_Tasks_WebsiteExport data = BuildExport(uid, state);

		DME_Tasks_IntegrationPaths.EnsureWebsiteDir();
		string path = DME_Tasks_IntegrationPaths.WebsiteFile(uid);
		string saveError;
		bool savedOk = JsonFileLoader<DME_Tasks_WebsiteExport>.SaveFile(path, data, saveError);
		if (!savedOk) {
			DME_Tasks_Log.Warn("WebsiteAdapter: Export fehlgeschlagen (%1): %2", path, saveError);
		}
	}

	private DME_Tasks_WebsiteExport BuildExport(string uid, DME_Tasks_PlayerState state) {
		DME_Tasks_WebsiteExport data = new DME_Tasks_WebsiteExport();
		data.Uid = uid;
		data.UpdatedAt = DME_Tasks_TimeUtil.NowEpoch();
		data.PlayerLevel = state.PlayerLevel;
		data.Experience = state.Experience;

		foreach (DME_Tasks_ActiveQuest activeQuest : state.ActiveQuests) {
			if (!activeQuest) {
				continue;
			}
			DME_Tasks_WebsiteActiveQuestExport questExport = new DME_Tasks_WebsiteActiveQuestExport();
			questExport.QuestId = activeQuest.QuestId;
			questExport.State = activeQuest.State;
			foreach (DME_Tasks_ObjectiveProgress progress : activeQuest.Objectives) {
				if (!progress) {
					continue;
				}
				DME_Tasks_WebsiteObjectiveExport objectiveExport = new DME_Tasks_WebsiteObjectiveExport();
				objectiveExport.ObjectiveId = progress.ObjectiveId;
				objectiveExport.Current = progress.Current;
				objectiveExport.Required = progress.Required;
				objectiveExport.Done = progress.Done;
				questExport.Objectives.Insert(objectiveExport);
			}
			data.ActiveQuests.Insert(questExport);
		}

		foreach (DME_Tasks_CompletedQuest completedQuest : state.CompletedQuests) {
			if (!completedQuest) {
				continue;
			}
			DME_Tasks_WebsiteCompletedQuestExport completedExport = new DME_Tasks_WebsiteCompletedQuestExport();
			completedExport.QuestId = completedQuest.QuestId;
			completedExport.CompletedAt = completedQuest.CompletedAt;
			completedExport.TimesCompleted = completedQuest.TimesCompleted;
			data.CompletedQuests.Insert(completedExport);
		}

		foreach (string traderId, DME_Tasks_TraderProgress traderProgress : state.TraderProgress) {
			if (!traderProgress) {
				continue;
			}
			DME_Tasks_WebsiteTraderExport traderExport = new DME_Tasks_WebsiteTraderExport();
			traderExport.TraderId = traderId;
			traderExport.Reputation = traderProgress.Reputation;
			traderExport.Turnover = traderProgress.Turnover;
			traderExport.LoyaltyLevel = traderProgress.LoyaltyLevel;
			data.TraderRep.Insert(traderExport);
		}

		data.UnlockedItems = DME_Tasks_UnlockLedger.GetInstance().GetUnlockedMarketItems(uid);

		foreach (string decision : state.Decisions) {
			if (decision != "") {
				data.Decisions.Insert(decision);
			}
		}

		return data;
	}
}
