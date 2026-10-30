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
Schreibtisch. Man muss nicht aktiv spielen, aber jedes Mal, wenn man hinschaut, passiert etwas: Greta legt frische
Laibe in die Auslage, Taylor treibt die Schafe heim, im Lokschuppen klopft Emil Rost von der alten Lok, und über dem
Bahnhof zieht ein Gewitter auf. (Beispiele seit dem gemeinsamen Setting, 2026-10-21; vorher Held, Dorfverein und
Zugfenster.)

Leitlinien:

- **Lebendig statt statisch.** Ständig kleine Bewegungen, regelmäßig Ereignisse, selten echte Überraschungen.
- **Idle zuerst.** Alles läuft von allein. Eingreifen ist möglich (Weboberfläche, Touch), aber nie nötig.
- **Liebe zum Detail.** Lieber eine Welt richtig gut als vier halb.
- **Flüssig.** Ziel 30 fps, Minimum 25 fps, ohne Ruckler.
- **Modular.** Neue Orte, Gebiete und Funktionen kommen per Update dazu, ohne den Rest umzubauen.
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
| **Nicht vorhanden** | Lichtsensor (LDR), RGB-LED | anders als beim CYD |
| Ton | Lautsprecher-Anschluss „Speak“ (P6) mit Verstärker U4 | beim Prüfen in 0.1.0 auf der Platine gefunden, noch ungetestet |

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

> **Seit 2026-10-21 (gemeinsames Setting):** Module sind **Orte und Gebiete** einer gemeinsamen Welt, keine eigenen
> Welten mehr (4.6, Kapitel 6). Die Tabelle unten ist der Arbeitsentwurf aus der Zeit getrennter Welten; die
> Schnittstelle für Orte und Gebiete entsteht in 0.5.8 nach [SETTING.md](SETTING.md) Kapitel 15. Statt
> `backgroundTick()` und `catchUp()` je Welt gibt es Stufen der Simulation für die ganze Welt (voll, grob, Nachholen).
> Statt „Hintergrund- und Live-Simulation liefern dieselben Ergebnisse“ gilt „gleiche Wirkung, andere Darstellung“:
> Waren, Bedürfnisse und Erinnerungen buchen in beiden Stufen identisch, die Bewegung nicht.

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

**Entscheidung (2026-10-18): Ja – lose gekoppelt.** Pixelia ist das Land, in dem alle Welten spielen. **Das Dorf ist
das Herz.**

**Neu gefasst am 2026-10-21 (gemeinsames Setting):** Pixelia ist **eine** Welt um 1850, das Dorf bleibt das Herz.
Statt getrennter Welten gibt es Orte und Gebiete, die über Kapitel und Updates dazukommen ([SETTING.md](SETTING.md)
Kapitel 4 und 15). Die Regeln unten ersetzen die früheren.

Frühere Beispiele (getrennte Welten, bis 2026-10-20):

- Der **Held wohnt im Dorf**. Ist er zu Hause, sieht man ihn im Dorf beim Händler, beim Schmied, in der Taverne. Seine
  Beute liegt am Marktstand aus.
- Der **Dorfverein „FC Pixelia“ ist dein Team** in der Liga. Am Heimspieltag strömen die Bewohner zum Sportplatz, nach
  einem Sieg wird auf der Terrasse gefeiert, Kinder kicken in Trikots.
- Der **Zug hält am Dorfbahnhof**. Die anderen Liga-Vereine kommen aus den Städten, an denen der Zug hält. Im Dorf sieht
  man den Zug ein- und ausfahren, manchmal steigen neue Bewohner oder ein Wanderzirkus aus.
- **Gemeinsamer Kalender und gemeinsames Wetter:** Gewitter im Dorf = Gewitter vor dem Zugfenster.
- **Kurzmeldungen** über Weltgrenzen hinweg („TOR für den FC Pixelia!“, während das Zugfenster läuft).

Regeln seit dem gemeinsamen Setting (2026-10-21; ersetzen „jede Welt funktioniert allein, Verbindungen nur über den
Ereignis-Bus, eigene Perspektive je Welt“):

- **Eine Welt, ein gemeinsamer Weltzustand:** Uhr, Wetter, Figuren, Warenbuch, Chronik, Verbindungen, Aufgaben und
  Projekte gehören der Welt und werden als Ganzes gespeichert.
- **Orte und Gebiete sind Module:** Ein Ort ist ein Bild mit Hintergrund, Baukasten-Einrichtung, Wegen,
  Benutzungspunkten, Tätigkeiten, Ereignissen und Ortszustand; ein Gebiet (Bahnhof, Dorf, Grube) ist eine Gruppe von
  Orten. Neues Gebiet = neuer Ordner, Daten, eine Zeile Registrierung; alte Speicherstände laden weiter.
- **Ein Ort greift nie direkt auf einen anderen zu.** Verbindungen laufen über den Weltzustand (eine Figur oder Ware
  wird in einem Schritt übergeben, nie kopiert) und über den Ereignis-Bus. Ein fehlerhaftes Ortsmodul darf Weltzustand
  und Speicherstand nicht beschädigen.
- **Eine Perspektive (3/4-Draufsicht) und ein Stil-Leitfaden für alle Orte**, Palette je Ort. Ein technisches
  Glanzstück hat jedes Gebiet.
- Der Schalter „Welten verbinden“ entfällt.

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

- **MINOR = Phase**, ab 0.6.x je ein Kapitel der Welt, z. B. 0.6.x = Kapitel 1 „Der erste Frühling“ (bis
  2026-10-21: je eine Welt, z. B. 0.6.x = Zugfenster).
- **PATCH = jede Iteration** innerhalb der Phase: 0.6.0 erste Version von Kapitel 1, 0.6.1 zweite, 0.6.2 dritte …
  bis zur nächsten Phase (0.7.0).
- Vor 1.0.0 sind alle Versionen Vorabversionen. 1.0.0 = alle Kapitel fertig und verbunden, Dauertest bestanden.
- Nach 1.0.0 (Vorschlag): neue Region oder großes Feature = neue MINOR, Iterationen = PATCH.
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

> **Erweitert am 18.10.2026** auf Wunsch des Users (ausführliche, technische Changelogs im Stil der
> PR-Beschreibungen, von 0.1.0 an): zusätzliche Felder `g` (Geprüft, mit Messwerten), `h` (Hinweise) und `pr`
> (Pull-Request-Nummern). Jeder PR trägt seine Änderungen sofort ein, auch Fixes und während der Entwicklung behobene
> Probleme. Der oberste Eintrag darf bis zum Release ohne Datum, Titel und Untertitel sein.
> `tools/release_notes.py --check` prüft das Format in der CI.

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
| **Ereignis-Bus** | Orte und Systeme melden Ereignisse; Chronik, Regie, Kurzmeldungen und Weboberfläche hören zu (bis 2026-10-21: Welten untereinander) |
| **Kurzmeldungen** | Kleine Einblendungen auf dem Display (abschaltbar, pro Welt filterbar) und in der Oberfläche |
| **Regie** | Wiedergabeliste: Rotation nach Zeit, Zeitplan (z. B. abends Zugfenster), feste Welt; Live-Unterbrechung bei Spielen des FC Pixelia; Kamera-Regie innerhalb der Welten |
| **Nachtmodus** | Zeitplan oder Sonnenstand: Display dimmen, Nachtszene oder aus; Touch weckt |
| **Helligkeit** | manuell, nach Sonnenstand oder (mit Nachrüst-Sensor) nach Umgebungslicht |
| **Touch** | Gesten (Tippen, Wischen, langes Drücken), alles in der Oberfläche einstellbar: an/aus, welche Gesten was tun, Info-Karten an/aus, Empfindlichkeit, Sperre |
| **Speichern** | Spielstände atomar in LittleFS, Einstellungen in NVS, Export/Import aller Einstellungen als Datei |
| **Diagnose** | Leistungszähler (FPS, Frame-Zeit pro Stufe, CPU-Last pro Kern), Live-Log, Absturzberichte |

## 6. Die Welt und ihre Orte

> **Neu gefasst am 2026-10-21 (gemeinsames Setting, Neuplanung):** Pixelia ist **eine** Welt um 1850 mit dem Dorf
> Pixelia als Herz. Wer dort lebt und was passiert, steht in [SETTING.md](SETTING.md) (Figuren, Gebiete, Ereignisse,
> Bauen) und [WIRTSCHAFT.md](WIRTSCHAFT.md). Die Welt wächst in Kapiteln, je Kapitel eine Phase:
>
> | Phase | Kapitel | Gebiete |
> |---|---|---|
> | 0.6.x | 1 „Der erste Frühling“ | Bahnhof mit gestrandetem Zug, Umgebung (Waldrand, Wald, See, Wiesen, Lichtung) |
> | 0.7.x | 2 „Das Dorf wächst“ (Schwerpunkt) | Dorf Pixelia, Bauernhof, Sykoras Hof, Museum |
> | 0.8.x | 3 „Die alte Grube“ | Grube, Berghang mit Steinbruch |
> | 0.9.x | 4 „Die Lok“ | Lokschuppen, Strecke, Abteil im fahrenden Zug |
> | nach 1.0 | Updates | neue Regionen mit dem Zug ([ZUSATZINHALTE.md](ZUSATZINHALTE.md)) |
>
> Orte und Gebiete sind Module (4.5, 4.6); Fahrplan und Systemplan stehen in [ROADMAP.md](ROADMAP.md). Die Abschnitte
> 6.1 bis 6.5 sind die **früheren Welt-Konzepte** aus der Zeit getrennter Welten (bis 2026-10-20). Sie bleiben als
> Ideenquelle stehen; was davon weiterlebt, steht in ROADMAP „Frühere Welt-Ideen“. Die Prinzipien in 6.0 gelten weiter.

### 6.0 Prinzipien gegen Langeweile

> Verbindlich ergänzt durch die [Lebendigkeits-Charta](LEBENDIGKEIT.md): Gestaltungsregeln, messbare Mindestziele pro
> Welt und Nachweis per Langzeit-Test im Simulator.

- **Drei Zeitebenen:**
  - *Sekunden:* ständige Mikro-Animationen (Rauch, Vögel, blinzelnde Figuren, wehende Fahnen, Blätter, Glühwürmchen)
  - *Stunden:* Tagesablauf, Wetterwechsel, Ereignisse
  - *Wochen:* Fortschritt (Dorf wächst, Grube wird tiefer, Lok wird fertig, Jahreszeiten)
- **Ereignisse in Seltenheitsstufen:** häufig (mehrmals pro Stunde), gelegentlich (täglich), selten (wöchentlich),
  legendär (alle paar Monate). Man entdeckt auch nach Wochen noch Neues.
- **Kamera-Regie:** Die Kamera schwenkt von selbst zu interessanten Momenten.
- **Ausdrucksstarke Kleinigkeiten:** Sprechblasen-Icons (♥, Zzz, ♪, 🍞, !, ?), Gedanken auf Info-Karten.
- **Echtzeitbezug:** Die Tageszeit ist die echte Uhrzeit. Jahreszeiten laufen gestaucht (7 echte Tage), das Wetter
  wird passend dazu simuliert ([SETTING.md](SETTING.md) Kapitel 9; bis 2026-10-21: alles wie in der echten Welt).
