# DME Tarkov Tasks

Tarkov-inspiriertes Quest-System für DayZ (1.29+): Händler mit Reputations- und Loyalitätsstufen, verkettete Story-Quests, 19 Objective-Typen, Item-Herkunfts-Metadaten ("Found in World"), Expeditionen mit Extraktion, deterministische Daily-/Weekly-Quests, Entscheidungen mit Questlinien-Blockaden sowie optionale Integrations-Adapter — alles JSON-konfigurierbar, serverautoritativ und persistent.

- **Sprache/Engine:** Enforce Script, DayZ Enfusion
- **Pflicht-Abhängigkeit:** [Community Framework (CF / JM_CF_Scripts)](https://steamcommunity.com/sharedfiles/filedetails/?id=1559212036)
- **Autor:** Psyern
- **Community:** [Deadmans Echo](https://deadmansecho.com)

---

## 1. Installation

### 1.1 PBOs

Der Mod besteht aus **vier PBOs**:

| PBO | Inhalt | Pflicht |
|---|---|---|
| `DME_Tasks_Core` | Datenmodelle, Konfiguration, RPC, Quest-Engine, Persistenz, Rewards | Ja |
| `DME_Tasks_Objectives` | Objective-Handler, Event-Hooks (Kill/Item/Zone/Craft/Action), Origin-Metadaten | Ja |
| `DME_Tasks_UI` | Menü (F4), HUD-Tracker, Dialoge, Notifications, Keybind | Ja |
| `DME_Tasks_Integrations` | Optionale Adapter (Market/AI/Boss/Groups/Terje/Season/Website) | Optional |

### 1.2 Workshop / manuell

1. Community Framework abonnieren bzw. auf den Server kopieren.
2. Diesen Mod (alle gewünschten `DME_Tasks_*`-Ordner) in das Server-Verzeichnis kopieren (`@DME_Tasks_Core` usw. — je nach Paketierung auch als ein gemeinsamer `@DME_TarkovTasks`-Ordner mit allen PBOs unter `addons\`).
3. **Signaturen:** Die zum Signieren verwendete `.bikey` in den `keys\`-Ordner des Servers legen, sonst werden Clients bei `verifySignatures=2` gekickt.
4. Clients benötigen dieselben Mods (siehe 1.3).

### 1.3 `-mod` vs. `-serverMod` (WICHTIG)

**ALLE PBOs gehören in `-mod` — NICHT in `-serverMod`.**

`DME_Tasks_UI` enthält Client-Content (Layouts, Inputs, Menü) und `DME_Tasks_Core` client-seitige RPC-Handler/Modelle. Mods unter `-serverMod` werden Clients **nie** announced — der Client könnte weder das Menü öffnen noch Sync-Daten empfangen. Auch `DME_Tasks_Integrations` läuft unter `-mod` problemlos (alle Adapter sind server-guarded).

```text
-mod=@CF;@DME_Tasks_Core;@DME_Tasks_Objectives;@DME_Tasks_UI;@DME_Tasks_Integrations
```

### 1.4 Load-Order

Die Ladereihenfolge ist über `requiredAddons` abgesichert, sollte aber auch in der Modliste so stehen:

```text
JM_CF_Scripts (CF)  →  DME_Tasks_Core  →  DME_Tasks_Objectives  →  DME_Tasks_UI  →  optional DME_Tasks_Integrations
```

`DME_Tasks_Integrations` benötigt `DME_Tasks_Core` **und** `DME_Tasks_Objectives` (AI-/Boss-Adapter speisen den Objectives-EventBus).

---

## 2. Bedienung (Client)

- **Menü öffnen:** Standard-Taste **F4** (Input `UADMETasksMenu`). Umbelegbar im Spiel unter *Optionen → Tastenbelegung → Kategorie "DME Tasks"*.
- Menü: Händler-Kopf (Name, Reputation, Loyalitätsstufe), Quest-Liste nach Kategorien, Detailansicht mit Fortschrittsbalken, Belohnungen und Buttons (Annehmen / Abbrechen / Abgeben / Abschließen / Belohnung abholen / Daily ersetzen / Entscheidung treffen).
- **HUD-Tracker:** bis zu 3 verfolgte Quests (im Menü an-/abwählbar), Auswahl wird serverseitig persistiert.
- Notifications melden Fortschritt, Statuswechsel und Belohnungen.

---

## 3. Server-Profilstruktur (`$profile:DME_Tasks`)

Beim ersten Start legt der Server die komplette Struktur an. Ist der `Traders`-/`Quests`-Ordner leer, wird automatisch der **eingebettete MVP-Content** geschrieben (1 Händler + Questkette `west_001`–`west_006` + 2 Templates — identisch zur Vorlage in `_ServerProfile_Example\DME_Tasks`).

```text
$profile:DME_Tasks\
├── Settings.json               Globale Einstellungen (siehe 3.1)
├── Traders\<TraderId>.json     Händler-Definitionen
├── Quests\<QuestId>.json       Quest-Definitionen
├── DailyTemplates\*.json       Vorlagen für tägliche Quests
├── WeeklyTemplates\*.json      Vorlagen für wöchentliche Quests
├── Generated\                  Generierte Daily-/Weekly-Sets (restart-stabil, automatisch)
├── Players\<uid>.json          Spieler-Persistenz (automatisch)
├── Backups\                    Rotierende Spieler-Backups (automatisch)
├── Logs\                       reserviert
├── Integrations\               Configs/Exporte der Adapter (nur mit DME_Tasks_Integrations)
│   ├── Bosses.json             Klassenname → BossId Mapping (Boss-Adapter)
│   ├── AIClasses.json          KI-Basisklassen (AI-Adapter, Default ["eAIBase"])
│   ├── MarketUnlocks\<uid>.json  Export freigeschalteter Market-Items
│   └── PendingGrants_<uid>.json  Export offener Skill-/Season-Gutschriften
└── Website\<uid>.json          Spieler-Projektion für Web-Export (Website-Adapter)
```

### 3.1 Settings.json — alle Felder

| Feld | Default | Bedeutung |
|---|---|---|
| `Version` | 1 | Schema-Version, nicht ändern |
| `FlushIntervalSeconds` | 30.0 | Intervall des Coalescing-Flush geänderter Spielerstände (Sekunden) |
| `BackupSlots` | 3 | Anzahl rotierender Backups pro Spieler (Slot 0 = neuestes) |
| `DailyReplaceCost` | 5000 | Kosten für "Daily ersetzen" — wird via Currency-Invoker gemeldet (Durchsetzung braucht Market-/Currency-Brücke, siehe 6) |
| `DailyQuestCount` | 3 | Anzahl generierter Tagesquests |
| `WeeklyQuestCount` | 2 | Anzahl generierter Wochenquests |
| `EnableExpeditions` | true | Expedition-Sessions (EXTRACT-Quests, Combat-Logout-Fail) |
| `EnableDailyWeekly` | true | Daily-/Weekly-Generator ein/aus |
| `EnableIntegrationMarket` | false | Market-Adapter (Export-Brücke) |
| `EnableIntegrationAI` | false | AI-Adapter (Kill-Credit für eAI-KI) |
| `EnableIntegrationGroups` | false | Groups-Adapter (Erkennung + Anschlusspunkt) |
| `EnableIntegrationBoss` | false | Boss-Adapter (Bosses.json-Mapping) |
| `EnableIntegrationTerje` | false | Terje-Adapter (Skillpunkte-Export) |
| `EnableIntegrationSeason` | false | Season-Adapter (SeasonXP-Export) |
| `EnableIntegrationWebsite` | false | Website-Adapter (JSON-Export pro Spieler) |

Integrations-Toggles wirken nur, wenn zusätzlich das `DME_Tasks_Integrations`-PBO geladen ist und (bei Fremd-Mod-Adaptern) der Zielmod erkannt wurde.

---

## 4. Content erstellen

Alle JSON-Keys entsprechen **exakt** den Model-Feldern (case-sensitiv). Enum-artige Felder sind **UPPERCASE-Strings**. Unbekannte Werte werden geloggt und ignoriert. Dateiname = `<TraderId>.json` bzw. `<QuestId>.json`.

### 4.1 Händler (`Traders\<TraderId>.json`)

```json
{
	"Version": 1,
	"TraderId": "west_quartermaster",
	"DisplayName": "Quartiermeister Rask",
	"Faction": "WEST",
	"LoyaltyLevels": [
		{ "Level": 1, "RequiredPlayerLevel": 0,  "RequiredReputation": 0.0,  "RequiredTurnover": 0 },
		{ "Level": 2, "RequiredPlayerLevel": 10, "RequiredReputation": 0.2,  "RequiredTurnover": 250000 },
		{ "Level": 3, "RequiredPlayerLevel": 20, "RequiredReputation": 0.45, "RequiredTurnover": 1000000 }
	]
}
```

`Faction`: `NEUTRAL`, `WEST`, `EAST`, `BANDITS`, `MONSTERS`, `TRADE_UNION`.

### 4.2 Quest (`Quests\<QuestId>.json`)

Beispiel (gekürzt — vollständige Beispiele in `_ServerProfile_Example\DME_Tasks\Quests\`):

```json
{
	"Version": 1,
	"QuestId": "west_002",
	"TraderId": "west_quartermaster",
	"Category": "STORY",
	"Title": "Das Lager saeubern",
	"Description": "...",
	"Repeatable": false,
	"FailOnDeath": false,
	"FailOnFactionChange": false,
	"TimeLimit": -1,
	"Prerequisites": { "RequiredCompletedQuests": ["west_001"] },
	"Objectives": [
		{
			"ObjectiveId": "kill_infected_camp",
			"Type": "KILL",
			"Amount": 5,
			"TargetCategory": "INFECTED",
			"RequiredZones": ["mil_camp_west"],
			"Zone": { "ZoneId": "mil_camp_west", "CenterX": 2035.0, "CenterY": 290.0, "CenterZ": 7295.0, "Radius": 150.0, "Height": 120.0 }
		}
	],
	"Rewards": { "PlayerExperience": 2500, "Currency": 7500, "TraderReputation": 0.02 },
	"Unlocks": { "QuestIds": [], "MarketItems": [], "Keys": [], "BossAccess": "" },
	"Choices": []
}
```

- `Category`: `STORY`, `SIDE`, `FACTION`, `BOSS`, `EXPEDITION`, `DAILY`, `WEEKLY`.
- `TimeLimit`: Sekunden ab Annahme; `-1` = kein Limit. Wiederholbare Quests haben 3600 s Cooldown nach Abschluss.
- `Prerequisites`-Felder: `MinimumPlayerLevel`, `RequiredFaction`, `MinimumTraderReputation`, `RequiredTraderLevel`, `RequiredCompletedQuests[]`, `BlockedByCompletedQuests[]`, `RequiredDecisions[]`, `BlockedByDecisions[]` (Einträge im Format `"questId:choiceId"`), `RequiredSkills[{Skill,Level}]`, `RequiredSeasonLevel`, `RequiredBossProgress`, `RequiredItem`, `FromHour`/`ToHour`.
  - **Achtung `RequiredFaction`:** Ohne Fraktions-Provider haben alle Spieler die Fraktion `NEUTRAL` — eine Quest mit `RequiredFaction: "WEST"` bleibt dann gesperrt. Integrations können den `DME_Tasks_FactionService` modden.
  - Skill-/Season-/Boss-Prereqs sind Provider-Seams: ohne entsprechende Integration gelten sie als erfüllt (einmalige Warnung im Log).
- `Rewards`-Felder: `PlayerExperience` (10000 XP = 1 Level), `Currency` (nur via Currency-Brücke ausgezahlt, siehe 6), `TraderReputation`, `RivalReputation[{TraderId,Delta}]`, `Items[{ClassName,Amount,Attachments[]}]` (erhalten Origin `QUEST_REWARD`), `SkillPoints`, `SeasonXp`.
- `Unlocks`-Felder: `QuestIds[]` (gelistete Quests sind **unlock-gated** — erst nach Freischaltung verfügbar), `MarketItems[]`, `Keys[]`, `BossAccess`.
- `Choices[]` (Entscheidungen, Konzept §14): `{ "ChoiceId", "DisplayName", "Rewards", "Unlocks", "ReputationEffects": [{TraderId,Delta}] }`. Die gewählte Choice **ersetzt** die Quest-Rewards (ReputationEffects zusätzlich); die Entscheidung wird als `"questId:choiceId"` gespeichert und kann andere Questlinien via `RequiredDecisions`/`BlockedByDecisions` freischalten oder blockieren. Pro Quest ist nur EINE Entscheidung möglich.

### 4.3 Zonen

`Zone` ist ein Zylinder: `ZoneId`, `CenterX/CenterY/CenterZ` (Weltkoordinaten; `CenterY` = Höhe), `Radius` (Default 25 m, wenn <= 0), `Height` (Default 50 m, vertikal symmetrisch um `CenterY`). Die Beispielkoordinaten der MVP-Kette (`mil_camp_west`, X 2035 / Z 7295, Myshkino-Militärlager auf Chernarus) **vor Livegang an die eigene Karte anpassen**.

`RequiredZones` bei KILL-Objectives referenziert die `ZoneId` der **eigenen** `Zone` (Geometrie ist Pflicht — `RequiredZones` ohne `Zone` ist eine geloggte Fehlkonfiguration ohne Fortschritt). Bei TRAVEL/DISCOVER kann `RequiredZones` mehrere Zonen-Ids listen (`Amount` = Anzahl); jede Id braucht einen aktiven Zonen-Trigger (d. h. ein Objective mit gesetzter `Zone` gleicher Id).

### 4.4 Alle 19 Objective-Typen

| Typ | Zählweise | Relevante Filter-Felder |
|---|---|---|
| `KILL` | +1 pro passendem Kill | `ClassNames` (Opfer-Klasse, inkl. Config-Vererbung), `TargetCategory` (`INFECTED`/`PLAYER`/`ANIMAL`/`AI`), `BossId`, `MinimumDistance`/`MaximumDistance`, `RequiredWeaponCategories`, `SuppressorRequired`, `HitZone` (exakt, z. B. `"Head"`), `Zone`+`RequiredZones`, `FromHour`/`ToHour` |
| `BOSS` | wie KILL | wie KILL; `BossId` matcht das Boss-Adapter-Mapping. Ohne Boss-Mod: `ClassNames` als Fallback pflegen, sonst nie Fortschritt |
| `COLLECT` | Inventarbestand (absolut, live nachgezählt) | `ClassNames`, `Amount`, `FoundInWorldRequired`, `AllowedOrigins[]`, `MustBeFirstOwner`, `AcquiredAfterQuestAccept`, `PlayerTransferAllowed` |
| `HANDOVER` | +N pro Abgabe (Button im Menü) | `Amount`, `AllowPartialHandover`, `ReferencesObjective` (Match-Regeln des referenzierten COLLECT-Objectives **derselben Quest**) oder eigene `ClassNames`/Origin-Felder, optional `Zone` (Abgabeort) |
| `DELIVER` | wie HANDOVER, automatisch beim Betreten | wie HANDOVER; `Zone` ist Pflicht (Abgabe beim Zonen-Eintritt) |
| `TRAVEL` | +1 pro erstmals besuchter Zone | `Zone` (eine) oder `RequiredZones` (mehrere), `FromHour`/`ToHour` |
| `DISCOVER` | wie TRAVEL | wie TRAVEL |
| `MARK` | +1 pro passender Aktion | `ClassNames` (Item in der Hand), `TargetCategory` (Ziel-Typ), `Zone`, `FromHour`/`ToHour` |
| `STASH` | wie MARK | wie MARK |
| `INTERACT` | wie MARK | wie MARK |
| `USE_ITEM` | wie MARK | wie MARK |
| `SIGNAL` | wie MARK | wie MARK (`ClassNames` matcht zusätzlich den Ziel-Typ — platzierte Signal-Items) |
| `SURVIVE` | +1 pro überlebter Sekunde; Tod = Reset | `Amount` = Sekunden |
| `RETURN_TO_TRADER` | komplett bei Rückkehr | `TraderId`, `MustBeAlive`; mit `Zone`: Betreten der Händler-Zone; ohne `Zone`: automatisch beim "Abschließen"-Klick im Menü |
| `CRAFT` | +1 pro hergestelltem Ergebnis-Item | `ClassNames` (Ergebnis-Typ), `Zone`, `FromHour`/`ToHour` |
| `ESCORT` | — | Benötigt AI-Integration; ohne sie: einmalige Warnung, kein Fortschritt |
| `DEFEND` | +1 pro Sekunde in der Zone | `Zone` (Pflicht), `Amount` = Sekunden |
| `GROUP` | komplett, sobald Gruppengröße erreicht | `Amount` = Gruppengröße; benötigt Groups-Integration (GROUP_UPDATE-Events) |
| `EXTRACT` | komplett nach Extraktionszeit in der Zone | `Zone` (Extraktionspunkt), `Amount` = Extraktionszeit in Sekunden; Teil des Expeditions-Systems (Combat-Logout/Tod failen FailOnDeath-Quests) |

### 4.5 Item-Herkunft ("Found in World")

Jedes Item trägt persistente Metadaten (Origin-Typ, Erstbesitzer, Erwerbszeit, Transfer-Flag). Herkunfts-Strings für `AllowedOrigins`: `UNKNOWN`, `WORLD_LOOT`, `INFECTED_DROP`, `AI_DROP`, `BOSS_DROP`, `EVENT_REWARD`, `QUEST_REWARD`, `TRADER_PURCHASE`, `PLAYER_TRADE`, `ADMIN_SPAWN`, `CRAFTED`.

- `FoundInWorldRequired: true` akzeptiert nur `WORLD_LOOT`, `INFECTED_DROP`, `AI_DROP`, `BOSS_DROP`.
- Quest-Belohnungen werden automatisch als `QUEST_REWARD` gestempelt und damit von Found-in-World-Zielen abgelehnt; Craft-Ergebnisse als `CRAFTED`.
- **Ehrliche Grenze:** `TRADER_PURCHASE`/`ADMIN_SPAWN` werden nur gestempelt, wenn das Trader-/Admin-Tool die Seam-API aufruft (`DME_Tasks_OriginService.StampOrigin(...)` bzw. `item.DME_SetOrigin(...)`). Ohne diese Anbindung erhält ein direkt ins Inventar gespawntes Item beim ersten Inventar-Kontakt `WORLD_LOOT`.

### 4.6 Daily-/Weekly-Templates

`DailyTemplates\*.json` / `WeeklyTemplates\*.json`, Schema `DME_Tasks_TemplateDef`:

```json
{
	"Version": 1,
	"TemplateId": "daily_kill_infected",
	"TraderId": "west_quartermaster",
	"ObjectiveType": "KILL",
	"TargetTypes": ["INFECTED"],
	"MinimumAmount": 10,
	"MaximumAmount": 40,
	"AllowedZones": ["ANY", "MILITARY", "CITY"],
	"RewardTier": 2,
	"TimeLimit": 86400
}
```

Generator-Verhalten (deterministisch, **restart-stabil** über `Generated\<Datum>.json`):

- Seed aus Datum/Woche + TemplateId — nach einem Neustart entstehen exakt dieselben Quests.
- `TargetTypes`: Klassennamen → `ClassNames`; die Kategorie-Schlüsselwörter `INFECTED`/`PLAYER`/`ANIMAL`/`AI` werden bei KILL/BOSS zu `TargetCategory`.
- `Amount` wird zwischen `MinimumAmount` und `MaximumAmount` (inklusive) gewürfelt.
- `AllowedZones`: `"ANY"` = keine Zonen-Bedingung. Nur TRAVEL/DISCOVER erhalten die gewürfelte Zone als harten Filter (`RequiredZones`); bei allen anderen Typen ist sie rein informativ (Templates tragen keine Zonen-Geometrie).
- `Rewards`: Wenn im Template gesetzt, werden sie übernommen; sonst Tier-Formel `XP = 1000 × RewardTier`, `Currency = 5000 × RewardTier`, `Reputation = 0.01 × RewardTier`.
- **Daily ersetzen:** kostet `DailyReplaceCost` (via Currency-Invoker gemeldet), die ersetzte Quest erhält einen Cooldown bis Mitternacht UTC (persistiert), die Ersatzquest ist deterministisch pro Spieler.

---

## 5. MVP-Questkette (Seed)

| Quest | Typ | Inhalt | Voraussetzung |
|---|---|---|---|
| `west_001` | DISCOVER | Militärlager entdecken (Zone `mil_camp_west`) | — |
| `west_002` | KILL | 5 Infizierte **in der Lagerzone** | west_001 |
| `west_003` | COLLECT | 3 Munitionskisten 5,45×39, nur Found-in-World | west_002 |
| `west_004` | COLLECT + HANDOVER | Kisten abgeben (Teilabgabe erlaubt) | west_003 |
| `west_005` | KILL (BossId) + COLLECT + HANDOVER | Lagerkommandant + GPS-Beweisstück | west_004, Rep ≥ 0.05 |
| `west_006` | RETURN_TO_TRADER | Lebend melden → AK-74 + Market-Unlock `AK74` | west_005, Rep ≥ 0.08 |

Design-Hinweise:

- **west_004:** `ReferencesObjective` verweist immer auf ein Objective **derselben** Quest — deshalb trägt west_004 ein eigenes COLLECT-Objective. Bereits in west_003 gesammelte Kisten zählen beim Annehmen sofort (Initial-Nachzählung des Inventars).
- **west_005:** Ohne Boss-Mod greift der Klassennamen-Fallback (`ZmbM_PatrolNormal_Base` = alle Patrouillen-Infizierten, auch am Lager). Mit aktivem Boss-Adapter mappt `Integrations\Bosses.json` eine Boss-Klasse auf `camp_commander`; **dann zusätzlich den Boss-Klassennamen in `ClassNames` des Objectives eintragen** (oder `ClassNames` leeren), da der Klassenfilter vor dem BossId-Filter greift.
- Zonen-Koordinaten sind Chernarus-Beispiele (Myshkino) — für andere Karten anpassen.

---

## 6. Integrationen (optional, `DME_Tasks_Integrations`)

Kein Adapter referenziert Fremd-Mod-Klassen hart — Erkennung läuft über `CfgPatches`-Präsenz plus Settings-Toggle. Fehlt der Zielmod: einmaliges "idle"-Log, keine Aktivität, kein Crash. **Ehrliche Grenzen:** Die meisten Adapter sind Export-/Log-Brücken, keine Direktanbindungen.

| Adapter | Zielmod (CfgPatches) | Funktion | Grenze |
|---|---|---|---|
| Market | `DayZExpansion_Market` | Exportiert freigeschaltete Market-Items nach `Integrations\MarketUnlocks\<uid>.json`; loggt Currency-Beträge (Rewards, Daily-Replace-Kosten) | **Export-/Log-Brücke** — das tatsächliche Ein-/Ausblenden im Expansion-Market und die Geld-Buchung erfordern eine server-spezifische Anbindung, die diese JSONs/Invoker konsumiert |
| AI | `DayZExpansion_AI` | Kill-Credit für eAI: Opfer ohne Identity, Klassen-Match gegen `Integrations\AIClasses.json` (Default `["eAIBase"]`) → `VictimCategory "AI"` | Nur Kill-Klassifikation; ESCORT-Ziele brauchen weiterhin eigene Anbindung |
| Boss | — (kein Fremd-Mod nötig) | Mappt Opfer-Klassennamen via `Integrations\Bosses.json` auf `BossId` (Toggle `EnableIntegrationBoss`) | Mapping-Pflege liegt beim Admin; Standard-Datei wird mit leerem Mapping + Beispiel angelegt |
| Groups | `DayZExpansion_Groups` | Erkennung + dokumentierter Anschlusspunkt `DME_Tasks_GroupsAdapter.ReportGroupUpdate(uid, groupSize, position)` | Ohne eigene Brücke entstehen keine GROUP_UPDATE-Events — GROUP-Ziele bleiben ohne Fortschritt |
| Terje | `TerjeCore` | Loggt Skillpunkt-Rewards und exportiert sie nach `Integrations\PendingGrants_<uid>.json` (Typ `SKILL_POINTS`, letzte 50) | **Export-Brücke** — die Gutschrift ins Terje-Skillsystem muss ein server-eigenes Skript aus der JSON übernehmen |
| Season | `DME_SeasonPass` | Wie Terje, Typ `SEASON_XP` in derselben PendingGrants-Datei | Export-Brücke; CfgPatches-Name des Season-Mods ggf. anpassen |
| Website | — (kein Fremd-Mod nötig) | Schreibt bei Quest-Abschluss/Unlock-Änderung `Website\<uid>.json` (Level, XP, aktive/abgeschlossene Quests, Reputation, Unlocks, Entscheidungen) | Nur Datei-Export — Upload/Anzeige übernimmt die Website-Seite |

**Currency generell:** `Rewards.Currency` und `DailyReplaceCost` laufen ausschließlich über den Invoker `DME_Tasks_IntegrationEvents.s_DME_OnGrantCurrency` (uid, Betrag; negativ = Kosten). Ohne Abonnenten wird nur geloggt — es wird kein Geld gutgeschrieben oder abgezogen.

---

## 7. Persistenz & Sicherheit

- Spielerstände unter `Players\<uid>.json` (uid = gehashte DayZ-Id), atomares Speichern (tmp → Backup-Rotation → Copy), Coalescing-Flush, Flush bei Disconnect und Mission-Ende.
- Alle Quest-Transitionen, Fortschritte, Prereq-Checks und Belohnungen laufen **ausschließlich serverseitig**; die UI sendet nur Anfrage-RPCs.
- Belohnungen sind idempotent (Transaktions-Records) — Doppel-Claims per RPC-Spam zahlen nicht doppelt aus.
- Item-Metadaten überleben Server-Neustarts (OnStoreSave/OnStoreLoad mit Versions-Guard).

---

## 8. Troubleshooting

Alle Meldungen tragen das Präfix **`[DME_Tasks]`** im Server-Skriptlog (`DayZServer_*.RPT` bzw. `script*.log`).

| Symptom | Ursache / Lösung |
|---|---|
| Menü öffnet nicht (F4) | Mod in `-serverMod` statt `-mod`? Keybind umbelegt/kollidiert? CF geladen? |
| Client-Kick bei Join | `.bikey` fehlt im Server-`keys\`-Ordner (bei `verifySignatures=2`) |
| Keine Quests sichtbar | Log auf `Konfiguration geladen: X Trader, Y Quests` prüfen. 0 Quests → JSON-Parse-Fehler (`Quest-Datei uebersprungen ...` im Log, Datei + Zeile prüfen: Kommas, Anführungszeichen, exakte Key-Namen) |
| Quest bleibt gesperrt (LOCKED) | Prereqs prüfen: `RequiredFaction` ohne Fraktions-Provider sperrt dauerhaft (alle Spieler sind `NEUTRAL`); `Unlocks.QuestIds`-gelistete Quests brauchen erst die Freischaltung |
| Kill zählt nicht | `TargetCategory`/`ClassNames` prüfen (Kategorie gehört in `TargetCategory`, nicht in `ClassNames`); Zonen-Kill: Warn-Log `Zonen-Fehlkonfiguration` = `RequiredZones` ohne `Zone`-Geometrie |
| Items zählen nicht (COLLECT/HANDOVER) | Origin prüfen: `FoundInWorldRequired` lehnt gekaufte/gecraftete/Belohnungs-Items ab; Stack-Items zählen pro Item-Instanz, nicht pro Quantity |
| Daily-Quests fehlen | `EnableDailyWeekly` an? Templates vorhanden und gültig (`TraderId` gesetzt)? Log: `TaskGenerator: X Daily- und Y Weekly-Quests registriert` |
| Dailies nach Neustart anders | Darf nicht passieren — `Generated\<Datum>.json` gelöscht/beschädigt? (`Generated-Set unlesbar` im Log) |
| Currency kommt nicht an | Erwartet: nur Log-Meldung. Auszahlung braucht eine Currency-Brücke (siehe 6) |
| Integration "idle" | Log nennt den Grund: Zielmod nicht erkannt oder Settings-Toggle aus |
| RPC scheinbar ohne Wirkung | Load-Order prüfen (CF zuerst); Client- und Server-Modliste identisch? |

---

## 9. Abnahmetests

Siehe [`ACCEPTANCE_TESTS.md`](ACCEPTANCE_TESTS.md) — die 12 Testfragen des Konzepts (§24) plus Zusatztests als Checkliste mit konkreten Testschritten entlang der MVP-Kette.
