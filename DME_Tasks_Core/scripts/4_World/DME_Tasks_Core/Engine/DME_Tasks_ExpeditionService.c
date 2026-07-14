//! Expedition-Session-Service (SPEC §7 Agent 12 + Konzept §11) — Singleton, ausschliesslich Dedicated Server.
//! Verwaltet pro (uid, questId) den DME_Tasks_ExpeditionState fuer Quests mit EXTRACT-Objective:
//! Session-Anlage bei Quest-Aktivierung (Abo s_DME_OnQuestActivated), EnteredZone-/Extracted-Pflege,
//! Tod- und Combat-Logout-Fails. Der State liegt in ActiveQuest.Expedition und wird mit dem
//! PlayerState-JSON persistiert — Sessions ueberleben Reconnects automatisch (bei erneuter
//! Aktivierung derselben Quest wird KEINE neue Session angelegt).
//!
//! Dokumentierte Fehlbedingungen (NUR diese failen die Expedition):
//! - Tod: Session aktiv + nicht extrahiert → FailQuest("Tod in Expedition"), unabhaengig von
//!   FailOnDeath (FailOnDeath-Quests failt bereits Engine.OnPlayerDied — dieser Service laeuft
//!   danach und pflegt nur noch die verbliebenen Expeditions-Quests).
//! - Zeitlimit: macht die Engine selbst (ExpiresAt-Expiry → EXPIRED), hier nichts zu tun.
//! - Combat-Logout: Disconnect < COMBAT_LOGOUT_WINDOW_SECONDS nach letztem ReportCombat →
//!   DisconnectedInCombat = true + FailQuest("Combat-Logout").
//! Zone-Verlassen failt NICHT automatisch (Entscheidung) — es bricht nur eine laufende Extraktion
//! ab (macht der DME_Tasks_ExtractHandler im Objectives-PBO ueber ExtractStartedAt).
//!
//! KONTRAKT fuer Agent 9 (Objectives, Hooks/DME_Tasks_KillHook.c): EEHitBy MUSS bei Spieler-Opfern
//! DME_Tasks_ExpeditionService.GetInstance().ReportCombat(victimUid) rufen (server-only), sonst
//! greift der Combat-Logout-Check nie. Core kennt die Objectives-Hooks bewusst NICHT (§2) —
//! die Kopplung laeuft ausschliesslich ueber diesen public Melde-Pfad.
//!
//! Bootstrap: Der Ctor abonniert die EngineEvents-Invoker; die Erst-Erzeugung (GetInstance) macht
//! das Objectives-PBO beim Server-Init (Hooks/DME_Tasks_EngineDeathHook.c, modded QuestEngine).
class DME_Tasks_ExpeditionService {
	private static ref DME_Tasks_ExpeditionService s_DME_Instance;

	private static const int COMBAT_LOGOUT_WINDOW_SECONDS = 60;

	//! uid → Epoch des letzten gemeldeten Kampfkontakts (ReportCombat); Cleanup beim Disconnect
	private ref map<string, int> m_DME_LastCombatEpoch;

	static DME_Tasks_ExpeditionService GetInstance() {
		if (!s_DME_Instance) {
			s_DME_Instance = new DME_Tasks_ExpeditionService();
		}
		return s_DME_Instance;
	}

	void DME_Tasks_ExpeditionService() {
		m_DME_LastCombatEpoch = new map<string, int>();

		DME_Tasks_EngineEvents.s_DME_OnQuestActivated.Insert(OnQuestActivated);
		DME_Tasks_EngineEvents.s_DME_OnPlayerUnloaded.Insert(OnPlayerDisconnected);
	}

	// ==================================================================
	// Session-Lifecycle
	// ==================================================================

	//! Abo s_DME_OnQuestActivated (feuert bei Accept UND je aktiver Quest beim Reconnect-Rebuild).
	//! Legt die Session NUR an, wenn die Quest ein EXTRACT-Objective hat und noch keine existiert
	//! (Reconnect: Expedition kommt bereits deserialisiert aus dem PlayerState-JSON).
	private void OnQuestActivated(string uid, string questId) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (uid == "" || questId == "") {
			return;
		}