- **Jedes Gebiet hat ein technisches Glanzstück**, das es unverwechselbar macht (bis 2026-10-21: jede Welt).

### 6.1 Früheres Konzept: Zugfenster – „Der ruhige Ausgleich“

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

### 6.2 Früheres Konzept: Pixel-Dorf – „Das Herz von Pixelia“

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

**Aufbau-Wirtschaft und Gedächtnis** (ergänzt am 18.10.2026, Wunsch des Users, Ideen aus der Inselwelt):

- Das Dorf baut sich selbst auf. Rohstoffe wie Holz, Stein, Lehm, Nahrung und Werkzeug sind begrenzt und werden von
  den Bewohnern selbst gewonnen, transportiert und gelagert (Holzfäller, Steinbruch, Felder, Werkstätten).
- Bauprojekte verbrauchen Material exakt. Fehlt Material, ruht die Baustelle sichtbar, bis Nachschub kommt.
- **Neue Bewohner ziehen nur ein, wenn Wohnraum da ist:** Wachstum heißt, Häuser zu bauen.
- Lager haben eine Kapazität. Bestände sind im Bild sichtbar (Holzstapel, Säcke, Kisten) und in der Weboberfläche genau
  einsehbar.
- Zuverlässiges Gedächtnis: Bestände, angefangene Bauprojekte, Besuche und Ereignisse bleiben exakt erhalten, auch über
  Neustarts ([LEBENDIGKEIT.md](LEBENDIGKEIT.md), Regel 10).
- Kein Pflichtspiel: Das Dorf gerät nicht in Not, nur weil niemand hinschaut oder das Gerät aus war.
- Vor dem Bau gibt es **mehrere Darstellungs-Vorschläge** als Simulator-Mockups (siehe Perspektive oben).

**Kamera:** Rundgang (gleitet langsam durchs Dorf), Ereignis-Fokus (schwenkt zu Hochzeit, Brand, Fest), „Folge
Bewohner X“, fest.

**Touch:** Bewohner antippen → Info-Karte (Name, Alter, Beruf, Laune, was er/sie gerade tut, aktueller Gedanke wie
„Hoffentlich gibt es noch Zimtschnecken“). Gebäude antippen → wer ist drin, was passiert dort.

**Weboberfläche:** Bewohnerliste mit Steckbriefen und Beziehungen, **Dorfchronik** (Tagebuch aller Ereignisse),
Statistiken, Aktionen: Sturm/Gewitter, Schnee, Feuerwerk, Fest ausrufen, Regenbogen, neue Familie einladen, Bewohner
umbenennen, **eigenen Bewohner erstellen**, geheime Aktion (UFO).

### 6.3 Früheres Konzept: Dungeon-Held – „Das Abenteuer im Hintergrund“

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

### 6.4 Früheres Konzept: Pixel-Liga – „Mitfiebern“

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

### 6.5 Früheres Konzept: Inselwelt – „Ein Zuhause, das eine Geschichte bekommt“

Aufgenommen am 18.10.2026 auf Wunsch des Users. Rohkonzept: [welten/inselwelt-konzept.md](welten/inselwelt-konzept.md).

- **Eigenständige, ruhige und gemütliche („cozy“) Welt.** Gemütlichkeit ist ausdrücklich wichtig. Sie gehört nicht zum
  Pixelia-Universum, Verbindungen höchstens später und optional.
- Ein Bewohner baut Haus, Garten und Boot, später Hühner- und Kuhstall und einen Hofladen. Dazu eine Katze, die
  zutraulich wird, und wenige wiederkehrende Bekanntschaften. Natur, Handwerk, Alltag, keine Magie, keine Pflichten für
  den Betrachter.
- **Zuverlässiges Gedächtnis ist der Kern:** Jede Ware hat einen Bestand und einen Standort. Rezepte und Bauten
  verbrauchen exakt, angefangene Arbeiten gehen am selben Stand weiter, Besuche und Erinnerungen bleiben. Nichts
  verdoppelt sich oder verschwindet, auch nicht bei einem Neustart.
