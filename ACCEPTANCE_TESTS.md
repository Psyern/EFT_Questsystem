# DME Tarkov Tasks — Abnahmetests (Konzept §24)

Checkliste der 12 Testfragen aus dem Modkonzept plus Zusatztests. Jeder Test nennt konkrete
Schritte entlang der MVP-Kette (`west_001`–`west_006`, siehe README §5) und das erwartete Ergebnis.

## Voraussetzungen (einmalig)

- [ ] Dedicated Server mit `-mod=@CF;@DME_Tasks_Core;@DME_Tasks_Objectives;@DME_Tasks_UI;@DME_Tasks_Integrations` (Load-Order siehe README §1.4), Karte **Chernarus** (Zonen-Koordinaten der Seed-Kette liegen am Myshkino-Militärlager, ca. X 2035 / Z 7295).
- [ ] Frischer Zustand: `$profile:DME_Tasks` löschen (oder umbenennen) → beim Start legt der Server Struktur + MVP-Content selbst an. RPT/Skriptlog prüfen: `[DME_Tasks] Default-Content angelegt: ...` (9 Dateien) und `Konfiguration geladen: 1 Trader, 6 Quests, 2 Templates` (plus registrierte Daily-/Weekly-Quests).
- [ ] 2 Test-Clients (T1, T2) mit identischer Modliste; Admin-Werkzeug zum Item-Spawnen und Teleportieren.
- [ ] Menü öffnet per **F4** (Input `UADMETasksMenu`).

---

## Die 12 Testfragen

### 1. Speichert der Fortschritt zuverlässig?

- [ ] **Schritte:** T1: F4 → `west_001` annehmen → zum Lager laufen/teleportieren (Zone `mil_camp_west`) → Fortschritt 1/1 (Notification + Tracker). `west_001` abschließen + Belohnung abholen. Danach `west_002` annehmen und **2 von 5** Infizierte in der Zone töten. T1 disconnecten und wieder verbinden.
- [ ] **Erwartet:** Nach Reconnect zeigt `west_002` weiterhin 2/5; `west_001` steht unter "Abgeschlossen". `$profile:DME_Tasks\Players\<uid>.json` enthält den Stand; Disconnect erzwingt einen Flush.

### 2. Funktionieren Serverneustarts?

- [ ] **Schritte:** Zustand aus Test 1 (west_002 bei 2/5). Server komplett stoppen und neu starten. T1 verbindet erneut.
- [ ] **Erwartet:** Fortschritt 2/5 unverändert, abgeschlossene Quests bleiben abgeschlossen, Reputation unverändert. Keine Fehler-/Migrationsmeldungen im Log außer regulärem Load.

### 3. Werden Kills korrekt zugeordnet?

- [ ] **Schritte:** T1 und T2 haben beide `west_002` aktiv und stehen gemeinsam in der Lagerzone. T1 tötet 2 Infizierte, T2 tötet 1 Infizierten.
- [ ] **Erwartet:** Nur der jeweilige Schütze erhält Fortschritt (T1 +2, T2 +1) — Kills des anderen zählen nicht. Kill mit Fahrzeug/Umgebung ohne Spieler-Schützen zählt für niemanden.

### 4. Zählen Waffen-, Distanz- und Zonenbedingungen?

- [ ] **Schritte (Zone, direkt in der Seed-Kette):** `west_002` aktiv: 1 Infizierten **außerhalb** der Zone töten (>150 m vom Zentrum), dann 1 **innerhalb**.
- [ ] **Erwartet:** Außerhalb kein Fortschritt, innerhalb +1.
- [ ] **Schritte (Waffe/Distanz/HitZone):** `Quests\west_002.json` temporär erweitern: `"MinimumDistance": 50.0`, `"RequiredWeaponCategories": ["Rifle_Base"]`, `"SuppressorRequired": true`, `"HitZone": "Head"` → Server-Neustart. Gegentests: Kill unter 50 m / mit Pistole / ohne Schalldämpfer / Körpertreffer; dann Positivtest: Kopfschuss mit schallgedämpftem Gewehr über 50 m in der Zone.
- [ ] **Erwartet:** Nur der Positivtest zählt. Danach Test-Änderungen zurücksetzen.

### 5. Können Items doppelt abgegeben werden?

- [ ] **Schritte:** Kette bis `west_004` spielen. 3 Munitionskisten im Inventar, im Quest-Detail "Abgeben" klicken; direkt danach erneut "Abgeben" klicken (und mehrfach spammen).
- [ ] **Erwartet:** Erste Abgabe entfernt genau 3 Kisten und setzt 3/3; jede weitere Abgabe meldet "Keine passenden Gegenstaende" bzw. bricht bei Done ab. Es wird nie mehr entfernt als der offene Rest (Teilabgabe: 2 Kisten abgeben → 2/3, dritte Kiste später → 3/3).

