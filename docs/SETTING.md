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
| 1 | Ideen des Users: Grundidee, Ton, Hauptfiguren, Skript | entschieden; offen: Namen der Bewohner |
| 2 | Gebiete, Figuren, Reihenfolge | entschieden bis auf die Reihenfolge Grube/Lok (Kapitel 18) |
| 2a | Wirtschaft: Waren, Berufe, Lager, Werkzeuge, Wünsche, Dauer | Vorschlag in Kapitel 8 (2026-10-21) |
| 3 | Modularer Aufbau, Regie, Kamera | entschieden (Kapitel 13 und 15) |
| 4 | Hardware-Eignung mit Zahlen (inkl. SD-Karte, Baukasten) | entschieden (Kapitel 16) |
| 5 | Rahmenbedingungen für Claude | entschieden (Kapitel 17) |
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

**Gründungsgeschichte [entschieden], wie aus Gestrandeten ein Dorf wird:**

- **Vor dem Winter** war Pixelia nur ein einsamer Haltepunkt an einer Nebenstrecke durchs Bergland: Bahnhofsgebäude
  mit Warteraum, Wasserturm, Lokschuppen, ein Güterschuppen. Ringsum Wald, ein See, Wiesen, oben am Berg eine längst
  aufgegebene Grube. Nur zwei Menschen lebten in der Nähe: Martha, die Kräuterfrau, in ihrer Hütte am Waldrand, und
  Hannes, der Fischer, am See.
- **Der Winter:** Ein Personenzug mit Abteilwagen, Speisewagen, Gepäckwagen und einem Viehwagen bleibt im Schnee
  stecken. Hinter dem Haltepunkt verschüttet ein Felssturz den Tunnel, durch den der Zug gekommen ist; nach vorne
  liegen die Gleise unter meterhohem Schnee. Die Reisenden überleben in den Wagen und im Warteraum. Gustav, der Koch
  des Speisewagens, kocht für alle, Anton jagt, Martha kennt die Kräuter gegen Husten, Hannes hackt Löcher ins Eis.
  Emil, der Lokführer, fühlt sich für alle verantwortlich und hält sie zusammen. Das Auswandererpaar teilt sein
  Saatgut nicht, sondern hütet es: Es soll im Frühling in die Erde.
- **Der Frühling:** Der Schnee taut, doch nach vorne sind die Gleise zugewachsen und unterspült, nach hinten bleibt der
  Tunnel verschüttet. Über den Bergpass kommt man nur zu Fuß, tagelang, und nur in der warmen Zeit (vom späten
  Frühling bis zum frühen Herbst). Am Abend nach der ersten
  warmen Nacht sitzen alle am Feuer vor dem Bahnhof und entscheiden: Wir bleiben. Das Schild am Bahnsteig gibt dem Dorf
  seinen Namen: **Pixelia**.
- **Hier beginnt das Spiel [entschieden]: einige Wochen nach dem Entschluss zu bleiben.** Es gibt den Bahnhof, den
  gestrandeten Zug, Marthas und Hannes' Hütten und die ersten halb fertigen Notunterkünfte auf der Lichtung. Die
  meisten schlafen noch provisorisch: in Abteilen, im Warteraum, im Gepäckwagen. Der **Speisewagen ist die erste Taverne**, der **Viehwagen der erste Stall** für Taylors Schafe, der
  **Güterschuppen das erste Lager**. Jedes Haus, jede Werkstatt und Sykoras Hof müssen erst gebaut werden.
- **Warum der Zug wieder fahren soll:** Niemand will mehr weg. Die Lok wird repariert, damit Pixelia **angebunden**
  ist: Waren, Gäste, Briefe, neue Bewohner. Erst fährt sie bis zur nächsten Stadt hinter den zugewachsenen Gleisen,
  später (mit Updates) weiter; irgendwann wird vielleicht auch der Tunnel wieder frei.
- **Verbindung nach außen bis dahin:** der Bergpass. Solange er offen ist, kommt die Händlerin regelmäßig mit ihrem
  Maultier und bringt Post und Tauschware (Kapitel 8), selten ein Wanderer. Im Winter ist Pixelia ganz für sich.

**Bogen über Monate [entschieden]**, keine feste Geschichte, sondern Meilensteine, deren Reihenfolge und Tempo aus der
Simulation entstehen:

1. *Der erste Frühling:* (Spielstart einige Wochen nach dem Entschluss) Feuerstelle vor dem Bahnhof, Schlafplätze in
   Wagen und Warteraum, erste Notunterkünfte werden fertig, erster Garten. Die Katze taucht auf.
2. *Die ersten Häuser:* Notunterkünfte entstehen, der Speisewagen wird zur Taverne, Schwarzes Brett am Bahnsteig,
   Werkstätten (Schmiede, Tischlerei, Bäckerei), Holzfällerhütte, die ersten Felder des Bauernhofs.
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

### Sykora [entschieden]

- **Mann, 30, freundlich**, auf dem Land aufgewachsen.
- Will die Zügel in die Hand nehmen und das Dorf nach vorne bringen; **bewirbt sich als Bürgermeister**.
- **In Taylor verliebt.**
- Eine Figur unter vielen: keine Sonderkräfte, keine Sonderwerte. Er muss lernen, helfen und Vertrauen gewinnen.
- [entschieden] War auf dem Weg in die Stadt, um dort Arbeit zu suchen, und saß im gestrandeten Zug. Schläft am Anfang
  in einem Abteil; **sein eigener Hof am Dorfrand ist sein erstes großes Projekt**. Startfähigkeiten: Holzarbeit geübt, Tierpflege und Gärtnern Anfänger, Lesen und Schreiben geübt (wichtig fürs Amt).
  Führt ein Tagebuch, das sichtbar auf dem Tisch wächst.
- **Schwäche [entschieden, Wunsch des Users]:** will zu viel auf einmal und vergisst dabei Pausen. Wenn er Taylor
  näher kennt, erzählt er ihr davon: dass ihm manchmal alles zu viel wird, dass er Angst hat, nicht alles zu schaffen,
  und dass er sich sorgt, was die anderen über ihn denken. [Vorschlag] Das ist ein einmaliger Abendmoment (z. B. auf
  Taylors Hügel über der Weide), sobald die beiden sich „vertraut“ sind. Vorher zeigen nur seine Gedanken die Sorge
  leise („Ob die anderen denken, ich schaffe das nicht?“); danach erinnert Taylor ihn ab und zu an eine Pause.
- Erkennungsstücke: Tagebuch, Hut am Haken, später die Amtskette bzw. das Bürgermeisterbuch.

### Taylor [entschieden]

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
- **Aussehen [entschieden]:** **immer langes, offenes blondes Haar, nie eine Kopfbedeckung.** [entschieden] Schürzenkleid
  in warmem Grün, Hirtenstab, Umhängetasche; abends in der Taverne ein Tuch um die Schultern.
- [entschieden] **Charakter:** herzlich, geduldig mit Tieren und Kindern, lacht viel und laut. Eigensinnig, wenn es um
  ihre Tiere geht. Verträumt: vergisst beim Hüten oder Singen die Zeit. Etwas scheu, wenn es um eigene Gefühle geht,
  deshalb braucht die Romanze Zeit.
- [entschieden] **Herkunft und Ziel:** Schäferstochter aus den Bergdörfern. Sie war mit ihrer kleinen Herde im
  Viehwagen unterwegs zum Markt in der Stadt und hat im Winter jedes Tier durchgebracht, Nacht für Nacht im kalten
  Wagen. Der Viehwagen ist ihr erster Stall. Wünscht sich eine richtige Weide mit Stall, Lämmer im Frühling und volle
  Abende in der Taverne.
- [entschieden] **Fähigkeiten zum Start:** Tierpflege Meisterin, Melken geübt, Musizieren und Singen geübt, Spinnen geübt,
  Kräuterkunde Anfängerin (lernt bei Martha).
- [entschieden] **Gewohnheiten:** singt beim Hüten, zählt abends ihre Tiere, bringt der Katze Milch, sitzt bei gutem
  Wetter auf dem Hügel über der Weide. Bei Gewitter geht sie zu den Tieren in den Stall.
