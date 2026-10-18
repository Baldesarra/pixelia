# Pixelia – Arbeitsregeln für Claude

Pixelia: lebendige Idle-Pixel-Welten (Zugfenster, Pixel-Dorf, Dungeon-Held, Pixel-Liga) auf einem
**Sunton ESP32-8048S050C_I** (ESP32-S3 N16R8, 5" 800×480 RGB-Parallel, GT911-Touch) mit Weboberfläche im
Material 3 Expressive Design und Updates über GitHub.

## Zuerst lesen

- [docs/PROJEKTPLAN.md](docs/PROJEKTPLAN.md): Gründungsdokument mit allen Grundsatzentscheidungen und Begründungen.
  **Nicht stillschweigend ändern.** Neue Entscheidungen als Zeile ins Entscheidungsprotokoll (Kapitel 14).
- [docs/ROADMAP.md](docs/ROADMAP.md): aktueller Stand, Phasen, Checklisten, Backlog. **Bei jedem Fortschritt pflegen**
  (Abschnitt „Aktueller Stand“, Häkchen, neue Wünsche des Users ins Backlog mit Datum).

## Zusammenarbeit

- Mit dem User **Deutsch** sprechen. Doku, UI-Texte und Changelog auf Deutsch.
- Claude ist „Kapitän“: technische Entscheidungen selbst treffen und kurz begründen, Verbesserungen aktiv vorschlagen.
  Geschmacks- und Prioritätsfragen an den User geben.
- Der User hat keine GitHub-Erfahrung: Git-/GitHub-Schritte erklären und so weit wie möglich automatisieren.
- Hardware-Tests macht der User. Bei visuellen Änderungen Simulator-Screenshots liefern.
- **Das Pixel-Dorf ist der Schwerpunkt** und soll besonders detailliert und lebendig werden.
- **Lebendigkeit ist das oberste Qualitätskriterium:** [docs/LEBENDIGKEIT.md](docs/LEBENDIGKEIT.md) ist verbindlich
  (Systeme statt Schleifen, Mindestziele pro Welt, Langzeit-Test). Nur berichten, was gebaut und geprüft ist; keine
  Versprechen ohne Nachweis.
- Bei GitHub-Aktionen, die der User ausführen muss (z. B. Merge), immer ausdrücklich sagen: was, wo, welcher Knopf.

## Technik (Kurzfassung, Details im Projektplan)

- ESP-IDF + C++20, kein Arduino. Weboberfläche: Svelte 5 + TypeScript + Vite, gzip in die Firmware eingebettet,
  komplett offline (keine CDNs).
- Rendering: 400×240, 8 Bit Palette, 2× hochskaliert im RGB-Bounce-Buffer-Callback. Logik 30 Hz.
  Kern 1 = Grafik/Welten, Kern 0 = WLAN/Web/Hintergrund-Simulation.
- Engine und Welten sind plattformunabhängig (laufen auch im PC-/Browser-Simulator). Keine ESP-IDF-Header in
  `engine/` oder `worlds/`.
- Welten sind Module mit fester Schnittstelle. Verbindungen zwischen Welten **nur über den Ereignis-Bus**.
- Inhalte (Ereignisse, Berufe, Gegenstände …) datengetrieben in `assets/`, nicht im Code verstreut.
- Zufall deterministisch mit Startwert.
- Code-Bezeichner Englisch, Kommentare Deutsch.

## Werkzeuge (lokal, Windows)

- ESP-IDF **v6.1** über EIM: IDF in `D:\Espressif\v6.1\esp-idf`, Werkzeuge in `C:\Espressif\tools`.
- **Immer `tools/idf.ps1` statt idf.py direkt** benutzen: aktiviert die Umgebung, schreibt das volle Log nach
  `.logs/idf-last.log` und zeigt nur Fehler, Warnungen und die Zusammenfassung. Beispiel: `.\tools\idf.ps1 -C firmware build`.
  Das Log nur bei Fehlern gezielt lesen.
- Nach Installationen fehlt der neue PATH evtl. in der laufenden Shell: in PowerShell aus Machine+User-Umgebung nachladen.
- Simulator-Compiler: LLVM-MinGW (`g++`/`clang++`), CMake, Ninja. Node.js 24 für die Weboberfläche.
- Grundsatz: lange Ausgaben (Builds, Logs, serielle Ausgabe) immer über Skripte filtern, nicht roh ins Gespräch holen.

## Leistungsbudget

30 fps stabil (nie unter 25). Logik + Rendern ≤ 20 ms pro Bild. Freier interner RAM ≥ 60 KB, freier PSRAM ≥ 1 MB.
Firmware ≤ 75 % der App-Partition (6 MB).

## Versionen und Changelog

- Immer drei Stellen `MAJOR.MINOR.PATCH`. **MINOR = Phase/Welt** (0.6.x = Zug, 0.7.x = Dorf, …),
  **PATCH = jede Iteration** (0.6.0 erste Zug-Version, 0.6.1 zweite …). Vorschau-Versionen: `X.Y.Z-beta.N`.
- Version kommt ausschließlich aus dem Git-Tag `vX.Y.Z`. Kein manuelles `FW_VERSION`.
- Jede veröffentlichte Version bekommt einen Eintrag in `CHANGELOG.json` (neuester oben):
  `{v, d (Datum), t (Titel), s (Untertitel), n: Neu, i: Verbessert, f: Behoben, r: Entfernt}`.
- Jeder Stichpunkt beginnt mit dem Bereich: `System:`, `Display:`, `WLAN:`, `Web:`, `Update:`, `Engine:`, `Touch:`,
  `Zug:`, `Dorf:`, `Held:`, `Liga:` …
- Experimentelle Funktionen tragen „(Vorschau)“ und liegen hinter dem Experimentell-Schalter.

## Git-Arbeitsweise

- Änderungen auf eigenem Branch → Pull Request → CI grün → Merge durch/nach Freigabe des Users.
  Ausnahme: Solange es noch keine CI gibt (vor 0.1.0), gehen Doku- und Werkzeug-Änderungen direkt auf `main`.
- Commit-Nachrichten auf Deutsch, kurz und beschreibend (z. B. „Zug: Regentropfen an der Scheibe“).
- Release: Tag `vX.Y.Z` auf `main` → GitHub Actions erstellt das Release mit `.bin`, Factory-Image und Release-Notes.
- Keine Geheimnisse ins Repo (WLAN-Daten, Tokens). Fremde Grafiken nur mit CC0/CC-BY-Lizenz, Eintrag in
  `assets/CREDITS.md`.
