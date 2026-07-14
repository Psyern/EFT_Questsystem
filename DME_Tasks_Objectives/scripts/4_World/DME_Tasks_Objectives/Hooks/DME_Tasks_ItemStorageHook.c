//! Item origin metadata (CONTRACTS §6.3 + §6.5) — modded ItemBase, persisted via CF ModStorage.
//!
//! MUST use CF_OnStoreSave/CF_OnStoreLoad, NEVER raw OnStoreSave/OnStoreLoad with ctx.Write/ctx.Read.
//! Raw appends to the entity stream are unsafe here: CF itself writes its ModStorage blob into that
//! same stream (CF ItemBase.c: super.OnStoreSave(ctx) then m_CF_ModStorage.OnStoreSave(ctx)). On an
//! item saved BEFORE this mod was installed, a raw ctx.Read() does NOT fail — it happily consumes
//! CF's bytes, desyncing the stream. Every later read then returns garbage (observed: a shotgun
//! loading "muzzle index 536886856"), which throws VM exceptions and crash-loops the server.
//!
//! CF ModStorage solves this by construction: storage.Get(MOD_NAME) returns null for items that
//! predate the mod, so legacy saves simply keep the defaults. Requires storageVersion > 0 in the
//! CfgMods class of this PBO (see config.cpp) — the key below MUST match that class name.
//!
//! WICHTIG (Chain-Regel): Dieses PBO enthaelt ZWEI modded-Deklarationen von ItemBase (diese Datei +
//! DME_Tasks_ItemHook.c). Jede Deklaration ist SELBSTAENDIG — Zugriff auf die hier deklarierten
//! Member/Methoden erfolgt von aussen NUR ueber externe Klassen (DME_Tasks_OriginService,
//! DME_Tasks_QuestItemHelper, ...), nie quer zwischen den beiden modded-Deklarationen
//! (Muster wie DayZExpansion_Vehicles ItemBase.c + ItemBase_Towing.c).
modded class ItemBase {
	//! MUST match the CfgMods class name of this PBO (config.cpp), or the context is never found.
	private static const string DME_TASKS_STORAGE_MOD = "DME_Tasks_Objectives";

	//! EDME_Tasks_OriginType — UNKNOWN = noch nie gestempelt
	int m_DME_OriginType;
	//! Optionale Event-Referenz (z. B. Custom-Event-Id), gesetzt von Adaptern
	string m_DME_OriginEventId;
	//! UID (identity.GetId()) des ERSTEN Spielers, der das Item aufgenommen hat
	string m_DME_FirstOwner;
	//! Epoch-Sekunden der Erst-Aufnahme bzw. des letzten expliziten Origin-Stempels
	int m_DME_AcquiredAt;
	//! QuestId, wenn das Item von Quest-Logik stammt (QUEST_REWARD/EVENT_REWARD)
	string m_DME_AcquiredQuestId;
	//! true, sobald das Item im Inventar eines ANDEREN Spielers als dem FirstOwner war
	bool m_DME_Transferred;

	void ItemBase() {
		m_DME_OriginType = EDME_Tasks_OriginType.UNKNOWN;
		m_DME_OriginEventId = "";
		m_DME_FirstOwner = "";
		m_DME_AcquiredAt = 0;
		m_DME_AcquiredQuestId = "";
		m_DME_Transferred = false;
	}

	// ==================================================================
	// Public API (fuer OriginService, andere Hooks und Adapter)
	// ==================================================================

	//! Setzt den Origin-Stempel. uid == "" laesst FirstOwner unveraendert (z. B. System-Spawns).
	//! AcquiredAt wird auf JETZT gesetzt; Transferred/OriginEventId bleiben unveraendert.
	void DME_SetOrigin(int originType, string uid, string questId) {
		m_DME_OriginType = originType;
		if (uid != "") {
			m_DME_FirstOwner = uid;
		}
		m_DME_AcquiredQuestId = questId;
		m_DME_AcquiredAt = DME_Tasks_TimeUtil.NowEpoch();
	}

	void DME_SetOriginEventId(string eventId) {
		m_DME_OriginEventId = eventId;
	}

	void DME_MarkTransferred() {
		m_DME_Transferred = true;
	}

	bool DME_HasOrigin() {
		return m_DME_OriginType != EDME_Tasks_OriginType.UNKNOWN;
	}

	int DME_GetOriginType() {
		return m_DME_OriginType;
	}

	string DME_GetOriginEventId() {
		return m_DME_OriginEventId;
	}

	string DME_GetFirstOwner() {
		return m_DME_FirstOwner;
	}

	int DME_GetAcquiredAt() {
		return m_DME_AcquiredAt;
	}

	string DME_GetAcquiredQuestId() {
		return m_DME_AcquiredQuestId;
	}

	bool DME_IsTransferred() {
		return m_DME_Transferred;
	}

	// ==================================================================
	// Persistenz (§6.3-Muster)
	// ==================================================================

	override void CF_OnStoreSave(CF_ModStorageMap storage) {
		super.CF_OnStoreSave(storage);

		CF_ModStorage ctx = storage.Get(DME_TASKS_STORAGE_MOD);
		if (!ctx) {
			return;
		}

		ctx.Write(m_DME_OriginType);
		ctx.Write(m_DME_OriginEventId);
		ctx.Write(m_DME_FirstOwner);
		ctx.Write(m_DME_AcquiredAt);
		ctx.Write(m_DME_AcquiredQuestId);
		ctx.Write(m_DME_Transferred);
	}

	override bool CF_OnStoreLoad(CF_ModStorageMap storage) {
		if (!super.CF_OnStoreLoad(storage)) {
			return false;
		}

		CF_ModStorage ctx = storage.Get(DME_TASKS_STORAGE_MOD);
		if (!ctx) {
			//! Item was saved before this mod existed — keep the defaults. No stream is consumed,
			//! so nothing downstream can desync. This is exactly what the old raw reader got wrong.
			return true;
		}

		if (!ctx.Read(m_DME_OriginType)) {
			return false;
		}
		if (!ctx.Read(m_DME_OriginEventId)) {
			return false;
		}
		if (!ctx.Read(m_DME_FirstOwner)) {
			return false;
		}
		if (!ctx.Read(m_DME_AcquiredAt)) {
			return false;
		}
		if (!ctx.Read(m_DME_AcquiredQuestId)) {
			return false;
		}
		if (!ctx.Read(m_DME_Transferred)) {
			return false;
		}

		//! Future fields: append at the END and guard with `if (ctx.GetVersion() >= 2)`,
		//! bumping storageVersion in config.cpp.

		return true;
	}
}
