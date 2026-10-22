<script lang="ts">
  // Karte „Anzeige folgt“ (seit 0.5.6, Seite Welt): Standard der Kamera (Hauptfigur, eine Figur oder ein fester Ort),
  // Rückkehr nach einer Zeit ohne Eingabe, Wechsel zu einem Ort (Modus Frei). Zustand aus /api/camera alle 2 s.
  import { onDestroy } from 'svelte';
  import { Button, ButtonGroup, Card, Icon } from './m3';
  import { device } from './status.svelte';
  import { get, post } from './api';

  type Fig = { id: number; name: string; place: number; lead: boolean };
  type Cam = {
    mode: number;
    target: number;
    targetName: string;
    place: number;
    returnIn: number;
    places: { id: string; title: string }[];
    figures: Fig[];
  };

  let cam = $state<Cam | null>(null);
  let timer = 0;
  async function load() {
    cam = await get<Cam>('/api/camera').catch(() => cam);
  }
  load();
  timer = window.setInterval(load, 2000);
  onDestroy(() => clearInterval(timer));

  const set = $derived(device.data?.set);
  // Lokale Wahl gilt sofort, bis das Gerät sie bestätigt
  let local = $state<{ home?: string; fig?: number; place?: number; ret?: string }>({});
  const home = $derived(local.home ?? String(set?.camHome ?? 0));
  const fig = $derived(local.fig ?? set?.camFig ?? 0);
  const place = $derived(local.place ?? set?.camPlace ?? 0);
  const ret = $derived(local.ret ?? String(set?.camRet ?? 600));

  function save(patch: Record<string, string | number>, keep: typeof local) {
    local = { ...local, ...keep };
    void post('/api/settings', patch)
      .catch(() => null)
      .finally(() => setTimeout(() => (local = {}), 2500));
  }
  // Zwei Knöpfe: Figur (Hauptfigur oder eine andere, Auswahl darunter) oder fester Ort
  const kind = $derived(home === '2' ? 'ort' : 'figur');
  function setKind(v: string) {
    if (v === 'ort') save({ camHome: 2, camPlace: cam?.place ?? 0 }, { home: '2', place: cam?.place ?? 0 });
    else save({ camHome: 0 }, { home: '0' });
  }
  // Auswahl der Figur: 0 = Hauptfigur (folgt ihr auch, wenn sie später eine andere ist), sonst die Kennung
  const figChoice = $derived(home === '1' ? fig : 0);
  function setFigure(v: number) {
    if (v === 0) save({ camHome: 0 }, { home: '0' });
    else save({ camHome: 1, camFig: v }, { home: '1', fig: v });
  }
  const placeTitle = (i: number) => cam?.places[i]?.title ?? '–';
  const status = $derived.by(() => {
    if (!cam) return '';
    const back = cam.returnIn >= 0 ? ` · zurück in ${Math.max(1, Math.ceil(cam.returnIn / 60))} min` : '';
    if (cam.mode === 1) return `Frei: ${placeTitle(cam.place)}${back}`;
    if (cam.mode === 2) return `Folgt ${cam.targetName || 'Figur'}${back}`;
    if (cam.target) return `Folgt ${cam.targetName || 'Figur'} · ${placeTitle(cam.place)}`;
    return `Bleibt an ${placeTitle(cam.place)}`;
  });
</script>

<Card>
  <h2 class="card-title"><Icon name="visibility" size={20} />Anzeige folgt</h2>
  {#if cam}
    <p class="status"><span class="dot" class:free={cam.mode !== 0}></span>{status}</p>
    <div class="setting">
      <span>Standard</span>
      <ButtonGroup
        label="Standard der Anzeige"
        options={[
          { value: 'figur', label: 'Figur' },
          { value: 'ort', label: 'Ort' },
        ]}
        bind:value={() => kind, setKind}
      />
      {#if kind === 'figur'}
        <select
          aria-label="Figur"
          value={figChoice}
          onchange={(e) => setFigure(Number((e.currentTarget as HTMLSelectElement).value))}
        >
          <option value={0}>Hauptfigur{cam.figures[0]?.lead ? ' (' + cam.figures[0].name + ')' : ''}</option>
          {#each cam.figures.filter((f) => !f.lead) as f (f.id)}
            <option value={f.id}>{f.name} · {placeTitle(f.place)}</option>
          {/each}
        </select>
      {:else}
        <select
          aria-label="Ort"
          value={place}
          onchange={(e) => {
            const v = Number((e.currentTarget as HTMLSelectElement).value);
            save({ camPlace: v }, { place: v });
          }}
        >
          {#each cam.places as p, i (p.id)}
            <option value={i}>{p.title}</option>
          {/each}
        </select>
      {/if}
    </div>
    <div class="setting">
      <span>Zurück zum Standard nach (Minuten)</span>
      <ButtonGroup
        label="Rückkehr"
        options={[
          { value: '600', label: '10' },
          { value: '1800', label: '30' },
          { value: '0', label: 'Nie' },
        ]}
        bind:value={() => ret, (v) => save({ camRet: v }, { ret: v })}
      />
    </div>
    <div class="setting">
      <span>Jetzt zeigen</span>
      <div class="places">
        {#each cam.places as p, i (p.id)}
          <Button variant={i === cam.place ? 'filled' : 'tonal'} onclick={() => post('/api/camera', { a: 'go', v: i }).then(load)}>
            {p.title}
          </Button>
        {/each}
      </div>
    </div>
    <div class="actions">
      <Button variant="outlined" icon="undo" disabled={cam.mode === 0} onclick={() => post('/api/camera', { a: 'home' }).then(load)}>
        Zum Standard
      </Button>
    </div>
    <p class="hint">Am Display: wischen = nächster Ort, Figur antippen = Karte, lange halten = folgen.</p>
  {:else}
    <p class="hint">Lade …</p>
  {/if}
</Card>

<style>
  .status {
    display: flex;
    align-items: center;
    gap: 8px;
    margin: 0 0 16px;
    font-weight: 600;
  }
  .dot {
    width: 10px;
    height: 10px;
    border-radius: 50%;
    background: var(--md-sys-color-primary);
    flex: none;
  }
  .dot.free {
    background: var(--md-sys-color-tertiary);
  }
  .setting {
    display: grid;
    gap: 10px;
    margin-bottom: 18px;
    color: var(--md-sys-color-on-surface-variant);
  }
  select {
    font: inherit;
    padding: 10px 12px;
    border-radius: 12px;
    border: 1px solid var(--md-sys-color-outline);
    background: var(--md-sys-color-surface-container-high);
    color: var(--md-sys-color-on-surface);
    min-width: 0;
    width: 100%;
  }
  .places {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(130px, 1fr));
    gap: 8px;
  }
  .hint {
    margin: 12px 0 0;
    color: var(--md-sys-color-on-surface-variant);
    font-size: 0.875rem;
  }
</style>
