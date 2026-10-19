# Pixelia – Code-Landkarte

> Kurzer Wegweiser durch den Code (Stand 0.3.0). Wer neu einsteigt, liest hier, statt den halben Code zu öffnen.
> Begründungen stehen in [PROJEKTPLAN.md](PROJEKTPLAN.md), Messwerte und Schnittstellen im Detail in
> [HARDWARE.md](HARDWARE.md). Am Ende jeder Phase aktualisieren.

## Ordner

| Ordner | Inhalt |
|---|---|
| `engine/` | Plattformunabhängige Engine (Gerät **und** Simulator), keine ESP-IDF-Header. Namensraum `pixelia` |
| `firmware/` | ESP-IDF-Projekt: `main/` (App), `components/board/` (Display, Touch, Hintergrundbeleuchtung) |
| `sim/` | PC-Simulator (SDL3-Fenster oder kopflos), Selbsttests `--test` |
| `web/` | Weboberfläche: Svelte 5 + TypeScript + Vite, eigene M3-Expressive-Komponenten (siehe unten) |
| `worlds/`, `assets/` | noch leer bzw. README und Credits (Welten ab 0.5/0.6) |
| `tools/` | PowerShell-Werkzeuge für Build, Flashen, Log, Updates, Abstürze (siehe unten) |
| `pages/` | Web-Flasher auf GitHub Pages (ESP Web Tools) |
| `docs/` | Projektplan, Roadmap, Hardware, Lebendigkeit, diese Landkarte, `welten/` |

## Engine (`engine/include/pixelia/engine/`, Quellen in `engine/src/`)

| Datei | Aufgabe |
|---|---|
| `canvas.h` | Zeichenfläche 400×240, 1 Byte pro Pixel (Paletten-Index) |
| `color.h` | Palette RGB888 ↔ RGB565 |
| `scene.h`, `scenes.cpp` | Schnittstelle `pixelia::Scene` (tick/render), `make_scene("scrolltest")` |
| `scroll_test.h` | Testszene mit Tag/Nacht, Wolken, Parallax, Vögeln |
| `world_clock.h` | Weltzeit mit Zeitraffer (`kTicksPerSecond = 30`) |
| `system_ui.h` | Systemanzeigen: Setup, Update, Meldung, Kurzmeldung, Info-Karte, Halte-Anzeige (Paletten 240–255) |
| `font.h`, `tiny_font.h` | Systemschrift 5×7 mit Umlauten; 3×5-Prüfschrift |
| `qr.h`, `png.h` | eigener QR-Code-Erzeuger und PNG-Encoder |
| `gesture.h` | Tippen und langer Druck aus rohen Touch-Daten |
| `rng.h` | deterministischer Zufall (PCG32) |
| `redact.h` | Passwort-Filter für Diagnose-Ausgaben (`SecretFilter`) |

## Firmware (`firmware/main/`)

Start in `main.cpp` (`app_main`): Log-Puffer → Systeminfo → NVS-Einstellungen → Update-Zustand → Diagnose
(Startzähler) → I²C/Touch → Hintergrundbeleuchtung → Grafik auf Kern 1 → Konsole, Leistungsmonitor → WLAN →
Webserver → Selbsttest (nur im Probelauf).

