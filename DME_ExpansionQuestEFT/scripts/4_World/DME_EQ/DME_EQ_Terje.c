//! Terje-Skill-Integration: XP bei Quest-Abschluss + Terje-gebundene Quests (Skill-Level als Voraussetzung).
//! Die Config-Auswertung ist immer kompiliert; die ECHTEN Terje-Aufrufe stehen unter #ifdef TERJE_SKILLS_MOD
//! (verifiziert: Define aus TerjeSkills/config.cpp; PlayerBase.GetTerjeSkills().AddSkillExperience/GetSkillLevel).
//! Ohne TerjeSkills: XP-Vergabe ist ein No-op, Terje-Voraussetzungen werden ignoriert (Quest NICHT blockiert).
class DME_EQ_Terje
{
	//! Bei Quest-Abschluss die konfigurierten Skill-XP gutschreiben.
	static void AwardQuestXp(PlayerBase player, int questId)
	{
		if (!player)
			return;
		DME_EQ_Config cfg = DME_EQ_LoyaltyService.GetInstance().GetConfig();
		if (!cfg.EnableTerjeIntegration)
			return;

		int i;
		for (i = 0; i < cfg.TerjeQuestRewards.Count(); i++)
		{
			DME_EQ_TerjeReward r = cfg.TerjeQuestRewards.Get(i);
			if (!r || r.QuestId != questId)
				continue;
			GiveSkillXp(player, r.SkillId, r.Xp);
		}
	}

	//! true, wenn der Spieler alle Terje-Skill-Voraussetzungen der Quest erfuellt.
	static bool MeetsRequirements(PlayerBase player, int questId)
	{
		DME_EQ_Config cfg = DME_EQ_LoyaltyService.GetInstance().GetConfig();
		if (!cfg.EnableTerjeIntegration)
			return true;
		if (!player)
			return true;

		int i;
		for (i = 0; i < cfg.TerjeQuestRequirements.Count(); i++)
		{
			DME_EQ_TerjeRequirement req = cfg.TerjeQuestRequirements.Get(i);
			if (!req || req.QuestId != questId)
				continue;
			if (!HasSkillLevel(player, req.SkillId, req.MinLevel))
				return false;
		}
		return true;
	}

	// ------------------------------------------------------------------
	// Terje-Bruecke (nur mit TerjeSkills aktiv)
	// ------------------------------------------------------------------

	private static void GiveSkillXp(PlayerBase player, string skillId, int xp)
	{
		if (skillId == "" || xp == 0)
			return;
		#ifdef TERJE_SKILLS_MOD
		if (player.GetTerjeSkills())
		{
			player.GetTerjeSkills().AddSkillExperience(skillId, xp);
			DME_EQ_Log.Info("Terje: +" + xp.ToString() + " XP auf Skill '" + skillId + "'");
		}
		#endif
	}

	private static bool HasSkillLevel(PlayerBase player, string skillId, int minLevel)
	{
		if (skillId == "" || minLevel <= 0)
			return true;
		#ifdef TERJE_SKILLS_MOD
		if (player.GetTerjeSkills())
			return player.GetTerjeSkills().GetSkillLevel(skillId) >= minLevel;
		return false;
		#else
		//! TerjeSkills nicht installiert -> Voraussetzung ignorieren (Quest nicht dauerhaft sperren).
		return true;
		#endif
	}
}
