<script lang="ts">
  // Seite „Info“: Laufzeit, Netz mit Signalbewertung, Uhrzeit, Speicherbalken (Programm, RAM, PSRAM, NVS, Dateisystem)
  // und Warnung bei falscher Partitionierung (Projektplan 4.12)
  import { onMount } from 'svelte';
  import { Card, Chip, Icon, LoadingIndicator, WavyProgress } from '../lib/m3';
  import SignalBars from '../lib/SignalBars.svelte';
  import { device } from '../lib/status.svelte';
  import { get, type SystemInfo } from '../lib/api';
  import { duration, kb, num, signal } from '../lib/format';

  const APP_PART_MIN = 6 * 1024 * 1024; // partitions.csv: ota_0/ota_1 je 6 MB
  const APP_BUDGET = 0.75; // CLAUDE.md: Firmware ≤ 75 % der App-Partition

  const s = $derived(device.data);
  let sys = $state<SystemInfo | null>(null);
  let failed = $state(false);

  onMount(() => {
    const load = () =>
      get<SystemInfo>('/api/system')
        .then((r) => ((sys = r), (failed = false)))
        .catch(() => (failed = true));
    void load();
    const t = setInterval(load, 15_000);
    return () => clearInterval(t);
  });

  const parts = $derived(sys?.partitions ?? []);
  const storage = $derived(parts.find((p) => p.type === 'littlefs' || p.label === 'storage'));
  const nvsPart = $derived(parts.find((p) => p.type === 'nvs'));
  const apps = $derived(parts.filter((p) => p.type === 'ota' || p.type === 'app'));
  const partWarn = $derived.by(() => {
    if (!sys) return '';
    const w: string[] = [];
    if (sys.flash.mb < 16) w.push(`Flash nur ${sys.flash.mb} MB statt 16 MB`);
    if (apps.length < 2) w.push('nur eine App-Partition (Updates per WLAN brauchen zwei)');
    if (sys.appPart && sys.appPart < APP_PART_MIN) w.push(`App-Partition ${kb(sys.appPart / 1024)} statt 6 MB`);
    return w.join(', ');
  });
  const mb = (bytes: number) => kb(bytes / 1024);
</script>

