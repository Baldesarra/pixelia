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

## 9. Grafik-Format und Werkzeug (seit 0.5.5)

**Weg einer Grafik:** PNG in `assets/` → `tools/asset_pack.py` → Paket `*.pxa` (eingebettet, LittleFS oder SD) →
Grafik-Speicher im PSRAM → Zeichnen. Fremde Grafik-Pakete gibt es nicht (Entscheidung 2026-10-21); Vorbild für den
Look bleibt die Küche aus Skizze v4 (Kapitel 8).

**Zeichenpalette (`assets/paletten/*.gpl`, GIMP-Palette).** Lässt sich in Aseprite, LibreSprite, GIMP und Krita
laden. Zeile i ist Index i der Grundfarben (Kapitel 2), Zeile 0 heißt „durchsichtig“. Nur benannte Farben sind
Zeichenfarben, jede mit eigenem RGB-Wert. Namensregeln:

- **Rampe** = gleicher Name mit Nummer, hell → dunkel, zusammenhängend: „holz 1“ … „holz 4“ (Kapitel 3).
- **Platzhalter-Rampe** heißt „platzhalter 1“ … „platzhalter 4“: Möbel, Kleidung und alles mit Farbvarianten wird
  damit gezeichnet; eine Farbreihe legt eine andere Rampe darauf.
- **Glasfarben je Raum:** „glas <raum> 1/2“ (Rolle Himmel). Fenster zu verschiedenen Räumen brauchen eigene Glasfarben,
  weil das Wetter je Farbe wirkt (Kapitel 7); ein Raum mit Fenstern reserviert 2 Grundfarben dafür.
- **Wetterfarben:** jeder Außenort reserviert 3–4 Grundfarben: „wetter regen“ (hell, bläulich wie Glas), „wetter
  schnee“ (fast weiß, auch Spritzer), „wetter blatt 1/2“. Die Engine liest sie über `WeatherLook` (Kapitel 7).
- Die Rollen leuchtend, Himmel, fest und die Farbrotation setzt der Ort im Code bzw. ab 0.5.8 in seinen Daten; der
  Name dokumentiert sie („flamme 1“, „wasser 1“ …).

**Bilder sind PNG-Streifen** (Entscheidung des Users 2026-10-22): alle Bilder eines Teils gleich groß nebeneinander,
Zustand für Zustand (z. B. Lampe: aus, an), innerhalb eines Zustands die Animation. Durchsichtig ist jeder Punkt mit
Alpha < 50 %. Ausbaustufen mit anderer Größe sind eigene Bilder („teil/bett/0“, „teil/bett/1“, „teil/bett/bau“).
Der Fußpunkt ist unten mittig, sonst angeben. **Zustandsbilder** (Lampe an/aus, Kamin brennt/aus) schaltet der Ort
über den Zustand des Teils (`Room::set_state`); die Lichtquellen (`LightBoard`) tun das bei Lampe und Laterne, so
braucht nicht jede Lampe eine eigene Leuchtfarbe.

**Farben beim Umwandeln** (`wandeln`, später beim Hochladen in der Weboberfläche, Entscheidung des Users 2026-10-22):

| Modus | Wirkung |
|---|---|
| streng (Standard für `assets/`) | nur Farben der Zeichenpalette; sonst Fehler mit Farbe, Zahl und Lage der Stellen |
| naechste | fremde Farben werden zur nächsten Zeichenfarbe, die Vorschau zeigt das Ergebnis |
| eigene | fremde Farben bleiben (höchstens 16 je Bild); sie stehen im Bild als Indizes 224–239 und die Engine legt sie beim Betreten auf freie Grundfarben des Ortes (Zuordnung kommt mit dem Hochladen, 0.7.x) |

**Wahl der Fassung je Möbel** (Klarstellung des Users 2026-10-22): Beim Hochladen zeigt die Oberfläche beide Fassungen
nebeneinander, die Originalfarben (eigene Farben) und die auf Pixelia-Farben umgerechnete; der User wählt je Möbel,
welche eingebunden wird. Mit dem Werkzeug geht dasselbe heute schon: `wandeln … --farben eigene --vorschau a.png` und
`--farben naechste --vorschau b.png` vergleichen, dann die gewählte Fassung in `pakete.json` eintragen (`"farben"`).