| Datei | Aufgabe |
|---|---|
| `app.h` | gemeinsame Schnittstellen: `sysinfo`, `hwtest`, `gfx`, `perf`, `console`; Kerne `kGfxCore=1`, `kSystemCore=0` |
| `gfx.cpp` | Grafik-Task (Kern 1): Panel anlegen, Szene ticken/rendern, Systemanzeigen darüber, Puffertausch |
| `ui.h` (in `gfx.cpp`) | Systemanzeigen von jeder Task aus ein-/ausblenden (`toast`, `show_update`, `show_info` …) |
| `input.cpp/.h` | Touch → Gesten → Aktionen (Info-Karte), Daten der Info-Karte |
| `hwtest.cpp` | I²C-Scan, Touch-Task (Polling alle 20 ms), geführte Tests `tour`, Flash-Last, Helligkeit |
| `perf.cpp` | Leistungsmonitor alle 2 s (Snapshot für API), Zeile seriell, im Log-Puffer 1×/min; ruft `diag::heartbeat()` |
| `console.cpp` | Diagnose-Befehle (seriell und `run_remote` für `/api/cmd`), Liste `kDangerous` |
| `logbuf.cpp/.h` | Log-Hook (`esp_log_set_vprintf`), Ringpuffer 256 KB PSRAM No-Init + RTC-Sicherung, `read()`, `Capture` |
| `diag.cpp/.h` | Startzähler, Reset-Historie (NVS), Herzschlag (RTC), Core-Dump-Infos, Task-Liste, JSON-Bausteine |
| `settings.cpp/.h` | NVS-Einstellungen: Drehung, Helligkeit, langer Druck |
| `wifi.cpp`, `net.h` | WLAN-Manager (5 Netze), Setup-Modus mit AP, mDNS; `for_each_secret` für den Filter |
| `captive_dns.cpp` | Mini-DNS fürs Captive Portal (nur im Setup-WLAN) |
| `timesync.cpp` | Zeitzone Europa/Berlin, NTP |
| `ota.cpp/.h` | Update-Upload mit Prüfung, Probelauf mit Selbsttest, Rückfall-Erkennung, Testvarianten (`PIXELIA_TEST`) |
| `web.cpp` | Webserver: Oberfläche (`/`, auch im Setup-Modus; Dateien aus `web_assets.h`, ETag), Einrichtungsseite (`/setup`), Status-JSON, WLAN, Einstellungen, Update, Captive Portal, WebSocket `/ws` (empfängt Geräte-Anmeldungen) |
| `web_info.cpp` | `/api/system` (Hardware, Netz, Partitionen, Temperatur), `/api/clients` + `/api/clients/name` (verbundene Geräte, NVS `clients`), `client_message()` für `/ws` |
| `web_assets.h` | Tabelle der eingebetteten Oberflächen-Dateien; `web_assets.cpp` erzeugt `tools/web_embed.py` beim Build im Build-Ordner |
| `web_diag.cpp` | Fernwartung: `/api/log`, `/ws/log`, `/api/cmd`, `/api/perf`, `/api/resets`, `/api/coredump*`, `/api/diag` |
| `web_internal.h` | gemeinsame Web-Hilfen (`send_json`, `form_value`, `authorized` = PIN-Prüfstelle) |
| `json.cpp/.h` | kleiner JSON-Schreiber (bewusst nicht inline: Codegröße) |
| `web/setup.html` | einfache Einrichtungsseite im M3-Look (eingebettet, ohne Build-Schritt, Notfallweg), Abfrage per WebSocket bzw. alle 2 s |

`components/board/`: `display.*` (RGB-Panel ohne Bildspeicher, Bounce-Buffer-Callback mit Palette + 2×, Doppelpuffer,
Zähler für Interrupt-Last und Aussetzer), `touch_gt911.*`, `backlight.*` (LEDC 200 Hz), `pins.h`.

## Weboberfläche (`web/`)

Build: `npm run build` → `web/dist` (feste Namen `index.html`, `app.js`, `app.css`, Schrift). Der Firmware-Build ruft
`tools/web_embed.py` auf: baut bei Bedarf neu, gzip, erzeugt `web_assets.cpp`. Entwicklung: `npm run dev` (Proxy aufs
Gerät, `PIXELIA_TARGET`), `npm run mock` (ohne Gerät, Beispielwerte), `npm run check` (Typen, läuft in der CI).

| Datei | Aufgabe |
|---|---|
| `src/main.ts`, `src/App.svelte` | Start, Rahmen: Navigation (Handy 5 Ziele + „Mehr“, Schiene alle), obere Leiste, Hash-Routen, Setup-Modus → `#/wlan` |
| `src/pages/` | `Overview` (Übersicht), `Wifi` (`#/wlan`), `System`, `Log`, `Info`, `Hardware`, `Devices` (`#/geraete`), `Appearance` (`#/darstellung`), `More` (`#/mehr`) |
| `src/lib/status.svelte.ts` | Live-Status: WebSocket `/ws`, Rückfall auf `/api/status`, Typ `Status` (= `status_json()` in web.cpp); meldet Geräte-ID und Seite |
| `src/lib/api.ts` | `get`/`post` (Formulardaten) und Antworttypen der API (`SystemInfo`, `ClientList`, `WifiList`, `ResetList`, `LogLine` …) |
| `src/lib/client.ts` | Geräte-ID (localStorage `pixelia.client`) und Kurzname „Browser · System“ aus der Browser-Kennung |
| `src/lib/firmware.ts` | Update: Kopf der .bin prüfen (wie setup.html), Hochladen mit Fortschritt und 30-s-Wächter |
| `src/lib/page.css` | globale Seiten-Bausteine: `.page-grid`, `.card-title`, `.kv`, `.list`/`.list-item`, `.meter-row` |
| `src/lib/format.ts`, `Logo.svelte`, `SignalBars.svelte` | Anzeige-Hilfen (Zahlen, Dauer, Signal), Pixelia-Zeichen, WLAN-Balken |
| `src/lib/m3/` | M3-Expressive-Bibliothek: `tokens.css` (Formen, Typo, Federn), `theme.svelte.ts` (dynamische Farben), `Button`, `IconButton`, `ButtonGroup`, `Card`, `Chip`, `Navigation`, `WavyProgress`, `CircularProgress`, `LoadingIndicator`, `TextField`, `Slider`, `Dialog` (`confirmDialog()`), `shapes.ts`, `Icon` |
| `src/lib/m3/icon-names.ts` | Liste der benutzten Material Symbols; nur diese bettet das Vite-Plugin in `vite.config.ts` ein |
| `src/assets/roboto-flex-pixelia.woff2` | reduzierte Schrift (aus `tools/font_subset.py`, im Repo) |

