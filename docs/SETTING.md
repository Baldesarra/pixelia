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
| 1 | Ideen des Users: Grundidee, Ton, Hauptfiguren, Skript | weitgehend entschieden; offen: Ideenliste, Gründungsgeschichte und Herkunft prüfen |
| 2 | Gebiete, Figuren, Reihenfolge | Vorschlag in Kapitel 3 und 4 |
| 3 | Modularer Aufbau, Regie, Kamera | Vorschlag in Kapitel 13 und 15 |
| 4 | Hardware-Eignung mit Zahlen (inkl. SD-Karte, Baukasten) | Vorschlag in Kapitel 16 |
| 5 | Rahmenbedingungen für Claude | Vorschlag in Kapitel 17 |
| 6 | Planung: späte 0.5.x-Patches, Phasen ab 0.6.x, PR | offen |

## Inhalt

1. Grundidee, Ton und Epoche
2. Die Hauptfiguren und der Betrachter
3. Die Bewohner (Startbesetzung und Wachstum)
4. Gebiete und Kapitel der Welt
5. Figuren: Bedürfnisse, Eigenschaften, Fähigkeiten
6. Tagesgerüst und Entscheidungslogik
7. Tiere
8. Bestand, Wirtschaft und Bauen
9. Zeit, Kalender, Wetter und Licht
10. Ortszustände
11. Abende und Ereignisse
12. Eingreifen und Weboberfläche
13. Kamera und Regie
14. Perspektive
15. Modularer Aufbau und Stufen der Simulation
16. Hardware-Rahmen mit Zahlen
17. Rahmenbedingungen für Claude
18. Offene Fragen
19. Entscheidungen

---

## 1. Grundidee, Ton und Epoche

**Grundidee [entschieden]:** Eine einzige Welt, die von selbst lebt. Figuren versorgen sich, bauen ihr Zuhause aus,
haben Launen, Beziehungen und Gewohnheiten, und die Welt verändert sich langsam über Wochen und Monate. Idle statt
Spiel: Der Betrachter muss nichts tun, kann aber über die Weboberfläche zuschauen, Tipps geben und entscheiden. Tiefe
entsteht aus Zuständen, nicht aus Abläufen. Neue Gebiete kommen mit Updates in dieselbe Welt. Die bisherigen
getrennten Welten (Zugfenster, Dungeon-Held, Pixel-Liga, Inselwelt als eigene Welt) sind **verworfen**; das Projekt
wird inhaltlich neu aufgezogen, gute Ideen aus den alten Konzepten werden weiterverwendet.

**Ton [entschieden]:** gemütlich und bodenständig, warm, natürlich, kleine Reibung ohne Bestrafung. **Keine Magie.**

**Epoche [entschieden]: frühe Dampfzeit auf dem Land, um 1850**, in einem erfundenen Land:

- Im Dorf ist der Alltag vorindustriell: Laternen in der Hand, Fackeln und Öllampen an den Häusern, Kerzen, Kamine,
  Brunnen, Pferdekarren. **Schmiede und Tischlerei sind echte Handarbeit**, nichts ist automatisiert, die Bewohner
  arbeiten sichtbar.
- Die Eisenbahn ist die einzige „moderne“ Technik und kommt von außen: Dampflok, Bahnwärterhaus, Signale.
- Kein Strom, keine Fabriken im Ort. Licht kostet Brennstoff (Öl, Talg, Holz) und ist Teil der Wirtschaft.
- Dinge heißen wie damals: Kisten und Säcke statt Kartons, Strohsack und Decken statt Schlafsack, Sofa oder Ofenbank
  statt Couch.

**Das Dorf heißt Pixelia [entschieden].**

**Vorgeschichte [entschieden, Idee des Users]: „Der Winter, der nicht endete.“**
Ein ungewöhnlich harter Winter zwang die Reisenden eines Zuges, im kleinen Bahnhof Pixelia zu bleiben. Als der
Frühling kam, waren die Gleise hinter dem Dorf so zugewachsen, dass niemand mehr weiterkonnte. Sie blieben, weil Gehen
damals schwerer war als Bleiben. Der Zug ist durch die fehlende Benutzung kaputt und verrostet.

**Gründungsgeschichte [Vorschlag], wie aus Gestrandeten ein Dorf wird:**

- **Vor dem Winter** war Pixelia nur ein einsamer Haltepunkt an einer Nebenstrecke durchs Bergland: Bahnhofsgebäude
  mit Warteraum, Wasserturm, Lokschuppen, ein Güterschuppen. Ringsum Wald, ein See, Wiesen, oben am Berg eine längst
  aufgegebene Grube. Nur zwei Menschen lebten in der Nähe: Martha, die Kräuterfrau, in ihrer Hütte am Waldrand, und
  Hannes, der Fischer, am See.
- **Der Winter:** Ein Personenzug mit Abteilwagen, Speisewagen, Gepäckwagen und einem Viehwagen bleibt im Schnee
  stecken. Hinter dem Haltepunkt verschüttet ein Felssturz den Tunnel, durch den der Zug gekommen ist; nach vorne
  liegen die Gleise unter meterhohem Schnee. Die Reisenden überleben in den Wagen und im Warteraum. Gustav, der Koch
  des Speisewagens, kocht für alle, Anton jagt, Martha kennt die Kräuter gegen Husten, Hannes hackt Löcher ins Eis.
  Emil, der Lokführer, fühlt sich für alle verantwortlich und hält sie zusammen.
- **Der Frühling:** Der Schnee taut, doch nach vorne sind die Gleise zugewachsen und unterspült, nach hinten bleibt der
  Tunnel verschüttet. Über den Bergpass kommt man nur zu Fuß, tagelang, und nur im Sommer. Am Abend nach der ersten
  warmen Nacht sitzen alle am Feuer vor dem Bahnhof und entscheiden: Wir bleiben. Das Schild am Bahnsteig gibt dem Dorf
  seinen Namen: **Pixelia**.
- **Hier beginnt das Spiel [entschieden]: einige Wochen nach dem Entschluss zu bleiben.** Es gibt den Bahnhof, den
  gestrandeten Zug, Marthas und Hannes' Hütten und die ersten halb fertigen Notunterkünfte auf der Lichtung. Die
  meisten schlafen noch provisorisch: in Abteilen, im Warteraum, im Gepäckwagen. Der **Speisewagen ist die erste Taverne**, der **Viehwagen der erste Stall** für Taylors Schafe, der
  **Güterschuppen das erste Lager**. Jedes Haus, jede Werkstatt und Sykoras Hof müssen erst gebaut werden.
- **Warum der Zug wieder fahren soll:** Niemand will mehr weg. Die Lok wird repariert, damit Pixelia **angebunden**
  ist: Waren, Gäste, Briefe, neue Bewohner. Erst fährt sie bis zur nächsten Stadt hinter den zugewachsenen Gleisen,
  später (mit Updates) weiter; irgendwann wird vielleicht auch der Tunnel wieder frei.
- **Verbindung nach außen bis dahin:** der Bergpass. Im Sommer kommt die Händlerin mit ihrem Maultier und bringt Post
  und Tauschware, selten ein Wanderer. Im Winter ist Pixelia ganz für sich.

**Bogen über Monate [Vorschlag]**, keine feste Geschichte, sondern Meilensteine, deren Reihenfolge und Tempo aus der
Simulation entstehen:

1. *Der erste Frühling:* (Spielstart einige Wochen nach dem Entschluss) Feuerstelle vor dem Bahnhof, Schlafplätze in
   Wagen und Warteraum, erste Notunterkünfte werden fertig, erster Garten. Die Katze taucht auf.
2. *Die ersten Häuser:* Notunterkünfte entstehen, der Speisewagen wird zur Taverne, Schwarzes Brett am Bahnsteig,
   Werkstätten (Schmiede, Tischlerei, Backofen).
3. *Die Wahl:* Sykora wird Bürgermeister (Kapitel 2).
4. *Die alte Grube:* mit Wilhelm hinein, Stück für Stück tiefer (Kapitel 4).
5. *Die Lok:* Instandsetzung in vielen Teilaufgaben (Kapitel 8), Gleise freilegen.
6. *Hinaus:* die erste Fahrt bis zur nächsten Stadt (kommt als Update).

**Leitsätze für alle Inhalte:**

