//! Inventar-Hook (CONTRACTS §6.5, Agent 10) — stempelt Origin-Metadaten und feuert
//! ITEM_ACQUIRED/ITEM_LOST-Events NUR, wenn der Spieler entsprechende Refs im
//! Active-Objective-Index hat (allokationsfreier Cheap-Guard via CountRefs).
//!
//! Signaturen quellcode-verifiziert (vanilla 1.29, ItemBase.c:3923/3935):
//!   void OnInventoryEnter(Man player) / void OnInventoryExit(Man player)
//! Beide werden aus OnItemLocationChanged NUR bei echtem Besitzerwechsel gerufen — das Item ist
//! zum Zeitpunkt des Aufrufs bereits an der neuen Position (die EnumerateInventory-Zaehlung im
//! CollectHandler ist damit konsistent). Enter/Exit feuern auch fuer alle enthaltenen Sub-Items.
//!
//! WICHTIG (Chain-Regel): zweite modded-Deklaration von ItemBase in diesem PBO (Members/Persistenz
//! liegen in DME_Tasks_ItemStorageHook.c). Diese Deklaration ist SELBSTAENDIG — Zugriff auf die
//! Origin-Member laeuft ausschliesslich ueber die externe Klasse DME_Tasks_OriginService,
//! nie quer zwischen den beiden modded-Deklarationen.
modded class ItemBase {
	override void OnInventoryEnter(Man player) {
		super.OnInventoryEnter(player);

		DME_Tasks_ItemHook.OnItemInventoryEnter(this, player);
	}

	override void OnInventoryExit(Man player) {
		super.OnInventoryExit(player);

		DME_Tasks_ItemHook.OnItemInventoryExit(this, player);
	}
}

//! Statischer Hook-Helfer: server-only Guard, UID-Ermittlung (identity.GetId(), §6.4),
//! Origin-Stempelung und Event-Erzeugung fuer den EventBus.
class DME_Tasks_ItemHook {
	static void OnItemInventoryEnter(ItemBase item, Man player) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!item || !player) {
			return;
		}

		PlayerBase playerBase = PlayerBase.Cast(player);
		if (!playerBase) {
			return;
		}
		PlayerIdentity identity = playerBase.GetIdentity();
		if (!identity) {
			return;
		}
		string uid = identity.GetId();
		if (uid == "") {
			return;
		}

		//! Stempelung laeuft IMMER (auch ohne aktive Item-Ziele): Erst-Aufnahme -> WORLD_LOOT,
		//! Fremd-Aufnahme -> Transferred (Regeln in DME_Tasks_OriginService.StampInventoryEnter).
		DME_Tasks_OriginService.StampInventoryEnter(item, uid);

		//! Cheap-Guard: kein Event-Bau, wenn der Spieler keine ITEM_ACQUIRED-Refs hat
		if (DME_Tasks_ObjectiveIndex.GetInstance().CountRefs(uid, EDME_Tasks_EventType.ITEM_ACQUIRED) == 0) {
			return;
		}

		DME_Tasks_ItemEvent evt = new DME_Tasks_ItemEvent();
		evt.SetAcquired(true);
		evt.m_DME_PlayerUid = uid;
		evt.m_DME_Position = playerBase.GetPosition();
		evt.m_DME_ItemType = item.GetType();
		evt.m_DME_Item = item;
		DME_Tasks_EventBus.GetInstance().Dispatch(evt);
	}

	static void OnItemInventoryExit(ItemBase item, Man player) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!item || !player) {
			return;
		}

		PlayerBase playerBase = PlayerBase.Cast(player);
		if (!playerBase) {
			return;
		}
		PlayerIdentity identity = playerBase.GetIdentity();
		if (!identity) {
			return;
		}
		string uid = identity.GetId();
		if (uid == "") {
			return;
		}

		//! Cheap-Guard: kein Event-Bau, wenn der Spieler keine ITEM_LOST-Refs hat
		if (DME_Tasks_ObjectiveIndex.GetInstance().CountRefs(uid, EDME_Tasks_EventType.ITEM_LOST) == 0) {
			return;
		}

		DME_Tasks_ItemEvent evt = new DME_Tasks_ItemEvent();
		evt.SetAcquired(false);
		evt.m_DME_PlayerUid = uid;
		evt.m_DME_Position = playerBase.GetPosition();
		evt.m_DME_ItemType = item.GetType();
		evt.m_DME_Item = item;
		DME_Tasks_EventBus.GetInstance().Dispatch(evt);
	}
}
