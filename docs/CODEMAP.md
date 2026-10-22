# Pixelia – Code-Landkarte

> Kurzer Wegweiser durch den Code (Stand 0.5.3). Wer neu einsteigt, liest hier, statt den halben Code zu öffnen.
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
| `canvas.h` | Zeichenfläche 400×240, 1 Byte pro Pixel (Paletten-Index); grobes Raster `kCoarseWidth/Height` 200×120, `kSceneColors` 240; `blit` mit Spiegeln und Farbreihe, `blit_clipped` (seit 0.5.2 optional im Licht: `LightBlit`), `Rect`; `CoarseLayers` (seit 0.5.2: Hintergrund-Schicht, Bereiche der bewegten Teile je Zeile, Lichtkarte; seit 0.5.3 Schleier: Farbtabelle + Anteil je Feld 4 × 4, `kVeilW/H` 50 × 30) |
| `color.h` | Palette RGB888 ↔ RGB565 |
| `light.h` | Farbe und Licht (seit 0.5.2): `PlacePalette` (Grundfarben, drei Lichtstufen à 80, Rollen leuchtend/Himmel/fest, Farbrotation `ColorCycle`, Farbtabellen `tint_table`/`blend_table`), `Daylight` (Tageslicht und Himmel aus Stützpunkten, Mindesthelligkeit, Vorgaben `daylight_preset`), `LightCircle`, `build_light_map` (Stufen der vier feinen Bildpunkte je grobem, 2 Bit), `light_level_at`, `dither4`; Aufbau der Palette: docs/STIL.md |
| `weather.h` | Wetter (seit 0.5.3): `WeatherKind` (sonne … wind), `Weather` (Ziel, weiche Übergänge, Böen, Blitze 12–30 s, `drift`, `apply` aufs Umgebungslicht: Wolken, Dunst, Blitz), `PlaceWeather` je Ort (Partikel nur betreten, Vorlauf beim Betreten; Schleier Nebelbänke/Regenschleier/Wolkenschatten über `tint_table`, Rauschen je Feld; drinnen nur über Glasfarben), `WeatherLook`, Stile `RainStyle`/`SnowStyle`/`FogStyle`/`FlashStyle` (Vergleichsbefehle regen, schnee, nebelstil, blitzstil) |
| `particles.h` | Partikel (seit 0.5.3): `Particle` (16 Byte), `ParticlePool` mit Obergrenze im Grafik-Speicher |
| `environment.h` | Umgebung der Orte (seit 0.5.3): `Environment` (Wetter + Lichtquellen) hält die Bühne; `LightBoard` (an/aus je Ort und Quelle, `version()`), `WindowLight` (Fenster am Außenort → Raum dahinter) |
| `scene.h`, `scenes.cpp` | Schnittstelle `pixelia::Scene` (tick rechnet, render zeichnet nur; `scale()` 1 = fein, 2 = grob, `colors()`, `render_overlay()` fein; seit 0.5.1 `enter/leave`, `take_travel/arrive` mit `Travel`, `set_inspect`, `command`; seit 0.5.3 `render_fine` (Partikel nach dem Verdoppeln), `set_environment`), `Transition`, `make_scene`, `add_standard_scenes` (scrolltest, testort-wiese, testort-raum, testort-platz) |
| `stage.h` | Bühne (seit 0.5.0): mehrere Orte, alle rechnen, sichtbarer wird gezeichnet; `go()` mit Übergang (Abblenden Standard, Überblendung, Raster, Schnitt; gemeinsame Palette während der Überblendung, seit 0.5.3 mit Toleranz: ähnliche Farben zusammengelegt, statt abzublenden), `upscale2x` (seit 0.5.2 mit `CoarseLayers`: direkt aus Hintergrund und bewegten Teilen, Licht fein gerastert; `compose` der Überblendung ebenso), `flatten_layers`; seit 0.5.1 Betreten/Verlassen, Reisende zwischen Orten (`travels()`), `set_follow`, `set_inspect`; seit 0.5.3 `environment()`/`weather()` (Wetter rechnet vor den Orten), `command()` (wetter, wind, blitz, blitze, sonst an den Ort), Schleier in `upscale2x` (Bitmuster je Feld, Zeilen ohne Schleier im schnellen Weg) |
| `tiles.h` | Kacheln (seit 0.5.1): `TileSet` (je 64 Byte, Eigenschaft `kTileBlocks`), `TileMap` mit drei Ebenen (Boden, Wand, Deko), `load()` aus Textzeilen mit Legende, `draw()` deckend oder mit Durchsicht, Farbreihe je Ebene |
| `depth.h` | Tiefensortierung (seit 0.5.1): `DepthList` (192 Einträge) nach Fußpunkt, stabil, Tiefenversatz, Schatten; eingebackene Teile nur beschnitten über bewegten neu; seit 0.5.2 `use_layers` (Hintergrund nur unter bewegten Teilen kopieren), `use_light` (Figuren gleichmäßig nach dem Fußpunkt), `set_shadow_table` (durchscheinende Schatten) |
| `room.h` | Raum-Baukasten (seit 0.5.1): `PartDef`/`PartStage`/`UsePoint`/`PartCatalog` (Art, Ebene, Ausbaustufen, Stellfläche, Benutzungspunkte, Bau-Bild, Materialstapel, Erkennungsstück), `RoomPart` (7 Byte), `Exit`, `RoomDef`; `Room`: Raumliste änderbar, `set_stage`, `set_building`, Hintergrundbild bei `enter()`, `begin()` → Tiefenliste, `step_toward()` (Breitensuche), `draw_inspect()`; Arbeitsspeicher ~8 KB im Grafik-Speicher; seit 0.5.2 `begin(…, layers)`, `set_lights` (Lichtkarte 24 KB, einmal je Änderung), `set_light_on` (tagsüber aus), `set_shadow_table` |
| `sprite.h` | Bildstreifen mit Fußpunkt, `Facing`/`view_of` (4 Richtungen aus 3 Ansichten), `Animation`/`Animator` in Logik-Schritten, `Recolor` (Farbreihe, 256 Byte) |
| `memory.h` | Speicher für Grafiken (`asset_alloc`), auf dem Gerät PSRAM |
| `test_places.h` | Testorte Wiese, Raum, Platz (seit 0.5.1 aus Kacheln und Möbel-Teilen, Tür und Bildrand verbunden): Katalog mit 14 Teilen, Bau-Bilder, Farbreihen (sechs Figuren, Holz, Tapete), Figuren auf dem Kachelraster mit Benutzungspunkten und Reisen, Begleiterin; Befehle `ausbau`, `baustelle`; Objekte im Grafik-Speicher; seit 0.5.2 Palette mit Lichtstufen und Tageslicht (`tick` rechnet sie aus), Lampe als Lichtkreis, Fenster am Abend (Wiese) bzw. Himmel (Raum), Wasserbecken (Platz), Befehle `stimmung`, `tageslicht`, `nacht`, `licht fein|grob|hart [innen %]`; seit 0.5.3 Wetter je Ort (`PlaceWeather`, draußen Wiese/Platz, Raum in den Fenstern), Lampe und Laterne im `LightBoard`, Fenster der Wiese gehören zum Raum; Befehle `innenlicht an|aus|um`, `regen striche|tropfen|fein`, `schnee gemischt|klein|gross`, `dichte <%>`, `nebelbaenke <%>` |
| `scroll_test.h` | Testszene „Landschaft“ mit Tag/Nacht, Wolken, Parallax, Vögeln (fein, Farben 0–79) |
| `world_clock.h` | Weltzeit mit Zeitraffer (`kTicksPerSecond = 30`) |
| `system_ui.h` | Systemanzeigen: Setup, Update, Meldung, Kurzmeldung, Info-Karte, Halte-Anzeige (Paletten 240–255) |
| `font.h`, `tiny_font.h` | Systemschrift 5×7 mit Umlauten; 3×5-Prüfschrift |
| `qr.h`, `png.h` | eigener QR-Code-Erzeuger und PNG-Encoder |
| `gesture.h` | Tippen und langer Druck aus rohen Touch-Daten |
| `rng.h` | deterministischer Zufall (PCG32) |
| `redact.h` | Passwort-Filter für Diagnose-Ausgaben (`SecretFilter`) |

