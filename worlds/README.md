# worlds/ – die Welten von Pixelia

Hier entstehen ab Phase 0.6.x die Welten, jede als eigenes Modul:

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
- **Verbindungen nur über den Ereignis-Bus** (kommt in 0.5.x), nie direkt von Welt zu Welt.
- **Inhalte datengetrieben** in `assets/` (Ereignisse, Berufe, Gegenstände …), nicht im Code verstreut.
- **Zufall deterministisch** mit Startwert (`pixelia::Rng`).
- **Lebendigkeit ist Abnahmekriterium:** Jede Welt erfüllt die Mindestziele aus `docs/LEBENDIGKEIT.md` und belegt
  sie mit einem Langzeit-Test im Simulator (Zeitraffer, Bericht, Kontaktbogen).
