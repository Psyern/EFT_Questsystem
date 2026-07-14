//! Kill-Hooks (SPEC §7 Agent 9 + CONTRACTS §6.5) — modded PlayerBase/ZombieBase/AnimalBase.
//! EEHitBy (super ZUERST) puffert Last-Hit-Metadaten in m_DME_-Membern des OPFERS; der LETZTE
//! Treffer ueberschreibt den Puffer immer vollstaendig (inkl. Loeschen der Schuetzen-Uid bei
//! Nicht-Spieler-Quellen), damit z. B. ein spaeterer Zombie-/Umwelt-Treffer keinen Spieler-Credit
//! hinterlaesst. EEKilled (super ZUERST — Projektregel; Vanilla-PlayerBase ruft super zuletzt,
//! WIR bewusst zuerst) baut daraus ein DME_Tasks_KillEvent und dispatcht es NUR, wenn die
//! Schuetzen-Uid KILL-Refs im Active-Objective-Index hat (CountRefs-Cheap-Guard, allokationsfrei).
//!
//! Quellcode-verifizierte APIs (scripts - 1.29):
//! - EEHitBy(TotalDamageResult, int, EntityAI source, int, string dmgZone, string ammo, vector, float)
//!   — EntityAI; Overrides: PlayerBase.c:1224, ZombieBase.c:969 (AnimalBase erbt EntityAI-Fassung).
//! - EEKilled(Object killer) — EntityAI.c:1078; PlayerBase-Override PlayerBase.c:1183.
//! - source-Semantik (PluginAdminLog.c:219ff): FIRE_ARM/Melee-Waffe → source IST die Waffe,
//!   Schuetze = Hierarchy-Root; Faeuste → source IST der Spieler selbst.
//! - Man GetHierarchyRootPlayer() — EntityAI.c:877.
//! - HumanInventory GetHumanInventory() — Man.c:79; EntityAI GetEntityInHands() — HumanInventory.c:16.
//! - ItemSuppressor GetAttachedSuppressor() — Weapon.c:407 (proto native; Weapon_Base extends Weapon,
//!   Weapon_Base.c:38) — kein Slotnamen-Raten noetig.
//!
//! Dokumentierte Entscheidungen:
//! - Distanz = vector.Distance(Schuetzen-Root-Position, Opfer-Position) zum Trefferzeitpunkt
//!   (Muster PluginAdminLog.c:263).
//! - Event-Position = Opfer-Position beim Tod (Kill-Ort fuer den Zonen-Filter des KillHandlers).
//! - m_DME_VictimIsAI bleibt false und m_DME_BossId bleibt "" — die setzen erst die Welle-4-Adapter
//!   (AI/Boss) ueber den DME_Tasks_KillClassifier-Seam (Classify-Aufruf in DispatchKill vor dem
//!   Dispatch, CONTRACTS §6.8); der KillHandler hat dafuer den Klassennamen-Fallback.
//! - Quellen ohne Spieler-Hierarchie (geworfene Granaten, Feuer, Fallschaden) ergeben keine
//!   Schuetzen-Uid → kein Dispatch (bewusst: kein Kill-Credit ohne eindeutigen Verursacher).
//! - Selbst-Kill (Schuetzen-Uid == Opfer-Uid) zaehlt NICHT als Kill-Fortschritt;
//!   QuestEngine.OnPlayerDied laeuft davon unabhaengig immer (FailOnDeath, CONTRACTS §6.2).

//! Statische Helfer — gemeinsame Logik der drei modded Opfer-Klassen (kein gemeinsamer Basistyp moeglich).
class DME_Tasks_KillHookUtil {
	//! Kill-Credit nur, wenn der letzte Spieler-Treffer hoechstens so viele Sekunden zurueckliegt
	//! (verhindert Credit fuer laengst veraltete Treffer, z. B. spaeteres Verbluten/Umwelt-Tod).
	static const int DME_KILL_CREDIT_WINDOW_SECONDS = 60;

	//! Schuetze (PlayerBase) aus der EEHitBy-source: Waffe/Melee → Hierarchy-Root-Spieler;
	//! Faeuste → source selbst. null bei Quellen ohne Spieler-Hierarchie.
	static PlayerBase GetShooter(EntityAI source) {
		if (!source) {
			return null;
		}

		PlayerBase shooter = null;
		Man rootMan = source.GetHierarchyRootPlayer();
		if (rootMan) {
			shooter = PlayerBase.Cast(rootMan);
		}
		if (!shooter) {
			shooter = PlayerBase.Cast(source);
		}
		return shooter;
	}

	//! Waffentyp: source selbst, wenn source nicht der Schuetze ist (Fernwaffe/Melee-Waffe laut
	//! PluginAdminLog-Semantik); sonst (Faeuste) das Item in den Haenden des Schuetzen — "" wenn keins.
	static string GetWeaponType(EntityAI source, PlayerBase shooter) {
		if (source && source != shooter) {
			return source.GetType();
		}
		if (!shooter) {
			return "";
		}

		HumanInventory humanInventory = shooter.GetHumanInventory();
		if (!humanInventory) {
			return "";
		}
		EntityAI inHands = humanInventory.GetEntityInHands();
		if (!inHands) {
			return "";
		}
		return inHands.GetType();
	}

