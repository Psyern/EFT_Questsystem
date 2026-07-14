//! Engine-Override (CONTRACTS §6.5, Agent 11): RETURN_TO_TRADER-Objectives OHNE Zone werden beim
//! RequestTurnIn-RPC auto-vervollstaendigt — der Spieler steht in diesem Moment beim Haendler-UI,
//! MustBeAlive ist damit trivially erfuellt. Objectives MIT Zone laufen stattdessen ueber den
//! DME_Tasks_ReturnHandler (ZONE_ENTER des ZoneTriggers).
//! super zuerst (Basis validiert nur + Notification, keine Transition); danach setzt
//! SetObjectiveProgress(required) den Fortschritt — die Engine prueft dabei selbst auf
//! READY_TO_TURN_IN, sendet Delta-RPC und markiert Dirty.
//!
//! Zusaetzlich: OnPlayerDied dispatcht ein PLAYER_DIED-Event auf den Bus (Cheap-Guard: nur wenn
//! der Spieler PLAYER_DIED-Refs im Index hat) — der DME_Tasks_SurviveHandler setzt darueber die
//! Ueberlebenszeit zurueck. Der Reset ist idempotent (SetObjectiveProgress(0)), ein etwaiger
//! zweiter Dispatch aus spaeteren Wellen ist daher unschaedlich. Mehrere modded-Layer derselben
//! Klasse im selben Modul chainen sauber (Praezedenz: Expansion Vehicles, 3x modded ItemBase).
modded class DME_Tasks_QuestEngine {
	override void OnPlayerDied(string uid) {
		super.OnPlayerDied(uid);

		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (uid == "") {
			return;
		}
		if (DME_Tasks_ObjectiveIndex.GetInstance().CountRefs(uid, EDME_Tasks_EventType.PLAYER_DIED) == 0) {
			return;
		}

		DME_Tasks_Event evt = new DME_Tasks_Event();
		evt.m_DME_EventType = EDME_Tasks_EventType.PLAYER_DIED;
		evt.m_DME_PlayerUid = uid;
		PlayerBase player = FindPlayerByUid(uid);
		if (player) {
			evt.m_DME_Position = player.GetPosition();
		}
		DME_Tasks_EventBus.GetInstance().Dispatch(evt);
	}

	override void RequestTurnIn(PlayerIdentity sender, string questId) {
		super.RequestTurnIn(sender, questId);

		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!sender) {
			return;
		}

		string uid = sender.GetId();
		DME_Tasks_ActiveQuest active = GetActiveQuest(uid, questId);
		if (!active) {
			return;
		}

		DME_Tasks_QuestDef def = GetQuestDef(questId);
		if (!def) {
			return;
		}

		foreach (DME_Tasks_ObjectiveDef objectiveDef : def.Objectives) {
			if (!objectiveDef) {
				continue;
			}
			//! Nur RETURN_TO_TRADER OHNE Zone (mit Zone uebernimmt der ReturnHandler)
			if (objectiveDef.Zone) {
				continue;
			}
			int objectiveType = DME_Tasks_EnumUtil.ObjectiveTypeFromString(objectiveDef.Type);
			if (objectiveType != EDME_Tasks_ObjectiveType.RETURN_TO_TRADER) {
				continue;
			}
			if (DME_Tasks_IsReturnObjectiveDone(active, objectiveDef.ObjectiveId)) {
				continue;
			}

			int required = objectiveDef.Amount;
			if (required < 1) {
				required = 1;
			}
			SetObjectiveProgress(uid, questId, objectiveDef.ObjectiveId, required);
		}
	}

	//! Eigener Done-Check ueber die oeffentlichen Model-Felder (private Engine-Helfer sind aus
	//! der modded class nicht erreichbar).
	private bool DME_Tasks_IsReturnObjectiveDone(DME_Tasks_ActiveQuest active, string objectiveId) {
		foreach (DME_Tasks_ObjectiveProgress progress : active.Objectives) {
			if (progress && progress.ObjectiveId == objectiveId) {
				return progress.Done;
			}
		}
		return false;
	}
}
