// Screenshots der Weboberfläche: Handy (390 px) und Desktop (1280 px), hell und dunkel, ganze Seite.
// Nutzt Edge/Chrome kopflos über das DevTools-Protokoll (keine npm-Pakete nötig, Node.js 24).
//
//   node tools/web_screens.mjs                                  # Entwicklungsserver (npm run mock) auf :5173
//   node tools/web_screens.mjs --url http://pixelia --out .logs/screens --pages "#/,#/darstellung"
//   --variants handy-hell,desktop-dunkel   nur diese Varianten; w360, w412-hell: beliebige Handybreite
//   --eval "document.querySelector('button').click()"   nach dem Laden ausführen (z. B. klicken), danach --wait2 ms
//   --text 1                                Text der Seite ausgeben (Prüfung ohne Bild); Skriptfehler stehen immer da
import { spawn } from 'node:child_process';
import { existsSync, mkdirSync, mkdtempSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';

const arg = (name, def) => {
  const i = process.argv.indexOf(`--${name}`);
  return i > 0 ? process.argv[i + 1] : def;
};
const base = arg('url', 'http://localhost:5173').replace(/\/$/, '');
const out = arg('out', '.logs/screens');
const pages = arg('pages', '#/,#/darstellung').split(',');
const wait = Number(arg('wait', '4000'));
const evalJs = arg('eval', '');
const wait2 = Number(arg('wait2', '1500'));
const printText = arg('text', '') === '1';
const only = arg('variants', '');

const browsers = [
  'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe',
  'C:/Program Files/Microsoft/Edge/Application/msedge.exe',
  'C:/Program Files/Google/Chrome/Application/chrome.exe',
  '/usr/bin/chromium',
  '/usr/bin/google-chrome',
];
const exe = arg('browser', browsers.find((b) => existsSync(b)));
if (!exe) throw new Error('Kein Edge/Chrome gefunden (--browser <Pfad>)');

const allVariants = [
  { name: 'handy-hell', width: 390, height: 844, mobile: true, dpr: 2, scheme: 'light' },
  { name: 'handy-dunkel', width: 390, height: 844, mobile: true, dpr: 2, scheme: 'dark' },
  { name: 'desktop-hell', width: 1280, height: 860, mobile: false, dpr: 1, scheme: 'light' },
  { name: 'desktop-dunkel', width: 1280, height: 860, mobile: false, dpr: 1, scheme: 'dark' },
];
// Beliebige Handybreite: w360, w412-hell … (dunkel, wenn nicht anders angegeben)
const custom = (n) => {
  const m = /^w(\d+)(?:-(hell|dunkel))?$/.exec(n);
  return m && { name: n, width: +m[1], height: 844, mobile: true, dpr: 2, scheme: m[2] === 'hell' ? 'light' : 'dark' };
};
const variants = only
  ? only.split(',').map((n) => allVariants.find((v) => v.name === n) ?? custom(n)).filter(Boolean)
  : allVariants;

const port = 9300 + Math.floor(Math.random() * 500);
const profile = mkdtempSync(join(tmpdir(), 'pixelia-screens-'));
const proc = spawn(exe, ['--headless=new', '--disable-gpu', '--hide-scrollbars', `--remote-debugging-port=${port}`,
  `--user-data-dir=${profile}`, 'about:blank'], { stdio: 'ignore' });

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
let target;
for (let i = 0; i < 50 && !target; i++) {
  await sleep(200);
  try {
    target = (await (await fetch(`http://127.0.0.1:${port}/json/list`)).json()).find((t) => t.type === 'page');
  } catch { /* Browser startet noch */ }
}
if (!target) throw new Error('Browser antwortet nicht');

const ws = new WebSocket(target.webSocketDebuggerUrl);
await new Promise((r) => (ws.onopen = r));
let seq = 0;
const pending = new Map();
ws.onmessage = (e) => {
  const m = JSON.parse(e.data);
  if (m.method === 'Runtime.exceptionThrown') {
    const d = m.params.exceptionDetails;
    console.log(`Skriptfehler: ${d.exception?.description ?? d.text}`);
  }
  if (m.id && pending.has(m.id)) {
    pending.get(m.id)(m);
    pending.delete(m.id);
  }
};
const send = (method, params = {}) =>
  new Promise((resolve, reject) => {
    const id = ++seq;
    pending.set(id, (m) => (m.error ? reject(new Error(`${method}: ${m.error.message}`)) : resolve(m.result)));
    ws.send(JSON.stringify({ id, method, params }));
  });

mkdirSync(out, { recursive: true });
await send('Runtime.enable');
try {
  for (const v of variants) {
    await send('Emulation.setDeviceMetricsOverride', {
      width: v.width, height: v.height, deviceScaleFactor: v.dpr, mobile: v.mobile, screenWidth: v.width, screenHeight: v.height,
    });
    await send('Emulation.setEmulatedMedia', { features: [{ name: 'prefers-color-scheme', value: v.scheme }] });
    // Neu laden erzwingen: Seiten mit gleichem Pfad (#/…) laden sonst nicht neu und behalten die alte Handybreite
    await send('Page.navigate', { url: 'about:blank' });
    await sleep(300);
    for (const p of pages) {
      await send('Page.navigate', { url: `${base}/${p}` });
      await sleep(wait);
      if (evalJs) {
        const r = await send('Runtime.evaluate', { expression: evalJs, awaitPromise: true, returnByValue: true });
        if (r.exceptionDetails) console.log(`--eval: ${r.exceptionDetails.exception?.description ?? r.exceptionDetails.text}`);
        await sleep(wait2);
      }
      if (printText) {
        const r = await send('Runtime.evaluate', { expression: "document.querySelector('main')?.innerText ?? document.body.innerText", returnByValue: true });
        console.log(`--- ${p} (${v.name}) ---`);
        console.log(String(r.result.value).replace(/\n{2,}/g, '\n').trim());
      }
      const { cssContentSize } = await send('Page.getLayoutMetrics');
      // Viewport auf die volle Seitenhöhe, damit feste Elemente (Navigationsleiste) unten im Bild landen
      const height = Math.max(v.height, Math.ceil(cssContentSize.height));
      await send('Emulation.setDeviceMetricsOverride', {
        width: v.width, height, deviceScaleFactor: v.dpr, mobile: v.mobile, screenWidth: v.width, screenHeight: height,
      });
      await sleep(500);
      const shot = await send('Page.captureScreenshot', { format: 'png' });
      await send('Emulation.setDeviceMetricsOverride', {
        width: v.width, height: v.height, deviceScaleFactor: v.dpr, mobile: v.mobile, screenWidth: v.width, screenHeight: v.height,
      });
      const page = (p.replace(/^#\/?/, '') || 'uebersicht').replace(/\//g, '-');
      const file = join(out, `${page}-${v.name}.png`);
      writeFileSync(file, Buffer.from(shot.data, 'base64'));
      console.log(file);
    }
  }
} finally {
  ws.close();
  proc.kill();
}
