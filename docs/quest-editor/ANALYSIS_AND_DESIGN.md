# DME Quest Editor — Analyse des Referenz-Editors & Design

Ziel: ein visueller Online-Quest-Konfigurator für **DME_Tasks**, der wie der
[DayZ-Expansion Quest-Editor](https://dayzexpansion.com/tools/custom/quest-editor)
funktioniert, aber **unser** JSON-Format erzeugt und als self-contained HTML über
GitHub Pages abrufbar ist.

---

## 1. Analyse des Referenz-Editors (dayzexpansion.com/tools/custom/quest-editor)

Der Editor ist eine client-gerenderte SPA (kein server-seitiges HTML; Quelltext liefert nur die App-Shell).
Die folgende Analyse basiert auf den Referenz-Screenshots + dem bekannten Expansion-Quest-Format
(Questbeispiele/ + lokale Expansion-Quelle).

### 1.1 Grundkonzept
Ein **Node-Graph-Editor** ("visual node-graph editor for building Expansion quest chains with
draggable nodes, bezier connections, objectives, rewards, and ZIP import/export"). Nodes werden auf
einer unendlichen, pan-/zoombaren Leinwand platziert und über Bezier-Kanten verbunden.

### 1.2 Node-Typen
| Node | Farbe/Tag | Inhalt |
|------|-----------|--------|
| **NPC** | Cyan-Tag "NPC" | Questgeber (ExpansionQuestNPC…): Name, ClassName, Position, Fraktion. Validierung: "Position X and Z are 0" → ERR. |
| **Quest** | Titelkarte | Titel, Badges (GROUP, DAILY, AUTO, REP), Objective-Typ-Zeilen (TARGET/COLLECT/TRAVEL/AI PATROL/DELIVERY). |
| **Objective** (standalone) | Typfarbe | TARGET (rot), COLLECT (grün), TRAVEL (cyan), AI PATROL (orange), DELIVERY (orange). Eigene Kachel mit Kurztext. |
| **Script** | Lila-Tag "SCRIPT" | Hook wie "Award Faction Rep — On Quest Completion". |

Nodes können in **Gruppen-Containern** liegen ("Main Quest Chain", "Daily Quests") — kollabierbar, mit Zähler.

### 1.3 Kanten (Ports & Verbindungen)
Jede Node hat Ein-/Ausgangsports (Rauten/Kreise links/rechts, plus Bottom-Port). Kantenlabels:
- **Start / End** — verbinden eine Quest mit ihrer ersten/letzten Objective (Objective-Reihenfolge).
- **Follow-Up** — Kette Quest → Nachfolge-Quest (Expansion `FollowUpQuest`).
- **Turn-In** — Quest → Abgabe-NPC (`QuestTurnInIDs`).
- (Quest → Objective = "besteht aus"; NPC → Quest = "gibt Quest".)

### 1.4 Toolbar
Sparkles (Auto/AI), Undo, Redo, Copy, Delete, Auto-Layout, Zoom, Grid-Toggle, Filter, Download,
Upload, "</> Pro" (Code-/JSON-Ansicht), Clipboard, Keyboard-Hilfe, Fullscreen. Zoom-% + "Clear Session".

### 1.5 Properties-Panel (rechts, "QUEST PROPERTIES")
Zusammenfassung ("2 objectives · 2 rewards · → #3") + kollabierbare Sektionen:
**IDENTITY** (Title, Objective-Text kurz), **DESCRIPTIONS**, **FLAGS**, **REWARDS** (Player Selects,
Random Reward, Owner Only, Rand. Amount, Behavior-Dropdown, Reward-Items mit Amount), **QUEST ITEMS**,
**REPUTATION**, **FACTION**, **OTHER**, **OBJECTIVE ORDER**.

### 1.6 Validierung
Inline-Badges an Nodes: **ERR** (rot, z.B. "Position X and Z are 0"), **WARNING** (gelb, z.B.
"No AISpawn configured"), **TIP** (Hinweis). Blockiert nicht das Editieren.

### 1.7 Import/Export
"ZIP import/export" — bündelt die Expansion-Quest-Dateistruktur (Quests/, Objectives/, NPCs/).
Persistenz per Session (localStorage), "Clear Session".

### 1.8 Keyboard
`N: new`, `F: fit`, `L: layout`, `Space+drag: pan`, `Ctrl+C/V: copy/paste`, `Ctrl+D: duplicate`,
`?: all shortcuts`. Node-Tipps: "Drag from the bottom port to create follow-up quests. Tab to cycle."

---

## 2. Warum wir NICHT das Expansion-Format erzeugen

DME_Tasks hat ein **anderes Datenmodell** (kein 1:1-Nachbau des Expansion-Outputs):

| Expansion | DME_Tasks |
|-----------|-----------|
| NPC (ExpansionQuestNPC) + Position + Waypoints | **Trader** (`TraderId`, `DisplayName`, `Faction`, `LoyaltyLevels[]`) — reiner Questgeber, keine Weltposition |
| Objective = eigene Datei, per ID referenziert | **Objective** = **eingebettet** in `QuestDef.Objectives[]` (gehört genau zu einer Quest) |
| `FollowUpQuest` / `PreQuestIDs` | `Prerequisites.RequiredCompletedQuests[]` (Kette) |
| `QuestGiverIDs` / `QuestTurnInIDs` | `QuestDef.TraderId` (ein Trader gibt UND nimmt an; `RETURN_TO_TRADER`-Objective optional mit eigener `TraderId`) |
| `Rewards[]` (Items) + `ReputationReward` | `RewardDef` (`PlayerExperience`, `Currency`, `TraderReputation`, `Items[]`, `RivalReputation[]`, `SkillPoints`, `SeasonXp`) |
| AISpawn im Objective | KILL-Objective mit `TargetCategory:"AI"` — Spawns liefert der AI-Mod, nicht das Questsystem |

Deshalb übernehmen wir die **UX** (Node-Graph, Ports, Inspector, ZIP-Export, Validierung), aber die
**Nodes und der Export** sprechen DME_Tasks.

---

## 3. Design des DME Quest Editors

### 3.1 Node-Typen (reduziert, YAGNI)
- **Trader-Node** (Questgeber): `TraderId`, `DisplayName`, `Faction`, `LoyaltyLevels[]`.
- **Quest-Node**: alle `QuestDef`-Felder; Objectives als farbige Stencil-Chips **im** Node,
  Volleditierung im Inspector.
- Objectives sind **keine** eigenen Nodes (sie gehören genau zu einer Quest — separate Nodes
  brächten keinen Datenmodell-Vorteil, nur Komplexität). Bewusste Abweichung vom Referenz-Editor.

### 3.2 Kanten
- **Trader → Quest** (`quest.TraderId == trader.id`): "bietet an". Ziehen vom Trader-Ausgangsport auf
  eine Quest setzt deren `TraderId`.
- **Quest → Quest** (Prerequisite): Kante A→B bedeutet "B verlangt A abgeschlossen"
  (`B.Prerequisites.RequiredCompletedQuests += A`). Ziehen vom Bottom-Port erzeugt Kette.

### 3.3 Interaktion
Pan (Space+Drag / Leinwand-Drag / Mittelklick), Zoom (Wheel, zum Cursor), Node-Drag (Header),
Select→Inspector, Connect (Port→Port), Delete, Kontextmenü (Rechtsklick), Undo/Redo (Snapshot-Stack),
Auto-Layout (topologisch nach Prerequisites, Trader über ihren Quests).

### 3.4 Import
- **Mehrere JSON-Dateien** auf einmal (`<input multiple>`) — deckt "alle Dateien aus Traders/ + Quests/"
  ab, ohne ZIP-Parser.
- **ZIP** (store-Methode), den der Editor selbst erzeugt hat — leicht lesbar ohne inflate-Implementierung.
- **Einzel-JSON einfügen** (Paste).

### 3.5 Export
- **ZIP** mit exakter Server-Profilstruktur `DME_Tasks/Traders/*.json` + `DME_Tasks/Quests/*.json`
  (eigener minimaler ZIP-Writer: store + CRC32) → direkt ins Serverprofil kopierbar.
- **Einzeldatei-Download** je Node.
- **Bundle-JSON** (alles in einer Datei) für schnelles Re-Import.

### 3.6 Validierung (ERR/WARN-Badges + Panel)
Doppelte QuestId/TraderId, `TraderId` zeigt auf fehlenden Trader, Prerequisite → fehlende Quest,
**Prerequisite-Zyklus**, Quest ohne Objectives, KILL/BOSS ohne Filter (TargetCategory/ClassNames/BossId),
HANDOVER ohne passendes COLLECT (`ReferencesObjective`), `RequiredZones` ohne passende `Zone`,
leere IDs/Titel, Trader ohne LoyaltyLevels (Warn). (Spiegelt die bereits vorhandene Python-Validierung.)

### 3.7 Technik
**Ein self-contained `index.html`** (Vanilla JS + SVG-Kanten + HTML-Nodes). Kein Build-Step, keine
CDN/Fonts (offline-fähig, CSP-freundlich), hand-editierbar, direkt über GitHub Pages abrufbar und
lokal per `file://` nutzbar. Persistenz per localStorage.

### 3.8 Aesthetik
Tarkov/Militär: Gunmetal-Void (`#0E1013`), Panels (`#16191E`), Hairlines (`#2A2F37`), warmes
Off-White (`#D6D3CB`), **Messing-Amber-Signatur** (`#C9A227`) sparsam für Auswahl/Primäraktionen.
Objective-Typfarben funktional (KILL rot, COLLECT grün, TRAVEL cyan …). Mono-Stencil-Labels
(uppercase, letter-spacing) als Node-Tags und Inspector-Header. Subtile Grid-Textur auf der Leinwand
(taktische Karte). Nodes = "Task-Karten" mit farbiger Kategorie-Kante.

### 3.9 Hosting
Ablage: `docs/quest-editor/index.html` im Mod-Repo. GitHub Pages Source = `/docs` →
`https://<user>.github.io/<repo>/quest-editor/`. Funktioniert auch offline per Doppelklick (file://).

---

## 4. Feld-Abdeckung (DME_Tasks-Schema, vollständig)
- **TraderDef**: Version, TraderId, DisplayName, Faction (NEUTRAL/WEST/EAST/BANDITS/MONSTERS/TRADE_UNION),
  LoyaltyLevels[] (Level, RequiredPlayerLevel, RequiredReputation, RequiredTurnover).
- **QuestDef**: Version, QuestId, TraderId, Category (STORY/SIDE/FACTION/BOSS/EXPEDITION/DAILY/WEEKLY),
  Title, Description, Repeatable, FailOnDeath, FailOnFactionChange, TimeLimit, Prerequisites,
  Objectives[], Rewards, Unlocks, Choices[].
- **ObjectiveDef**: ObjectiveId, Type (19 Typen), Amount, ClassNames[], TargetCategory, ReferencesObjective,
  AllowPartialHandover, FoundInWorldRequired, AllowedOrigins[], MustBeFirstOwner, AcquiredAfterQuestAccept,
  PlayerTransferAllowed, MinimumDistance, MaximumDistance, RequiredWeaponCategories[], SuppressorRequired,
  RequiredZones[], HitZone, FromHour, ToHour, Zone (ZoneVolume), TraderId, MustBeAlive, BossId.
- **Prereqs**: MinimumPlayerLevel, RequiredFaction, MinimumTraderReputation, RequiredTraderLevel,
  RequiredCompletedQuests[], BlockedByCompletedQuests[], RequiredDecisions[], BlockedByDecisions[],
  RequiredSkills[], RequiredSeasonLevel, RequiredBossProgress, RequiredItem, FromHour, ToHour.
- **RewardDef**: PlayerExperience, Currency, TraderReputation, RivalReputation[], Items[], SkillPoints, SeasonXp.
- **UnlockDef**: QuestIds[], MarketItems[], Keys[], BossAccess.
- **ChoiceDef**: ChoiceId, DisplayName, Rewards, Unlocks, ReputationEffects[].