## Firmware (`firmware/main/`)

Start in `main.cpp` (`app_main`): Log-Puffer → Systeminfo → NVS-Einstellungen → Experimentell (Panel-Startwerte,
Fluchtweg) → PIN → Update-Zustand → GitHub (Kanal, Hintergrundprüfung) → Diagnose (Startzähler) → I²C/Touch →
Hintergrundbeleuchtung → Grafik auf Kern 1 → Konsole, Leistungsmonitor → WLAN → Webserver → Selbsttest (nur im
Probelauf).

| Datei | Aufgabe |
|---|---|
| `app.h` | gemeinsame Schnittstellen: `sysinfo`, `hwtest`, `gfx`, `perf`, `console`; Kerne `kGfxCore=1`, `kSystemCore=0` |
| `gfx.cpp` | Grafik-Task (Kern 1): seit 0.5.3 Befehle an die Bühne als Warteschlange (4 × 47 Zeichen, `place_command`), Wetterstand in `EngineStats`; Panel anlegen, Bühne ticken/rendern (seit 0.5.0; Orte, Übergang, Rundgang, Figuren als Anfragen anderer Tasks; grober Puffer und Grafiken im PSRAM), Systemanzeigen darüber, Puffertausch; `grab()` kopiert für die Live-Vorschau das fertige Bild samt Palette vor dem Tausch (seit 0.4.2) |
| `ui.h` (in `gfx.cpp`) | Systemanzeigen von jeder Task aus ein-/ausblenden (`toast`, `show_update`, `show_info` …); seit 0.5.0 `show_picture`/`hide_picture` (eigenes Testbild, Anzeige 5, eigene Sperre `s_pic_lock`, die Welt rechnet weiter) |
| `input.cpp/.h` | Touch → Gesten → Aktionen (Info-Karte), Daten der Info-Karte, Zurücksetzen am Gerät (Knopf 3 s halten: PIN entfernen, Werksreset) |
| `hwtest.cpp` | I²C-Scan, Touch-Task (Polling alle 20 ms), geführte Tests `tour`, Flash-Last, Helligkeit; `touch_live()` für den Touch-Test |
| `exper.cpp/.h` | Seite Experimentell (seit 0.4.2): Schalter (NVS `exp`), Panel-Timing und Stromsparmodus als Probe mit Rückfall nach 15 s (Zeitgeber `probe`), Speichern erst nach Bestätigung (Blob `panel`), Fluchtweg über Startzähler `exp_boot`, Testbilder mit Rückkehr nach 60 s |
| `perf.cpp` | Leistungsmonitor alle 2 s (Snapshot für API), Zeile seriell, im Log-Puffer 1×/min; ruft `diag::heartbeat()` |
| `console.cpp` | Diagnose-Befehle (seriell und `run_remote` für `/api/cmd`), Liste `kDangerous` |
| `logbuf.cpp/.h` | Log-Hook (`esp_log_set_vprintf`), Ringpuffer 256 KB PSRAM No-Init + RTC-Sicherung, `read()`, `Capture` |
| `diag.cpp/.h` | Startzähler, Reset-Historie (NVS), Herzschlag (RTC), Core-Dump-Infos, Task-Liste, JSON-Bausteine |
| `settings.cpp/.h` | NVS-Einstellungen: Drehung, Helligkeit, langer Druck, Farbwahl der Oberfläche (`ui`); `factory_reset()`, `frozen()` |
| `auth.cpp/.h` | optionaler PIN: gesalzener Hash im NVS, Sitzungs-Tokens im RAM, Sperre nach Fehlversuchen |
| `wifi.cpp`, `net.h` | WLAN-Manager (5 Netze), Setup-Modus mit AP, mDNS; `for_each_secret` für den Filter |
| `captive_dns.cpp` | Mini-DNS fürs Captive Portal (nur im Setup-WLAN) |
| `timesync.cpp` | Zeitzone Europa/Berlin, NTP |
| `ota.cpp/.h` | Update in Stücken (Upload oder GitHub, `Status.source`) mit Prüfung, Probelauf mit Selbsttest, Rückfall-Erkennung, Testvarianten (`PIXELIA_TEST`) |
| `github.cpp/.h` | Update von GitHub (seit 0.4.1): Releases-API (`per_page=10`, Antwort im PSRAM, eigener JSON-Leser), Kanal Stabil/Vorschau (NVS `pixelia/upd_ch`), Versionsvergleich wie `version_key`, Download per HTTPS (`esp_http_client`, Weiterleitungen, `esp_crt_bundle`) in `ota::begin/write/finish`; Task `github`. Seit 0.4.2: Abbrechen (`cancel_install`), Prüfung im Hintergrund (Zeitgeber `upd-auto` alle 5 min, höchstens täglich), ältere Versionen ab 0.4.0 (`start_install(v, older)`), `available()`, Zähler `seq()` |
| `web.cpp` | Webserver: Oberfläche (`/`, auch im Setup-Modus; Dateien aus `web_assets.h`, ETag), Einrichtungsseite (`/setup`), Status-JSON, WLAN, Einstellungen, Update, Captive Portal, WebSocket `/ws` (empfängt Geräte-Anmeldungen), Prüfstelle `authorized()`, Anmeldung, PIN, Werksreset, Sicherung |
| `web_info.cpp` | `/api/system` (Hardware, Netz, Partitionen, Temperatur), `/api/clients` + `/api/clients/name` (verbundene Geräte, NVS `clients`), `client_message()` für `/ws` |
| `web_assets.h` | Tabelle der eingebetteten Oberflächen-Dateien; `web_assets.cpp` erzeugt `tools/web_embed.py` beim Build im Build-Ordner. Dazu `kChangelog` (Versionsverlauf, gzip): `changelog_block.cpp` erzeugt `tools/release_notes.py --embed` aus `CHANGELOG.json`, Block mit Marker `PIXELIA:CHLOG:v1` + Länge + CRC-32 (auch aus der .bin lesbar) |
| `web_diag.cpp` | Fernwartung: `/api/log`, `/ws/log`, `/api/cmd`, `/api/perf`, `/api/resets`, `/api/coredump*`, `/api/diag` |
| `web_exp.cpp` | Seite Experimentell (`/api/exp`) und Live-Vorschau (`/api/screen`, PackBits, Kopf `Server-Timing`), seit 0.4.2; eigenes Testbild (`POST /api/picture`, 96 000 Indizes + Palette 240 × RGB, direkt ins PSRAM) seit 0.5.0 |
| `web_internal.h` | gemeinsame Web-Hilfen (`send_json`, `form_value`, `authorized` = PIN-Prüfstelle, `deny` = 401/429) |
| `json.cpp/.h` | kleiner JSON-Schreiber (bewusst nicht inline: Codegröße) |
| `web/setup.html` | einfache Einrichtungsseite im M3-Look (eingebettet, ohne Build-Schritt, Notfallweg), Abfrage per WebSocket bzw. alle 2 s |

