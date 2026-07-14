class DME_Tasks_Log {
	private static const string PREFIX = "[DME_Tasks] ";

	static void Info(string msg, string p1 = "", string p2 = "", string p3 = "") {
		CF_Log.Info(PREFIX + msg, p1, p2, p3);
	}

	static void Warn(string msg, string p1 = "", string p2 = "", string p3 = "") {
		CF_Log.Warn(PREFIX + msg, p1, p2, p3);
	}

	static void Error(string msg, string p1 = "", string p2 = "", string p3 = "") {
		CF_Log.Error(PREFIX + msg, p1, p2, p3);
	}

	static void Debug(string msg, string p1 = "", string p2 = "", string p3 = "") {
		CF_Log.Debug(PREFIX + msg, p1, p2, p3);
	}
}
