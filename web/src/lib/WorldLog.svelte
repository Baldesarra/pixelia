<script lang="ts">
  // Speicherstand und Chronik (seit 0.5.9, Seite Welt): wann zuletzt gespeichert, „Jetzt speichern“ und was in der
  // Welt passiert ist (neueste zuerst, nach Welttag). Stand aus /api/world?mehr=1 alle 30 s.
  import { onDestroy } from 'svelte';
  import { Button, Card, Icon, type IconName } from './m3';
  import { post } from './api';
  import { agoText, loadWorld, minutesText, type ChronicleRow, type WorldState } from './world';
  import { num } from './format';

  let w = $state<(WorldState & { day?: number }) | null>(null);
  const load = () => loadWorld(true).then((v) => (w = v), () => null);
  load();
  const timer = window.setInterval(load, 30000);
  onDestroy(() => clearInterval(timer));

  let saving = $state(false);
  let msg = $state<{ ok: boolean; text: string } | null>(null);
  async function saveNow() {
    saving = true;
    msg = null;
    const r = await post('/api/world', { a: 'save' }).catch(() => null);
    msg = r?.ok ? { ok: true, text: 'Gespeichert.' } : { ok: false, text: r?.msg ?? 'Keine Antwort vom Gerät' };
    setTimeout(() => void load().finally(() => (saving = false)), 1500);
  }

  const s = $derived(w?.save);
  const blocked = $derived(!!s && (s.test || s.waiting));
  const status = $derived(
    !s ? '' : s.error ? s.error : s.waiting ? 'Wartet auf die Uhrzeit' : s.test ? 'Testuhr: es wird nicht gespeichert' : s.ago < 0 ? (s.loaded ? 'Beim Start geladen' : 'Noch nicht gespeichert') : `Gespeichert ${agoText(s.ago)}`,
  );
  const people = $derived(s ? `${s.figures} Figur${s.figures === 1 ? '' : 'en'}${s.transit ? `, ${s.transit} unterwegs` : ''}` : '');

  // Chronik nach Welttag
  const SHOW = 6;
  let all = $state(false);
  const days = $derived.by(() => {
    const out: { day: number; rows: ChronicleRow[] }[] = [];
    for (const r of (w?.chronicle ?? []).slice(0, all ? undefined : SHOW)) {
      if (out[out.length - 1]?.day !== r.day) out.push({ day: r.day, rows: [] });
      out[out.length - 1].rows.push(r);
    }
    return out;
  });
  const total = $derived(w?.chronicle?.length ?? 0);
  const today = $derived(w?.day ?? w?.chronicle?.[0]?.day ?? 0);
  const dayTitle = (d: number) => (d === today ? `Heute · Tag ${d}` : d === today - 1 ? `Gestern · Tag ${d}` : `Tag ${d}`);
  const kindIcon: Record<number, IconName> = { 1: 'celebration', 2: 'power_off', 3: 'settings_backup_restore' };
</script>


<Card>
  <h2 class="card-title"><Icon name="save" size={20} />Speicherstand</h2>
  {#if !s}
    <p class="muted">Lade …</p>
  {:else}
    <div class="hero">
      <span class="badge" class:warn={blocked || !!s.error}><Icon name={blocked || s.error ? 'warning' : 'save'} size={26} /></span>
      <span><b>{status}</b><span class="muted">{people} · {s.chronicle} Einträge Chronik</span></span>
    </div>
    <dl class="kv">
      <dt>Nächste Speicherung</dt>
      <dd>{blocked ? '–' : s.next > 0 ? `in ${Math.ceil(s.next / 60)} Min.` : 'gleich'}</dd>
      <dt>Stand</dt>
      <dd class="tnum">Nr. {s.gen} · {num(s.bytes / 1024, 1)} KB</dd>
      <dt>Beim Start</dt>
      <dd>{s.loaded ? (s.caughtUp ? `geladen, ${minutesText(s.caughtUp)} nachgeholt` : 'geladen') : 'neu begonnen'}</dd>
    </dl>
    <div class="actions">
      <Button variant="tonal" icon="save" disabled={saving || blocked} onclick={saveNow}>Jetzt speichern</Button>
    </div>
    {#if msg}<p class={msg.ok ? 'ok-text' : 'err-text'}>{msg.text}</p>{/if}
  {/if}
</Card>

<Card>
  <h2 class="card-title"><Icon name="history_edu" size={20} />Chronik</h2>
  {#if !w}
    <p class="muted">Lade …</p>
  {:else if !total}
    <p class="muted">Noch nichts passiert.</p>
  {:else}
    <div class="timeline">
      {#each days as d (d.day)}
        <span class="day-chip">{dayTitle(d.day)}</span>
        {#each d.rows as r, i (i)}
          <div class="tl-row" class:special={r.kind}>
            <span class="dot"></span>
            <span class="time tnum">{r.time}</span>
            <span>{r.text}</span>
            {#if kindIcon[r.kind]}<Icon name={kindIcon[r.kind]} size={18} />{:else}<span></span>{/if}
          </div>
        {/each}
      {/each}
    </div>
    {#if total > SHOW}
      <div class="actions">
        <Button variant="text" icon={all ? 'remove' : 'keyboard_arrow_down'} onclick={() => (all = !all)}>{all ? 'Weniger' : `Alle ${total} zeigen`}</Button>
      </div>
    {/if}
  {/if}
</Card>

<style>
  p {
    margin: 10px 0 0;
  }
  .hero {
    display: flex;
    gap: 14px;
    align-items: center;
    margin-bottom: 14px;
  }
  .hero > span:last-child {
    display: grid;
    min-width: 0;
  }
  .hero b {
    font: var(--md-sys-typescale-title-medium);
    font-weight: 650;
  }
  .badge {
    display: grid;
    place-items: center;
    flex: none;
    width: 48px;
    height: 48px;
    border-radius: var(--md-sys-shape-corner-large);
    background: var(--md-sys-color-secondary-container);
    color: var(--md-sys-color-on-secondary-container);
  }
  .badge.warn {
    background: var(--md-sys-color-error-container);
    color: var(--md-sys-color-on-error-container);
  }
  /* Zeitleiste (Wahl des Users 2026-10-28): Tag als farbiger Chip, Einträge mit Punkt an einer Linie */
  .timeline {
    display: grid;
    gap: 2px;
    padding-left: 4px;
  }
  .day-chip {
    justify-self: start;
    margin: 10px 0 4px;
    padding: 2px 10px;
    border-radius: var(--md-sys-shape-corner-full);
    background: var(--md-sys-color-tertiary-container);
    color: var(--md-sys-color-on-tertiary-container);
    font: var(--md-sys-typescale-label-medium);
    font-weight: 650;
  }
  .day-chip:first-child {
    margin-top: 0;
  }
  .tl-row {
    display: grid;
    grid-template-columns: 12px 3.2em 1fr auto;
    gap: 10px;
    align-items: baseline;
    margin-left: 5px;
    padding: 5px 0;
    border-left: 2px solid var(--md-sys-color-outline-variant);
  }
  .tl-row :global(svg) {
    align-self: center;
    color: var(--md-sys-color-on-surface-variant);
  }
  .dot {
    align-self: center;
    width: 10px;
    height: 10px;
    margin-left: -6px;
    border-radius: 50%;
    background: var(--md-sys-color-primary);
  }
  .tl-row.special .dot {
    background: var(--md-sys-color-tertiary);
  }
  .time {
    color: var(--md-sys-color-on-surface-variant);
  }
</style>
