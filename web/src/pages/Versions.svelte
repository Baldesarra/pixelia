<script lang="ts">
  // Seite „Versionsverlauf“: alle Versionen aus dem in die Firmware eingebetteten CHANGELOG.json als Timeline, die
  // installierte markiert. Seit 0.5.4 nur die Kurzfassung; aufgeklappt sind die neueste und die installierte Version.
  import { onMount } from 'svelte';
  import { Button, Card, Chip, Icon, LoadingIndicator } from '../lib/m3';
  import ChangeEntry from '../lib/ChangeEntry.svelte';
  import { device } from '../lib/status.svelte';
  import { cmpVersion, installedEntry, installedLog, REPO_URL, type ChangeEntry as Entry } from '../lib/changelog';

  let log = $state<Entry[] | null>(null);
  let failed = $state(false);
  let open = $state<Record<string, boolean>>({});

  const fw = $derived(device.data?.v ?? '');
  const current = $derived(log && fw ? installedEntry(log, fw) : '');
  // Unveröffentlichte Versionen (ohne Datum, z. B. 0.5.4 während der Vorschau) nur, wenn sie installiert sind
  const shown = $derived(log?.filter((e) => e.d || e.v === current) ?? null);
  const allOpen = $derived(!!shown?.length && shown.every((e) => open[e.v]));

  onMount(async () => {
    try {
      log = await installedLog();
    } catch {
      failed = true;
    }
  });
  // Startzustand der Klappen, sobald Verlauf und Version da sind
  let seeded = false;
  $effect(() => {
    if (seeded || !shown || !fw) return;
    seeded = true;
    open = Object.fromEntries(shown.map((e, k) => [e.v, k === 0 || e.v === current]));
  });

  function markOf(e: Entry): string {
    if (e.v !== current) return '';
    return e.d ? 'Installiert' : 'Installiert · Entwicklungsstand';
  }
  const setAll = (v: boolean) => (open = Object.fromEntries((shown ?? []).map((e) => [e.v, v])));
</script>

<div class="page-grid">
  <Card class="wide">
    <h2 class="card-title">
      <Icon name="history" size={20} />Versionen
      <span class="end"><Chip tone="primary" icon="check_circle"><span class="tnum">installiert: {fw || '…'}</span></Chip></span>
    </h2>
    <p class="muted">
      Was sich in jeder Version geändert hat, kurz gefasst. Alle Details mit Messwerten stehen bei jeder Version auf
      GitHub; vor einem Update zeigt die Seite Update, was die neue Version bringt.
    </p>
    <div class="actions">
      <Button variant="tonal" icon="open_in_new" href="{REPO_URL}/releases" target="_blank" rel="noopener">Releases auf GitHub</Button>
      {#if shown?.length}
        <Button variant="text" icon="keyboard_arrow_down" onclick={() => setAll(!allOpen)}>{allOpen ? 'Alle zuklappen' : 'Alle aufklappen'}</Button>
        <span class="muted small tnum count">{shown.filter((e) => e.d).length} veröffentlicht</span>
      {/if}
    </div>
  </Card>

  {#if failed}
    <Card variant="error" class="wide">
      <p class="plain">Der Versionsverlauf ließ sich nicht laden. Ältere Firmware (vor 0.4.0) enthält ihn noch nicht.</p>
    </Card>
  {:else if !shown}
    <div class="wide waiting"><LoadingIndicator size={56} label="Lade" /></div>
  {:else}
    <ol class="timeline wide">
      {#each shown as e, k (e.v)}
        {@const newer = !!current && cmpVersion(e.v, current) > 0}
        <li class:current={e.v === current} class:newer class:last={k === shown.length - 1}>
          <span class="rail" aria-hidden="true"><span class="dot"></span></span>
          <Card variant={e.v === current ? 'elevated' : 'filled'} shape={e.v === current ? 'extra-large' : 'large'} class="tl-card">
            <ChangeEntry entry={e} mark={markOf(e)} toggle bind:open={() => open[e.v] ?? false, (v) => (open[e.v] = v)} />
          </Card>
        </li>
      {/each}
    </ol>
  {/if}
</div>

<style>
  .waiting {
    display: grid;
    place-items: center;
    padding: 48px 0;
  }
  p {
    margin: 0;
  }
  .count {
    margin-left: auto;
  }
  .timeline {
    display: grid;
    gap: 12px;
    margin: 0;
    padding: 0;
    list-style: none;
  }
  .timeline li {
    position: relative;
    display: grid;
    grid-template-columns: 28px 1fr;
    gap: 8px;
  }
  /* Linie durch alle Punkte; beim letzten Eintrag endet sie am Punkt */
  .rail {
    position: relative;
    display: flex;
    justify-content: center;
  }
  .rail::before {
    content: '';
    position: absolute;
    top: 0;
    bottom: -12px;
    width: 2px;
    background: var(--md-sys-color-outline-variant);
  }
  li:first-child .rail::before {
    top: 28px;
  }
  .last .rail::before {
    bottom: calc(100% - 28px);
  }
  .dot {
    position: relative;
    margin-top: 22px;
    width: 12px;
    height: 12px;
    border-radius: 50%;
    background: var(--md-sys-color-outline);
    box-shadow: 0 0 0 4px var(--md-sys-color-surface);
  }
  .newer .dot {
    background: var(--md-sys-color-tertiary);
  }
  .current .dot {
    margin-top: 18px;
    width: 20px;
    height: 20px;
    border-radius: 30%;
    background: var(--md-sys-color-primary);
    transform: rotate(45deg);
  }
  .timeline :global(.tl-card) {
    min-width: 0;
  }
  @media (min-width: 600px) {
    .timeline li {
      grid-template-columns: 40px 1fr;
    }
  }
</style>
