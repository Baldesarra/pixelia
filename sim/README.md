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
```

`-Headless` baut ohne SDL nach `build-sim-headless\` (schneller). Alle Optionen: `--help`.

Tasten im Fenster: Leertaste Pause, `+`/`-` Zeitraffer, `T` eine Stunde weiter, `S` Screenshot nach `screenshots/`,
`Esc` Ende.

## CI

`.github/workflows/simulator.yml` baut unter Linux und legt Screenshots (Morgen, Mittag, Abend) und einen
Kontaktbogen als Artefakt **simulator-screenshots** ab.

## Werkzeuge

Lokal: LLVM-MinGW (clang), CMake, Ninja. SDL3 3.4.16 wird von CMake geladen und per SHA-256 geprüft.
