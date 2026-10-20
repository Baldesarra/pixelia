# Pixelia – Setting und Skript (Arbeitsdokument)

> **Arbeitsdokument der Setting-Werkstatt**, angelegt am 2026-10-20. Hier entsteht das gemeinsame Setting, also das
> „Skript“ der Welt, nach dem spätere Arbeits-Sitzungen bauen. Grundlage sind das Skript des Users
> („Skript Pixelia“, Stand 2026-10-20), seine Antworten in der Werkstatt und Vorschläge von Claude.
> **Inhalt, Ton und Geschmack entscheidet der User; Technik und Machbarkeit schlägt Claude vor.**
>
> Kennzeichnung: **[entschieden]** = vom User bestätigt (steht auch in Kapitel 19). **[Vorschlag]** = Entwurf von
> Claude, wartet auf das OK des Users. **[offen]** = noch zu klären.
>
> Verbindlich bleibt [LEBENDIGKEIT.md](LEBENDIGKEIT.md): Systeme statt Schleifen, Individuen, sichtbare Folgen,
> zuverlässiges Gedächtnis (Regel 10).

## Ablauf der Werkstatt

| Schritt | Thema | Stand |
|---|---|---|
| 1 | Ideen des Users: Grundidee, Ton, Hauptfigur, Skript | Entwurf steht (dieses Dokument), Rückmeldung des Users ausstehend |
| 2 | Gebiete, Figuren, Reihenfolge | Vorschlag in Kapitel 3 und 4 |
| 3 | Modularer Aufbau, Regie, Kamera | Vorschlag in Kapitel 13 und 15 |
| 4 | Hardware-Eignung mit Zahlen (inkl. SD-Karte) | Vorschlag in Kapitel 16 |
| 5 | Rahmenbedingungen für Claude | Vorschlag in Kapitel 17 |
| 6 | Planung: späte 0.5.x-Patches, Phasen ab 0.6.x, PR | offen |

## Inhalt

1. Grundidee, Ton und Epoche
2. Die Hauptfigur und der Betrachter
3. Die Bewohner (Startbesetzung und Wachstum)
4. Gebiete und Kapitel der Welt
5. Figuren: Bedürfnisse, Eigenschaften, Fähigkeiten
6. Entscheidungslogik
7. Tiere
8. Bestand, Wirtschaft und Bauen
9. Zeit, Kalender, Wetter und Licht
10. Ortszustände
11. Tagesrhythmus, Abende und Ereignisse
12. Eingreifen und Weboberfläche
13. Kamera und Regie
14. Perspektive (Platzhalter)
15. Modularer Aufbau und Stufen der Simulation
16. Hardware-Rahmen mit Zahlen
17. Rahmenbedingungen für Claude
18. Offene Fragen
19. Entscheidungen

---

## 1. Grundidee, Ton und Epoche

**Grundidee [entschieden]:** Eine einzige Welt, die von selbst lebt. Figuren versorgen sich, bauen ihr Zuhause aus,
haben Launen, Beziehungen und Gewohnheiten, und die Welt verändert sich langsam über Wochen und Monate. Idle statt
Spiel: Der Betrachter muss nichts tun, kann aber über die Weboberfläche zuschauen, Tipps geben und Wünsche äußern.
Tiefe entsteht aus Zuständen, nicht aus Abläufen. Neue Gebiete kommen mit Updates in dieselbe Welt. Die bisherigen
getrennten Welten (Zugfenster, Dungeon-Held, Pixel-Liga, Inselwelt als eigene Welt) sind **verworfen**; das Projekt
wird inhaltlich neu aufgezogen. Gute Ideen aus den alten Konzepten werden weiterverwendet (vor allem Inselwelt und
Dorf).

**Ton [entschieden]:** gemütlich und bodenständig, warm, natürlich, kleine Reibung ohne Bestrafung. **Keine Magie.**

**Epoche [Vorschlag]: „Frühe Dampfzeit auf dem Land“**, angelehnt an etwa 1840–1860, aber ein erfundenes Land:

- Im Dorf ist der Alltag noch vorindustriell: Laternen in der Hand, Fackeln und Öllampen an den Häusern, Kerzen,
  Kamine, Brunnen, Schmiede mit Esse, Pferdekarren, Handwerk. Das wirkt „mittelalterlich“ und ist trotzdem stimmig,
  denn Dörfer sahen damals noch so aus.
- Die Eisenbahn ist die einzige „moderne“ Technik und kommt von außen. Dampflok, Bahnwärterhaus, Signalmasten.
- Kein Strom, keine Fabriken im Tal. Licht kostet Brennstoff (Öl, Talg, Holz) und ist damit Teil der Wirtschaft.

**Prämisse [Vorschlag]: „Das Dorf am Ende der Nebenbahn“** (Arbeitsname des Dorfs: *Erlengrund*):

- Erlengrund lag an einer kleinen Nebenbahn, die zur Grube im Berg führte. Nach einem Wassereinbruch wurde die Grube
  geschlossen, der Zug fuhr nicht mehr, die meisten Familien zogen weg. Übrig blieben eine Handvoll Leute. Häuser
  stehen leer, der Dorfplatz verwildert, die Schienen wachsen zu.
- Die Hauptfigur erbt den verfallenen Hof ihrer Großmutter am Dorfrand und zieht ein (das ist der ursprüngliche
  „Mensch auf seinem Hof“).
- Danach erwacht das Dorf langsam wieder, nicht durch eine Heldentat, sondern weil alle gemeinsam etwas tun: Häuser
  werden bewohnbar, Leute ziehen zu, die Taverne wird wieder ein Ort für Abende, die Grube wird vorsichtig wieder
  betreten, irgendwann wird die Bahn repariert und fährt hinaus.

Warum diese Prämisse trägt: Sie erklärt die Ausbaustufen (Ruine → bewohnbar → gemütlich), den Zuzug nur bei
Wohnraum, den kaputten Bahnhof und die Grube als Sammelort. Und sie gibt den Updates einen Platz in der Welt: Jede
neue Region liegt hinter der Strecke.

