# Hardware: ESP32-8048S050C_I

> **Status: GEPRÜFT (2026-10-18).** Geprüft am echten Gerät mit der Hardware-Detektiv-Firmware (Phase 0.1.0):
> Chipdaten per esptool und Systeminfo, Display per Prüfbild-Foto, Touch per Eckentest, Leistung per Messung.
> Bestätigte Werte tragen ✅ und das Prüfdatum. Was nicht geprüft werden konnte oder nicht gebraucht wird, steht dabei.

## Eckdaten

| Bauteil | Ergebnis | Geprüft |
|---|---|---|
| Modul | Aufdruck „ESP32-S3 N16R8“, FCC-ID 2BB77-ESPS3-32 (kompatibles Modul, nicht Espressif-WROOM) | ✅ 2026-10-18 (Foto) |
| Chip | ESP32-S3 (QFN56) mit **8 MB Octal-PSRAM im Chip** (ESP32-S3R8, AP 3,3 V), 240 MHz, 2 Kerne | ✅ 2026-10-18 (esptool) |
| Chip-Revision | **v0.2** | ✅ 2026-10-18 |
| Flash | **16 MB**, ID `0x5E4018`, Quad-SPI laut eFuse, läuft mit QIO 80 MHz | ✅ 2026-10-18 |
| PSRAM | 8 MB Octal, läuft mit 80 MHz, XIP aus PSRAM aktiv | ✅ 2026-10-18 |
| MAC (WLAN) | `1c:db:d4:49:2f:00` | ✅ 2026-10-18 |
| Display | 5", **800×480**, RGB565 parallel (16 Bit), ohne eigenen Bildspeicher | ✅ 2026-10-18 (Prüfbild) |
| Panel-Typ | „_I“ = vermutlich IPS. Per Software nicht prüfbar. | – |
| Touch | **GT911** auf I²C-Adresse **0x5D**, Firmware 0x1060, konfiguriert auf 800×480 (Konfig v68) | ✅ 2026-10-18 |
| USB | USB-C mit **CH340** (VID 1A86, PID 7523) als USB-UART, Auto-Reset über DTR/RTS funktioniert | ✅ 2026-10-18 |
| Tasten | RST und BOOT (Rückseite) | ✅ 2026-10-18 (Foto) |
| Speicherkarte | microSD-Halter (TF1), SPI | nicht getestet (nicht eingeplant) |
| Lautsprecher | Anschluss „Speak“ (P6) mit Verstärker-IC U4 auf der Platine | gefunden (Foto), noch nicht getestet |

## Pinbelegung

### RGB-Panel

Alle 16 Datenleitungen und die Steuerleitungen sind mit dem Prüfbild bestätigt (Farbreihenfolge, gleichmäßige
5/6-Bit-Verläufe ohne Sprünge, 1-Pixel-Rand an allen vier Seiten).

| Signal | GPIO | Geprüft |
|---|---|---|
| DE (Data Enable) | 40 | ✅ 2026-10-18 |
| VSYNC | 41 | ✅ 2026-10-18 |
| HSYNC | 39 | ✅ 2026-10-18 |
| PCLK | 42 | ✅ 2026-10-18 |
| R0–R4 | 45, 48, 47, 21, 14 | ✅ 2026-10-18 |
| G0–G5 | 5, 6, 7, 15, 16, 4 | ✅ 2026-10-18 |
| B0–B4 | 8, 3, 46, 9, 1 | ✅ 2026-10-18 |

Im Treiber: `data_gpio_nums` D0..D15 = B0..B4, G0..G5, R0..R4 (RGB565, Bit 0 = B0), keine Byte-Vertauschung.

### Sonstiges

| Funktion | GPIO | Geprüft |
|---|---|---|
| Hintergrundbeleuchtung (PWM, LEDC **200 Hz**, 12 Bit) | 2 | ✅ 2026-10-18 (Sichtprüfung, siehe unten) |
| Touch SDA | 19 | ✅ 2026-10-18 (I²C-Scan) |
| Touch SCL | 20 | ✅ 2026-10-18 (I²C-Scan) |
| Touch RST | 38 | ✅ 2026-10-18 (bei RST = low verschwindet der GT911 vom Bus) |
| Touch INT | nicht verwendet, Abfrage per Polling alle 20 ms | – |
| SD CS / MOSI / SCK / MISO | 10 / 11 / 12 / 13 (Community-Angabe) | nicht getestet |

### Steckverbinder (Aufdruck Rückseite, nicht elektrisch geprüft)

| Bezeichnung | Aufdruck |
|---|---|
| P1 (neben USB) | UART0 (5 V, TX, RX, GND) |
| P2 „SPI“ | IO-Pins des SPI-Busses |
| P3 „UART1 / USB“ | IO17, IO18, IO19, IO20 |
| P4 | GND, 3.3V, IO17, IO18 |
| P5 | GND, 3.3V, IO17, IO18 |
| P6 „Speak“ | Lautsprecher (über U4) |

