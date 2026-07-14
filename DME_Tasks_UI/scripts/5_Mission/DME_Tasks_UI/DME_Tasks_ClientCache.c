//! Client-seitiger Datencache der Server-Sync-Daten (gehalten vom DME_Tasks_UIModule).
//! Die UI liest AUSSCHLIESSLICH aus diesem Cache — kein Gameplay-Code, keine Server-Logik.
//! JSON-Parsing MUSS ueber DME_Tasks_Json<T>.FromJson auf frisch allozierte Objekte laufen
//! (CONTRACTS 6.4 Fix 1: JsonSerializer mit basisklassen-typisierter Variable serialisiert leer).
class DME_Tasks_ClientCache {
	//! Haendler, gemerged per TraderId (RPC_SyncTraders kann in mehreren Teil-Listen kommen).
	ref map<string, ref DME_Tasks_TraderSyncEntry> Traders;
	//! Quest-Definitionen inkl. per-Spieler-State, gemerged per QuestId (RPC_SyncQuestDefs-Chunks).
	ref map<string, ref DME_Tasks_QuestSyncEntry> Quests;
	//! Letzter voller PlayerState-Sync (null bis zum ersten RPC_SyncPlayerState).
	ref DME_Tasks_PlayerState PlayerState;
	//! true nach RPC_SyncComplete; false ab erneutem RPC_RequestSync (Menue zeigt solange "Synchronisiere...").
	bool SyncComplete;

	//! Stabile Anzeige-Reihenfolge der Haendler (Einfuege-Reihenfolge der Syncs).
	private ref array<string> m_DME_TraderOrder;

	void DME_Tasks_ClientCache() {
		Traders = new map<string, ref DME_Tasks_TraderSyncEntry>();
		Quests = new map<string, ref DME_Tasks_QuestSyncEntry>();
		PlayerState = null;
		SyncComplete = false;
		m_DME_TraderOrder = new array<string>();
	}

	// ------------------------------------------------------------------
	// Apply (vom UIModule aus den ClientEvents-Invokern gerufen)
	// ------------------------------------------------------------------

	//! Payload von RPC_SyncTraders (DME_Tasks_TraderSyncList als JSON) einmergen.
	void ApplyTraderJson(string json) {
		DME_Tasks_TraderSyncList list = new DME_Tasks_TraderSyncList();
		bool ok = DME_Tasks_Json<DME_Tasks_TraderSyncList>.FromJson(list, json, "ClientCache.ApplyTraderJson");
		if (!ok) {
			return;
		}
		if (!list.Traders) {
			return;
		}

		for (int i = 0; i < list.Traders.Count(); i++) {
			DME_Tasks_TraderSyncEntry entry = list.Traders.Get(i);
			if (!entry) {
				continue;
			}
			if (entry.TraderId == "") {
				continue;
			}
			if (m_DME_TraderOrder.Find(entry.TraderId) == -1) {
				m_DME_TraderOrder.Insert(entry.TraderId);
			}
			Traders.Set(entry.TraderId, entry);
		}
	}

	//! Payload von RPC_SyncQuestDefs (DME_Tasks_QuestSyncChunk als JSON) einmergen.
	void ApplyQuestChunkJson(string traderId, string json) {
		DME_Tasks_QuestSyncChunk chunk = new DME_Tasks_QuestSyncChunk();
		bool ok = DME_Tasks_Json<DME_Tasks_QuestSyncChunk>.FromJson(chunk, json, "ClientCache.ApplyQuestChunkJson");
		if (!ok) {
			return;
		}
		if (!chunk.Quests) {
			return;
		}

		for (int i = 0; i < chunk.Quests.Count(); i++) {
			DME_Tasks_QuestSyncEntry entry = chunk.Quests.Get(i);
			if (!entry) {
				continue;
			}
			if (entry.QuestId == "") {
				continue;
			}
			if (entry.TraderId == "") {
				entry.TraderId = traderId;
			}
			Quests.Set(entry.QuestId, entry);
		}
	}

