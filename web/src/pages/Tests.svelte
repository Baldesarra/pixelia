<script lang="ts">
  // Seite „Gerätetests“ (System → Labor, seit 0.5.4 ohne Experimentell-Schalter): Testbilder auf dem Display, eigenes
  // Testbild, Touch-Test. Stand und Aktionen über lib/exp.svelte.ts (/api/exp), wie auf der Seite Experimentell.
  import { onMount } from 'svelte';
  import { Button, Card, Chip, Icon } from '../lib/m3';
  import { post } from '../lib/api';
  import { exp } from '../lib/exp.svelte';
  import { num } from '../lib/format';
  import { preparePicture, type Picture } from '../lib/picture';

  const ex = $derived(exp.ex);
  const busy = $derived(exp.busy);
  const act = exp.act;
  onMount(exp.poll);

  const tests = [
    { mode: 'bars', label: 'Farbbalken', icon: 'palette' },
    { mode: 'frame', label: 'Rahmen', icon: 'crop_square' },
    { mode: 'motion', label: 'Bewegung', icon: 'motion_photos_on' },
  ] as const;

  // --- Eigenes Testbild (seit 0.5.0) ---------------------------------------------------------------------
  let fileInput = $state<HTMLInputElement>();
  let pic = $state<(Picture & { name: string }) | null>(null);
  let picCanvas = $state<HTMLCanvasElement>();
  let picErr = $state('');
  async function choosePicture(e: Event) {
    const f = (e.currentTarget as HTMLInputElement).files?.[0];
    if (!f) return;
    picErr = '';
    try {
      pic = { ...(await preparePicture(f)), name: f.name };
    } catch {
      pic = null;
      picErr = 'Das Bild ließ sich nicht öffnen.';
    }
  }
  $effect(() => {
    if (pic && picCanvas) picCanvas.getContext('2d')!.putImageData(pic.rgba, 0, 0);
  });
  async function showPicture() {
    if (!pic) return;
    exp.busy = true;
    exp.msg = null;
    const r = await post('/api/picture', pic.body).catch(() => null);
    exp.busy = false;
    exp.msg = r?.ok ? { ok: true, text: 'Testbild auf dem Display. Tippen aufs Display beendet es.' } : { ok: false, text: r?.msg ?? 'Pixelia antwortet nicht' };
    await exp.load();
  }

  const cornerNames = ['oben links', 'oben rechts', 'unten links', 'unten rechts'];
</script>

