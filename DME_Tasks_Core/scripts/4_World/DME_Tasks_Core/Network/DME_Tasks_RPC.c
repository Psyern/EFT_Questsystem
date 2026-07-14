//! Statische Server->Client-Sender (CF-RPC, Namespace DME_Tasks_Const.MOD_NAME).
//! Nur der Server sendet hierueber; Fortschritts-Deltas (SendObjectiveProgress) sind
//! nicht garantiert (guaranteed=false), alle anderen Nachrichten garantiert (true).
//! identity darf NIE null sein — SendRPC mit null-Identity waere ein Broadcast an alle.
class DME_Tasks_RPC {
	static void SendSyncTraders(PlayerIdentity identity, string json) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!identity) {
			DME_Tasks_Log.Warn("SendSyncTraders: identity null — RPC nicht gesendet");
			return;
		}

		Param1<string> data = new Param1<string>(json);
		GetRPCManager().SendRPC(DME_Tasks_Const.MOD_NAME, "RPC_SyncTraders", data, true, identity);
	}

	static void SendSyncQuestDefs(PlayerIdentity identity, string traderId, string json) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!identity) {
			DME_Tasks_Log.Warn("SendSyncQuestDefs: identity null — RPC nicht gesendet");
			return;
		}

		Param2<string, string> data = new Param2<string, string>(traderId, json);
		GetRPCManager().SendRPC(DME_Tasks_Const.MOD_NAME, "RPC_SyncQuestDefs", data, true, identity);
	}

	static void SendSyncPlayerState(PlayerIdentity identity, string json) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!identity) {
			DME_Tasks_Log.Warn("SendSyncPlayerState: identity null — RPC nicht gesendet");
			return;
		}

		Param1<string> data = new Param1<string>(json);
		GetRPCManager().SendRPC(DME_Tasks_Const.MOD_NAME, "RPC_SyncPlayerState", data, true, identity);
	}

	static void SendSyncComplete(PlayerIdentity identity) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!identity) {
			DME_Tasks_Log.Warn("SendSyncComplete: identity null — RPC nicht gesendet");
			return;
		}

		Param1<int> data = new Param1<int>(0);
		GetRPCManager().SendRPC(DME_Tasks_Const.MOD_NAME, "RPC_SyncComplete", data, true, identity);
	}

	static void SendObjectiveProgress(PlayerIdentity identity, string questId, string objectiveId, int current) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!identity) {
			DME_Tasks_Log.Warn("SendObjectiveProgress: identity null — RPC nicht gesendet");
			return;
		}

		Param3<string, string, int> data = new Param3<string, string, int>(questId, objectiveId, current);
		GetRPCManager().SendRPC(DME_Tasks_Const.MOD_NAME, "RPC_ObjectiveProgress", data, false, identity);
	}

	static void SendQuestStateChanged(PlayerIdentity identity, string questId, int state) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!identity) {
			DME_Tasks_Log.Warn("SendQuestStateChanged: identity null — RPC nicht gesendet");
			return;
		}

		Param2<string, int> data = new Param2<string, int>(questId, state);
		GetRPCManager().SendRPC(DME_Tasks_Const.MOD_NAME, "RPC_QuestStateChanged", data, true, identity);
	}

	static void SendRewardGranted(PlayerIdentity identity, string json) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!identity) {
			DME_Tasks_Log.Warn("SendRewardGranted: identity null — RPC nicht gesendet");
			return;
		}

		Param1<string> data = new Param1<string>(json);
		GetRPCManager().SendRPC(DME_Tasks_Const.MOD_NAME, "RPC_RewardGranted", data, true, identity);
	}

	static void SendNotification(PlayerIdentity identity, string title, string message, int type) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!identity) {
			DME_Tasks_Log.Warn("SendNotification: identity null — RPC nicht gesendet");
			return;
		}

		Param3<string, string, int> data = new Param3<string, string, int>(title, message, type);
		GetRPCManager().SendRPC(DME_Tasks_Const.MOD_NAME, "RPC_Notification", data, true, identity);
	}
}
