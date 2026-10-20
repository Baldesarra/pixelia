# Pixelia – Setting (Arbeitsdokument)

> **Arbeitsdokument der Setting-Werkstatt**, angelegt am 2026-10-20. Hier entsteht das gemeinsame Setting und das
> „Skript“ der Welt, nach dem spätere Arbeits-Sitzungen bauen. Inhalt, Ton und Geschmack entscheidet der User; Technik
> und Machbarkeit schlägt Claude vor. Jedes Kapitel hat **Stand** und **Offene Fragen**. Was hier als „Vorschlag“
> steht, ist noch nicht entschieden. Feste Entscheidungen stehen in Kapitel 12 und kommen am Ende als Zeilen ins
> Entscheidungsprotokoll ([PROJEKTPLAN.md](PROJEKTPLAN.md) Kapitel 14).
>
> Verbindlich bleibt [LEBENDIGKEIT.md](LEBENDIGKEIT.md): Systeme statt Schleifen, Individuen, sichtbare Folgen,
> zuverlässiges Gedächtnis (Regel 10).

## Inhalt

1. Grundidee und Ton
2. Welt und Geografie
3. Figuren
4. Kreisläufe und Wirtschaft
5. Zeit, Kalender und Wetter
6. Orte- und Modul-Katalog
7. Perspektive (Platzhalter)
8. Modularer Aufbau und Stufen der Simulation
9. Hardware-Rahmen mit Zahlen
10. Rahmenbedingungen für Claude
11. Offene Fragen (Sammlung)
12. Entscheidungen

---

## 1. Grundidee und Ton

**Stand:** Idee des Users vom 2026-10-20 (ROADMAP-Backlog): statt getrennter Welten **eine gemeinsame Welt**. Orte sind
Module, die mit Updates dazukommen. Sichtbar ist immer ein Ort, der voll simuliert wird; die anderen laufen vereinfacht
weiter. Figuren wechseln zwischen den Orten: Eine Person geht in den Dungeon, dort sieht man dann die
Idle-Dungeon-Ansicht; der Zug fährt in ein anderes Dorf. **Startidee:** Ein Mensch lebt auf seinem Hof (Haus, Fluss,
Weide).

Was schon feststeht (aus PROJEKTPLAN und Charta):

- Das Gerät ist ein lebendiges Bild zum Nebenbei-Hinschauen, kein Spiel mit Pflichten. Die Welt gerät nicht in Not, nur
  weil niemand hinschaut oder das Gerät aus war.
- Tiefe muss im Bild erkennbar sein, Zähler und Textmeldungen allein reichen nicht.
- Bisherige Ton-Bausteine: Dorf als „Herz“ (warm, gesellig), Inselwelt „cozy“ (bodenständig, Handwerk, Natur, keine
  Magie), Dungeon „Abenteuer im Hintergrund“, Liga „Mitfiebern“, Zug „ruhiger Ausgleich“.

**Offene Fragen:**

- Welcher Grundton trägt die ganze Welt? Gemütlich-bodenständig, märchenhaft, leicht humorvoll, Mischung?
- Gibt es Magie oder Fantastisches? Wenn der Dungeon kommt, braucht er Monster; wie passt das zum Hof-Alltag (z. B.
  „Fantastisches nur unter der Erde“)?
- Epoche und Technikstand: Dampflok und Petroleumlampe, 1950er, heute? Das entscheidet über Zug, Laternen, Fußball,
  Post, Werkzeuge.
- Gibt es eine übergreifende Geschichte (ein Ziel, ein Geheimnis) oder nur entstehende Geschichten aus den Systemen?
- Name der Welt bzw. des Landes: bleibt „Pixelia“?

## 2. Welt und Geografie

**Stand:** Noch keine Karte. Der Hof ist als Startort gesetzt.

**Vorschlag (Technik, Kapitän):**

- Die Welt ist ein **Graph aus Orten**. Ein Ort ist das, was auf einen Bildschirm (oder eine Karte, durch die die
  Kamera fährt) passt und eine eigene Darstellung hat. Ein Ort kann Unterbereiche haben (Zonen, z. B. Küche und Stube
  im Haus).
- **Verbindungen sind Daten**, kein Code. Ein Übergang hat: Ort A + Punkt, Ort B + Punkt, Art (Tür, Weg, Brücke,
  Treppe, Bahnhof, Steg, Höhleneingang), Reisezeit, Bedingungen (z. B. nur bei Tag, Brücke erst gebaut, Zug nur nach
  Fahrplan, Fähre nicht bei Sturm).
