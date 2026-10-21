# Pixelia

Pixelia ist ein kleines Dorf auf deinem Schreibtisch, das von selbst lebt.

Im Winter um 1850 strandete ein Zug am einsamen Bahnhof Pixelia, und die Reisenden blieben. Jetzt bauen sie Hütten,
backen Brot, hüten Schafe und flicken die alte Lok, Tag für Tag in echter Zeit. Du schaust zu, gibst Tipps und
entscheidest, was als Nächstes gebaut wird. Alles läuft auf einem ESP32-S3 mit 5″-Display.

> **Status:** Engine-Phase 0.5.x (zuletzt 0.5.3 „Wetter und Partikel“). Bühne, Baukasten, Licht und Wetter laufen am
> Gerät mit Testorten. Die Welt mit ihren Bewohnern beginnt mit 0.6 („Kapitel 1: Der erste Frühling“). Fahrplan:
> [Roadmap](docs/ROADMAP.md).

## Die Welt

**„Der Winter, der nicht endete.“** Ein ungewöhnlich harter Winter hielt einen Personenzug am Haltepunkt Pixelia fest,
irgendwo im Bergland. Hinter dem Bahnhof verschüttete ein Felssturz den Tunnel, vorne lagen die Gleise unter Schnee.
Die Reisenden überlebten in den Wagen. Als der Frühling kam, waren die Gleise zugewachsen, und alle saßen am Feuer
vor dem Bahnhof und beschlossen: Wir bleiben. Das Schild am Bahnsteig gab dem Dorf seinen Namen.

Das Spiel beginnt einige Wochen danach. Der Speisewagen ist die erste Taverne, der Viehwagen der erste Stall, der
Güterschuppen das erste Lager. Die meisten schlafen noch in Abteilen und im Warteraum, auf der Lichtung stehen die
ersten halb fertigen Notunterkünfte. Sechzehn Menschen und eine Katze leben hier, darunter **Sykora**, der
Bürgermeister werden will, und **Taylor**, die Schäferin, die abends in der Taverne singt. Dazu kommen ein Wirt, ein
Schmied, ein Tischler, eine Bäckerin, ein Fischer, ein Jäger, eine Kräuterfrau, ein alter Bergmann und weitere. Jede
Figur hat einen Beruf, ein Zuhause und eine Geschichte. Niemand altert, niemand stirbt, niemand zieht weg; das Dorf
wächst durch Zuzug.

Über Wochen und Monate entstehen Häuser, Werkstätten und Felder, es gibt eine Wahl, die alte Grube am Berg wird
erkundet, und irgendwann fährt die Lok wieder. Das ist keine feste Geschichte, sondern ergibt sich aus der Simulation.
Es gibt keine Magie und kein Geld, alles kommt aus echter Arbeit. Der Betrachter muss nichts tun. Er kann aber über
die Weboberfläche zuschauen, Sykora Tipps geben und entscheiden, was als Nächstes gebaut wird.

## Was Pixelia besonders macht

**Gibt es schon** (Stand 0.5.x, am Gerät gemessen):

- **Pixel-Engine für eine Welt mit vielen Orten:** Jeder Ort ist ein ganzes Bild in 3/4-Draufsicht, ohne Scrollen.
  Gewechselt wird mit Abblenden oder Überblendung, Figuren werden nach Tiefe sortiert, und das Ganze läuft mit 30 fps.
- **Innenraum-Baukasten:** Räume sind Datenlisten aus Teilen, mit Ausbaustufen (Strohsack → Bett), Farbvarianten per
  Palettentausch und dem Zustand „im Bau“.
- **Licht und Wetter:** Tageslicht nach Uhrzeit, Lampen als weiche Lichtkreise, Regen, Schnee, Nebelschwaden, Gewitter
  und Wind mit weichen Übergängen. Nachts leuchten Fenster nur, wenn drinnen Licht brennt.
- **Weboberfläche im Material 3 Expressive Design:** Infos zu Hardware, WLAN und System, Live-Vorschau mit
  Leistungsanzeige, Diagnose, Versionsverlauf und eine Seite „Experimentell“ mit den Testorten.
- **Updates direkt von GitHub** mit Probelauf und automatischem Rückfall auf die alte Version, Fernwartung per WLAN
  (Log, Absturzberichte, Diagnosepaket).

**Geplant** (das Setting steht, gebaut wird ab 0.5.8, siehe [Roadmap](docs/ROADMAP.md)):

- **Bewohner mit eigenem Leben:** Bedürfnisse, ein Tagesablauf mit Zeitfenstern, Berufe mit eigenem Arbeitsrhythmus,
  Fähigkeiten, Beziehungen und eine Romanze. Hinter jeder Entscheidung steht ein echter Grund, den man nachlesen kann.
- **Exaktes Warenbuch:** etwa 70 Waren, jede mit Menge und Ort. Im Bild sieht man sie Stück für Stück, etwa jeden
  Brotlaib in der Auslage. Die Wirtschaft funktioniert ohne Geld als Kette von der Quelle bis zum Verbrauch und wird im
  Zeitraffer über drei Weltjahre geprüft.
