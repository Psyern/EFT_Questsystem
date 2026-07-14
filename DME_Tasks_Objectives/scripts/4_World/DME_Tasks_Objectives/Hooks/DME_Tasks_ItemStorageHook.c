//! Origin-Metadaten am Item (CONTRACTS §6.3 + §6.5, Agent 10) — modded ItemBase mit Persistenz.
//! BEWUSST KEIN CF_ModStorage (braucht CF_MODSTORAGE-Define) — plain OnStoreSave/OnStoreLoad-Muster:
//! super ZUERST, dann eigene Versions-int, dann Felder in FESTER Reihenfolge; beim Load jeden Read pruefen.
//!
//! Legacy-Save-Schutz: Items, die VOR Installation dieses Mods gespeichert wurden, enthalten unsere
//! Felder nicht. Schlaegt bereits der Versions-Read fehl, wird das als Alt-Stand behandelt (Defaults
//! bleiben, return true) — sonst wuerde der erste Serverstart nach Installation alle Items loeschen.
//! Schlaegt ein SPAETERER Read fehl, ist der Stream wirklich korrupt -> return false.
//!
//! WICHTIG (Chain-Regel): Dieses PBO enthaelt ZWEI modded-Deklarationen von ItemBase (diese Datei +
//! DME_Tasks_ItemHook.c). Jede Deklaration ist SELBSTAENDIG — Zugriff auf die hier deklarierten
//! Member/Methoden erfolgt von aussen NUR ueber externe Klassen (DME_Tasks_OriginService,
//! DME_Tasks_QuestItemHelper, ...), nie quer zwischen den beiden modded-Deklarationen
//! (Muster wie DayZExpansion_Vehicles ItemBase.c + ItemBase_Towing.c).
modded class ItemBase {
	private static const int DME_TASKS_ORIGIN_STORE_VERSION = 1;
	private static bool s_DME_Tasks_LegacyStoreLogged = false;

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

	override void OnStoreSave(ParamsWriteContext ctx) {
		super.OnStoreSave(ctx);

		ctx.Write(DME_TASKS_ORIGIN_STORE_VERSION);
		ctx.Write(m_DME_OriginType);
		ctx.Write(m_DME_OriginEventId);
		ctx.Write(m_DME_FirstOwner);
		ctx.Write(m_DME_AcquiredAt);
		ctx.Write(m_DME_AcquiredQuestId);
		ctx.Write(m_DME_Transferred);
	}

	override bool OnStoreLoad(ParamsReadContext ctx, int version) {
		if (!super.OnStoreLoad(ctx, version)) {
			return false;
		}

		int storeVersion;
		if (!ctx.Read(storeVersion)) {
			//! Alt-Save ohne unsere Daten (Erststart nach Installation) — Defaults behalten
			if (!s_DME_Tasks_LegacyStoreLogged) {
				s_DME_Tasks_LegacyStoreLogged = true;
				DME_Tasks_Log.Info("ItemBase.OnStoreLoad: Alt-Save ohne Origin-Daten erkannt — Defaults werden verwendet (einmalige Meldung)");
			}
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

		//! Versions-Guard fuer kuenftige Felder: NUR lesen, wenn storeVersion >= 2 (ans Ende anfuegen!)

		return true;
	}
}
