// Vite: feste Dateinamen (die Firmware bettet sie ein), keine CDNs, Icons nur die benutzten.
// Entwicklung am echten Gerät: npm run dev  (Ziel ändern: PIXELIA_TARGET=http://192.168.1.50 npm run dev)
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { defineConfig, type Plugin } from 'vite';
import { svelte } from '@sveltejs/vite-plugin-svelte';
import { ICONS } from './src/lib/m3/icon-names.ts';

const target = process.env.PIXELIA_TARGET ?? 'http://pixelia';

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
        out[name] = d[1];
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
  return {
    name: 'pixelia-mock',
    apply: 'serve',
    configureServer(server) {
      if (server.config.mode !== 'mock') return;
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
      server.middlewares.use('/api/coredump/summary', (_req, res) => json(res, { present: false }));
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
        const up = Math.floor((Date.now() - t0) / 1000) + 3 * 3600 + 17 * 60;
        const j = (x: number) => Math.round((x + Math.random() * 0.4 - 0.2) * 10) / 10;
        res.setHeader('Content-Type', 'application/json');
        res.end(
          JSON.stringify({
            v: '0.3.1', built: 'Oct 19 2026 18:02:11', idf: 'v6.1', part: 'ota_0', uptime: up, host: 'pixelia',
            wifi: setup
              ? { state: 'setup', ssid: '', rssi: 0, ch: 1, ip: '', ap: true, apClients: 1, retry: 87, err: 'Kein gespeichertes Netz', saved: 0 }
              : { state: 'verbunden', ssid: 'FRITZ!Box 7590', rssi: -58, ch: 6, ip: '192.168.178.74', ap: false,
                  apClients: 0, retry: 0, err: '', saved: 2 },
            time: { valid: !setup, local: setup ? '' : new Date().toLocaleString('de-DE').replace(',', '') },
            perf: { fps: 30, render: j(4.0), isr: j(20), lcd: 60.02, cpu0: Math.round(j(9)), cpu1: Math.round(j(34)),
              desync: 0, ram: 117, ramMin: 107, ramBlk: 92, psram: 7310, psramMin: 7280 },
            ota: { busy: false, bytes: 0, total: 0, newV: '', err: '', trial: false, trialS: 0, trialNeed: 60,
              trialNote: '', passed: true },
            rollback: null, set: { rot: 0, bright: 80, lp: 3000 },
            diag: { boot: 42, reset: 'Software-Neustart', errors: 0, warnings: 2, logLines: 1834, crash: null },
          }),
        );
      });
    },
  };
}

export default defineConfig({
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
  server: {
    proxy: {
      '/api': target,
      '/setup': target,
      '/ws': { target, ws: true },
    },
  },
});
