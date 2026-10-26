// Vite: feste Dateinamen (die Firmware bettet sie ein), keine CDNs, Icons nur die benutzten.
// Entwicklung am echten Gerät: npm run dev  (Ziel ändern: PIXELIA_TARGET=http://192.168.1.50 npm run dev)
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { defineConfig, type Plugin } from 'vite';
import { svelte } from '@sveltejs/vite-plugin-svelte';
import { ICONS } from './src/lib/m3/icon-names.ts';

const target = process.env.PIXELIA_TARGET ?? 'http://pixelia';

// Pfad eines Icons verkleinern (seit 0.5.4, spart ~6 KB gzip): Raster 960 → 240 (viewBox in Icon.svelte), ganze
// Zahlen. Relative Befehle runden ohne Drift: jedes Delta zielt auf den gerundeten Zielpunkt, nicht auf den exakten.
// Fehler höchstens 1/2 Einheit = 0,05 px bei 24 px.
const ICON_SCALE = 4;
function compactPath(d: string): string {
  const ARGS: Record<string, number> = { M: 2, L: 2, H: 1, V: 1, C: 6, S: 4, Q: 4, T: 2, A: 7, Z: 0 };
  const tokens = d.match(/[a-zA-Z]|-?(?:\d+\.?\d*|\.\d+)(?:e-?\d+)?/g) ?? [];
  let x = 0, y = 0, sx = 0, sy = 0; // exakter Punkt und Start des Teilpfads
  let rx = 0, ry = 0, rsx = 0, rsy = 0; // gerundete Gegenstücke
  const R = (v: number) => Math.round(v / ICON_SCALE);
  let out = '';
  let i = 0;
  let cmd = '';
  const num = (v: number) => {
    const t = String(v);
    out += out && /[\d.]$/.test(out) && !t.startsWith('-') ? ' ' + t : t;
  };
  while (i < tokens.length) {
    if (/[a-zA-Z]/.test(tokens[i])) {
      cmd = tokens[i++];
      out += cmd;
      if (cmd === 'z' || cmd === 'Z') [x, y, rx, ry] = [sx, sy, rsx, rsy];
      if (ARGS[cmd.toUpperCase()] === 0) continue;
    } else if (cmd === 'm') cmd = 'l'; else if (cmd === 'M') cmd = 'L';
    const up = cmd.toUpperCase();
    const rel = cmd !== up;
    const a = tokens.slice(i, i + ARGS[up]).map(Number);
    i += ARGS[up];
    const ax = (v: number) => (rel ? x + v : v); // absolut
    const ay = (v: number) => (rel ? y + v : v);
    const ox = (v: number) => (rel ? R(v) - rx : R(v)); // Ausgabe
    const oy = (v: number) => (rel ? R(v) - ry : R(v));
    let nx = x, ny = y;
    if (up === 'H') { nx = ax(a[0]); num(ox(nx)); }
    else if (up === 'V') { ny = ay(a[0]); num(oy(ny)); }
    else if (up === 'A') {
      nx = ax(a[5]); ny = ay(a[6]);
      [R(a[0]), R(a[1]), a[2], a[3], a[4], ox(nx), oy(ny)].forEach(num);
    } else {
      for (let k = 0; k < a.length; k += 2) { num(ox(ax(a[k]))); num(oy(ay(a[k + 1]))); }
      nx = ax(a[a.length - 2]); ny = ay(a[a.length - 1]);
    }
    [x, y, rx, ry] = [nx, ny, R(nx), R(ny)];
    if (up === 'M') [sx, sy, rsx, rsy] = [x, y, rx, ry];
  }
  return out;
}

// Material Symbols (Rounded, 400): aus den SVG-Dateien nur den Pfad übernehmen, nur für die Namen aus icon-names.ts
function materialSymbols(): Plugin {
  const id = 'virtual:pixelia-icons';
  return {
    name: 'pixelia-icons',
    resolveId: (s) => (s === id ? '\0' + id : undefined),
    load(s) {
      if (s !== '\0' + id) return;
      const out: Record<string, string> = {};
      for (const name of ICONS) {
        const file = fileURLToPath(
          new URL(`./node_modules/@material-symbols/svg-400/rounded/${name}.svg`, import.meta.url),
        );
        const d = /<path d="([^"]+)"/.exec(readFileSync(file, 'utf8'));
        if (!d) throw new Error(`Icon ${name}: kein Pfad gefunden`);
        out[name] = compactPath(d[1]);
      }
      return `export default ${JSON.stringify(out)};`;
    },
  };
}

