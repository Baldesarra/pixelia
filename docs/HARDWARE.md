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
.\tools\idf.ps1 -C firmware -B firmware/build-test-hang -DPIXELIA_TEST=hang build
.\tools\idf.ps1 -C firmware -B firmware/build-test-selftest -DPIXELIA_TEST=selftest build
```

Danach die jeweilige `pixelia.bin` über die Einrichtungsseite hochladen oder per
`curl.exe -X POST -H "Content-Type: application/octet-stream" --data-binary "@datei.bin" http://pixelia/api/ota`.

## Diagnose-Schnittstelle

Die Firmware nimmt über die serielle Konsole (115200 Baud) Befehle entgegen, `help` listet sie. `tools/monitor.ps1 -Send`
schickt Befehle und liest das Log gefiltert mit, ohne das Board neu zu starten. Der Leistungsmonitor schreibt alle 2 s
eine Zeile: Modus, Bildrate des Panels, Pixeltakt, Interrupt-Last, Aussetzer (desync), fps, Zeichen- und Logikzeit,
CPU-Last pro Kern, freier RAM und PSRAM.

Wichtige Befehle: `pat bars|frame|motion|touch|engine|black`, `bl <0-100>|cycle|stop`, `pclk <MHz>`,
`timing …`, `pol …`, `rot 0|180` (wird gespeichert), `set` / `set bright <1-100>`, `swap <n>`, `sprites <n>`,
`buf int|psram`, `touch`, `touchrst`, `flash <s>`, `tour [tap]`, `tour2`, `time <HH:MM>|real`, `speed <Faktor>`,
`perf <s>|off`, `info`, `reboot`.

Netzwerk und Updates: `wifi` (Zustand), `wifi scan`, `wifi list`, `wifi forget <Name>`, `wifi setup` (Setup-Modus
erzwingen), `wifi connect`, `ota` (Partitionen, Probelauf, Rückfall), `ntp` (Uhrzeit), `web`, `toast <Text>`
(Kurzmeldung zum Prüfen der Anzeige). WLAN-Passwörter lassen sich über die Konsole bewusst nicht eintragen, nur über
die Einrichtungsseite.

Über das Netz (JSON): `GET /api/status`, `GET /api/wifi`, `POST /api/wifi/scan`, `POST /api/settings`, `POST /api/ota`,
`POST /api/reboot`, `POST /api/speedtest` (Daten empfangen und verwerfen, misst die Übertragungsrate), WebSocket `/ws`
mit dem Status jede Sekunde.

Geführte Tests im Tempo des Users (Stufe = Zahl der weißen Quadrate oben links):

- `tour`: vier Ecken antippen → Szene → Helligkeitsstufen → Pixeltakt 30/36/40 MHz → Flash-Schreiben
  (`tour tap`: ohne Ecken, Start per Tippen)
- `tour2`: Tippen → PWM-Frequenzen 200 Hz bis 10 kHz → Tippen → A/B-Vergleich normal gegen Flash-Schreiben

## Prüfplan (Phase 0.1.0) – erledigt

1. Fotos von Vorder- und Rückseite ✅
2. Anschließen, Chip auslesen mit esptool ✅
3. Hardware-Detektiv-Firmware: Systeminfo ✅, I²C-Scan ✅, Prüfbild (Farben, Verläufe, Rand) ✅, Bewegung ✅,
   Touch-Eckentest ✅, RST-Test ✅, Hintergrundbeleuchtung ✅ (200 Hz)
4. Feinabstimmung: Pixeltakt auf 60 Hz festgelegt ✅, ESP-Grenze ≥ 40 MHz ✅, Panel-Grenze > 40 MHz ✅,
   Flash-Schreiben ohne Störung ✅
