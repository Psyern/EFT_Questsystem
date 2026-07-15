# DME Quest Editor

Ein visueller Node-Graph-Editor für **DME_Tasks**-Quests — im Stil des
[DayZ-Expansion Quest-Editors](https://dayzexpansion.com/tools/custom/quest-editor),
aber er erzeugt **unser** JSON-Format und läuft komplett im Browser ohne Server.

## Was er kann
- **Trader-** und **Quest-Nodes** auf einer pan-/zoombaren Leinwand.
- **Blaue Kante** (Trader → Quest) = „Trader bietet Quest an" (`quest.TraderId`).
- **Messing-Kante** (Quest → Quest, vom unteren Port ziehen) = Quest-Kette
  (`Prerequisites.RequiredCompletedQuests`) — mit Zyklus-Schutz.
- **Inspector** rechts: alle `QuestDef`-/`TraderDef`-Felder, Objectives (19 Typen),
  Rewards (Items + Anbauteile, Rivalen-Reputation), Voraussetzungen, Freischaltungen, Zonen.
- **Live-Validierung** (ERR/WARN): fehlende Trader, ungültige Prereqs, Zyklen, KILL ohne Filter,
  HANDOVER ohne passendes COLLECT, doppelte IDs u. a.
- **Import**: mehrere `.json` auf einmal oder ein vom Editor erzeugtes `.zip`.
- **Export**: **ZIP** in exakter Serverprofil-Struktur
  `DME_Tasks/Traders/*.json` + `DME_Tasks/Quests/*.json` — direkt ins `$profile` kopierbar;
  oder Einzeldatei je Node (Rechtsklick), oder Bundle-JSON-Ansicht (`</> JSON`).
- **Autospeichern** pro Browser-Session (localStorage), Undo/Redo, Auto-Layout.

## Nutzung

### A) Lokal (ohne Internet)
`index.html` doppelklicken — läuft direkt per `file://` in jedem modernen Browser.

### B) Online über GitHub Pages
1. Diese Dateien liegen unter `docs/quest-editor/` im Repo.
2. Im GitHub-Repo: **Settings → Pages → Build and deployment → Source: „Deploy from a branch"**,
   Branch **`main`**, Ordner **`/docs`**, speichern.
3. Nach ~1 Minute erreichbar unter:
   ```
   https://<user>.github.io/<repo>/quest-editor/
   ```
   (z. B. `https://psyern.github.io/EFT_Questsystem/quest-editor/`)

> Der Editor ist eine einzelne, in sich geschlossene HTML-Datei — keine Build-Schritte,
> keine externen Skripte/Fonts, keine Server-Komponente. Er lässt sich auch auf jedem
> beliebigen Static-Host oder Intranet ablegen.

## Workflow: Quests bauen → auf den Server
1. Im Editor Trader/Quests anlegen und verketten.
2. **Export ZIP** → `DME_Tasks_<datum>.zip`.
3. ZIP entpacken; Inhalt nach `<Serverprofil>/DME_Tasks/` kopieren
   (also `DME_Tasks/Traders/*.json` und `DME_Tasks/Quests/*.json`).
4. Server neu starten — `DME_Tasks_ConfigService.LoadAll` liest die Dateien.

Bestehende Serverdateien wieder in den Editor holen: **Import** → alle `.json` aus
`Traders/` und `Quests/` auf einmal auswählen.

## Tastenkürzel
`N` neue Quest · `T` neuer Trader · `F` einpassen · `L` Auto-Layout ·
`Space`+Ziehen verschieben · Mausrad Zoom · `Ctrl+Z/Y` Undo/Redo · `Ctrl+D` Duplizieren ·
`Entf` löschen · Rechtsklick Kontextmenü · `?` Hilfe.

## Hinweise / Grenzen
- **Koordinaten** in Zonen sind kartenabhängig — im Editor frei setzbar, aber selbst prüfen.
- **KILL mit `TargetCategory:"AI"`** zählt Kills nur mit AI-Mod + aktivem DME_Tasks-AI-Adapter;
  das Questsystem spawnt keine AI.
- **Item-Belohnungen** mit Mod-Klassennamen funktionieren nur, wenn die Mods auf dem Server laufen.
- Objectives sind (wie im DME_Tasks-Modell) **in die Quest eingebettet**, keine eigenen Nodes.
- Titel/Beschreibungen: führendes `#` = Stringtable-Key, sonst Klartext (verbatim angezeigt).

## Entwicklung / Rebuild
Die ausgelieferte `index.html` ist **generiert**. Die wartbaren Quellen liegen modular unter `src/`
(je ein `.css`/`.html`/`.js`-Modul, siehe `src/README.md`). Nach Änderungen an `src/` neu bauen:
```
python build.py
```
Das inlined `styles.css` + `shell.html` + die JS-Module (in Abhängigkeitsreihenfolge) zu einer einzigen
self-contained `index.html`. Dieser modulare Aufbau + `build.py` entstand aus dem Multi-Agent-Workflow
`build_dme_quest_editor.md` (Projekt-Root).

Details zur Analyse des Referenz-Editors und den Design-Entscheidungen:
[ANALYSIS_AND_DESIGN.md](ANALYSIS_AND_DESIGN.md).
