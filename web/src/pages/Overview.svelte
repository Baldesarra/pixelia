<script lang="ts">
  // Seite „Übersicht“: Live-Status des Geräts (WebSocket /ws, 1/s); seit 0.5.4 mit Hinweis auf eine neue Version und
  // der Karte „Neu in dieser Version“ (Kurzfassung aus dem Versionsverlauf, je Version einmal wegklickbar)
  import { onDestroy, onMount } from 'svelte';
  import { Button, Card, Chip, CircularProgress, Icon, IconButton, LoadingIndicator, WavyProgress } from '../lib/m3';
  import Logo from '../lib/Logo.svelte';
  import { device } from '../lib/status.svelte';
  import { prefs } from '../lib/prefs.svelte';
  import { installedEntry, installedLog, type ChangeEntry } from '../lib/changelog';
  import ChangeEntryView from '../lib/ChangeEntry.svelte';
  import { duration, kb, num, signal, WIFI_STATE } from '../lib/format';
  import { get } from '../lib/api';
  import { agoText, type SaveInfo } from '../lib/world';

  const s = $derived(device.data);
  const sig = $derived(signal(s?.wifi.rssi ?? 0));
  const RAM_BUDGET_KB = 60; // CLAUDE.md: freier interner RAM ≥ 60 KB
  const FRAME_BUDGET_MS = 20; // Logik + Rendern ≤ 20 ms pro Bild

  let log = $state<ChangeEntry[] | null>(null);
  onMount(() => void installedLog().then((l) => (log = l), () => null));
  const news = $derived.by(() => {
    if (!log || !s) return null;
    const v = installedEntry(log, s.v);
    const e = log.find((x) => x.v === v);
    return e?.k && prefs.seen !== e.v ? e : null;
  });

  // Welt in einer Zeile (seit 0.5.7, Wunsch des Users): Jahreszeit und Tag, Wetter, Mond; alle 60 s
  type WorldLine = {
    started: boolean;
    seasonTitle?: string;
    dayInSeason?: number;
    seasonDays?: number;
    moon?: { title: string };
    weather: { title: string };
    save?: SaveInfo;
  };
  let world = $state<WorldLine | null>(null);
  const loadWorld = () => get<WorldLine>('/api/world').then((w) => (world = w), () => null);
  onMount(() => void loadWorld());
  const worldTimer = window.setInterval(loadWorld, 60000);
  onDestroy(() => clearInterval(worldTimer));
  const worldText = $derived(
    world?.started
      ? `${world.seasonTitle}, Tag ${world.dayInSeason} von ${world.seasonDays} · ${world.weather.title} · ${world.moon?.title ?? ''}`
      : '',
  );
  // Speicherstand in einer Zeile (seit 0.5.9, Wunsch des Users)
  const saveText = $derived.by(() => {
    const v = world?.save;
    if (!v || v.waiting) return '';
    return `${v.ago < 0 ? (v.loaded ? 'Beim Start geladen' : 'Noch nicht gespeichert') : `Gespeichert ${agoText(v.ago)}`} · ${v.figures} Figur${v.figures === 1 ? '' : 'en'}`;
  });
</script>

