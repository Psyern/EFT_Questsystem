//! Engine-Override der Item-Abgabe (CONTRACTS §6.4 + §6.5, Agent 10).
//! Die Core-Basisimplementation von HandoverItems VALIDIERT NUR (keine Transition) und schickt
//! bei Fehlern bereits Notifications ("Quest ist nicht aktiv" / "Nichts abzugeben") — deshalb
//! super ZUERST; die eigentliche Abgabe-Pipeline (Zonen-Check, ReferencesObjective-Aufloesung,
//! Inventar-Scan, Origin-Pruefung, partielle Abgabe, ObjectDelete, AddObjectiveProgress, SaveNow,
//! Ergebnis-Notification) laeuft in DME_Tasks_HandoverProcessor.ProcessHandover. Der Processor
//! prueft dieselben Abbruchbedingungen erneut STILL, damit keine doppelten Notifications entstehen.
modded class DME_Tasks_QuestEngine {
	override void HandoverItems(PlayerIdentity sender, string questId, string objectiveId) {
		super.HandoverItems(sender, questId, objectiveId);

		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!sender) {
			return;
		}

		string uid = sender.GetId();
		if (uid == "") {
			return;
		}

		DME_Tasks_HandoverProcessor.ProcessHandover(uid, questId, objectiveId, true);
	}
}