- [entschieden] **Die Gitarre:** hat sie einst dem Kesselflicker gegen einen Winter voll Wolle abgetauscht; der
  Kesselflicker erinnert sich daran, wenn er kommt.
- [entschieden] **Beziehungen zum Start:** Gustav (der Wirt) ist wie ein Onkel für sie, mit Martha teilt sie die Liebe
  zu Pflanzen, mit Anton (Jäger) gibt es leise Reibung, weil er Tiere jagt.
- **Wohnt:** am Anfang im Abteil neben dem Viehwagen; später Schäferhaus mit Stall am Dorfrand. Erkennungsstücke: Gitarre an der Wand, Hirtenstab an der Tür,
  Schafsglocke, Wollkorb, Trockenblumen am Fenster.

### Der Betrachter

**[entschieden]** Der Betrachter beeinflusst Sykora am stärksten; die Kamera folgt ihm überwiegend.

**Bürgermeister [entschieden]: Wahl als Meilenstein.**

- Am Anfang führt Emil, der Lokführer, das Dorf kommissarisch, weil er die Reisenden durch den Winter gebracht hat.
  Er will das Amt aber abgeben und sich um seine Lok und seine Bienen kümmern.
- **Auch nach der Wahl bleibt Emil Sykoras wichtigster Ratgeber** (Kapitel 3), gibt Tipps und weiß, was für die
  Bewohner das Beste ist.
- Sykora sammelt Vertrauen, indem er hilft, baut und zuhört. Nach einigen Wochen ist Wahl auf dem Dorfplatz.
- **Vor der Wahl:** Der Betrachter gibt Tipps an Sykora. **Nach der Wahl:** Der Betrachter entscheidet zusätzlich über
  Bauvorhaben: Er ordnet die Wunschliste (Kapitel 8).

**Tipps [entschieden]:**

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
mit dem Jäger als Sohn; dazu ein Bauernpaar und ein Waldarbeiter (2026-10-21); Rest ebenfalls entschieden].**
16 Menschen und eine Katze. Vierzehn von ihnen sind Reisende aus dem gestrandeten Zug, zwei lebten schon vorher hier.
**Namen außer Sykora und Taylor werden noch überarbeitet** (Kapitel 18), die Tabelle zeigt den alten Stand.

| Figur | Rolle | Woher, warum im Zug | Schläft am Anfang | Baut sich später | Erkennungsstücke |
|---|---|---|---|---|---|
| **Sykora**, 30 | Hauptfigur, will Bürgermeister werden | vom Land, wollte in der Stadt Arbeit suchen | Abteil | eigenen Hof am Dorfrand | Tagebuch, Hut am Haken |
| **Taylor**, 30 | Schäferin, Sängerin | Schäferstochter, mit ihrer Herde unterwegs zum Markt | Abteil neben dem Viehwagen | Schäferhaus mit Stall | Gitarre an der Wand, Hirtenstab, Schafsglocke |
| Gustav, 58 | Wirt | Koch im Speisewagen, hat im Winter alle satt bekommen | Speisewagen (erste Taverne) | Taverne am Platz, wohnt oben | Kupferkessel, Kreidetafel mit Tagesgericht |
| Emil, 47 | Lokführer, Imker, kommissarischer Dorfvorsteher, später Ratgeber | fuhr den Zug, fühlt sich für alle verantwortlich | Lokschuppen | Haus am Bahnhof mit Bienenstöcken | Signallaterne, Taschenuhr, Fahrplan an der Wand, Honigtöpfe |
| Konrad, 56 | Schmied | auf dem Weg zu einer neuen Stelle in einer Fabrik | Gepäckwagen | Schmiede mit Esse | Amboss, Hufeisen über der Tür |
| Jakob, 41 | Tischler | Wandergeselle auf der Walz | Warteraum | Tischlerei | Hobelbank, Späne am Boden, halbfertiger Stuhl |
| Greta, 38 | Bäckerin | wollte in der Stadt eine Backstube übernehmen | Warteraum | Bäckerei mit Backofen | Brotschieber, Mehlsäcke, Brezel am Schild |
| Wilhelm, 66 | Bergmann a. D. | auf dem Weg zu seiner Schwester; erkennt die alte Grube am Berg | Abteil | Hütte am Hang | Grubenlampe, Erzbrocken auf dem Tisch |
| Paul Brandt, 68 | Zimmermann im Ruhestand, schnitzt | mit Frau und Sohn unterwegs zu Verwandten | Abteil der Familie | Haus der Brandts | Schnitzbank, Holzfiguren auf dem Sims |
| Lene Brandt, 66 | Weberin | wie Paul | Abteil der Familie | Haus der Brandts | Webstuhl, Garnkörbe |
| Anton Brandt, 34 | Jäger | Sohn der Brandts; hat im Winter alle mit Wild versorgt | Abteil der Familie | Jagdhütte am Waldrand | Geweih an der Wand, Felle, Bogen bzw. Flinte |
| Martha, 72 | Kräuterfrau | **lebte schon hier**, half mit Kräutern durch den Winter | eigene Hütte am Waldrand | Kräutergarten, Pflanzenhaus | Kräuterbündel an der Decke, Mörser |
| Hannes, 52 | Fischer, schneidet Schilf für Dächer | **lebte schon hier**, wortkarg, kennt jedes Wetter | eigene Hütte am See | Steg, Bootsschuppen, Räucherofen | Reusen, Netze zum Trocknen, Boot am Steg |
| Bauer, ~40 (Name folgt) | Bauer: Felder, Hühner, später Mühle | Auswanderer auf dem Weg zum Hafen nach Amerika, Saatgut im Gepäck | Gepäckwagen | Bauernhof mit Scheune und Feldern | Saatgutkiste, Sense an der Wand |
| Bäuerin, ~38 (Name folgt) | Bäuerin: Gemüse, Hühner, Raps und Öl | wie ihr Mann; „wir wollten neues Land, jetzt haben wir es hier“ | Gepäckwagen | Bauernhof | Hühnerkorb, Einmachgläser, Bild vom Schiff, das sie nie bestiegen haben |
| Waldarbeiter, ~25 (Name folgt) | Holzfäller und Köhler | wollte im Sägewerk der Stadt arbeiten; kräftig, still | Warteraum | Holzfällerhütte am Waldrand (ein Raum) mit Holzstapel, später Meiler | Axt im Hackklotz, Holzstapel, rußige Hände |
| Katze (Name vom Betrachter) | vollwertige Figur | streunt im Bahnhof herum, niemand weiß woher | wo es warm ist | bleibt bei Sykora oder Taylor | Lieblingskissen, Gabenhaufen |

Kinder gibt es zum Start nicht; Zugezogene können Kinder mitbringen, die Kinder bleiben.

**Emil, der Vertrauensmann [entschieden, Wunsch des Users 2026-10-21]:**

- Emil hält die Truppe zusammen, seit dem Winter. Er war damals nur ein Lokführer; die Zeit nach dem Stranden hat ihn
  **weise** gemacht. Er hat viel über die Natur (Wetter, Pflanzen, Bienen) und über den Umgang mit Menschen gelernt.
- **Vertrauensperson aller:** Wer Kummer hat, setzt sich zu ihm. Streit schlichtet er mit wenigen Worten.
- **Bleibt nach der Wahl in den Themen drin:** Er berät Sykora, gibt Tipps und weiß, was für die Bewohner das Beste
  ist. In der Wunschliste der Weboberfläche steht bei jedem Wunsch **Emils Rat** in einem Satz, mit echtem Grund aus
  der Simulation („Erst Gretas Dach, sonst regnet es ins Mehl.“).
- **Zwei Arbeiten am Tag:** Er bleibt der Lokführer und arbeitet tagsüber an der Lok, und er ist **Imker**. Morgens
  und am späten Nachmittag sieht er nach seinen Bienen, dazwischen ist er im Lokschuppen. Die Bienen liefern Honig
  (macht froh) und Wachs (gute Kerzen).

**Wiederkehrende Gäste [entschieden]**, alle mit Namen und Geschichte: die Händlerin, die im Sommer mit ihrem Maultier
über den Bergpass kommt (Tausch, Post, Neuigkeiten), der Kesselflicker auf Wanderschaft (selten, erzählt in der
Taverne). Später kommen Gäste mit dem Zug.

