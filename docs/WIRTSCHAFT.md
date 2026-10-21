# Pixelia – Wirtschaft: Waren, Kausalketten und Balancing

> Teil des Settings, ausgelagert aus [SETTING.md](SETTING.md) Kapitel 8 (2026-10-21), weil die Wirtschaft zu groß für
> ein Kapitel geworden ist. Kennzeichnung wie dort: **[entschieden]** = vom User bestätigt, **[Vorschlag]** = Entwurf
> von Claude, **[offen]** = noch zu klären.
>
> **Zuständigkeit:** Die Logik (welche Waren, wer sie macht, wie das Gleichgewicht entsteht, was protokolliert und
> geprüft wird) legt die Setting-Werkstatt fest. Alle Zahlen hier sind **Startwerte**. Die genauen Werte stellt die
> Arbeits-Sitzung der Dorf-Phase mit dem Wirtschafts-Protokoll (Kapitel 15) und den Prüfgrößen (Kapitel 14) ein.

> **Prüfen statt raten [entschieden, ausdrücklicher Wunsch des Users 2026-10-21]:**
> **Jede Arbeits-Sitzung, die Wirtschaftswerte einführt oder ändert, lässt die Wirtschaft im PC-Simulator im
> Zeitraffer laufen (mindestens drei Weltjahre) und weist nach, dass alle Prüfgrößen aus Kapitel 14 eingehalten sind:
> keine Ware bleibt länger als 2 Tage auf null (außer Ware von außen), kein Lager ist dauerhaft voll, und die
> Standardsuppe gibt es nach dem ersten Frühling nur selten. Ohne diesen Nachweis wird keine Wirtschaftsänderung
> gemergt.** Zusätzlich werden echte Läufe auf dem Gerät über das Wirtschafts-Protokoll ausgewertet.

## Inhalt

1. Grundsätze
2. Zeit, Einheiten und was „realistisch“ heißt
3. Berufe: wer was liefert
4. Waren
5. Kausalketten
6. Lager, Wege und sichtbare Bestände
7. Verbrauch
8. Herstellung, Arbeitsrhythmus, Jahreslauf und Natur
9. Notlösungen statt Mangel
10. Werkzeuge
11. Händlerin und Tausch
12. Bilanz-Check und Kipppunkte
13. Gleichgewicht: die Regeln
14. Prüfen statt raten: Prüfgrößen
15. Wirtschafts-Protokoll für das Balancing
16. Offene Punkte

---

## 1. Grundsätze [entschieden, Wunsch des Users 2026-10-21]

- **Kein Geld**, auch nicht später mit der Bahn (wenn doch, dann als eigenes Update). Getauscht wird Ware gegen Ware,
  im Dorf wird geteilt.
- **Alles kommt aus echter Arbeit.** Jede verbrauchte Ware hat jemand gesammelt, angebaut, hergestellt oder
  eingetauscht. Ein Kamin brennt nur mit Holz, das der Waldarbeiter wirklich geschlagen hat.
- **Jedes Haus, jedes Bett und jede Mahlzeit verbraucht Waren.**
- **Kausalkette:** Jeder Bewohner gewinnt die Waren seines Berufs und bringt sie in **sein eigenes Lager** (der
  Waldarbeiter Holz und Äste an seine Holzfällerhütte, der Bauer Weizen in seine Scheune). **Wer etwas braucht, holt es
  sich selbst** (die Bäckerin holt abends Weizen für den nächsten Tag oder früh am Morgen).
- **Gleichgewicht:** nicht zu viel von einer Ware und gleichzeitig nichts von einer anderen (Kapitel 13).
- **Niemand verhungert:** Fehlen die Zutaten, gibt es die **Standardsuppe** (Kapitel 9).
- **Lager sichtbar:** in der Weboberfläche jeder Lagerplatz mit Inhalt, Reichweite und wer wann was geholt hat; im
  Bild jedes Stück einzeln, z. B. jeder Brotlaib in der Auslage (Kapitel 6).
- **Werkzeuge** vom Schmied für jeden Beruf, sie nutzen sich ab (Kapitel 10).

## 2. Zeit, Einheiten und was „realistisch“ heißt [Vorschlag]

- Ein **Welttag ist ein echter Tag**. Eine Jahreszeit dauert 7 Tage, ein **Weltjahr 28 Tage**. Aussaat, Ernte, Schur
  und Heu kommen also einmal pro Weltjahr, d. h. etwa alle vier echten Wochen.
- Gearbeitet wird tagsüber, etwa **8 Stunden**; nachts arbeitet niemand (SETTING.md Kapitel 6).
- Weil die Zeit gestaucht ist, rechnen wir **nicht mit echten Hektar-Erträgen**. „Realistisch“ heißt hier dreierlei:
  1. **Die Verhältnisse stimmen:** Ein Waldarbeiter versorgt etwa 11 Häuser, ein Bauernpaar etwa 25–30 Menschen, eine
     Bäckerin mit Handmühle etwa 30.
  2. **Jede Menge ist im Bild glaubhaft:** ein Holzstapel, 20 Laibe in der Auslage, ein Dutzend Säcke in der Scheune.
  3. **Jede Ware hat eine Quelle und einen Zweck.** Nichts fällt vom Himmel, nichts wird nur gesammelt, um zu liegen.
- **Regel fürs Balancing: Ertrag anheben statt Quellen vervielfachen [entschieden, Wunsch des Users]:** Zeigt die
  Prüfung (Kapitel 14), dass eine Ware nicht reicht, wird beim Einstellen der Werte zuerst der Ertrag pro Tier, Acker
  oder Beruf angehoben, statt mehr Tiere oder Äcker einzuplanen (keine 200 Schafe auf der Weide). Das ist eine Regel für
  die Arbeits-Sitzungen, die die Werte festlegen; **im laufenden Spiel ändert sich dadurch nichts**, die Erträge sind
  feste Werte. Die Startwerte in dieser Datei sind schon danach gewählt.

| Einheit | Bedeutung |
|---|---|
| Brennholz | ein Armvoll Scheite = **eine Stunde Feuer** (Rechnung des Users) |
| Rundholz | ungespaltenes Brennholz; 1 Rundholz wird beim Spalten zu 1 Brennholz |
| Stamm | Bauholz; 1 Stamm ergibt 6 Bretter |
| Baum | Bestand im Wald; 1 Baum ergibt 40 Rundholz **oder** 4 Stämme, dazu Reisig, Rinde, Kienspan |
| Sack | Korn oder Mehl; 1 Sack Mehl ergibt 10 Laibe |
| Portion | eine Person, eine Mahlzeit, eine Zutat |
| Krug | Milch oder Öl |
| Bund | Stroh, Heu, Schilf |
| Stück | Kerze, Seife, Stoff, Kleidung, Werkzeug, Möbel |
| Wolle | 1 Schaf gibt 12 Wolle pro Schur; 1 Wolle wird zu 1 Garn, 2 Garn zu 1 Stoff |

## 3. Berufe: wer was liefert [Vorschlag]

Namen wie in SETTING.md Kapitel 3; neu sind Otto und Ida (Bauernpaar), Veit (Waldarbeiter) und Dora (Händlerin).

| Figur | liefert | eigenes Lager | holt sich vor allem |
|---|---|---|---|
| Veit (Waldarbeiter) | Rundholz, Stämme, Hartholz (Eiche), Reisig, Rinde, Kienspan; im Meiler Holzkohle und Teer | Holzplatz an der Holzfällerhütte, Meiler | Brot, Essen |
| Otto (Bauer) | Weizen, Stroh, Saatgut, Heu (mit Taylor); später Gerste, Hanf, Mehl aus der Wassermühle | Scheune | Stiele, Werkzeug |
| Ida (Bäuerin) | Kartoffeln, Gemüse, Raps → Rüböl (Ölpresse), Eier, Federn | Erdkeller, Hühnerstall, Ölkammer | Kleie für die Hühner |
| Taylor | Milch, Käse, Wolle, Garn (spinnt beim Hüten), Mist, Heu (mit dem Bauern) | Schäferhaus, Käsekammer | Labkraut, Salz, Heu |
| Greta | Brot (mahlt anfangs selbst mit der Handmühle: Mehl und Kleie) | Auslage, Mehlkammer | Weizen, Rundholz |
| Gustav | Mahlzeiten (anfangs für alle), später Bier | Speisekammer, Keller | alle Zutaten |
| Anton | Fleisch (räuchert), Felle → Leder (gerbt mit Rinde), Talg | Jagdhütte mit Räucherkammer | Rinde, Pulver |
| Hannes | Fisch (räuchert), Schilf (schneidet im Winter) | Räucherofen, Bootsschuppen | Garn für Netze |
| Martha | Kräuter, Pilze, Beeren, Wildgemüse, wilder Hopfen, Labkraut; zieht Kerzen, siedet Seife | Kräuterhütte | Talg, Wachs, Rüböl, Asche, Garn (Dochte) |
| Emil | Honig, Wachs (Bienen); Arbeit an der Lok | Emils Haus | – |
| Konrad | Werkzeuge, Nägel, Beschläge, Drahtbürsten, Fallen, Haken, Lampen, Lok-Teile; später Eisen aus Erz | Schmiede | Eisen, Holzkohle, Stiele |
| Jakob | Bretter, Möbel, Holznägel, Stiele, Schubkarren, Fässer, Zuber, Eimer, Leitern, Dreschflegel, Spinnrad, Webstuhl, Ölpresse (mit Konrad) | Tischlerei | Stämme, Hartholz, Nägel |
| Wilhelm | Bruchstein, Kalkstein, Kalk (Kalkofen); später aus der Grube Erz, Kohle, Blei, Ton | Steinbruch | Holz für den Kalkofen |
| Lene | Garn, Stoff, Kleidung, Decken, Polster, Seile und Leinen (aus Hanf), Netze, Dochte | Haus der Brandts | Wolle, Hanf |
| Paul | Bautrupp (Zimmermann), einfache Möbel, Schnitzfiguren zum Tauschen; Kustos des Museums | – | Holzreste |
| Sykora | Bautrupp, eigener Hof (Garten, später Obst), hilft überall | – | – |
| **alle** | nebenbei Reisig, Lehm, Sand, Laubstreu, Wildgemüse; jeder Haushalt **spaltet sein Brennholz** am eigenen Hackklotz und pflegt ab dem eigenen Haus einen Garten | – | – |

