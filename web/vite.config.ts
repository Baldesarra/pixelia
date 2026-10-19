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

// Ohne Gerät entwickeln: npm run mock liefert /api/status mit Beispielwerten (kein WebSocket,
// die Oberfläche fällt dann auf Abfrage zurück)
function mockDevice(): Plugin {
  const t0 = Date.now();
  return {
    name: 'pixelia-mock',
    apply: 'serve',
    configureServer(server) {
      if (server.config.mode !== 'mock') return;
      server.middlewares.use('/api/status', (_req, res) => {
        const up = Math.floor((Date.now() - t0) / 1000) + 3 * 3600 + 17 * 60;
        const j = (x: number) => Math.round((x + Math.random() * 0.4 - 0.2) * 10) / 10;
        res.setHeader('Content-Type', 'application/json');
        res.end(
          JSON.stringify({
            v: '0.3.0', built: 'Oct 19 2026 18:02:11', idf: 'v6.1', part: 'ota_0', uptime: up, host: 'pixelia',
            wifi: { state: 'verbunden', ssid: 'FRITZ!Box 7590', rssi: -58, ch: 6, ip: '192.168.178.74', ap: false,
              apClients: 0, retry: 0, err: '', saved: 2 },
            time: { valid: true, local: new Date().toLocaleString('de-DE').replace(',', '') },
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
