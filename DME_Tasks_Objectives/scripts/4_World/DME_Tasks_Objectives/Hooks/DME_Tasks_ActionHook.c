//! Action-Abschluss-Hook (CONTRACTS §6.5, Agent 11) — quellcode-verifiziert in scripts - 1.29:
//! - AnimatedActionBase.OnExecuteServer(ActionData) (AnimatedActionBase.c:175, protected) —
//!   feuert beim UA_ANIM_EVENT der Einmal-Aktionen (ActionSingleUseBase & animierte Aktionen).
//! - ActionContinuousBase.OnFinishProgress(ActionData) (ActionContinuousBase.c:243) — Dispatcher
//!   beim ERFOLGREICHEN Abschluss einer Continuous-Action (Abbruch feuert nicht); feuert fuer ALLE
//!   Ableitungen (OnFinishProgressServer wird von vielen Actions OHNE super ueberschrieben).
//! - ActionData: m_Player/m_MainItem/m_Target (ActionBase.c:28ff); ActionTarget.GetObject/GetParent
//!   (ActionTargets.c:111ff); Object.GetType() (Object.c:473); Class.ClassName() (EnScript.c:37).
//!
//! Der gemeinsame Dispatcher lebt als DME_Tasks_-Methode auf modded ActionBase; Continuous-Actions
//! werden im OnExecuteServer-Pfad uebersprungen (sie melden ueber OnFinishProgress), damit
//! kein Abschluss doppelt zaehlt. Cheap-Guard: ohne ACTION_USED-/INTERACT-Refs im Index wird kein
//! Event alloziert. EventType-Wahl: Ziel-Objekt vorhanden → INTERACT, sonst ACTION_USED (der
//! DME_Tasks_InteractHandler ist unter BEIDEN EventTypes indexiert; ein Event traegt genau einen Typ).
modded class ActionBase {
	//! Baut aus dem Server-Abschluss einer Aktion ein DME_Tasks_ActionEvent und dispatcht es.
	void DME_Tasks_EmitActionEvent(ActionData action_data) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!action_data) {
			return;
		}

		PlayerBase player = action_data.m_Player;
		if (!player) {
			return;
		}

		PlayerIdentity identity = player.GetIdentity();
		if (!identity) {
			return;
		}

		string uid = identity.GetId();
		if (uid == "") {
			return;
		}

		//! Cheap-Guard: Aktionen feuern oft — ohne passende Refs kein Event bauen
		DME_Tasks_ObjectiveIndex index = DME_Tasks_ObjectiveIndex.GetInstance();
		int actionRefs = index.CountRefs(uid, EDME_Tasks_EventType.ACTION_USED);
		int interactRefs = index.CountRefs(uid, EDME_Tasks_EventType.INTERACT);
		if (actionRefs == 0 && interactRefs == 0) {
			return;
		}

		string itemInHandsType = "";
		if (action_data.m_MainItem) {
			itemInHandsType = action_data.m_MainItem.GetType();
		}

		string targetType = "";
		if (action_data.m_Target) {
			Object targetObject = action_data.m_Target.GetObject();
			if (!targetObject) {
				targetObject = action_data.m_Target.GetParent();
			}
			if (targetObject) {
				targetType = targetObject.GetType();
			}
		}

		DME_Tasks_ActionEvent evt = new DME_Tasks_ActionEvent();
		evt.m_DME_ActionName = ClassName();
		evt.m_DME_ItemInHandsType = itemInHandsType;
		evt.m_DME_TargetType = targetType;
		evt.m_DME_PlayerUid = uid;
		evt.m_DME_Position = player.GetPosition();
		if (targetType != "") {
			evt.m_DME_EventType = EDME_Tasks_EventType.INTERACT;
		}

		DME_Tasks_EventBus.GetInstance().Dispatch(evt);
	}
}

modded class AnimatedActionBase {
	override protected void OnExecuteServer(ActionData action_data) {
		super.OnExecuteServer(action_data);

		//! Continuous-Actions melden ihren Abschluss ueber OnFinishProgress (kein Doppel-Event)
		if (ActionContinuousBase.Cast(this)) {
			return;
		}

		DME_Tasks_EmitActionEvent(action_data);
	}
}

modded class ActionContinuousBase {
	//! Dispatcher-Hook statt OnFinishProgressServer: viele Vanilla-Continuous-Actions ueberschreiben
	//! OnFinishProgressServer OHNE super-Aufruf — ein modded Override der Basisklasse wuerde dort nie
	//! laufen. OnFinishProgress (ActionContinuousBase.c:243) feuert fuer ALLE Ableitungen; der
	//! IsDedicatedServer-Guard sitzt im Dispatcher DME_Tasks_EmitActionEvent.
	override void OnFinishProgress(ActionData action_data) {
		super.OnFinishProgress(action_data);

		DME_Tasks_EmitActionEvent(action_data);
	}
}
