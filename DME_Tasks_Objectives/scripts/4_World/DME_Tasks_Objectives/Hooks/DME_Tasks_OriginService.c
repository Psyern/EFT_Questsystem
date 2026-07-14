//! Origin-Service (CONTRACTS §6.5, Agent 10) — statische Origin-Pruefung und -Stempelung
//! + kleines CF-Modul fuer die EngineEvents-Abos (Core kennt den OriginService NICHT — §6.2).
//!
//! Konventionen (verbindlich, siehe auch DME_Tasks_QuestItemHelper):
//! - "FoundInWorld" = Loot-artige Herkuenfte: WORLD_LOOT, INFECTED_DROP, AI_DROP, BOSS_DROP.
//!   TRADER_PURCHASE/PLAYER_TRADE/ADMIN_SPAWN/QUEST_REWARD/EVENT_REWARD/CRAFTED/UNKNOWN gelten
//!   NICHT als "in der Welt gefunden".
//! - QUEST_REWARD = Belohnungs-Items (bleiben IMMER beim Spieler);
//!   EVENT_REWARD + AcquiredQuestId = von Quest-Logik gespawnte Questgegenstaende
//!   (werden bei Quest-Ende entfernt, Konzept §10).
class DME_Tasks_OriginService {
	//! true, wenn das Objective ueberhaupt Origin-/Besitz-Regeln definiert (Cheap-Guard fuer Zaehl-Pfade).
	static bool HasOriginConstraints(DME_Tasks_ObjectiveDef def) {
		if (!def) {
			return false;
		}
		if (def.FoundInWorldRequired) {
			return true;
		}
		if (def.AllowedOrigins && def.AllowedOrigins.Count() > 0) {
			return true;
		}
		if (def.MustBeFirstOwner) {
			return true;
		}
		if (def.AcquiredAfterQuestAccept) {
			return true;
		}
		if (!def.PlayerTransferAllowed) {
			return true;
		}
		return false;
	}

	//! §6.5-API: prueft FoundInWorldRequired / AllowedOrigins (Strings via EnumUtil.OriginFromString) /
	//! MustBeFirstOwner / AcquiredAfterQuestAccept (AcquiredAt >= acceptedAt) / PlayerTransferAllowed.
	static bool MatchesOrigin(ItemBase item, DME_Tasks_ObjectiveDef def, string uid, int acceptedAt) {
		if (!item || !def) {
			return false;
		}

		int originType = item.DME_GetOriginType();

		if (def.FoundInWorldRequired && !IsFoundInWorld(originType)) {
			return false;
		}

		if (def.AllowedOrigins && def.AllowedOrigins.Count() > 0) {
			if (!IsOriginAllowed(originType, def.AllowedOrigins)) {
				return false;
			}
		}

		if (def.MustBeFirstOwner && item.DME_GetFirstOwner() != uid) {
			return false;
		}

		if (def.AcquiredAfterQuestAccept) {
			int acquiredAt = item.DME_GetAcquiredAt();
			if (acquiredAt <= 0) {
				return false;
			}
			if (acquiredAt < acceptedAt) {
				return false;
			}
		}

		if (!def.PlayerTransferAllowed && item.DME_IsTransferred()) {
			return false;
		}

		return true;
	}

	//! Stempel-Regeln §6.5 fuer OnInventoryEnter:
	//! - erster Kontakt ohne Origin -> WORLD_LOOT + FirstOwner = uid + AcquiredAt = jetzt
	//! - nimmt ein ANDERER Spieler als der FirstOwner das Item -> Transferred = true
	static void StampInventoryEnter(ItemBase item, string uid) {
		if (!item || uid == "") {
			return;
		}

		if (!item.DME_HasOrigin()) {
			item.DME_SetOrigin(EDME_Tasks_OriginType.WORLD_LOOT, uid, "");
			return;
		}

		string firstOwner = item.DME_GetFirstOwner();
		if (firstOwner != "" && firstOwner != uid && !item.DME_IsTransferred()) {
			item.DME_MarkTransferred();
		}
	}

	//! Generischer Stempel-Helfer fuer andere Hooks/Adapter (z. B. CraftHook -> CRAFTED,
	//! AI-Adapter -> AI_DROP, RewardService-Abo -> QUEST_REWARD). questId darf "" sein.
	static void StampOrigin(EntityAI entity, int originType, string uid, string questId) {
		ItemBase item = ItemBase.Cast(entity);
		if (!item) {
			return;
		}
		item.DME_SetOrigin(originType, uid, questId);
	}