GPIO 19/20 liegen also auch auf P3 (gemeinsam mit dem Touch-I²C). Ein nachgerüsteter Lichtsensor (BH1750, I²C) kann
dort mit an den Bus.

### Hintergrundbeleuchtung: PWM-Frequenz

GPIO 2 schaltet offenbar den LED-Spannungswandler ein und aus. Der braucht nach jedem Einschalten Zeit zum Hochfahren,
deshalb hängt das Dimmen stark von der PWM-Frequenz ab. Getestet mit 100 % → 50 % → 15 % → 100 % (Sichtprüfung):

| PWM-Frequenz | Ergebnis |
|---|---|
| **200 Hz** | dimmt sauber in Stufen, kein Pfeifen, kein Flimmern → **Standard** |
| 500 Hz, 1 kHz | dimmt, aber hörbares Pfeifen (lauter mit steigender Frequenz) |
| 2 kHz, 5 kHz, 10 kHz | Display geht unter 100 % ganz aus |
| 20 kHz (erster Versuch) | Display geht unter 100 % ganz aus |

Die Helligkeitskurve (wahrgenommene Helligkeit, kleinster noch leuchtender Wert) ist noch nicht vermessen.

### Ausrichtung

Der Bildursprung (Pixel 0/0) liegt in der Ecke auf der dem USB-Anschluss **gegenüberliegenden** Seite, oben. Steht das
Gerät mit dem USB-Kabel rechts, braucht es die 180°-Drehung. Die Drehung kostet keine Rechenzeit (im
Bounce-Buffer-Callback), Touch-Koordinaten werden mitgedreht. Einstellbar per Befehl `rot 0|180`, später in den
Systemeinstellungen.

## Panel-Timings

| Parameter | Wert | Geprüft |
|---|---|---|
| Pixeltakt | **24,6 MHz** (Taktquelle PLL 160 MHz) → **60,0 Hz** Bildwiederholrate | ✅ 2026-10-18 |
| HSYNC Front/Pulse/Back Porch | 8 / 4 / 8 | ✅ 2026-10-18 |
| VSYNC Front/Pulse/Back Porch | 8 / 4 / 8 | ✅ 2026-10-18 |
| Gesamt pro Bild | 820 × 500 Takte | ✅ 2026-10-18 |
| PCLK aktiv bei fallender Flanke | ja (`pclk_active_neg = 1`) | ✅ 2026-10-18 |
| HSYNC/VSYNC im Ruhezustand | low (`hsync_idle_low = 1`, `vsync_idle_low = 1`) | ✅ 2026-10-18 |
| Bounce-Buffer | 2 × 8 Zeilen (2 × 12,8 KB, interner RAM) | ✅ 2026-10-18 |

Grenze des Pixeltakts:

- **ESP32-Seite:** bis 40 MHz (97 Hz) ohne einen einzigen Aussetzer (desync 0), Interrupt-Last 28 %. Höhere Werte lässt
  der Taktteiler nicht zu (48 MHz angefordert ergab 24 MHz).
- **Panel-Seite:** 16, 24,6, 30, 36 und 40 MHz einwandfrei und flüssig (Sichtprüfung). Die Grenze liegt also über
  40 MHz. Mit 24,6 MHz bleibt reichlich Sicherheitsabstand.

## Leistungsmessung Engine-Prototyp (2026-10-18)

400×240 Pixel, 8 Bit Palette, Palette + 2× Hochskalieren im Bounce-Buffer-Callback auf Kern 1, Doppelpuffer mit Tausch am
Bildende, Vollbild-Scrolling-Test (Himmel mit Dithering, Wolken, zwei Parallax-Ebenen, Kachelboden, Wasser mit
Farbrotation, Vögel als Sprites). Panel 60 Hz, Tausch jedes 2. Bild = **30 fps**.

| Engine-Puffer | Zeichnen 48 Sprites | 500 Sprites | 2000 Sprites | Interrupt-Last | Bildrate | RAM intern frei |
|---|---|---|---|---|---|---|
| interner RAM | 2,1 ms | 5,3 ms | 16,2 ms | 17,2 % | 30 fps | 115 KB |
| **PSRAM (Standard)** | 4,1 ms | 8,4 ms | 22,6 ms | 20,2 % | 30 fps | 303 KB |

Weitere Messwerte:

