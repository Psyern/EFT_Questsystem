//! Reputation, Umsatz und Loyalitaetsstufen pro Spieler+Haendler (server-only).
//! Reputation ist auf [-1.0 .. 1.0] geklemmt. Loyalitaet = hoechstes LoyaltyLevel,
//! dessen Anforderungen (RequiredPlayerLevel/RequiredReputation/RequiredTurnover) erfuellt sind;
//! erfuellt keine Stufe die Anforderungen, gilt Basisstufe 1.
class DME_Tasks_TraderService {
	private static ref DME_Tasks_TraderService s_DME_Instance;

	static DME_Tasks_TraderService GetInstance() {
		if (!s_DME_Instance) {
			s_DME_Instance = new DME_Tasks_TraderService();
		}
		return s_DME_Instance;
	}

	//! Reputation aendern (delta darf negativ sein), clamp, Loyalty-Recalc, Dirty.
	void AddReputation(string uid, string traderId, float delta) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (uid == "" || traderId == "") {
			DME_Tasks_Log.Warn("TraderService.AddReputation: uid oder traderId leer — ignoriert");
			return;
		}

		DME_Tasks_PlayerState state = GetState(uid);
		if (!state) {
			DME_Tasks_Log.Warn("TraderService.AddReputation: kein PlayerState fuer %1 — ignoriert", uid);
			return;
		}

		DME_Tasks_TraderProgress progress = GetOrCreateProgress(state, traderId);
		progress.Reputation = Math.Clamp(progress.Reputation + delta, -1.0, 1.0);

		RecalculateLoyalty(uid, traderId);
		DME_Tasks_PlayerStore.GetInstance().MarkDirty(uid);
	}

	//! Umsatz erhoehen (z. B. bei Kaeufen), Loyalty-Recalc, Dirty.
	void AddTurnover(string uid, string traderId, int amount) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (uid == "" || traderId == "") {
			DME_Tasks_Log.Warn("TraderService.AddTurnover: uid oder traderId leer — ignoriert");
			return;
		}
		if (amount <= 0) {
			return;
		}

		DME_Tasks_PlayerState state = GetState(uid);
		if (!state) {
			DME_Tasks_Log.Warn("TraderService.AddTurnover: kein PlayerState fuer %1 — ignoriert", uid);
			return;
		}

		DME_Tasks_TraderProgress progress = GetOrCreateProgress(state, traderId);
		progress.Turnover = progress.Turnover + amount;

		RecalculateLoyalty(uid, traderId);
		DME_Tasks_PlayerStore.GetInstance().MarkDirty(uid);
	}

	//! 0.0 wenn Spieler/Haendler unbekannt.
	float GetReputation(string uid, string traderId) {
		DME_Tasks_PlayerState state = DME_Tasks_PlayerStore.GetInstance().Get(uid);
		if (!state) {
			return 0.0;
		}
		DME_Tasks_TraderProgress progress = state.TraderProgress.Get(traderId);
		if (!progress) {
			return 0.0;
		}
		return progress.Reputation;
	}

	//! Basisstufe 1 wenn Spieler/Haendler unbekannt.
	int GetLoyaltyLevel(string uid, string traderId) {
		DME_Tasks_PlayerState state = DME_Tasks_PlayerStore.GetInstance().Get(uid);
		if (!state) {
			return 1;
		}
		DME_Tasks_TraderProgress progress = state.TraderProgress.Get(traderId);
		if (!progress) {
			return 1;
		}
		return progress.LoyaltyLevel;
	}

	//! Hoechstes LoyaltyLevel ermitteln, dessen Anforderungen erfuellt sind (Basis = 1).
	void RecalculateLoyalty(string uid, string traderId) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		DME_Tasks_PlayerState state = GetState(uid);
		if (!state) {
			return;
		}
		DME_Tasks_TraderProgress progress = state.TraderProgress.Get(traderId);
		if (!progress) {
			return;
		}

		DME_Tasks_TraderDef traderDef = DME_Tasks_ConfigService.GetInstance().GetTrader(traderId);
		if (!traderDef) {
			DME_Tasks_Log.Warn("TraderService.RecalculateLoyalty: unbekannter Haendler %1 — Stufe bleibt %2", traderId, progress.LoyaltyLevel.ToString());
			return;
		}

		int bestLevel = 1;
		foreach (DME_Tasks_LoyaltyLevel loyaltyLevel : traderDef.LoyaltyLevels) {
			if (!loyaltyLevel) {
				continue;
			}
			bool meetsPlayerLevel = state.PlayerLevel >= loyaltyLevel.RequiredPlayerLevel;
			bool meetsReputation = progress.Reputation >= loyaltyLevel.RequiredReputation;
			bool meetsTurnover = progress.Turnover >= loyaltyLevel.RequiredTurnover;
			if (meetsPlayerLevel && meetsReputation && meetsTurnover && loyaltyLevel.Level > bestLevel) {
				bestLevel = loyaltyLevel.Level;
			}
		}

		if (progress.LoyaltyLevel != bestLevel) {
			progress.LoyaltyLevel = bestLevel;
			DME_Tasks_PlayerStore.GetInstance().MarkDirty(uid);
			DME_Tasks_Log.Info("Loyalitaet %1@%2 -> Stufe %3", uid, traderId, bestLevel.ToString());
		}
	}

	// ------------------------------------------------------------------
	// intern
	// ------------------------------------------------------------------

	//! Geladenen State bevorzugen, sonst laden (Rival-Rep kann auch Offline-relevante Daten treffen).
	private DME_Tasks_PlayerState GetState(string uid) {
		DME_Tasks_PlayerState state = DME_Tasks_PlayerStore.GetInstance().Get(uid);
		if (!state) {
			state = DME_Tasks_PlayerStore.GetInstance().LoadOrCreate(uid);
		}
		return state;
	}

	private DME_Tasks_TraderProgress GetOrCreateProgress(DME_Tasks_PlayerState state, string traderId) {
		DME_Tasks_TraderProgress progress = state.TraderProgress.Get(traderId);
		if (!progress) {
			progress = new DME_Tasks_TraderProgress();
			state.TraderProgress.Insert(traderId, progress);
		}
		return progress;
	}
}