{#if !s}
  <div class="waiting">
    <LoadingIndicator size={72} contained label="Verbinde mit Pixelia" />
    <p>Verbinde mit Pixelia …</p>
  </div>
{:else}
  <div class="grid">
    <Card variant="primary" shape="extra-large" class="hero">
      <div class="hero-row">
        <Logo size={88} shape="cookie12" strong />
        <div class="hero-text">
          <div class="name">Pixelia</div>
          <div class="meta tnum">Version {s.v} · {s.part} · {s.host}.local</div>
          {#if worldText}<a class="meta world" href="#/welt">{worldText}</a>{/if}
          {#if saveText}<a class="meta world" href="#/welt">{saveText}</a>{/if}
        </div>
      </div>
      <div class="hero-foot">
        <Chip tone="neutral" icon="schedule"><span class="tnum">läuft seit {duration(s.uptime)}</span></Chip>
        {#if s.time.valid}<Chip tone="neutral"><span class="tnum">{s.time.local}</span></Chip>{/if}
        <span class="grow"></span>
        <Button variant="filled" icon="settings" href="#/system">System</Button>
      </div>
    </Card>

    {#if s.rollback}
      <Card variant="error" class="wide">
        <div class="alert">
          <Icon name="error" />
          <div>
            <strong>Update auf {s.rollback.v} zurückgenommen</strong>
            <p>{s.rollback.why}</p>
          </div>
        </div>
      </Card>
    {/if}
    {#if news}
      <Card variant="outlined" class="wide">
        <div class="news-head">
          <Icon name="auto_stories" />
          <strong class="fill">Neu in dieser Version</strong>
          <IconButton icon="close" label="Gelesen" onclick={() => (prefs.seen = news!.v)} />
        </div>
        <ChangeEntryView entry={news} />
      </Card>
    {/if}
    {#if s.ota.avail && !s.ota.busy}
      <Card variant="primary" class="wide">
        <div class="alert">
          <Icon name="system_update_alt" />
          <div class="fill">
            <strong>Neue Version {s.ota.avail} verfügbar</strong>
          </div>
          <Button variant="filled" href="#/update">Ansehen</Button>
        </div>
      </Card>
    {/if}
    {#if s.diag.crash}
      <Card variant="error" class="wide">
        <div class="alert">
          <Icon name="bug_report" />
          <div class="fill">
            <strong>Absturzbericht vorhanden</strong>
            <p>Task {s.diag.crash.task}: {s.diag.crash.reason} (Version {s.diag.crash.v})</p>
          </div>
          <Button variant="tonal" href="#/diagnose">Ansehen</Button>
        </div>
      </Card>
    {/if}
    {#if s.ota.busy}
      <Card variant="tertiary" class="wide">
        <div class="alert">
          <Icon name="system_update_alt" />
          <div class="fill">
            <strong>Update wird installiert {s.ota.newV}</strong>
            <WavyProgress value={s.ota.total ? s.ota.bytes / s.ota.total : undefined} tone="tertiary" label="Update" />
          </div>
        </div>
      </Card>
    {:else if s.ota.trial}
      <Card variant="tertiary" class="wide">
        <div class="alert">
          <Icon name="system_update_alt" />
          <div class="fill">
            <strong>Probelauf nach dem Update: {s.ota.trialS} von {s.ota.trialNeed} s</strong>
            <WavyProgress value={s.ota.trialS / s.ota.trialNeed} tone="tertiary" label="Probelauf" />
          </div>
        </div>
      </Card>
    {/if}
    {#if s.wifi.ap}
      <Card variant="tertiary" class="wide">
        <div class="alert">
          <Icon name="router" />
          <div>
            <strong>Setup-Modus aktiv</strong>
            <p>Pixelia öffnet ein eigenes WLAN ({s.wifi.apClients} verbunden). <a href="#/wlan">WLAN einrichten</a></p>
          </div>
        </div>
      </Card>
    {/if}

    <Card>
      <h2><Icon name="speed" size={20} />Anzeige</h2>
      <div class="split">
        <CircularProgress value={s.perf.fps / 30} size={116} label="Bilder pro Sekunde">
          <div class="big tnum">{num(s.perf.fps)}</div>
          <div class="unit">fps</div>
        </CircularProgress>
        <dl class="kv">
          <dt>Zeichnen</dt>
          <dd class="tnum">{num(s.perf.render, 1)} ms</dd>
          <dt>Hochskalierer</dt>
          <dd class="tnum">{num(s.perf.isr)} %</dd>
          <dt>Panel</dt>
          <dd class="tnum">{num(s.perf.lcd, 1)} Hz</dd>
          <dt>Aussetzer</dt>
          <dd class="tnum">{s.perf.desync}</dd>
        </dl>
      </div>
      <div class="meter">
        <span>Bildzeit von {FRAME_BUDGET_MS} ms Budget</span>
        <WavyProgress value={s.perf.render / FRAME_BUDGET_MS} label="Bildzeit" />
      </div>
    </Card>

    <Card>
      <h2><Icon name="memory" size={20} />Prozessor und Speicher</h2>
      <div class="meter">
        <span class="row"><span>Kern 0 (WLAN, Web)</span><span class="tnum">{s.perf.cpu0} %</span></span>
        <WavyProgress value={s.perf.cpu0 / 100} label="Last Kern 0" />
      </div>
      <div class="meter">
        <span class="row"><span>Kern 1 (Grafik)</span><span class="tnum">{s.perf.cpu1} %</span></span>
        <WavyProgress value={s.perf.cpu1 / 100} tone="tertiary" label="Last Kern 1" />
      </div>
      <div class="mem">
        <div>
          <div class="big tnum">{kb(s.perf.ram)}</div>
          <div class="unit">intern frei · Tiefstwert {kb(s.perf.ramMin)}</div>
        </div>
        <div>
          <div class="big tnum">{kb(s.perf.psram)}</div>
          <div class="unit">PSRAM frei</div>
        </div>
      </div>
      <div class="chips">
        {#if s.perf.ramMin >= RAM_BUDGET_KB}
          <Chip tone="primary" icon="check_circle">Budget ≥ {RAM_BUDGET_KB} KB eingehalten</Chip>
        {:else}
          <Chip tone="error" icon="warning">Tiefstwert unter {RAM_BUDGET_KB} KB</Chip>
        {/if}
        <Chip>größter Block {kb(s.perf.ramBlk)}</Chip>
      </div>
    </Card>

    <Card>
      <h2><Icon name={s.wifi.state === 'verbunden' ? 'wifi' : 'wifi_off'} size={20} />WLAN</h2>
      <div class="wifi">
        <div class="bars" aria-label="Signal {sig.bars} von 4">
          {#each [1, 2, 3, 4] as b (b)}<span class:on={b <= sig.bars} style:height="{b * 25}%"></span>{/each}
        </div>
        <div>
          <div class="ssid">{s.wifi.ssid || WIFI_STATE[s.wifi.state] || s.wifi.state}</div>
          <div class="unit tnum">
            {#if s.wifi.state === 'verbunden'}{s.wifi.rssi} dBm · {sig.text}{:else}{WIFI_STATE[s.wifi.state] ??
                s.wifi.state}{/if}
          </div>
        </div>
      </div>
      <dl class="kv">
        <dt>Adresse</dt>
        <dd class="tnum">{s.wifi.ip || '–'}</dd>
        <dt>Kanal</dt>
        <dd class="tnum">{s.wifi.ch || '–'}</dd>
        <dt>Gespeicherte Netze</dt>
        <dd class="tnum">{s.wifi.saved} von 5</dd>
        {#if s.wifi.err}
          <dt>Letzter Fehler</dt>
          <dd>{s.wifi.err}</dd>
        {/if}
      </dl>
    </Card>

    <Card>
      <h2><Icon name="restart_alt" size={20} />System</h2>
      <dl class="kv">
        <dt>Start Nr.</dt>
        <dd class="tnum">{s.diag.boot}</dd>
        <dt>Letzter Neustart</dt>
        <dd>{s.diag.reset}</dd>
        <dt>Uhrzeit</dt>
        <dd>{s.time.valid ? 'per NTP gestellt' : 'noch nicht gestellt'}</dd>
        <dt>Gebaut</dt>
        <dd class="tnum">{s.built}</dd>
        <dt>ESP-IDF</dt>
        <dd class="tnum">{s.idf}</dd>
      </dl>
      <div class="chips">
        <Chip tone={s.diag.errors ? 'error' : 'neutral'} icon={s.diag.errors ? 'error' : 'check_circle'}>
          {s.diag.errors} Fehler
        </Chip>
        <Chip tone={s.diag.warnings ? 'tertiary' : 'neutral'} icon="warning">{s.diag.warnings} Warnungen</Chip>
        <Chip><span class="tnum">{num(s.diag.logLines)} Logzeilen</span></Chip>
      </div>
    </Card>
  </div>
{/if}

<style>
  .waiting {
    display: grid;
    place-items: center;
    gap: 16px;
    padding: 96px 0;
    color: var(--md-sys-color-on-surface-variant);
    font: var(--md-sys-typescale-title-medium);
  }
  .news-head {
    display: flex;
    align-items: center;
    gap: 12px;
    margin: -4px -8px 16px 0;
    color: var(--md-sys-color-primary);
  }
  .news-head strong {
    font: var(--md-sys-typescale-title-medium);
    font-weight: 650;
  }
  .grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(min(100%, 320px), 1fr));
    gap: 16px;
    align-items: start;
  }
  .grid :global(.hero),
  .grid :global(.wide) {
    grid-column: 1 / -1;
  }
  .hero-row {
    display: flex;
    align-items: center;
    gap: 20px;
  }
  .name {
    font: var(--md-sys-typescale-display-medium);
    font-weight: 800;
    font-stretch: 120%;
    letter-spacing: -0.5px;
  }
  .meta {
    font: var(--md-sys-typescale-body-large);
    opacity: 0.85;
  }
  .world {
    display: block;
    margin-top: 2px;
    color: inherit;
    text-decoration: none;
  }
  .hero-foot {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 8px;
    margin-top: 20px;
  }
  .grow {
    flex: 1;
  }
  h2 {
    display: flex;
    align-items: center;
    gap: 8px;
    margin: 0 0 16px;
    font: var(--md-sys-typescale-title-medium);
    font-weight: 650;
    color: var(--md-sys-color-on-surface-variant);
  }
  .alert {
    display: flex;
    gap: 16px;
    align-items: flex-start;
  }
  .alert p {
    margin: 4px 0 0;
  }
  .alert strong {
    font: var(--md-sys-typescale-title-medium);
    font-weight: 650;
  }
  .fill {
    flex: 1;
    display: grid;
    gap: 10px;
  }
  .split {
    display: flex;
    align-items: center;
    gap: 24px;
  }
  .big {
    font: var(--md-sys-typescale-headline-large);
    font-weight: 750;
    font-stretch: 110%;
    color: var(--md-sys-color-on-surface);
  }
  .unit {
    font: var(--md-sys-typescale-body-medium);
    color: var(--md-sys-color-on-surface-variant);
  }
  .kv {
    flex: 1;
    display: grid;
    grid-template-columns: auto 1fr;
    gap: 6px 16px;
    margin: 0;
  }
  dt {
    color: var(--md-sys-color-on-surface-variant);
  }
  dd {
    margin: 0;
    text-align: right;
    font-weight: 550;
    overflow-wrap: anywhere;
  }
  .meter {
    display: grid;
    gap: 6px;
    margin-top: 16px;
    color: var(--md-sys-color-on-surface-variant);
  }
  .row {
    display: flex;
    justify-content: space-between;
  }
  .mem {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 16px;
    margin-top: 20px;
  }
  .chips {
    display: flex;
    flex-wrap: wrap;
    gap: 8px;
    margin-top: 16px;
  }
  .wifi {
    display: flex;
    align-items: center;
    gap: 16px;
    margin-bottom: 16px;
  }
  .ssid {
    font: var(--md-sys-typescale-headline-small);
    font-weight: 700;
    font-stretch: 108%;
    overflow-wrap: anywhere;
  }
  .bars {
    display: flex;
    align-items: flex-end;
    gap: 4px;
    height: 40px;
  }
  .bars span {
    width: 9px;
    border-radius: 3px;
    background: var(--md-sys-color-surface-container-highest);
    transition: background-color var(--m3-effects);
  }
  .bars span.on {
    background: var(--md-sys-color-primary);
  }
  @media (max-width: 420px) {
    .hero-row :global(.logo) {
      width: 64px;
      height: 64px;
    }
    .name {
      font: var(--md-sys-typescale-display-small);
      font-weight: 800;
      font-stretch: 115%;
    }
    .split {
      gap: 16px;
    }
  }
</style>