- Jede sichtbare Veränderung hat eine Ursache im System.
- Ruhe ist eine Tätigkeit (am Feuer sitzen, dem Regen zuhören).
- Kleine Unfälle ja, Strafen nein. Niemand hungert, nichts verfällt, weil der Betrachter weg war.
- Keine Fremden ohne Geschichte: Jede Figur hat einen Namen und ein Zuhause oder einen Grund, hier zu sein.

## 2. Die Hauptfiguren und der Betrachter

### Sykora [entschieden, Details Vorschlag]

- **Mann, 30, freundlich**, auf dem Land aufgewachsen.
- Will die Zügel in die Hand nehmen und das Dorf nach vorne bringen; **bewirbt sich als Bürgermeister**.
- **In Taylor verliebt.**
- Eine Figur unter vielen: keine Sonderkräfte, keine Sonderwerte. Er muss lernen, helfen und Vertrauen gewinnen.
- [Vorschlag] War auf dem Weg in die Stadt, um dort Arbeit zu suchen, und saß im gestrandeten Zug. Schläft am Anfang
  in einem Abteil; **sein eigener Hof am Dorfrand ist sein erstes großes Projekt**. Startfähigkeiten: Holzarbeit geübt, Tierpflege und Gärtnern Anfänger, Lesen und Schreiben geübt (wichtig fürs Amt).
  Führt ein Tagebuch, das sichtbar auf dem Tisch wächst. Schwäche: will zu viel auf einmal und vergisst dabei Pausen.
- Erkennungsstücke: Tagebuch, Hut am Haken, später die Amtskette bzw. das Bürgermeisterbuch.

### Taylor [entschieden, ausführliches Profil Vorschlag]

Taylor ist neben Sykora die zweite Hauptfigur und bekommt deshalb das ausführlichste Profil.

- **Frau, 30, blond, hübsch, freundlich, tierlieb.** Singt gerne in der Taverne und spielt dort ab und zu Gitarre.
- **Beruf [entschieden]: Schäferin.** Hält Ziegen und Schafe am Dorfrand, liefert Milch und Wolle.
- **Gefühle [entschieden]: Der Funke ist schon da.** Sie ist heimlich auch interessiert. Die Romanze ist ziemlich
  sicher, offen sind Weg und Tempo.
- **Wie sie zusammenkommen [entschieden]:** Grundlage ist eine **gemeinsame Wintererinnerung**: Sykora hat ihr in den
  kalten Nächten im Viehwagen geholfen, die Schafe warmzuhalten; beide wissen es, keiner spricht darüber. Darauf bauen
  zwei mögliche Meilensteine auf, die die Simulation je nach Beziehung auslöst: **das verlorene Lamm** (Sturmnacht, ein
  Lamm fehlt, Sykora sucht und findet es; einzige Ausnahme von „nachts arbeitet niemand“) und **der erste Tanz** beim
  ersten Dorffest (z. B. Jahrestag des Bleibens).
- **Aussehen [entschieden]:** **immer langes, offenes blondes Haar, nie eine Kopfbedeckung.** [Vorschlag] Schürzenkleid
  in warmem Grün, Hirtenstab, Umhängetasche; abends in der Taverne ein Tuch um die Schultern.
- [Vorschlag] **Charakter:** herzlich, geduldig mit Tieren und Kindern, lacht viel und laut. Eigensinnig, wenn es um
  ihre Tiere geht. Verträumt: vergisst beim Hüten oder Singen die Zeit. Etwas scheu, wenn es um eigene Gefühle geht,
  deshalb braucht die Romanze Zeit.
- [Vorschlag] **Herkunft und Ziel:** Schäferstochter aus den Bergdörfern. Sie war mit ihrer kleinen Herde im
  Viehwagen unterwegs zum Markt in der Stadt und hat im Winter jedes Tier durchgebracht, Nacht für Nacht im kalten
  Wagen. Der Viehwagen ist ihr erster Stall. Wünscht sich eine richtige Weide mit Stall, Lämmer im Frühling und volle
  Abende in der Taverne.
- [Vorschlag] **Fähigkeiten zum Start:** Tierpflege Meisterin, Melken geübt, Musizieren und Singen geübt, Spinnen geübt,
  Kräuterkunde Anfängerin (lernt bei Martha).
- [Vorschlag] **Gewohnheiten:** singt beim Hüten, zählt abends ihre Tiere, bringt der Katze Milch, sitzt bei gutem
  Wetter auf dem Hügel über der Weide. Bei Gewitter geht sie zu den Tieren in den Stall.
- [Vorschlag] **Die Gitarre:** hat sie einst dem Kesselflicker gegen einen Winter voll Wolle abgetauscht; der
  Kesselflicker erinnert sich daran, wenn er kommt.
- [Vorschlag] **Beziehungen zum Start:** Gustav (der Wirt) ist wie ein Onkel für sie, mit Martha teilt sie die Liebe
  zu Pflanzen, mit Anton (Jäger) gibt es leise Reibung, weil er Tiere jagt.
- **Wohnt:** am Anfang im Abteil neben dem Viehwagen; später Schäferhaus mit Stall am Dorfrand. Erkennungsstücke: Gitarre an der Wand, Hirtenstab an der Tür,
  Schafsglocke, Wollkorb, Trockenblumen am Fenster.

### Der Betrachter

**[entschieden]** Der Betrachter beeinflusst Sykora am stärksten; die Kamera folgt ihm überwiegend.

**Bürgermeister [entschieden]: Wahl als Meilenstein.**

- Am Anfang führt Emil, der Lokführer, das Dorf kommissarisch, weil er die Reisenden durch den Winter gebracht hat.
  Er will das Amt aber abgeben und sich um seine Lok kümmern.
- Sykora sammelt Vertrauen, indem er hilft, baut und zuhört. Nach einigen Wochen ist Wahl auf dem Dorfplatz.
- **Vor der Wahl:** Der Betrachter gibt Tipps an Sykora. **Nach der Wahl:** Der Betrachter entscheidet zusätzlich über
  Bauvorhaben (Bauvorschläge der Bewohner, Kapitel 8).

**Tipps [Vorschlag]:**

- Ein Tipp an Sykora ist ein **sehr starker Wunsch**, kein Befehl: eine Aufgabe („hol Eier“, „geh mit in die Grube“),
  ein Besuch, ein Geschenk, ein Projekt. Er folgt fast immer, außer ein dringendes Bedürfnis oder die Lage spricht
  dagegen. Dann steht der echte Grund auf der Info-Karte („Zu müde. Morgen früh.“) und der Tipp wird später erledigt.
- Bei Taylor steuern Tipps nur Sykoras Verhalten. Wie Taylor reagiert, hängt von ihrer Laune, ihrem Tag und der
  Beziehung ab.
- **Bitten an andere Figuren** über ihre Charakterkarte oder das Schwarze Brett; angenommen, wenn Zeit, Laune und die
  Beziehung zu Sykora passen. Ein Nein hat einen sichtbaren Grund.
- Ohne Eingriff lebt Sykora selbstständig wie alle anderen.

## 3. Die Bewohner (Startbesetzung und Wachstum)

**[entschieden]** Jede Figur ist ein Charakter mit Job und wiedererkennbar. Keine zufälligen Statisten. Jede Figur
wohnt in einem festen Haus, und jedes Haus ist im Dorf sichtbar. **Niemand altert, niemand stirbt, niemand zieht
weg.** Alles bleibt, wie es ist; das Dorf wächst nur durch Zuzug.

**Startbesetzung [entschieden: Sykora, Taylor, Tischler, Bäckerin, Fischer, Jäger, Familie Brandt als altes Ehepaar
mit dem Jäger als Sohn; Rest Vorschlag].** 13 Menschen und eine Katze. Elf von ihnen sind Reisende aus dem
gestrandeten Zug, zwei lebten schon vorher hier. Namen außer Sykora und Taylor sind Vorschläge und jederzeit änderbar.

