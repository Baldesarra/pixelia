// Weltzustand für die Oberfläche (seit 0.5.9): Speicherstand, Figuren und Chronik aus /api/world (?mehr=1)
import { get } from './api';

export type SaveInfo = {
  gen: number;
  bytes: number;
  ago: number; // Sekunden seit der letzten Speicherung, −1 = seit dem Start noch keine
  next: number; // Sekunden bis zur nächsten
  loaded: boolean;
  packUs: number;
  writeMs: number;
  figures: number;
  transit: number;
  chronicle: number;
  caughtUp: number; // Weltminuten nachgeholt nach dem Laden
  skipped: number;
  catchingUp: number;
  test: boolean;
  waiting: boolean;
  error: string;
};

export type FigureRow = {
  id: number;
  name: string;
  age: number;
  place: string; // Ort bzw. „unterwegs nach …“
  home: string;
  doing: string;
  lead: boolean;
  moving: boolean;
  relations: number;
  memories: number;
  skills: { t: string; l: string; n: number; p?: number }[]; // Stufe n 1–3, Fortschritt p 0–100 zur nächsten
};

// kind: 0 Ereignis, 1 Start der Welt, 2 Pause (Pixelia war aus), 3 Speicherstand eingespielt
export type ChronicleRow = { day: number; time: string; kind: number; text: string };

export type WorldState = {
  save?: SaveInfo;
  figures?: FigureRow[];
  chronicle?: ChronicleRow[];
  visits?: number;
  visitsOpen?: number;
};

export const loadWorld = (more = false) => get<WorldState>(more ? '/api/world?mehr=1' : '/api/world');

// Lager (seit 0.5.10 Teil 4b, /api/world?lager=1): Lagerplätze mit Füllstand und Reichweite, Waren im Dorf, laufende
// Projekte. lv: 0 ohne Verbrauch, 1 leer bzw. reicht keinen Tag, 2 knapp, 3 gut, 4 reichlich; h: Reichweite in Stunden
// (−1 = kein Abgang)
export type StoreGood = { t: string; q: number; res: number };
export type StoreRow = { t: string; place: string; kind: string; load: number; cap: number; lv: number; h: number; goods: StoreGood[]; reach: string };
export type GoodRow = { t: string; cat?: string; q: number; res: number; lv: number; h: number; reach: string };
export type ProjectRow = { t: string; place: string; step: number; steps: number; stepT: string; pct: number; wait: string; who: string };
export type LagerState = { lager?: { stores: StoreRow[]; goods: GoodRow[]; projects: ProjectRow[] } };

// Entwicklung: Parameter der Seite gehen an den Mock (?leer=1, ?viele=1)
const devQuery = import.meta.env.DEV ? location.search.replace(/^\?/, '&') : '';
export const loadLager = () => get<LagerState>(`/api/world?lager=1${devQuery}`);

// knapp oder leer: braucht Nachschub
export const worry = (s: StoreRow) => s.lv === 1 || s.lv === 2;
// Leere zuerst, dann was am kürzesten reicht
export const worries = (stores: StoreRow[]) =>
  stores.filter(worry).sort((a, b) => a.lv - b.lv || a.load - b.load || (a.h < 0 ? 1e9 : a.h) - (b.h < 0 ? 1e9 : b.h));
export const ware = (s: StoreRow) => (s.goods.length === 1 ? s.goods[0].t : s.t);
// „Reicht noch 3 Tage“ → „noch 3 Tage“ (kurze Zeilen)
export const shortReach = (s: StoreRow) => (s.load ? s.reach.replace(/^Reicht noch /, 'noch ') : 'Leer');

// „gerade eben“, „vor 3 Min.“, „vor 2 Std.“, „vor 3 Tagen“
export function agoText(s: number): string {
  if (s < 0) return 'noch nicht';
  if (s < 60) return 'gerade eben';
  if (s < 3600) return `vor ${Math.floor(s / 60)} Min.`;
  if (s < 86400) return `vor ${Math.floor(s / 3600)} Std.`;
  const d = Math.floor(s / 86400);
  return `vor ${d} Tag${d === 1 ? '' : 'en'}`;
}

// „5 Min.“, „2 Std. 14 Min.“, „1 Tag 3 Std.“ aus Weltminuten
export function minutesText(m: number): string {
  const d = Math.floor(m / 1440);
  const h = Math.floor((m % 1440) / 60);
  const min = m % 60;
  if (d) return `${d} Tag${d === 1 ? '' : 'e'}${h ? ` ${h} Std.` : ''}`;
  if (h) return `${h} Std.${min ? ` ${min} Min.` : ''}`;
  return `${min} Min.`;
}