| Messung | Ergebnis |
|---|---|
| Interrupt-Last abhängig vom Pixeltakt (Puffer intern) | 16 MHz: 11 %, 24,6 MHz: 17 %, 30 MHz: 21 %, 40 MHz: 28 % |
| Längster Bounce-Callback (8 Zeilen) | 48 µs intern, 63 µs PSRAM (Zeit bis der Puffer gebraucht wird: 267 µs) |
| Kosten pro Ausgabepixel | ca. 1,8 CPU-Takte |
| 180°-Drehung | Interrupt 20,2 → 20,5 % (Quelle wird immer vorwärts gelesen, rückwärts geschrieben) |
| Start mit Engine (Puffer im PSRAM) | interner RAM frei 299 KB, PSRAM frei 7,7 MB |
| Flash-Schreiben während der Animation | 664 KB in 10 s gelöscht/geschrieben, desync 0, 30 fps, Zeichenzeit unverändert, jedes Bild genau ein Logik-Schritt; Sichtprüfung im A/B-Vergleich: gleich flüssig |
| Logik-Takt | aus dem Bildzähler des Panels: ein Schritt pro Engine-Bild (30 Hz), nach verpasstem Bild zwei |
| Freier RAM ohne Engine | intern 314 KB, PSRAM 7,9 MB |
| Firmware-Größe | 314 KB (5 % der App-Partition) |

## Netzwerk und Updates (Phase 0.2.x, gemessen 2026-10-18)

Firmware-Stand vor 0.2.0 mit WLAN, mDNS, NTP, Webserver mit WebSocket und OTA, Heim-WLAN an einer FritzBox,
Signal −63 bis −73 dBm, Kanal 1.

| Messung | Ergebnis |
|---|---|
| Freier interner RAM im Betrieb (WLAN verbunden, Seite offen, WebSocket aktiv) | 125–128 KB, Tiefstwert 119–121 KB ✅ (Budget ≥ 60 KB) |
| Freier interner RAM im Setup-Modus (Access Point + Station) | 117–120 KB, Tiefstwert 115 KB ✅ |
| Freier interner RAM während eines Updates | ≥ 121 KB ✅ |
| Freier PSRAM | 6,9 MB ✅ (Budget ≥ 1 MB) |
| Speicherbedarf beim Start | Display und Engine 62 KB, WLAN 83 KB (Setup-Modus 92 KB), Webserver 16 KB |
| Bildrate mit WLAN, Webserver, offener Seite | 30 fps, Panel 60 Hz, 0 Aussetzer (desync), Interrupt 20,2 % ✅ |
| Bildrate während Upload, Flash-Löschen und -Schreiben | 30 fps, 0 Aussetzer; Kern 0 dabei bis 100 % ausgelastet, Kern 1 (Grafik) unberührt ✅ |
| Zeichenzeit | Testszene 4,0 ms; Systemanzeigen 0,02 ms (unbewegt, nur bei Änderung neu gezeichnet, dann bis 9,5 ms) |
| Update-Upload 1113 KB | **7,4 s**: 3,2 s Zielbereich löschen (64-KB-Blöcke), 6,3 s Übertragen und Schreiben (176 KB/s) |
| Übertragungsrate (`/api/speedtest`, 2 MB) | 158–233 KB/s, längste Pause < 0,5 s, keine Zeitüberschreitung |
| Übertragungsrate vorher (TCP-Fenster 5,7 KB) | ~37 KB/s; bei −71 dBm blieb ein Upload ganz stehen (Wartezeiten verdoppeln sich nach jedem Paketverlust) |
| Neustart → wieder verbunden | 3,6 s nach dem Start (Software-Neustart); Uhrzeit bleibt über Software-Neustarts erhalten |

**Rückfall-Tests mit echten Updates** (Testvarianten per Browser-Schnittstelle hochgeladen, siehe unten):

| Fall | Ablauf | Ergebnis |
|---|---|---|
| Absturz im Probelauf | Testversion stürzt nach 20 s ab (StoreProhibited) | Absturzbericht im Flash, Bootloader startet die alte Version, Meldung „zurückgerollt: Absturz im Probelauf, Task ‚selbsttest‘ bei 0x…“ ✅ |
| Hänger im Probelauf | Testversion blockiert Kern 0 | Watchdog nach 10 s, Neustart, alte Version meldet „Hänger im Probelauf (Watchdog), Task ‚haenger‘“ ✅ |
| Selbsttest scheitert | Testversion läuft, liefert aber angeblich keine Bilder | nach 120 s Rückfall durch Pixelia selbst, Meldung „Selbsttest nicht bestanden: Engine liefert keine Bilder“ ✅ |
| Gute Version | normale Firmware | Selbsttest nach 60 s bestanden, gilt dauerhaft ✅ (auch vom User per Browser getestet) |

**Abgelehnte Dateien** (Gerät läuft ohne Neustart weiter): Factory-Image („für den Web-Flasher“), fremde Datei
(Kennung ≠ 0xE9), anderes Projekt („keine Pixelia-Firmware“), anderer Chip, ein verfälschtes Byte und abgeschnittene
Datei („Prüfsumme stimmt nicht“). ✅

**Besonderheiten im WLAN (FritzBox im WPA2/WPA3-Mischmodus):**

