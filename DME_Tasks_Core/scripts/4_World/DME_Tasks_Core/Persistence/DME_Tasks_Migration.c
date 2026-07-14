//! Schema-Migration fuer DME_Tasks_PlayerState (server-only, wird vom PlayerStore nach dem Laden gerufen).
//! Struktur: while-Schleife ueber state.Version — kuenftige Versionen ergaenzen genau EINEN case,
//! der die Felder hochzieht und state.Version um 1 anhebt (z. B. "case 1: MigrateV1ToV2(state); break;").
//! Unabhaengig von der Version laeuft IMMER RepairCollections: JsonFileLoader kann ref-Arrays/Maps
//! null lassen (fehlende Keys in der JSON) — alle Collections werden defensiv nachalloziiert und
//! null-Eintraege entfernt.
class DME_Tasks_Migration {
	static void Migrate(DME_Tasks_PlayerState state) {
		if (!state) {
			DME_Tasks_Log.Warn("Migration: null-State ignoriert");
			return;
		}

		int currentVersion = DME_Tasks_Const.STATE_SCHEMA_VERSION;

		if (state.Version > currentVersion) {
			DME_Tasks_Log.Warn("PlayerState %1: Schema-Version %2 ist NEUER als aktuell %3 — Daten bleiben unveraendert (Downgrade-Schutz)", state.SteamId, state.Version.ToString(), currentVersion.ToString());
			RepairCollections(state);
			return;
		}

		while (state.Version < currentVersion) {
			int fromVersion = state.Version;

			switch (state.Version) {
				// Kuenftige Migrationen hier ergaenzen — jeder case hebt genau eine Version an:
				// case 1:
				// 	MigrateV1ToV2(state);
				// 	break;
				default:
					// Unbekannte/fehlende Version (z. B. 0, wenn die JSON kein "Version"-Feld hatte):
					// Felder-Upgrade fuer Version 1 = reine null-Reparatur (RepairCollections unten).
					DME_Tasks_Log.Warn("PlayerState %1: Schema-Version %2 ohne Migrationspfad — hebe direkt auf %3", state.SteamId, fromVersion.ToString(), currentVersion.ToString());
					state.Version = currentVersion;
					break;
			}

			if (state.Version <= fromVersion) {
				DME_Tasks_Log.Error("Migration ohne Fortschritt bei Version %1 — Force-Upgrade auf %2", fromVersion.ToString(), currentVersion.ToString());
				state.Version = currentVersion;
			}
		}

		state.Version = currentVersion;
		RepairCollections(state);
	}

	//! Defensive Reparatur: alle ref-Collections nachalloziieren, null-Eintraege entfernen.
	private static void RepairCollections(DME_Tasks_PlayerState state) {
		int repaired = 0;

		if (!state.TraderProgress) {
			state.TraderProgress = new map<string, ref DME_Tasks_TraderProgress>();
			repaired++;
		} else {
			repaired += RepairTraderProgress(state);
		}

		if (!state.ActiveQuests) {
			state.ActiveQuests = new array<ref DME_Tasks_ActiveQuest>();
			repaired++;
		} else {
			repaired += RepairActiveQuests(state);
		}

		if (!state.CompletedQuests) {
			state.CompletedQuests = new array<ref DME_Tasks_CompletedQuest>();
			repaired++;
		} else {
			repaired += RepairCompletedQuests(state);
		}

		if (!state.Cooldowns) {
			state.Cooldowns = new map<string, int>();
			repaired++;
		}

		if (!state.Decisions) {
			state.Decisions = new array<string>();
			repaired++;
		}

		if (!state.TrackedQuests) {
			state.TrackedQuests = new array<string>();
			repaired++;
		}

		if (!state.Transactions) {
			state.Transactions = new array<ref DME_Tasks_TxRecord>();
			repaired++;
		} else {
			repaired += RepairTransactions(state);
		}

		if (repaired > 0) {
			DME_Tasks_Log.Debug("PlayerState %1: %2 Collection-Reparatur(en) durchgefuehrt", state.SteamId, repaired.ToString());
		}
	}

	private static int RepairTraderProgress(DME_Tasks_PlayerState state) {
		int repaired = 0;

		array<string> badKeys = new array<string>();
		foreach (string traderId, DME_Tasks_TraderProgress progress : state.TraderProgress) {
			if (!progress) {
				badKeys.Insert(traderId);
			}
		}
		foreach (string badKey : badKeys) {
			state.TraderProgress.Remove(badKey);
			repaired++;
		}

		return repaired;
	}

	private static int RepairActiveQuests(DME_Tasks_PlayerState state) {
		int repaired = 0;

		for (int i = state.ActiveQuests.Count() - 1; i >= 0; i--) {
			DME_Tasks_ActiveQuest activeQuest = state.ActiveQuests.Get(i);
			if (!activeQuest) {
				state.ActiveQuests.RemoveOrdered(i);
				repaired++;
				continue;
			}

			if (!activeQuest.Objectives) {
				activeQuest.Objectives = new array<ref DME_Tasks_ObjectiveProgress>();
				repaired++;
			} else {
				for (int j = activeQuest.Objectives.Count() - 1; j >= 0; j--) {
					DME_Tasks_ObjectiveProgress objectiveProgress = activeQuest.Objectives.Get(j);
					if (!objectiveProgress) {
						activeQuest.Objectives.RemoveOrdered(j);
						repaired++;
					}
				}
			}

			// Expedition darf null bleiben (wird erst vom ExpeditionService alloziert).
		}

		return repaired;
	}

	private static int RepairCompletedQuests(DME_Tasks_PlayerState state) {
		int repaired = 0;

		for (int i = state.CompletedQuests.Count() - 1; i >= 0; i--) {
			DME_Tasks_CompletedQuest completedQuest = state.CompletedQuests.Get(i);
			if (!completedQuest) {
				state.CompletedQuests.RemoveOrdered(i);
				repaired++;
			}
		}

		return repaired;
	}

	private static int RepairTransactions(DME_Tasks_PlayerState state) {
		int repaired = 0;

		for (int i = state.Transactions.Count() - 1; i >= 0; i--) {
			DME_Tasks_TxRecord txRecord = state.Transactions.Get(i);
			if (!txRecord) {
				state.Transactions.RemoveOrdered(i);
				repaired++;
			}
		}

		return repaired;
	}
}
