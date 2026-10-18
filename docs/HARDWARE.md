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
| Hintergrundbeleuchtung (PWM, LEDC 20 kHz, 10 Bit) | 2 | ⏳ Stufentest läuft |
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
- **Panel-Seite:** 16 und 24,6 MHz einwandfrei (Sichtprüfung). 30 / 36 / 40 MHz: ⏳ Sichtprüfung läuft.

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
| Flash-Schreiben während der Animation | 424 KB in 6 s gelöscht/geschrieben, desync 0, 30 fps, Zeichenzeit unverändert |
| Freier RAM ohne Engine | intern 314 KB, PSRAM 7,9 MB |
| Firmware-Größe | 314 KB (5 % der App-Partition) |

## Diagnose-Schnittstelle

Die Firmware nimmt über die serielle Konsole (115200 Baud) Befehle entgegen, `help` listet sie. `tools/monitor.ps1 -Send`
schickt Befehle und liest das Log gefiltert mit, ohne das Board neu zu starten. Der Leistungsmonitor schreibt alle 2 s
eine Zeile: Modus, Bildrate des Panels, Pixeltakt, Interrupt-Last, Aussetzer (desync), fps, Zeichen- und Logikzeit,
CPU-Last pro Kern, freier RAM und PSRAM.

Wichtige Befehle: `pat bars|frame|motion|touch|engine|black`, `bl <0-100>|cycle|stop`, `pclk <MHz>`,
`timing …`, `pol …`, `rot 0|180`, `swap <n>`, `sprites <n>`, `buf int|psram`, `touch`, `touchrst`, `flash <s>`,
`tour [tap]`, `perf <s>|off`, `info`, `reboot`.

## Prüfplan (Phase 0.1.0) – erledigt

1. Fotos von Vorder- und Rückseite ✅
2. Anschließen, Chip auslesen mit esptool ✅
3. Hardware-Detektiv-Firmware: Systeminfo ✅, I²C-Scan ✅, Prüfbild (Farben, Verläufe, Rand) ✅, Bewegung ✅,
   Touch-Eckentest ✅, RST-Test ✅, Hintergrundbeleuchtung ⏳
4. Feinabstimmung: Pixeltakt auf 60 Hz festgelegt, ESP-Grenze gemessen ✅, Panel-Grenze ⏳