- Figuren planen Wege über diesen Graphen (kleiner Graph, Rechenzeit vernachlässigbar) und innerhalb des sichtbaren
  Orts mit Wegfindung auf dem Raster.
- Fahrzeuge (Zug, Boot, Kutsche) sind **bewegliche Orte**: Wer einsteigt, ist am Ort „Zug“, der selbst einen Standort
  auf der Strecke hat.
- Bauen kann Verbindungen erzeugen (Inselwelt: „eine gebaute Brücke verkürzt später einen Weg“), das ist ein starker
  Lebendigkeits-Hebel.

**Offene Fragen:**

- Wie groß ist die Welt am Anfang und wohin wächst sie? Ein Tal, eine Insel, ein Land mit Städten?
- Ist das Haus ein eigener Ort (Tür als Übergang) oder Teil des Hofs? Hängt an der Perspektive (Kapitel 7).
- Wo liegt was: Hof – Dorf – Bahnhof – Dungeon – weitere Städte? (Karte als Artifact-Skizze, sobald Ideen da sind.)
- Gibt es Orte, die nur zu bestimmten Zeiten erreichbar sind (Ebbe, Winter, Fahrplan)?

## 3. Figuren

**Stand:** Aus dem Dorf-Konzept (PROJEKTPLAN 6.2) und der Inselwelt gibt es schon viel Material: Name, Alter, Beruf,
Zuhause, Aussehen aus einem Baukasten, Charakterzüge, Bedürfnisse (Hunger, Energie, Geselligkeit, Spaß, Hygiene),
Beziehungen, Tagesplan + Bedürfnisse entscheiden das Handeln, Gedächtnis (Regel 10), Info-Karte mit echtem Grund.
Inselwelt: ein dauerhafter Bewohner ohne Altern oder Tod, wenige wiederkehrende Bekanntschaften, eine Katze, die
zutraulich wird.

**Vorschlag (Technik, Kapitän):**

- **Figuren gehören der Welt, nicht einem Ort.** Jede Figur ist zu jedem Zeitpunkt an genau einem Ort (oder unterwegs
  auf genau einer Verbindung). Ein Ortswechsel ist eine Übergabe in einem Schritt, nie eine Kopie.
- Handeln über **Tätigkeiten mit Dauer und Wirkung** (z. B. „Holz hacken, 40 min, −1 Stamm, +6 Scheite, Energie −15“).
  Die Wirkung ist in beiden Simulationsstufen gleich, nur die Darstellung unterscheidet sich (Kapitel 8).
- Gedächtnis als Ringpuffer kurzer Einträge (Ereignis, Tag, Wertung, Bezugsperson) plus wenige dauerhafte
  „Lebensereignisse“.
- Haupt- und Nebenfiguren: wenige Hauptfiguren mit voller Tiefe (Tagebuch, Fertigkeiten, Lieblingsorte), viele
  Nebenfiguren mit schlankerem Datensatz. Das spart Speicherstand und Nachholzeit, nicht so sehr RAM.

**Offene Fragen:**

- Wer ist die Hauptperson am Anfang (Name, Alter, Beruf, Charakter)? Bleibt es bei „ein Mensch auf seinem Hof“?
- Altern, Geburt, Tod, Zu- und Wegzug: ja oder nein? (Dorf-Konzept: ja; Inselwelt: kein Altern.)
- Wie viele Figuren soll die Welt am Ende haben, wie viele an einem Ort gleichzeitig?
- Tiere als Figuren mit eigenem Gedächtnis (Katze, Kuh mit Namen) oder als einfachere Wesen?
- Wer geht in den Dungeon: eine feste Heldin, jeder mit dem passenden Charakter, ein Fremder, der ins Dorf kommt?

## 4. Kreisläufe und Wirtschaft

**Stand:** Beschlossen ist die **Aufbau-Wirtschaft** mit begrenzten Rohstoffen, exaktem Verbrauch und Wohnraum als
Voraussetzung für Zuzug (PROJEKTPLAN 6.2) und der **Bilanz-Test** (LEBENDIGKEIT Kapitel 3). Die Inselwelt bringt Hof,
Garten, Tiere, Vorräte, Hofladen und „freie Zeit statt Produktionskette“.

**Vorschlag (Technik, Kapitän):**

- **Ein Warenbuch für die ganze Welt:** Jede Ware hat Menge und Standort (Korb, Lager, Laden, Zug, Rucksack). Orte
  und Module lesen und buchen nur über dieses Buch. So stimmt die Bilanz auch, wenn Waren mit dem Zug reisen oder Beute
  aus dem Dungeon ins Dorf kommt.