**Wachstum [entschieden]:** Endausbau **30–40 Bewohner in etwa 20 Häusern**, manche wohnen zu zweit (z. B. die
Brandts). Neue Bewohner kommen erst, wenn ein neues Haus gebaut ist. Sie ziehen in eine **Notunterkunft** ein, danach
wird Raum für Raum ausgebaut (Kapitel 8). Neue Figuren entstehen **einmal** (aus einer handgeschriebenen Liste oder aus
Bausteinen mit Herkunft, Ziel und Charakter) und bleiben für immer. [entschieden] Bis die Lok fährt, kommen Neue über
den Bergpass (Wanderer, Leute aus den Bergdörfern, die von Pixelia gehört haben), danach mit dem Zug.

## 4. Gebiete und Kapitel der Welt

**[entschieden]** Das Dorf ist das Herz. Der Zug ist vor allem Reisemittel. Die Grube (der „Dungeon“) dient zum
Rohstoffe-Sammeln, ohne Kampf. Neue Gebiete kommen mit Updates. Orte werden nach und nach zugänglich.

**[entschieden] Übersicht der ersten und wichtigsten Gebiete:**

| Kapitel | Gebiet | Orte (je ein Bild) | Zugang | Zweck |
|---|---|---|---|---|
| Start | **Bahnhof mit gestrandetem Zug** | Bahnsteig mit Feuerstelle, Warteraum, Abteilwagen (innen), Speisewagen als erste Taverne (innen), Viehwagen als Stall, Güterschuppen als Lager, Lokschuppen | ab Tag 1 | Keimzelle des Dorfs, Schlafplätze, Abende, später Lok-Bogen |
| Start | **Umgebung** | Waldrand mit Marthas Hütte, Wald mit Holzfällerhütte, See mit Hannes' Hütte, Wiesen, Lichtung für die ersten Häuser, Lehmgrube am Bach | ab Tag 1 | Holz, Pilze, Beeren, Kräuter, Wild, Wasser, Fisch, Schilf, Lehm |
| 1–2 | **Dorf Pixelia** (wächst) | Dorfplatz mit Brunnen und Schwarzem Brett, Häuserzeilen, Taverne, Schmiede, Tischlerei, Bäckerei (innen), Schäferhaus und Weide, Vorratsscheune | entsteht Haus für Haus | Gemeinschaft, Handwerk, Bauprojekte |
| 1–2 | **Bauernhof** | Hof mit Scheune, Felder (Weizen, Kartoffeln, Raps), Hühnerstall; später Wassermühle am Bach | Felder ab dem ersten Frühling | Brot, Gemüse, Stroh, Öl |
| 1–2 | **Sykoras Hof** | Stube mit Kamin und Küche, Schlafkammer, Hofplatz, Garten, Weide, Bach; später Werkstatt, Vorratskeller, Stall | Sykoras erstes großes Projekt | Zuhause, Selbstversorgung, Katze |
| 3 | **Die alte Grube** | Stolleneingang, mehrere Stollenabschnitte, Kohleflöz, tiefe Sohle | stufenweise über Werkzeug und Fähigkeiten | Kohle, Erz, Stein, Ton, Fundstücke |
| 3 | **Berghang** | Steinbruch, Aussichtspunkt | mit Wilhelm | Stein, Ausflüge, Lieblingsort |
| 4 | **Die Strecke** | zugewachsene Gleise, Brücke, Abteil im fahrenden Zug; der verschüttete Tunnel nach hinten | nach der Lok | Zugfahrt |
| 5+ | **Neue Regionen (Updates)** | z. B. Marktstadt, Küste, See, Bergdorf | mit dem Zug | wählt der User, wenn es soweit ist |

**Sonderorte [entschieden], verstreut und selten:** die Grabstelle eines Unbekannten am Waldrand (langes Rätsel ohne
Magie: Fundstücke aus Grube, Wald und Briefen erzählen über Monate, wer das war), ein verlassener Planwagen, die Ruine
auf dem Hügel, eine Höhle hinter dem Wasserfall.

**Die Grube [entschieden: an bestimmten Stellen geht es nur mit Werkzeug oder Fähigkeit weiter; Rest Vorschlag]:**
Expeditionen mit einer kleinen Gruppe (Sykora, Wilhelm, wer Lust und Zeit hat). Kein Kampf. Spannung entsteht aus
Licht, Wasser und Gestein. Jedes Tor ist im Bild sichtbar, so wird die Grube über Wochen tiefer:

| Hindernis | Braucht | Kommt von |
|---|---|---|
| dunkle Stollen | Grubenlampe mit Rüböl oder Talg (wie damals die Unschlittlampen) | Konrad (Lampe), Bauernhof (Raps → Rüböl), Anton (Talg); Öl der Händlerin nur zusätzlich |
| verschütteter Gang | Spitzhacke, Bergbau geübt | Konrad, Wilhelm bringt es bei |
| harte Sohle ab dem zweiten Abschnitt | Grubenwerkzeug aus Stahl (die erste Spitzhacke reicht nur für Eingang und ersten Stollen) | Konrad, Stahl von der Händlerin |
| brüchige Decke | Stützbalken | Jakob, Holz aus dem Wald |
| senkrechter Schacht | Leiter, Seil | Jakob, Lene (Seil) |
| abgesoffener Abschnitt | Pumpe mit Lederdichtungen | Konrad, Anton (Leder) |
| schwere Funde | Lore auf Schienen | Konrad, Jakob |

**Regel für Updates [entschieden]:** Jedes Projekt lässt sich mit dem vorhandenen Inhalt abschließen. Was auf ein Update
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

**Fähigkeiten [entschieden]: alle Figuren haben sie.**

- Stufen statt Zahlen: *unbekannt, Anfänger, geübt, Meister*. Jede Stufe schaltet Tätigkeiten frei (Flechten
  geübt → Körbe). Intern zählt ein kleiner Erfahrungswert.
- Gelernt wird durch Zuschauen, gemeinsames Arbeiten, Ausprobieren und Gezeigt-Bekommen. Dabei wächst die Beziehung
  **zwischen den beiden Figuren** (Konrad bringt Sykora das Schmieden bei, nach Wochen an der Esse sind sie Freunde).
- Etwa 27 Fähigkeiten zum Start: Kochen, Backen, Holzarbeit, Holzfällen, Tischlern, Zimmern, Mauern, Schmieden,
  Reparieren, Flechten, Spinnen, Weben, Nähen, Gärtnern, Ackerbau, Kräuterkunde, Pilzkunde, Tierpflege, Melken,
  Imkern, Fischen, Jagen, Bergbau, Musizieren, Erzählen, Lesen und Schreiben, Bahntechnik.

## 6. Tagesgerüst und Entscheidungslogik

**[übernommen]** Nutzenbewertung, Wiederholungsbremse, leichte Streuung, deterministischer Zufall mit Startwert,
Eingriffe als starker Wunsch.

**Tagesgerüst [entschieden]:** Tätigkeiten haben **Zeitfenster**:

- Schlafen nur nachts; tagsüber höchstens ein Nickerchen (Sofa, Ofenbank, im Gras).
- Gearbeitet wird nur tagsüber; nachts arbeitet niemand.
- Taverne, Feiern und Musik nur abends.

[entschieden] Zeitfenster hängen an der Tageszeit der Welt und verschieben sich mit Jahreszeit und Charakter
(Frühaufsteher, Nachteule). Ausnahme zum Besprechen: Greta beginnt in der Morgendämmerung mit dem Backen.

**Aufbau der Entscheidung [entschieden]:**

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
Beziehungen, einmaligen Momenten (erster Schnee). Kein Altern, wird mit der Zeit ruhiger und anhänglicher. [entschieden]
Taucht erst nach einigen Tagen auf und wählt selbst, bei wem sie bleibt.

**Wildtiere [übernommen]:** wiederkehrende Einzeltiere mit Vertrauensstufen (scheu, wartet, zutraulich, kommt auf
Ruf). [entschieden] Fuchs mit hellem Schwanzende, Reh mit Kitz im Frühling, Igel im Herbst, eine Krähe, die Glänzendes
sammelt, die Eule am Bahnhof, Glühwürmchen im Sommer.

