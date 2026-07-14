//! Questgegenstand-Helfer (Konzept §10, Agent 10) — VERBINDLICHE KONVENTION (dokumentiert):
//!
//! "Questitem" = Item, das von der Quest-Logik gespawnt und via
//!   DME_Tasks_OriginService.StampOrigin(item, EDME_Tasks_OriginType.EVENT_REWARD, uid, questId)
//! gestempelt wurde (OriginType == EVENT_REWARD UND AcquiredQuestId == questId) — z. B.
//! Beweisstuecke oder Lieferpakete. Diese Items verschwinden bei Quest-Ende
//! (Abandon/Complete/Fail/Expire — s_DME_OnQuestDeactivated-Abo im DME_Tasks_OriginModule).
//!
//! ABGRENZUNG: QUEST_REWARD-Items (Belohnungen aus dem RewardService) sind KEINE Questitems
//! und bleiben IMMER beim Spieler; ebenso alle regulaer gesammelten Items (WORLD_LOOT etc.).
class DME_Tasks_QuestItemHelper {
	//! true, wenn das Item von Quest-Logik stammt (AcquiredQuestId gesetzt — QUEST_REWARD
	//! ODER EVENT_REWARD). Fuer Verkaufs-/Trade-Sperren durch Integrations-Adapter nutzbar.
	static bool IsQuestItem(ItemBase item) {
		if (!item) {
			return false;
		}
		return item.DME_GetAcquiredQuestId() != "";
	}

	//! Entfernt beim Quest-Ende genau die Questitems dieser Quest aus dem Spieler-Inventar:
	//! NUR Items mit m_DME_AcquiredQuestId == questId UND m_DME_OriginType == EVENT_REWARD.
	//! Spieler offline -> no-op (Warn); die Items behalten ihren Stempel und koennen von einem
	//! spaeteren Sweep/Adapter erkannt werden (IsQuestItem).
	static void RemoveQuestItemsFor(string uid, string questId) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (uid == "" || questId == "") {
			return;
		}

		PlayerBase player = DME_Tasks_QuestEngine.GetInstance().FindPlayerByUid(uid);
		if (!player) {
			return;
		}

		GameInventory inventory = player.GetInventory();
		if (!inventory) {
			return;
		}

		array<EntityAI> entities = new array<EntityAI>();
		inventory.EnumerateInventory(InventoryTraversalType.LEVELORDER, entities);

		int removed = 0;
		foreach (EntityAI entity : entities) {
			ItemBase item = ItemBase.Cast(entity);
			if (!item) {
				continue;
			}
			if (item.DME_GetAcquiredQuestId() != questId) {
				continue;
			}
			if (item.DME_GetOriginType() != EDME_Tasks_OriginType.EVENT_REWARD) {
				continue;
			}

			g_Game.ObjectDelete(item);
			removed++;
		}

		if (removed > 0) {
			DME_Tasks_Log.Info("QuestItemHelper: %1 Questitem(s) von %2 entfernt (Quest %3)", removed.ToString(), uid, questId);
		}
	}
}