**Bogen über Monate [Vorschlag]**, keine feste Geschichte, sondern Meilensteine, deren Reihenfolge und Tempo aus der
Simulation entstehen:

1. *Ankommen:* Hof bewohnbar machen, Nachbarn kennenlernen, die Katze taucht auf.
2. *Das Dorf erwacht:* Dorfplatz freilegen, Taverne ausbauen, erste Zuzüge, Schwarzes Brett.
3. *Die alte Grube:* mit Wilhelm wieder hinein, Stollen sichern, Kohle und Erz für die Schmiede.
4. *Die Strecke:* Schienen freilegen, Brücke flicken, Lok instand setzen.
5. *Hinaus:* die erste Fahrt in eine neue Region (kommt als Update, siehe Kapitel 4).

**Leitsätze für alle Inhalte:**

- Jede sichtbare Veränderung hat eine Ursache im System.
- Ruhe ist eine Tätigkeit (am Feuer sitzen, dem Regen zuhören).
- Kleine Unfälle ja, Strafen nein. Niemand stirbt an Hunger, nichts verfällt, weil der Betrachter weg war.
- Keine Fremden ohne Geschichte: Jede Figur, die man sieht, hat einen Namen, ein Zuhause oder einen Grund, hier zu sein.

**Offene Fragen:**

- Passt die Epoche, oder soll es noch älter wirken (dann wäre die Dampflok ein Anachronismus) bzw. etwas später?
- Gefällt die Prämisse „verlassenes Dorf erwacht“? Name des Dorfs, des Tals, des Landes (bleibt „Pixelia“ das Land?).

## 2. Die Hauptfigur und der Betrachter

**[entschieden]** Die Hauptfigur ist **eine Figur unter vielen**: keine Superkräfte, keine Sonderwerte, am Anfang
kennt sie kaum jemand. Sie wird am meisten vom Betrachter beeinflusst. Die Kamera folgt ihr überwiegend.

**[Vorschlag] Der Betrachter als Ratgeber:**

- **Tipps an die Hauptfigur** (Web, später auch per Touch): eine Aufgabe („hol Eier“, „geh mit in die Grube“),
  einen Besuch („schau bei Martha vorbei“), ein Geschenk („bring Lene Blumen“), ein Bauprojekt, eine Entscheidung.
- Ein Tipp ist ein **sehr starker Wunsch**, kein Befehl. Die Hauptfigur folgt fast immer, außer ein dringendes
  Bedürfnis oder die Lage spricht dagegen (mitten in der Nacht, Sturm, erschöpft). Dann steht der echte Grund auf der
  Info-Karte („Zu müde. Morgen früh.“) und der Tipp wird später erledigt.
- **Liebe und Freundschaft:** Tipps steuern nur das Verhalten der Hauptfigur. Wie die andere Person reagiert, hängt
  von deren Charakter, Laune und der Beziehung ab. Eine Annäherung braucht Zeit und kann auch scheitern; so bleibt
  sie glaubwürdig.
- **Bitten an andere Figuren:** über ihre Charakterkarte oder das Schwarze Brett. Sie werden angenommen, wenn Zeit,
  Laune und die Beziehung zur Hauptfigur passen. Ein Nein hat einen sichtbaren Grund.
- Gibt der Betrachter nichts vor, lebt die Hauptfigur selbstständig wie alle anderen.
- Die Hauptfigur führt ein **Tagebuch** (wächst sichtbar auf dem Tisch, im Web lesbar).

**Offene Fragen:** Name, Geschlecht, Alter, Aussehen und Charakter der Hauptfigur. Herkunft: Stadtkind, das aufs Land
zieht, oder jemand, der als Kind hier die Sommer verbrachte?

## 3. Die Bewohner (Startbesetzung und Wachstum)

**[entschieden]** Jede Figur ist ein Charakter. Es laufen keine zufälligen Figuren herum, die man nie wieder sieht.
Jede Figur wohnt in einem festen Haus, und jedes Haus ist im Dorf sichtbar.

**[Vorschlag] Startbesetzung: 9 Menschen und eine Katze.** Namen, Alter und Rollen sind Entwürfe.

| Figur | Rolle | Wohnt | Herkunft und Ziel | Bringt ins Dorf |
|---|---|---|---|---|
| Hauptfigur (Name offen) | Neusiedler, lernt alles | Hof am Dorfrand (Erbe) | will ein eigenes ruhiges Leben | Hof, Garten, Tiere, Tagebuch |
| Hedwig, 61 | Wirtin | Bretterbude am Platz (wird Taverne) | blieb, „weil einer das Licht anlassen muss“ | Abende, Essen, Geschichten, Gäste |
| Konrad, 56 | Schmied | Schmiede | seit der Grube ohne Aufträge, grantig und gutherzig | Werkzeug, Reparaturen, bringt Fertigkeiten bei |
| Martha, 72 | Kräuterfrau | Häuschen mit Garten | kennt jede Pflanze im Tal, hält eine Ziege | Kräuter, Medizin, Saatgut, Pflanzenhaus |
| Wilhelm, 66 | Bergmann a. D. | Hütte am Hang | vermisst die Arbeit unter Tage | Wissen über die Grube, Lampen, Sicherheit |
| Paul, 36 | Zimmermann | Haus der Familie Brandt | hält die Familie mit Gelegenheitsarbeit über Wasser | Bauprojekte, Holz |
| Lene, 34 | Weberin, Näherin | Haus der Familie Brandt | will, dass Fritz eine Zukunft hier hat | Kleidung, Stoffe, Decken (Wolle) |
| Fritz, 8 | Kind | Haus der Familie Brandt | neugierig, findet Dinge | Spiele, Fundstücke, Unfug |
| Emil, 47 | Bahnwärter ohne Zug | Bahnwärterhaus am überwucherten Bahnhof | zieht jeden Tag die Bahnhofsuhr auf und träumt von der ersten Fahrt | Bahn-Bogen, Signale, Strecke |
| Katze (Name vom Betrachter) | vollwertige Figur | streunt, später am Hof | erst scheu, wird zutraulich | siehe Kapitel 7 |

