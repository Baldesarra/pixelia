# Pixelia – Stil-Leitfaden (Entwurf)

> Entwurf seit 0.5.2 („Farbe und Licht“). Grundlage für Farbreihen, eigene Möbel und die echten Grafiken nach dem
> Setting. Das Grafik-Format kommt mit 0.5.5 (Asset-Quelle), fertig ausgebaut wird er in 0.6.0 (Rampen je Ortsart,
> Figuren-Aussehen, Format und Zeichenanleitung für eigene Möbel; Neuplanung 2026-10-21). Technik dazu:
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

## 7. Wetter (seit 0.5.3)

- Jeder Ort legt fest, mit welchen Grundfarben das Wetter gezeichnet wird (`WeatherLook`): Regen (hell, bläulich
  wie Glas), Spritzer und Schnee (fast weiß), zwei Blattfarben. In den Testorten sind das vorhandene Farben; echte
  Orte reservieren dafür 3–4 Grundfarben.
- Regen als Striche, Schnee gemischt (klein und groß), Nebel als Schwaden, die langsam von oben nach unten ziehen
  (dazwischen lichte Stellen, gleichmäßiger Dunst nur schwach), Blitze gedämpft alle 12–30 s (Entscheidungen des
  Users). Ein Blitz hellt im Verhältnis zum Licht auf, nie zu Weiß: Nachts ist er nicht greller als tagsüber.
  Partikel sind fein (2 × 2 Displaypunkte) und stehen im Licht des Ortes.
- Nebel, Regenschleier und Wolkenschatten sind Farbtabellen (`tint_table`) mit feinem Raster. Die Tabellen suchen
  die nächste Grundfarbe: Je mehr Grautöne und dunkle Varianten ein Ort hat, desto weicher wirken sie.
- Innenräume: Das Fensterglas trägt die Rolle Himmel; nur dort erscheinen Regen und Schnee, der Himmel wird grau.
  Fenster nach draußen leuchten nachts nur, wenn im Raum dahinter Licht brennt. Glasfarben gelten je Farbe: Fenster
  zu verschiedenen Räumen brauchen eigene Glasfarben.

## 8. Referenz und Grafik-Quellen

**Referenz für den Look:** die Küche aus [Skizze v4](skizzen/massstab-v4.html) („Maßstab Küche nah“). Der User nennt
sie „sehr gelungen“ (2026-10-21): grobe Pixel, große Figuren, wenig Feinkram, dafür viele kleine Geschichten im Bild
(der Topf dampft, ein Korb Kartoffeln, der Napf für die Katze, abends glüht der Ofen). Dinge der Moderne aus der
Skizze (Kühlschrank, Plattenspieler) gibt es um 1850 nicht.

**Große Fenster mit Fensterbank** (Wunsch des Users 2026-10-22): Ein Fenster ist ein Blick nach draußen, kein
Wandschmuck. Richtwert ist das Küchenfenster der Skizze: etwa 4–5 Kacheln breit (32–40 grobe Bildpunkte, ein Fünftel
der Raumbreite) und fast so hoch, damit man Regen, Schnee, Nebel und Tageszeit draußen gut erkennt. Darunter liegt
eine Fensterbank, breit genug für zwei bis drei Dinge: Kerze (Kerze im Fenster für Heimkehrer, Lichterabend),
Blumentopf, Krug. Sie ist ein Stellplatz im Baukasten. Die Testräume aus 0.5.1 haben Fenster von nur 2 Kacheln
(16 Punkte) und sind deshalb kein Maßstab. Kleine Fenster gibt es nur, wo sie zur Geschichte gehören (Notunterkunft,
Stall, Keller).

**Eigene Grafiken statt fremder Pakete** (Entscheidung des Users 2026-10-21). Geprüft wurden frei verfügbare
Pixel-Art-Pakete gegen unsere Vorgaben (3/4-Draufsicht, grobes Raster 200 × 120, Kachel 8 × 8, Figur 28 hoch in 4
Richtungen, warme Palette mit Rampen). Ergebnis: Kein Paket hat Figuren in unserer Größe; alle setzen auf 16er-Figuren,
und der Stil wirkt dem User zu „low-budget“. Übernommen wird keines. Die Regel für fremde Inhalte (nur CC0 oder CC-BY,
Eintrag in `assets/CREDITS.md`) bleibt für Ausnahmen.

| Quelle | Lizenz | Stil | Eignung | Aufwand beim Umwandeln |
|---|---|---|---|---|
| Kenney: Roguelike Indoors, Tiny Town, Roguelike/RPG | CC0, Nennung freiwillig | 16 × 16, flach, bunt, schlicht | Möbel und Innenräume als Vorlage; Figuren 16 hoch | mittel: umfärben, Kacheln teilen, 3/4-Sicht nachzeichnen |
| Tiny 16: Basic (+ Buildings), Lanea Zimmerman (Sharm) | CC-BY 4.0 / 3.0, OGA-BY 3.0 | 16 × 16, DawnBringer-16-Palette, klassisches RPG-Dorf, Bodenecken in 8 × 8 | Außenkacheln, Bäume, Brücken; Figuren 16 hoch | mittel |
| Ninja Adventure (Pixel-boy, AAA) | CC0 laut Autor (Lizenzdatei ungeprüft, die Seite verlangt eine Bot-Prüfung) | 16 × 16, asiatischer Einschlag, sehr umfangreich, auch Tiere und Töne | einzelne Tiere und Kisten; Thema passt nicht | hoch |
| Zelda-like tilesets and sprites (ArMM1998) | CC0 | 16 × 16, kräftige Farben, außen, innen, Höhle | Höhle als Vorlage für die Grube | mittel bis hoch |
| DawnLike (DragonDePlatino) | CC-BY 4.0 | 16 × 16, Rogue-like mit Monstern und Gegenständen | wenig Passendes (kein Kampf) | hoch |
| Mini Farm (Jofra / Nikuman Studio) | CC0, aber Weitergabe der Dateien untersagt | 16 × 16, Hof mit Tieren | nicht verwendbar: das öffentliche Repo wäre eine Weitergabe | – |
| LPC (Liberated Pixel Cup) | großteils CC-BY-SA 3.0 und GPL 3.0 | 32 × 32, Figuren im 64er-Rahmen | nicht erlaubt | – |
| Sprout Lands, Time Fantasy, Mana Seed u. a. | eigene Lizenzen ohne Weitergabe bzw. kostenpflichtig | gemütlich, oft hübsch | nicht erlaubt | – |

## Offen (für 0.6.0)

- Format und Zeichenanleitung für eigene Möbel (Maße im Raster, welche Rampen, Umriss); das Grafik-Format selbst
  legt 0.5.5 fest.
- Feste Liste der Rampen je Ortsart nach dem Setting (Bahnhof, Hütte, Wald, See …).
- Figuren-Aussehen: Ebenen des Figuren-Baukastens (0.5.9), Kleidung um 1850, Erkennungsmerkmale der 16 Startfiguren.