**Nutztiere [übernommen]:** Hühner, Ziegen, Kuh, Schafe mit eigenem Rhythmus; liefern Eier, Milch, Wolle. Keine
Pflichtversorgung durch den Betrachter. [entschieden] Taylors Herde gehört zu den ersten Tieren, dazu die Hühner des
Bauernhofs und Emils Bienen (als Schwarm, keine Einzelfiguren). Nutztiere sind einfache
Figuren mit Namen, Farbe und einer Vorliebe (das Schaf, das immer ausbüxt), ohne volles Gedächtnis.

## 8. Bestand, Wirtschaft und Bauen

**Bestand [übernommen]:** sammeln, lagern, verarbeiten, verbrauchen. Jede Ware hat Menge und Ort. Einzelstücke mit
Herkunft und Zustand, gleichartige Waren als Mengen. Lager mit Kapazität. Verarbeitung verbraucht exakt.
Sammelertrag hängt von Ort, Jahreszeit, Ortszustand, Energie der Figur und Zufall ab. Vorräte sind sichtbar.

**[entschieden]** Ein **Warenbuch für die ganze Welt**; alle Orte buchen nur darüber.

### Grundsätze der Wirtschaft [entschieden, Wunsch des Users 2026-10-21]

- **Kein Geld, auch nicht später mit der Bahn.** Falls doch einmal, dann als eigenes Update. Getauscht wird Ware gegen
  Ware, im Dorf wird geteilt.
- **Alles kommt aus echter Arbeit.** Jede verbrauchte Ware hat jemand gesammelt, angebaut, hergestellt oder
  eingetauscht. Ein Kamin brennt nur mit Holz, das der Waldarbeiter wirklich geschlagen hat.
- **Jedes Haus, jedes Bett und jede Mahlzeit verbraucht Waren.** Häuser brauchen Baustoffe und Brennholz, Betten Stroh
  und Decken, Mahlzeiten Zutaten.
- **Kausalkette:** Jeder Bewohner gewinnt die Waren seines Berufs und bringt sie in **sein eigenes Lager** (der
  Waldarbeiter Holz und Äste in seine Holzfällerhütte, der Bauer Weizen in seine Scheune). **Wer etwas braucht, holt es
  sich selbst** (die Bäckerin holt abends Weizen bzw. Mehl für den nächsten Tag oder früh am Morgen).
- **Gleichgewicht:** Es darf nicht zu viel von einer Ware geben und gleichzeitig nichts von einer anderen. Die Logik
  dafür steht unten.
- **Niemand verhungert:** Fehlen die Zutaten, gibt es die **Standardsuppe**, mit Folgen für Laune und Kraft.
- **Lager sichtbar:** In der Weboberfläche sieht der Betrachter jeden Lagerplatz mit seinem Inhalt.

### Waren und wer sie liefert [Vorschlag]

Etwa 40 Waren in fünf Gruppen. „Ab“ sagt, wann die Ware ins Spiel kommt. Zuständig für die Liste und die Logik ist
diese Werkstatt; **die genauen Zahlen** stellt die Arbeits-Sitzung der Dorf-Phase im Langzeit-Test ein (Kapitel 17).

**Holz, Stein und Bau**

| Ware | Wer liefert | Gelagert | Wofür (Beispiele) | Ab |
|---|---|---|---|---|
| Brennholz | Waldarbeiter; alle sammeln nebenbei Reisig | Holzstapel an der Holzfällerhütte, Holzlege an jedem Haus | Kamine, Herde, Backofen, Feuer am Platz | Start |
| Bauholz (Stämme, Balken) | Waldarbeiter | Holzfällerhütte, Bauplatz | Hausbau, Dachstuhl, Stützbalken der Grube, Schwellen | Start |
| Bretter | Jakob (sägt aus Stämmen) | Tischlerei | Möbel, Dielen, Türen, Fensterläden | Start |
| Holzkohle | Waldarbeiter (Meiler: 2 Brennholz → 1 Holzkohle) | Meiler im Wald | Esse der Schmiede | Meiler gebaut (Woche 2–3) |
| Bruchstein | Wilhelm im Steinbruch | Steinbruch | Fundamente, Kamine, Brunnen, Steinhaus | Start |
| Lehm | alle (Lehmgrube am Bach) | am Bach | Fachwerk, Backofen, Kamine verputzen | Start |
| Schilf (Reet) | Hannes am See | Bootsschuppen | Dächer der ersten Häuser | Start |
| Kalk, Mörtel | Steinbruch + Kalkofen | Steinbruch | Steinhaus, Brunnen | Ausbau |
| Ziegel | Ziegelei (Lehm + Brennholz) | Ziegelei | Ziegeldach, Ziegelhaus | Ausbau (Zuzug Ziegler) |
| Glas | nur die Händlerin | Güterschuppen | Fensterscheiben, Laternen | Pass offen |

**Nahrung**

| Ware | Wer liefert | Gelagert | Wofür | Ab |
|---|---|---|---|---|
| Weizen | Bauer (Felder) | Scheune | Mehl; beim Dreschen fällt Stroh an | erste Ernte Ende des ersten Sommers; vorher Restvorrat aus dem Zug |
| Mehl | Greta mit der Handmühle, später Wassermühle am Bauernhof | Bäckerei | Brot, Kuchen zum Fest | Start (Restvorrat) |
| Brot | Greta | Bäckerei, Brotkorb im Haus | Frühstück, Proviant für Grube und Wald | Start |
| Gemüse (Kartoffeln, Kohl, Rüben, Bohnen) | Bäuerin, Hausgärten | Erdkeller, Keller | Eintopf, Suppe | Gärten ab Sommer 1; Restvorrat |
| Eier | Hühner am Bauernhof | Bauernhof | Mahlzeiten, Backen | Hühnerstall gebaut |
| Milch, Käse | Taylor (Ziegen, Schafe) | Schäferhaus, Käse im Keller | Frühstück, Taverne, Tausch | Start (Milch), Käse ab Käsekammer |
| Fleisch | Anton (Wild, Fallen) | Räucherkammer der Jagdhütte | Mahlzeiten; geräuchert oder gepökelt für den Winter | Start |
| Fisch | Hannes | Räucherofen am See | Mahlzeiten (im Winter Eisfischen, weniger) | Start |
| Kräuter, Pilze, Beeren | Martha; alle nebenbei | Marthas Hütte | Tee, Hustenmittel, Würze (Laune) | Start, nach Jahreszeit |
| Honig | Emil (Bienen) | Emils Haus | Süßes (Laune), Tausch | Sommer 1 |
| Salz | nur die Händlerin | Güterschuppen | Pökeln, Käse, Brot | Pass offen |
| Bier | Gustav (Gerste vom Bauern) | Keller der Taverne | Abende in der Taverne | Ausbau |

**Tiere, Stoffe und Licht**

| Ware | Wer liefert | Gelagert | Wofür | Ab |
|---|---|---|---|---|
| Wolle | Taylor (Schur im Frühling) | Schäferhaus | Garn | Start |
| Garn, Stoff | Lene (spinnt, webt) | Haus der Brandts | Decken, Vorhänge, Polster fürs Sofa, Kleidung | Start |
| Stroh | Bauer (beim Dreschen) | Scheune | Strohsack, Tierstreu, Dach | anfangs Rest aus dem Viehwagen, dann ab Ernte |
| Felle, Leder | Anton (gerbt) | Jagdhütte | Lederdichtungen (Pumpe, Lok), Riemen, Schuhe | Start |
| Talg | Anton | Jagdhütte | Kerzen, Grubenlampe, Lok (Stopfbuchsen) | Start |
| Wachs | Emil (Bienen) | Emils Haus | gute Kerzen, Holzpflege | Sommer 1 |
| Kerzen | Martha zieht sie aus Talg oder Wachs | Marthas Hütte, dann in jedem Haus | Licht am Abend | Start |
| Rüböl | Bäuerin (Raps) + Ölpresse | Bauernhof | Öllampen, Grubenlampe, Lok ölen | erste Rapsernte |
| Hanf, Seil | Bauer (Hanf), Lene (dreht Seile) | Haus der Brandts | Schachtseil der Grube, Lok packen | Ausbau |