## 4. Waren [Vorschlag]

Etwa 70 Waren. „Ab“ sagt, wann die Ware ins Spiel kommt; zum Verderb siehe Kapitel 7. **Wasser** aus Brunnen, Bach
und See ist keine Ware: Es ist unbegrenzt, aber das Holen mit Eimer oder Fass kostet sichtbar Zeit.

**Holz, Stein und Bau**

| Ware | Quelle | Wofür | Ab |
|---|---|---|---|
| Rundholz | Waldarbeiter | wird von jedem Haushalt zu Brennholz gespalten | Start |
| Brennholz | jeder Haushalt spaltet Rundholz | Kamine, Herde, Backofen, Räuchern, Kalkofen | Start |
| Reisig | alle nebenbei, Waldarbeiter | zählt wie Brennholz (2 Reisig = 1 Brennholz), Anzünden | Start |
| Kienspan | Waldarbeiter (Kiefernholz) | Notlicht (Kapitel 9), Anzünden | Start |
| Stamm (Bauholz) | Waldarbeiter | Bretter, Balken, Dachstuhl, Stützbalken der Grube, Leitern | Start |
| Hartholz (Eiche) | Waldarbeiter, langsamer (halbe Leistung) | Bremsklötze, Schwellen, Lore, Pflug, Werkbank | Start |
| Bretter | Jakob (1 Stamm → 6 Bretter) | Möbel, Dielen, Türen, Fensterläden, Fässer | Start |
| Holznägel | Jakob, aus Hartholz-Resten | Dachstuhl und Fachwerk; sparen Eisen | Start |
| Rinde | Nebenprodukt beim Bauholz | Gerberlohe für Leder | Start |
| Holzkohle | Waldarbeiter, Meiler (2 Rundholz → 1 Holzkohle) | Esse der Schmiede, Rennofen | Meiler gebaut |
| Teer, Pech | Nebenprodukt des Meilers | Fackeln, Boot und Schwellen abdichten, Seile teeren | Meiler gebaut |
| Bruchstein | Wilhelm, Steinbruch | Fundamente, Kamine, Brunnen, Pflaster, Steinhaus | Start |
| Kalkstein, Kalk | Wilhelm, Kalkofen (Kalkstein + Brennholz) | Mörtel, Kalkanstrich, Steinhaus | Ausbau |
| Lehm | alle, Lehmgrube am Bach | Fachwerk, Backofen, Kamine verputzen, Ziegel | Start |
| Sand | alle, am Bach und am See | Mörtel, Lehmputz, Pflaster | Start |
| Schilf (Reet) | Hannes, im Winter geschnitten | Dächer der ersten Häuser | Start |
| Ziegel | Ziegelei (Lehm + Brennholz) | Ziegeldach, Ziegelhaus, Badehaus-Ofen | Ausbau, Zuzug Ziegler |

**Nahrung**

| Ware | Quelle | Wofür | Ab |
|---|---|---|---|
| Weizen | Bauer | Mehl; beim Dreschen fallen Stroh und Saatgut an | Ernte Ende Sommer 1; vorher Restvorrat |
| Gerste | Bauer | Malz für Bier, Graupen für Eintopf | Ausbau |
| Mehl, Kleie | Greta (Handmühle), später Wassermühle | Brot; Kleie für die Hühner | Start (Restvorrat) |
| Brot (Laib) | Greta | Frühstück, Proviant | Start |
| Kartoffeln | Bäuerin | Eintopf, Bratkartoffeln | Ernte Herbst 1; vorher Restvorrat |
| Gemüse (Kohl, Rüben, Bohnen) | Bäuerin, Hausgärten | Eintopf | Sommer 1 |
| Waldkost (Pilze, Beeren, Wildgemüse) | Martha, alle nebenbei | Mahlzeiten, Laune | Start, nach Jahreszeit |
| Kräuter | Martha | Tee, Hustenmittel, Würze (Laune) | Start |
| Labkraut | Martha | Käse | Start |
| Hopfen | Martha (wild am Waldrand) | Bier | Sommer |
| Eier | Hühner der Bäuerin (6 Hühner im Korb mitgebracht) | Mahlzeiten, Backen | Start (wenige) |
| Milch | Taylor (8 Schafe, 4 Ziegen) | Frühstück, Käse; ein Schälchen für die Katze | Start |
| Käse | Taylor (Milch + Labkraut, für Hartkäse Salz) | Mahlzeiten, Proviant, Tausch | Käsekammer gebaut |
| Fleisch | Anton (Fallen, Flinte) | Mahlzeiten; geräuchert für den Winter | Start |
| Fisch | Hannes | Mahlzeiten; geräuchert für den Winter | Start |
| Honig | Emil (fängt im ersten Sommer einen Schwarm) | Süßes (Laune), Tausch | Sommer 1 |
| Bier | Gustav (Gerste, Hopfen, Brennholz, Fässer) | Abende in der Taverne (Laune) | Ausbau |

**Tiere, Fasern und Stoffe**

| Ware | Quelle | Wofür | Ab |
|---|---|---|---|
| Heu | Taylor und Otto mähen die Wiesen (2 Mahden à 80 Bund) | Winterfutter für die Tiere | Sommer 1 |
| Stroh | Bauer, beim Dreschen | Strohsack, Streu, Lehmputz | Restvorrat, dann ab Ernte |
| Laubstreu | alle, im Herbst | Streu im Stall (Ersatz für Stroh) | Herbst 1 |
| Mist | Taylors Stall, Hühner | Dünger (+20 % Ernte) | Start |
| Asche | Nebenprodukt jedes Feuers (10 Brennholz → 1 Asche) | Seife, Dünger, Lauge | Start |
| Wolle | Taylor, Schur im späten Frühling | Garn, Polster | Start |
| Garn | Lene; Taylor spinnt beim Hüten mit der Handspindel | Stoff, Dochte, Netze | Start |
| Stoff | Lene (Webstuhl) | Kleidung, Decken, Strohsack-Hülle, Vorhänge, Polster | Start |
| Kleidung, Decke | Lene näht aus Stoff | Kleidung nutzt sich ab, Decken in jedem Bett | Restvorrat aus dem Gepäck, dann Lene |
| Hanf, Leinen, Seil | Bauer (Hanf), Lene (Seile drehen, Leinen weben) | Grubenseil, Brunnen, Lok packen, Säcke, Fensterleinen | Ausbau; anfangs Seile aus dem Restvorrat |
| Felle, Leder | Anton (gerbt mit Rinde) | Dichtungen (Pumpe, Lok), Riemen, Schuhe | Start |
| Federn | Hühner | Federbett (Bonus) | Start |

**Licht, Wärme und Sauberkeit**

| Ware | Quelle | Wofür | Ab |
|---|---|---|---|
| Talg | Anton (Wild) | Kerzen, Grubenlampe, Stopfbuchsen der Lok | Start |
| Wachs | Emil (Bienen) | gute Kerzen, Holzpflege | Sommer 1 |
| Kerzen | Martha (1 Talg oder 1 Wachs + Docht → 4 Kerzen) | Licht am Abend | Start |
| Raps, Rüböl | Bäuerin (Raps), Ölpresse | Öllampen, Grubenlampe, Lok ölen, Seife, Fensterleinen tränken | Ernte Sommer 1; vorher Restvorrat |
| Seife | Martha (1 Krug Rüböl oder 2 Talg, dazu 2 Asche → 5 Seife) | Waschen (Hygiene) | Start |
| Fackeln | alle (Holz + Pech) | Licht am Platz und vor der Taverne | Meiler gebaut |

**Metall und Grube**

