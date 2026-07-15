//! JSON-Datenmodelle (Keys = JSON, KEIN Prefix an den Membern).

//! Eine Loyalitaetsstufe eines Questgebers.
class DME_EQ_LoyaltyTier
{
	int Tier;
	string DisplayName;
	int RequiredReputation;                       //! Hardline-Reputation (0 wenn Hardline aus)
	ref array<int> RequiredCompletedQuestIDs;     //! alle muessen abgeschlossen sein
	ref array<string> UnlockedMarketCategories;   //! Market-Kategorie-Dateinamen, die diese Stufe freischaltet
	ref array<string> UnlockedMarketClassNames;   //! einzelne Item-Klassennamen, die diese Stufe freischaltet

	void DME_EQ_LoyaltyTier()
	{
		Tier = 0;
		DisplayName = "";
		RequiredReputation = 0;
		RequiredCompletedQuestIDs = new array<int>();
		UnlockedMarketCategories = new array<string>();
		UnlockedMarketClassNames = new array<string>();
	}
}

//! Loyalitaets-Definition eines Questgebers (NpcId = ExpansionQuestNPCData.ID; -1 = global/faction).
class DME_EQ_TraderLoyalty
{
	int NpcId;
	string Faction;
	ref array<ref DME_EQ_LoyaltyTier> Tiers;

	void DME_EQ_TraderLoyalty()
	{
		NpcId = -1;
		Faction = "";
		Tiers = new array<ref DME_EQ_LoyaltyTier>();
	}
}

//! Terje-Skill-XP-Belohnung bei Quest-Abschluss (nur wirksam mit TerjeSkills; siehe DME_EQ_Terje).
class DME_EQ_TerjeReward
{
	int QuestId;      //! ExpansionQuestConfig.ID
	string SkillId;   //! Terje-Skill-ID (CfgTerjeSkills, z.B. "hunt", "athl")
	int Xp;

	void DME_EQ_TerjeReward()
	{
		QuestId = -1;
		SkillId = "";
		Xp = 0;
	}
}

//! Terje-gebundene Quest: Mindest-Skill-Level, um die Quest zu sehen/starten.
class DME_EQ_TerjeRequirement
{
	int QuestId;
	string SkillId;
	int MinLevel;

	void DME_EQ_TerjeRequirement()
	{
		QuestId = -1;
		SkillId = "";
		MinLevel = 0;
	}
}

//! Wurzel-Config (LoyaltyConfig.json).
class DME_EQ_Config
{
	int Version;
	ref array<ref DME_EQ_TraderLoyalty> Traders;
	ref map<string, int> QuestMinTiers;  //! Quest-ID (als String) -> Mindest-Tier beim zugehoerigen Questgeber
	ref array<ref DME_EQ_TerjeReward> TerjeQuestRewards;
	ref array<ref DME_EQ_TerjeRequirement> TerjeQuestRequirements;
	bool EnableQuestTierGate;
	bool EnableMarketUnlocks;
	bool EnableTerjeIntegration;
	bool HideLockedMarketItems;          //! true = gesperrte Items ausblenden, false = anzeigen+sperren

	void DME_EQ_Config()
	{
		Version = 1;
		Traders = new array<ref DME_EQ_TraderLoyalty>();
		QuestMinTiers = new map<string, int>();
		TerjeQuestRewards = new array<ref DME_EQ_TerjeReward>();
		TerjeQuestRequirements = new array<ref DME_EQ_TerjeRequirement>();
		EnableQuestTierGate = true;
		EnableMarketUnlocks = true;
		EnableTerjeIntegration = true;
		HideLockedMarketItems = false;
	}
}