**Wiederkehrende Gäste [Vorschlag]**, alle mit Namen und Geschichte, kommen über die Straße und später mit dem Zug:
die Händlerin mit ihrem Karren (alle paar Tage, Tausch), der Postbote aus dem Nachbardorf (Briefe am Schwarzen Brett),
ein Kesselflicker auf Wanderschaft (selten, erzählt in der Taverne).

**Wachstum [Vorschlag]:**

- Neue Bewohner kommen **nur, wenn ein Haus bewohnbar ist**. Vorher kündigt ein Brief am Schwarzen Brett sie an.
- Neue Figuren entstehen **einmal** (aus einer handgeschriebenen Liste oder aus Bausteinen mit Herkunft, Ziel und
  Charakter) und bleiben für immer gespeichert. Wer wegzieht, wird verabschiedet und kann als Gast wiederkommen.
- **Ziel im Endausbau des Tals: 30–40 Bewohner** in 15–20 Häusern, dazu 10–15 wiederkehrende Gäste. Begründung in
  Kapitel 16. Spätere Regionen bringen ihre eigenen Bewohner mit.

**Offene Fragen:** Altern, Geburt, Tod? (Vorschlag: Kinder werden langsam älter, Erwachsene altern sichtbar kaum,
niemand stirbt; Wegzug ist möglich.) Gefällt die Startbesetzung, fehlt eine Rolle (z. B. Bäckerin, Pfarrer, Lehrerin)?

## 4. Gebiete und Kapitel der Welt

**[entschieden]** Dorf bleibt das Herz. Der Zug ist vor allem Reisemittel. Dungeons sind enthalten, aber zum
Rohstoffe-Sammeln, nicht zum Kämpfen. Neue Gebiete kommen mit Updates, wenn dem User etwas fehlt. Orte werden nach
und nach zugänglich.

**[Vorschlag] Übersicht der ersten und wichtigsten Gebiete:**

| Kapitel | Gebiet | Ansichten (Orte) | Zugang | Zweck |
|---|---|---|---|---|
| Start | **Hof der Hauptfigur** | Stube mit Kamin und Küche, Schlafkammer, Hofplatz, Garten, Weide, Bach mit Steg; später Werkstatt, Vorratskeller, Stall | ab Tag 1 | Zuhause, Selbstversorgung, Tiere, Katze |
| Start | **Dorf Erlengrund** | Dorfplatz mit Brunnen, Kerbstein und Schwarzem Brett; Taverne (innen); Wohnhäuser; Schmiede (innen); Marthas Garten; Vorratsscheune | ab Tag 1 | Gemeinschaft, Abende, Handwerk, Bauprojekte |
| Start | **Umgebung** | Waldrand und Wald, Bach und Teich, Wiesen und Feld | ab Tag 1 | Holz, Pilze, Beeren, Wasser, Fisch, Lehm |
| 2 | **Bahnhof** (überwuchert) | Bahnsteig, Bahnwärterhaus, Güterschuppen, Lokschuppen | sichtbar ab Tag 1, Ausbau ab Kapitel 2 | Bahn-Bogen, später Handel und Reisen |
| 3 | **Die alte Grube** | Stolleneingang, Stollen im Querschnitt mit Lampenlicht, Kohleflöz, abgesoffener Teil | nach Werkzeug und Lampen | Kohle, Erz, Stein, Ton, Fundstücke |
| 3 | **Berghang** | Steinbruch, Aussichtspunkt | mit Wilhelm | Stein, Ausflüge, Lieblingsort |
| 4 | **Die Strecke** | zugewachsene Gleise, Brücke, Tunnel, Abteil im fahrenden Zug | nach Werkzeug aus der Schmiede | Reparatur-Bogen, Zugfahrt |
| 5+ | **Neue Regionen (Updates)** | z. B. Marktstadt, Küste, See, Bergdorf | mit dem Zug | wählt der User, wenn es soweit ist |

**Sonderorte [Vorschlag], verstreut und selten:** die Grabstelle eines Unbekannten am Waldrand (ein langes Rätsel
ohne Magie: Fundstücke aus Grube, Wald und Briefen erzählen über Monate, wer das war), ein verlassener Planwagen, die
Ruine auf dem Hügel, eine Höhle hinter dem Wasserfall.

**Die Grube statt Idle-Dungeon [Vorschlag]:** Expeditionen mit einer kleinen Gruppe (Hauptfigur, Wilhelm, wer Lust
und Zeit hat). Kein Kampf. Spannung entsteht aus Licht, Wasser und Gestein: Die Lampe geht zur Neige und man muss
umkehren, ein Stollen ist gesperrt, bis Stützbalken gebaut sind, Wasser steigt, bis eine Pumpe da ist, ein seltener
Fund (Fossil, Kristall, altes Werkzeug eines Bergmanns). Ausbau: Stützbalken, Pumpe, Lore, Leiter, Lampenstation.
Technisches Glanzstück wird die Lichtdarstellung (Lampenkegel in der Dunkelheit).

**Regel für Updates [Vorschlag]:** Jedes Projekt lässt sich mit dem vorhandenen Inhalt abschließen. Was auf ein Update
wartet, ist in der Welt plausibel versperrt (z. B. „hinter dem Tunnel ist die Strecke verschüttet“). Der reparierte
Zug fährt schon vor dem ersten Regionen-Update, nämlich zu einer Nachbarstadt außerhalb des Bildes (Handel, Gäste,
Briefe). Mit dem Update wird diese Stadt zu einem echten Ort.

**Offene Fragen:** Reihenfolge Grube vor Bahn oder umgekehrt? Welche Region soll die erste hinter der Strecke sein?

## 5. Figuren: Bedürfnisse, Eigenschaften, Fähigkeiten

**Bedürfnisse [übernommen aus dem Skript]**, als Antriebe, ohne Balken auf dem Display: Hunger, Energie, Wärme,
Gesellschaft, Neugier, Hygiene. Dazu **Wünsche** („ein Bett statt Stroh“) und **Mangelzustände** („kalt“, „Werkzeug
fehlt“), die in Bauvorschläge einfließen.

**Eigenschaften jeder Figur [übernommen, ergänzt]:**

