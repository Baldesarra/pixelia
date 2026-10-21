# Pixelia – Arbeitsregeln für Claude

Pixelia: eine lebendige Idle-Pixel-Welt um 1850 (das Dorf Pixelia mit Bahnhof, Umgebung, Grube und Strecke als
Orte) auf einem **Sunton ESP32-8048S050C_I** (ESP32-S3 N16R8, 5" 800×480 RGB-Parallel, GT911-Touch) mit Weboberfläche
im Material 3 Expressive Design und Updates über GitHub. Was bis 1.0 wann kommt: [docs/ROADMAP.md](docs/ROADMAP.md)
„Systemplan“.

## Zuerst lesen

- [docs/PROJEKTPLAN.md](docs/PROJEKTPLAN.md): Gründungsdokument mit allen Grundsatzentscheidungen und Begründungen.
  **Nicht stillschweigend ändern.** Neue Entscheidungen als Zeile ins Entscheidungsprotokoll (Kapitel 14).
- [docs/ROADMAP.md](docs/ROADMAP.md): aktueller Stand, Phasen, Checklisten, Backlog. **Bei jedem Fortschritt pflegen**
  (Abschnitt „Aktueller Stand“, Häkchen, neue Wünsche des Users ins Backlog mit Datum).
- [docs/CODEMAP.md](docs/CODEMAP.md): Code-Landkarte (Module, Tasks und Kerne, Speicher, Schnittstellen, Werkzeuge).
  Statt den Code zu durchsuchen zuerst hier nachsehen; am Ende jeder Phase aktualisieren.
- [docs/SETTING.md](docs/SETTING.md) und [docs/WIRTSCHAFT.md](docs/WIRTSCHAFT.md): das Skript der Welt (Figuren, Orte,
  Ereignisse, Bauen) und die Wirtschaft (Waren, Kausalketten, Protokoll, Prüfgrößen). Vor Arbeit an Welt-Inhalten
  die betroffenen Kapitel gezielt lesen; **Wirtschaftsänderungen nur mit grünem Zeitraffer-Bericht** (WIRTSCHAFT.md
  Kapitel 14).

## Sitzungen und Nutzungslimit

Ablauf für den User: [docs/ARBEITSWEISE.md](docs/ARBEITSWEISE.md). Kosten entstehen pro Schritt: Jeder Schritt liest
den ganzen Verlauf neu (0.2.2: 634 Schritte, 740 K Kontext, ein komplettes 5-Stunden-Limit). Daher:

- **Eine Arbeits-Sitzung pro Version** (0.2.3, 0.2.4 …), frisch gestartet. Der Planungs-Chat („Masterplan“) baut nicht.
- **Obergrenze ca. 250 K Kontext.** Darüber: Übergabe schreiben (siehe unten), User startet eine neue Sitzung.
  Große Versionen lieber in zwei Patches teilen.
- Einarbeitung schlank: CLAUDE.md, ROADMAP „Aktueller Stand“, docs/CODEMAP.md. PROJEKTPLAN, LEBENDIGKEIT und Code
  nur abschnittsweise lesen (Grep/offset), keine Datei zweimal komplett.
- Sparsam arbeiten: Builds und Logs nur über die Filter-Skripte, Screenshots nur zur Sichtprüfung, zusammengehörige
  Änderungen in wenigen Edits bündeln, nicht nach jeder Kleinigkeit neu bauen.
- **Protokoll im Repo, laufend gepflegt:** ROADMAP = was fertig ist (Häkchen), was in Arbeit ist und was ansteht
  („Aktueller Stand“ mit Zeile „Offen“, Checklisten, Backlog); CHANGELOG = was umgesetzt ist; PROJEKTPLAN Kapitel 14
  = Entscheidungen; CODEMAP = wo was im Code liegt. Das Repo ist das Gedächtnis, nicht der Chatverlauf.
- **Übergabe** (am Ende jeder Version, bei der Obergrenze oder wenn der User „Übergabe“ schreibt): erst den laufenden
  Arbeitsschritt sauber abschließen (Code baut, nichts halb geändert), dann auf dem Branch committen und pushen,
  ROADMAP „Aktueller Stand“/„Offen“, CHANGELOG und bei neuen Modulen CODEMAP aktualisieren. Zum Schluss dem User den
  Auftrag für die nächste Sitzung als kopierbaren Textblock geben (Vorlage in ARBEITSWEISE.md).
- **Rahmen halten:** Arbeits-Sitzungen setzen den Auftrag innerhalb von PROJEKTPLAN, LEBENDIGKEIT und dieser Datei um.
  Grundsatzänderungen (Architektur, Umfang, Reihenfolge der Phasen) nicht selbst beschließen, sondern dem User als
  Punkt für den Planungs-Chat nennen und in ROADMAP „Offen“ notieren.
- Arbeits-Sitzungen melden sich nicht beim Planungs-Chat; der User sagt dort kurz Bescheid.
- Denkstufe „high“. „xhigh“ nur nach ausdrücklicher Ansage an den User (Engine-Kern, Simulations-Design, harte
  Timing-/Speicherfehler); dann im Auftrag in der ersten Zeile vermerken.

## Zusammenarbeit

- Mit dem User **Deutsch** sprechen. Doku, UI-Texte und Changelog auf Deutsch.
- Claude ist „Kapitän“: technische Entscheidungen selbst treffen und kurz begründen, Verbesserungen aktiv vorschlagen.
  Geschmacks- und Prioritätsfragen an den User geben (wie, siehe „Mitentscheiden“).
- **Zwischenstände:** nach jedem fertigen Teilschritt 1–3 Zeilen „Neu: …“ in Alltagssprache, ohne Dateinamen. Am
  Sitzungsende vor dem Auftragstext „Das ist neu für dich“ (höchstens 6 Punkte).
- **Mitentscheiden („Nur Wichtiges“):** große Design-, UI- und Inhaltsentscheidungen (Menüpunkte, Farben und
  Standardwerte, Einstellungen und Experimentell-Funktionen, Aussehen von Szenen und Figuren, sichtbare Namen und
  Texte) vor der Umsetzung mit AskUserQuestion vorlegen: 2–4 Optionen, Empfehlung zuerst mit „(Empfohlen)“, bei
  Aussehen mit Vorschau (Screenshot oder Skizze). Bis 4 Fragen bündeln, währenddessen an unabhängigen Teilen
  weiterarbeiten. Kleinigkeiten mit Empfehlung selbst entscheiden und in der Zusammenfassung nennen; Technik weiter
  selbst entscheiden, kurz begründet.
- **Zusatzfunktionen** und zusätzliche Einstellungen als Auswahlliste (Mehrfachauswahl) anbieten, nicht stillschweigend
  einbauen oder weglassen.
- Der User hat keine GitHub-Erfahrung: Git-/GitHub-Schritte erklären und so weit wie möglich automatisieren.
- Hardware-Tests macht der User. Bei visuellen Änderungen Simulator-Screenshots liefern.
- **Das Dorf ist das Herz der Welt und der Schwerpunkt** und soll besonders detailliert und lebendig werden.
- **Lebendigkeit ist das oberste Qualitätskriterium:** [docs/LEBENDIGKEIT.md](docs/LEBENDIGKEIT.md) ist verbindlich
  (Systeme statt Schleifen, Mindestziele pro Kapitel, Langzeit-Test). Nur berichten, was gebaut und geprüft ist; keine
  Versprechen ohne Nachweis.
- Bei GitHub-Aktionen, die der User ausführen muss (z. B. Merge), immer ausdrücklich sagen: was, wo, welcher Knopf.

## Technik (Kurzfassung, Details im Projektplan)

- ESP-IDF + C++20, kein Arduino. Weboberfläche: Svelte 5 + TypeScript + Vite, gzip in die Firmware eingebettet,
  komplett offline (keine CDNs).
- Rendering: 400×240, 8 Bit Palette, 2× hochskaliert im RGB-Bounce-Buffer-Callback. Logik 30 Hz.
  Kern 1 = Grafik/Welten, Kern 0 = WLAN/Web/Hintergrund-Simulation.
- Engine und Welten sind plattformunabhängig (laufen auch im PC-/Browser-Simulator). Keine ESP-IDF-Header in
  `engine/` oder `worlds/`.
- Eine Welt mit gemeinsamem Weltzustand (Uhr, Wetter, Figuren, Warenbuch, Chronik, Aufgaben, Projekte). **Orte und
  Gebiete sind Module** mit fester Schnittstelle; ein Ort greift nie direkt auf einen anderen zu, Verbindungen **nur
  über den Weltzustand und den Ereignis-Bus** ([docs/SETTING.md](docs/SETTING.md) Kapitel 15).
- Inhalte (Ereignisse, Berufe, Gegenstände …) datengetrieben in `assets/`, nicht im Code verstreut.
- Zufall deterministisch mit Startwert.
- Code-Bezeichner Englisch, Kommentare Deutsch.

## Werkzeuge (lokal, Windows)

- ESP-IDF **v6.1** über EIM: IDF in `D:\Espressif\v6.1\esp-idf`, Werkzeuge in `C:\Espressif\tools`.
- **Immer `tools/idf.ps1` statt idf.py direkt** benutzen: aktiviert die Umgebung, schreibt das volle Log nach
  `.logs/idf-last.log` und zeigt nur Fehler, Warnungen und die Zusammenfassung. Beispiel: `.\tools\idf.ps1 -C firmware build`.
  Das Log nur bei Fehlern gezielt lesen.
- Nach Installationen fehlt der neue PATH evtl. in der laufenden Shell: in PowerShell aus Machine+User-Umgebung nachladen.
- **Updates aufs Board bevorzugt per WLAN:** `.\tools\ota.ps1` (Standardziel `http://pixelia/`, sonst `-Target <IP>`,
  `-Build` baut vorher) lädt hoch und prüft danach Version und Selbsttest. USB (`tools/flash.ps1`) nur für die
  Erstinstallation, wenn das WLAN nicht geht, oder fürs serielle Log (`tools/monitor.ps1`).
- **Log und Diagnose per WLAN (seit 0.2.2):** `.\tools\netlog.ps1` (Live-Log mit Filter, `-Send` Befehle, `-Boot prev`
  für den Start vor einem Neustart), Abstürze mit `.\tools\crash.ps1` (holt den Bericht, findet das ELF, dekodiert,
  zeigt das Log davor; `-File` für ein Diagnosepaket vom Handy). Das serielle Log nur, wenn das WLAN nicht geht oder
  bei Abstürzen ganz früh beim Start. Die Task-Liste (`tasks`, `/api/perf?tasks=1`) kostet einen Display-Aussetzer.
- Simulator-Compiler: LLVM-MinGW (`g++`/`clang++`), CMake, Ninja. Node.js 24 für die Weboberfläche.
- Grundsatz: lange Ausgaben (Builds, Logs, serielle Ausgabe) immer über Skripte filtern, nicht roh ins Gespräch holen.

## Leistungsbudget

30 fps stabil (nie unter 25). Logik + Rendern ≤ 20 ms pro Bild. Freier interner RAM ≥ 60 KB, freier PSRAM ≥ 1 MB.
Firmware ≤ 75 % der App-Partition (6 MB).

## Versionen und Changelog

- Immer drei Stellen `MAJOR.MINOR.PATCH`. **MINOR = Phase**, ab 0.6.x je ein Kapitel der Welt (0.6.x = Kapitel 1
  „Der erste Frühling“, 0.7.x = Kapitel 2 „Das Dorf wächst“, …, siehe ROADMAP), **PATCH = jede Iteration** (0.6.0
  erste Version von Kapitel 1, 0.6.1 zweite …). Vorschau-Versionen: `X.Y.Z-beta.N`.
- Version kommt ausschließlich aus dem Git-Tag `vX.Y.Z`. Kein manuelles `FW_VERSION`.
- **Changelog: technisch, aber knapp** (der User fand 0.2.2 mit 40 Stichpunkten zu lang). Er erscheint später im
  Versionsverlauf der Oberfläche; die ausführlichen Details (Ursachen, alle Messwerte) gehören in die PR-Beschreibung.
  Deshalb gilt:
  - `CHANGELOG.json` (neuester oben): `{v, d (Datum), t (Titel), s (Untertitel), n: Neu, i: Verbessert, f: Behoben,
    r: Entfernt, g: Geprüft (am Gerät/im Simulator, mit Messwerten), h: Hinweise, pr: [PR-Nummern]}`.
  - **Jeder PR trägt seine Änderungen sofort ein**, auch Fixes, Doku und Werkzeuge, auch Probleme, die während der
    Entwicklung gefunden und behoben wurden. Ist der oberste Eintrag schon veröffentlicht, legt der PR den Eintrag der
    nächsten Version an (`d`, `t`, `s` leer bis zum Release). PR-Nummer in `pr` ergänzen.
  - Ein Stichpunkt = eine Änderung in einem Satz, **höchstens 220 Zeichen**, mit dem wichtigsten Messwert oder der
    Schnittstelle. **Höchstens 24 Stichpunkte pro Version**; Kleinigkeiten und Entwicklungsfehler zusammenfassen.
    Vorbild: Eintrag 0.2.2. `--check` prüft beide Grenzen.
  - Jeder Stichpunkt beginnt mit dem Bereich: `System:`, `Display:`, `WLAN:`, `Web:`, `Update:`, `Engine:`, `Touch:`,
    `Simulator:`, `Doku:`, `Zug:`, `Dorf:`, `Held:`, `Liga:`, `Insel:` …
  - `python tools/release_notes.py --check` prüft das Format (läuft in der CI). Beim Release `d`, `t`, `s` setzen.
- Experimentelle Funktionen tragen „(Vorschau)“ und liegen hinter dem Experimentell-Schalter.

## Git-Arbeitsweise

- **Zu Beginn jeder Sitzung:** `git fetch`, auf `main` den neuesten Stand holen (es kann Arbeit aus Cloud-Sitzungen oder
  vom Handy dazugekommen sein), dann ROADMAP „Aktueller Stand“ lesen.
- Änderungen auf eigenem Branch → Pull Request → CI grün → Merge durch/nach Freigabe des Users.
  Ausnahme: Solange es noch keine CI gibt (vor 0.1.0), gehen Doku- und Werkzeug-Änderungen direkt auf `main`.
- Commit-Nachrichten auf Deutsch, kurz und beschreibend (z. B. „Zug: Regentropfen an der Scheibe“).
- Release: Tag `vX.Y.Z` auf `main` → GitHub Actions erstellt das Release mit `.bin`, Factory-Image und Release-Notes.
- Keine Geheimnisse ins Repo (WLAN-Daten, Tokens). Fremde Grafiken nur mit CC0/CC-BY-Lizenz, Eintrag in
  `assets/CREDITS.md`.
