<script lang="ts">
  // Figurenliste (seit 0.5.9, Experimentell → Orte): wer wo ist, was sie gerade tut und was sie kann. Steckbriefe
  // (Wahl des Users 2026-10-28): Ort als Farbe links, Tätigkeit als Chip, Fähigkeiten mit Stufen-Balken wie am Display.
  // Stand aus /api/world?mehr=1 alle 10 s.
  import { onDestroy } from 'svelte';
  import { Card, Icon, type IconName } from './m3';
  import SkillMeter from './SkillMeter.svelte';
  import { loadWorld, type FigureRow } from './world';

  let figs = $state<FigureRow[] | null>(null);
  const load = () => loadWorld(true).then((v) => (figs = v.figures ?? []), () => null);
  load();
  const timer = window.setInterval(load, 10000);
  onDestroy(() => clearInterval(timer));

  const doingIcon = (f: FigureRow): IconName =>
    f.moving
      ? 'hiking'
      : f.doing === 'geht umher'
        ? 'directions_walk'
        : f.doing === 'ist beschäftigt'
          ? 'construction'
          : f.doing === 'geht hinaus'
            ? 'logout'
            : 'accessibility_new';
  // Farbe je Ort in der Reihenfolge des ersten Auftretens; unterwegs neutral
  const tones = ['primary', 'secondary', 'tertiary', 'neutral'];
  const places = $derived([...new Set((figs ?? []).filter((f) => !f.moving).map((f) => f.place))]);
  const tone = (f: FigureRow) => (f.moving ? 'neutral' : tones[places.indexOf(f.place) % tones.length]);
  const best = (f: FigureRow) => [...f.skills].sort((a, b) => b.n - a.n || (b.p ?? 0) - (a.p ?? 0));
</script>

<Card class="wide">
  <h2 class="card-title"><Icon name="group" size={20} />Figuren{figs ? ` (${figs.length})` : ''}</h2>
  {#if !figs}
    <p class="muted">Lade …</p>
  {:else if !figs.length}
    <p class="muted">Noch keine Figuren.</p>
  {:else}
    <ul class="people">
      {#each figs as f (f.id)}
        <li>
          <span class="av tone-{tone(f)}">{f.lead ? '★' : f.name.replace(/\D+/g, '') || f.name[0]}</span>
          <span class="who"><b>{f.name}</b><span class="muted small">{f.place}</span></span>
          <span class="doing"><Icon name={doingIcon(f)} size={16} />{f.moving ? 'unterwegs' : f.doing || '–'}</span>
          {#if f.skills.length}
            <span class="skills">
              {#each best(f) as k (k.t)}
                <span class="sk"><span class="name">{k.t}</span><SkillMeter level={k.n} progress={k.p ?? 0} title={k.l} /></span>
              {/each}
            </span>
          {/if}
        </li>
      {/each}
    </ul>
  {/if}
</Card>

<style>
  p {
    margin: 0;
  }
  .small {
    font-size: 0.8125rem;
  }
  .people {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(min(100%, 300px), 1fr));
    gap: 6px;
    margin: 0;
    padding: 0;
    list-style: none;
  }
  li {
    display: grid;
    grid-template-columns: 40px 1fr auto;
    gap: 4px 12px;
    align-content: start;
    align-items: center;
    padding: 10px 12px 10px 10px;
    border-radius: var(--md-sys-shape-corner-large);
    background: var(--md-sys-color-surface-container-high);
  }
  .av {
    display: grid;
    place-items: center;
    width: 40px;
    height: 40px;
    border-radius: var(--md-sys-shape-corner-medium);
    background: var(--c);
    color: var(--on);
    font-weight: 700;
  }
  .tone-primary {
    --c: var(--md-sys-color-primary-container);
    --on: var(--md-sys-color-on-primary-container);
  }
  .tone-secondary {
    --c: var(--md-sys-color-secondary-container);
    --on: var(--md-sys-color-on-secondary-container);
  }
  .tone-tertiary {
    --c: var(--md-sys-color-tertiary-container);
    --on: var(--md-sys-color-on-tertiary-container);
  }
  .tone-neutral {
    --c: var(--md-sys-color-surface-container-highest);
    --on: var(--md-sys-color-on-surface-variant);
  }
  .who {
    display: grid;
    min-width: 0;
  }
  .doing {
    display: inline-flex;
    gap: 4px;
    align-items: center;
    padding: 3px 10px 3px 6px;
    border-radius: var(--md-sys-shape-corner-full);
    background: var(--md-sys-color-surface-container-lowest);
    font: var(--md-sys-typescale-label-medium);
    white-space: nowrap;
  }
  .skills {
    display: grid;
    grid-column: 2 / -1;
    gap: 3px;
    max-width: 360px;
  }
  .sk {
    display: flex;
    gap: 8px;
    align-items: center;
    justify-content: space-between;
    min-width: 0;
    font-size: 0.8125rem;
    color: var(--md-sys-color-on-surface-variant);
  }
  .name {
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }
</style>
