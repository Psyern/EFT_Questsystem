//! GROUP (CONTRACTS §6.5, Agent 11) — indexiert unter GROUP_UPDATE.
//!
//! Amount = geforderte Gruppengroesse. GROUP_UPDATE-Events liefert erst der Groups-
//! Integrationsadapter (Welle 4, z. B. Expansion Groups/LBmaster) — ohne Adapter kommt hier
//! nie ein Event an: Warn-once beim Indexieren, kein Progress.
//! m_DME_GroupSize >= Amount → Objective wird komplett vervollstaendigt (one-way; ein spaeteres
//! Schrumpfen der Gruppe nimmt den Fortschritt nicht zurueck).
class DME_Tasks_GroupHandler : DME_Tasks_ObjectiveHandlerBase {
	private bool m_DME_NoIntegrationWarned;

	void DME_Tasks_GroupHandler() {
		m_DME_NoIntegrationWarned = false;
	}

	override bool CanHandle(int objectiveType) {
		return objectiveType == EDME_Tasks_ObjectiveType.GROUP;
	}

	override void GetEventTypesFor(int objectiveType, array<int> outTypes) {
		WarnOnce();
		outTypes.Insert(EDME_Tasks_EventType.GROUP_UPDATE);
	}

	override void OnEvent(DME_Tasks_ObjectiveRef objectiveRef, DME_Tasks_Event evt) {
		if (!objectiveRef || !objectiveRef.Def) {
			return;
		}

		DME_Tasks_GroupEvent groupEvent = DME_Tasks_GroupEvent.Cast(evt);
		if (!groupEvent) {
			return;
		}

		int required = objectiveRef.Def.Amount;
		if (required < 1) {
			required = 1;
		}
		if (groupEvent.m_DME_GroupSize < required) {
			return;
		}

		//! Gruppengroesse erreicht → in einem Schritt vervollstaendigen (Engine klemmt auf Required)
		ReportProgress(objectiveRef, required);
	}

	private void WarnOnce() {
		if (m_DME_NoIntegrationWarned) {
			return;
		}
		m_DME_NoIntegrationWarned = true;
		DME_Tasks_Log.Warn("GroupHandler: GROUP-Objectives benoetigen den Groups-Integrationsadapter (Welle 4) — ohne ihn kein Progress");
	}
}
