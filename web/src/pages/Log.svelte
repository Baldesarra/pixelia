<script lang="ts">
  // Seite „Live-Log“: WebSocket /ws/log (zuerst die letzten 200 Zeilen, dann jede neue), Rückfall auf /api/log
  // alle 2 s (z. B. Captive-Portal-Fenster). Filter nach Stufe, Suche, Pause, Befehle über /api/cmd.
  // Passwörter filtert schon die Firmware heraus (web_diag.cpp).
  import { onMount, tick } from 'svelte';
  import { Button, ButtonGroup, Card, Chip, confirmDialog, Icon, IconButton, TextField } from '../lib/m3';
  import type { LogLine } from '../lib/api';

  const MAX_LINES = 2000;
  const TAIL = 200;

  let lines = $state<LogLine[]>([]);
  let lastN = -1;
  let rid = 0;
  let mode = $state<'verbinde' | 'live' | 'abfrage' | 'getrennt'>('verbinde');
  let paused = $state(false);
  let held: LogLine[] = [];
  let heldCount = $state(0);
  let follow = $state(true);
  let level = $state<'all' | 'warn' | 'err'>('all');
  let query = $state('');
  let box = $state<HTMLElement>();

  function take(list: LogLine[], newRid?: number) {
    if (newRid !== undefined && rid && newRid !== rid) lastN = -1; // Puffer neu angelegt
    if (newRid !== undefined) rid = newRid;
    const fresh = list.filter((l) => l.n > lastN);
    if (!fresh.length) return;
    lastN = fresh[fresh.length - 1].n;
    if (paused) {
      held.push(...fresh);
      heldCount = held.length;
      return;
    }
    append(fresh);
  }

  function append(list: LogLine[]) {
    const all = lines.concat(list);
    lines = all.length > MAX_LINES ? all.slice(all.length - MAX_LINES) : all;
    if (follow) void tick().then(() => box?.scrollTo({ top: box.scrollHeight }));
  }

  function togglePause() {
    paused = !paused;
    if (!paused && held.length) {
      append(held);
      held = [];
      heldCount = 0;
    }
  }

  // --- Verbindung: WebSocket, sonst Abfrage -----------------------------------------------------------------
  let ws: WebSocket | null = null;
  let fails = 0;
  let timer = 0;
  let stopped = false;

  function connect() {
    if (stopped) return;
    const q = lastN >= 0 ? `since=${lastN}` : `tail=${TAIL}`;
    try {
      ws = new WebSocket(`${location.protocol === 'https:' ? 'wss' : 'ws'}://${location.host}/ws/log?${q}`);
    } catch {
      return fallback();
    }
    const me = ws;
    me.onopen = () => {
      fails = 0;
      mode = 'live';
    };
    me.onmessage = (e) => {
      try {
        const j = JSON.parse(e.data) as { rid: number; lines: LogLine[] };
        take(j.lines, j.rid);
      } catch {
        /* unvollständig */
      }
    };
    me.onclose = () => {
      if (ws === me) ws = null;
      if (stopped) return;
      fails++;
      if (fails >= 2) fallback();
      else timer = window.setTimeout(connect, 2000);
    };
  }

  async function poll() {
    if (stopped) return;
    try {
      const q = lastN >= 0 ? `since=${lastN}&rid=${rid}` : `tail=${TAIL}`;
      const r = await fetch(`/api/log?${q}&max=24576`, { cache: 'no-store' });
      const j = (await r.json()) as { rid: number; lines: LogLine[]; reset: boolean };
      if (j.reset) lastN = -1;
      take(j.lines, j.rid);
      mode = 'abfrage';
    } catch {
      mode = 'getrennt';
    }
    timer = window.setTimeout(poll, 2000);
    // Ab und zu wieder den WebSocket probieren
    if (++fails % 15 === 0) {
      clearTimeout(timer);
      fails = 0;
      connect();
    }
  }

  function fallback() {
    mode = 'abfrage';
    void poll();
  }

  onMount(() => {
    connect();
    return () => {
      stopped = true;
      clearTimeout(timer);
      ws?.close();
    };
  });

  // --- Anzeige ----------------------------------------------------------------------------------------------
  const shown = $derived.by(() => {
    const q = query.trim().toLowerCase();
    return lines.filter(
      (l) =>
        (level === 'all' || l.l === 'E' || (level === 'warn' && l.l === 'W')) &&
        (!q || l.m.toLowerCase().includes(q)),
    );
  });

  // Die Zeilen tragen das ESP-IDF-Format „I (12345) tag: …“; Stufe und Zeit stehen schon in eigenen Spalten
  const PREFIX = /^[EWIDV] \(\d+\) /;
  const pad = (v: number) => String(v).padStart(2, '0');
  function stamp(l: LogLine): string {
    if (l.t > 0) {
      const d = new Date(l.t * 1000);
      return `${pad(d.getHours())}:${pad(d.getMinutes())}:${pad(d.getSeconds())}`;
    }
    return `+${(l.u / 1000).toFixed(1)} s`;
  }

  function onScroll() {
    if (!box) return;
    follow = box.scrollHeight - box.scrollTop - box.clientHeight < 40;
  }
  function toEnd() {
    follow = true;
    box?.scrollTo({ top: box.scrollHeight, behavior: 'smooth' });
  }

  // --- Befehle ----------------------------------------------------------------------------------------------
  let cmd = $state('');
  let running = $state(false);
  let result = $state<{ ok: boolean; cmd: string; out: string; msg?: string; ms?: number } | null>(null);

  async function send(e: SubmitEvent, confirmed = false) {
    e.preventDefault();
    const c = cmd.trim();
    if (!c) return;
    running = true;
    try {
      const body = new URLSearchParams({ cmd: c, ...(confirmed ? { confirm: '1' } : {}) });
      const r = await fetch('/api/cmd', { method: 'POST', body });
      const j = await r.json();
      if (j.needsConfirm && !confirmed) {
        running = false;
        const ok = await confirmDialog({ title: 'Befehl wirklich ausführen?', text: j.msg, ok: 'Ausführen', icon: 'warning', danger: true });
        if (ok) return send(e, true);
        return;
      }
      result = { ok: j.ok, cmd: c, out: j.out ?? '', msg: j.msg, ms: j.ms };
    } catch {
      result = { ok: false, cmd: c, out: '', msg: 'Keine Antwort vom Gerät' };
    }
    running = false;
  }