{#if !s}
  <div class="waiting"><LoadingIndicator size={64} label="Lade" /></div>
{:else}
  <div class="page-grid">
    {#if partWarn}
      <Card variant="error" class="wide">
        <div class="alert">
          <Icon name="warning" />
          <div>
            <strong>Partitionierung passt nicht</strong>
            <p>{partWarn}. Abhilfe: einmal über den Web-Flasher neu installieren (Factory-Image).</p>
          </div>
        </div>
      </Card>
    {/if}

    <Card>
      <h2 class="card-title"><Icon name="schedule" size={20} />Laufzeit und Zeit</h2>
      <div class="big tnum">{duration(s.uptime)}</div>
      <div class="muted">seit dem letzten Start (Nr. {s.diag.boot}, {s.diag.reset})</div>
      <dl class="kv gap">
        <dt>Uhrzeit</dt>
        <dd class="tnum">{s.time.valid ? s.time.local : 'noch nicht gestellt'}</dd>
        <dt>Zeitzone</dt>
        <dd>{sys?.time.tz ?? 'Europa/Berlin'}</dd>
        <dt>Zeitabgleich (NTP)</dt>
        <dd class="tnum">{sys ? (sys.time.syncs ? `${sys.time.syncs}× seit dem Start` : 'noch keiner') : '…'}</dd>
        <dt>Version</dt>
        <dd class="tnum">{s.v} ({s.part})</dd>
      </dl>
    </Card>

    <Card>
      <h2 class="card-title"><Icon name="lan" size={20} />Netz</h2>
      <div class="net">
        <SignalBars rssi={s.wifi.state === 'verbunden' ? s.wifi.rssi : 0} size={36} />
        <div>
          <div class="headline">{s.wifi.state === 'verbunden' ? s.wifi.ssid : s.wifi.ap ? 'Setup-Modus' : 'nicht verbunden'}</div>
          {#if s.wifi.state === 'verbunden'}
            <div class="muted tnum">{s.wifi.rssi} dBm · {signal(s.wifi.rssi).text}</div>
          {/if}
        </div>
      </div>
      <dl class="kv gap">
        <dt>Adresse</dt>
        <dd class="tnum">{s.wifi.ip || '–'}</dd>
        <dt>Router</dt>
        <dd class="tnum">{sys?.net.gw || '–'}</dd>
        <dt>DNS</dt>
        <dd class="tnum">{sys?.net.dns || '–'}</dd>
        <dt>Name</dt>
        <dd>{s.host}.local</dd>
        <dt>Live-Verbindung</dt>
        <dd>{device.link === 'live' ? 'WebSocket' : device.link === 'abfrage' ? 'Abfrage alle 2 s' : device.link}</dd>
      </dl>
    </Card>

    <Card class="wide">
      <h2 class="card-title"><Icon name="storage" size={20} />Speicher</h2>
      {#if !sys}
        <p class="muted">{failed ? 'Speicherdaten nicht abrufbar.' : 'Lade …'}</p>
      {:else}
        <div class="meters">
          <div class="meter-row">
            <span class="head"><span>Programm (App-Partition)</span><span class="tnum">{mb(sys.appSize)} von {mb(sys.appPart)}</span></span>
            <WavyProgress wavy={false} value={sys.appSize / sys.appPart} tone={sys.appSize / sys.appPart > APP_BUDGET ? 'error' : 'primary'} label="Programm" />
            <span class="small muted tnum">{num((sys.appSize / sys.appPart) * 100)} % belegt · Budget {APP_BUDGET * 100} %</span>
          </div>
          <div class="meter-row">
            <span class="head"><span>RAM intern</span><span class="tnum">{kb(s.perf.ram)} frei von {kb(sys.heap.intTotal)}</span></span>
            <WavyProgress wavy={false} value={1 - s.perf.ram / sys.heap.intTotal} label="RAM intern" />
            <span class="small muted tnum">Tiefstwert {kb(s.perf.ramMin)} frei · größter Block {kb(s.perf.ramBlk)}</span>
          </div>
          <div class="meter-row">
            <span class="head"><span>PSRAM</span><span class="tnum">{kb(s.perf.psram)} frei von {kb(sys.heap.psramTotal)}</span></span>
            <WavyProgress wavy={false} value={1 - s.perf.psram / sys.heap.psramTotal} tone="tertiary" label="PSRAM" />
            <span class="small muted tnum">Tiefstwert {kb(s.perf.psramMin)} frei</span>
          </div>
          <div class="meter-row">
            <span class="head"><span>Einstellungen (NVS{nvsPart ? `, ${mb(nvsPart.size)}` : ''})</span><span class="tnum">{sys.nvs.used} von {sys.nvs.total} Einträgen</span></span>
            <WavyProgress wavy={false} value={sys.nvs.used / (sys.nvs.total || 1)} tone="tertiary" label="NVS" />
          </div>
          {#if storage}
            <div class="meter-row">
              <span class="head"><span>Dateisystem „{storage.label}“</span><span class="tnum">{mb(storage.size)}</span></span>
              <WavyProgress wavy={false} value={0} label="Dateisystem" />
              <span class="small muted">noch ungenutzt (für Welten-Daten vorgesehen)</span>
            </div>
          {/if}
        </div>
        <div class="chips gap">
          {#each apps as p (p.label)}
            <Chip tone={p.running ? 'primary' : 'neutral'} icon={p.running ? 'check_circle' : undefined}>
              <span class="tnum">{p.label}: {p.v || 'leer'}{p.running ? ' (läuft)' : ''}{p.nextBoot && !p.running ? ' (nächster Start)' : ''}</span>
            </Chip>
          {/each}
          <Chip><span class="tnum">Flash {sys.flash.mb} MB</span></Chip>
        </div>
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
  .big {
    font: var(--md-sys-typescale-headline-large);
    font-weight: 750;
    font-stretch: 110%;
  }
  .gap {
    margin-top: 16px;
  }
  .net {
    display: flex;
    align-items: center;
    gap: 16px;
  }
  .headline {
    font: var(--md-sys-typescale-title-large);
    font-weight: 650;
    overflow-wrap: anywhere;
  }
  .meters {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(min(100%, 300px), 1fr));
    column-gap: 32px;
  }
  .alert {
    display: flex;
    gap: 16px;
  }
  .alert p {
    margin: 4px 0 0;
  }
  .alert strong {
    font: var(--md-sys-typescale-title-medium);
    font-weight: 650;
  }
</style>