| Figur | Rolle | Woher, warum im Zug | Schläft am Anfang | Baut sich später | Erkennungsstücke |
|---|---|---|---|---|---|
| **Sykora**, 30 | Hauptfigur, will Bürgermeister werden | vom Land, wollte in der Stadt Arbeit suchen | Abteil | eigenen Hof am Dorfrand | Tagebuch, Hut am Haken |
| **Taylor**, 30 | Schäferin, Sängerin | Schäferstochter, mit ihrer Herde unterwegs zum Markt | Abteil neben dem Viehwagen | Schäferhaus mit Stall | Gitarre an der Wand, Hirtenstab, Schafsglocke |
| Gustav, 58 | Wirt | Koch im Speisewagen, hat im Winter alle satt bekommen | Speisewagen (erste Taverne) | Taverne am Platz, wohnt oben | Kupferkessel, Kreidetafel mit Tagesgericht |
| Emil, 47 | Lokführer, kommissarischer Dorfvorsteher | fuhr den Zug, fühlt sich für alle verantwortlich | Lokschuppen | Haus am Bahnhof | Signallaterne, Taschenuhr, Fahrplan an der Wand |
| Konrad, 56 | Schmied | auf dem Weg zu einer neuen Stelle in einer Fabrik | Gepäckwagen | Schmiede mit Esse | Amboss, Hufeisen über der Tür |
| Jakob, 41 | Tischler | Wandergeselle auf der Walz | Warteraum | Tischlerei | Hobelbank, Späne am Boden, halbfertiger Stuhl |
| Greta, 38 | Bäckerin | wollte in der Stadt eine Backstube übernehmen | Warteraum | Bäckerei mit Backofen | Brotschieber, Mehlsäcke, Brezel am Schild |
| Wilhelm, 66 | Bergmann a. D. | auf dem Weg zu seiner Schwester; erkennt die alte Grube am Berg | Abteil | Hütte am Hang | Grubenlampe, Erzbrocken auf dem Tisch |
| Paul Brandt, 68 | Zimmermann im Ruhestand, schnitzt | mit Frau und Sohn unterwegs zu Verwandten | Abteil der Familie | Haus der Brandts | Schnitzbank, Holzfiguren auf dem Sims |
| Lene Brandt, 66 | Weberin | wie Paul | Abteil der Familie | Haus der Brandts | Webstuhl, Garnkörbe |
| Anton Brandt, 34 | Jäger | Sohn der Brandts; hat im Winter alle mit Wild versorgt | Abteil der Familie | Jagdhütte am Waldrand | Geweih an der Wand, Felle, Bogen bzw. Flinte |
| Martha, 72 | Kräuterfrau | **lebte schon hier**, half mit Kräutern durch den Winter | eigene Hütte am Waldrand | Kräutergarten, Pflanzenhaus | Kräuterbündel an der Decke, Mörser |
| Hannes, 52 | Fischer | **lebte schon hier**, wortkarg, kennt jedes Wetter | eigene Hütte am See | Steg, Bootsschuppen | Reusen, Netze zum Trocknen, Boot am Steg |
| Katze (Name vom Betrachter) | vollwertige Figur | streunt im Bahnhof herum, niemand weiß woher | wo es warm ist | bleibt bei Sykora oder Taylor | Lieblingskissen, Gabenhaufen |

Kinder gibt es zum Start nicht; Zugezogene können Kinder mitbringen, die Kinder bleiben.

**Wiederkehrende Gäste [entschieden]**, alle mit Namen und Geschichte: die Händlerin, die im Sommer mit ihrem Maultier
über den Bergpass kommt (Tausch, Post, Neuigkeiten), der Kesselflicker auf Wanderschaft (selten, erzählt in der
Taverne). Später kommen Gäste mit dem Zug.

**Wachstum [entschieden]:** Endausbau **30–40 Bewohner in etwa 20 Häusern**, manche wohnen zu zweit (z. B. die
Brandts). Neue Bewohner kommen erst, wenn ein neues Haus gebaut ist. Sie ziehen in eine **Notunterkunft** ein, danach
wird Raum für Raum ausgebaut (Kapitel 8). Neue Figuren entstehen **einmal** (aus einer handgeschriebenen Liste oder aus
Bausteinen mit Herkunft, Ziel und Charakter) und bleiben für immer. [Vorschlag] Bis die Lok fährt, kommen Neue über
den Bergpass (Wanderer, Leute aus den Bergdörfern, die von Pixelia gehört haben), danach mit dem Zug.

## 4. Gebiete und Kapitel der Welt

**[entschieden]** Das Dorf ist das Herz. Der Zug ist vor allem Reisemittel. Die Grube (der „Dungeon“) dient zum
Rohstoffe-Sammeln, ohne Kampf. Neue Gebiete kommen mit Updates. Orte werden nach und nach zugänglich.

**[Vorschlag] Übersicht der ersten und wichtigsten Gebiete:**

| Kapitel | Gebiet | Orte (je ein Bild) | Zugang | Zweck |
|---|---|---|---|---|
| Start | **Bahnhof mit gestrandetem Zug** | Bahnsteig mit Feuerstelle, Warteraum, Abteilwagen (innen), Speisewagen als erste Taverne (innen), Viehwagen als Stall, Güterschuppen als Lager, Lokschuppen | ab Tag 1 | Keimzelle des Dorfs, Schlafplätze, Abende, später Lok-Bogen |
| Start | **Umgebung** | Waldrand mit Marthas Hütte, Wald, See mit Hannes' Hütte, Wiesen, Lichtung für die ersten Häuser | ab Tag 1 | Holz, Pilze, Beeren, Kräuter, Wild, Wasser, Fisch, Lehm |
| 1–2 | **Dorf Pixelia** (wächst) | Dorfplatz mit Brunnen und Schwarzem Brett, Häuserzeilen, Taverne, Schmiede, Tischlerei, Bäckerei (innen), Schäferhaus und Weide, Vorratsscheune | entsteht Haus für Haus | Gemeinschaft, Handwerk, Bauprojekte |
| 1–2 | **Sykoras Hof** | Stube mit Kamin und Küche, Schlafkammer, Hofplatz, Garten, Weide, Bach; später Werkstatt, Vorratskeller, Stall | Sykoras erstes großes Projekt | Zuhause, Selbstversorgung, Katze |
| 3 | **Die alte Grube** | Stolleneingang, mehrere Stollenabschnitte, Kohleflöz, tiefe Sohle | stufenweise über Werkzeug und Fähigkeiten | Kohle, Erz, Stein, Ton, Fundstücke |
| 3 | **Berghang** | Steinbruch, Aussichtspunkt | mit Wilhelm | Stein, Ausflüge, Lieblingsort |
| 4 | **Die Strecke** | zugewachsene Gleise, Brücke, Abteil im fahrenden Zug; der verschüttete Tunnel nach hinten | nach der Lok | Zugfahrt |
| 5+ | **Neue Regionen (Updates)** | z. B. Marktstadt, Küste, See, Bergdorf | mit dem Zug | wählt der User, wenn es soweit ist |

**Sonderorte [Vorschlag], verstreut und selten:** die Grabstelle eines Unbekannten am Waldrand (langes Rätsel ohne
Magie: Fundstücke aus Grube, Wald und Briefen erzählen über Monate, wer das war), ein verlassener Planwagen, die Ruine
auf dem Hügel, eine Höhle hinter dem Wasserfall.

**Die Grube [entschieden: an bestimmten Stellen geht es nur mit Werkzeug oder Fähigkeit weiter; Rest Vorschlag]:**
Expeditionen mit einer kleinen Gruppe (Sykora, Wilhelm, wer Lust und Zeit hat). Kein Kampf. Spannung entsteht aus
Licht, Wasser und Gestein. Jedes Tor ist im Bild sichtbar, so wird die Grube über Wochen tiefer:

| Hindernis | Braucht | Kommt von |
|---|---|---|
| dunkle Stollen | Grubenlampe und Öl | Konrad (Lampe), Händlerin (Öl) |
| verschütteter Gang | Spitzhacke, Bergbau geübt | Konrad, Wilhelm bringt es bei |
| brüchige Decke | Stützbalken | Jakob, Holz aus dem Wald |
| senkrechter Schacht | Leiter, Seil | Jakob, Lene (Seil) |
| abgesoffener Abschnitt | Pumpe mit Lederdichtungen | Konrad, Anton (Leder) |
| schwere Funde | Lore auf Schienen | Konrad, Jakob |