### 6. Kann eine Belohnung doppelt abgeholt werden?

- [ ] **Schritte:** `west_004` auf READY_TO_TURN_IN bringen → "Belohnung abholen" klicken und den Button/RPC unmittelbar mehrfach auslösen (Doppelklick). Danach Relog und erneuter Versuch über die abgeschlossene Quest.
- [ ] **Erwartet:** XP/Reputation/Items werden genau EINMAL gutgeschrieben (idempotente Transaktion `uid:questId:claim:<n>`); weitere Versuche sind wirkungslos. Log zeigt eine einzige Auszahlung.

### 7. Bleiben Item-Metadaten erhalten?

- [ ] **Schritte:** `west_003` aktiv: 1 Munitionskiste in der Welt looten (2/3 fehlen noch). Kiste im Inventar behalten, Server neu starten, wieder verbinden.
- [ ] **Erwartet:** Die Kiste zählt weiterhin (COLLECT zeigt nach Reconnect wieder 1/3) — Origin `WORLD_LOOT`, Erstbesitzer und Erwerbszeit überleben den Neustart (OnStoreSave/OnStoreLoad). Kein `Alt-Save ohne Origin-Daten`-Spam für neu gelootete Items.

### 8. Wird die nächste Quest freigeschaltet?

- [ ] **Schritte:** Vor Abschluss von `west_001` den Zustand von `west_002` im Menü prüfen (gesperrt, Sperrgrund sichtbar). `west_001` abschließen + Belohnung abholen.
- [ ] **Erwartet:** `west_002` wechselt ohne Relog auf VERFÜGBAR (Sync nach Claim). Analog entlang der ganzen Kette bis `west_006` (dort zusätzlich Reputations-Schwellen 0.05/0.08 — siehe Test 9).

### 9. Funktionieren Reputation und Händlerstufe?

- [ ] **Schritte:** Nach jedem Claim den Händler-Kopf im Menü prüfen (Reputation steigt: 0.02/0.02/0.03/0.03/0.04/0.05 entlang der Kette). `west_005` DARF vor Erreichen von 0.05 nicht verfügbar sein (nach west_004-Claim: Summe 0.10 → verfügbar).
- [ ] **Erwartet:** Reputationswerte exakt kumulativ; Loyalitätsstufe bleibt 1 (Stufe 2 braucht Level 10 + Rep 0.2 + Umsatz 250000 — Anzeige der Anforderungen korrekt).

### 10. Werden Admin-Items abgelehnt?

- [ ] **Schritte (Reward-Origin, reproduzierbar ohne Tool-Anbindung):** `Quests\west_002.json` temporär um `"Items": [{ "ClassName": "AmmoBox_545x39_20Rnd", "Amount": 1, "Attachments": [] }]` in `Rewards` ergänzen → Neustart → west_002 abschließen. Die Belohnungs-Kiste ist `QUEST_REWARD`. In `west_003` prüfen: Belohnungs-Kiste zählt NICHT, eine gelootete Kiste zählt.
- [ ] **Erwartet:** COLLECT bleibt bei 0/3, solange nur die Belohnungs-Kiste im Inventar liegt (FoundInWorldRequired + AllowedOrigins lehnen `QUEST_REWARD` ab). Test-Änderung danach zurücksetzen.
- [ ] **Dokumentierte Grenze:** Direkt per Admin-Tool ins Inventar gespawnte Items werden nur dann als `ADMIN_SPAWN` erkannt, wenn das Admin-Tool die Seam-API (`DME_Tasks_OriginService.StampOrigin(...)`) aufruft — ohne Anbindung stempelt der erste Inventar-Kontakt `WORLD_LOOT` (README §4.5). Mit angebundenem Tool: gespawnte Kiste zählt nicht.

### 11. Was passiert bei Tod und Disconnect?

- [ ] **Schritte (Tod):** `west_002` bei Teilfortschritt: T1 stirbt (FailOnDeath=false) und respawnt.
- [ ] **Erwartet:** Quest bleibt aktiv, Fortschritt bleibt erhalten (Kill-Zähler ist state-basiert). SURVIVE-Ziele würden zurückgesetzt (nicht Teil der Seed-Kette).
- [ ] **Schritte (Disconnect):** Beliebige aktive Quest, Disconnect mitten im Spiel, Reconnect.
- [ ] **Erwartet:** Zustand identisch (Flush beim Disconnect); keine Quest failt allein durch Ausloggen (Combat-Logout betrifft nur Expeditions-Quests mit FailOnDeath — siehe Zusatztest Z4).

### 12. Funktioniert alles in Gruppen?