	//! Payload von RPC_SyncPlayerState (DME_Tasks_PlayerState als JSON) uebernehmen (ersetzt komplett).
	void ApplyPlayerStateJson(string json) {
		DME_Tasks_PlayerState state = new DME_Tasks_PlayerState();
		bool ok = DME_Tasks_Json<DME_Tasks_PlayerState>.FromJson(state, json, "ClientCache.ApplyPlayerStateJson");
		if (!ok) {
			return;
		}
		PlayerState = state;
	}

	//! Live-Delta RPC_ObjectiveProgress: Fortschritt im PlayerState aktualisieren.
	//! Fehlt der Progress-Eintrag (Delta vor State-Sync), wird er aus der QuestSyncEntry-Definition
	//! (Required = ObjectiveDef.Amount) nachgezogen.
	void ApplyObjectiveProgress(string questId, string objectiveId, int current) {
		DME_Tasks_ActiveQuest active = FindActiveQuest(questId);
		if (!active) {
			return;
		}

		DME_Tasks_ObjectiveProgress progress = null;
		for (int i = 0; i < active.Objectives.Count(); i++) {
			DME_Tasks_ObjectiveProgress candidate = active.Objectives.Get(i);
			if (!candidate) {
				continue;
			}
			if (candidate.ObjectiveId == objectiveId) {
				progress = candidate;
				break;
			}
		}

		if (!progress) {
			progress = new DME_Tasks_ObjectiveProgress();
			progress.ObjectiveId = objectiveId;
			progress.Required = FindRequiredAmount(questId, objectiveId);
			active.Objectives.Insert(progress);
		}

		progress.Current = current;
		bool isDone = false;
		if (progress.Required > 0 && progress.Current >= progress.Required) {
			isDone = true;
		}
		progress.Done = isDone;
	}

	//! Live-Delta RPC_QuestStateChanged: State in QuestSyncEntry UND PlayerState spiegeln.
	//! ACTIVE/READY_TO_TURN_IN ohne vorhandenen ActiveQuest-Eintrag legt einen Anzeige-Eintrag
	//! aus der QuestSyncEntry-Definition an; Terminal-States raeumen ActiveQuests/TrackedQuests auf.
	void ApplyQuestStateChanged(string questId, int state) {
		DME_Tasks_QuestSyncEntry entry = GetQuest(questId);
		if (entry) {
			entry.State = state;
		}

		if (!PlayerState) {
			return;
		}

		DME_Tasks_ActiveQuest active = FindActiveQuest(questId);

		bool isRunning = false;
		if (state == EDME_Tasks_QuestState.ACTIVE || state == EDME_Tasks_QuestState.READY_TO_TURN_IN) {
			isRunning = true;
		}

		if (isRunning) {
			if (!active) {
				active = CreateActiveQuestFromEntry(entry, questId);
				PlayerState.ActiveQuests.Insert(active);
			}
			active.State = state;
			return;
		}

		if (active) {
			int activeIdx = PlayerState.ActiveQuests.Find(active);
			if (activeIdx > -1) {
				PlayerState.ActiveQuests.Remove(activeIdx);
			}
		}

		int trackedIdx = PlayerState.TrackedQuests.Find(questId);
		if (trackedIdx > -1) {
			PlayerState.TrackedQuests.Remove(trackedIdx);
		}
	}

	//! Cache vollstaendig leeren (z.B. bei Reconnect).
	void Clear() {
		Traders.Clear();
		Quests.Clear();
		PlayerState = null;
		SyncComplete = false;
		m_DME_TraderOrder.Clear();
	}

	// ------------------------------------------------------------------
	// Query-Helfer fuer die UI (Agenten 14/15)
	// ------------------------------------------------------------------

	//! Frisches Array aller Quest-Eintraege eines Haendlers (nie null).
	array<ref DME_Tasks_QuestSyncEntry> GetQuestsForTrader(string traderId) {
		array<ref DME_Tasks_QuestSyncEntry> result = new array<ref DME_Tasks_QuestSyncEntry>();
		for (int i = 0; i < Quests.Count(); i++) {
			DME_Tasks_QuestSyncEntry entry = Quests.GetElement(i);
			if (!entry) {
				continue;
			}
			if (entry.TraderId == traderId) {
				result.Insert(entry);
			}
		}
		return result;
	}

