//! Persistenz fuer DME_Tasks_PlayerState (Singleton, ausschliesslich Server).
//!
//! Atomares Speichern (verifiziert 2026-07-14): EnScript 1.29 hat KEINE native Rename-/Move-API
//! (Grep ueber "scripts - 1.29\1_Core" und CF; CF_File.Rename ist selbst nur CopyFile+DeleteFile).
//! Fallback-Muster daher: SaveFile -> <uid>.json.tmp -> Backup-Rotation der bestehenden
//! <uid>.json -> CopyFile(tmp -> final) -> DeleteFile(tmp). Schlaegt der tmp-Write fehl,
//! bleibt die bestehende Datei unangetastet; schlaegt die Kopie fehl, existiert Backup-Slot 0.
//!
//! Backup-Rotation (CopyFile-Kette, einfachste korrekte Loesung ohne Rename):
//! Slot 0 = neuestes Backup. Vor jedem Ueberschreiben von <uid>.json:
//! aeltester Slot (BackupSlots-1) wird geloescht, dann jeder Slot s -> s+1 verschoben
//! (CopyFile+DeleteFile), dann die aktuelle <uid>.json nach Slot 0 kopiert.
//! Laden nach Fehler probiert die Slots 0..BackupSlots-1 in dieser Reihenfolge (neuester zuerst).
class DME_Tasks_PlayerStore {
	private static ref DME_Tasks_PlayerStore s_DME_Instance;

	private ref map<string, ref DME_Tasks_PlayerState> m_DME_Cache;
	private ref DME_Tasks_SaveQueue m_DME_SaveQueue;

	static DME_Tasks_PlayerStore GetInstance() {
		if (!s_DME_Instance) {
			s_DME_Instance = new DME_Tasks_PlayerStore();
		}
		return s_DME_Instance;
	}

	void DME_Tasks_PlayerStore() {
		m_DME_Cache = new map<string, ref DME_Tasks_PlayerState>();
		m_DME_SaveQueue = new DME_Tasks_SaveQueue();
	}

