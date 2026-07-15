//! Quest-Gating: haengt Loyalitaetsstufen an Expansions offizielle Seams. super IMMER zuerst,
//! Ergebnis respektiert (nur zusaetzlich verschaerfen). Laeuft server-autoritativ.

modded class MissionBaseWorld
{
	//! Hartes Start-Gate: Quest kann nur angenommen werden, wenn Mindest-Tier UND Terje-Skill-Level erreicht sind.
	override bool Expansion_CanStartQuest(ExpansionQuestConfig questConfig, PlayerIdentity identity)
	{
		if (!super.Expansion_CanStartQuest(questConfig, identity))
			return false;
		if (!identity)
			return true;
		PlayerBase player = PlayerBase.ExpansionGetPlayerByIdentity(identity);
		if (!player)
			return true;
		if (!DME_EQ_LoyaltyService.GetInstance().MeetsQuestTier(player, questConfig, -1))
			return false;
		if (!DME_EQ_Terje.MeetsRequirements(player, questConfig.GetID()))
			return false;
		return true;
	}

	//! Nach Turn-In: konfigurierte Terje-Skill-XP gutschreiben (No-op ohne TerjeSkills).
	override void Expansion_OnQuestCompletion(ExpansionQuest quest)
	{
		super.Expansion_OnQuestCompletion(quest);
		if (!quest)
			return;
		ExpansionQuestConfig cfg = quest.GetQuestConfig();
		PlayerBase player = quest.GetPlayer();
		if (!cfg || !player)
			return;
		DME_EQ_Terje.AwardQuestXp(player, cfg.GetID());
	}
}

modded class ExpansionQuestModule
{
	//! Sichtbarkeits-Gate: Quest erscheint beim Questgeber erst ab dem geforderten Mindest-Tier.
	//! Signatur EXAKT wie Basis (sonst kein Override). Auf dem Client ist die Add-On-Config leer ->
	//! MeetsQuestTier liefert true (kein Client-Gate); die Autoritaet liegt server-seitig (CanStartQuest
	//! + server-seitiger Aufruf dieser Methode beim Listenaufbau).
	override bool QuestDisplayConditions(ExpansionQuestConfig config, PlayerBase player, ExpansionQuestPersistentData playerQuestData = null, int questNPCID = -1, bool displayQuestsWithCooldown = false, bool skipPreQuestCheck = false)
	{
		if (!super.QuestDisplayConditions(config, player, playerQuestData, questNPCID, displayQuestsWithCooldown, skipPreQuestCheck))
			return false;
		if (!DME_EQ_LoyaltyService.GetInstance().MeetsQuestTier(player, config, questNPCID))
			return false;
		if (config && !DME_EQ_Terje.MeetsRequirements(player, config.GetID()))
			return false;
		return true;
	}
}