- Name, Alter, Aussehen (Baukasten), Rolle bzw. Beruf, Zuhause
- Herkunft und Ziel: woher, warum hier, was will sie
- 2–3 **Charakterzüge** (Frühaufsteherin, gesellig, ordentlich, ängstlich, stur …) [Ergänzung]
- **Interessen**, unterschiedlich gewichtet, verändern sich langsam durch Erfahrung
- **Fähigkeiten** (siehe unten)
- **Erinnerungen:** Anlass, Person, Ort, Gegenstand. Einige werden sichtbar (Gegenstand im Regal)
- **Beziehungen:** Stufen fremd → bekannt → vertraut → hilft; dazu Familie, Zuneigung, Rivalität

**Fähigkeiten [Vorschlag]: Alle Figuren haben sie, nicht nur die Hauptfigur.**

- Stufen statt Zahlen: *unbekannt, Anfänger, geübt, Meister*. Jede Stufe schaltet Tätigkeiten frei (Flechten
  geübt → Körbe), es gibt keine reinen Zahlenboni. Intern zählt ein kleiner Erfahrungswert pro Fähigkeit.
- Gelernt wird durch Zuschauen, gemeinsames Arbeiten, Ausprobieren und Gezeigt-Bekommen. Wer einem anderen etwas
  beibringt, kommt ihm dabei näher: Fähigkeiten und Beziehungen wachsen zusammen.
- Etwa 24 Fähigkeiten zum Start, zum Beispiel: Kochen, Backen, Holzarbeit, Zimmern, Mauern, Schmieden, Reparieren,
  Flechten, Nähen, Weben, Gärtnern, Kräuterkunde, Pilzkunde, Tierpflege, Melken, Fischen, Bergbau, Lampen und Licht,
  Erzählen, Musizieren, Lesen und Schreiben, Bahntechnik.
- Kosten: rund 50 Byte pro Figur. Für alle Figuren problemlos.

## 6. Entscheidungslogik

**[übernommen]** Nutzenbewertung statt Skript, Wiederholungsbremse, leichte Streuung, deterministischer Zufall mit
Startwert, Eingriffe aus dem Web als starker Wunsch.

**[Vorschlag] Verbesserung: ein Gerüst plus Nutzenbewertung.** Reine Nutzenbewertung wirkt oft ziellos (Figuren
pendeln zwischen Tätigkeiten). Deshalb drei Ebenen:

1. **Tagesgerüst:** Schlaf, Mahlzeiten, Arbeitszeit nach Rolle, Rituale (morgens Tee, abends Laterne anzünden). Je
   nach Charakter, Jahreszeit und Wetter verschoben.
2. **Aufgaben der Gemeinschaft:** Was das Dorf braucht, steht als Aufgabe auf einer Liste (Holz für die Taverne,
   Balken für die Baustelle, Wasser holen). Wer Zeit und die passende Fähigkeit hat, übernimmt.
3. **Nutzenbewertung** für die Lücken und die Freizeit: Bedürfnisse, Interessen, Beziehungen, Wetter, Tageszeit und
   Wiederholungsbremse.

Jede Entscheidung speichert ihren **Grund** in einem Satz für die Info-Karte. Eingriffe des Betrachters werden mit
Zeitpunkt protokolliert, damit Läufe mit Startwert reproduzierbar bleiben (wichtig für den Langzeit-Test).

## 7. Tiere

**Katze [übernommen]:** vollwertige Figur mit Namen vom Betrachter, eigenem Tagesablauf nach Jahreszeit und Wetter,
wanderndem Lieblingsplatz, Gaben (Blatt, Feder, Maus, Stein), die zu Erinnerungen werden, Eifersucht, eigenen
Beziehungen, einmaligen Momenten (erster Schnee). Kein Altern, aber sie wird mit der Zeit ruhiger und anhänglicher.
[Ergänzung] Sie taucht erst nach einigen Tagen auf und wählt selbst, bei wem sie bleibt (meist bei der Hauptfigur,
wenn die sie füttert, aber nicht zwingend).

**Wildtiere [übernommen]:** wiederkehrende Einzeltiere mit Vertrauensstufen (scheu, wartet, zutraulich, kommt auf
Ruf). [Vorschlag] Fuchs mit hellem Schwanzende, Reh mit Kitz im Frühling, Igel im Herbst unter dem Laubhaufen, eine
Krähe, die Glänzendes sammelt, die Eule am Bahnhof, Glühwürmchen im Sommer.

**Nutztiere [übernommen]:** Hühner, Ziege, Kuh, Schafe mit eigenem Rhythmus, liefern Eier, Milch, Wolle. Keine
Pflichtversorgung durch den Betrachter. [Vorschlag] Nutztiere sind einfache Figuren mit Namen, Farbe und einer Vorliebe
(das Huhn, das immer ausbüxt), ohne volles Gedächtnis.

## 8. Bestand, Wirtschaft und Bauen

**Bestand [übernommen]:** sammeln, lagern, verarbeiten, verbrauchen. Jede Ware hat Menge und Ort. Einzelstücke
(Werkzeug, Möbel, Fundstücke) mit Herkunft und Zustand, gleichartige Waren als Mengen. Lager mit Kapazität.
Verarbeitung verbraucht exakt. Sammelertrag hängt von Ort, Jahreszeit, Ortszustand, Energie der Figur und Zufall ab.
Vorräte sind sichtbar.

**[Vorschlag] Ein Warenbuch für die ganze Welt.** Alle Orte buchen nur darüber. So stimmt die Bilanz auch, wenn Kohle
aus der Grube ins Dorf getragen wird oder Waren mit dem Zug reisen.

**[Vorschlag] Kein Geld am Anfang.** Im fast leeren Dorf gibt es Tausch, Geschenke und ein Gemeinschaftslager in der
Scheune; jeder Haushalt hat zusätzlich ein eigenes Lager. Die Händlerin tauscht gegen Waren von außen. Geld kann
später mit der Bahn kommen.

