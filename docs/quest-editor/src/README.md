# Quest-Editor — modulare Quellen

Diese Dateien werden von `../build.py` zur ausgelieferten `../index.html` zusammengefügt.
**Nicht** `index.html` direkt editieren — hier ändern und `python ../build.py` laufen lassen.

## Module (Ladereihenfolge = Abhängigkeitsreihenfolge)
| Datei | Inhalt |
|-------|--------|
| `styles.css` | Design-System (Tokens, Layout, Nodes, Inspector, Panels, Modals) |
| `shell.html` | Body-Markup (Toolbar, Canvas, Inspector, Modals) — alle Element-IDs |
| `constants.js` | Enums, Farben, `OBJ_FIELDS`, `STORE_KEY` |
| `model.js` | Factories (`newTraderDef`/`newQuestDef`/`newObjective`…), `uid`/`clone`, Zugriffe |
| `state.js` | State/View/Selection, `$`/`el`/`toast`/`snap`, Persistenz, Undo/Redo, Transform |
| `render.js` | `renderAll`/`renderNode`/`renderEdges`, Ports, Bezier-Kanten, Measure |
| `interact.js` | Pan/Zoom/Drag/Connect (Pointer-Events), Keyboard, `wouldCycle` |
| `inspector.js` | dynamische Formulare (Trader/Quest/Objectives/Rewards/Prereqs/Unlocks) |
| `validate.js` | Validierungsregeln + Panel (`validationCache` wird in `state.js` deklariert) |
| `io.js` | Import/Export, ZIP-Writer (store+CRC32), ZIP-Reader, Code-View |
| `main.js` | Aktionen, Kontextmenü, Modals, Toolbar-Wiring, Seed, `init()` |

## Regeln
- Reines Vanilla-JS, ES2019, **keine** `import`/`export` (Module werden konkateniert), kein `"use strict"`
  in den Modulen (setzt `build.py` einmal an den Anfang des finalen `<script>`).
- Jedes Symbol wird in **genau einem** Modul deklariert (keine Doppel-Deklaration; siehe Contract §4.4 in
  `build_dme_quest_editor.md`). Cross-Modul-Aufrufe sind erlaubt (Laufzeit).
- Keine externen Ressourcen (kein CDN/Font/Remote) — offline-/CSP-fest.
