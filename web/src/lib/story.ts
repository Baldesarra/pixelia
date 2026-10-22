// „So entstand Pixelia“ (seit 0.5.4): Rückblick auf die Entwicklung, als Easter Egg nach 7× Tippen aufs
// Pixelia-Zeichen oben links und als Seite unter System → Experimentell. Neue Arbeitsstände und Zwischenstände hier unten anhängen (Datum, Titel, ein bis zwei
// Sätze, optional Links). Die Skizzen sind zu groß für die Firmware (zusammen ~920 KB) und liegen auf GitHub Pages
// (pages.yml kopiert docs/skizzen); wo sie später auf dem Gerät liegen, entscheidet 0.5.5 (Asset-Quelle).

export const SKETCH_URL = 'https://baldesarra.github.io/pixelia/skizzen/';

export type StoryLink = { label: string; href: string };
export type StoryStep = { d: string; title: string; text: string; links?: StoryLink[] };

const sketch = (file: string, label: string): StoryLink => ({ label, href: SKETCH_URL + file });

export const STORY: StoryStep[] = [
  {
    d: '2026-10-18',
    title: 'Die Idee',
    text: 'Kleine lebendige Pixelwelten auf einem 5-Zoll-Display, das einfach auf dem Tisch steht. Am ersten Tag läuft schon eine bewegte Landschaft flüssig.',
  },
  {
    d: '2026-10-18',
    title: 'Ohne Kabel',
    text: 'Pixelia kommt ins WLAN, bekommt Updates ohne USB und zeigt per langem Druck seine Adresse.',
  },
  {
    d: '2026-10-19',
    title: 'Eine eigene Oberfläche',
    text: 'Die Weboberfläche im Material-Design entsteht, komplett offline vom Gerät, mit Farben zum Aussuchen.',
  },
  {
    d: '2026-10-20',
    title: 'Updates von GitHub',
    text: 'Pixelia holt sich neue Versionen selbst. Der Versionsverlauf und die Diagnose ziehen ein.',
  },
  {
    d: '2026-10-20',
    title: 'Fünf Blickwinkel',
    text: 'Wie soll die Welt aussehen? Seitenansicht, Querschnitt, Draufsicht, Isometrie und Bühne im Vergleich, dann die Mischung mit fester Kamera.',
    links: [sketch('perspektiven-v1.html', 'Skizze 1: Perspektiven'), sketch('szenen-v2.html', 'Skizze 2: Szenen')],
  },
  {
    d: '2026-10-20',
    title: 'Figuren und Maßstab',
    text: 'Kleine Chibi-Figuren, zehn Bewohner zum Ausprobieren, dann der Maßstab, in dem heute jede Szene entsteht.',
    links: [sketch('figuren-v3.html', 'Skizze 3: Figuren'), sketch('massstab-v4.html', 'Skizze 4: Maßstab')],
  },
  {
    d: '2026-10-21',
    title: 'Das Dorf',
    text: 'Die Draufsicht für alle Orte ist entschieden. Und die Geschichte: ein Zug, ein Winter, der nicht endete, und Gestrandete, die bleiben.',
    links: [sketch('dorfansicht-v5.html', 'Skizze 5: Dorfplatz')],
  },
  {
    d: '2026-10-21',
    title: 'Bühne, Licht und Wetter',
    text: 'Die Engine wächst: Orte und Figuren, Räume zum Ausbauen, Tageslicht und Lampen, dann Regen, Schnee und Gewitter.',
  },
  {
    d: '2026-10-22',
    title: 'Aufgeräumt',
    text: 'Die Oberfläche bekommt eine neue Ordnung mit Welt, Übersicht, System und Einstellungen. Und dieses kleine Geheimnis.',
  },
];
