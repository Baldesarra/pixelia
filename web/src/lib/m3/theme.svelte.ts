// Dynamische Farben (M3, Spezifikation 2025) aus einer Akzentfarbe, hell/dunkel/System.
// Die Wahl liegt im Browser (localStorage, für den schnellen Start) und seit 0.3.2 auf dem Gerät (theme-sync).
import {
  argbFromHex,
  Hct,
  hexFromArgb,
  MaterialDynamicColors,
  SchemeExpressive,
  SchemeTonalSpot,
  SchemeVibrant,
  TonalPalette,
  type DynamicScheme,
} from '@material/material-color-utilities';

export type ThemeMode = 'system' | 'light' | 'dark';
export type ThemeStyle = 'tonal' | 'vibrant' | 'expressive';
/** Aussehen der Oberfläche (seit 0.5.4, Vorschau unter Experimentell): Material 3 oder eigene Designs (design.css) */
export type Design = 'm3' | 'pixel' | 'fahrplan';

export const SEEDS = [
  { hex: '#2e6b5e', name: 'Moos' },
  { hex: '#3a6ea5', name: 'Himmel' },
  { hex: '#c2552b', name: 'Abendrot' },
  { hex: '#7a5aa6', name: 'Lavendel' },
  { hex: '#a07a00', name: 'Sonne' },
  { hex: '#b23a6b', name: 'Beere' },
];
/** Ab Werk (seit 0.5.0, Rückblick-Runde): Lavendel; eine gespeicherte Wahl bleibt */
export const DEFAULT_SEED = '#7a5aa6';

/** Zusatzfarben: success = Grün (Behoben, OK), warning = Bernstein (Hinweise) → --pixelia-color-<name>[-container] */
const CUSTOM = [
  ['success', '#2e7d32'],
  ['warning', '#b07800'],
] as const;

// Eigene Designs: feste Farben statt Akzentfarbe, je hell und dunkel in der Reihenfolge von ROLES; die übrigen Rollen
// (Fehler, inverse Flächen) kommen aus einem M3-Schema mit der Hauptfarbe als Grundton.
const ROLES = [
  'surface', 'surface-container-lowest', 'surface-container-low', 'surface-container', 'surface-container-high',
  'surface-container-highest', 'on-surface', 'on-surface-variant', 'outline', 'outline-variant', 'primary', 'on-primary',
  'primary-container', 'on-primary-container', 'secondary', 'secondary-container', 'on-secondary-container', 'tertiary',
  'tertiary-container', 'on-tertiary-container',
];
const LOOKS: Record<Exclude<Design, 'm3'>, [string, string]> = {
  // Pixelia: Pergament und Ziegelrot am Tag, Nachtblau und Laternengelb im Dunkeln
  pixel: [
    'efe4cc fffaf0 f6edd9 fbf5e6 f3e6c8 ead9b4 2a2140 5d5470 6e6385 c9b791 b8462f fff8ec f6cdb0 4a1709 8a5a14 f3dfb0 3d2c08 3f7a3a cfe3b0 20330f',
    '161a33 11142a 1d2140 232847 2b3155 353b63 f4ead2 b8b0cc 8d86a8 3f456f f2b544 2a1d05 5a4317 ffe2a6 e9b949 3a3f6b e3defa 8fcf7a 2f4a2a cdeebf',
  ],
  // Fahrplan 1850: Sepia-Papier, Tinte und Bahngrün am Tag, Messing auf dunklem Holz im Dunkeln
  fahrplan: [
    'e9dfc8 fbf6ea f3ebd8 f4ecd8 ede2c9 e4d7ba 2b2118 6b5b48 6b5333 c5b48f 1f4d3a f3ead2 cfe0c9 0f2a1e 8a6a2a e6d6ae 3d2f10 9a3b26 f0d2c4 4a1b0e',
    '1c1a16 151310 221f1a 29251e 312c24 3a342a eadfc6 b3a68c 8a7448 4a4030 c9a24e 1c1a16 4a3c1c f2deb0 b8a37a 3b3427 eadfc6 8fbf9f 2e4237 cfe6d6',
  ],
};

const KEY = 'pixelia.theme';
const media = matchMedia('(prefers-color-scheme: dark)');

function load(): { mode: ThemeMode; seed: string; style: ThemeStyle } {
  const fallback = { mode: 'system' as ThemeMode, seed: DEFAULT_SEED, style: 'tonal' as ThemeStyle };
  try {
    return { ...fallback, ...JSON.parse(localStorage.getItem(KEY) ?? '{}') };
  } catch {
    return fallback;
  }
}