| Ware | Quelle | Wofür | Ab |
|---|---|---|---|
| Eisen | anfangs **knapp**: etwa 40 Alteisen aus dem Güterwagen, den der Felssturz am Tunnel zerdrückt hat; dann die Händlerin; später Konrad aus Erz (Rennofen) | Werkzeuge, Nägel, Beschläge, Lok | Start |
| Stahl | Händlerin; später Konrad selbst (Meister, mit Kohle) | Stahlwerkzeug, Grubenwerkzeug, Siederohre | Pass offen, später Grube |
| Nägel, Beschläge | Konrad (1 Eisen → 20 Nägel) | jeder Bau, Möbel, Schienennägel | Start (200 Nägel im Restvorrat) |
| Werkzeuge | Konrad, Stiel von Jakob | jeder Beruf (Kapitel 10) | Start |
| Erz | Grube | Eisen | Grube |
| Kohle | Grube | Lok, Schmiede, Kamine (1 Kohle = 3 Brennholz) | Grube, tiefer Abschnitt |
| Blei | Grube | Weißmetall für die Radlager, Kugeln für die Flinte | Grube |
| Ton | Grube | Töpferware und Kachelofen (Zuzug Töpfer) | Grube |
| Fundstücke | Grube, Wald | Rätsel der Grabstelle, Erinnerungen | Grube |

**Nur von der Händlerin** (Kapitel 11): Salz, Glas, Messing, Zinn, Papier und Tinte, Essig, Pulver, bunte Farbe,
Kaffee, Gewürze, neues Saatgut. Keine davon ist lebenswichtig; Kapitel 9 nennt für jede die Notlösung.

## 5. Kausalketten [Vorschlag]

Jede Kette ist im Bild zu sehen: Menschen gehen, tragen, arbeiten, und die Lager wachsen und schrumpfen.

- **Wärme:** Wald → Waldarbeiter fällt → Rundholz am Holzplatz → Haushalt holt es mit Korb oder Schubkarre → spaltet
  am Hackklotz → Holzlege → Kamin und Herd → Wärme, Licht, Asche.
- **Brot:** Saatgut aus der Kiste der Auswanderer → Aussaat im Frühling → Ernte Ende Sommer (Erntehilfe als
  Gemeinschaftsaufgabe) → Dreschen: Weizen, Stroh, Saatgut fürs nächste Jahr → Greta holt abends Weizen → Mühle: Mehl
  und Kleie (für die Hühner) → Backen am Morgen (Backofen braucht Brennholz) → **jeder Laib einzeln in der Auslage** →
  die Haushalte holen ihn → Frühstück; altbackene Laibe → Brotsuppe oder Hühner.
- **Mittag- und Abendessen** (zu Hause, sonst Taverne, sonst Suppenkessel): Kartoffeln und Gemüse aus dem Erdkeller +
  Fleisch oder Fisch aus der Räucherkammer +
  Kräuter → Küche (Herd braucht Holz) → Mahlzeit; anfangs kocht Gustav für alle.
- **Milch und Käse:** Wiese → Heu → Taylors Tiere → Milch → Käse (Labkraut von Martha, für Hartkäse Salz) → Keller.
- **Licht:** Kaminfeuer; Kienspan; Talg (Anton) oder Wachs (Emil) + Docht (Lene) → Kerzen (Martha); Raps → Ölpresse →
  Rüböl → Lampe und Grubenlampe.
- **Sauberkeit:** Rüböl oder Talg + Asche → Seife (Martha) → Waschen; später das Badehaus (Holz, Wasser).
- **Stoff:** Schur → Wolle → Garn (Taylor beim Hüten, Lene) → Stoff (Lene) → Kleidung, Decken, Polster, Vorhänge; Hanf
  → Seil und Leinen.
- **Leder:** Anton jagt → Fell → gerbt mit Rinde vom Waldarbeiter → Leder → Dichtungen, Riemen, Schuhe.
- **Werkzeug:** Alteisen, Händlerin oder Erz → Eisen; Meiler → Holzkohle → Esse; Stiel von Jakob → Werkzeug → stumpf →
  Schärfen → nach dem dritten Mal verbraucht → neu.
- **Haus:** Stämme → Bretter (Jakob); Bruchstein (Wilhelm); Lehm und Sand (alle); Schilf (Hannes); Nägel (Konrad) →
  Bautrupp → Notunterkunft → Räume (Möbel von Jakob, Polster von Lene) → Ausbaustufen (Kalk, Stein, Glas).
- **Grube und Lok:** Grubenlampe (Konrad) + Rüböl oder Talg; Spitzhacke, später Stahl-Grubenwerkzeug; Stützbalken;
  Seil → Erz, Kohle, Blei → Eisen (Rennofen), Weißmetall (Blei + Zinn von der Händlerin) → Lok.
- **Tausch:** Überschuss über der Obergrenze → Tauschkiste im Güterschuppen → Händlerin → Salz, Glas, Stahl …

## 6. Lager, Wege und sichtbare Bestände

**Lagerplätze [Vorschlag]:**

- **Erzeugerlager** am Arbeitsplatz: Holzplatz an der Holzfällerhütte, Scheune, Erdkeller, Räucherkammer, Auslage der
  Bäckerei. Jedes hat eine Obergrenze.
- **Hausvorrat:** Holzlege (reicht etwa 2 Tage), Speisekammer, Brotkorb, Kerzenkiste.
- **Gemeinschaftslager** im Güterschuppen, später in der Vorratsscheune: Restvorrat, Wintervorrat, Tauschkiste,
  Ware von außen.

**Holen und Bringen [Vorschlag]:** Erzeuger bringen in ihr eigenes Lager. Fällt ein Hausvorrat unter etwa einen Tag,
holt der Bewohner selbst Nachschub (Korb: 10 Einheiten; Schubkarre von Jakob: 30; später Handkarren: 60). Die Wege
sind sichtbar und kosten auch in der groben Simulation echte Zeit.

**Weboberfläche „Lager“ [entschieden, vom User ausdrücklich gelobt]:** jeder Lagerplatz mit Inhalt und Füllstand, Zu-
und Abgänge des Tages und **Reichweite** („Brennholz reicht noch 3 Tage“); dazu eine Übersicht pro Ware über das ganze
Dorf. **Jede Bewegung steht in der Chronik** („08:40 Greta holt einen Sack Weizen an der Scheune“).

**Sichtbare Bestände [entschieden, Wunsch des Users 2026-10-21]:** Was im Lager liegt, sieht man im Bild, Stück für
Stück. **Jeder Brotlaib liegt einzeln in der Auslage der Bäckerei:** Greta legt die frischen Laibe nacheinander
hinein; holt jemand einen, fehlt genau dieser eine. Dasselbe gilt für Holzscheite im Stapel, Mehlsäcke, Käselaibe im
Keller, Fische an der Räucherstange, Honiggläser im Regal, Werkzeuge an der Wand und Kerzen in der Kiste.

- [Vorschlag, Technik] Jeder Lagerplatz bekommt im Innenraum-Baukasten eine Liste von **Stellplätzen** (Position, Bild,
  Reihenfolge). Die Menge im Warenbuch füllt die Stellplätze der Reihe nach. Passt mehr hinein als Plätze da sind,
  liegt der Rest „hinten“ (Korb unter der Theke) und steht als Zahl auf der Info-Karte und im Web.
- Altbackene Laibe wandern in den Korb „vom Vortag“. Ein Holzstapel zeigt eine Reihe Scheite pro 10 Brennholz.
- Kosten: Ein Laib ist ein kleines Bild; 20–40 Stück kosten beim Zeichnen fast nichts (gemessen: 48 Sprites zusammen
  4,1 ms, siehe SETTING.md Kapitel 16).

## 7. Verbrauch [Vorschlag, Startwerte]

**Feuer** (Brennholz pro Tag):

| Feuerstelle | Frühling | Sommer | Herbst | Winter |
|---|---|---|---|---|
| Kamin je Wohnhaus (abends ~6 h, im Winter auch morgens) | 5 | 1 | 6 | 9 |
| Herd je Küche | 2 | 2 | 2 | 2 |
| Backofen der Bäckerei | 6 | 6 | 6 | 6 |
| Taverne (Herd und Kamin) | 8 | 5 | 8 | 10 |
| Feuer am Dorfplatz (abends) | 3 | 3 | 3 | – |
| Räuchern (Anton und Hannes) | 2 | 2 | 4 | 1 |
| Schmiede (4 Holzkohle aus 8 Rundholz) | 8 | 8 | 8 | 8 |

Ein Steinhaus braucht ein Drittel weniger für den Kamin, ein Kachelofen noch einmal weniger. Kalkofen, Ziegelei und
Brauerei verbrauchen je Brand bzw. Sud einmalig 20–40 Brennholz.

**Essen:** drei Mahlzeiten am Tag. Das **Wunschmenü** pro Person und Tag: ½ Laib Brot, 2 Portionen Kartoffeln oder
Gemüse, 1 Portion Fleisch, Fisch, Eier oder Käse, ½ Krug Milch.

