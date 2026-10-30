# Pixelia – Roadmap

> Lebendes Dokument: Status, Versionen und Wünsche werden laufend gepflegt.
> Die Grundsatzentscheidungen und ihre Begründungen stehen in [PROJEKTPLAN.md](PROJEKTPLAN.md).

## Aktueller Stand

| | |
|---|---|
| **Phase** | 0.5.x Engine, Phase A (settingunabhängig): 0.5.0 „Bühne und Figuren“ (PR #24), 0.5.1 „Raum und Baukasten“ (PR #26) , 0.5.2 „Farbe und Licht“ und 0.5.3 „Wetter und Partikel“ fertig: Wetter für alle Orte (Regen, Schnee, Nebel, Gewitter, Wind, Wolkenschatten) mit weichen Übergängen, Partikel mit Obergrenze, Schleier über Farbtabellen, Wetter im Fenster, Fensterschein nur bei Licht innen; 0.5.4 „Weboberfläche aufgeräumt“, 0.5.5 „Asset-Quelle und Grafik-Werkzeug“, 0.5.6 „Schrift, Einblendungen und Touch“ und 0.5.7 „Weltzeit, Kalender und Wetter-Simulation“ fertig: **Phase A abgeschlossen**. Phase B (Systeme mit Testinhalt im Testdorf): 0.5.8 „Orte und Gebiete“ fertig (PR #42), 0.5.9 „Weltzustand, Figuren und Speicherstand“ fertig (PR #45), **0.5.10 „Warenbuch, Aufgaben und Projekte“ in Arbeit** (Branch `engine-0.5.10`, Teil 1, 2a, 2b, 3a, 3b, 4a und 4b fertig; Messen am Gerät und Release offen) |
| **Letzte Version** | 0.5.9 „Weltzustand, Figuren und Speicherstand“ (2026-10-29, PR #45), [Release auf GitHub](https://github.com/Baldesarra/pixelia/releases/tag/v0.5.9). Figuren gehören der Welt (Fähigkeiten, Beziehungen, Erinnerungen, Aufenthalt), Speicherstand mit zwei Plätzen und Nachholen bis 3 Tage, Stufen der Simulation, Lichtkarte verteilt, Figuren-Baukasten mit Statur (4 Größen, 3 Breiten, Bauch, gebeugt) und neuer Kleidung, Seite Welt mit Speicherstand, Chronik, Figurenliste und Sicherung. Am Gerät (48 Figuren je Ort, nachts, Regen, Abblenden): 29,8 fps, Zeichnen max 18,2 ms, Logik max 6,9 ms; Baukasten 716–736 µs je Bild; RAM-Tiefstwert 89 KB, PSRAM 4,25 MB; Firmware 1,95 MB (32 %). Davor 0.5.8 „Orte und Gebiete“ (2026-10-27, PR #42), [Release auf GitHub](https://github.com/Baldesarra/pixelia/releases/tag/v0.5.8). Orte als Module mit Verbindungen als Daten und Orts-Graph, Ortszustände (Licht mit Brennstoff, Feuer und Glut, Nässe, Füllstand), Lichtquellen-Arten, Ereignis-Bus, Fahrzeuge als bewegliche Orte, großes Fenster mit Fensterbank und Mond, Kammer, Feldweg, Herd, Becken, nasser Boden; Nachprüfung Phase A. Am Gerät (64 Figuren, Regen, 9 Ortswechsel): 30,0 fps, Zeichnen 11,2 ms (max 11,7), Logik 2,2 ms (max 13,3, vorher bis 23); RAM 94 KB frei (Tiefstwert 86 KB), PSRAM 5,59 MB; Firmware 1,85 MB (31 %). Davor 0.5.7 „Weltzeit, Kalender und Wetter-Simulation“ (2026-10-26, PR #41), [Release auf GitHub](https://github.com/Baldesarra/pixelia/releases/tag/v0.5.7). Weltkalender (Tag 1 = 26.10.2026 am Gerät, Jahreszeit 7 Tage, Wechsel Nacht auf Montag), Dämmerung nach Weltjahreszeit, echter Mond (groß mit Kratern, ±0,4 h), Wetter-Simulation viermal am Tag, Erstlinge als Kurzmeldung, Jahreszeit-Symbol vor der Uhr, Nachtmodus mit Nachtzeiten, Helligkeitskurve, Download von GitHub macht per HTTP-Range weiter. Am Gerät: normal 29,8 fps, Zeichnen 5,1 ms, Logik max 1,3 ms; 64 Figuren mit Regen nachts 29,8–30,3 fps, Zeichnen 14,5–15,7 ms (max 17,0), Logik max 2,4–3,6 ms; RAM 98 KB (Tiefstwert 91), PSRAM 5,58 MB; Firmware 1,79 MB (30 %); Abriss bei 304 KB, weitergemacht, 1759 KB in 31,5 s. Davor 0.5.6 „Schrift, Einblendungen und Touch“ (2026-10-22, PR #38), [Release auf GitHub](https://github.com/Baldesarra/pixelia/releases/tag/v0.5.6). Karte rechts mit Knopf „Folgen“/„Betreten“/„Hingehen“, Türen und Wege mit Ziel, Pixel-Uhr (nachts gedimmt, antippen = Datum), Kurzmeldungen oben mit Warteschlange, Wischen, Kamera-Modi, Ortsname nach dem Wechsel; Seiten Display und Welt. Am Gerät (64 Figuren, Regen, nachts, Karte + Uhr + Meldung): 29,8 fps, Zeichnen 14,1 ms (max 15,4), Einblendungen 2,8 ms (max 3,2), Logik max 3,1 ms; RAM 99 KB (Tiefstwert 92 KB), PSRAM 5,4 MB; Firmware 1,84 MB. Davor 0.5.5 „Asset-Quelle und Grafik-Werkzeug“ (2026-10-22), [Release auf GitHub](https://github.com/Baldesarra/pixelia/releases/tag/v0.5.5). Grafiken als Pakete (eingebettet, LittleFS, SD vorbereitet), Grafik-Speicher im PSRAM mit Budget, Laden beim Ortswechsel auf Kern 0, Grafik-Format und Werkzeug PNG → Pixelia-Farben, Zustandsbilder, Credits-Prüfung in der CI, Logik-Maximum in `/api/perf`. Am Gerät: Ortswechsel 29,8 fps, 0 Aussetzer, Abblenden wartete nie; Ladezeit je Ort eingebettet 0,3–0,6 ms, LittleFS 7,5–18 ms (256 KB in 118 ms, 2,2 MB/s); Zeichnen bis 8,8 ms, Logik im Mittel 0,9–1,3 ms, Maximum beim Betreten bis 17 ms, bei der Überblendung bis 23 ms; RAM 101 KB frei (Tiefstwert 93 KB), PSRAM 5,70 MB; Firmware 1,72 MB (29 %). Davor 0.5.4 „Weboberfläche aufgeräumt“ (2026-10-22, PR #34), [Release auf GitHub](https://github.com/Baldesarra/pixelia/releases/tag/v0.5.4). Neue Navigation (Welt, Übersicht, System, Einstellungen), Hubs mit Kategorien, Seite Welt, Kurzfassung `k` im Changelog, zwei Designs unter Experimentell, neue Update-Karte; Bundle 147,0 KB gzip, Firmware 1,58 MB (26 %); 15 Vorabversionen im Kanal Vorschau am Gerät installiert, danach gelöscht. Davor 0.5.3 „Wetter und Partikel“ (2026-10-21), [Release auf GitHub](https://github.com/Baldesarra/pixelia/releases/tag/v0.5.3). Am Gerät: 29,8–30,3 fps, Zeichnen Sonne 5,7 ms (ohne Wolken 4,4), Regen 8,0, Schnee 7,2, Nebel 8,4, Bewölkt 10,1, Gewitter nachts 8,7 (64 Figuren 12,4), 64 Figuren mit Regen bis 16,0 ms, Überblendung nachts mit Regen und 64 Figuren bis 14,3 ms; RAM 109 KB frei (Tiefstwert 102 KB), PSRAM 5,79 MB frei. Davor 0.5.2 „Farbe und Licht“ (2026-10-21), [Release auf GitHub](https://github.com/Baldesarra/pixelia/releases/tag/v0.5.2). Am Gerät: 29,8–30,3 fps, 0 Aussetzer, Zeichnen tagsüber 3,8–4,2 ms, nachts mit Licht 4,4–5,8 ms, 64 Figuren 7,1–9,4 ms, Überblendung 12,5–16 ms (mit 64 Figuren je Ort bis 19,7 ms), RAM 110 KB frei (Tiefstwert 103 KB), PSRAM 5,82 MB frei; „bench mem“: 96 KB in den PSRAM schreiben 2,4 ms (CPU wie DMA). Davor 0.5.1 „Raum und Baukasten“ (2026-10-21, PR #26), [Release auf GitHub](https://github.com/Baldesarra/pixelia/releases/tag/v0.5.1). Am Gerät: alle Testorte 30,3 fps, Zeichnen 4,1–4,2 ms, Raum mit 64 Figuren 6,8 ms, Überblendung höchstens 14,6 ms, 0 Aussetzer, RAM 111 KB frei (Tiefstwert 104 KB), PSRAM 5,96 MB frei; „bench“: Verdoppeln 2,96 ms, Kopie des Hintergrunds 0,99 ms pro Bild. Davor 0.5.0 „Bühne und Figuren“ (2026-10-21, PR #24), [Release auf GitHub](https://github.com/Baldesarra/pixelia/releases/tag/v0.5.0). Am Gerät: 30 fps ohne Aussetzer, Zeichnen 4,2 ms (Testort mit 64 Figuren 5,9 ms, Überblendung höchstens 13,7 ms), RAM 102–106 KB, PSRAM 6,0 MB frei. Update von GitHub am Gerät (0.4.2-dev → 0.5.0): 1574 KB in 17,3 s (111 KB/s), RAM-Tiefstwert 78 KB, fps min 29,8, Probelauf bestanden; Prüfung mit TLS RAM 97 → 83 KB. Davor 0.4.2 „Experimentell, Live-Vorschau, Leistung“ (2026-10-20, PR #20), [Release auf GitHub](https://github.com/Baldesarra/pixelia/releases/tag/v0.4.2). Am Gerät: mit Live-Vorschau 29,7–30,3 fps, RAM 115 KB, 0 Aussetzer; Stromsparmodus 30 Hz mit Rückfall nach 15 s; Download nach 932 KB abgebrochen ohne Neustart. Davor 0.4.1 „Update von GitHub“ (2026-10-20, PR #19), [Release auf GitHub](https://github.com/Baldesarra/pixelia/releases/tag/v0.4.1); davor Vorschau v0.4.1-beta.1 und 0.4.0 „Versionsverlauf und Diagnose“ (PR #17, #18). Zweiter echter Durchlauf im Kanal Stabil (0.4.1-beta.1 → 0.4.1 über GitHub): 1522 KB in 15,8 s (Verbindung 3,2 s, 121 KB/s), 30,0–30,2 fps, RAM-Tiefstwert 91 KB; Probelauf bestanden, danach 30,3 fps, RAM 117/111 KB |
| **Repo** | https://github.com/Baldesarra/pixelia (öffentlich, alle Rechte vorbehalten) |
| **Installieren** | Web-Flasher https://baldesarra.github.io/pixelia/ (Chrome/Edge, USB), danach WLAN im Setup-Modus einrichten |
| **Nächster Schritt** | **0.5.10 Teil 4c (Hoch, frische Sitzung, Auftrag unten in „Offen“ unter „Übergabe für Teil 4c“):** Messen am Gerät mit dem User (48 Figuren je Ort, nachts, Regen, Abblenden; `flushMs`, `/data/wirtschaft`, LittleFS frei, PSRAM-Tiefstwert, Strom ziehen vor und nach dem Speichern, Säge auf der Bauplane), Lager-Karte am Handy ansehen, Release-Titel mit dem User, PR mit allen Teilen, Release wie bei 0.5.9 |
| **Offen** | **0.5.10 in Arbeit** (Branch `engine-0.5.10`, vier Teile, Plan vom User bestätigt 2026-10-29). **Teil 4b ✅ 2026-10-30 (Lager im Web):** Lager-Karte auf der Seite Welt **„Sorgen zuerst“** (Wahl des Users nach drei Runden: Entwürfe A/B/C, Beispieldorf mit 20 Orten auf- und zugeklappt, vier Lösungswege, [0510-lager-karte-entwuerfe.png](skizzen/0510-lager-karte-entwuerfe.png), [0510-lager-viele-sammel.png](skizzen/0510-lager-viele-sammel.png), [0510-lager-ideen.png](skizzen/0510-lager-ideen.png)): bis zu 3 knappe oder leere Lager („Brot · Haus Emil · Leer“, „Alle 11 zeigen“), sonst „Alle n Lager in m Orten sind gut versorgt“, darunter „In Arbeit“ mit Schritt und Wartegrund, Fortschritt als **Schritt-Segmente** (ein Stück je Teilaufgabe, breit nach Arbeitszeit, steht still; Wahl des Users statt der bewegten Welle, [0510-fortschritt-varianten.png](skizzen/0510-fortschritt-varianten.png)); Knopf **„Alle Lager“** → **Unterseite Lager** (`#/lager`) mit Umschalter **Waren | Orte** (Wunsch des Users): Waren = eine Zeile je Ware im Dorf mit Reichweite, antippen zeigt, wo sie liegt (knappe zuerst); Orte = Orte mit Sorgen offen, ab 9 Lagern alle übrigen in einer Sammelzeile „16 weitere Orte · alles gut“ ([0510-lager-gewaehlt.png](skizzen/0510-lager-gewaehlt.png)); leere Zustände „Noch keine Lagerplätze“, „Kein Projekt in Arbeit“; Gruppen der Waren (Essen, Brennstoff …) erst mit den echten Waren in 0.6.x (`cat`, im Testdorf nicht nötig). Daten: `GET /api/world?lager=1` (Grafik-Task füllt `LagerView` ~7 KB PSRAM nur auf Anfrage, eigene Anfrage neben `?mehr=1`, Abfrage alle 30 s wie die Chronik), `Ledger::store_level`, `ui::reach_text` (aus `store_card` herausgelöst); Konsole `lager` (Lagerplätze, Waren im Dorf, Projekte ins Log). Mock: `?viele=1` (Beispieldorf, 21 Orte, 88 Lager), `?leer=1`. `--test warenbuch` 32/32, alle 140 Simulator-Prüfungen grün, Firmware 2,09 MB (33 %), **am Gerät noch nicht geprüft**. **Für den Planungs-Chat (Wunsch des Users 2026-10-30):** Wie zeigt die volle Lager-Seite ein Dorf mit 16+ Häusern und 20+ Waren? Gebaut ist der Grundstock (Sorgen zuerst, Waren | Orte mit Sammelzeile); offen sind Gruppen der Waren, Häuser zusammengefasst, Raster Häuser × Hausvorräte als eigene Ansicht ([0510-lager-ideen.png](skizzen/0510-lager-ideen.png) Weg 4), Zu- und Abgänge des Tages. **Übergabe für Teil 4c (frische Sitzung, Hoch):** (1) `.\tools\ota.ps1 -Build`, Seite Welt und „Alle Lager“ am Handy ansehen (Testdorf hat 5 Lager, also alles offen), Konsole `lager`; (2) Teil 4a erstmals am Gerät: `/data/wirtschaft` entsteht, `flushMs` beim Speichern (`/api/world` → `wirtschaft`, Kern 0, kein Display-Aussetzer), LittleFS frei vorher und nach einigen Welttagen (Partition `storage` 3,8 MB; 1 MB Protokoll muss sicher passen), PSRAM-Tiefstwert mit 2 × 128 KB (Ziel ≥ 1 MB), **Strom ziehen mit dem User** einmal kurz nach und einmal kurz vor einem Speichern (danach jede Buchung genau einmal im Protokoll, Bilanz stimmt), Säge auf der Bauplane und „wartet auf die Säge“ am Display (`ort cmd projekt bett …`); (3) Messen: 48 Figuren je Ort, nachts, Regen, Abblenden (fps, Zeichnen max, Logik max, RAM-Tiefstwert); (4) CHANGELOG: 1 Stichpunkt frei für `g` mit Messwerten (sonst zusammenfassen), `t`/`s` mit 2–3 Vorschlägen an den User; ROADMAP, CODEMAP (Messwerte); PR mit allen Teilen (1–4b) und Messwerten, Merge nach Freigabe mit Anleitung, Release-Doku-PR und Tag `v0.5.10` wie bei 0.5.9 (PR #45/#46), Beta-Releases löschen, falls es welche gibt; danach 0.5.11 im Planungs-Chat (xhigh). **Teil 4a ✅ 2026-10-30 (Säge und Protokoll):** **Säge als Werkzeug am Projekt** (`ProjectNeed::tool`): wird für Gestell und Liegefläche einmal aus der Vorratskiste geholt, mit dem Material reserviert, nicht verbaut, nach dem Projekt zurückgebracht; ihr Gewicht zählt in der Trage; fehlt sie, wartet der Schritt (Wartegrund `werkzeug`, „wartet auf die Säge“), liegt sie an einer anderen Baustelle, wartet ein anderes Projekt („Säge ist an der Baustelle (…)“); Karte zeigt „Säge da / unterwegs / fehlt“ im laufenden Schritt; **Handsäge liegt sichtbar vor dem Stapel auf der Bauplane** (Wahl des Users, [0510-saege.png](skizzen/0510-saege.png)). **Wirtschafts-Protokoll** (`econ_log.h`): `bewegungen`, `projekte`, `bedarf`, `bestand_lager_tag`, `wetter_tag` (Tabellen Wahl des Users) als CSV je Welttag in `/data/wirtschaft` (≤ 1 MB, älteste Tage weg), Puffer 2 × 128 KB PSRAM, angehängt erst nach dem Schreiben des Speicherstands (jede Buchung genau einmal, auch nach Neustart ohne Speichern), bei ¾ vollem Puffer wird früher gespeichert; Angaben unter `wirtschaft` in `/api/world` und im Diagnosepaket; Simulator `--wirtschaft DIR` (7 Welttage Testdorf: 24 KB). `--test warenbuch` 31/31, alle 139 Simulator-Prüfungen grün, Firmware 2,08 MB (33 %), **am Gerät noch nicht geprüft**. Kontext der Sitzung über 150 K → Übergabe (Ergänzung D). **Übergabe für Teil 4b:** Lager-Karte im Web: Daten fehlen noch (Grafik-Task muss Lagerplätze mit Füllstand und Reichweite sowie Projekte mit Fortschritt in eine Ansicht wie `WorldView` füllen, z. B. `/api/world?lager=1`); Reichweite in Worten wie `ui::store_card` (dort als eigene Funktion herauslösen, auch für die Konsole `lager`); Abholen, Bericht und Diagnose-Seite des Protokolls gehören zu 0.5.12; Zustand und Verschleiß der Säge (WIRTSCHAFT 10) zu 0.6.x. **Sichtprüfung am Display (für den User, nach dem Release):** Holzstapel und Hackklotz (Wiese), Brotregal und Holzlege (Raum), Lehmbackofen mit Brett und Glut (Platz), Bett-Baustelle mit Bauplane, Stapel und Handsäge (Kammer, tagsüber), Karten von Lager, Figur und Projekt antippen (Projekt-Karte zeigt „Säge da“ im laufenden Schritt); am Handy Seite Welt → Lager-Karte und „Alle Lager“ (Waren | Orte). **Teil 3b ✅ 2026-10-30 (Bilder und Karte):** Bett im Bau in vier Stufen (Bauplatz mit Kreide und Sägebock, Gestell, Latten, Strohsack wird gefüllt), daneben der Materialstapel (Bretter in zwei Stapeln, Strohbunde, Nägel je zwei ein Bild), der mit der Arbeit schrumpft, auf einer **Bauplane** am Boden, **helles Holz** (Wahl des Users nach zwei Runden, [0510-im-bau-varianten.png](skizzen/0510-im-bau-varianten.png), [0510-im-bau-stufen.png](skizzen/0510-im-bau-stufen.png)); `Room::set_fill` mit Stellplatz-Bereichen, `set_state` zeichnet nur sein Rechteck neu; Karte eines Projekts aus echten Daten (`ui::project_card`, `--screen karte-schritte` in der Kammer); Kurzmeldung bei fertigem Projekt, Strohsack darf jeder füllen (Wünsche des Users); Stapel versperrt nichts, ist kein Stehplatz, nach dem Laden sofort richtig, alter Stand mit leerem Stapel. `--test warenbuch` 26/26, alle 134 Simulator-Prüfungen grün; Messung (PC): voller Stapel +0,4–2,6 µs je Bild, Neuzeichnen eines Bretts 1,1 µs, Stufe des Betts 1,3 µs statt 12,9 µs fürs ganze Bild; Firmware 2,07 MB (33 %). **Säge am Projekt** gewünscht, kommt in Teil 4 (Engine-Kern, eigene Prüfungen). Zeichen mit Fortschritt über der Baustelle (Variante C) im Hinterkopf. **Am Gerät ansehen (freiwillig):** `.\tools\ota.ps1 -Build`, dann Kammer betreten (Bett-Projekt läuft tagsüber); Bett, Stapel oder Plane antippen zeigt die Projekt-Karte; `ort cmd projekt bett start` beginnt neu. **Teil 3a ✅ 2026-10-30 (Kern):** Projekte mit Teilaufgaben (`projects.h`, Abschnitt PROJ): Material über Holen-Aufgaben ein Schritt voraus (Wahl des Users), ein Schritt beginnt erst, wenn sein ganzes Material an der Baustelle liegt, und reserviert es dort; Verbauen mit jedem Arbeitsgang (aufgerundet nach Fortschritt, der Stapel schrumpft mit der Arbeit), übriges Material zurück in die Vorratskiste; arbeiten darf nur, wer Tischlern kann (Wahl des Users), nur 7–19 Uhr (Wahl des Users, Platzhalter bis 0.5.11), lernt dabei; Wartezeit nach Grund (Material, Fähigkeit, Arbeitskraft, Wetter, Feierabend) und `ProjectRecord` für `projekte.csv` (Teil 4 schreibt nur noch); Chronik „Bett bauen: Gestell zimmern fertig“ und „Projekt fertig: Bett bauen“; Testinhalt Bett in der Kammer (Gestell 6 Bretter + 4 Nägel, 4 h; Liegefläche 2 + 2, 2½ h; Strohsack 4 Stroh, 1½ h). Ortszustand Arbeit (`StateKind::Work`, „Bett im Bau“ im Raum) entfernt; ein alter Stand lädt, das Bett-Projekt beginnt frisch. Befehle `ort cmd projekte`, `ort cmd projekt bett start|stufe <n>|fertig`. `--test warenbuch` 24/24 (Bett fertig mit genau 8/6/4 und 8 Personenstunden, Neustart mitten im Bau und mitten im Transport zur Baustelle, P10 mit Bau, alter Stand), alle 132 Simulator-Prüfungen grün; Firmware 2,06 MB (33 %). Noch ohne eigene Bilder: das Bett zeigt im Bau das bisherige Bau-Bild, das Material liegt unsichtbar an der Baustelle (Teil 3b). **Teil 2b ✅ 2026-10-29:** Lagerplätze zeigen jedes Stück einzeln (`Room::set_fill`, Stellplätze je Stück, spätere vorn, Spielarten): Holzstapel mit Pfählen (Wiese, 40, Wahl des Users aus vier Formen, [0510-holzstapel-formen.png](skizzen/0510-holzstapel-formen.png)), Hackklotz davor, Holzlege neben dem Herd (12), Brotregal an der Wand (8, **nur Testinhalt**: um 1850 lag Brot im Brotkorb, Brotschrank oder auf der Brotleiter; die echte Küche kommt mit 0.6.0), Lehmbackofen am Platz mit Brett für 6 Laibe und Glut, solange jemand backt (Wünsche des Users); Bilder Scheit und Laib selbst gezeichnet; Stücke stehen im Hintergrundbild, eine Änderung zeichnet nur ihr Rechteck neu (pixelgleich geprüft); Karte eines Lagerplatzes mit echten Daten (`ui::store_card`: eine Ware in Zeilen, mehrere als Liste, „hinten“ als Zahl), Vorlagen `--screen karte-lager`/`karte-vorrat`; Befehl `ort cmd vorrat 50%` bzw. `vorrat holzlege 5`; `--test warenbuch` 18/18, alle 126 Simulator-Prüfungen grün; Messung (PC): 40 Scheite +3 µs je Bild, 20 Stücke im Raum ±0, Neuzeichnen eines Lagers 1,0–1,4 µs statt 9–12 µs für das ganze Bild; Firmware 2,05 MB (33 %). **Sichtbares Tragen** (Kontaktbogen [0510-tragen.png](skizzen/0510-tragen.png)): Wahl des Users **in 0.6.x** mit den echten Figuren. **Am Gerät ansehen (freiwillig):** `.\tools\ota.ps1 -Build`, dann Wiese (Holzstapel, Hackklotz), Raum (Brotregal, Holzlege), Platz (Lehmbackofen mit Brett); Lager antippen zeigt die Karte; `ort cmd vorrat 100%` / `vorrat 10%` am betretenen Ort. **Teil 2a ✅ 2026-10-29:** Aufgabenliste der Gemeinschaft (`tasks.h`, Abschnitt AUFG): Holen und Bringen mit Reservierung aus den Regeln der Lagerplätze, Herstellen am Erzeugerlager, Notbestand als Gemeinschaftsaufgabe (3 Helfer, Eintrag in der Chronik, Kurzmeldung beim Notbestand); Planer der Testorte übernimmt Aufgaben nur an Takt-Grenzen, geht über mehrere Orte (Orts-Graph, manchmal der Feldweg), trägt im Korb; Hackklotz-Platzhalter spaltet, Backofen-Platzhalter am Platz backt (Wahl des Users), Platzhalter-Mahlzeiten aus dem Brotregal; Karte einer Figur mit Aufgabe und „Trägt 8 Scheite“; Befehl `ort cmd aufgaben`; `--test warenbuch` 15/15 (Neustart mitten im Transport auf dem Feldweg, P10 mit Transporten), alle 123 Simulator-Prüfungen grün; 48 Figuren in einem Ort: Schritt mit Takt-Grenze im Mittel 8 µs, höchstens 70–110 µs (PC); Firmware 2,04 MB (32 %). Nachholen nach langer Pause: nur Ortszustände, Figuren holen danach Nachschub (Wahl des Users, „grob nachschätzen“ vorgemerkt für 0.5.12). **Am Gerät ansehen (freiwillig):** `.\tools\ota.ps1 -Build`, dann auf der Wiese die Kiste rechts oben (Holzstapel), im Raum Herd und Kiste links unten (Holzlege, Brotregal), auf dem Platz die zwei Kisten (Backofen links, Vorratskiste rechts); Figur antippen zeigt Aufgabe und Trage; Konsole `ort cmd aufgaben`. **Teil 1 ✅ 2026-10-29:** Warenbuch-Kern (`ledger.h`), Speicherstand WARE/LAGR/STCK/RESV, alter Stand 0.5.9 lädt mit Test-Startvorrat, Herd verbrennt Brennholz aus der Holzlege, Reihenfolge je Schritt Takt-Grenzen → Ortszustände → betretene Orte, Tätigkeiten warten während langem Nachholen, `--test warenbuch` 10/10, alle 128 Simulator-Prüfungen grün, Firmware 2,02 MB (32 %). Entscheidungen des Users: Chronik nur Besonderes, einfache Lager-Karte in 0.5.10, Testinhalt Holzkette + Bett-Projekt + Brot im Regal, Protokoll in LittleFS (1 MB) gesichert mit dem Speicherstand. **Aussehen gewählt 2026-10-29** (Kontaktbögen in [skizzen/](skizzen/), `0510-*.png`): Stellplätze zeigen **jedes Stück einzeln** (Rest „hinten“ als Zahl auf der Karte); „im Bau“ zeigt **beides**: halbfertiges Teil in Stufen (je Teilaufgabe ein Bild) und daneben den Materialstapel, der mit der Arbeit schrumpft; Karte eines Lagerplatzes mit einer Ware **in Zeilen** („9 von 12“, Reichweite in Worten, heute gebracht/verbraucht), mit mehreren Waren **als kurze Liste** („Brot 24 · gut“); Karte eines Projekts mit **senkrechter Schritt-Leiste** (Wunsch des Users), nur der laufende Schritt zeigt sein Material („Bretter 1 / 2“), schon gebaut (`CardStep`, `--screen karte-schritte`). WIRTSCHAFT 6 an die Wahl zur Chronik angepasst (2026-10-29, freigegeben über den Planungs-Chat). **0.5.9 ✅ 2026-10-28** (PR #45, Release v0.5.9). **Statur ✅ 2026-10-28** (alles Wahl des Users nach Kontaktbogen und Gehbild-GIF, [figuren-statur-besetzung.png](skizzen/figuren-statur-besetzung.png)): Ansatz A, 4 Größen, 3 Breiten, Bauch 0–2, gebeugt; Strecken beim Zusammensetzen statt fertig gezeichneter Staturen; Kopftuch, Strohhut, Gehrock, Wirtsschürze, Gehstock; zehn Testfiguren aus SETTING Kapitel 3; größte Figur 32 hoch, Bild 24 × 40 (STIL, PROJEKTPLAN 14); alter Stand 0.5.9 lädt mit Standard-Statur; am Gerät 716–736 µs je Bild (vorher geschätzt 375, `kFrameCost` jetzt 3000; interner RAM brachte nichts), 48 Figuren je Ort, nachts, Regen, Abblenden: 29,8 fps, Zeichnen max 18,2 ms (danach 14,7), Logik max 6,9 ms, RAM-Tiefstwert 89 KB (67 KB einmal bei Live-Log und Dauerabfrage), PSRAM 4,25 MB. **Sichtprüfung Statur am Display:** Testfiguren auf Platz und Wiese (groß, klein, Bauch, gebeugt mit Stock, Hüte), Karte einer großen Figur antippen. **Teil 4 ✅ 2026-10-28:** Seite Welt mit Karten Speicherstand und Chronik (Zeitleiste), Figurenliste als Steckbriefe, Zeile in der Übersicht, Speicherstand herunterladen und einspielen (mit Neustart), Diagnosepaket mit Weltzustand, Fähigkeiten auf der Karte mit drei Segmenten (alles Wahl des Users); am Gerät: Speichern und Neustart mit Tätigkeiten, Strom gezogen, Einspielen (kaputte Dateien abgelehnt); Baukasten 288 µs je Bild (geschätzt 375), alte Testfigur nach dem Start nur 0,6 s zu sehen; Abblenden mit 48 Figuren je Ort, nachts, Regen: 29,8 fps, Zeichnen max 16,5 ms, Logik max 6,7 ms, RAM 85 KB, PSRAM 4,67 MB. In der Test-Welt am Gerät steht ein alter Chronik-Eintrag „Tag 1, 00:00 Speicherstand eingespielt“ (vor der Fehlerbehebung, harmlos); kurze Ausfälle unter 1 Weltminute erscheinen bewusst nicht in der Chronik (Wahl des Users 2026-10-28). **Sichtprüfung 0.5.8 am Display:** Herd im Testraum (abends Feuer, nachts Glut; Konsole `ort cmd ofen feuer|glut|kalt`), Kerze auf der Fensterbank nachts, solange die Begleiterin unterwegs ist, rundes Becken auf dem Platz (`ort cmd becken 0|50|100`), nasser Boden mit Pfützen (`ort cmd nass 100`, `nass 0`), Fenster der Wiese nachts heller, je mehr im Raum brennt, orange bei reinem Herdfeuer (`ort cmd innenlicht aus` um 21 Uhr), Rauch über dem Haus (Wiese, solange der Herd brennt), Karte mit Zustand (Herd, Becken, Kerze antippen), `ort cmd zustaende` (Liste im Log); **Mond im Fenster am Display bestätigen** (Konsole `ort cmd wetter sonne sofort`, dann `ort cmd mond 50 70 30`, `mond 25 60 70`, `mond 10 40 80`; Raum und Kammer); **Sichtprüfung 0.5.7 am Display:** Jahreszeit-Symbol vor der Uhr, Uhr antippen (drei Zeilen), Mond nachts im Fenster des Raums und in der Landschaft (Konsole `ort cmd mond 50 70 30` erzwingt ihn), Nachtmodus ab 22 Uhr (Abblenden, Berühren hellt auf), Helligkeit 1–100 % (wirken die Stufen gleichmäßig? leuchtet 1 % noch sicher? sonst `blduty <n>` suchen und mit `bldmin <n>` speichern); Kurzmeldungen am Sonntag 18 Uhr und Montag 6 Uhr; **Weltkalender läuft am Gerät seit 26.10.2026** (Tag 1), mit 0.6.0 ggf. neu starten (Entscheidung im Planungs-Chat); Weltzeit braucht die Uhrzeit (ohne Internet kein Kalender und kein simuliertes Wetter); alter Absturzbericht (Stapelüberlauf Konsole, Version vor 0.5.6) liegt noch im Gerät; **Sichtprüfung 0.5.6 am Display:** Figur und Tür antippen (Karte), wischen, Knopf „Folgen“ auf der Karte einer Figur, Knopf „Betreten“ auf der Karte einer Tür, Türschwelle im Raum und Weg am rechten Rand der Wiese antippen, Ortsname nach dem Wechsel, Hinweis vor der Rückkehr, Uhr nachts gedimmt und antippen (Datum), langes Halten (Info-Karte), Pixel-Uhr, Kurzmeldung oben, Hinweis auf eine neue Version, alles auch mit Bilddrehung 180°; Seite Welt → „Anzeige folgt“ am Handy; **SD-Karte messen**, sobald eine Karte im Gerät steckt (Treiber seit 0.5.5 fertig, ungetestet): `sd neu`, `assets kopieren sd`, `assets messen`, `assets quelle sd`, Ortswechsel; dabei Tempo, Aussetzer und Ziehen der Karte prüfen; **Logik-Spitze beim Betreten** eines Ortes bis 17 ms (Hintergrundbild, Wetter-Vorlauf) und beim Start einer Überblendung bis 23 ms (`merge_palettes`), gemessen mit `logicMax` in 0.5.5: in 0.5.8 auf mehrere Schritte verteilen (Grenze 20 ms inkl. Zeichnen); eigene Farben eines Bildes (Indizes 224–239) auf freie Grundfarben legen: mit dem Hochladen eigener Möbel (0.7.x); Sichtprüfung 0.5.5 am Display: Raum nachts mit Licht aus (Lampe ohne leuchtenden Kern), Ortswechsel; Sichtprüfung 0.5.4 am Display und Handy: Designs Pixelia und Fahrplan, grünes Feld „Pixelia ist aktuell“ (im Simulator nicht darstellbar); alte Vorabversion v0.4.1-beta.1 steht noch in den Releases (löschen auf Wunsch); Skizzen-Links in „So entstand Pixelia“ gehen erst nach dem Pages-Lauf des Releases; Sichtprüfung 0.5.3 am Display: Wetter auf Experimentell → Orte (alle Arten, Stärke, Übergänge, Blitz, Licht innen an/aus nachts an Wiese und Raum); Sichtprüfung 0.5.2 am Display: Licht und Tageszeit auf Experimentell → Orte (Schieber, Zeitraffer), feines Raster der Lichtkreise aus Betrachtungsabstand, Nacht lesbar?; Zeichenweg bleibt (Entscheidung 2026-10-21, Neuplanung): „Display liest das grobe Bild direkt“ ist eine Reserve, Messung des schlechtesten Falls in 0.5.12; Überblendung mit 64 Figuren je Ort bis 19,7 ms (0.5.2; Grenze 20 ms inkl. Logik), Standard ist Abblenden; Sichtprüfung am Display durch den User: Testorte, Übergänge, Ausgänge, Knöpfe Ausbau/Im Bau, Prüfansicht (Seite Experimentell → Orte), Stromsparmodus (Panel 30 Hz: flimmert es?), Touch-Test, Displaytests; Rückfall mit einem von GitHub geladenen kaputten Image nicht eigens geprüft (gleicher Weg wie beim Upload, dort geprüft; ein absichtlich kaputtes Release wäre für alle im Kanal Vorschau sichtbar); Captive Portal unter iOS einmal ansehen (Android und Windows geprüft); Screenshot-Läufe schreiben die Standard-Farbwahl aufs Gerät, solange es noch keine hat (harmlos); Geräteliste: kopflose Screenshot-Läufe legen jedes Mal ein neues Gerät an (harmlos, älteste fallen raus); **Planung:** Neuplanung nach dem Setting fertig (PR #31: Systemplan, Phasen ab 0.6.x als Kapitel der Welt, Entscheidungen des Users: Museum komplett in 0.7.x, keine fremden Grafik-Pakete, Grube vor Lok); Setting-Werkstatt: Licht-Regeln, Grube vor Lok und Mindestziele Kapitel 1 erledigt (2026-10-21, [SETTING.md](SETTING.md) Kapitel 9 und 11, [LEBENDIGKEIT.md](LEBENDIGKEIT.md) Kapitel 2); Fragen vor 1.0 in [ZUSATZINHALTE.md](ZUSATZINHALTE.md); Grafik-Format seit 0.5.5 ([STIL.md](STIL.md) Kapitel 9), Zeichenanleitung für Möbel in 0.6.0 |
| **Aktualisiert** | 2026-10-30 (0.5.10 Teil 4b) |

Messergebnisse und geprüfte Pinbelegung: [HARDWARE.md](HARDWARE.md). Kurz: 30 fps stabil bei 60 Hz Panel, Zeichnen
4,0 ms pro Bild, Hochskalier-Interrupt 20 %, keine Aussetzer, auch mit WLAN, Webserver, während Updates und unter Last
auf allen Diagnose-Schnittstellen. Interner RAM im Betrieb 116–121 KB frei (Tiefstwert ≥ 107 KB). Update per WLAN in
7–11 s, Rückfall bei Absturz, Hänger oder gescheitertem Selbsttest am Gerät geprüft. Log, Befehle, Leistung,
Reset-Historie und Absturzberichte per WLAN (`tools/netlog.ps1`, `tools/crash.ps1`), USB nur noch für die
Erstinstallation.
Simulator: `tools/sim.ps1` (Fenster) bzw. kopflos mit Screenshot, Zeitraffer und Kontaktbogen, siehe [sim/README.md](../sim/README.md).

## Versionsschema

Immer drei Stellen: `MAJOR.MINOR.PATCH`.

- **MINOR = Phase.** Bis 0.5.x ist das Technik (Fundament, Netz, Oberfläche, Updates, Engine), ab 0.6.x ist jede Phase
  ein **Kapitel der Welt** ([SETTING.md](SETTING.md) Kapitel 4, Neuplanung 2026-10-21). Die erste Version einer Phase
  endet auf `.0` (z. B. 0.6.0 = erste Version von Kapitel 1).
- **PATCH = jede weitere Iteration** derselben Phase (0.6.1, 0.6.2, … – auch für Anpassungswünsche und Fehlerbehebungen).
- Testversionen für den Kanal „Vorschau“: `X.Y.Z-beta.N`.
- Vor 1.0.0 sind alle Versionen Vorabversionen.

| Version | Phase | Inhalt |
|---|---|---|
| 0.1.x | Fundament | Repo, CI, Display und Touch, Engine-Prototyp, Simulator |
| 0.2.x | Vernetzt | WLAN, Setup-Modus, Webserver, OTA per Upload mit Rollback, Fernwartung per WLAN |
| 0.3.x | Weboberfläche | M3-Expressive-Komponenten, Info-/Hardware-/WLAN-/System-Seiten |
| 0.4.x | Updates & Diagnose | Update von GitHub, Versionsverlauf, Absturzberichte, Experimentell, Live-Vorschau |
| 0.5.x | Engine | Bühne, Baukasten, Licht, Wetter, Asset-Quelle, Einblendungen, Weltzeit; dann Orte, Weltzustand, Warenbuch, Tagesgerüst, Nachweis mit Testinhalt |
| 0.6.x | 🚉 Kapitel 1 „Der erste Frühling“ | Bahnhof mit gestrandetem Zug, Umgebung, 16 Bewohner, Grundwirtschaft, Notunterkünfte, Abende in der Taverne, Katze |
| 0.7.x | 🏘️ Kapitel 2 „Das Dorf wächst“ (Schwerpunkt) | Dorf, Bauernhof, Sykoras Hof, Wunschliste, Ausbaustufen, Wahl, Romanze, Zuzug, Händlerin, Briefe, Museum |
| 0.8.x | ⛏️ Kapitel 3 „Die alte Grube“ | Grube mit Toren, Berghang mit Steinbruch, Funde, Rätsel der Grabstelle |
| 0.9.x | 🚂 Kapitel 4 „Die Lok“ | Lok in Teilaufgaben, Strecke, Abteil im fahrenden Zug, erste Fahrt zur Nachbarstadt |
| 1.0.0 | Pixelia 1.0 | alle Kapitel verbunden, Feinschliff, 72-h-Dauertest |
| nach 1.0 | Zusatzinhalte | neue Regionen mit dem Zug und mehr, siehe [ZUSATZINHALTE.md](ZUSATZINHALTE.md) |

---

## Vorbereitung (ohne Version)

- [x] Git für Windows installieren
- [x] GitHub CLI installieren, Anmeldung durch den User (Account `Baldesarra`)
- [x] Repo `pixelia` auf GitHub anlegen (öffentlich, keine Lizenz, vom User bestätigt)
- [x] Projektordner als Git-Repo, `docs/PROJEKTPLAN.md`, `docs/ROADMAP.md`, `CLAUDE.md` einchecken
- [ ] Optional: GitHub mit claude.ai verbinden (für Arbeit in der Cloud)
- [x] Werkzeuge installiert: ESP-IDF v6.1 (`D:\Espressif`, Werkzeuge in `C:\Espressif\tools`), Node.js 24 LTS,
      CMake 4.4, Ninja, Python 3.13, LLVM-MinGW (Simulator-Compiler)
- [x] Hilfsskript `tools/idf.ps1` (ESP-IDF aktivieren, Ausgabe kürzen), Test-Build erfolgreich
- [x] `docs/HARDWARE.md` mit erwarteter Pinbelegung und Prüfplan

## 0.1.x – Fundament

- [x] Repo-Grundgerüst (`firmware/`, `engine/`, `worlds/`, `sim/`, `web/`, `tools/`, `assets/` mit `CREDITS.md`,
      dazu `pages/` für GitHub Pages), jeweils mit README
- [x] GitHub Actions: Firmware-Build, Größenbericht, Artefakte (Budget 75 % wird geprüft)
- [x] Pinbelegung und Panel-Timings verifizieren → `docs/HARDWARE.md` (Status GEPRÜFT)
- [x] Display-Bring-up: RGB-Panel (24,6 MHz, 60 Hz), Hintergrundbeleuchtung (PWM 200 Hz, GPIO 2)
- [x] Touch-Bring-up: GT911 (0x5D, Polling, Eckentest bestanden)
- [x] **Engine-Prototyp:** 400×240, 8 Bit Palette, Hochskalieren im Bounce-Buffer, Doppelpuffer mit Tausch am Bildende
- [x] **Messung:** FPS, Interrupt-Last, RAM-Belegung, Vollbild-Scrolling-Test → **Plan A bestätigt**
- [x] Hardware-Detektiv: Prüfbild, Touch-Test, Helligkeitsstufen, Pixeltakt-Test, Flash-Lasttest, geführter Test (`tour`)
- [x] Diagnose-Schnittstelle (serielle Befehle) und Leistungsmonitor (Zeile alle 2 s)
- [x] Hilfsskripte `tools/monitor.ps1` und `tools/flash.ps1` (Log mitlesen ohne Interaktion, Backtrace-Dekodierung)
- [x] 180°-Drehung des Bildes (ohne Mehrlast), Touch passend gedreht
- [x] Szenen-Schnittstelle in `engine/`: Gerät und Simulator nutzen denselben Szenen-Code
- [x] Weltzeit mit Zeitraffer (Befehle `time`, `speed`); Testszene mit Tag und Nacht
- [x] Simulator-Grundgerüst: SDL3-Fenster, Kopflos-Modus mit fester Uhrzeit, Vorspulen, Zeitraffer, PNG-Screenshot und
      **Kontaktbogen** (24 zufällige Momente) als Grundlage für den Langzeit-Test; `tools/sim.ps1`; CI-Job mit
      Screenshots als Artefakt
- [x] Partitionstabelle, Bootloader-Rollback aktiviert, Core-Dump (Rollback-Test mit echtem Update in 0.2.x)
- [x] Web-Flasher auf GitHub Pages (ESP Web Tools), Erstinstallation per USB; Factory-Image am Gerät geprüft
- [x] Einfacher Release-Weg: Tag `vX.Y.Z` → GitHub-Release mit `.bin`, Factory-Image und Notes aus `CHANGELOG.json`
- [x] **Release 0.1.0** (Tag `v0.1.0`, per Web-Flasher am Gerät installiert und geprüft)

## 0.2.x – Vernetzt

- [x] WLAN-Manager: bis zu 5 Netze, stärkstes zuerst, Hostname „Pixelia“; Ausweichmodus für Router im
      WPA2/WPA3-Mischmodus (Grund 17), beim Start 45 s Geduld, danach Setup-Modus
- [x] Setup-Modus: AP „Pixelia-Setup“ (WPA2, Passwort auf dem Display), Captive Portal (DNS + Prüfadressen von Android,
      iOS, Windows, Firefox + DHCP-Option 114), Neuversuch alle 2 min, Anleitung + zwei QR-Codes auf dem Display
- [x] mDNS `pixelia.local`, NTP-Zeit (Europa/Berlin), Weltzeit folgt der echten Uhrzeit
- [x] HTTP-Server + WebSocket, einfache Setup-Seite (WLAN, Update, Anzeige, System)
- [x] OTA per `.bin`-Upload mit Fortschritt (Browser + Display), Prüfung von Projekt, Chip, Revision, Prüfsumme
- [x] A/B-Rollback mit Selbsttest: Absturz, Hänger und gescheiterter Selbsttest am Gerät getestet
- [x] Einstellungen in NVS (Bilddrehung 0/180°, Helligkeit)
- [x] Systemanzeigen in der Engine: Schrift mit Umlauten, QR-Code-Erzeuger, Setup/Update/Meldung/Kurzmeldung
      (auch im Simulator, `--screen`)
- [x] Messung: 30 fps mit WLAN, Webserver und während Updates; ≥ 115 KB interner RAM frei
- [x] Backlog-Punkt RAM-Einbruch beim Start geklärt (DMA-Pool von ESP-IDF, kein echter Verbrauch)
- [x] **Release 0.2.0** (Tag `v0.2.0`, Web-Flasher aktualisiert); echtes Update auf die veröffentlichte `.bin` per
      Browser durch den User: 1113 KB in 7,1 s, Probelauf mit Selbsttest bestanden
- [x] **Ab hier Updates ohne USB**
- [x] 0.2.1: Changelog ausführlich im Stil der PR-Beschreibungen: Felder „Geprüft“ (g), „Hinweise“ (h), „Pull Requests“
      (pr), 0.1.0 und 0.2.0 vollständig nachgetragen, Format-Prüfung `tools/release_notes.py --check` in der CI
- [x] 0.2.1: **Info-Karte per langem Druck (3 s)** irgendwo aufs Display: IP groß, QR-Code mit `http://<IP>/`,
      `pixelia.local`, `http://pixelia/`, `pixelia.fritz.box`, WLAN-Name, Signal (Balken und dBm), Hostname, Firmware;
      im Setup-Modus die Daten des Setup-WLANs. Tippen schließt, sonst nach 20 s. Halte-Anzeige während des Drückens,
      Dauer im NVS einstellbar (`set lp`, `/api/settings`). Gestenerkennung plattformunabhängig in
      `engine/gesture.h`, im Simulator per `--test gestures` geprüft (11/11)
- [x] 0.2.1: Kurzmeldung „Verbunden mit „WLAN“ · IP“ nach jedem Verbinden (Wunsch des Users)
- [x] 0.2.1: Simulator: Info-Karte per `--screen info|info-setup|info-offline|hold|toast-ip`, Maus-Geste im Fenster
- [x] 0.2.1: Werkzeug `tools/ota.ps1`: Update per WLAN (`POST /api/ota`) mit Fortschritt, Prüfung von Version und
      Selbsttest über `/api/status`, Rückfall-Erkennung; gegen einen nachgebauten Server geprüft
- [x] **Release 0.2.1** (Tag `v0.2.1`, Release mit `.bin`, Factory-Image und Notes, Web-Flasher aktualisiert)
- [x] 0.2.1 am Gerät: vom User getestet („läuft“); erstes Update per `tools/ota.ps1` am echten Gerät in 0.2.2
      (1168 KB in 6,9–10,8 s, Selbsttest bestanden)
- [x] 0.2.2: **Log-Ringpuffer** 256 KB im PSRAM (No-Init, übersteht Neustarts, Updates und Abstürze, Cache wird pro
      Zeile zurückgeschrieben) mit Nummer, Laufzeit, Uhrzeit, Startzähler; RTC-Sicherung der letzten 6 KB für
      Versionswechsel (Probelauf zeilenweise, sonst Shutdown-Handler)
- [x] 0.2.2: Schnittstellen per WLAN: `GET /api/log` (inkrementell), WebSocket `/ws/log` (Live-Log), `POST /api/cmd`
      (Konsolenbefehle, gefährliche nur mit Bestätigung, PIN vorbereitet), `GET /api/perf`, `GET /api/resets`,
      `GET /api/coredump` (roh), `/api/coredump/summary`, `POST /api/coredump/erase`
- [x] 0.2.2: Reset-Historie der letzten 16 Starts im NVS (Zeit, Grund, Version, Rückfall, Absturzstelle, Laufzeit davor)
- [x] 0.2.2: **Diagnosepaket** (`/api/diag`, eine JSON-Datei ohne Passwörter) mit Knopf auf der Einrichtungsseite;
      Passwort-Filter über alle Ausgaben (`engine/redact.h`, Simulator-Test 9/9, Ende-zu-Ende am Gerät geprüft)
- [x] 0.2.2: Werkzeuge `tools/netlog.ps1` (Live-Log, Befehle, frühere Starts) und `tools/crash.ps1` (Bericht holen,
      ELF finden, dekodieren, Log vor dem Absturz); beide am Gerät ohne seriellen Port nachgewiesen, auch mit den
      Absturz-Testvarianten `crash` und `crash-shift`
- [x] 0.2.2: Messung: 30 fps, 0 Aussetzer unter Last, interner RAM ≥ 107 KB, Hook 16–19 µs pro Zeile
      ([HARDWARE.md](HARDWARE.md)); Status-WebSocket `/ws` repariert (sendete unter ESP-IDF 6.1 nie)
- [x] **Release 0.2.2** (Tag `v0.2.2`, Release mit `.bin`, Factory-Image, `.elf` und Notes, Web-Flasher aktualisiert).
      Per `tools/ota.ps1` aufs Board: 1170 KB in 7,0 s, Selbsttest bestanden, 29,8 fps; Log-Puffer über das Update
      übernommen (2716 Zeilen); ELF-SHA256 des Releases passt zur laufenden Firmware (für `tools/crash.ps1`)

## 0.3.x – Weboberfläche

Aufgeteilt in drei Patches, je eine Arbeits-Sitzung (siehe [ARBEITSWEISE.md](ARBEITSWEISE.md)). Denkstufe: Hoch.

**0.3.0 Grundgerüst**
- [x] Svelte-Projekt, Build in CI, gzip-Einbettung in die Firmware (89 KB, Firmware 1261 KB = 21 % der Partition)
- [x] M3-Expressive-Komponentenbibliothek (Tokens, Formen, Bewegung, dynamische Farben, hell/dunkel)
- [x] Offline-Schriften und -Icons (reduziert: Roboto Flex 33 KB, 23 Material Symbols als SVG-Pfade)
- [x] Navigation (Handy + Desktop) und Seite Übersicht, dazu Seite Darstellung (Farbwahl, Komponenten-Vorschau)

**0.3.1 Seiten**
- [x] Einrichtungsseite (Setup, `firmware/main/web/setup.html`) ans neue Design anpassen, ohne Funktionsverlust
      (WLAN, Update-Upload, Anzeige, System, Diagnosepaket); Setup-Modus im Captive Portal am Gerät prüfen
      (Wunsch des Users, 2026-10-19). Ergebnis: Setup-Modus zeigt die neue Oberfläche (WLAN-Seite), `/setup` bleibt
      als schlanke Notfallseite im M3-Look; Android-Portal geprüft, Windows-Portal noch offen
- [x] Seiten: Info, Hardware, WLAN (Scan, Signalbalken), Verbundene Geräte, System (Bundle gzip 119 KB)
- [x] Live-Log als Seite in der Oberfläche (Schnittstellen `/ws/log` und `/api/log` gibt es seit 0.2.2), mit Befehlszeile

**0.3.2 System und Schutz**
- [x] Neustart, Werksreset (Oberfläche und Touch-Geste am Gerät), Einstellungen exportieren/importieren (inkl. Farbwahl,
      die jetzt auf dem Gerät liegt); die Sicherung enthält nie WLAN-Passwörter oder den PIN
- [x] PIN/Passwort optional (Standard aus): schützt Einstellungen, Aktionen, Updates, Werksreset, `/api/cmd`, Absturzbericht
      löschen und Gerätenamen; Sperre nach 5 Fehlversuchen; vergessen → am Gerät zurücksetzen (Info-Karte → Zurücksetzen)
- [x] Captive Portal unter Windows geprüft: Windows öffnet die Einrichtung von selbst (iOS noch offen)

## 0.4.x – Updates & Diagnose

In drei Patches (Entscheidung 2026-10-20, eine Arbeits-Sitzung pro Patch):

- [x] Release-Workflow: Tag → GitHub-Release mit `.bin` und Factory-Image (seit 0.1.0)

**0.4.0 Versionsverlauf und Diagnose**

- [x] `CHANGELOG.json` → eingebetteter gzip-Block mit Marker in der Firmware (`tools/release_notes.py --embed`),
      `GET /api/changelog`
- [x] Versionsverlauf-Timeline mit **allen Feldern** (Neu, Verbessert, Behoben, Entfernt, Geprüft, Hinweise, verlinkte
      Pull Requests), Markierung „Installiert“, Änderungsvorschau aus gewählter `.bin` (Seite System, vor dem Hochladen)
- [x] Absturzberichte und Reset-Historie in der Oberfläche anzeigen (Seite Diagnose; Daten und Schnittstellen seit 0.2.2)

**0.4.1 Update von GitHub**

- [x] Prüfung auf neue Versionen (GitHub-Releases), Kanal Stabil/Vorschau, Installation per HTTPS mit Rollback
- [x] Änderungsvorschau auch für die Version auf GitHub (Release-Notes aller neueren Versionen, ohne die .bin zu laden)
- [x] Echter Durchlauf: 0.4.0-1 → v0.4.1-beta.1 im Kanal Vorschau (1521 KB in 16,6 s, 29,8 fps, RAM ≥ 90 KB)

**0.4.2 Experimentell, Live-Vorschau, Leistung**

- [x] Experimentell-Seite: Touch-Test, Displaytests, Stromsparmodus, Panel-Timing, Experimentell-Schalter; Panel-Werte
      nur auf Probe (15 s), Fluchtweg über zwei kurze Starts
- [x] Live-Vorschau und Screenshot (PNG), Seite Live (`/api/screen`, Paletten-Indizes PackBits-gepackt, ≤ 3 Bilder/s)
- [x] Leistungsanzeige (FPS, Frame-Zeiten, CPU pro Kern, Tasks, Heap/PSRAM) über `/api/perf`; die Task-Liste nur auf
      Knopfdruck, nie regelmäßig (hält 1,1 ms die Kernel-Sperre, kostet einen Display-Aussetzer, [HARDWARE.md](HARDWARE.md))
- [x] Update von GitHub: Abbrechen, Prüfung im Hintergrund (täglich, Punkt an „System“), ältere Version ab 0.4.0
      (Seite Experimentell); Seite System lädt den Stand neu (sichtbar werden, Suche von anderem Browser)

**0.4.x abgeschlossen (2026-10-20).**

## 0.5.x – Engine

Aufteilung vom User bestätigt am 2026-10-21 und mit der Neuplanung nach dem Setting am selben Tag neu geschnitten
(Entscheidungsprotokoll). Phase A (0.5.0–0.5.3) ist settingunabhängig gebaut und wird in 0.5.8 gegen das Setting
nachgeprüft (Wunsch des Users). 0.5.4 ist fest „Weboberfläche aufräumen“. Danach folgen die übrigen Engine-Bausteine
und Phase B mit den Systemen aus [SETTING.md](SETTING.md) Kapitel 15, gebaut und geprüft mit Testinhalt im
**Testdorf**; die echte Welt beginnt mit 0.6.0. Welches System wann kommt, steht im
[Systemplan](#systemplan-wann-welches-system-entsteht). Jeder Patch ist eine eigene Arbeits-Sitzung (< ~250 K
Kontext) und wird im Simulator und am Gerät gemessen (fps, Bildzeit, RAM, PSRAM). Kamerafahrten und Parallax als
Hauptmittel entfallen, weil nichts scrollt (Perspektive 2026-10-21); Parallax gibt es höchstens am Fenster des
fahrenden Zugs (0.9.x).

**Phase A (settingunabhängig)**

- [x] **0.5.0 Fundament** (xhigh) ✅ 2026-10-21, PR #24: Bühne mit mehreren Orten, Wechsel per Sprung mit Übergang
      (Standard Abblenden, dazu Überblendung, Raster, Schnitt); Rechnen ohne Zeichnen; grobes Raster 200 × 120 (PSRAM),
      2× in 400 × 240, Einblendungen fein; Sprites, Blickrichtungen, Animationen; Farbreihen per Umfärbe-Tabelle;
      Testorte Wiese und Raum; 11 Bühnen-Tests im Simulator. Web: Lavendel als Standardfarbe, Versionskarte, Karte
      „Orte (Vorschau)“ (Ort, Übergang, Figuren, Rundgang). Eigenes Testbild (Vorschau)
- [x] **0.5.1 Raum und Baukasten** (xhigh) ✅ 2026-10-21, PR #26: Kacheln 8 × 8 grob, 25 × 15 je Ort, drei Ebenen
      (Textzeilen mit Legende, Farbreihe je Ebene); Tiefensortierung nach Fußpunkt (eingebackene Teile nur über bewegten
      neu, pixelgleich geprüft); Raum-Baukasten (Raumliste: Teil, Lage, Stufe, Farbreihe, Besitzer; Erkennungsstück über
      den Besitzer), Ausbaustufen, Zustand „im Bau“ (Wunsch des Users), Möbel-Daten (Art, Ebene, Stellfläche,
      Benutzungspunkte); Hintergrundbild beim Betreten; Wege per Breitensuche; Türen und Bildränder als Ausgänge, Bühne mit
      Reisen und „Blick folgt“; Testorte Wiese, Raum, Platz; Prüfansicht; 18 Bühnen-Tests. Web: Ausbau, Im Bau, Blick
      folgt, Prüfansicht
- [x] **0.5.2 Farbe und Licht** (xhigh) ✅ 2026-10-21: Paletten je Ort mit drei Lichtstufen (80 Grundfarben),
      Farbrampen und `docs/STIL.md`-Entwurf; Tageslicht nach Uhrzeit über die Tageszeit-Schnittstelle (Testuhr bis
      0.5.6), Rollen leuchtend/Himmel/fest, Farbrotation, Lichtkreise weich im feinen Raster, Farbtabellen
      (Schatten, Mischung), Himmelsverlauf und Raster; Verdoppeln direkt aus dem Hintergrund; 22 Bühnen-Tests. Web:
      Tageszeit-Schieber und Zeitraffer. Entscheidungen des Users: warm/gemütlich, natürlich, Nacht mittel,
      Lichtkreis hell bis zur Hälfte
- [x] **0.5.3 Wetter und Partikel** (Hoch) ✅ 2026-10-21: Wetter für alle Orte mit weichen Übergängen (Teststeuerung bis
      0.5.6), Partikel fein mit Obergrenze 512 je Ort (Regen, Schnee, Blätter), Schleier über Farbtabellen (Nebelbänke,
      Regenschleier, Wolkenschatten), Blitz über die Palette, Wetter im Fenster; Lichtquellen an/aus je Ort, Fensterschein
      nur bei Licht innen (Wunsch des Users); 26 Bühnen-Tests. Entscheidungen des Users: Regen Striche, Schnee gemischt,
      Nebel als Schwaden von oben, Blitze gedämpft alle 12–30 s; Überblendung und Raster auch nachts (ähnliche Farben zusammengelegt)
- [x] **0.5.4 Weboberfläche aufräumen** (Hoch, Wunsch des Users): Umfang laut Karte aus dem Planungs-Chat; erst als
      Vorschau-Version `0.5.4-beta.N` im Kanal Vorschau (beta.1 bis beta.15), dann als Release (2026-10-22, PR #34).
      Zusätzlich auf Wunsch: Designs Pixelia und Fahrplan 1850 (Vorschau), „So entstand Pixelia“; Pixel-Uhr weggelassen
- [x] **0.5.5 Asset-Quelle und Grafik-Werkzeug** (xhigh) ✅ 2026-10-22: Asset-Quelle (eingebettet, LittleFS, SD
      vorbereitet) mit Grafik-Speicher im PSRAM (Budget, Anheften, Vorladen, Verdrängen), Laden auf Kern 0, Abblenden
      wartet in der dunklen Mitte; Format PXA1, Zeichenpalette `.gpl`, PNG-Streifen, Zustandsbilder, eigene Farben;
      `tools/asset_pack.py`; Testorte aus PNGs (pixelgleich); Credits-Prüfung in der CI; `logicMax`; SD-Messung
      offen (keine Karte). Ursprünglicher Plan (bisher 0.5.4): Quelle eingebettet /
      LittleFS / microSD, Laden auf Kern 0 beim Ortswechsel hinter dem Abblenden; SD-Karte messen (Tempo, keine
      Aussetzer, Karte ziehen; braucht eine Karte im Gerät). Speicher-Stufen (Entscheidung 2026-10-21): Ablage →
      Grafik-Speicher im PSRAM (Budget, Vorladen der Nachbarorte, Verdrängen) → Zeichnen; Grundausstattung (immer
      geladen) getrennt von Ortspaketen; Kennungen statt Zeiger; gepackt abgelegt; Ladezeit pro Ort messen (Flash,
      LittleFS, SD). **Format für eigene Grafiken** (Paletten-Indizes, Rampen, Platzhalter-Rampe) und Werkzeug
      PNG → Paletten-Indizes; Prüfung von `assets/CREDITS.md` in der CI (fremde Pakete sind nicht geplant, Entscheidung
      2026-10-21); Ablage der Skizzen für das Easter Egg festlegen
- [x] **0.5.6 Schrift, Einblendungen und Touch** (Hoch) ✅ 2026-10-22: Pixel-Schriften mit Umlauten, Kurzmeldungen
      (Warteschlange, Obergrenze pro Stunde), **Info-Karte als Baustein** mit Bild und Text (Figur mit Grund, Gebäude
      „wer ist drin“, später Museum), Pixel-Uhr (Ecke, Format beim User abfragen), Hinweis auf neue Version am Display;
      Touch: Figur oder Teil antippen (Treffer nach Tiefe), Wischen → nächster Ort, langes Halten auf einer Figur →
      „Folgen“, abgestimmt mit der Info-Karte per langem Druck (0.2.1). Der Ereignis-Bus wandert nach 0.5.8. Gebaut: Schnitte fett und klein im Paket „system“, Karte (`overlay.h`) mit Tätigkeit, Ort, Gedanke und „Drin: …“, Kurzmeldungen als Warteschlange (Welt 4 pro Stunde), Pixel-Uhr mit Ortszeit, Treffer nach Tiefe, Wischen, Kamera-Modi (`camera.h`), `/api/camera`, Seiten Display und Welt; 64 Simulator-Tests. Entscheidungen des Users: Karte rechts mit fester Höhe, Knopf „Folgen“ (statt langem Halten) und „Betreten“ bei Türen, Meldung oben, Schrift 5×7 + fett + klein, ohne Symbole vor Tätigkeit und Ort, Uhr oben rechts 14:05 an, Rückkehr nach 10 min (auch nie), Standard Figur oder fester Ort. Am Gerät (64 Figuren, Regen, nachts, Karte, Uhr, Meldung): 29,8 fps, Zeichnen 14,1 ms (max 15,4), Einblendungen 2,8 ms (max 3,2), Logik max 3,1 ms
- [x] **0.5.7 Weltzeit, Kalender und Wetter-Simulation** ✅ 2026-10-26 (Hoch; bisher 0.5.6 „Dienste“, neu gefasst nach
      [SETTING.md](SETTING.md) Kapitel 9): Weltkalender (Tag 1, Jahreszeit 7 echte Tage, einstellbar, Weltjahr 28
      Tage), Dämmerung nach Weltjahreszeit, Zeitfenster (morgens … nachts) als Schnittstelle fürs Tagesgerüst,
      Jahreszeitenwechsel in der Nacht von Sonntag auf Montag mit Jahreszeitenabend als festem Kalendertermin, Erstlinge
      (erster Schnee …) als Ereignis; **echter Mond** (Phase aus dem Datum ohne Internet, sichtbar bei klarem Himmel,
      Vollmond hebt die Nachthelligkeit an, [SETTING.md](SETTING.md) Kapitel 9); **Wetter-Simulation** (viermal am Tag, nach Jahreszeit, Gewitter auch
      mittendrin) füttert die Wetter-Schnittstelle aus 0.5.3; Nachtmodus und Helligkeitskurve; Download von GitHub
      robuster (Abbruch ohne Fortschritt, Weitermachen per HTTP-Range). Entfällt: Standort und echtes Wetter
      (Open-Meteo), weil die Weltjahreszeit gestaucht ist (Frage in [ZUSATZINHALTE.md](ZUSATZINHALTE.md)). Gebaut:
      `calendar.h`, `sky.h`, `weather_sim.h`, Nachtmodus `night.cpp`, `/api/world`, Karte Weltkalender, Simulator
      `--weltjahr` und `--test welt`. Entscheidungen des Users: Mond groß mit Kratern, Jahreszeit-Symbol vor der Uhr,
      sanft dimmen, Meldungen für Abend, Wechsel und Erstlinge; dazu Wetter-Vorschau, Zeile in der Übersicht,
      Nachtzeiten einstellbar

**Phase B (nach dem Setting, Systeme mit Testinhalt im Testdorf)**

- [x] **0.5.8 Orte und Gebiete** ✅ 2026-10-27 (xhigh, PR #42), Teil 1 ✅ 2026-10-26: großes Fenster mit Vorhang und Fensterbank im
      Testraum (Wahl des Users, STIL.md Kapitel 8), Stellplätze im Baukasten (Kerze, Blumentöpfe), Mond ganz in einer
      Scheibe und beim Ortswechsel sichtbar (eigene Mondfarben je Ort), Orte als Module (`place.h`, `worlds/testdorf/`,
      `worlds/gebiete.cpp`), Verbindungen als Daten (Art, Reisezeit, Bedingungen), Orts-Graph, Kammer, Feldweg,
      `--test orte`. Teil 2 ✅ 2026-10-27: Ortszustände mit Zeitbezug (Licht mit Brennstoff, Feuer und Glut, Nässe,
      Füllstand, angefangene Arbeit; `place_state.h`, unsichtbar gleich), Lichtquellen-Arten mit Stärke, Ereignis-Bus
      (`events.h`, Kalender darüber), Fahrzeuge als bewegliche Orte (`vehicle.h`, Schnittstelle), kein Wetter unter
      Tage, Rechenspitzen verteilt, Herd mit Glut, rundes Becken, nasser Boden mit Pfützen, Kerze für Heimkehrer
      (Wahl des Users), Nachprüfung Phase A (Tabelle unten); Zusatzwünsche des Users: Karte mit Zustand, Rauch aus
      dem Schornstein, Fenster orange bei Herdfeuer, `ort cmd zustaende`.
      Ursprünglicher Umfang: **Nachprüfung Phase A** (Liste unten) mit Anpassungen; **Pflicht (Wunsch
      des Users 2026-10-22): jeder Ort ist per Tippen erreichbar und wieder verlassbar** – jede Verbindung (Tür, Weg,
      Bildrand, Treppe, Stolleneingang …) hat am Display eine antippbare Stelle mit Karte (Ziel, wer dort ist,
      „Betreten“/„Hingehen“, seit 0.5.6), und der Orts-Graph ist so geplant, dass von jedem Ort jeder andere in beide
      Richtungen erreichbar ist (Bühnen-Test, wie `--test overlay` für die Testorte); Modul-Schnittstelle
      für Orte und Gebiete (neuer Ort = Ordner, Daten, eine Zeile Registrierung; Schema → Einstellungsseiten);
      Orts-Graph mit **Verbindungen als Daten** (Tür, Bildrand, Weg, Bahnhof, Stolleneingang; Reisezeit; Bedingungen wie
      „nur bei Tag“, „erst nach Bau“, „Werkzeug nötig“); Fahrzeuge als bewegliche Orte (Schnittstelle);
      **Ortszustände mit Zeitbezug** (Feuer und Glut, Licht, Nässe, Füllstand, angefangene Arbeit), die auch unsichtbar
      weiterlaufen; Lichtquellen-Arten (Kerze, Öllampe, Kamin, Kienspan, Laterne, Fackel) mit Stärke; **Stellplätze**
      im Baukasten (sichtbare Bestände, auch auf der Fensterbank: Kerze im Fenster); Ereignis-Bus
- [x] **0.5.9 Weltzustand, Figuren und Speicherstand** ✅ 2026-10-28 (xhigh, PR #45): Weltzustand als Ganzes (Uhr, Wetter, Figuren, Chronik
      mit Zeitstempel); Figuren-Datensatz (Stammdaten, Charakterzüge, Interessen, Fähigkeiten in Stufen mit
      Erfahrungswert, Beziehungen in Stufen, Erinnerungen, Zuhause, Besitz); **Figuren-Baukasten** fürs Aussehen (Ebenen
      mit Farbreihen, 4 Richtungen); jede Figur ist an genau einem Ort oder auf einer Verbindung (Übergabe in einem
      Schritt); **Stufen der Simulation** (voll auf Kern 1 mit 30 Hz, grob auf Kern 0 mit ~1 Hz, Übergaben nur an
      Takt-Grenzen über eine Warteschlange, beim Betreten an die passende Stelle der laufenden Tätigkeit); Speicherstand
      als Ganzes (atomar, versioniert, alte Stände laden weiter); Nachholen nach dem Einschalten (begrenzt);
      Gedächtnis-Bausteine ([LEBENDIGKEIT.md](LEBENDIGKEIT.md) Regel 10). Teil 1 ✅ 2026-10-27: Figuren-Datensatz
      (`figure.h`: 27 Fähigkeiten mit Stufe, Erfahrung und Fehlversuchen, Lernen durch Zeigen, Beziehungen in Stufen,
      Erinnerungen mit Gewicht), Weltzustand mit Aufenthalt je Figur (Übergabe in einem Schritt, `check_figures`),
      Chronik und Besuche (`chronicle.h`), Speicherstand PXW1 mit zwei Plätzen (`save.h`, `world_save.cpp`, LittleFS,
      alle 5 min, Wahl des Users), Nachholen bis 3 Tage (Wahl des Users), `--test speicher` (11 Prüfungen). Teil 2 ✅
      2026-10-27: Stufen der Simulation (`world_sim.cpp`: Tätigkeiten als Zeitblöcke, grob an jeder Takt-Grenze für alle
      Orte, voll dazwischen für die betretenen, beide auf Kern 1; Übergaben nur an Takt-Grenzen; Einsetzen genau an der
      Stelle; Abschnitt TAET), P10 byte-gleich und Neustart mitten in einer Übergabe (`--test speicher`, 13 Prüfungen).
      Teil 3 ✅ 2026-10-27: Lichtkarte in Zeilen mit Budget, Figuren-Baukasten Version 3. Teil 4 ✅ 2026-10-28:
      Weboberfläche (Speicherstand, Chronik, Figurenliste, Sicherung der Welt), Fähigkeiten auf der Karte, Diagnosepaket,
      Prüfungen am Gerät. Teil 5 ✅ 2026-10-28: Figuren mit Statur (Ansatz A, Wahl des Users: 4 Größen, 3 Breiten, Bauch, gebeugt, Strecken beim Zusammensetzen, neue Kleidung, zehn Testfiguren); PR und Release
- [ ] **0.5.10 Warenbuch, Aufgaben und Projekte** (xhigh): Warenbuch für die ganze Welt (Waren, Lagerplätze mit
      Obergrenze, Einzelstücke mit Besitzer und Zustand, Buchungsarten, Reservierung); Holen und Bringen mit
      Tragekapazität, auch in der groben Stufe mit echter Wegzeit; Stellplätze zeigen den Bestand Stück für Stück;
      Reichweite, Sollbestand, Obergrenze, Notbestand; Aufgabenliste der Gemeinschaft; **Projekte mit Teilaufgaben**
      (Material, Fähigkeit, Zwischenstand, Zustand „im Bau“ im Baukasten); Wirtschafts-Protokoll schreiben (CSV, gleiche
      Dateien in Gerät und Simulator, [WIRTSCHAFT.md](WIRTSCHAFT.md) Kapitel 15). Teil 1 ✅ 2026-10-29: Warenbuch-Kern
      (`ledger.h`: Waren, Lagerplätze mit Obergrenze, Einzelstücke, Trage je Figur nach Gewicht, Buchungsarten,
      Reservierung, Bedarf, Reichweite mit Stunden-Fächern, Bilanz `check`), Speicherstand, Herd mit Brennstoff,
      Reihenfolge der Stufen für das Warenbuch, `--test warenbuch`. Teil 2a ✅ 2026-10-29: Aufgabenliste
      (`tasks.h`), Holen und Bringen über mehrere Orte mit Korb und Reservierung, Erzeuger (Hackklotz, Backofen als
      Platzhalter), Soll- und Notbestand mit Gemeinschaftsaufgabe, Platzhalter-Mahlzeiten, Neustart mitten im Transport,
      P10 mit Transporten. Teil 2b ✅ 2026-10-29: Stellplätze Stück für Stück (Holzstapel 40, Holzlege 12, Brotregal 8,
      Brett am Lehmbackofen 6, Rest „hinten“ auf der Karte; Stücke im Hintergrundbild, Neuzeichnen nur des Rechtecks),
      Hackklotz und Lehmbackofen mit Glut beim Backen, Karte eines Lagerplatzes mit echten Daten, Befehl `vorrat`.
      Teil 3a ✅ 2026-10-30: Projekte mit Teilaufgaben (`projects.h`): Material ein Schritt voraus über Holen-Aufgaben,
      Reservierung an der Baustelle, Verbauen mit der Arbeit, Fähigkeit, Arbeitszeit, Wartezeit nach Grund,
      Speicherstand, Chronik; Bett-Projekt in der Kammer statt Ortszustand Arbeit; Neustart mitten im Bau und im
      Transport, P10 mit Bau. Teil 3b ✅: Bau-Bilder in Stufen, schrumpfender Materialstapel, Karte mit Schritt-Leiste.
      Teil 4a ✅ 2026-10-30: Säge als Werkzeug am Projekt (geholt, nicht verbaut, zurück; Handsäge auf der Bauplane),
      Wirtschafts-Protokoll CSV (`econ_log.h`, angehängt mit dem Speicherstand, ≤ 1 MB). **Hinweis:** Zustand und
      Verschleiß der Säge (WIRTSCHAFT 10) kommen mit 0.6.x. Teil 4b: Lager-Karte im Web, Konsole `lager`, Messen, PR
- [ ] **0.5.11 Tagesgerüst, Entscheidung und Arbeitsrhythmus** (xhigh, Simulations-Design): Tagesgerüst mit Zeitfenstern
      und Ritualen (Licht anzünden in der Dämmerung, löschen beim Schlafengehen); Bedürfnisse (Hunger, Energie, Wärme,
      Gesellschaft, Neugier, Hygiene) mit Wünschen und Mangelzuständen; Nutzenbewertung (Wiederholungsbremse, Streuung,
      deterministisch); Arbeitsschritte als Daten mit Blockwahl (Zeitfenster × Dringlichkeit × Bedingungen × Charakter
      × Laune) und Leistungsfaktoren; **Grund jeder Entscheidung in einem Satz**; Tipps als starker Wunsch mit
      sichtbarem Nein-Grund, Eingriffe protokolliert ([SETTING.md](SETTING.md) Kapitel 6, [WIRTSCHAFT.md](WIRTSCHAFT.md)
      Kapitel 8). **Hinweis aus 0.5.10 Teil 2:** Hunger ersetzt den Platzhalter „zwei feste Mahlzeiten je Figur und Tag,
      nur wo Brot liegt“ (`meal_due` in `test_place.cpp`); das Schauen nach Aufgaben (30 % der Entscheidungen, warten
      bis zur Takt-Grenze) und die Wahl der Aufgabe (Gemeinschaft, Weg, Alter) gehen in die Nutzenbewertung ein.
      **Hinweis aus 0.5.10 Teil 3:** Die Blockwahl ersetzt den PLATZHALTER „wer am Projekt arbeitet“ (`claim_project`,
      `build_work` in `test_place.cpp`: wer Tischlern kann, nimmt an einer Takt-Grenze zuerst ein Projekt, Arbeitsgänge
      15–30 Minuten) und die feste Arbeitszeit 7–19 Uhr (`ProjectList::kWorkFrom/kWorkTo`); Projektarbeit wird ein
      Arbeitsschritt mit Zeitfenster, Helfer kommen mit dem Bautrupp in 0.6.x
- [ ] **0.5.12 Nachweis und Gesamtprobe** (Hoch): **Zeichenspitze bei der Überblendung** mit vielen Figuren messen und
      beheben (0.5.9: 48 Figuren je Ort, nachts, Regen bis 28,4 ms, Grenze 20 ms inkl. Logik; Ursache vor allem die
      größeren Figuren aus dem Baukasten, +2,6 bis +6 ms; fps blieben 29,8, Standard ist Abblenden; Wahl des Users
      2026-10-28; Abhilfen: Figuren schneller zeichnen oder bei vielen Figuren abblenden); Zeitraffer ohne Grafik im PC-Simulator (≥ 3 Weltjahre in Minuten,
      mehrere Seeds); **Nachholen mit Transporten** (vorgemerkt 0.5.10 Teil 2, Wahl des Users): nach langer Pause
      Hausvorräte grob aus ihrer Quelle auffüllen und Erzeuger nach Takt herstellen lassen, falls sich „nur Ortszustände“
      mit echten Inhalten falsch anfühlt; **Bilanz-Test** mit Speichern und Neustart mitten in Übergaben; `tools/wirtschaft.ps1` und
      `tools/balance_report.py` mit Ampel P1–P11; **Inhaltszähler**, Blick-Test, Kontaktbogen; CI-Job, der Bericht und
      Kontaktbogen an jeden Welt-PR hängt ([LEBENDIGKEIT.md](LEBENDIGKEIT.md) Kapitel 3); Wirtschafts-Protokoll in der
      Weboberfläche (Diagnose). **Gesamtprobe:** alle Bausteine zusammen im Testdorf (Testfiguren, ein Dutzend
      Testwaren, ein Projekt) und Messung des schlechtesten Falls am Gerät (entscheidet über die Reserve „Display liest
      das grobe Bild direkt“, Backlog)

**Nachprüfung Phase A in 0.5.8** (Wunsch des Users 2026-10-20), mindestens:

| Baustein | Prüfen gegen das Setting | Ergebnis 0.5.8 (2026-10-27) |
|---|---|---|
| Bühne (0.5.0) | Heute rechnen alle Orte mit 30 Hz; laut Setting rechnen unsichtbare Orte grob mit ~1 Hz auf Kern 0, auch bei Hunderten Orten ([SETTING.md](SETTING.md) Kapitel 15) | Vorbereitet: Ortszustände, Kalender und Fahrzeuge laufen schon nach Weltzeit für alle Orte gleich (Bühnen-Test sichtbar/unsichtbar), Ereignisse an Takt-Grenzen. Die Stufen der Simulation (grob auf Kern 0) kommen mit 0.5.9 |
| Sprites (0.5.0) | Figur 28 hoch, 4 Richtungen, Ebenen für den Figuren-Baukasten; Animationen sitzen, liegen, arbeiten, tragen | Unverändert passend (28 hoch, 4 Richtungen); Ebenen und weitere Animationen kommen mit dem Figuren-Baukasten (0.5.9) |
| Raum-Baukasten (0.5.1) | Haus mit mehreren Räumen (Notunterkunft → Raum für Raum), Stellplätze, Benutzungspunkte für Arbeitsschritte, Besitzer und Erkennungsstücke, Ausgänge → Verbindungen mit Bedingungen; Fenster groß genug für Wetter und Fensterbank ([STIL.md](STIL.md) Kapitel 8, heute 2 Kacheln breit) | Erledigt: Haus aus Raum und Kammer (Haushalt `house`), Stellplätze, Verbindungen mit Art, Reisezeit und Bedingungen, großes Fenster mit Fensterbank; Zustandsbilder zeigen Ortszustände (Herd, Becken, Pfützen), „im Bau“ mit angefangener Arbeit |
| Farbe und Licht (0.5.2) | Reichen 80 Grundfarben für echte Orte? Rampen je Ortsart; Lichtquellen-Arten mit Brennstoff statt nur an/aus | Lichtquellen-Arten mit Stärke und Brennstoff erledigt. 80 Grundfarben werden eng: Mond und Glut kosteten zwei Farbreihen der Testfiguren. Für echte Orte eigene Paletten je Ortsart planen (Punkt für den Planungs-Chat, 0.6.0) |
| Wetter (0.5.3) | ein Wetter für die Welt statt je Bühne, kein Wetter unter Tage (Grube); `LightBoard` wird Ortszustand „Licht“ | Erledigt: ein Wetter für die Welt (seit 0.5.7 aus der Wetter-Simulation), unter Tage kein Wetter und kein Himmel (Bühnen-Test), `LightBoard` ist Ortszustand „Licht“, Nässe liest das Wetter |
| Experimentell → Orte (Web) | bleibt Prüfwerkzeug; die Welt bekommt eigene Seiten (0.6.x) | Bleibt so; neue Prüfbefehle `ofen`, `nass`, `becken` über „Befehl an den Ort“ |

**Prüfliste Baukasten und Asset-Quellen** (Wunsch des Users über den Planungs-Chat, geprüft 2026-10-21, vor dem Bau
von Asset-Pipeline und „Raum“). Alle Punkte sind machbar; Kosten bezogen auf die Szene in 200 × 120 groben Bildpunkten.

| Punkt | Wie | Speicher / Rechenzeit | Patch |
|---|---|---|---|
| 1 Innenraum-Baukasten ✅ 0.5.1 | Raum = Datenliste (Teil, Kachel-Lage, Stufe, Farbreihe, Besitzer). Boden und Wände als Kachelebene, Möbel und Deko als Sprites mit Fußpunkt, mit den Figuren nach Tiefe sortiert. Unbewegte Teile beim Betreten einmal in ein Hintergrundbild (24 KB) vorzeichnen | ~6 Byte je Teil (60 Teile < 0,5 KB); pro Bild nur Kopie des Hintergrunds + bewegte Teile, < 1 ms | 0.5.1 |
| 2 Ausbaustufen ✅ 0.5.1 | Teil hat mehrere Stufen-Bilder; Ausbau = Datenänderung in der Raumliste (oder Raumliste je Stufe) | nur die Stufen-Grafiken | Engine 0.5.1, Umbau als Aufgabe nach dem Setting |
| 3 Farbvarianten ✅ 0.5.2 | Umfärbe-Tabelle (256 Byte, Index → Index): Möbel einmal mit einer Platzhalter-Farbrampe zeichnen, die Farbreihe ersetzt die Rampe beim Zeichnen; keine zusätzlichen Grafiken | 256 Byte je Farbreihe; Zeichnen gleich schnell wie ohne (ein Tabellenzugriff pro Pixel) | Zeichnen 0.5.0, Rampen der Palette 0.5.2 + `docs/STIL.md` |
| 4 Auswahl in der Weboberfläche, sichtbarer Umbau | Engine lässt es zu: Raumliste zur Laufzeit änderbar. Umbau mit Spielzeit ist eine Aufgabe der Figur | – | Projekte 0.5.10, Web-Seite 0.7.x |
| 5 Erkennungsstücke ✅ Feld 0.5.1 | Teil mit Besitzer-Kennung bzw. Platzhalter „Erkennungsstück von Bewohner X“ in der Raumliste | 1–2 Byte je Teil | Feld 0.5.1, Inhalte nach dem Setting |
| 6 Haupthaus anpassbar | Kombination aus 1, 3, 4: der Speicherstand hält die Raumliste des Haupthauses (überschreibt die Vorlage) | wenige KB im Speicherstand | 0.7.x (Sykoras Hof) |
| 7 Asset-Quellen ✅ 0.5.5 | Schnittstelle „Quelle“ (eingebettet, LittleFS, später microSD), Laden auf Kern 0 beim Ortswechsel ins PSRAM hinter der Überblendung, nie im Zeichenpfad. SD-Pins 10/11/12/13 sind frei (RGB nutzt 1, 3–9, 14–16, 21, 39–42, 45–48; Licht 2; Touch 19/20/38; Flash/PSRAM 27–37) und sind die Standardpins von SPI2, also volles Tempo | Flash reicht für den Anfang: App 1,5 von 6 MB, LittleFS 3,8 MB frei | 0.5.5 (SD-Treiber und Messung, sobald eine Karte steckt) |
| 8 Eigene Möbel (Format ✅ 0.5.5) | PNG-Streifen; fremde Farben: der User wählt je Möbel zwischen Originalfarben (eigene Farben) und der auf Pixelia-Farben umgerechneten Fassung, beide als Vorschau (Entscheidung des Users 2026-10-22, [STIL.md](STIL.md) Kapitel 9). Browser wandelt PNG → Paletten-Indizes (wie beim Testbild, am Gerät pixelgenau geprüft), Firmware prüft nur Maße und Indizes und speichert in LittleFS (bzw. SD). Kein PNG-Decoder auf dem Gerät | ~1 KB je Möbel (32 × 32) | Format und „Zeichenanleitung für Möbel“ mit `docs/STIL.md` (0.6.0), Upload 0.7.x |

## Systemplan: wann welches System entsteht

Neuplanung nach dem Setting (2026-10-21, Schritt 6 der Setting-Werkstatt). Jedes System aus [SETTING.md](SETTING.md)
und [WIRTSCHAFT.md](WIRTSCHAFT.md) hat hier seinen Platz. **Baustein** = Engine-Code, gebaut in 0.5.x und mit
Testinhalt geprüft. **Inhalt** = Daten, Grafiken und Texte der echten Welt, ab 0.6.x. Regel: erst der Baustein, dann
die Inhaltswellen ([LEBENDIGKEIT.md](LEBENDIGKEIT.md) Kapitel 4). xhigh nur mit Ansage an den User.

| System | Baustein | Inhalt | braucht | Nachweis | Denkstufe |
|---|---|---|---|---|---|
| Asset-Quelle, Grafik-Format und -Werkzeug | 0.5.5 ✅ | Grafiken je Ort ab 0.6.0 | – | Ladezeit je Ort, keine Aussetzer, CREDITS-Prüfung in der CI | xhigh |
| Schrift, Einblendungen, Info-Karte, Touch | 0.5.6 ✅ | Kartentexte ab 0.6.x | – | Gesten-Tests im Simulator, fps am Gerät | Hoch |
| Weltzeit, Kalender, Wetter-Simulation | 0.5.7 ✅ | Feste, Erstlinge ab 0.6.x | – | Zeitraffer über ein Weltjahr: Wetter passt zur Jahreszeit | Hoch |
| Orte, Gebiete, Verbindungen | 0.5.8 | Bahnhof und Umgebung 0.6.x, Dorf und Höfe 0.7.x, Grube 0.8.x, Strecke 0.9.x | 0.5.5 | neuer Ort ohne Umbau, Bühnen-Tests | xhigh |
| Ortszustände (Feuer, Licht, Nässe, Füllstand, Pflanzen, Abnutzung) | 0.5.8 | je Ort | 0.5.7 | gleicher Zustand, ob der Ort sichtbar war oder nicht | xhigh |
| Weltzustand, Speicherstand, Nachholen | 0.5.9 | Start-Speicherstand 0.6.0 | 0.5.8 | Neustart mitten in Übergaben, Stromausfall, alter Stand lädt | xhigh |
| Figuren (Datensatz, Fähigkeiten, Aussehen-Baukasten) | 0.5.9 | 16 Startfiguren 0.6.0, Zuzügler 0.7.x | 0.5.8 | jede Figur an genau einem Ort | xhigh |
| Stufen der Simulation (voll, grob, Nachholen) | 0.5.9 | – | 0.5.8 | P10 gleiche Wirkung: Ort ein- und ausblenden, Differenz 0 | xhigh |
| Bestand und Inventar (Warenbuch, Lager, Einzelstücke, Holen und Bringen, Stellplätze) | 0.5.10 | Lagerplätze und Startvorrat 0.6.x | 0.5.9 | Bilanz-Test exakt, auch über Neustarts | xhigh |
| Aufgaben und Projekte (Teilaufgaben, „im Bau“) | 0.5.10 | Baupläne ab 0.6.x | 0.5.9 | angefangene Arbeit geht am selben Stand weiter, Material genau einmal | xhigh |
| Tagesgerüst und Entscheidungslogik | 0.5.11 | Rituale und Gewohnheiten ab 0.6.x | 0.5.7, 0.5.10 | keine zwei gleichen Tage, Grund auf jeder Info-Karte | xhigh |
| Bedürfnisse | 0.5.11 | Werte ab 0.6.x | 0.5.9 | Zeitraffer: niemand dauerhaft im Mangel | xhigh |
| Berufe und Arbeitsrhythmus | 0.5.11 | Grundberufe 0.6.x, die übrigen 0.7.x | 0.5.10 | P5 Auslastung, `arbeit_tag.csv` | Hoch |
| Nachweis (Zeitraffer, Bilanz-Test, P1–P11, Inhaltszähler, Kontaktbogen) | 0.5.12 | läuft in jedem Welt-PR | 0.5.10, 0.5.11 | läuft selbst in der CI | Hoch |
| Wirtschaft und Ertragsregel (Ketten, Verbrauch, Verderb, Notlösungen, Standardsuppe, Werkzeuge, Natur, Händlerin) | Regeln 0.5.10/0.5.11 | Grundkreisläufe 0.6.x, Ausbau 0.7.x, Grube 0.8.x, Lok 0.9.x | 0.5.12 | **Zeitraffer ≥ 3 Weltjahre, ≥ 3 Seeds, P1–P11 grün (Pflicht vor jedem Merge)** | Hoch |
| Licht als System (Fensterlicht) | `LightBoard` ✅ 0.5.3, Lichtquellen-Arten 0.5.8, Rituale 0.5.11 | Brennstoff-Kette 0.6.x | Warenbuch | P4 (Kienspan ≤ 20 %); nachts leuchten nur Fenster, hinter denen jemand wach ist und Brennstoff hat | Hoch |
| Bauen im Dorf (Notunterkunft, Räume, Bautrupp, Wunschliste, Ausbaustufen, Großbauten) | Projekte 0.5.10 | Notunterkünfte und Räume 0.6.x; Wunschliste, Ausbaustufen, Großbauten, Sykoras Haus 0.7.x | 0.5.10 | P6 Bautempo | Hoch |
| Beziehungen und Romanze | Modell 0.5.9 | Begegnungen und Lernen 0.6.x; Romanze, Bitten, Vertrauen und Wahl 0.7.x | 0.5.11 | ≥ 3 Beziehungsänderungen in 30 Tagen | Hoch |
| Gedächtnis (Regel 10) | Speicherstand, Erinnerungen 0.5.9; Bestände, angefangene Arbeit 0.5.10 | Erinnerungen, Erstlinge, Gaben ab 0.6.x | – | Bilanz-Test, Besuchsverlauf vollständig | xhigh |
| Ereignisse und Regie (Regisseur, Kamera-Modi, Zuschauzeiten) | Regisseur mit der ersten Ereigniswelle 0.6.x | Wellen in jedem Welt-Patch; besondere Ereignisse über die Weboberfläche 0.7.x | 0.5.6, 0.5.11 | Inhaltszähler, Wiederholungsquote, Verteilung nach Seltenheit | xhigh (Regisseur), sonst Hoch |
| Gedanken | Vorlagen mit Platzhaltern 0.6.x | ≥ 300 Vorlagen bis Ende 0.7.x | 0.5.6, 0.5.11 | Inhaltszähler | Hoch |
| Briefe | Ansicht 0.7.x | nach der Beispieldatei des Users | Händlerin, Beziehungen | Inhaltszähler | Hoch |
| Tiere (Katze, Nutztiere, Wildtiere, Bienen) | nutzt Figuren und Entscheidung | Katze, Herde, Hühner 0.6.x; Wildtiere, Bienen 0.7.x | 0.5.11 | ≥ 10 Tierarten mit eigenem Verhalten | Hoch |
| Museum | Funde über das Warenbuch, Info-Karte | komplett 0.7.x (Stufen 1–3, mehr als 45 Stücke), Grubenfunde 0.8.x | 0.5.6, 0.5.10 | echte Angaben mit Quellen im PR | Hoch |
| Weboberfläche der Welt | – | Bewohner, Lager, Tagebuch, Verpasst 0.6.x; Wunschliste, Projekte, Ereignisse, Tipps, Briefe, Museum 0.7.x | jeweiliges System | Screenshots im PR | Hoch |
| Stil und Grafiken | Format und Werkzeug 0.5.5 | `docs/STIL.md` fertig und Figuren-Aussehen 0.6.0, dann je Ort | 0.5.5 | Sichtprüfung durch den User | Hoch |

**Wo alles zusammenläuft:**

1. **0.5.12 Gesamtprobe (Technik):** Alle Bausteine laufen zum ersten Mal zusammen, im Testdorf mit Testinhalt.
   Zeitraffer und Bilanz-Test sind grün, der schlechteste Fall ist am Gerät gemessen.
2. **0.6.0 Der erste Tag:** Die echte Welt startet mit Bahnhof, gestrandetem Zug, 16 Bewohnern und Restvorrat. Ab hier
   wächst jedes System über Inhalte und kleine Erweiterungen.
3. **Abnahmen:** Ende 0.6.x Kapitel 1, Ende 0.7.x das Dorf (Mindestziele, Langzeit-Bericht), Ende 0.8.x und 0.9.x.
4. **1.0.0:** Alle Kapitel sind verbunden (Dorf → Grube → Lok → Fahrt), der Endausbau ist im Zeitraffer erreichbar,
   72-h-Dauertest bestanden.

## Abnahme der Phasen

Jede Phase ab 0.6.x endet erst, wenn die **Mindestziele aus [LEBENDIGKEIT.md](LEBENDIGKEIT.md) Kapitel 2** erfüllt und
per Langzeit-Bericht belegt sind. In jeder Phase wird hier der Stand „Ist → Ziel“ gepflegt. Zu Beginn jeder Phase
entsteht der Ideenspeicher `docs/welten/<kapitel>.md`. Die Mindestziele für Kapitel 1 stehen fest
(2026-10-21), die Dorf-Ziele gelten für Ende 0.7.x („Zuzug“ statt „Zu- oder Wegzug“). Für Grube, Lok und die Welt als
Ganzes legt der User sie zu Beginn der jeweiligen Phase fest ([SETTING.md](SETTING.md) Kapitel 18, Punkt 6).

## 0.6.x – 🚉 Kapitel 1 „Der erste Frühling“

Spielstart einige Wochen nach dem Entschluss zu bleiben ([SETTING.md](SETTING.md) Kapitel 1). Gebiete: Bahnhof mit
gestrandetem Zug und Umgebung. Die Bausteine stehen in dieser Reihenfolge; den Schnitt in Patches (je < 250 K) legt
der Planungs-Chat zu Beginn der Phase fest.

- [ ] **0.6.0 Der erste Tag:** Mindestziele Kapitel 1 stehen fest ([LEBENDIGKEIT.md](LEBENDIGKEIT.md) Kapitel 2), Ideenspeicher `docs/welten/kapitel-1.md`;
      `docs/STIL.md` fertig (Referenz: die Küche aus [Skizze v4](skizzen/massstab-v4.html), Rampen je Ortsart,
      Figuren-Aussehen, Format für eigene Möbel); 16 Startfiguren mit Aussehen und Stammdaten; erste Orte des Bahnhofs
      (Bahnsteig mit Feuerstelle, Warteraum, Speisewagen als Taverne); Start-Speicherstand mit Restvorrat
- [ ] Bahnhof komplett: Abteilwagen, Viehwagen als Stall, Güterschuppen als Lager, Lokschuppen mit verrosteter Lok
- [ ] Umgebung: Waldrand mit Marthas Hütte, Wald mit Holzfällerhütte, See mit Hannes' Hütte, Wiesen, Lichtung, Lehmgrube
- [ ] Tagesablauf: Bedürfnisse, Rituale, Arbeitsrhythmus der Grundberufe (Waldarbeiter, Bäckerin, Wirt, Schäferin,
      Jäger, Fischer, Kräuterfrau, Schmied, Tischler)
- [ ] Grundwirtschaft: Wärme, Brot, Mahlzeiten und Standardsuppe, Milch, Werkzeuge, Natur-Bestände; Zeitraffer grün
- [ ] Licht als System: Kerzen, Lampenöl, Kienspan, Kamin; Fenster leuchten nur, wo jemand wach ist und Brennstoff hat
      (Regeln: [SETTING.md](SETTING.md) Kapitel 9, Lichtritzen, Laternen, echter Mond)
- [ ] Bauen: Notunterkünfte und erste Räume (Bautrupp, Tischler reihum, Material reserviert, „im Bau“ sichtbar),
      Tagebuch des Dorfs
- [ ] Abende in der Taverne im Speisewagen, Jahreszeitenabende (sonntags; Sommerfeuer, Lichterabend); Ereignis-Regisseur mit erster Ereigniswelle; Kamera-Modi (Sykora, Frei,
      Folge X); Kurzmeldungen
- [ ] Gedanken mit Grund auf der Info-Karte; die Katze (taucht nach einigen Tagen auf), Taylors Herde, Hühner
- [ ] Weboberfläche der Welt: Bewohner, Lager (Füllstand, Reichweite, Chronik), Tagebuch, Verpasst-Übersicht,
      Wirtschafts-Protokoll; Handy-Leiste neu ordnen
- [ ] Abnahme Kapitel 1 (Langzeit-Bericht)

## 0.7.x – 🏘️ Kapitel 2 „Das Dorf wächst“ (Schwerpunkt)

Meilensteine „Die ersten Häuser“ und „Die Wahl“ ([SETTING.md](SETTING.md) Kapitel 1). Das Dorf ist der Schwerpunkt
und bekommt die meisten Patches.

- [ ] Dorf Pixelia: Dorfplatz mit Brunnen und Schwarzem Brett, Häuserzeilen, Werkstätten (Schmiede, Tischlerei,
      Bäckerei), Vorratsscheune
- [ ] Bauernhof von Otto und Ida (Felder, Scheune, Hühnerstall, später Wassermühle) und Sykoras Hof
- [ ] Wunschliste mit Emils Rat in der Weboberfläche, Bonuswünsche mit echter Wirkung, Ausbaustufen (Fachwerk,
      Steinhaus, Glasfenster, Federbett, Badehaus), Großbauten
- [ ] Wirtschaft im Ausbau: Ernte und Jahreslauf, Händlerin Dora über den Bergpass (Tauschkiste, Post), Stoff, Leder,
      Kipppunkte und Zuzugsliste; Zeitraffer grün
- [ ] Beziehungen: voneinander lernen, Bitten an andere, Romanze (Wintererinnerung, das verlorene Lamm, der erste Tanz),
      Sykoras Schwäche
- [ ] Die Wahl: Vertrauen der Bewohner in Sykora, Wahl auf dem Dorfplatz, danach ordnet der Betrachter die Wunschliste
- [ ] Zuzug über den Bergpass: Neue ziehen in fertige Häuser, Notunterkunft → Raum für Raum
- [ ] Besondere Ereignisse über die Weboberfläche (Voraussetzungen, Zeitfenster, Vorbereitung, Schmuck),
      Zuschauzeiten, Feste
- [ ] Briefe mit Text (Ansicht nach der Beispieldatei des Users), Gedanken-Verlauf im Web
- [ ] Tiere: Wildtiere mit Vertrauensstufen, Emils Bienen
- [ ] **Museum komplett** (Entscheidung des Users 2026-10-21): Kuriositätenregal → Naturalienkabinett → Museum am
      Dorfplatz, Sammlung mit mehr als 45 Stücken (wie viele realistisch sind, prüft die Sitzung), Infokarten mit
      echten Angaben und Quellen, Museumsseite in der Weboberfläche
- [ ] Sykoras Haus anpassbar in der Weboberfläche; eigene Möbel des Users hochladen
- [ ] Sonderorte (Grabstelle, Planwagen, Ruine) und der Anfang des langen Rätsels
- [ ] Abnahme Dorf ([LEBENDIGKEIT.md](LEBENDIGKEIT.md) Kapitel 2)

## 0.8.x – ⛏️ Kapitel 3 „Die alte Grube“

Reihenfolge Grube vor Lok vom User bestätigt (2026-10-21): Die Lok braucht Kohle und Blei aus der Grube.

- [ ] Berghang: Steinbruch, Aussichtspunkt (mit Wilhelm)
- [ ] Die alte Grube: Stolleneingang, Abschnitte, Kohleflöz, tiefe Sohle; jedes Tor ist im Bild (Licht, Spitzhacke,
      Stahlwerkzeug, Stützbalken, Leiter und Seil, Pumpe, Lore)
- [ ] Expeditionen mit einer kleinen Gruppe, kein Kampf; Spannung aus Licht, Wasser und Gestein (Grubenlampe als
      Lichtkreis)
- [ ] Neue Waren: Erz, Kohle, Blei, Ton; Rennofen; Zeitraffer grün
- [ ] Funde: Mineralien und Fossilien fürs Museum, Teile des Rätsels der Grabstelle
- [ ] Abnahme (Mindestziele Grube, zu Beginn der Phase festlegen)

## 0.9.x – 🚂 Kapitel 4 „Die Lok“

- [ ] Lok in 13 Teilaufgaben ([SETTING.md](SETTING.md) Kapitel 8), sichtbarer Zwischenstand im Lokschuppen
- [ ] Die Strecke: zugewachsene Gleise freilegen, Brücke, verschütteter Tunnel nach hinten
- [ ] Abteil im fahrenden Zug als beweglicher Ort: Landschaft und Wetter am Fenster (aus dem früheren Zugfenster)
- [ ] Erste Fahrt bis zur Nachbarstadt außerhalb des Bildes: Waren, Gäste, Briefe; Neue kommen danach mit dem Zug
- [ ] Abnahme (Mindestziele Lok und Strecke)

## 1.0.0 – Pixelia 1.0

- [ ] Alle Kapitel verbunden: Dorf, Grube, Lok, Fahrt; Endausbau (~20 Häuser, 30–40 Bewohner) im Zeitraffer erreichbar
- [ ] Alle Punkte aus „Vor 1.0.0 klären“ ([ZUSATZINHALTE.md](ZUSATZINHALTE.md)) entschieden
- [ ] Mindestziele „Welt als Ganzes“ (ersetzen „Verbindungen zwischen Welten“)
- [ ] Feinschliff, Leistungsoptimierung
- [ ] 72-h-Dauertest ohne Neustart
- [ ] README mit Bildern/GIFs

## Nach 1.0: Zusatzinhalte

Neue Regionen mit dem Zug (Marktstadt, Küste, Bergdorf), der freie Tunnel und weitere Updates:
[ZUSATZINHALTE.md](ZUSATZINHALTE.md).

## Frühere Welt-Ideen

Bis 2026-10-20 waren getrennte Welten geplant ([PROJEKTPLAN.md](PROJEKTPLAN.md) Kapitel 6, frühere Konzepte). Was
davon weiterlebt:

| Frühere Idee | lebt weiter als | wann |
|---|---|---|
| 🚂 Zugfenster (Landschaft am Abteilfenster, Parallax, Regen an der Scheibe, Bahnhöfe, „Nächster Halt“) | Abteil im fahrenden Zug; Wetter im Fenster gibt es schon (0.5.3); Bahnhöfe der neuen Regionen | 0.9.x, Regionen nach 1.0 |
| 🏘️ Pixel-Dorf | das Herz der Welt | 0.6.x–0.7.x |
| ⚔️ Dungeon-Held (Erkundung, Fackelschein, Nebel, Beute) | die alte Grube ohne Kampf: dunkle Stollen, Grubenlampe, Fundstücke statt Beute | 0.8.x |
| 🏝️ Inselwelt (ein Bewohner baut sich ein Zuhause, exaktes Gedächtnis, Lagerübersicht) | Gedächtnis und Lager-Seite für die ganze Welt (Regel 10); als eigener Ort offen | 0.5.10, 0.6.x; Ort: [ZUSATZINHALTE.md](ZUSATZINHALTE.md) |
| Aquarium, Ameisenfarm (spätere Welten) | Aquarium und Terrarium im Museum | 0.7.x |
| Bauernhof (spätere Welt) | Bauernhof von Otto und Ida | 0.7.x |
| Leuchtturm (spätere Welt) | Region Küste | nach 1.0 |
| Uhr/Kalender-Overlay | Pixel-Uhr | 0.5.6 |

Nicht weiter verfolgt, weil es nicht zum Setting passt (um 1850, gemütlich, keine Magie, kein Kampf, kein Geld,
niemand altert, stirbt oder zieht weg): Pixel-Liga mit FC Pixelia, Spielengine, Saison und Wappen-Editor ([Vorschlag]
Spiele auf dem Dorffest wie Tauziehen oder Kegeln als Ereignisse); Kampf, Gegner, Bosse, Klassen und Gold des
Dungeon-Helds; Raumstation und Wetterstation-Ansicht; Dorf-Ideen wie Wegzug, Altern, UFO, Eiswagen, Feuerwerk über
der Stadt.

---

## Backlog – Wünsche und Ideen

Neue Wünsche des Users landen hier (mit Datum) und werden einer Version zugeordnet.

| Datum | Wunsch / Idee | Geplant für |
|---|---|---|
| 2026-10-30 | Säge als Werkzeug am Projekt: geholt, nicht verbaut, danach zurück (Wunsch des Users) | 0.5.10 Teil 4 |
| 2026-10-30 | Zeichen mit Hammer und Fortschritt über einer Baustelle (Variante C, „im Hinterkopf behalten“) | offen, mit den echten Baustellen 0.6.x prüfen |
| 2026-10-22 | Asset-Update: Ortspakete getrennt von der Firmware nach LittleFS bzw. auf die SD-Karte bringen (z. B. von GitHub wie das Firmware-Update), damit große Orte nicht eingebettet sein müssen (Befund 0.5.5: Eingebettetes liegt wegen XIP im PSRAM) | Vorschlag Claude, sobald echte Orte die Firmware sprengen (spätestens 0.7.x) |
| 2026-10-18 | Weitere Welten später: Aquarium, Raumstation, Ameisenfarm, Leuchtturm, Bauernhof | Frühere Welt-Ideen (oben): Aquarium und Ameisenfarm im Museum, Bauernhof (0.7.x), Leuchtturm mit der Küste (nach 1.0); Raumstation passt nicht |
| 2026-10-18 | Optional Lichtsensor BH1750 nachrüsten (lernende Auto-Helligkeit) | Vor 1.0 klären ([ZUSATZINHALTE.md](ZUSATZINHALTE.md)) |
| 2026-10-18 | Optional Ton über I²S-Verstärker nachrüsten. **Neu:** Board hat Lautsprecher-Anschluss „Speak“ (P6) mit Verstärker U4, prüfen | Vor 1.0 klären ([ZUSATZINHALTE.md](ZUSATZINHALTE.md)) |
| 2026-10-18 | Leistungsmonitor (FPS, Last, Speicher) in der Weboberfläche, Log per Schalter unter „Experimentell“ ein/aus (Idee des Users) | ✅ 0.4.2 (Seite Live); Log-Schalter offen |
| 2026-10-18 | Display-Drehung 0°/180° als Systemeinstellung, Aufstellort steht noch nicht fest (Wunsch des Users) | ✅ 0.2.0 (NVS, Einrichtungsseite, `rot`), 0.3.x (richtige Oberfläche) |
| 2026-10-18 | Engine: Bounce-Callback mit SIMD (PIE) beschleunigen oder intern rendern und ins PSRAM kopieren, falls Welten mehr Zeit brauchen | ❌ 0.5.2 gemessen: SIMD/PIE und DMA bringen beim Verdoppeln nichts (Entscheidungsprotokoll) |
| 2026-10-21 | Engine: Display liest das grobe Bild direkt (4× im Bounce-Callback), feine Einblendungen nur in ihren Zeilen: spart 96 KB PSRAM-Schreiben je Bild (2,4 ms). SIMD/PIE und DMA bringen beim Verdoppeln nachweislich nichts (Messung 0.5.2) | Entschieden 2026-10-21 (Neuplanung): vorerst nicht, Zeichenweg bleibt; Reserve, falls ein echter Ort im schlechtesten Fall über 16 ms braucht (Messung 0.5.12) |
| 2026-10-18 | RAM: kurzer Einbruch von ~30 KB internem RAM beim Start und beim Anlegen der Engine untersuchen | ✅ 0.2.0 geklärt: DMA-Pool von ESP-IDF meldet Tiefstwert 0, kein echter Verbrauch ([HARDWARE.md](HARDWARE.md)) |
| 2026-10-18 | Helligkeitskurve vermessen (gleichmäßig wahrgenommene Stufen, kleinster noch leuchtender Wert) | ✅ 0.5.7 Kurve gebaut (Exponent 2,2); kleinste Pulsbreite am Display prüfen (`blduty`, `bldmin`) |
| 2026-10-18 | **Tiefe statt Kulisse:** Welten sollen wirklich leben, mit Features und Ereignissen statt Dauerschleife. Umgesetzt als verbindliche [Lebendigkeits-Charta](LEBENDIGKEIT.md) mit messbaren Zielen (Wunsch des Users) | alle Welten, Abnahmekriterium |
| 2026-10-18 | WLAN gleich im Web-Flasher eintragen (Improv Serial, ESP Web Tools unterstützt das), dann entfällt der Umweg über den Setup-Modus | 0.3.x |
| 2026-10-18 | Setup-Modus und neues Setup-Passwort auch per Touch-Geste am Gerät (zusammen mit Werksreset per Touch) | 0.3.x |
| 2026-10-18 | Upload-Hänger bei schwachem WLAN (User-Test): behoben durch 32-KB-TCP-Fenster und SACK; Seite meldet 30 s ohne Fortschritt | ✅ 0.2.0 |
| 2026-10-18 | **Neue Welt „Inselwelt“** (cozy, eigenständig, ein Bewohner baut Haus, Hof, Boot, später Hofladen). Rohkonzept gesichert in [welten/inselwelt-konzept.md](welten/inselwelt-konzept.md), wird später für 5" überarbeitet und gemeinsam weiter geplant (Wunsch des Users) | Vor 1.0 klären ([ZUSATZINHALTE.md](ZUSATZINHALTE.md)); Ideen Gedächtnis und Lager sind übernommen |
| 2026-10-18 | **Zuverlässiges Gedächtnis** (vor allem Inselwelt, aber für alle Welten): exakte Bestände (welche Waren, wie viele, wo), angefangene und unbeendete Arbeiten, wer schon zu Besuch war; nichts verdoppelt oder verschwindet, auch nicht bei Neustart; Bilanz-Test im Langzeit-Test. In LEBENDIGKEIT.md als Regel ergänzen | ✅ Regel 10 in LEBENDIGKEIT.md; Bausteine 0.5.9 und 0.5.10, Bilanz-Test 0.5.12 |
| 2026-10-18 | **Entscheidung Hauptwelt:** Inselwelt, Pixel-Dorf oder Dungeon-Held als Schwerpunkt der Software. Gemeinsam planen und diskutieren | ✅ entfällt: gemeinsames Setting, das Dorf ist das Herz ([SETTING.md](SETTING.md)) |
| 2026-10-18 | **Dorf als Aufbau-Wirtschaft:** begrenzte Rohstoffe, die das Dorf selbst gewinnen muss; Bauprojekte verbrauchen Material exakt; neue Häuser müssen gebaut werden, damit neue Bewohner kommen; alles dauerhaft gespeichert (Ideen aus der Inselwelt übernehmen) | Warenbuch und Projekte 0.5.10, Inhalte 0.6.x und 0.7.x (Systemplan) |
| 2026-10-18 | **Mehrere Darstellungs-Vorschläge** für Dorf und Inselwelt (Perspektive, Ausschnitt, Figurengröße, Kamera), passend zum 5"-Display, mit Simulator-Mockups, bevor gebaut wird | ✅ Perspektive entschieden 2026-10-21 (Skizzen v1–v5) |
| 2026-10-18 | **Systemschrift** (5×7, `engine/src/font.cpp`, Setup-Bildschirm) gefällt dem User: beibehalten, Grundlage für den Stil-Leitfaden | ✅ im Entscheidungsprotokoll festgehalten; STIL.md in 0.5.x |
| 2026-10-18 | **Ausführliche Changelogs** im Stil der PR-Beschreibungen: Neu, Geprüft am Gerät (mit Messwerten), gefundene und behobene Probleme, Hinweise. Alle Änderungen inkl. Fixes seit 0.1.0. Format um Felder „Geprüft“ und „Hinweise“ erweitern, 0.1.0/0.2.0 ausführlich nachtragen (aus PR #1, #3, #4, #5), `tools/release_notes.py` anpassen, Regel in CLAUDE.md. Anzeige später unter „Versionsverlauf“ | ✅ 0.2.1 (Format, Nachtrag, CI-Prüfung, Regel); Anzeige 0.4.x |
| 2026-10-18 | **IP jederzeit sehen:** langer Druck (3 s) aufs Display → Info-Karte mit IP, `pixelia.local`, WLAN-Name, Signal und QR-Code zur Weboberfläche; zusätzlich IP nach jedem Verbinden kurz einblenden; Geste in der Weboberfläche einstellbar | ✅ 0.2.1 (Geste, Info-Karte, Kurzmeldung, Dauer im NVS); 0.3.x (einstellbar in der Oberfläche) |
| 2026-10-18 | **Arbeiten von unterwegs:** Cloud-Sitzungen über claude.ai bzw. die Claude-App (GitHub einmal verbinden), Remote Control, um lokale Sitzungen vom Handy aus weiterzuführen, wenn der PC läuft. Rückweg immer über Pull Requests auf GitHub | bei Bedarf einrichten |
| 2026-10-18 | **USB optional machen:** Updates per WLAN als Werkzeug (`tools/ota.ps1`), Log und Diagnose per WebSocket (`tools/netlog.ps1`) | ✅ 0.2.1 (`tools/ota.ps1`); ✅ 0.2.2 (`tools/netlog.ps1`, `tools/crash.ps1`) |
| 2026-10-19 | **Fernwartung für den Notfall:** Daten, Fehler und Logs komplett per WLAN abrufbar (Board hängt normal per USB am PC), Diagnosepaket, das der User vom Handy aus schicken kann (Wunsch des Users) | ✅ 0.2.2 |
| 2026-10-19 | Task-Liste mit Stack-Reserven ohne Display-Aussetzer (eigene Messung außerhalb der Kernel-Sperre), Grundlage für eine laufende Task-Anzeige | 0.4.x bei Bedarf |
| 2026-10-19 | **Inselwelt: Lagerübersicht in der Weboberfläche** – welche Waren und Gegenstände der Bewohner hat, wie viele, wo sie gelagert sind (Lagerplatz, Kapazität). Genaue Ausarbeitung folgt gemeinsam (Idee des Users). In [welten/inselwelt-konzept.md](welten/inselwelt-konzept.md) vermerkt | Seite „Lager“ für die ganze Welt 0.6.x |
| 2026-10-19 | **Kürzere Changelogs:** technisch, aber höchstens 220 Zeichen pro Stichpunkt und 24 pro Version, Details in die PR-Beschreibung; 0.2.1 und 0.2.2 gekürzt (Wunsch des Users) | ✅ 0.3.0 (Regel, `--check`) |
| 2026-10-19 | **Arbeitsweise mit Sitzung pro Version** und Übergabe übers Repo, damit eine Version nicht das ganze Nutzungslimit braucht ([ARBEITSWEISE.md](ARBEITSWEISE.md)) | ✅ 0.3.0 |
| 2026-10-20 | **Gemeinsames Setting** (Idee des Users): statt getrennter Welten eine große gemeinsame Welt. Zugfahrt, Dungeon usw. sind Module bzw. Orte, die später nachgereicht werden. Figuren wechseln zwischen den Orten (z. B. eine Person geht in den Dungeon, dort sieht man die Idle-Dungeon-Ansicht). Immer nur ein Ort sichtbar und voll simuliert, die anderen laufen vereinfacht („inaktiv“) im Hintergrund weiter. Wird im Planungs-Chat ausgearbeitet und ersetzt dann die Frage „Hauptwelt vor 0.6.0 festlegen“ | ✅ [SETTING.md](SETTING.md); Neuplanung 2026-10-21 (Systemplan, Phasen ab 0.6.x) |
| 2026-10-20 | **Erste Phase von 0.5.x nachprüfen** (ausdrücklicher Wunsch des Users): Alles, was in der ersten Phase von 0.5.x gebaut wird, wird erneut geprüft und an das Setting angepasst, sobald dieses feststeht | 0.5.8 (Liste „Nachprüfung Phase A“) |
| 2026-10-20 | **Download von GitHub robuster:** Bei einem WLAN-Aussetzer (`bcn_timeout`) hing der Download 0.4.1 → 0.4.2 vier Minuten bei 432 KB und brach dann ab (alte Version blieb, zweiter Versuch 20,8 s). Ideen: Abbruch nach ~30 s ohne Fortschritt mit klarer Meldung, automatisch weitermachen per HTTP-Range ab dem letzten Stück | ✅ 0.5.7 (am Gerät mit `update abriss` geprüft) |
| 2026-10-20 | **Hinweis auf eine neue Version auch auf dem Display** (gefunden von der Prüfung im Hintergrund), Entscheidung des Planungs-Chats | 0.5.6 (Einblendungen) |
| 2026-10-20 | Automatische Installation neuer Versionen ohne Nachfrage | ❌ entschieden: nicht, auch nicht als Option; Hinweis reicht (Entscheidung des Users) |
| 2026-10-20 | **Perspektive vor der Engine wählen** (Wunsch des Users): Beispiele, wie die Welt dargestellt wird (Seitenansicht mit Parallax, Querschnitt, 3/4-Draufsicht, isometrisch, seitliche Bühne mit Tiefe), Hof mit Haus, Fluss, Weide bei Tag und Nacht, dazu ein Ortswechsel Hof → Haus → Zug. Erst danach Tilemaps, Kamera, Parallax, Tiefensortierung. Je Ort eigene Perspektive möglich | ✅ 2026-10-21 (3/4-Draufsicht) |
| 2026-10-20 | **Modular wachsende Welt** (Wunsch des Users): Start im Dorf auf einem Hof (Haus, Fluss, Weide als eigene Szenen), später andere Gebäude oder mit dem Zug in ein anderes Dorf; neue Orte (z. B. Zug) kommen als Module per Update dazu und fügen sich ohne Umbau ein | Grundstein ✅ 0.5.0; Orte und Gebiete als Module 0.5.8 |
| 2026-10-20 | **Standardfarbe der Oberfläche ab Werk „Lavendel“** (#7a5aa6) statt „Moos“; alle sechs Farben bleiben wählbar, eine gespeicherte Wahl bleibt erhalten. Farbstil bleibt „Tonal“ (Rückblick-Runde) | ✅ 0.5.0 (auch das Symbol im Browser-Tab) |
| 2026-10-20 | **Seite System: Versionskarte** ganz oben mit „Pixelia“ und darunter „Version X.Y.Z“ (installierte Version), weil die Version heute schwer zu finden ist | ✅ 0.5.0 |
| 2026-10-20 | **Display um 180° drehen** für den Einbau mit USB oben, Touch mitgedreht, Live-Vorschau richtig herum | gibt es schon seit 0.2.0 (Seite System → Anzeige → „Bilddrehung“; Touch wird mitgedreht, die Vorschau kopiert das Bild vor der Drehung); Beschriftung „Bilddrehung“ bleibt (Entscheidung des Users 2026-10-21) |
| 2026-10-20 | **Uhrzeit-Einblendung:** kleine Pixel-Uhr in einer Ecke des Displays, abschaltbar; Ecke, 24-h-Format, Sekunden als Auswahl mit Empfehlung beim User abfragen | ✅ 0.5.6 (oben rechts, 14:05, an; in der Weboberfläche einstellbar) |
| 2026-10-20 | Handy-Leiste unten vorerst unverändert; **neu ordnen, sobald Menüpunkte zu Pixelia bzw. dem Dorf dazukommen** (Wunsch des Users) | mit den ersten Welt-Seiten (0.6.x) |
| 2026-10-20 | **Eigenes Testbild am Display** (Skizzen am echten 5″-Display ansehen): Seite Experimentell, Bild wählen → 2× auf dem Display bis Tippen oder „Beenden“ (Entscheidungen des Users: Ort Experimentell, Dauer bis Tippen, andere Größen pixelgenau anpassen) | ✅ 0.5.0 (am Gerät gemessen; Anzeigen und Tippen zum Beenden vom User geprüft, 2026-10-21) |
| 2026-10-21 | **Perspektive entschieden** (Skizzen v1–v5, `docs/skizzen/`): 3/4-Draufsicht für alle Orte, feste Kamera, ein Bildschirm pro Bereich oder Raum, Wechsel per Sprung mit Überblendung, kein Scrollen; Szene in 200 × 120 groben Bildpunkten (2× in 400 × 240), Kachel 8 × 8 grob, Chibi-Figur 28 Bildpunkte, 4 Richtungen | ✅ 0.5.0 und 0.5.1 |
| 2026-10-21 | **Austauschbare Asset-Quelle** (Setting-Werkstatt, vom User beschlossen): Grafiken aus Flash oder SD-Karte, laden nur beim Ortswechsel auf Kern 0 ins PSRAM hinter einer Überblendung; SD-Karte vorher messen (Tempo, keine Aussetzer, Karte ziehen); Speicherstand bleibt in LittleFS | 0.5.5 (Asset-Quelle) |
| 2026-10-21 | **Innenraum-Baukasten** mit Ausbaustufen (Notunterkunft → Raum für Raum), Farbreihen per Palettentausch (auch für das Haus der Hauptfigur über die Weboberfläche), Möbel-Daten (Art, Stellfläche, Benutzungspunkte); Ziel ~20 Häuser. Dazu (Ideen des Users): **Erkennungsstücke** an eine Figur gebunden (Gitarre an der Wand, Geweih beim Jäger); **Haupthaus anpassbar** (Typ, Farbe, Couch-Auswahl) in der Weboberfläche, die Figur baut bzw. streicht dann sichtbar um und braucht dafür Spielzeit. Prüfergebnis siehe 0.5.x „Prüfliste“ | Engine ✅ 0.5.1; Umbau als Projekt 0.5.10, Web-Auswahl 0.7.x |
| 2026-10-21 | **Briefe mit Text** (Idee des Users): Ansicht wie die „Vorschau“ bei den Katzen von Pixelpfote; der User schickt eine Beispieldatei, wenn es so weit ist | 0.7.x (nach Händlerin und Beziehungen) |
| 2026-10-21 | **Gedanken der Bewohner**, auch in der Weboberfläche sichtbar (Idee des Users) | 0.6.x (Vorlagen, Info-Karte, Verlauf im Web) |
| 2026-10-21 | **Eigene Möbel des Users:** Format im Stil-Leitfaden, Upload über die Weboberfläche nach SD/Flash oder per Update; Machbarkeit prüfen | Grafik-Format 0.5.5, Format für Möbel 0.6.0 (STIL.md), Upload 0.7.x |
| 2026-10-20 | **Easter Egg „So entstand Pixelia“:** Knopf in der Weboberfläche öffnet die frühen Skizzen ([docs/skizzen](skizzen/README.md): fünf Perspektiven, dann die gewählte Mischung), als Rückblick auf die Entwicklung (Wunsch des Users) | Vor 1.0 klären ([ZUSATZINHALTE.md](ZUSATZINHALTE.md)), Ablage 0.5.5 |
| 2026-10-21 | **Easter Eggs (prüfen)** (Wunsch des Users über den Planungs-Chat): klären, wie Pixelia mit Easter Eggs umgeht: welche es gibt, wo sie versteckt sind (Weboberfläche, Display), wie man sie findet und ob sie hinter einem Schalter liegen. Erstes geplantes: die Beispielbilder und Beispiel-Seiten der Perspektiv-Sitzung (Hof, Haus, Dorf, Zug in verschiedenen Perspektiven), abgelegt in [docs/skizzen](skizzen/README.md) (v1–v5 als HTML), in der Weboberfläche als verstecktes Extra ansehen. Bei der Prüfung beachten: Web-Bundle ≤ 150 KB gzip (Bilder evtl. aus LittleFS oder später von der SD-Karte statt eingebettet); Umfang als Auswahlliste dem User vorlegen | Vor 1.0 klären ([ZUSATZINHALTE.md](ZUSATZINHALTE.md)), Ablage der Bilder 0.5.5 |
| 2026-10-19 | Log-Zeilen der WLAN-Bibliothek, die in zwei Teilen kommen (z. B. „wifi:“ + „<ba-add>…“), im Log-Puffer zu einer Zeile zusammenfügen | offen (Kleinigkeit) |
| 2026-10-21 | **Innenräume als Baukasten** (Wunsch des Users): Wände, Böden, Fenster, Türen, Möbel und Deko als einzelne Teile, ein Raum ist eine Datenliste; 20 verschiedene Häuser ohne 20 gemalte Bilder ([SETTING.md](SETTING.md) Kapitel 8) | ✅ 0.5.1 (Engine-Baustein); Inhalte 0.6.x und 0.7.x |
| 2026-10-21 | **Ausbaustufen von Möbeln und Räumen** über denselben Baukasten (Notunterkunft mit Kisten und Strohsack → Raum für Raum ausgebaut; Strohsack → Bett) | ✅ 0.5.1 (Engine-Baustein); Bau als Projekt 0.5.10, Inhalte 0.6.x und 0.7.x |
| 2026-10-21 | **Farbvarianten** für Bett, Sofa, Vorhänge, Tapete usw. per Palettentausch (einmal zeichnen, Farbreihe austauschbar, kein Zusatzspeicher) | ✅ 0.5.0/0.5.2 (Paletten, Baukasten) |
| 2026-10-21 | **Erkennungsstücke** pro Haushalt (Gitarre bei Taylor, Geweih beim Jäger, Mehlsäcke bei der Bäckerin), Liste in SETTING.md Kapitel 3 | Feld ✅ 0.5.1; Inhalte 0.6.x und 0.7.x |
| 2026-10-21 | **Haus der Hauptfigur anpassbar** in der Weboberfläche (Haustyp, Farben, Möbel wie verschiedene Sofas); die Figur baut bzw. streicht um, das kostet Zeit und Material | 0.7.x (Sykoras Hof, Weboberfläche) |
| 2026-10-21 | **Austauschbare Asset-Quelle** (Flash oder SD-Karte): erst ohne Karte, später bei Bedarf alle Orte auf die Karte umziehen, Laden beim Ortswechsel hinter einer Überblendung; vorher SD-Karte messen (Lesegeschwindigkeit, keine Bildaussetzer, Karte ziehen) | 0.5.5, Prüfliste Punkt 7 |
| 2026-10-21 | **Wirtschafts-Protokoll für das Balancing** (Wunsch des Users): Bedarf, Verbrauch, Herstellung, Arbeitszeit, Versorgung und Bau als CSV mitschreiben, Export über Weboberfläche und `tools/wirtschaft.ps1`, Bericht mit `tools/balance_report.py` ([WIRTSCHAFT.md](WIRTSCHAFT.md) Kapitel 15) | Schreiben 0.5.10; Abholen, Bericht und Seite Diagnose 0.5.12 |
| 2026-10-21 | **Prüfen statt raten** (ausdrücklicher Wunsch des Users): jede Wirtschaftsänderung im PC-Simulator im Zeitraffer (≥ 3 Weltjahre, mehrere Seeds) gegen die Prüfgrößen P1–P11 prüfen, Bericht in den PR, ohne grünen Bericht kein Merge ([WIRTSCHAFT.md](WIRTSCHAFT.md) Kapitel 14) | Zeitraffer 0.5.12; Pflicht ab der ersten Wirtschaft in 0.6.x |
| 2026-10-21 | **Museum als Naturalienkabinett** (Wunsch des Users): echte Fische, Insekten und andere Tiere in Aquarium und Terrarium, Infokarte mit Pixel-Art und echten Angaben (Quellen im PR), füllt sich durch Spenden der Bewohner; dazu Steine, Fossilien, Pflanzen und Fundstücke der Katze ([SETTING.md](SETTING.md) Kapitel 4) | 0.7.x komplett im Grundspiel, mehr als 45 Stücke (Entscheidung des Users 2026-10-21); Info-Karte 0.5.6, Grubenfunde 0.8.x |
| 2026-10-21 | **Sichtbare Bestände** (Wunsch des Users): jedes Stück im Lager einzeln im Bild, z. B. jeder Brotlaib in der Auslage der Bäckerei; Stellplätze im Innenraum-Baukasten | Stellplätze 0.5.8, Warenbuch füllt sie 0.5.10, Inhalte ab 0.6.x |
| 2026-10-21 | **Eigene Möbel des Users** (Machbarkeit prüfen): Maße im Raster und Palette als Vorgabe, Daten mit Stellfläche und Benutzungspunkten, Upload über die Weboberfläche (Speicher SD oder Flash) oder per Update ins Repo | Format 0.6.0 (STIL.md), Upload 0.7.x |
| 2026-10-21 | **Zustand „im Bau“** (Wunsch des Users bei der Design-Frage 0.5.1): wo ein Möbel hinkommt, steht während der Bauphase ein halbfertiges Teil oder liegt Material; Knopf „Im Bau“ auf Experimentell | ✅ 0.5.1 (Engine-Baustein); Bau als Projekt 0.5.10, Aufgabe der Figuren ab 0.6.x |
| 2026-10-21 | **Fensterlicht als System** (Wunsch des Users): Nachts leuchtet ein Fenster nach außen nur, wenn innen eine Lampe, Kerze oder der Kamin brennt; die Ursachen kommen aus der Simulation (Brennstoff fehlt, Bewohner schläft oder ist weg). [WIRTSCHAFT.md](WIRTSCHAFT.md) deckt den Verbrauch ab (Lichtabend, Rüböl, Kienspan als Notlicht, Prüfgröße P4), [SETTING.md](SETTING.md) Kapitel 9 und 10 das Prinzip. **Von der Setting-Werkstatt geklärt (2026-10-21):** Kamin und Herd als Lichtquelle (Schein ohne Kerze?), Dauer eines Lichtabends und wann gelöscht wird (Schlafengehen, Nachteulen), Licht in Taverne, Werkstatt und Stall, Außenlaternen und Fackeln (wer zündet sie an?), Stärke je Brennstoff (Kienspan schwach, Öllampe hell), Fensterstufe (Laden zu, Leinen, Glas) bestimmt den Schein nach außen | Engine ✅ 0.5.3 (`LightBoard`); Lichtquellen-Arten 0.5.8, Rituale 0.5.11, Brennstoff-Kette 0.6.x (Systemplan); Regeln ✅ [SETTING.md](SETTING.md) Kapitel 9 |
| 2026-10-21 | **Neuplanung nach dem Setting** (Schritt 6 der Setting-Werkstatt, Aufträge des Users): Systemplan mit Patch, Nachweis und Denkstufe je System, 0.5.x neu geschnitten, Phasen ab 0.6.x als Kapitel der Welt, Grundsatztexte nachgezogen, README neu | ✅ 2026-10-21, PR #31 (Systemplan, Entscheidungsprotokoll) |
| 2026-10-21 | **Freie Grafik-Bibliotheken** für Häuser, Möbel, Bewohner und Tiere prüfen (nur CC0 oder CC-BY) | ❌ geprüft, keine übernommen: sieht zu „low-budget“ aus (Entscheidung des Users); eigene Grafiken im Stil der Küche aus [Skizze v4](skizzen/massstab-v4.html), Tabelle in [STIL.md](STIL.md) |
| 2026-10-21 | **Checkliste „Vor 1.0.0 klären“ und Liste der Zusatzinhalte nach 1.0** (Wunsch des Users) | ✅ [ZUSATZINHALTE.md](ZUSATZINHALTE.md) |
| 2026-10-21 | **Jahreszeitenabende** (Wunsch des Users): Jahreszeiten wechseln sonntags; am letzten Abend jeder Jahreszeit großes Zusammenkommen in der Taverne, am Sommerende das **Sommerfeuer** auf dem geschmückten Dorfplatz (Holz wird angespart), am Herbstende der **Lichterabend** ([SETTING.md](SETTING.md) Kapitel 9 und 11) | 0.6.x (Kalender, Ereignis-Regisseur; Mindestziel Kapitel 1), Sommerfeuer am Dorfplatz ab 0.7.x |
| 2026-10-21 | **Schlafenszeit eine Stunde später** (Wunsch des Users, er schaut abends länger zu): Frühaufsteher 22 Uhr, die meisten 23 Uhr, Nachteulen 1 Uhr ([SETTING.md](SETTING.md) Kapitel 6) | Tagesablauf (Rituale 0.5.11, Grundberufe 0.6.x) |
| 2026-10-21 | **Echter Mond** (Idee von Claude, vom User gewählt): Mondphase von heute am Nachthimmel, Vollmond hellt die Nacht auf, manche gehen ohne Laterne heim | ✅ 0.5.7 Mond und Nachthelligkeit; ohne Laterne heim mit dem Tagesgerüst (0.5.11) |
| 2026-10-22 | **Große Fenster mit Fensterbank** (Wunsch des Users): Die Fenster der Testräume sind zu klein, draußen erkennt man das Wetter kaum, und es passt keine Kerze davor. Fenster etwa 4–5 Kacheln breit wie in der Küche aus Skizze v4, mit einer Fensterbank als Stellplatz (Kerze, Blumentopf); Regel in [STIL.md](STIL.md) Kapitel 8 | Grafik-Format 0.5.5, Stellplätze und Nachprüfung 0.5.8, echte Räume 0.6.x |
| 2026-10-26 | **Bewohner jünger** (Wunsch des Users): Handwerker und Bauernpaar 10–14 Jahre jünger, sie wissen noch nicht alles, machen aber alles mit Liebe; Lernen sichtbar über Fähigkeitsstufen ([SETTING.md](SETTING.md) Kapitel 3) | ✅ Setting; Fähigkeitsstufen 0.5.9, Startstufen 0.6.x |
| 2026-10-28 | **Tür-Animation beim Ortswechsel** (Idee des Users): Die Tür geht auf, die Figur tritt hindurch, danach schließt sie sich. Seit 0.5.9 Teil 2 wartet eine Figur an der Tür bis zur nächsten vollen Weltsekunde; die Animation macht dieses Warten (0 bis 1 s) gewollt: Tür öffnet beim Ankommen, Durchtreten an der Sekundengrenze. Zustandsbilder für Türen (0.5.5) und Ortszustände (0.5.8) gibt es schon | 0.5.11 (Figuren gehen von Ort zu Ort), spätestens 0.6.0 mit den echten Türen |
| 2026-10-28 | **Figuren mit Statur** (Wunsch des Users, Vorlage: Pixelfiguren mit unterschiedlichen Formen, Breiten und Größen): Figuren unterscheiden sich nicht nur in Farbe und Kleidung, sondern in Größe, Breite, Bauch, Haltung und Kleidungsform; Ansatz A „Chibi mit Statur“ gewählt (Skizze [figuren-statur.png](skizzen/figuren-statur.png)) | ✅ 0.5.9 Teil 5 (Wahl des Users) |
| 2026-10-28 | **Statur im Steckbrief** (Idee aus 0.5.9 Teil 5, nicht gewählt): Figurenliste und Karte nennen die Statur in Worten („groß, kräftig“, „gebeugt, mit Stock“) | 0.6.0 (mit der Startbesetzung) |
| 2026-10-28 | **Tempo nach Statur** (Idee aus 0.5.9 Teil 5): Gebeugte und Ältere gehen etwas langsamer, Große mit längeren Schritten | 0.5.11 (Tagesgerüst, Simulations-Design) |
| 2026-10-28 | **Kinder-Statur** (Idee aus 0.5.9 Teil 5): Stufe „Kind“ mit kleinerem Kopf für Zugezogene mit Kindern (Startbesetzung ohne Kinder) | mit dem ersten Zuzug |
| 2026-10-29 | **Sichtbares Tragen** (Frage aus 0.5.10 Teil 2b, Kontaktbogen [0510-tragen.png](skizzen/0510-tragen.png)): Korb am Arm (Holz, Brot) als Handstück im Figuren-Baukasten, Kosten am Gerät messen | 0.6.x (Wahl des Users: mit den echten Figuren) |
| 2026-10-29 | **Späne am Hackklotz** (Idee aus 0.5.10 Teil 2b): nach dem Spalten liegen ein paar Späne, die mit der Zeit verschwinden | 0.6.x (Wunsch des Users: nicht vergessen) |
| 2026-10-29 | **Vorratskiste zeigt Waren** (Idee aus 0.5.10 Teil 2b): Kiste mit offenem Deckel und kleinen Zeichen der Waren darin | 0.6.x (Wunsch des Users: nicht vergessen) |
| 2026-10-29 | **Brot um 1850 richtig aufbewahren** (Hinweis des Users): kein Wandregal, sondern Brotkorb, Brotschrank oder Brotleiter unter der Decke; das Brotregal im Testraum ist nur Testinhalt | 0.6.0 (echte Küche) |