- ESP-IDF 6.1 nutzt standardmäßig den WPA3-Kompatibilitätsmodus (RSN Override). An dieser FritzBox scheitert damit der
  Schlüsselaustausch mit Grund 17 („IE in 4-Way-Handshake weicht ab“). Pixelia probiert dann ohne diesen Modus
  (verbindet sofort) und merkt sich das pro Netz im NVS.
- Nach einem harten Reset (z. B. beim Flashen per USB) beantwortet der Router die ersten Anmeldungen manchmal nicht
  (Grund 2, 4 oder 202). Pixelia versucht es beim Start deshalb 45 s lang, bevor der Setup-Modus kommt.
- **mDNS funktioniert** (Pixelia antwortet auf `pixelia.local`), aber der DNS der FritzBox beantwortet jede
  `.local`-Adresse mit 0.0.0.0, und Windows glaubt dieser Antwort. Am PC deshalb `http://pixelia/` oder
  `http://pixelia.fritz.box` benutzen. Android öffnet `pixelia.local` direkt.
- Setup-Modus am Android-Handy: Die Einrichtungsseite öffnete sich nach dem Verbinden von selbst (Captive Portal),
  die Verbindung ins Heimnetz stand wenige Sekunden nach dem Speichern (Test des Users).
- MAC des Access Points: `1c:db:d4:49:2f:01`.

**Speicher beim Start (Backlog-Punkt geklärt):** Der scheinbare Einbruch von ~31 KB internem RAM ist kein echter
Verbrauch. ESP-IDF reserviert beim Start einen DMA-Pool (`SPIRAM_MALLOC_RESERVE_INTERNAL`, 32 KB) als eigenen
Heap-Bereich, und dieser meldet einen Tiefstwert von 0, solange niemand daraus Speicher anfordert (per Heap-Hook und
`heap_caps_print_heap_info` nachgewiesen). Pixelia startet die Tiefstwert-Messung deshalb zu Beginn von `app_main` neu.

**Testvarianten für den Rückfall-Test** (nur lokal, nie veröffentlichen; Version endet auf `-test-…`):

```
.\tools\idf.ps1 -C firmware -B firmware/build-test-crash -DPIXELIA_TEST=crash build
.\tools\idf.ps1 -C firmware -B firmware/build-test-crash-shift -DPIXELIA_TEST=crash-shift build
.\tools\idf.ps1 -C firmware -B firmware/build-test-hang -DPIXELIA_TEST=hang build
.\tools\idf.ps1 -C firmware -B firmware/build-test-selftest -DPIXELIA_TEST=selftest build
```

`crash-shift` (seit 0.2.2) stürzt wie `crash` ab, hat aber 64 KB mehr Konstanten: Der Log-Puffer im PSRAM liegt dann
eine MMU-Seite weiter (0x3C130000 statt 0x3C120000), wie nach einem echten Update mit mehr Code. Damit wird der Weg über
die RTC-Sicherung geprüft (siehe Fernwartung).

Danach die jeweilige `pixelia.bin` über die Einrichtungsseite hochladen oder per WLAN mit
`.\tools\ota.ps1 -Bin firmware\build-test-crash\pixelia.bin` (meldet den Rückfall samt Grund und endet mit Exit 1).

**Update per WLAN aus der Kommandozeile (seit 0.2.1):** `.\tools\ota.ps1` prüft den Kopf der `.bin` (ESP-Image, Projekt
„pixelia“, Version), fragt `/api/status` ab, lädt mit Fortschritt in 10-%-Schritten hoch, wartet den Neustart ab und
prüft danach, dass die neue Version läuft und den Selbsttest bestanden hat. Standardziel `http://pixelia/`, sonst
`-Target <IP>`; `-Build` baut vorher, `-NoWait` lädt nur hoch. Exit 0 = läuft und bestanden, 1 = Fehler oder Rückfall,
2 = falsche Datei. Volles Protokoll in `.logs/ota-last.log`.

**Update von GitHub (seit 0.4.1, gemessen 2026-10-20, WLAN -64 dBm):** Das Gerät fragt
`api.github.com/repos/Baldesarra/pixelia/releases?per_page=10` ab (9 Releases = 107 KB Antwort im PSRAM, 2,8 s inkl.
TLS-Handschlag) und lädt `pixelia-vX.Y.Z.bin` über die Weiterleitung auf `release-assets.githubusercontent.com`.

