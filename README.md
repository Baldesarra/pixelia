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

> **Status:** Version 0.2 „Vernetzt“: WLAN mit Setup-Modus, Einrichtungsseite im Browser, Updates ohne USB mit
> automatischem Rückfall. Die Welten folgen ab Version 0.6.

## Installieren

Am einfachsten über den **Web-Flasher**: https://baldesarra.github.io/pixelia/ in Chrome oder Edge öffnen, das Board
per USB-C-Datenkabel anschließen und auf „Pixelia installieren“ klicken.

## Einrichten und Updates

1. Ohne gespeichertes WLAN startet Pixelia im **Setup-Modus** und zeigt auf dem Display zwei QR-Codes: der erste
   verbindet das Handy mit dem WLAN „Pixelia-Setup“, der zweite öffnet die Einrichtungsseite (http://192.168.4.1).
2. Dort das eigene WLAN auswählen und das Passwort eingeben. Bis zu 5 Netze, Pixelia nimmt das stärkste.
   Passwörter bleiben auf dem Gerät.
3. Danach ist Pixelia im Heimnetz unter **http://pixelia.local** erreichbar (an manchen PCs mit FritzBox stattdessen
   http://pixelia/ oder http://pixelia.fritz.box).
4. **Updates:** auf der Seite unter „Update“ die Datei `pixelia-vX.Y.Z.bin` aus den
   [Releases](https://github.com/Baldesarra/pixelia/releases) hochladen. Pixelia prüft die Datei, startet die neue
   Version im Probelauf und kehrt automatisch zur alten zurück, falls sie abstürzt, hängt oder den Selbsttest nicht
   besteht.

## Aufbau

| Ordner | Inhalt |
|---|---|
| `firmware/` | ESP-IDF-Projekt: Hardware-Treiber, Diagnose, WLAN, Webserver, Updates |
| `engine/` | plattformunabhängige Engine: Zeichenfläche, Palette, Weltzeit, Szenen, Schrift, QR-Code, Systemanzeigen, PNG |
| `worlds/` | die Welten (ab 0.6) |
| `sim/` | PC-Simulator mit Fenster und Kopflos-Modus |
| `web/` | Weboberfläche des Geräts (ab 0.3; bis dahin eine einfache Seite in `firmware/main/web/`) |
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
