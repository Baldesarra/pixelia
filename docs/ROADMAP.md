# Pixelia – Roadmap

> Lebendes Dokument: Status, Versionen und Wünsche werden laufend gepflegt.
> Die Grundsatzentscheidungen und ihre Begründungen stehen in [PROJEKTPLAN.md](PROJEKTPLAN.md).

## Aktueller Stand

| | |
|---|---|
| **Phase** | 0.2.x Vernetzt: 0.2.1 „Immer erreichbar“ **veröffentlicht**, Gerätetest durch den User steht aus |
| **Letzte Version** | 0.2.1 „Immer erreichbar“ (2026-10-18), [Release auf GitHub](https://github.com/Baldesarra/pixelia/releases/tag/v0.2.1) |
| **Repo** | https://github.com/Baldesarra/pixelia (öffentlich, alle Rechte vorbehalten) |
| **Installieren** | Web-Flasher https://baldesarra.github.io/pixelia/ (Chrome/Edge, USB), danach WLAN im Setup-Modus einrichten |
| **Nächster Schritt** | Gerätetest 0.2.1 durch den User (Update per Browser, Bildrate mit offener Karte, langer Druck, Tippen, QR-Scan, Kurzmeldung nach Neustart); danach 0.3.x Weboberfläche (Svelte, M3 Expressive) |
| **Aktualisiert** | 2026-10-18 |

Messergebnisse und geprüfte Pinbelegung: [HARDWARE.md](HARDWARE.md). Kurz: 30 fps stabil bei 60 Hz Panel, Zeichnen
4,0 ms pro Bild, Hochskalier-Interrupt 20 %, keine Aussetzer, auch mit WLAN, Webserver und während Updates. Interner RAM
im Betrieb 125 KB frei (Tiefstwert ≥ 115 KB). Update per Browser in 7 s, Rückfall bei Absturz, Hänger oder
gescheitertem Selbsttest am Gerät geprüft.
Simulator: `tools/sim.ps1` (Fenster) bzw. kopflos mit Screenshot, Zeitraffer und Kontaktbogen, siehe [sim/README.md](../sim/README.md).

## Versionsschema

Immer drei Stellen: `MAJOR.MINOR.PATCH`.

- **MINOR = Phase/Welt.** Die erste Version einer Phase endet auf `.0` (z. B. 0.6.0 = erste Zug-Version).
- **PATCH = jede weitere Iteration** derselben Phase (0.6.1, 0.6.2, … – auch für Anpassungswünsche und Fehlerbehebungen).
- Testversionen für den Kanal „Vorschau“: `X.Y.Z-beta.N`.
- Vor 1.0.0 sind alle Versionen Vorabversionen.

| Version | Phase | Inhalt |
|---|---|---|
| 0.1.x | Fundament | Repo, CI, Display und Touch, Engine-Prototyp, Simulator |
| 0.2.x | Vernetzt | WLAN, Setup-Modus, Webserver, OTA per Upload mit Rollback |
| 0.3.x | Weboberfläche | M3-Expressive-Komponenten, Info-/Hardware-/WLAN-/System-Seiten |
| 0.4.x | Updates & Diagnose | Update von GitHub, Versionsverlauf, Absturzberichte, Experimentell, Live-Vorschau |
| 0.5.x | Engine | Sprites, Licht, Wetter, Schrift, Dienste, Regie, Asset-Pipeline |
| 0.6.x | 🚂 Zugfenster | |
| 0.7.x | 🏘️ Pixel-Dorf (Schwerpunkt) | |
| 0.8.x | ⚔️ Dungeon-Held | |
| 0.9.x | ⚽ Pixel-Liga | |
| offen | 🏝️ Inselwelt (cozy, eigenständig) | wird umgesetzt; Position nach der Entscheidung über die Hauptwelt (vor 0.6.0) |
| 1.0.0 | Pixelia 1.0 | Feinschliff, Verbindungen komplett, 72-h-Dauertest |

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
- [ ] 0.2.1 am Gerät (durch den User, ausnahmsweise allein): Bildrate mit offener Karte, langer Druck und Tippen,
      QR-Code mit dem Handy, Kurzmeldung nach Neustart; erstes Update per `tools/ota.ps1` am echten Gerät

## 0.3.x – Weboberfläche

- [ ] Svelte-Projekt, Build in CI, gzip-Einbettung in die Firmware
- [ ] M3-Expressive-Komponentenbibliothek (Tokens, Formen, Bewegung, dynamische Farben, hell/dunkel)
- [ ] Offline-Schriften und -Icons (reduziert)
- [ ] Navigation (Handy + Desktop)
- [ ] Seiten: Übersicht, Info, Hardware, WLAN (Scan, Signalbalken), Verbundene Geräte, System
- [ ] Neustart, Werksreset, Einstellungen exportieren/importieren
- [ ] PIN/Passwort optional (Standard aus)
- [ ] Live-Log in der Oberfläche
- [ ] **Diagnose über WLAN:** Log und Diagnose-Befehle per WebSocket, Werkzeug `tools/netlog.ps1` für Claude. Danach ist
      USB nur noch für Erstinstallation, Notfälle und Abstürze ganz früh beim Start nötig

## 0.4.x – Updates & Diagnose

- [ ] `CHANGELOG.json` → eingebetteter Block in der Firmware (Release-Notes-Generator steht seit 0.1.0/0.2.1)
- [x] Release-Workflow: Tag → GitHub-Release mit `.bin` und Factory-Image (seit 0.1.0)
- [ ] Update von GitHub: Prüfung, Kanal Stabil/Vorschau, Installation per HTTPS
- [ ] Versionsverlauf-Timeline mit **allen Feldern** (Neu, Verbessert, Behoben, Entfernt, Geprüft, Hinweise, verlinkte
      Pull Requests), Markierung „Installiert“, Änderungsvorschau aus gewählter `.bin`
- [ ] Absturzberichte (Core-Dump, Backtrace), Reset-Historie
- [ ] Experimentell-Seite: Touch-Test, Displaytests, Stromsparmodus, Panel-Timing, Experimentell-Schalter
- [ ] Live-Vorschau und Screenshot (PNG)
- [ ] Leistungsanzeige (FPS, Frame-Zeiten, CPU pro Kern, Tasks, Heap/PSRAM)

## 0.5.x – Engine

- [ ] Sprites, Animationen, Tilemaps, Kamera, Parallax
- [ ] Paletten, Tag/Nacht, Farbrotation, Lichttabellen, Transparenztabellen, Rastereffekte
- [ ] Partikel und Wetter (Regen, Schnee, Nebel, Blitz)
- [ ] Pixel-Schriften mit Umlauten, Overlays, Kurzmeldungen, Info-Karten
- [ ] Dienste: Weltzeit, Standort (Open-Meteo-Geokodierung), Wetter (Open-Meteo), Ereignis-Bus, Regie, Nachtmodus
- [ ] Modul-Schnittstelle für Welten, Schema → automatische Einstellungsseiten
- [ ] Touch-Gesten und Einstellungen
- [ ] Asset-Pipeline (PNG → Palette), `assets/CREDITS.md`, `docs/STIL.md`
- [ ] Browser-Simulator auf GitHub Pages, Screenshots im Pull Request
- [ ] **Langzeit-Test** im Simulator (Zeitraffer, Bericht, Blick-Test, Kontaktbogen, **Bilanz-Test**) und
      **Inhaltszähler**, automatisch in jedem Welt-PR ([LEBENDIGKEIT.md](LEBENDIGKEIT.md) Kapitel 3)
- [ ] **Ereignis-Regisseur** als Engine-Baustein (Spannungsbogen, Sperrzeiten, Kontext Tageszeit/Wetter/Jahreszeit)
- [ ] **Gedächtnis-Bausteine** (LEBENDIGKEIT.md Regel 10), von allen Welten nutzbar: Bestände mit Menge und Standort und
      exakter Bilanz, unterbrechbare Vorhaben, Besuchs- und Ereignisverlauf, Zustände mit Zeitbezug, atomares Speichern
      mit begrenztem Nachholen; dazu Beziehungen und Erinnerungen für Figuren
- [ ] Demo-Szene als Test

## Abnahme der Welten

Jede Welt-Phase endet erst, wenn die **Mindestziele aus [LEBENDIGKEIT.md](LEBENDIGKEIT.md) Kapitel 2** erfüllt und per
Langzeit-Bericht belegt sind. In jeder Welt-Phase wird hier der Stand „Ist → Ziel“ gepflegt. Zu Beginn jeder Welt-Phase
entsteht der Ideenspeicher `docs/welten/<welt>.md`.

## 0.6.x – 🚂 Zugfenster

- [ ] 0.6.0: Abteilfenster, Parallax-Landschaft, Tageszeit mit Himmelsverlauf
- [ ] Regionen mit Übergängen, Bahnhöfe, „Nächster Halt“
- [ ] Wetter am Fenster (Regentropfen, Schnee, Nebel, Gewitter)
- [ ] Ereignisse (Gegenzug, Tunnel, Tiere, Ballons, Feuerwerk …)
- [ ] Einstellungen, Aktionen, Touch
- [ ] Weitere Iterationen nach Wünschen des Users

## 0.7.x – 🏘️ Pixel-Dorf (Schwerpunkt)

- [ ] 0.7.0: **mehrere Darstellungs-Vorschläge** als Simulator-Mockups (Perspektive, Ausschnitt, Figurengröße, Kamera,
      passend zu 5"), gemeinsame Entscheidung; Grundgerüst Dorf, erste Bewohner mit Tagesplan
- [ ] Figuren-Baukasten, Bewohner mit Charakter, Bedürfnissen, Beziehungen
- [ ] Berufe und Tagesabläufe, Wochenende
- [ ] Licht bei Nacht (Fenster, Laternen, Nachtwächter)
- [ ] Tiere
- [ ] Wetter und Jahreszeiten im Dorf
- [ ] Ereignis-Katalog (häufig → legendär), Anlässe und Feiertage
- [ ] Dorfchronik, Bewohnerliste, Statistiken in der Oberfläche
- [ ] Kamera-Regie, Info-Karten per Touch
- [ ] **Aufbau-Wirtschaft** (PROJEKTPLAN 6.2): Rohstoffe selbst gewinnen, Lager mit Kapazität, Bauprojekte mit exaktem
      Materialverbrauch, Wohnraum als Voraussetzung für neue Bewohner, alles dauerhaft gespeichert
- [ ] Langzeit: Wachstum, Zu- und Wegzüge, Familien
- [ ] Verbindung zum Zug (Bahnhof, Ankünfte)
- [ ] Weitere Iterationen nach Wünschen des Users

## 0.8.x – ⚔️ Dungeon-Held

- [ ] 0.8.0: Dungeon-Generator, Held, Erkundung mit Nebel und Fackelschein
- [ ] Kampf, Gegner, Bosse
- [ ] Beute, Inventar, Ausrüstung am Sprite
- [ ] Rückkehr ins Dorf (Händler, Schmied, Taverne), sichtbar in der Dorf-Welt
- [ ] Hintergrund-Simulation, Abenteuer-Log
- [ ] Charakterbogen, Aktionen in der Oberfläche
- [ ] Weitere Iterationen nach Wünschen des Users

## 0.9.x – ⚽ Pixel-Liga

- [ ] 0.9.0: Vereine, Spielplan, Spielengine, Tabelle
- [ ] Live-Übertragung mit Torwiederholung
- [ ] Konferenz-Scoreboard, Live-Unterbrechung der Regie
- [ ] Saison, Auf-/Abstieg, Pokal, Spielerentwicklung, Transfers
- [ ] FC Pixelia: Name, Farben, Wappen-Editor, Taktik
- [ ] Verbindung zum Dorf (Heimspieltag, Feiern)
- [ ] Weitere Iterationen nach Wünschen des Users

## 🏝️ Inselwelt (Position offen)

Rohkonzept: [welten/inselwelt-konzept.md](welten/inselwelt-konzept.md), Einordnung in [PROJEKTPLAN.md](PROJEKTPLAN.md) 6.5.

- [ ] **Entscheidung über die Hauptwelt** (Inselwelt, Pixel-Dorf oder Dungeon-Held), gemeinsam vor Beginn 0.6.0
- [ ] Konzept gemeinsam für 5" überarbeiten, neue Ideen sammeln, Mindestziele in LEBENDIGKEIT.md festlegen
- [ ] Mehrere Darstellungs-Vorschläge als Simulator-Mockups
- [ ] Umsetzung, Versionsnummer nach der Entscheidung; zuverlässiges Gedächtnis mit Bilanz-Test von Anfang an

## 1.0.0 – Pixelia 1.0

- [ ] Alle Verbindungen zwischen den Welten
- [ ] Feinschliff, Leistungsoptimierung
- [ ] 72-h-Dauertest ohne Neustart
- [ ] README mit Bildern/GIFs

---

## Backlog – Wünsche und Ideen

Neue Wünsche des Users landen hier (mit Datum) und werden einer Version zugeordnet.

| Datum | Wunsch / Idee | Geplant für |
|---|---|---|
| 2026-10-18 | Weitere Welten später: Aquarium, Raumstation, Ameisenfarm, Leuchtturm, Bauernhof | nach 1.0.0 |
| 2026-10-18 | Optional Lichtsensor BH1750 nachrüsten (lernende Auto-Helligkeit) | offen |
| 2026-10-18 | Optional Ton über I²S-Verstärker nachrüsten. **Neu:** Board hat Lautsprecher-Anschluss „Speak“ (P6) mit Verstärker U4, prüfen | offen |
| 2026-10-18 | Leistungsmonitor (FPS, Last, Speicher) in der Weboberfläche, Log per Schalter unter „Experimentell“ ein/aus (Idee des Users) | 0.4.x |
| 2026-10-18 | Display-Drehung 0°/180° als Systemeinstellung, Aufstellort steht noch nicht fest (Wunsch des Users) | ✅ 0.2.0 (NVS, Einrichtungsseite, `rot`), 0.3.x (richtige Oberfläche) |
| 2026-10-18 | Engine: Bounce-Callback mit SIMD (PIE) beschleunigen oder intern rendern und ins PSRAM kopieren, falls Welten mehr Zeit brauchen | 0.5.x bei Bedarf |
| 2026-10-18 | RAM: kurzer Einbruch von ~30 KB internem RAM beim Start und beim Anlegen der Engine untersuchen | ✅ 0.2.0 geklärt: DMA-Pool von ESP-IDF meldet Tiefstwert 0, kein echter Verbrauch ([HARDWARE.md](HARDWARE.md)) |
| 2026-10-18 | Helligkeitskurve vermessen (gleichmäßig wahrgenommene Stufen, kleinster noch leuchtender Wert) | 0.5.x (Nachtmodus) |
| 2026-10-18 | **Tiefe statt Kulisse:** Welten sollen wirklich leben, mit Features und Ereignissen statt Dauerschleife. Umgesetzt als verbindliche [Lebendigkeits-Charta](LEBENDIGKEIT.md) mit messbaren Zielen (Wunsch des Users) | alle Welten, Abnahmekriterium |
| 2026-10-18 | WLAN gleich im Web-Flasher eintragen (Improv Serial, ESP Web Tools unterstützt das), dann entfällt der Umweg über den Setup-Modus | 0.3.x |
| 2026-10-18 | Setup-Modus und neues Setup-Passwort auch per Touch-Geste am Gerät (zusammen mit Werksreset per Touch) | 0.3.x |
| 2026-10-18 | Upload-Hänger bei schwachem WLAN (User-Test): behoben durch 32-KB-TCP-Fenster und SACK; Seite meldet 30 s ohne Fortschritt | ✅ 0.2.0 |
| 2026-10-18 | **Neue Welt „Inselwelt“** (cozy, eigenständig, ein Bewohner baut Haus, Hof, Boot, später Hofladen). Rohkonzept gesichert in [welten/inselwelt-konzept.md](welten/inselwelt-konzept.md), wird später für 5" überarbeitet und gemeinsam weiter geplant (Wunsch des Users) | eingeplant (PROJEKTPLAN 6.5, Abschnitt „Inselwelt“ oben), Position nach Hauptwelt-Entscheidung |
| 2026-10-18 | **Zuverlässiges Gedächtnis** (vor allem Inselwelt, aber für alle Welten): exakte Bestände (welche Waren, wie viele, wo), angefangene und unbeendete Arbeiten, wer schon zu Besuch war; nichts verdoppelt oder verschwindet, auch nicht bei Neustart; Bilanz-Test im Langzeit-Test. In LEBENDIGKEIT.md als Regel ergänzen | ✅ Regel 10 in LEBENDIGKEIT.md; Bausteine 0.5.x; Inselwelt und alle Welten |
| 2026-10-18 | **Entscheidung Hauptwelt:** Inselwelt, Pixel-Dorf oder Dungeon-Held als Schwerpunkt der Software. Gemeinsam planen und diskutieren | vor Beginn 0.6.0 |
| 2026-10-18 | **Dorf als Aufbau-Wirtschaft:** begrenzte Rohstoffe, die das Dorf selbst gewinnen muss; Bauprojekte verbrauchen Material exakt; neue Häuser müssen gebaut werden, damit neue Bewohner kommen; alles dauerhaft gespeichert (Ideen aus der Inselwelt übernehmen) | 0.7.x (PROJEKTPLAN 6.2, Checkliste oben) |
| 2026-10-18 | **Mehrere Darstellungs-Vorschläge** für Dorf und Inselwelt (Perspektive, Ausschnitt, Figurengröße, Kamera), passend zum 5"-Display, mit Simulator-Mockups, bevor gebaut wird | Beginn 0.7.0 bzw. Inselwelt |
| 2026-10-18 | **Systemschrift** (5×7, `engine/src/font.cpp`, Setup-Bildschirm) gefällt dem User: beibehalten, Grundlage für den Stil-Leitfaden | ✅ im Entscheidungsprotokoll festgehalten; STIL.md in 0.5.x |
| 2026-10-18 | **Ausführliche Changelogs** im Stil der PR-Beschreibungen: Neu, Geprüft am Gerät (mit Messwerten), gefundene und behobene Probleme, Hinweise. Alle Änderungen inkl. Fixes seit 0.1.0. Format um Felder „Geprüft“ und „Hinweise“ erweitern, 0.1.0/0.2.0 ausführlich nachtragen (aus PR #1, #3, #4, #5), `tools/release_notes.py` anpassen, Regel in CLAUDE.md. Anzeige später unter „Versionsverlauf“ | ✅ 0.2.1 (Format, Nachtrag, CI-Prüfung, Regel); Anzeige 0.4.x |
| 2026-10-18 | **IP jederzeit sehen:** langer Druck (3 s) aufs Display → Info-Karte mit IP, `pixelia.local`, WLAN-Name, Signal und QR-Code zur Weboberfläche; zusätzlich IP nach jedem Verbinden kurz einblenden; Geste in der Weboberfläche einstellbar | ✅ 0.2.1 (Geste, Info-Karte, Kurzmeldung, Dauer im NVS); 0.3.x (einstellbar in der Oberfläche) |
| 2026-10-18 | **Arbeiten von unterwegs:** Cloud-Sitzungen über claude.ai bzw. die Claude-App (GitHub einmal verbinden), Remote Control, um lokale Sitzungen vom Handy aus weiterzuführen, wenn der PC läuft. Rückweg immer über Pull Requests auf GitHub | bei Bedarf einrichten |
| 2026-10-18 | **USB optional machen:** Updates per WLAN als Werkzeug (`tools/ota.ps1`), Log und Diagnose per WebSocket (`tools/netlog.ps1`) | ✅ 0.2.1 (`tools/ota.ps1`); 0.3.x (`tools/netlog.ps1`) |