**Regel für Updates [Vorschlag]:** Jedes Projekt lässt sich mit dem vorhandenen Inhalt abschließen. Was auf ein Update
wartet, ist in der Welt plausibel versperrt (z. B. „hinter dem Tunnel ist die Strecke verschüttet“). Die fertige Lok
fährt schon vor dem ersten Regionen-Update, nämlich zu einer Nachbarstadt außerhalb des Bildes (Handel, Gäste, Briefe).
Mit dem Update wird diese Stadt ein echter Ort.

## 5. Figuren: Bedürfnisse, Eigenschaften, Fähigkeiten

**Bedürfnisse [übernommen aus dem Skript]**, als Antriebe, ohne Balken auf dem Display: Hunger, Energie, Wärme,
Gesellschaft, Neugier, Hygiene. Dazu **Wünsche** („ein Bett statt Stroh“) und **Mangelzustände** („kalt“, „Werkzeug
fehlt“), die in Bauvorschläge einfließen.

**Eigenschaften jeder Figur [übernommen, ergänzt]:** Name, Alter, Aussehen (Baukasten), Rolle bzw. Beruf, Zuhause;
Herkunft und Ziel; 2–3 Charakterzüge; Interessen (gewichtet, verändern sich langsam); Fähigkeiten; Erinnerungen (einige
sichtbar als Gegenstand); **Erkennungsstücke** im eigenen Haus; Beziehungen zu anderen Figuren in Stufen fremd →
bekannt → vertraut → hilft, dazu Familie, Zuneigung, Rivalität.

**Fähigkeiten [Vorschlag]: alle Figuren haben sie.**

- Stufen statt Zahlen: *unbekannt, Anfänger, geübt, Meister*. Jede Stufe schaltet Tätigkeiten frei (Flechten
  geübt → Körbe). Intern zählt ein kleiner Erfahrungswert.
- Gelernt wird durch Zuschauen, gemeinsames Arbeiten, Ausprobieren und Gezeigt-Bekommen. Dabei wächst die Beziehung
  **zwischen den beiden Figuren** (Konrad bringt Sykora das Schmieden bei, nach Wochen an der Esse sind sie Freunde).
- Etwa 24 Fähigkeiten zum Start: Kochen, Backen, Holzarbeit, Tischlern, Zimmern, Mauern, Schmieden, Reparieren,
  Flechten, Spinnen, Weben, Nähen, Gärtnern, Kräuterkunde, Pilzkunde, Tierpflege, Melken, Fischen, Jagen, Bergbau,
  Musizieren, Erzählen, Lesen und Schreiben, Bahntechnik.

## 6. Tagesgerüst und Entscheidungslogik

**[übernommen]** Nutzenbewertung, Wiederholungsbremse, leichte Streuung, deterministischer Zufall mit Startwert,
Eingriffe als starker Wunsch.

**Tagesgerüst [entschieden]:** Tätigkeiten haben **Zeitfenster**:

- Schlafen nur nachts; tagsüber höchstens ein Nickerchen (Sofa, Ofenbank, im Gras).
- Gearbeitet wird nur tagsüber; nachts arbeitet niemand.
- Taverne, Feiern und Musik nur abends.

[Vorschlag] Zeitfenster hängen an der Tageszeit der Welt und verschieben sich mit Jahreszeit und Charakter
(Frühaufsteher, Nachteule). Ausnahme zum Besprechen: Greta beginnt in der Morgendämmerung mit dem Backen.

**Aufbau der Entscheidung [Vorschlag]:**

1. **Tagesgerüst:** Schlaf, Mahlzeiten, Arbeitszeit nach Rolle, Rituale (morgens Tee, abends Laterne anzünden).
2. **Aufgaben der Gemeinschaft:** Was das Dorf braucht, steht als Aufgabe auf einer Liste (Holz für die Taverne,
   Balken für die Baustelle). Wer Zeit und die passende Fähigkeit hat, übernimmt.
3. **Nutzenbewertung** für die Lücken und die Freizeit: Bedürfnisse, Interessen, Beziehungen, Wetter, Tageszeit,
   Wiederholungsbremse.

Jede Entscheidung speichert ihren **Grund** in einem Satz für die Info-Karte. Eingriffe des Betrachters werden mit
Zeitpunkt protokolliert, damit Läufe mit Startwert reproduzierbar bleiben.

## 7. Tiere

**Katze [übernommen]:** vollwertige Figur mit Namen vom Betrachter, eigenem Tagesablauf nach Jahreszeit und Wetter,
wanderndem Lieblingsplatz, Gaben (Blatt, Feder, Maus, Stein), die zu Erinnerungen werden, Eifersucht, eigenen
Beziehungen, einmaligen Momenten (erster Schnee). Kein Altern, wird mit der Zeit ruhiger und anhänglicher. [Vorschlag]
Taucht erst nach einigen Tagen auf und wählt selbst, bei wem sie bleibt.

**Wildtiere [übernommen]:** wiederkehrende Einzeltiere mit Vertrauensstufen (scheu, wartet, zutraulich, kommt auf
Ruf). [Vorschlag] Fuchs mit hellem Schwanzende, Reh mit Kitz im Frühling, Igel im Herbst, eine Krähe, die Glänzendes
sammelt, die Eule am Bahnhof, Glühwürmchen im Sommer.

**Nutztiere [übernommen]:** Hühner, Ziegen, Kuh, Schafe mit eigenem Rhythmus; liefern Eier, Milch, Wolle. Keine
Pflichtversorgung durch den Betrachter. [Vorschlag] Taylors Herde gehört zu den ersten Tieren. Nutztiere sind einfache
Figuren mit Namen, Farbe und einer Vorliebe (das Schaf, das immer ausbüxt), ohne volles Gedächtnis.

## 8. Bestand, Wirtschaft und Bauen

**Bestand [übernommen]:** sammeln, lagern, verarbeiten, verbrauchen. Jede Ware hat Menge und Ort. Einzelstücke mit
Herkunft und Zustand, gleichartige Waren als Mengen. Lager mit Kapazität. Verarbeitung verbraucht exakt.
Sammelertrag hängt von Ort, Jahreszeit, Ortszustand, Energie der Figur und Zufall ab. Vorräte sind sichtbar.

**[Vorschlag]** Ein **Warenbuch für die ganze Welt**; alle Orte buchen nur darüber. **Kein Geld am Anfang:** Tausch,
Geschenke, Gemeinschaftslager in der Scheune, jeder Haushalt hat zusätzlich ein eigenes Lager; die Händlerin tauscht
gegen Waren von außen. Geld kann später mit der Bahn kommen.

**Bauen [übernommen]:** Projekte brauchen Material, Arbeit und Zeit; jeder Ausbau verändert das Leben; kein fester
Bauplan; Anpassung aus Nutzung (oft am Feuer → Windschutz). **Bauvorschläge** der Bewohner hängen am Schwarzen Brett
und im Web; der Betrachter (als Bürgermeister) antwortet mit Ja, Nein oder Später; **ohne Antwort nach 24 Stunden
entscheiden die Bewohner selbst**.

**Großprojekte bestehen aus Teilaufgaben [entschieden].** Jede Teilaufgabe hat eigenes Material, eigene Fähigkeit und
einen sichtbaren Zwischenstand. Das verknüpft die Systeme: Die Lok braucht Kohle aus der Grube, Holz vom Tischler, Leder
vom Jäger, Eisen vom Schmied.

**[Vorschlag] Die Dampflok** (das längste Projekt, öffnet später neue Gebiete):

| Teilaufgabe | Braucht |
|---|---|
| Lokschuppen freischneiden, Tor gangbar machen | Werkzeug, Holzarbeit |
| Rost abklopfen und Kessel ölen | Drahtbürsten (Konrad), Öl |
| Kessel entkalken und Druckprobe | Essig, Wasser, Schmieden geübt |
| Siederohre ersetzen | Eisen, Kohle, Konrad (Meister) |
| Radlager neu ausgießen | Weißmetall aus Erz der Grube, Schmieden |
| Kolben und Stopfbuchsen neu packen | Hanf, Talg |
| Bremsklötze aus Hartholz | Eichenholz, Jakob |
| Pfeife, Laternen, Manometer instand setzen | Messing, Reparieren |
| Wasserkran am Bahnhof reparieren | Lederdichtungen (Anton), Pumpe |
| Kohle bunkern | Kohle aus der Grube |
| Gleise freilegen, verfaulte Schwellen ersetzen | Eichenholz, Schienennägel (Konrad) |
| Weiche und Signal gangbar machen | Reparieren, Emil (Bahntechnik) |
| Probefahrt im Bahnhof, dann bis zum Tunnel | alles oben |

