// Versionsverlauf: Einträge aus CHANGELOG.json, eingebettet in die Firmware (tools/release_notes.py --embed).
// Vom Gerät über GET /api/changelog, aus einer Update-Datei über den Block mit festem Marker in der .bin.
// Seit 0.5.4 enthält die Firmware nur die Kurzfassung (k); die Details stehen in den Release-Notes auf GitHub.

import { get } from './api';

export type ChangeEntry = {
  v: string;
  d: string; // Datum JJJJ-MM-TT, leer = noch nicht veröffentlicht
  t: string;
  s: string;
  k?: Short; // Kurzfassung in Alltagssprache (seit 0.5.4), die Oberfläche zeigt nur sie
  n?: string[];
  i?: string[];
  f?: string[];
  r?: string[];
  g?: string[];
  h?: string[];
  pr?: number[];
};

/** Kurzfassung: Unterzeile und die Abschnitte Neu, Verbessert, Behoben, Entfernt mit „Stichwort: Satz“ */
export type Short = { s?: string; n?: string[]; i?: string[]; f?: string[]; r?: string[] };

export const SECTIONS = [
  { key: 'n', label: 'Neu', icon: 'add' },
  { key: 'i', label: 'Verbessert', icon: 'arrow_upward' },
  { key: 'f', label: 'Behoben', icon: 'bug_report' },
  { key: 'r', label: 'Entfernt', icon: 'remove' },
  { key: 'g', label: 'Geprüft', icon: 'check_circle' },
  { key: 'h', label: 'Hinweise', icon: 'info' },
] as const;

export const REPO_URL = 'https://github.com/Baldesarra/pixelia';
/** Abschnitt der Release-Notes mit der Kurzfassung (tools/release_notes.py) */
export const SHORT_TITLE = 'Kurz gesagt';

/** Release einer Version auf GitHub; unveröffentlichte Stände → Liste aller Releases */
export const releaseUrl = (e: Pick<ChangeEntry, 'v' | 'd'>) => (e.d ? `${REPO_URL}/releases/tag/v${e.v}` : `${REPO_URL}/releases`);

let cached: Promise<ChangeEntry[]> | null = null;

/** Verlauf der installierten Firmware (einmal pro Seitenaufruf geladen) */
export function installedLog(): Promise<ChangeEntry[]> {
  cached ??= get<ChangeEntry[]>('/api/changelog').catch((e) => ((cached = null), Promise.reject(e)));
  return cached;
}

// --- Versionen ------------------------------------------------------------------------------------------------

/** Firmware-Version (git describe): „0.4.0“, „0.4.0-beta.1“, nach weiteren Commits „0.4.0-3-gabc1234[-dirty]“ */
export function parseFw(v: string): { base: string; dev: boolean } {
  const m = /^(\d+\.\d+\.\d+(?:-beta\.\d+)?)(-\d+-g[0-9a-f]+)?(-dirty)?/.exec(v);
  if (!m) return { base: v, dev: true };
  return { base: m[1], dev: !!(m[2] || m[3]) };
}

/** Vergleich wie tools/release_notes.py: Vorabversion X.Y.Z-beta.N kommt vor X.Y.Z */
export function cmpVersion(a: string, b: string): number {
  const p = (v: string) => {
    const m = /^(\d+)\.(\d+)\.(\d+)(?:-(.+))?$/.exec(v);
    return m ? [+m[1], +m[2], +m[3], m[4] ? 0 : 1, m[4] ?? ''] : [0, 0, 0, 0, v];
  };
  const x = p(a);
  const y = p(b);
  for (let i = 0; i < 4; i++) if (x[i] !== y[i]) return (x[i] as number) - (y[i] as number);
  return String(x[4]).localeCompare(String(y[4]), 'en', { numeric: true });
}

/** Eintrag, zu dem die laufende Firmware gehört (bei Entwicklungsständen der oberste, noch offene Eintrag) */
export function installedEntry(log: ChangeEntry[], fw: string): string {
  const { base, dev } = parseFw(fw);
  if (dev && log[0] && !log[0].d && cmpVersion(log[0].v, base) > 0) return log[0].v;
  return log.some((e) => e.v === base) ? base : '';
}

// --- Block in der .bin ----------------------------------------------------------------------------------------

const MAGIC = new TextEncoder().encode('PIXELIA:CHLOG:v1');
const HEAD = 24;