| Messung | Wert |
|---|---|
| Interner RAM während der TLS-Verbindung | 109 → 96 KB frei (mbedTLS-Puffer im PSRAM, `CONFIG_MBEDTLS_EXTERNAL_MEM_ALLOC`) |
| Download 0.4.0-1-gf8d1acc → 0.4.1-beta.1 (1521 KB) | 16,6 s gesamt: Verbindung mit Weiterleitung 3,3 s, Löschen 4,5 s, ~114 KB/s |
| Während des Downloads | 29,8–30,3 fps, interner RAM ≥ 90 KB frei (Tiefstwert seit Start 88 KB), Stack der Task 5,4 von 8 KB |
| Danach | Neustart, Probelauf 60 s bestanden, 29,8 fps, RAM 117/111 KB |
| Zweiter Durchlauf, Kanal Stabil: 0.4.1-beta.1 → 0.4.1 (1522 KB) | 15,8 s gesamt: Verbindung 3,2 s, 121 KB/s; dabei 30,0–30,2 fps, RAM-Tiefstwert 91 KB; Probelauf bestanden, danach 30,3 fps, RAM 117/111 KB |

Ohne Token erlaubt GitHub 60 Abfragen pro Stunde je öffentlicher IP (geteilt mit allen Geräten im Heimnetz); der Download
der Datei zählt nicht. Deshalb nur auf Knopfdruck.

## Diagnose-Schnittstelle

Die Firmware nimmt über die serielle Konsole (115200 Baud) und seit 0.2.2 auch per WLAN (`POST /api/cmd`, siehe
Fernwartung) Befehle entgegen, `help` listet sie. `tools/monitor.ps1 -Send` schickt Befehle über USB und liest das Log
gefiltert mit, ohne das Board neu zu starten; `tools/netlog.ps1 -Send` macht dasselbe per WLAN. Der Leistungsmonitor
schreibt alle 2 s eine Zeile auf die serielle Konsole (in den Log-Puffer nur jede Minute): Modus, Bildrate des Panels,
Pixeltakt, Interrupt-Last, Aussetzer (desync), fps, Zeichen- und Logikzeit, CPU-Last pro Kern, freier RAM und PSRAM.

Wichtige Befehle: `pat bars|frame|motion|touch|engine|black`, `bl <0-100>|cycle|stop`, `pclk <MHz>`,
`timing …`, `pol …`, `rot 0|180` (wird gespeichert), `set` / `set bright <1-100>` / `set lp <1000-10000>`
(Dauer des langen Drucks in ms, gespeichert), `card` / `card off` (Info-Karte), `swap <n>`, `sprites <n>`,
`buf int|psram`, `touch`, `touchrst`, `flash <s>`, `tour [tap]`, `tour2`, `time <HH:MM>|real`, `speed <Faktor>`,
`perf <s>|off`, `info`, `reboot`.

Netzwerk und Updates: `wifi` (Zustand), `wifi scan`, `wifi list`, `wifi forget <Name>`, `wifi setup` (Setup-Modus
erzwingen), `wifi connect`, `ota` (Partitionen, Probelauf, Rückfall), `ntp` (Uhrzeit), `web`, `toast <Text>`
(Kurzmeldung zum Prüfen der Anzeige). WLAN-Passwörter lassen sich über die Konsole bewusst nicht eintragen, nur über
die Einrichtungsseite.

Log, Diagnose und Absturzbericht (seit 0.2.2): `log` (Puffer, Herkunft beim Start, Kosten des Hooks), `log bench [n]`
(Messung), `log clear`, `tasks` (Kern, Priorität, Stack-Reserve, CPU-Last; kostet einen Display-Aussetzer, siehe unten),
`resets` (Reset-Historie), `coredump` / `coredump erase`.

Über das Netz (JSON): `GET /api/status`, `GET /api/wifi`, `POST /api/wifi/scan`, `POST /api/settings` (`rot`, `bright`,
`lp`), `POST /api/ota`,
`POST /api/reboot`, `POST /api/speedtest` (Daten empfangen und verwerfen, misst die Übertragungsrate), WebSocket `/ws`
mit dem Status jede Sekunde (sendet erst seit 0.2.2 wirklich, siehe unten). Fernwartung siehe nächster Abschnitt.

### Fernwartung per WLAN (seit 0.2.2, gemessen 2026-10-19)

Daten, Fehler und Logs lassen sich komplett per WLAN abrufen, USB ist nur noch für die Erstinstallation und Abstürze ganz
früh beim Start nötig. Alles läuft nur im Heimnetz. Ist der optionale PIN gesetzt (seit 0.3.2), brauchen `/api/cmd`, das
Löschen des Absturzberichts und alle ändernden Endpunkte eine Anmeldung: Skripte schicken den PIN im Kopf
`X-Pixelia-Pin` (bzw. Feld `pin`), `tools/netlog.ps1`, `crash.ps1` und `ota.ps1` mit `-Pin` oder `$env:PIXELIA_PIN`.
Lesen (Log, Status, Diagnosepaket) bleibt offen. Sperre nach 5 Fehlversuchen: 30 s, danach jeweils doppelt, höchstens
15 min. Am Gerät gemessen: Anmeldung 140–250 ms (1000 Runden SHA-256), Sperre greift beim 5. Fehlversuch.

