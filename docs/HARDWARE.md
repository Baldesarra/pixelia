# Hardware: ESP32-8048S050C_I

> **Status: UNGEPRÜFT.** Die Werte unten stammen aus Community-Angaben zu den Sunton-Boards 8048S043/8048S050 und
> müssen am echten Gerät bestätigt werden (Prüfplan unten). Bestätigte Werte bekommen ein ✅ und das Prüfdatum.

## Eckdaten

| Bauteil | Erwartet | Geprüft |
|---|---|---|
| Modul | ESP32-S3-WROOM-1 N16R8 (16 MB Flash, 8 MB Octal-PSRAM) | ☐ |
| Chip-Revision | v0.1 oder v0.2 | ☐ |
| Display | 5", 800×480, RGB565 parallel, ohne eigenen Bildspeicher | ☐ |
| Panel-Typ | „_I“ = vermutlich IPS | ☐ |
| Touch | GT911, I²C-Adresse 0x5D oder 0x14 | ☐ |
| USB | USB-C, vermutlich USB-UART-Wandler (CH340 o. ä.) mit Auto-Reset | ☐ |
| Speicherkarte | microSD über SPI | ☐ |

## Pinbelegung (erwartet)

### RGB-Panel

| Signal | GPIO | Geprüft |
|---|---|---|
| DE (Data Enable) | 40 | ☐ |
| VSYNC | 41 | ☐ |
| HSYNC | 39 | ☐ |
| PCLK | 42 | ☐ |
| R0–R4 | 45, 48, 47, 21, 14 | ☐ |
| G0–G5 | 5, 6, 7, 15, 16, 4 | ☐ |
| B0–B4 | 8, 3, 46, 9, 1 | ☐ |

### Sonstiges

| Funktion | GPIO | Geprüft |
|---|---|---|
| Hintergrundbeleuchtung (PWM) | 2 | ☐ |
| Touch SDA | 19 | ☐ |
| Touch SCL | 20 | ☐ |
| Touch RST | 38 | ☐ |
| Touch INT | nicht verbunden (?) | ☐ |
| SD CS / MOSI / SCK / MISO | 10 / 11 / 12 / 13 | ☐ |

### Panel-Timings (Startwerte)

| Parameter | Wert | Geprüft |
|---|---|---|
| Pixeltakt | 14–16 MHz | ☐ |
| HSYNC Front/Pulse/Back Porch | 8 / 4 / 8 | ☐ |
| VSYNC Front/Pulse/Back Porch | 8 / 4 / 8 | ☐ |
| PCLK aktiv bei fallender Flanke | ja | ☐ |
| Polarität HSYNC/VSYNC | 0 / 0 | ☐ |

## Prüfplan (Phase 0.1.0)

Ziel: Jede Angabe oben wird bestätigt oder korrigiert, ohne dass der User Technik beschreiben muss.

### Schritt 1: Fotos (User)

- Vorder- und Rückseite der Platine, scharf, damit die Aufdrucke lesbar sind (Modul-Beschriftung, USB-Chip,
  Steckverbinder).

### Schritt 2: Anschließen und Chip auslesen (Claude, automatisch)

- Board per USB-C anschließen → Claude erkennt den COM-Port.
- `esptool`: Chiptyp, Revision, MAC, Flash-Größe und -ID.

### Schritt 3: Hardware-Detektiv-Firmware (Claude flasht, liest das serielle Log selbst)

| Test | Was Claude automatisch sieht | Was der User beobachtet |
|---|---|---|
| Systeminfo | PSRAM-Größe/-Modus, Flash-Takt, Heap, Resetgrund | – |
| I²C-Scan | Adresse des GT911 (bestätigt SDA/SCL) | – |
| Hintergrundbeleuchtung | – | Wird es in Stufen heller/dunkler? Flackert es? |
| Farbbalken | – | Reihenfolge Rot-Grün-Blau-Weiß korrekt? (Foto) |
| Ränder | – | Ist der 1-Pixel-Rahmen an allen vier Seiten sichtbar? Bild verschoben? (Foto) |
| Bewegungstest | FPS, Interrupt-Last | Läuft es flüssig? Zittern, Streifen, Flackern? |
| Touch-Test | Koordinaten der vier Ecken im Log | Nacheinander die vier markierten Ecken antippen |

### Schritt 4: Feinabstimmung

- Pixeltakt schrittweise erhöhen, bis Bildfehler auftreten, dann mit Sicherheitsabstand festlegen.
- Ergebnisse in diese Datei eintragen, Status auf GEPRÜFT setzen.