let crcTable: Uint32Array | null = null;
function crc32(u8: Uint8Array): number {
  if (!crcTable) {
    crcTable = new Uint32Array(256);
    for (let n = 0; n < 256; n++) {
      let c = n;
      for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1;
      crcTable[n] = c >>> 0;
    }
  }
  let c = 0xffffffff;
  for (let i = 0; i < u8.length; i++) c = crcTable[(c ^ u8[i]) & 0xff] ^ (c >>> 8);
  return (c ^ 0xffffffff) >>> 0;
}

export type BinLog = { ok: true; log: ChangeEntry[] } | { ok: false; why: 'none' | 'browser' | 'broken' };

/** Verlauf aus einer Update-Datei lesen: Marker suchen, Länge und CRC prüfen, gzip im Browser entpacken */
export async function logFromBin(file: Blob): Promise<BinLog> {
  if (typeof DecompressionStream === 'undefined') return { ok: false, why: 'browser' };
  const u8 = new Uint8Array(await file.arrayBuffer());
  const view = new DataView(u8.buffer);
  for (let at = u8.indexOf(MAGIC[0]); at >= 0 && at + HEAD <= u8.length; at = u8.indexOf(MAGIC[0], at + 1)) {
    let hit = true;
    for (let k = 1; k < MAGIC.length && hit; k++) hit = u8[at + k] === MAGIC[k];
    if (!hit) continue;
    const len = view.getUint32(at + 16, true);
    if (at + HEAD + len > u8.length) continue;
    const gz = u8.subarray(at + HEAD, at + HEAD + len);
    if (crc32(gz) !== view.getUint32(at + 20, true)) continue;
    try {
      const stream = new Blob([gz]).stream().pipeThrough(new DecompressionStream('gzip'));
      const log = (await new Response(stream).json()) as ChangeEntry[];
      if (Array.isArray(log)) return { ok: true, log };
    } catch {
      /* weitersuchen */
    }
    return { ok: false, why: 'broken' };
  }
  return { ok: false, why: 'none' };
}

// --- Release-Notes von GitHub ---------------------------------------------------------------------------------

/**
 * Eintrag aus den Release-Notes eines GitHub-Releases (Markdown von tools/release_notes.py: „## Pixelia X – Titel“,
 * „*Untertitel* · Datum“, „### Kurz gesagt“, „### Neu“ … mit Stichpunkten, „### Pull Requests“). So zeigt die Vorschau die Version
 * auf GitHub mit denselben Abschnitten wie der Versionsverlauf, ohne die .bin dafür zu laden. Fremder Text landet
 * unter „Hinweise“.
 */
export function entryFromNotes(v: string, md: string, date: string): ChangeEntry {
  const e: ChangeEntry = { v, d: date, t: '', s: '' };
  const label = new Map<string, string>(SECTIONS.map((x): [string, string] => [x.label, x.key]));
  let key: string | null = 'h';
  let short: Short | null = null; // innerhalb von „### Kurz gesagt“ (seit 0.5.4)
  let shortKey = 'n';
  const push = (o: Record<string, unknown>, k: string, item: string) => void ((o[k] ??= []) as string[]).push(item);
  for (const raw of md.split('\n')) {
    const line = raw.trim();
    if (!line) continue;
    let m: RegExpExecArray | null;
    if ((m = /^##\s+(.*)$/.exec(line))) {
      e.t = m[1].replace(/^Pixelia\s+\S+\s+[–-]\s+/, '');
    } else if ((m = /^\*(.+)\*\s+·\s+(\d{4}-\d{2}-\d{2})$/.exec(line))) {
      e.s = m[1];
      e.d = m[2];
    } else if ((m = /^###\s+(.*)$/.exec(line))) {
      short = m[1] === SHORT_TITLE ? (e.k = {}) : null;
      key = m[1] === 'Pull Requests' ? 'pr' : m[1] === 'Alle Details' ? 'h' : (label.get(m[1]) ?? 'h');
    } else if (short && (m = /^\*\*(.+)\*\*$/.exec(line))) {
      shortKey = label.get(m[1]) ?? 'n';
    } else if (short && (m = /^\*(.+)\*$/.exec(line))) {
      short.s = m[1];
    } else if (short) {
      push(short as Record<string, unknown>, shortKey, line.replace(/^[-*]\s+/, ''));
    } else if (key === 'pr') {
      e.pr = [...line.matchAll(/#(\d+)/g)].map((x) => +x[1]);
    } else if (key) {
      push(e as Record<string, unknown>, key, line.replace(/^[-*]\s+/, ''));
    }
  }
  return e;
}

/** Datum JJJJ-MM-TT → TT.MM.JJJJ */
export const dateDe = (d: string) => d.split('-').reverse().join('.');