**[entschieden] Hausbau mit Notunterkunft:** Ein neues Haus wird gebaut, dann zieht jemand ein. Erst ist es eine
Notunterkunft (Kisten, Säcke, Strohsack, überall Gerümpel und Baumaterial). Danach entstehen Stück für Stück die
Räume: Küche (Feuerstelle → Herd), Schlafkammer (Strohsack → Bett), Stube (Kisten → Tisch und Bank → Ofen und Sofa),
Waschkammer (Zuber → Bad).

**[Vorschlag] Dauer in echter Zeit:**

| Größe | Beispiele | Dauer |
|---|---|---|
| klein | Regal, Windschutz, Bank, Zaun flicken, ein Möbelstück | 1–3 Tage |
| mittel | ein Raum, Werkbank, Hühnerstall, Dorfplatz freilegen | 1–2 Wochen |
| groß | Haus bis zur Notunterkunft, Taverne zur Schenke, Scheune | 3–6 Wochen |
| sehr groß | Lok und Strecke | 2–3 Monate, in Teilaufgaben |

**Ausbaustufen im Dorf [übernommen aus dem Skript]:**

| Ort | Stufen | Was er verändert |
|---|---|---|
| Feuerstelle vor dem Bahnhof → Dorfplatz | Feuerstelle, Platz mit Brunnen, gepflastert, Marktplatz | Treffpunkt, Märkte, Feste |
| Taverne | Speisewagen, Schenke am Platz, Gasthaus mit Zimmern | Abende, Erzählungen, Gäste übernachten |
| Wohnhäuser (pro Haushalt) | Schlafplatz im Abteil oder Warteraum, Neubau als Notunterkunft, Räume ausgebaut, gemütlich, persönlich | Schlafqualität, Wärme, Zuzug |
| Werkstatt, Schmiede | Werkzeugkiste, Werkbank, Esse | Reparaturen, Werkzeug, Ausrüstung für die Grube |
| Pflanzenhaus | Frühbeetkasten, Glasanbau, Gewächshaus | Kräuter, Gemüse im Winter, Saatgut |
| Vorratsscheune | offener Unterstand, Scheune, Keller | Lagerkapazität |
| Bahnhof | Notlager, aufgeräumt, Warteraum wieder Wartesaal, Güterschuppen als Umschlag | Bahnverkehr, Handel |
| Badehaus (spät) | Waschzuber, Badehaus mit Ofen | Hygiene, Gesellschaft |
| Schwarzes Brett | ein Brett, überdachte Tafel | Wünsche, Bauvorschläge, Briefe; Spiegel der Weboberfläche |

### Innenräume und Möbel als Baukasten [entschieden: Wunsch des Users]

- **Innenräume aus Teilen:** Wände, Böden, Fenster, Türen, Möbel und Deko sind einzelne Teile. Ein Raum ist eine
  Datenliste, welches Teil wo steht. So entstehen 20 verschiedene Häuser ohne 20 gemalte Bilder.
- **Ausbaustufen über den Baukasten:** Möbel und Räume haben Stufen (Strohsack → Bett, Kisten → Tisch); ein Ausbau
  tauscht oder ergänzt Teile.
- **Farbliche Unterschiede:** Bett, Sofa, Vorhänge, Tapete usw. gibt es in Farbvarianten. [Vorschlag, Technik] Umgesetzt
  über **Palettentausch**: Ein Möbelstück wird einmal gezeichnet, die Farbe kommt aus einer austauschbaren Farbreihe.
  Kostet keinen zusätzlichen Speicher pro Farbe.
- **Erkennungsstücke:** Jedes Haus bekommt eigene Farben, Tapete, Boden und 3–5 Erkennungsstücke seiner Bewohner
  (Gitarre bei Taylor, Geweih beim Jäger, Mehlsäcke bei der Bäckerin; Tabelle in Kapitel 3).
- **Sykoras Haus ist anpassbar:** Haustyp, Farben und Möbel (z. B. Auswahl aus mehreren Sofas) in der Weboberfläche
  wählbar. [Vorschlag] Eine Auswahl wird zum kleinen Projekt: Sykora muss das Möbel bauen, tauschen oder streichen, das
  kostet Zeit und Material (Farbe von der Händlerin, Holz vom Tischler). Optional später auch für Taylor.
- **Eigene Möbel des Users [prüfen in 0.5.x]:** Der User zeichnet Möbelstücke nach Vorgaben (Maße im Raster, Palette)
  und lädt sie über die Weboberfläche hoch oder bringt sie per Update ins Repo. Siehe Kapitel 16.

## 9. Zeit, Kalender, Wetter und Licht

**[entschieden]**

- **Tageszeit = echte Zeit**, mehrere Lichtstufen von Morgen bis Nacht.
- **Jahreszeiten beschleunigt:** 7 echte Tage pro Jahreszeit (einstellbar), ein Weltjahr = 4 Wochen.
- **Wetter wird simuliert**, passend zur Weltjahreszeit, und kann sich **viermal am Tag ändern**: morgens, mittags,
  abends, nachts. Übergänge sind weich (Wolken ziehen auf, Regen setzt langsam ein).

**[Vorschlag]**

- Wetterarten: Sonne, bewölkt, Regen, Schnee, Wind, Nebel, Gewitter. Ein Gewitter darf auch einmal mitten in einer
  Wetterzeit kommen (Sommernachmittag).
- **Kalender der Welt:** Jahr 1 beginnt mit dem ersten Tag. Feste entstehen aus dem Leben (fertiger Raum, erste Ernte,
  Wahl, erste Zugfahrt, Jahrestag) und aus wenigen Jahresfesten (Frühlingsmarkt, Erntefest, Lichterabend zur
  Wintersonnenwende).
- **Erstlinge** jeder Jahreszeit (erste Schwalbe, erster Schnee) werden einmalig als Erinnerung gespeichert.
- **Licht als System:** Laternen, Fackeln, Kerzen und Kamine brauchen Brennstoff aus dem Warenbuch. Das abendliche
  Anzünden ist ein Ritual, ein leerer Ort bleibt dunkel.

## 10. Ortszustände

**[übernommen]** Jede sichtbare Veränderung hat eine Ursache:

| Zustand | Verhalten | Sichtbar als |
|---|---|---|
| Feuer, Glut | brennt mit Brennstoff, kühlt langsam ab | Flamme, Glut, Asche, Rauch aus dem Schornstein |
| Licht | brennt, solange Öl oder Talg reicht | Fenster hell oder dunkel, Fackel an oder aus |
| Wasser | steigt bei Regen, sinkt bei Verbrauch | Füllstand in Fass oder Brunnen |
| Pflanzen | wachsen nach Jahreszeit, Wasser, Licht | Spross, Blüte, Frucht |
| Nässe | steigt bei Regen, trocknet langsam | Tropfen, Pfützen, dunkles Holz |
| Angefangene Arbeit | bleibt liegen, bis sie fortgesetzt wird | Werkstück auf der Bank |
| Abnutzung | steigt mit Nutzung | ausgetretene Wege, glatte Sitzplätze |
| Verschleiß | Dinge gehen kaputt und werden repariert | wackelnder Stuhl, Loch im Dach, geklebte Tasse |

Ortszustände laufen weiter, wenn niemand hinschaut. Schaltet man in ein leeres Haus, sieht man genau diesen Zustand:
Kamin aus, Fenster dunkel, die Katze allein auf dem Stuhl.

## 11. Abende und Ereignisse

**[übernommen]** Rituale verschieben sich mit Jahreszeit und Wetter. **Abende als emotionales Herz:** gemeinsames
Essen, Gespräche (visuell), Musik (Taylor singt), Spiele, Geschichten, Schweigen; jeder Abend anders. Ereignisse:
kleine Unfälle ohne Strafe, Probleme mit Folgen, seltene Beobachtungen, Freude ohne Zweck, Ruhe als Ereignis. Zeit
sichtbar machen: wachsendes Tagebuch, selbst gepflanzter Baum, Steinhaufen mit Fundstücken.

