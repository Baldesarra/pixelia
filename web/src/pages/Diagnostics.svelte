<script lang="ts">
  // Seite „Diagnose“: Absturzbericht mit allen Feldern (Rohdaten, Löschen mit PIN), dieser Start, Diagnosepaket und
  // die Reset-Historie der letzten 16 Starts mit Bilanz. Daten aus /api/coredump/summary und /api/resets (0.2.2).
  import { onMount } from 'svelte';
  import { Button, Card, Chip, confirmDialog, Icon, IconButton, LoadingIndicator, type IconName } from '../lib/m3';
  import { device } from '../lib/status.svelte';
  import { get, post, type CrashSummary, type ResetList } from '../lib/api';
  import { duration, num } from '../lib/format';

  type Reset = ResetList['entries'][number];
  const s = $derived(device.data);

  let crash = $state<CrashSummary | null>(null);
  let resets = $state<ResetList | null>(null);
  let loading = $state(false);
  let eraseMsg = $state<{ ok: boolean; text: string } | null>(null);
  let copied = $state(false);

  async function load() {
    loading = true;
    [crash, resets] = await Promise.all([
      get<CrashSummary>('/api/coredump/summary').catch(() => null),
      get<ResetList>('/api/resets').catch(() => null),
    ]);
    loading = false;
  }
  onMount(() => void load());
  // Neuer Absturzbericht oder Neustart (Startnummer): neu laden
  let seen = '';
  $effect(() => {
    const k = s ? `${s.diag.boot}/${s.diag.crash?.task ?? '-'}` : '';
    if (k && seen && k !== seen) void load();
    seen = k;
  });

  // Ausnahmegründe des Xtensa-Kerns (EXCCAUSE), nur die häufigen
  const EXC: Record<number, string> = {
    0: 'Ungültiger Befehl',
    2: 'Fehler beim Befehlsabruf',
    3: 'Fehler beim Laden/Speichern',
    6: 'Division durch null',
    9: 'Unausgerichteter Zugriff',
    20: 'Befehlsabruf verboten',
    28: 'Lesen verboten (z. B. Nullzeiger)',
    29: 'Schreiben verboten (z. B. Nullzeiger)',
  };

  async function eraseCrash() {
    const ok = await confirmDialog({
      title: 'Absturzbericht löschen?',
      text: 'Vorher am besten das Diagnosepaket oder die Rohdaten herunterladen: Sie enthalten den Bericht zum Auswerten.',
      ok: 'Löschen',
      icon: 'delete',
      danger: true,
    });
    if (!ok) return;
    const r = await post('/api/coredump/erase').catch(() => null);
    eraseMsg = { ok: !!r?.ok, text: r?.msg ?? 'Keine Antwort vom Gerät' };
    void load();
  }

  async function copyBacktrace() {
    if (!crash?.backtrace) return;
    const text = `Pixelia ${crash.version ?? ''} · ${crash.task ?? ''} · ${crash.reason ?? ''}\nPC ${crash.pc}\nBacktrace: ${crash.backtrace.join(' ')}`;
    try {
      await navigator.clipboard.writeText(text);
      copied = true;
      setTimeout(() => (copied = false), 2000);
    } catch {
      /* ohne sicheren Kontext (http) gibt es keine Zwischenablage */
    }
  }

  // Art des Starts: Absturz, Watchdog, Unterspannung, Rückfall oder normal (ESP_RST_* in esp_system.h)
  function kind(r: Reset): { icon: IconName; bad: boolean } {
    if (r.crash || r.reason === 4) return { icon: 'bug_report', bad: true };
    if (r.rollback) return { icon: 'error', bad: true };
    if (r.reason >= 5 && r.reason <= 7) return { icon: 'schedule', bad: true };
    if (r.reason === 9) return { icon: 'warning', bad: true };
    if (r.reason === 1) return { icon: 'power_settings_new', bad: false };
    return { icon: 'restart_alt', bad: false };
  }
  const tally = $derived.by(() => {
    const e = resets?.entries ?? [];
    return {
      crashes: e.filter((r) => r.crash || r.reason === 4).length,
      watchdogs: e.filter((r) => r.reason >= 5 && r.reason <= 7).length,
      brownouts: e.filter((r) => r.reason === 9).length,
      rollbacks: e.filter((r) => r.rollback).length,
      trials: e.filter((r) => r.trial).length,
    };
  });
  const details = (r: Reset) =>
    [
      r.prevUptime !== null ? `lief davor ${duration(r.prevUptime)}` : '',
      // letztes Lebenszeichen davor: nur ohne eigene Startzeit nötig, sonst fast dieselbe Uhrzeit
      r.prevTime && !r.time ? `bis ${r.prevTime}` : '',
      r.trial ? 'Probelauf' : '',
      r.rollback ? 'Rückfall auf die vorige Version' : '',
      r.crash ? `Absturz in „${r.crash.task}“ bei ${r.crash.pc}` : '',
      r.note,
    ]
      .filter(Boolean)
      .join(' · ');

  // Neustarts: die letzten drei, der Rest aufklappbar (Wunsch des Users, 0.5.4)
  const SHOWN = 3;
  let allResets = $state(false);
