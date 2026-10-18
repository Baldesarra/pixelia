# Pixelia

Lebendige Pixel-Welten für den Schreibtisch: ein Idle-Projekt für das **ESP32-8048S050C** (ESP32-S3, 5"-Display
800×480, kapazitiver Touch).

Auf dem Display laufen nebenbei kleine Welten, in denen ständig etwas passiert:

- 🚂 **Zugfenster**: Landschaften, Bahnhöfe, Tag- und Wetterwechsel
- 🏘️ **Pixel-Dorf**: Bewohner mit Tagesablauf, Berufen, Beziehungen und Ereignissen
- ⚔️ **Dungeon-Held**: ein Idle-RPG-Held erkundet zufällige Dungeons
- ⚽ **Pixel-Liga**: Fußballliga mit KI-Teams, Tabelle und Live-Spielen

Dazu kommt eine Weboberfläche im Material 3 Expressive Design mit vielen Einstellungen, Hardware-Infos und Updates
direkt von GitHub.

> **Status:** Version 0.1 „Fundament“: Hardware geprüft, Engine-Prototyp mit 30 fps, Simulator. Die Welten folgen ab
> Version 0.6.

## Installieren

Am einfachsten über den **Web-Flasher**: https://baldesarra.github.io/pixelia/ in Chrome oder Edge öffnen, das Board
per USB-C-Datenkabel anschließen und auf „Pixelia installieren“ klicken.

## Aufbau

| Ordner | Inhalt |
|---|---|
| `firmware/` | ESP-IDF-Projekt: Hardware-Treiber, Diagnose, später Netzwerk, Webserver, Updates |
| `engine/` | plattformunabhängige Engine: Zeichenfläche, Palette, Weltzeit, Szenen, PNG |
| `worlds/` | die Welten (ab 0.6) |
| `sim/` | PC-Simulator mit Fenster und Kopflos-Modus |
| `web/` | Weboberfläche des Geräts (ab 0.3) |
| `assets/` | Grafiken, Paletten, Inhalte, [Credits](assets/CREDITS.md) |
| `pages/` | Web-Flasher auf GitHub Pages |
| `tools/` | Hilfsskripte (Bauen, Flashen, Log, Simulator, Release-Notes) |

## Dokumentation

- [Projektplan](docs/PROJEKTPLAN.md): Vision, Architektur, alle Entscheidungen
- [Roadmap](docs/ROADMAP.md): aktueller Stand, Versionen, Wünsche
- [Hardware](docs/HARDWARE.md): geprüfte Pinbelegung, Timings, Messwerte, Diagnose-Befehle
- [Versionsverlauf](CHANGELOG.json)

## Lizenz

Alle Rechte vorbehalten. Fremde Inhalte stehen unter ihren eigenen Lizenzen, siehe [assets/CREDITS.md](assets/CREDITS.md).