- Das Rohkonzept ist für 320×240 Pixel (2,8") geschrieben. Unser logisches Bild ist mit 400×240 gleich hoch und breiter,
  die Nahansichten passen also gut. Trotzdem wird die Darstellung für 5" neu geplant, mit mehreren Vorschlägen.
- Vor der Umsetzung: gemeinsame Überarbeitung des Konzepts, eigene Mindestziele in LEBENDIGKEIT.md, Ideenspeicher.
- **Offen:** Ob Inselwelt, Pixel-Dorf oder Dungeon-Held der Schwerpunkt der Software wird, entscheiden wir gemeinsam vor
  Beginn von 0.6.0. Danach richtet sich auch die Position der Inselwelt in der Roadmap.

### 6.6 Spätere Ideen (nicht eingeplant)

Weitere Welten als Module: Aquarium, Raumstation, Ameisenfarm, Leuchtturm am Meer, Bauernhof, Wetterstation-Ansicht,
Uhr/Kalender-Overlay. Sammelt sich in [ROADMAP.md](ROADMAP.md) im Backlog. Einordnung seit 2026-10-21: ROADMAP
„Frühere Welt-Ideen“ und [ZUSATZINHALTE.md](ZUSATZINHALTE.md).

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
| Hochskalier-Interrupt | ≤ 20 % eines Kerns (anfangs 15 %, nach Messung angepasst, siehe Kapitel 14) |
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
├─ pages/                 GitHub Pages: Web-Flasher, später Browser-Simulator (ab 0.1.0)
├─ tools/                 Asset-Pipeline, Changelog-Generator, Hilfsskripte (Python)
└─ .github/workflows/     Builds, Tests, Releases, GitHub Pages
```

## 11. Entwicklungsumgebung und Arbeitsweise

**Auf dem PC des Users (einmalig, Claude unterstützt Schritt für Schritt):**

- Git für Windows, GitHub CLI (Anmeldung macht der User selbst im Browser)
- ESP-IDF (für schnelles Flashen per USB und serielle Logs in Phase 0. Danach läuft alles über OTA)
- Node.js (Weboberfläche lokal bauen)
- Compiler für den Simulator (optional, Builds laufen auch auf GitHub)

Stand 18.10.2026: alles installiert. Git, GitHub CLI, ESP-IDF v6.1, Node.js 24 LTS, CMake, Ninja, Python 3.13,
LLVM-MinGW. ESP-IDF baut auch im Projektpfad mit Leerzeichen (`D:\VS Code\ESP32-S3`).

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
- ~~Genaue Panel-Timings und Pixeltakt (0.1.x)~~ erledigt: 24,6 MHz, 60 Hz, siehe `docs/HARDWARE.md`
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
| 2026-10-18 | **ESP-IDF v6.1** (neueste stabile Version, August 2026) | längste Unterstützungsdauer, keine Altlasten im neuen Projekt; Wechsel auf v6.0.x/v5.5.x möglich, falls Komponenten fehlen |
| 2026-10-18 | Simulator-Compiler auf Windows: LLVM-MinGW, SDL per CMake FetchContent | keine schwere Visual-Studio-Installation, gleicher Build-Weg wie in CI |
| 2026-10-18 | Reihenfolge Zug → Dorf → Held → Liga, Dorf als Schwerpunkt | Zug = härtester Leistungstest bei einfacher Logik; Wunsch des Users |
| 2026-10-18 | **Plan A bestätigt:** RGB-Panel ohne Bildspeicher, Palette + 2× Hochskalieren im Bounce-Buffer-Callback, Doppelpuffer mit Tausch am Bildende. Plan B (Bildspeicher im PSRAM) entfällt | Messung am Gerät: 30 fps stabil, Zeichnen 4,1 ms, Interrupt 20 %, 0 Aussetzer, auch während Flash-Schreiben; ESP-Seite schafft bis 40 MHz Pixeltakt (HARDWARE.md) |
| 2026-10-18 | Panel mit **60 Hz** (24,6 MHz Pixeltakt), jedes Engine-Bild wird zweimal gezeigt = 30 fps. Logik-Takt aus dem Bildzähler des Panels: ein Schritt pro Engine-Bild (≈ 30 Hz), nach verpasstem Bild zwei, höchstens vier | gleichmäßige Bewegung ohne Verhaken zweier Uhren; bei 39 Hz käme es zu ungleichmäßigen Bildfolgen, 30 Hz am Panel riskiert Flimmern (4.2). Uhrzeit-Abhängiges rechnet mit Echtzeit |
| 2026-10-18 | Hintergrundbeleuchtung mit **200 Hz** PWM (12 Bit) | Sichtprüfung: 200 Hz dimmt sauber und still; 500 Hz–1 kHz pfeifen, ab 2 kHz geht das Display aus (LED-Wandler fährt nicht hoch) |
| 2026-10-18 | Engine-Puffer (2 × 96 KB) im **PSRAM** statt im internen RAM, umschaltbar | 303 statt 115 KB interner RAM frei, Reserve für WLAN/TLS/Webserver; Kosten: Zeichnen 4,1 statt 2,1 ms, Interrupt 20 statt 17 %; flackerfrei beim Flash-Schreiben, weil XIP aus PSRAM den Cache aktiv lässt (4.2, 4.3) |
| 2026-10-18 | Budget Hochskalier-Interrupt **≤ 20 %** eines Kerns statt ≤ 15 % (Kapitel 9) | gemessen 20 % bei 60 Hz; Kern 1 ist mit Zeichnen insgesamt nur ~33 % ausgelastet |
| 2026-10-18 | Diagnose-Schnittstelle (serielle Befehle) und Leistungsmonitor fest in der Firmware | messen und testen ohne Neu-Flashen; Idee des Users (Leistungs-Log), später in der Weboberfläche |
| 2026-10-18 | Bilddrehung 0°/180° als Einstellung, Standard 0° | Aufstellort steht noch nicht fest (User); kostet keine Rechenzeit |
| 2026-10-18 | Touch per Polling alle 20 ms, ohne INT-Leitung | INT-Pin nicht nötig, GT911 liefert zuverlässig |
| 2026-10-18 | „Kein Sound“ wird neu bewertet | Platine hat einen Lautsprecher-Anschluss „Speak“ mit Verstärker (Foto), Test steht aus (Backlog) |
| 2026-10-18 | **Lebendigkeits-Charta** ([LEBENDIGKEIT.md](LEBENDIGKEIT.md)) verbindlich: Systeme statt Schleifen, Ereignis-Regisseur, messbare Mindestziele pro Welt, Langzeit-Test mit Kontaktbogen in jedem Welt-PR, Zahlen in jedem Release-Bericht | ausdrücklicher Wunsch des Users: Tiefe und echtes Leben statt einfacher Szenen in Dauerschleife; „lebendig“ muss überprüfbar sein, nicht nur versprochen |
| 2026-10-18 | Szenen-Schnittstelle `pixelia::Scene` und **Weltzeit mit Zeitraffer** in `engine/`; Gerät und Simulator nutzen denselben Szenen-Code | eine Codebasis für Gerät, Simulator und Langzeit-Test; feste Uhrzeit und Zeitraffer machen Tageszeit-Verhalten prüfbar (4.4, LEBENDIGKEIT.md) |
| 2026-10-18 | Simulator mit **SDL3** (3.4.16, per FetchContent, SHA-256 geprüft) statt SDL2; Kopflos-Modus ohne SDL | SDL3 ist die aktuelle Hauptversion; kopflos braucht CI keine Grafikbibliotheken |
| 2026-10-18 | Eigener kleiner **PNG-Encoder** in `engine/` statt Fremdbibliothek | Screenshots im Simulator und später auf dem Gerät (Live-Vorschau) mit demselben Code, keine Lizenzfragen |
| 2026-10-18 | Web-Flasher mit **ESP Web Tools** auf GitHub Pages, eigener Ordner `pages/`; Pages-Workflow läuft nach jedem Release vom Hauptzweig | Erstinstallation ohne Werkzeuge; die Pages-Umgebung erlaubt Veröffentlichungen nur vom Hauptzweig, nicht von Tags |
| 2026-10-18 | Einfacher Release-Weg für 0.1.x: Tag → Workflow baut, erzeugt Factory-Image (`idf.py merge-bin`) und Release-Notes aus `CHANGELOG.json` | reicht bis 0.4.x; Release bricht ab, wenn der Changelog-Eintrag fehlt |
| 2026-10-18 | Setup-WLAN „Pixelia-Setup“ mit **WPA2 und zufälligem Passwort** (einmal erzeugt, im NVS, steht auf dem Display und im QR-Code) statt offen | Das Heim-WLAN-Passwort wird darüber übertragen; nur wer das Display sieht, kann sich verbinden. Der QR-Code macht das Abtippen überflüssig |
| 2026-10-18 | Captive Portal mit eigenem Mini-DNS (nur auf 192.168.4.1), Weiterleitung der Prüfadressen von Android, iOS, Windows, Firefox und DHCP-Option 114 | Einrichtungsseite öffnet sich von selbst (am Android-Handy des Users bestätigt) |
| 2026-10-18 | **Systemschrift, QR-Code-Erzeuger und Systemanzeigen in `engine/`** (eigener Code, keine Fremdbibliothek); Palettenindizes **240–255 gehören der System-Oberfläche** | Gerät und Simulator zeigen dieselben Bilder; Kurzmeldungen passen über jede Welt. QR-Code gegen zxing-cpp und eine unabhängige Reed-Solomon-Bibliothek geprüft |
| 2026-10-18 | Systemanzeigen werden nur bei Änderung neu gezeichnet (Schlüssel pro Puffer) | statisch: 0,02 statt 9,5 ms pro Bild, weniger PSRAM-Last |
| 2026-10-18 | WLAN-Ausweichmodus: bei Problemen im Schlüsselaustausch zweiter Versuch ohne WPA3-Kompatibilitätsmodus (RSN Override), Erfolg pro Netz im NVS | FritzBox im WPA2/WPA3-Mischmodus scheiterte mit ESP-IDF 6.1 immer mit Grund 17 |
| 2026-10-18 | Ohne Verbindung beim Start 45 s, nach Verlust 2 min schnelle Neuversuche, erst dann Setup-Modus; im Setup-Modus alle 2 min, direkt nach dem Eintragen zwei schnelle Versuche | Router ignorieren nach hartem Reset die ersten Anmeldungen; Stromausfall soll nicht sofort den Setup-Modus auslösen |
| 2026-10-18 | TCP-Empfangsfenster 32 KB, SACK, Puffer im PSRAM; Update-Zielbereich vorab in 64-KB-Blöcken löschen | Upload blieb bei −71 dBm mit 5,7-KB-Fenster stehen (User-Test); jetzt 160–230 KB/s, Update in 7 s statt 30 s |
| 2026-10-18 | Task-Watchdog löst einen Neustart aus (Panic, 10 s) | Ein Hänger im Probelauf führt so zum automatischen Rückfall; Normalbetrieb bleibt nicht hängen |
| 2026-10-18 | Selbsttest im Probelauf: mindestens 60 s Panel ≥ 50 Hz und Engine ≥ 20 fps, WLAN verbunden oder Setup-Modus; nach 120 s ohne Erfolg Rückfall. Grund und Absturzstelle werden für die alte Version im NVS gemerkt | konkretisiert Kapitel 4.10, alle drei Rückfall-Wege am Gerät getestet |
| 2026-10-18 | mDNS als Fremdkomponente `espressif/mdns` 1.14 (Component Registry, Version in `firmware/dependencies.lock`); eigener JSON-Schreiber statt cJSON | mDNS gehört seit ESP-IDF 5 nicht mehr zum Kern; cJSON fehlt in ESP-IDF 6.1, der Bedarf ist klein |
| 2026-10-18 | **Inselwelt** als weitere Welt eingeplant (cozy, eigenständig, 6.5), Position offen; Rohkonzept in `docs/welten/` | Wunsch des Users: viel Tiefe und Gemütlichkeit |
| 2026-10-18 | **Entscheidung über die Hauptwelt** (Inselwelt, Pixel-Dorf oder Dungeon-Held) gemeinsam vor Beginn 0.6.0; bis dahin bleibt das Dorf Schwerpunkt | Der User ist unsicher und will das planen und diskutieren |
| 2026-10-18 | **Zuverlässiges Gedächtnis** als Regel 10 der Lebendigkeits-Charta, mit Bilanz-Test | exakte Bestände, angefangene Arbeiten, Besuche (Wunsch des Users, vor allem für die Inselwelt, aber für alle Welten) |
| 2026-10-18 | Pixel-Dorf als **Aufbau-Wirtschaft** mit begrenzten Rohstoffen, exaktem Materialverbrauch und Wohnraum als Voraussetzung für Zuzug (6.2) | Wunsch des Users, Ideen aus der Inselwelt |
| 2026-10-18 | **Systemschrift aus 0.2.0 bleibt** Grundschrift der System-Oberfläche und Ausgangspunkt für den Stil-Leitfaden | gefällt dem User |
| 2026-10-18 | **Changelog erweitert** um `g` (Geprüft), `h` (Hinweise), `pr`; jeder PR trägt sofort ein; Format-Prüfung in der CI; 0.1.0 und 0.2.0 nachgetragen (4.11) | Wunsch des Users nach ausführlichen technischen Changelogs von 0.1.0 an, später im Versionsverlauf der Oberfläche |
| 2026-10-19 | **Log-Ringpuffer** (256 KB) im No-Init-Bereich des PSRAM, jede Zeile sofort aus dem Cache zurückgeschrieben; **RTC-Sicherung** (6 KB) nur im Probelauf zeilenweise und per Shutdown-Handler vor jedem Software-Neustart | Logs überstehen Neustarts, Updates und Abstürze. Die PSRAM-Lage hängt an der Codegröße (64-KB-Seiten), deshalb die RTC-Sicherung für Versionswechsel; der RTC-Speicher ist langsam (~0,3 µs pro Zugriff), dauerhaft zeilenweise kostete er 16 µs pro Zeile. Ergebnis 16–19 µs pro Zeile, Nachweis mit Absturz-Testvarianten (HARDWARE.md) |
| 2026-10-19 | **Fernwartung per WLAN:** `/api/log`, Live-Log `/ws/log`, `/api/cmd` (gleiche Befehle wie seriell, gefährliche nur mit `confirm=1`), `/api/perf`, `/api/resets`, `/api/coredump`, Diagnosepaket `/api/diag`; Werkzeuge `tools/netlog.ps1` und `tools/crash.ps1` | Wunsch des Users: im Notfall alles ohne USB, Diagnosepaket vom Handy verschicken. Ergänzt 4.10 (Absturzbericht) und 4.12; die Anzeige folgt in der Oberfläche (0.3.x/0.4.x) |
| 2026-10-19 | **Passwort-Filter** über alle Diagnose-Ausgaben mit freiem Text (gespeicherte WLAN-Passwörter und Setup-Passwort, roh und in JSON-Schreibweise); PIN-Prüfstelle für `/api/cmd` und Löschen schon vorgesehen | „WLAN-Passwörter verlassen das Gerät nie“ (4.12) gilt auch für Log und Diagnosepaket; der optionale PIN kommt mit 0.3.x |
| 2026-10-19 | Reset-Historie im NVS (16 Starts, ein Schlüssel pro Start), Startzähler im NVS, Lebenszeichen alle 2 s im RTC-Speicher | wenig NVS-Schreiblast pro Start; die Laufzeit vor einem Absturz ist ohne Flash-Schreiben bekannt |
| 2026-10-19 | **Task-Liste nur auf Wunsch**, nie regelmäßig abfragen | `uxTaskGetSystemState()` hält 1,1 ms die Kernel-Sperre, Tick- und Display-Interrupt auf Kern 1 warten, etwa ein Display-Aussetzer pro Abfrage (gemessen). Gilt auch für die Leistungsanzeige in 0.4.x |
| 2026-10-19 | **Oberfläche unter `/`, Einrichtungsseite bleibt** unter `/setup` und im Setup-Modus unter `/` | Captive-Portal-Fenster (Android/iOS) können wenig und haben kein Internet; die schlanke Seite (18 KB) funktioniert dort sicher. WLAN, Update und Einstellungen wandern bis 0.3.2/0.4.x in die Oberfläche |
| 2026-10-19 | **Icons als SVG-Pfade statt Icon-Schrift**, Hash-Routen (`#/…`), Dateien mit festen Namen und ETag (`no-cache`) | nur benutzte Icons im Bundle (je ~0,5 KB), kein Server-Routing nötig, nach Updates keine veralteten Dateien im Browser-Cache (4.8) |
| 2026-10-19 | **Roboto Flex nur mit den Achsen Stärke und Breite** und 135 Zeichen (33 KB) | ausdrucksstarke Typografie (M3 Expressive) braucht Stärke und Breite; opsz, slnt usw. würden die Datei auf über 100 KB bringen |
| 2026-10-19 | **Farbwahl der Oberfläche vorerst im Browser** (localStorage) | kommt mit den Geräte-Einstellungen (Export/Import, 0.3.2) aufs Gerät |
| 2026-10-19 | **Setup-Modus zeigt die neue Oberfläche** (Start auf der WLAN-Seite), /setup bleibt schlanke Notfallseite im M3-Look (ändert die Zeile oben) | am Gerät geprüft: Androids Anmeldefenster (WebView) lädt die Oberfläche (119 KB) und hält den WebSocket; eine Oberfläche statt zwei, /setup bleibt der Weg, falls das Bundle einmal kaputt ist |
| 2026-10-19 | **Verbundene Geräte über den Status-WebSocket**, bis 12 Geräte im NVS | kein eigener Socket nötig (7 frei), Online-Status kommt aus der offenen Sitzung; NVS-Schreiben nur bei neuem Gerät, Namen oder alle 10 min |
| 2026-10-19 | **PIN als gesalzener Hash** (SHA-256, 1000 Runden) im NVS „auth“, Sitzungs-Token nur im RAM, Skripte mit Kopf `X-Pixelia-Pin`; Sperre nach 5 Fehlversuchen (30 s, verdoppelnd bis 15 min, gilt für alle Absender) | der PIN steht nie im Klartext im Gerät; Token statt PIN im Browser, damit er nicht bei jeder Anfrage mitgeht; eine gemeinsame Sperre macht Raten aus dem Heimnetz aussichtslos (4 Ziffern: Jahre). Angemeldete Sitzungen bleiben während einer Sperre gültig |
| 2026-10-19 | **Schutzumfang:** ändernde Endpunkte brauchen den PIN, Lesen (Status, Log, Diagnosepaket, Sicherung) bleibt offen; `/api/wifi/add` im Setup-Modus ohne PIN | Lesen enthält nie Passwörter (Filter seit 0.2.2). Ins Setup-WLAN kommt nur, wer das Passwort auf dem Display sieht, also dieselbe Berechtigung wie die Touch-Geste; ein umgezogenes Gerät mit vergessenem PIN lässt sich so ohne Werksreset einrichten |
| 2026-10-19 | **Vergessener PIN nur am Gerät:** Info-Karte → „Zurücksetzen …“ → „PIN entfernen“ oder „Werksreset“, Knopf 3 s halten | wer das Display berühren kann, hat das Gerät ohnehin in der Hand; über das Netz gäbe ein Rücksetzweg jedem im WLAN den Schlüssel. Halten statt Tippen verhindert versehentliches Auslösen (auch durch Kinder) |
| 2026-10-19 | **Werksreset** löscht die NVS-Namensräume `pixelia`, `wifi`, `clients`, `auth`; Startzähler, Reset-Historie (`diag`) und Rückfall-Grund (`ota`) bleiben | Diagnose über den Reset hinweg; alles, was der Nutzer eingestellt hat, ist weg |
| 2026-10-19 | **Sicherung als JSON-Datei** vom Gerät (`/api/backup`), Import in der Oberfläche über die vorhandenen Endpunkte; ohne WLAN-Passwörter und ohne PIN (auch kein Hash), WLAN-Namen nur zur Erinnerung | kein JSON-Leser in der Firmware nötig; ein PIN-Hash von 4 Ziffern wäre in Sekunden geknackt |
| 2026-10-19 | **Farbwahl der Oberfläche auf dem Gerät** (Einstellung `ui`, ändert die Zeile „vorerst im Browser“): das Gerät ist die Quelle, alle Browser zeigen dieselben Farben; ohne Anmeldung gilt eine Änderung nur im Browser | eine Wahl für alle Geräte und in der Sicherung; der Browser startet trotzdem sofort mit der zuletzt gesehenen Wahl (localStorage) |
| 2026-10-20 | **Phase 0.4.x in drei Patches:** 0.4.0 Versionsverlauf und Diagnose, 0.4.1 Update von GitHub (Prüfung, Kanal, HTTPS), 0.4.2 Experimentell, Live-Vorschau, Leistung | eine Arbeits-Sitzung pro Patch bleibt unter der Kontext-Obergrenze; jeder Patch ist für sich am Gerät prüfbar |
| 2026-10-20 | **Changelog als Block mit festem Marker** in den Konstanten der Firmware: `PIXELIA:CHLOG:v1`, Länge, CRC-32, gzip-JSON (`tools/release_notes.py --embed`); `/api/changelog` liefert die gzip-Daten direkt | konstante Daten liegen am Stück im Flash-Segment der .bin, der Browser findet den Marker in einer gewählten Datei in Millisekunden und entpackt mit `DecompressionStream`. Eine feste Adresse wäre bei jeder Codeänderung eine andere, eine eigene Partition bräuchte ein neues Layout und ginge beim Update per .bin nicht mit. CRC schützt vor zufälligen Treffern, gzip spart 60 % (25 → 10 KB) |
| 2026-10-20 | **Download von GitHub abbrechen:** Knopf „Abbrechen“ während des Downloads; die OTA-Partition bleibt ungültig, kein Neustart | ein langsamer oder versehentlich gestarteter Download soll nicht bis zum Ende laufen müssen; die laufende Firmware bleibt unberührt |
| 2026-10-20 | **Prüfung im Hintergrund:** höchstens einmal täglich, abschaltbar auf der Seite System (Standard an); nur ein Hinweis in der Weboberfläche (Punkt an „System“), nie automatisch installieren. Hinweis auf dem Display erst mit den Overlays (0.5.x) | eine Abfrage pro Tag schont das Limit von 60 Abfragen pro Stunde und IP; installiert wird nur auf Wunsch |
| 2026-10-20 | **Ältere Version installieren (Downgrade)** erlaubt, nur auf der Seite Experimentell mit deutlicher Warnung, Auswahl aus den Releases ab 0.4.0 | hilft bei Fehlern in einer neuen Version; Einstellungen können verloren gehen und Versionen vor 0.4.1 können nicht selbst von GitHub updaten |
| 2026-10-20 | **Keine automatische Installation ohne Nachfrage**, auch nicht als Option (Entscheidung des Users, nachgetragen vom Planungs-Chat) | ein Hinweis auf neue Versionen reicht, installieren will der User selbst; ein Update unterbricht die Anzeige und kann Einstellungen betreffen |
| 2026-10-20 | **Live-Vorschau als Paletten-Bild:** `GET /api/screen` liefert 400×240 Indizes plus Palette, PackBits-gepackt (~68 KB); Kopie im Grafik-Task direkt vor dem Puffertausch, höchstens alle 200 ms; der Browser malt ins Canvas und erzeugt das PNG (800 × 480) selbst, ≤ 3 Bilder/s | kein Reißen, kein PNG-Encoder und kein Deflate in der Firmware (Speicher, Rechenzeit); gemessen 29,7–30,3 fps und RAM 115 KB während der Vorschau. Ein Deflate (tdefl) bräuchte über 300 KB Arbeitsspeicher |
| 2026-10-20 | **Probe für Panel-Werte:** neue Werte nur im RAM, ohne „Behalten“ nach 15 s zurück; ins NVS erst nach Bestätigung, ein Neustart während der Probe startet also mit den bestätigten Werten. Fluchtweg: zwei Starts mit eigenen Werten, die keine 60 s laufen, → nächster Start mit Standardwerten | wie beim Ändern der Bildschirmauflösung am PC; der Fluchtweg hilft, wenn bestätigte Werte nach einem Kaltstart doch schwarz bleiben und kein Browser zur Hand ist (Stecker zweimal ziehen) |
| 2026-10-20 | **Stromsparmodus = halber Pixeltakt** (Panel 30 Hz, jedes Engine-Bild einmal gezeigt), hinter Experimentell | Welt bleibt bei 30 fps, Hochskalier-Interrupt und PSRAM-Verkehr halbieren sich (gemessen 20 → 10 %); Helligkeit regelt schon die Seite System, Nachtmodus folgt in 0.5.x |
| 2026-10-20 | **Seite Live** (Vorschau und Leistung zusammen) und **Seite Experimentell** als eigene Ziele (Schiene, auf dem Handy unter „Mehr“); Experimentell aus = Panel auf Standardwerten, eigene Werte gelöscht; ältere Version auch serverseitig nur mit eingeschaltetem Experimentell | beim Beobachten der Vorschau sieht man gleich die fps; Experimentelles bleibt ohne Schalter ganz aus dem Weg |
| 2026-10-20 | **Zwischenstände in Alltagssprache und Mitentscheiden** (Regel des Users): nach jedem Teilschritt 1–3 Zeilen „Neu: …“, am Sitzungsende „Das ist neu für dich“ (≤ 6 Punkte); große Design-, UI- und Inhaltsfragen vor der Umsetzung als Auswahl (2–4 Optionen, Empfehlung zuerst, bei Aussehen mit Vorschau), gebündelt bis 4; Zusatzfunktionen als Mehrfachauswahl; Kleinigkeiten und Technik entscheidet Claude mit Begründung | der User will den Fortschritt ohne Fachbegriffe verfolgen und bei allem mitreden, was er sieht und bedient, ohne mit Technikfragen belastet zu werden |
| 2026-10-20 | **Rückblick-Runde Oberfläche:** Standardfarbe ab Werk „Lavendel“ (#7a5aa6) statt „Moos“, alle sechs Farben bleiben wählbar, gespeicherte Wahl bleibt; Farbstil bleibt „Tonal“; Handy-Leiste bleibt vorerst, neu ordnen mit den ersten Welt-Menüpunkten; neu: Versionskarte oben auf der Seite System, Pixel-Uhr als abschaltbares Overlay | Entscheidung des Users; die Version soll auf einen Blick sichtbar sein. Die gewünschte 180°-Drehung gibt es schon seit 0.2.0 (Seite System → Anzeige) |
| 2026-10-21 | **Phase 0.5.x in 11 Patches** (vom User bestätigt): Phase A settingunabhängig 0.5.0 Fundament, 0.5.1 Raum und Baukasten, 0.5.2 Farbe und Licht, 0.5.3 Wetter und Partikel, 0.5.4 Asset-Quelle (mit SD-Messung), 0.5.5 Schrift und Einblendungen, 0.5.6 Dienste, 0.5.7 Browser-Simulator und Langzeit-Test; Phase B nach dem Setting 0.5.8 Orte und Stil, 0.5.9 Gemeinsamer Weltzustand, 0.5.10 Regie und Nachweis. Kamerafahrten und Parallax entfallen als Hauptmittel | jede Sitzung bleibt unter ~250 K Kontext und ist für sich am Gerät messbar; die Perspektive (ein Bildschirm pro Ort, kein Scrollen) steht schon, das Setting noch nicht, darum kommt alles Settingabhängige zuletzt |
| 2026-10-21 | **Szene im groben Raster 200 × 120**, beim Zusammensetzen 2× in den 400 × 240-Puffer; Einblendungen und Text fein in 400 × 240. Arbeitspuffer und Grafiken im PSRAM | ein Viertel der Pixel pro Bild, Platz für viele Figuren; entspricht der gewählten Perspektive (4 × 4 Displaypixel je grobem Bildpunkt). Gemessen: Puffer intern 3,7 ms, im PSRAM 4,2 ms pro Bild, intern kostet aber 24 KB internen RAM, den ein Update über HTTPS braucht |
| 2026-10-21 | **Speicher-Stufen für Grafiken** (Frage des Users, Ausbau in 0.5.4): (1) Ablage: in der Firmware eingebettet (Flash, kostet keinen RAM), LittleFS (3,8 MB) oder microSD; (2) Grafik-Speicher im PSRAM (6 MB frei) mit Budget: Grundausstattung immer geladen (Figuren der Bewohner, gemeinsame Baukasten-Teile, Farbreihen), Ortspakete beim Betreten geladen, Nachbarorte vorab, selten besuchte verdrängt; (3) gezeichnet wird nur aus PSRAM oder eingebettetem Flash, nie direkt von der Karte. Interner RAM nur für kleine Puffer, Stapel und Netzwerk (Ziel ≥ 60 KB frei, auch beim Update über HTTPS). Unsichtbare Orte rechnen ohne Grafik. Inhalte über Kennungen, nicht über Zeiger | der interne RAM (334 KB, davon ~100 KB frei) ist der knappste Speicher, PSRAM hat 6 MB frei; die Ablage kann so wachsen (Flash → Karte), ohne dass sich Zeichnen oder Speicherstand ändern; der Baukasten (Räume als Datenlisten, Farbreihen) hält die Grafikmenge pro Ort klein |
| 2026-10-21 | **Ortswechsel standardmäßig mit Abblenden** über Schwarz (0,6 s); Überblendung, Raster und Schnitt bleiben wählbar (Entscheidung des Users nach Bildfolgen im Simulator) | ruhig wie ein Filmschnitt; braucht keine gemeinsame Palette beider Orte |
| 2026-10-21 | **Raum-Baukasten mit Hintergrundbild** (0.5.1): Ein Ort ist eine Datenliste aus Kacheln (drei Ebenen, Textzeilen mit Legende) und Teilen (Lage, Stufe, Farbreihe, Besitzer, Flags). Beim Betreten wird alles Unbewegte einmal vorgezeichnet (24 KB PSRAM), beim Verlassen freigegeben; eingebackene Möbel werden nur über bewegten Teilen neu gezeichnet. Änderungen der Raumliste zeichnen sofort neu | pro Bild nur Kopie plus bewegte Teile (gemessen: Zeichnen 4,1–4,2 ms wie vorher, mit 64 Figuren 6,8 ms); pixelgleich mit „alles zeichnen“ (Test); höchstens zwei Hintergrundbilder gleichzeitig (während eines Übergangs) |
| 2026-10-21 | **Zustand „im Bau“** als Engine-Baustein (Wunsch des Users): Teil zeigt sein Bau-Bild (z. B. halbfertiges Bett) oder einen Materialstapel, Stellfläche bleibt belegt, benutzbar sind nur Arbeitsplätze | sichtbar machen, dass etwas entsteht; passt zu Ausbaustufen und späteren Bau-Aufgaben der Figuren |
| 2026-10-21 | **Test-Grafiken der Testorte im Stil „Formen“** (einfache, erkennbare Möbel; nur Seite Experimentell, ersetzt durch die echten Grafiken nach Setting und Stil-Leitfaden); **Ausgänge am Bildrand nicht markiert**, nur die Prüfansicht zeigt sie; Karte „Orte“ mit Ausbau, Im Bau, Blick folgt, Prüfansicht (Entscheidungen des Users) | Ausgänge erkennt man am Ort selbst (Weg, Tür, Lücke), die Welt soll nicht wie ein Spiel wirken |
| 2026-10-21 | **Palette je Ort mit drei Lichtstufen** (0.5.2): 80 Grundfarben, Stufe 1 und 2 (Lampenlicht) rechnet die Engine; Licht = Index verschieben. Die Lichtkarte (Stufen der vier feinen Bildpunkte je grobem) wird beim Verdoppeln angewendet, Figuren bekommen gleichmäßig die Stufe am Fußpunkt; tagsüber (Lampenwirkung 0) wird nichts gerechnet. 240–255 bleiben der System-Oberfläche | echtes Licht in der Dunkelheit (warm, heller als die Umgebung) ohne Rechnen pro Bildpunkt; feines Raster (2 × 2 Displaypunkte) wirkt weich; Überblendung nachts meist nicht möglich (zu viele Farben), dann Abblenden (Standard) |
| 2026-10-21 | **Stil: warm und gemütlich, Tageszeiten natürlich, Nacht mittel (Mindesthelligkeit 85/256), Lichtkreis hell bis zur Hälfte und dann weich im feinen Raster** (Entscheidungen des Users nach Vergleichsbildern im Simulator; ganz ohne Raster geht es mit drei Helligkeiten je Farbe nicht). Entwurf in `docs/STIL.md` | passt zu Ton und Epoche (um 1850, gemütlich); Figuren bleiben nachts erkennbar, Einblendungen immer lesbar |
| 2026-10-21 | **Tageszeit-Schnittstelle über WorldClock**, bis 0.5.6 gefüttert von der Testuhr (echte Uhrzeit, Schieber, Zeitraffer auf Experimentell → Orte). Die Dunkelheit der Palette ist getrennt von der Helligkeitskurve der Hintergrundbeleuchtung (0.5.6) | Weltzeit mit Tagesgerüst kommt erst mit den Diensten; die Engine braucht nur die Uhrzeit |
| 2026-10-21 | **Zeichnen: Verdoppeln direkt aus Hintergrund und bewegten Teilen, kein SIMD/PIE und kein DMA** (0.5.2). Gemessen („bench mem“): 96 KB in den PSRAM schreiben kostet 2,4 ms, per CPU wie per DMA; die Schleife ist nicht der Engpass. Weiterer Hebel (Display liest das grobe Bild direkt) ändert den Zeichenweg und geht an den Planungs-Chat | spart die Kopie des Hintergrunds (0,9 ms); am Tag zeichnen Raum und Platz in 3,8 statt 4,1–4,2 ms; SIMD-Code ohne Nutzen wäre nur Pflegeaufwand |
| 2026-10-21 | **Setting der Welt, Zwischenstand der Setting-Werkstatt:** eine Welt um 1850 mit dem Dorf Pixelia als Herz, Hauptfiguren Sykora und Taylor, 16 feste Bewohner ohne Altern, Tod und Wegzug; Wirtschaft ohne Geld als Kausalkette mit Protokoll und Pflicht-Prüfung im Zeitraffer („Prüfen statt raten“). Einzelentscheidungen in [SETTING.md](SETTING.md) Kapitel 19, Wirtschaft in [WIRTSCHAFT.md](WIRTSCHAFT.md) | Entscheidungen des Users in der Setting-Werkstatt; Grundlage für Phase B (0.5.8 ff.). Die Reihenfolge der Phasen ab 0.6.x ist noch nicht geändert, der Vorschlag kommt mit dem Abschluss der Werkstatt |
| 2026-10-21 | **Wetter-Schnittstelle wie die Tageszeit** (0.5.3): ein Wetter je Bühne (`Weather`, Ziel Art + Stärke), bis 0.5.6 gefüttert von der Teststeuerung (Experimentell → Orte, Konsole, Simulator), danach von der Wettersimulation. Die Engine macht die weichen Übergänge (erst Wolken, dann Regen) | wie bei der Uhrzeit braucht die Engine nur das Ziel; die Simulation (SETTING.md Kapitel 9) entscheidet später, wann es regnet |
| 2026-10-21 | **Partikel fein nach dem Verdoppeln, nicht im groben Bild** (0.5.3): Regen und Schnee machen so keine Zeile „bewegt“ (CoarseLayers bleibt schmal), Obergrenze 512 je Ort im PSRAM, nur solange der Ort betreten ist. Schleier (Nebel, Regenschleier, Wolkenschatten) über Farbtabellen beim Verdoppeln | gemessen: Regen +4 ms statt einer Kopie aller Zeilen; Striche 2 × 2 Displaypunkte fein wie die Lichtkreise |
| 2026-10-21 | **Wetter-Aussehen** (Entscheidungen des Users nach animierten Vergleichsbildern): Regen als Striche mit Spritzern, Schnee gemischt (klein und groß), Nebel dicht, Blitze zufällig alle 12–30 s bei vollem Gewitter | Stimmung und Lesbarkeit am Display |
| 2026-10-21 | **Blitz gedämpft, Nebel als Schwaden** (Entscheidungen des Users nach der ersten Sichtprüfung): Blitz hellt im Verhältnis zum Licht auf statt zu Weiß (nachts nicht greller als tagsüber), Nebel zieht in Schwaden von oben nach unten, gleichmäßiger Dunst schwächer | ein weißes Bild blendet im dunklen Zimmer; starrer Dunst wirkte wie zu helle Pixel statt Nebel |
| 2026-10-21 | **Überblendung mit Toleranz** (0.5.3): reichen die 240 Farben beider Orte nicht, werden ähnliche Farben für die Dauer des Übergangs zusammengelegt, statt still abzublenden | der User wählte Überblendung und sah abends trotzdem Abblenden; die Abweichung ist für 0,6 s nicht sichtbar |
| 2026-10-21 | **Fensterschein nur bei Licht innen** (Wunsch des Users, 0.5.3): Lichtquellen je Ort mit Zustand an/aus in der Umgebung der Bühne (`LightBoard`), gilt auch für unsichtbare Orte; Fenster am Außenort verweisen auf den Raum dahinter. Warum ein Licht aus ist, entscheidet später die Simulation (Neuplanung ordnet ein) | Außen- und Innenansicht lesen denselben Zustand; Licht als System (SETTING.md Kapitel 9) braucht genau diese Schnittstelle |
| 2026-10-21 | **Neuplanung nach dem Setting: Phasen als Kapitel der Welt** (Entscheidung des Users): 0.5.x Engine bis 0.5.12, dann 0.6.x Kapitel 1 „Der erste Frühling“ (Bahnhof, Zug, Umgebung, 16 Bewohner, Grundwirtschaft), 0.7.x Kapitel 2 „Das Dorf wächst“ (Schwerpunkt), 0.8.x Kapitel 3 „Die alte Grube“, 0.9.x Kapitel 4 „Die Lok“, 1.0.0; Grube vor Lok. Zusammengeführt wird in 0.5.12 (Gesamtprobe im Testdorf), 0.6.0 (die echte Welt startet), in den Abnahmen am Ende jeder Phase und in 1.0.0. Systemplan mit Baustein, Inhalt, Nachweis und Denkstufe je System in [ROADMAP.md](ROADMAP.md) | erst die Bausteine mit Testinhalt, dann Inhaltswellen (LEBENDIGKEIT Kapitel 4); die Kernsysteme (Speicherstand, Warenbuch, Stufen der Simulation) lassen sich später kaum umbauen und brauchen den Bilanz-Test, bevor echte Wirtschaftswerte kommen; die Lok braucht Kohle und Blei aus der Grube; ersetzt die Phasen Zug, Dorf, Dungeon, Liga, Inselwelt |
| 2026-10-21 | **0.5.x neu geschnitten:** 0.5.4 Weboberfläche aufräumen (fest, Wunsch des Users, erst im Kanal Vorschau), 0.5.5 Asset-Quelle und Grafik-Werkzeug (bisher 0.5.4), 0.5.6 Schrift, Einblendungen und Touch (bisher 0.5.5, Ereignis-Bus nach 0.5.8), 0.5.7 Weltzeit, Kalender und Wetter-Simulation (bisher 0.5.6 „Dienste“, ohne Standort und Open-Meteo), Phase B: 0.5.8 Orte und Gebiete mit Nachprüfung von Phase A, 0.5.9 Weltzustand, Figuren und Speicherstand, 0.5.10 Warenbuch, Aufgaben und Projekte, 0.5.11 Tagesgerüst, Entscheidung und Arbeitsrhythmus, 0.5.12 Nachweis und Gesamtprobe. Browser-Simulator auf GitHub Pages (bisher 0.5.7) wird eine Frage vor 1.0, Ereignis-Regisseur (bisher 0.5.10) kommt mit der ersten Ereigniswelle in 0.6.x | der frühere Patch „Gemeinsamer Weltzustand“ war für eine Sitzung unter 250 K zu groß; das Setting simuliert das Wetter passend zur gestauchten Jahreszeit (7 Tage), echtes Wetter passt nicht dazu; Kontaktbogen und Bericht im PR kommen aus dem kopflosen Simulator; der Regisseur wird mit echten Ereignissen besser entworfen |
| 2026-10-21 | **Zeichenweg bleibt** (grob → 400 × 240 → 2× im Bounce-Callback). Der Vorschlag „Display liest das grobe Bild direkt (4× im Bounce-Callback), feine Einblendungen nur in ihren Zeilen“ wird nicht jetzt umgesetzt, sondern bleibt eine Reserve: Sie wird gezogen, wenn ein echter Ort im schlechtesten Fall mehr als 16 ms für Logik und Zeichnen braucht; gemessen wird in 0.5.12 | Der Gewinn von 2,4 ms fällt nur in Bildern ohne feine Teile an; nachts, bei Regen oder Nebel liegen Lichtraster, Schleier und Partikel in fast allen Zeilen, gerade im schlechtesten Fall bliebe also wenig übrig, außer Licht und Schleier wandern in den Interrupt, was Aussetzer riskiert. Heute liegt der schlechteste Fall bei 16 ms (64 Figuren mit Regen), die Logik einer echten Welt schätzt SETTING.md auf unter 1 ms; Live-Vorschau, Screenshot und Überblendung brauchen das 400 × 240-Bild |
| 2026-10-21 | **Orte und Gebiete als Module einer gemeinsamen Welt** mit gemeinsamem Weltzustand; ein Ort greift nie direkt auf einen anderen zu, Verbindungen über Weltzustand und Ereignis-Bus; eine Perspektive für alle Orte; MINOR = Phase bzw. Kapitel der Welt. Angepasst: CLAUDE.md, PROJEKTPLAN 1, 4.5, 4.6, 4.11, 5 und Kapitel 6 (frühere Welt-Konzepte bleiben als Ideenquelle stehen) | ersetzt „jede Welt funktioniert allein, Verbindungen nur über den Ereignis-Bus“ und „MINOR = Phase/Welt“ aus der Zeit getrennter Welten ([SETTING.md](SETTING.md) Kapitel 15, Entscheidung des Users 2026-10-20) |
| 2026-10-21 | **Echtzeitbezug:** Tageszeit ist die echte Uhrzeit, Jahreszeiten laufen gestaucht (7 Tage), das Wetter wird simuliert (PROJEKTPLAN 6.0 angepasst); ob echtes Wetter oder die echte Jahreszeit als Option kommt, steht in [ZUSATZINHALTE.md](ZUSATZINHALTE.md) | Setting-Entscheidung „Jahreszeit 7 echte Tage, Wetter simuliert“ (SETTING.md Kapitel 9) |
| 2026-10-21 | **Museum komplett im Grundspiel** in 0.7.x mit mehr als den 45 Stücken der Startliste; wie viele realistisch sind, prüft die Museums-Sitzung (Entscheidung des Users) | der User wollte es zuerst als Zusatzinhalt, entschied sich dann fürs Grundspiel; neue Sammlungen aus neuen Gebieten kommen per Update |
| 2026-10-21 | **Keine fremden Grafik-Pakete als Grundstock:** Kenney, Tiny 16, Ninja Adventure, Zelda-like u. a. geprüft (Tabelle in [STIL.md](STIL.md)), keines übernommen; eigene Grafiken, Referenz für den Look ist die Küche aus Skizze v4. Das Grafik-Format und das Werkzeug PNG → Paletten-Indizes (0.5.5) dienen eigenen Grafiken und den Möbeln des Users; die Regel CC0/CC-BY mit `assets/CREDITS.md` bleibt für Ausnahmen | Entscheidung des Users: die Pakete wirken zu „low-budget“; keines hat Figuren in unserer Größe (28 hoch, 4 Richtungen, Kleidung um 1850) |
| 2026-10-21 | **Fensterlicht als System eingeordnet** (Wunsch des Users): `LightBoard` (0.5.3) bleibt die Schnittstelle; Lichtquellen-Arten und Ortszustand „Licht“ 0.5.8, Rituale (anzünden, löschen beim Schlafengehen) 0.5.11, Brennstoff-Kette 0.6.x. Offene Regeln (Kamin als Lichtquelle, Dauer eines Lichtabends, Außenlaternen, Fensterstufe) gehen an die Setting-Werkstatt (ROADMAP-Backlog) | WIRTSCHAFT.md deckt den Verbrauch ab (Lichtabend, Kienspan, P4), nicht aber, wann ein Licht brennt und wie stark es nach außen scheint |
| 2026-10-21 | **Checkliste „Vor 1.0.0 klären“ und Zusatzinhalte nach 1.0** in eigener Datei [ZUSATZINHALTE.md](ZUSATZINHALTE.md); frühere Welt-Ideen in ROADMAP „Frühere Welt-Ideen“ eingeordnet statt gelöscht | Wunsch des Users; die ROADMAP bleibt der Fahrplan bis 1.0 |
| 2026-10-21 | **Setting-Werkstatt: Licht, Schlafenszeit, Jahreszeitenabende, Mindestziele Kapitel 1** (Entscheidungen des Users): Fenster leuchten nur, wo jemand wach ist und Brennstoff hat, Lichtritzen bei geschlossenem Laden, keine Straßenlaternen (Bahnsteig, Taverne, später Dorfplatz), echter Mond; Schlafenszeit 22/23/1 Uhr; Jahreszeiten wechseln sonntags mit großem Abend in der Taverne, Sommerfeuer und Lichterabend; Mindestziele Kapitel 1 in LEBENDIGKEIT Kapitel 2, alte Welt-Ziele entfallen; Grube vor Lok in SETTING nachgetragen | Wünsche des Users; Details in [SETTING.md](SETTING.md) Kapitel 6, 9, 11 und [LEBENDIGKEIT.md](LEBENDIGKEIT.md) Kapitel 2 |
| 2026-10-22 | **Zwei eigene Designs der Weboberfläche als Vorschau** (Wunsch des Users): Pixelia (Pixelschrift des Displays, Pixelrahmen) und Fahrplan 1850 (Serifenschrift, Sepia); nur Aussehen, Seiten und Einstellungen gleich; Auswahl unter Experimentell, gespeichert mit der Farbwahl auf dem Gerät; M3 Expressive mit Lavendel und Tonal bleibt Standard. Pixel-Uhr auf Wunsch des Users weggelassen | gleiche Bausteine, Design nur über Tokens und `design.css`; +3,5 KB, Bundle 147 KB |
| 2026-10-22 | **Grafik-Format und Werkzeug (0.5.5):** Quellen sind PNG-Streifen (alle Bilder eines Teils gleich groß nebeneinander, Zustand für Zustand) und eine Zeichenpalette als GIMP-Palette (`.gpl`, Rampen über Namen „holz 1“ …, Platzhalter-Rampe „platzhalter“); `tools/asset_pack.py` baut daraus Pakete im Format PXA1 (PackBits, CRC-32, Kennungen FNV-1a); Zustandsbilder (Lampe an/aus) statt eigener Leuchtfarbe je Lampe; Farben beim Umwandeln streng/nächste/eigene, eigene Farben bleiben auf Wunsch erhalten (höchstens 16 je Bild) ([STIL.md](STIL.md) Kapitel 9) | Malprogramme laden die Palette direkt; ein Möbel = eine Datei (Entscheidung des Users); Umrechnen mit Vorschau, aber Originalfarben möglich (Wunsch des Users); PackBits ist einfach und entpackt schneller als gelesen wird |
| 2026-10-22 | **Asset-Quelle und Grafik-Speicher (0.5.5):** Quelle eingebettet (Standard), LittleFS oder SD mit Rückfall auf eingebettet; Lade-Task auf Kern 0; die Bühne heftet die Pakete betretener Orte an und lädt die Nachbarorte vor; Abblenden wartet notfalls in der dunklen Mitte, Überblendung und Schnitt beginnen erst nach dem Laden; Budget 2 MB, Verdrängen beim Laden (am längsten unbenutzt, nie angeheftet oder Grundausstattung) | gemessen: eingebettet unter 1,5 ms je Paket, LittleFS 2,2 MB/s (Ort mit 200 KB ≈ 0,11 s, bis zur dunklen Mitte sind es 0,3 s), 0 Aussetzer; das Rechnen der Orte hängt nicht vom Laden ab |
| 2026-10-22 | **Eingebettete Pakete kosten PSRAM** (Befund 0.5.5): mit `SPIRAM_XIP_FROM_PSRAM` liegt die Firmware samt Konstanten im PSRAM, eingebettete Pakete also gepackt im PSRAM und entpackt noch einmal im Grafik-Speicher. Eingebettet bleiben nur Grundausstattung und kleine Inhalte; große Ortspakete gehören nach LittleFS bzw. auf die SD-Karte | Projektplan 2026-10-21 nahm an, eingebettet koste keinen RAM; PSRAM ist mit 5,7 MB frei reichlich, aber nicht für alle Orte doppelt |
| 2026-10-22 | **Skizzen für „So entstand Pixelia“ bleiben auf GitHub Pages** (Lösung aus 0.5.4 übernommen, Frage aus 0.5.5) | ~920 KB: zu groß für das Web-Bundle (≤ 150 KB gzip) und zu viel für LittleFS (Speicherstand, Wirtschafts-Protokoll); ohne Internet zeigt die Zeitleiste nur die Texte; später optional von der SD-Karte |
| 2026-10-22 | **Credits-Prüfung in der CI (0.5.5):** jede Datei in `assets/` braucht eine Zeile in `assets/CREDITS.md` (eigen mit „eigen“, fremd mit CC0 oder CC-BY ohne SA/NC/ND), fremde Dateien liegen nur unter `assets/fremd/<Quelle>/` | Lizenzregel aus CLAUDE.md und assets/README.md maschinell prüfbar; eigene Ordner können fremde Dateien nicht verdecken |
| 2026-10-22 | **Eigene Möbel: Wahl der Fassung je Möbel** (Klarstellung des Users zur Zeile „Grafik-Format und Werkzeug“): beim Hochladen zeigt die Oberfläche die Originalfarben und die auf Pixelia-Farben umgerechnete Fassung nebeneinander, der User wählt je Möbel, welche eingebunden wird | der User will selbst entscheiden; das Format kann beides (eigene Farben, Indizes 224–239), das Werkzeug zeigt beide Vorschauen |
| 2026-10-22 | **Einblendungen (0.5.6, Entscheidungen des Users nach Simulator-Bildern):** Karte als Seitenkarte rechts, die Welt bleibt links hell, die Figur ist markiert; Kurzmeldungen oben als schmale Leiste; Schriften 5×7 + fett (Namen, Uhr) + klein 3×5 (Hinweise); Tätigkeit und Ort ohne Symbole; Pixel-Uhr oben rechts, 24 h ohne Sekunden (Doppelpunkt blinkt), ab Werk an | Bilder in `docs/skizzen/0.5.6/` |
| 2026-10-22 | **Kamera-Modi (0.5.6, Wunsch des Users):** Standard ist Hauptfigur, eine andere Figur oder ein fester Ort (Weboberfläche); Wischen und Ortswahl = „Frei“, Knopf „Folgen“ auf der Karte einer Figur = „Folgen“; Rückkehr zum Standard nach 10 min ohne Eingabe, auch 30 min oder nie | SETTING Kapitel 13 erweitert um „fester Ort“ und „nie“ |
| 2026-10-22 | **Touch-Gesten (0.5.6):** Wischen nach links = nächster Ort (Umblättern); Folgen über den Knopf auf der Karte, langer Druck bleibt die Info-Karte (0.2.1) (Wunsch des Users: Halten auf bewegten Figuren traf oft die falsche); Karte mit fester Höhe; Treffer nach Tiefe, durchsichtige Punkte zählen nicht; Schriften und Symbole als Grafik im Paket „system“ (Grundausstattung), die Systemschrift bleibt im Code für Setup und Update | – |
| 2026-10-22 | **Jeder Ort per Tippen erreichbar und verlassbar** (Wunsch des Users): Türen, Wege, Bildränder und Schwellen sind antippbar und zeigen eine Karte mit Ziel, wer dort ist, und „Betreten“/„Hingehen“; die Weltkarte (Orts-Graph, 0.5.8) wird so geplant, dass jeder Ort von jedem anderen aus durch Tippen erreichbar ist, in beide Richtungen. Dazu in 0.5.6 auf Wunsch: Ortsname kurz nach jedem Wechsel, Hinweis vor der Rückkehr, Knopf „Nicht mehr folgen“, Uhr 22–6 Uhr gedimmt, Uhr antippen = Datum | ROADMAP 0.5.8, SETTING Kapitel 15 |
| 2026-10-26 | **Weltzeit (0.5.7, Entscheidungen des Users nach Simulator-Bildern):** Mond groß mit Kratern (15 Pixel, graue Meere, dunkle Seite schwach sichtbar); Jahreszeit-Symbol vor der Pixel-Uhr, Antippen zeigt Datum, Jahreszeit mit Tag, Weltjahr und Mond; Nachtmodus dimmt sanft (15 min) auf die Nacht-Helligkeit (Standard 25 %), Berühren hellt 1 min auf; Jahreszeitenabend (So 18 Uhr), Wechsel (Mo 6 Uhr) und Erstlinge als Kurzmeldung. Zusätze auf Wunsch: Wetter-Vorschau auf der Seite Welt, Zeile in der Übersicht, Nachtzeiten einstellbar (eine Nacht für Uhr und Beleuchtung) | Bilder `.logs/mond-vergleich.png` im PR |
| 2026-10-27 | **Licht und Figuren (0.5.9 Teil 3, Entscheidungen des Users):** Figuren im Chibi-Stil der Skizze v3 (große runde Köpfe, Frisuren mit Volumen, Bärte, Brillen, Hüte), Version 3 „Mehr Tiefe“ aus drei Animationen im Raum: Augen mit Glanzpunkt, Lichtkante an der Kleidung, farbige Kontur; die erste Proportions-Skizze war nur die Größe. Mischen der Paletten bei der Überblendung bleibt, wie es ist (gemessen ≤ 5,4 ms; die Spitze von 13,3 ms kam von der Lichtkarte beim Betreten). | Wunsch des Users; Messung am Gerät |
| 2026-10-27 | **Licht und Figuren (0.5.9 Teil 3, technisch, Claude):** Teure Darstellung entsteht mit festem Budget je Logik-Schritt (`Scene::prepare`, `Stage::kPrepareBudget` 6000 ≈ 1,5 ms; betretene Orte zuerst). Lichtkarte Zeile für Zeile aus Tabellen (Abfall je Kreis nach d², Stufen nach Lage im Raster), sichtbar in einen zweiten Puffer, unsichtbar an Ort und Stelle; Tabellen im internen RAM brachten nur 15 % und bleiben im PSRAM. Figuren-Baukasten: Ebenen im Grafik-Paket mit Platzhalter-Rampe, `FigureArt` legt sie je Aussehen mit Farbreihen übereinander und zieht die Kontur erst danach (nur außen); 24 fertige Aussehen im Grafik-Speicher; Varianten-Nummern stehen im Speicherstand, also nur hinten anhängen. | Leistungsbudget (kein Schritt über 8 ms), deterministisch, ein Bild je Figur beim Zeichnen |
| 2026-10-27 | **Stufen der Simulation (0.5.9 Teil 2, Entscheidungen des Users):** Beide Stufen laufen auf Kern 1 (statt grob auf Kern 0, SETTING.md Kapitel 15): Die grobe Stufe kostet unter 1 ms je Sekunde, auf Kern 0 bräuchte sie für exakte Ergebnisse an jeder Takt-Grenze einen Handschlag mit Kern 1 (Gefahr von Bildaussetzern); die Übergaben laufen trotzdem nur an Takt-Grenzen, ein Umzug auf Kern 0 bleibt möglich (z. B. bei Hunderten Orten). Tätigkeiten sind Zeitblöcke mit exakten Weltzeiten (nicht auf Sekunden gerundet, nicht in Minutenschritten), Gehzeit aus der Weglänge; beim Betreten steht jede Figur genau an ihrer Stelle im Block; wer hinausgeht, wartet bis zur nächsten vollen Sekunde an der Tür. Technisch (Claude): Entscheidung zur Zeit des Blockendes mit eigenem Zufall je Figur und Entscheidung, Blockenden auf einer Takt-Grenze rechnet nur die grobe Schleife, Neuankömmlinge entscheiden an der nächsten Takt-Grenze, Kalender und Ortszustände erst danach: so bucht ein Ort sichtbar wie unsichtbar dasselbe (P10 byte-gleich). Sprung der Uhr über 10 Weltminuten: alle neu ansetzen (gleicher Ort). Bis zur echten Uhrzeit läuft die Weltuhr am Gerät ab dem geladenen Stand weiter. |
| 2026-10-27 | **Weltzustand und Speicherstand (0.5.9 Teil 1, Entscheidungen des Users):** Speichern alle 5 Minuten (dazu vor Neustart und Update), Nachholen nach dem Einschalten bis 3 Tage; Weboberfläche bekommt Karte Speicherstand mit „Jetzt speichern“, Chronik, Figurenliste (Experimentell → Orte) und den Speicherstand in der Sicherung (Teil 2) | Bei Stromausfall fehlen höchstens 5 Minuten, die die Welt nachholt; 3 Tage wirken auch nach längeren Pausen echt, das Nachholen verteilt sich über die Bilder |
| 2026-10-27 | **Speicherstand (0.5.9 Teil 1, technisch, Claude):** LittleFS `/data`, zwei Plätze `welt-a.pxw`/`welt-b.pxw` mit Generation und CRC-32, geschrieben wird der ältere; Format PXW1 aus Abschnitten mit eigener Version, Bezüge auf Orte und Fähigkeiten als Kennungen; Kern 1 packt an der Takt-Grenze in 96 KB PSRAM, die Lade-Task auf Kern 0 schreibt; gespeichert erst mit gültiger Weltzeit, nie unter der Testuhr. Figuren gehören der Welt (ein Feld `where` je Figur: Ort oder Verbindung), Orte zeigen sie; Statisten nur für Messungen | Zwei Plätze brauchen kein atomares Umbenennen und gelten auch für die SD-Karte; ein abgebrochener Schreibvorgang lässt immer den vorigen Stand übrig. Abschnitte und Kennungen lassen alte Stände in neuer Firmware laden (Vorlage `sim/testdata/welt-0.5.9.pxw`). Ein einziges Aufenthaltsfeld macht doppelte und verlorene Figuren unmöglich. Am Gerät 1,9 KB, packen ~1 ms, schreiben ~60 ms, Flash-Verschleiß rechnerisch weit über 100 Jahre |
| 2026-10-27 | **Ortszustände (0.5.8 Teil 2, Entscheidungen des Users):** Im Testraum steht ein gemauerter Herd mit Rohr und Topf (Wahl aus Kachelofen, Herd, Kamin), auf dem Platz ein rundes Wasserbecken (statt eckigem Trog), nasser Boden wird bis gut ein Drittel dunkler und bekommt ab „nass“ Pfützen (wie in der Vorschau). |
| 2026-10-27 | **Ortszustände, Ereignis-Bus, Fahrzeuge (0.5.8 Teil 2, technisch, Claude):** Ortszustände gehören zur Welt (Umgebung), nicht zur Szene: Die Bühne schreibt sie je Weltminute für alle Orte fort, nach Sprüngen Minute für Minute (höchstens ein Tag); Orte zeigen sie nur und ändern sie über Handlungen. Licht hat Art, Stärke (SETTING Kapitel 9) und Brennstoff, der bis zum Warenbuch unbegrenzt vorrätig ist; bis zum Tagesgerüst (0.5.11) entscheiden Hausregeln (Abendlicht bis 23 Uhr, Kerze für Heimkehrer, Herdfeuer mit abgedeckter Glut). Der Ereignis-Bus stellt an Takt-Grenzen zu (alle Leser sehen dasselbe, unabhängig von der Reihenfolge der Orte); der Kalender meldet nur noch dorthin. Fahrzeuge sind Orte mit Fahrplan als reine Rechnung aus der Weltminute; Verbindungen mit Fahrplan sind Daten wie alle anderen. Die Glut bekommt die vier Farben der Farbreihe Grün der Testfiguren. |
| 2026-10-26 | **Orte und Gebiete (0.5.8 Teil 1, Entscheidungen des Users):** Fenster der Räume groß mit Vorhang und Fensterbank (Variante B aus drei Simulator-Bildern); Kerze auf der Fensterbank brennt für Heimkehrer (nachts, solange jemand aus dem Haus unterwegs ist); sichtbare Ortszustände im Testdorf: Nässe, Füllstand eines Wasserbeckens, Ofen mit Glut; neue Testorte Kammer (Haus mit mehreren Räumen) und Feldweg mit Reisezeit, kein Keller. Alles in 0.5.x ist Baustein mit Platzhaltern; Pixelia selbst übernimmt die Regeln ab 0.6.0 (Frage des Users). |
| 2026-10-26 | **Orte und Gebiete (0.5.8 Teil 1, technisch, Claude):** Orte sind Daten (`PlaceDef`) in `worlds/<gebiet>/orte/`, eine Zeile Registrierung je Ort und je Gebiet; die Testort-Klasse wandert nach `worlds/testdorf/`. Verbindungen sind die Ausgänge der Raumliste mit Art, Reisezeit und Bedingungen; der Orts-Graph prüft Rückwege und Erreichbarkeit. Wer auf einem Weg mit Reisezeit ist, gehört keinem Ort (Bühne hält ihn, Ankunft nach Weltzeit). Der Mond hat eigene feste Farben je Ort (Testorte: Türkis-Reihe), damit er beim Ortswechsel mitblendet; im Fenster steht er ganz in einer Scheibe (Scheiben als Daten des Fensterbilds). Bedingungen wie „nach Bau“ lesen bis 0.5.10 Merker der Welt (`WorldFlags`). |
| 2026-10-26 | **Weltzeit (0.5.7, technisch, Claude):** Kalender und Wetter-Simulation sind reine Rechnungen aus einem kleinen Zustand und dem Datum (Testuhr und Zeitraffer schaden nicht, nach einem Neustart dasselbe Wetter); Sonne 12 h ± 4 h 18 min um 12:45, Dämmerungen behalten ihre Länge; Mond nach Meeus (±0,4 h gegen Finsternisse), Lage aus der Phase; Mond in den Farben der System-Oberfläche (die Ortspaletten sind voll), daher nicht während eines Ortswechsels; Kalender bis 0.5.9 als Blob „welt“ im NVS; Weltzeit erst mit bekannter Uhrzeit; Länge 3/7/14/28 Tage in der Weboberfläche, Konsole 1–28 | SETTING Kapitel 9 |
| 2026-10-28 | **Figuren mit Statur** (0.5.9 Teil 5, Wahl des Users nach Kontaktbogen und Gehbild-GIF): Ansatz A „Chibi mit Statur“, Kopf bleibt; 4 Größen (klein, mittel, groß, sehr groß: −2 bis +4 Zeilen), 3 Breiten, Bauch 0–2, gebeugt; neue Kleidung Kopftuch, Strohhut, Gehrock, Wirtsschürze, Gehstock; zehn Testfiguren aus SETTING Kapitel 3. **Größte Figur 32 statt 28 hoch**, fertiges Bild 24 × 40 (Fußpunkt 11, 37), Platz im Grafik-Speicher 14,4 statt 13 KB. Technik: **Strecken beim Zusammensetzen** statt jede Statur fertig zu zeichnen (Zeilen und Spalten im Inneren verdoppeln oder weglassen; Werkzeug und Engine rechnen gleich), Statur als Feld hinten im Figuren-Datensatz (alte Stände: Standard) | jede Kombination ohne zusätzlichen Speicher (fertig gezeichnet: je Statur 180–310 KB bei 2 MB Budget); am Gerät 716–736 µs je Bild, kFrameCost 3000 (STIL Kapitel 9) |
| 2026-10-29 | **Warenbuch (0.5.10 Teil 1, technisch, Claude):** ein Warenbuch für die ganze Welt im Weltzustand (`ledger.h`): Mengen je Lagerplatz, Einzelstücke mit eigenem Datensatz, die Trage jeder Figur als Lagerplatz (was unterwegs ist, gehört genau einem Lagerplatz); jede Buchung ein Aufruf, nie negativ, nie über der Obergrenze; Zähler je Ware führen die Bilanz. Je Schritt erst die Takt-Grenzen (grob, mit Warenbuch), dann Kalender und Ortszustände, dann die betretenen Orte (voll); Ortszustände nie über die letzte Takt-Grenze hinaus; was mehrere Orte betrifft (Reservierung, Aufgaben, Projekte), ändert sich nur an Takt-Grenzen; Speicherstand sortiert nach Kennungen | Bilanz exakt und sichtbar wie unsichtbar gleich (P10), auch wenn Ortszustände im Warenbuch buchen (LEBENDIGKEIT Regel 10) |
| 2026-10-29 | **Nachholen vor Tätigkeiten (0.5.10, technisch, Claude):** Holen die Ortszustände mehr als 10 Weltminuten nach (Laden, Uhrsprung), warten die Tätigkeiten und setzen danach neu an | Vergangenes kommt vor dem Jetzt ins Warenbuch; die Pause dauert beim Start höchstens etwa 0,6 s |
| 2026-10-29 | **Chronik und Warenbuch (Wahl des Users):** In die Chronik kommt nur Besonderes (Lager leer, voll, Notbestand, Teilaufgabe oder Projekt fertig), nicht jede Bewegung; jede Bewegung steht im Wirtschafts-Protokoll und in der Lager-Karte. Weicht von WIRTSCHAFT 6 („jede Bewegung steht in der Chronik“) ab, Punkt für den Planungs-Chat | Die Chronik hat 64 Einträge; wichtige Einträge wären sonst nach Minuten verdrängt |
| 2026-10-29 | **Lager-Karte schon in 0.5.10 (Wahl des Users):** einfache Karte auf der Seite Welt (Lagerplätze mit Füllstand und Reichweite, Projekte mit Fortschritt, offene Aufgaben); die volle Seite „Lager“ kommt in 0.7.x | Am Handy mitverfolgen, was das Testdorf tut |
| 2026-10-29 | **Testinhalt 0.5.10 (Wahl des Users):** Holzkette (Hackklotz → Holzstapel → Holzlege → Herd), Bett-Projekt in der Kammer, Brot einzeln im Regal; die Axt als abnutzendes Einzelstück wurde nicht gewählt (Einzelstücke prüft der Simulator); alle Zahlen sind Testwerte | Prüfsteine für Holen und Bringen, Projekte und sichtbare Bestände |
| 2026-10-29 | **Wirtschafts-Protokoll (Wahl des Users mit eigener Ergänzung):** LittleFS `/data/wirtschaft`, je Tag und Tabelle eine Datei, ungepackt, höchstens 1 MB, die ältesten Tage fallen heraus; gesichert **zusammen mit dem Speicherstand** (alle 5 Minuten, Knopf „Speichern“, vor einem geplanten Neustart) statt stündlich | Der User startet unregelmäßig neu; Welt und Protokoll stehen so immer am selben Zeitpunkt |
| 2026-10-29 | **Aussehen von Bestand, Bau und Karten (0.5.10, Wahl des Users nach Kontaktbögen `docs/skizzen/0510-*.png`):** Stellplätze zeigen jedes Stück einzeln (was keinen Platz hat, liegt „hinten“ und steht als Zahl auf der Karte); „im Bau“ zeigt das halbfertige Teil in Stufen und daneben den Materialstapel, der mit der Arbeit schrumpft; Karte eines Lagerplatzes in Zeilen (eine Ware) bzw. als kurze Liste mit Stand in einem Wort (mehrere Waren); Karte eines Projekts mit senkrechter Schritt-Leiste, nur der laufende Schritt zeigt sein Material (Idee des Users) | „Jedes Stück im Bild“ ist ein Wunsch des Users; die Schritt-Leiste bleibt auch bei 13 Teilaufgaben (Lok) übersichtlich |
| 2026-10-29 | **WIRTSCHAFT 6 angepasst (freigegeben über den Planungs-Chat):** „Jede Bewegung steht in der Chronik“ ersetzt durch „Die Chronik zeigt nur Besonderes; jede Bewegung steht im Wirtschafts-Protokoll und auf der Lager-Karte“ | Folgt der Wahl des Users zur Chronik (Zeile oben) |
| 2026-10-29 | **Aufgabenliste der Gemeinschaft (0.5.10 Teil 2):** eigenes Modul im Weltzustand (`tasks.h`, Abschnitt AUFG). Aufgaben entstehen, reservieren und enden nur an der Takt-Grenze nach allen Planern; übernommen werden sie nur genau an einer Takt-Grenze (Figuren ohne Aufgabe warten dafür kurz); danach ändert nur die eigene Figur Phase und Trage. Holen nimmt mindestens die halbe Ladung (leerer Vorrat: was da ist) | Gleiche Wirkung sichtbar und unsichtbar (P10) ohne Sperren; geprüft mit Transporten und Neustart mitten im Transport |
| 2026-10-29 | **Nachholen mit Transporten (Wahl des Users):** Nach mehr als 10 Weltminuten Pause rechnen beim Nachholen nur die Ortszustände (z. B. der Herd verbrennt, was in der Holzlege liegt), Figuren holen erst danach Nachschub. „Grob nachschätzen“ (Hausvorräte aus der Quelle auffüllen, Erzeuger nach Takt) ist für 0.5.12 vorgemerkt | Einfach, schneller Start, Bilanz aufs Stück; ob es sich mit echten Inhalten richtig anfühlt, zeigt die Gesamtprobe |
| 2026-10-29 | **Brot im Testdorf (Wahl des Users):** Platzhalter-Erzeugerlager „Backofen“ am Platz (Figuren backen wie am Hackklotz), die Vorratskiste holt dort, das Brotregal aus der Vorratskiste; Essen ist bis 0.5.11 ein Platzhalter (zwei feste Mahlzeiten je Figur und Tag, nur wo Brot liegt). Dazu Befehl „aufgaben“, Chronik bei Gemeinschaftsaufgaben, Kurzmeldung bei Notbestand (Wunsch des Users) | Geschlossene Kette, das Testdorf bleibt am Gerät auch nach Tagen lebendig |
| 2026-10-29 | **Stellplätze Stück für Stück (0.5.10 Teil 2b):** Der Raum-Baukasten zeigt die Füllung eines Teils (`Room::set_fill`) auf seinen Stellplätzen, je Stück einer, im Hintergrundbild wie das Teil selbst; eine Änderung zeichnet nur das Rechteck der betroffenen Stücke neu (auf Kacheln erweitert, pixelgleich mit dem ganzen Bild). Begründung: pro Bild fast keine Kosten (40 Scheite +3 µs am PC), Neuzeichnen 1–1,4 µs statt 9–12 µs. Holzstapel mit Pfählen (Wahl des Users aus vier Formen), Backofen-Glut und Brot auf dem Brett (Wünsche des Users); sichtbares Tragen erst in 0.6.x mit den echten Figuren (Wahl des Users). |
| 2026-10-30 | **Projekte mit Teilaufgaben (0.5.10 Teil 3a, Wahl des Users):** Material kommt über Holen-Aufgaben **ein Schritt voraus** (laufender und nächster Schritt; passt auch zur Lok mit 13 Schritten), ein Schritt beginnt erst, wenn sein ganzes Material an der Baustelle liegt, und reserviert es dort; verbaut wird mit jedem Arbeitsgang (der Stapel schrumpft mit der Arbeit). Am Bett arbeitet **nur, wer Tischlern kann**, **nur tagsüber 7–19 Uhr** (Platzhalter bis 0.5.11); Helfer kommen mit dem Bautrupp in 0.6.x. Der Ortszustand „angefangene Arbeit“ (`StateKind::Work`) ist durch Projekte ersetzt; ein alter Stand beginnt das Bett-Projekt frisch (das alte Bett im Raum war ein anderes Teil). Chronik: Teilaufgabe fertig und Projekt fertig | Material genau einmal und nachvollziehbar (Reservierung, Bilanz exakt über Neustarts), lebendiger Ablauf (holen, warten, bauen), projekte.csv schreibt Teil 4 aus `ProjectRecord` |
| 2026-10-30 | **Aussehen „im Bau“ (0.5.10 Teil 3b, Wahl des Users nach zwei Kontaktbögen, [0510-im-bau-varianten.png](skizzen/0510-im-bau-varianten.png), [0510-im-bau-stufen.png](skizzen/0510-im-bau-stufen.png)):** Runde 1 (warmes Holz auf Dielen) war dem User zu schwer lesbar („alles dieselbe Farbe“, Baustelle nicht markiert). Gewählt: **Bauplane** (Leinentuch als eigenes Bodenteil unter Bett und Stapel, nur solange gebaut wird) mit **hellem, frisch gesägtem Holz** und dunkler Kontur; Bett in vier Stufen (Bauplatz mit Kreide und Sägebock, Gestell, Latten, Strohsack wird gefüllt). Zusatzwünsche: Kurzmeldung bei fertigem Projekt, Strohsack darf jeder füllen (umgesetzt), **Säge als Werkzeug am Projekt** (geholt, nicht verbaut, zurückgebracht; kommt in Teil 4). Zeichen mit Fortschritt über der Baustelle (Variante C) im Hinterkopf | Baustelle auf einen Blick erkennbar, auch nachts; Material hebt sich vom Boden ab |