- Kreisläufe über Orte hinweg sind der Kern des gemeinsamen Settings, Beispiele zum Prüfen: Hof → Milch und Eier →
  Markt im Dorf → Geld → Werkzeug vom Schmied → besserer Hof; Dungeon → Beute → Händler → Ausrüstung; Zug → Waren und
  neue Bewohner von außen.
- Außenwelt als Quelle und Senke (Händler mit dem Zug), damit nichts verhungert und nichts endlos wächst.

**Offene Fragen:**

- Gibt es Geld oder nur Tausch und Geschenke?
- Was ist der erste Kreislauf, der sichtbar funktionieren muss (Vorschlag: Hof allein, z. B. Holz, Garten, Küche)?
- Wie stark soll Mangel sichtbar sein (leeres Regal, ruhende Baustelle), ohne dass es nach Not aussieht?

## 5. Zeit, Kalender und Wetter

**Stand:** PROJEKTPLAN 4.6/6.0: **gemeinsamer Kalender, gemeinsames Wetter**, Echtzeitbezug (Tageszeit, Jahreszeit,
Wetter von Open-Meteo, Feiertage) abschaltbar; Dienste Weltzeit und Wetter kommen in 0.5.x.

**Vorschlag (Technik, Kapitän):**

- Eine Weltuhr für alle Orte (1 Weltminute = 1 echte Minute als Standard, Zeitraffer im Simulator).
- Wetter pro **Region**, nicht pro Ort: Hof und Dorf im selben Tal haben dasselbe Wetter, eine Stadt am Ende der
  Zugstrecke kann anderes haben. Unter der Erde (Dungeon) gibt es kein Wetter, aber Folgen (nasse Kleidung beim
  Zurückkommen).

**Offene Fragen:**

- Folgt die Welt immer der echten Tages- und Jahreszeit, oder läuft sie schneller (z. B. ein Weltjahr = ein echter
  Monat), damit man Jahreszeiten öfter sieht?
- Echtes Wetter vom Standort oder ein eigenes Weltwetter, das zum Ort passt?
- Eigene Feiertage der Welt zusätzlich zu den echten?

## 6. Orte- und Modul-Katalog

**Stand:** Bisher geplante Welten (PROJEKTPLAN Kapitel 6) und ihre mögliche Rolle im gemeinsamen Setting. Die rechte
Spalte ist ein erster Vorschlag zum Besprechen, nicht entschieden.

| Bisherige Welt | Phase bisher | Mögliche Rolle im Setting (Vorschlag) |
|---|---|---|
| Inselwelt (cozy, ein Bewohner, Hof, Boot, Hofladen) | Position offen | **Kern des Startorts „Hof“**: Haus, Garten, Tiere, Werkstatt, später Hofladen. Viel vom Rohkonzept passt direkt |
| Pixel-Dorf (Bewohner, Berufe, Aufbau-Wirtschaft) | 0.7.x | **Zweiter Ort und Herz**: Markt, Läden, Nachbarn, Bahnhof. Schwerpunkt bleibt die Figuren-Simulation |
| Zugfenster (Parallax-Landschaft) | 0.6.x | **Fahrzeug-Ort**: Wer mit dem Zug fährt, wird im Abteil gezeigt; Strecke verbindet Dorf mit anderen Orten |
| Dungeon-Held (prozedurale Dungeons, Kampf, Beute) | 0.8.x | **Abenteuer-Ort** mit eigener Ansicht; Held wohnt im Dorf, Beute geht ins Warenbuch |
| Pixel-Liga (Spielengine, Saison) | 0.9.x | **Sportplatz-Ort** im Dorf plus abstrakte Liga im Hintergrund (andere Städte nur als Daten) |
| Spätere Ideen (Aquarium, Leuchtturm, Bauernhof …) | nach 1.0 | weitere Orte in derselben Welt (Leuchtturm an der Küste, Aquarium im Rathaus …) |

**Reihenfolge-Vorschlag zum Besprechen:** Hof → Dorf (mit Bahnhof als Endpunkt) → Zug → Dungeon → Sportplatz/Liga.
Begründung: Der Hof testet Figur, Gedächtnis und Warenbuch mit einer Person; das Dorf bringt viele Figuren; erst danach
lohnen Fahrzeuge und Sonderansichten.

**Offene Fragen:**

