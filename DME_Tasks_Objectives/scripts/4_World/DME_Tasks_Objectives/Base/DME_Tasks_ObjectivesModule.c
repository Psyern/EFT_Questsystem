//! CF-Modul des Objectives-PBO (CONTRACTS §6.5) — verdrahtet die DME_Tasks_EngineEvents-Invoker
//! mit ObjectiveIndex + ZoneTriggerManager, registriert die Handler bei MissionStart und
//! dispatcht 1-Hz-TimerEvents NUR fuer Spieler mit TIMER_TICK-Refs im Index.
[CF_RegisterModule(DME_Tasks_ObjectivesModule)]
class DME_Tasks_ObjectivesModule : CF_ModuleWorld {
	private static const float TIMER_TICK_SECONDS = 1.0;

	private float m_DME_TimerAccumulator;
	private ref array<string> m_DME_OnlineUids;
	private ref array<string> m_DME_TickScratch;

	void DME_Tasks_ObjectivesModule() {
		m_DME_TimerAccumulator = 0.0;
		m_DME_OnlineUids = new array<string>();
		m_DME_TickScratch = new array<string>();
	}

	override void OnInit() {
		super.OnInit();

		DME_Tasks_EngineEvents.s_DME_OnQuestActivated.Insert(OnQuestActivated);
		DME_Tasks_EngineEvents.s_DME_OnQuestDeactivated.Insert(OnQuestDeactivated);
		DME_Tasks_EngineEvents.s_DME_OnPlayerLoaded.Insert(OnPlayerLoaded);
		DME_Tasks_EngineEvents.s_DME_OnPlayerUnloaded.Insert(OnPlayerUnloaded);

		EnableMissionStart();
		EnableUpdate();
	}

	// ------------------------------------------------------------------
	// CF-Lifecycle
	// ------------------------------------------------------------------

	override void OnMissionStart(Class sender, CF_EventArgs args) {
		super.OnMissionStart(sender, args);

		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		DME_Tasks_ObjectiveRegistry.GetInstance().RegisterAll();
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

		m_DME_TimerAccumulator += updateArgs.DeltaTime;
		if (m_DME_TimerAccumulator < TIMER_TICK_SECONDS) {
			return;
		}
		m_DME_TimerAccumulator -= TIMER_TICK_SECONDS;
		//! Burst-Klemmung: nach Lag-Spikes keine Tick-Salven nachholen
		if (m_DME_TimerAccumulator > TIMER_TICK_SECONDS) {
			m_DME_TimerAccumulator = 0.0;
		}

		DispatchTimerTicks();
	}

	// ------------------------------------------------------------------
	// EngineEvents-Abos (Core → Objectives-Seam)
	// ------------------------------------------------------------------

	private void OnQuestActivated(string uid, string questId) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		DME_Tasks_ObjectiveIndex.GetInstance().AddQuest(uid, questId);
		DME_Tasks_ZoneTriggerManager.GetInstance().OnQuestActivated(uid, questId);

		//! Initial-Zaehlung bereits gehaltener Items: COLLECT setzt den Fortschritt absolut per
		//! Recount — ohne diesen Kick zaehlt eine frisch aktivierte Quest erst beim naechsten Item-Move.
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

	private void OnQuestDeactivated(string uid, string questId) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		DME_Tasks_ObjectiveIndex.GetInstance().RemoveQuest(uid, questId);
		DME_Tasks_ZoneTriggerManager.GetInstance().OnQuestDeactivated(uid, questId);
	}

	private void OnPlayerLoaded(string uid) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (uid == "") {
			return;
		}

		if (m_DME_OnlineUids.Find(uid) == -1) {
			m_DME_OnlineUids.Insert(uid);
		}
	}

	private void OnPlayerUnloaded(string uid) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		m_DME_OnlineUids.RemoveItem(uid);
		DME_Tasks_ObjectiveIndex.GetInstance().RemovePlayer(uid);
		DME_Tasks_ZoneTriggerManager.GetInstance().OnPlayerRemoved(uid);
	}

	// ------------------------------------------------------------------
	// 1-Hz-Timer-Ticks
	// ------------------------------------------------------------------

	//! CountRefs-Guard zuerst (allokationsfrei); nur fuer Spieler MIT TIMER_TICK-Refs wird ein
	//! DME_Tasks_TimerEvent gebaut (Position aus der Online-Spielerliste, EIN GetPlayers-Pass).
	private void DispatchTimerTicks() {
		if (m_DME_OnlineUids.Count() == 0) {
			return;
		}

		DME_Tasks_ObjectiveIndex index = DME_Tasks_ObjectiveIndex.GetInstance();
		m_DME_TickScratch.Clear();
		foreach (string uid : m_DME_OnlineUids) {
			if (index.CountRefs(uid, EDME_Tasks_EventType.TIMER_TICK) > 0) {
				m_DME_TickScratch.Insert(uid);
			}
		}
		if (m_DME_TickScratch.Count() == 0) {
			return;
		}

		int nowEpoch = DME_Tasks_TimeUtil.NowEpoch();
		DME_Tasks_EventBus bus = DME_Tasks_EventBus.GetInstance();

		array<Man> players = new array<Man>();
		g_Game.GetPlayers(players);
		foreach (Man man : players) {
			if (!man) {
				continue;
			}
			PlayerIdentity identity = man.GetIdentity();
			if (!identity) {
				continue;
			}
			string playerUid = identity.GetId();
			if (m_DME_TickScratch.Find(playerUid) == -1) {
				continue;
			}

			DME_Tasks_TimerEvent evt = new DME_Tasks_TimerEvent();
			evt.m_DME_PlayerUid = playerUid;
			evt.m_DME_NowEpoch = nowEpoch;
			evt.m_DME_Position = man.GetPosition();
			bus.Dispatch(evt);
		}
	}
}
