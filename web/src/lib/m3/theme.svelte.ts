// Dynamische Farben (M3, Spezifikation 2025) aus einer Akzentfarbe, hell/dunkel/System.
// Die Wahl liegt vorerst im Browser (localStorage); ab 0.3.2 kann das Gerät sie speichern.
import {
  argbFromHex,
  Hct,
  hexFromArgb,
  MaterialDynamicColors,
  SchemeExpressive,
  SchemeTonalSpot,
  SchemeVibrant,
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

const KEY = 'pixelia.theme';
const media = matchMedia('(prefers-color-scheme: dark)');

function load(): { mode: ThemeMode; seed: string; style: ThemeStyle } {
  const fallback = { mode: 'system' as ThemeMode, seed: SEEDS[0].hex, style: 'tonal' as ThemeStyle };
  try {
    return { ...fallback, ...JSON.parse(localStorage.getItem(KEY) ?? '{}') };
  } catch {
    return fallback;
  }
}

class Theme {
  mode = $state<ThemeMode>('system');
  seed = $state(SEEDS[0].hex);
  style = $state<ThemeStyle>('tonal');
  systemDark = $state(media.matches);
  dark = $derived(this.mode === 'dark' || (this.mode === 'system' && this.systemDark));

  constructor() {
    Object.assign(this, load());
    media.addEventListener('change', (e) => (this.systemDark = e.matches));
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
