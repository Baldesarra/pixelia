<script lang="ts">
  // Lager (seit 0.5.10 Teil 4b, Seite Welt, Wahl des Users „Sorgen zuerst“): nur was knapp oder leer ist, sonst ein
  // Satz; laufende Projekte mit Fortschritt; Knopf zur Seite Lager (Waren | Orte). Stand aus /api/world?lager=1 alle 30 s.
  import { onDestroy } from 'svelte';
  import { Button, Card, Icon, WavyProgress } from './m3';
  import { loadLager, shortReach, ware, worries, type LagerState } from './world';

  let w = $state<LagerState | null>(null);
  const load = () => loadLager().then((v) => (w = v), () => null);
  load();
  const timer = window.setInterval(load, 30000);
  onDestroy(() => clearInterval(timer));

  const stores = $derived(w?.lager?.stores ?? []);
  const projects = $derived(w?.lager?.projects ?? []);
  const worst = $derived(worries(stores));
  // Orte der gut versorgten Lager
  const places = $derived(new Set(stores.filter((s) => !worst.includes(s)).map((s) => s.place)).size);
  const SHOW = 3;
  let all = $state(false);
  const calm = $derived(stores.length - worst.length);
</script>

<Card>
  <h2 class="card-title"><Icon name="inventory_2" size={20} />Lager</h2>
  {#if !w}
    <p class="muted">Lade …</p>
  {:else}
    {#if !stores.length}
      <p class="empty muted"><Icon name="inventory_2" size={20} />Noch keine Lagerplätze</p>
    {:else}
      {#if worst.length}
        <p class="summary" class:red={worst[0].lv === 1}>
          {worst.length === 1 ? '1 Lager braucht' : `${worst.length} Lager brauchen`} Nachschub
        </p>
        {#each worst.slice(0, all ? undefined : SHOW) as s (s.t + s.place)}
          <div class="line">
            <span class="dot lv{s.lv}"></span>
            <span class="name"><b>{ware(s)}</b> <span class="muted">· {s.place}</span></span>
            <span class="end small lv{s.lv}">{shortReach(s)}</span>
          </div>
        {/each}
        {#if worst.length > SHOW}
          <Button variant="text" icon={all ? 'remove' : 'keyboard_arrow_down'} onclick={() => (all = !all)}>{all ? 'Weniger' : `Alle ${worst.length} zeigen`}</Button>
        {/if}
      {/if}
      {#if calm}
        <p class="calm">
          <Icon name="check_circle" size={20} />
          {worst.length ? 'Alle anderen' : 'Alle'}
          {calm === 1 ? 'Lager ist' : `${calm} Lager in ${places} ${places === 1 ? 'Ort' : 'Orten'} sind`} gut versorgt.
        </p>
      {/if}
    {/if}

    <h3><Icon name="construction" size={18} />In Arbeit</h3>
    {#if !projects.length}
      <p class="empty muted">Kein Projekt in Arbeit</p>
    {:else}
      <div class="projects">
        {#each projects as p (p.t + p.place)}
          <div class="project">
            <div class="head">
              <b>{p.t}</b>
              <span class="tnum qty">{p.pct} %</span>
            </div>
            <span class="muted small">{p.place} · Schritt {p.step} von {p.steps}: {p.stepT}</span>
            <WavyProgress value={p.pct / 100} label="Fortschritt {p.t}" />
            {#if p.wait}
              <span class="state lv2"><Icon name="schedule" size={16} />{p.wait}</span>
            {:else if p.who}
              <span class="state"><Icon name="construction" size={16} />{p.who} arbeitet</span>
            {/if}
          </div>
        {/each}
      </div>
    {/if}
    {#if stores.length}
      <div class="actions">
        <Button variant="tonal" icon="inventory_2" href="#/lager">Alle Lager</Button>
      </div>
    {/if}
  {/if}
</Card>

<style>
  p {
    margin: 0;
  }
  .empty {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 4px 0 8px;
  }
  .summary {
    margin-bottom: 4px;
    font: var(--md-sys-typescale-title-small);
    font-weight: 650;
    color: var(--md-sys-color-tertiary);
  }
  .summary.red {
    color: var(--md-sys-color-error);
  }
  .line {
    display: grid;
    grid-template-columns: 8px 1fr auto;
    gap: 10px;
    align-items: center;
    padding: 6px 4px;
  }
  .name {
    min-width: 0;
  }
  .end {
    text-align: right;
    white-space: nowrap;
  }
  .dot {
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: var(--md-sys-color-tertiary);
  }
  .dot.lv1 {
    background: var(--md-sys-color-error);
  }
  .lv1 {
    color: var(--md-sys-color-error);
  }
  .lv2 {
    color: var(--md-sys-color-tertiary);
  }
  .calm {
    display: flex;
    gap: 8px;
    align-items: center;
    margin-top: 8px;
    padding: 10px 12px;
    border-radius: var(--md-sys-shape-corner-medium);
    background: var(--md-sys-color-secondary-container);
    color: var(--md-sys-color-on-secondary-container);
    font: var(--md-sys-typescale-body-medium);
  }
  .calm :global(svg) {
    flex: none;
  }
  h3 {
    display: flex;
    align-items: center;
    gap: 6px;
    margin: 20px 0 8px;
    font: var(--md-sys-typescale-label-large);
    font-weight: 700;
    color: var(--md-sys-color-primary);
  }
  .projects {
    display: grid;
    gap: 8px;
  }
  .project {
    display: grid;
    gap: 4px;
    padding: 12px 14px;
    border-radius: var(--md-sys-shape-corner-large);
    background: var(--md-sys-color-surface-container-high);
  }
  .head {
    display: flex;
    justify-content: space-between;
    align-items: baseline;
    gap: 8px;
  }
  .head b {
    font: var(--md-sys-typescale-title-small);
    font-weight: 650;
  }
  .qty {
    font-weight: 600;
  }
  .state {
    display: flex;
    align-items: center;
    gap: 6px;
    font: var(--md-sys-typescale-body-medium);
  }
</style>
