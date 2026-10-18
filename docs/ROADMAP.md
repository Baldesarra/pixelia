# Pixelia – Roadmap

> Lebendes Dokument: Status, Versionen und Wünsche werden laufend gepflegt.
> Die Grundsatzentscheidungen und ihre Begründungen stehen in [PROJEKTPLAN.md](PROJEKTPLAN.md).

## Aktueller Stand

| | |
|---|---|
| **Phase** | 0.1.x Fundament: Hardware geprüft, Engine-Prototyp gemessen, **Plan A bestätigt** |
| **Letzte Version** | – (noch kein Release; [PR #1](https://github.com/Baldesarra/pixelia/pull/1) mit Hardware-Prüfung und Engine-Prototyp ist in `main`) |
| **Repo** | https://github.com/Baldesarra/pixelia (öffentlich, alle Rechte vorbehalten) |
| **Nächster Schritt** | Simulator-Grundgerüst (kopflos, Zeitraffer), Repo-Grundgerüst vervollständigen, Web-Flasher → Release 0.1.0 |
| **Aktualisiert** | 2026-10-18 |

Messergebnisse und geprüfte Pinbelegung: [HARDWARE.md](HARDWARE.md). Kurz: 30 fps stabil bei 60 Hz Panel, Zeichnen
4,1 ms pro Bild, Hochskalier-Interrupt 20 %, keine Aussetzer (auch nicht beim Flash-Schreiben), 299 KB interner RAM frei.

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

- [ ] Repo-Grundgerüst (`firmware/`, `engine/`, `worlds/`, `sim/`, `web/`, `tools/`, `assets/`)
      – `firmware/`, `engine/`, `tools/` stehen
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
- [ ] Simulator-Grundgerüst (SDL, Kopflos-Screenshot, Zeitraffer als Grundlage für den Langzeit-Test)
- [x] Partitionstabelle, Bootloader-Rollback aktiviert, Core-Dump (Rollback-Test mit echtem Update in 0.2.x)
- [ ] Web-Flasher auf GitHub Pages, Erstinstallation per USB

## 0.2.x – Vernetzt

- [ ] WLAN-Manager: bis zu 5 Netze, stärkstes zuerst, Hostname „Pixelia“
- [ ] Setup-Modus: AP „Pixelia-Setup“, Captive Portal, Neuversuch alle 2 min, Anleitung + QR-Code auf dem Display
- [ ] mDNS `pixelia.local`, NTP-Zeit
- [ ] HTTP-Server + WebSocket, einfache Setup-Seite
- [ ] OTA per `.bin`-Upload mit Fortschritt (Browser + Display)
- [ ] A/B-Rollback mit Selbsttest
- [ ] Einstellungen in NVS
- [ ] **Ab hier Updates ohne USB**

## 0.3.x – Weboberfläche

- [ ] Svelte-Projekt, Build in CI, gzip-Einbettung in die Firmware
- [ ] M3-Expressive-Komponentenbibliothek (Tokens, Formen, Bewegung, dynamische Farben, hell/dunkel)
- [ ] Offline-Schriften und -Icons (reduziert)
- [ ] Navigation (Handy + Desktop)
- [ ] Seiten: Übersicht, Info, Hardware, WLAN (Scan, Signalbalken), Verbundene Geräte, System
- [ ] Neustart, Werksreset, Einstellungen exportieren/importieren
- [ ] PIN/Passwort optional (Standard aus)
- [ ] Live-Log

## 0.4.x – Updates & Diagnose

- [ ] `CHANGELOG.json` → eingebetteter Block + Release-Notes (Generator)
- [ ] Release-Workflow: Tag → GitHub-Release mit `.bin` und Factory-Image
- [ ] Update von GitHub: Prüfung, Kanal Stabil/Vorschau, Installation per HTTPS
- [ ] Versionsverlauf-Timeline, Änderungsvorschau aus gewählter `.bin`
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
- [ ] **Langzeit-Test** im Simulator (Zeitraffer, Bericht, Blick-Test, Kontaktbogen) und **Inhaltszähler**, automatisch
      in jedem Welt-PR ([LEBENDIGKEIT.md](LEBENDIGKEIT.md) Kapitel 3)
- [ ] **Ereignis-Regisseur** als Engine-Baustein (Spannungsbogen, Sperrzeiten, Kontext Tageszeit/Wetter/Jahreszeit)
- [ ] Gedächtnis-/Beziehungs-Bausteine für Figuren (von allen Welten nutzbar)
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

- [ ] 0.7.0: Perspektive per Mockups entscheiden, Grundgerüst Dorf, erste Bewohner mit Tagesplan
- [ ] Figuren-Baukasten, Bewohner mit Charakter, Bedürfnissen, Beziehungen
- [ ] Berufe und Tagesabläufe, Wochenende
- [ ] Licht bei Nacht (Fenster, Laternen, Nachtwächter)
- [ ] Tiere
- [ ] Wetter und Jahreszeiten im Dorf
- [ ] Ereignis-Katalog (häufig → legendär), Anlässe und Feiertage
- [ ] Dorfchronik, Bewohnerliste, Statistiken in der Oberfläche
- [ ] Kamera-Regie, Info-Karten per Touch
- [ ] Langzeit: Wachstum, Bauprojekte, Zu- und Wegzüge
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
| 2026-10-18 | Display-Drehung 0°/180° als Systemeinstellung, Aufstellort steht noch nicht fest (Wunsch des Users) | 0.2.x (NVS), 0.3.x (Oberfläche) |
| 2026-10-18 | Engine: Bounce-Callback mit SIMD (PIE) beschleunigen oder intern rendern und ins PSRAM kopieren, falls Welten mehr Zeit brauchen | 0.5.x bei Bedarf |
| 2026-10-18 | RAM: kurzer Einbruch von ~30 KB internem RAM beim Start und beim Anlegen der Engine untersuchen | 0.2.x |
| 2026-10-18 | Helligkeitskurve vermessen (gleichmäßig wahrgenommene Stufen, kleinster noch leuchtender Wert) | 0.5.x (Nachtmodus) |
| 2026-10-18 | **Tiefe statt Kulisse:** Welten sollen wirklich leben, mit Features und Ereignissen statt Dauerschleife. Umgesetzt als verbindliche [Lebendigkeits-Charta](LEBENDIGKEIT.md) mit messbaren Zielen (Wunsch des Users) | alle Welten, Abnahmekriterium |
