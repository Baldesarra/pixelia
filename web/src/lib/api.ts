// Zugriff auf die JSON-API des Geräts (Eingaben als Formulardaten, wie in firmware/main/web.cpp beschrieben).
// Ist ein PIN gesetzt, gehen ändernde Anfragen mit dem Sitzungs-Token; fehlt es, fragt post() einmal nach dem PIN
// und versucht es danach noch einmal (auth.svelte.ts).
import { auth } from './auth.svelte';

export type Reply = { ok: boolean; msg?: string; auth?: boolean; lockS?: number; [k: string]: unknown };

export async function get<T>(path: string): Promise<T> {
  const r = await fetch(path, { cache: 'no-store' });
  if (!r.ok) throw new Error(`${path}: ${r.status}`);
  return (await r.json()) as T;
}

/** quiet: bei fehlender Anmeldung nicht nach dem PIN fragen (z. B. Farbwahl, die auch nur im Browser gilt).
 *  Ein Blob geht unverändert als Rumpf (z. B. das Testbild für /api/picture). */
export async function post<T = Reply>(
  path: string,
  data: Record<string, string | number> | Blob = {},
  opts: { quiet?: boolean } = {},
): Promise<T> {
  const body = () => (data instanceof Blob ? data : new URLSearchParams(Object.entries(data).map(([k, v]) => [k, String(v)])));
  let r = await fetch(path, { method: 'POST', body: body(), headers: auth.headers() });
  if ((r.status === 401 || r.status === 429) && !opts.quiet) {
    const j = (await r.json()) as Reply;
    if (!j.auth) return j as T;
    if (!(await auth.login())) return { ok: false, msg: 'Abgebrochen: PIN nötig', auth: true } as T;
    r = await fetch(path, { method: 'POST', body: body(), headers: auth.headers() });
  }
  return (await r.json()) as T;
}

// --- Typen der Antworten (siehe web.cpp, web_info.cpp, web_diag.cpp) ----------------------------------------

export type WifiList = {
  saved: string[];
  scan: { ssid: string; rssi: number; ch: number; secure: boolean; saved: boolean }[];
};

export type Partition = {
  label: string;
  type: string;
  offset: number;
  size: number;
  v?: string;
  running?: boolean;
  nextBoot?: boolean;
};

export type SystemInfo = {
  board: string;
  panel: { type: string; w: number; h: number; renderW: number; renderH: number; hz: number; pclk: number; isr: number; desync: number; mode: string };
  touch: { ok: boolean; chip?: string; addr?: string; fw?: string; w?: number; h?: number; cfg?: number; touches?: number };
  chip: { model: string; rev: number; cores: number; mhz: number; temp: number | null; wifi: boolean; ble: boolean };
  mac: { wifi: string; ap: string; bt: string };
  flash: { mb: number; id: string; mode: string; freq: string };
  psram: { kb: number; mode: string; mhz: number };
  heap: { intTotal: number; psramTotal: number };
  pins: { de: number; vsync: number; hsync: number; pclk: number; data: number[]; backlight: number; sda: number; scl: number; touchRst: number };
  build: { v: string; date: string; time: string; idf: string; compiler: string };
  appSize: number;
  appPart: number;
  partitions: Partition[];
  nvs: { used: number; free: number; total: number };
  net: { host: string; mdns: string; ip?: string; mask?: string; gw?: string; dns?: string; bssid?: string; phy?: string; connectedS?: number };
  time: { valid: boolean; syncs: number; epoch: number; tz: string };
};

export type ClientList = {
  epoch: number;
  you: string;
  list: {
    id: string;
    name: string;
    agent: string;
    ip: string;
    page: string;
    online: boolean;
    first: number;
    last: number;
    agoS: number | null;
    visits: number;
  }[];
  ap: { mac: string; ip: string; rssi: number }[];
};

export type ResetList = {
  boot: number;
  entries: {
    boot: number;
    v: string;
    part: string;
    reason: number;
    reasonText: string;
    time: string;
    timeLate: boolean;
    prevUptime: number | null;
    prevTime: string;
    trial: boolean;
    rollback: boolean;
    crash: { task: string; pc: string } | null;
    note: string;
  }[];
};