**Metall und Werkzeug**

| Ware | Wer liefert | Gelagert | Wofür | Ab |
|---|---|---|---|---|
| Eisen | anfangs **knapp**: Alteisen aus dem Güterwagen, den der Felssturz am Tunnel zerdrückt hat (reicht eine Weile); dann die Händlerin; später Konrad aus Erz der Grube | Schmiede | Werkzeuge, Nägel, Beschläge, Lok | Start |
| Stahl | Händlerin; später Konrad selbst (Meister, mit Kohle) | Schmiede | Stahlwerkzeug, Grubenwerkzeug | Pass offen / Grube |
| Nägel, Beschläge | Konrad | Schmiede | jeder Bau, Möbel, Schienennägel | Start |
| Werkzeuge | Konrad, Stiel vom Tischler | beim Besitzer | jeder Beruf (unten) | Start |

**Aus der Grube** (Kapitel 4): Erz (Eisen schmelzen, Weißmetall für die Radlager), Kohle (Lok, Schmiede; Kamine
brauchen dann weniger Holz), Ton (Töpferware), Fundstücke (Rätsel der Grabstelle).

Später mit Zuzug und Updates: Ziegler, Müller, Steinmetz, Köhler als eigener Beruf, Kuh und Ochse (Pflug), Gerste und
Hopfen, Obstbäume, Waren aus der Stadt mit der Bahn.

### Lager und Wege: wer bringt, wer holt [Vorschlag]

- **Erzeugerlager** am Arbeitsplatz: Holzstapel an der Holzfällerhütte, Scheune am Bauernhof, Räucherkammer bei Anton.
  Jedes hat eine Obergrenze und ist im Bild sichtbar (der Stapel wächst und schrumpft).
- **Hausvorrat:** Holzlege (reicht etwa 2 Tage), Speisekammer, Brotkorb, Kerzenkiste.
- **Gemeinschaftslager** im Güterschuppen, später in der Vorratsscheune: Wintervorrat, Tauschkiste für die Händlerin,
  Ware von außen.
- **Holen:** Fällt ein Hausvorrat unter etwa einen Tag, holt der Bewohner selbst Nachschub (Korb: 10 Einheiten, später
  Schubkarre 30, Handkarren 60). Die Wege sind sichtbar und kosten auch im Hintergrund echte Zeit.
- **Kochen:** Anfangs kocht Gustav für alle im Speisewagen. Wer eine eigene Küche hat, kocht zu Hause; abends isst man
  oft in der Taverne.
- **Weboberfläche „Lager“:** jeder Lagerplatz mit Inhalt und Füllstand, Zu- und Abgänge des Tages und **Reichweite**
  („Brennholz reicht noch 3 Tage“); dazu eine Übersicht pro Ware über das ganze Dorf. Jede Bewegung steht in der Chronik
  („08:40 Greta holt einen Sack Mehl an der Mühle“).

### Verbrauch [Vorschlag, Startwerte]

Einheit: **1 Holz = ein Armvoll Scheite = eine Stunde Feuer** (die Rechnung des Users).

| Verbrauch pro Tag | Frühling | Sommer | Herbst | Winter |
|---|---|---|---|---|
| Kamin je Wohnhaus (abends ~6 h, im Winter auch morgens) | 5 | 1 | 6 | 9 |
| Herd je Küche | 2 | 2 | 2 | 2 |
| Backofen der Bäckerei | 6 | 6 | 6 | 6 |
| Taverne (Herd und Kamin) | 8 | 5 | 8 | 10 |
| Feuer am Dorfplatz (abends) | 3 | 3 | 3 | – |
| Schmiede (4 Holzkohle = 8 Holz) | 8 | 8 | 8 | 8 |

Ein Steinhaus braucht **ein Drittel weniger** für den Kamin, ein Kachelofen noch einmal weniger.

| Verbrauch pro Person und Tag | Menge |
|---|---|
| Brot | ½ Laib |
| Gemüse oder Kartoffeln | 2 Portionen |
| Fleisch, Fisch, Eier oder Käse | 1 Portion |
| Milch | ½ Krug |
| Licht | 1 Kerze bzw. Lampenfüllung pro Haus und Abend, im Winter 2 |
| Bett | Strohsack: 2 Stroh alle 14 Tage frisch stopfen; Decke hält etwa 8 Weltwochen |
| Kleidung, Schuhe | etwa alle 4 Weltwochen ein Stück Stoff bzw. Leder |

### Herstellung [Vorschlag, Startwerte]

Leistung pro Arbeitstag (etwa 8 Stunden) bei Fähigkeit „geübt“ und gutem Werkzeug:

| Beruf | Leistung | Faustregel |
|---|---|---|
| Waldarbeiter | 60 Brennholz **oder** 8 Stämme **oder** Meiler anlegen (2 Tage → 30 Holzkohle) | versorgt etwa 8 Wohnhäuser |
| alle nebenbei | 1–3 Reisig pro Person und Tag | etwa ein Siebtel des Holzbedarfs |
| Bauer und Bäuerin | 1 Acker Weizen ergibt 12 Säcke pro Ernte; das Paar bestellt 3 Äcker plus Gemüse und Raps | 1 Acker Weizen ≈ Brot für 8 Personen ein Weltjahr lang |
| Greta | 1 Ofenladung = 20 Laibe; Handmühle 1 Sack Mehl (= 10 Laibe) in 3 Stunden | eine Bäckerin + Wassermühle ≈ 40 Personen |
| Anton | im Schnitt 10 Portionen Fleisch, dazu Fell und Talg | |
| Hannes | 8–15 Portionen Fisch (Winter 4–6) | |
| Taylor | 8 Krüge Milch; einmal im Frühling Schur: 30 Wolle | |
| Lene | 2 Garn oder 1 Stoff | |
| Wilhelm | 6 Bruchstein (er ist 66; ein Steinmetz schafft später 12) | |
| Konrad | 2 Werkzeuge **oder** 40 Nägel **oder** 6 Werkzeuge schärfen | |
| Jakob | 1 Stamm → 6 Bretter in einer Stunde; Möbel siehe „Dauer“ | |

Die Leistung ändert sich mit Fähigkeit (Anfänger 0,6 · geübt 1,0 · Meister 1,3), Werkzeug (stumpf 0,7 · gut 1,0 ·
Stahl 1,3), Wetter (Regen bremst Feldarbeit auf 0,8; Holzfällen ist Winterarbeit und wird nicht gebremst) sowie Kraft
und Laune. Helfer ohne den Beruf schaffen etwa 40 % eines Profis.

### Gleichgewicht: wie das Dorf von selbst im Lot bleibt [Vorschlag]

1. **Reichweite statt Menge:** Das Warenbuch rechnet für jede Ware aus, wie viele Tage der Vorrat beim jetzigen
   Verbrauch noch reicht.
2. **Sollbestand und Obergrenze** pro Ware, in Tagen (Brennholz: Soll 4, Obergrenze 10; im Herbst für den Winter Soll
   10, Obergrenze 20). Liegt eine Ware unter dem Soll, macht der Erzeuger sie zuerst. Liegt sie über der Obergrenze,
   macht er etwas anderes (zweite Ware, Werkzeug pflegen, beim Bau helfen, Freizeit). Weil jeder Erzeuger immer an
   seiner knappsten Ware arbeitet, häuft sich nichts an und nichts bleibt leer.
3. **Notbestand** (unter einem Tag): Die Ware kommt als Gemeinschaftsaufgabe ans Schwarze Brett. Wer frei hat, hilft
   mit, z. B. bei einer sichtbaren „Holzaktion“.
4. **Kapazität mit Luft:** Die Startwerte sind so gewählt, dass jeder Beruf etwa das 1,3- bis 1,5-Fache des Bedarfs
   schafft. Die Luft ist Freizeit, Hilfe beim Bauen und Puffer für schlechtes Wetter.