**Bauen [übernommen]:** Projekte brauchen Material, Arbeit und Zeit, jeder Ausbau verändert das Leben, kein fester
Bauplan, Anpassung aus Nutzung (oft am Feuer → Windschutz). Ausbaustufen für Gebäude, Räume und Gegenstände.

**Bauvorschläge [übernommen, Wunsch des Users]:** Bewohner schlagen Projekte vor (aus Mangel, Wunsch oder Nutzung).
Der Vorschlag hängt am Schwarzen Brett und erscheint im Web. Der Betrachter antwortet mit Ja, Nein oder Später.
**Ohne Antwort nach 24 Stunden entscheiden die Bewohner selbst.**

**[Vorschlag] Dauer in echter Zeit:**

| Größe | Beispiele | Dauer |
|---|---|---|
| klein | Regal, Windschutz, Bank, Zaun flicken | 1–3 Tage |
| mittel | Raum, Werkbank, Hühnerstall, Dorfplatz freilegen | 1–2 Wochen |
| groß | Taverne zur Schenke, Scheune, Haus bewohnbar machen | 3–6 Wochen |
| sehr groß | Lok und Strecke instand setzen | 2–3 Monate (Teilprojekte mit eigenen Zwischenständen) |

**Ausbaustufen im Dorf [übernommen aus dem Skript]:**

| Ort | Stufen | Was er verändert |
|---|---|---|
| Dorfplatz mit Kerbstein und Brunnen | verwildert, freigelegt, gepflastert, Marktplatz | Treffpunkt, Märkte, Feste |
| Taverne | Bretterbude, Schenke, Gasthaus mit Zimmern | Abende, Erzählungen, Gäste übernachten |
| Wohnhäuser (pro Haushalt) | Ruine, bewohnbar, gemütlich, persönlich | Schlafqualität, Wärme, Zuzug |
| Werkstatt, Schmiede | Werkzeugkiste, Werkbank, Esse | Reparaturen, Werkzeug, Ausrüstung für die Grube |
| Pflanzenhaus | Frühbeetkasten, Glasanbau, Gewächshaus | Kräuter, Gemüse im Winter, Saatgut |
| Vorratsscheune | offener Unterstand, Scheune, Keller | Lagerkapazität |
| Bahnhof | überwuchert, Bahnsteig frei, Warteraum, Güterschuppen | Bahnverkehr, Handel |
| Badehaus (spät) | Waschzuber, Badehaus mit Ofen | Hygiene, weniger Krankheit, Gesellschaft |
| Schwarzes Brett | ein Brett, überdachte Tafel | Wünsche, Bauvorschläge, Briefe; Spiegel der Weboberfläche |

[Ergänzung] Der Hof der Hauptfigur bekommt dieselbe Stufenlogik: Feuerstelle → Herd → Küche, Strohlager → Bett,
Zaun → Hühnerstall → Stall.

## 9. Zeit, Kalender, Wetter und Licht

**[Vorschlag]**

- **Tageszeit = echte Zeit.** Mehrere Lichtstufen von Morgen bis Nacht, je eigene Palette und Aktivitätsprofil.
- **Jahreszeiten beschleunigt:** Standard 7 echte Tage pro Jahreszeit (einstellbar), ein Weltjahr = 4 Wochen.
- **Wetter wird simuliert, passend zur Weltjahreszeit**, mit weichen Übergängen (Sonne, Regen, Schnee, Wind, Nebel,
  Gewitter). Echtes Wetter vom Standort passt nicht zu beschleunigten Jahreszeiten (Schnee im Juli); es kann eine
  Option werden, wenn die Jahreszeiten auf „echt“ stehen.
- **Kalender der Welt:** Jahr 1 beginnt mit der Ankunft der Hauptfigur. Feste entstehen aus dem Leben (fertiger Raum,
  erste Ernte, Jahrestag der Ankunft, neue Figur) und aus wenigen Jahresfesten (Frühlingsmarkt, Erntefest,
  Lichterabend zur Wintersonnenwende).
- **Erstlinge** jeder Jahreszeit (erste Schwalbe, erster Schnee) werden einmalig als Erinnerung gespeichert.
- **Licht als System [Ergänzung]:** Laternen, Fackeln, Kerzen und Kamine brauchen Brennstoff aus dem Warenbuch. Das
  abendliche Anzünden ist ein Ritual, ein leerer Ort bleibt dunkel. Das passt zur Epoche und zur Lichttechnik der
  Engine (Lichttabellen).

## 10. Ortszustände

**[übernommen]** Jede sichtbare Veränderung hat eine Ursache:

| Zustand | Verhalten | Sichtbar als |
|---|---|---|
| Feuer, Glut | brennt mit Brennstoff, kühlt langsam ab | Flamme, Glut, Asche, Rauch aus dem Schornstein |
| Licht [Ergänzung] | brennt, solange Öl oder Talg reicht | Fenster hell oder dunkel, Fackel an oder aus |
| Wasser | steigt bei Regen, sinkt bei Verbrauch | Füllstand in Fass oder Brunnen |
| Pflanzen | wachsen nach Jahreszeit, Wasser, Licht | Spross, Blüte, Frucht |
| Nässe | steigt bei Regen, trocknet langsam | Tropfen, Pfützen, dunkles Holz |
| Angefangene Arbeit | bleibt liegen, bis sie fortgesetzt wird | Werkstück auf der Bank |
| Abnutzung | steigt mit Nutzung | ausgetretene Wege, glatte Sitzplätze |
| Verschleiß | Dinge gehen kaputt und werden repariert | wackelnder Stuhl, Loch im Dach, geklebte Tasse |

Ortszustände laufen weiter, wenn niemand hinschaut (Glut kühlt ab, Brot wird alt). Schaltet man in ein leeres Haus,
sieht man genau diesen Zustand: Kamin aus, Fenster dunkel, die Katze allein auf dem Stuhl.

## 11. Tagesrhythmus, Abende und Ereignisse