| Schnittstelle | Inhalt |
|---|---|
| `GET /api/log` | Zeilen aus dem Log-Puffer: `since=<nr>` (nur neuere), `tail=<n>`, `boot=<nr>` (ein Start), `rid=<kennung>` (Puffer neu angelegt → ab der ältesten Zeile, `reset:true`), `max=<Byte>` (Standard 48 KB), `format=text` |
| WebSocket `/ws/log` | Live-Log: zuerst die letzten 50 Zeilen (`tail`, `since`), dann jede neue, bis 5× pro Sekunde gebündelt |
| `POST /api/cmd` | `cmd=<Befehl>`: wie die serielle Konsole (gleiche Task), Ausgabe als Antwort (bis 16 KB, das Ende bleibt). Gefährliche Befehle (reboot, wifi forget/setup/connect, flash, pclk, timing, pol, bounce, clk, buf, touchrst, log clear, coredump erase) nur mit `confirm=1`, sonst 409 mit Grund |
| `GET /api/perf` | Leistungsmonitor komplett, Speicher mit Tiefstwerten, Kosten des Log-Hooks; `tasks=1` zusätzlich die Task-Liste |
| `GET /api/resets` | Reset-Historie der letzten 16 Starts (NVS): Zeit, Grund, Version, Partition, Probelauf, Rückfall, Absturz-Task/PC, Laufzeit davor |
| `GET /api/coredump` | Rohdaten des Absturzberichts aus der Partition (Format „raw“ für esp-coredump) |
| `GET /api/coredump/summary` | Panic-Grund bzw. CPU-Ausnahme, Task, PC, Backtrace-Adressen, ELF-SHA256 und passende Version |
| `POST /api/coredump/erase` | Absturzbericht löschen |
| `GET /api/diag` | Diagnosepaket (eine JSON-Datei): Status, System, Einstellungen ohne Geheimnisse, Leistung mit Tasks, Resets, Absturzbericht (Base64), kompletter Log-Puffer, `secretCheck` am Ende. Knopf „Diagnosepaket herunterladen“ auf der Einrichtungsseite |

**Werkzeuge für Claude** (beide ohne seriellen Port, Ziel `http://pixelia/` oder `-Target <IP>`):

- `tools/netlog.ps1`: Live-Log per WebSocket, Filter wie `monitor.ps1` (`-Full`, `-Filter`, `-Until`, `-MaxLines`),
  Befehle mit `-Send 'info','+2','tasks'` (`-Confirm` für gefährliche), frühere Starts mit `-Boot prev`/`-Boot <nr>`,
  ganzer Puffer mit `-All`. Volles Log in `.logs\netlog-last.log`.
