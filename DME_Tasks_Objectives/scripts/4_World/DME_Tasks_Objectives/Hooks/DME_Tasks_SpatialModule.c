//! Gemeinsame Zonen-Helfer + Handler-State-Cleanup fuer die raeumlichen Handler (Agent 11).
//!
//! DME_Tasks_SpatialUtil: statische Zonen-Mathematik (Zylinder-Containment, ZoneId-Aufloesung,
//! State-Keys) — Defaults (Radius 25 m / Hoehe 50 m) identisch zum DME_Tasks_ZoneTriggerManager.
//!
//! DME_Tasks_SpatialModule: kleines CF-Modul, das die EngineEvents-Invoker abonniert und den
//! per-Spieler-Zustand der stateful Handler (Travel/EscortDefend) bei Quest-Deaktivierung bzw.
//! Disconnect aufraeumt (Handler-State-Disziplin: Keys = uid|questId|objectiveId).
class DME_Tasks_SpatialUtil {
	static const float DEFAULT_ZONE_RADIUS = 25.0;
	static const float DEFAULT_ZONE_HEIGHT = 50.0;

	//! ZoneId-Aufloesung wie im DME_Tasks_ZoneTriggerManager: Zone.ZoneId, Fallback "questId:objectiveId".
	static string ResolveZoneId(string questId, string objectiveId, DME_Tasks_ZoneVolume zone) {
		if (!zone) {
			return "";
		}
		string zoneId = zone.ZoneId;
		if (zoneId == "") {
			zoneId = questId + ":" + objectiveId;
		}
		return zoneId;
	}

	//! Zylinder-Containment: horizontale Distanz <= Radius UND |dy| <= Hoehe/2 (vertikal symmetrisch).
	static bool IsPositionInZone(vector position, DME_Tasks_ZoneVolume zone) {
		if (!zone) {
			return false;
		}

		float radius = zone.Radius;
		if (radius <= 0) {
			radius = DEFAULT_ZONE_RADIUS;
		}
		float height = zone.Height;
		if (height <= 0) {
			height = DEFAULT_ZONE_HEIGHT;
		}

		float dx = position[0] - zone.CenterX;
		float dz = position[2] - zone.CenterZ;
		float distSq = dx * dx + dz * dz;
		float radiusSq = radius * radius;
		if (distSq > radiusSq) {
			return false;
		}

		float dy = position[1] - zone.CenterY;
		if (dy < 0) {
			dy = -dy;
		}
		float halfHeight = height * 0.5;
		return dy <= halfHeight;
	}

	//! Einheitlicher State-Key fuer Handler-Maps: uid|questId|objectiveId.
	static string StateKey(DME_Tasks_ObjectiveRef objectiveRef) {
		return objectiveRef.PlayerUid + "|" + objectiveRef.QuestId + "|" + objectiveRef.ObjectiveId;
	}
}

//! Cleanup-Verdrahtung: Quest-Ende/Disconnect → stateful Handler leeren ihre Maps.
[CF_RegisterModule(DME_Tasks_SpatialModule)]
class DME_Tasks_SpatialModule : CF_ModuleWorld {
	override void OnInit() {
		super.OnInit();

		DME_Tasks_EngineEvents.s_DME_OnQuestDeactivated.Insert(OnQuestDeactivated);
		DME_Tasks_EngineEvents.s_DME_OnPlayerUnloaded.Insert(OnPlayerUnloaded);
	}

	private void OnQuestDeactivated(string uid, string questId) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		DME_Tasks_TravelHandler travelHandler = GetTravelHandler();
		if (travelHandler) {
			travelHandler.ClearQuestState(uid, questId);
		}

		DME_Tasks_EscortDefendHandler defendHandler = GetEscortDefendHandler();
		if (defendHandler) {
			defendHandler.ClearQuestState(uid, questId);
		}
	}

	private void OnPlayerUnloaded(string uid) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		DME_Tasks_TravelHandler travelHandler = GetTravelHandler();
		if (travelHandler) {
			travelHandler.ClearPlayerState(uid);
		}

		DME_Tasks_EscortDefendHandler defendHandler = GetEscortDefendHandler();
		if (defendHandler) {
			defendHandler.ClearPlayerState(uid);
		}
	}

	//! Registry haelt die einzige Handler-Instanz — hier nur nachschlagen + casten (Null-Check!).
	private DME_Tasks_TravelHandler GetTravelHandler() {
		DME_Tasks_ObjectiveHandlerBase handler = DME_Tasks_ObjectiveRegistry.GetInstance().GetHandler(EDME_Tasks_ObjectiveType.TRAVEL);
		DME_Tasks_TravelHandler travelHandler = DME_Tasks_TravelHandler.Cast(handler);
		return travelHandler;
	}

	private DME_Tasks_EscortDefendHandler GetEscortDefendHandler() {
		DME_Tasks_ObjectiveHandlerBase handler = DME_Tasks_ObjectiveRegistry.GetInstance().GetHandler(EDME_Tasks_ObjectiveType.DEFEND);
		DME_Tasks_EscortDefendHandler defendHandler = DME_Tasks_EscortDefendHandler.Cast(handler);
		return defendHandler;
	}
}