	//! Laedt den PlayerState (Datei -> Backups -> neu), migriert ihn und baut den UnlockLedger auf.
	DME_Tasks_PlayerState LoadOrCreate(string uid) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return null;
		}
		if (uid == "") {
			DME_Tasks_Log.Warn("LoadOrCreate: leere uid ignoriert");
			return null;
		}

		DME_Tasks_PlayerState cached = m_DME_Cache.Get(uid);
		if (cached) {
			return cached;
		}

		CleanupStaleTmp(uid);

		bool needsSave = false;
		DME_Tasks_PlayerState state = LoadFromDisk(uid, needsSave);
		if (!state) {
			state = new DME_Tasks_PlayerState();
			state.Version = DME_Tasks_Const.STATE_SCHEMA_VERSION;
			state.SteamId = uid;
			needsSave = true;
			DME_Tasks_Log.Info("Neuer PlayerState angelegt fuer %1", uid);
		}

		int versionBefore = state.Version;
		DME_Tasks_Migration.Migrate(state);
		if (state.Version != versionBefore) {
			needsSave = true;
		}
		if (state.SteamId != uid) {
			state.SteamId = uid;
			needsSave = true;
		}

		m_DME_Cache.Set(uid, state);

		DME_Tasks_UnlockLedger.GetInstance().RebuildFor(uid);

		if (needsSave) {
			m_DME_SaveQueue.MarkDirty(uid);
		}

		return state;
	}

	//! null, wenn nicht geladen (kein implizites Laden).
	DME_Tasks_PlayerState Get(string uid) {
		return m_DME_Cache.Get(uid);
	}

	void MarkDirty(string uid) {
		m_DME_SaveQueue.MarkDirty(uid);
	}

	//! Delegiert an den Coalescing-Writer — flusht alle dirty uids alle Settings.FlushIntervalSeconds.
	void OnUpdate(float deltaTime) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		m_DME_SaveQueue.OnUpdate(deltaTime);
	}

	//! Atomar: tmp schreiben -> Backup-Rotation -> CopyFile(tmp -> final) -> DeleteFile(tmp).
	void SaveNow(string uid) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (uid == "") {
			return;
		}

		DME_Tasks_PlayerState state = m_DME_Cache.Get(uid);
		if (!state) {
			DME_Tasks_Log.Warn("SaveNow: kein geladener State fuer %1 — uebersprungen", uid);
			m_DME_SaveQueue.ClearDirty(uid);
			return;
		}

		string tmpPath = DME_Tasks_Paths.PlayerTmpFile(uid);
		string finalPath = DME_Tasks_Paths.PlayerFile(uid);

		string saveError;
		bool savedOk = JsonFileLoader<DME_Tasks_PlayerState>.SaveFile(tmpPath, state, saveError);
		if (!savedOk) {
			DME_Tasks_Log.Error("SaveNow: tmp-Write fehlgeschlagen fuer %1: %2", uid, saveError);
			m_DME_SaveQueue.MarkDirty(uid);
			return;
		}

		RotateBackups(uid);

		bool copiedOk = CopyFile(tmpPath, finalPath);
		if (!copiedOk) {
			DME_Tasks_Log.Error("SaveNow: CopyFile tmp -> final fehlgeschlagen fuer %1 (%2)", uid, finalPath);
			m_DME_SaveQueue.MarkDirty(uid);
			return;
		}

		bool tmpDeleted = DeleteFile(tmpPath);
		if (!tmpDeleted) {
			DME_Tasks_Log.Warn("SaveNow: tmp-Datei nicht loeschbar: %1", tmpPath);
		}

		m_DME_SaveQueue.ClearDirty(uid);
	}

	//! Sofort-Flush aller dirty uids (Intervall wird ignoriert) — z. B. bei Server-Shutdown.
	void FlushAll() {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		m_DME_SaveQueue.FlushAll();
	}

	//! Disconnect: sofort speichern und aus dem Cache entfernen.
	void Unload(string uid) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (uid == "") {
			return;
		}

		DME_Tasks_PlayerState state = m_DME_Cache.Get(uid);
		if (state) {
			SaveNow(uid);
		}

		if (m_DME_Cache.Contains(uid)) {
			m_DME_Cache.Remove(uid);
		}
		m_DME_SaveQueue.ClearDirty(uid);
	}

	// ------------------------------------------------------------------
	// intern
	// ------------------------------------------------------------------

	//! Primaerdatei laden; bei Fehler Backups (neuester zuerst) probieren.
	//! restoredFromBackup = true, wenn der State aus einem Backup kam (Primaerdatei wird beim
	//! naechsten Flush wieder gesund geschrieben).
	private DME_Tasks_PlayerState LoadFromDisk(string uid, out bool restoredFromBackup) {
		restoredFromBackup = false;

		string primaryPath = DME_Tasks_Paths.PlayerFile(uid);
		if (FileExist(primaryPath)) {
			string loadError;
			DME_Tasks_PlayerState primaryState = new DME_Tasks_PlayerState();
			bool loadedOk = JsonFileLoader<DME_Tasks_PlayerState>.LoadFile(primaryPath, primaryState, loadError);
			if (loadedOk && primaryState) {
				return primaryState;
			}
			DME_Tasks_Log.Warn("PlayerState %1 unlesbar (%2) — versuche Backups", uid, loadError);
		}

		int slots = GetBackupSlots();
		for (int slot = 0; slot < slots; slot++) {
			string backupPath = DME_Tasks_Paths.BackupFile(uid, slot);
			if (!FileExist(backupPath)) {
				continue;
			}

			string backupError;
			DME_Tasks_PlayerState backupState = new DME_Tasks_PlayerState();
			bool backupOk = JsonFileLoader<DME_Tasks_PlayerState>.LoadFile(backupPath, backupState, backupError);
			if (backupOk && backupState) {
				DME_Tasks_Log.Warn("PlayerState %1 aus Backup-Slot %2 wiederhergestellt", uid, slot.ToString());
				restoredFromBackup = true;
				return backupState;
			}
			DME_Tasks_Log.Warn("Backup-Slot unlesbar (%1): %2", backupPath, backupError);
		}

		return null;
	}

	//! Rotation via CopyFile-Kette (kein natives Rename verfuegbar): aeltester Slot faellt raus,
	//! jeder Slot rueckt um 1 auf, aktuelle <uid>.json wird Slot 0.
	private void RotateBackups(string uid) {
		int slots = GetBackupSlots();
		if (slots <= 0) {
			return;
		}

		string finalPath = DME_Tasks_Paths.PlayerFile(uid);
		if (!FileExist(finalPath)) {
			return;
		}

		string oldestPath = DME_Tasks_Paths.BackupFile(uid, slots - 1);
		if (FileExist(oldestPath)) {
			bool oldestDeleted = DeleteFile(oldestPath);
			if (!oldestDeleted) {
				DME_Tasks_Log.Warn("Backup-Rotation: aeltester Slot nicht loeschbar: %1", oldestPath);
			}
		}

		for (int slot = slots - 2; slot >= 0; slot--) {
			string fromPath = DME_Tasks_Paths.BackupFile(uid, slot);
			if (!FileExist(fromPath)) {
				continue;
			}

			string toPath = DME_Tasks_Paths.BackupFile(uid, slot + 1);
			bool shifted = CopyFile(fromPath, toPath);
			if (!shifted) {
				DME_Tasks_Log.Warn("Backup-Rotation: Shift fehlgeschlagen: %1 -> %2", fromPath, toPath);
				continue;
			}

			bool shiftSourceDeleted = DeleteFile(fromPath);
			if (!shiftSourceDeleted) {
				DME_Tasks_Log.Warn("Backup-Rotation: Quelle nach Shift nicht loeschbar: %1", fromPath);
			}
		}

		string newestPath = DME_Tasks_Paths.BackupFile(uid, 0);
		bool backedUp = CopyFile(finalPath, newestPath);
		if (!backedUp) {
			DME_Tasks_Log.Warn("Backup-Rotation: Kopie nach Slot 0 fehlgeschlagen: %1 -> %2", finalPath, newestPath);
		}
	}

	//! Verwaiste tmp-Datei eines abgebrochenen Saves entfernen (Primaerdatei/Backups sind autoritativ).
	private void CleanupStaleTmp(string uid) {
		string tmpPath = DME_Tasks_Paths.PlayerTmpFile(uid);
		if (!FileExist(tmpPath)) {
			return;
		}

		DME_Tasks_Log.Warn("Verwaiste tmp-Datei gefunden (unvollstaendiger Save?) — wird entfernt: %1", tmpPath);
		bool staleDeleted = DeleteFile(tmpPath);
		if (!staleDeleted) {
			DME_Tasks_Log.Warn("Verwaiste tmp-Datei nicht loeschbar: %1", tmpPath);
		}
	}

	private int GetBackupSlots() {
		DME_Tasks_ConfigService configService = DME_Tasks_ConfigService.GetInstance();
		DME_Tasks_Settings settings = configService.GetSettings();
		if (!settings) {
			return DME_Tasks_Const.DEFAULT_BACKUP_SLOTS;
		}
		if (settings.BackupSlots < 0) {
			return DME_Tasks_Const.DEFAULT_BACKUP_SLOTS;
		}
		return settings.BackupSlots;
	}
}
