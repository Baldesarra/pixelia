<script lang="ts">
  // Seite „Live“ (seit 0.4.2): Live-Vorschau des Displays mit Screenshot (PNG) und Leistungsanzeige.
  // Vorschau: GET /api/screen liefert das 400×240-Bild als Paletten-Indizes (PackBits), der Browser malt es ins Canvas
  // und erzeugt das PNG selbst. Höchstens ~3 Bilder pro Sekunde, nächste Anfrage erst nach der Antwort, nur solange die
  // Seite sichtbar ist. Leistung: /api/perf alle 2 s (so oft misst das Gerät); die Task-Liste nur auf Knopfdruck,
  // sie hält 1,1 ms die Kernel-Sperre und kostet einen Display-Aussetzer (docs/HARDWARE.md).
  import { onMount } from 'svelte';
  import { Button, Card, Chip, Icon, confirmDialog } from '../lib/m3';
  import { get, type PerfData } from '../lib/api';
  import { kb, num } from '../lib/format';
  import Sparkline from '../lib/Sparkline.svelte';

  const kMinFrameMs = 333;
  const kPoints = 60; // 2 min Verlauf

  // --- Live-Vorschau ----------------------------------------------------------------------------------------
  let canvas = $state<HTMLCanvasElement>();
  let running = $state(true);
  let note = $state('');
  let shown = $state(false);
  let rate = $state(0); // Vorschau-Bilder pro Sekunde
  let bytes = $state(0); // Größe des letzten Bildes
  let frameNo = $state(0);

  // PackBits entpacken und über die Palette direkt in die Pixel des Canvas schreiben
  function draw(buf: ArrayBuffer) {
    const dv = new DataView(buf);
    if (dv.getUint32(0) !== 0x50585331) throw new Error('Kein Vorschaubild'); // „PXS1“
    const w = dv.getUint16(4, true);
    const h = dv.getUint16(6, true);
    frameNo = dv.getUint32(8, true);
    const src = new Uint8Array(buf);
    const lut = new Uint32Array(256);
    for (let i = 0; i < 256; i++) {
      const o = 16 + i * 3;
      lut[i] = 0xff000000 | (src[o + 2] << 16) | (src[o + 1] << 8) | src[o]; // ABGR (Little Endian)
    }
    const ctx = canvas!.getContext('2d')!;
    const img = ctx.createImageData(w, h);
    const px = new Uint32Array(img.data.buffer);
    let i = 16 + 768;
    let o = 0;
    while (i < src.length && o < px.length) {
      const c = src[i++];
      if (c < 128) {
        for (let k = 0; k <= c; k++) px[o++] = lut[src[i++]];
      } else if (c > 128) {
        px.fill(lut[src[i++]], o, o + 257 - c);
        o += 257 - c;
      }
    }
    ctx.putImageData(img, 0, 0);
    shown = true;
  }

  const sleep = (ms: number) => new Promise((r) => setTimeout(r, ms));

  async function previewLoop(alive: () => boolean) {
    const times: number[] = [];
    while (alive()) {
      if (!running || document.hidden) {
        await sleep(300);
        continue;
      }
      const t0 = performance.now();
      try {
        const r = await fetch('/api/screen', { cache: 'no-store' });
        if (!r.ok) {
          note = r.status === 503 ? 'Gerade kein Bild: Testbild aktiv oder Pixelia startet.' : `Vorschau nicht verfügbar (HTTP ${r.status}).`;
          await sleep(2000);
          continue;
        }
        const buf = await r.arrayBuffer();
        if (!alive()) return;
        draw(buf);
        bytes = buf.byteLength;
        note = '';
        times.push(performance.now());
        while (times.length > 6) times.shift();
        if (times.length > 1) rate = ((times.length - 1) * 1000) / (times[times.length - 1] - times[0]);
      } catch {
        note = 'Pixelia antwortet nicht.';
        await sleep(2000);
        continue;
      }
      await sleep(Math.max(0, kMinFrameMs - (performance.now() - t0)));
    }
  }

  function screenshot() {
    if (!canvas) return;
    // 2× wie auf dem Display (800×480), ohne Glätten
    const big = document.createElement('canvas');
    big.width = canvas.width * 2;
    big.height = canvas.height * 2;
    const ctx = big.getContext('2d')!;
    ctx.imageSmoothingEnabled = false;
    ctx.drawImage(canvas, 0, 0, big.width, big.height);
    const d = new Date();
    const p = (v: number) => String(v).padStart(2, '0');
    const name = `pixelia-${d.getFullYear()}${p(d.getMonth() + 1)}${p(d.getDate())}-${p(d.getHours())}${p(d.getMinutes())}${p(d.getSeconds())}.png`;
    big.toBlob((blob) => {
      if (!blob) return;
      const a = document.createElement('a');
      a.href = URL.createObjectURL(blob);
      a.download = name;
      a.click();
      setTimeout(() => URL.revokeObjectURL(a.href), 1000);
    }, 'image/png');
  }

  // --- Leistung -------------------------------------------------------------------------------------------
  let perf = $state<PerfData | null>(null);
  let perfFailed = $state(false);
  let hist = $state({
    fps: [] as (number | null)[],
    render: [] as (number | null)[],
    renderMax: [] as (number | null)[],
    cpu0: [] as (number | null)[],
    cpu1: [] as (number | null)[],
    ram: [] as (number | null)[],
    psram: [] as (number | null)[],
  });
  let lastAt = -1;

  function push(p: PerfData) {
    if (p.at === lastAt) return;
    lastAt = p.at;
    const add = (k: keyof typeof hist, v: number | null) => {
      hist[k].push(v);
      if (hist[k].length > kPoints) hist[k].shift();
    };
    add('fps', p.engine?.fps ?? null);
    add('render', p.engine?.render ?? null);
    add('renderMax', p.engine?.renderMax ?? null);
    add('cpu0', p.cpu[0]);
    add('cpu1', p.cpu[1]);
    add('ram', p.heap.ram);
    add('psram', p.heap.psram / 1024);
  }

  let tasks = $state<PerfData['tasks']>(null);
  let taskInfo = $state('');
  let loadingTasks = $state(false);
  async function loadTasks() {
    const ok = await confirmDialog({
      title: 'Task-Liste abrufen?',
      text: 'Das Abfragen sperrt den Systemkern etwa 1 ms lang. Das Display kann dabei einmal kurz zucken. Deshalb gibt es die Liste nur auf Knopfdruck.',
      ok: 'Abrufen',
      icon: 'list',
    });
    if (!ok) return;
    loadingTasks = true;
    const p = await get<PerfData>('/api/perf?tasks=1').catch(() => null);
    loadingTasks = false;
    if (!p?.tasks) return (taskInfo = 'Task-Liste nicht abrufbar.');
    tasks = [...p.tasks].sort((a, b) => b.cpu - a.cpu);
    taskInfo = `${p.tasks.length} Tasks, CPU-Anteil über ${num((p.taskWindowMs ?? 0) / 1000, 1)} s, Abfrage ${num((p.taskQueryUs ?? 0) / 1000, 2)} ms`;
  }

  onMount(() => {
    let alive = true;
    void previewLoop(() => alive);
    const load = () =>
      document.hidden
        ? undefined
        : get<PerfData>('/api/perf')
            .then((p) => ((perf = p), (perfFailed = false), push(p)))
            .catch(() => (perfFailed = true));
    void load();
    const t = setInterval(load, 2000);
    return () => {
      alive = false;
      clearInterval(t);
    };
  });

  const last = (a: (number | null)[]) => a[a.length - 1] ?? null;
  const minOf = (a: (number | null)[]) => {
    const v = a.filter((x): x is number => x != null);
    return v.length ? Math.min(...v) : null;
  };