	//! null bei unbekannter Id.
	DME_Tasks_TraderSyncEntry GetTrader(string traderId) {
		return Traders.Get(traderId);
	}

	//! null bei unbekannter Id.
	DME_Tasks_QuestSyncEntry GetQuest(string questId) {
		return Quests.Get(questId);
	}

	//! Frisches Array der Haendler-Ids in stabiler Sync-Reihenfolge (nie null).
	array<string> GetTraderIds() {
		array<string> result = new array<string>();
		for (int i = 0; i < m_DME_TraderOrder.Count(); i++) {
			result.Insert(m_DME_TraderOrder.Get(i));
		}
		return result;
	}

	//! Fortschritts-Eintrag eines aktiven Objectives; null wenn Quest nicht aktiv oder Objective unbekannt.
	DME_Tasks_ObjectiveProgress FindProgress(string questId, string objectiveId) {
		DME_Tasks_ActiveQuest active = FindActiveQuest(questId);
		if (!active) {
			return null;
		}
		for (int i = 0; i < active.Objectives.Count(); i++) {
			DME_Tasks_ObjectiveProgress progress = active.Objectives.Get(i);
			if (!progress) {
				continue;
			}
			if (progress.ObjectiveId == objectiveId) {
				return progress;
			}
		}
		return null;
	}

	//! true wenn die Quest aktuell am HUD-Tracker verfolgt wird.
	bool IsTracked(string questId) {
		if (!PlayerState) {
			return false;
		}
		if (PlayerState.TrackedQuests.Find(questId) > -1) {
			return true;
		}
		return false;
	}

	//! Aktiver Quest-Eintrag aus dem PlayerState; null wenn nicht aktiv (oder noch kein State-Sync).
	DME_Tasks_ActiveQuest FindActiveQuest(string questId) {
		if (!PlayerState) {
			return null;
		}
		for (int i = 0; i < PlayerState.ActiveQuests.Count(); i++) {
			DME_Tasks_ActiveQuest active = PlayerState.ActiveQuests.Get(i);
			if (!active) {
				continue;
			}
			if (active.QuestId == questId) {
				return active;
			}
		}
		return null;
	}

	// ------------------------------------------------------------------
	// Intern
	// ------------------------------------------------------------------

	//! Required-Wert (ObjectiveDef.Amount) aus der Quest-Definition; 1 als Fallback.
	private int FindRequiredAmount(string questId, string objectiveId) {
		DME_Tasks_QuestSyncEntry entry = GetQuest(questId);
		if (!entry) {
			return 1;
		}
		if (!entry.Objectives) {
			return 1;
		}
		for (int i = 0; i < entry.Objectives.Count(); i++) {
			DME_Tasks_ObjectiveDef def = entry.Objectives.Get(i);
			if (!def) {
				continue;
			}
			if (def.ObjectiveId == objectiveId) {
				return def.Amount;
			}
		}
		return 1;
	}

	//! Anzeige-ActiveQuest aus der Sync-Definition bauen (entry darf null sein).
	private DME_Tasks_ActiveQuest CreateActiveQuestFromEntry(DME_Tasks_QuestSyncEntry entry, string questId) {
		DME_Tasks_ActiveQuest active = new DME_Tasks_ActiveQuest();
		active.QuestId = questId;
		if (!entry) {
			return active;
		}
		if (!entry.Objectives) {
			return active;
		}
		for (int i = 0; i < entry.Objectives.Count(); i++) {
			DME_Tasks_ObjectiveDef def = entry.Objectives.Get(i);
			if (!def) {
				continue;
			}
			DME_Tasks_ObjectiveProgress progress = new DME_Tasks_ObjectiveProgress();
			progress.ObjectiveId = def.ObjectiveId;
			progress.Required = def.Amount;
			progress.Current = 0;
			progress.Done = false;
			active.Objectives.Insert(progress);
		}
		return active;
	}
}