- **Wo gegessen wird [entschieden, Wunsch des Users]:**
  1. **Zu Hause**, wer eine Küche, Zutaten und Zeit hat.
  2. **In der Taverne:** Gustav kocht **tagsüber** ein Tagesgericht aus Zutaten, die er selbst holt; abends wird es
     ausgegeben, an alle ohne eigenes Essen und an alle, die zum Zusammensein kommen. Es gibt so viele Portionen, wie er
     gekocht hat; Reste gibt es am nächsten Mittag. Anfangs, solange kaum ein Haus eine Küche hat, kocht er im
     Speisewagen alle Mahlzeiten für alle.
  3. **Am Suppenkessel im Speisewagen** gibt es die Standardsuppe (Kapitel 9), wenn es sonst nichts gibt.
- **Ersatzregel:** Fehlt eine Zutat, nimmt der Koch eine andere essbare Ware (Fisch statt Fleisch, Kartoffeln statt
  Brot, Waldkost statt Gemüse). Erst wenn im Hausvorrat, beim Erzeuger in Reichweite und in der Taverne nichts
  Essbares mehr ist, gibt es die Standardsuppe.
- **Abwechslung macht froh:** Drei Tage dasselbe senkt die Laune leicht; Honig, Käse und Kaffee heben sie.

| Weiterer Verbrauch | Menge |
|---|---|
| Licht | 1 Lichtabend pro bewohntem Haus (Winter 2): Kerze oder Lampenfüllung, sonst Kienspan |
| Rüböl | 1 Krug = 10 Lampenabende |
| Seife | 1 Stück pro Person alle 14 Tage |
| Bett | Strohsack: 1 Bund Stroh alle 14 Tage frisch stopfen; Decke hält etwa 8 Weltwochen |
| Kleidung, Schuhe | etwa alle 4 Weltwochen 1 Stoff bzw. 1 Leder pro Person |
| Tiere | im Winter 1 Bund Heu pro Schaf oder Ziege und Tag; Streu 1 Bund pro Tag im Stall; Hühner fressen Kleie und Küchenreste |
| Werkzeug | Verschleiß nach Arbeitsstunden (Kapitel 10) |

**Baumaterial:**

| Bauabschnitt | Material |
|---|---|
| Notunterkunft (ein Raum) | 8 Stämme, 18 Bretter, 8 Lehm, 4 Sand, 6 Schilf, 10 Bruchstein (Fundament, Feuerstelle), 15 Nägel, 30 Holznägel |
| Raum anbauen (Rohbau) | 6 Stämme, 18 Bretter, 8 Lehm, 4 Sand, 4 Schilf, 4 Bruchstein, 10 Nägel, 20 Holznägel |
| Möbel | Hocker 2 Bretter · Bank 4 Bretter · Tisch 6 Bretter, 8 Nägel · Truhe 6 Bretter, 2 Beschläge · Bett 8 Bretter, 6 Nägel + Strohsack (1 Stoff, 4 Stroh) · Schrank 10 Bretter, 2 Beschläge · Sofa 6 Bretter, 2 Stoff, 4 Wolle |
| Fenster | Standard: Laden aus 2 Brettern; Stufe 2: geöltes Leinen (1 Leinen, 1 Rüböl); Stufe 3 (Bonus): 2 Glas |
| Steinhaus, je Raum | 30 Bruchstein, 6 Kalk, 10 Sand, 4 Bretter |
| Federbett (Bonus) | 1 Stoff, 10 Federn |
| Kleine Reparatur | 1–3 Bretter oder 2 Schilf, 5 Nägel |

**Verderb [Vorschlag]:** einfach als Anteil pro Tag, im Keller halb so schnell: Milch 50 % (deshalb Käse), frischer
Fisch und frisches Fleisch 30 % (deshalb Räuchern), Eier 5 %, Gemüse und Kartoffeln im Keller 1 %. Brot ist 3 Tage
frisch, dann bis zum 6. Tag altbacken (Brotsuppe), danach Hühnerfutter. Korn, Mehl, Käse, Geräuchertes, Holz und alles
Übrige verdirbt nicht. Was verdirbt, wird als Abgang „verdorben“ gebucht.

## 8. Herstellung, Arbeitsrhythmus, Jahreslauf und Natur [entschieden 2026-10-21: Startwerte, Prüfung im Zeitraffer]

Leistung pro Arbeitstag (etwa 8 Stunden) bei Fähigkeit „geübt“ und gutem Werkzeug:

| Beruf | Leistung |
|---|---|
| Veit (Waldarbeiter) | 3 Bäume → 120 Rundholz **oder** 3 Bäume → 12 Stämme (Hartholz: 6); Meiler anlegen ½ Tag → nach 2 Tagen 30 Holzkohle und 3 Pech |
| alle nebenbei | 2–3 Reisig pro Person und Tag (im Winter kaum) |
| Haushalt, Holz spalten | 15 Brennholz pro Stunde (ein Haus braucht im Winter etwa 45 Minuten am Tag) |
| Bauer und Bäuerin | je Acker etwa 7 Arbeitstage pro Weltjahr; das Paar schafft 6 Äcker, mit Erntehilfe 7–8; Ertrag je Acker: 15 Sack Weizen + 25 Stroh, 400 Portionen Kartoffeln, 250 Portionen Gemüse, 50 Krüge Rüböl; Wiese: 80 Bund Heu pro Mahd; Mist und Asche als Dünger +20 %; von jeder Ernte bleibt Saatgut zurück (etwa ein Zehntel) |
| Greta | Ofenladung 20 Laibe in etwa 3 Stunden; Handmühle 1 Sack in 3 Stunden; mit der Wassermühle mahlt sie nicht mehr selbst |
| Anton | im Schnitt 12 Portionen Fleisch (Fallen und Flinte; nur Fallen: 7), 2 Talg pro Tag, alle 5 Tage ein Fell |
| Hannes | 10–18 Portionen Fisch (Winter 5–8); im Winter 15 Bund Schilf pro Tag |
| Taylor | 10 Krüge Milch (Winter 5); Schur: 12 Wolle pro Schaf; nebenbei 2 Garn beim Hüten |
| Lene | 4 Garn **oder** 2 Stoff **oder** 3 Stück nähen pro Tag; zusammen mit Taylor etwa 1,5 Stoff pro Tag |
| Martha | 20 Kerzen oder 15 Seife pro Tag, dazu Kräuter und Waldkost |
| Emil | Bienen 1–2 Stunden am Tag; Honigernte Ende Sommer: 3 Völker → 30 Honig, 10 Wachs |
| Konrad | 2 neue Werkzeuge **oder** 40 Nägel **oder** 12 Werkzeuge schärfen |
| Jakob | 1 Stamm → 6 Bretter in einer Stunde; Möbel siehe SETTING.md Kapitel 8 „Dauer“ |
| Wilhelm | 8 Bruchstein (er ist 66; ein Steinmetz schafft später 12) |

**Faktoren:** Fähigkeit (Anfänger 0,6 · geübt 1,0 · Meister 1,3) × Werkzeug (keins 0,4 · stumpf 0,7 · gut 1,0 ·
Stahl 1,3) × Wetter (Regen 0,8 und Schnee 0,6 für Feld und Bau; Holzfällen ist Winterarbeit und wird nicht gebremst) ×
Kraft und Laune (0,8–1,1). Helfer ohne den Beruf arbeiten als Anfänger.

### Arbeitsrhythmus: wer wann was macht [entschieden]

**Braucht es das, oder nimmt es Leben aus der Welt?** Wir brauchen es, aber als **Rhythmus, nicht als Schleife**.
Eine Schleife ist eine feste Abfolge, die abläuft, egal was los ist; die wäre leblos. Ein Rhythmus ist eine Gewohnheit,
wie sie echte Handwerker hatten: Der Bäcker backt vor dem Frühstück, der Fischer fährt im Morgengrauen raus, der Jäger
sitzt in der Abenddämmerung an. Gerade das macht die Welt glaubwürdig: Morgens riecht es nach Brot. Lebendig wird sie
durch die **Abweichungen mit Grund**: Greta backt eine zweite Ladung, weil die Auslage mittags leer ist; Veit spaltet
statt zu fällen, weil es stürmt; Hannes bleibt länger draußen, weil die Fische beißen; ein Tipp oder ein Ereignis
unterbricht. Das ist genau „Systeme statt Schleifen“ aus LEBENDIGKEIT.md. Fürs Balancing ist der Rhythmus nötig, weil
er bestimmt, **wann** eine Ware da ist (Brot zum Frühstück, nicht abends).

**So funktioniert es:**

- Jeder Beruf hat **Arbeitsschritte als Daten**: Schritt, Ort, Werkzeug, Menge pro Stunde, Blocklänge (etwa 1–2
  Stunden), **bevorzugtes Zeitfenster**, Bedingungen (Wetter, Material da, Lagerplatz frei) und einen Ersatzschritt.
- Für jeden Block wählt die Figur den Schritt mit der höchsten Bewertung: Passung zum Zeitfenster × Dringlichkeit
  (Reichweite der Ware, Kapitel 13) × Bedingungen × Charakter (Frühaufsteher, gründlich, gesellig) × Laune × kleine
  Streuung, dazu die Wiederholungsbremse. Das Zeitfenster ist eine Vorliebe, kein Zwang.