**Pakete (`assets/<ordner>/pakete.json`).** Ein Ordner beschreibt seine Pakete: Palette, Kachelbilder (8 × 8, Kachel
0 leer, `sperren` = versperrende Kacheln), Farbreihen (`{"platzhalter": "holz"}` = Rampe auf Rampe) und Bilder
(`datei`, `bilder`, `zustaende`, `fuss`, `farben`). Namen wie „teil/lampe“ sind im ganzen Spiel eindeutig, die Engine
benutzt ihre Kennung (FNV-1a, 32 Bit). Die Testorte (`assets/testorte/`) zeigen alles: Grundausstattung „grund“
(Palette, Kacheln, Figur, Farbreihen, immer geladen), gemeinsame Möbel „moebel“, Ortspakete „wiese“ und „raum“.

**Format PXA1** (Einzelheiten in `engine/include/pixelia/engine/assets.h`): Kopf mit Prüfsumme (CRC-32), Verzeichnis
mit Kennung, Art, Packung und Größe, Namen, Daten. Gepackt mit PackBits (Testorte: 17 KB → 11 KB, Raum-Möbel auf 39 %),
entpackt wird beim Laden auf Kern 0. Arten: Bild (Maße, Bilder, Zustände, Fußpunkt, eigene Farben, Indizes), Kacheln,
Farbreihe (256 Byte), Palette, Daten.

**Werkzeug** (`python tools/asset_pack.py …`): `build` (läuft im Build von Firmware und Simulator), `check` (CI:
Pakete und `assets/CREDITS.md`), `info`, `zeige` (Bild aus einem Paket als PNG), `wandeln` (ein PNG prüfen und
umrechnen, mit Vorschau). Ohne Fremdbibliotheken.


### Schriften und Symbole (seit 0.5.6)

- Paket „system“ (`assets/system`, Palette `assets/paletten/system.gpl`: durchsichtig, tinte, breite), Grundausstattung. Die Systemschrift 5×7 steht im Code (`engine/src/font.cpp`) und geht immer; fett und klein kommen als Grafik.
- Schrift = PNG-Streifen, ein Bild je Zeichen in der Reihenfolge der Tabelle in font.cpp (ASCII 32–126, dann Sonderzeichen, zuletzt das Ersatzzeichen). Bildhöhe = Zellhöhe + 1; die unterste Zeile markiert in der Farbe „breite“ die Breite des Zeichens (Spalten 0 … Breite − 1), darüber die Glyphe in „tinte“. Fett: Zelle 11 (wie 5×7), klein: Zelle 7 (Zeile 0 Umlautpunkte, 1–5 Zeichen).
- Symbole „ui/symbole“: 9 × 9, einfarbig in „tinte“ (Gedanke, Folgen, Haus, Fund, Neu, Ort).
- Erzeugt mit `python tools/font_png.py` aus font.cpp; danach dürfen die PNGs direkt weitergezeichnet werden (das Werkzeug überschreibt sie beim nächsten Lauf).

## Offen (für 0.6.0)

- Zeichenanleitung für eigene Möbel (Maße im Raster, welche Rampen, Umriss); das Grafik-Format steht seit 0.5.5
  (Kapitel 9).
- Feste Liste der Rampen je Ortsart nach dem Setting (Bahnhof, Hütte, Wald, See …).
- Figuren-Aussehen: Baukasten steht seit 0.5.9 Teil 3 (Stil wie Skizze v3, Version „Mehr Tiefe“: Kopf 14 breit mit runden Ecken, Augen 2 × 3 mit Glanzpunkt, rosa Wangen, Lichtkante links, farbige Kontur nur außen; Ebenen 24 × 36, Fußpunkt (11, 33); Ebenen mit der Platzhalter-Rampe 20–22, erzeugt mit `tools/figure_art.py`). **Statur seit 0.5.9 Teil 5** (Ansatz A „Chibi mit Statur“, Wahl des Users 2026-10-28): Kopf gleich, 4 Größen (−2 bis +4 Zeilen an Rumpf und Beinen), 3 Breiten, Bauch 0–2, gebeugt; der Baukasten streckt die Ebenen beim Zusammensetzen, fertige Bilder 24 × 40 mit Fußpunkt (11, 37). **Figuren sind 25 bis 32 hoch** (Standard 28), Kopfbedeckung darüber; Vorschau `python tools/figure_art.py bogen|gehen`. Offen: Kleidung um 1850 je Beruf und Erkennungsmerkmale der 16 Startfiguren (0.6.0), Haut- und Haar-Rampen in den echten Paletten.
