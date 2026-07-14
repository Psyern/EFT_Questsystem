//! Event-Basisklassen des Objectives-PBO (CONTRACTS §6.5 — Felder VERBINDLICH, nicht aendern).
//! Events werden ausschliesslich server-seitig erzeugt (Hooks der Agenten 9-12) und via
//! DME_Tasks_EventBus.Dispatch(evt) an die Handler des betroffenen Spielers verteilt.
//! HANDOVER laeuft NICHT ueber den Bus (Engine-Override); EXTRACT nutzt ZoneEvent + TimerEvent.
class DME_Tasks_Event {
	int m_DME_EventType;
	string m_DME_PlayerUid;
	vector m_DME_Position;

	void DME_Tasks_Event() {
		m_DME_EventType = -1;
		m_DME_PlayerUid = "";
		m_DME_Position = vector.Zero;
	}
}

//! EDME_Tasks_EventType.KILL — vom Kill-Hook (Agent 9) aus EEHitBy-Puffer + EEKilled gebaut.
class DME_Tasks_KillEvent : DME_Tasks_Event {
	string m_DME_VictimType;
	bool m_DME_VictimIsPlayer;
	bool m_DME_VictimIsInfected;
	bool m_DME_VictimIsAnimal;
	bool m_DME_VictimIsAI;
	string m_DME_VictimCategory;
	string m_DME_BossId;
	float m_DME_Distance;
	string m_DME_WeaponType;
	bool m_DME_Suppressed;
	string m_DME_HitZone;

	void DME_Tasks_KillEvent() {
		m_DME_EventType = EDME_Tasks_EventType.KILL;
		m_DME_VictimType = "";
		m_DME_VictimIsPlayer = false;
		m_DME_VictimIsInfected = false;
		m_DME_VictimIsAnimal = false;
		m_DME_VictimIsAI = false;
		m_DME_VictimCategory = "";
		m_DME_BossId = "";
		m_DME_Distance = 0.0;
		m_DME_WeaponType = "";
		m_DME_Suppressed = false;
		m_DME_HitZone = "";
	}
}

//! EDME_Tasks_EventType.ZONE_ENTER / ZONE_LEAVE — vom DME_Tasks_ZoneTrigger (Agent 11) erzeugt.
//! WICHTIG: Erzeuger setzen Entered/EventType NUR ueber SetEntered(), damit beide konsistent bleiben.
class DME_Tasks_ZoneEvent : DME_Tasks_Event {
	string m_DME_ZoneId;
	bool m_DME_Entered;

	void DME_Tasks_ZoneEvent() {
		m_DME_EventType = EDME_Tasks_EventType.ZONE_ENTER;
		m_DME_ZoneId = "";
		m_DME_Entered = true;
	}

	void SetEntered(bool entered) {
		m_DME_Entered = entered;
		if (entered) {
			m_DME_EventType = EDME_Tasks_EventType.ZONE_ENTER;
		} else {
			m_DME_EventType = EDME_Tasks_EventType.ZONE_LEAVE;
		}
	}
}

//! EDME_Tasks_EventType.ITEM_ACQUIRED / ITEM_LOST — vom Item-Hook (Agent 10) erzeugt.
//! WICHTIG: Erzeuger setzen Acquired/EventType NUR ueber SetAcquired().
class DME_Tasks_ItemEvent : DME_Tasks_Event {
	string m_DME_ItemType;
	bool m_DME_Acquired;
	EntityAI m_DME_Item;

	void DME_Tasks_ItemEvent() {
		m_DME_EventType = EDME_Tasks_EventType.ITEM_ACQUIRED;
		m_DME_ItemType = "";
		m_DME_Acquired = true;
		m_DME_Item = null;
	}

	void SetAcquired(bool acquired) {
		m_DME_Acquired = acquired;
		if (acquired) {
			m_DME_EventType = EDME_Tasks_EventType.ITEM_ACQUIRED;
		} else {
			m_DME_EventType = EDME_Tasks_EventType.ITEM_LOST;
		}
	}
}

//! EDME_Tasks_EventType.CRAFT — vom Craft-Hook (Agent 11) erzeugt.
class DME_Tasks_CraftEvent : DME_Tasks_Event {
	string m_DME_ResultType;
	EntityAI m_DME_Item;

	void DME_Tasks_CraftEvent() {
		m_DME_EventType = EDME_Tasks_EventType.CRAFT;
		m_DME_ResultType = "";
		m_DME_Item = null;
	}
}

//! EDME_Tasks_EventType.ACTION_USED (Default) — vom Action-Hook (Agent 11) erzeugt.
//! Fuer echte Interaktions-Events darf der Erzeuger m_DME_EventType = EDME_Tasks_EventType.INTERACT setzen
//! (der DME_Tasks_InteractHandler ist unter BEIDEN EventTypes indexiert).
class DME_Tasks_ActionEvent : DME_Tasks_Event {
	string m_DME_ActionName;
	string m_DME_ItemInHandsType;
	string m_DME_TargetType;

	void DME_Tasks_ActionEvent() {
		m_DME_EventType = EDME_Tasks_EventType.ACTION_USED;
		m_DME_ActionName = "";
		m_DME_ItemInHandsType = "";
		m_DME_TargetType = "";
	}
}

//! EDME_Tasks_EventType.TIMER_TICK — 1-Hz vom DME_Tasks_ObjectivesModule, NUR fuer Spieler mit TIMER_TICK-Refs.
class DME_Tasks_TimerEvent : DME_Tasks_Event {
	int m_DME_NowEpoch;

	void DME_Tasks_TimerEvent() {
		m_DME_EventType = EDME_Tasks_EventType.TIMER_TICK;
		m_DME_NowEpoch = 0;
	}
}

//! EDME_Tasks_EventType.GROUP_UPDATE — vom Groups-Integrationsadapter (Welle 4) erzeugt.
class DME_Tasks_GroupEvent : DME_Tasks_Event {
	int m_DME_GroupSize;

	void DME_Tasks_GroupEvent() {
		m_DME_EventType = EDME_Tasks_EventType.GROUP_UPDATE;
		m_DME_GroupSize = 0;
	}
}