function hasStored(): boolean {
  try {
    return localStorage.getItem(KEY) !== null;
  } catch {
    return false;
  }
}

class Theme {
  mode = $state<ThemeMode>('system');
  seed = $state(DEFAULT_SEED);
  style = $state<ThemeStyle>('tonal');
  design = $state<Design>('m3');
  /** Eigene Designs nur, solange Experimentell an ist (App.svelte setzt das aus dem Status) */
  allowed = $state(true);
  look = $derived(this.allowed ? this.design : 'm3');
  systemDark = $state(media.matches);
  /** Hatte dieser Browser schon eine eigene Wahl gespeichert? (Ein frischer Browser überträgt seinen Standard nicht) */
  readonly hadChoice = hasStored();
  dark = $derived(this.mode === 'dark' || (this.mode === 'system' && this.systemDark));

  constructor() {
    Object.assign(this, load());
    media.addEventListener('change', (e) => (this.systemDark = e.matches));
  }

  /** Auswahl als kurzer JSON-Text (so speichert sie das Gerät, seit 0.3.2) */
  json(): string {
    const { mode, seed, style, design } = this;
    return JSON.stringify(design === 'm3' ? { mode, seed, style } : { mode, seed, style, design });
  }

  /** Auswahl aus JSON übernehmen; Unbekanntes wird ignoriert */
  adopt(json: string) {
    try {
      const v = JSON.parse(json) as Partial<{ mode: string; seed: string; style: string; design: string }>;
      if (v.mode === 'system' || v.mode === 'light' || v.mode === 'dark') this.mode = v.mode;
      if (typeof v.seed === 'string' && /^#[0-9a-f]{6}$/i.test(v.seed)) this.seed = v.seed;
      if (v.style === 'tonal' || v.style === 'vibrant' || v.style === 'expressive') this.style = v.style;
      this.design = v.design === 'pixel' || v.design === 'fahrplan' ? v.design : 'm3';
    } catch {
      /* kaputter Wert: Auswahl bleibt */
    }
  }

  /** Farben auf :root schreiben und Auswahl merken (läuft als $effect in App.svelte) */
  apply() {
    const look = this.look === 'm3' ? undefined : LOOKS[this.look][+this.dark].split(' ');
    const hct = Hct.fromInt(argbFromHex(look ? '#' + look[10] : this.seed));
    const Kind = { tonal: SchemeTonalSpot, vibrant: SchemeVibrant, expressive: SchemeExpressive }[look ? 'tonal' : this.style];
    const scheme: DynamicScheme = new Kind(hct, this.dark, 0, '2025');
    const root = document.documentElement;
    for (const c of new MaterialDynamicColors().allColors) {
      root.style.setProperty(`--md-sys-color-${c.name.replaceAll('_', '-')}`, hexFromArgb(c.getArgb(scheme)));
    }
    look?.forEach((hex, i) => root.style.setProperty(`--md-sys-color-${ROLES[i]}`, '#' + hex));
    root.dataset.design = this.look;
    // Zusatzfarben (M3 „custom colors“) mit festem Farbton, damit Grün und Bernstein bei jeder Akzentfarbe
    // erkennbar bleiben; Töne wie bei den Systemfarben (hell 40/100/90/10, dunkel 80/20/30/90)
    const tones = this.dark ? [80, 20, 30, 90] : [40, 100, 90, 10];
    for (const [name, hex] of CUSTOM) {
      const p = TonalPalette.fromInt(argbFromHex(hex));
      const [c, on, box, onBox] = tones.map((t) => hexFromArgb(p.tone(t)));
      root.style.setProperty(`--pixelia-color-${name}`, c);
      root.style.setProperty(`--pixelia-color-on-${name}`, on);
      root.style.setProperty(`--pixelia-color-${name}-container`, box);
      root.style.setProperty(`--pixelia-color-on-${name}-container`, onBox);
    }
    root.style.colorScheme = this.dark ? 'dark' : 'light';
    root.dataset.theme = this.dark ? 'dark' : 'light';
    document
      .querySelector('meta[name="theme-color"]')
      ?.setAttribute('content', look ? '#' + look[3] : hexFromArgb(MaterialDynamicColors.surfaceContainer.getArgb(scheme)));
    try {
      localStorage.setItem(KEY, this.json());
    } catch {
      /* privates Fenster: dann eben nicht merken */
    }
  }
}

export const theme = new Theme();