- Welche der bisherigen Welten bleiben, welche fallen weg oder werden später nachgereicht?
- Bleibt das Dorf der Schwerpunkt (CLAUDE.md, LEBENDIGKEIT), oder wird es der Hof?
- Liga: passt Fußball überhaupt noch in das Setting?
- Was wird aus den Mindestzielen pro Welt (LEBENDIGKEIT Kapitel 2)? Vorschlag: Mindestziele pro Ort plus Ziele für die
  ganze Welt (Wege zwischen Orten, ortsübergreifende Kreisläufe).

## 7. Perspektive (Platzhalter)

**Stand:** Die Entscheidung kommt aus der parallelen Sitzung mit Perspektiv-Beispielen (Hof, Haus, Dorf, Zug, je Tag
und Nacht); der User bringt sie mit. Die dortige Empfehlung lautete: 3/4-Draufsicht für Außenorte (große Karte, Kamera
fährt), Querschnitt für Gebäude und Zug, ein gemeinsamer Figurensatz (Zelle 16×24, Figur ≈ 10×22, 4 Richtungen),
keine Isometrie. **Noch nicht entschieden.**

Für das Setting wichtig: Darf jeder Ort seine eigene Perspektive haben (Hof in 3/4, Haus im Querschnitt, Dungeon mit
Fackelschein, Zug als Abteil)? Technisch ja, solange Figuren in allen Ansichten wiedererkennbar sind.

**Offene Fragen:**

- Welche Perspektive je Ort?
- Wie sieht der Wechsel zwischen Orten aus (Überblendung, Figur geht aus dem Bild, Karte)?

## 8. Modularer Aufbau und Stufen der Simulation

**Stand:** PROJEKTPLAN 4.5 beschreibt eine Modul-Schnittstelle für getrennte Welten mit `backgroundTick()` auf Kern 0
und `catchUp()`. Die ROADMAP verschiebt Modul-Schnittstelle, Regie und gemeinsamen Weltzustand in die letzten
0.5.x-Patches, erst nach diesem Setting.

**Vorschlag (Technik, Kapitän):**

Grundsätze:

1. **Welt = gemeinsamer Zustand:** Uhr, Wetter, Figuren, Warenbuch, Chronik, Verbindungen. Gespeichert als Ganzes.
2. **Ort = Modul:** bringt Darstellung, Karte, ortstypische Tätigkeiten und Ereignisse als Daten mit, dazu seinen
   eigenen Ortszustand (z. B. Bauzustand, Beete, Dungeon-Ebene). Ein Ort greift nie direkt auf einen anderen zu.
3. **Eine Figur ist immer an genau einem Ort.** Wechsel nur über eine Verbindung und als Übergabe in einem Schritt.
4. **Neuer Ort ohne Umbau:** neuer Ordner, Daten für Tätigkeiten und Verbindungen, eine Zeile Registrierung. Ein
   Speicherstand ohne den neuen Ort lädt weiter; Figuren können den Ort ab dem Update benutzen.

Stufen der Simulation:

| Stufe | Wo | Takt | Was simuliert wird |
|---|---|---|---|
| **Voll** | sichtbarer Ort, Kern 1 | 30 Hz | Wegfindung, Animation, Begegnungen, Mikro-Ereignisse, Kamera |
| **Grob** | alle anderen Orte, Kern 0, niedrige Priorität | ~1 Hz bzw. pro Weltminute | Tätigkeiten als Zeitblöcke mit derselben Wirkung, Positionen nur als Zone, Ereignisse des Regisseurs |
| **Nachholen** | nach dem Einschalten | einmalig, Minutenschritte | begrenzt (z. B. höchstens 24 h) und plausibel (Regel 10) |

Der Schlüssel ist **„gleiche Wirkung, andere Darstellung“**: Tätigkeiten buchen Waren, Bedürfnisse und Erinnerungen in
beiden Stufen identisch. Wird ein Ort sichtbar, werden die Figuren an die passende Stelle ihrer laufenden Tätigkeit
gesetzt. Wird er unsichtbar, laufen die Tätigkeiten als Zeitblöcke weiter. Eine pixelgenau gleiche Bewegung in beiden
Stufen ist nicht möglich und nicht nötig; die Bilanz muss exakt sein. Dieser Punkt ändert die Formulierung „Hintergrund-
und Live-Simulation liefern dieselben Ergebnisse“ (PROJEKTPLAN 4.5) und braucht eine Entscheidung.

