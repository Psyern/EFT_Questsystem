//! Server-autoritativer Loyalitaets-Service: Tier-Berechnung aus Hardline-Reputation + abgeschlossenen
//! Quests, Market-Freischaltung. KEINE eigene Spieler-Persistenz (alles aus Expansion/Hardline gelesen).
//! Config aus $profile:DME_ExpansionQuestEFT/LoyaltyConfig.json (fehlt -> Beispiel-Defaults schreiben).
class DME_EQ_LoyaltyService
{
	private static ref DME_EQ_LoyaltyService s_DME_EQ_Instance;

	private ref DME_EQ_Config m_DME_EQ_Config;
	private bool m_DME_EQ_Loaded;
	private ref array<string> m_DME_EQ_GatedClasses;
	private ref array<string> m_DME_EQ_GatedCategories;

	static DME_EQ_LoyaltyService GetInstance()
	{
		if (!s_DME_EQ_Instance)
			s_DME_EQ_Instance = new DME_EQ_LoyaltyService();
		return s_DME_EQ_Instance;
	}

	void DME_EQ_LoyaltyService()
	{
		m_DME_EQ_Config = new DME_EQ_Config();
		m_DME_EQ_Loaded = false;
		m_DME_EQ_GatedClasses = new array<string>();
		m_DME_EQ_GatedCategories = new array<string>();
	}

	DME_EQ_Config GetConfig()
	{
		EnsureLoaded();
		return m_DME_EQ_Config;
	}

	// ------------------------------------------------------------------
	// Laden / Seed
	// ------------------------------------------------------------------
	void EnsureLoaded()
	{
		if (m_DME_EQ_Loaded)
			return;
		m_DME_EQ_Loaded = true;

		if (!g_Game || !g_Game.IsDedicatedServer())
			return;

		if (!FileExist(DME_EQ_Const.PROFILE_DIR))
			MakeDirectory(DME_EQ_Const.PROFILE_DIR);

		if (FileExist(DME_EQ_Const.CONFIG_FILE))
		{
			DME_EQ_Config loaded = new DME_EQ_Config();
			bool ok = ExpansionJsonFileParser<DME_EQ_Config>.Load(DME_EQ_Const.CONFIG_FILE, loaded);
			if (ok && loaded)
			{
				m_DME_EQ_Config = loaded;
				DME_EQ_Log.Info("LoyaltyConfig geladen: " + m_DME_EQ_Config.Traders.Count().ToString() + " Questgeber");
			}
			else
			{
				DME_EQ_Log.Warn("LoyaltyConfig.json unlesbar - Defaults aktiv");
			}
		}
		else
		{
			m_DME_EQ_Config = BuildExampleConfig();
			ExpansionJsonFileParser<DME_EQ_Config>.Save(DME_EQ_Const.CONFIG_FILE, m_DME_EQ_Config);
			DME_EQ_Log.Info("LoyaltyConfig.json mit Beispiel-Defaults angelegt");
		}

		BuildGatedSets();
	}

	//! Sammelt alle Klassennamen/Kategorien, die IRGENDEINE Stufe erwaehnt (= "gated").
	private void BuildGatedSets()
	{
		m_DME_EQ_GatedClasses.Clear();
		m_DME_EQ_GatedCategories.Clear();
		int i;
		int j;
		int k;
		for (i = 0; i < m_DME_EQ_Config.Traders.Count(); i++)
		{
			DME_EQ_TraderLoyalty tl = m_DME_EQ_Config.Traders.Get(i);
			if (!tl)
				continue;
			for (j = 0; j < tl.Tiers.Count(); j++)
			{
				DME_EQ_LoyaltyTier tier = tl.Tiers.Get(j);
				if (!tier)
					continue;
				for (k = 0; k < tier.UnlockedMarketClassNames.Count(); k++)
				{
					string cn = tier.UnlockedMarketClassNames.Get(k);
					if (m_DME_EQ_GatedClasses.Find(cn) == -1)
						m_DME_EQ_GatedClasses.Insert(cn);
				}
				for (k = 0; k < tier.UnlockedMarketCategories.Count(); k++)
				{
					string cat = tier.UnlockedMarketCategories.Get(k);
					if (m_DME_EQ_GatedCategories.Find(cat) == -1)
						m_DME_EQ_GatedCategories.Insert(cat);
				}
			}
		}
	}

	// ------------------------------------------------------------------
	// Tier-Berechnung
	// ------------------------------------------------------------------

	//! Hoechster erreichter Tier eines Spielers beim Questgeber npcId (NpcId==-1 in der Config gilt fuer alle).
	int GetTierForNpc(PlayerBase player, int npcId)
	{
		EnsureLoaded();
		if (!player)
			return 0;

		int rep = GetPlayerReputation(player);
		string uid = "";
		if (player.GetIdentity())
			uid = player.GetIdentity().GetId();

		int best = 0;
		int i;
		int j;
		for (i = 0; i < m_DME_EQ_Config.Traders.Count(); i++)
		{
			DME_EQ_TraderLoyalty tl = m_DME_EQ_Config.Traders.Get(i);
			if (!tl)
				continue;
			if (tl.NpcId != npcId && tl.NpcId != -1)
				continue;
			for (j = 0; j < tl.Tiers.Count(); j++)
			{
				DME_EQ_LoyaltyTier tier = tl.Tiers.Get(j);
				if (!tier)
					continue;
				if (!MeetsTierRequirements(uid, rep, tier))
					continue;
				if (tier.Tier > best)
					best = tier.Tier;
			}
		}
		return best;
	}