**[übernommen]** Tags arbeiten, abends zusammensitzen, nachts schlafen. Rituale, die sich mit Jahreszeit und Wetter
verschieben. **Abende als emotionales Herz:** gemeinsames Essen, Gespräche (visuell), Musik, Spiele, Geschichten,
Schweigen; jeder Abend anders. Ereignisse: kleine Unfälle ohne Strafe, Probleme mit Folgen, seltene Beobachtungen,
Freude ohne Zweck, Ruhe als Ereignis. Zeit sichtbar machen: wachsendes Notizbuch, selbst gepflanzter Baum,
Steinhaufen mit Fundstücken.

**[Vorschlag]** Ein **Ereignis-Regisseur** pro Welt (nicht pro Ort) wählt nach Verlauf, Spannungsbogen und Sperrzeiten
aus (LEBENDIGKEIT Regel 6). Ereignisse stehen als Daten in Seltenheitsstufen von häufig bis legendär.

## 12. Eingreifen und Weboberfläche

**[übernommen]** Anzeigen: Charakterkarten (Bedürfnisse, Stimmung, Rolle, Fähigkeiten, Interessen, Beziehungen,
Wünsche), Lager (Waren, Mengen, Lagerorte mit Kapazität, Einzelstücke mit Herkunft), Bauprojekte mit Fortschritt und
Material, Logbuch (vor allem was während der Abwesenheit passiert ist), Live-Vorschau (vorhanden).

Eingreifen: Tipps an die Hauptfigur, Bitten an andere (Kapitel 2), Bauvorschläge beantworten (Kapitel 8), Kamera
umschalten (Kapitel 13), Namen vergeben (Katze, Tiere, Hauptfigur).

**Schwarzes Brett [übernommen]:** die Brücke zwischen Dorf und Weboberfläche. Was dort hängt (Wünsche, Aushänge zu
Projekten, Briefe aus Nachbardörfern), erscheint gleichzeitig im Web. [Ergänzung] Beantwortete Zettel werden im Dorf
abgenommen, so sieht man die eigene Antwort in der Welt.

**[Vorschlag] Touch am Gerät:** Figur antippen → Info-Karte mit Grund; Gebäude antippen → wer ist drin; Wischen →
nächste Ansicht; langes Halten auf einer Figur → „Folgen“.

## 13. Kamera und Regie

**[entschieden]** Die Kamera zeigt überwiegend die Hauptfigur und geht mit ihr auf Reisen. Der Betrachter kann
jederzeit zu jedem bekannten Ort schalten, auch dorthin, wo gerade niemand ist (leeres Haus, Kamin aus).

**[Vorschlag]**

- Modus **„Hauptfigur“** (Standard): Die Kamera folgt ihr von Ort zu Ort. Schläft sie, schwenkt die Regie behutsam zu
  Nachteulen, Tieren oder der Taverne und kehrt morgens zurück.
- Modus **„Frei“**: per Touch oder Web zu jedem Ort; das Dorf hat mehrere Ansichten zum Durchschalten (Platz,
  Häuserzeile, Bahnhof …). Ohne weitere Eingabe nach einer einstellbaren Zeit (z. B. 10 min) zurück zur Hauptfigur.
- Modus **„Folge X“**: jede andere Figur.
- **Kurzmeldungen** von anderen Orten nur bei Wichtigem („Die Gruppe kehrt aus der Grube zurück“), höchstens wenige pro
  Stunde.

## 14. Perspektive (Platzhalter)

**[offen]** Die Entscheidung kommt aus der parallelen Sitzung mit Perspektiv-Beispielen (Hof, Haus, Dorf, Zug, je Tag
und Nacht). Deren Empfehlung: 3/4-Draufsicht für Außenorte, Querschnitt für Gebäude und Zug, ein gemeinsamer
Figurensatz (Figur ≈ 10×22 Pixel, 4 Richtungen), keine Isometrie.

**Wichtig für das Setting:** Viele Ideen leben von **kleinen Dingen** (Stein auf der Fensterbank, geklebte Tasse,
Katze auf dem Werkstück). Bei einer Figur von 22 Pixeln Höhe ist eine Tasse 2–3 Pixel groß. Das geht, wenn
Gegenstände als klare Zeichen mit eigener Farbe gestaltet werden und die Info-Karte bzw. das Logbuch sie erklärt.
Innenräume sollten deshalb eher **größer** dargestellt werden (Querschnitt mit wenigen Räumen pro Bild) als im Maßstab
der Dorfansicht.

## 15. Modularer Aufbau und Stufen der Simulation

**[Vorschlag, Technik]**

Grundsätze:

1. **Welt = gemeinsamer Zustand:** Uhr, Wetter, Figuren, Warenbuch, Chronik, Verbindungen, Aufgabenliste. Gespeichert
   als Ganzes.
2. **Ort = Modul:** bringt Darstellung, Karte, ortstypische Tätigkeiten und Ereignisse als Daten mit, dazu seinen
   Ortszustand (Bauzustand, Beete, Feuer, Grubenstollen). Ein Ort greift nie direkt auf einen anderen zu.
3. **Eine Figur ist immer an genau einem Ort** oder auf genau einer Verbindung. Ein Wechsel ist eine Übergabe in einem
   Schritt, nie eine Kopie.
4. **Verbindungen sind Daten:** Ort A + Punkt, Ort B + Punkt, Art (Tür, Weg, Brücke, Bahnhof, Stolleneingang),
   Reisezeit, Bedingungen (nur bei Tag, erst nach Bau, Zug nach Fahrplan). Fahrzeuge sind bewegliche Orte.
5. **Neues Gebiet ohne Umbau:** neuer Ordner, Daten, eine Zeile Registrierung. Ein alter Speicherstand lädt weiter.

Stufen der Simulation:

| Stufe | Wo | Takt | Was simuliert wird |
|---|---|---|---|
| **Voll** | sichtbarer Ort, Kern 1 | 30 Hz | Wegfindung, Animation, Begegnungen, Mikro-Ereignisse, Kamera |
| **Grob** | alle anderen Orte, Kern 0, niedrige Priorität | ~1 Hz | Tätigkeiten als Zeitblöcke mit derselben Wirkung, Position nur als Zone, Ortszustände |
| **Nachholen** | nach dem Einschalten | einmalig, in Minutenschritten | begrenzt und plausibel (Regel 10) |