5. **Wachstum braucht Leute:** Jedes neue Haus erhöht den Bedarf. Deshalb schreiben wir die Liste der Zuzügler so, dass
   mit dem Bedarf auch die Berufe wachsen (etwa ab 10 Häusern ein zweiter Waldarbeiter, dazu Müller, Steinmetz,
   Ziegler). Ausbauten senken den Bedarf (Steinhaus, Kachelofen, Kohle aus der Grube) oder heben die Leistung
   (Stahlwerkzeug, Wassermühle, Handkarren).
6. **Jahreszeiten:** Weizen, Gemüse und Wolle haben Erntezeiten. Im Herbst steigen die Sollbestände für den Winter
   (Holz machen, Einkellern, Räuchern). Ein schlechter Herbst zeigt sich im Winter durch mehr Standardsuppe und
   kühlere Häuser, ist aber nie bedrohlich.
7. **Prüfen statt raten:** Die Dorf-Sitzung lässt die Wirtschaft im PC-Simulator im Zeitraffer laufen (z. B. drei
   Weltjahre in wenigen Minuten). Geprüft wird: Keine Ware bleibt länger als 2 Tage auf null (außer Ware von außen),
   kein Lager ist dauerhaft voll, und die Standardsuppe gibt es nach dem ersten Frühling nur selten.

**Rechenbeispiel Brennholz** (nach dem Muster des Users):

- **Start im ersten Frühling:** Es brennen etwa 6 Feuerstellen (Warteraum, Ofen im Lokschuppen, zwei Notunterkünfte,
  die Hütten von Martha und Hannes), also 6 × (5 + 2) = 42 Holz. Dazu kommen 8 für den Speisewagen und 3 fürs
  Platzfeuer, zusammen etwa **53 Holz pro Tag**. Ein Waldarbeiter schafft 60, mit Reisig etwa 80. Das reicht mit Luft,
  und die Luft geht ins Bauholz.
- **Endausbau, 20 Häuser im Winter:** 20 × (9 + 2) = 220 Holz, dazu Backofen 6, Taverne 10 und Schmiede 8, zusammen
  etwa **245 Holz pro Tag** (im Jahresmittel etwa 170). Zwei Waldarbeiter als Meister mit Stahlaxt schaffen etwa
  2 × 60 × 1,3 × 1,3 ≈ 200, mit Reisig etwa 250. Im Winter ist das knapp, deshalb gibt es den Wintervorrat aus dem
  Herbst, und Steinhäuser und Kohle aus der Grube entlasten.

### Standardsuppe [entschieden, Wunsch des Users; Details Vorschlag]

- Fehlen für eine Mahlzeit die Zutaten, gibt es die **Standardsuppe** (im Dorf heißt sie „Wassersuppe“: Wasser, eine
  Handvoll Graupen und Kräuter). Sie ist immer verfügbar, **niemand hungert**.
- **Folgen:** Sie sättigt nur halb, der Hunger kommt also früher. Kraft und Laune sinken, Arbeit geht etwas langsamer.
  Einmal stört sie kaum, ab dem dritten Mal hintereinander deutlich. Dann erscheint der Gedanke „Schon wieder
  Wassersuppe …“ und am Schwarzen Brett ein Zettel („Wir brauchen Brot!“), der eine Gemeinschaftsaufgabe anstößt.
- In der Weboberfläche zeigt der **Anteil Standardsuppe** auf einen Blick, wie gut das Dorf versorgt ist.

### Werkzeuge [entschieden, Wunsch des Users; Haltbarkeit Vorschlag]

Jeder Beruf braucht Werkzeug vom Schmied (Eisen, Stiel vom Tischler). Ohne Werkzeug geht nur Handarbeit mit etwa 40 %
Leistung (Reisig sammeln statt Bäume fällen).

| Beruf | Werkzeug |
|---|---|
| Waldarbeiter | Axt, Säge, Spaltkeil |
| Bauer, Bäuerin | Hacke, Sense, Dreschflegel, später Pflug (mit Zugtier) |
| Bergmann, Steinbruch | Spitzhacke, Schlägel und Eisen, Brechstange |
| Tischler | Säge, Hobel, Stemmeisen |
| Bautrupp | Hammer, Maurerkelle, Nägel als Verbrauch |
| Schmied | Hammer und Zange (repariert er selbst) |
| Jäger | Messer, Fallen |
| Fischer | Haken (Schmied), Netz (Lene) |
| Schäferin | Schere, Melkeimer (Tischler) |
| Bäckerin | Brotschieber, Teigtrog (Tischler; nutzen sich kaum ab) |

- **Abnutzung:** Grundwerkzeug aus Eisen wird nach etwa 10 Arbeitstagen **stumpf** (Leistung 70 %). Konrad schärft es
  kurz und ohne Material. Nach dem dritten Schärfen ist es verbraucht und muss neu geschmiedet werden (1 Eisen + 1
  Stiel); ein Werkzeug hält also etwa 40 Arbeitstage.
- **Stufen:** Grundwerkzeug (Eisen) → **Stahlwerkzeug** (Leistung 130 %, hält doppelt so lange; Stahl erst von der
  Händlerin, später selbst gemacht) → **Grubenwerkzeug** (Stahl, nötig ab dem zweiten Abschnitt der Grube).
- Werkzeuge sind Einzelstücke mit Besitzer und Zustand; die Info-Karte zeigt „Axt stumpf, morgen zum Schmied“.

### Händlerin und Tausch [Vorschlag; beantwortet die Frage des Users nach dem Öl]

- **Wann sie kommt:** Der Bergpass ist vom späten Frühling bis zum frühen Herbst offen (etwa 2 der 4 Weltwochen). In
  dieser Zeit kommt sie **alle 3–4 Tage**, bleibt eine Nacht in der Taverne und zieht weiter. Das sind etwa vier Besuche
  pro Weltjahr; bei Unwetter kommt sie einen Tag später.
- **Tausch ohne Geld, aber sicher:** Es gibt eine feste **Tauschtafel** (z. B. 1 Käse = 2 Salz, 10 Wolle = 1 Glasscheibe,
  2 Felle = 1 Stahl). Was über der Obergrenze liegt, legen die Bewohner in die Tauschkiste im Güterschuppen. Was das
  Dorf von außen braucht, hängt als Bestellung am Schwarzen Brett. Die Händlerin tauscht **immer**, wenn die Bestellung
  durch Ware gedeckt ist; das ist kein Zufall. Im Web sieht man Tauschkiste, Bestellungen und den nächsten Besuch.
- **Sie bringt:** Salz, Glas, Stahl, Messing, Gewürze, Kaffee (selten, macht froh), Farbe (für Sykoras Haus), Saatgut,
  Rüböl, **Post und Neuigkeiten**. **Sie nimmt:** Käse, Wolle, Stoff, Felle, Honig, Paul Brandts Schnitzfiguren,
  Kräuter.
- **Regel:** Nichts Lebenswichtiges hängt allein an ihr. **Das Grubenlicht kommt aus dem Dorf:** Grubenlampen brennen
  Rüböl (Raps vom Bauernhof) oder Talg (Anton), wie die Unschlittlampen der Bergleute damals. Ihr Öl ist eine Ergänzung,
  vor allem vor der ersten Rapsernte. Was nur sie bringt (Salz, Glas), muss vor dem Winter eingetauscht sein, und das
  ist eine schöne Aufgabe für den Herbst.

### Wünsche am Schwarzen Brett [entschieden, Wunsch des Users; Details Vorschlag]

- **Grundbedarf** (Dach dicht, warm, Schlafplatz, Essen, Werkzeug, Reparaturen) erledigt die Gemeinschaft von selbst,
  ohne Rückfrage. Das Tagebuch zeigt, wer was gemacht hat.
- **Bonuswünsche** (ein besseres Haus, ein neues Zimmer, ein Steinhaus statt der Holzhütte, ein Sofa, Glasfenster,
  das Badehaus) hängen als Zettel am Schwarzen Brett und erscheinen in der **Wunschliste** der Weboberfläche.
- **Der Betrachter ordnet die Wunschliste:** was zuerst, was danach. Bei jedem Wunsch steht **Emils Rat** (Kapitel 3).
- Gebaut werden so viele Wünsche gleichzeitig, wie Bautrupp und Tischler schaffen (siehe Dauer), **höchstens ein
  Raum pro Haus**.
