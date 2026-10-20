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

/** quiet: bei fehlender Anmeldung nicht nach dem PIN fragen (z. B. Farbwahl, die auch nur im Browser gilt) */
export async function post<T = Reply>(
  path: string,
  data: Record<string, string | number> = {},
  opts: { quiet?: boolean } = {},
): Promise<T> {
  const body = () => new URLSearchParams(Object.entries(data).map(([k, v]) => [k, String(v)]));
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
