//! Dirty-Set + Coalescing-Writer (server-only). KEIN Save pro Frame:
//! MarkDirty sammelt uids; OnUpdate akkumuliert dt und flusht ALLE dirty uids gebuendelt,
//! sobald Settings.FlushIntervalSeconds erreicht ist. FlushAll ignoriert das Intervall (Disconnect/Shutdown).
//! Die eigentliche Datei-Arbeit macht DME_Tasks_PlayerStore.SaveNow — bei Save-Fehlern markiert
//! der Store die uid erneut dirty, damit der naechste Flush es wieder versucht.
class DME_Tasks_SaveQueue {
	private ref map<string, bool> m_DME_DirtyUids;
	private float m_DME_Accumulator;

	void DME_Tasks_SaveQueue() {
		m_DME_DirtyUids = new map<string, bool>();
		m_DME_Accumulator = 0.0;
	}

	void MarkDirty(string uid) {
		if (uid == "") {
			return;
		}
		m_DME_DirtyUids.Set(uid, true);
	}

	void ClearDirty(string uid) {
		if (m_DME_DirtyUids.Contains(uid)) {
			m_DME_DirtyUids.Remove(uid);
		}
	}

	bool IsDirty(string uid) {
		return m_DME_DirtyUids.Contains(uid);
	}

	void OnUpdate(float deltaTime) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		m_DME_Accumulator += deltaTime;
		float flushInterval = GetFlushInterval();
		if (m_DME_Accumulator < flushInterval) {
			return;
		}

		m_DME_Accumulator = 0.0;
		Flush();
	}

	void FlushAll() {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		m_DME_Accumulator = 0.0;
		Flush();
	}

	private void Flush() {
		if (m_DME_DirtyUids.Count() == 0) {
			return;
		}

		// Snapshot + Clear VOR dem Speichern: SaveNow darf bei Fehlern via MarkDirty
		// wieder eintragen, ohne die laufende Iteration zu stoeren.
		array<string> uids = new array<string>();
		foreach (string dirtyUid, bool flagged : m_DME_DirtyUids) {
			uids.Insert(dirtyUid);
		}
		m_DME_DirtyUids.Clear();

		DME_Tasks_PlayerStore store = DME_Tasks_PlayerStore.GetInstance();
		foreach (string flushUid : uids) {
			store.SaveNow(flushUid);
		}

		DME_Tasks_Log.Debug("SaveQueue: %1 PlayerState(s) geflusht", uids.Count().ToString());
	}

	private float GetFlushInterval() {
		DME_Tasks_ConfigService configService = DME_Tasks_ConfigService.GetInstance();
		DME_Tasks_Settings settings = configService.GetSettings();
		if (!settings) {
			return DME_Tasks_Const.DEFAULT_FLUSH_INTERVAL;
		}
		if (settings.FlushIntervalSeconds <= 0) {
			return DME_Tasks_Const.DEFAULT_FLUSH_INTERVAL;
		}
		return settings.FlushIntervalSeconds;
	}
}