	//! Suppressor-Check ueber die native Weapon-API (Weapon.c:407). Fallback auf die Waffe in den
	//! Haenden des Schuetzen, falls source keine Weapon_Base ist.
	static bool IsSuppressed(EntityAI source, PlayerBase shooter) {
		Weapon_Base weapon = Weapon_Base.Cast(source);
		if (!weapon && shooter) {
			HumanInventory humanInventory = shooter.GetHumanInventory();
			if (humanInventory) {
				weapon = Weapon_Base.Cast(humanInventory.GetEntityInHands());
			}
		}
		if (!weapon) {
			return false;
		}

		ItemSuppressor suppressor = weapon.GetAttachedSuppressor();
		if (suppressor) {
			return true;
		}
		return false;
	}

	//! Baut das DME_Tasks_KillEvent aus dem EEHitBy-Puffer des Opfers und dispatcht es.
	//! Cheap-Guard: nichts bauen, wenn der Schuetze keine KILL-Refs im Index hat.
	static void DispatchKill(EntityAI victim, string shooterUid, float distance, string weaponType, bool suppressed, string hitZone, bool victimIsPlayer, bool victimIsInfected, bool victimIsAnimal, string victimCategory) {
		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}
		if (!victim || shooterUid == "") {
			return;
		}
		if (DME_Tasks_ObjectiveIndex.GetInstance().CountRefs(shooterUid, EDME_Tasks_EventType.KILL) == 0) {
			return;
		}

		DME_Tasks_KillEvent evt = new DME_Tasks_KillEvent();
		evt.m_DME_PlayerUid = shooterUid;
		evt.m_DME_Position = victim.GetPosition();
		evt.m_DME_VictimType = victim.GetType();
		evt.m_DME_VictimIsPlayer = victimIsPlayer;
		evt.m_DME_VictimIsInfected = victimIsInfected;
		evt.m_DME_VictimIsAnimal = victimIsAnimal;
		evt.m_DME_VictimCategory = victimCategory;
		evt.m_DME_Distance = distance;
		evt.m_DME_WeaponType = weaponType;
		evt.m_DME_Suppressed = suppressed;
		evt.m_DME_HitZone = hitZone;
		DME_Tasks_KillClassifier.GetInstance().Classify(victim, evt);
		DME_Tasks_EventBus.GetInstance().Dispatch(evt);
	}
}

modded class PlayerBase {
	//! Last-Hit-Puffer (nur server-seitig befuellt; EnScript-Default-Init "" / 0.0 / false genuegt,
	//! da EEKilled den Puffer nur bei gesetzter Schuetzen-Uid liest).
	string m_DME_LastHitShooterUid;
	float m_DME_LastHitDistance;
	string m_DME_LastHitWeaponType;
	bool m_DME_LastHitSuppressed;
	string m_DME_LastHitZone;
	int m_DME_LastHitAtEpoch;

	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef) {
		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);

		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		//! Combat-Logout-Vertrag (DME_Tasks_ExpeditionService.ReportCombat): jeder Fremd-Treffer
		//! auf einen Spieler zaehlt als Kampfkontakt des OPFERS (Eigen-/Umgebungsschaden nicht).
		if (source && source != this) {
			PlayerIdentity combatIdentity = GetIdentity();
			if (combatIdentity) {
				DME_Tasks_ExpeditionService.GetInstance().ReportCombat(combatIdentity.GetId());
			}
		}

		m_DME_LastHitShooterUid = "";
		PlayerBase shooter = DME_Tasks_KillHookUtil.GetShooter(source);
		if (!shooter) {
			return;
		}
		PlayerIdentity shooterIdentity = shooter.GetIdentity();
		if (!shooterIdentity) {
			return;
		}

		m_DME_LastHitShooterUid = shooterIdentity.GetId();
		m_DME_LastHitDistance = vector.Distance(shooter.GetPosition(), GetPosition());
		m_DME_LastHitWeaponType = DME_Tasks_KillHookUtil.GetWeaponType(source, shooter);
		m_DME_LastHitSuppressed = DME_Tasks_KillHookUtil.IsSuppressed(source, shooter);
		m_DME_LastHitZone = dmgZone;
		m_DME_LastHitAtEpoch = DME_Tasks_TimeUtil.NowEpoch();
	}

	override void EEKilled(Object killer) {
		super.EEKilled(killer);

		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		//! Opfer-Pfad (CONTRACTS §6.2/§6.5): FailOnDeath-Quests des GESTORBENEN failen — immer,
		//! unabhaengig davon, ob der Kill einem Schuetzen gutgeschrieben wird.
		string victimUid = "";
		PlayerIdentity victimIdentity = GetIdentity();
		if (victimIdentity) {
			victimUid = victimIdentity.GetId();
		}
		if (victimUid != "") {
			DME_Tasks_QuestEngine.GetInstance().OnPlayerDied(victimUid);
		}

		//! Selbst-Kill zaehlt nicht als Kill-Fortschritt.
		if (m_DME_LastHitShooterUid != "" && m_DME_LastHitShooterUid == victimUid) {
			return;
		}

		//! Kill-Credit nur innerhalb des Zeitfensters nach dem letzten Spieler-Treffer.
		if (m_DME_LastHitShooterUid == "") {
			return;
		}
		int nowEpoch = DME_Tasks_TimeUtil.NowEpoch();
		if (nowEpoch - m_DME_LastHitAtEpoch > DME_Tasks_KillHookUtil.DME_KILL_CREDIT_WINDOW_SECONDS) {
			return;
		}

		DME_Tasks_KillHookUtil.DispatchKill(this, m_DME_LastHitShooterUid, m_DME_LastHitDistance, m_DME_LastHitWeaponType, m_DME_LastHitSuppressed, m_DME_LastHitZone, true, false, false, "PLAYER");
	}
}

