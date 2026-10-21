# Pixelia – Setting und Skript (Arbeitsdokument)

> **Arbeitsdokument der Setting-Werkstatt**, angelegt am 2026-10-20. Hier entsteht das gemeinsame Setting, also das
> „Skript“ der Welt, nach dem spätere Arbeits-Sitzungen bauen. Grundlage sind das Skript des Users
> („Skript Pixelia“, Stand 2026-10-20), seine Antworten in der Werkstatt und Vorschläge von Claude.
> **Inhalt, Ton und Geschmack entscheidet der User; Technik und Machbarkeit schlägt Claude vor.**
>
> Kennzeichnung: **[entschieden]** = vom User bestätigt (steht auch in Kapitel 19). **[Vorschlag]** = Entwurf von
> Claude, wartet auf das OK des Users. **[offen]** = noch zu klären.
>
> **Ausgelagert:** Die Wirtschaft (Waren, Kausalketten, Balancing, Protokoll, Prüfgrößen) steht in
> [WIRTSCHAFT.md](WIRTSCHAFT.md).
>
> Verbindlich bleibt [LEBENDIGKEIT.md](LEBENDIGKEIT.md): Systeme statt Schleifen, Individuen, sichtbare Folgen,
> zuverlässiges Gedächtnis (Regel 10).

## Ablauf der Werkstatt

