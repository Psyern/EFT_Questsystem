class DME_Tasks_ObjectiveDef
{
	string ObjectiveId;
	string Type;
	int Amount;
	ref array<string> ClassNames;
	string TargetCategory;
	string ReferencesObjective;
	bool AllowPartialHandover;
	bool FoundInWorldRequired;
	ref array<string> AllowedOrigins;
	bool MustBeFirstOwner;
	bool AcquiredAfterQuestAccept;
	bool PlayerTransferAllowed;
	float MinimumDistance;
	float MaximumDistance;
	ref array<string> RequiredWeaponCategories;
	bool SuppressorRequired;
	ref array<string> RequiredZones;
	string HitZone;
	int FromHour;
	int ToHour;
	ref DME_Tasks_ZoneVolume Zone;
	string TraderId;
	bool MustBeAlive;
	string BossId;

	void DME_Tasks_ObjectiveDef()
	{
		ObjectiveId = "";
		Type = "";
		Amount = 1;
		ClassNames = new array<string>();
		TargetCategory = "";
		ReferencesObjective = "";
		AllowPartialHandover = false;
		FoundInWorldRequired = false;
		AllowedOrigins = new array<string>();
		MustBeFirstOwner = false;
		AcquiredAfterQuestAccept = false;
		PlayerTransferAllowed = true;
		MinimumDistance = -1.0;
		MaximumDistance = -1.0;
		RequiredWeaponCategories = new array<string>();
		SuppressorRequired = false;
		RequiredZones = new array<string>();
		HitZone = "";
		FromHour = -1;
		ToHour = -1;
		Zone = null;
		TraderId = "";
		MustBeAlive = false;
		BossId = "";
	}
}