Ausnahme Liga und Dungeon-Kampf: Dort ist das Ergebnis das Wichtigste. Vorschlag: Das Ergebnis wird immer von derselben
Engine berechnet (wie in 6.4 geplant), die volle Stufe inszeniert es nur.

Engine-Teile, die das braucht (für die letzten 0.5.x-Patches): Weltzustand mit Figurenliste und Warenbuch, Orts-Graph
mit Verbindungen als Daten, Modul-Schnittstelle für Orte (statt Welten), Übergabe von Figuren und Waren zwischen
Stufen und Kernen, Regie (welcher Ort sichtbar ist), Speicherstand als Ganzes.

**Offene Fragen:**

- **Regie: Welcher Ort ist sichtbar?** Folge der Hauptfigur, Wiedergabeliste, Regisseur springt zu interessanten
  Momenten, Auswahl per Touch oder Weboberfläche? Mischung?
- Wie viele Kurzmeldungen von anderen Orten („Tor!“, „Der Held kehrt zurück“) sollen eingeblendet werden?
- Technisch (entscheidet die Engine-Sitzung, hier nur vermerkt): Wer besitzt den Weltzustand, wenn volle Stufe auf
  Kern 1 und grobe Stufe auf Kern 0 laufen? Vorschlag: Übergaben nur an Takt-Grenzen über eine Warteschlange, keine
  gemeinsamen Schreibzugriffe.

## 9. Hardware-Rahmen mit Zahlen

**Budget:** 30 fps (nie unter 25), Logik + Zeichnen ≤ 20 ms pro Bild, interner RAM ≥ 60 KB frei, PSRAM 8 MB
(≥ 1 MB frei), Bild 400×240 mit 8-Bit-Palette, App-Partition 6 MB (Firmware ≤ 75 %), Speicher-Partition `storage`
(LittleFS) 3,8 MB, noch ungenutzt.

**Gemessen** (HARDWARE.md): Zeichnen 4,1 ms mit 48 Sprites, 8,4 ms mit 500 Sprites, 22,6 ms mit 2000 Sprites (Puffer
im PSRAM); PSRAM frei ~7,7 MB mit Engine; Flash-Schreiben während der Animation ohne Aussetzer; Firmware heute ~1,5 MB.

**Schätzungen** (Claude, noch nicht gemessen, Größenordnung):

| Posten | Schätzung | Folgerung |
|---|---|---|
| Datensatz pro Figur (Stammdaten, Zustand, Bedürfnisse, Tagesplan, Besitz, 24 Beziehungen, 32 Erinnerungen) | 0,5–1 KB | 300 Figuren ≈ 0,3 MB PSRAM: **RAM ist nicht die Grenze** |
| Figurengrafik (Baukasten zur Laufzeit, Teile-Atlas gemeinsam) | Atlas 0,1–0,3 MB, Zwischenspeicher pro sichtbarer Figur ~9 KB | 40 sichtbare Figuren ≈ 0,4 MB |
| Aktiver Ort (Karte 3×3 Bildschirme, 3 Ebenen, Kacheln, Objekte, Wegnetz) | 0,3–1 MB | ein aktiver Ort plus ein vorgeladener Nachbar passen gut |
| Inaktiver Ort (nur Ortszustand) | 2–20 KB | 30 Orte < 1 MB |
| Logik voller Ort (60 Figuren à ~15 µs, 2 Wegsuchen pro Bild à 0,3–1 ms) | 1–3 ms pro Bild | mit Zeichnen 4–8 ms bleibt man unter 12 ms |
| Grobe Stufe (5–20 µs pro Figur und Schritt, 1 Hz) | 300 Figuren ≈ 6 ms pro Sekunde | < 1 % von Kern 0, WLAN und Web bleiben unberührt |
| Nachholen 8 h aus, Minutenschritte | 480 Schritte × 300 Figuren × 20 µs ≈ 3 s | beim Start unkritisch, Obergrenze trotzdem nötig |
| Speicherstand gesamt | 0,3–0,6 MB roh, ~0,15–0,25 MB gepackt | passt vielfach in `storage` |
| Flash-Verschleiß (250 KB alle 10 min, 100 000 Löschzyklen, LittleFS verteilt) | ~9 Partitions-Durchläufe pro Tag | rechnerisch > 25 Jahre; wichtige Ereignisse zusätzlich als kleines Journal |

