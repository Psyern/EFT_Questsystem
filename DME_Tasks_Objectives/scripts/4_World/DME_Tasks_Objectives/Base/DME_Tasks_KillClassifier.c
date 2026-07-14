//! Kill-Klassifikator-Seam (SPEC §7 Agent 18 + CONTRACTS §6.8) — Singleton im Objectives-PBO.
//! Der KillHook (DME_Tasks_KillHookUtil.DispatchKill) ruft Classify() NACH dem Setzen der
//! Basis-Felder und VOR EventBus.Dispatch. Default: no-op — die Basis-Flags (VictimIsPlayer/
//! VictimIsInfected/VictimIsAnimal, VictimCategory "PLAYER"/"INFECTED"/"ANIMAL") setzt weiterhin
//! ausschliesslich der KillHook.
//!
//! Welle-4-Integrationsadapter (AI/Boss im Integrations-PBO) liefern `modded class
//! DME_Tasks_KillClassifier` mit `override void Classify(...)` — super ZUERST (Kette!) — und
//! reichern das Event an (m_DME_VictimIsAI/m_DME_VictimCategory bzw. m_DME_BossId), OHNE
//! Klassen-Referenzen auf Fremd-Mods (nur String-Checks). Laeuft ausschliesslich server-seitig
//! (DispatchKill ist bereits IsDedicatedServer-geguarded).
class DME_Tasks_KillClassifier {
	private static ref DME_Tasks_KillClassifier s_DME_Instance;

	static DME_Tasks_KillClassifier GetInstance() {
		if (!s_DME_Instance) {
			s_DME_Instance = new DME_Tasks_KillClassifier();
		}
		return s_DME_Instance;
	}

	//! Default-Implementierung: bewusst no-op.
	void Classify(EntityAI victim, DME_Tasks_KillEvent evt) {
	}
}