**Texte [entschieden]:** eher kurz; ab und zu ein Gedanke oder Satz sichtbar.

**[Vorschlag]** Ein **Ereignis-Regisseur** für die ganze Welt wählt nach Verlauf, Spannungsbogen und Sperrzeiten aus
(LEBENDIGKEIT Regel 6). Ereignisse stehen als Daten in Seltenheitsstufen.

### Besondere Ereignisse starten [entschieden, Wunsch des Users 2026-10-21]

Grund: Der Betrachter schaut nicht rund um die Uhr hin und würde besondere Momente sonst verpassen.

- **Start über die Weboberfläche mit Voraussetzungen.** Eine Liste zeigt alle besonderen Ereignisse (Dorffest,
  Tavernenabend, das verlorene Lamm …). Bereite Ereignisse lassen sich starten. **Gesperrte** zeigen, was fehlt
  (z. B. „erst, wenn Sykora und Taylor sich vertraut sind“, „das Fest braucht Essen und Musik“).
- **Nur zu passenden Uhrzeiten:** Jedes Ereignis hat ein Zeitfenster. Dorffest und Tavernenereignisse nur abends; ein
  Abendereignis kann tagsüber nicht gestartet werden. Außerhalb des Fensters ist es gesperrt mit Hinweis, ab wann es
  geht. [Vorschlag] Man kann es tagsüber für den Abend ansetzen.
- **Vorbereitung ist sichtbar:** Für ein Dorffest werden tagsüber Vorkehrungen getroffen (Bänke tragen, Girlanden und
  Laternen aufhängen, Essen kochen, Holz fürs Feuer). Die Orte sind **geschmückt und sehen anders aus**; der Schmuck
  bleibt bis zum Aufräumen am nächsten Tag. [Vorschlag] Ein Fest, das man abends startet, wurde dann „schon tagsüber
  vorbereitet“ nur, wenn es vorher angesetzt war; sonst wird es ein kleineres, spontanes Fest.
- [Vorschlag] Einmalige Meilensteine (das verlorene Lamm, der erste Tanz) lassen sich höchstens einmal auslösen;
  wiederholbare Ereignisse (Dorffest, Tavernenabend) haben eine Sperrzeit, damit sie besonders bleiben.

### Zuschauzeiten und nichts verpassen [entschieden]

- **Zuschauzeiten:** Der Betrachter stellt ein, wann er meist hinschaut (z. B. 18–22 Uhr). **Große Ereignisse, die
  nicht zum normalen Tagesablauf gehören, legt der Regisseur in diese Zeiten.** Der normale Alltag läuft unabhängig
  davon weiter.
- **Kamera springt hin:** Passiert etwas Besonderes, schwenkt die Anzeige automatisch dorthin, mit Kurzmeldung.
  Abschaltbar.
- **Verpasst-Übersicht:** Beim Öffnen der Weboberfläche zuerst eine kurze Zusammenfassung, was seit dem letzten Besuch
  passiert ist.
- Nicht gewählt: automatische Erinnerungsfotos (bleibt als Idee im Hinterkopf).

## 12. Eingreifen und Weboberfläche

**[übernommen]** Anzeigen: Charakterkarten (Bedürfnisse, Stimmung, Rolle, Fähigkeiten, Interessen, Beziehungen,
Wünsche), Lager (Waren, Mengen, Lagerorte, Einzelstücke mit Herkunft), Bauprojekte mit Teilaufgaben, Fortschritt und
Material, Logbuch (vor allem was während der Abwesenheit passiert ist), Live-Vorschau (vorhanden).

Eingreifen: Tipps an Sykora, Bitten an andere (Kapitel 2), Bauvorschläge beantworten (ab der Wahl), Kamera umschalten
(Kapitel 13), Namen vergeben, Sykoras Haus einrichten (Kapitel 8), eigene Möbel hochladen (falls machbar).

**Schwarzes Brett [übernommen]:** die Brücke zwischen Dorf und Weboberfläche. Was dort hängt, erscheint gleichzeitig
im Web. [Vorschlag] Beantwortete Zettel werden im Dorf abgenommen.

**Briefe mit Text [Wunsch des Users, 2026-10-21, auszuarbeiten]:** Briefe (vom Postboten, aus Nachbardörfern, von
Bewohnern untereinander) haben echten Text und eine eigene Ansicht, ähnlich der „Vorschau“ bei den Katzen im
CYD-Projekt Pixelpfote. Der User bringt eine Beispieldatei mit.

**Gedanken der Bewohner [Wunsch des Users, 2026-10-21]:** Was eine Figur gerade denkt (mit echtem Grund aus der
Simulation), ist auf der Info-Karte am Gerät und in der Weboberfläche lesbar, dort auch als Verlauf.

**[Vorschlag] Touch am Gerät:** Figur antippen → Info-Karte mit Grund; Gebäude antippen → wer ist drin; Wischen →
nächster Ort; langes Halten auf einer Figur → „Folgen“.

## 13. Kamera und Regie

**[entschieden]** Die Kamera zeigt überwiegend Sykora und geht mit ihm auf Reisen. Der Betrachter kann jederzeit zu
jedem bekannten Ort schalten, auch dorthin, wo niemand ist (leeres Haus, Kamin aus).

**[Vorschlag]**

- Modus **„Sykora“** (Standard): folgt ihm von Ort zu Ort. Schläft er, schwenkt die Regie behutsam zu Taylor, zu
  Nachteulen, Tieren oder der Taverne und kehrt morgens zurück.
- Modus **„Frei“**: per Touch oder Web zu jedem Ort; das Dorf hat mehrere Orte zum Durchschalten (Platz,
  Häuserzeilen, Bahnhof …). Nach einer einstellbaren Zeit ohne Eingabe (z. B. 10 min) zurück zu Sykora.
- Modus **„Folge X“**: jede andere Figur, z. B. Taylor.
- **Kurzmeldungen** von anderen Orten nur bei Wichtigem, höchstens wenige pro Stunde.

## 14. Perspektive

**[Stand laut User, Entscheidung aus der Perspektiv-Sitzung]** Die Darstellung wird **deutlich größer**, damit auch
kleine Dinge wie Tassen erkennbar sind. Dafür ist jeder Ort **ein ganzes Bild ohne Scrollen**; man wechselt zwischen
Orten bzw. Hintergründen statt durch eine große Karte zu fahren. Genaue Maße (Figurengröße, Raster) kommen aus der
Perspektiv-Sitzung und landen im Stil-Leitfaden (`docs/STIL.md`, 0.5.x).

Folgen fürs Setting: Ein Ort fasst weniger Figuren und Häuser. Das Dorf besteht deshalb aus mehreren Orten
(Dorfplatz, Häuserzeilen, Bahnhof), und jedes Haus hat eigene Innenorte. Figuren wechseln den Ort durch Türen und
Bildränder.

## 15. Modularer Aufbau und Stufen der Simulation

**[Vorschlag, Technik]**

Grundsätze:

1. **Welt = gemeinsamer Zustand:** Uhr, Wetter, Figuren, Warenbuch, Chronik, Verbindungen, Aufgabenliste, Projekte.
   Gespeichert als Ganzes.
2. **Ort = ein Bild = Modul-Baustein:** bringt Hintergrund, Baukasten-Einrichtung, Wege- und Benutzungspunkte,
   ortstypische Tätigkeiten und Ereignisse als Daten mit, dazu seinen Ortszustand. Ein Gebiet (Hof, Dorf, Grube) ist
   eine Gruppe von Orten. Ein Ort greift nie direkt auf einen anderen zu.
3. **Eine Figur ist immer an genau einem Ort** oder auf genau einer Verbindung. Ein Wechsel ist eine Übergabe in einem
   Schritt, nie eine Kopie.
4. **Verbindungen sind Daten:** Ort A + Punkt, Ort B + Punkt, Art (Tür, Bildrand, Weg, Bahnhof, Stolleneingang),
   Reisezeit, Bedingungen (nur bei Tag, erst nach Bau, Werkzeug nötig, Zug nach Fahrplan). Fahrzeuge sind bewegliche
   Orte.
5. **Neues Gebiet ohne Umbau:** neuer Ordner, Daten, eine Zeile Registrierung. Ein alter Speicherstand lädt weiter.
6. **Grafiken über eine austauschbare Asset-Quelle** (Flash oder SD-Karte), Kapitel 16.