/** GET /api/coredump/summary (diag::crash_json); die Felder gibt es nur mit present */
export type CrashSummary = {
  present: boolean;
  valid?: boolean;
  size?: number;
  addr?: string;
  task?: string;
  pc?: string;
  reason?: string;
  excCause?: number;
  excVaddr?: string;
  backtrace?: string[];
  btCorrupted?: boolean;
  elfSha?: string;
  version?: string;
  part?: string;
};

export type LogLine = { n: number; b: number; t: number; u: number; l: string; m: string; cut?: boolean };

/** Sicherung der Einstellungen (GET /api/backup): nie mit WLAN-Passwörtern oder PIN */
export type Backup = {
  pixelia: 'settings';
  format: number;
  v: string;
  date: string;
  host: string;
  set: { rot: number; bright: number; lp: number; ui: string };
  names: { id: string; name: string }[];
  wifi: string[];
  pin: boolean;
};

/** GET /api/update (seit 0.4.1): Update von GitHub, neuere Versionen mit Release-Notes (Markdown) */
export type GhRelease = { v: string; pre: boolean; date: string; size: number; notes: string };
export type GhState = {
  ch: 'stable' | 'preview';
  job: '' | 'check' | 'install';
  err: string;
  net: boolean;
  checkedS: number; // Sekunden seit der letzten Prüfung, -1 = noch nie
  checkedCh: 'stable' | 'preview';
  latest: string; // neueste Version im Kanal (auch wenn nicht neuer)
  rel: GhRelease[]; // neuer als installiert, neueste zuerst
  rate: { left: number; limit: number; resetS: number };
  // seit 0.4.2
  cancelled?: boolean; // letzter Download wurde abgebrochen
  auto?: boolean; // Prüfung im Hintergrund (einmal täglich)
  lastAuto?: boolean; // letzte Prüfung kam aus dem Hintergrund
  lastT?: number; // Unix-Zeit der letzten Prüfung (auch über Neustarts), 0 = unbekannt
  avail?: string;
  seq?: number;
  old?: { v: string; pre: boolean; date: string; size: number }[]; // ältere Versionen ab 0.4.0, neueste zuerst
};

/** GET /api/exp (seit 0.4.2): Seite Experimentell */
export type PanelValues = {
  pclk: number; // kHz, ohne Stromsparmodus
  hp: number;
  hb: number;
  hf: number;
  vp: number;
  vb: number;
  vf: number;
  hlow: boolean;
  vlow: boolean;
  neg: boolean;
  eco: boolean;
  hz: number; // tatsächliche Bildwiederholung (mit Stromsparmodus)
  swap: number; // Panel-Bilder pro Engine-Bild
};
export type ExpState = {
  on: boolean;
  safeBoot: boolean; // Fluchtweg hat gegriffen: Standardwerte trotz gespeicherter Werte
  trialS: number; // Sekunden bis zum Rückfall, -1 = keine Probe
  trialNeed: number;
  panel: PanelValues;
  saved: PanelValues;
  std: PanelValues;
  mode: 'black' | 'bars' | 'frame' | 'motion' | 'touch' | 'engine';
  testS: number;
  pic?: boolean; // eigenes Testbild auf dem Display (seit 0.5.0)
  // Orte der Bühne (seit 0.5.0): Liste, sichtbarer Ort, Übergang, Rundgang in s (0 = aus), Figuren (0 = Standard)
  places?: { id: string; title: string }[];
  place?: number;
  trans?: 'schnitt' | 'ueberblendung' | 'raster' | 'abblenden';
  tour?: number;
  figures?: number;
  touch:{ ok: boolean; down: boolean; x: number; y: number; corners: number; n: number };
};

/** GET /api/perf (seit 0.2.2): Leistung, Messung alle 2 s; Task-Liste nur mit ?tasks=1 */
export type PerfData = {
  uptime: number;
  at: number;
  mode: string;
  lcd: { hz: number; pclk: number; isr: number; isrMaxUs: number; desync: number };
  engine: { fps: number; render: number; renderMax: number; logic: number; ticks: number[]; sprites: number; buf: string } | null;
  cpu: [number, number];
  heap: { ram: number; ramMin: number; ramBlk: number; dma: number; psram: number; psramMin: number; psramBlk: number };
  wifi: { state: string; rssi: number; ch: number };
  taskWindowMs?: number;
  taskQueryUs?: number;
  tasks: { name: string; core: number; prio: number; stackFree: number; cpu: number; state: string }[] | null;
};
