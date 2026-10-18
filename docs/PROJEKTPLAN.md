# Pixelia – Projektplan

> **Gründungsdokument.** Festgehalten am 18.10.2026 zu Projektbeginn, gemeinsam erarbeitet in der Planungsphase.
> Grundsatzentscheidungen werden hier **nicht stillschweigend überschrieben**. Ändert sich etwas, kommt ein Eintrag ins
> [Entscheidungsprotokoll](#14-entscheidungsprotokoll) am Ende. Den laufenden Fortschritt pflegt [ROADMAP.md](ROADMAP.md).

## Inhalt

1. [Vision](#1-vision)
2. [Ausgangslage: Wünsche und Rahmenbedingungen](#2-ausgangslage-wünsche-und-rahmenbedingungen)
3. [Hardware](#3-hardware)
4. [Grundsatzentscheidungen](#4-grundsatzentscheidungen)
5. [Gemeinsame Dienste](#5-gemeinsame-dienste)
6. [Die Welten](#6-die-welten)
7. [Weboberfläche](#7-weboberfläche)
8. [Experimentell und Hardware-Infos](#8-experimentell-und-hardware-infos)
9. [Leistungsbudget und Qualitätsregeln](#9-leistungsbudget-und-qualitätsregeln)
10. [Repo-Struktur](#10-repo-struktur)
11. [Entwicklungsumgebung und Arbeitsweise](#11-entwicklungsumgebung-und-arbeitsweise)
12. [Risiken und offene Punkte](#12-risiken-und-offene-punkte)
13. [FAQ – Fragen aus der Planungsphase](#13-faq--fragen-aus-der-planungsphase)
14. [Entscheidungsprotokoll](#14-entscheidungsprotokoll)

---

## 1. Vision

**Pixelia** verwandelt das 5"-Display in ein Fenster in eine lebendige Pixel-Welt. Das Gerät läuft nebenbei auf dem
Schreibtisch. Man muss nicht aktiv spielen, aber jedes Mal, wenn man hinschaut, passiert etwas: Menschen gehen zum
Bäcker, ein Held findet einen legendären Schatz, der Dorfverein schießt ein Tor, draußen vor dem Zugfenster zieht ein
Gewitter auf.

Leitlinien:

- **Lebendig statt statisch.** Ständig kleine Bewegungen, regelmäßig Ereignisse, selten echte Überraschungen.
- **Idle zuerst.** Alles läuft von allein. Eingreifen ist möglich (Weboberfläche, Touch), aber nie nötig.
- **Liebe zum Detail.** Lieber eine Welt richtig gut als vier halb.
- **Flüssig.** Ziel 30 fps, Minimum 25 fps, ohne Ruckler.
- **Modular.** Neue Welten und Funktionen kommen per Update dazu, ohne den Rest umzubauen.
- **Viele Infos.** Hardware, Speicher, WLAN, Leistung – alles einsehbar.

## 2. Ausgangslage: Wünsche und Rahmenbedingungen

Dieser Abschnitt hält fest, was der User zu Beginn vorgegeben hat. Die Vorgaben aus dem alten CYD-Projekt waren
ausdrücklich nur Orientierung („nicht genauso umsetzen, falls es besser geht“).

### 2.1 Allgemeine Wünsche

- Arbeiten über **GitHub**, damit auch in der Cloud entwickelt werden kann und Updates direkt aus GitHub kommen
  (auch über die Weboberfläche installierbar).
- Software auf dem Gerät mit mehreren „Welten“ (siehe 2.2).
- **Weboberfläche im Material 3 Expressive Design.**
- Viele Einstellungsmöglichkeiten, viele Hardware-Infos, Speicherbelegung, WLAN-Infos.
- Claude ist „Kapitän“ und entscheidet technisch; der User liefert Ideen.
- Erst Projektplan, dann Umsetzung.
- **Performance: mindestens 20–25 fps.**
- **Modular**, damit später neue Features und Wünsche per Update dazukommen können.

### 2.2 Die Welten (Wunschliste des Users)

1. **Pixel-Dorf**, das in Echtzeit lebt: morgens zum Bäcker, tagsüber arbeiten, abends ein Getränk auf der Terrasse,
   nachts schlafen die meisten. Soll nicht langweilig sein und viel Inhalt bieten. **Schwerpunkt: sehr genau und
   ausführlich, wirklich lebendig.**
2. **Dungeon-Held** (Idle-RPG): erkundet zufällig erzeugte Dungeons, levelt auf, verkauft ab und zu Loot auf dem Markt,
   geht zwischendurch nach Hause.
3. **Pixel-Fußballliga**: KI-Teams, eigenes KI-Team, Tabelle, Spieltage, live anschaubar; Live-Scoreboard, wenn mehrere
   Spiele parallel laufen.
4. **Zugfenster**: ruhiger Ausgleich, schöne Landschaften, Tages- und Wetterwechsel, verschiedene Städte und Bahnhöfe.

Querschnittlich: Welten sollen lebendig sein, meist im Idle-Stil laufen, über die Weboberfläche einstellbar sein und
oberflächliche Interaktion erlauben (Beispiel: „Sturm über das Pixel-Dorf ziehen lassen“).

### 2.3 Übernommen aus dem CYD-Projekt (als Orientierung)

- **Start ohne WLAN:** eigener Access Point „<Projekt>-Setup“, Captive Portal auf 192.168.4.1, alle 2 Minuten neuer
  Verbindungsversuch.
- **Gespeicherte WLANs:** zwei Netze, beim Start das stärkste; Hostname im Router = Projektname.
- **Sicherheit:** WLAN-Passwörter werden nie an den Browser geschickt. „Verbundene Geräte“-Ansicht.
- **System:** Werkseinstellungen und Neustart in der Oberfläche.
- **OTA-Update über den Browser:** Fortschrittsbalken in der Oberfläche und auf dem Display, Einstellungen bleiben
  erhalten, bei Abbruch startet die alte Version.
- **Versionierung:** `FW_VERSION` in der `.ino`; semantische Versionen, Vorabversionen 0.x.
- **Changelog** `HIST` mit Einträgen `{v, t (Titel), s (Untertitel), n: Neu, i: Verbessert, f: Behoben, r: Entfernt}`,
  neuester oben. Jeder Stichpunkt beginnt mit dem Bereich („System: …“, „Display: …“), experimentelle Funktionen
  bekommen „(Vorschau)“ und liegen hinter einem Experimentell-Schalter.
- **Versionsverlauf** als aufklappbare Timeline: Versions-Badge, Markierung „Installiert“, Gruppen mit Icon. Vor einem
  Update liest die Oberfläche den Changelog direkt aus der gewählten `.bin` und zeigt nur die Änderungen gegenüber der
  installierten Version.
- **Experimentell-Seite** mit Live-Werten: Lichtsensor mit Auto-Helligkeit (lernt hell/dunkel, „Sensor umkehren“,
  Roh/Mittel/Min/Max), Touch-Test mit Koordinaten, Touch-Kalibrierung, RGB-LED, Stundengong, Ton testen,
  Stromsparmodus (160 statt 240 MHz), Kantenglättung, Displaytest (Farbbalken, Verläufe, Schachbrett, Weiß, Ränder),
  Screenshot als BMP.
- **Info-Seite:** Laufzeit, WLAN/RSSI mit Bewertung, IP, letzter Datenabruf, Speicherbalken (Programm/Arbeitsspeicher),
  Warnung bei falscher Partitionierung.
- **Hardware-Seite:** Boardvariante, Panel, Touch, Chipmodell/Revision, Kerne und Takt, Flash-Größe und -Takt,
  Heap/PSRAM, Pins, MAC, Chiptemperatur, letzter Resetgrund, Arduino-Core, ESP-IDF, Build-Datum.
- WLAN-Signal als Balkenanzeige, Netzwerkscan. Programmspeicherbelegung im Blick behalten.

Was davon wie übernommen, verbessert oder gestrichen wird, steht in Kapitel 4, 7 und 8.

### 2.4 Antworten des Users auf die Startfragen (18.10.2026)

| Frage | Antwort |
|---|---|
| Board-Variante | **ESP32-8048S050C_I**, kapazitiver Touch |
| Projektname | **Pixelia** (Repo, Hostname, Access Point) |
| GitHub | Account vorhanden, aber keine Erfahrung, **Unterstützung bei der Einrichtung gewünscht** |
| Gemeinsames Universum | Claude entscheidet, Kriterium: was die meisten Features und die meiste Liebe zum Detail bringt |
| Reihenfolge | Zug → Dorf → Held → Liga passt. **Dorf besonders genau und ausführlich** |
| Standort/Wetter | So einfach wie möglich, ohne Login. Stadt in den Einstellungen eintragen ist okay |
| Sound | Nicht wichtig, **kein Lautsprecher vorhanden** |
| Touch | Auch Kleinigkeiten (z. B. Bewohner antippen). **Touch über die Weboberfläche einstellbar** |
| Passwort | PIN/Passwort als Option, **standardmäßig aus** |
| Verbundene Geräte | Welches Smartphone/welcher Browser verbunden ist: IP, erste Verbindung, Gerätename usw. |
| Grafikstil | Mischung aus generierter Grafik, freien Paketen und eigenen Sprites, Claude entscheidet |
| Versionsnummern | **Immer drei Stellen.** Jede Welt/Phase bekommt eine Minor-Version, jede Iteration eine Patch-Version (z. B. 0.6.0 erste Zug-Version, 0.6.1 zweite …). Viele Anpassungswünsche zwischendurch sind eingeplant |

## 3. Hardware

| Bauteil | Stand | Bemerkung |
|---|---|---|
| Board | Sunton **ESP32-8048S050C_I** | „C“ = kapazitiver Touch; „_I“ vermutlich IPS-Panel (wird geprüft) |
| SoC | ESP32-S3, 2 × Xtensa LX7 @ 240 MHz | Modul vermutlich ESP32-S3-WROOM-1 **N16R8** |
| Flash / PSRAM | 16 MB / 8 MB (Octal) | wird beim Start ausgelesen und angezeigt |
| Display | 5", **800×480**, paralleles RGB565-Interface | kein Display-Controller mit eigenem Bildspeicher, der ESP32 muss das Bild ständig liefern |
| Hintergrundbeleuchtung | PWM, vermutlich GPIO 2 | dimmbar |
| Touch | **GT911**, kapazitiv, I²C | beim Schwestermodell SDA 19 / SCL 20; braucht keine Kalibrierung |
| Speicherkarte | microSD (SPI) | optional, aktuell nicht eingeplant |
| Erweiterung | Steckverbinder für GPIO/I²C | z. B. für einen nachrüstbaren Lichtsensor |
| **Nicht vorhanden** | Lichtsensor (LDR), RGB-LED, Lautsprecher | anders als beim CYD |

**Die genaue Pinbelegung wird in Phase 0.1 gegen Schaltplan und echtes Gerät geprüft** und in `docs/HARDWARE.md`
dokumentiert. Online ist das 5"-Modell kaum dokumentiert, gut beschrieben ist nur das 4,3"-Schwestermodell
ESP32-8048S043C.

## 4. Grundsatzentscheidungen

### 4.1 ESP-IDF statt Arduino

**Entscheidung:** Die Firmware entsteht mit **ESP-IDF** (Espressifs offiziellem Framework) in **C++20**, ohne Arduino.

Begründung:

- Arduino-ESP32 ist selbst nur eine Schicht über ESP-IDF. **Derselbe Code läuft nicht schneller oder langsamer.** Der
  Unterschied liegt in der Konfiguration: Bei Arduino ist die ESP-IDF-Konfiguration (`sdkconfig`) fest vorkompiliert.
- Für Pixelia brauchen wir genau diese Stellschrauben:
  - IRAM-sichere Display-Interrupts (`CONFIG_LCD_RGB_ISR_IRAM_SAFE`), damit das Bild bei Flash-Zugriffen nicht flackert
  - Code und Konstanten beim Start ins PSRAM spiegeln (`CONFIG_SPIRAM_XIP_FROM_PSRAM`), damit Speichervorgänge und
    OTA-Updates das System nicht anhalten
  - Cache-Größen, Platzierung der WLAN-/TLS-Puffer im PSRAM (spart internen RAM für die Grafik)
  - Bootloader-Rollback für A/B-Updates mit Selbsttest
  - Core-Dump in den Flash für Absturzberichte
- Alle benötigten Bausteine bringt ESP-IDF mit: WLAN, HTTP-Server mit WebSockets, HTTPS-OTA, mDNS, SNTP,
  Zertifikats-Bundle für HTTPS, NVS, LittleFS (Komponente), GT911-Treiber (Komponente).

Nachteile und wie wir damit umgehen:

| Nachteil | Umgang |
|---|---|
| Mehr Schreibarbeit als Arduino | schreibt Claude |
| Viele Arduino-Bibliotheken nicht direkt nutzbar | brauchen wir nicht. Header-only-Bibliotheken wie ArduinoJson laufen trotzdem |
| Arduino IDE geht nicht | gebaut wird in VS Code (ESP-IDF-Erweiterung) und automatisch auf GitHub |
| Lernkurve | betrifft vor allem Claude; der User muss lokal nichts bauen |

Verworfene Mittelwege: „Arduino als ESP-IDF-Komponente“ (Versionskonflikte, mehr Build-Komplexität ohne Nutzen) und
PlatformIO (offizielle Unterstützung für aktuelle ESP-IDF-/Arduino-Versionen auf dem ESP32 ist unsicher).

### 4.2 Grafik-Engine: 400×240, 8 Bit Palette, Hochskalieren im Bounce-Buffer

**Das Problem:** Ein Vollbild mit 800×480 Pixeln in RGB565 ist 768 KB groß. Das RGB-Panel hat keinen eigenen
Bildspeicher, der ESP32 muss das Bild ständig (ca. 30–40 × pro Sekunde) aus seinem Speicher senden. Liegt der
Bildspeicher im PSRAM, liest allein das Display ~25–30 MB/s, und das Zeichnen muss sich dieselbe Leitung teilen. Das ist
der typische Grund für Ruckeln und Bildstörungen bei diesen Boards.

**Entscheidung:**

1. Welten rendern intern in **400×240 Pixeln** mit **8 Bit pro Pixel** (256-Farben-Palette). Ein Bild = **96 KB**.
2. Der RGB-Treiber läuft **ohne Bildspeicher** (`no_fb`) mit kleinen **Bounce-Buffern** im internen RAM. Er ruft eine
   Funktion auf, die die nächsten Zeilen liefert. Dort wird **Palette nachgeschlagen und 2× hochskaliert**
   (jede Quellzeile → zwei Bildzeilen, jedes Pixel → zwei Pixel).
3. **Doppelpufferung:** Gezeichnet wird in den Hinterpuffer, beim vertikalen Rücklauf (VSync) wird getauscht.
   Kein Tearing.
4. **Feste Logik-Rate von 30 Hz**, Rendern entkoppelt. Kern 1: Grafik und Weltlogik. Kern 0: WLAN, Webserver,
   Hintergrund-Simulation.

**Warum das die beste Lösung ist:**

- Das Display liest nur noch ~3,5 MB/s statt ~28 MB/s, also 8× weniger. Rendern im schnellen internen RAM.
- **Jedes Bild kann komplett neu gezeichnet werden.** Parallax, Wetter-Partikel und viele Figuren gleichzeitig, ohne
  komplizierte „nur geänderte Bereiche“-Logik.
- 400×240 bei 2× ist für Pixel-Art ideal: knackscharf, klar erkennbare Figuren auf 5" (bei 800×480 nativ wäre eine
  16-Pixel-Figur nur ~2 mm groß).
- **Palette = Effekte fast gratis:**
  - Tag/Nacht/Dämmerung/Gewitterblitz: 256 Farben umrechnen statt 96.000 Pixel
  - **Farbrotation (Color Cycling)** für Wasser, Feuer, Wasserfälle, Lichterketten – der Klassiker aus den 90ern
  - **Lichttabellen (Colormaps)** für lokales Licht: Laternen, leuchtende Fenster, Fackelschein im Dungeon
  - **Transparenz-Tabellen** für Nebel, Schatten, Glas
- **Rastereffekte**, weil die Umrechnung zeilenweise passiert:
  - Palette pro Zeile → weiche **Himmelsverläufe** bei Sonnenauf- und -untergang
  - horizontaler Versatz pro Zeile → **Wasserspiegelungen**, Hitzeflimmern, Unterwasser-Wellen
  - optionaler **CRT-/Scanline-Filter**
- **Flackerfrei bei OTA und Speichervorgängen**, weil die Display-Versorgung komplett im internen RAM und IRAM läuft.
- **Live-Vorschau und Screenshots** in der Weboberfläche: 96 KB + Palette lassen sich leicht übertragen.
- Mit 256 Farben gleichzeitig lässt sich viel machen (das SNES hatte 256 aus 32.768), dazu Paletten pro Welt.

**Erwartete Last:** Das Hochskalieren kostet geschätzt 10–15 % eines Kerns. Rendern und Logik müssen pro Bild unter
~20 ms bleiben (Budget 33 ms).

**Wird im Prototyp (0.1.x) gemessen:** Panel-Timings (Pixeltakt), echte FPS, Interrupt-Last, ob beide 96-KB-Puffer
in den internen RAM passen (sonst Hinterpuffer im PSRAM und Kopie direkt nach VSync).
**Plan B**, falls die Bounce-Buffer-Lösung nicht trägt: klassischer doppelter Bildspeicher im PSRAM mit
Teil-Aktualisierung.

**Verworfen:** LVGL. Gut für Menüs und Widgets, aber nicht für Vollbild-Spieleszenen und nicht kompatibel mit
Palette-plus-Hochskalieren. Overlays (Info-Karten, Scoreboard, Toasts) zeichnet die eigene Engine mit Pixel-Schriften
inkl. Umlauten.

### 4.3 Speicherstrategie

- **Schnell und klein (interner RAM, ~512 KB):** Front-/Hinterpuffer, Bounce-Buffer, Display-Interrupt, heiße
  Zeichenroutinen.
- **Groß (PSRAM, 8 MB):** gespiegelter Programmcode/Konstanten (XIP), Welt-Grafiken, Simulationszustand, WLAN-/TLS-Puffer.
- **Flash:** Firmware, Einstellungen (NVS), Spielstände (LittleFS).
- Spielstände werden **atomar** geschrieben (temporäre Datei + Umbenennen), damit ein Stromausfall nichts zerstört, und
  nur alle paar Minuten bzw. bei Ereignissen (schont den Flash).
- Grafiken liegen zunächst in der Firmware (eine Datei, ein Update, Version passt immer). Wird das App-Image zu groß für
  den PSRAM-Spiegel, wandern Grafiken in eine eigene Partition, die pro Welt geladen wird. Das wird bei Bedarf entschieden.

### 4.4 PC-Simulator

**Entscheidung:** Engine und Welten sind plattformunabhängiges C++ und zeichnen nur in den 400×240-Puffer. Sie werden
zusätzlich für den PC gebaut:

- **Browser-Version (WebAssembly)** auf GitHub Pages: Link öffnen, Pixelia läuft im Browser, auch auf dem Handy.
  **Der User muss nichts installieren.**
- **Windows-Programm** als Download aus GitHub.
- **Kopflos-Modus** für automatische Screenshots/GIFs: In jedem Pull Request sieht man, wie eine Änderung aussieht.
- **Zeitraffer** (z. B. ein Dorftag in 30 Sekunden), feste Uhrzeit, Wetter erzwingen, Ereignisse auslösen.
- **Automatische Tests** der Simulationslogik (Spielergebnisse, Dungeon-Generator, Tagespläne) bei jedem Push.

Warum: Claude sieht das echte Display nicht. Der Simulator ist der größte Produktivitätshebel des Projekts und
ermöglicht Arbeit in der Cloud ohne Hardware. Hardware-spezifisches (Timing, Touch, Leistung) testet der User auf dem
Gerät.

Einrichtung: macht Claude komplett (SDL für Fenster/Eingabe, Emscripten für die Browser-Version, Build über GitHub
Actions).

### 4.5 Modulare Welten

Jede Welt ist ein Modul mit fester Schnittstelle (Arbeitsentwurf):

| Funktion | Zweck |
|---|---|
| `describe()` | Name, Icon, Einstellungen und Aktionen als Schema (daraus baut die Weboberfläche ihre Seiten) |
| `enter()` / `leave()` | Welt wird angezeigt / verlassen (Grafiken laden/freigeben) |
| `update(dt)` | Logik bei 30 Hz, solange sichtbar |
| `render(canvas)` | zeichnet in den 400×240-Puffer |
| `backgroundTick()` | vereinfachte Simulation, wenn die Welt nicht sichtbar ist (Kern 0, ~1 Hz) |
| `catchUp(sekunden)` | holt nach einem Neustart verpasste Zeit auf |
| `onTouch(x, y, geste)` | Touch-Eingaben |
| `onAction(id, params)` | Aktionen aus der Weboberfläche („Sturm“, „Fest“ …) |
| `save()` / `load()` | Zustand speichern |
| `status()` | Live-Infos für Weboberfläche (z. B. Tabelle, Bewohnerliste) |

**Neue Welt = neuer Ordner + eine Zeile Registrierung.** Die Weboberfläche passt sich automatisch an.

**Datengetriebene Inhalte:** Ereignisse, Berufe, Gegenstände, Gegner, Städte usw. stehen in Datendateien (JSON/YAML im
Repo, beim Build in kompakte Tabellen umgewandelt), nicht verstreut im Code. So kann der Inhalt schnell wachsen.

**Deterministisch:** Zufall läuft über reproduzierbare Zufallsgeneratoren mit Startwert. Gleiche Eingaben = gleiches
Ergebnis auf Gerät und Simulator; Hintergrund- und Live-Simulation liefern dieselben Ergebnisse.

### 4.6 Gemeinsames Universum „Pixelia“

**Entscheidung: Ja – lose gekoppelt.** Pixelia ist das Land, in dem alle Welten spielen. **Das Dorf ist das Herz.**

Verbindungen (Beispiele):

- Der **Held wohnt im Dorf**. Ist er zu Hause, sieht man ihn im Dorf beim Händler, beim Schmied, in der Taverne. Seine
  Beute liegt am Marktstand aus.
- Der **Dorfverein „FC Pixelia“ ist dein Team** in der Liga. Am Heimspieltag strömen die Bewohner zum Sportplatz, nach
  einem Sieg wird auf der Terrasse gefeiert, Kinder kicken in Trikots.
- Der **Zug hält am Dorfbahnhof**. Die anderen Liga-Vereine kommen aus den Städten, an denen der Zug hält. Im Dorf sieht
  man den Zug ein- und ausfahren, manchmal steigen neue Bewohner oder ein Wanderzirkus aus.
- **Gemeinsamer Kalender und gemeinsames Wetter:** Gewitter im Dorf = Gewitter vor dem Zugfenster.
- **Kurzmeldungen** über Weltgrenzen hinweg („TOR für den FC Pixelia!“, während das Zugfenster läuft).

Regeln, damit jede Welt einzigartig bleibt:

- **Jede Welt funktioniert komplett allein** und lässt sich einzeln abschalten. Verbindungen laufen nur über den
  Ereignis-Bus, nie über direkte Aufrufe zwischen Welten. Ein Fehler in einer Welt bricht keine andere.
- **Jede Welt hat eigene Perspektive, eigene Palette, eigene Mechanik und ein eigenes technisches Glanzstück**
  (siehe Kapitel 6). Gemeinsam ist nur ein Stil-Leitfaden, damit Figuren wiedererkennbar sind.
- Schalter „Welten verbinden“ in der Weboberfläche (Standard: an).

Abwägung: Etwas mehr Planungsaufwand (Stil-Leitfaden, Ereignis-Katalog) und die Verbindungen entstehen erst, wenn
beide beteiligten Welten existieren. Dafür wirkt alles zusammen deutlich lebendiger als vier getrennte
Bildschirmschoner. Der Mehraufwand im Code ist klein.

### 4.7 Grafikstil

**Entscheidung: Pixel-Art im 16-Bit-Stil** (SNES/Mega-Drive-Ära), Grundraster 16×16 Pixel, Figuren ca. 12–24 Pixel
hoch, kuratierte Palette pro Welt.

Geprüfte Alternativen:

| Stil | Bewertung auf diesem Board |
|---|---|
| **Pixel-Art (16-Bit)** | ✅ ideal: schnell, scharf bei 2×, wenig Speicher, Palette-Effekte, per Code erzeugbar |
| Pixel-Art isometrisch | ⚠️ sehr schön, aber deutlich mehr Grafikaufwand und Sortierlogik. Möglich für einzelne Szenen |
| 1-Bit / Game-Boy (2–4 Farben) | ⚠️ charmant und leicht zu erstellen, aber weniger Stimmung (Tag/Nacht, Wetter) |
| Vektor/Flat (weiche Kanten) | ❌ braucht volle 800×480 mit Kantenglättung, zu langsam für bewegte Vollbilder |
| Low-Poly-3D | ❌ kein Grafikchip; Software-3D wäre grob und aufwendig |
| Gemalte Hintergründe | ❌ riesige Bilddaten, wenig Animation |

Pixel-Art passt zur Hardware und zum Wunsch nach Leben: Die stimmungsvollsten „lebenden“ Pixelszenen
(Farbrotation, Tag/Nacht) wurden genau mit dieser Technik gemacht.

**Woher die Grafik kommt:**

1. **Prozedural per Code erzeugt:** Landschaften, Himmel, Wolken, Wetter, Dungeons, Pflanzen. Endlose Vielfalt ohne
   Speicherbedarf.
2. **Figuren-Baukasten:** Körper, Frisur, Kleidung, Hautton per Palette kombiniert → jeder Bewohner sieht anders aus.
3. **Freie Grafikpakete mit CC0-Lizenz** als Basis für Gebäude, Möbel, Gegenstände, z. B.:
   - **Kenney** (kenney.nl): niederländisches Studio, tausende Spielgrafiken unter **CC0** (gemeinfrei: jede Nutzung
     erlaubt, auch Weitergabe, ohne Namensnennung). Pakete wie *Tiny Town*, *Tiny Dungeon*, *1-Bit Pack*.
   - **Ninja Adventure** (itch.io, CC0): Top-Down-Paket mit animierten Figuren.
   - **OpenGameArt.org**: nur Einträge mit CC0 oder CC-BY (Namensnennung) verwenden.
4. **Eigene Sprites**, die Claude als Pixel-Raster im Repo anlegt, bei Bedarf auch vom User.

Regeln:

- **Nur Lizenzen, die Weitergabe erlauben (CC0, CC-BY)**, weil das Repo öffentlich ist und die Grafiken darin liegen.
  Viele itch.io-Pakete verbieten das Weitergeben der Rohdateien.
- Jede fremde Grafik wird mit Quelle und Lizenz in `assets/CREDITS.md` erfasst.
- Pakete werden auf die Welt-Palette umgefärbt und mit eigenen Animationen ergänzt, damit kein Stilmix entsteht.
- Die Asset-Pipeline (Python) wandelt PNG automatisch in das Palettenformat. Grafiken lassen sich jederzeit
  austauschen.

### 4.8 Weboberfläche

- **Svelte 5 + TypeScript + Vite.** Kleine Bundles, gut für viele Einstellungsseiten.
- Gebaut auf GitHub, **gzip-komprimiert in die Firmware eingebettet**. Firmware und Oberfläche passen immer zusammen,
  ein Update tauscht beides.
- **Komplett offline-fähig:** keine CDNs, Schriften (Roboto Flex, auf benötigte Zeichen reduziert) und Icons
  (Material Symbols, nur die benutzten) eingebettet. Wichtig, weil im Setup-Modus (Access Point) kein Internet da ist.
- **Eigene, schlanke Material-3-Expressive-Komponentenbibliothek.** Googles offizielle Web-Komponenten
  (`@material/web`) werden nicht mehr weiterentwickelt und unterstützen Expressive nicht. Umgesetzt werden:
  Design-Tokens, Formen-Vielfalt und Form-Morphing, federnde Animationen, ausdrucksstarke Typografie, Button-Gruppen,
  wellenförmige Fortschrittsanzeigen, dynamische Farben aus einer Akzentfarbe (`material-color-utilities`),
  hell/dunkel.
- Live-Werte per **WebSocket**.
- Seitenübersicht: Kapitel 7.

### 4.9 Flash-Aufteilung (16 MB)

| Partition | Größe | Inhalt |
|---|---|---|
| nvs | 24 KB | Einstellungen, WLAN-Zugangsdaten |
| otadata | 8 KB | welche App-Partition aktiv ist |
| phy_init | 4 KB | Funk-Kalibrierung |
| app0 (ota_0) | 6 MB | Firmware A (inkl. Weboberfläche und Grafiken) |
| app1 (ota_1) | 6 MB | Firmware B |
| storage (LittleFS) | ~3,8 MB | Spielstände, Statistiken, Chroniken, Screenshots |
| coredump | 64 KB | Absturzbericht |

Die Oberfläche warnt, wenn die Partitionstabelle auf dem Gerät nicht zur Firmware passt.

### 4.10 Updates: A/B mit Rollback und Selbsttest

- **A/B-Prinzip:** Zwei App-Partitionen. Läuft A, wird das Update nach B geschrieben. Bricht der Download ab, bleibt A
  unberührt aktiv.
- Nach erfolgreichem Schreiben (Prüfsumme ok) startet das Gerät von B im Zustand **„Probelauf“**.
- **Selbsttest:** B muss sich gesund melden (Display läuft, Engine liefert Bilder, WLAN verbunden oder Setup-Modus
  aktiv, ~60 s ohne Absturz). Erst dann wird B dauerhaft aktiv.
- **Stürzt B vorher ab oder hängt** (Watchdog), startet der Bootloader automatisch wieder A. Die Oberfläche zeigt
  danach: „Update auf vX.Y.Z wurde zurückgerollt“ samt Absturzbericht.
- Einstellungen und Spielstände liegen außerhalb der App-Partitionen und bleiben erhalten.
- **Zwei Update-Wege:**
  1. `.bin` im Browser hochladen (lokal gebaut oder von GitHub heruntergeladen)
  2. **„Update von GitHub“:** Das Gerät fragt regelmäßig (z. B. alle 6 h) die GitHub-Releases ab, zeigt „Neue Version
     verfügbar“ mit Changelog. Ein Klick lädt per HTTPS direkt von GitHub. **Kanal wählbar: Stabil oder Vorschau.**
- Fortschritt in der Oberfläche **und** auf dem Display.
- Vor dem Annehmen eines Images wird geprüft: gültiges ESP-App-Image, Projektname „pixelia“, Chip ESP32-S3, Prüfsumme.

### 4.11 Versionierung und Changelog

**Versionsschema (Vorgabe des Users):** immer drei Stellen `MAJOR.MINOR.PATCH`.

- **MINOR = Phase bzw. Welt**, z. B. 0.6.x = Zugfenster.
- **PATCH = jede Iteration** innerhalb der Phase: 0.6.0 erste Zug-Version, 0.6.1 zweite, 0.6.2 dritte … bis zur
  nächsten Phase (0.7.0).
- Vor 1.0.0 sind alle Versionen Vorabversionen. 1.0.0 = alle vier Welten fertig und verbunden, Dauertest bestanden.
- Nach 1.0.0 (Vorschlag): neue Welt oder großes Feature = neue MINOR, Iterationen = PATCH.
- **Vorschau-Kanal:** Testversionen als `X.Y.Z-beta.N` (GitHub-Prerelease). Nur Geräte im Kanal „Vorschau“ bieten sie an.

**Die Version kommt aus dem Git-Tag** (`v0.6.1`). Der Build schreibt sie automatisch in die Firmware (ESP-IDF
App-Beschreibung). Kein manuelles `FW_VERSION`, das man vergessen kann. Die Oberfläche liest die Version direkt aus dem
Kopf jeder `.bin`-Datei.

**Changelog:** eine einzige Quelle `CHANGELOG.json` im Repo-Hauptverzeichnis, neuester Eintrag oben, im bewährten
Format, ergänzt um ein Datum:

```json
[
  {
    "v": "0.6.1",
    "d": "2026-12-03",
    "t": "Regen am Fenster",
    "s": "Wetter wird sichtbar",
    "n": ["Zug: Regentropfen laufen an der Scheibe herab", "Zug: Gewitter mit Blitzen (Vorschau)"],
    "i": ["Display: Himmelsverlauf weicher"],
    "f": ["WLAN: Neuverbindung nach Router-Neustart"],
    "r": []
  }
]
```

- Jeder Stichpunkt beginnt mit dem Bereich: `System`, `Display`, `WLAN`, `Web`, `Update`, `Engine`, `Zug`, `Dorf`,
  `Held`, `Liga`, `Touch` …
- Experimentelles bekommt „(Vorschau)“ und liegt hinter dem Experimentell-Schalter.
- Der Build erzeugt daraus **automatisch**: (1) einen eingebetteten Block in der Firmware mit Markierung, damit die
  Oberfläche ihn aus jeder `.bin` lesen und nur die Änderungen seit der installierten Version zeigen kann, und
  (2) die **Release-Notes auf GitHub**. Eine Quelle, keine Doppelpflege.
- Versionsverlauf in der Oberfläche als aufklappbare Timeline mit Versions-Badge, „Installiert“-Markierung und Gruppen
  mit Icons (wie im CYD-Projekt).

### 4.12 WLAN, Netzwerk, Sicherheit

- **Bis zu 5 gespeicherte WLANs** (statt 2), verbunden wird mit dem stärksten aus dem Scan.
- **Setup-Modus:** Ohne WLAN startet der Access Point **„Pixelia-Setup“** mit Captive Portal (192.168.4.1, inkl. der
  Erkennungsadressen von Android, iOS und Windows). Der AP bleibt aktiv, während **alle 2 Minuten** im Hintergrund ein
  neuer Verbindungsversuch läuft. Das Display zeigt den Setup-Modus mit Anleitung und QR-Code zum Verbinden.
- **Hostname „Pixelia“** im Router, **mDNS: `http://pixelia.local`**.
- **WLAN-Passwörter verlassen das Gerät nie** (die Oberfläche sieht nur „gespeichert ✓“).
- **PIN/Passwort optional, standardmäßig aus.** Wenn aktiv, schützt es Einstellungen, Aktionen, Updates und Werksreset;
  mit Sperre nach mehreren Fehlversuchen. Werksreset auch per Touch-Geste am Gerät möglich (falls PIN vergessen).
- **Verbundene Geräte:** Jeder Browser bekommt eine zufällige Geräte-ID (lokal im Browser gespeichert). Angezeigt
  werden: IP, Browser und Betriebssystem (aus der Browser-Kennung, z. B. „Chrome · Android“), erste Verbindung,
  letzte Aktivität, aktuelle Seite, Online-Status. **Einen echten Gerätenamen gibt ein Browser nicht preis.** Deshalb
  kann man jedem Gerät einen eigenen Namen geben („Handy Wohnzimmer“). Im Setup-Modus zusätzlich die am Access Point
  angemeldeten Geräte (MAC, ggf. Hostname).
- **Bewusst verzichtet:** HTTPS auf dem Gerät (selbstsignierte Zertifikate erzeugen Browser-Warnungen, kostet RAM) sowie
  Secure Boot und Flash-Verschlüsselung (brennen unumkehrbar eFuses, ein Fehler macht das Board unbrauchbar).
  Stattdessen Image-Prüfung bei Updates (4.10).

### 4.13 GitHub-Workflow

- **Repo `pixelia`, öffentlich** (Empfehlung). Gründe: Das Gerät kann Updates ohne Zugangsschlüssel laden, GitHub Pages
  (Web-Flasher, Browser-Simulator) ist für öffentliche Repos kostenlos, und es liegen keine Geheimnisse im Code
  (WLAN-Daten nur auf dem Gerät).
- **Branches und Pull Requests:** Jede Änderung auf einem eigenen Branch → Pull Request → automatischer Build und
  Simulator-Screenshots → der User schaut drüber → Merge.
- **GitHub Actions** bauen bei jedem Push: Firmware, Weboberfläche, Simulator (Windows + Browser), Tests.
- **Release:** Tag `vX.Y.Z` → automatisch GitHub-Release mit `pixelia-vX.Y.Z.bin` (für OTA),
  `pixelia-vX.Y.Z-factory.bin` (für die Erstinstallation) und Release-Notes aus `CHANGELOG.json`.
- **GitHub Pages:** Web-Flasher (ESP Web Tools: Board per USB anstecken, in Chrome/Edge auf „Installieren“) und
  Browser-Simulator.
- **Wünsche** gerne einfach im Chat. Optional als GitHub-Issues mit Label pro Welt, dann geht nichts verloren.
- **Cloud-Arbeit:** Mit verbundenem GitHub kann Claude auch über claude.ai/code von überall am Repo arbeiten.
  Hardware-Tests macht der User; Fotos und Videos helfen bei Problemen.

### 4.14 Bewusst gestrichen oder geändert gegenüber dem CYD-Projekt

| CYD-Vorgabe | Pixelia |
|---|---|
| `FW_VERSION` in der `.ino` | Version aus Git-Tag, automatisch |
| `HIST` in `web/versionsverlauf.h` | `CHANGELOG.json` als einzige Quelle, Header und Release-Notes werden erzeugt |
| 2 WLANs | 5 WLANs |
| Lichtsensor (lernend) | kein Sensor auf dem Board → Auto-Helligkeit nach Sonnenstand. **Optional:** I²C-Lichtsensor BH1750 (ca. 2 €) nachrüsten, dann wieder lernend mit Roh/Mittel/Min/Max |
| Touch-Kalibrierung (4 Kreuze) | entfällt, kapazitiver GT911 ist werkskalibriert. Touch-Test bleibt |
| RGB-Status-LED | nicht vorhanden → Statusanzeige auf dem Display und in der Oberfläche |
| Stundengong, Ton testen | kein Lautsprecher → gestrichen (später nachrüstbar über I²S-Verstärker) |
| Kantenglättung an/aus | entfällt (Pixel-Art soll scharf sein) → stattdessen CRT-/Scanline-Filter |
| Screenshot als BMP | Screenshot als **PNG** (kleiner), zusätzlich Live-Vorschau |
| „Arduino-Core“ auf der Hardware-Seite | ESP-IDF-Version, Compiler-Version, Build-Datum, Git-Commit |

## 5. Gemeinsame Dienste

| Dienst | Aufgabe |
|---|---|
| **Weltzeit** | Echtzeit per NTP, Zeitzone (Standard Europa/Berlin), Sonnenauf-/-untergang und Mondphase für den Standort, Jahreszeiten, deutsche Feiertage und Anlässe (Advent, Weihnachten, Silvester, Ostern, Halloween …) |
| **Standort** | Stadt in den Einstellungen eintragen → Koordinaten über die kostenlose Geokodierung von **Open-Meteo** (kein Login, kein Schlüssel) |
| **Wetter** | Wahlweise **echtes Wetter** (Open-Meteo, alle 30 min), **simuliert** (plausibel nach Jahreszeit) oder **fest**. Aktionen aus der Oberfläche überschreiben kurzfristig („Sturm jetzt“) |
| **Ereignis-Bus** | Welten melden Ereignisse; andere Welten, Kurzmeldungen und Weboberfläche hören zu |
| **Kurzmeldungen** | Kleine Einblendungen auf dem Display (abschaltbar, pro Welt filterbar) und in der Oberfläche |
| **Regie** | Wiedergabeliste: Rotation nach Zeit, Zeitplan (z. B. abends Zugfenster), feste Welt; Live-Unterbrechung bei Spielen des FC Pixelia; Kamera-Regie innerhalb der Welten |
| **Nachtmodus** | Zeitplan oder Sonnenstand: Display dimmen, Nachtszene oder aus; Touch weckt |
| **Helligkeit** | manuell, nach Sonnenstand oder (mit Nachrüst-Sensor) nach Umgebungslicht |
| **Touch** | Gesten (Tippen, Wischen, langes Drücken), alles in der Oberfläche einstellbar: an/aus, welche Gesten was tun, Info-Karten an/aus, Empfindlichkeit, Sperre |
| **Speichern** | Spielstände atomar in LittleFS, Einstellungen in NVS, Export/Import aller Einstellungen als Datei |
| **Diagnose** | Leistungszähler (FPS, Frame-Zeit pro Stufe, CPU-Last pro Kern), Live-Log, Absturzberichte |

## 6. Die Welten

### 6.0 Prinzipien gegen Langeweile

- **Drei Zeitebenen:**
  - *Sekunden:* ständige Mikro-Animationen (Rauch, Vögel, blinzelnde Figuren, wehende Fahnen, Blätter, Glühwürmchen)
  - *Stunden:* Tagesablauf, Wetterwechsel, Ereignisse
  - *Wochen:* Fortschritt (Dorf wächst, Held wird stärker, Saisons, Jahreszeiten)
- **Ereignisse in Seltenheitsstufen:** häufig (mehrmals pro Stunde), gelegentlich (täglich), selten (wöchentlich),
  legendär (alle paar Monate). Man entdeckt auch nach Wochen noch Neues.
- **Kamera-Regie:** Die Kamera schwenkt von selbst zu interessanten Momenten.
- **Ausdrucksstarke Kleinigkeiten:** Sprechblasen-Icons (♥, Zzz, ♪, 🍞, !, ?), Gedanken auf Info-Karten.
- **Echtzeitbezug:** Tageszeit, Jahreszeit, Wetter und Feiertage stimmen mit der echten Welt überein (abschaltbar).
- **Jede Welt hat ein technisches Glanzstück**, das sie unverwechselbar macht.

### 6.1 Zugfenster (0.6.x) – „Der ruhige Ausgleich“

*Glanzstück: Parallax-Landschaft mit Rastereffekten (Himmelsverläufe pro Zeile, Wasserspiegelungen) und Regen auf der Scheibe.*

- **Blick aus dem Abteilfenster:** Rahmen, Vorhang, kleiner Tisch mit dampfender Kaffeetasse; bei Nacht spiegelt sich das
  Abteil schwach in der Scheibe.
- **Endlose, prozedural erzeugte Landschaft aus 6–8 Ebenen** mit unterschiedlicher Geschwindigkeit: Masten und Büsche
  vorne rasen vorbei, Berge hinten ziehen langsam.
- **Regionen mit fließenden Übergängen:** Küste mit Leuchtturm, Wälder, Alpen mit Viadukten, Felder und Weinberge, Seen,
  Flüsse mit Booten, Kleinstädte, Großstadt bei Nacht mit Lichtermeer, Industriegebiet, Tunnel.
- **Tageszeit:** weicher Himmelsverlauf, Sonne, Mond (echte Mondphase), Sterne, Sternschnuppen.
- **Wetter:** Regentropfen, die schräg über die Scheibe laufen; Schnee; Nebel; Gewitter mit Blitzen, die die Landschaft
  kurz erhellen; Regenbogen nach dem Regen.
- **Ereignisse:** Gegenzug rauscht vorbei, Tunnel (dunkel, Lampen huschen vorbei), Vogelschwärme, Hirsche am Waldrand,
  Kühe auf der Weide, Heißluftballons, Feuerwerk über der Stadt, Polarlicht (legendär).
- **Bahnhöfe:** Zug bremst, Bahnsteig mit wartenden Leuten, Stadtschild (fiktive Städte von Pixelia, darunter der
  Dorfbahnhof Pixelia), Halt, Abfahrt. Anzeige „Nächster Halt: …“.
- **Einstellungen und Aktionen:** Region/Route, Tempo, Wetter (echt/simuliert/fest), Tageszeit (echt/fest/Zeitraffer),
  Zugtyp (Regionalbahn, Schnellzug, Dampflok-Abteil, Nachtzug), Regen jetzt, Tunnel jetzt.
- **Touch:** Fenster antippen → Info zur Region bzw. Stadt; Wischen → Zug beschleunigen/bremsen (optional).

### 6.2 Pixel-Dorf (0.7.x) – „Das Herz von Pixelia“ (Schwerpunkt)

*Glanzstück: Bewohner-Simulation mit Bedürfnissen, Beziehungen und Tagesplänen, dazu Licht bei Nacht (leuchtende
Fenster, Laternen) und eine Dorfchronik.*

**Perspektive:** Wird zu Beginn von 0.7.0 anhand von Simulator-Mockups gemeinsam entschieden.

- (A) Draufsicht 3/4 wie klassische Rollenspiele: man sieht Wege, Plätze und Felder, Innenräume nur angedeutet.
- (B) **Puppenhaus-Querschnitt** von der Seite: Häuser sind offen, man sieht *drinnen* den Bäcker um 5 Uhr Teig kneten,
  Familien beim Abendessen und Leute in ihren Betten schlafen. Straße, Terrasse und Marktplatz liegen davor, dahinter
  Hügel mit Parallax.
- Tendenz: **(B)**, weil man beim Nebenbei-Hinschauen am meisten Leben sieht und das breite Display als lebendiges
  Diorama perfekt passt.

**Bewohner (anfangs 30–40, wachsend bis ~60):**

- Jeder mit **Name, Alter, Beruf, Zuhause, Aussehen** (Figuren-Baukasten, jeder einzigartig), **Charakterzügen**
  (Frühaufsteher/Nachteule, gesellig/schüchtern, fleißig/gemütlich, …), **Bedürfnissen** (Hunger, Energie,
  Geselligkeit, Spaß, Hygiene) und **Beziehungen** (Familie, Freunde, Paare, kleine Rivalitäten).
- **Entscheidungen:** Tagesplan (Arbeit, Schule) plus Bedürfnisse entscheiden, was jemand gerade tut. Wer Hunger hat,
  holt sich ein Brötchen; wer gesellig ist, geht abends in die Taverne.
- **Berufe:** Bäckerin (ab 4–5 Uhr), Bauer (Säen, Ernten je nach Jahreszeit), Schmied, Händlerin am Markt, Wirt (Taverne
  bis Mitternacht), Lehrerin mit Schulkindern (Pause auf dem Schulhof), Postbote, **Nachtwächter (zündet in der
  Dämmerung die Laternen an)**, Fischer am Teich, Ärztin, Bahnhofsvorsteher, Trainer des FC Pixelia, Rentner auf der
  Parkbank.
- **Tiere:** Katzen (schlafen in der Sonne, sitzen auf Dächern), Hunde (folgen ihren Besitzern), Hühner, Enten im Teich,
  Vögel, nachts Fuchs, Eule und Glühwürmchen.

**Tagesablauf (Beispiel Werktag):**
früh Bäckerei hell, erste Kunden; morgens Schulweg und Arbeitsbeginn; mittags Markt und Mittagessen; nachmittags
spielende Kinder und Handwerk; abends Feierabend, Terrasse und Taverne voll, Fenster leuchten auf, Nachtwächter
unterwegs; nachts fast alle schlafen, ein paar Nachteulen, Fuchs im Gemüsebeet. Wochenende anders (Markttag, Ausflüge,
Heimspiel).

**Wetter und Jahreszeiten:** Regen → Schirme, Leute bleiben drinnen, Pfützen; Schnee → Schneemann bauen,
Schneeballschlacht, Rauch aus allen Schornsteinen; Herbst → bunte Blätter, Kürbisse, Erntefest; Sommer → Terrasse voll,
Badesee, Eiswagen; Frühling → Blüten, Gartenarbeit.

**Ereignisse (datengetrieben, ständig erweiterbar):**

- *häufig:* Gespräche, Brötchen holen, Katze jagt Vogel, Kinder spielen Fangen
- *gelegentlich:* Markttag, Geburtstagsfeier, Postlieferung, Streit und Versöhnung, Heimspiel des FC Pixelia
- *selten:* Hochzeit, Geburt, neue Familie zieht ein (kommt mit dem Zug), Brand mit Feuerwehreinsatz, Dieb in der Nacht,
  Dorffest/Kirmes, Wanderzirkus
- *legendär:* Heißluftballon landet, Polarlicht, Sternschnuppenregen, UFO
- *Anlässe:* Adventsbeleuchtung, Weihnachtsmarkt, Silvesterfeuerwerk, Ostereiersuche, Halloween

**Langzeit-Entwicklung:** Das Dorf wächst über Wochen. Neue Häuser werden gebaut (Baustelle über Tage sichtbar),
Bewohner ziehen ein und aus, Kinder werden älter, Paare finden sich, Läden eröffnen.

**Kamera:** Rundgang (gleitet langsam durchs Dorf), Ereignis-Fokus (schwenkt zu Hochzeit, Brand, Fest), „Folge
Bewohner X“, fest.

**Touch:** Bewohner antippen → Info-Karte (Name, Alter, Beruf, Laune, was er/sie gerade tut, aktueller Gedanke wie
„Hoffentlich gibt es noch Zimtschnecken“). Gebäude antippen → wer ist drin, was passiert dort.

**Weboberfläche:** Bewohnerliste mit Steckbriefen und Beziehungen, **Dorfchronik** (Tagebuch aller Ereignisse),
Statistiken, Aktionen: Sturm/Gewitter, Schnee, Feuerwerk, Fest ausrufen, Regenbogen, neue Familie einladen, Bewohner
umbenennen, **eigenen Bewohner erstellen**, geheime Aktion (UFO).

### 6.3 Dungeon-Held (0.8.x) – „Das Abenteuer im Hintergrund“

*Glanzstück: prozedurale Dungeons mit Nebel des Krieges und Fackel-Lichtkegel (Lichttabellen).*

- **Held:** Name, Klasse (Krieger, Magierin, Waldläufer, …), Werte, Level, Talente; **Ausrüstung sichtbar am Sprite**.
- **Dungeons:** prozedural erzeugt (Räume und Gänge, Höhlen, Krypta, Eisgrotte, Lavaschmiede, versunkener Tempel),
  mehrere Ebenen, Nebel des Krieges, Fackelschein, Fallen, Truhen, Geheimräume.
- **Kampf:** automatisch, mit Schadenszahlen, Treffereffekten, Zaubern; Gegnertypen je Dungeon-Thema; **Bosse** alle
  paar Ebenen als Höhepunkt.
- **Beute:** Seltenheitsstufen (Gewöhnlich → Legendär) mit Lichtsäule beim Fund, Sets, Tränke, Gold.
- **Rhythmus:** Expedition → Rückkehr ins Dorf → Händler (Beute verkaufen, sichtbar am Marktstand), Schmied
  (Verbesserungen), Taverne, Schlafen → nächste Expedition. Der Rhythmus ist einstellbar.
- **Tod:** Rückkehr ins Dorf, etwas Gold verloren, kein Fortschrittsverlust.
- **Hintergrund:** Ist die Welt nicht sichtbar, läuft eine statistische Simulation weiter, mit Abenteuer-Log.
- **Weboberfläche:** Charakterbogen, Inventar, Statistiken (besiegte Monster, Gold, tiefste Ebene), Abenteuer-Log;
  Aktionen: Heiltrank schicken, Segen (zeitweiser Bonus), Dungeon-Thema wählen, Held umbenennen.
- **Touch:** Held antippen → Werte; Truhe antippen → Inhalt.

### 6.4 Pixel-Liga (0.9.x) – „Mitfiebern“

*Glanzstück: Spielengine mit Taktik und Spielerwerten plus Live-Übertragung im Draufsicht-Stil mit Torwiederholung.*

- **Liga:** 12–18 Vereine (einstellbar), fiktive Clubs aus den Städten von Pixelia, dein Verein **FC Pixelia**.
  Optional mehrere Ligen mit Auf- und Abstieg, dazu ein Pokal.
- **Spielplan:** Spieltage nach Zeitplan (z. B. täglich 20 Uhr oder alle X Stunden, einstellbar), parallele Anstöße.
- **Spielengine:** simuliert Spielzüge mit Spielerwerten, Form, Müdigkeit, Taktik, Heimvorteil und Zufall. **Dieselbe
  Engine** läuft für Live- und Hintergrundspiele, gleiche Ergebnisse, ob man zuschaut oder nicht. Die Live-Ansicht
  inszeniert die Spielzüge mit 22 Spielern, Ball, Jubel und Torwiederholung.
- **Konferenz:** Laufen mehrere Spiele parallel, zeigt ein Scoreboard alle Spielstände, mit Einblendung bei Toren.
- **Saison:** Tabelle, Torjägerliste, Form-Kurven, Verletzungen, Sperren; Spieler altern, Talente aus der Jugend, einfache
  Transfers in der Sommerpause; Historie vergangener Meister.
- **Dein Team:** Name, Farben, Wappen (Pixel-Editor in der Oberfläche), Aufstellung, Taktik, Kapitän.
- **Regie:** Spiel des FC Pixelia unterbricht auf Wunsch die Wiedergabeliste; Tor-Kurzmeldungen auf allen Welten.
- **Weboberfläche:** Tabelle, Spielplan, Ergebnisse, Liveticker, Kader und Statistiken, Historie.
- **Touch:** Spieler antippen → Name und Werte; Scoreboard antippen → zum Spiel wechseln.

### 6.5 Spätere Ideen (nicht eingeplant)

Weitere Welten als Module: Aquarium, Raumstation, Ameisenfarm, Leuchtturm am Meer, Bauernhof, Wetterstation-Ansicht,
Uhr/Kalender-Overlay. Sammelt sich in [ROADMAP.md](ROADMAP.md) im Backlog.

## 7. Weboberfläche

Erreichbar unter `http://pixelia.local` bzw. der IP. Navigation als Navigationsleiste (Handy) bzw. Seitenleiste
(Desktop).

| Seite | Inhalt |
|---|---|
| **Übersicht** | aktuelle Welt, **Live-Vorschau** des Displays, Schnellaktionen, Statuskacheln (WLAN, Speicher, Temperatur, Laufzeit, Update verfügbar) |
| **Welten** | je Welt: Einstellungen, Aktionen, Welt-Infos (Bewohner, Chronik, Held, Tabelle …), automatisch aus dem Modul-Schema |
| **Anzeige & Regie** | Helligkeit, Nachtmodus, Wiedergabeliste/Zeitplan, Kurzmeldungen, CRT-Filter, FPS-Anzeige |
| **Touch** | an/aus, Gesten-Belegung, Info-Karten, Empfindlichkeit, Touch-Test |
| **Standort & Zeit** | Stadt, Zeitzone, Wetterquelle (echt/simuliert/fest), Sonnenzeiten |
| **WLAN** | gespeicherte Netze (max. 5, Reihenfolge), Netzwerkscan mit Signalbalken, Hostname, Setup-Modus |
| **Verbundene Geräte** | IP, Browser/OS, erste Verbindung, zuletzt aktiv, eigener Gerätename, online/offline |
| **Info** | Laufzeit, WLAN/RSSI mit Bewertung, IP/Gateway/DNS, letzter Datenabruf (Wetter, GitHub, NTP), Speicherbalken (Programm, Heap intern, PSRAM, LittleFS), Partitionswarnung |
| **Hardware** | siehe Kapitel 8 |
| **Experimentell** | siehe Kapitel 8 |
| **Update & Versionsverlauf** | Update von GitHub (Kanal), `.bin` hochladen mit Änderungsvorschau, Fortschritt, Timeline |
| **Protokoll** | Live-Log mit Filter, herunterladbar |
| **Absturzberichte** | letzter Absturz mit Backtrace, Reset-Historie |
| **System** | Neustart, Werksreset, Einstellungen exportieren/importieren, PIN/Passwort |

## 8. Experimentell und Hardware-Infos

**Hardware (vom Gerät selbst ausgelesen):** Boardvariante, Panel und Timings, Touch-Controller (GT911, Firmware-ID),
Chipmodell und Revision, Kerne und Takt, Flash-Größe/-Modus/-Takt, PSRAM-Größe/-Modus/-Takt, MAC-Adressen (WLAN/BT),
Pinbelegung, Chiptemperatur, Resetgrund (Einschalten / Software / Watchdog / Absturz / Unterspannung / Deep Sleep …),
Reset-Historie, ESP-IDF-Version, Compiler, Build-Datum, Git-Commit, Partitionstabelle mit Belegung.

**Leistung (live):** FPS, Frame-Zeit aufgeteilt (Logik / Rendern / Hochskalier-Interrupt), CPU-Last pro Kern,
Task-Liste mit Stack-Reserven, Heap intern und PSRAM (frei, größter Block, Fragmentierung, Tiefstwert seit Start),
WLAN-Durchsatz.

**Experimentell (Live-Werte und Tests):**

- Touch-Test mit Live-Koordinaten und Mehrfinger-Anzeige
- Displaytests: Farbbalken, Verläufe, Schachbrett, Weiß, Ränder, Pixelfehler-Suche, Farbrotations-Demo
- Screenshot (PNG) und Live-Vorschau
- Stromsparmodus (160 statt 240 MHz) mit FPS-Vergleich
- Panel-Timing-Feinabstimmung (Pixeltakt) für unterschiedliche Panel-Chargen
- CRT-/Scanline-Filter
- Welt-Debug: Zeitraffer, Uhrzeit/Wetter erzwingen, Ereignis auslösen, Kollisions-/Pfadanzeige
- Funktionen mit „(Vorschau)“ hinter dem Experimentell-Schalter
- Optional (nur mit nachgerüstetem BH1750): Lichtsensor-Werte Roh/Mittel/Min/Max, lernende Auto-Helligkeit, „Sensor umkehren“

## 9. Leistungsbudget und Qualitätsregeln

| Größe | Ziel |
|---|---|
| Bildrate | **30 fps stabil**, nie unter 25 fps |
| Frame-Budget | 33,3 ms. Logik + Rendern ≤ 20 ms auf Kern 1 |
| Hochskalier-Interrupt | ≤ 15 % eines Kerns |
| Freier interner RAM | jederzeit ≥ 60 KB |
| Freier PSRAM | jederzeit ≥ 1 MB |
| Firmware-Größe | ≤ 75 % der App-Partition (Warnung im Build bei Überschreitung) |
| Weboberfläche | gzip ≤ 400 KB inkl. Schriften und Icons |
| Stabilität | 72 h Dauertest ohne Neustart vor 1.0.0 |

Regeln:

- Kein Release, wenn der Build die Budgets reißt oder Tests fehlschlagen.
- Jede visuelle Änderung bekommt im Pull Request einen Simulator-Screenshot.
- Leistungszähler sind immer eingebaut (in der Oberfläche abrufbar, auf dem Display einblendbar).
- Watchdog aktiv; Abstürze landen als Bericht in der Oberfläche.

## 10. Repo-Struktur (Entwurf)

```
pixelia/
├─ CLAUDE.md              Arbeitsregeln für Claude (jede Sitzung)
├─ CHANGELOG.json         einzige Quelle für den Versionsverlauf
├─ README.md
├─ docs/
│  ├─ PROJEKTPLAN.md      dieses Dokument
│  ├─ ROADMAP.md          Phasen, Versionen, Status, Backlog
│  ├─ HARDWARE.md         verifizierte Pinbelegung und Panel-Timings (ab 0.1.x)
│  └─ STIL.md             Stil-Leitfaden für Grafik (ab 0.5.x)
├─ firmware/              ESP-IDF-Projekt (main, Hardware-Treiber, Netzwerk, Webserver, OTA)
├─ engine/                plattformunabhängig: Renderer, Palette, Sprites, Tiles, Schrift, Partikel, Zufall, Ereignis-Bus
├─ worlds/
│  ├─ train/
│  ├─ village/
│  ├─ hero/
│  └─ league/
├─ sim/                   PC- und Browser-Simulator (SDL / Emscripten)
├─ web/                   Weboberfläche (Svelte + TypeScript)
├─ assets/                Quell-PNGs, Paletten, Datendateien, CREDITS.md
├─ tools/                 Asset-Pipeline, Changelog-Generator, Hilfsskripte (Python)
└─ .github/workflows/     Builds, Tests, Releases, GitHub Pages
```

## 11. Entwicklungsumgebung und Arbeitsweise

**Auf dem PC des Users (einmalig, Claude unterstützt Schritt für Schritt):**

- Git für Windows, GitHub CLI (Anmeldung macht der User selbst im Browser)
- ESP-IDF (für schnelles Flashen per USB und serielle Logs in Phase 0. Danach läuft alles über OTA)
- Node.js (Weboberfläche lokal bauen)
- Compiler für den Simulator (optional, Builds laufen auch auf GitHub)

Stand 18.10.2026: VS Code und winget vorhanden. Git, GitHub CLI, Node.js, CMake und ESP-IDF fehlen noch.

**Entwicklungsschleife:**

- *Schnell (lokal):* bauen → per USB oder OTA aufs Gerät → testen. Für Phase 0 und Feinarbeit.
- *Offiziell (GitHub):* Pull Request → CI → Merge → Tag → Release → Gerät bietet Update an.
- *Visuell:* Simulator (lokal, Browser oder als Screenshot im Pull Request).

**Rollen:**

- Claude: Architektur, Code, Grafiken, Tests, Doku, Releases vorbereiten.
- User: Ideen, Wünsche, Entscheidungen bei Geschmacksfragen, Tests auf dem echten Gerät, Freigaben (Merge, Release).

## 12. Risiken und offene Punkte

| Risiko | Gegenmaßnahme |
|---|---|
| Bounce-Buffer-Ansatz trägt auf diesem Panel nicht | Prototyp in 0.1.x mit Messung, Plan B (doppelter PSRAM-Bildspeicher) |
| Interner RAM reicht nicht für zwei 96-KB-Puffer | Hinterpuffer im PSRAM, Kopie direkt nach VSync; WLAN-/TLS-Puffer ins PSRAM |
| Pinbelegung/Panel-Timings weichen ab | Prüfung in 0.1.x, Timings in der Oberfläche einstellbar |
| Menge und Qualität der Pixel-Art | prozedural + Figuren-Baukasten + CC0-Pakete + Pipeline; User kann jederzeit Grafiken beisteuern |
| Umfang wächst durch viele Wünsche | Patch-Iterationen, Backlog in ROADMAP.md, eine Phase nach der anderen |
| GitHub-Einstieg für den User | Claude führt Schritt für Schritt durch, Abläufe weitgehend automatisiert |
| Lizenzprobleme bei Grafiken | nur CC0/CC-BY, CREDITS.md |
| PSRAM-Spiegel (XIP) wird zu groß | Grafiken in eigene Partition, pro Welt laden |

**Offen (wird im Verlauf entschieden):**

- Perspektive des Dorfs (Draufsicht vs. Puppenhaus-Querschnitt), zu Beginn von 0.7.0 mit Mockups
- Genaue Panel-Timings und Pixeltakt (0.1.x)
- Liga-Größe und Spielrhythmus (0.9.0)
- Rhythmus des Helden (wie oft zu Hause, 0.8.0)

## 13. FAQ – Fragen aus der Planungsphase

**ESP-IDF statt Arduino – Vor- oder Nachteile? Was läuft flüssiger?**
Gleicher Chip, gleicher Compiler: Derselbe Code ist gleich schnell. Flüssiger wird es mit ESP-IDF, weil wir Dinge
einstellen können, die Arduino fest vorgibt (IRAM-sichere Display-Interrupts, PSRAM-Spiegel, Cache, Pufferplatzierung,
Rollback, Core-Dump). ESP-IDF bietet alles, was Arduino kann, plus vollen Zugriff. Nachteil: mehr Schreibarbeit und
weniger fertige Bibliotheken. Beides trägt Claude, der User muss nichts selbst bauen. Details: 4.1.

**PC-Simulator – was muss der User tun?**
Nichts. Claude richtet alles ein. Der User öffnet später einfach einen Link (Browser-Version auf GitHub Pages) oder lädt
eine `.exe` herunter. Details: 4.4.

**Ist GitHub das richtige Vorgehen?**
Ja, ausdrücklich. Versionsgeschichte und Rückgängigmachen, reproduzierbare Builds, automatische Releases für OTA,
GitHub Pages für Web-Flasher und Simulator, Arbeit in der Cloud. Ergänzt um: Pull Requests mit Simulator-Screenshots,
optional Issues als Wunschliste, lokale Schnellschleife für Hardware-Arbeit. Details: 4.13 und 11.

**Ist „Rückfall mit Selbsttest“ ein A/B-Update?**
Ja. Zwei App-Partitionen; das Update landet in der jeweils anderen. Zusätzlich muss sich die neue Version im Probelauf
gesund melden, sonst startet automatisch wieder die alte. Details: 4.10.

**Gemeinsames Universum – machbar? Nachteile?**
Gut machbar, weil die Welten nur über Ereignisse lose verbunden sind. Jede Welt bleibt eigenständig, abschaltbar und mit
eigener Perspektive, Palette, Mechanik und eigenem Glanzstück. Nachteil: etwas mehr Planung (Stil-Leitfaden,
Ereignis-Katalog). Dafür deutlich mehr Leben. Details: 4.6.

**Ist Pixel-Art noch das Richtige? Welche Stile gibt es?**
Pixel-Art im 16-Bit-Stil ist für diese Hardware die beste Wahl. Alternativen und Bewertung: 4.7.

**Was sind „Grafikpakete von Kenney“?**
Kenney ist ein niederländisches Studio, das tausende Spielgrafiken kostenlos unter CC0 veröffentlicht (gemeinfrei, jede
Nutzung erlaubt). Ähnlich: Ninja Adventure, OpenGameArt (nur CC0/CC-BY). Wir nutzen sie als Basis und färben sie auf
unsere Paletten um. Details: 4.7.

## 14. Entscheidungsprotokoll

| Datum | Entscheidung | Grund |
|---|---|---|
| 2026-10-18 | Projektname **Pixelia** | Wunsch des Users |
| 2026-10-18 | ESP-IDF + C++20 statt Arduino | volle Kontrolle über Speicher, Interrupts, Cache, Rollback (4.1) |
| 2026-10-18 | Rendering 400×240, 8 Bit Palette, 2× im Bounce-Buffer | Leistung, Palette-Effekte, flackerfrei bei OTA (4.2) |
| 2026-10-18 | PC-/Browser-Simulator | Entwicklung ohne Hardware, Screenshots im Pull Request (4.4) |
| 2026-10-18 | Gemeinsames Universum, lose gekoppelt, Dorf als Herz | mehr Leben bei voller Eigenständigkeit der Welten (4.6) |
| 2026-10-18 | Pixel-Art 16-Bit-Stil, Quellen: prozedural + CC0 + eigene | passt zur Hardware, Lizenzsicherheit im öffentlichen Repo (4.7) |
| 2026-10-18 | Svelte-Weboberfläche, eingebettet, offline, eigene M3E-Komponenten | `@material/web` ohne Expressive und nicht mehr gepflegt (4.8) |
| 2026-10-18 | A/B-OTA mit Selbsttest, Updates von GitHub-Releases (Stabil/Vorschau) | Sicherheit bei Updates, bequem (4.10) |
| 2026-10-18 | Versionsschema 0.MINOR.PATCH: MINOR = Phase/Welt, PATCH = Iteration | Wunsch des Users (4.11) |
| 2026-10-18 | `CHANGELOG.json` als einzige Quelle, Version aus Git-Tag | keine Doppelpflege, keine vergessenen Versionsnummern (4.11) |
| 2026-10-18 | 5 WLANs, mDNS `pixelia.local`, PIN optional (aus) | Komfort, Wunsch des Users (4.12) |
| 2026-10-18 | Kein Secure Boot, kein HTTPS auf dem Gerät | Risiko/Nutzen im Heimnetz (4.12) |
| 2026-10-18 | Öffentliches GitHub-Repo `Baldesarra/pixelia`, vom User bestätigt; vorerst keine Lizenz (alle Rechte beim User) | Updates ohne Token, kostenlose GitHub Pages (4.13) |
| 2026-10-18 | Kein Sound | kein Lautsprecher vorhanden |
| 2026-10-18 | Reihenfolge Zug → Dorf → Held → Liga, Dorf als Schwerpunkt | Zug = härtester Leistungstest bei einfacher Logik; Wunsch des Users |
