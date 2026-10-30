<script lang="ts">
  // Seite Lager (seit 0.5.10 Teil 4b, Unterseite von Welt, Wahl des Users): Umschalter Waren | Orte. Waren: eine Zeile
  // je Ware fürs ganze Dorf, antippen zeigt, wo sie liegt (knappe Lager zuerst). Orte: Orte mit knappem oder leerem
  // Lager offen, alle anderen in einer Sammelzeile (Wahl des Users nach Beispieldorf mit 20 Orten). Die volle
  // Lager-Seite mit Zu- und Abgängen und Gruppen der Waren kommt mit 0.6.x/0.7.x. Stand alle 30 s.
  import { onDestroy } from 'svelte';
  import { ButtonGroup, Card, Icon } from '../lib/m3';
  import { loadLager, shortReach, worry, type LagerState, type StoreRow } from '../lib/world';

  let w = $state<LagerState | null>(null);
  const load = () => loadLager().then((v) => (w = v), () => null);
  load();
  const timer = window.setInterval(load, 30000);
  onDestroy(() => clearInterval(timer));

  let view = $state<'waren' | 'orte'>('waren');
  const stores = $derived(w?.lager?.stores ?? []);
  const goods = $derived(w?.lager?.goods ?? []);

  // Waren: Gruppen erst mit den echten Waren (cat), Testdorf ohne
  const cats = $derived([...new Set(goods.map((g) => g.cat ?? ''))]);
  let openGood = $state<string | null>(null);
  const where = (t: string) =>
    stores
      .map((s) => ({ s, q: s.goods.find((x) => x.t === t)?.q ?? -1 }))
      .filter((r) => r.q >= 0)
      .sort((a, b) => +worry(b.s) - +worry(a.s) || b.q - a.q);
  const goodWorries = (t: string) => stores.filter((s) => worry(s) && s.goods.length === 1 && s.goods[0].t === t).length;
  const MAXW = 8;

  // Orte: Orte mit Sorgen offen, ab 9 Lagerplätzen die übrigen in einer Sammelzeile
  type Group = { place: string; rows: StoreRow[]; worries: number; empty: number; red: boolean };
  const groups = $derived.by(() => {
    const out: Group[] = [];
    for (const s of stores) {
      let p = out.find((x) => x.place === s.place);
      if (!p) out.push((p = { place: s.place, rows: [], worries: 0, empty: 0, red: false }));
      p.rows.push(s);
      p.worries += +worry(s);
      p.empty += +(s.load === 0 && s.lv === 1);
      p.red ||= s.lv === 1;
    }
    return out;
  });
  const fold = $derived(stores.length > 8);
  let toggled = $state<Record<string, boolean>>({});
  let showCalm = $state(false);
  const isOpen = (g: Group) => !fold || (toggled[g.place] ?? g.worries > 0);
  const calm = $derived(groups.filter((g) => !g.worries));
  const shown = $derived(fold ? [...groups.filter((g) => g.worries), ...(showCalm ? calm : [])] : groups);
  const status = (g: Group) => (g.empty ? `${g.empty} leer` : g.worries ? `${g.worries} knapp` : 'alles gut');
  const worryCount = $derived(groups.reduce((n, g) => n + g.worries, 0));
  const goodsLine = (s: StoreRow) => s.goods.map((x) => `${x.t} ${x.q}${x.res ? ` (${x.res} reserviert)` : ''}`).join(' · ');
</script>