`components/board/`: `display.*` (RGB-Panel ohne Bildspeicher, Bounce-Buffer-Callback mit Palette + 2×, Doppelpuffer,
Zähler für Interrupt-Last und Aussetzer), `touch_gt911.*`, `backlight.*` (LEDC 200 Hz), `pins.h`.

## Weboberfläche (`web/`)

Build: `npm run build` → `web/dist` (feste Namen `index.html`, `app.js`, `app.css`, Schrift). Der Firmware-Build ruft
`tools/web_embed.py` auf: baut bei Bedarf neu, gzip, erzeugt `web_assets.cpp`. Entwicklung: `npm run dev` (Proxy aufs
Gerät, `PIXELIA_TARGET`), `npm run mock` (ohne Gerät, Beispielwerte; `PIXELIA_MOCK=setup`, `PIXELIA_MOCK_PIN=1234`, `PIXELIA_MOCK_CRASH=0`, `PIXELIA_MOCK_GH=limit|fail` für das Update von GitHub), `npm run check` (Typen, läuft in der CI). Seit 0.4.2 im Mock auch Vorschau, Leistung, Experimentell
(`PIXELIA_MOCK_EXP=1` startet eingeschaltet) und ältere Versionen (`PIXELIA_MOCK_V=0.4.1` = installierte Version).
Eine Datei `app.js` ohne Code-Splitting: jede Datei bräuchte einen eigenen URI-Handler (Grenze 60, belegt 54).