- Die Tagesleistung aus der Tabelle oben ist die Summe der Blöcke. Das Protokoll (Kapitel 15) schreibt jeden Block
  mit; so sieht man, ob ein Beruf zur falschen Zeit oder zu wenig arbeitet.

**Rhythmus der Berufe** (Morgendämmerung je nach Jahreszeit etwa 5–7 Uhr; mittags eine Pause):

| Wer | Morgens | Vormittag | Nachmittag | Abend | Bei Regen, im Winter |
|---|---|---|---|---|---|
| Greta (Bäckerin) | Ofen anheizen, backen; gegen 7 Uhr liegen die frischen Laibe in der Auslage | mahlen; zweite Ladung nur, wenn die Auslage leer wird | Weizen für morgen holen, Holz spalten | Sauerteig für morgen ansetzen (kurz), dann frei | gleich (drinnen) |
| Veit (Waldarbeiter) | Bäume fällen | fällen | aufarbeiten: entasten, ablängen, Rundholz stapeln, Stämme entrinden | brennt ein Meiler, kurz nachsehen | Sturm: Holzplatz ordnen, Werkzeug, Setzlinge; Winter: Hauptzeit fürs Fällen |
| jeder Haushalt | Wasser und Brot holen | Beruf | auf dem Heimweg Rundholz mitnehmen | Holz spalten (30–45 min), Kamin anzünden, kochen | – |
| Taylor (Schäferin) | melken, Herde austreiben | hüten, nebenbei spinnen; jeden zweiten Tag Käse machen | hüten | eintreiben, melken, Tiere zählen | Gewitter: bei den Tieren im Stall; Winter: Stall und Heu |
| Otto und Ida (Bauernhof) | Hühner füttern, Eier sammeln | Feldarbeit nach Jahreszeit (in den kühlen Stunden) | Hof: dreschen, Ölpresse, Garten | Hühner einsperren | Regen: in der Scheune dreschen, Geräte richten; Erntetage ganztags mit Erntehilfe |
| Gustav (Wirt) | steht später auf | Zutaten holen, Tagesgericht ansetzen | Mittagessen ausgeben (anfangs für alle), Abendessen kochen | **Taverne: sein Hauptdienst** | – |
| Anton (Jäger) | Fallen in der Morgendämmerung kontrollieren | zerlegen, räuchern, gerben | Fallen stellen, Rinde holen | Ansitz in der Abenddämmerung, wenn das Wild unterwegs ist | Regen: gerben, Fallen richten |
| Hannes (Fischer) | Reusen leeren, angeln im Morgengrauen | Fang ausnehmen, räuchern | Netze flicken | – | Winter: Eisfischen, Schilf schneiden |
| Martha (Kräuter) | – | Kräuter sammeln, sobald der Tau getrocknet ist | trocknen, Kerzen ziehen, Seife sieden | Tee für Kranke | Regen: Kerzen, Seife, Tee |
| Konrad (Schmied) | Esse anheizen | schmieden (große Stücke) | schärfen und reparieren: Werkzeug wird morgens gebracht und nachmittags abgeholt | – | gleich |
| Jakob (Tischler) | Bretter sägen | Möbel bauen | Möbel bauen, am späten Nachmittag ausliefern | – | gleich |
| Wilhelm (Steinbruch) | – | Steine brechen | nach der Mittagsruhe behauen und sortieren | – | Regen: Werkzeug, alte Grubenpläne |
| Lene (Weberin) | – | am Webstuhl beim Fenster (Tageslicht) | weben, nähen | spinnen in der **Spinnstube** (gesellig) | gleich |
| Paul (Kustos) | Tiere im Museum füttern | Bautrupp | schnitzen (Schildchen, Figuren) | – | Regen: schnitzen |
| Emil | Bienen | Lok | Lok, danach Bienen | Rat geben, Taverne | Regen: Lok im Schuppen |
| Bautrupp (Sykora, Paul, Helfer) | – | bauen | bauen; Helfer kommen, wenn ihre eigenen Lager voll sind | – | Regen: innen statt Dach; Schnee bremst |

**Ausnahmen vom Tagesgerüst** (sonst gilt: nachts arbeitet niemand, abends ist frei): Greta in der Morgendämmerung,
Gustav abends in der Taverne, Antons Ansitz in der Abenddämmerung, ein kurzer Blick auf den Meiler. **Leise
Handarbeit am Abend** (Spinnstube, Schnitzen, Flicken) ist Freizeit mit kleinem Ertrag, wie damals üblich.

**Jahreslauf** (eine Jahreszeit = 7 Tage):

| Jahreszeit | Feld | Tiere | Wald | See |
|---|---|---|---|---|
| Frühling | Aussaat (Weizen, Kartoffeln, Raps, Gemüse) | Lämmer, Schur | Bauholz, Setzlinge pflanzen | Fischen |
| Sommer | erste Mahd, Hacken; am Ende Ernte Weizen und Raps (Gemeinschaftsaufgabe) | Weide, viel Milch, Honig | Meiler | viel Fisch |
| Herbst | Kartoffeln und Gemüse einkellern, zweite Mahd, Dreschen, Ölpressen | Räuchern für den Winter | Holz machen (Wintervorrat) | Fischen |
| Winter | Dreschen, Geräte reparieren | Stall, Heu, wenig Milch | Holzfällen (Hauptzeit) | Eisfischen, Schilf schneiden |

**Natur-Bestände [Vorschlag]:** Wald (Bäume), Wild, Fische, Wiesen, Schilf und Waldkost haben einen Bestand und einen
Nachwuchs pro Weltjahr. Erzeuger nehmen nur, was nachwächst; der Waldarbeiter pflanzt für jeden gefällten Baum einen
Setzling, der nach zwei Weltjahren nachwächst. Das passt in die Zeit: Der Gedanke der Nachhaltigkeit stammt aus der
Forstwirtschaft des 18. Jahrhunderts. Sichtbar wird es als Ortszustand: Der Wald wird lichter, es gibt Baumstümpfe und
junge Bäume. Kohle aus der Grube und Steinhäuser entlasten den Wald.

## 9. Notlösungen statt Mangel [entschieden: Standardsuppe; Rest Vorschlag]

**Regel:** Jede lebenswichtige Kette hat eine Notlösung, die nur Zeit kostet und nie die Händlerin braucht.
Notlösungen kosten Laune oder Kraft und werden im Protokoll gezählt, denn sie zeigen, wo die Balance hakt.

| Fehlt | Notlösung | Folge |
|---|---|---|
| Essen | **Standardsuppe** | sättigt halb, Laune und Kraft sinken (unten) |
| Brennholz | Reisig sammeln, zusammenrücken (Nachbarn, Taverne) | kühler Abend, Laune leicht weniger |
| Licht | Kienspan | rußig, Laune leicht weniger |
| Stroh als Streu | Laubstreu | keine |
| Salz | Räuchern statt Pökeln, Frischkäse statt Hartkäse | Vorräte halten kürzer |
| Seife | Waschen mit Aschenlauge vom Herd | Sauberkeit etwas schlechter |
| Werkzeug | Handarbeit | 40 % Leistung |
| Heu | Winterweide, Laub | weniger Milch |
| Pulver | nur Fallen | weniger Fleisch |
| Glas | Laden oder geöltes Leinen | dunkler Raum |

**Standardsuppe [entschieden, Wunsch des Users; Details Vorschlag]:**

- Gibt es für eine Mahlzeit gar nichts Essbares (auch nicht nach der Ersatzregel und nicht in der Taverne), gibt es
  die Standardsuppe, im Dorf „Wassersuppe“: Wasser, Brennnesseln und Wildkräuter vom Wegrand. **Sie braucht keine
  Ware aus dem Warenbuch** und ist deshalb immer verfügbar. **Niemand hungert.**
- **Wo [entschieden, Wunsch des Users; Ort von Claude gewählt]:** am **Suppenkessel im Speisewagen**. Es ist der
  Kessel, der im langen Winter alle satt gehalten hat. Wer Hunger hat und zu Hause nichts kochen kann (keine Zutaten
  oder keine Zeit, z. B. an Erntetagen), geht mit einem Henkeltopf hin, schöpft und isst zu Hause. Wenn die Taverne
  später an den Dorfplatz zieht, bleibt der Speisewagen als Suppenküche stehen.
- Der Kessel hängt über dem Herd im Speisewagen, der ohnehin brennt; später heizt man ihn mit Reisig, und wer Suppe
  holt, bringt einen Arm voll mit. So bleibt die Suppe unabhängig vom Warenbuch.
- **Folgen:** sättigt nur halb (der Hunger kommt früher), Kraft und Laune sinken, die Arbeit geht etwas langsamer.
  Einmal stört sie kaum, ab dem dritten Mal hintereinander deutlich. Dann erscheint der Gedanke „Schon wieder
  Wassersuppe …“ und am Schwarzen Brett ein Zettel („Wir brauchen Brot!“), der eine Gemeinschaftsaufgabe anstößt.
- In der Weboberfläche zeigt der **Anteil Standardsuppe** auf einen Blick, wie gut das Dorf versorgt ist.