// Ohne Gerät entwickeln: npm run mock liefert die API mit Beispielwerten (kein WebSocket,
// die Oberfläche fällt dann auf Abfrage zurück). PIXELIA_MOCK=setup zeigt den Setup-Modus.
function mockDevice(): Plugin {
  const t0 = Date.now();
  const setup = process.env.PIXELIA_MOCK === 'setup';
  const json = (res: import('node:http').ServerResponse, v: unknown) => {
    res.setHeader('Content-Type', 'application/json');
    res.end(JSON.stringify(v));
  };
  const now = () => Math.floor(Date.now() / 1000);
  const logLine = (n: number, l: string, m: string) => ({ n, b: 42, t: now() - (300 - n), u: n * 900, l, m });
  let logN = 300;
  // Einstellungen und Schutz wie auf dem Gerät (auth.cpp): PIXELIA_MOCK_PIN=1234 startet mit PIN
  const set = { rot: 0, bright: 80, lp: 3000, ui: '', clk: true, clkCorner: 0, clkSec: false, camHome: 0, camFig: 0, camPlace: 1, camRet: 600, night: true, nightBr: 25, nightNow: false, nightFrom: 22, nightTo: 6 };
  // Weltkalender (seit 0.5.7, /api/world)
  let seasonLen = 7;
  // Kamera (seit 0.5.6): Orte und Figuren wie die Testorte
  const cam = { mode: 0, place: 1, target: 1000 };
  const camPlaces = [
    { id: 'scrolltest', title: 'Landschaft' },
    { id: 'testort-wiese', title: 'Testort Wiese' },
    { id: 'testort-raum', title: 'Testort Raum' },
    { id: 'testort-platz', title: 'Testort Platz' },
  ];
  const camFigs = [
    { id: 1000, name: 'Begleiterin', place: 1, lead: true },
    ...[1001, 1002, 1003, 1004, 1005].map((id) => ({ id, name: 'Testfigur ' + (id - 900), place: 1, lead: false })),
    ...[2000, 2001, 2002].map((id) => ({ id, name: 'Testfigur ' + (id - 1800), place: 2, lead: false })),
    ...[3000, 3001, 3002, 3003].map((id) => ({ id, name: 'Testfigur ' + (id - 2700), place: 3, lead: false })),
  ];
  const names: Record<string, string> = { a1b2c3d4e5f6: 'Handy Wohnzimmer' };
  let pin = process.env.PIXELIA_MOCK_PIN ?? '';
  // Absturzbericht vorhanden (PIXELIA_MOCK_CRASH=0: keiner); „Bericht löschen“ entfernt ihn
  let crashed = process.env.PIXELIA_MOCK_CRASH !== '0';
  let fails = 0;
  let lockedUntil = 0;
  const tokens = new Set<string>();
  // Update von GitHub (0.4.1): gespielte Releases aus CHANGELOG.json (Release-Notes wie tools/release_notes.py), dazu
  // eine Vorabversion des offenen Eintrags. PIXELIA_MOCK_GH=limit spielt das Limit der API, =fail einen Abbruch.
  const ghMode = process.env.PIXELIA_MOCK_GH ?? '';
  let installedV = process.env.PIXELIA_MOCK_V ?? '0.3.2'; // z. B. 0.4.1: dann gibt es ältere Versionen ab 0.4.0
  const ota = { busy: false, src: 'upload', bytes: 0, total: 0, newV: '', err: '', trial: false, trialS: 0, trialNeed: 60, trialNote: '', passed: true };
  const gh = { ch: 'stable', job: '', err: '', checkedAt: 0, checkedCh: 'stable', latest: '', rel: [] as unknown[], left: 60 };
  let downUntil = 0; // simulierter Neustart
  // 0.4.2: Hintergrundprüfung, Abbrechen, ältere Versionen; Experimentell (PIXELIA_MOCK_EXP=1 startet eingeschaltet)
  Object.assign(gh, { auto: true, lastAuto: false, lastT: 0, avail: '', seq: 0, cancelled: false, old: [] as unknown[] });
  let ghCancel = false;
  const std = { pclk: 24600, hp: 4, hb: 8, hf: 8, vp: 4, vb: 8, vf: 8, hlow: true, vlow: true, neg: true, eco: false, hz: 60, swap: 2 };
  const exp = {
    on: process.env.PIXELIA_MOCK_EXP === '1', safeBoot: false, trialS: -1, trialNeed: 15,
    panel: { ...std }, saved: { ...std }, std, mode: 'engine', testS: -1, pic: false,
    places: [{ id: 'scrolltest', title: 'Landschaft' }, { id: 'testort-wiese', title: 'Testort Wiese' }, { id: 'testort-raum', title: 'Testort Raum' }, { id: 'testort-platz', title: 'Testort Platz' }],
    place: 0, trans: 'abblenden', tour: 0, figures: 0, follow: true, inspect: false, minute: 720, speed: 1, real: true, weather: 'sonne', weatherPct: 100, innen: true, particles: 0,
    touch: { ok: true, down: false, x: -1, y: -1, corners: 0, n: 312 },
  };
  let prevPanel = { ...std };
  let expTimer: ReturnType<typeof setInterval> | undefined;
  const panelOf = (f: URLSearchParams) => {
    const p = { ...exp.panel } as Record<string, number | boolean>;
    for (const k of ['pclk', 'hp', 'hb', 'hf', 'vp', 'vb', 'vf']) if (f.has(k)) p[k] = Number(f.get(k));
    for (const k of ['hlow', 'vlow', 'neg', 'eco']) if (f.has(k)) p[k] = f.get(k) === '1';
    const ht = 800 + (p.hp as number) + (p.hb as number) + (p.hf as number);
    const vt = 480 + (p.vp as number) + (p.vb as number) + (p.vf as number);
    p.hz = Math.round(((p.pclk as number) * 1000) / (ht * vt) / (p.eco ? 2 : 1) * 100) / 100;
    p.swap = Math.max(1, Math.round((p.hz as number) / 30));
    return p as typeof std;
  };
  // Takt für Probe, Testbild und Touch-Test (Finger wandert über das Display)
  const expTick = () => {
    if (exp.trialS > 0 && --exp.trialS === 0) Object.assign(exp, { trialS: -1, panel: { ...prevPanel } });
    if (exp.testS > 0 && --exp.testS === 0) Object.assign(exp, { testS: -1, mode: 'engine' });
    if (exp.mode === 'touch') {
      const t = Date.now() / 1000;
      Object.assign(exp.touch, { down: Math.sin(t * 2) > -0.3, x: Math.round(400 + 360 * Math.sin(t * 0.7)), y: Math.round(240 + 200 * Math.cos(t * 1.1)) });
      if (exp.touch.x < 100 && exp.touch.y < 100) exp.touch.corners |= 1;
      if (exp.touch.x >= 700 && exp.touch.y >= 380) exp.touch.corners |= 8;
    }
    if (exp.trialS < 0 && exp.testS < 0) (clearInterval(expTimer), (expTimer = undefined));
  };
  const expRun = () => (expTimer ??= setInterval(expTick, 1000));
  // Vorschaubild wie /api/screen: Himmel mit Verlauf, Sonne, Hügel, Wolke; PackBits wie web_exp.cpp
  const packbits = (src: Uint8Array) => {
    const out: number[] = [];
    let i = 0;
    while (i < src.length) {
      let run = 1;
      while (i + run < src.length && run < 128 && src[i + run] === src[i]) run++;
      if (run >= 3) {
        out.push(257 - run, src[i]);
        i += run;
        continue;
      }
      let lit = 0;
      while (i + lit < src.length && lit < 128) {
        if (i + lit + 2 < src.length && src[i + lit] === src[i + lit + 1] && src[i + lit] === src[i + lit + 2]) break;
        lit++;
      }
      out.push(lit - 1, ...src.subarray(i, i + lit));
      i += lit;
    }
    return Buffer.from(out);
  };
  let frameNo = 0;
  // 0.5.0: eigenes Testbild (POST /api/picture: 96 000 Indizes + Palette 240 × RGB), erscheint dann in /api/screen
  let picture: Buffer | null = null;
  const screen = () => {
    const w = 400, h = 240, px = new Uint8Array(w * h), pal = Buffer.alloc(768);
    if (exp.pic && picture) {
      px.set(picture.subarray(0, w * h));
      picture.copy(pal, 0, w * h);
      const head = Buffer.alloc(16);
      head.write('PXS1');
      head.writeUInt16LE(w, 4);
      head.writeUInt16LE(h, 6);
      head.writeUInt32LE(++frameNo, 8);
      head[12] = 1;
      return Buffer.concat([head, pal, packbits(px)]);
    }
    for (let i = 0; i < 64; i++) pal.set([40 + i, 90 + i, 160 + i * 1.4], i * 3); // Himmel
    pal.set([255, 214, 90], 64 * 3); // Sonne
    pal.set([250, 250, 255], 65 * 3); // Wolke
    pal.set([70, 140, 60], 66 * 3); // Hügel
    pal.set([50, 110, 45], 67 * 3);
    const t = Date.now() / 1000, sx = 80 + ((t * 8) % 260), cx = (t * 14) % 480 - 80;
    for (let y = 0; y < h; y++)
      for (let x = 0; x < w; x++) {
        let c = Math.min(63, Math.floor(y / 3));
        if ((x - sx) ** 2 + (y - 60) ** 2 < 400) c = 64;
        if (Math.abs(x - cx) < 40 && Math.abs(y - 50) < 8 + Math.cos((x - cx) / 13) * 5) c = 65;
        const hill = 170 + 18 * Math.sin(x / 37) + 9 * Math.sin(x / 11);
        if (y > hill) c = y > hill + 30 ? 67 : 66;
        px[y * w + x] = c;
      }
    const head = Buffer.alloc(16);
    head.write('PXS1');
    head.writeUInt16LE(w, 4);
    head.writeUInt16LE(h, 6);
    head.writeUInt32LE(++frameNo, 8);
    head[12] = 1;
    return Buffer.concat([head, pal, packbits(px)]);
  };
  const cmpV = (a: string, b: string) => {
    const p = (v: string) => { const m = /^(\d+)\.(\d+)\.(\d+)(?:-beta\.(\d+))?/.exec(v) ?? []; return [+m[1], +m[2], +m[3], m[4] ? 0 : 1, +(m[4] ?? 0)]; };
    const x = p(a), y = p(b);
    for (let i = 0; i < 5; i++) if (x[i] !== y[i]) return x[i] - y[i];
    return 0;
  };
  type Entry = { v: string; d: string; t: string; s: string; pr?: number[] } & Record<string, unknown>;
  const SECT: [string, string][] = [['n', 'Neu'], ['i', 'Verbessert'], ['f', 'Behoben'], ['r', 'Entfernt'], ['g', 'Geprüft'], ['h', 'Hinweise']];
  const notesMd = (e: Entry) => {
    const l = [`## Pixelia ${e.v} – ${e.t}`, '', `*${e.s}* · ${e.d}`, ''];
    // Kurzfassung wie tools/release_notes.py (seit 0.5.4)
    const k = e.k as Record<string, string | string[]> | undefined;
    if (k) {
      l.push('### Kurz gesagt', '', `*${k.s}*`, '');
      for (const [key, name] of SECT.slice(0, 4)) if (k[key]?.length) l.push(`**${name}**`, '', ...(k[key] as string[]).map((x) => `- ${x}`), '');
      l.push('### Alle Details', '');
    }
    for (const [k, name] of SECT) {
      const items = (e[k] as string[] | undefined) ?? [];
      if (items.length) l.push(`### ${name}`, ...items.map((x) => `- ${x}`), '');
    }
    if (e.pr?.length) l.push('### Pull Requests', e.pr.map((n) => `[#${n}](https://github.com/Baldesarra/pixelia/pull/${n})`).join(', '));
    return l.join('\n').trim();
  };
  const ghCheck = () => {
    const log = JSON.parse(readFileSync(fileURLToPath(new URL('../CHANGELOG.json', import.meta.url)), 'utf8')) as Entry[];
    const today = new Date().toISOString().slice(0, 10);
    const all = log.filter((e) => e.d).map((e) => ({ v: e.v, pre: e.v.includes('-'), date: e.d, size: 1_556_000, notes: notesMd(e) }));
    if (!log[0].d && !all.some((r) => r.v.startsWith(`${log[0].v}-`))) {
      const v = `${log[0].v}-beta.1`;
      all.unshift({ v, pre: true, date: today, size: 1_559_000, notes: notesMd({ ...log[0], v, t: log[0].t || 'Update von GitHub', s: log[0].s || 'Vorabversion zum Ausprobieren', d: today }) });
    }
    const inCh = all.filter((r) => gh.ch === 'preview' || !r.pre).sort((a, b) => cmpV(b.v, a.v));
    gh.latest = inCh[0]?.v ?? '';
    gh.rel = inCh.filter((r) => cmpV(r.v, installedV) > 0);
    (gh as Record<string, unknown>).old = inCh
      .filter((r) => cmpV(r.v, installedV) < 0 && cmpV(r.v, '0.4.0') >= 0)
      .map(({ v, pre, date, size }) => ({ v, pre, date, size }));
    gh.checkedAt = Date.now();
    gh.checkedCh = gh.ch;
    gh.left = Math.max(0, gh.left - 1);
  };
  type Req = import('node:http').IncomingMessage;
  type Res = import('node:http').ServerResponse;
  const form = (req: Req) =>
    new Promise<URLSearchParams>((resolve) => {
      let s = '';
      req.on('data', (c) => (s += c));
      req.on('end', () => resolve(new URLSearchParams(s)));
    });
  const lockS = () => Math.max(0, Math.ceil((lockedUntil - Date.now()) / 1000));
  const checkPin = (p: string | null) => {
    if (!pin) return true;
    if (p === null || lockS() > 0) return false;
    if (p === pin) return ((fails = 0), true);
    if (++fails >= 5) lockedUntil = Date.now() + Math.min(30_000 << Math.min(fails - 5, 5), 900_000);
    return false;
  };
  const authorized = (req: Req, f?: URLSearchParams) => {
    if (!pin) return true;
    const t = req.headers['x-pixelia-token'];
    if (typeof t === 'string' && tokens.has(t)) return true;
    const p = req.headers['x-pixelia-pin'];
    return checkPin(typeof p === 'string' ? p : (f?.get('pin') ?? null));
  };
  const deny = (res: Res) => {
    res.statusCode = lockS() ? 429 : 401;
    json(res, { ok: false, auth: true, msg: lockS() ? 'Zu viele Fehlversuche, bitte warten' : 'PIN nötig', lockS: lockS(), left: Math.max(1, 5 - fails) });
  };
  const newToken = () => {
    const t = [...Array(32)].map(() => Math.floor(Math.random() * 16).toString(16)).join('');
    tokens.add(t);
    return t;
  };
  return {
    name: 'pixelia-mock',
    apply: 'serve',
    configureServer(server) {
      if (server.config.mode !== 'mock') return;
      // Kopflose Screenshot-Läufe kappen Verbindungen hart: ohne Handler beendet ECONNRESET den ganzen Mock
      server.httpServer?.on('connection', (socket) => socket.on('error', () => {}));
      server.middlewares.use('/api/auth', (req, res) => {
        const t = req.headers['x-pixelia-token'];
        json(res, { pin: !!pin, ok: !pin || (typeof t === 'string' && tokens.has(t)), lockS: lockS(), left: Math.max(1, 5 - fails) });
      });
      server.middlewares.use('/api/login', async (req, res) => {
        const f = await form(req);
        if (checkPin(f.get('pin'))) return json(res, { ok: true, msg: 'Angemeldet', token: newToken() });
        res.statusCode = lockS() ? 429 : 401;
        const left = Math.max(1, 5 - fails);
        json(res, {
          ok: false, auth: true, lockS: lockS(), left,
          msg: lockS() ? `Zu viele Fehlversuche. Nächster Versuch in ${lockS()} s` : `PIN falsch, noch ${left} Versuch${left === 1 ? '' : 'e'}`,
        });
      });
      server.middlewares.use('/api/logout', (req, res) => {
        tokens.delete(String(req.headers['x-pixelia-token']));
        json(res, { ok: true, msg: 'Abgemeldet' });
      });
      server.middlewares.use('/api/pin', async (req, res) => {
        const f = await form(req);
        if (!authorized(req, f)) return deny(res);
        const p = f.get('new') ?? '';
        if (p && (p.length < 4 || p.length > 32)) return json(res, { ok: false, msg: 'Der PIN braucht 4 bis 32 Zeichen' });
        pin = p;
        tokens.clear();
        json(res, { ok: true, msg: p ? 'PIN gespeichert' : 'PIN ausgeschaltet', ...(p ? { token: newToken() } : {}) });
      });
      server.middlewares.use('/api/settings', async (req, res) => {
        const f = await form(req);
        if (!authorized(req, f)) return deny(res);
        if (f.has('rot')) set.rot = f.get('rot') === '180' ? 180 : 0;
        if (f.has('bright')) set.bright = Number(f.get('bright'));
        if (f.has('lp')) set.lp = Number(f.get('lp'));
        if (f.has('ui')) set.ui = f.get('ui') ?? '';
        if (f.has('clk')) set.clk = f.get('clk') === '1';
        if (f.has('clkCorner')) set.clkCorner = Number(f.get('clkCorner'));
        if (f.has('clkSec')) set.clkSec = f.get('clkSec') === '1';
        for (const k of ['camHome', 'camFig', 'camPlace', 'camRet', 'nightBr', 'nightFrom', 'nightTo'] as const) if (f.has(k)) set[k] = Number(f.get(k));
        if (f.has('night')) set.night = f.get('night') === '1';
        if (f.has('seasonLen')) seasonLen = Math.min(28, Math.max(1, Number(f.get('seasonLen'))));
        if (['camHome', 'camFig', 'camPlace'].some((k) => f.has(k))) cam.mode = 0;
        json(res, { ok: true, msg: 'Gespeichert' });
      });
      server.middlewares.use('/api/world', (_req, res) => {
        json(res, {
          valid: true, started: true, test: false, start: '2026-10-26', day: 17, year: 1, season: 'herbst', seasonTitle: 'Herbst',
          dayInSeason: 3, seasonDays: seasonLen, evening: false, seasonFrom: '2026-11-09', seasonTo: '2026-11-15', next: 'Winter',
          nextFrom: '2026-11-16', sunrise: '07:41', sunset: '17:49', dayPart: 'Nachmittag',
          moon: { title: 'zunehmende Sichel', illum: 6, phase: 60, waxing: true, up: true, visible: false },
          firsts: ['Erstes Gewitter', 'Erster Herbstnebel', 'Erster Vollmond des Frühlings'],
          seasonLen, weather: { kind: 'regen', title: 'Regen', pct: 60, override: false },
          forecast: [
            { slot: 'abends', kind: 'bewoelkt', title: 'Bewölkt', pct: 80 },
            { slot: 'nachts', kind: 'nebel', title: 'Nebel', pct: 70 },
            { slot: 'morgens', kind: 'sonne', title: 'Sonne', pct: 100 },
          ],
        });
      });
      server.middlewares.use('/api/camera', async (req, res) => {
        if (req.method === 'POST') {
          const f = await form(req);
          if (!authorized(req, f)) return deny(res);
          const a = f.get('a');
          const v = Number(f.get('v'));
          if (a === 'home') cam.mode = 0;
          if (a === 'go') Object.assign(cam, { mode: 1, place: v });
          if (a === 'follow') Object.assign(cam, { mode: 2, target: v });
          return json(res, { ok: true, msg: 'Ok' });
        }
        const target = cam.mode === 1 ? 0 : cam.mode === 2 ? cam.target : set.camHome === 0 ? 1000 : set.camHome === 1 ? set.camFig : 0;
        const place =
          cam.mode === 0 && set.camHome === 2 ? set.camPlace : cam.mode === 1 ? cam.place : (camFigs.find((x) => x.id === target)?.place ?? 1);
        json(res, {
          mode: cam.mode,
          target,
          targetName: camFigs.find((x) => x.id === target)?.name ?? '',
          place,
          returnIn: cam.mode === 0 || set.camRet === 0 ? -1 : 412,
          places: camPlaces,
          figures: camFigs,
        });
      });
      server.middlewares.use('/api/clients/name', async (req, res) => {
        const f = await form(req);
        if (!authorized(req, f)) return deny(res);
        names[f.get('id') ?? ''] = f.get('name') ?? '';
        json(res, { ok: true, msg: 'Gespeichert' });
      });
      for (const p of ['/api/reboot', '/api/factory-reset', '/api/coredump/erase']) {
        server.middlewares.use(p, async (req, res) => {
          const f = await form(req);
          if (!authorized(req, f)) return deny(res);
          if (p === '/api/coredump/erase') crashed = false;
          if (p === '/api/factory-reset') {
            pin = '';
            tokens.clear();
            Object.assign(set, { rot: 0, bright: 100, lp: 3000, ui: '' });
          }
          json(res, { ok: true, msg: 'OK' });
        });
      }
      server.middlewares.use('/api/backup', (_req, res) => {
        res.setHeader('Content-Disposition', 'attachment; filename="pixelia-einstellungen-2026-10-19.json"');
        json(res, {
          pixelia: 'settings', format: 1, v: '0.3.2', date: '2026-10-19', host: 'pixelia', set,
          names: Object.entries(names).filter(([, n]) => n).map(([id, name]) => ({ id, name })),
          wifi: setup ? [] : ['FRITZ!Box 7590', 'Gartenhaus'], pin: !!pin,
        });
      });
      server.middlewares.use('/api/wifi', (req, res) => {
        json(res, {
          saved: setup ? [] : ['FRITZ!Box 7590', 'Gartenhaus'],
          scan: [
            { ssid: 'FRITZ!Box 7590', rssi: -58, ch: 6, secure: true, saved: !setup },
            { ssid: 'Nachbar-WLAN', rssi: -71, ch: 11, secure: true, saved: false },
            { ssid: 'Gartenhaus', rssi: -80, ch: 1, secure: true, saved: !setup },
            { ssid: 'Telekom Hotspot', rssi: -86, ch: 1, secure: false, saved: false },
          ].slice(0, req.method === 'POST' || !setup ? 4 : 0),
        });
      });
      server.middlewares.use('/api/system', (_req, res) => {
        json(res, {
          board: 'Sunton ESP32-8048S050C_I',
          panel: { type: '5" TFT, RGB565 parallel', w: 800, h: 480, renderW: 400, renderH: 240, hz: 60.02, pclk: 24.6, isr: 20.1, desync: 0, mode: 'engine' },
          touch: { ok: true, chip: 'GT911', addr: '0x5D', fw: '0x1060', w: 800, h: 480, cfg: 65, touches: 312 },
          chip: { model: 'ESP32-S3', rev: 0.02, cores: 2, mhz: 240, temp: 41.3, wifi: true, ble: true },
          mac: { wifi: '3c:84:27:c1:0a:5c', ap: '3c:84:27:c1:0a:5d', bt: '3c:84:27:c1:0a:5e' },
          flash: { mb: 16, id: '0x204018', mode: 'QIO', freq: '80m' },
          psram: { kb: 8192, mode: 'Octal', mhz: 80 },
          heap: { intTotal: 330, psramTotal: 8160 },
          pins: { de: 40, vsync: 41, hsync: 39, pclk: 42, data: [8, 3, 46, 9, 1, 5, 6, 7, 15, 16, 4, 45, 48, 47, 21, 14], backlight: 2, sda: 19, scl: 20, touchRst: 38 },
          build: { v: '0.3.1', date: 'Oct 19 2026', time: '18:02:11', idf: 'v6.1', compiler: '14.2.0' },
          appSize: 1304304,
          appPart: 6291456,
          partitions: [
            { label: 'nvs', type: 'nvs', offset: 0x9000, size: 0x6000 },
            { label: 'otadata', type: 'otadata', offset: 0xf000, size: 0x2000 },
            { label: 'ota_0', type: 'ota', offset: 0x20000, size: 0x600000, v: '0.3.1', running: true, nextBoot: true },
            { label: 'ota_1', type: 'ota', offset: 0x620000, size: 0x600000, v: '0.3.0', running: false, nextBoot: false },
            { label: 'storage', type: 'littlefs', offset: 0xc20000, size: 0x3c0000 },
            { label: 'coredump', type: 'coredump', offset: 0xfe0000, size: 0x10000 },
          ],
          nvs: { used: 41, free: 463, total: 504 },
          net: setup
            ? { host: 'Pixelia', mdns: 'pixelia' }
            : { host: 'Pixelia', mdns: 'pixelia', ip: '192.168.178.74', mask: '255.255.255.0', gw: '192.168.178.1', dns: '192.168.178.1', bssid: 'e0:28:6d:1a:2b:3c', phy: '802.11n', connectedS: 11780 },
          time: { valid: !setup, syncs: setup ? 0 : 3, epoch: setup ? 0 : now(), tz: 'Europa/Berlin' },
        });
      });
      server.middlewares.use('/api/clients', (_req, res) => {
        const id = 'a1b2c3d4e5f6';
        json(res, {
          epoch: now(),
          you: '192.168.178.20',
          list: [
            { id, name: 'Handy Wohnzimmer', agent: 'Chrome · Android', ip: '192.168.178.31', page: 'uebersicht', online: true, first: now() - 86400 * 9, last: now(), agoS: 0, visits: 23 },
            { id: 'ffeeddccbbaa', name: '', agent: 'Edge · Windows', ip: '192.168.178.20', page: 'geraete', online: true, first: now() - 86400 * 2, last: now(), agoS: 0, visits: 7 },
            { id: '0a0b0c0d0e0f', name: '', agent: 'Safari · iOS', ip: '192.168.178.44', page: '', online: false, first: now() - 86400 * 30, last: now() - 5400, agoS: 5400, visits: 3 },
          ],
          ap: setup ? [{ mac: '5a:12:9f:00:11:22', ip: '192.168.4.2', rssi: -48 }] : [],
        });
      });
      server.middlewares.use('/api/resets', (_req, res) => {
        json(res, {
          boot: 42,
          entries: [
            { boot: 42, v: '0.3.1', part: 'ota_0', reason: 3, reasonText: 'Software-Neustart', time: '19.10.2026 18:05', timeLate: false, prevUptime: 4012, prevTime: '', trial: false, rollback: false, crash: null, note: '' },
            { boot: 41, v: '0.3.0', part: 'ota_1', reason: 1, reasonText: 'Einschalten', time: '19.10.2026 16:58', timeLate: false, prevUptime: null, prevTime: '', trial: false, rollback: false, crash: null, note: '' },
            { boot: 40, v: '0.3.0', part: 'ota_1', reason: 4, reasonText: 'Absturz (Panic)', time: '18.10.2026 21:14', timeLate: true, prevUptime: 812, prevTime: '', trial: true, rollback: false, crash: { task: 'httpd', pc: '0x42012345' }, note: '' },
          ],
        });
      });
      server.middlewares.use('/api/coredump/summary', (_req, res) =>
        json(res, crashed ? {
          present: true, valid: true, size: 25732, addr: '0xfe0000', task: 'httpd', pc: '0x42012345',
          reason: 'LoadProhibited', excCause: 28, excVaddr: '0x00000004',
          backtrace: ['0x42012342', '0x42011e8b', '0x4200f7a1', '0x4037c2d5'], btCorrupted: false,
          elfSha: '9f3c2a7d51e04b6c8a1f0e2d3c4b5a69', version: '0.3.0', part: 'ota_1',
        } : { present: false }),
      );
      // erst nach /summary, sonst fängt der Präfix-Abgleich auch diese Anfragen
      server.middlewares.use('/api/coredump', (_req, res) => {
        res.setHeader('Content-Disposition', 'attachment; filename="pixelia-coredump-mock.bin"');
        res.end(Buffer.alloc(crashed ? 1024 : 0));
      });
      // Update von GitHub (erst die Unterpfade, sonst fängt der Präfix-Abgleich von /api/update alles)
      server.middlewares.use('/api/update/check', (_req, res) => {
        if (setup) return json(res, { ok: false, msg: 'Im Setup-Modus hat Pixelia kein Internet. Erst das WLAN einrichten.' });
        if (gh.job) return json(res, { ok: false, msg: 'Die Prüfung läuft schon' });
        gh.job = 'check';
        gh.err = '';
        setTimeout(() => {
          gh.job = '';
          if (ghMode === 'limit') gh.err = 'GitHub-Limit erreicht: höchstens 60 Abfragen pro Stunde je Internetanschluss. Wieder möglich um 18:42 Uhr (in 23 min).';
          else ghCheck();
        }, 1800);
        json(res, { ok: true, msg: 'Prüfung läuft' });
      });
      server.middlewares.use('/api/update/channel', async (req, res) => {
        const f = await form(req);
        if (!authorized(req, f)) return deny(res);
        if (f.has('ch')) gh.ch = f.get('ch') === 'preview' ? 'preview' : 'stable';
        if (f.has('auto')) (gh as Record<string, unknown>).auto = f.get('auto') === '1';
        json(res, { ok: true, msg: 'OK' });
      });
      server.middlewares.use('/api/update/cancel', async (req, res) => {
        const f = await form(req);
        if (!authorized(req, f)) return deny(res);
        if (gh.job !== 'install') return json(res, { ok: false, msg: 'Es läuft kein Download von GitHub' });
        ghCancel = true;
        json(res, { ok: true, msg: 'Wird abgebrochen' });
      });
      server.middlewares.use('/api/exp', async (req, res) => {
        if (req.method !== 'POST') return json(res, exp);
        const f = await form(req);
        const a = f.get('a');
        if (a !== 'revert' && !authorized(req, f)) return deny(res);
        const ok = (msg = 'OK') => json(res, { ok: true, msg });
        if (a === 'on' || a === 'off') {
          exp.on = a === 'on';
          if (!exp.on) Object.assign(exp, { trialS: -1, testS: -1, mode: 'engine', pic: false, panel: { ...std }, saved: { ...std }, place: 0, trans: 'abblenden', tour: 0, figures: 0, follow: true, inspect: false });
          return ok();
        }
        if (a === 'picoff') return ((exp.pic = false), ok('Testbild beendet'));
        if (a === 'test') {
          const m = f.get('mode') ?? '';
          if (m === 'engine') return (Object.assign(exp, { mode: 'engine', testS: -1 }), ok());
          if (exp.trialS >= 0) return json(res, { ok: false, msg: 'Erst die Probe der Panel-Werte abschließen' });
          if (m === 'touch') exp.touch.corners = 0;
          Object.assign(exp, { mode: m, testS: 60 });
          expRun();
          return ok();
        }
        if (!exp.on) return json(res, { ok: false, msg: 'Experimentell ist aus' });
        if (a === 'panel') {
          const p = panelOf(f);
          if (p.hz < 24 || p.hz > 75) return json(res, { ok: false, msg: 'Bildwiederholung muss zwischen 24 und 75 Hz liegen' });
          if (exp.trialS < 0) prevPanel = { ...exp.panel };
          Object.assign(exp, { panel: p, trialS: exp.trialNeed, mode: 'engine', testS: -1 });
          expRun();
          return ok('Probe läuft');
        }
        if (a === 'confirm') {
          if (exp.trialS < 0) return json(res, { ok: false, msg: 'Keine Probe aktiv (schon zurückgesetzt?)' });
          Object.assign(exp, { trialS: -1, saved: { ...exp.panel } });
          return ok('Gespeichert');
        }
        if (a === 'revert') return (exp.trialS >= 0 && Object.assign(exp, { trialS: -1, panel: { ...prevPanel } }), ok());
        if (a === 'reset') return (Object.assign(exp, { trialS: -1, safeBoot: false, panel: { ...std }, saved: { ...std } }), ok());
        // Orte (seit 0.5.0)
        if (a === 'place') return ((exp.place = Number(f.get('n') ?? 0)), ok(exp.places[exp.place]?.title));
        if (a === 'trans') return ((exp.trans = f.get('v') ?? 'abblenden'), ok('Übergang gesetzt'));
        if (a === 'tour') return ((exp.tour = Number(f.get('s') ?? 0)), ok(exp.tour ? 'Rundgang an' : 'Rundgang aus'));
        if (a === 'figures') return ((exp.figures = Number(f.get('n') ?? 0)), ok('Figuren gesetzt'));
        // Raum-Baukasten (seit 0.5.1)
        if (a === 'weather') return ((exp.weather = f.get('v') as typeof exp.weather), (exp.weatherPct = Number(f.get('p') ?? 100)), ok('Wetter gesetzt'));
        if (a === 'cmd' && f.get('v') === 'innenlicht um') return ((exp.innen = !exp.innen), ok(exp.innen ? 'Licht innen an' : 'Licht innen aus'));
        if (a === 'cmd' && f.get('v') === 'blitz') return ok('Blitz');
        if (a === 'cmd') return f.get('v') === 'ausbau' ? ok('Nächste Ausbaustufe') : f.get('v') === 'baustelle' ? ok('Im Bau umgeschaltet') : json(res, { ok: false, msg: 'Befehl unbekannt' });
        if (a === 'follow') return ((exp.follow = f.get('v') !== '0'), ok(exp.follow ? 'Blick folgt' : 'Blick bleibt'));
        if (a === 'inspect') return ((exp.inspect = f.get('v') === '1'), ok(exp.inspect ? 'Prüfansicht an' : 'Prüfansicht aus'));
        if (a === 'time') {
          if (f.get('v') === 'real') return ((exp.real = true), (exp.speed = 1), ok('Echte Uhrzeit'));
          return ((exp.minute = Number(f.get('v'))), (exp.real = false), ok('Uhrzeit gestellt'));
        }
        if (a === 'speed') return ((exp.speed = Number(f.get('v'))), (exp.real = false), ok('Zeitraffer an'));
        json(res, { ok: false, msg: 'Aktion unbekannt' });
      });
      server.middlewares.use('/api/picture', async (req, res) => {
        if (!authorized(req, new URLSearchParams())) return deny(res);
        const chunks: Buffer[] = [];
        for await (const c of req) chunks.push(c as Buffer);
        const body = Buffer.concat(chunks);
        if (body.length !== 400 * 240 + 240 * 3) return ((res.statusCode = 400), json(res, { ok: false, msg: 'Bild muss 400 × 240 groß sein' }));
        Object.assign(exp, { mode: 'engine', testS: -1, pic: true });
        picture = body;
        json(res, { ok: true, msg: 'Testbild auf dem Display' });
      });
      server.middlewares.use('/api/screen', (_req, res) => {
        if (exp.mode !== 'engine') return ((res.statusCode = 503), json(res, { ok: false, msg: 'Kein Bild (Testbild aktiv?)' }));
        res.setHeader('Content-Type', 'application/octet-stream');
        res.end(screen());
      });
      server.middlewares.use('/api/perf', (req, res) => {
        const j = (x: number, d = 0.4) => Math.round((x + Math.random() * d - d / 2) * 100) / 100;
        const withTasks = (req.url ?? '').includes('tasks=1');
        const eco = exp.panel.eco;
        json(res, {
          uptime: Math.floor((Date.now() - t0) / 1000), at: Math.floor((Date.now() - t0) / 2000) * 2, mode: 'engine',
          lcd: { hz: exp.panel.hz, pclk: exp.panel.pclk / 1000 / (eco ? 2 : 1), isr: eco ? j(10.1) : j(20.2), isrMaxUs: 66, desync: 0 },
          engine: { fps: j(30.1, 0.3), render: j(4.3), renderMax: j(4.9), logic: 0.08, logicMax: j(0.9), holds: 0, ticks: [0, 61, 0], sprites: 48, buf: 'psram' },
          cpu: [Math.round(j(4, 4)), Math.round(eco ? j(8, 2) : j(13, 2))],
          heap: { ram: Math.round(j(116, 3)), ramMin: 94, ramBlk: 31, dma: 110, psram: 6012, psramMin: 5690, psramBlk: 5920 },
          wifi: { state: 'verbunden', rssi: -61, ch: 1 },
          ...(withTasks
            ? {
                taskWindowMs: 2000, taskQueryUs: 1105,
                tasks: [
                  ['gfx', 1, 10, 3120, 12.9, 'bereit'], ['IDLE1', 1, 0, 812, 87.1, 'bereit'], ['IDLE0', 0, 0, 760, 95.2, 'bereit'],
                  ['httpd', 0, 5, 4180, 2.1, 'blockiert'], ['wifi', 0, 23, 2900, 1.4, 'blockiert'], ['touch', 0, 5, 1720, 0.6, 'blockiert'],
                  ['perf', 0, 2, 1980, 0.1, 'blockiert'], ['webstatus', 0, 2, 2210, 0.3, 'blockiert'], ['logpush', 0, 2, 1460, 0.1, 'blockiert'],
                ].map(([name, core, prio, stackFree, cpu, state]) => ({ name, core, prio, stackFree, cpu, state })),
              }
            : { tasks: null }),
        });
      });
      server.middlewares.use('/api/update/install', async (req, res) => {
        const f = await form(req);
        if (!authorized(req, f)) return deny(res);
        const list = (f.get('old') === '1' ? (gh as Record<string, unknown>).old : gh.rel) as { v: string; size: number }[];
        const r = list.find((x) => x.v === f.get('v'));
        if (!r) return json(res, { ok: false, msg: 'Version ist nicht das Ergebnis der letzten Prüfung, bitte neu suchen' });
        if (f.get('old') === '1' && !exp.on) return json(res, { ok: false, msg: 'Ältere Versionen nur mit eingeschaltetem Experimentell' });
        gh.job = 'install';
        gh.err = '';
        Object.assign(gh, { cancelled: false });
        ghCancel = false;
        json(res, { ok: true, msg: 'Download läuft' });
        setTimeout(() => {
          Object.assign(ota, { busy: true, src: 'github', bytes: 0, total: r.size, newV: r.v });
          const timer = setInterval(() => {
            ota.bytes = Math.min(r.size, ota.bytes + Math.round(r.size / 16));
            if (ghCancel) {
              clearInterval(timer);
              Object.assign(ota, { busy: false });
              Object.assign(gh, { job: '', cancelled: true });
              return;
            }
            if (ghMode === 'fail' && ota.bytes > r.size / 3) {
              clearInterval(timer);
              Object.assign(ota, { busy: false });
              gh.job = '';
              gh.err = 'Download nach 512 KB abgebrochen';
              return;
            }
            if (ota.bytes < r.size) return;
            clearInterval(timer);
            // Neustart, danach Probelauf (im Mock schneller)
            downUntil = Date.now() + 3000;
            setTimeout(() => {
              installedV = r.v;
              gh.job = '';
              gh.rel = [];
              Object.assign(ota, { busy: false, trial: true, trialS: 0, passed: false });
              const trial = setInterval(() => {
                ota.trialS += 6;
                if (ota.trialS >= 60) Object.assign(ota, { trial: false, passed: true }), clearInterval(trial);
              }, 1000);
            }, 3000);
          }, 400);
        }, 1500);
      });
      server.middlewares.use('/api/update', (_req, res) => {
        if (Date.now() < downUntil) return ((res.statusCode = 503), res.end());
        json(res, {
          ch: gh.ch, job: gh.job, err: gh.err, net: !setup, checkedS: gh.checkedAt ? Math.floor((Date.now() - gh.checkedAt) / 1000) : -1,
          checkedCh: gh.checkedCh, latest: gh.latest, rel: gh.rel, rate: { left: gh.left, limit: 60, resetS: 1380 },
          cancelled: (gh as Record<string, unknown>).cancelled, auto: (gh as Record<string, unknown>).auto,
          lastAuto: false, lastT: gh.checkedAt ? Math.floor(gh.checkedAt / 1000) : 0, avail: (gh.rel[0] as { v?: string })?.v ?? '',
          seq: gh.checkedAt, old: (gh as Record<string, unknown>).old,
        });
      });
      // Versionsverlauf wie in der Firmware: CHANGELOG.json aus dem Repo, nur v, d, t, k (tools/release_notes.py --embed)
      server.middlewares.use('/api/changelog', (_req, res) => {
        res.setHeader('Content-Type', 'application/json');
        const log = JSON.parse(readFileSync(fileURLToPath(new URL('../CHANGELOG.json', import.meta.url)), 'utf8')) as Entry[];
        res.end(JSON.stringify(log.map(({ v, d, t, k }) => ({ v, d, t, k }))));
      });
      server.middlewares.use('/api/log', (req, res) => {
        const u = new URL(req.url ?? '', 'http://x');
        const since = Number(u.searchParams.get('since') ?? -1);
        const msgs: [string, string][] = [
          ['I', 'perf: 30,0 fps, Zeichnen 4,0 ms, ISR 20 %, RAM 117 KB'],
          ['I', 'web: Neues Gerät verbunden: Chrome · Android (192.168.178.31)'],
          ['W', 'wifi: Signal schwach (-81 dBm), suche stärkeres Netz'],
          ['D', 'gfx: Puffertausch 1/1'],
          ['E', 'ota: Update abgelehnt: Firmware für einen anderen Chip'],
        ];
        const lines = [];
        if (since < 0) for (let n = 100; n < logN; n++) lines.push(logLine(n, ...msgs[n % 5]));
        else if (Math.random() < 0.7) lines.push(logLine(++logN, ...msgs[logN % 5]));
        json(res, { rid: 7, reset: false, lines });
      });
      server.middlewares.use('/setup', (_req, res) => {
        res.setHeader('Content-Type', 'text/html; charset=utf-8');
        res.end(readFileSync(fileURLToPath(new URL('../firmware/main/web/setup.html', import.meta.url))));
      });
      server.middlewares.use('/roboto-flex-pixelia.woff2', (_req, res) => {
        res.setHeader('Content-Type', 'font/woff2');
        res.end(readFileSync(fileURLToPath(new URL('./src/assets/roboto-flex-pixelia.woff2', import.meta.url))));
      });
      server.middlewares.use('/api/cmd', (_req, res) => json(res, { ok: true, cmd: 'help', out: 'help    Befehle anzeigen\nperf    Leistung\nwifi    WLAN-Status', ms: 12 }));
      server.middlewares.use('/api/status', (_req, res) => {
        if (Date.now() < downUntil) return ((res.statusCode = 503), res.end());
        const up = Math.floor((Date.now() - t0) / 1000) + 3 * 3600 + 17 * 60;
        const j = (x: number) => Math.round((x + Math.random() * 0.4 - 0.2) * 10) / 10;
        res.setHeader('Content-Type', 'application/json');
        res.end(
          JSON.stringify({
            v: installedV, built: 'Oct 19 2026 18:02:11', idf: 'v6.1', part: 'ota_0', uptime: up, host: 'pixelia',
            wifi: setup
              ? { state: 'setup', ssid: '', rssi: 0, ch: 1, ip: '', ap: true, apClients: 1, retry: 87, err: 'Kein gespeichertes Netz', saved: 0 }
              : { state: 'verbunden', ssid: 'FRITZ!Box 7590', rssi: -58, ch: 6, ip: '192.168.178.74', ap: false,
                  apClients: 0, retry: 0, err: '', saved: 2 },
            time: { valid: !setup, local: setup ? '' : new Date().toLocaleString('de-DE').replace(',', '') },
            perf: { fps: 30, render: j(4.0), isr: j(20), lcd: 60.02, cpu0: Math.round(j(9)), cpu1: Math.round(j(34)),
              desync: 0, ram: 117, ramMin: 107, ramBlk: 92, psram: 7310, psramMin: 7280 },
            ota: { ...ota, avail: (gh.rel[0] as { v?: string })?.v ?? '', ghSeq: gh.checkedAt },
            rollback: null, set: { ...set, exp: exp.on }, auth: { pin: !!pin, lockS: lockS() },
            diag: { boot: 42, reset: 'Software-Neustart', errors: 0, warnings: 2, logLines: 1834,
              crash: crashed ? { task: 'httpd', reason: 'LoadProhibited', v: '0.3.0' } : null },
          }),
        );
      });
    },
  };
}

export default defineConfig(({ mode }) => ({
  plugins: [svelte(), materialSymbols(), mockDevice()],
  build: {
    target: 'es2020',
    outDir: 'dist',
    emptyOutDir: true,
    assetsDir: '',
    assetsInlineLimit: 0,
    cssCodeSplit: false,
    modulePreload: false,
    reportCompressedSize: true,
    rollupOptions: {
      output: {
        entryFileNames: 'app.js',
        chunkFileNames: 'app-[name].js',
        assetFileNames: (info) => (info.names?.[0]?.endsWith('.css') ? 'app.css' : '[name][extname]'),
      },
    },
  },
  // Im Mock nichts ans Gerät weiterleiten (sonst kam der Status per WebSocket vom echten Gerät)
  server: mode === 'mock' ? {} : {
    proxy: {
      '/api': target,
      '/setup': target,
      '/ws': { target, ws: true },
    },
  },
}));
