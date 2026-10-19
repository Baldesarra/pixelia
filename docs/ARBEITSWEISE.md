# Arbeitsweise mit Claude: Qualität halten, Nutzungslimit schonen

Stand 2026-10-19. Anlass: Die Sitzung für 0.2.2 hat allein ein ganzes 5-Stunden-Limit verbraucht.

## Warum eine Sitzung teuer wird

- Claude arbeitet in Schritten (Datei lesen, ändern, bauen, prüfen …). **Bei jedem Schritt wird der ganze bisherige
  Verlauf neu gelesen.**
- Je länger eine Sitzung läuft, desto teurer wird also jeder weitere Schritt. 0.2.2 hatte 634 Schritte und am Ende
  740.000 Token Kontext. Zusammengerechnet wurden etwa 318 Millionen Token gelesen.
- Eine **neue** Sitzung kostet zum Start nur etwa 80.000 bis 100.000 Token (Regeln, CLAUDE.md, Roadmap,
  Code-Landkarte). Das ist billig im Vergleich.
- Die Denkstufe „Extra Hoch“ (xhigh) schreibt bei jedem Schritt längere Überlegungen in den Verlauf. Die werden
  danach bei jedem weiteren Schritt mitgelesen und kosten dadurch doppelt.

**Merksatz: Viele kurze Sitzungen sind billiger als eine lange.**

## Die zwei Arten von Chats

| | Planungs-Chat („Masterplan“) | Arbeits-Sitzung |
|---|---|---|
| Wofür | Ideen, Entscheidungen, Reihenfolge, Aufträge vorbereiten | Eine Version bauen (z. B. 0.3.0) |
| Wie lange | Dauerhaft; mit `/compact` klein halten | Eine Version, höchstens ca. 250.000 Token |
| Denkstufe | Hoch | Hoch (Extra Hoch nur auf Ansage) |
| Gedächtnis | Memory und Repo | Repo: CLAUDE.md, ROADMAP, CODEMAP, CHANGELOG |

## Schritt für Schritt

### A. Eine neue Version starten

1. Im **Planungs-Chat** schreiben: „Bereite die nächste Version vor.“
2. Claude legt eine **Sitzungs-Karte** an. Denkstufe und Modell sind darin schon eingestellt. Steht im Auftrag
   ganz oben **„⚠ Denkstufe: Extra Hoch“**, sagt Claude das auch ausdrücklich im Chat.
3. Auf der Karte auf **Starten** klicken. Die Sitzung läuft für sich und meldet sich nicht im Planungs-Chat.
4. In der Arbeits-Sitzung beantwortest du Fragen und machst die Hardware-Tests, wie bisher.

### B. Während die Arbeits-Sitzung läuft

5. Ab und zu auf die **Kontextanzeige** schauen (der Kreis bzw. die Prozentzahl am Eingabefeld). 250.000 Token sind
   bei einem 1-Millionen-Fenster etwa **25 %**.
6. Ist die Anzeige über 25 % und die Version noch nicht fertig, schreibst du: **„Bitte Übergabe schreiben.“**
   Das darfst du jederzeit schreiben, auch wenn Claude gerade arbeitet. Die Nachricht wird beim nächsten Schritt
   gelesen. Claude bringt dann zuerst die laufende Änderung sauber zu Ende (Code baut, nichts halb fertig),
   sichert alles auf GitHub, trägt den Stand ins Repo ein und gibt dir einen kurzen Auftragstext.
   Nicht auf **Stopp** drücken: Das bricht mitten in einer Änderung ab.
7. Neue Sitzung starten (Knopf **Neue Sitzung** in der Seitenleiste), den Auftragstext einfügen und abschicken.
   Die Denkstufe im Eingabefeld auf **Hoch** lassen, außer im Auftrag steht etwas anderes.

### C. Wenn die Version fertig ist

8. Die Arbeits-Sitzung meldet „fertig“ und hat die Übergabe schon geschrieben (Roadmap, Changelog, Release).
9. Im Planungs-Chat kurz schreiben: **„0.3.0 ist fertig.“** Claude liest den Stand im Repo nach, nicht im Chat.
10. Die alte Arbeits-Sitzung kannst du **archivieren** (Rechtsklick in der Seitenleiste). Weitermachen solltest du
    darin nicht, sie ist schon groß.

### D. Den Planungs-Chat klein halten

11. Wird der Planungs-Chat groß (über ca. 15 %, also 150.000 Token), tippst du **`/compact`** ein.
12. Bauen, testen und lange Fehlersuchen gehören in Arbeits-Sitzungen, nicht in den Planungs-Chat.

### E. Kleine Spartipps

- Wünsche und Ideen ruhig sammeln und gebündelt schicken statt einzeln.
- Eine neue Idee zwischendurch? Kurz in den Planungs-Chat schreiben. Claude trägt sie ins Backlog ein.
- Läuft das Limit fast leer, die Sitzung lieber eine Übergabe schreiben lassen, statt sie mitten in einer Änderung
  abbrechen zu lassen.

## Denkstufe: Hoch oder Extra Hoch?

- **Hoch** ist der Normalfall und sehr gründlich: Weboberfläche, Werkzeuge, Doku, normale Funktionen, kleine Fehler.
- **Extra Hoch** nur, wenn Claude es **ausdrücklich ansagt**, zum Beispiel:
  - Engine-Grundgerüst (0.5.0),
  - Design der Dorf-Simulation (0.7.0),
  - hartnäckige Abstürze, Timing- oder Speicherfehler.
- Claude stellt die Denkstufe beim Anlegen der Sitzungs-Karte selbst ein. Startest du eine Sitzung von Hand, steht
  die empfohlene Stufe in der ersten Zeile des Auftrags.

## Vorlage: Auftrag für eine neue Arbeits-Sitzung

```text
Denkstufe: Hoch
Aufgabe: Pixelia <Version> „<Titel>“.
Lies zuerst CLAUDE.md (geschieht automatisch), docs/ROADMAP.md „Aktueller Stand“ und den Abschnitt <Phase>,
dann docs/CODEMAP.md. Weitere Dateien nur gezielt.
Umfang: <Liste der Punkte aus der Roadmap>
Offen aus der letzten Sitzung: <Punkte oder „nichts“>
Fertig heißt: PR gemergt, Changelog-Eintrag, Roadmap und CODEMAP aktualisiert, Release <vX.Y.Z>, am Gerät geprüft.
Obergrenze ca. 250 K Kontext, dann Übergabe schreiben.
```