| Datei | Aufgabe |
|---|---|
| `src/main.ts`, `src/App.svelte` | Start, Rahmen (seit 0.5.4): Navigation mit vier Zielen Welt, Übersicht, System, Einstellungen (Leiste unten, in der breiten Ansicht Schiene), obere Leiste mit Zurück-Pfeil auf Unterseiten (`parent`), Hash-Routen inkl. `#/experimentell/orte`, Weiterleitung alter Adressen (`info`, `hardware`, `mehr`), Setup-Modus → `#/wlan`, 7× Logo → „So entstand Pixelia“ |
| `src/pages/` (Ziele) | `World` (`#/welt`, Ausblick), `Overview` (Übersicht mit „Neu in dieser Version“), `SystemHub` (`#/system`: Wartung, Technik, Labor), `SettingsHub` (`#/einstellungen`: Verbindung, Gerät, Oberfläche) |
| `src/pages/` (System) | `Update` (`#/update`, GitHub und Datei mit Kurzfassung), `Versions` (`#/versionen`), `Backup` (`#/sicherung`), `Restart` (`#/neustart`), `Live` (`#/live`, Leistung und Live-Bild), `Log`, `Diagnostics` (`#/diagnose`, drei Neustarts, Rest aufklappbar), `Hardware` (`#/geraet`, Gerät und Speicher, früher Info + Hardware), `Tests` (`#/tests`, Gerätetests ohne Schalter), `Experimental` (`#/experimentell[/orte|/panel|/version|/geschichte]`) |
| `src/pages/` (Einstellungen) | `Wifi` (`#/wlan`), `Devices` (`#/geraete`), `Display` (`#/display`), `Protection` (`#/schutz`), `Appearance` (`#/darstellung`, Farben auf dem Gerät, Ansicht/Schrift/Startseite/Bewegung je Browser) |
| `src/lib/HubList.svelte` | Liste mit Kategorien (Gruppentitel, Kachel-Zeilen mit Symbol, Zustand, Pfeil); in der breiten Ansicht Gruppen nebeneinander |
| `src/lib/prefs.svelte.ts` | Darstellung je Browser (localStorage `pixelia.prefs`): `wide`, `font`, `reduce`, `start`, `seen` (Karte „Neu in dieser Version“); setzt `html.wide`, `html.reduce`, `data-font` |
| `src/lib/exp.svelte.ts` | Zustand von `/api/exp` für Experimentell und Gerätetests (Laden, Abfrage-Schleife, Aktionen) |
| `src/lib/story.ts`, `Story.svelte` | „So entstand Pixelia“: Zeitleiste (Daten in `story.ts`, neue Stände unten anhängen), Skizzen-Links auf GitHub Pages; als Blatt oder Seite (`inline`) |
| `src/lib/picture.ts` | eigenes Testbild (seit 0.5.0): Bilddatei dekodieren, pixelgenau auf 400 × 240, Farbtiefe bis ≤ 240 Farben senken; `post()` schickt auch einen Blob |
| `src/lib/Sparkline.svelte` | kleiner Verlauf als SVG (eine oder zwei Reihen, Grenzlinie), für die Leistungsanzeige |
| `src/lib/changelog.ts`, `ChangeEntry.svelte` | Versionsverlauf (seit 0.5.4 nur Kurzfassung `k`: Version als Chip, Abschnitte mit „**Stichwort:** Satz“, Link zu GitHub; ältere Einträge mit allen Abschnitten): `/api/changelog`, Block aus einer .bin lesen (`logFromBin`: Marker, CRC, `DecompressionStream`), Release-Notes von GitHub als Eintrag (`entryFromNotes`, liest „Kurz gesagt“ als `k`), Versionsvergleich (`cmpVersion`, `parseFw`, `installedEntry`); Anzeige eines Eintrags mit allen Abschnitten |
| `src/lib/status.svelte.ts` | Live-Status: WebSocket `/ws`, Rückfall auf `/api/status`, Typ `Status` (= `status_json()` in web.cpp); meldet Geräte-ID und Seite |
| `src/lib/api.ts` | `get`/`post` (Formulardaten; bei 401 einmal PIN abfragen und wiederholen, `quiet` ohne Abfrage) und Antworttypen der API (`SystemInfo`, `ClientList`, `Backup` …) |
| `src/lib/auth.svelte.ts` | Anmeldung: Token im localStorage (Kopf `X-Pixelia-Token`), PIN-Dialog, `ensure()` vor Uploads |
| `src/lib/theme-sync.svelte.ts` | Farbwahl mit dem Gerät abgleichen (Gerät ist die Quelle, `unsaved` ohne Anmeldung) |
| `src/lib/client.ts` | Geräte-ID (localStorage `pixelia.client`) und Kurzname „Browser · System“ aus der Browser-Kennung |
| `src/lib/firmware.ts` | Update: Kopf der .bin prüfen (wie setup.html), Hochladen mit Fortschritt und 30-s-Wächter |
| `src/lib/design.css` | eigene Designs (seit 0.5.4, Vorschau unter Experimentell): `html[data-design='pixel'|'fahrplan']` setzt Formen, Schrift und Rahmen über die M3-Bausteine; Farben je Design in `theme.svelte.ts` (`LOOKS`), Wahl `theme.design` und Akzentfarbe `theme.dseed` stecken in der Farbwahl auf dem Gerät (unbekannte Felder bleiben erhalten), gilt nur mit `set.exp` (`theme.allowed`); App.svelte lädt neu, wenn sich die Firmware-Version ändert |
| `src/lib/page.css` | globale Seiten-Bausteine: `.page-grid`, `.card-title`, `.kv`, `.list`/`.list-item`, `.meter-row` |
| `src/lib/format.ts`, `Logo.svelte`, `SignalBars.svelte` | Anzeige-Hilfen (Zahlen, Dauer, Signal), Pixelia-Zeichen, WLAN-Balken |
| `src/lib/m3/` | M3-Expressive-Bibliothek: `tokens.css` (Formen, Typo, Federn), `theme.svelte.ts` (dynamische Farben), `Button`, `IconButton`, `ButtonGroup`, `Card`, `Chip`, `Navigation`, `WavyProgress`, `CircularProgress`, `LoadingIndicator`, `TextField`, `Slider`, `Switch`, `Dialog` (`confirmDialog()`, `promptDialog()` mit Prüfung), `shapes.ts`, `Icon`; `Navigation` mit Punkt (`badge`) |
| `src/lib/m3/icon-names.ts` | Liste der benutzten Material Symbols; nur diese bettet das Vite-Plugin in `vite.config.ts` ein (seit 0.5.4 Pfade auf Raster 240 gerundet, `compactPath`) |
| `src/assets/pixelia-pixel.woff2` | Pixelschrift des Displays für das Design Pixelia (aus `tools/pixel_font.py`, 2 KB) |
| `src/assets/roboto-flex-pixelia.woff2` | reduzierte Schrift (aus `tools/font_subset.py`, im Repo; seit 0.5.4 wght 400–800, wdth 100–120, 21 KB) |

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
| `github` | 0 | 3 | nur während Prüfung oder Download von GitHub (8 KB Stack), beendet sich danach |
| `perf`, `webstatus`, `logpush` | 0 | 2 | Leistung alle 2 s, Status-WebSocket 1/s, Live-Log 5/s |
| (`esp_timer`) | 0 | – | Zeitgeber ohne eigene Task: `probe` (15 s), `testbild` (60 s), `exp-stabil` (60 s nach dem Start), `upd-auto` (alle 5 min), `neustart` |

