//! Groups-Adapter (CONTRACTS §6.8) — Erkennung: CfgPatches "DayZExpansion_Groups";
//! Toggle: Settings.EnableIntegrationGroups.
//!
//! Faehigkeits-Grenzen (ehrlich dokumentiert): Ohne Hard-Referenz auf Expansion-Party-/
//! Gruppen-Klassen gibt es KEINE API-Bruecke — dieser Adapter liefert nur Erkennung + Log
//! und dokumentiert den GROUP_UPDATE-Anschlusspunkt.
//!
//! GROUP_UPDATE-Anschlusspunkt (fuer eine server-spezifische Bruecke, z. B. kleines
//! Companion-PBO mit requiredAddons auf Expansion Groups):
//! 1. Bei jeder Gruppen-Aenderung (Join/Leave/Disband) fuer JEDES betroffene Mitglied
//!    DME_Tasks_GroupsAdapter.ReportGroupUpdate(uid, groupSize, position) aufrufen —
//!    uid = identity.GetId() des Mitglieds, groupSize = aktuelle Mitgliederzahl.
//! 2. Der Helper baut ein DME_Tasks_GroupEvent (m_DME_GroupSize, EventType GROUP_UPDATE,
//!    gesetzt im Event-Ctor) und dispatcht es via DME_Tasks_EventBus.GetInstance().Dispatch(evt)
//!    — der DME_Tasks_GroupHandler (GROUP-Objectives) konsumiert es (CONTRACTS §6.5/§6.7).
//! 3. Ohne Bruecke bleiben GROUP-Objectives Warn-once-no-op (GroupHandler) — kein Fehler.
class DME_Tasks_GroupsAdapter : DME_Tasks_IntegrationBase {
	private static bool s_DME_Active = false;

	void DME_Tasks_GroupsAdapter() {
		m_DME_Name = "Groups";
		m_DME_AddonPatchName = "DayZExpansion_Groups";
	}

	override bool IsEnabled(DME_Tasks_Settings settings) {
		return settings.EnableIntegrationGroups;
	}

	override void OnActivate() {
		s_DME_Active = true;
		DME_Tasks_Log.Info("GroupsAdapter: Expansion Groups erkannt — keine API-Bruecke ohne Hard-Refs; GROUP_UPDATE-Anschlusspunkt: DME_Tasks_GroupsAdapter.ReportGroupUpdate(uid, groupSize, position)");
	}

	//! Dokumentierter Anschlusspunkt (siehe Header) — dispatcht ein DME_Tasks_GroupEvent fuer den
	//! Spieler. true = dispatcht; false = Adapter inaktiv/Parameter unbrauchbar (safe no-op).
	static bool ReportGroupUpdate(string uid, int groupSize, vector position) {
		if (!s_DME_Active) {
			return false;
		}
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return false;
		}
		if (uid == "" || groupSize < 0) {
			return false;
		}

		DME_Tasks_GroupEvent evt = new DME_Tasks_GroupEvent();
		evt.m_DME_PlayerUid = uid;
		evt.m_DME_Position = position;
		evt.m_DME_GroupSize = groupSize;
		DME_Tasks_EventBus.GetInstance().Dispatch(evt);
		return true;
	}
}