Schlüssel: **gleiche Wirkung, andere Darstellung.** Tätigkeiten buchen Waren, Bedürfnisse, Fähigkeiten und
Erinnerungen in beiden Stufen identisch. Schaltet man zu einem Ort, werden die Figuren an die passende Stelle ihrer
laufenden Tätigkeit gesetzt. Pixelgenau gleiche Bewegungen in beiden Stufen sind weder möglich noch nötig; die Bilanz
muss exakt sein. Das ersetzt die Formulierung „Hintergrund- und Live-Simulation liefern dieselben Ergebnisse“
(PROJEKTPLAN 4.5).

Engine-Teile, die das braucht (letzte 0.5.x-Patches): Weltzustand mit Figuren, Warenbuch und Aufgabenliste,
Orts-Graph mit Verbindungen als Daten, Modul-Schnittstelle für Orte, Übergaben zwischen Stufen und Kernen (nur an
Takt-Grenzen, über eine Warteschlange), Regie und Kamera-Modi, Speicherstand als Ganzes, Asset-Quelle Flash oder
SD-Karte.

## 16. Hardware-Rahmen mit Zahlen

**Budget:** 30 fps (nie unter 25), Logik + Zeichnen ≤ 20 ms pro Bild, interner RAM ≥ 60 KB frei, PSRAM 8 MB
(≥ 1 MB frei), 400×240 mit 8-Bit-Palette, App-Partition 6 MB (Firmware ≤ 75 %), Speicher-Partition `storage`
(LittleFS) 3,8 MB, noch ungenutzt.

**Gemessen** (HARDWARE.md): Zeichnen 4,1 ms mit 48 Sprites, 8,4 ms mit 500, 22,6 ms mit 2000 (Puffer im PSRAM); PSRAM
frei ~7,7 MB mit Engine; Flash-Schreiben während der Animation ohne Aussetzer; Firmware heute ~1,5 MB.

**Schätzungen** (Claude, noch nicht gemessen, Größenordnung):

| Posten | Schätzung | Folgerung |
|---|---|---|
| Datensatz pro Figur (Stammdaten, Zustand, Bedürfnisse, Tagesgerüst, Besitz, 24 Fähigkeiten, 24 Beziehungen, 32 Erinnerungen) | 0,5–1 KB | 300 Figuren ≈ 0,3 MB PSRAM: **RAM ist nicht die Grenze** |
| Figurengrafik (Baukasten zur Laufzeit) | Teile-Atlas 0,1–0,3 MB, ~9 KB pro sichtbarer Figur | 40 sichtbare Figuren ≈ 0,4 MB |
| Aktiver Ort (Karte mehrere Bildschirme, Ebenen, Kacheln, Objekte, Wegnetz) | 0,3–1 MB | ein aktiver Ort plus ein vorgeladener Nachbar |
| Inaktiver Ort (nur Ortszustand inkl. Abnutzung pro Kachel) | 2–20 KB | 50 Orte < 1 MB |
| Logik voller Ort (40 Figuren à ~15 µs, 2 Wegsuchen pro Bild à 0,3–1 ms) | 1–3 ms pro Bild | mit Zeichnen 4–8 ms bleibt man unter 12 ms |
| Grobe Stufe (5–20 µs pro Figur, 1 Hz) | 300 Figuren ≈ 6 ms pro Sekunde | < 1 % von Kern 0 |
| Nachholen 8 h aus, Minutenschritte | 480 × 300 Figuren × 20 µs ≈ 3 s | unkritisch, Obergrenze trotzdem nötig |
| Speicherstand gesamt | 0,3–0,6 MB roh, ~0,15–0,25 MB gepackt | passt vielfach in `storage` |
| Flash-Verschleiß (250 KB alle 10 min, 100 000 Löschzyklen, LittleFS verteilt) | ~9 Durchläufe der Partition pro Tag | rechnerisch > 25 Jahre |

**Wie viele Personen? [Vorschlag]** Technisch gehen 300 und mehr. Die echten Grenzen sind andere:

1. **Wiedererkennbarkeit:** Bei etwa 10×22 Pixeln unterscheidet man Figuren an Silhouette, Haaren, Kleidung und
   Farbe. Ein Baukasten liefert hunderte Kombinationen, aber ein Mensch merkt sich ungefähr 30–40 Figuren als
   Persönlichkeiten. (Vergleich: Stardew Valley hat rund 30 Dorfbewohner mit eigenem Tagesablauf.)
2. **Häuser:** Jede Figur wohnt in einem sichtbaren Haus. Ein Dorf über mehrere Ansichten fasst etwa 15–20 Häuser,
   also mit Familien 30–45 Menschen.
3. **Inhalt:** Jede Figur braucht Herkunft, Ziel, Tagesgerüst und eigene Gedanken- und Dialogzeilen (10–20 eigene plus
   allgemeine). Das ist Schreibarbeit pro Figur.

Empfehlung: **Start mit 9, Endausbau des Tals 30–40 Bewohner plus 10–15 wiederkehrende Gäste.** Das deckt auch das
Ziel „≥ 40 Bewohner“ der Charta ab. Spätere Regionen bringen eigene Bewohner, die meist in der groben Stufe laufen.

**Fähigkeitswerte:** alle Figuren (Kapitel 5), rund 50 Byte pro Figur.

**SD-Karte [Vorschlag]:** Das Board hat einen microSD-Halter (SPI, Pins 10/11/12/13 laut Community-Angabe, noch nicht
getestet). Einschätzung:

| Nutzung | Eignung | Begründung |
|---|---|---|
| **Grafiken und Inhalte (Asset-Pakete)** | sehr gut | hebt die eigentliche Grenze auf: In der App-Partition sind neben der Firmware nur ~3 MB für Grafiken frei (reicht für etwa 10–15 Orte), auf einer Karte sind es Gigabyte |
| **Chronik, Logbuch, Tagebuch als Archiv** | sehr gut | jahrelange Geschichte ohne Platzsorgen (~1–5 KB pro Tag) |
| **Sicherungskopien des Speicherstands** | gut | zusätzliche Kopie, z. B. täglich, zum Mitnehmen und Wiederherstellen |
| **Der Speicherstand selbst** | nein | FAT auf SD-Karten verträgt Stromausfälle schlecht; der Spielstand bleibt im internen Flash (LittleFS, stromausfallsicher) |
| **Laden während des Zeichnens** | nein | SPI liest realistisch ~1–2 MB/s; ein Ort (0,5 MB) lädt in ~0,3–0,5 s, also beim Ortswechsel hinter einer Überblendung, auf Kern 0 ins PSRAM |