</script>

{#if !s}
  <div class="waiting"><LoadingIndicator size={64} label="Lade" /></div>
{:else}
  <div class="page-grid">
    <Card class="wide" variant={crash?.present ? 'outlined' : 'filled'}>
      <h2 class="card-title">
        <Icon name="bug_report" size={20} />Absturzbericht
        <span class="end">
          {#if crash?.present}
            <Chip tone="error" icon="error">vorhanden</Chip>
          {:else if crash}
            <Chip tone="primary" icon="check_circle">keiner</Chip>
          {/if}
        </span>
      </h2>
      {#if !crash}
        <p class="muted">{loading ? 'Lade …' : 'Nicht abrufbar.'}</p>
      {:else if !crash.present}
        <p class="muted">
          Kein Absturzbericht gespeichert.
        </p>
      {:else}
        <p class="lead-text">
          Absturz in Task <b>„{crash.task}“</b>{crash.reason ? `: ${crash.reason}` : ''}.
        </p>
        <dl class="kv">
          <dt>Firmware</dt>
          <dd class="tnum">
            {crash.version || 'unbekannt'}{crash.part ? ` · ${crash.part}` : ''}
            {#if crash.version && crash.version !== s.v}<span class="err-text"> (nicht die installierte {s.v})</span>{/if}
          </dd>
          <dt>Programmzähler</dt>
          <dd class="mono">{crash.pc}</dd>
          {#if crash.excCause !== undefined}
            <dt>Ausnahme</dt>
            <dd>
              {EXC[crash.excCause] ?? `Ursache ${crash.excCause}`}{crash.excVaddr && crash.excVaddr !== '0x00000000' ? ` · Adresse ${crash.excVaddr}` : ''}
            </dd>
          {/if}
          <dt>Bericht</dt>
          <dd class="tnum">
            {num((crash.size ?? 0) / 1024, 1)} KB bei {crash.addr}{crash.valid === false ? ' · Prüfsumme falsch' : ' · gültig'}
          </dd>
          {#if crash.elfSha}
            <dt>ELF-Prüfsumme</dt>
            <dd class="mono">{crash.elfSha.slice(0, 16)}…</dd>
          {/if}
        </dl>
        {#if crash.backtrace?.length}
          <div class="bt-head">
            <h3>Backtrace</h3>
            <IconButton icon={copied ? 'check' : 'content_copy'} label="Backtrace kopieren" onclick={copyBacktrace} />
          </div>
          <ol class="bt mono">
            {#each crash.backtrace as a, k (k)}<li>{a}</li>{/each}
          </ol>
          {#if crash.btCorrupted}<p class="err-text small">Der Backtrace ist beschädigt und endet vorzeitig.</p>{/if}
        {/if}
        <div class="actions">
          <Button icon="download" href="/api/coredump" download>Rohdaten (.bin)</Button>
          <Button variant="text" class="danger" icon="delete" onclick={eraseCrash}>Bericht löschen</Button>
        </div>
        <p class="muted small">
          Auswerten am PC mit <code>tools/crash.ps1</code>.
        </p>
      {/if}
      {#if eraseMsg}<p class={eraseMsg.ok ? 'ok-text' : 'err-text'}>{eraseMsg.text}</p>{/if}
    </Card>

    <Card>
      <h2 class="card-title"><Icon name="monitor_heart" size={20} />Dieser Start</h2>
      <dl class="kv">
        <dt>Start</dt>
        <dd class="tnum">Nr. {s.diag.boot} · {s.diag.reset}</dd>
        <dt>Läuft seit</dt>
        <dd class="tnum">{duration(s.uptime)}</dd>
        <dt>Seit dem Start</dt>
        <dd class="tnum">{s.diag.errors} Fehler · {s.diag.warnings} Warnungen</dd>
        <dt>Log-Puffer</dt>
        <dd class="tnum">{num(s.diag.logLines)} Zeilen</dd>
        <dt>Firmware</dt>
        <dd class="tnum">{s.v} · {s.part}</dd>
      </dl>
      <div class="actions">
        <Button icon="download" href="/api/diag" download>Diagnosepaket</Button>
        <Button variant="tonal" icon="terminal" href="#/log">Live-Log</Button>
      </div>
      <p class="muted small">
        Status, Versionen, Einstellungen, Neustarts, Log und Absturzbericht in einer Datei, ohne Passwörter.
        {#if s.wifi.ap}Im Setup-Fenster des Handys klappt das Herunterladen oft nicht; dann im normalen Browser
          <a href="http://192.168.4.1/">http://192.168.4.1</a> öffnen.{/if}
      </p>
    </Card>

    <Card>
      <h2 class="card-title"><Icon name="history" size={20} />Bilanz der letzten {resets?.entries.length ?? 16} Starts</h2>
      <div class="tally">
        {#each [
          { n: tally.crashes, label: 'Abstürze' },
          { n: tally.watchdogs, label: 'Watchdog' },
          { n: tally.brownouts, label: 'Unterspannung' },
          { n: tally.rollbacks, label: 'Rückfälle' },
          { n: tally.trials, label: 'Probeläufe' },
        ] as t (t.label)}
          <div class="tile" class:bad={t.n > 0 && t.label !== 'Probeläufe'}>
            <span class="tnum big">{t.n}</span>
            <span class="small">{t.label}</span>
          </div>
        {/each}
      </div>
      <p class="muted small">
        Probelauf: erste Starts nach einem Update. Scheitert der Selbsttest, kommt die vorige Version zurück.
      </p>
    </Card>

    <Card class="wide">
      <h2 class="card-title">
        <Icon name="restart_alt" size={20} />Neustarts
        <span class="end"><IconButton icon="refresh" label="Neu laden" disabled={loading} onclick={load} /></span>
      </h2>
      {#if resets?.entries.length}
        <ul class="list">
          {#each allResets ? resets.entries : resets.entries.slice(0, SHOWN) as r (r.boot)}
            {@const k = kind(r)}
            <li class="list-item top">
              <span class="lead" class:bad={k.bad}><Icon name={k.icon} size={20} /></span>
              <span class="text">
                <span class="headline">
                  Nr. {r.boot} · {r.reasonText}
                  {#if r.boot === resets.boot}<span class="now">dieser Start</span>{/if}
                </span>
                <span class="supporting tnum">
                  {r.v} · {r.part}{r.time ? ` · ${r.time}` : ''}{r.timeLate ? ' (Uhrzeit nachgetragen)' : ''}
                </span>
                {#if details(r)}<span class="supporting tnum">{details(r)}</span>{/if}
              </span>
            </li>
          {/each}
        </ul>
        {#if resets.entries.length > SHOWN}
          <div class="actions">
            <Button variant="text" icon="keyboard_arrow_down" onclick={() => (allResets = !allResets)}>
              {allResets ? 'Weniger zeigen' : `Alle ${resets.entries.length} Neustarts zeigen`}
            </Button>
          </div>
        {/if}
      {:else}
        <p class="muted">{resets ? 'Keine Einträge.' : loading ? 'Lade …' : 'Nicht abrufbar.'}</p>
      {/if}
    </Card>
  </div>
{/if}

<style>
  .waiting {
    display: grid;
    place-items: center;
    padding: 48px 0;
  }
  p {
    margin: 12px 0 0;
  }
  .card-title + p {
    margin-top: 0;
  }
  .lead-text {
    margin: 0 0 16px;
    font: var(--md-sys-typescale-body-large);
  }
  .mono,
  code {
    font-family: ui-monospace, 'Cascadia Mono', Consolas, monospace;
    font-size: 0.92em;
  }
  .bt-head {
    display: flex;
    align-items: center;
    justify-content: space-between;
    margin-top: 16px;
  }
  h3 {
    margin: 0;
    font: var(--md-sys-typescale-title-small);
    font-weight: 650;
    color: var(--md-sys-color-on-surface-variant);
  }
  .bt {
    display: flex;
    flex-wrap: wrap;
    gap: 6px;
    margin: 4px 0 0;
    padding: 12px;
    list-style: none;
    border-radius: var(--md-sys-shape-corner-medium);
    background: var(--md-sys-color-surface-container-highest);
  }
  .bt li {
    padding: 2px 8px;
    border-radius: var(--md-sys-shape-corner-small);
    background: var(--md-sys-color-surface);
  }
  .tally {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(92px, 1fr));
    gap: 8px;
  }
  .tile {
    display: grid;
    gap: 2px;
    padding: 12px;
    border-radius: var(--md-sys-shape-corner-large);
    background: var(--md-sys-color-surface-container-high);
    color: var(--md-sys-color-on-surface-variant);
  }
  .tile.bad {
    background: var(--md-sys-color-error-container);
    color: var(--md-sys-color-on-error-container);
  }
  .big {
    font: var(--md-sys-typescale-headline-small);
    font-weight: 700;
  }
  .list-item.top {
    align-items: flex-start;
  }
  .lead.bad {
    background: var(--md-sys-color-error-container);
    color: var(--md-sys-color-on-error-container);
  }
  .now {
    margin-left: 6px;
    padding: 1px 8px;
    border-radius: var(--md-sys-shape-corner-full);
    background: var(--md-sys-color-primary-container);
    color: var(--md-sys-color-on-primary-container);
    font: var(--md-sys-typescale-label-medium);
  }
  .list-item .headline {
    white-space: normal;
  }
</style>