## Tasks und Kerne

| Task | Kern | Prio | Aufgabe |
|---|---|---|---|
| `gfx` | 1 | 10 | Engine 30 fps; Display-Interrupt läuft ebenfalls auf Kern 1 |
| `touch` | 0 | 5 | GT911 alle 20 ms, Gesten |
| `wlan` | 0 | 5 | Verbinden, Setup-Modus, Neuversuche |
| `httpd` | 0 | 5 | Webserver (eine Task für alle Anfragen und WebSocket-Sendungen) |
| `console` | 0 | 4 | serielle Befehle und Befehle aus `/api/cmd` |
| `dns` | 0 | 4 | nur im Setup-Modus |
| `selbsttest` | 0 | 3 | nur im Probelauf, beendet sich danach |
| `perf`, `webstatus`, `logpush` | 0 | 2 | Leistung alle 2 s, Status-WebSocket 1/s, Live-Log 5/s |

Regeln: Kern 1 gehört der Grafik. Nichts regelmäßig aufrufen, was die FreeRTOS-Kernel-Sperre lange hält (z. B.
`uxTaskGetSystemState`, 1,1 ms → Display-Aussetzer). Allokationen < 16 KB landen im internen RAM, größere im PSRAM.

## Speicher

- Flash: `partitions.csv` – nvs 24 KB, otadata, ota_0/ota_1 je 6 MB, storage (LittleFS, noch ungenutzt), coredump 64 KB.
- NVS-Namensräume: `pixelia` (Einstellungen), `wifi` (Netze, Setup-Passwort, Hinweise), `ota` (Rückfall-Grund),
  `diag` (Startzähler `boot`, Reset-Historie `r00`–`r15`), `clients` (Blob `list`: bis 12 Browser, je 92 Byte).
- PSRAM No-Init: Log-Puffer (Lage hängt an der Codegröße). RTC-Speicher No-Init: Log-Sicherung 6 KB, Herzschlag.

## Schnittstellen

- **Konsole** (seriell 115200 und `/api/cmd`): `help` listet alles; Übersicht in HARDWARE.md „Diagnose-Schnittstelle“.
- **HTTP/JSON** (`web.cpp`): `/api/status`, `/api/wifi`, `/api/wifi/scan|add|remove`, `/api/settings`, `/api/ota`,
  `/api/ota/ack`, `/api/reboot`, `/api/speedtest`; WebSocket `/ws` (Status 1/s; vom Browser `hello=<id>&ua=…&page=…`).
- **Info** (`web_info.cpp`): `/api/system`, `/api/clients`, `/api/clients/name`.
- **Fernwartung** (`web_diag.cpp`): siehe HARDWARE.md „Fernwartung per WLAN“.
- WebSocket-Anmeldung über `ws_post_handshake_cb` (ESP-IDF 6.1 ruft beim Handschlag keinen Handler auf).

## Simulator (`sim/src/`)

`main.cpp` (Befehlszeile), `window.cpp` (SDL3, Maus = Touch), `headless.cpp` (Screenshot, Zeitraffer, Kontaktbogen,
`--screen` für Systemanzeigen), `tests.cpp` (`--test gestures|redact|all`, läuft in der CI).

## Werkzeuge (`tools/`, Windows)

| Werkzeug | Zweck |
|---|---|
| `idf.ps1` | `idf.py` mit Umgebung, gekürzte Ausgabe, Log in `.logs/idf-last.log` |
| `ota.ps1` | Update per WLAN mit Prüfung von Version und Selbsttest (Standardweg) |
| `netlog.ps1` | Live-Log, Befehle (`-Send`), frühere Starts (`-Boot prev`) per WLAN |
| `crash.ps1` | Absturzbericht per WLAN oder aus Diagnosepaket holen und dekodieren |
| `flash.ps1`, `monitor.ps1` (+ `serial_log.py`) | USB: Erstinstallation, serielles Log |
| `sim.ps1` | Simulator bauen und starten (`-Headless`) |
| `release_notes.py` | CHANGELOG.json prüfen (`--check`) und Release-Notes erzeugen |
| `web_embed.py` | Oberfläche für die Firmware einbetten (läuft im Firmware-Build) |
| `web_screens.mjs` | Screenshots der Oberfläche (Handy/Desktop, hell/dunkel) per Edge kopflos, `--url http://pixelia` fürs Gerät |
| `font_subset.py` | Roboto Flex reduzieren (nur bei neuen Zeichen, braucht `pip install fonttools brotli`) |

## CI (`.github/workflows/`)

`firmware.yml` (Job `web`: Oberfläche bauen und prüfen; dann Firmware-Build, Größenbudget), `simulator.yml` (Linux-Build, Selbsttests, Screenshots), `release.yml`
(ebenfalls mit Job `web`; Tag `vX.Y.Z` → Release mit `.bin`, Factory-Image, `.elf`, Notes), `pages.yml` (Web-Flasher nach jedem Release).