Stufen der Simulation:

| Stufe | Wo | Takt | Was simuliert wird |
|---|---|---|---|
| **Voll** | sichtbarer Ort, Kern 1 | 30 Hz | Wege, Animation, Begegnungen, Mikro-Ereignisse |
| **Grob** | alle anderen Orte, Kern 0, niedrige Priorität | ~1 Hz | Tätigkeiten als Zeitblöcke mit derselben Wirkung, Ortszustände |
| **Nachholen** | nach dem Einschalten | einmalig, in Minutenschritten | begrenzt und plausibel (Regel 10) |

Schlüssel: **gleiche Wirkung, andere Darstellung.** Tätigkeiten buchen Waren, Bedürfnisse, Fähigkeiten und
Erinnerungen in beiden Stufen identisch. Schaltet man zu einem Ort, werden die Figuren an die passende Stelle ihrer
laufenden Tätigkeit gesetzt. Die Bilanz muss exakt sein, die Bewegung nicht. Das ersetzt die Formulierung
„Hintergrund- und Live-Simulation liefern dieselben Ergebnisse“ (PROJEKTPLAN 4.5).

Engine-Teile, die das braucht (letzte 0.5.x-Patches): Weltzustand mit Figuren, Warenbuch, Aufgabenliste und Projekten
mit Teilaufgaben; Orts-Graph mit Verbindungen als Daten; Modul-Schnittstelle für Orte und Gebiete; Übergaben zwischen
Stufen und Kernen (nur an Takt-Grenzen, über eine Warteschlange); Regie und Kamera-Modi; Speicherstand als Ganzes;
Innenraum-Baukasten mit Palettentausch; Asset-Quelle Flash oder SD.

## 16. Hardware-Rahmen mit Zahlen

**Budget:** 30 fps (nie unter 25), Logik + Zeichnen ≤ 20 ms pro Bild, interner RAM ≥ 60 KB frei, PSRAM 8 MB
(≥ 1 MB frei), 400×240 mit 8-Bit-Palette, App-Partition 6 MB (Firmware ≤ 75 %), Speicher-Partition `storage`
(LittleFS) 3,8 MB, noch ungenutzt.

**Gemessen** (HARDWARE.md): Zeichnen 4,1 ms mit 48 Sprites, 8,4 ms mit 500, 22,6 ms mit 2000 (Puffer im PSRAM); PSRAM
frei ~7,7 MB mit Engine; Flash-Schreiben während der Animation ohne Aussetzer; Firmware heute ~1,5 MB.

**Schätzungen** (Claude, noch nicht gemessen, Größenordnung):

| Posten | Schätzung | Folgerung |
|---|---|---|
| Datensatz pro Figur (Stammdaten, Zustand, Bedürfnisse, Tagesgerüst, Besitz, 24 Fähigkeiten, 24 Beziehungen, 32 Erinnerungen) | 0,5–1 KB | 40 Figuren ≈ 40 KB: **RAM ist nicht die Grenze** |
| Ein Ort (ein Bild, Hintergrund 400×240 + Teile + Wegpunkte) | 0,1–0,3 MB im PSRAM | aktiver Ort plus vorgeladene Nachbarn |
| Innenraum als Baukasten-Liste | ~0,5 KB pro Raum | 20 Häuser × 4 Räume ≈ 40 KB |
| Möbel- und Teile-Atlas für alle Innenräume | 0,3–0,6 MB | einmal für alle Häuser; Farbvarianten per Palettentausch kosten nichts |
| Zum Vergleich: jeder Raum als eigenes Bild | 20–40 KB gepackt pro Raum, 60 Räume ≈ 2 MB | im Flash knapp, auf SD egal; Baukasten bleibt besser wegen der Ausbaustufen |
| Inaktiver Ort (nur Ortszustand) | 1–10 KB | 100 Orte < 1 MB |
| Logik voller Ort (bis 15 Figuren im Bild) | < 1 ms pro Bild | viel Luft |
| Grobe Stufe (5–20 µs pro Figur, 1 Hz) | 40 Figuren < 1 ms pro Sekunde | vernachlässigbar |
| Speicherstand gesamt | 0,1–0,3 MB roh, < 0,1 MB gepackt | passt vielfach in `storage` |
| Flash-Verschleiß (100 KB alle 10 min, 100 000 Löschzyklen, LittleFS verteilt) | ~4 Durchläufe der Partition pro Tag | rechnerisch Jahrzehnte |

**Personen [entschieden]:** 30–40 Bewohner, jeder mit Charakter, Job und wiedererkennbar. Technisch gingen weit mehr;
die echten Grenzen sind Wiedererkennbarkeit, Platz für sichtbare Häuser und Schreibarbeit. **Fähigkeiten** haben alle
Figuren (~50 Byte pro Figur).

**SD-Karte [entschieden]: erst ohne Karte arbeiten, später bei Bedarf alles auf die Karte umziehen.** Bedingung: Die
Engine liest Grafiken und Inhalte von Anfang an über eine **austauschbare Asset-Quelle** (Flash oder SD). Dann ist der
spätere Umzug nur Kopieren und Umschalten, auch für fertige Orte. Ein Ort lädt von der Karte in ~0,1–0,3 s (SPI,
~1–2 MB/s), beim Ortswechsel hinter einer Überblendung auf Kern 0. Nach dem Umzug braucht die Welt die Karte; fehlt sie,
zeigt das Gerät einen Hinweis.

| Nutzung der Karte | Eignung |
|---|---|
| Grafiken und Inhaltspakete | sehr gut, hebt die Flash-Grenze auf (neben der Firmware ~3 MB) |
| Chronik, Logbuch, Tagebuch als Archiv | sehr gut |
| Sicherungskopien des Speicherstands | gut |
| eigene Möbel des Users | gut |
| der Speicherstand selbst | nein: bleibt im internen Flash (LittleFS, stromausfallsicher) |
| Laden während des Zeichnens | nein: nur beim Ortswechsel |

Vorher messen (0.5.x): Lesegeschwindigkeit, keine Bildaussetzer während des Lesens, Ziehen der Karte. Pins laut
Community-Angabe 10/11/12/13 (SPI), noch nicht getestet.

**Eigene Möbel des Users [Vorschlag, in 0.5.x prüfen]:** Technisch machbar.

- **Vorgaben:** Maße in ganzen Rastereinheiten (genaue Zahlen nach der Perspektiv-Entscheidung im Stil-Leitfaden),
  nur Farben aus der Pixelia-Palette bzw. aus einer austauschbaren Farbreihe (dann gibt es automatisch Farbvarianten),
  transparenter Hintergrund, optional Ausbaustufen als weitere Bilder gleicher Größe.
- **Dazu Daten:** Name, Art (Bett, Sitz, Tisch, Regal, Deko), Stellfläche und **Benutzungspunkte** (wo eine Figur
  sitzt oder liegt). Ohne diese Punkte kann keine Figur das Möbel benutzen.
- **Weg 1, Weboberfläche:** PNG hochladen, die Oberfläche zeigt eine Vorschau im Raum, prüft Maße und Farben, rechnet
  fremde Farben auf die Palette um und speichert das Möbel auf SD oder im Flash (wenige KB pro Stück).
- **Weg 2, Update:** Der User gibt die PNG an Claude, sie kommt ins Repo (`assets/`, eigene Werke) und mit dem nächsten
  Update auf alle Geräte.
- Speicherstände verweisen auf Möbel per Kennung; wird ein eigenes Möbel gelöscht, ersetzt ein Standardmöbel es.

## 17. Rahmenbedingungen für Claude

**[Vorschlag]**, gilt nach Bestätigung für alle Arbeits-Sitzungen, die nach diesem Setting bauen.

Arbeits-Sitzungen **entscheiden selbst** (und begründen kurz im PR):

- technische Umsetzung, Datenformate, Algorithmen, Aufteilung auf Kerne, Speicherlayout
- Zahlenwerte für Balance (Dauer, Verbrauch, Wahrscheinlichkeiten), solange der Langzeit-Test sie belegt
- neue Inhalte innerhalb eines beschlossenen Gebiets (Ereignisse, Tätigkeiten, Gedanken, Gegenstände), passend zu Ton
  und Epoche
