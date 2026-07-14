//! Zentrale Quest-State-Machine (SPEC §5.4) — die EINZIGE Stelle, die Uebergaenge validiert und anwendet.
//! LOCKED → AVAILABLE → ACTIVE → READY_TO_TURN_IN → COMPLETED; plus FAILED, EXPIRED, ABANDONED, COOLDOWN.
class DME_Tasks_QuestLifecycle {
	static bool CanTransition(int fromState, int toState) {
		if (fromState == toState) {
			return false;
		}

		if (fromState == EDME_Tasks_QuestState.LOCKED) {
			return toState == EDME_Tasks_QuestState.AVAILABLE;
		}

		if (fromState == EDME_Tasks_QuestState.AVAILABLE) {
			if (toState == EDME_Tasks_QuestState.ACTIVE) return true;
			if (toState == EDME_Tasks_QuestState.LOCKED) return true;
			return false;
		}

		if (fromState == EDME_Tasks_QuestState.ACTIVE) {
			if (toState == EDME_Tasks_QuestState.READY_TO_TURN_IN) return true;
			if (toState == EDME_Tasks_QuestState.FAILED) return true;
			if (toState == EDME_Tasks_QuestState.EXPIRED) return true;
			if (toState == EDME_Tasks_QuestState.ABANDONED) return true;
			return false;
		}

		if (fromState == EDME_Tasks_QuestState.READY_TO_TURN_IN) {
			//! ACTIVE = Progress-Regression (z.B. SetObjectiveProgress unter Required)
			if (toState == EDME_Tasks_QuestState.ACTIVE) return true;
			if (toState == EDME_Tasks_QuestState.COMPLETED) return true;
			if (toState == EDME_Tasks_QuestState.FAILED) return true;
			if (toState == EDME_Tasks_QuestState.EXPIRED) return true;
			if (toState == EDME_Tasks_QuestState.ABANDONED) return true;
			return false;
		}

		if (fromState == EDME_Tasks_QuestState.COMPLETED) {
			//! Nur Repeatable-Quests verlassen COMPLETED wieder
			if (toState == EDME_Tasks_QuestState.COOLDOWN) return true;
			if (toState == EDME_Tasks_QuestState.AVAILABLE) return true;
			return false;
		}

		if (fromState == EDME_Tasks_QuestState.FAILED || fromState == EDME_Tasks_QuestState.EXPIRED || fromState == EDME_Tasks_QuestState.ABANDONED) {
			if (toState == EDME_Tasks_QuestState.AVAILABLE) return true;
			if (toState == EDME_Tasks_QuestState.COOLDOWN) return true;
			return false;
		}

		if (fromState == EDME_Tasks_QuestState.COOLDOWN) {
			return toState == EDME_Tasks_QuestState.AVAILABLE;
		}

		return false;
	}

	//! Wendet einen validierten Uebergang auf eine aktive Quest an; false + Warn-Log bei Regelverstoss.
	static bool ApplyState(DME_Tasks_ActiveQuest quest, int newState) {
		if (!quest) {
			DME_Tasks_Log.Warn("QuestLifecycle.ApplyState: quest == null");
			return false;
		}

		if (!CanTransition(quest.State, newState)) {
			string fromName = DME_Tasks_EnumUtil.QuestStateToString(quest.State);
			string toName = DME_Tasks_EnumUtil.QuestStateToString(newState);
			DME_Tasks_Log.Warn("QuestLifecycle: Uebergang %1 -> %2 fuer Quest %3 nicht erlaubt", fromName, toName, quest.QuestId);
			return false;
		}

		quest.State = newState;
		return true;
	}

	//! Terminal = Quest verlaesst die ActiveQuests-Liste.
	static bool IsTerminal(int state) {
		if (state == EDME_Tasks_QuestState.COMPLETED) return true;
		if (state == EDME_Tasks_QuestState.FAILED) return true;
		if (state == EDME_Tasks_QuestState.EXPIRED) return true;
		if (state == EDME_Tasks_QuestState.ABANDONED) return true;
		return false;
	}
}