- **Bonuswünsche haben echte Wirkung:** Ein Steinhaus braucht ein Drittel weniger Holz, Glasfenster machen den Tag
  heller und sparen Kerzen, ein Federbett lässt besser schlafen, das Badehaus bringt Hygiene und Gesellschaft.
- Ein begonnener Bau reserviert sein Material. Grundbedarf geht vor: Liegt Brennholz im Notbestand, wird es nicht
  verbaut.
- [Vorschlag] **Vor der Wahl** ordnet Emil die Liste; der Betrachter kann über einen Tipp an Sykora einen Wunsch
  unterstützen, und Sykora redet dann mit Emil. **Ab der Wahl** ordnet der Betrachter.
- [Vorschlag] Ordnet der Betrachter länger nichts, kommen neue Wünsche ans Ende der Liste. Ist der Bautrupp frei und
  die Liste ungeordnet, nehmen die Bewohner den Wunsch, den Emil empfiehlt. Das ersetzt die frühere Regel „nach 24
  Stunden entscheiden die Bewohner selbst“; die Welt bleibt so nie stehen.

**Bauen [übernommen]:** Projekte brauchen Material, Arbeit und Zeit; jeder Ausbau verändert das Leben; kein fester
Bauplan; Anpassung aus Nutzung (oft am Feuer → Windschutz). Bauvorhaben laufen über die Wunschliste (oben).

**Großprojekte bestehen aus Teilaufgaben [entschieden].** Jede Teilaufgabe hat eigenes Material, eigene Fähigkeit und
einen sichtbaren Zwischenstand. Das verknüpft die Systeme: Die Lok braucht Kohle aus der Grube, Holz vom Tischler, Leder
vom Jäger, Eisen vom Schmied.

**[entschieden] Die Dampflok** (das längste Projekt, öffnet später neue Gebiete):

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

### Dauer in echter Zeit [neu gefasst 2026-10-21: Regeln entschieden (Wunsch des Users), Zahlen Vorschlag]

**Regeln des Users:** Kleine Arbeiten dauern einen Tag und stehen im Tagebuch der Weboberfläche (wer hat was
gemacht). Jedes Möbelstück baut der Tischler, deshalb hängt die Dauer eines Raums von seinen Möbeln ab; **ein Raum
dauert insgesamt höchstens 1–2 Wochen**. Es wird an mehreren Häusern gleichzeitig gebaut, und der Tischler beliefert
mehrere Häuser parallel. **Pro Haus entsteht immer nur ein Raum zur Zeit.** Ausbaustufen sind etwas Eigenes.

| Art | Beispiele | Dauer | Wer |
|---|---|---|---|
| Kleine Arbeit | Dach flicken, Zaun reparieren, Regal aufhängen, Fenster abdichten, Werkzeug schärfen | **1 Tag** (sichtbar 1–4 Stunden) | Bewohner selbst, Sykora, Paul |
| Möbelstück | Hocker, Regal: ½ Tag · Tisch, Bank, Truhe: 1 Tag · Bett, Schrank: 2 Tage · Sofa: 2 Tage + 1 Tag Polster (Lene) | ½–3 Tage | Jakob |
| Raum: Rohbau | Wände, Boden, Fenster, Tür, Feuerstelle | 3–4 Tage | Bautrupp (2–3 Leute) |
| Raum: Einrichten | 3–5 Möbel, Jakob fängt schon während des Rohbaus an | 3–6 Tage | Jakob, Bautrupp stellt auf |
| **Raum gesamt** | Küche, Schlafkammer, Stube, Waschkammer | **7–10 Tage, höchstens 14** | |
| Neues Haus bis zur Notunterkunft | Fundament, Wände, Dach, Tür, Strohsack | 5–7 Tage | Bautrupp |
| Haus fertig | Notunterkunft + 3 Räume nacheinander | etwa 4–6 Wochen | |
| Ausbaustufe Möbel | Strohsack → Bett, Bank → Sofa, Farbe wechseln | 1–3 Tage | Jakob, Lene |
| Ausbaustufe Raum | Lehmboden → Dielen, Glas ins Fenster, Tapete, Ofen statt Feuerstelle | 2–4 Tage; der Raum bleibt benutzbar | Bautrupp, Jakob |
| Ausbaustufe Haus | Holzhütte → Fachwerkhaus → Steinhaus | 2–3 Wochen pro Stufe, Raum für Raum; die Bewohner wohnen weiter darin | Bautrupp, Wilhelm (Stein) |
| Werkstatt, Stall | Backofen, Esse, Hühnerstall, Meiler | 1–2 Wochen | Bautrupp + Handwerker |
| Großbau | Taverne → Schenke am Platz, Scheune, Wassermühle | 3–5 Wochen in Teilaufgaben | alle mit Zeit |
| Lok und Strecke | 13 Teilaufgaben (oben) | 2–3 Monate, jede Teilaufgabe 3–7 Tage | Emil, Konrad, Jakob u. a. |

**Damit kein Raum länger als zwei Wochen dauert:**

- Jakob arbeitet die Aufträge **reihum** ab, nicht Haus für Haus, so wartet niemand ewig. Faustregel: Ein Tischler
  schafft etwa zwei Räume gleichzeitig in 10 Tagen; mit Paul Brandt als Helfer für einfache Stücke (Hocker, Bänke,
  Regale) etwa drei.
- Die Bauplanung beginnt nur so viele Räume gleichzeitig, wie Bautrupp und Tischler schaffen; die nächsten Wünsche
  warten in der Wunschliste.
- Hat Jakob keinen Auftrag, baut er **einfache Möbel auf Vorrat** (Hocker, Bänke), die beim nächsten Raum sofort
  bereitstehen.
- Beispiel Haus der Bäckerin: Notunterkunft 6 Tage, Backstube 10 Tage, Schlafkammer 8 Tage, Stube 10 Tage, also nach
  gut 5 Wochen fertig. Der Ausbau zum Steinhaus kommt später und dauert etwa 3 Wochen.

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
- **Farbliche Unterschiede:** Bett, Sofa, Vorhänge, Tapete usw. gibt es in Farbvarianten. [entschieden, Technik] Umgesetzt
  über **Palettentausch**: Ein Möbelstück wird einmal gezeichnet, die Farbe kommt aus einer austauschbaren Farbreihe.
  Kostet keinen zusätzlichen Speicher pro Farbe.
- **Erkennungsstücke:** Jedes Haus bekommt eigene Farben, Tapete, Boden und 3–5 Erkennungsstücke seiner Bewohner
  (Gitarre bei Taylor, Geweih beim Jäger, Mehlsäcke bei der Bäckerin; Tabelle in Kapitel 3).
- **Sykoras Haus ist anpassbar:** Haustyp, Farben und Möbel (z. B. Auswahl aus mehreren Sofas) in der Weboberfläche
  wählbar. [entschieden] Eine Auswahl wird zum kleinen Projekt: Sykora muss das Möbel bauen, tauschen oder streichen, das
  kostet Zeit und Material (Farbe von der Händlerin, Holz vom Tischler). Optional später auch für Taylor.
- **Eigene Möbel des Users [prüfen in 0.5.x]:** Der User zeichnet Möbelstücke nach Vorgaben (Maße im Raster, Palette)
  und lädt sie über die Weboberfläche hoch oder bringt sie per Update ins Repo. Siehe Kapitel 16.

## 9. Zeit, Kalender, Wetter und Licht

**[entschieden]**

- **Tageszeit = echte Zeit**, mehrere Lichtstufen von Morgen bis Nacht.
- **Jahreszeiten beschleunigt:** 7 echte Tage pro Jahreszeit (einstellbar), ein Weltjahr = 4 Wochen.
- **Wetter wird simuliert**, passend zur Weltjahreszeit, und kann sich **viermal am Tag ändern**: morgens, mittags,
  abends, nachts. Übergänge sind weich (Wolken ziehen auf, Regen setzt langsam ein).

**[entschieden]**

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

**[entschieden]** Ein **Ereignis-Regisseur** für die ganze Welt wählt nach Verlauf, Spannungsbogen und Sperrzeiten aus
(LEBENDIGKEIT Regel 6). Ereignisse stehen als Daten in Seltenheitsstufen.