Regeln: Kern 1 gehört der Grafik. Nichts regelmäßig aufrufen, was die FreeRTOS-Kernel-Sperre lange hält (z. B.
`uxTaskGetSystemState`, 1,1 ms → Display-Aussetzer). Allokationen < 16 KB landen im internen RAM, größere im PSRAM.

## Speicher

- Flash: `partitions.csv` – nvs 24 KB, otadata, ota_0/ota_1 je 6 MB, storage (LittleFS, noch ungenutzt), coredump 64 KB.
- NVS-Namensräume: `pixelia` (Einstellungen inkl. `ui`, Update-Kanal `upd_ch`, seit 0.4.2 `upd_auto`, `upd_last`,
  `upd_new`, Experimentell `exp`, Panel-Werte `panel`, Startzähler `exp_boot`), `wifi` (Netze, Setup-Passwort, Hinweise), `auth` (Salz und Hash des PINs), `ota` (Rückfall-Grund),
  `diag` (Startzähler `boot`, Reset-Historie `r00`–`r15`), `clients` (Blob `list`: bis 12 Browser, je 92 Byte).
- PSRAM (seit 0.5.0): Engine-Puffer 2 × 96 KB, grober Puffer der Orte 2 × 24 KB, Grafiken über `asset_alloc`: Testorte-Grafiken ~16 KB, je Ort `Room` ~8 KB und Ort-Objekt ~4,5 KB (mit Palette), Hintergrundbild 24 KB je betretenem Ort (höchstens zwei), seit 0.5.2 Lichtkarte 24 KB je Ort mit Lichtern (drei Testorte); seit 0.5.3 Partikel 8 KB je betretenem Ort, Schleier-Felder 3 KB je Ort.
- PSRAM No-Init: Log-Puffer (Lage hängt an der Codegröße). RTC-Speicher No-Init: Log-Sicherung 6 KB, Herzschlag.