## 10. Werkzeuge [entschieden, Wunsch des Users; Haltbarkeit Vorschlag]

Jeder Beruf braucht Werkzeug vom Schmied (Eisen, Stiel von Jakob) oder vom Tischler (Holzwerkzeug).

| Wer | Werkzeug |
|---|---|
| Waldarbeiter | Axt, Säge, Spaltkeil |
| jeder Haushalt | Spaltaxt am Hackklotz |
| Bauer, Bäuerin | Hacke, Sense, Dreschflegel (Holz), Ölpresse; später Pflug mit Zugtier |
| Bergmann, Steinbruch | Spitzhacke, Schlägel und Eisen, Brechstange |
| Tischler | Säge, Hobel, Stemmeisen |
| Bautrupp | Hammer, Maurerkelle; Nägel als Verbrauch |
| Schmied | Hammer und Zange (repariert er selbst) |
| Jäger | Messer, Fallen, Flinte (Pulver von der Händlerin, später Blei aus der Grube) |
| Fischer | Haken (Schmied), Netz (Lene) |
| Schäferin | Schere, Melkeimer (Jakob) |
| Bäckerin | Brotschieber, Teigtrog (Jakob), Handmühle |
| Weberin | Spinnrad, Webstuhl (Jakob) |

- **Abnutzung nach Arbeitsstunden:** Grundwerkzeug aus Eisen wird nach etwa 100 Arbeitsstunden **stumpf** (bei
  Vollzeit etwa 14 Arbeitstage; die Spaltaxt eines Haushalts hält viel länger). Konrad schärft es kurz und ohne
  Material. Nach dem dritten Schärfen ist es verbraucht und muss neu geschmiedet werden. Holzwerkzeug von Jakob nutzt
  sich halb so schnell ab.
- **Stufen:** Grundwerkzeug (Eisen) → **Stahlwerkzeug** (Leistung 130 %, hält doppelt so lange) → **Grubenwerkzeug**
  (Stahl, nötig ab dem zweiten Abschnitt der Grube; die erste Spitzhacke reicht nur für den Eingang und den ersten
  Stollen).
- **Zum Start:** Eine Grundausstattung (einfache Äxte, Sensen, Hacken, teils stumpf) stammt aus der Fracht im
  Güterschuppen, die für die Bergdörfer bestimmt war. Konrad arbeitet anfangs an der kleinen Esse im Lokschuppen.
- Werkzeuge sind Einzelstücke mit Besitzer und Zustand; die Info-Karte zeigt z. B. „Axt stumpf, morgen zum Schmied“.

## 11. Händlerin und Tausch [Vorschlag]

- **Wann:** Der Bergpass ist vom späten Frühling bis zum frühen Herbst offen (etwa 2 der 4 Weltwochen). In dieser Zeit
  kommt sie **alle 3–4 Tage**, bleibt eine Nacht in der Taverne und zieht weiter: etwa **vier Besuche pro Weltjahr**.
  Bei Unwetter kommt sie einen Tag später.
- **Tausch ohne Geld, aber sicher:** Eine feste **Tauschtafel** (z. B. 1 Käse = 2 Salz, 10 Wolle = 1 Glas, 2 Felle = 1
  Stahl, 3 Honig = 1 Eisen). Was über der Obergrenze liegt, legen die Bewohner in die **Tauschkiste** im Güterschuppen.
  Was das Dorf von außen braucht, hängt als **Bestellung** am Schwarzen Brett. Sie tauscht **immer**, wenn eine
  Bestellung durch Ware in der Tauschkiste gedeckt ist; das ist kein Zufall. Im Web: Tauschkiste, Bestellungen,
  nächster Besuch.
- **Mengen pro Besuch** (Obergrenze, ihr Maultier trägt nicht mehr): bis 10 Eisen, 4 Stahl, 6 Glas, 10 Salz, 4
  Rüböl, 2 Messing, 2 Zinn, 4 Pulver, Papier und Tinte, etwas Kaffee und Gewürze, Farbe, Saatgut; dazu Post und
  Neuigkeiten.
- **Sie nimmt:** Käse, Wolle, Stoff, Felle, Leder, Honig, Wachs, Paul Brandts Schnitzfiguren, Kräuter.
- **Nur sie bringt**, jeweils mit Notlösung (Kapitel 9): Salz (Pökeln, Hartkäse), Glas (Fenster, Laternen,
  Gewächshaus, Aquarium und Terrarien im Museum), Messing (Lok-Armaturen, Lampen), Zinn (Weißmetall), Papier und Tinte (Sykoras Tagebuch, Briefe; am
  Brett ersatzweise Holztäfelchen), Essig (Kessel der Lok entkalken), Pulver (Flinte), bunte Farbe (selbst gibt es
  Kalkweiß, Ocker aus Lehm und Pflanzenfarben von Martha), Kaffee und Gewürze (Laune).
- **Antwort auf die Frage nach dem Öl:** Nichts Lebenswichtiges hängt allein an ihr. **Das Grubenlicht kommt aus dem
  Dorf:** Grubenlampen brennen Rüböl oder Talg, wie die Unschlittlampen der Bergleute damals. Ihr Öl ist eine
  Ergänzung, vor allem vor der ersten Rapsernte. Was nur sie bringt, muss vor dem Winter eingetauscht sein; das ist
  eine schöne Aufgabe für den Herbst.

## 12. Bilanz-Check und Kipppunkte [Vorschlag]

Geprüft für zwei Zeitpunkte: **Start** (erstes Weltjahr, 16 Menschen, im Mittel etwa 8 Feuerstellen) und
**Endausbau** (40 Menschen, 20 Häuser). Ziel ist überall **Herstellung = 1,2- bis 1,5-mal Bedarf**. Die Luft ist
Freizeit und Arbeitskraft für den Bau (SETTING.md Kapitel 8, „Dauer“). **Neu gerechnet für die halbierten Bauzeiten**
(2026-10-21): Essen und Heizen hängen an der Uhr und bleiben gleich; was pro Tag verbaut wird, verdoppelt sich und ist
durch kleinere Hütten, Holznägel und höhere Erträge ausgeglichen.

| Ware | Start: Bedarf → Herstellung | Endausbau: Bedarf → Herstellung | Was der Endausbau braucht |
|---|---|---|---|
| Brennholz pro Tag | ~75 (Winter ~125) → 120 Rundholz + ~30 Reisig (Winter ~15); im ersten Winter knapp, deshalb Herbstvorrat | ~180 (Winter ~260) → 2 × 120 + ~60 Reisig | zweiter Waldarbeiter, Herbstvorrat, Steinhäuser, Kohle |
| Bauholz pro Tag | ~4½ Stämme → 12 pro Waldarbeiter-Tag; Veit ist damit zu ~80 % ausgelastet | ~7 → Rest der zwei Waldarbeiter-Tage | im Winter bremst Schnee den Bau, dann geht die Kraft ins Brennholz |
| Brot pro Tag | 8 Laibe → 20 | 20 → 20 (mit Wassermühle 30) | Wassermühle |
| Weizen pro Weltjahr | 25 Sack (mit Saatgut) → 2 Äcker = 30 | 61 → 4 Äcker mit Dünger = 72 | Ochse und Pflug |
| Kartoffeln und Gemüse pro Tag | 32 → ~46 (2 Kartoffeläcker, 1 Gemüsefeld, 3 Gärten, Waldkost) | 80 → ~98 (3 + 1 Äcker, 20 Gärten) | Hausgärten an jedem Haus |
| Fleisch, Fisch, Eier, Käse pro Tag | 16 → 30 (Überschuss wird geräuchert oder getauscht) | 40 → ~47 | mehr Hühner, Käse, Räuchervorrat |
| Milch pro Tag | 8 → 10 | 20 → ~23 | Herde 14 Schafe und 6 Ziegen, eine Kuh |
| Wolle pro Weltjahr | 62 → 96 (8 Schafe à 12) | 157 → 168 (14 Schafe) + Leinen aus Hanf | Lämmer, bis 14 Schafe |
| Stoff pro Tag | 1,1 → 1,5 | 2,8 → 1,5 | Weberin oder Schneiderin |
| Licht pro Weltjahr | 245 Abende → 264 Kerzen (Talg, Wachs) + 500 Lampenabende (1 Raps-Acker) | 700 → 264 + 1000 (2 Raps-Äcker), abzüglich Öl für Seife | zweiter Raps-Acker |
| Heu pro Weltjahr | 120 Bund → 1 Wiese = 160 | ~260 → 2 Wiesen = 320 | zweite Wiese |
| Stroh pro Weltjahr | ~70 → 50 + Laubstreu | ~180 → 125 + Laubstreu | Laubstreu ist im Endausbau normal |
| Bruchstein pro Tag | ~3 → 8 | ~12 (Steinhäuser) → 8 + Steinmetz 12 | Steinmetz |
| Schilf pro Weltjahr | ~55 → 105 (Hannes im Winter) | ~85 → 105 | später Ziegeldächer |
| Eisen pro Weltjahr | ~21 (Holznägel sparen Nägel) → 40 Alteisen (einmalig) + bis 40 von der Händlerin | ~48 → 40 + Grube | Erz aus der Grube |
| Schmiede-Arbeit pro Tag | ~0,5 Konrad-Tage → 1 | ~1,1 → 1 | Stahlwerkzeug oder ein Lehrling |