### Besondere Ereignisse starten [entschieden, Wunsch des Users 2026-10-21]

Grund: Der Betrachter schaut nicht rund um die Uhr hin und würde besondere Momente sonst verpassen.

- **Start über die Weboberfläche mit Voraussetzungen.** Eine Liste zeigt alle besonderen Ereignisse (Dorffest,
  Tavernenabend, das verlorene Lamm …). Bereite Ereignisse lassen sich starten. **Gesperrte** zeigen, was fehlt
  (z. B. „erst, wenn Sykora und Taylor sich vertraut sind“, „das Fest braucht Essen und Musik“).
- **Nur zu passenden Uhrzeiten:** Jedes Ereignis hat ein Zeitfenster. Dorffest und Tavernenereignisse nur abends; ein
  Abendereignis kann tagsüber nicht gestartet werden. Außerhalb des Fensters ist es gesperrt mit Hinweis, ab wann es
  geht. [entschieden] Man kann es tagsüber für den Abend ansetzen.
- **Vorbereitung ist sichtbar:** Für ein Dorffest werden tagsüber Vorkehrungen getroffen (Bänke tragen, Girlanden und
  Laternen aufhängen, Essen kochen, Holz fürs Feuer). Die Orte sind **geschmückt und sehen anders aus**; der Schmuck
  bleibt bis zum Aufräumen am nächsten Tag. [entschieden] Ein Fest, das man abends startet, wurde dann „schon tagsüber
  vorbereitet“ nur, wenn es vorher angesetzt war; sonst wird es ein kleineres, spontanes Fest.
- [entschieden] Einmalige Meilensteine (das verlorene Lamm, der erste Tanz) lassen sich höchstens einmal auslösen;
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

[entschieden, Wunsch des Users 2026-10-21] Dazu: **Lager** (jeder Lagerplatz mit Füllstand, Zu- und Abgängen und
Reichweite), **Wunschliste** mit Emils Rat, **Tagebuch des Dorfs** (wer hat was gemacht, z. B. „Konrad hat das Dach der
Taverne geflickt“), Tauschkiste und nächster Besuch der Händlerin, Anteil Standardsuppe (Kapitel 8).

Eingreifen: Tipps an Sykora, Bitten an andere (Kapitel 2), Wunschliste ordnen (ab der Wahl), Kamera umschalten
(Kapitel 13), Namen vergeben, Sykoras Haus einrichten (Kapitel 8), eigene Möbel hochladen (falls machbar).

**Schwarzes Brett [übernommen]:** die Brücke zwischen Dorf und Weboberfläche. Was dort hängt, erscheint gleichzeitig
im Web. [entschieden] Beantwortete Zettel werden im Dorf abgenommen.

**Briefe mit Text [Wunsch des Users, 2026-10-21, auszuarbeiten]:** Briefe (vom Postboten, aus Nachbardörfern, von
Bewohnern untereinander) haben echten Text und eine eigene Ansicht, ähnlich der „Vorschau“ bei den Katzen im
CYD-Projekt Pixelpfote. Der User bringt eine Beispieldatei mit.

**Gedanken der Bewohner [Wunsch des Users, 2026-10-21]:** Was eine Figur gerade denkt (mit echtem Grund aus der
Simulation), ist auf der Info-Karte am Gerät und in der Weboberfläche lesbar, dort auch als Verlauf.

**[entschieden] Touch am Gerät:** Figur antippen → Info-Karte mit Grund; Gebäude antippen → wer ist drin; Wischen →
nächster Ort; langes Halten auf einer Figur → „Folgen“.

## 13. Kamera und Regie

**[entschieden]** Die Kamera zeigt überwiegend Sykora und geht mit ihm auf Reisen. Der Betrachter kann jederzeit zu
jedem bekannten Ort schalten, auch dorthin, wo niemand ist (leeres Haus, Kamin aus).

**[entschieden]**

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

**[entschieden, Technik]**

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
| Warenbuch (~40 Waren × ~40 Lagerplätze × 2 Byte) plus ~300 Einzelstücke (Werkzeuge, Möbel mit Zustand) | ~8 KB | vernachlässigbar |
| Wirtschafts-Takt (Bedarf, Reichweite, Soll/Obergrenze, Aufgaben vergeben) einmal pro Weltminute | < 1 ms | vernachlässigbar |
| Zeitraffer-Test der Wirtschaft im PC-Simulator (3 Weltjahre) | wenige Minuten | Balance prüfen ohne Gerät |
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

**Eigene Möbel des Users [entschieden, in 0.5.x prüfen]:** Technisch machbar.

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

**[entschieden]**, gilt nach Bestätigung für alle Arbeits-Sitzungen, die nach diesem Setting bauen.

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
2. **Namen der Bewohner** (Kapitel 3): werden überarbeitet; nicht alle passen oder sind einprägsam. Dazu Namen für
   Bauer, Bäuerin, Waldarbeiter, Händlerin und Kesselflicker.
3. **Reihenfolge** (Kapitel 4): Grube vor Lok oder umgekehrt? Erste Region hinter der Strecke?
4. **Greta in der Morgendämmerung** als Ausnahme vom Tagesgerüst? (Mit der Wirtschaft spricht viel dafür: Brot zum
   Frühstück.)
5. **Wirtschaft** (Kapitel 8): Warenliste, Startwerte, Händlerin und Wunschliste sind Vorschläge und warten auf das
   OK des Users.
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
| 2026-10-20 | **Bauvorschläge** der Bewohner im Web beantworten; ohne Antwort nach 24 h entscheiden sie selbst (2026-10-21 ersetzt durch die Wunschliste) | Wunsch des Users (Skript) |
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
| 2026-10-21 | **Alle bisherigen Vorschläge übernommen** (Gründungsgeschichte, Herkunft und Profile, Gebiete, Regie, Technik, Rahmenbedingungen), soweit der User sie nicht geändert hat | Entscheidung des Users |
| 2026-10-21 | **Kein Geld**, auch nicht später mit der Bahn (wenn doch, dann als eigenes Update) | Entscheidung des Users |
| 2026-10-21 | **Wirtschaft als Kausalkette:** Jeder Beruf liefert in sein eigenes Lager, Bewohner holen selbst; Häuser, Betten und Mahlzeiten verbrauchen Waren; Gleichgewicht ohne Überfluss und ohne Leere; Lagerplätze im Web sichtbar | Wunsch des Users |
| 2026-10-21 | **Standardsuppe** statt Hunger: niemand verhungert, aber Laune und Kraft sinken | Wunsch des Users |
| 2026-10-21 | **Werkzeuge** vom Schmied für jeden Beruf, sie nutzen sich ab; erst Grundwerkzeug, später besseres (z. B. für die Grube) | Wunsch des Users |
| 2026-10-21 | **Wunschliste:** Bonuswünsche vom Schwarzen Brett ordnet der Betrachter im Web; Grundbedarf erledigt die Gemeinschaft selbst | Wunsch des Users |
| 2026-10-21 | **Dauer:** kleine Arbeiten 1 Tag mit Tagebuch, ein Raum höchstens 1–2 Wochen, ein Raum pro Haus zur Zeit, mehrere Häuser parallel, Möbel vom Tischler | Wunsch des Users |
| 2026-10-21 | **Neue Figuren:** ein Auswanderer-Bauernpaar mit Saatgut und ein junger Waldarbeiter (Holzfäller, Köhler); Startbesetzung 16 Menschen | Entscheidung des Users |
| 2026-10-21 | **Emil** wird weise Vertrauensperson aller und bleibt nach der Wahl Ratgeber (Emils Rat in der Wunschliste); er ist weiter Lokführer und zusätzlich Imker | Wunsch des Users |
| 2026-10-21 | **Eisen ist anfangs knapp** (Alteisen aus dem zerdrückten Güterwagen, dann Händlerin, dann Grube) | Entscheidung des Users |
| 2026-10-21 | **Sykoras Schwäche:** Er vertraut Taylor an, dass ihm alles zu viel wird und er sich sorgt, was die anderen denken | Wunsch des Users |
