//! Zylindrischer Zonen-Trigger (CONTRACTS §6.3/§6.5, Agent 11) — wird ausschliesslich vom
//! DME_Tasks_ZoneTriggerManager erzeugt (CreateObjectEx + SetExtents + SetZoneInfo) und zerstoert.
//! Die Box-Extents [-r,-h/2,-r]..[r,h/2,r] sind nur die Grob-Huelle; die echte Zone ist ein
//! ZYLINDER um das Trigger-Zentrum (horizontale Distanz <= Radius, Hoehe deckt die Box ab).
//! Ecken-Faelle (Spieler betritt die Box, aber nicht den Zylinder) werden ueber OnStayServerEvent
//! nachgeschaerft: Enter/Leave des Zylinders wird pro Spieler-Uid getrackt und NUR bei
//! Uebergaengen als DME_Tasks_ZoneEvent (SetEntered) an den EventBus dispatcht.
//! Vanilla-verifiziert: TriggerEvents.c:45/97/148 (OnEnter/OnStay/OnLeaveServerEvent, protected),
//! Trigger.c:2 (TriggerInsider.GetObject), Trigger.c:116 (SetExtents), Object.c:523 (IsAlive).
class DME_Tasks_ZoneTrigger : Trigger {
	protected string m_DME_ZoneId;
	protected float m_DME_Radius;
	//! Uids, die aktuell im ZYLINDER stehen (nicht nur in der Box)
	protected ref array<string> m_DME_InsideUids;

	void DME_Tasks_ZoneTrigger() {
		m_DME_ZoneId = "";
		m_DME_Radius = 0.0;
		m_DME_InsideUids = new array<string>();
	}

	//! ERFORDERLICHE Seam-Methode (CONTRACTS §6.5) — der ZoneTriggerManager ruft sie direkt
	//! nach CreateObjectEx + SetExtents. Radius ist der bereits default-korrigierte Zylinder-Radius.
	void SetZoneInfo(string zoneId, float radius) {
		m_DME_ZoneId = zoneId;
		m_DME_Radius = radius;
	}

	//! Disconnect-Cleanup (vom ZoneTriggerManager gerufen): entfernt die Uid aus dem
	//! Insider-Tracking OHNE Leave-Dispatch — der Aufrufer raeumt den Spieler-State selbst ab.
	void RemoveInsider(string uid) {
		int foundIndex = m_DME_InsideUids.Find(uid);
		if (foundIndex != -1) {
			m_DME_InsideUids.RemoveOrdered(foundIndex);
		}
	}

	override protected void OnEnterServerEvent(TriggerInsider insider) {
		super.OnEnterServerEvent(insider);
		EvaluateInsider(insider);
	}

	//! Nachschaerfung pro Update: faengt Spieler, die die Box am Rand betreten und erst spaeter
	//! in den Zylinder laufen (oder ihn verlassen, ohne die Box zu verlassen).
	override protected void OnStayServerEvent(TriggerInsider insider, float deltaTime) {
		super.OnStayServerEvent(insider, deltaTime);
		EvaluateInsider(insider);
	}

	override protected void OnLeaveServerEvent(TriggerInsider insider) {
		super.OnLeaveServerEvent(insider);
		//! Box verlassen → Zylinder sicher verlassen (Zylinder liegt komplett in der Box)
		ForceLeave(insider);
	}

	// ==================================================================
	// intern
	// ==================================================================

	//! Prueft die Zylinder-Zugehoerigkeit des Insiders und dispatcht NUR bei Uebergaengen.
	protected void EvaluateInsider(TriggerInsider insider) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!insider) {
			return;
		}

		PlayerBase player = PlayerBase.Cast(insider.GetObject());
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

		vector playerPos = player.GetPosition();

		//! Tote Spieler gelten als "draussen" — sonst bleibt die Uid nach dem Tod im Tracking haengen
		if (!player.IsAlive()) {
			int deadIndex = m_DME_InsideUids.Find(uid);
			if (deadIndex == -1) {
				return;
			}
			m_DME_InsideUids.RemoveOrdered(deadIndex);
			DispatchZoneEvent(uid, playerPos, false);
			return;
		}

		bool inside = IsInsideCylinder(playerPos);
		int foundIndex = m_DME_InsideUids.Find(uid);
		bool wasInside = foundIndex != -1;

		if (inside && !wasInside) {
			m_DME_InsideUids.Insert(uid);
			DispatchZoneEvent(uid, playerPos, true);
		} else if (!inside && wasInside) {
			m_DME_InsideUids.RemoveOrdered(foundIndex);
			DispatchZoneEvent(uid, playerPos, false);
		}
	}

	//! Box-Leave: Spieler ist definitiv draussen — kein Distanz-Check noetig.
	protected void ForceLeave(TriggerInsider insider) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!insider) {
			return;
		}

		PlayerBase player = PlayerBase.Cast(insider.GetObject());
		if (!player) {
			return;
		}

		PlayerIdentity identity = player.GetIdentity();
		if (!identity) {
			return;
		}

		string uid = identity.GetId();
		int foundIndex = m_DME_InsideUids.Find(uid);
		if (foundIndex == -1) {
			return;
		}

		m_DME_InsideUids.RemoveOrdered(foundIndex);
		DispatchZoneEvent(uid, player.GetPosition(), false);
	}

	//! Horizontale Distanz zum Trigger-Zentrum <= Radius (Zylinder; vertikal begrenzt die Box).
	protected bool IsInsideCylinder(vector position) {
		if (m_DME_Radius <= 0) {
			return true;
		}

		vector center = GetPosition();
		float dx = position[0] - center[0];
		float dz = position[2] - center[2];
		float distSq = dx * dx + dz * dz;
		float radiusSq = m_DME_Radius * m_DME_Radius;
		return distSq <= radiusSq;
	}

	protected void DispatchZoneEvent(string uid, vector position, bool entered) {
		DME_Tasks_ZoneEvent evt = new DME_Tasks_ZoneEvent();
		evt.SetEntered(entered);
		evt.m_DME_ZoneId = m_DME_ZoneId;
		evt.m_DME_PlayerUid = uid;
		evt.m_DME_Position = position;
		DME_Tasks_EventBus.GetInstance().Dispatch(evt);
	}
}