<div class="page">
  <ButtonGroup label="Ansicht" bind:value={view} options={[{ value: 'waren', label: 'Waren', icon: 'inventory_2' }, { value: 'orte', label: 'Orte', icon: 'map' }]} />

  <Card>
    {#if !w}
      <p class="muted">Lade …</p>
    {:else if !stores.length}
      <p class="empty muted"><Icon name="inventory_2" size={20} />Noch keine Lagerplätze</p>
    {:else if view === 'waren'}
      <p class="summary muted">{goods.length} Waren im Dorf · antippen zeigt, wo sie liegen</p>
      {#each cats as c (c)}
        {#if c}<span class="cat">{c}</span>{/if}
        {#each goods.filter((x) => (x.cat ?? '') === c) as x (x.t)}
          {@const n = goodWorries(x.t)}
          {@const open = openGood === x.t}
          <button class="gline" class:open aria-expanded={open} onclick={() => (openGood = open ? null : x.t)}>
            <span class="dot lv{x.lv}"></span>
            <span class="name"><b>{x.t}</b> <span class="tnum muted">{x.q}</span></span>
            <span class="end small" class:lv1={x.lv === 1} class:lv2={x.lv === 2}>
              {x.reach || (x.res ? `${x.res} reserviert` : '')}{#if n}<span class="lv1">{` · ${n} knapp`}</span>{/if}
            </span>
            <Icon name="keyboard_arrow_down" size={18} />
          </button>
          {#if open}
            {@const rows = where(x.t)}
            <div class="where">
              {#each rows.slice(0, MAXW) as r (r.s.t + r.s.place)}
                <span class="wchip" class:lv1={r.s.lv === 1 && worry(r.s)} class:lv2={r.s.lv === 2}>{r.s.place} · {r.s.t}: {r.q}</span>
              {/each}
              {#if rows.length > MAXW}<span class="wchip more">+ {rows.length - MAXW} weitere</span>{/if}
            </div>
          {/if}
        {/each}
      {/each}
    {:else}
      <p class="summary muted">
        {groups.length} {groups.length === 1 ? 'Ort' : 'Orte'} · {stores.length} Lager{#if worryCount} · <span class="warn">{worryCount} knapp oder leer</span>{/if}
      </p>
      <div class="groups" class:fold>
        {#each shown as g (g.place)}
          {@const open = isOpen(g)}
          {#if fold}
            <button class="ghead" class:open aria-expanded={open} onclick={() => (toggled = { ...toggled, [g.place]: !open })}>
              <span class="place-chip">{g.place}</span>
              <span class="gstat" class:lv1={g.red} class:lv2={g.worries && !g.red}>
                {#if !open}<span class="muted">{`${g.rows.length} Lager · `}</span>{/if}{status(g)}
              </span>
              <Icon name="keyboard_arrow_down" size={20} />
            </button>
          {:else}
            <span class="place-chip">{g.place}</span>
          {/if}
          {#if open}
            {#each g.rows as s (s.t)}
              <div class="line">
                <span class="dot lv{s.lv}"></span>
                <span class="name"><b>{s.t}</b> <span class="tnum muted">{s.load}/{s.cap}</span></span>
                <span class="end small" class:lv1={s.lv === 1} class:lv2={s.lv === 2}>{s.goods.length > 1 ? `${s.goods.length} Waren` : shortReach(s)}</span>
                {#if s.goods.length > 1}<span class="goods small muted">{goodsLine(s)}</span>{/if}
              </div>
            {/each}
          {/if}
        {/each}
        {#if fold && calm.length}
          <button class="ghead calm" class:open={showCalm} aria-expanded={showCalm} onclick={() => (showCalm = !showCalm)}>
            <span class="place-chip">{showCalm ? 'Weniger' : `${calm.length} weitere ${calm.length === 1 ? 'Ort' : 'Orte'}`}</span>
            <span class="gstat">{showCalm ? '' : 'alles gut'}</span>
            <Icon name="keyboard_arrow_down" size={20} />
          </button>
        {/if}
      </div>
    {/if}
  </Card>
</div>

<style>
  .page {
    display: grid;
    gap: 12px;
  }
  p {
    margin: 0;
  }
  .empty {
    display: flex;
    align-items: center;
    gap: 8px;
  }
  .summary {
    margin-bottom: 10px;
    font: var(--md-sys-typescale-body-medium);
  }
  .warn {
    color: var(--md-sys-color-tertiary);
    font-weight: 600;
  }
  .cat {
    display: block;
    margin: 12px 0 2px 4px;
    font: var(--md-sys-typescale-label-medium);
    font-weight: 650;
    color: var(--md-sys-color-tertiary);
  }
  .gline,
  .ghead {
    display: grid;
    gap: 10px;
    align-items: center;
    width: 100%;
    margin: 0;
    border: 0;
    border-radius: var(--md-sys-shape-corner-small);
    background: none;
    color: inherit;
    font: inherit;
    text-align: left;
    cursor: pointer;
  }
  .gline {
    grid-template-columns: 8px 1fr auto 18px;
    padding: 8px 4px;
  }
  .ghead {
    grid-template-columns: auto 1fr 20px;
    gap: 8px;
    padding: 6px 4px;
  }
  .gline:hover,
  .ghead:hover {
    background: color-mix(in srgb, var(--md-sys-color-on-surface) 6%, transparent);
  }
  .gline.open {
    background: var(--md-sys-color-surface-container-high);
  }
  .gline :global(svg),
  .ghead :global(svg) {
    transition: transform 0.2s;
    color: var(--md-sys-color-on-surface-variant);
  }
  .gline.open :global(svg),
  .ghead.open :global(svg) {
    transform: rotate(180deg);
  }
  .end {
    text-align: right;
    color: var(--md-sys-color-on-surface-variant);
  }
  .where {
    display: flex;
    flex-wrap: wrap;
    gap: 4px;
    padding: 6px 4px 10px 22px;
  }
  .wchip {
    padding: 2px 8px;
    border-radius: var(--md-sys-shape-corner-full);
    background: var(--md-sys-color-surface-container-highest);
    font: var(--md-sys-typescale-label-medium);
  }
  .wchip.more {
    background: none;
    color: var(--md-sys-color-on-surface-variant);
  }
  .groups {
    display: grid;
    gap: 2px;
  }
  .place-chip {
    justify-self: start;
    margin: 10px 0 4px;
    padding: 2px 10px;
    border-radius: var(--md-sys-shape-corner-full);
    background: var(--md-sys-color-tertiary-container);
    color: var(--md-sys-color-on-tertiary-container);
    font: var(--md-sys-typescale-label-medium);
    font-weight: 650;
  }
  .place-chip:first-child,
  .ghead .place-chip {
    margin: 0;
  }
  .fold .ghead:not(.open) .place-chip {
    background: var(--md-sys-color-surface-container-highest);
    color: var(--md-sys-color-on-surface);
  }
  .gstat {
    justify-self: end;
    font: var(--md-sys-typescale-body-small);
  }
  .line {
    display: grid;
    grid-template-columns: 8px 1fr auto;
    gap: 2px 10px;
    align-items: center;
    padding: 6px 4px;
  }
  .fold .line {
    padding-left: 14px;
  }
  .name {
    min-width: 0;
  }
  .goods {
    grid-column: 2 / 4;
  }
  .dot {
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: var(--md-sys-color-outline);
  }
  .dot.lv1 {
    background: var(--md-sys-color-error);
  }
  .dot.lv2 {
    background: var(--md-sys-color-tertiary);
  }
  .dot.lv3,
  .dot.lv4 {
    background: var(--md-sys-color-primary);
  }
  .lv1,
  .end.lv1,
  .wchip.lv1 {
    color: var(--md-sys-color-error);
  }
  .lv2,
  .end.lv2,
  .wchip.lv2 {
    color: var(--md-sys-color-tertiary);
  }
</style>
