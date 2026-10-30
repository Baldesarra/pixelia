# sim/ – Pixelia-Simulator

Derselbe Engine- und Szenen-Code wie auf dem Gerät (`engine/`, später `worlds/`), am PC:

- **Fenster** (SDL3): 800×480 wie das Display, 30 Logik-Schritte pro Sekunde, Farben wie auf dem Display (RGB565)
- **Kopflos** (ohne SDL): Screenshot zu fester Uhrzeit, vorspulen, Zeitraffer, Kontaktbogen mit zufälligen Momenten
  und ein kurzer Bericht. Das ist die Grundlage für den Langzeit-Test der Welten (`docs/LEBENDIGKEIT.md`, Kapitel 3).

## Bauen und starten (Windows)

```powershell
.\tools\sim.ps1                       # nur bauen (mit Fenster, beim ersten Mal lädt CMake SDL3)
.\tools\sim.ps1 --time 20:30          # Fenster öffnen, Welt startet um 20:30
.\tools\sim.ps1 -Headless --headless --time 06:30 --screenshot build-sim-headless/out/morgen.png
.\tools\sim.ps1 -Headless --headless --contact-sheet build-sim-headless/out/kontaktbogen.png --span-days 2
.\tools\sim.ps1 -Headless --headless --screen setup --screenshot build-sim-headless/out/setup.png
.\tools\sim.ps1 -Headless --headless --scene testort-wiese --go testort-raum:ueberblendung --sequence build-sim-headless/out/wechsel.png --every 4 --count 4
```

`-Headless` baut ohne SDL nach `build-sim-headless\` (schneller). Alle Optionen: `--help`.

Seit 0.5.0 läuft im Simulator dieselbe Bühne wie am Gerät mit den Orten `scrolltest` (Landschaft), `testort-wiese` und
`testort-raum`. `--scene` wählt den Startort, `--go ORT[:ÜBERGANG]` wechselt nach dem Vorspulen (`abblenden` Standard,
`ueberblendung`, `raster`, `schnitt`), `--after N` rechnet danach weiter, `--sequence DATEI` legt eine Bildfolge an
(`--every`, `--count`), z. B. für Übergänge und Animationen. `--load N` setzt die Figuren der Testorte bzw. die Vögel
der Landschaft.

Seit 0.5.1 gibt es dazu `testort-platz`; die Testorte sind aus Kacheln und Möbel-Teilen gebaut und über Tür und Bildrand
verbunden. `--cmd ausbau,baustelle` schickt Befehle an den Startort (Ausbaustufe, „im Bau“), `--inspect` blendet
Stellflächen, Benutzungspunkte und Ausgänge ein, `--no-follow` lässt den Blick am Startort (sonst folgt er der
Begleiterin durch die Ausgänge). Bildschirmfotos vom echten Display: `python tools/screen.py DATEI.png`.

`--screen NAME` zeichnet eine Systemanzeige mit Beispieldaten (Setup-Modus, Update, Meldung, Kurzmeldung), mit
demselben Code wie auf dem Gerät: `setup`, `setup-leer`, `setup-fehler`, `update`, `update-fertig`, `failed`,
`message`, `toast`, `toast-lang`, `toast-ip` (Kurzmeldung nach dem Verbinden), `info`, `info-setup`, `info-offline`
(Info-Karte im WLAN, im Setup-Modus und ohne Verbindung), `hold` (Halte-Anzeige während des langen Drucks).

`--wirtschaft DIR` (seit 0.5.10, kopflos) schreibt das Wirtschafts-Protokoll wie am Gerät (CSV je Welttag und
Tabelle, [WIRTSCHAFT.md](../docs/WIRTSCHAFT.md) Kapitel 15), z. B. mit `--frames 30000 --speed 600` für eine Woche.

`--test gestures` (bzw. `--test all`) prüft die Gestenerkennung ohne Hardware mit nachgestellten Touch-Abläufen:
Tippen, langer Druck (auch ohne neue Messwerte), Zittern, Bewegung, zwei Finger, einstellbare Dauer, Zeitüberlauf.
`--test redact` prüft den Passwort-Filter der Diagnose-Ausgaben (`engine/redact.h`, seit 0.2.2): Geheimnisse über
Blockgrenzen hinweg, JSON-Schreibweise, ineinander enthaltene Geheimnisse, zu kurze, Abbruch des Ziels.
`--test stage` prüft die Bühne (seit 0.5.0): gleicher Startwert ergibt dieselben Bilder, Zeichnen ändert den Ablauf
nicht, unsichtbare Orte rechnen weiter, Übergänge (Dauer, Mischung, Abblenden), Verdoppeln, Farbreihen, Animationen.
Exit-Code 1, wenn ein Fall scheitert.

Tasten im Fenster: Leertaste Pause, `+`/`-` Zeitraffer, `T` eine Stunde weiter, `S` Screenshot nach `screenshots/`,
`1`–`9` Ort wechseln, `U` Übergang durchschalten, `Esc` Ende. Die Maus ersetzt den Touch, mit derselben Gestenerkennung wie auf dem Gerät: 3 s gedrückt halten zeigt
die Info-Karte (mit Beispieldaten), ein Klick schließt sie.

## CI

`.github/workflows/simulator.yml` baut unter Linux, führt die Selbsttests aus (`--test all`) und legt Screenshots
(Morgen, Mittag, Abend) und einen Kontaktbogen sowie alle Systemanzeigen (`system-*.png`) als Artefakt
**simulator-screenshots** ab.

## Werkzeuge

Lokal: LLVM-MinGW (clang), CMake, Ninja. SDL3 3.4.16 wird von CMake geladen und per SHA-256 geprüft.