| Schritt | Thema | Stand |
|---|---|---|
| 1 | Ideen des Users: Grundidee, Ton, Hauptfiguren, Skript | entschieden (Namen bleiben, 2026-10-21) |
| 2 | Gebiete, Figuren, Reihenfolge | entschieden (Grube vor Lok seit der Neuplanung, PR #31) |
| 2a | Wirtschaft: Waren, Berufe, Lager, Werkzeuge, Wünsche, Dauer, Protokoll | Grundsätze entschieden, Startwerte Vorschlag ([WIRTSCHAFT.md](WIRTSCHAFT.md), Kapitel 8) |
| 3 | Modularer Aufbau, Regie, Kamera | entschieden (Kapitel 13 und 15) |
| 4 | Hardware-Eignung mit Zahlen (inkl. SD-Karte, Baukasten) | entschieden (Kapitel 16) |
| 5 | Rahmenbedingungen für Claude | entschieden (Kapitel 17) |
| 6 | Planung: späte 0.5.x-Patches, Phasen ab 0.6.x, PR | übernommen von der Neuplanung (PR #31) |

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

- Im Dorf ist der Alltag vorindustriell: Laternen in der Hand, Öllampen und Kerzen in den Stuben, Kamine, Fackeln bei Festen,
  Brunnen, Schubkarren und Handkarren (später ein Ochsenkarren). **Schmiede und Tischlerei sind echte Handarbeit**, nichts ist automatisiert, die Bewohner
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
  **Güterschuppen das erste Lager**. Darin liegt noch Fracht, die für die Bergdörfer bestimmt war und nie ankam (Mehl,
  Kartoffeln, Salz, Lampenöl, Seile, Nägel, einfache Werkzeuge): der Restvorrat, der den ersten Frühling trägt
  (WIRTSCHAFT.md Kapitel 12). Jedes Haus, jede Werkstatt und Sykoras Hof müssen erst gebaut werden.
  **Zu Beginn finden die meisten Zusammenkünfte in der Taverne im Speisewagen statt** [entschieden, Wunsch des
  Users]: Dort kocht Gustav für alle, dort wird gegessen, erzählt und gesungen.
- **Warum der Zug wieder fahren soll:** Niemand will mehr weg. Die Lok wird repariert, damit Pixelia **angebunden**
  ist: Waren, Gäste, Briefe, neue Bewohner. Erst fährt sie bis zur nächsten Stadt hinter den zugewachsenen Gleisen,
  später (mit Updates) weiter; irgendwann wird vielleicht auch der Tunnel wieder frei.
- **Verbindung nach außen bis dahin:** der Bergpass. Solange er offen ist, kommt die Händlerin regelmäßig mit ihrem
  Maultier und bringt Post und Tauschware ([WIRTSCHAFT.md](WIRTSCHAFT.md) Kapitel 11), selten ein Wanderer. Im Winter ist Pixelia ganz für sich.

**Bogen über Monate [entschieden]**, keine feste Geschichte, sondern Meilensteine, deren Reihenfolge und Tempo aus der
Simulation entstehen:

1. *Der erste Frühling:* (Spielstart einige Wochen nach dem Entschluss) Feuerstelle vor dem Bahnhof, Schlafplätze in
   Wagen und Warteraum, Abende in der Taverne im Speisewagen, erste Notunterkünfte werden fertig, erster Garten. Die
   Katze taucht auf.
2. *Die ersten Häuser:* Notunterkünfte entstehen, Schwarzes Brett am Bahnsteig,
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
**Namen [entschieden 2026-10-21]:** Die Namen bleiben, wie sie sind. Die neuen Figuren heißen **Otto** und **Ida**
(Bauernpaar) und **Veit** (Waldarbeiter); [Vorschlag von Claude, änderbar].

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
| Anton Brandt, 34 | Jäger | Sohn der Brandts; hat im Winter alle mit Wild versorgt | Abteil der Familie | Jagdhütte am Waldrand mit Räucherkammer | Geweih an der Wand, Felle, Flinte über dem Kamin, Fallen |
| Martha, 72 | Kräuterfrau | **lebte schon hier**, half mit Kräutern durch den Winter | eigene Hütte am Waldrand | Kräutergarten, Pflanzenhaus | Kräuterbündel an der Decke, Mörser |
| Hannes, 52 | Fischer, schneidet Schilf für Dächer | **lebte schon hier**, wortkarg, kennt jedes Wetter | eigene Hütte am See | Steg, Bootsschuppen, Räucherofen | Reusen, Netze zum Trocknen, Boot am Steg |
| Otto, ~40 | Bauer: Felder, Hühner, später Mühle | Auswanderer auf dem Weg zum Hafen nach Amerika, Saatgut im Gepäck | Gepäckwagen | Bauernhof mit Scheune und Feldern | Saatgutkiste, Sense an der Wand |
| Ida, ~38 | Bäuerin: Gemüse, Hühner, Raps und Öl | wie ihr Mann; „wir wollten neues Land, jetzt haben wir es hier“ | Gepäckwagen | Bauernhof | Hühnerkorb, Einmachgläser, Bild vom Schiff, das sie nie bestiegen haben |
| Veit, ~25 | Holzfäller und Köhler | wollte im Sägewerk der Stadt arbeiten; kräftig, still | Warteraum | Holzfällerhütte am Waldrand (ein Raum) mit Holzstapel, später Meiler | Axt im Hackklotz, Holzstapel, rußige Hände |
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

**Wiederkehrende Gäste [entschieden]**, alle mit Namen und Geschichte: **Dora**, die Händlerin, die mit ihrem Maultier
über den Bergpass kommt, solange er offen ist (Tausch, Post, Neuigkeiten), **Balthasar**, der Kesselflicker auf
Wanderschaft (selten, erzählt in der
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

**Reihenfolge [entschieden 2026-10-21, Neuplanung PR #31]: Grube vor Lok.** Kapitel 3 „Die alte Grube“ (0.8.x) kommt
vor Kapitel 4 „Die Lok“ (0.9.x), denn die Lok braucht Kohle und Blei aus der Grube. Die Phasen stehen in
[ROADMAP.md](ROADMAP.md).

**Sonderorte [entschieden], verstreut und selten:** die Grabstelle eines Unbekannten am Waldrand (langes Rätsel ohne
Magie: Fundstücke aus Grube, Wald und Briefen erzählen über Monate, wer das war), ein verlassener Planwagen, die Ruine
auf dem Hügel, eine Höhle hinter dem Wasserfall.

### Das Museum: ein Naturalienkabinett [entschieden, Wunsch des Users 2026-10-21; Details Vorschlag]

**Idee des Users:** Im Museum leben eine Handvoll Fische, Insekten und andere Tiere im Aquarium oder Terrarium. Tippt
man ein Tier an, öffnet sich eine **Infokarte mit Pixel-Art-Bild und echten Angaben**. Das Museum füllt sich nach und
nach durch Spenden der Bewohner: Hannes bringt alle paar Tage einen neuen Fisch, Anton Insekten und so weiter.

**Passt zur Zeit:** Um 1850 hießen solche Sammlungen **Naturalienkabinett**, und Mitte der 1850er wurde das Aquarium in
Europa gerade Mode. Ein Museum ist in Pixelia also kein Fremdkörper, sondern genau das, was gebildete Leute damals
anlegten.

**Ausbaustufen:**

| Stufe | Was | Braucht |
|---|---|---|
| 1 | Kuriositätenregal im Warteraum des Bahnhofs: Steine, Federn, gepresste Pflanzen; ein Holzzuber als Schaubecken für den ersten Fisch | Bretter |
| 2 | Naturalienkabinett im Wartesaal mit Glaskästen, erstem Aquarium und Terrarium | Glas von der Händlerin (Aquarium 6, Terrarium 4), Bretter, Kalk |
| 3 | Eigenes Museum am Dorfplatz mit Aquarienwand und Terrarien (Großbau, Bonuswunsch) | Stein, Glas, Bretter |

**Sammlungen und Spender** (Startliste mit echten Arten aus Bergland, Wald und See; etwa 45 Stücke):

| Sammlung | Spender | Beispiele |
|---|---|---|
| Fische (Aquarium) | Hannes, alle paar Tage ein neuer | Bachforelle, Äsche, Hecht, Flussbarsch, Rotauge, Schleie, Elritze, Groppe, Aal, Quappe |
| Insekten (Terrarium) | Anton, Martha, Sykora | Hirschkäfer, Alpenbock, Schwalbenschwanz, Tagpfauenauge, Großes Heupferd, Gebänderte Prachtlibelle, Glühwürmchen (nur in Sommernächten), Maikäfer, Rote Waldameise |
| Amphibien, Reptilien, Weichtiere | Anton, Martha, Hannes | Feuersalamander, Bergmolch, Erdkröte, Laubfrosch, Zauneidechse, Blindschleiche, Weinbergschnecke, Edelkrebs |
| Pflanzen (Herbarium) [Idee angenommen] | Martha | Arnika, Enzian, Frauenschuh, Edelweiß, Bärlauch, Fingerhut, Wollgras |
| Steine, Mineralien, Fossilien [Idee angenommen] | Wilhelm (Steinbruch, Grube) | Bergkristall, Pyrit („Katzengold“), Bleiglanz, Ammonit, Farnabdruck aus der Steinkohle, Belemnit („Donnerkeil“); manche Funde gehören zum Rätsel der Grabstelle |
| Fundstücke der Katze [Idee angenommen] | die Katze | blaue Eichelhäher-Feder, Schneckenhaus, Hühnergott (Stein mit Loch) |

- **Wie Funde entstehen:** nebenbei bei der Arbeit, mit Ort, Jahreszeit, Tageszeit, Wetter und Seltenheit als
  Bedingung (Glühwürmchen nur in Sommernächten, Feuersalamander nach Regen, Fossilien erst in der Grube). Über alle
  Spender kommt etwa **alle 2–3 Tage ein neues Stück**; nach drei bis vier Monaten ist die Startliste fast voll. Neue
  Gebiete per Update bringen neue Arten (z. B. die Küste).
- **Infokarte:** großes Pixel-Art-Bild, Name (deutsch und wissenschaftlich), Herkunft und Lebensraum, Größe, Alter
  (wie alt die Art werden kann), kurze Beschreibung und ein Fun-Fact. **Alle Angaben sind echt.** Die Inhalts-Sitzung
  schreibt sie aus verlässlichen Quellen und nennt diese im PR; nichts wird erfunden. Die Karte ist Wissen für den
  Betrachter und darf deshalb auch Heutiges nennen („heute streng geschützt“).
- **Spender und Fundmoment [Idee angenommen]:** Auf der Karte steht auch, wer das Stück wann und wo gefunden hat, mit
  einem Satz des Spenders („Den hab ich im Morgengrauen am Steg erwischt.“). Der Bewohner behält den Fund als
  Erinnerung und ist stolz, wenn er im Museum vorbeischaut.
- **Pflege:** Paul Brandt wird Kustos: Er füttert die Tiere (Würmer und Wasserflöhe vom Teich, keine Ware), schnitzt
  Schildchen und Rahmen, Martha berät ihn. Kein Tier stirbt; die Karte nennt das echte Alter der Art.
- **Besucher:** Bewohner gehen in ihrer Freizeit hin (stillt die Neugier), Spender zeigen anderen ihren Fund.
- **Bedienung:** am Gerät ein Tier oder Stück antippen → Infokarte; in der Weboberfläche eine Museumsseite mit allen
  gefundenen Stücken, ihren Karten und Spendern.
- **Beispiel einer Infokarte** (Fakten vor der Umsetzung noch einmal prüfen): **Feuersalamander** (*Salamandra
  salamandra*) · Mittel- und Südeuropa, feuchte Laubwälder im Bergland · 15–20 cm · in der Natur über 20 Jahre, in
  Menschenobhut über 50 Jahre · schwarz mit gelben Flecken, die warnen: Seine Haut ist giftig. · Fun-Fact: Das Weibchen
  legt keine Eier, sondern setzt fertige Larven in kühle Bäche ab. · Gespendet von Anton, Tag 34, nach einem
  Sommerregen am Waldbach: „Saß einfach auf dem Weg und hat mich angeguckt.“
- **Daten pro Stück:** Kennung, Sammlung, Namen, Herkunft, Größe, Alter, Text, Fun-Fact, Quellen, Fundbedingungen,
  mögliche Finder, kleines Bild (fürs Aquarium, mit Schwimm-Animation) und großes Bild (für die Karte).

**Die Grube [entschieden: an bestimmten Stellen geht es nur mit Werkzeug oder Fähigkeit weiter; Rest Vorschlag]:**
Expeditionen mit einer kleinen Gruppe (Sykora, Wilhelm, wer Lust und Zeit hat). Kein Kampf. Spannung entsteht aus
Licht, Wasser und Gestein. Jedes Tor ist im Bild sichtbar, so wird die Grube über Wochen tiefer:

| Hindernis | Braucht | Kommt von |
|---|---|---|
| dunkle Stollen | Grubenlampe mit Rüböl oder Talg (wie damals die Unschlittlampen) | Konrad (Lampe), Bauernhof (Raps → Rüböl), Anton (Talg); Öl der Händlerin nur zusätzlich |
| verschütteter Gang | Spitzhacke, Bergbau geübt | Konrad, Wilhelm bringt es bei |
| harte Sohle ab dem zweiten Abschnitt | Grubenwerkzeug aus Stahl (die erste Spitzhacke reicht nur für Eingang und ersten Stollen) | Konrad, Stahl von der Händlerin |
| brüchige Decke | Stützbalken | Jakob, Holz aus dem Wald |
| senkrechter Schacht | Leiter, Seil | Jakob, Lene (Seil aus Hanf; anfangs Seile aus dem Restvorrat) |
| abgesoffener Abschnitt | Pumpe mit Lederdichtungen | Konrad, Jakob, Anton (Leder) |
| schwere Funde | Lore auf Holzschienen mit Eisenband | Konrad (Eisen), Jakob (Hartholz) |

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
(Frühaufsteher, Nachteule). **Schlafenszeit [entschieden 2026-10-21, Wunsch des Users]:** Frühaufsteher gehen gegen
22 Uhr ins Bett, die meisten gegen 23 Uhr, Nachteulen gegen 1 Uhr; aufgestanden wird je nach Charakter zwischen 5 und
7½ Uhr. So ist abends lange etwas los, wenn der Betrachter zusieht. [entschieden] Die Bäckerin backt in der Morgendämmerung, damit das Brot zum Frühstück da ist. Wer wann welche
Arbeit macht, steht als Arbeitsrhythmus der Berufe in [WIRTSCHAFT.md](WIRTSCHAFT.md) Kapitel 8.

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

### Wirtschaft: ausgelagert nach [WIRTSCHAFT.md](WIRTSCHAFT.md)

Die Wirtschaft ist so umfangreich geworden, dass sie eine eigene Datei hat (2026-10-21). Kurzfassung:

- **[entschieden, Wunsch des Users]** Kein Geld, auch nicht später mit der Bahn. Alles kommt aus echter Arbeit; jedes
  Haus, jedes Bett und jede Mahlzeit verbraucht Waren. Jeder Beruf liefert in sein eigenes Lager, wer etwas braucht,
  holt es selbst (Kausalkette). Gleichgewicht ohne Überfluss und ohne Leere. Niemand verhungert: Die Standardsuppe
  gibt es am **Suppenkessel im Speisewagen**, Bewohner holen sie sich und essen zu Hause. Die Taverne kocht tagsüber
  ein Tagesgericht für den Abend.
  Werkzeuge vom Schmied nutzen sich ab. Lagerplätze mit Reichweite und Chronik in der Weboberfläche.
- **[entschieden, Wunsch des Users]** **Sichtbare Bestände:** Was im Lager liegt, sieht man Stück für Stück im Bild,
  **jeder Brotlaib einzeln in der Auslage der Bäckerei** (WIRTSCHAFT.md Kapitel 6).
- **[entschieden, Wunsch des Users]** **Wirtschafts-Protokoll:** Bedarf, Verbrauch und Herstellung jeder Ware werden
  mitgeschrieben, damit eine spätere Sitzung nach einigen Tagen das Balancing verbessern kann (WIRTSCHAFT.md
  Kapitel 15).
- **[entschieden, ausdrücklicher Wunsch des Users]** **Prüfen statt raten: Jede Wirtschaftsänderung wird im
  PC-Simulator im Zeitraffer über mindestens drei Weltjahre geprüft; keine Ware länger als 2 Tage auf null (außer Ware
  von außen), kein Lager dauerhaft voll, Standardsuppe nach dem ersten Frühling nur selten** (alle Prüfgrößen in
  WIRTSCHAFT.md Kapitel 14).
- **[Vorschlag]** Etwa 70 Waren mit Quelle und Zweck, Kausalketten, Verbrauch und Herstellung mit Startwerten,
  Notlösungen statt Mangel, Händlerin mit fester Tauschtafel, Bilanz-Check für Start und Endausbau, Kipppunkte für die
  Zuzugsliste.

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
| Rost abklopfen und Kessel ölen | Drahtbürsten (Konrad), Rüböl (Bauernhof) |
| Kessel entkalken und Druckprobe | Essig (Händlerin), Wasser, Schmieden geübt |
| Siederohre ersetzen | Stahl, Kohle, Konrad (Meister) |
| Radlager neu ausgießen | Weißmetall aus Blei (Grube) und Zinn (Händlerin), Schmieden |
| Kolben und Stopfbuchsen neu packen | Hanf (Bauernhof, Lene), Talg (Anton) |
| Bremsklötze aus Hartholz | Hartholz (Eiche vom Waldarbeiter), Jakob |
| Pfeife, Laternen, Manometer instand setzen | Messing (Händlerin), Glas, Reparieren |
| Wasserkran am Bahnhof reparieren | Leder (Anton), Pumpe (Konrad, Jakob) |
| Kohle bunkern | Kohle aus der Grube |
| Gleise freilegen, verfaulte Schwellen ersetzen | Hartholz, Teer aus dem Meiler, Schienennägel (Konrad) |
| Weiche und Signal gangbar machen | Reparieren, Emil (Bahntechnik) |
| Probefahrt im Bahnhof, dann bis zum Tunnel | alles oben |

**[entschieden] Hausbau mit Notunterkunft:** Ein neues Haus wird gebaut, dann zieht jemand ein. Erst ist es eine
Notunterkunft (Kisten, Säcke, Strohsack, überall Gerümpel und Baumaterial). Danach entstehen Stück für Stück die
Räume: Küche (Feuerstelle → Herd), Schlafkammer (Strohsack → Bett), Stube (Kisten → Tisch und Bank → Ofen und Sofa),
Waschkammer (Zuber → Bad).

### Dauer in echter Zeit [entschieden 2026-10-21: halbierte Zeiten (Wunsch des Users); Einzelzahlen Vorschlag]

**Regeln des Users:** Kleine Arbeiten sind am selben Tag erledigt und stehen im Tagebuch der Weboberfläche (wer hat
was gemacht). Jedes Möbelstück baut der Tischler, deshalb hängt die Dauer eines Raums von seinen Möbeln ab. Es wird an
mehreren Häusern gleichzeitig gebaut, und der Tischler beliefert mehrere Häuser parallel. **Pro Haus entsteht immer nur
ein Raum zur Zeit.** Ausbaustufen sind etwas Eigenes. Die Jahreszeit bleibt bei 7 Tagen.

**Halbiert [entschieden]:** Fortschritt soll schneller sichtbar sein. Spätere Gebiete aus Updates dürfen langsamer
wachsen. Die Zeiten sind gegenüber dem ersten Entwurf halbiert:

| Art | Beispiele | Dauer | Wer |
|---|---|---|---|
| Kleine Arbeit | Dach flicken, Zaun reparieren, Regal aufhängen, Fenster abdichten, Werkzeug schärfen | **am selben Tag** (sichtbar 1–4 Stunden) | Bewohner selbst, Sykora, Paul |
| Möbelstück | Hocker, Regal: 2 Stunden · Tisch, Bank, Truhe: ½ Tag · Bett, Schrank: 1 Tag · Sofa: 1 Tag + ½ Tag Polster (Lene) | 2 Stunden bis 1½ Tage | Jakob |
| Raum: Rohbau | Wände, Boden, Fensterladen, Tür, Feuerstelle | 1½–2 Tage | Bautrupp (2–3 Leute) |
| Raum: Einrichten | 3–5 Möbel; Jakob fängt schon während des Rohbaus an | 2–3 Tage | Jakob, Bautrupp stellt auf |
| **Raum gesamt** | Küche, Schlafkammer, Stube, Waschkammer | **4–5 Tage, höchstens 7** | |
| Neues Haus bis zur Notunterkunft | Fundament, Wände, Dach, Tür, Strohsack | 2–3 Tage | Bautrupp |
| **Grundhaus** (Holzhütte) | Notunterkunft + 1–2 Räume mit einfachen Möbeln | **1½–2 Wochen** | |
| Haus mit allen Räumen | Grundhaus + weitere Räume | 2–3 Wochen | |
| Ausbaustufe Möbel | Strohsack → Bett, Bank → Sofa, Farbe wechseln | ½–1½ Tage | Jakob, Lene |
| Ausbaustufe Raum | Lehmboden → Dielen, Leinen oder Glas ins Fenster, Kalkanstrich, Ofen statt Feuerstelle | 1–2 Tage; der Raum bleibt benutzbar | Bautrupp, Jakob |
| Ausbaustufe Haus | Holzhütte → Fachwerkhaus → Steinhaus | 1–1½ Wochen pro Stufe, Raum für Raum; die Bewohner wohnen weiter darin | Bautrupp, Wilhelm (Stein) |
| Werkstatt, Stall | Backofen, Esse, Hühnerstall, Meiler | 4–7 Tage | Bautrupp + Handwerker |
| Großbau | Taverne → Schenke am Platz, Scheune, Wassermühle, Museum | 1½–2½ Wochen in Teilaufgaben | alle mit Zeit |
| Lok und Strecke | 13 Teilaufgaben (oben) | 5–6 Wochen, jede Teilaufgabe 2–4 Tage | Emil, Konrad, Jakob u. a. |

**Ablauf über die Monate [entschieden, Rückfrage des Users]:**

| Zeitraum | Was entsteht |
|---|---|
| Monat 0–2 | **Grundhäuser (Holzhütten) für alle 16 Startbewohner** und ihre Werkstätten, noch ohne Ausbaustufen; erste Felder und Ernte; die Wahl |
| Monat 2 bis 5–6 | Zuzügler und ihre Häuser, Ausbaustufen nach der Wunschliste (Fachwerk, Steinhaus, Glasfenster, Federbett, Badehaus), Großbauten (Taverne am Platz, Wassermühle, Museum), Grube und Lok |
| ab Monat 5–6 | **Endausbau** mit etwa 20 Häusern; danach weitere Bonuswünsche und neue Gebiete per Update |

**Damit kein Raum länger als eine Woche dauert:**

- Jakob arbeitet die Aufträge **reihum** ab, nicht Haus für Haus; so wartet niemand ewig. Faustregel: Ein Tischler
  schafft etwa einen eingerichteten Raum alle 2 Tage, mit Paul Brandt als Helfer für einfache Stücke (Hocker, Bänke,
  Regale) etwa alle 1½ Tage.
- Die Bauplanung beginnt nur so viele Räume gleichzeitig, wie Bautrupp und Tischler schaffen; die nächsten Wünsche
  warten in der Wunschliste.
- Hat Jakob keinen Auftrag, baut er **einfache Möbel auf Vorrat** (Hocker, Bänke), die beim nächsten Raum sofort
  bereitstehen.
- Beispiel Haus der Bäckerin: Notunterkunft 3 Tage, Backstube 5 Tage, Schlafkammer 4 Tage, also Grundhaus nach knapp 2
  Wochen; die Stube folgt mit 5 Tagen. Der Ausbau zum Steinhaus kommt später und dauert etwa 1½ Wochen.
- Material je Bauabschnitt: [WIRTSCHAFT.md](WIRTSCHAFT.md) Kapitel 7.

**Geht das, wenn jeder auch seinen Beruf hat? [Prüfung 2026-10-21, für die halbierten Zeiten neu gerechnet]** Ja. Die
Rechnung in Personentagen (ein Mensch, ein Arbeitstag). Weil die Zeit gestaucht ist wie beim Weltjahr, ist auch die
Arbeit pro Bauabschnitt kleiner gerechnet; die Zahl der Hände bleibt gleich:

| Posten | Arbeit |
|---|---|
| Notunterkunft | ~6 Personentage (Bautrupp), keine Möbel vom Tischler nötig |
| Raum: Rohbau | ~4 Personentage (Bautrupp) |
| Raum: Möbel | ~1¾ Tischler-Tage plus ¼ Tag Bretter sägen |
| Bautrupp zum Start | Sykora (ganzer Tag), Paul (gut ½ Tag, er ist 68) und Helfer aus der **Luft der Berufe**: Weil jeder Beruf 1,2- bis 1,5-mal so viel schafft wie nötig, hat er Stunden übrig. Zusammen etwa **2,5–3 Personentage pro Tag** |

- **Folge fürs Dorf:** Etwa **alle 2–2½ Tage eine Notunterkunft oder ein Raum**. Für die Startbewohner sind das etwa 17
  Neubauten (Notunterkünfte, Werkstätten, Ställe) und 20 Räume, zusammen ~180 Personentage, also **rund 2 Monate**.
  Danach wächst der Bautrupp mit den Zuzüglern; Endausbau nach **5–6 Monaten**.
- **Was sich in der Wirtschaft ändert:** Was pro Tag gegessen und verheizt wird, hängt an der Uhr und bleibt gleich.
  Was pro Tag **verbaut** wird (Bauholz, Bretter, Steine, Nägel, Lehm, Schilf), verdoppelt sich. Ausgeglichen durch:
  kleinere Materialmengen für die Hütten, mehr Bauholz pro Waldarbeiter-Tag, mehr Steine aus dem Steinbruch und
  **Holznägel** von Jakob für Dachstuhl und Fachwerk, die Eisen sparen. Im Winter bremst der Schnee den Bau; dann
  braucht der Waldarbeiter seine Kraft fürs Brennholz. Neue Bilanz in [WIRTSCHAFT.md](WIRTSCHAFT.md) Kapitel 12.
- **Sichtbare Folge von Tipps:** Schickt der Betrachter Sykora in die Grube, fehlt er dem Bautrupp, und der Bau geht
  langsamer. Grube und Lok kosten Arbeitskraft, die dann am Bau fehlt; die Reihenfolge der Wunschliste entscheidet mit.

**Ausbaustufen im Dorf [übernommen aus dem Skript]:**

| Ort | Stufen | Was er verändert |
|---|---|---|
| Feuerstelle vor dem Bahnhof → Dorfplatz | Feuerstelle, Platz mit Brunnen, gepflastert, Marktplatz | Treffpunkt, Märkte, Feste |
| Taverne | Speisewagen, Schenke am Platz, Gasthaus mit Zimmern; der Speisewagen bleibt danach Suppenküche | Abende, Erzählungen, Gäste übernachten |
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
  Wahl, erste Zugfahrt, Jahrestag) und aus wenigen Jahresfesten (Frühlingsmarkt, Erntefest und die vier
  Jahreszeitenabende mit Sommerfeuer und Lichterabend, Kapitel 11).
- **Jahreszeiten wechseln sonntags [entschieden 2026-10-21, Wunsch des Users]:** In der Nacht von Sonntag auf Montag
  beginnt die neue Jahreszeit; der Sonntagabend ist der letzte Abend der alten (Jahreszeitenabend, Kapitel 11). Der
  erste Frühling dauert bis zum ersten Sonntag, der mindestens eine Woche nach dem Spielstart liegt (7–13 Tage).
  Ist eine andere Länge eingestellt, ist der Jahreszeitenabend am letzten Tag der Jahreszeit.
- **Erstlinge** jeder Jahreszeit (erste Schwalbe, erster Schnee) werden einmalig als Erinnerung gespeichert.
- **Licht als System:** Laternen, Fackeln, Kerzen und Kamine brauchen Brennstoff aus dem Warenbuch. Das abendliche
  Anzünden ist ein Ritual, ein leerer Ort bleibt dunkel. Die Regeln stehen im nächsten Abschnitt.

### Licht: wann es brennt und wie hell [entschieden 2026-10-21]

**Grundidee (Wunsch des Users):** Nachts sieht man von außen, wer wach ist. Ein Fenster leuchtet nur, wenn dahinter
jemand wach ist und Brennstoff hat. Jede Ursache kommt aus der Simulation: Brennstoff fehlt, alle schlafen, alle sind
fort. Regeln von Claude, Aussehen und Außenlicht vom User gewählt.

- **Wann es brennt:** wenn es dunkel ist, jemand wach im Raum ist und Brennstoff da ist, auch morgens. Bei Greta
  brennt im Winter um 5 Uhr das erste Licht im Dorf. Wer zuerst heimkommt, zündet an (Ritual). Wer als Letzter ins
  Bett geht oder den Raum verlässt, löscht; draußen nimmt man die Laterne mit.
- **Kamin und Herd geben Licht,** schwach und flackernd: genug zum Essen und Reden, nicht für Handarbeit (Spinnen,
  Lesen und Schnitzen brauchen Kerze oder Lampe). Das Fenster glimmt dann orange. Zum Schlafen wird die Glut
  abgedeckt; sie glimmt im Raum, nach außen bleibt das Fenster dunkel.
- **Verbrauch nach Brenndauer:** Eine Kerze oder eine Lampenfüllung brennt etwa 2½ Stunden, ein „Lichtabend“
  ([WIRTSCHAFT.md](WIRTSCHAFT.md) Kapitel 7). Wer länger aufbleibt, verbraucht mehr.
- **Sparen mit Grund:** Wird das Licht knapp (Reichweite von Kerzen und Öl unter dem Soll), brennt nur noch ein Licht im
  Haus. Man sitzt am Kamin, geht früher ins Bett oder in die Taverne bzw. die Spinnstube; ganz ohne gibt es Kienspan.
  Wer abends in der Taverne sitzt, spart zu Hause Licht, wie damals.
- **Stärke** (die Engine setzt sie in Radius und Helligkeit um; mehrere Quellen in einem Raum addieren sich bis zu
  einer Obergrenze):

| Quelle | Stärke | Licht |
|---|---|---|
| Glut | 1 | rot, nur im Raum, kein Schein nach außen |
| Kienspan | 2 | gelb-orange, flackert, rußt |
| Talgkerze | 2 | warm, flackert |
| Wachskerze | 3 | warm, ruhiger |
| Kaminfeuer, Herd | 3 | orange-rot, flackert stark, hellt die Seite zum Feuer auf |
| Laterne (Hand- und Sturmlaterne) | 3 | wie Kerze oder Öl, fällt nach unten, wandert mit dem Träger |
| Rüböllampe | 4 | gelb-weiß, ruhig |
| Fackel | 4 | hell, unruhig, rußt |

- **Fenster nach außen:** Laden offen: voller Schein. **Laden zu: Lichtritzen** [Wahl des Users], dünne helle Linien
  durch die Fugen der Bretter. Geöltes Leinen: weicher, gelblicher Schein (etwa 60 %). Glas: klar und hell. Die Läden
  sind zu im Winter, bei Regen und Sturm und zum Schlafen; im Sommer bleiben sie offen, solange jemand wach ist. Im
  Winter ist das Dorf nachts voller Lichtritzen.
- **Taverne:** das hellste Fenster im Dorf (Lampen und Kamin). Gustav zündet die Laterne über der Tür an, wenn er
  öffnet, und löscht sie, wenn der letzte Gast geht. Licht über der Tür heißt „offen“.
- **Werkstätten** sind abends dunkel; Esse und Backofen werden nach Feierabend abgedeckt. Morgens brennt Licht in der
  Backstube.
- **Stall:** dunkel. Licht gibt es nur, wenn jemand mit der Sturmlaterne hineingeht (Lammen, Gewitter, krankes Tier);
  offenes Feuer gibt es im Stall nie.
- **Draußen [Wahl des Users: wie um 1850, dazu eine Dorfplatz-Laterne]:** Es gibt keine Straßenlaternen. Feste
  Lichter sind:
  - die **Bahnsteiglaterne**, die Emil in der Dämmerung anzündet und löscht, wenn er schlafen geht (sein Ritual als
    Lokführer);
  - die **Laterne über der Tavernentür** (Gustav);
  - später die **Laterne am Dorfplatz** als Bonuswunsch (braucht Glas von der Händlerin und jede Nacht Rüböl): Paul
    zündet sie in der Dämmerung an, Gustav löscht sie, wenn er die Taverne schließt.

  Wer nachts unterwegs ist, trägt eine **Handlaterne**; man sieht Lichter über die Wege wandern. **Licht für die, die
  noch unterwegs sind:** Ist jemand aus dem Haushalt noch fort (Taverne, Stall), stellt der, der daheim ist, eine
  Kerze ins Fenster; wer heimkommt, löscht sie. **Fackeln** gibt es nur bei Festen.
- **Echter Mond [entschieden, Idee von Claude, vom User gewählt]:** Der Mond am Himmel hat die Phase von heute (aus dem
  Datum gerechnet, ohne Internet) und steht nachts am Himmel, wenn es klar ist. Bei Vollmond ist die Nacht heller, und
  manche gehen ohne Laterne heim. Bei Neumond ist es stockdunkel, dann leuchten die Fenster umso mehr.
- **Prüfung:** Kein Fenster leuchtet ohne wache Person und Brennstoff dahinter. Ausnahmen mit Grund sind die Kerze
  für die, die noch unterwegs sind, und der Lichterabend (Kapitel 11). Kienspan höchstens 20 % der Lichtabende
  ([WIRTSCHAFT.md](WIRTSCHAFT.md) P4).

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

**[übernommen]** Rituale verschieben sich mit Jahreszeit und Wetter. **Zu Beginn finden die meisten Abende in der Taverne im Speisewagen statt** [entschieden]. **Abende als emotionales Herz:** gemeinsames
Essen, Gespräche (visuell), Musik (Taylor singt), Spiele, Geschichten, Schweigen; jeder Abend anders. Ereignisse:
kleine Unfälle ohne Strafe, Probleme mit Folgen, seltene Beobachtungen, Freude ohne Zweck, Ruhe als Ereignis. Zeit
sichtbar machen: wachsendes Tagebuch, selbst gepflanzter Baum, Steinhaufen mit Fundstücken.

**Texte [entschieden]:** eher kurz; ab und zu ein Gedanke oder Satz sichtbar.

**[entschieden]** Ein **Ereignis-Regisseur** für die ganze Welt wählt nach Verlauf, Spannungsbogen und Sperrzeiten aus
(LEBENDIGKEIT Regel 6). Ereignisse stehen als Daten in Seltenheitsstufen.

### Jahreszeitenabende [entschieden 2026-10-21, Wunsch des Users]

Am letzten Abend jeder Jahreszeit (sonntags, Kapitel 9) kommt das Dorf in großer Runde zusammen. Das ist ein fester
Termin im Kalender; der Regisseur legt nichts anderes darüber.

| Ende des … | Wo | Was |
|---|---|---|
| Winters | Taverne | großer Abend: Essen für alle, Musik, man erzählt vom Winter |
| Frühlings | Taverne | großer Abend in der Taverne |
| Sommers: **Sommerfeuer** | Dorfplatz (bis es ihn gibt: an der Feuerstelle vor dem Bahnhof) | großes Lagerfeuer zur Feier des Sommers auf dem geschmückten Platz (Kränze, Wimpel aus Stoffresten, Blumen, Laternen) |
| Herbstes: **Lichterabend** (Wintersonnenwende) | jedes Haus, dann die Taverne | in der Dämmerung stellt jedes Haus eine Kerze ins Fenster, dann geht man zusammen in die Taverne |

- **Fast alle kommen,** bis auf wenige mit Grund: Jemand ist krank (Martha sieht nach ihm), Taylor kommt in der
  Lammzeit später, Veit hält Wache am Meiler, Hannes bleibt nur kurz. Höchstens zwei bis drei fehlen.
- **Vorbereitung ist sichtbar:** Fürs Sommerfeuer schichten die Bewohner ab der Mitte des Sommers einen Holzstoß auf,
  der Tag für Tag wächst: Reisig, Äste und Bauholz-Reste, dazu etwas Rundholz, aber nur aus dem Überschuss
  ([WIRTSCHAFT.md](WIRTSCHAFT.md) Kapitel 7). In einem knappen Jahr wird das Feuer kleiner. Für den Lichterabend legen
  die Haushalte vorher Kerzen zurück; das sieht man im Lager.
- In der Taverne am Platz bleibt es dabei; mit 40 Bewohnern wird es eng und laut, und das ist gewollt.

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

- **Zuschauzeiten:** Der Betrachter stellt ein, wann er meist hinschaut (z. B. 18–23 Uhr). **Große Ereignisse, die
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
Taverne geflickt“), Tauschkiste und nächster Besuch der Händlerin, Anteil Standardsuppe, unter Diagnose das
**Wirtschafts-Protokoll** zum Herunterladen (WIRTSCHAFT.md Kapitel 15), die **Museumsseite** mit allen gefundenen
Stücken, Infokarten und Spendern (Kapitel 4).

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
nächster Ort; langes Halten auf einer Figur → „Folgen“; Tier oder Stück im Museum antippen → Infokarte (Kapitel 4).

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
| Museum: ~45 Stücke mit Text (~1 KB), kleinem Bild und großem Kartenbild (~4 KB) | ~0,25 MB | über die Asset-Quelle, später SD-Karte |
| Warenbuch (~70 Waren × ~40 Lagerplätze × 2 Byte) plus ~300 Einzelstücke (Werkzeuge, Möbel mit Zustand) | ~10 KB | vernachlässigbar |
| Sichtbare Bestände (Stellplätze, z. B. 20–40 Brotlaibe als einzelne kleine Bilder) | wenige Sprites mehr pro Ort | vernachlässigbar (48 Sprites zusammen 4,1 ms) |
| Wirtschafts-Protokoll (~1000 Bewegungen pro Welttag, gepackt ~25 KB) | 14 Tage Einzelbewegungen + 60 Tage Tageswerte ≈ 1,5 MB in LittleFS | passt; mit SD-Karte unbegrenzt |
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

**Prüfen statt raten [entschieden, ausdrücklicher Wunsch des Users]:** **Jede Arbeits-Sitzung, die Wirtschaftswerte
oder -logik einführt oder ändert, prüft sie im PC-Simulator im Zeitraffer (mindestens drei Weltjahre, mehrere Seeds)
gegen die Prüfgrößen in WIRTSCHAFT.md Kapitel 14 und legt den Bericht in die PR-Beschreibung. Ohne grünen Bericht
kein Merge.**

**Lesen vor dem Bauen:** Arbeits-Sitzungen lesen vor der Umsetzung die betroffenen Kapitel dieses Dokuments und von
[WIRTSCHAFT.md](WIRTSCHAFT.md) (gezielt über das Inhaltsverzeichnis, nicht alles am Stück).

**Immer:** Inhalte als Daten, nicht im Code; Verbindungen zwischen Orten nur über den Weltzustand und den
Ereignis-Bus; nur berichten, was gebaut und im Langzeit-Test geprüft ist; keine Technik nach 1850 außer der Bahn,
keine Magie, kein Altern, kein Tod, kein Wegzug.

## 18. Offene Fragen

1. **Ideenliste von Claude:** Katze als Kupplerin zwischen Sykora und Taylor; Taylors Lieder wachsen mit dem Dorf
   (Lieder über echte Ereignisse, im Web lesbar); Vertrauen der Bewohner in Sykora (entscheidet die Wahl, spiegelt die
   Entscheidungen des Bürgermeisters); Lok taufen per Abstimmung am Schwarzen Brett.
2. **Reihenfolge [entschieden 2026-10-21, Neuplanung PR #31]:** Grube vor Lok (Kapitel 4). Die erste Region hinter
   der Strecke wählt der User, wenn es soweit ist.
3. **Wirtschaft** ([WIRTSCHAFT.md](WIRTSCHAFT.md)): Warenliste, Händlerin, Kipppunkte, Startvorrat und die
   Wunschliste (Kapitel 8) sind Vorschläge und warten auf das OK des Users.
4. **Nachholen** nach dem Ausschalten: wie lange (Vorschlag: höchstens 24 h)?
5. **Briefe und Gedanken** (Kapitel 12): Wer schreibt wem, wie oft, wie lang? Ansicht nach der Beispieldatei des
   Users (Pixelpfote-Vorschau). Vorgemerkt für die Themen Figuren, Gedächtnis, Wohnen und Besitz.
6. **Charta [Kapitel 1 entschieden 2026-10-21]:** Die Mindestziele für Kapitel 1 stehen in
   [LEBENDIGKEIT.md](LEBENDIGKEIT.md) Kapitel 2; die Dorf-Ziele bleiben („Zuzug“ statt „Zu- oder Wegzug“). Die Ziele
   für Grube, Lok und die Welt als Ganzes legt der User zu Beginn der jeweiligen Phase fest.

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
| 2026-10-21 | **Wirtschaft in eigener Datei** [WIRTSCHAFT.md](WIRTSCHAFT.md): Waren mit Quelle und Zweck, Kausalketten, Notlösungen, Bilanz-Check | Umfang (Claude, Technik) |
| 2026-10-21 | **Sichtbare Bestände:** jedes Stück im Lager einzeln im Bild, z. B. jeder Brotlaib in der Auslage | Wunsch des Users |
| 2026-10-21 | **Wirtschafts-Protokoll** für das Balancing: Bedarf, Verbrauch, Herstellung, Arbeitszeit, Versorgung, Bau als CSV, Auswertung per Bericht | Wunsch des Users |
| 2026-10-21 | **Bautempo:** etwa alle 4–5 Tage eine Notunterkunft oder ein Raum; Startbewohner nach ~4–5 Monaten fertig, Endausbau nach ~10–12 Monaten (am selben Tag ersetzt durch die halbierten Zeiten, siehe unten) | Entscheidung des Users |
| 2026-10-21 | **Prüfen statt raten:** jede Wirtschaftsänderung im Zeitraffer gegen feste Prüfgrößen prüfen; ohne grünen Bericht kein Merge | ausdrücklicher Wunsch des Users |
| 2026-10-21 | **Bauzeiten halbiert:** Raum 4–5 Tage (höchstens 7), Notunterkunft 2–3 Tage; Grundhäuser (Holzhütten) aller Startbewohner nach ~2 Monaten, danach Zuzug und Ausbaustufen, Endausbau nach ~5–6 Monaten; Jahreszeit bleibt 7 Tage | Wunsch des Users: Fortschritt schneller sichtbar, spätere Gebiete dürfen langsamer wachsen |
| 2026-10-21 | **Museum als Naturalienkabinett:** echte Tiere in Aquarium und Terrarium, Infokarte mit Pixel-Art und echten Angaben, füllt sich durch Spenden der Bewohner; dazu Spender und Fundmoment auf der Karte, Steine, Fossilien und Pflanzen, Fundstücke der Katze | Wunsch des Users |
| 2026-10-21 | **Standardsuppe am Suppenkessel im Speisewagen**, Bewohner holen sie und essen zu Hause; die Taverne kocht tagsüber ein Tagesgericht für den Abend | Wunsch des Users, Ort von Claude gewählt (Kessel aus dem Winter) |
| 2026-10-21 | **Zu Beginn treffen sich die meisten in der Taverne im Speisewagen** | Wunsch des Users |
| 2026-10-21 | **Namen bleiben**; neue Figuren Otto und Ida (Bauernpaar), Veit (Waldarbeiter), Dora (Händlerin), Balthasar (Kesselflicker) | Entscheidung des Users, neue Namen von Claude vorgeschlagen |
| 2026-10-21 | **Die Bäckerin backt in der Morgendämmerung** | Hinweis des Users (abends backen ist unrealistisch) |
| 2026-10-21 | **Regel fürs Balancing: Erträge anheben statt Quellen vervielfachen:** reicht eine Ware nicht, wird beim Einstellen der Werte erst der Ertrag pro Tier, Acker oder Beruf angehoben, bevor mehr Tiere oder Äcker eingeplant werden; im laufenden Spiel ändert sich nichts | Wunsch des Users |
| 2026-10-21 | **Herstellung und Arbeitsrhythmus der Berufe** (WIRTSCHAFT.md Kapitel 8) übernommen; geprüft wird im Zeitraffer | Entscheidung des Users |
| 2026-10-21 | **Neue Namen:** Otto (statt Fritz) für den Bauern, Balthasar (statt Quirin) für den Kesselflicker | Entscheidung des Users |
| 2026-10-21 | **Licht-Regeln:** Licht brennt nur, wo jemand wach ist und Brennstoff hat; Kamin gibt schwaches Licht; geschlossene Läden zeigen Lichtritzen; draußen wie um 1850 (Bahnsteig, Taverne, später eine Dorfplatz-Laterne, Handlaternen, Fackeln nur bei Festen); echter Mond | Wunsch des Users (Fensterlicht), Regeln von Claude, Aussehen und Außenlicht vom User gewählt |
| 2026-10-21 | **Schlafenszeit eine Stunde später:** Frühaufsteher 22 Uhr, die meisten 23 Uhr, Nachteulen 1 Uhr | Wunsch des Users: er schaut abends länger zu |
| 2026-10-21 | **Jahreszeitenabende:** Jahreszeiten wechseln sonntags, am letzten Abend großes Zusammenkommen in der Taverne; am Sommerende das Sommerfeuer auf dem Dorfplatz (Holz wird angespart), am Herbstende der Lichterabend | Wunsch des Users |
| 2026-10-21 | **Mindestziele Kapitel 1** (LEBENDIGKEIT Kapitel 2) | Entscheidung des Users, Werte von Claude vorgeschlagen |
| 2026-10-21 | **Grube vor Lok:** Kapitel 3 „Die alte Grube“ (0.8.x) vor Kapitel 4 „Die Lok“ (0.9.x) | Entscheidung des Users in der Neuplanung (PR #31); die Lok braucht Kohle und Blei aus der Grube |