</script>

<div class="page-grid">
  <Card class="wide">
    <h2 class="card-title">
      <Icon name="videocam" size={20} />Live-Vorschau
      <span class="end">
        {#if running && shown && !note}<Chip pulse tone="primary"><span class="tnum">{num(rate, 1)} Bilder/s · {num(bytes / 1024)} KB</span></Chip>{/if}
      </span>
    </h2>
    <div class="screen" class:empty={!shown}>
      <canvas bind:this={canvas} width="400" height="240" aria-label="Bild des Displays"></canvas>
    </div>
    {#if note}<p class="muted">{note}</p>{/if}
    <div class="actions">
      <Button variant="tonal" icon={running ? 'pause' : 'play_arrow'} onclick={() => (running = !running)}>
        {running ? 'Anhalten' : 'Weiter'}
      </Button>
      <Button icon="photo_camera" disabled={!shown} onclick={screenshot}>Screenshot (PNG)</Button>
    </div>
    <p class="muted small">
      Bis zu drei Bilder pro Sekunde; der Screenshot hat 800 × 480 Pixel{frameNo ? ` (Bild ${frameNo})` : ''}.
    </p>
  </Card>

  {#if perf}
    {@const e = perf.engine}
    <Card>
      <h2 class="card-title"><Icon name="speed" size={20} />Bildrate</h2>
      <div class="big tnum">{e ? num(e.fps, 1) : '–'} <span class="unit">fps</span></div>
      <Sparkline series={[hist.fps]} min={0} max={35} limit={25} label="Bildrate der letzten 2 Minuten" />
      <dl class="kv">
        <dt>Tiefstwert (2 min)</dt>
        <dd class="tnum">{minOf(hist.fps) != null ? `${num(minOf(hist.fps)!, 1)} fps` : '–'}</dd>
        <dt>Panel</dt>
        <dd class="tnum">{num(perf.lcd.hz, 1)} Hz · {num(perf.lcd.pclk, 1)} MHz</dd>
        <dt>Aussetzer</dt>
        <dd class="tnum">{perf.lcd.desync}</dd>
      </dl>
    </Card>

    <Card>
      <h2 class="card-title"><Icon name="schedule" size={20} />Frame-Zeiten</h2>
      <div class="big tnum">{e ? num(e.render, 1) : '–'} <span class="unit">ms Zeichnen</span></div>
      <Sparkline series={[hist.render, hist.renderMax]} min={0} limit={20} label="Zeichenzeit, Mittel und Maximum" />
      <dl class="kv">
        <dt><span class="dot s0"></span>Mittel / <span class="dot s1"></span>Maximum</dt>
        <dd class="tnum">{e ? `${num(e.render, 2)} / ${num(e.renderMax, 2)} ms` : '–'}</dd>
        <dt>Logik</dt>
        <dd class="tnum">{e ? `${num(e.logic, 2)} ms` : '–'}</dd>
        <dt>Budget</dt>
        <dd class="tnum">20 ms (gestrichelt)</dd>
      </dl>
    </Card>

    <Card>
      <h2 class="card-title"><Icon name="memory" size={20} />CPU</h2>
      <div class="big tnum">{perf.cpu[0]} <span class="unit">%</span> · {perf.cpu[1]} <span class="unit">%</span></div>
      <Sparkline series={[hist.cpu0, hist.cpu1]} min={0} max={100} label="CPU-Last Kern 0 und Kern 1" />
      <dl class="kv">
        <dt><span class="dot s0"></span>Kern 0</dt>
        <dd>WLAN, Web, Hintergrund</dd>
        <dt><span class="dot s1"></span>Kern 1</dt>
        <dd>Grafik</dd>
        <dt>Hochskalier-Interrupt</dt>
        <dd class="tnum">{num(perf.lcd.isr, 1)} % von Kern 1</dd>
      </dl>
    </Card>

    <Card>
      <h2 class="card-title"><Icon name="storage" size={20} />Speicher</h2>
      <div class="big tnum">{perf.heap.ram} <span class="unit">KB RAM frei</span></div>
      <Sparkline series={[hist.ram]} min={0} limit={60} label="Freier interner RAM" />
      <dl class="kv">
        <dt>Interner RAM</dt>
        <dd class="tnum">{perf.heap.ram} KB frei · Tiefstwert {perf.heap.ramMin} KB · Block {perf.heap.ramBlk} KB</dd>
        <dt>PSRAM</dt>
        <dd class="tnum">{kb(perf.heap.psram)} frei · Tiefstwert {kb(perf.heap.psramMin)}</dd>
        <dt>Mindestens</dt>
        <dd class="tnum">60 KB RAM, 1 MB PSRAM</dd>
      </dl>
    </Card>

    <Card class="wide">
      <h2 class="card-title"><Icon name="list" size={20} />Tasks</h2>
      <p class="muted">Rechenzeit und freier Stack je Task, nur auf Knopfdruck.</p>
      <div class="actions">
        <Button variant="tonal" icon="list" disabled={loadingTasks} onclick={loadTasks}>{tasks ? 'Neu abrufen' : 'Task-Liste abrufen'}</Button>
      </div>
      {#if taskInfo}<p class="muted small">{taskInfo}</p>{/if}
      {#if tasks}
        <div class="table">
          <table>
            <thead><tr><th>Task</th><th>Kern</th><th>Prio</th><th>CPU</th><th>Stack frei</th><th>Zustand</th></tr></thead>
            <tbody>
              {#each tasks as t (t.name)}
                <tr>
                  <td>{t.name}</td>
                  <td class="tnum">{t.core < 0 ? 'beide' : t.core}</td>
                  <td class="tnum">{t.prio}</td>
                  <td class="tnum">{num(t.cpu, 1)} %</td>
                  <td class="tnum">{num(t.stackFree)} B</td>
                  <td>{t.state}</td>
                </tr>
              {/each}
            </tbody>
          </table>
        </div>
      {/if}
    </Card>
  {:else}
    <Card><p class="muted">{perfFailed ? 'Leistungsdaten nicht abrufbar.' : 'Lade Leistungsdaten …'}</p></Card>
  {/if}
</div>

<style>
  .screen {
    border-radius: var(--md-sys-shape-corner-medium);
    overflow: hidden;
    background: #000;
    aspect-ratio: 5 / 3;
    max-width: 800px;
  }
  .screen canvas {
    display: block;
    width: 100%;
    height: 100%;
    image-rendering: pixelated;
  }
  .screen.empty canvas {
    opacity: 0;
  }
  .big {
    font: var(--md-sys-typescale-display-small);
    font-weight: 600;
    margin: 4px 0 8px;
  }
  .unit {
    font: var(--md-sys-typescale-title-medium);
    color: var(--md-sys-color-on-surface-variant);
  }
  .dot {
    display: inline-block;
    width: 8px;
    height: 8px;
    border-radius: 50%;
    margin-right: 4px;
  }
  .dot.s0 {
    background: var(--md-sys-color-primary);
  }
  .dot.s1 {
    background: var(--md-sys-color-tertiary);
  }
  .table {
    overflow-x: auto;
  }
  table {
    width: 100%;
    border-collapse: collapse;
    font: var(--md-sys-typescale-body-medium);
  }
  th {
    text-align: left;
    font-weight: 600;
    color: var(--md-sys-color-on-surface-variant);
  }
  th,
  td {
    padding: 6px 8px;
    border-bottom: 1px solid var(--md-sys-color-outline-variant);
    white-space: nowrap;
  }
</style>