**Kipppunkte:** Bis wohin reicht jeder Beruf der Startbesetzung? Das ist die Grundlage für die handgeschriebene
Zuzugsliste. Es ist **kein Automatismus**; die Idee „Zuzug nach Engpass“ hat der User nicht gewählt. Mit den halbierten
Bauzeiten werden die Kipppunkte früher erreicht (in Monaten), aber bei denselben Einwohnerzahlen.

| Beruf | reicht bis etwa | dann |
|---|---|---|
| 1 Waldarbeiter | 11 Häuser | zweiter Waldarbeiter oder Köhler |
| Greta mit Handmühle | 30 Menschen | Wassermühle am Bauernhof |
| Otto und Ida mit Erntehilfe | 25 Menschen | Ochse und Pflug (bis ~35), danach eine zweite Bauernfamilie |
| Lene und Taylor (Spinnen, Weben) | 20 Menschen | Weberin oder Schneiderin |
| Anton, Hannes, Hühner | 30 Menschen | mehr Hühner, Käse, Fischer |
| 1 Raps-Acker | 17 Häuser | zweiter Acker |
| Wilhelm | Bauen ohne Steinhäuser | Steinmetz |
| Konrad mit Eisenwerkzeug | 35 Menschen | Stahlwerkzeug oder Lehrling (z. B. Sykora) |
| Jakob | bestimmt das Bautempo (SETTING.md Kapitel 8) | zweiter Tischler macht den Ausbau schneller |

**Vorschlag für die Reihenfolge der Zuzügler:** zweiter Waldarbeiter bzw. Köhler, Weberin oder Schneiderin, Müller
(Wassermühle), zweite Bauernfamilie mit Ochse, Fischer, Steinmetz, Ziegler, zweiter Tischler; dazwischen Figuren
ohne Engpass-Beruf.

**Startvorrat** (Restvorrat im Güterschuppen aus der Fracht für die Bergdörfer, dazu das Gepäck): 10 Sack Mehl, 150
Portionen Kartoffeln und Rüben, 40 Portionen Rauchfleisch, 10 Salz, 6 Krüge Lampenöl, 6 Seile, 200 Nägel, 30 Stroh,
Decken und Kleidung aus dem Gepäck, Werkzeug-Grundausstattung, Gretas Handmühle, Saatgut aus der Kiste der Auswanderer (Weizen,
Kartoffeln, Raps, Gemüse, etwas Gerste und Hanf), 6 Hühner im Korb, Taylors 8 Schafe und 4 Ziegen, etwa 40 Alteisen
am Tunnel. **Folge:** Der erste Frühling ist knapp, aber machbar. Bis zur ersten Ernte gibt es etwa jede vierte
Mahlzeit Standardsuppe; das ist gewollt und erzählt, wie schwer der Anfang war.

## 13. Gleichgewicht: die Regeln [Vorschlag]

1. **Reichweite statt Menge:** Das Warenbuch rechnet für jede Ware aus, wie viele Tage der Vorrat beim jetzigen Bedarf
   noch reicht (gleitender Durchschnitt der letzten 24 Stunden, im Herbst mit Blick auf den Winter).
2. **Sollbestand und Obergrenze** pro Ware, in Tagen (Brennholz: Soll 4, Obergrenze 10; im Herbst Soll 10, Obergrenze
   20). Unter dem Soll macht der Erzeuger diese Ware zuerst; über der Obergrenze macht er etwas anderes (zweite Ware,
   Werkzeug pflegen, beim Bau helfen, Freizeit). Weil jeder Erzeuger immer an seiner knappsten Ware arbeitet, häuft sich
   nichts an und nichts bleibt leer.
3. **Notbestand** (unter einem Tag): Die Ware kommt als Gemeinschaftsaufgabe ans Schwarze Brett, wer frei hat, hilft
   (z. B. eine sichtbare „Holzaktion“).
4. **Kapazität mit Luft:** Startwerte so, dass jeder Beruf das 1,2- bis 1,5-Fache des Bedarfs schafft (Kapitel 12).
5. **Wachstum braucht Leute:** Jedes neue Haus erhöht den Bedarf; die Zuzugsliste folgt den Kipppunkten. Ausbauten
   senken den Bedarf (Steinhaus, Kachelofen, Kohle) oder heben die Leistung (Stahlwerkzeug, Wassermühle, Handkarren).
6. **Jahreszeiten:** Im Herbst steigen die Sollbestände für den Winter (Holz machen, Einkellern, Räuchern, Heu). Ein
   schlechter Herbst zeigt sich im Winter (mehr Standardsuppe, kühlere Häuser), ist aber nie bedrohlich.
7. **Reservieren:** Ein begonnener Bau reserviert sein Material. Grundbedarf geht vor: Liegt Brennholz im Notbestand,
   wird kein Holz verbaut.

## 14. Prüfen statt raten: Prüfgrößen [entschieden, ausdrücklicher Wunsch des Users 2026-10-21]

**Das ist verbindlich für alle Arbeits-Sitzungen, die die Wirtschaft bauen oder ändern.**

**Wann geprüft wird:**

1. **Zeitraffer im PC-Simulator** bei jeder Änderung an Wirtschaftswerten oder -logik: mindestens **drei Weltjahre**,
   mit mindestens drei verschiedenen Startwerten (Seeds), ohne Grafik in wenigen Minuten. Der Bericht (Kapitel 15)
   gehört in die PR-Beschreibung.
2. **Echte Läufe auf dem Gerät:** Nach einigen Tagen holt der User das Wirtschafts-Protokoll; eine Balancing-Sitzung
   wertet es aus und schlägt Änderungen vor, die wieder mit Punkt 1 geprüft werden.

**Prüfgrößen und Grenzwerte** (Startwerte, die Balancing-Sitzung darf sie mit Begründung nachschärfen):

| Nr. | Prüfgröße | Grün, wenn |
|---|---|---|
| P1 | **Nichts bleibt leer** | keine Ware länger als **2 Welttage** am Stück auf null (ausgenommen Ware nur von außen) |
| P2 | **Nichts häuft sich an** | kein Lager länger als 3 Welttage an seiner Obergrenze; keine Ware 7 Tage über der Obergrenze im ganzen Dorf |
| P3 | **Standardsuppe selten** | im ersten Frühling höchstens jede 4. Mahlzeit, **danach höchstens 5 % pro Weltwoche**; kein Haushalt 3 Tage am Stück |
| P4 | **Notlösungen selten** | nach dem ersten Weltjahr Kienspan höchstens 20 % der Lichtabende, kalte Abende (Holzmangel) höchstens 2 % |
| P5 | **Gesunde Auslastung** | jede Person arbeitet im Mittel 50–90 % ihrer Arbeitszeit; kein Beruf 3 Tage am Stück über 100 %; Warten auf Material höchstens 10 % |
| P6 | **Bautempo** | kleine Arbeit am selben Tag, Notunterkunft ≤ 4 Tage, Raum ≤ 7 Tage (Ziel 4–5), Grundhäuser aller Startbewohner nach ~2 Monaten |
| P7 | **Werkzeug** | höchstens 20 % der Arbeitszeit mit stumpfem Werkzeug |
| P8 | **Verderb** | höchstens 10 % der verderblichen Herstellung |
| P9 | **Natur** | Wald, Wild, Fische und Wiesen schrumpfen über eine Weltwoche nicht dauerhaft |
| P10 | **Gleiche Wirkung** | grobe und volle Simulation buchen dieselbe Bilanz (Stichprobe: Ort ein- und ausblenden, Differenz 0) |
| P11 | **Händlerin** | jede gedeckte Bestellung wird beim nächsten Besuch erfüllt |

Ein Lauf gilt erst als bestanden, wenn alle Prüfgrößen grün sind. Ausnahmen werden im PR begründet und dem User
genannt.

## 15. Wirtschafts-Protokoll für das Balancing [entschieden: Wunsch des Users; Ausgestaltung Vorschlag]

**Zweck:** Verbrauch, Bedarf und Herstellung aller Waren so mitschreiben, dass eine spätere Arbeits-Sitzung nach
einigen Tagen Laufzeit genau sieht, **was knapp war, was sich angehäuft hat und warum**, und die Startwerte gezielt
verbessern kann. Dasselbe Protokoll speist die Lager-Ansicht und die Chronik der Weboberfläche: eine Quelle, keine
doppelte Logik.

### Was protokolliert wird

**Kernbegriffe:** **Bedarf** = was jemand haben wollte (z. B. Kamin anzünden: 6 Brennholz). **Verbrauch** = was er
bekommen hat. **Fehlmenge** = Bedarf − Verbrauch, mit der Notlösung, die stattdessen griff. **Herstellung** = alles,
was neu entsteht (geerntet, gesammelt, gefertigt). Ohne die Trennung von Bedarf und Verbrauch sieht man Mangel nicht,
weil ein leeres Lager sonst einfach „nichts verbraucht“.