## Schnittstellen

- **Konsole** (seriell 115200 und `/api/cmd`): `help` listet alles; Übersicht in HARDWARE.md „Diagnose-Schnittstelle“.
- **HTTP/JSON** (`web.cpp`): `/api/status`, `/api/changelog` (seit 0.4.0, gzip mit ETag), `/api/wifi`, `/api/wifi/scan|add|remove`, `/api/settings`, `/api/ota`,
  `/api/ota/ack`, `/api/reboot`, `/api/speedtest`; WebSocket `/ws` (Status 1/s; vom Browser `hello=<id>&ua=…&page=…`).
- **Schutz** (`web.cpp`, seit 0.3.2): `/api/auth`, `/api/login`, `/api/logout`, `/api/pin`, `/api/factory-reset`,
  `/api/backup`. Werksreset löscht `pixelia`, `wifi`, `clients`, `auth`; `diag` und `ota` bleiben.
- **Info** (`web_info.cpp`): `/api/system`, `/api/clients`, `/api/clients/name`.
- **Update von GitHub** (`web.cpp` → `github.h`, seit 0.4.1): `GET /api/update`, `POST /api/update/check` (ohne PIN),
  `/api/update/channel` (`ch`, seit 0.4.2 auch `auto=0/1`), `/api/update/install` (`v`, seit 0.4.2 `old=1` für ältere
  Versionen, nur mit Experimentell), `/api/update/cancel` (seit 0.4.2), alle mit PIN; Fortschritt im Status
  (`ota.src = github`), dazu `ota.avail` (neuere Version) und `ota.ghSeq` (neuer Stand von `/api/update`).