{#if !ex}
  <Card><p class="muted">{exp.failed ? 'Pixelia antwortet nicht.' : 'Lade …'}</p></Card>
{:else}
  <div class="page-grid">
    {#if exp.msg}<p class="wide {exp.msg.ok ? 'ok-text' : 'err-text'}">{exp.msg.text}</p>{/if}
  <Card class="wide">
    <h2 class="card-title"><Icon name="monitor" size={20} />Displaytests</h2>
    <p class="muted">Testbilder auf dem Display; nach {60} s läuft die Welt von selbst weiter.</p>
    <div class="actions">
      {#each tests as t (t.mode)}
        <Button variant={ex.mode === t.mode ? 'filled' : 'tonal'} disabled={busy || ex.trialS >= 0} onclick={() => act({ a: 'test', mode: t.mode })}>
          {t.label}
        </Button>
      {/each}
      {#if ex.mode !== 'engine'}
        <Button variant="text" icon="close" onclick={() => act({ a: 'test', mode: 'engine' })}>Zurück zur Welt</Button>
      {/if}
    </div>
    {#if ex.testS >= 0}<p class="muted small tnum">Testbild „{ex.mode}“ noch {ex.testS} s</p>{/if}
  </Card>

  <Card class="wide">
    <h2 class="card-title">
      <Icon name="image" size={20} />Eigenes Testbild
      {#if ex.pic}<span class="end"><Chip pulse tone="primary">auf dem Display</Chip></span>{/if}
    </h2>
    <p class="muted">
      Ein Bild (400 × 240, andere Größen werden angepasst) bleibt bis zum Tippen aufs Display oder „Beenden“, höchstens 30 min. Es wird nicht gespeichert.
    </p>
    <input type="file" accept="image/*" hidden bind:this={fileInput} onchange={choosePicture} />
    <div class="actions">
      <Button variant="tonal" icon="upload_file" disabled={busy} onclick={() => fileInput?.click()}>Bild wählen</Button>
      {#if pic}
        <Button icon="monitor" disabled={busy || ex.trialS >= 0} onclick={showPicture}>Auf dem Display zeigen</Button>
      {/if}
      {#if ex.pic}<Button variant="text" icon="close" onclick={() => act({ a: 'picoff' })}>Beenden</Button>{/if}
    </div>
    {#if picErr}<p class="err-text">{picErr}</p>{/if}
    {#if pic}
      <canvas class="pic" width="400" height="240" bind:this={picCanvas}></canvas>
      <p class="muted small tnum">
        {pic.name} · {pic.srcW} × {pic.srcH}{pic.srcW !== 400 || pic.srcH !== 240 ? ' → angepasst auf 400 × 240' : ''} ·
        {pic.colors} Farben{pic.srcColors > pic.colors ? ` (reduziert von ${num(pic.srcColors, 0)})` : ''}
      </p>
    {/if}
  </Card>

  <Card class="wide">
    <h2 class="card-title">
      <Icon name="touch_app" size={20} />Touch-Test
      {#if ex.mode === 'touch'}<span class="end"><Chip pulse tone="primary">läuft · noch {ex.testS} s</Chip></span>{/if}
    </h2>
    {#if !ex.touch.ok}
      <p class="err-text">Kein Touch-Controller gefunden.</p>
    {:else}
      <p class="muted">
        Display berühren: Der Punkt zeigt die gemessene Stelle. Alle vier Ecken antippen.
      </p>
      <div class="touch" class:live={ex.mode === 'touch'}>
        {#each [0, 1, 2, 3] as c (c)}
          <span class="corner c{c}" class:hit={(ex.touch.corners >> c) & 1} title={cornerNames[c]}></span>
        {/each}
        {#if ex.touch.x >= 0}
          <span class="finger" class:down={ex.touch.down} style:left="{(ex.touch.x / 800) * 100}%" style:top="{(ex.touch.y / 480) * 100}%"></span>
        {/if}
      </div>
      <p class="muted small tnum">
        {ex.touch.x >= 0 ? `Letzter Punkt x ${ex.touch.x}, y ${ex.touch.y}` : 'Noch keine Berührung'} · {ex.touch.n} Berührungen seit
        dem Start · Ecken {[0, 1, 2, 3].filter((c) => (ex!.touch.corners >> c) & 1).length} von 4
      </p>
      <div class="actions">
        <Button variant={ex.mode === 'touch' ? 'tonal' : 'filled'} icon="touch_app" disabled={busy || ex.trialS >= 0} onclick={() => act({ a: 'test', mode: 'touch' })}>
          {ex.mode === 'touch' ? 'Neu starten' : 'Touch-Test starten'}
        </Button>
        {#if ex.mode === 'touch'}<Button variant="text" icon="close" onclick={() => act({ a: 'test', mode: 'engine' })}>Beenden</Button>{/if}
      </div>
    {/if}
  </Card>
  </div>
{/if}

<style>
  p.wide {
    margin: 0;
  }
  .touch {
    position: relative;
    aspect-ratio: 5 / 3;
    max-width: 480px;
    margin: 12px 0;
    border-radius: var(--md-sys-shape-corner-medium);
    background: var(--md-sys-color-surface-container-highest);
    outline: 2px dashed var(--md-sys-color-outline-variant);
    overflow: hidden;
  }
  .touch.live {
    outline-color: var(--md-sys-color-primary);
  }
  .corner {
    position: absolute;
    width: 12.5%;
    height: 20.8%;
    background: color-mix(in srgb, var(--md-sys-color-outline) 25%, transparent);
  }
  .corner.hit {
    background: color-mix(in srgb, var(--md-sys-color-primary) 55%, transparent);
  }
  .c0 {
    left: 0;
    top: 0;
  }
  .c1 {
    right: 0;
    top: 0;
  }
  .c2 {
    left: 0;
    bottom: 0;
  }
  .c3 {
    right: 0;
    bottom: 0;
  }
  .finger {
    position: absolute;
    width: 24px;
    height: 24px;
    margin: -12px 0 0 -12px;
    border-radius: 50%;
    border: 3px solid var(--md-sys-color-tertiary);
    transition:
      left 0.2s linear,
      top 0.2s linear;
  }
  .finger.down {
    background: var(--md-sys-color-tertiary);
  }
  .pic {
    display: block;
    width: 100%;
    max-width: 800px;
    aspect-ratio: 5 / 3;
    margin: 12px 0 4px;
    border-radius: var(--md-sys-shape-corner-medium);
    image-rendering: pixelated;
    background: #000;
  }
</style>