</script>

<div class="layout">
  <Card class="log-card">
    <div class="toolbar">
      <Chip tone={mode === 'live' ? 'primary' : mode === 'getrennt' ? 'error' : 'neutral'} pulse={mode === 'live' && !paused}>
        {paused ? `Pausiert${heldCount ? ` (${heldCount} neu)` : ''}` : { verbinde: 'Verbinde …', live: 'Live', abfrage: 'Abfrage', getrennt: 'Getrennt' }[mode]}
      </Chip>
      <ButtonGroup
        label="Stufe"
        options={[
          { value: 'all', label: 'Alle' },
          { value: 'warn', label: 'Warnungen' },
          { value: 'err', label: 'Fehler' },
        ]}
        bind:value={level}
      />
      <span class="grow"></span>
      <IconButton icon={paused ? 'play_arrow' : 'pause'} label={paused ? 'Weiter' : 'Anhalten'} onclick={togglePause} />
      <IconButton icon="delete_sweep" label="Anzeige leeren" onclick={() => (lines = [])} />
      <Button variant="text" icon="download" href="/api/log?format=text" download="pixelia-log.txt">Als Text</Button>
    </div>
    <TextField label="Suchen" icon="search" bind:value={query} class="search">
      {#snippet trailing()}
        {#if query}<IconButton icon="close" label="Suche leeren" onclick={() => (query = '')} />{/if}
      {/snippet}
    </TextField>

    <div class="log" bind:this={box} onscroll={onScroll} role="log" aria-live="off">
      {#each shown as l, i (l.n)}
        {#if i > 0 && shown[i - 1].b !== l.b}
          <div class="boot">Neustart · Start Nr. {l.b}</div>
        {/if}
        <div class="line lv-{l.l}">
          <span class="t tnum">{stamp(l)}</span>
          <span class="lv">{l.l}</span>
          <span class="m">{l.m.replace(PREFIX, '')}{l.cut ? ' …' : ''}</span>
        </div>
      {:else}
        <div class="empty muted">{lines.length ? 'Keine Zeile passt zum Filter.' : 'Warte auf Log-Zeilen …'}</div>
      {/each}
    </div>
    {#if !follow}
      <div class="jump"><Button variant="tonal" size="xs" icon="vertical_align_bottom" onclick={toEnd}>Zum Ende</Button></div>
    {/if}
    <p class="small muted">{shown.length} von {lines.length} Zeilen (höchstens {MAX_LINES} im Browser). Passwörter sind herausgefiltert.</p>
  </Card>

  <Card>
    <h2 class="card-title"><Icon name="terminal" size={20} />Befehl senden</h2>
    <form class="cmd" onsubmit={(e) => send(e)}>
      <TextField label="Befehl, z. B. help, perf, wifi" bind:value={cmd} maxlength={200} autocapitalize="none" spellcheck="false" />
      <Button type="submit" icon="send" disabled={running || !cmd.trim()}>{running ? 'Läuft …' : 'Senden'}</Button>
    </form>
    {#if result}
      <p class={result.ok ? 'muted small' : 'err-text'}>
        {result.cmd}: {result.ok ? `fertig${result.ms !== undefined ? ` in ${result.ms} ms` : ''}` : result.msg}
      </p>
      {#if result.out}<pre class="out">{result.out}</pre>{/if}
    {/if}
    <p class="muted small">Wie auf der seriellen Konsole. Gefährliche Befehle fragen vorher nach.</p>
  </Card>
</div>

<style>
  .layout {
    display: grid;
    gap: 16px;
  }
  .layout :global(.log-card) {
    position: relative;
  }
  .toolbar {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 8px;
    margin-bottom: 12px;
  }
  .grow {
    flex: 1;
  }
  .layout :global(.search) {
    margin-bottom: 12px;
  }
  .log {
    height: min(60dvh, 640px);
    overflow: auto;
    padding: 8px 0;
    border-radius: var(--md-sys-shape-corner-large);
    background: var(--md-sys-color-surface-container-lowest);
    font: 12.5px/1.5 ui-monospace, 'Cascadia Mono', Consolas, monospace;
  }
  .line {
    display: grid;
    grid-template-columns: auto auto 1fr;
    gap: 10px;
    padding: 1px 12px;
  }
  .line:hover {
    background: color-mix(in srgb, var(--md-sys-color-on-surface) 6%, transparent);
  }
  .t {
    color: var(--md-sys-color-on-surface-variant);
  }
  .lv {
    width: 1.4em;
    text-align: center;
    border-radius: 4px;
    font-weight: 700;
    color: var(--md-sys-color-on-surface-variant);
  }
  .m {
    white-space: pre-wrap;
    overflow-wrap: anywhere;
  }
  .lv-E .lv {
    background: var(--md-sys-color-error);
    color: var(--md-sys-color-on-error);
  }
  .lv-E .m {
    color: var(--md-sys-color-error);
  }
  .lv-W .lv {
    background: var(--md-sys-color-tertiary-container);
    color: var(--md-sys-color-on-tertiary-container);
  }
  .lv-W .m {
    color: var(--md-sys-color-tertiary);
  }
  .lv-D .m,
  .lv-V .m {
    color: var(--md-sys-color-on-surface-variant);
  }
  .boot {
    margin: 6px 12px;
    padding: 2px 10px;
    border-radius: var(--md-sys-shape-corner-full);
    background: var(--md-sys-color-secondary-container);
    color: var(--md-sys-color-on-secondary-container);
    font: var(--md-sys-typescale-label-medium);
    width: fit-content;
  }
  .empty {
    padding: 24px;
    text-align: center;
    font: var(--md-sys-typescale-body-medium);
  }
  .jump {
    position: absolute;
    right: 32px;
    bottom: 56px;
  }
  .cmd {
    display: flex;
    flex-wrap: wrap;
    gap: 8px;
    align-items: center;
  }
  .cmd :global(.field) {
    flex: 1 1 260px;
  }
  .out {
    max-height: 320px;
    overflow: auto;
    margin: 0 0 12px;
    padding: 12px;
    border-radius: var(--md-sys-shape-corner-medium);
    background: var(--md-sys-color-surface-container-lowest);
    font: 12.5px/1.5 ui-monospace, 'Cascadia Mono', Consolas, monospace;
    white-space: pre-wrap;
    overflow-wrap: anywhere;
  }
  p {
    margin: 10px 0 0;
  }
</style>
