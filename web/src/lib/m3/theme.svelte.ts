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
    return JSON.stringify({ mode: this.mode, seed: this.seed, style: this.style });
  }

  /** Auswahl aus JSON übernehmen; Unbekanntes wird ignoriert */
  adopt(json: string) {
    try {
      const v = JSON.parse(json) as Partial<{ mode: string; seed: string; style: string }>;
      if (v.mode === 'system' || v.mode === 'light' || v.mode === 'dark') this.mode = v.mode;
      if (typeof v.seed === 'string' && /^#[0-9a-f]{6}$/i.test(v.seed)) this.seed = v.seed;
      if (v.style === 'tonal' || v.style === 'vibrant' || v.style === 'expressive') this.style = v.style;
    } catch {
      /* kaputter Wert: Auswahl bleibt */
    }
  }

  /** Farben auf :root schreiben und Auswahl merken (läuft als $effect in App.svelte) */
  apply() {
    const hct = Hct.fromInt(argbFromHex(this.seed));
    const Kind = { tonal: SchemeTonalSpot, vibrant: SchemeVibrant, expressive: SchemeExpressive }[this.style];
    const scheme: DynamicScheme = new Kind(hct, this.dark, 0, '2025');
    const root = document.documentElement;
    for (const c of new MaterialDynamicColors().allColors) {
      root.style.setProperty(`--md-sys-color-${c.name.replaceAll('_', '-')}`, hexFromArgb(c.getArgb(scheme)));
    }
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
      ?.setAttribute('content', hexFromArgb(MaterialDynamicColors.surfaceContainer.getArgb(scheme)));
    try {
      localStorage.setItem(KEY, JSON.stringify({ mode: this.mode, seed: this.seed, style: this.style }));
    } catch {
      /* privates Fenster: dann eben nicht merken */
    }
  }
}

export const theme = new Theme();
