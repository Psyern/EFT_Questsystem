# DME Expansion Quest EFT — Add-On

Ein **Add-On für das DayZ-Expansion Quest-System**, das dessen UI im Tarkov/EFT-Stil umskinnt und ein
**Loyalitätsstufen-System** ergänzt, das mit dem **Expansion-Markt** handshaked: Loyalitätsstufen schalten
Waffen/Kategorien frei. Der Mod **ersetzt Expansion nicht** — er dockt per `modded class` an offizielle Seams an.

> **Status:** Code-vollständig, aber **nicht in-game kompiliert/getestet** (hier steht kein EnScript-Compiler /
> kein DayZ zur Verfügung). Vor dem Produktiveinsatz auf einem Testserver kompilieren und prüfen.

## Abhängigkeiten / Load-Order
Pflicht: Community Framework (CF) → DayZ-Expansion **Core** + **Quests** (+ Quests-GUI). Optional (per `#ifdef`
eingebunden, wenn vorhanden): Expansion **Market**, Expansion **Hardline** (für Reputation).

```
-mod=@CF;@DayZ-Expansion;@DayZ-Expansion-Quests;@DayZ-Expansion-Market;@DME_ExpansionQuestEFT
```
`@DME_ExpansionQuestEFT` **nach** Expansion laden. `requiredAddons`: `DayZExpansion_Core_Scripts`,
`DayZExpansion_Quests_Scripts`, `DayZExpansion_Quests_GUI` (Market/Hardline sind **kein** requiredAddon —
sie werden nur bei Anwesenheit einkompiliert, der Mod läuft auch ohne sie).

## Was der Mod tut
1. **UI-Reskin (lizenz-sauber):** DayZ-Expansion steht unter **CC BY-NC-ND** (NoDerivatives) — es werden
   **keine** Expansion-Layouts kopiert oder weiterverbreitet. Stattdessen färbt `modded ExpansionQuestMenu.OnShow`
   die vorhandenen Stock-Widgets zur Laufzeit auf unsere EFT/Tarkov-Palette um (Panels Gunmetal, Aktions-Buttons
   Messing) — nur über die auto-gebundenen Member der Basis, null-sicher. Vollflächige eigene Layouts (DME_Tasks-Stil)
   sind eine spätere In-Game-Iterationsarbeit (Expansions Menü erwartet ~35 typisierte Widgets per MVC-Auto-Binding;
   ein eigenes Layout muss dieses Skelett vollständig treffen).
2. **Loyalitätsstufen:** aus **Hardline-Reputation + abgeschlossenen Quests** zur Laufzeit berechnet (keine
   eigene Spieler-Persistenz). Konfiguriert in `$profile:DME_ExpansionQuestEFT/LoyaltyConfig.json`.
3. **Quest-Gating:** `modded MissionBaseWorld.Expansion_CanStartQuest` (hartes Start-Gate) +
   `modded ExpansionQuestModule.QuestDisplayConditions` (Sichtbarkeit) — Quest erscheint/startet erst ab dem
   in `QuestMinTiers` geforderten Tier beim Questgeber.
4. **Markt-Handshake** (`#ifdef EXPANSIONMODMARKET`): `modded ExpansionMarketModule.FindPriceOfPurchaseInternal`
   (server-autoritativ, `super` zuerst) blockiert den Kauf gesperrter Items. Loyalitätsstufe schaltet
   Kategorien/Klassen frei.
5. **Terje-Skill-Integration** (`#ifdef TERJE_SKILLS_MOD`): Quest-Abschluss gibt konfigurierte **Terje-Skill-XP**
   (`GetTerjeSkills().AddSkillExperience`), und **Terje-gebundene Quests** verlangen ein Mindest-Skill-Level
   (`GetTerjeSkills().GetSkillLevel`), um zu erscheinen/starten. Ohne TerjeSkills: XP-Vergabe ist ein No-op,
   Terje-Voraussetzungen werden ignoriert (Quest bleibt spielbar). Ist **kein** requiredAddon.

## Konfiguration (`LoyaltyConfig.json`)
Fehlt die Datei, wird beim ersten Serverstart eine **Beispiel-Config** angelegt (siehe
`_ServerProfile_Example/`). Schema:
- `Traders[]`: pro Questgeber `NpcId` (= `ExpansionQuestNPCData.ID`; **-1 = gilt für alle Questgeber**),
  optional `Faction`, und `Tiers[]`.
- `Tiers[]`: `Tier` (1..N), `DisplayName`, `RequiredReputation` (Hardline; 0 wenn Hardline aus),
  `RequiredCompletedQuestIDs[]` (alle müssen erledigt sein), `UnlockedMarketCategories[]`
  (Market-Kategorie-**Dateinamen**, z. B. `weapons_assault_rifles`), `UnlockedMarketClassNames[]` (einzelne Items).
- `QuestMinTiers`: `{ "<QuestID>": <MinTier> }` — Quest erscheint erst ab diesem Tier beim Questgeber.
- `TerjeQuestRewards[]`: `{ "QuestId": <int>, "SkillId": "<terje-skill-id>", "Xp": <int> }` — Skill-XP bei Abschluss.
- `TerjeQuestRequirements[]`: `{ "QuestId": <int>, "SkillId": "<terje-skill-id>", "MinLevel": <int> }` — Quest
  erscheint/startet erst ab diesem Terje-Skill-Level. `SkillId` = Terje-Skill-ID aus `CfgTerjeSkills`.
- Toggles: `EnableQuestTierGate`, `EnableMarketUnlocks`, `EnableTerjeIntegration`, `HideLockedMarketItems`.

**Freischalt-Regel:** Nur Kategorien/Klassen, die in **irgendeiner** Stufe genannt sind, gelten als „gesperrt".
Sie werden kaufbar, sobald der Spieler eine Stufe erreicht, die sie freischaltet. Alles Ungenannte ist immer frei.

### Anzupassen für deinen Server
- **`NpcId`** in `Traders[]` auf die echten `ExpansionQuestNPCData.ID`-Werte deiner Questgeber setzen
  (oder `-1` global lassen).
- **`UnlockedMarketCategories`** auf die Kategorie-Dateinamen deiner Expansion-Market-Setups mappen.

## Bekannte Grenzen (v1)
- **Server-Autorität ist vollständig** (Quest- + Markt-Gate). **Client-seitiges Ausblenden/Sperren** gesperrter
  Markt-Items (Vorschau) ist v1 **nicht** aktiv — der Server lehnt den Kauf ab (Fehlermeldung im Markt).
  Für client-seitige Vorschau müsste die Loyalty-Config zum Client gesynct werden (v2).
- **Loyalitätsstufe im Trader-Header** wird v1 nicht separat angezeigt (nur die vorhandene Expansion-
  Reputationsanzeige); dafür wäre ebenfalls Client-Sync nötig (v2).
- Der Reskin ist v1 nur eine **Laufzeit-Umfärbung** der Stock-UI (Palette). Volles EFT-Layout (Positionen,
  Chrome, Fortschrittsbalken) = eigenes, von Grund auf geschriebenes Layout, das Expansions ~35-Widget-Vertrag
  trifft — In-Game-Iterationsarbeit, bewusst nicht als (lizenzpflichtige) Layout-Kopie umgesetzt.

## Installation Kurzfassung
1. `LoyaltyConfig.json` aus `_ServerProfile_Example/DME_ExpansionQuestEFT/` nach
   `<Serverprofil>/DME_ExpansionQuestEFT/` kopieren (oder beim ersten Start automatisch anlegen lassen) und anpassen.
2. Mod hinter Expansion in `-mod` laden. Server starten.