		DME_Tasks_QuestEngine engine = DME_Tasks_QuestEngine.GetInstance();
		DME_Tasks_QuestDef def = engine.GetQuestDef(questId);
		if (!def || !HasExtractObjective(def)) {
			return;
		}

		DME_Tasks_ActiveQuest active = engine.GetActiveQuest(uid, questId);
		if (!active) {
			return;
		}
		if (active.Expedition) {
			//! Re-Attach nach Server-Crash: eine mitten in der Extraktion persistierte Startzeit
			//! darf nicht offline weiterlaufen (sonst extrahiert der Reconnect sofort).
			if (active.Expedition.ExtractStartedAt > 0) {
				active.Expedition.ExtractStartedAt = -1;
				DME_Tasks_PlayerStore.GetInstance().MarkDirty(uid);
			}
			return;
		}

		int now = DME_Tasks_TimeUtil.NowEpoch();

		DME_Tasks_ExpeditionState session = new DME_Tasks_ExpeditionState();
		session.SessionId = uid + ":" + questId + ":" + now.ToString();
		session.QuestId = questId;
		session.StartedAt = now;
		active.Expedition = session;

		DME_Tasks_PlayerStore.GetInstance().MarkDirty(uid);
		DME_Tasks_Log.Info("Expedition gestartet: %1 (Session %2)", questId, session.SessionId);
	}

	//! Vom Objectives-PBO via modded QuestEngine.OnPlayerDied gerufen (super = FailOnDeath-Fails
	//! laufen ZUERST). Hier nur noch Expedition-State pflegen + Expedition-spezifische Fails:
	//! Session aktiv + nicht extrahiert → FailQuest, unabhaengig von FailOnDeath.
	void OnPlayerDied(string uid) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (uid == "") {
			return;
		}

		DME_Tasks_QuestEngine engine = DME_Tasks_QuestEngine.GetInstance();
		DME_Tasks_PlayerState state = engine.GetPlayerState(uid);
		if (!state) {
			return;
		}

		bool changed = false;
		array<string> toFail = new array<string>();
		foreach (DME_Tasks_ActiveQuest active : state.ActiveQuests) {
			if (!active || !active.Expedition) {
				continue;
			}
			DME_Tasks_ExpeditionState session = active.Expedition;
			if (session.Extracted) {
				continue;
			}

			session.PlayerDied = true;
			session.ExtractStartedAt = -1;
			changed = true;

			DME_Tasks_QuestDef def = engine.GetQuestDef(active.QuestId);
			if (def && HasExtractObjective(def)) {
				toFail.Insert(active.QuestId);
			}
		}

		if (changed) {
			DME_Tasks_PlayerStore.GetInstance().MarkDirty(uid);
		}

		foreach (string failQuestId : toFail) {
			engine.FailQuest(uid, failQuestId, "Tod in Expedition");
		}
	}

	//! Abo s_DME_OnPlayerUnloaded — laeuft VOR PlayerStore.Unload (Engine.OnPlayerDisconnect-
	//! Reihenfolge), Aenderungen werden also vom Unload-SaveNow noch mitgeschrieben.
	//! Combat-Logout-Check + Abbruch einer laufenden Extraktion (kein Offline-Weiterzaehlen:
	//! ohne Reset wuerde ein Reconnect nach Ablauf der Extraktionszeit sofort extrahieren).
	void OnPlayerDisconnected(string uid) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (uid == "") {
			return;
		}

		bool inCombat = false;
		int lastCombat;
		if (m_DME_LastCombatEpoch.Find(uid, lastCombat)) {
			int now = DME_Tasks_TimeUtil.NowEpoch();
			if (now - lastCombat < COMBAT_LOGOUT_WINDOW_SECONDS) {
				inCombat = true;
			}
		}
		m_DME_LastCombatEpoch.Remove(uid);

		DME_Tasks_QuestEngine engine = DME_Tasks_QuestEngine.GetInstance();
		DME_Tasks_PlayerState state = engine.GetPlayerState(uid);
		if (!state) {
			return;
		}

		bool changed = false;
		array<string> toFail = new array<string>();
		foreach (DME_Tasks_ActiveQuest active : state.ActiveQuests) {
			if (!active || !active.Expedition) {
				continue;
			}
			DME_Tasks_ExpeditionState session = active.Expedition;
			if (session.Extracted) {
				continue;
			}

			if (session.ExtractStartedAt > 0) {
				session.ExtractStartedAt = -1;
				changed = true;
			}

			if (inCombat) {
				session.DisconnectedInCombat = true;
				changed = true;
				DME_Tasks_QuestDef def = engine.GetQuestDef(active.QuestId);
				if (def && HasExtractObjective(def)) {
					toFail.Insert(active.QuestId);
				}
			}
		}

		if (changed) {
			DME_Tasks_PlayerStore.GetInstance().MarkDirty(uid);
		}

		foreach (string failQuestId : toFail) {
			engine.FailQuest(uid, failQuestId, "Combat-Logout");
		}
	}

	// ==================================================================
	// Zonen-/Extraktions-State (vom DME_Tasks_ExtractHandler gerufen)
	// ==================================================================

	//! EnteredZone-Pflege (Praesenz-Flag). Zone-Betreten/-Verlassen failt NICHTS.
	void OnZoneEnter(string uid, string questId, string zoneId) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		DME_Tasks_ExpeditionState session = GetSession(uid, questId);
		if (!session) {
			return;
		}
		if (session.EnteredZone) {
			return;
		}

		session.EnteredZone = true;
		DME_Tasks_PlayerStore.GetInstance().MarkDirty(uid);
		DME_Tasks_Log.Debug("Expedition %1: %2 hat Zone %3 betreten", questId, uid, zoneId);
	}

	void OnZoneLeave(string uid, string questId, string zoneId) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		DME_Tasks_ExpeditionState session = GetSession(uid, questId);
		if (!session) {
			return;
		}
		if (!session.EnteredZone) {
			return;
		}

		session.EnteredZone = false;
		DME_Tasks_PlayerStore.GetInstance().MarkDirty(uid);
		DME_Tasks_Log.Debug("Expedition %1: %2 hat Zone %3 verlassen", questId, uid, zoneId);
	}

	//! Erfolgreiche Extraktion (ExtractHandler nach Ablauf der Extraktionszeit im Punkt).
	void OnExtracted(string uid, string questId) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		DME_Tasks_ExpeditionState session = GetSession(uid, questId);
		if (!session) {
			return;
		}
		if (session.Extracted) {
			return;
		}

		session.Extracted = true;
		DME_Tasks_PlayerStore.GetInstance().MarkDirty(uid);
		DME_Tasks_Log.Info("Expedition %1: %2 erfolgreich extrahiert", questId, uid);
	}

	// ==================================================================
	// Combat-Meldung (Seam fuer den Kill-/Hit-Hook, Agent 9)
	// ==================================================================

	//! MUSS von DME_Tasks_KillHook.EEHitBy bei Spieler-Opfern gerufen werden (uid = Opfer-UID,
	//! identity.GetId()). Fenster fuer den Combat-Logout-Check: COMBAT_LOGOUT_WINDOW_SECONDS.
	void ReportCombat(string uid) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (uid == "") {
			return;
		}

		int now = DME_Tasks_TimeUtil.NowEpoch();
		m_DME_LastCombatEpoch.Set(uid, now);
	}

	// ==================================================================
	// intern
	// ==================================================================

	private DME_Tasks_ExpeditionState GetSession(string uid, string questId) {
		DME_Tasks_ActiveQuest active = DME_Tasks_QuestEngine.GetInstance().GetActiveQuest(uid, questId);
		if (!active) {
			return null;
		}
		return active.Expedition;
	}

	//! Direkter String-Vergleich statt EnumUtil (EnumUtil wuerde je unbekanntem Typ warnen);
	//! "EXTRACT" entspricht exakt der EnumUtil-Konvention (case-sensitiv UPPERCASE).
	private bool HasExtractObjective(DME_Tasks_QuestDef def) {
		foreach (DME_Tasks_ObjectiveDef objectiveDef : def.Objectives) {
			if (objectiveDef && objectiveDef.Type == "EXTRACT") {
				return true;
			}
		}
		return false;
	}
}