	//! Gemeinsamer Sammel-Pfad fuer COLLECT-Zaehlung und HANDOVER/DELIVER-Abgabe:
	//! EnumerateInventory(LEVELORDER) ueber das komplette Spieler-Inventar (inkl. Haende/Cargo/
	//! Attachments), Matching via ClassNames (exakt ODER Config-Vererbung g_Game.IsKindOf) und —
	//! nur wenn das Objective Origin-Regeln definiert — via MatchesOrigin.
	//! Zaehlweise: 1 pro Item-INSTANZ (Stack-Quantity wird bewusst nicht summiert — konsistent
	//! zur Handover-Entfernung via g_Game.ObjectDelete).
	static void CollectMatchingItems(PlayerBase player, DME_Tasks_ObjectiveDef matchDef, string uid, int acceptedAt, array<ItemBase> outItems) {
		if (!player || !matchDef || !outItems) {
			return;
		}

		GameInventory inventory = player.GetInventory();
		if (!inventory) {
			return;
		}

		array<EntityAI> entities = new array<EntityAI>();
		inventory.EnumerateInventory(InventoryTraversalType.LEVELORDER, entities);

		bool checkOrigin = HasOriginConstraints(matchDef);

		foreach (EntityAI entity : entities) {
			ItemBase item = ItemBase.Cast(entity);
			if (!item) {
				continue;
			}
			if (!MatchesItemTypes(item.GetType(), matchDef.ClassNames)) {
				continue;
			}
			if (checkOrigin && !MatchesOrigin(item, matchDef, uid, acceptedAt)) {
				continue;
			}
			outItems.Insert(item);
		}
	}

	//! Klassennamen-Filter: leere/null Liste = alles erlaubt; exakter Match ODER Config-Vererbung.
	static bool MatchesItemTypes(string type, array<string> classNames) {
		if (!classNames || classNames.Count() == 0) {
			return true;
		}
		if (type == "") {
			return false;
		}

		foreach (string className : classNames) {
			if (className == "") {
				continue;
			}
			if (type == className) {
				return true;
			}
			if (g_Game && g_Game.IsKindOf(type, className)) {
				return true;
			}
		}
		return false;
	}

	// ==================================================================
	// intern
	// ==================================================================

	private static bool IsFoundInWorld(int originType) {
		if (originType == EDME_Tasks_OriginType.WORLD_LOOT) {
			return true;
		}
		if (originType == EDME_Tasks_OriginType.INFECTED_DROP) {
			return true;
		}
		if (originType == EDME_Tasks_OriginType.AI_DROP) {
			return true;
		}
		if (originType == EDME_Tasks_OriginType.BOSS_DROP) {
			return true;
		}
		return false;
	}

	private static bool IsOriginAllowed(int originType, array<string> allowedOrigins) {
		foreach (string originName : allowedOrigins) {
			if (originName == "") {
				continue;
			}
			int allowedType = DME_Tasks_EnumUtil.OriginFromString(originName);
			if (allowedType != -1 && allowedType == originType) {
				return true;
			}
		}
		return false;
	}
}

//! CF-Modul des Origin-Systems (Agent 10) — abonniert die Core-Invoker (Core darf Objectives
//! NICHT referenzieren, §2; deshalb laeuft die Verdrahtung hier im Objectives-PBO).
[CF_RegisterModule(DME_Tasks_OriginModule)]
class DME_Tasks_OriginModule : CF_ModuleWorld {
	override void OnInit() {
		super.OnInit();

		DME_Tasks_EngineEvents.s_DME_OnRewardItemSpawned.Insert(OnRewardItemSpawned);
		DME_Tasks_EngineEvents.s_DME_OnQuestDeactivated.Insert(OnQuestDeactivated);
	}

	//! §6.2: RewardService feuert pro gespawntem Reward-Item -> Origin = QUEST_REWARD stempeln.
	private void OnRewardItemSpawned(EntityAI item, string uid, string questId) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		DME_Tasks_OriginService.StampOrigin(item, EDME_Tasks_OriginType.QUEST_REWARD, uid, questId);

		//! Der Inventar-Eintritt hat das Item transient als WORLD_LOOT gezaehlt — nach dem
		//! QUEST_REWARD-Stempel den Besitzstand per Recount-Event korrigieren.
		DispatchItemRecount(uid);
	}

	//! Recount-Konvention: ITEM_ACQUIRED mit LEEREM ItemType = expliziter Recount-Request
	//! (der CollectHandler ueberspringt dann seinen Cheap-Typ-Filter). CountRefs-Guard haelt
	//! den Pfad allokationsfrei, wenn der Spieler keine ITEM_ACQUIRED-Objectives hat.
	private void DispatchItemRecount(string uid) {
		if (uid == "") {
			return;
		}
		if (DME_Tasks_ObjectiveIndex.GetInstance().CountRefs(uid, EDME_Tasks_EventType.ITEM_ACQUIRED) == 0) {
			return;
		}

		vector position = vector.Zero;
		PlayerBase player = DME_Tasks_QuestEngine.GetInstance().FindPlayerByUid(uid);
		if (player) {
			position = player.GetPosition();
		}

		DME_Tasks_ItemEvent evt = new DME_Tasks_ItemEvent();
		evt.SetAcquired(true);
		evt.m_DME_PlayerUid = uid;
		evt.m_DME_Position = position;
		DME_Tasks_EventBus.GetInstance().Dispatch(evt);
	}

	//! Konzept §10: Questgegenstaende (EVENT_REWARD + AcquiredQuestId) verschwinden bei Quest-Ende
	//! (Abandon/Complete/Fail/Expire — alle Pfade feuern s_DME_OnQuestDeactivated).
	private void OnQuestDeactivated(string uid, string questId) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		DME_Tasks_QuestItemHelper.RemoveQuestItemsFor(uid, questId);
	}
}