modded class ZombieBase {
	string m_DME_LastHitShooterUid;
	float m_DME_LastHitDistance;
	string m_DME_LastHitWeaponType;
	bool m_DME_LastHitSuppressed;
	string m_DME_LastHitZone;
	int m_DME_LastHitAtEpoch;

	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef) {
		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);

		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		m_DME_LastHitShooterUid = "";
		PlayerBase shooter = DME_Tasks_KillHookUtil.GetShooter(source);
		if (!shooter) {
			return;
		}
		PlayerIdentity shooterIdentity = shooter.GetIdentity();
		if (!shooterIdentity) {
			return;
		}

		m_DME_LastHitShooterUid = shooterIdentity.GetId();
		m_DME_LastHitDistance = vector.Distance(shooter.GetPosition(), GetPosition());
		m_DME_LastHitWeaponType = DME_Tasks_KillHookUtil.GetWeaponType(source, shooter);
		m_DME_LastHitSuppressed = DME_Tasks_KillHookUtil.IsSuppressed(source, shooter);
		m_DME_LastHitZone = dmgZone;
		m_DME_LastHitAtEpoch = DME_Tasks_TimeUtil.NowEpoch();
	}

	override void EEKilled(Object killer) {
		super.EEKilled(killer);

		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		//! Kill-Credit nur innerhalb des Zeitfensters nach dem letzten Spieler-Treffer.
		if (m_DME_LastHitShooterUid == "") {
			return;
		}
		int nowEpoch = DME_Tasks_TimeUtil.NowEpoch();
		if (nowEpoch - m_DME_LastHitAtEpoch > DME_Tasks_KillHookUtil.DME_KILL_CREDIT_WINDOW_SECONDS) {
			return;
		}

		DME_Tasks_KillHookUtil.DispatchKill(this, m_DME_LastHitShooterUid, m_DME_LastHitDistance, m_DME_LastHitWeaponType, m_DME_LastHitSuppressed, m_DME_LastHitZone, false, true, false, "INFECTED");
	}
}

modded class AnimalBase {
	string m_DME_LastHitShooterUid;
	float m_DME_LastHitDistance;
	string m_DME_LastHitWeaponType;
	bool m_DME_LastHitSuppressed;
	string m_DME_LastHitZone;
	int m_DME_LastHitAtEpoch;

	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef) {
		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);

		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		m_DME_LastHitShooterUid = "";
		PlayerBase shooter = DME_Tasks_KillHookUtil.GetShooter(source);
		if (!shooter) {
			return;
		}
		PlayerIdentity shooterIdentity = shooter.GetIdentity();
		if (!shooterIdentity) {
			return;
		}

		m_DME_LastHitShooterUid = shooterIdentity.GetId();
		m_DME_LastHitDistance = vector.Distance(shooter.GetPosition(), GetPosition());
		m_DME_LastHitWeaponType = DME_Tasks_KillHookUtil.GetWeaponType(source, shooter);
		m_DME_LastHitSuppressed = DME_Tasks_KillHookUtil.IsSuppressed(source, shooter);
		m_DME_LastHitZone = dmgZone;
		m_DME_LastHitAtEpoch = DME_Tasks_TimeUtil.NowEpoch();
	}

	override void EEKilled(Object killer) {
		super.EEKilled(killer);

		if (!g_Game || !g_Game.IsDedicatedServer()) {
			return;
		}

		//! Kill-Credit nur innerhalb des Zeitfensters nach dem letzten Spieler-Treffer.
		if (m_DME_LastHitShooterUid == "") {
			return;
		}
		int nowEpoch = DME_Tasks_TimeUtil.NowEpoch();
		if (nowEpoch - m_DME_LastHitAtEpoch > DME_Tasks_KillHookUtil.DME_KILL_CREDIT_WINDOW_SECONDS) {
			return;
		}

		DME_Tasks_KillHookUtil.DispatchKill(this, m_DME_LastHitShooterUid, m_DME_LastHitDistance, m_DME_LastHitWeaponType, m_DME_LastHitSuppressed, m_DME_LastHitZone, false, false, true, "ANIMAL");
	}
}
