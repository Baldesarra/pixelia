# Credits – fremde Inhalte

Alle Inhalte von Dritten, die in Pixelia verwendet werden. Für Grafiken sind nur Lizenzen **CC0** und **CC-BY**
erlaubt; Schrift und Icons der Weboberfläche sind im Projektplan (4.8) festgelegt.

**Prüfung in der CI (seit 0.5.5):** `python tools/asset_pack.py check` verlangt für jede Datei in `assets/` (außer
`*.md`) eine Zeile in einer der Tabellen: fremde Dateien mit Urheber, Quelle und Lizenz CC0 oder CC-BY (ohne SA, NC,
ND), eigene mit der Lizenz „eigen“. Die erste Spalte darf Platzhalter enthalten (`*`). Fremde Dateien liegen immer
unter `assets/fremd/<Quelle>/`; dort zählt eine Zeile mit „eigen“ nicht.

| Datei/Ordner | Inhalt | Urheber | Quelle | Lizenz |
|---|---|---|---|---|
| – | Noch keine fremden Inhalte. | – | – | – |

## Eigene Inhalte (seit 0.5.5)

| Datei/Ordner | Inhalt | Urheber | Quelle | Lizenz |
|---|---|---|---|---|
| `assets/paletten/*` | Zeichenpaletten (GIMP-Palette, Rampen benannt) | Pixelia | – | eigen |
| `assets/testorte/*` | Grafiken und Paketbeschreibung der Testorte (bis 0.5.4 im Code gemalt) | Pixelia | – | eigen |

## Schrift und Icons der Weboberfläche (seit 0.3.0)

| Datei/Ordner | Inhalt | Urheber | Quelle | Lizenz |
|---|---|---|---|---|
| `web/src/assets/roboto-flex-pixelia.woff2` | Roboto Flex, reduziert auf 135 Zeichen und die Achsen Stärke (300–900) und Breite (75–125), erzeugt mit `tools/font_subset.py` | The Roboto Flex Project Authors | [googlefonts/roboto-flex](https://github.com/googlefonts/roboto-flex) über `@fontsource-variable/roboto-flex` | [SIL Open Font License 1.1](https://openfontlicense.org) (Hinweis steht auch in der Schriftdatei) |
| Icons in `web/src/lib/m3/icon-names.ts` | Material Symbols Rounded (nur die benutzten, als SVG-Pfade eingebettet) | Google | [google/material-design-icons](https://github.com/google/material-design-icons) über `@material-symbols/svg-400` | Apache 2.0 |

## Software-Bausteine

Nicht in `assets/`, der Vollständigkeit halber:

| Baustein | Verwendung | Lizenz |
|---|---|---|
| [ESP-IDF](https://github.com/espressif/esp-idf) | Firmware-Framework | Apache 2.0 |
| [SDL3](https://github.com/libsdl-org/SDL) | Fenster des PC-Simulators | zlib |
| [ESP Web Tools](https://github.com/esphome/esp-web-tools) | Web-Flasher auf GitHub Pages | Apache 2.0 |
| [Svelte](https://svelte.dev) | Weboberfläche (im Bundle) | MIT |
| [Material Color Utilities](https://github.com/material-foundation/material-color-utilities) | dynamische Farben der Weboberfläche (im Bundle) | Apache 2.0 |
| [Vite](https://vite.dev), TypeScript | Build der Weboberfläche (nicht im Bundle) | MIT, Apache 2.0 |
