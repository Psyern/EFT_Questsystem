//! Kill-/Boss-Objective-Handler (SPEC §7 Agent 9 + CONTRACTS §6.5) — bedient
//! EDME_Tasks_ObjectiveType.KILL und BOSS, indexiert unter EDME_Tasks_EventType.KILL.
//! Instanziierung ausschliesslich durch DME_Tasks_ObjectiveRegistry.RegisterAll (Singleton dort).
//! Alle Filter stammen aus DME_Tasks_ObjectiveDef; Fortschritt NUR via ReportProgress (Basisklasse).
//!
//! Dokumentierte Filter-Entscheidungen:
//! - HitZone: EXAKTER, case-sensitiver Stringvergleich mit dmgZone aus EEHitBy (bewusst KEIN
//!   ToLower/case-insensitiver Vergleich — Admins pflegen die Zone exakt wie im Damage-System
//!   benannt, z. B. "Head", "Brain", "Torso", "LeftArm").
//! - RequiredZones: matcht gegen Def.Zone.ZoneId der EIGENEN Objective — RequiredZones referenziert
//!   KEINE Zonen anderer Quests/Objectives. Ist RequiredZones gesetzt, MUSS Def.Zone gepflegt sein
//!   und dessen ZoneId in RequiredZones vorkommen; sonst Warn-once + kein Fortschritt
//!   (Fehlkonfiguration, damit Admins es im Log sehen statt stiller Progress-Luecken).
//! - Zonen-Geometrie: horizontale Distanz (XZ) der Kill-Position zum Zonen-Zentrum <= Radius
//!   (Radius <= 0 → 25 m Default, identisch zum ZoneTriggerManager). Die Hoehe wird beim
//!   Kill-Filter bewusst ignoriert — vertikale Praezision liefert nur der DME_Tasks_ZoneTrigger.
//! - BossId: Ist Def.BossId gesetzt und traegt das Event eine BossId (Boss-Adapter Welle 4),
//!   muessen beide exakt uebereinstimmen. Traegt das Event KEINE BossId (kein Adapter aktiv),
//!   greift der Klassennamen-Fallback: Def.ClassNames muss gepflegt sein (der ClassNames-Filter
//!   hat dann bereits gematcht) — kein Hard-Ref auf einen Boss-Mod (CONTRACTS §6.3).
//! - AcquiredAfterQuestAccept-Analogon entfaellt: Kills zaehlen nur waehrend die Quest aktiv ist,
//!   da der Active-Objective-Index ausschliesslich aktive, nicht erledigte Objectives enthaelt.
class DME_Tasks_KillHandler : DME_Tasks_ObjectiveHandlerBase {
	private static const float DEFAULT_ZONE_RADIUS = 25.0;

	private bool m_DME_WarnedZoneMisconfig;

	void DME_Tasks_KillHandler() {
		m_DME_WarnedZoneMisconfig = false;
	}

	override bool CanHandle(int objectiveType) {
		if (objectiveType == EDME_Tasks_ObjectiveType.KILL) {
			return true;
		}
		return objectiveType == EDME_Tasks_ObjectiveType.BOSS;
	}

	override void GetEventTypesFor(int objectiveType, array<int> outTypes) {
		if (!outTypes) {
			return;
		}
		outTypes.Insert(EDME_Tasks_EventType.KILL);
	}

	override void OnEvent(DME_Tasks_ObjectiveRef objectiveRef, DME_Tasks_Event evt) {
		if (!objectiveRef || !objectiveRef.Def) {
			return;
		}

		DME_Tasks_KillEvent killEvt = DME_Tasks_KillEvent.Cast(evt);
		if (!killEvt) {
			return;
		}

		DME_Tasks_ObjectiveDef def = objectiveRef.Def;

		if (!MatchesClassNames(killEvt.m_DME_VictimType, def.ClassNames)) {
			return;
		}
		if (!MatchesTargetCategory(killEvt, def.TargetCategory)) {
			return;
		}
		if (!MatchesBossId(killEvt, def)) {
			return;
		}
		if (!MatchesDistance(killEvt.m_DME_Distance, def.MinimumDistance, def.MaximumDistance)) {
			return;
		}
		if (!MatchesClassNames(killEvt.m_DME_WeaponType, def.RequiredWeaponCategories)) {
			return;
		}
		if (def.SuppressorRequired && !killEvt.m_DME_Suppressed) {
			return;
		}
		if (def.HitZone != "" && killEvt.m_DME_HitZone != def.HitZone) {
			return;
		}
		if (!MatchesZone(killEvt.m_DME_Position, def)) {
			return;
		}
		if (!IsInTimeWindow(def.FromHour, def.ToHour)) {
			return;
		}

		ReportProgress(objectiveRef, 1);
	}

