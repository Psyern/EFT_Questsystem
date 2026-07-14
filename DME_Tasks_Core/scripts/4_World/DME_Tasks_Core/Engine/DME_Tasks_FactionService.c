//! Fraktions-Provider-Seam (CONTRACTS §6.8 Agent 17) — Singleton, ausschliesslich Server.
//!
//! Die Fraktion wird bewusst NICHT persistiert: Core besitzt keine autoritative Fraktions-Quelle.
//! GetFaction liefert per Default "NEUTRAL"; SetFaction pflegt nur einen fluechtigen In-Memory-Zustand
//! fuer die laufende Session. Integrations-Mods (Welle 4+) liefern die echte Quelle, indem sie diesen
//! Service via `modded class DME_Tasks_FactionService` ueberschreiben (GetFaction aus dem eigenen
//! Fraktionssystem beantworten) und/oder bei Wechseln SetFaction aufrufen. Nach einem Server-Neustart
//! ist der In-Memory-Zustand leer — der Provider muss die Fraktion erneut melden.
//!
//! SetFaction failt bei TATSAECHLICHEM Wechsel alle aktiven Quests des Spielers mit
//! FailOnFactionChange via QuestEngine.FailQuest (Konzept §14: Fraktionskonflikte) und
//! benachrichtigt den Spieler. Fraktions-Strings folgen der UPPERCASE-Konvention
//! (DME_Tasks_EnumUtil, z.B. "WEST"/"EAST"/"NEUTRAL") — Eingaben werden normalisiert.
class DME_Tasks_FactionService {
	private static ref DME_Tasks_FactionService s_DME_Instance;

	static const string DEFAULT_FACTION = "NEUTRAL";

	private ref map<string, string> m_DME_Factions;

	static DME_Tasks_FactionService GetInstance() {
		if (!s_DME_Instance) {
			s_DME_Instance = new DME_Tasks_FactionService();
		}
		return s_DME_Instance;
	}

	void DME_Tasks_FactionService() {
		m_DME_Factions = new map<string, string>();
	}

	//! Aktuelle Fraktion des Spielers — Default "NEUTRAL" (Provider-Seam, siehe Kopf-Doku).
	string GetFaction(string uid) {
		if (uid == "") {
			return DEFAULT_FACTION;
		}

		string faction;
		if (m_DME_Factions.Find(uid, faction) && faction != "") {
			return faction;
		}
		return DEFAULT_FACTION;
	}

	//! Setzt die Fraktion (in-memory). Bei tatsaechlichem Wechsel: alle aktiven Quests mit
	//! FailOnFactionChange schlagen fehl (FailQuest benachrichtigt je Quest) + MarkDirty.
	void SetFaction(string uid, string faction) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (uid == "") {
			return;
		}

		string newFaction = faction;
		newFaction.ToUpper();
		if (newFaction == "") {
			newFaction = DEFAULT_FACTION;
		}

		string oldFaction = GetFaction(uid);
		if (newFaction == oldFaction) {
			return;
		}

		m_DME_Factions.Set(uid, newFaction);
		DME_Tasks_Log.Info("Fraktion von %1 gewechselt: %2 -> %3", uid, oldFaction, newFaction);

		FailFactionQuests(uid, newFaction);
	}

	// ------------------------------------------------------------------
	// intern
	// ------------------------------------------------------------------

	private void FailFactionQuests(string uid, string newFaction) {
		DME_Tasks_QuestEngine engine = DME_Tasks_QuestEngine.GetInstance();

		DME_Tasks_PlayerState state = engine.GetPlayerState(uid);
		if (!state) {
			return;
		}

		array<string> toFail = new array<string>();
		foreach (DME_Tasks_ActiveQuest active : state.ActiveQuests) {
			if (!active) {
				continue;
			}
			DME_Tasks_QuestDef def = engine.GetQuestDef(active.QuestId);
			if (def && def.FailOnFactionChange) {
				toFail.Insert(active.QuestId);
			}
		}

		if (toFail.Count() > 0) {
			//! p1 = Fraktions-Display-Key ('#'-Regel: der Client loest den Param nested auf)
			string factionKey = DME_Tasks_EnumUtil.FactionDisplayKeyFromString(newFaction);
			engine.NotifyPlayer(uid, DME_Tasks_LocKeys.NOTIF_TITLE_FACTION, DME_Tasks_LocKeys.NOTIF_FACTION_CHANGE_WARNING, EDME_Tasks_NotificationType.WARNING, factionKey);
		}

		foreach (string failQuestId : toFail) {
			engine.FailQuest(uid, failQuestId, DME_Tasks_LocKeys.NOTIF_REASON_FACTION_CHANGED);
		}

		//! §6.8 "+ Dirty": FailQuest markiert bereits je Quest; dieser Aufruf deckt den Fall ohne
		//! FailOnFactionChange-Quests ab (harmlos — Coalescing-Flush schreibt identischen State).
		DME_Tasks_PlayerStore.GetInstance().MarkDirty(uid);
	}
}
