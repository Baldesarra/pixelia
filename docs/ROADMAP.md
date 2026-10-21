# Pixelia – Roadmap

> Lebendes Dokument: Status, Versionen und Wünsche werden laufend gepflegt.
> Die Grundsatzentscheidungen und ihre Begründungen stehen in [PROJEKTPLAN.md](PROJEKTPLAN.md).

## Aktueller Stand

| | |
|---|---|
| **Phase** | 0.5.x Engine, Phase A (settingunabhängig): 0.5.0 „Bühne und Figuren“ (PR #24) und 0.5.1 „Raum und Baukasten“ (PR #26) fertig: Kacheln in drei Ebenen, Tiefensortierung, Räume als Datenliste mit Ausbaustufen und „im Bau“, Möbel-Daten, Türen und Bildränder als Ausgänge, drei verbundene Testorte |
| **Letzte Version** | 0.5.1 „Raum und Baukasten“ (2026-10-21, PR #26), [Release auf GitHub](https://github.com/Baldesarra/pixelia/releases/tag/v0.5.1). Am Gerät: alle Testorte 30,3 fps, Zeichnen 4,1–4,2 ms, Raum mit 64 Figuren 6,8 ms, Überblendung höchstens 14,6 ms, 0 Aussetzer, RAM 111 KB frei (Tiefstwert 104 KB), PSRAM 5,96 MB frei; „bench“: Verdoppeln 2,96 ms, Kopie des Hintergrunds 0,99 ms pro Bild. Davor 0.5.0 „Bühne und Figuren“ (2026-10-21, PR #24), [Release auf GitHub](https://github.com/Baldesarra/pixelia/releases/tag/v0.5.0). Am Gerät: 30 fps ohne Aussetzer, Zeichnen 4,2 ms (Testort mit 64 Figuren 5,9 ms, Überblendung höchstens 13,7 ms), RAM 102–106 KB, PSRAM 6,0 MB frei. Update von GitHub am Gerät (0.4.2-dev → 0.5.0): 1574 KB in 17,3 s (111 KB/s), RAM-Tiefstwert 78 KB, fps min 29,8, Probelauf bestanden; Prüfung mit TLS RAM 97 → 83 KB. Davor 0.4.2 „Experimentell, Live-Vorschau, Leistung“ (2026-10-20, PR #20), [Release auf GitHub](https://github.com/Baldesarra/pixelia/releases/tag/v0.4.2). Am Gerät: mit Live-Vorschau 29,7–30,3 fps, RAM 115 KB, 0 Aussetzer; Stromsparmodus 30 Hz mit Rückfall nach 15 s; Download nach 932 KB abgebrochen ohne Neustart. Davor 0.4.1 „Update von GitHub“ (2026-10-20, PR #19), [Release auf GitHub](https://github.com/Baldesarra/pixelia/releases/tag/v0.4.1); davor Vorschau v0.4.1-beta.1 und 0.4.0 „Versionsverlauf und Diagnose“ (PR #17, #18). Zweiter echter Durchlauf im Kanal Stabil (0.4.1-beta.1 → 0.4.1 über GitHub): 1522 KB in 15,8 s (Verbindung 3,2 s, 121 KB/s), 30,0–30,2 fps, RAM-Tiefstwert 91 KB; Probelauf bestanden, danach 30,3 fps, RAM 117/111 KB |
| **Repo** | https://github.com/Baldesarra/pixelia (öffentlich, alle Rechte vorbehalten) |
| **Installieren** | Web-Flasher https://baldesarra.github.io/pixelia/ (Chrome/Edge, USB), danach WLAN im Setup-Modus einrichten |
| **Nächster Schritt** | 0.5.2 Farbe und Licht (Paletten je Ort, Farbrampen mit `docs/STIL.md`-Entwurf, Tag/Nacht, Farbrotation, Lichtkreise, Transparenztabellen, Himmelsverläufe, Rastereffekte; Zeichnen beschleunigen); frische Arbeits-Sitzung, Denkstufe xhigh |
| **Offen** | Sichtprüfung am Display durch den User: Testorte, Übergänge, Ausgänge, Knöpfe Ausbau/Im Bau, Prüfansicht (Seite Experimentell → Orte), Stromsparmodus (Panel 30 Hz: flimmert es?), Touch-Test, Displaytests; Zeichnen in 0.5.2 beschleunigen: Verdoppeln 2,96 ms + Kopie des Hintergrunds 0,99 ms pro Bild (gemessen mit „bench“; Backlog SIMD, Verdoppeln direkt aus dem Hintergrund); Rückfall mit einem von GitHub geladenen kaputten Image nicht eigens geprüft (gleicher Weg wie beim Upload, dort geprüft; ein absichtlich kaputtes Release wäre für alle im Kanal Vorschau sichtbar); Captive Portal unter iOS einmal ansehen (Android und Windows geprüft); Screenshot-Läufe schreiben die Standard-Farbwahl aufs Gerät, solange es noch keine hat (harmlos); Geräteliste: kopflose Screenshot-Läufe legen jedes Mal ein neues Gerät an (harmlos, älteste fallen raus); Planungs-Chat: gemeinsames Setting ausarbeiten (vor Modul-Schnittstelle in 0.5.x); Perspektive entschieden (2026-10-21, 3/4-Draufsicht, siehe Backlog); Prüfliste Baukasten/Asset-Quellen geprüft (0.5.x), offen für `docs/STIL.md`: Aufteilung der Palette in Farbrampen (Grundlage für Farbreihen und eigene Möbel); `docs/SETTING.md` auf main (PR #25), die Werkstatt arbeitet weiter, Schritt 6 (Planung der späten 0.5.x-Patches) offen; Inselwelt-Konzept auf 400×240 überarbeiten; mehrere Layout-Vorschläge für Dorf und Insel |
| **Aktualisiert** | 2026-10-21 |

Messergebnisse und geprüfte Pinbelegung: [HARDWARE.md](HARDWARE.md). Kurz: 30 fps stabil bei 60 Hz Panel, Zeichnen
4,0 ms pro Bild, Hochskalier-Interrupt 20 %, keine Aussetzer, auch mit WLAN, Webserver, während Updates und unter Last
auf allen Diagnose-Schnittstellen. Interner RAM im Betrieb 116–121 KB frei (Tiefstwert ≥ 107 KB). Update per WLAN in
7–11 s, Rückfall bei Absturz, Hänger oder gescheitertem Selbsttest am Gerät geprüft. Log, Befehle, Leistung,
Reset-Historie und Absturzberichte per WLAN (`tools/netlog.ps1`, `tools/crash.ps1`), USB nur noch für die
Erstinstallation.
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
| 0.2.x | Vernetzt | WLAN, Setup-Modus, Webserver, OTA per Upload mit Rollback, Fernwartung per WLAN |
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

Aufteilung in 11 Patches, vom User bestätigt am 2026-10-21 (Entscheidungsprotokoll). Phase A ist settingunabhängig
und geht sofort los. Phase B (0.5.8 ff.) startet erst, wenn die Setting-Werkstatt Schritt 6 (Planung der späten
0.5.x-Patches) abgeschlossen hat und der User das bestätigt; dass `docs/SETTING.md` auf main ist (PR #25), reicht dafür
nicht. An der Reihenfolge ändert sich nichts. Jeder Patch ist eine eigene Arbeits-Sitzung
(< ~250 K Kontext) und wird im Simulator und am Gerät gemessen (fps, Bildzeit, RAM, PSRAM). Kamerafahrten und Parallax
als Hauptmittel entfallen, weil nichts scrollt (Perspektive 2026-10-21); braucht der Zug Parallax, kommt es mit der
Zug-Phase.

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
- [ ] **0.5.2 Farbe und Licht** (xhigh): Paletten je Ort, Farbrampen (mit `docs/STIL.md`-Entwurf), Tag/Nacht,
      Farbrotation, Lichtkreise, Transparenztabellen, Himmelsverläufe, Rastereffekte
- [ ] **0.5.3 Wetter und Partikel** (Hoch): Regen, Schnee, Nebel, Blitz, Wind
- [ ] **0.5.4 Asset-Quelle** (xhigh, Speicher-Architektur): Werkzeug PNG → Paletten-Indizes, `assets/CREDITS.md` mit
      Prüfung; Quelle eingebettet / LittleFS / microSD, Laden auf Kern 0 beim Ortswechsel hinter dem Abblenden; SD-Karte
      messen (Tempo, keine Aussetzer, Karte ziehen; braucht eine Karte im Gerät). Speicher-Stufen (Entscheidung
      2026-10-21): Ablage → Grafik-Speicher im PSRAM (Budget, Vorladen der Nachbarorte, Verdrängen) → Zeichnen;
      Grundausstattung (immer geladen) getrennt von Ortspaketen; Kennungen statt Zeiger; gepackt abgelegt;
      Ladezeit pro Ort messen (Flash, LittleFS, SD)
- [ ] **0.5.5 Schrift und Einblendungen** (Hoch): Pixel-Schriften mit Umlauten, Kurzmeldungen, Info-Karten, Pixel-Uhr,
      Hinweis auf neue Version am Display, Ereignis-Bus, Touch-Gesten
- [ ] **0.5.6 Dienste** (Hoch): Weltzeit (Tagesgerüst, Jahreszeit), Standort und Wetter (Open-Meteo), Nachtmodus,
      Helligkeitskurve; Download von GitHub robuster (Abbruch ohne Fortschritt, Weitermachen per HTTP-Range)
- [ ] **0.5.7 Browser-Simulator und Langzeit-Test** (Hoch): Simulator auf GitHub Pages, Screenshots im PR, Zeitraffer,
      Bericht, Blick-Test, Kontaktbogen

**Phase B (nach der Setting-Entscheidung)**

- [ ] **0.5.8 Orte und Stil** (xhigh): Phase A nachprüfen und anpassen; Schnittstelle für Orte und Gebiete, Schema →
      Einstellungsseiten; `docs/STIL.md` mit Format für eigene Möbel (Upload danach)
- [ ] **0.5.9 Gemeinsamer Weltzustand** (xhigh): Figuren, Warenbuch, Aufgaben, Projekte; sichtbarer Ort voll, Rest grob
      mit gleicher Wirkung; Gedächtnis-Bausteine (LEBENDIGKEIT.md Regel 10), atomares Speichern
- [ ] **0.5.10 Regie und Nachweis** (Hoch): Ereignis-Regisseur, **Bilanz-Test** und **Inhaltszähler** automatisch in
      jedem Welt-PR ([LEBENDIGKEIT.md](LEBENDIGKEIT.md) Kapitel 3), Demo-Szene

**Prüfliste Baukasten und Asset-Quellen** (Wunsch des Users über den Planungs-Chat, geprüft 2026-10-21, vor dem Bau
von Asset-Pipeline und „Raum“). Alle Punkte sind machbar; Kosten bezogen auf die Szene in 200 × 120 groben Bildpunkten.

| Punkt | Wie | Speicher / Rechenzeit | Patch |
|---|---|---|---|
| 1 Innenraum-Baukasten ✅ 0.5.1 | Raum = Datenliste (Teil, Kachel-Lage, Stufe, Farbreihe, Besitzer). Boden und Wände als Kachelebene, Möbel und Deko als Sprites mit Fußpunkt, mit den Figuren nach Tiefe sortiert. Unbewegte Teile beim Betreten einmal in ein Hintergrundbild (24 KB) vorzeichnen | ~6 Byte je Teil (60 Teile < 0,5 KB); pro Bild nur Kopie des Hintergrunds + bewegte Teile, < 1 ms | 0.5.1 |
| 2 Ausbaustufen ✅ 0.5.1 | Teil hat mehrere Stufen-Bilder; Ausbau = Datenänderung in der Raumliste (oder Raumliste je Stufe) | nur die Stufen-Grafiken | Engine 0.5.1, Umbau als Aufgabe nach dem Setting |
| 3 Farbvarianten | Umfärbe-Tabelle (256 Byte, Index → Index): Möbel einmal mit einer Platzhalter-Farbrampe zeichnen, die Farbreihe ersetzt die Rampe beim Zeichnen; keine zusätzlichen Grafiken | 256 Byte je Farbreihe; Zeichnen gleich schnell wie ohne (ein Tabellenzugriff pro Pixel) | Zeichnen 0.5.0, Rampen der Palette 0.5.2 + `docs/STIL.md` |
| 4 Auswahl in der Weboberfläche, sichtbarer Umbau | Engine lässt es zu: Raumliste zur Laufzeit änderbar. Umbau mit Spielzeit ist eine Aufgabe der Figur | – | nach dem Setting (Weltzustand, dann Web-Seite) |
| 5 Erkennungsstücke ✅ Feld 0.5.1 | Teil mit Besitzer-Kennung bzw. Platzhalter „Erkennungsstück von Bewohner X“ in der Raumliste | 1–2 Byte je Teil | Feld 0.5.1, Inhalte nach dem Setting |
| 6 Haupthaus anpassbar | Kombination aus 1, 3, 4: der Speicherstand hält die Raumliste des Haupthauses (überschreibt die Vorlage) | wenige KB im Speicherstand | nach dem Setting |
| 7 Asset-Quellen | Schnittstelle „Quelle“ (eingebettet, LittleFS, später microSD), Laden auf Kern 0 beim Ortswechsel ins PSRAM hinter der Überblendung, nie im Zeichenpfad. SD-Pins 10/11/12/13 sind frei (RGB nutzt 1, 3–9, 14–16, 21, 39–42, 45–48; Licht 2; Touch 19/20/38; Flash/PSRAM 27–37) und sind die Standardpins von SPI2, also volles Tempo | Flash reicht für den Anfang: App 1,5 von 6 MB, LittleFS 3,8 MB frei | 0.5.4 (SD-Treiber und Messung, sobald eine Karte steckt) |
| 8 Eigene Möbel | Browser wandelt PNG → Paletten-Indizes (wie beim Testbild, am Gerät pixelgenau geprüft), Firmware prüft nur Maße und Indizes und speichert in LittleFS (bzw. SD). Kein PNG-Decoder auf dem Gerät | ~1 KB je Möbel (32 × 32) | Format und „Zeichenanleitung für Möbel“ mit `docs/STIL.md` (0.5.8), Upload danach |

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
| 2026-10-18 | Leistungsmonitor (FPS, Last, Speicher) in der Weboberfläche, Log per Schalter unter „Experimentell“ ein/aus (Idee des Users) | ✅ 0.4.2 (Seite Live); Log-Schalter offen |
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
| 2026-10-18 | **Mehrere Darstellungs-Vorschläge** für Dorf und Inselwelt (Perspektive, Ausschnitt, Figurengröße, Kamera), passend zum 5"-Display, mit Simulator-Mockups, bevor gebaut wird | **vorgezogen (2026-10-20):** Planungs-Chat, vor Tilemaps/Kamera in 0.5.x |
| 2026-10-18 | **Systemschrift** (5×7, `engine/src/font.cpp`, Setup-Bildschirm) gefällt dem User: beibehalten, Grundlage für den Stil-Leitfaden | ✅ im Entscheidungsprotokoll festgehalten; STIL.md in 0.5.x |
| 2026-10-18 | **Ausführliche Changelogs** im Stil der PR-Beschreibungen: Neu, Geprüft am Gerät (mit Messwerten), gefundene und behobene Probleme, Hinweise. Alle Änderungen inkl. Fixes seit 0.1.0. Format um Felder „Geprüft“ und „Hinweise“ erweitern, 0.1.0/0.2.0 ausführlich nachtragen (aus PR #1, #3, #4, #5), `tools/release_notes.py` anpassen, Regel in CLAUDE.md. Anzeige später unter „Versionsverlauf“ | ✅ 0.2.1 (Format, Nachtrag, CI-Prüfung, Regel); Anzeige 0.4.x |
| 2026-10-18 | **IP jederzeit sehen:** langer Druck (3 s) aufs Display → Info-Karte mit IP, `pixelia.local`, WLAN-Name, Signal und QR-Code zur Weboberfläche; zusätzlich IP nach jedem Verbinden kurz einblenden; Geste in der Weboberfläche einstellbar | ✅ 0.2.1 (Geste, Info-Karte, Kurzmeldung, Dauer im NVS); 0.3.x (einstellbar in der Oberfläche) |
| 2026-10-18 | **Arbeiten von unterwegs:** Cloud-Sitzungen über claude.ai bzw. die Claude-App (GitHub einmal verbinden), Remote Control, um lokale Sitzungen vom Handy aus weiterzuführen, wenn der PC läuft. Rückweg immer über Pull Requests auf GitHub | bei Bedarf einrichten |
| 2026-10-18 | **USB optional machen:** Updates per WLAN als Werkzeug (`tools/ota.ps1`), Log und Diagnose per WebSocket (`tools/netlog.ps1`) | ✅ 0.2.1 (`tools/ota.ps1`); ✅ 0.2.2 (`tools/netlog.ps1`, `tools/crash.ps1`) |
| 2026-10-19 | **Fernwartung für den Notfall:** Daten, Fehler und Logs komplett per WLAN abrufbar (Board hängt normal per USB am PC), Diagnosepaket, das der User vom Handy aus schicken kann (Wunsch des Users) | ✅ 0.2.2 |
| 2026-10-19 | Task-Liste mit Stack-Reserven ohne Display-Aussetzer (eigene Messung außerhalb der Kernel-Sperre), Grundlage für eine laufende Task-Anzeige | 0.4.x bei Bedarf |
| 2026-10-19 | **Inselwelt: Lagerübersicht in der Weboberfläche** – welche Waren und Gegenstände der Bewohner hat, wie viele, wo sie gelagert sind (Lagerplatz, Kapazität). Genaue Ausarbeitung folgt gemeinsam (Idee des Users). In [welten/inselwelt-konzept.md](welten/inselwelt-konzept.md) vermerkt | Inselwelt (Welt-Seite in der Oberfläche) |
| 2026-10-19 | **Kürzere Changelogs:** technisch, aber höchstens 220 Zeichen pro Stichpunkt und 24 pro Version, Details in die PR-Beschreibung; 0.2.1 und 0.2.2 gekürzt (Wunsch des Users) | ✅ 0.3.0 (Regel, `--check`) |
| 2026-10-19 | **Arbeitsweise mit Sitzung pro Version** und Übergabe übers Repo, damit eine Version nicht das ganze Nutzungslimit braucht ([ARBEITSWEISE.md](ARBEITSWEISE.md)) | ✅ 0.3.0 |
| 2026-10-20 | **Gemeinsames Setting** (Idee des Users): statt getrennter Welten eine große gemeinsame Welt. Zugfahrt, Dungeon usw. sind Module bzw. Orte, die später nachgereicht werden. Figuren wechseln zwischen den Orten (z. B. eine Person geht in den Dungeon, dort sieht man die Idle-Dungeon-Ansicht). Immer nur ein Ort sichtbar und voll simuliert, die anderen laufen vereinfacht („inaktiv“) im Hintergrund weiter. Wird im Planungs-Chat ausgearbeitet und ersetzt dann die Frage „Hauptwelt vor 0.6.0 festlegen“ | Planungs-Chat, vor Modul-Schnittstelle in 0.5.x |
| 2026-10-20 | **Erste Phase von 0.5.x nachprüfen** (ausdrücklicher Wunsch des Users): Alles, was in der ersten Phase von 0.5.x gebaut wird, wird erneut geprüft und an das Setting angepasst, sobald dieses feststeht | nach der Setting-Entscheidung |
| 2026-10-20 | **Download von GitHub robuster:** Bei einem WLAN-Aussetzer (`bcn_timeout`) hing der Download 0.4.1 → 0.4.2 vier Minuten bei 432 KB und brach dann ab (alte Version blieb, zweiter Versuch 20,8 s). Ideen: Abbruch nach ~30 s ohne Fortschritt mit klarer Meldung, automatisch weitermachen per HTTP-Range ab dem letzten Stück | 0.5.x oder später, klein |
| 2026-10-20 | **Hinweis auf eine neue Version auch auf dem Display** (gefunden von der Prüfung im Hintergrund), Entscheidung des Planungs-Chats | 0.5.x (mit den Overlays) |
| 2026-10-20 | Automatische Installation neuer Versionen ohne Nachfrage | ❌ entschieden: nicht, auch nicht als Option; Hinweis reicht (Entscheidung des Users) |
| 2026-10-20 | **Perspektive vor der Engine wählen** (Wunsch des Users): Beispiele, wie die Welt dargestellt wird (Seitenansicht mit Parallax, Querschnitt, 3/4-Draufsicht, isometrisch, seitliche Bühne mit Tiefe), Hof mit Haus, Fluss, Weide bei Tag und Nacht, dazu ein Ortswechsel Hof → Haus → Zug. Erst danach Tilemaps, Kamera, Parallax, Tiefensortierung. Je Ort eigene Perspektive möglich | Planungs-Chat, vor „Raum und Perspektive“ in 0.5.x |
| 2026-10-20 | **Modular wachsende Welt** (Wunsch des Users): Start im Dorf auf einem Hof (Haus, Fluss, Weide als eigene Szenen), später andere Gebäude oder mit dem Zug in ein anderes Dorf; neue Orte (z. B. Zug) kommen als Module per Update dazu und fügen sich ohne Umbau ein | Grundstein ✅ 0.5.0 (Bühne mit Orten, Wechsel mit Übergang, unsichtbare Orte rechnen weiter), Orts-Schnittstelle 0.5.8 nach der Setting-Entscheidung |
| 2026-10-20 | **Standardfarbe der Oberfläche ab Werk „Lavendel“** (#7a5aa6) statt „Moos“; alle sechs Farben bleiben wählbar, eine gespeicherte Wahl bleibt erhalten. Farbstil bleibt „Tonal“ (Rückblick-Runde) | ✅ 0.5.0 (auch das Symbol im Browser-Tab) |
| 2026-10-20 | **Seite System: Versionskarte** ganz oben mit „Pixelia“ und darunter „Version X.Y.Z“ (installierte Version), weil die Version heute schwer zu finden ist | ✅ 0.5.0 |
| 2026-10-20 | **Display um 180° drehen** für den Einbau mit USB oben, Touch mitgedreht, Live-Vorschau richtig herum | gibt es schon seit 0.2.0 (Seite System → Anzeige → „Bilddrehung“; Touch wird mitgedreht, die Vorschau kopiert das Bild vor der Drehung); Beschriftung „Bilddrehung“ bleibt (Entscheidung des Users 2026-10-21) |
| 2026-10-20 | **Uhrzeit-Einblendung:** kleine Pixel-Uhr in einer Ecke des Displays, abschaltbar; Ecke, 24-h-Format, Sekunden als Auswahl mit Empfehlung beim User abfragen | 0.5.x mit den Overlays |
| 2026-10-20 | Handy-Leiste unten vorerst unverändert; **neu ordnen, sobald Menüpunkte zu Pixelia bzw. dem Dorf dazukommen** (Wunsch des Users) | mit den ersten Welt-Seiten |
| 2026-10-20 | **Eigenes Testbild am Display** (Skizzen am echten 5″-Display ansehen): Seite Experimentell, Bild wählen → 2× auf dem Display bis Tippen oder „Beenden“ (Entscheidungen des Users: Ort Experimentell, Dauer bis Tippen, andere Größen pixelgenau anpassen) | ✅ 0.5.0 (am Gerät gemessen; Anzeigen und Tippen zum Beenden vom User geprüft, 2026-10-21) |
| 2026-10-21 | **Perspektive entschieden** (Skizzen v1–v5, `docs/skizzen/`): 3/4-Draufsicht für alle Orte, feste Kamera, ein Bildschirm pro Bereich oder Raum, Wechsel per Sprung mit Überblendung, kein Scrollen; Szene in 200 × 120 groben Bildpunkten (2× in 400 × 240), Kachel 8 × 8 grob, Chibi-Figur 28 Bildpunkte, 4 Richtungen | 0.5.x (Fundament, Raum) |
| 2026-10-21 | **Austauschbare Asset-Quelle** (Setting-Werkstatt, vom User beschlossen): Grafiken aus Flash oder SD-Karte, laden nur beim Ortswechsel auf Kern 0 ins PSRAM hinter einer Überblendung; SD-Karte vorher messen (Tempo, keine Aussetzer, Karte ziehen); Speicherstand bleibt in LittleFS | 0.5.x (Asset-Quelle) |
| 2026-10-21 | **Innenraum-Baukasten** mit Ausbaustufen (Notunterkunft → Raum für Raum), Farbreihen per Palettentausch (auch für das Haus der Hauptfigur über die Weboberfläche), Möbel-Daten (Art, Stellfläche, Benutzungspunkte); Ziel ~20 Häuser. Dazu (Ideen des Users): **Erkennungsstücke** an eine Figur gebunden (Gitarre an der Wand, Geweih beim Jäger); **Haupthaus anpassbar** (Typ, Farbe, Couch-Auswahl) in der Weboberfläche, die Figur baut bzw. streicht dann sichtbar um und braucht dafür Spielzeit. Prüfergebnis siehe 0.5.x „Prüfliste“ | Engine 0.5.x (Raum), Umbau als Aufgabe und Web-Auswahl nach dem Setting |
| 2026-10-21 | **Briefe mit Text** (Idee des Users): Ansicht wie die „Vorschau“ bei den Katzen von Pixelpfote; der User schickt eine Beispieldatei, wenn es so weit ist | Welt-Phase (nach dem Setting) |
| 2026-10-21 | **Gedanken der Bewohner**, auch in der Weboberfläche sichtbar (Idee des Users) | Welt-Phase (nach dem Setting) |
| 2026-10-21 | **Eigene Möbel des Users:** Format im Stil-Leitfaden, Upload über die Weboberfläche nach SD/Flash oder per Update; Machbarkeit prüfen | Format mit `docs/STIL.md`, Upload danach |
| 2026-10-20 | **Easter Egg „So entstand Pixelia“:** Knopf in der Weboberfläche öffnet die frühen Skizzen ([docs/skizzen](skizzen/README.md): fünf Perspektiven, dann die gewählte Mischung), als Rückblick auf die Entwicklung (Wunsch des Users) | später (Weboberfläche) |
| 2026-10-21 | **Easter Eggs (prüfen)** (Wunsch des Users über den Planungs-Chat): klären, wie Pixelia mit Easter Eggs umgeht: welche es gibt, wo sie versteckt sind (Weboberfläche, Display), wie man sie findet und ob sie hinter einem Schalter liegen. Erstes geplantes: die Beispielbilder und Beispiel-Seiten der Perspektiv-Sitzung (Hof, Haus, Dorf, Zug in verschiedenen Perspektiven), abgelegt in [docs/skizzen](skizzen/README.md) (v1–v5 als HTML), in der Weboberfläche als verstecktes Extra ansehen. Bei der Prüfung beachten: Web-Bundle ≤ 150 KB gzip (Bilder evtl. aus LittleFS oder später von der SD-Karte statt eingebettet); Umfang als Auswahlliste dem User vorlegen | später (Weboberfläche), Speicherort mit der Asset-Quelle 0.5.4 |
| 2026-10-19 | Log-Zeilen der WLAN-Bibliothek, die in zwei Teilen kommen (z. B. „wifi:“ + „<ba-add>…“), im Log-Puffer zu einer Zeile zusammenfügen | offen (Kleinigkeit) |
| 2026-10-21 | **Innenräume als Baukasten** (Wunsch des Users): Wände, Böden, Fenster, Türen, Möbel und Deko als einzelne Teile, ein Raum ist eine Datenliste; 20 verschiedene Häuser ohne 20 gemalte Bilder ([SETTING.md](SETTING.md) Kapitel 8) | 0.5.1, Prüfliste Punkt 1 (Engine-Baustein), Inhalte in den Welt-Phasen |
| 2026-10-21 | **Ausbaustufen von Möbeln und Räumen** über denselben Baukasten (Notunterkunft mit Kisten und Strohsack → Raum für Raum ausgebaut; Strohsack → Bett) | 0.5.1, Prüfliste Punkt 2 (Engine-Baustein) |
| 2026-10-21 | **Farbvarianten** für Bett, Sofa, Vorhänge, Tapete usw. per Palettentausch (einmal zeichnen, Farbreihe austauschbar, kein Zusatzspeicher) | 0.5.0/0.5.2, Prüfliste Punkt 3 (Paletten, Baukasten) |
| 2026-10-21 | **Erkennungsstücke** pro Haushalt (Gitarre bei Taylor, Geweih beim Jäger, Mehlsäcke bei der Bäckerin), Liste in SETTING.md Kapitel 3 | 0.5.1 Feld, Prüfliste Punkt 5; Inhalte in den Welt-Phasen |
| 2026-10-21 | **Haus der Hauptfigur anpassbar** in der Weboberfläche (Haustyp, Farben, Möbel wie verschiedene Sofas); die Figur baut bzw. streicht um, das kostet Zeit und Material | Prüfliste Punkt 6; Welt-Phase Hof/Dorf |
| 2026-10-21 | **Austauschbare Asset-Quelle** (Flash oder SD-Karte): erst ohne Karte, später bei Bedarf alle Orte auf die Karte umziehen, Laden beim Ortswechsel hinter einer Überblendung; vorher SD-Karte messen (Lesegeschwindigkeit, keine Bildaussetzer, Karte ziehen) | 0.5.4, Prüfliste Punkt 7 (Asset-Pipeline) |
| 2026-10-21 | **Eigene Möbel des Users** (Machbarkeit prüfen): Maße im Raster und Palette als Vorgabe, Daten mit Stellfläche und Benutzungspunkten, Upload über die Weboberfläche (Speicher SD oder Flash) oder per Update ins Repo | 0.5.8, Prüfliste Punkt 8 (Format im Stil-Leitfaden), Umsetzung später |
| 2026-10-21 | **Zustand „im Bau“** (Wunsch des Users bei der Design-Frage 0.5.1): wo ein Möbel hinkommt, steht während der Bauphase ein halbfertiges Teil oder liegt Material; Knopf „Im Bau“ auf Experimentell | ✅ 0.5.1 (Engine-Baustein und Testraum), Bau als Aufgabe der Figuren nach dem Setting |
