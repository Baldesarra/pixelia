# worlds/ – die Welten von Pixelia

Seit 0.5.8 sind Orte und Gebiete Module (`engine/include/pixelia/engine/place.h`): Ein Gebiet ist ein Ordner
hier, jeder Ort eine Datei mit seinen Daten in `<gebiet>/orte/`. Neuer Ort = Datei + eine Zeile in der Ortsliste des
Gebiets, neues Gebiet = Ordner + eine Zeile in `gebiete.cpp`. Heute gibt es das Testdorf (`testdorf/`). Ursprünglicher
Plan der Welten (vor dem Setting, überholt durch die Kapitel in docs/ROADMAP.md):

| Ordner | Welt | Phase |
|---|---|---|
| `train/` | 🚂 Zugfenster | 0.6.x |
| `village/` | 🏘️ Pixel-Dorf (Schwerpunkt) | 0.7.x |
| `hero/` | ⚔️ Dungeon-Held | 0.8.x |
| `league/` | ⚽ Pixel-Liga | 0.9.x |

Regeln (Details in [docs/PROJEKTPLAN.md](../docs/PROJEKTPLAN.md), Kapitel 4.5 und 6):

- **Plattformunabhängig:** kein ESP-IDF-Header. Jede Welt läuft unverändert auf dem Gerät und im Simulator (`sim/`).
- **Feste Schnittstelle:** Jede Welt ist eine `pixelia::Scene` (siehe `engine/include/pixelia/engine/scene.h`):
  Logik-Schritt mit der Weltzeit, Zeichnen in 400×240, Palette.
- **Verbindungen nur über Daten, Umgebung und Ereignis-Bus** (Bus folgt in 0.5.8 Teil 2), nie direkt von Ort zu Ort.
- **Inhalte datengetrieben** in `assets/` (Ereignisse, Berufe, Gegenstände …), nicht im Code verstreut.
- **Zufall deterministisch** mit Startwert (`pixelia::Rng`).
- **Lebendigkeit ist Abnahmekriterium:** Jede Welt erfüllt die Mindestziele aus `docs/LEBENDIGKEIT.md` und belegt
  sie mit einem Langzeit-Test im Simulator (Zeitraffer, Bericht, Kontaktbogen).