Vorgehen: Die Engine liest Grafiken von Anfang an über eine **Asset-Quelle**, die Flash oder SD sein kann. Hof und
Dorf passen in den Flash, das Gerät läuft also auch ohne Karte. Die SD-Karte wird sofort für Archiv und Sicherungen
genutzt und später für weitere Gebiete. Neue Inhaltspakete kommen wie Firmware-Updates von GitHub (z. B. 10 MB in
~90 s). Vorher in 0.5.x messen: Lesegeschwindigkeit, keine Bildaussetzer während des Lesens, Verhalten beim Ziehen
der Karte.

## 17. Rahmenbedingungen für Claude

**[Vorschlag]**, gilt nach Bestätigung für alle Arbeits-Sitzungen, die nach diesem Setting bauen.

Arbeits-Sitzungen **entscheiden selbst** (und begründen kurz im PR):

- technische Umsetzung, Datenformate, Algorithmen, Aufteilung auf Kerne, Speicherlayout
- Zahlenwerte für Balance (Dauer, Verbrauch, Wahrscheinlichkeiten), solange der Langzeit-Test sie belegt
- neue Inhalte innerhalb eines beschlossenen Gebiets (Ereignisse, Tätigkeiten, Gedanken, Gegenstände), passend zum
  Ton und zur Epoche
- Nebenfiguren und Gäste mit Namen und Charakter, solange sie zum Setting passen
- Grafikdetails im Rahmen von Stil-Leitfaden und Perspektive

**Zurück an den User** (als Frage oder in ROADMAP „Offen“):

- neues Gebiet, Wegfall eines Gebiets, Reihenfolge der Gebiete und Phasen
- Ton, Epoche, Magie, die großen Bögen (Grube, Bahn, Grabstelle)
- Hauptfigur und Startbesetzung (Namen, Wesen, Schicksal), Altern und Tod
- alles, was diesem Dokument widerspricht oder es ändert
- Abweichung von LEBENDIGKEIT.md oder vom Hardware-Budget

**Immer:** Inhalte als Daten, nicht im Code; Verbindungen zwischen Orten nur über den Weltzustand und den
Ereignis-Bus; nur berichten, was gebaut und im Langzeit-Test geprüft ist; keine anachronistische Technik und keine
Magie.

## 18. Offene Fragen

Nach Dringlichkeit:

1. **Prämisse und Epoche** (Kapitel 1): „Verlassenes Dorf erwacht“, frühe Dampfzeit um 1850?
2. **Hauptfigur** (Kapitel 2): Name, Geschlecht, Alter, Charakter, Herkunft.
3. **Startbesetzung** (Kapitel 3): passt sie, fehlt jemand? Altern, Geburt, Tod?
4. **Reihenfolge** (Kapitel 4): Grube vor Bahn oder umgekehrt? Erste Region hinter der Strecke?
5. **Zeit** (Kapitel 9): Tageszeit echt, Jahreszeit 7 Tage, Wetter simuliert?
6. **Geld** (Kapitel 8): erst Tausch, Geld später?
7. **Nachholen** nach dem Ausschalten: wie lange (Vorschlag: höchstens 24 h)?
8. **Perspektive** je Ort (Kapitel 14), aus der Perspektiv-Sitzung.
9. **Charta:** Die Mindestziele für Zug, Dungeon, Liga und Inselwelt (LEBENDIGKEIT Kapitel 2) passen nicht mehr. Vorschlag:
   Dorf-Ziele bleiben, dazu neue Ziele für Hof, Grube, Bahn und für die Welt als Ganzes.

## 19. Entscheidungen

Fest beschlossene Punkte dieser Werkstatt. Am Ende als Zeilen ins Entscheidungsprotokoll (PROJEKTPLAN Kapitel 14).

| Datum | Entscheidung | Begründung |
|---|---|---|
| 2026-10-20 | **Gemeinsames Setting:** eine Welt mit Gebieten als Modulen, ein sichtbarer Ort voll simuliert, die anderen vereinfacht im Hintergrund, Figuren wechseln zwischen Orten | Idee des Users (ROADMAP-Backlog) |
| 2026-10-20 | **Alte Welten verworfen** (Zugfenster, Dungeon-Held, Pixel-Liga, Inselwelt als eigene Welten); Projekt wird inhaltlich neu aufgezogen, gute Ideen werden weiterverwendet | Entscheidung des Users |
| 2026-10-20 | **Ton:** gemütlich, bodenständig, keine Magie | Entscheidung des Users |
| 2026-10-20 | **Dorf bleibt Herz**; Zug vor allem Reisemittel; Grube bzw. Dungeon zum Sammeln von Rohstoffen, ohne Kampf; neue Gebiete per Update | Entscheidung des Users |
| 2026-10-20 | **Hauptfigur** ist eine Figur unter vielen ohne Sonderkräfte, wird vom Betrachter am stärksten beeinflusst (Tipps), Kamera folgt ihr überwiegend | Entscheidung des Users |
| 2026-10-20 | **Kamera frei umschaltbar** zu jedem bekannten Ort, auch zu leeren Orten mit ihrem echten Zustand | Entscheidung des Users |
| 2026-10-20 | **Feste Bewohner:** jede Figur ein Charakter mit festem, sichtbarem Haus; keine zufälligen Statisten | Entscheidung des Users |
| 2026-10-20 | **Bauvorschläge** der Bewohner im Web beantworten; ohne Antwort nach 24 h entscheiden sie selbst | Wunsch des Users (Skript) |
| 2026-10-20 | **SD-Karte** steht zur Verfügung und darf für Daten und Grafiken eingeplant werden | Angabe des Users |
