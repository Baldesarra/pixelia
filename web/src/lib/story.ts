// „So entstand Pixelia“ (seit 0.5.4): Rückblick auf die Entwicklung, als Easter Egg nach 7× Tippen aufs
// Pixelia-Zeichen oben links und als Seite unter System → Experimentell. Neue Arbeitsstände und Zwischenstände hier unten anhängen (Datum, Titel, ein bis zwei
// Sätze, optional Links). Die Skizzen sind zu groß für die Firmware (zusammen ~920 KB) und liegen auf GitHub Pages
// (pages.yml kopiert docs/skizzen); entschieden in 0.5.5: sie bleiben dort (Projektplan Kapitel 14). Seit 0.5.10 dazu
// kleine Vorschaubilder in der Firmware, damit die Seite auch ohne Internet Bilder zeigt.

import lagerEntwuerfe from '../assets/story/0510-entwuerfe.webp';
import lagerIdeen from '../assets/story/0510-ideen.webp';
import karte from '../assets/story/0506-karte.webp';
import mond from '../assets/story/0507-mond.webp';
import raum from '../assets/story/0508-raum.webp';
import statur from '../assets/story/0509-statur.webp';
import lagerFortschritt from '../assets/story/0510-fortschritt.webp';

export const SKETCH_URL = 'https://baldesarra.github.io/pixelia/skizzen/';

export type StoryLink = { label: string; href: string };
// Bilder (seit 0.5.10): kleines Vorschaubild in der Firmware (web/src/assets/story, WebP ~10–20 KB), antippen öffnet
// das große Bild auf GitHub Pages
export type StoryImage = { src: string; alt: string; href: string };
export type StoryStep = { d: string; title: string; text: string; links?: StoryLink[]; images?: StoryImage[] };

const sketch = (file: string, label: string): StoryLink => ({ label, href: SKETCH_URL + file });
const picture = (src: string, file: string, alt: string): StoryImage => ({ src, alt, href: SKETCH_URL + file });

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
  {
    d: '2026-10-22',
    title: 'Doppelt im Speicher',
    text: 'Eine Überraschung hinter den Kulissen: Weil die Firmware selbst im PSRAM läuft, lag jedes eingebaute Bild zweimal im Speicher, einmal gepackt und einmal entpackt. Seitdem wohnt nur noch das Nötigste in der Firmware.',
  },
  {
    d: '2026-10-22',
    title: 'Die falsche Figur',
    text: 'Eigentlich sollte langes Halten auf einer Figur ihr folgen. Im Test erwischte der Finger aber ständig jemanden, der gerade vorbeilief. Also bekam die Karte einen Knopf „Folgen“, und langes Halten zeigt wie früher Adresse und Zustand des Geräts.',
    images: [picture(karte, '0.5.6/karte-seite.png', 'Karte einer Figur am Rand')],
  },
  {
    d: '2026-10-26',
    title: 'Tag 1',
    text: 'Am 26. Oktober 2026 begann in Pixelia die Zeit. Das Wetter wird seitdem aus dem Datum berechnet statt gewürfelt: Nach einem Neustart regnet es genau wie vorher. Und der Mond über dem Dorf hat dieselbe Phase wie der echte draußen.',
    images: [picture(mond, '0.5.7/mond-vergleich.png', 'Der Mond in seinen Phasen')],
  },
  {
    d: '2026-10-27',
    title: 'Eine Kerze für Heimkehrer',
    text: 'Ist nachts noch jemand unterwegs, brennt auf der Fensterbank eine Kerze, bis alle zu Hause sind. Zur Wahl standen Kachelofen, Herd und Kamin. Gewonnen hat der Herd, unter dessen Abdeckung nachts die Glut weiterglimmt.',
    images: [picture(raum, '0508-raum-mond.png', 'Testraum nachts: Herdfeuer, Kerze, Mond')],
  },
  {
    d: '2026-10-29',
    title: 'Doppelt so teuer wie gedacht',
    text: 'Figuren mit Bauch, Gehstock und vier Größen sollten 375 Mikrosekunden je Bild kosten, das Gerät brauchte doppelt so viel. Die Bildrate hielt trotzdem. Ob der Speicherstand hält, wurde ganz altmodisch geprüft: Stecker ziehen.',
    images: [picture(statur, 'figuren-statur.png', 'Figuren in vier Größen und drei Breiten')],
  },
  {
    d: '2026-10-30',
    title: 'Wo ist das Brot?',
    text: 'Im Testraum steht ein Brotregal, obwohl Brot um 1850 im Brotkorb lag. Es bleibt Testinhalt bis zur echten Küche. Die größere Frage kam später: Was zeigt die Lager-Karte, wenn das Dorf 16 Häuser hat? Vier Lösungswege später gewinnt „Sorgen zuerst“, und eine Welle, die stundenlang Eile vortäuschte, muss ruhigen Segmenten weichen.',
    images: [
      picture(lagerEntwuerfe, '0510-lager-karte-entwuerfe.png', 'Erste Runde: drei Entwürfe der Lager-Karte'),
      picture(lagerIdeen, '0510-lager-ideen.png', 'Großes Dorf: vier Lösungswege'),
      picture(lagerFortschritt, '0510-fortschritt-varianten.png', 'Ruhiger Fortschritt: Füllbalken, Segmente, stille Welle'),
    ],
  },
];