- [ ] **Schritte:** T1 und T2 spielen die Kette parallel: beide nehmen `west_001`/`west_002` an, laufen gemeinsam, töten abwechselnd; bei `west_004` übergibt T2 eine seiner gelooteten Kisten an T1 (Bodendrop), T1 gibt sie ab.
- [ ] **Erwartet:** Fortschritt strikt pro Spieler (Zone-Discover zählt für beide beim Betreten, Kills nur für den Schützen). Die von T2 übernommene Kiste zählt für T1 (Origin bleibt `WORLD_LOOT`, `PlayerTransferAllowed` ist true; das Transfer-Flag würde nur bei `PlayerTransferAllowed:false` ablehnen). GROUP-Objectives sind ohne Groups-Integration bewusst ohne Fortschritt (einmalige Log-Warnung — dokumentiert).

---

## Zusatztests

### Z1. Restart-Stabilität der Daily-/Weekly-Quests

- [ ] **Schritte:** Nach Serverstart Titel und Mengen der generierten Dailies/Weeklies notieren (F4 → Kategorie DAILY/WEEKLY; z. B. "Tagesauftrag: 27x INFECTED toeten"). `Generated\<JJJJ-MM-TT>.json` ansehen. Server neu starten (am selben Tag).
- [ ] **Erwartet:** Exakt dieselben Quests (Ids `daily_<Datum>_<n>`, gleiche Mengen/Zonen) — geladen aus der Generated-Datei. Auch nach Löschen der Datei (Gegenprobe) entstehen durch den deterministischen Seed identische Quests.

### Z2. Doppel-Claim unter Last

- [ ] **Schritte:** Wie Test 6, zusätzlich: Claim auslösen und **sofort** disconnecten; reconnecten und erneut claimen.
- [ ] **Erwartet:** Auch über den Disconnect hinweg genau eine Auszahlung (Transaktions-Record ist persistiert, PENDING-Wiederanlauf zahlt fehlende Teile höchstens einmal nach).

### Z3. AllowedOrigins-Mechanik (Verschärfung)

- [ ] **Schritte:** `Quests\west_003.json`: `AllowedOrigins` temporär auf `["INFECTED_DROP"]` reduzieren → Neustart. Eine in der Welt gelootete Kiste (Origin `WORLD_LOOT`) aufnehmen.
- [ ] **Erwartet:** Kiste zählt NICHT mehr (0/3) — der Origin-Filter greift unabhängig von `FoundInWorldRequired`. Änderung zurücksetzen.

### Z4. Combat-Logout bei Expeditionen

- [ ] **Vorbereitung:** Test-Quest `Quests\test_extract.json` anlegen (danach wieder löschen) und Server neu starten:

```json
{
	"Version": 1,
	"QuestId": "test_extract",
	"TraderId": "west_quartermaster",
	"Category": "EXPEDITION",
	"Title": "TEST Extraktion",
	"Description": "Testquest fuer Combat-Logout.",
	"Repeatable": false,
	"FailOnDeath": true,
	"FailOnFactionChange": false,
	"TimeLimit": -1,
	"Objectives": [
		{
			"ObjectiveId": "extract",
			"Type": "EXTRACT",
			"Amount": 15,
			"Zone": { "ZoneId": "test_extract_zone", "CenterX": 2035.0, "CenterY": 290.0, "CenterZ": 7295.0, "Radius": 25.0, "Height": 50.0 }
		}
	],
	"Rewards": { "PlayerExperience": 100 }
}
```

- [ ] **Schritte:** T1 nimmt `test_extract` an. T2 (oder ein Infizierter) fügt T1 Schaden zu; T1 loggt **innerhalb von 60 Sekunden** nach dem Treffer aus und wieder ein.
- [ ] **Erwartet:** Quest ist FAILED (Grund "Combat-Logout"). Gegenprobe: ohne Kampfkontakt (>60 s nach letztem Treffer) ausloggen → Quest bleibt aktiv. Positivpfad: 15 s in der Zone stehen → Extraktion vervollständigt das Objective; eine laufende Extraktion wird durch Verlassen der Zone abgebrochen (ohne Fail).

### Z5. Tracker-Persistenz

- [ ] **Schritte:** Zwei aktive Quests im Menü als "verfolgt" markieren (HUD-Tracker zeigt beide). Relog; danach Server-Neustart.
- [ ] **Erwartet:** Tracker zeigt nach Reconnect und nach Neustart dieselben Quests (`PlayerState.TrackedQuests` ist persistiert). Maximal 3 Quests gleichzeitig verfolgbar; Abschluss/Abbruch entfernt die Quest aus dem Tracker.

---

## Abschlusskriterium

Alle Haken gesetzt, keine ungeklärten `[DME_Tasks]`-Fehler im Log, temporäre Test-Änderungen (Tests 4, 10, Z3, Z4) zurückgesetzt.