	// ==================================================================
	// Filter
	// ==================================================================

	//! TargetCategory ("INFECTED"/"PLAYER"/"ANIMAL"/"AI") gegen m_DME_VictimCategory;
	//! ist die Kategorie im Event leer, wird sie aus den Opfer-Flags abgeleitet.
	private bool MatchesTargetCategory(DME_Tasks_KillEvent killEvt, string targetCategory) {
		if (targetCategory == "") {
			return true;
		}

		string category = killEvt.m_DME_VictimCategory;
		if (category == "") {
			if (killEvt.m_DME_VictimIsPlayer) {
				category = "PLAYER";
			} else if (killEvt.m_DME_VictimIsInfected) {
				category = "INFECTED";
			} else if (killEvt.m_DME_VictimIsAnimal) {
				category = "ANIMAL";
			} else if (killEvt.m_DME_VictimIsAI) {
				category = "AI";
			}
		}
		return category == targetCategory;
	}

	//! Siehe Header: exakter Match wenn das Event eine BossId traegt; sonst Klassennamen-Fallback.
	private bool MatchesBossId(DME_Tasks_KillEvent killEvt, DME_Tasks_ObjectiveDef def) {
		if (def.BossId == "") {
			return true;
		}
		if (killEvt.m_DME_BossId != "") {
			return killEvt.m_DME_BossId == def.BossId;
		}
		if (def.ClassNames && def.ClassNames.Count() > 0) {
			return true;
		}
		return false;
	}

	//! -1 = ungesetzt (ObjectiveDef-Default). Distanz stammt aus dem EEHitBy-Puffer des Kill-Hooks.
	private bool MatchesDistance(float distance, float minimumDistance, float maximumDistance) {
		if (minimumDistance >= 0.0 && distance < minimumDistance) {
			return false;
		}
		if (maximumDistance >= 0.0 && distance > maximumDistance) {
			return false;
		}
		return true;
	}

	//! Zonen-Filter (Semantik siehe Header). Ohne Zone und ohne RequiredZones: keine Einschraenkung.
	private bool MatchesZone(vector position, DME_Tasks_ObjectiveDef def) {
		bool hasRequiredZones = false;
		if (def.RequiredZones && def.RequiredZones.Count() > 0) {
			hasRequiredZones = true;
		}
		if (!def.Zone && !hasRequiredZones) {
			return true;
		}

		if (!def.Zone) {
			WarnZoneMisconfig(def.ObjectiveId, "RequiredZones gesetzt, aber Zone fehlt");
			return false;
		}

		if (hasRequiredZones) {
			bool zoneListed = false;
			if (def.Zone.ZoneId != "" && def.RequiredZones.Find(def.Zone.ZoneId) > -1) {
				zoneListed = true;
			}
			if (!zoneListed) {
				WarnZoneMisconfig(def.ObjectiveId, "Zone.ZoneId nicht in RequiredZones");
				return false;
			}
		}

		float radius = def.Zone.Radius;
		if (radius <= 0.0) {
			radius = DEFAULT_ZONE_RADIUS;
		}

		float dx = position[0] - def.Zone.CenterX;
		float dz = position[2] - def.Zone.CenterZ;
		float distSq = (dx * dx) + (dz * dz);
		return distSq <= (radius * radius);
	}

	private void WarnZoneMisconfig(string objectiveId, string reason) {
		if (m_DME_WarnedZoneMisconfig) {
			return;
		}
		m_DME_WarnedZoneMisconfig = true;
		DME_Tasks_Log.Warn("KillHandler: Zonen-Fehlkonfiguration bei Objective %1 (%2)", objectiveId, reason);
	}
}