| Datei | Eine Zeile pro | Inhalt |
|---|---|---|
| `meta.json` | Export | Schema-Version, Firmware-Version, **Version und Prüfsumme der Wirtschaftsdaten**, Startwert (Seed), Quelle (Gerät oder Simulator, Zeitraffer-Faktor), Zeitraum in Welt- und Echtzeit, Länge der Jahreszeit, Liste aller Kennungen mit Klarnamen |
| `bewegungen.csv` | Buchung im Warenbuch | Weltzeit, Simulationsstufe (voll, grob, nachgeholt), Art (herstellen, sammeln, verbrauchen, verbauen, umlagern, tauschen, verderben, finden), Ware, Menge, von Lager, nach Lager, Person, Tätigkeit, Projekt, Grund (z. B. `kamin_abend`, `fruehstueck`, `nachschub_holzlege`) |
| `bedarf.csv` | Bedarfsfall mit Fehlmenge | Weltzeit, Person bzw. Haushalt, Zweck, Ware, gewollt, bekommen, Notlösung bzw. Ersatzware |
| `bilanz_tag.csv` | Ware und Welttag | Anfang; Zugang nach Art (hergestellt, gesammelt, eingetauscht, gefunden); Abgang nach Art (verbraucht, verbaut, verdorben, ausgetauscht); Ende; Bedarf, gedeckt, Fehlmenge, ersetzt durch; reserviert; kleinste Reichweite; Soll und Obergrenze; Minuten auf null; Minuten über der Obergrenze |
| `bestand_stunde.csv` | Ware und Weltstunde | Gesamtbestand im Dorf (zeigt Engpässe im Tagesverlauf, z. B. Holz um 20 Uhr leer) |
| `bestand_lager_tag.csv` | Lagerplatz und Ware am Tagesende | Menge, Obergrenze (nur Einträge ungleich null) |
| `arbeit_tag.csv` | Person, Welttag, Tätigkeitsart | Minuten (herstellen je Ware, holen, tragen, bauen, helfen, Werkzeug pflegen, warten auf Material, warten auf Werkzeug, Wege, Freizeit, essen, schlafen); mittlere Leistungsfaktoren (Fähigkeit, Werkzeug, Wetter, Laune) |
| `entscheidungen.csv` | Wahl eines Erzeugers | Weltzeit, Person, gewählte Ware, die Kandidaten mit ihrer Reichweite, Grund (prüft Regel 2 aus Kapitel 13) |
| `versorgung_tag.csv` | Haushalt und Welttag | Personen, Mahlzeiten (gut, einfach, Standardsuppe), Lichtabende (Kerze, Öl, Kienspan), Stunden warm bzw. kühl, Seife ja oder nein, mittlere Laune |
| `projekte.csv` | Projektabschnitt beim Abschluss | Projekt, Haus, Art (klein, Möbel, Notunterkunft, Raum, Ausbaustufe, Großbau, Lok), Abschnitt, geplante und tatsächliche Dauer, Personentage, Wartezeit nach Grund (Material, Tischler, Bautrupp, Wetter), verbautes Material |
| `werkzeuge_tag.csv` | Werkzeug und Welttag | Besitzer, Stufe, Zustand, Arbeitsstunden, geschärft wie oft |
| `natur_tag.csv` | Bestand und Welttag | Wald, Wild, Fisch, Wiese, Schilf, Waldkost: Wert, Nachwuchs, Entnahme |
| `haendlerin.csv` | Besuch | Tag, gebracht, genommen, offene Bestellungen, Rest in der Tauschkiste |
| `wetter_tag.csv` | Welttag | Wetter morgens, mittags, abends, nachts |
| `eingriffe.csv` | Eingriff des Betrachters | Weltzeit, Art (Tipp, Reihenfolge der Wunschliste, Ereignis gestartet, Einstellung), Inhalt; trennt Folgen von Eingriffen und Folgen der Balance |

### Format und Speicher

- **CSV** (UTF-8, Komma, Punkt als Dezimalzeichen, Kopfzeile): direkt mit Python, einer Tabellenkalkulation oder von
  Claude lesbar. Kennungen sind **stabile Schlüssel aus den Datendateien** (`brennholz`, `auslage_baeckerei`,
  `h03_holzlege`); Personen nach **Rolle** (`baeckerin`, `waldarbeiter`), damit Umbenennungen alte Läufe nicht
  unvergleichbar machen. Klarnamen stehen nur in `meta.json`.
- **Zeit:** Welttag ab Weltbeginn (Tag 1), Jahreszeit, Weltjahr und Minute des Tages; dazu ein Zeitstempel in UTC für
  den Abgleich mit anderen Logs.
- **Menge** (geschätzt für 40 Bewohner): etwa 1000 Bewegungen und 125 KB roh pro Welttag, gepackt etwa 25 KB.
- **Speicher:** Der laufende Tag liegt im PSRAM und wird stündlich in eine Zwischendatei gesichert (Stromausfall).
  Nach Tagesende wird der Tag gepackt (`.csv.gz`, zlib ist in ESP-IDF vorhanden) in LittleFS abgelegt. Aufbewahrung:
  Einzelbewegungen 14 Tage, Tageswerte 60 Tage (zusammen etwa 1,5 MB; der Speicherstand bleibt davon unberührt).
  Später mit SD-Karte unbegrenzt.
- **Kein Datenschutzproblem:** keine WLAN-Daten, keine E-Mail, keine Zugangsdaten im Protokoll.
- **Der PC-Simulator schreibt dieselben Dateien** in einen Ordner. Gerät und Simulator sind damit direkt vergleichbar.

### Abholen und Auswerten

1. **Weboberfläche:** Diagnose → „Wirtschafts-Protokoll“ → Zeitraum wählen → als ein Archiv herunterladen; dazu eine
   API `GET /api/wirtschaft/protokoll?von=…&bis=…`.
2. **Werkzeug** `tools/wirtschaft.ps1` (wie `netlog.ps1`): holt das Protokoll per WLAN nach
   `.logs/wirtschaft/<datum>/`.
3. **Auswertung** `tools/balance_report.py <ordner> [--vergleich <ordner>]` schreibt einen **kurzen Bericht**
   (`bericht.md`, höchstens etwa 200 Zeilen) und optional Diagramme (Bestand je Ware über die Zeit):
   - Kopf: Zeitraum, Versionen, Seed, Bewohner, Häuser, Quelle
   - **Ampel aller Prüfgrößen P1–P11** mit Zahlen
   - **Engpässe:** Waren nach Fehlmenge, jeweils mit Ursache (Erzeuger ausgelastet, Erzeuger mit anderem beschäftigt,
     Vorprodukt fehlte, Werkzeug stumpf, Wetter, Wege zu lang)
   - **Überschüsse und Verderb**
   - **Auslastung** je Person und Beruf; Kipppunkte bei aktuellem Wachstum („Waldarbeiter erreicht 100 % bei 12
     Häusern“)
   - Versorgung je Haushalt (Standardsuppe, Kienspan, kalte Abende)
   - Bau: Projekte mit geplanter und tatsächlicher Dauer, Wartegründe
   - Natur-Bestände als Trend
   - Vergleich mit einem früheren Lauf (vorher/nachher je Prüfgröße)
   - Hinweise, wo man drehen könnte (nur Hinweise; entschieden wird mit dem User)
4. **Sparsam:** Die Balancing-Sitzung liest den Bericht und nur bei Bedarf gezielt einzelne Zeilen aus den CSVs, nie
   die Rohdaten im Ganzen (Nutzungslimit, CLAUDE.md).

### Ablauf einer Balancing-Sitzung

1. Gerät läuft einige Tage (oder Zeitraffer im Simulator).
2. `tools/wirtschaft.ps1` holt das Protokoll, `tools/balance_report.py` schreibt den Bericht.
3. Claude bespricht den Bericht mit dem User in Alltagssprache und schlägt Änderungen an den Wirtschaftsdaten vor.
4. Die Änderungen stehen in den Datendateien (`assets/`, mit neuer Versionsnummer), nicht im Code.
5. Zeitraffer mit denselben Seeds vorher und nachher (Kapitel 14); der Vergleich kommt in die PR-Beschreibung.
6. Nach dem Merge läuft das Gerät wieder einige Tage, dann wiederholen.

## 16. Offene Punkte

1. **Warenliste, Händlerin, Kipppunkte, Startvorrat** (diese Datei): warten auf das OK des Users. Kapitel 8
   (Herstellung, Arbeitsrhythmus) hat der User übernommen; geprüft wird im Zeitraffer (Kapitel 14).
2. **Ruhetag** [Idee]: ein freier Tag pro Weltwoche (Platz, Ausflüge, Feste). Senkt die Kapazität um ein Siebtel; die
   Startwerte haben dafür genug Luft.
3. **Gerste und Bier:** ab wann (erstes Herbstfest oder später)?
4. **Zuzugsliste:** Reihenfolge der Zuzügler nach den Kipppunkten (Kapitel 12) festlegen, wenn die Namen stehen.
