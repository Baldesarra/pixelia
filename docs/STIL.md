# Pixelia – Stil-Leitfaden (Entwurf)

> Entwurf seit 0.5.2 („Farbe und Licht“). Grundlage für Farbreihen, eigene Möbel und die echten Grafiken nach dem
> Setting. Ausgebaut wird er in 0.5.8 („Orte und Stil“: Format und Zeichenanleitung für eigene Möbel). Technik dazu:
> `engine/include/pixelia/engine/light.h`, Entscheidungen: [PROJEKTPLAN.md](PROJEKTPLAN.md) Kapitel 14.

## 1. Grundstimmung

Entschieden vom User am 2026-10-21 (Vergleichsbilder im Simulator):

- **Palette warm und gemütlich** passend zu „gemütlich, bodenständig, um 1850“ ([SETTING.md](SETTING.md) Kapitel 1):
  Sättigung etwas zurückgenommen (~90 %), Lichter warm (Rot leicht angehoben, Blau gesenkt), Schatten nicht grau,
  sondern leicht violettbraun angehoben. Keine knalligen Grundfarben, kein reines Schwarz und Weiß in der Szene.
- **Tageszeiten natürlich:** sanftes Gold am Morgen, Orange am Abend, Blau in der Nacht (Vorgabe 0 in `light.cpp`).
- **Nacht mittel:** deutlich Nacht, Figuren und Möbel bleiben erkennbar (Mindesthelligkeit 85/256 im Mittel).
- **Lichtkreise:** hell bis zur Hälfte des Kreises, dann weich auslaufend, im feinen Raster (2 × 2 Displaypunkte).

## 2. Aufbau der Palette eines Ortes

Jeder Ort hat seine eigene Palette mit 256 Einträgen:

| Indizes | Inhalt |
|---|---|
| 0 | durchsichtig (Sprites), im Hintergrund nicht verwenden |
| 1 … 79 | **Grundfarben**: damit wird gezeichnet (80 je Lichtstufe bei drei Stufen) |
| 80 … 159 | Lichtstufe 1 (halbes Lampenlicht), rechnet die Engine aus |
| 160 … 239 | Lichtstufe 2 (volles Lampenlicht), rechnet die Engine aus |
| 240 … 255 | **System-Oberfläche** (Einblendungen, Text). Tag/Nacht und Farbrotation verändern sie nie |

Gezeichnet wird nur mit den Grundfarben. Die Lichtstufen entstehen aus ihnen (Farbe × Umgebungslicht + Lampenlicht).
Ein Ort mit weniger Licht kann auch zwei oder eine Stufe nutzen (dann 120 bzw. 240 Grundfarben).

## 3. Farbrampen

Grundfarben sind in **Rampen** geordnet: 3–4 Farben eines Materials von hell nach dunkel, nebeneinander im Index
(z. B. Holz 64–67). Regeln:

- **Hell → dunkel aufsteigend** im Index (erster Eintrag = Licht, letzter = Schatten).
- **Farbton wandert** über die Rampe: Lichter wärmer (Richtung Gelb), Schatten kühler bzw. violettbraun. Nicht nur
  heller/dunkler mischen.
- **Platzhalter-Rampe** (graue Rampe 20–23 in den Testorten): Möbel, Kleidung und alles mit Farbvarianten wird einmal
  damit gezeichnet. Eine **Farbreihe** (Umfärbe-Tabelle, 256 Byte) tauscht sie beim Zeichnen gegen eine echte Rampe
  (Holz hell/dunkel, Stoff rot/blau …). Das kostet keinen Speicher je Farbe und keine Rechenzeit.
- Rampen gleicher Länge sind austauschbar: Eine Farbreihe kann jede 4er-Rampe auf jede andere legen.

Aufteilung der 80 Grundfarben (Richtwert, so in den Testorten):

| Bereich | Anzahl | Inhalt |
|---|---|---|
| Umriss, Dunkel, Schatten | ~4 | Umrisslinie, Türöffnung, Fugen |
| Boden, Wand, Weg, Stein, Gras | ~20 | Materialien des Ortes (je 3–4) |
| Platzhalter-Rampe | 4 | wird per Farbreihe ersetzt |
| Farbreihen | ~32 | Kleidung, Holz, Tapete, Stoffe (je 4) |
| Haut, Haare, Augen | ~7 | Figuren |
| Sonderrollen | ~8 | Fensterglas/Himmel, Wasser, Flamme (siehe 4) |
| frei | Rest | Reserve für den Ort |

## 4. Sonderrollen einzelner Farben

Die Engine kennt neben normalen Farben drei Rollen (`ColorRole`):

- **leuchtend** (Fenster am Abend, Lampenschirm, Flamme): am Tag die gezeichnete Farbe unter Tageslicht, mit der
  Dunkelheit wird sie zur Nachtfarbe (warmes Licht) statt dunkler.
- **Himmel**: eine Rampe, die die Engine nach der Tageszeit als Verlauf von oben nach unten füllt (Fenster von innen).
  Übergänge zwischen den Stufen im 4 × 4-Raster zeichnen.
- **fest**: immer gleich (z. B. Markierungen).

**Farbrotation** (Wasser, Feuer): eine Rampe dreht sich alle n Logik-Schritte um einen Platz. Wasser als schräge
Streifen aus 4 Farben zeichnen, die Rotation lässt es fließen.

## 5. Licht

- **Tageslicht** kommt aus der Tageszeit (Weltzeit ab 0.5.6, bis dahin die Testuhr auf der Seite Experimentell).
  Innenräume bekommen weniger Umgebungslicht (Testraum 86 %).
- **Lampen** sind Lichtkreise am Boden (Ellipse wegen der 3/4-Sicht). Bis zur Hälfte volles Licht, dann weich
  auslaufend; die Engine rechnet das Raster beim Betreten und wendet es beim Verdoppeln an. Figuren im Licht werden
  gleichmäßig nach ihrem Fußpunkt hell, ohne Raster auf der Figur.
- Lampen wirken nur bei Dunkelheit. Tagsüber sind alle Lichtstufen gleich und kosten keine Rechenzeit.
- **Nicht verwechseln:** Die Dunkelheit der Palette ist Teil des Bildes. Die Helligkeit der Hintergrundbeleuchtung
  (Nachtmodus, Helligkeitskurve) ist etwas anderes und kommt mit 0.5.6. Beide zusammen dürfen die Nacht nicht
  unlesbar machen.

## 6. Schatten und Transparenz

- Schatten der Figuren sind durchscheinend: Farbtabelle „jede Farbe ein Stück Richtung Dunkelviolett“ (Index → Index),
  so bleibt Gras unter dem Schatten Gras.
- Für Nebel, Regen und Glas gibt es Tönungstabellen und eine 50-%-Mischtabelle der Grundfarben (`PlacePalette`).

## Offen (für 0.5.8)

- Format und Zeichenanleitung für eigene Möbel (Maße im Raster, welche Rampen, Umriss).
- Feste Liste der Rampen je Ortsart nach dem Setting (Bahnhof, Hütte, Wald, See …).