- **Gedanken und Briefe:** Was eine Figur gerade denkt, steht auf ihrer Info-Karte und in der Weboberfläche, und Briefe
  haben echten Text.
- **Bauen mit Ausbaustufen:** von der Notunterkunft Raum für Raum bis zum Steinhaus, mit einer Wunschliste, die der
  Betrachter ordnet.
- **Eine Welt, viele Orte:** Bahnhof, Dorf, Bauernhof, die alte Grube und die Strecke. Nur ein Ort ist sichtbar, die
  anderen leben weiter, und neue Orte kommen per Update dazu.
- **Ein Museum als Naturalienkabinett** mit Tieren aus Bach, Wald und Berg und Infokarten mit echten Angaben.
- **Zuverlässiges Gedächtnis:** Nichts verdoppelt sich, nichts verschwindet, auch nicht bei einem Neustart.

## Installieren

Am einfachsten über den **Web-Flasher**: https://baldesarra.github.io/pixelia/ in Chrome oder Edge öffnen, das Board
per USB-C-Datenkabel anschließen und auf „Pixelia installieren“ klicken.

Hardware: **Sunton ESP32-8048S050C_I** (ESP32-S3 N16R8, 5″-Display 800×480, kapazitiver Touch GT911). Geprüfte
Pinbelegung und Messwerte: [HARDWARE.md](docs/HARDWARE.md).

## Einrichten

1. Ohne gespeichertes WLAN startet Pixelia im **Setup-Modus** und zeigt auf dem Display zwei QR-Codes: Der erste
   verbindet das Handy mit dem WLAN „Pixelia-Setup“, der zweite öffnet die Einrichtung (http://192.168.4.1).
2. Dort das eigene WLAN auswählen und das Passwort eingeben. Pixelia merkt sich bis zu 5 Netze und nimmt das stärkste.
   Passwörter bleiben auf dem Gerät.
3. Danach ist Pixelia im Heimnetz unter **http://pixelia.local** erreichbar (an manchen PCs mit FritzBox stattdessen
   http://pixelia/ oder http://pixelia.fritz.box).
4. Die IP-Adresse zeigt jederzeit ein **langer Druck (3 s)** aufs Display, mit QR-Code zur Weboberfläche.

## Updates

- **Von GitHub (seit 0.4.1):** Pixelia sucht einmal am Tag selbst nach neuen Versionen und zeigt einen Hinweis auf der
  Seite **System**. Installiert wird nur auf Knopfdruck, nie automatisch. Wählbar sind die Kanäle „Stabil“ und
  „Vorschau“ (Testversionen).
- **Per Datei:** Auf der Seite **System** die Datei `pixelia-vX.Y.Z.bin` aus den
  [Releases](https://github.com/Baldesarra/pixelia/releases) hochladen.
- In beiden Fällen startet die neue Version zuerst im **Probelauf**. Stürzt sie ab, hängt sie oder besteht sie den
  Selbsttest nicht, kehrt Pixelia automatisch zur alten Version zurück.

## Aufbau

| Ordner | Inhalt |
|---|---|
| `firmware/` | ESP-IDF-Projekt: Hardware-Treiber, Diagnose, WLAN, Webserver, Updates |
| `engine/` | plattformunabhängige Engine: Bühne mit Orten, Raum-Baukasten, Palette und Licht, Wetter, Partikel, Schrift, Systemanzeigen |
| `worlds/` | Orte und Gebiete der Welt (ab 0.6) |
| `sim/` | PC-Simulator mit Fenster und Kopflos-Modus (Screenshots, Zeitraffer, Kontaktbogen) |
| `web/` | Weboberfläche des Geräts (Svelte, offline, gzip in der Firmware) |
| `assets/` | Grafiken, Paletten, Inhalte, [Credits](assets/CREDITS.md) |
| `pages/` | Web-Flasher auf GitHub Pages |
| `tools/` | Hilfsskripte (Bauen, Flashen, Update per WLAN, Log, Absturzberichte, Simulator, Release-Notes) |

## Dokumentation

- [Setting](docs/SETTING.md) und [Wirtschaft](docs/WIRTSCHAFT.md): die Welt, ihre Figuren und Waren
- [Roadmap](docs/ROADMAP.md): aktueller Stand, Systemplan, Phasen, Wünsche
- [Vor 1.0 klären und Zusatzinhalte](docs/ZUSATZINHALTE.md)
- [Projektplan](docs/PROJEKTPLAN.md): Vision, Architektur, alle Entscheidungen
- [Lebendigkeits-Charta](docs/LEBENDIGKEIT.md) und [Stil-Leitfaden](docs/STIL.md)
- [Hardware](docs/HARDWARE.md): geprüfte Pinbelegung, Timings, Messwerte, Diagnose-Befehle
- [Code-Landkarte](docs/CODEMAP.md)
- [Versionsverlauf](CHANGELOG.json)

## Lizenz

Alle Rechte vorbehalten. Fremde Inhalte stehen unter ihren eigenen Lizenzen, siehe [assets/CREDITS.md](assets/CREDITS.md).
