//! Craft-Abschluss-Hook (CONTRACTS §6.5, Agent 11) — quellcode-verifiziert in scripts - 1.29:
//! RecipeBase.SpawnItems(ItemBase ingredients[], PlayerBase player, array<ItemBase> spawned_objects)
//! (RecipeBase.c:215) wird AUSSCHLIESSLICH aus RecipeBase.PerformRecipe (RecipeBase.c:521,
//! Erfolgs-Pfad nach CheckRecipe) gerufen; PerformRecipe wiederum nur server-seitig via
//! PluginRecipesManager.PerformRecipeServer (PluginRecipesManager.c:310; Aufrufer:
//! ActionWorldCraft.c:187 + PlayerBase.c:5624). KEINE Vanilla-Rezeptklasse ueberschreibt
//! SpawnItems/PerformRecipe (RecipeBase.Do wird dagegen von fast allen Rezepten OHNE super
//! ueberschrieben und ist deshalb als Hook ungeeignet).
//!
//! Nach super.SpawnItems enthaelt spawned_objects die Ergebnis-Items:
//! 1) Origin-Stempel CRAFTED auf JEDEM Ergebnis (DME_SetOrigin, Agent-10-OriginService) —
//!    unabhaengig von CRAFT-Refs, da die Herkunft auch fuer Handover-Prüfungen zaehlt.
//! 2) CraftEvent pro Ergebnis-Item an den EventBus (Cheap-Guard: nur bei CRAFT-Refs im Index);
//!    Rezepte ohne Ergebnis-Items dispatchen EIN CraftEvent mit leerem ResultType
//!    (matcht nur Objectives ohne ClassNames-Filter = "irgendetwas craften").
modded class RecipeBase {
	override void SpawnItems(ItemBase ingredients[], PlayerBase player, array<ItemBase> spawned_objects) {
		super.SpawnItems(ingredients, player, spawned_objects);

		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
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

		//! 1) Origin-Stempel — immer, auch ohne aktive CRAFT-Objectives
		foreach (ItemBase spawnedItem : spawned_objects) {
			if (!spawnedItem) {
				continue;
			}
			spawnedItem.DME_SetOrigin(EDME_Tasks_OriginType.CRAFTED, uid, "");
		}

		//! 2) CraftEvents — Cheap-Guard zuerst
		if (DME_Tasks_ObjectiveIndex.GetInstance().CountRefs(uid, EDME_Tasks_EventType.CRAFT) == 0) {
			return;
		}

		vector playerPos = player.GetPosition();
		DME_Tasks_EventBus bus = DME_Tasks_EventBus.GetInstance();

		if (spawned_objects.Count() == 0) {
			DME_Tasks_CraftEvent emptyEvent = new DME_Tasks_CraftEvent();
			emptyEvent.m_DME_PlayerUid = uid;
			emptyEvent.m_DME_Position = playerPos;
			bus.Dispatch(emptyEvent);
			return;
		}

		foreach (ItemBase resultItem : spawned_objects) {
			if (!resultItem) {
				continue;
			}
			DME_Tasks_CraftEvent evt = new DME_Tasks_CraftEvent();
			evt.m_DME_ResultType = resultItem.GetType();
			evt.m_DME_Item = resultItem;
			evt.m_DME_PlayerUid = uid;
			evt.m_DME_Position = playerPos;
			bus.Dispatch(evt);
		}
	}
}
