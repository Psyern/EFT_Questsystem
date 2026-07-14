//! In-Memory-Set der Freischaltungen eines Spielers (nicht persistiert — deterministisch
//! aus dem PlayerState rekonstruierbar, siehe DME_Tasks_UnlockLedger.RebuildFor).
class DME_Tasks_UnlockSet {
	ref map<string, bool> m_DME_MarketItems;
	ref map<string, bool> m_DME_QuestIds;
	ref map<string, bool> m_DME_Keys;
	ref map<string, bool> m_DME_BossAccess;

	void DME_Tasks_UnlockSet() {
		m_DME_MarketItems = new map<string, bool>();
		m_DME_QuestIds = new map<string, bool>();
		m_DME_Keys = new map<string, bool>();
		m_DME_BossAccess = new map<string, bool>();
	}
}

//! Freigeschaltete Market-Items/Quests/Keys/Boss-Zugaenge pro Spieler (server-only, in-memory).
//! Quelle der Wahrheit ist der PlayerState: RebuildFor baut die Sets deterministisch und
//! idempotent aus CompletedQuests (QuestDef.Unlocks) + Decisions ("questId:choiceId" ->
//! ChoiceDef.Unlocks) neu auf. ApplyUnlocks ergaenzt inkrementell (z. B. direkt beim Claim).
//! Beide Wege feuern DME_Tasks_IntegrationEvents.s_DME_OnUnlocksChanged(uid)
//! (RebuildFor genau einmal am Ende), damit z. B. der Market-Adapter neu anwenden kann.
class DME_Tasks_UnlockLedger {
	private static ref DME_Tasks_UnlockLedger s_DME_Instance;

	private ref map<string, ref DME_Tasks_UnlockSet> m_DME_ByUid;

	static DME_Tasks_UnlockLedger GetInstance() {
		if (!s_DME_Instance) {
			s_DME_Instance = new DME_Tasks_UnlockLedger();
		}
		return s_DME_Instance;
	}

	void DME_Tasks_UnlockLedger() {
		m_DME_ByUid = new map<string, ref DME_Tasks_UnlockSet>();
	}

	//! Eviction beim Disconnect (Speicherwachstum) — LoadOrCreate baut beim Reconnect via RebuildFor neu auf.
	void RemoveFor(string uid) {
		if (m_DME_ByUid.Contains(uid)) {
			m_DME_ByUid.Remove(uid);
		}
	}

	//! Deterministischer Neuaufbau aus dem PlayerState (idempotent) — von PlayerStore.LoadOrCreate gerufen.
	void RebuildFor(string uid) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (uid == "") {
			return;
		}

		DME_Tasks_UnlockSet unlockSet = new DME_Tasks_UnlockSet();
		m_DME_ByUid.Set(uid, unlockSet);

		DME_Tasks_PlayerState state = DME_Tasks_PlayerStore.GetInstance().Get(uid);
		if (!state) {
			DME_Tasks_Log.Warn("UnlockLedger.RebuildFor: kein PlayerState fuer %1 — Set geleert", uid);
			DME_Tasks_IntegrationEvents.s_DME_OnUnlocksChanged.Invoke(uid);
			return;
		}

		DME_Tasks_ConfigService config = DME_Tasks_ConfigService.GetInstance();

		foreach (DME_Tasks_CompletedQuest completed : state.CompletedQuests) {
			if (!completed || completed.QuestId == "") {
				continue;
			}
			DME_Tasks_QuestDef quest = config.GetQuest(completed.QuestId);
			if (!quest) {
				DME_Tasks_Log.Debug("UnlockLedger.RebuildFor: Quest %1 nicht (mehr) konfiguriert", completed.QuestId);
				continue;
			}
			ApplyToSet(unlockSet, quest.Unlocks);
		}

		foreach (string decision : state.Decisions) {
			ApplyDecision(unlockSet, decision);
		}