**Realistische Grenzen (Vorschlag):** 100–300 Figuren in der Welt, 40–60 an einem Ort sichtbar, 10–30 Orte.
**Die echte Grenze sind Grafikmenge und Inhalte:** Alle Grafiken liegen in der App-Partition (bei A/B-Updates ~3 MB
Platz neben der Firmware). Jeder Ort mit eigener Optik kostet geschätzt 0,1–0,4 MB gepackte Grafik. Ab etwa 10–15
Orten braucht es Kompression, gemeinsame Kachelsätze oder einen eigenen Grafikbereich mit eigenem Update-Weg. Das ist
heute kein Problem, gehört aber früh in die Asset-Pipeline (0.5.x).

**Speicherstand sicher (Vorschlag):** zwei Dateien im Wechsel mit Prüfsumme und Zähler (wie bei OTA), immer die ältere
überschreiben; Schnappschuss im PSRAM zwischen zwei Simulationsschritten, Schreiben auf Kern 0. Übergaben (Ware
wechselt den Besitzer, Figur wechselt den Ort) sind im Zustand atomar, ein Stromausfall kann also nie „halb“ speichern.

**Offene Fragen:**

- Wie lange soll die Welt nach dem Ausschalten nachholen (gar nicht, ein paar Stunden, einen Tag)?
- Speichern alle 10 min plus bei wichtigen Ereignissen, oder anders?

## 10. Rahmenbedingungen für Claude

**Vorschlag** (wird mit dem User abgestimmt und gilt dann für alle Arbeits-Sitzungen, die nach diesem Setting bauen):

Arbeits-Sitzungen **entscheiden selbst** (und begründen kurz im PR):

- technische Umsetzung, Datenformate, Algorithmen, Aufteilung auf Kerne, Speicherlayout
- Zahlenwerte für Balance (Dauer von Tätigkeiten, Verbrauch, Wahrscheinlichkeiten), solange der Langzeit-Test sie belegt
- neue Inhalte innerhalb eines beschlossenen Orts (Ereignisse, Tätigkeiten, Gedanken, Gegenstände), passend zum Ton
- Nebenfiguren mit Namen und Charakter, solange sie zum Setting passen
- Grafikdetails im Rahmen von Stil-Leitfaden und Perspektive

**Zurück an den User** (als Frage oder als Punkt in ROADMAP „Offen“):

- neuer Ort oder Wegfall eines Orts, Reihenfolge der Orte und Phasen
- Ton, Epoche, Magie ja/nein, übergreifende Geschichte
- Hauptfiguren (Name, Wesen, Schicksal), Altern und Tod
- alles, was das Setting-Dokument ändert oder ihm widerspricht
- Abweichung von LEBENDIGKEIT.md oder vom Hardware-Budget

**Immer:** Inhalte als Daten, nicht im Code; Verbindungen zwischen Orten nur über Weltzustand und Ereignis-Bus; nur
berichten, was gebaut und im Langzeit-Test geprüft ist.

**Offene Fragen:**

- Fehlt etwas auf einer der beiden Listen? Gibt es Inhalte, die der User grundsätzlich selbst schreiben will
  (z. B. Namen, Dialoge)?

## 11. Offene Fragen (Sammlung)

Die wichtigsten Fragen aus allen Kapiteln, nach Dringlichkeit für die Engine-Sitzung:

1. **Regie:** Welcher Ort ist sichtbar und wer entscheidet das? (Kapitel 8, wichtig für die Modul-Schnittstelle)
2. **Gleiche Wirkung statt gleicher Bewegung** in voller und grober Stufe akzeptiert? (Kapitel 8)
3. **Orte-Reihenfolge** und was aus den bisherigen Welten wird. (Kapitel 6)
4. **Ton, Epoche, Magie.** (Kapitel 1)
5. **Hauptperson am Hof** und ob Figuren altern. (Kapitel 3)
6. Zeitlauf: echte Zeit oder schneller? Nachholen nach dem Ausschalten? (Kapitel 5, 9)
7. Perspektive je Ort. (Kapitel 7, aus der Perspektiv-Sitzung)

## 12. Entscheidungen

Fest beschlossene Punkte dieser Werkstatt. Am Ende als Zeilen ins Entscheidungsprotokoll (PROJEKTPLAN Kapitel 14).

| Datum | Entscheidung | Begründung |
|---|---|---|
| 2026-10-20 | Ausgangspunkt: **gemeinsames Setting** mit Orten als Modulen, ein sichtbarer Ort voll simuliert, die anderen vereinfacht im Hintergrund, Figuren wechseln zwischen Orten (Idee des Users, ROADMAP-Backlog) | ersetzt die getrennten Welten; Details folgen in dieser Werkstatt |
