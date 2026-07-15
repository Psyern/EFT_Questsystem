//! Market-Handshake: Loyalitaetsstufen schalten Waffen/Kategorien frei (server-autoritativ).
//! Nur kompiliert, wenn das Expansion-Market-Mod geladen ist (Define aus DayZExpansion_Market_Preload).
//! WICHTIG: super MUSS zuerst aufgerufen werden — Expansion hat einen "called-flag"-Hard-Error-Guard,
//! der abbricht, wenn ein Override super uebergeht. Signatur EXAKT wie Basis (sonst kein Override).
#ifdef EXPANSIONMODMARKET
modded class ExpansionMarketModule
{
	override bool FindPriceOfPurchaseInternal(ExpansionMarketItem item, ExpansionMarketTraderZone zone, ExpansionMarketTrader trader, PlayerBase player, int amountWanted, inout int price, bool includeAttachments = true, inout ExpansionMarketResult result = ExpansionMarketResult.Success, out ExpansionMarketReserve reserved = NULL, inout map<string, int> removedStock = NULL, out TStringArray outOfStockList = NULL, int level = 0)
	{
		bool baseOk = super.FindPriceOfPurchaseInternal(item, zone, trader, player, amountWanted, price, includeAttachments, result, reserved, removedStock, outOfStockList, level);
		if (!baseOk)
			return false;

		//! Nur das eigentliche Item (level 0) gaten; Attachments/Varianten (level > 0) folgen dem Item.
		if (level != 0)
			return true;
		if (!item || !player)
			return true;

		string catName = "";
		if (item.Category)
			catName = item.Category.m_FileName;

		if (!DME_EQ_LoyaltyService.GetInstance().IsMarketItemUnlocked(player, item.ClassName, catName))
		{
			result = ExpansionMarketResult.FailedCannotBuy;
			return false;
		}
		return true;
	}
}
#endif