- `tools/crash.ps1`: Absturzbericht holen (oder `-File` mit einem Diagnosepaket vom Handy), passendes ELF über den
  SHA256 suchen (lokale Builds, Testvarianten, `.logs\elf\`, sonst `pixelia-vX.Y.Z.elf` aus dem GitHub-Release),
  Backtrace mit Datei und Zeile, Reset-Eintrag und letzte Log-Zeilen des abgestürzten Starts; `-Full` mit esp-coredump
  und GDB, `-Erase` löscht den Bericht. Ergebnis in `.logs\crash-last.txt`.

**Log-Puffer:** 256 KB im No-Init-Bereich des PSRAM (`EXT_RAM_NOINIT_ATTR`, `CONFIG_SPIRAM_ALLOW_NOINIT_SEG_EXTERNAL_MEMORY`;
der Speichertest beim Start spart ihn aus), dazu 6 KB Sicherung im RTC-Speicher bei 0x50000000. Jede Zeile: Nummer,
Laufzeit (ms), Uhrzeit, Startzähler. Wichtig für die Haltbarkeit:

- Der PSRAM hängt hinter einem Write-Back-Cache, den ESP-IDF beim Neustart nicht zurückschreibt. Der Hook schreibt
  deshalb jede Zeile und den Kopf sofort zurück (`cache_hal_writeback_addr` unter dem Cache-Spinlock von ESP-IDF).
- Die Lage hängt an der Größe von Code und Konstanten: `_ext_ram_noinit_start` liegt hinter den Konstanten, auf
  64 KB aufgerundet (0.2.2: 0x3C120000). Wächst die Firmware über eine 64-KB-Grenze, liegt der Puffer der nächsten
  Version woanders, und das Kopieren des Codes in den PSRAM beim Start überschreibt den alten. Dann helfen die letzten
  6 KB aus der RTC-Sicherung: im Probelauf zeilenweise geführt, sonst einmal vom Shutdown-Handler bei jedem
  Software-Neustart gefüllt.
- Der RTC-Speicher ist langsam: ~0,3 µs pro 32-Bit-Zugriff, gemessen 14–16 µs pro Zeile. Deshalb nicht dauerhaft.

| Messung | Ergebnis |
|---|---|
| Neustart per `/api/cmd reboot`, Updates auf neue Stände (gleiche Lage) | Puffer „übernommen“, z. B. 158 bzw. 711 Zeilen früherer Starts ✅ |
| Absturz-Testvariante `crash` (gleiche Lage) | nach dem Rückfall alle Zeilen da, letzte Zeile 100 ms vor dem Absturz; `crash.ps1`: StoreProhibited, `ota.cpp:125` ✅ |
| Testvariante `crash-shift` (Lage verschoben) | Befund „PSRAM: kein gültiger Kopf“, „RTC: 95 Zeilen gültig“ → Puffer „aus RTC-Sicherung“ mit der letzten Zeile vor dem Absturz ✅ |
| Kosten des Hooks (Puffer, `log bench`) | im Mittel 15,8–18,6 µs pro Zeile (PSRAM inkl. Cache-Rückschreiben 12,6–15,6 µs), höchstens 24–60 µs; im Probelauf mit RTC 30 µs |
| zum Vergleich (wie vor 0.2.2) | Formatieren 49–64 µs, serielle Ausgabe 3,6–8,7 ms pro Zeile |
| 100 Log-Zeilen am Stück | 30,6 fps, 0 Aussetzer ✅ |
| Lasttest 40 s: 162 Abrufe (Log 256 KB, Befehle, Absturzbericht, Leistung) + Live-Log | 29,7–30,3 fps, 0 Aussetzer, interner RAM 119–121 KB frei, Tiefstwert 108 KB ✅ (Budget ≥ 60 KB) |
| Diagnosepaket | 91 KB in 0,2 s; Passwort-Test mit bekanntem Test-WLAN: in keiner Ausgabe enthalten ✅ |
| Freier PSRAM | 6,4 MB (Puffer 256 KB, Index 64 KB, Befehlsausgabe 16 KB) ✅ |
| Stack-Reserven mit dem Hook | IDLE 736 B, sys_evt 1180 B, webstatus 1272 B, wlan 1348 B, console 1484 B, logpush 1680 B, httpd 4312 B |
| Firmware-Größe | 1170 KB (19 % der App-Partition) |

**Kernel-Sperre und Display (wichtig für die Leistungsanzeige in 0.4.x):** `uxTaskGetSystemState()` hält die globale
Kernel-Sperre, während es die Stacks aller Tasks nach dem Füllmuster durchsucht: gemessen 1,0–1,1 ms. Der Tick- und
der Display-Interrupt auf Kern 1 (`xSemaphoreGiveFromISR` am Bildende) warten so lange mit maskierten Interrupts, der
Bounce-Buffer kommt zu spät: etwa ein Aussetzer (desync) pro Abfrage, bei 39 Abfragen pro Sekunde 368 Aussetzer und
27 fps. Die Task-Liste gibt es deshalb nur auf Wunsch (`tasks`, `/api/perf?tasks=1`, einmal im Diagnosepaket). Regel:
nichts regelmäßig aufrufen, was die Kernel-Sperre lange hält.

**Status-WebSocket repariert:** ESP-IDF 6.1 ruft beim WebSocket-Handschlag den URI-Handler nicht mehr auf. Die Anmeldung
von `/ws` stand dort, der Status kam deshalb seit 0.2.0 nie per WebSocket (die Einrichtungsseite fragte ersatzweise
alle 2 s ab). Seit 0.2.2 über `ws_post_handshake_cb` (`CONFIG_HTTPD_WS_POST_HANDSHAKE_CB_SUPPORT`): 5 Nachrichten in 3,5 s.

**Info-Karte und Gesten (seit 0.2.1):** 3 s irgendwo aufs Display drücken zeigt die Info-Karte (IP, Namen, WLAN,
Signal, Firmware, QR-Code mit `http://<IP>/`; im Setup-Modus Name und Passwort des Setup-WLANs mit QR-Code zum
Beitreten). Tippen schließt sie, sonst nach 20 s. Die Erkennung steckt plattformunabhängig in
`engine/include/pixelia/engine/gesture.h` (im Simulator mit `--test gestures` geprüft), verteilt wird in
`firmware/main/input.cpp`. Regeln: Bewegung über 24 Panel-Pixel oder ein zweiter Finger heben Tippen und langen Druck
auf (frei für spätere Wischgesten), Tippen = höchstens 800 ms, die Halte-Anzeige erscheint nach einem Viertel der
Zeit. Das Log zeigt `input: Langer Druck bei x/y (… ms, n Messwerte) → Info-Karte` bzw. `Tippen … → geschlossen`.

Geführte Tests im Tempo des Users (Stufe = Zahl der weißen Quadrate oben links):

- `tour`: vier Ecken antippen → Szene → Helligkeitsstufen → Pixeltakt 30/36/40 MHz → Flash-Schreiben
  (`tour tap`: ohne Ecken, Start per Tippen)
- `tour2`: Tippen → PWM-Frequenzen 200 Hz bis 10 kHz → Tippen → A/B-Vergleich normal gegen Flash-Schreiben