- **Experimentell und Vorschau** (`web_exp.cpp`, seit 0.4.2): `GET /api/exp`, `POST /api/exp` (`a=on|off|panel|confirm|
  revert|reset|test|picoff`, mit PIN außer `revert`; seit 0.5.0 Orte `a=place&n`, `a=trans&v`, `a=tour&s`, `a=figures&n`,
  nur mit Experimentell; `places`, `place`, `trans`, `tour`, `figures` in `/api/exp`), `GET /api/screen`, `POST /api/picture`
  (seit 0.5.0, mit PIN, nur mit Experimentell; `pic` in `/api/exp`); Status `set.exp`.
- **Konsole, Orte** (seit 0.5.0): `ort [Nr|Kennung] [Übergang]`, `ort rundgang <s>`, `ort figuren <n>`, `buf grob int|psram` (Messung); seit 0.5.1 `ort ausbau|baustelle`, `ort folgen 0|1`, `ort pruefen 0|1`, `bench` (Verdoppeln und Hintergrund-Kopie messen); `/api/exp` `a=cmd&v=…`, `a=follow&v=`, `a=inspect&v=`. Seit 0.5.2 `bench mem` (PSRAM schreiben/lesen, Kopie aus internem RAM, DMA per `esp_async_memcpy`), `time HH:MM|real`, `speed x`; `/api/exp` `a=time&v=<Minute>|real`, `a=speed&v=<1–3600>`, in GET `minute`, `speed`, `real`. Seit 0.5.3 `wetter [Art [%] [sofort]]`, `wetter blitz`, `wetter wind links|rechts`, `ort cmd <Befehl>`; `/api/exp` `a=weather&v=<Art>&p=<%>`, `a=cmd&v=<Bühnenbefehl>`, in GET `weather`, `weatherPct`, `innen`, `particles`.
- **Ausgehend:** `https://api.github.com/repos/Baldesarra/pixelia/releases`, Dateien über `github.com/…/releases/download/`
  → `release-assets.githubusercontent.com`. Ohne Token: 60 Abfragen pro Stunde und IP.
