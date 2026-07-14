//! Zentrales CF-Modul des Questsystems (4_World, da CF_ModuleWorld eine 4_World-Klasse ist).
//! Registriert alle 17 CF-RPCs (SPEC 5.2), verdrahtet Server-Lifecycle (MissionStart/
//! Connect/Disconnect/Update) mit der QuestEngine und publiziert Server->Client-RPCs
//! client-seitig ueber die DME_Tasks_ClientEvents-Invoker.
[CF_RegisterModule(DME_Tasks_CoreModule)]
class DME_Tasks_CoreModule : CF_ModuleWorld {
	override void OnInit() {
		super.OnInit();

		// Client -> Server
		GetRPCManager().AddRPC(DME_Tasks_Const.MOD_NAME, "RPC_RequestSync", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC(DME_Tasks_Const.MOD_NAME, "RPC_AcceptQuest", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC(DME_Tasks_Const.MOD_NAME, "RPC_AbandonQuest", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC(DME_Tasks_Const.MOD_NAME, "RPC_RequestTurnIn", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC(DME_Tasks_Const.MOD_NAME, "RPC_HandoverItems", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC(DME_Tasks_Const.MOD_NAME, "RPC_ClaimReward", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC(DME_Tasks_Const.MOD_NAME, "RPC_SetTracked", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC(DME_Tasks_Const.MOD_NAME, "RPC_ReplaceDaily", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC(DME_Tasks_Const.MOD_NAME, "RPC_MakeChoice", this, SingleplayerExecutionType.Server);

		// Server -> Client
		GetRPCManager().AddRPC(DME_Tasks_Const.MOD_NAME, "RPC_SyncTraders", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC(DME_Tasks_Const.MOD_NAME, "RPC_SyncQuestDefs", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC(DME_Tasks_Const.MOD_NAME, "RPC_SyncPlayerState", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC(DME_Tasks_Const.MOD_NAME, "RPC_SyncComplete", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC(DME_Tasks_Const.MOD_NAME, "RPC_ObjectiveProgress", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC(DME_Tasks_Const.MOD_NAME, "RPC_QuestStateChanged", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC(DME_Tasks_Const.MOD_NAME, "RPC_RewardGranted", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC(DME_Tasks_Const.MOD_NAME, "RPC_Notification", this, SingleplayerExecutionType.Client);

		EnableMissionStart();
		EnableMissionFinish();
		EnableInvokeConnect();
		EnableClientDisconnect();
		EnableUpdate();
	}

	// ------------------------------------------------------------------
	// Lifecycle (server-only)
	// ------------------------------------------------------------------

	override void OnMissionStart(Class sender, CF_EventArgs args) {
		super.OnMissionStart(sender, args);

		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		DME_Tasks_QuestEngine.GetInstance().OnServerInit();
	}

	//! Server-Shutdown/Restart: alle dirty PlayerStates sofort persistieren
	//! (sonst Datenverlustfenster von bis zu Settings.FlushIntervalSeconds).
	override void OnMissionFinish(Class sender, CF_EventArgs args) {
		super.OnMissionFinish(sender, args);

		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		DME_Tasks_PlayerStore.GetInstance().FlushAll();
	}

	override void OnInvokeConnect(Class sender, CF_EventArgs args) {
		super.OnInvokeConnect(sender, args);

		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		CF_EventPlayerArgs cArgs = CF_EventPlayerArgs.Cast(args);
		if (!cArgs) {
			return;
		}
		if (!cArgs.Player || !cArgs.Identity) {
			return;
		}

		DME_Tasks_QuestEngine.GetInstance().OnPlayerConnect(cArgs.Player, cArgs.Identity);
	}

	override void OnClientDisconnect(Class sender, CF_EventArgs args) {
		super.OnClientDisconnect(sender, args);

		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		CF_EventPlayerDisconnectedArgs dArgs = CF_EventPlayerDisconnectedArgs.Cast(args);
		if (!dArgs) {
			return;
		}
		if (dArgs.UID == "") {
			return;
		}

		DME_Tasks_QuestEngine.GetInstance().OnPlayerDisconnect(dArgs.UID);
	}

	override void OnUpdate(Class sender, CF_EventArgs args) {
		super.OnUpdate(sender, args);

		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		CF_EventUpdateArgs updateArgs = CF_EventUpdateArgs.Cast(args);
		if (!updateArgs) {
			return;
		}

		DME_Tasks_QuestEngine.GetInstance().OnUpdate(updateArgs.DeltaTime);
	}

	// ------------------------------------------------------------------
	// Client -> Server RPC-Handler (Methodenname = registrierter funcName)
	// ------------------------------------------------------------------

	void RPC_RequestSync(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target) {
		if (type != CallType.Server) {
			return;
		}
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!sender) {
			return;
		}

		Param1<int> data;
		if (!ctx.Read(data)) {
			return;
		}

		DME_Tasks_QuestEngine.GetInstance().RequestSync(sender);
	}

	void RPC_AcceptQuest(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target) {
		if (type != CallType.Server) {
			return;
		}
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!sender) {
			return;
		}

		Param1<string> data;
		if (!ctx.Read(data)) {
			return;
		}

		DME_Tasks_QuestEngine.GetInstance().AcceptQuest(sender, data.param1);
	}

	void RPC_AbandonQuest(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target) {
		if (type != CallType.Server) {
			return;
		}
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!sender) {
			return;
		}

		Param1<string> data;
		if (!ctx.Read(data)) {
			return;
		}

		DME_Tasks_QuestEngine.GetInstance().AbandonQuest(sender, data.param1);
	}

	void RPC_RequestTurnIn(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target) {
		if (type != CallType.Server) {
			return;
		}
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!sender) {
			return;
		}

		Param1<string> data;
		if (!ctx.Read(data)) {
			return;
		}

		DME_Tasks_QuestEngine.GetInstance().RequestTurnIn(sender, data.param1);
	}

	void RPC_HandoverItems(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target) {
		if (type != CallType.Server) {
			return;
		}
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!sender) {
			return;
		}

		Param2<string, string> data;
		if (!ctx.Read(data)) {
			return;
		}

		DME_Tasks_QuestEngine.GetInstance().HandoverItems(sender, data.param1, data.param2);
	}

	void RPC_ClaimReward(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target) {
		if (type != CallType.Server) {
			return;
		}
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!sender) {
			return;
		}

		Param1<string> data;
		if (!ctx.Read(data)) {
			return;
		}

		DME_Tasks_QuestEngine.GetInstance().ClaimReward(sender, data.param1);
	}

	void RPC_SetTracked(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target) {
		if (type != CallType.Server) {
			return;
		}
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!sender) {
			return;
		}

		Param1<string> data;
		if (!ctx.Read(data)) {
			return;
		}

		DME_Tasks_QuestEngine.GetInstance().SetTracked(sender, data.param1);
	}

	void RPC_ReplaceDaily(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target) {
		if (type != CallType.Server) {
			return;
		}
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!sender) {
			return;
		}

		Param1<string> data;
		if (!ctx.Read(data)) {
			return;
		}

		DME_Tasks_QuestEngine.GetInstance().ReplaceDaily(sender, data.param1);
	}

	void RPC_MakeChoice(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target) {
		if (type != CallType.Server) {
			return;
		}
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!sender) {
			return;
		}

		Param2<string, string> data;
		if (!ctx.Read(data)) {
			return;
		}

		DME_Tasks_QuestEngine.GetInstance().MakeChoice(sender, data.param1, data.param2);
	}

	// ------------------------------------------------------------------
	// Server -> Client RPC-Handler (publizieren via DME_Tasks_ClientEvents)
	// ------------------------------------------------------------------

	void RPC_SyncTraders(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target) {
		if (type != CallType.Client) {
			return;
		}
		if (g_Game && g_Game.IsDedicatedServer()) {
			return;
		}

		Param1<string> data;
		if (!ctx.Read(data)) {
			return;
		}

		DME_Tasks_ClientEvents.s_DME_OnSyncTraders.Invoke(data.param1);
	}

	void RPC_SyncQuestDefs(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target) {
		if (type != CallType.Client) {
			return;
		}
		if (g_Game && g_Game.IsDedicatedServer()) {
			return;
		}

		Param2<string, string> data;
		if (!ctx.Read(data)) {
			return;
		}

		DME_Tasks_ClientEvents.s_DME_OnSyncQuestDefs.Invoke(data.param1, data.param2);
	}

	void RPC_SyncPlayerState(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target) {
		if (type != CallType.Client) {
			return;
		}
		if (g_Game && g_Game.IsDedicatedServer()) {
			return;
		}

		Param1<string> data;
		if (!ctx.Read(data)) {
			return;
		}

		DME_Tasks_ClientEvents.s_DME_OnSyncPlayerState.Invoke(data.param1);
	}

	void RPC_SyncComplete(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target) {
		if (type != CallType.Client) {
			return;
		}
		if (g_Game && g_Game.IsDedicatedServer()) {
			return;
		}

		Param1<int> data;
		if (!ctx.Read(data)) {
			return;
		}

		DME_Tasks_ClientEvents.s_DME_OnSyncComplete.Invoke();
	}

	void RPC_ObjectiveProgress(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target) {
		if (type != CallType.Client) {
			return;
		}
		if (g_Game && g_Game.IsDedicatedServer()) {
			return;
		}

		Param3<string, string, int> data;
		if (!ctx.Read(data)) {
			return;
		}

		DME_Tasks_ClientEvents.s_DME_OnObjectiveProgress.Invoke(data.param1, data.param2, data.param3);
	}

	void RPC_QuestStateChanged(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target) {
		if (type != CallType.Client) {
			return;
		}
		if (g_Game && g_Game.IsDedicatedServer()) {
			return;
		}

		Param2<string, int> data;
		if (!ctx.Read(data)) {
			return;
		}

		DME_Tasks_ClientEvents.s_DME_OnQuestStateChanged.Invoke(data.param1, data.param2);
	}

	void RPC_RewardGranted(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target) {
		if (type != CallType.Client) {
			return;
		}
		if (g_Game && g_Game.IsDedicatedServer()) {
			return;
		}

		Param1<string> data;
		if (!ctx.Read(data)) {
			return;
		}

		DME_Tasks_ClientEvents.s_DME_OnRewardGranted.Invoke(data.param1);
	}

	void RPC_Notification(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target) {
		if (type != CallType.Client) {
			return;
		}
		if (g_Game && g_Game.IsDedicatedServer()) {
			return;
		}

		Param6<string, string, string, string, string, int> data;
		if (!ctx.Read(data)) {
			return;
		}

		DME_Tasks_ClientEvents.s_DME_OnNotification.Invoke(data.param1, data.param2, data.param3, data.param4, data.param5, data.param6);
	}
}