- Nebenfiguren und Gäste mit Namen und Charakter, solange sie zum Setting passen
- Grafikdetails im Rahmen von Stil-Leitfaden und Perspektive

**Zurück an den User** (als Auswahlfrage mit Empfehlung oder in ROADMAP „Offen“):

- neues Gebiet, Wegfall eines Gebiets, Reihenfolge der Gebiete und Phasen
- Ton, Epoche, Magie, die großen Bögen (Wahl, Grube, Lok, Grabstelle)
- Sykora, Taylor und die Startbesetzung (Namen, Wesen, Beziehungen)
- alles, was diesem Dokument widerspricht oder es ändert
- Abweichung von LEBENDIGKEIT.md oder vom Hardware-Budget

**Immer:** Inhalte als Daten, nicht im Code; Verbindungen zwischen Orten nur über den Weltzustand und den
Ereignis-Bus; nur berichten, was gebaut und im Langzeit-Test geprüft ist; keine Technik nach 1850 außer der Bahn,
keine Magie, kein Altern, kein Tod, kein Wegzug.

## 18. Offene Fragen

1. **Ideenliste von Claude:** Katze als Kupplerin zwischen Sykora und Taylor; Taylors Lieder wachsen mit dem Dorf
   (Lieder über echte Ereignisse, im Web lesbar); Vertrauen der Bewohner in Sykora (entscheidet die Wahl, spiegelt die
   Entscheidungen des Bürgermeisters); Lok taufen per Abstimmung am Schwarzen Brett.
2. **Gründungsgeschichte und Herkunft der Bewohner** (Kapitel 1 und 3): passt der Entwurf?
3. **Reihenfolge** (Kapitel 4): Grube vor Lok oder umgekehrt? Erste Region hinter der Strecke?
4. **Greta in der Morgendämmerung** als Ausnahme vom Tagesgerüst?
5. **Geld** (Kapitel 8): erst Tausch, Geld später?
6. **Nachholen** nach dem Ausschalten: wie lange (Vorschlag: höchstens 24 h)?
7. **Briefe und Gedanken** (Kapitel 12): Wer schreibt wem, wie oft, wie lang? Ansicht nach der Beispieldatei des
   Users (Pixelpfote-Vorschau). Vorgemerkt für die Themen Figuren, Gedächtnis, Wohnen und Besitz.
8. **Charta:** Die Mindestziele für Zug, Dungeon, Liga und Inselwelt (LEBENDIGKEIT Kapitel 2) passen nicht mehr.
   Vorschlag: Dorf-Ziele bleiben (bis auf „Wegzug“), dazu neue Ziele für Hof, Grube, Lok und die Welt als Ganzes.

## 19. Entscheidungen

Fest beschlossene Punkte dieser Werkstatt. Am Ende als Zeilen ins Entscheidungsprotokoll (PROJEKTPLAN Kapitel 14).

| Datum | Entscheidung | Begründung |
|---|---|---|
| 2026-10-20 | **Gemeinsames Setting:** eine Welt mit Gebieten als Modulen, ein sichtbarer Ort voll simuliert, die anderen vereinfacht im Hintergrund, Figuren wechseln zwischen Orten | Idee des Users (ROADMAP-Backlog) |
| 2026-10-20 | **Alte Welten verworfen** (Zugfenster, Dungeon-Held, Pixel-Liga, Inselwelt als eigene Welten); Projekt inhaltlich neu, gute Ideen werden weiterverwendet | Entscheidung des Users |
| 2026-10-20 | **Ton:** gemütlich, bodenständig, keine Magie | Entscheidung des Users |
| 2026-10-20 | **Dorf bleibt Herz**; Zug vor allem Reisemittel; Grube zum Sammeln von Rohstoffen, ohne Kampf, Weiterkommen nur mit Werkzeug oder Fähigkeit; neue Gebiete per Update | Entscheidung des Users |
| 2026-10-20 | **Kamera** folgt überwiegend der Hauptfigur, frei umschaltbar zu jedem bekannten Ort, auch zu leeren Orten mit ihrem echten Zustand | Entscheidung des Users |
| 2026-10-20 | **Feste Bewohner:** jede Figur ein Charakter mit Job und festem, sichtbarem Haus; keine zufälligen Statisten | Entscheidung des Users |
| 2026-10-20 | **Bauvorschläge** der Bewohner im Web beantworten; ohne Antwort nach 24 h entscheiden sie selbst | Wunsch des Users (Skript) |
| 2026-10-21 | **Epoche um 1850**, Handwerk ist echte Handarbeit, die Bahn ist die einzige moderne Technik; Dorf heißt **Pixelia**; Vorgeschichte mit Wassereinbruch verworfen | Entscheidung des Users |
| 2026-10-21 | **Sykora** (Mann, 30, freundlich, vom Land) ist die Hauptfigur, verliebt in **Taylor** (Schäferin, 30, blond, tierlieb, singt und spielt Gitarre in der Taverne), zweite Hauptfigur; der Funke ist bei ihr schon da | Entscheidung des Users |
| 2026-10-21 | **Bürgermeister per Wahl** als Meilenstein: vorher Tipps an Sykora, danach entscheidet der Betrachter auch über Bauvorhaben | Entscheidung des Users |
| 2026-10-21 | **Startbesetzung** mit Tischler, Bäckerin, Fischer, Jäger und den Brandts als altem Ehepaar; Endausbau **30–40 Bewohner in ~20 Häusern**; wiederkehrende Gäste | Entscheidung des Users |
| 2026-10-21 | **Niemand altert, stirbt oder zieht weg** | Entscheidung des Users |
| 2026-10-21 | **Tagesgerüst mit Zeitfenstern** (Schlaf nachts, Arbeit tags, Taverne und Feste abends); **Wetter** viermal am Tag änderbar; Jahreszeit 7 echte Tage | Entscheidung des Users |
| 2026-10-21 | **Großprojekte in Teilaufgaben** (z. B. Dampflok); **Hausbau mit Notunterkunft**, danach Raum für Raum | Entscheidung des Users |
| 2026-10-21 | **Innenräume und Möbel als Baukasten** mit Ausbaustufen, Farbvarianten und Erkennungsstücken; Sykoras Haus in der Weboberfläche anpassbar | Wunsch des Users |
| 2026-10-21 | **Austauschbare Asset-Quelle:** erst ohne SD-Karte, später bei Bedarf alles auf die Karte, Laden beim Ortswechsel hinter einer Überblendung | Entscheidung des Users |
| 2026-10-21 | **Perspektive größer, ein Bild pro Ort, kein Scrollen** (aus der Perspektiv-Sitzung) | Angabe des Users |
| 2026-10-21 | **Vorgeschichte „Der Winter, der nicht endete“:** Reisende eines Zuges strandeten im harten Winter im Bahnhof Pixelia, die Gleise wuchsen zu, sie blieben; der Zug ist verrostet | Idee des Users |
| 2026-10-21 | **Das Dorf startet klein:** provisorische Schlafplätze, nicht alle Gebäude von Anfang an | Entscheidung des Users |
| 2026-10-21 | **Taylor:** immer langes, offenes blondes Haar, keine Kopfbedeckung | Entscheidung des Users |
| 2026-10-21 | **Der Wirt ist ein Mann** (Gustav statt Hedwig) | Hinweis des Users |
| 2026-10-21 | **Romanze:** gemeinsame Wintererinnerung als Grundlage, Meilensteine „das verlorene Lamm“ und „der erste Tanz“ | Entscheidung des Users |
| 2026-10-21 | **Spielstart einige Wochen nach dem Entschluss zu bleiben**, erste Notunterkünfte halb fertig | Entscheidung des Users |
| 2026-10-21 | **Besondere Ereignisse über die Weboberfläche starten**, mit Voraussetzungen (gesperrte zeigen, was fehlt) und nur in ihrem Zeitfenster (Dorffest, Taverne nur abends); Vorbereitung tagsüber sichtbar, Orte geschmückt | Wunsch des Users: er schaut nicht rund um die Uhr hin |
| 2026-10-21 | **Zuschauzeiten** bestimmen, wann große Ereignisse außerhalb des Alltags passieren; Kamera springt hin (abschaltbar); Verpasst-Übersicht in der Weboberfläche | Entscheidung des Users |