## Experimentell, Live-Vorschau, Leistung (seit 0.4.2, gemessen 2026-10-20)

**Live-Vorschau** (`GET /api/screen`, Seite Live): Der Grafik-Task kopiert das fertige Bild (400×240 Paletten-Indizes,
PSRAM → PSRAM) samt Palette direkt vor dem Puffertausch, also nie halb gezeichnet; nur auf Anforderung und höchstens
alle 200 ms ein neues Bild. Der Webserver packt es mit PackBits (Steuerbyte 0–127 = n+1 Bytes roh, 129–255 = nächstes
Byte 257−n-mal) und schickt `"PXS1"`, Breite, Höhe, Bildnummer, Verfahren, Palette 256 × RGB888, Daten. Der Browser malt
ins Canvas und erzeugt das PNG (800 × 480) selbst. Kopf `Server-Timing` nennt Kopier- und Packzeit.

| Messung (Szene `scrolltest`, 60 s, Abfrage wie die Seite Live) | ohne Vorschau | mit Vorschau |
|---|---|---|
| Bildrate Engine | 29,8–30,3 fps | 29,7–30,3 fps |
| Zeichnen, Maximum pro 2 s | 4,6–5,2 ms | 5,4–6,9 ms |
| Interner RAM frei | 116 KB | 115–116 KB (Tiefstwert seit Start 108 KB) |
| CPU Kern 0 / Kern 1 | 2 % / 14 % | 14 % / 15 % |
| Aussetzer (desync) | 0 | 0 |
| Vorschau | – | 2,9 Bilder/s, Ø 68,5 KB (von 96 KB), Ø 120 ms pro Bild |
| Kopie im Grafik-Task (Kern 1) / Packen im Webserver (Kern 0) | – | 3,9 ms / 10,7 ms |

**Stromsparmodus** (Seite Experimentell): halber Pixeltakt 12,3 MHz → Panel 29,8–30,3 Hz, jedes Engine-Bild einmal
gezeigt. Gemessen 29,8–30,3 fps, Hochskalier-Interrupt 20,2 → 10,1 %, CPU Kern 1 13 → 12 %, ein Aussetzer beim
Umschalten (Panel wird neu angelegt), danach 0. Sichtprüfung am Display (Flimmern?) steht noch aus.

**Probe und Rückfall** (`exper.cpp`): Panel-Werte und Stromsparmodus gelten erst nach „Behalten“ im Browser; sonst nach
15 s zurück (geprüft: Probe 30 Hz, nach 15 s wieder 60 Hz, ohne Neustart). Gespeichert als Blob `pixelia/panel`, Schalter
`pixelia/exp`, Startzähler `pixelia/exp_boot`. Start mit gespeicherten Werten geprüft (Log `lcd: … Pixeltakt 12.30 MHz …
30.00 Hz`, nach 60 s `exp: Eigene Panel-Werte laufen seit 60 s stabil`). Fluchtweg geprüft: drei Neustarts im Abstand
von ~15 s, der dritte startet mit Standardwerten (`safeBoot`). Grenzen der Eingabe: Pixeltakt 8–32 MHz, Austastwerte
1–200, Ergebnis 24–75 Hz.

**Update von GitHub abbrechen:** Download 0.4.0 über die Seite Experimentell (ältere Version) gestartet, nach 932 KB
abgebrochen (15,5 s nach dem Start, Löschen der Partition 4,3 s): kein Neustart, `ota_0` bleibt Startpartition; im
abgebrochenen `ota_1` steht noch der Kopf von 0.4.0, die Partition ist aber ungültig.

**Prüfung im Hintergrund:** Zeitgeber alle 5 min; prüft, wenn eingeschaltet (`pixelia/upd_auto`, Standard an), Uhrzeit
bekannt, WLAN verbunden, kein Update oder Probelauf und die letzte Prüfung (`pixelia/upd_last`, Unix-Zeit) mindestens
24 h her ist. Gefundene Version in `pixelia/upd_new` (Punkt an „System“, auch nach Neustart). Geprüft: erste Prüfung nach
301 s Laufzeit, im nächsten Start keine weitere.

## Prüfplan (Phase 0.1.0) – erledigt

1. Fotos von Vorder- und Rückseite ✅
2. Anschließen, Chip auslesen mit esptool ✅
3. Hardware-Detektiv-Firmware: Systeminfo ✅, I²C-Scan ✅, Prüfbild (Farben, Verläufe, Rand) ✅, Bewegung ✅,
   Touch-Eckentest ✅, RST-Test ✅, Hintergrundbeleuchtung ✅ (200 Hz)
4. Feinabstimmung: Pixeltakt auf 60 Hz festgelegt ✅, ESP-Grenze ≥ 40 MHz ✅, Panel-Grenze > 40 MHz ✅,
   Flash-Schreiben ohne Störung ✅
