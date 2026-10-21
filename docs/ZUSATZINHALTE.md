# Pixelia – Vor 1.0.0 klären und Zusatzinhalte

> Angelegt mit der Neuplanung nach dem Setting (2026-10-21, Wunsch des Users). Zwei Listen: was vor 1.0.0 entschieden
> sein muss, und was nach 1.0 als Update oder Zusatzinhalt kommen kann. Der Fahrplan bis 1.0 steht in
> [ROADMAP.md](ROADMAP.md), das Setting in [SETTING.md](SETTING.md). Neue Punkte mit Datum ergänzen; entschiedene
> Punkte abhaken und die Entscheidung ins Entscheidungsprotokoll ([PROJEKTPLAN.md](PROJEKTPLAN.md) Kapitel 14)
> schreiben.

## Vor 1.0.0 klären

Spätestens bis 1.0.0, die meisten früher. In Klammern, wann es spätestens nötig ist.

**Inhalt und Welt**

- [x] **Museum** (Wunsch des Users, zuerst als Zusatzinhalt gedacht): entschieden am 2026-10-21, **komplett im
      Grundspiel in 0.7.x**, mit mehr als den 45 Stücken der Startliste. Wie viele Stücke realistisch sind (echte
      Angaben, Quellen, zwei Bilder je Stück), prüft die Museums-Sitzung in 0.7.x. Neue Sammlungen aus neuen Gebieten
      (z. B. Küste) kommen per Update (unten)
- [ ] **Easter Eggs** (Backlog 2026-10-20/21): welche es gibt, wo sie versteckt sind (Weboberfläche, Display), wie man
      sie findet, ob sie hinter einem Schalter liegen. Erstes: die Skizzen „So entstand Pixelia“ ([docs/skizzen](skizzen/README.md)).
      Umfang als Auswahlliste vorlegen; Ablage der Bilder legt 0.5.5 fest (Web-Bundle ≤ 150 KB gzip)
- [ ] **Inselwelt** (Rohkonzept [welten/inselwelt-konzept.md](welten/inselwelt-konzept.md)): als eigene Welt verworfen,
      die Ideen Gedächtnis und Lager-Seite sind übernommen. Offen: als Ort oder Region per Update (Insel im See, Küste)
      oder ganz streichen?
- [ ] **Frühere Welt-Ideen als Module:** Was nicht schon eingeplant ist ([ROADMAP.md](ROADMAP.md) „Frühere
      Welt-Ideen“), z. B. Spiele auf dem Dorffest statt Pixel-Liga: aufnehmen oder streichen?
- [ ] **Erste Region hinter der Strecke** ([SETTING.md](SETTING.md) Kapitel 18, Punkt 2), damit die Lok in 0.9.x
      plausibel zur Nachbarstadt fährt (vor 0.9.0)
- [ ] **Mindestziele je Kapitel** ([SETTING.md](SETTING.md) Kapitel 18, Punkt 6): Kapitel 1 vor 0.6.0, Grube vor
      0.8.0, Lok vor 0.9.0, Welt als Ganzes vor 1.0.0
- [ ] **Ideenliste der Werkstatt** ([SETTING.md](SETTING.md) Kapitel 18, Punkt 1): Katze als Kupplerin, Taylors Lieder
      über echte Ereignisse, Vertrauen der Bewohner in Sykora (vor der Wahl in 0.7.x), Lok taufen per Abstimmung
- [ ] **Ruhetag** einmal pro Weltwoche, **Gerste und Bier** ab wann ([WIRTSCHAFT.md](WIRTSCHAFT.md) Kapitel 16) (vor
      der Wirtschaft in 0.7.x)

**Technik und Gerät**

- [ ] **Ab wann zählt die Welt?** Vor 1.0 darf ein Update die Welt neu starten (Vorabversionen). Ab welcher Version
      muss jeder Speicherstand weiterladen? (vor 0.6.0)
- [ ] **Nachholen nach dem Ausschalten:** wie lange höchstens (Vorschlag 24 h, [SETTING.md](SETTING.md) Kapitel 18,
      Punkt 4) (vor 0.5.9)
- [ ] **Echte Bezüge:** Die Weltjahreszeit dauert 7 Tage und das Wetter wird simuliert. Echtes Wetter vom Standort
      (Open-Meteo, früher für 0.5.6 geplant) oder die echte Jahreszeit als Option? (vor 0.5.7)
- [ ] **Eigene Möbel des Users:** Format in 0.6.0, Upload über die Weboberfläche in 0.7.x. Wo gespeichert (Flash, SD),
      wie viele?
- [ ] **Ton:** Das Board hat einen Lautsprecher-Anschluss „Speak“ (P6) mit Verstärker U4. Prüfen und entscheiden, ob
      Pixelia Geräusche bekommt (Regen, Feuer, Schafe, Taylors Lied) oder still bleibt
- [ ] **Lichtsensor** BH1750 nachrüsten (lernende Helligkeit) oder bei Helligkeitskurve und Nachtmodus (0.5.7) bleiben?
- [ ] **SD-Karte:** Ab wann zieht die Welt auf die Karte um, und braucht das Gerät dann eine Karte?
- [ ] **Browser-Simulator auf GitHub Pages** (früher 0.5.7): die Welt im Browser ansehen, ohne Gerät. Bericht und
      Kontaktbogen im PR kommen schon aus dem kopflosen Simulator (0.5.12). Noch gewünscht?
- [ ] **Sprache:** Oberfläche und Texte nur auf Deutsch?

## Zusatzinhalte und Updates (nach 1.0 bzw. DLC)

Alles hier ist eine Idee, kein Versprechen. Grundregel aus dem Setting: Jedes Projekt lässt sich mit dem vorhandenen
Inhalt abschließen; was auf ein Update wartet, ist in der Welt plausibel versperrt.

| Zusatzinhalt | Was | Herkunft |
|---|---|---|
| **Neue Regionen mit dem Zug** | Marktstadt, Küste (mit Leuchtturm), Bergdorf; die Nachbarstadt wird ein echter Ort | [SETTING.md](SETTING.md) Kapitel 4 |
| **Der Tunnel ist wieder frei** | die verschüttete Strecke nach hinten als eigenes Großprojekt | [SETTING.md](SETTING.md) Kapitel 1 |
| **Neue Museumsstücke** | Sammlungen aus neuen Gebieten (z. B. Meeresfische und Muscheln von der Küste) | Wunsch des Users (Museum) |
| **Weitere Zuzügler** | neue Bewohner mit Geschichte, Kinder von Zuzüglern, neue Berufe nach den Kipppunkten | [WIRTSCHAFT.md](WIRTSCHAFT.md) Kapitel 12 |
| **Jahresfeste und Ereigniswellen** | neue Anlässe, seltene und legendäre Ereignisse | [LEBENDIGKEIT.md](LEBENDIGKEIT.md) Regel 8 |
| **Inselwelt** | als Ort oder Region, falls oben so entschieden | früheres Welt-Konzept |
| **Geld und Markt** | nur als eigenes Update, falls je gewünscht (bis dahin kein Geld) | Entscheidung des Users 2026-10-21 |
| **Ton-Paket** | Geräusche und Musik, falls die Hardware-Prüfung oben positiv ist | Backlog 2026-10-18 |
| **Möbel-Galerie** | eigene Möbel des Users und neue Möbelsätze | Backlog 2026-10-21 |
