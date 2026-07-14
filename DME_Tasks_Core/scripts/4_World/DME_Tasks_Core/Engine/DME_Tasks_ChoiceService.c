//! Entscheidungen an Quests (Konzept §14, CONTRACTS §6.8 Agent 17) — Singleton, ausschliesslich Server.
//!
//! ApplyChoice (§6.8, exakt): Quest-Def muss die ChoiceId besitzen; Quest muss beim Spieler
//! ACTIVE oder READY_TO_TURN_IN sein; es darf noch KEINE Decision fuer diese questId existieren.
//! Die Decision wird EXAKT als "questId:choiceId" in PlayerState.Decisions abgelegt (identisches
//! Format parst DME_Tasks_UnlockLedger.ApplyDecision beim Reconnect-Rebuild), danach
//! MarkDirty + Notification + true.
//!
//! Design-Entscheidungen (dokumentiert):
//! - KEIN Quest-State-Wechsel: bleibt die Quest READY_TO_TURN_IN, macht erst ClaimReward den Abschluss.
//! - Rep-Effekte (inkl. Rival-Rep) werden hier NICHT angewendet — das macht ausschliesslich
//!   DME_Tasks_RewardService.ClaimQuestReward ueber die via GetChoiceIdFor aufgeloeste choiceId
//!   (Choice ERSETZT Quest-Rewards + wendet ChoiceDef.ReputationEffects an; keine Duplikate).
//! - Questlinien-Unlocks der Choice (SPEC §7 Agent 17: "Wahl A schaltet Linie A frei") werden sofort
//!   inkrementell via UnlockLedger.ApplyUnlocks angewendet; der Reconnect-Pfad rekonstruiert dasselbe
//!   deterministisch ueber UnlockLedger.RebuildFor aus PlayerState.Decisions.
//! - Benachrichtigungen (Erfolg + spezifische Fehlgruende) sendet dieser Service selbst;
//!   QuestEngine.MakeChoice sendet deshalb keine eigene Notification mehr (sonst doppelt).
class DME_Tasks_ChoiceService {
	private static ref DME_Tasks_ChoiceService s_DME_Instance;

	static DME_Tasks_ChoiceService GetInstance() {
		if (!s_DME_Instance) {
			s_DME_Instance = new DME_Tasks_ChoiceService();
		}
		return s_DME_Instance;
	}

	//! true = Decision gespeichert (Unlocks angewendet, Dirty markiert, Spieler benachrichtigt).
	bool ApplyChoice(string uid, string questId, string choiceId) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return false;
		}
		if (uid == "" || questId == "" || choiceId == "") {
			DME_Tasks_Log.Warn("ApplyChoice: ungueltige Parameter (uid/questId/choiceId leer)");
			return false;
		}

		DME_Tasks_QuestEngine engine = DME_Tasks_QuestEngine.GetInstance();

		DME_Tasks_PlayerState state = engine.GetPlayerState(uid);
		if (!state) {
			DME_Tasks_Log.Warn("ApplyChoice: PlayerState fuer %1 nicht geladen", uid);
			return false;
		}

		DME_Tasks_QuestDef def = engine.GetQuestDef(questId);
		if (!def) {
			engine.NotifyPlayer(uid, "Entscheidung", "Unbekannte Quest", EDME_Tasks_NotificationType.WARNING);
			return false;
		}

		DME_Tasks_ChoiceDef choice = FindChoice(def, choiceId);
		if (!choice) {
			engine.NotifyPlayer(uid, def.Title, "Unbekannte Entscheidung", EDME_Tasks_NotificationType.WARNING);
			return false;
		}

		DME_Tasks_ActiveQuest active = engine.GetActiveQuest(uid, questId);
		if (!active) {
			engine.NotifyPlayer(uid, def.Title, "Quest ist nicht aktiv", EDME_Tasks_NotificationType.WARNING);
			return false;
		}
		if (active.State != EDME_Tasks_QuestState.ACTIVE && active.State != EDME_Tasks_QuestState.READY_TO_TURN_IN) {
			engine.NotifyPlayer(uid, def.Title, "Entscheidung derzeit nicht moeglich", EDME_Tasks_NotificationType.WARNING);
			return false;
		}

		string existingChoiceId = GetChoiceIdFor(state, questId);
		if (existingChoiceId != "") {
			engine.NotifyPlayer(uid, def.Title, "Entscheidung wurde bereits getroffen", EDME_Tasks_NotificationType.WARNING);
			return false;
		}

		string decision = questId + ":" + choiceId;
		state.Decisions.Insert(decision);

		//! Questlinien-Unlocks sofort anwenden (feuert s_DME_OnUnlocksChanged; §14-Blockaden/Freischaltungen
		//! ueber RequiredDecisions/BlockedByDecisions prueft der PrereqEvaluator direkt gegen Decisions).
		DME_Tasks_UnlockLedger.GetInstance().ApplyUnlocks(uid, choice.Unlocks);

		DME_Tasks_PlayerStore.GetInstance().MarkDirty(uid);

		string label = choice.DisplayName;
		if (label == "") {
			label = choiceId;
		}
		engine.NotifyPlayer(uid, def.Title, "Entscheidung getroffen: " + label, EDME_Tasks_NotificationType.SUCCESS);
		DME_Tasks_Log.Info("Entscheidung %1 fuer Spieler %2 gespeichert", decision, uid);
		return true;
	}

	//! Loest die gespeicherte Decision "questId:choiceId" einer Quest zur choiceId auf — "" wenn keine.
	//! Wird von QuestEngine.ClaimReward genutzt, damit der RewardService Choice-Rewards +
	//! ReputationEffects statt der Standard-Quest-Rewards auszahlt.
	string GetChoiceIdFor(DME_Tasks_PlayerState state, string questId) {
		if (!state || questId == "") {
			return "";
		}

		string prefix = questId + ":";
		int prefixLength = prefix.Length();
		foreach (string decision : state.Decisions) {
			if (decision == "" || decision.Length() <= prefixLength) {
				continue;
			}
			if (decision.Substring(0, prefixLength) != prefix) {
				continue;
			}
			int choiceLength = decision.Length() - prefixLength;
			return decision.Substring(prefixLength, choiceLength);
		}
		return "";
	}

	// ------------------------------------------------------------------
	// intern
	// ------------------------------------------------------------------

	private DME_Tasks_ChoiceDef FindChoice(DME_Tasks_QuestDef def, string choiceId) {
		if (!def.Choices) {
			return null;
		}
		foreach (DME_Tasks_ChoiceDef choiceDef : def.Choices) {
			if (choiceDef && choiceDef.ChoiceId == choiceId) {
				return choiceDef;
			}
		}
		return null;
	}
}