- **Fernwartung** (`web_diag.cpp`): siehe HARDWARE.md „Fernwartung per WLAN“.
- WebSocket-Anmeldung über `ws_post_handshake_cb` (ESP-IDF 6.1 ruft beim Handschlag keinen Handler auf).

## Simulator (`sim/src/`)

`main.cpp` (Befehlszeile), `window.cpp` (SDL3, Maus = Touch, Tasten 1–9 Ort, U Übergang), `headless.cpp` (Bühne wie am
Gerät, Screenshot, Zeitraffer, Kontaktbogen, `--go ORT[:ÜBERGANG]`, `--sequence` Bildfolge, `--screen` für Systemanzeigen; seit 0.5.1 `--cmd`, `--inspect`, `--no-follow`; seit 0.5.2 `--grid` mit `--cells`/`--cols` für beschriftete Vergleichsbilder; seit 0.5.3 `--cmd` über `Stage::command`, `--frames` auch für `--grid`),
`tests.cpp` (`--test gestures|redact|stage|all`, läuft in der CI).

## Werkzeuge (`tools/`, Windows)

| Werkzeug | Zweck |
|---|---|
| `idf.ps1` | `idf.py` mit Umgebung, gekürzte Ausgabe, Log in `.logs/idf-last.log` |
| `ota.ps1` | Update per WLAN mit Prüfung von Version und Selbsttest (Standardweg) |
| `netlog.ps1` | Live-Log, Befehle (`-Send`), frühere Starts (`-Boot prev`) per WLAN |
| `crash.ps1` | Absturzbericht per WLAN oder aus Diagnosepaket holen und dekodieren |
| `flash.ps1`, `monitor.ps1` (+ `serial_log.py`) | USB: Erstinstallation, serielles Log |
| `sim.ps1` | Simulator bauen und starten (`-Headless`) |
| `screen.py` | Bildschirmfoto vom Display per WLAN (`/api/screen` → PNG, 2×), seit 0.5.1 |
| `release_notes.py` | CHANGELOG.json prüfen (`--check`), Release-Notes erzeugen, Changelog-Block für die Firmware (`--embed`, läuft im Firmware-Build) |
| `web_embed.py` | Oberfläche für die Firmware einbetten (läuft im Firmware-Build) |
| `web_screens.mjs` | Screenshots der Oberfläche (Handy/Desktop, hell/dunkel) per Edge kopflos, `--url http://pixelia` fürs Gerät; `--variants`, `--eval` (Skript nach dem Laden, z. B. klicken), `--text 1` (Seitentext), meldet Skriptfehler. In Git-Bash werden `#/…`-Pfade verbogen: aus PowerShell aufrufen |
| `pixel_font.py` | Pixelschrift aus `engine/src/font.cpp` als Webschrift (nach neuen Glyphen ausführen) |
| `font_subset.py` | Roboto Flex reduzieren (nur bei neuen Zeichen, braucht `pip install fonttools brotli`) |

## CI (`.github/workflows/`)

`firmware.yml` (Job `web`: Oberfläche bauen und prüfen; dann Firmware-Build, Größenbudget), `simulator.yml` (Linux-Build, Selbsttests, Screenshots), `release.yml`
(ebenfalls mit Job `web`; Tag `vX.Y.Z` → Release mit `.bin`, Factory-Image, `.elf`, Notes), `pages.yml` (Web-Flasher nach jedem Release).