		DME_Tasks_IntegrationEvents.s_DME_OnUnlocksChanged.Invoke(uid);
	}

	//! Sets inkrementell ergaenzen + Unlocks-Changed-Event.
	void ApplyUnlocks(string uid, DME_Tasks_UnlockDef unlocks) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (uid == "" || !unlocks) {
			return;
		}

		DME_Tasks_UnlockSet unlockSet = GetOrCreateSet(uid);
		ApplyToSet(unlockSet, unlocks);

		DME_Tasks_IntegrationEvents.s_DME_OnUnlocksChanged.Invoke(uid);
	}

	bool IsMarketItemUnlocked(string uid, string className) {
		DME_Tasks_UnlockSet unlockSet = m_DME_ByUid.Get(uid);
		if (!unlockSet) {
			return false;
		}
		return unlockSet.m_DME_MarketItems.Contains(className);
	}

	bool IsQuestUnlocked(string uid, string questId) {
		DME_Tasks_UnlockSet unlockSet = m_DME_ByUid.Get(uid);
		if (!unlockSet) {
			return false;
		}
		return unlockSet.m_DME_QuestIds.Contains(questId);
	}

	bool IsKeyUnlocked(string uid, string keyId) {
		DME_Tasks_UnlockSet unlockSet = m_DME_ByUid.Get(uid);
		if (!unlockSet) {
			return false;
		}
		return unlockSet.m_DME_Keys.Contains(keyId);
	}

	bool HasBossAccess(string uid, string bossId) {
		DME_Tasks_UnlockSet unlockSet = m_DME_ByUid.Get(uid);
		if (!unlockSet) {
			return false;
		}
		return unlockSet.m_DME_BossAccess.Contains(bossId);
	}

	//! Frisches Array (nie das interne Set herausgeben).
	array<string> GetUnlockedMarketItems(string uid) {
		array<string> result = new array<string>();
		DME_Tasks_UnlockSet unlockSet = m_DME_ByUid.Get(uid);
		if (!unlockSet) {
			return result;
		}
		foreach (string className, bool unused : unlockSet.m_DME_MarketItems) {
			result.Insert(className);
		}
		return result;
	}

	// ------------------------------------------------------------------
	// intern
	// ------------------------------------------------------------------

	private DME_Tasks_UnlockSet GetOrCreateSet(string uid) {
		DME_Tasks_UnlockSet unlockSet = m_DME_ByUid.Get(uid);
		if (!unlockSet) {
			unlockSet = new DME_Tasks_UnlockSet();
			m_DME_ByUid.Set(uid, unlockSet);
		}
		return unlockSet;
	}

	private void ApplyToSet(DME_Tasks_UnlockSet unlockSet, DME_Tasks_UnlockDef unlocks) {
		if (!unlocks) {
			return;
		}

		if (unlocks.MarketItems) {
			foreach (string marketItem : unlocks.MarketItems) {
				if (marketItem != "") {
					unlockSet.m_DME_MarketItems.Set(marketItem, true);
				}
			}
		}

		if (unlocks.QuestIds) {
			foreach (string questId : unlocks.QuestIds) {
				if (questId != "") {
					unlockSet.m_DME_QuestIds.Set(questId, true);
				}
			}
		}

		if (unlocks.Keys) {
			foreach (string keyId : unlocks.Keys) {
				if (keyId != "") {
					unlockSet.m_DME_Keys.Set(keyId, true);
				}
			}
		}

		if (unlocks.BossAccess != "") {
			unlockSet.m_DME_BossAccess.Set(unlocks.BossAccess, true);
		}
	}

	//! Decision-Format "questId:choiceId" -> ChoiceDef.Unlocks der gewaehlten Choice.
	private void ApplyDecision(DME_Tasks_UnlockSet unlockSet, string decision) {
		if (decision == "") {
			return;
		}

		int separator = decision.IndexOf(":");
		if (separator < 1) {
			DME_Tasks_Log.Warn("UnlockLedger: ungueltige Decision %1 — erwartet questId:choiceId", decision);
			return;
		}

		string questId = decision.Substring(0, separator);
		int choiceLength = decision.Length() - separator - 1;
		if (choiceLength < 1) {
			DME_Tasks_Log.Warn("UnlockLedger: Decision %1 ohne choiceId", decision);
			return;
		}
		string choiceId = decision.Substring(separator + 1, choiceLength);

		DME_Tasks_QuestDef quest = DME_Tasks_ConfigService.GetInstance().GetQuest(questId);
		if (!quest) {
			DME_Tasks_Log.Debug("UnlockLedger: Decision-Quest %1 nicht (mehr) konfiguriert", questId);
			return;
		}
		if (!quest.Choices) {
			return;
		}

		foreach (DME_Tasks_ChoiceDef choiceDef : quest.Choices) {
			if (choiceDef && choiceDef.ChoiceId == choiceId) {
				ApplyToSet(unlockSet, choiceDef.Unlocks);
				return;
			}
		}

		DME_Tasks_Log.Warn("UnlockLedger: Choice %1 nicht in Quest %2", choiceId, questId);
	}
}