	private bool MeetsTierRequirements(string uid, int rep, DME_EQ_LoyaltyTier tier)
	{
		if (rep < tier.RequiredReputation)
			return false;
		int q;
		for (q = 0; q < tier.RequiredCompletedQuestIDs.Count(); q++)
		{
			int qid = tier.RequiredCompletedQuestIDs.Get(q);
			if (!PlayerCompletedQuest(uid, qid))
				return false;
		}
		return true;
	}

	private int GetPlayerReputation(PlayerBase player)
	{
		int rep = 0;
		#ifdef EXPANSIONMODHARDLINE
		rep = player.Expansion_GetReputation();
		#endif
		return rep;
	}

	private bool PlayerCompletedQuest(string uid, int questID)
	{
		if (uid == "")
			return false;
		ExpansionQuestModule qm = ExpansionQuestModule.GetModuleInstance();
		if (!qm)
			return false;
		return qm.HasCompletedQuest(questID, uid);
	}

	// ------------------------------------------------------------------
	// Quest-Tier-Gate
	// ------------------------------------------------------------------

	//! true, wenn der Spieler den fuer die Quest geforderten Mindest-Tier beim Questgeber erreicht.
	bool MeetsQuestTier(PlayerBase player, ExpansionQuestConfig config, int questNPCID)
	{
		EnsureLoaded();
		if (!m_DME_EQ_Config.EnableQuestTierGate)
			return true;
		if (!config)
			return true;

		string key = config.GetID().ToString();
		int minTier = 0;
		if (m_DME_EQ_Config.QuestMinTiers.Contains(key))
			minTier = m_DME_EQ_Config.QuestMinTiers.Get(key);
		if (minTier <= 0)
			return true;

		int npcId = questNPCID;
		if (npcId <= -1)
		{
			array<int> givers = config.GetQuestGiverIDs();
			if (givers && givers.Count() > 0)
				npcId = givers.Get(0);
		}
		return GetTierForNpc(player, npcId) >= minTier;
	}

	// ------------------------------------------------------------------
	// Market-Freischaltung
	// ------------------------------------------------------------------

	//! true = kaufbar. Nicht in einer Stufe erwaehnte Items/Kategorien sind IMMER frei. Erwaehnte
	//! (gated) sind nur frei, wenn der Spieler eine Stufe erreicht hat, die sie freischaltet.
	bool IsMarketItemUnlocked(PlayerBase player, string className, string categoryName)
	{
		EnsureLoaded();
		if (!m_DME_EQ_Config.EnableMarketUnlocks)
			return true;

		bool gated = m_DME_EQ_GatedClasses.Find(className) > -1;
		if (!gated && categoryName != "" && m_DME_EQ_GatedCategories.Find(categoryName) > -1)
			gated = true;
		if (!gated)
			return true;

		if (!player)
			return false;

		int rep = GetPlayerReputation(player);
		string uid = "";
		if (player.GetIdentity())
			uid = player.GetIdentity().GetId();

		int i;
		int j;
		for (i = 0; i < m_DME_EQ_Config.Traders.Count(); i++)
		{
			DME_EQ_TraderLoyalty tl = m_DME_EQ_Config.Traders.Get(i);
			if (!tl)
				continue;
			for (j = 0; j < tl.Tiers.Count(); j++)
			{
				DME_EQ_LoyaltyTier tier = tl.Tiers.Get(j);
				if (!tier)
					continue;
				if (!MeetsTierRequirements(uid, rep, tier))
					continue;
				if (tier.UnlockedMarketClassNames.Find(className) > -1)
					return true;
				if (categoryName != "" && tier.UnlockedMarketCategories.Find(categoryName) > -1)
					return true;
			}
		}
		return false;
	}

	// ------------------------------------------------------------------
	// Beispiel-Default (identisch zur _ServerProfile_Example-Vorlage)
	// ------------------------------------------------------------------
	private DME_EQ_Config BuildExampleConfig()
	{
		DME_EQ_Config cfg = new DME_EQ_Config();
		//! Ein globaler Eintrag (NpcId -1 = gilt fuer alle Questgeber). Admins ersetzen durch echte NPC-IDs.
		DME_EQ_TraderLoyalty tl = new DME_EQ_TraderLoyalty();
		tl.NpcId = -1;
		tl.Faction = "";

		DME_EQ_LoyaltyTier t1 = new DME_EQ_LoyaltyTier();
		t1.Tier = 1;
		t1.DisplayName = "Rekrut";
		t1.RequiredReputation = 0;
		tl.Tiers.Insert(t1);

		DME_EQ_LoyaltyTier t2 = new DME_EQ_LoyaltyTier();
		t2.Tier = 2;
		t2.DisplayName = "Soldat";
		t2.RequiredReputation = 500;
		t2.UnlockedMarketCategories.Insert("weapons_assault_rifles");
		tl.Tiers.Insert(t2);

		DME_EQ_LoyaltyTier t3 = new DME_EQ_LoyaltyTier();
		t3.Tier = 3;
		t3.DisplayName = "Veteran";
		t3.RequiredReputation = 2000;
		t3.UnlockedMarketCategories.Insert("weapons_sniper_rifles");
		t3.UnlockedMarketClassNames.Insert("M79");
		tl.Tiers.Insert(t3);

		cfg.Traders.Insert(tl);

		//! Terje-Beispiele (nur wirksam mit TerjeSkills). QuestId/SkillId an den eigenen Server anpassen.
		DME_EQ_TerjeReward tr = new DME_EQ_TerjeReward();
		tr.QuestId = 1;
		tr.SkillId = "athl";
		tr.Xp = 50;
		cfg.TerjeQuestRewards.Insert(tr);

		DME_EQ_TerjeRequirement treq = new DME_EQ_TerjeRequirement();
		treq.QuestId = 2;
		treq.SkillId = "athl";
		treq.MinLevel = 2;
		cfg.TerjeQuestRequirements.Insert(treq);

		return cfg;
	}
}
