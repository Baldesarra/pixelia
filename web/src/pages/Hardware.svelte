<script lang="ts">
  // Seite „Hardware“ (Projektplan Kapitel 8): vom Gerät selbst ausgelesen über /api/system
  import { onMount } from 'svelte';
  import { Card, Chip, Icon, IconButton, LoadingIndicator } from '../lib/m3';
  import { get, type SystemInfo } from '../lib/api';
  import { kb, num } from '../lib/format';

  let sys = $state<SystemInfo | null>(null);
  let failed = $state(false);
  let copied = $state('');

  onMount(() => {
    const load = () =>
      get<SystemInfo>('/api/system')
        .then((r) => ((sys = r), (failed = false)))
        .catch(() => (failed = true));
    void load();
    const t = setInterval(load, 10_000); // Temperatur und Panelwerte
    return () => clearInterval(t);
  });

  const hex = (v: number) => '0x' + v.toString(16).toUpperCase().padStart(6, '0');
  const size = (b: number) => (b >= 1024 * 1024 ? `${num(b / 1048576, 1)} MB` : `${num(b / 1024)} KB`);

  async function copy(text: string) {
    try {
      await navigator.clipboard.writeText(text);
      copied = text;
      setTimeout(() => (copied = ''), 1500);
    } catch {
      /* ohne HTTPS nicht überall erlaubt */
    }
  }
  const canCopy = typeof navigator !== 'undefined' && !!navigator.clipboard;

  const dataPins = $derived.by(() => {
    const d = sys?.pins.data ?? [];
    return [
      ['B0–B4', d.slice(0, 5)],
      ['G0–G5', d.slice(5, 11)],
      ['R0–R4', d.slice(11, 16)],
    ] as const;
  });
</script>

{#if !sys}
  <div class="waiting">
    {#if failed}<p class="muted">Hardware-Daten nicht abrufbar.</p>{:else}<LoadingIndicator size={64} label="Lade" />{/if}
  </div>
{:else}
  <div class="page-grid">
    <Card variant="primary" shape="extra-large" class="wide">
      <div class="hero">
        <Icon name="developer_board" size={48} />
        <div>
          <div class="board">{sys.board}</div>
          <div class="tnum">
            {sys.chip.model} rev {num(sys.chip.rev, 2)} · {sys.chip.cores} Kerne · {sys.chip.mhz} MHz · {sys.flash.mb} MB
            Flash · {kb(sys.psram.kb)} PSRAM
          </div>
        </div>
      </div>
    </Card>

    <Card>
      <h2 class="card-title"><Icon name="monitor" size={20} />Panel</h2>
      <dl class="kv">
        <dt>Typ</dt>
        <dd>{sys.panel.type}</dd>
        <dt>Auflösung</dt>
        <dd class="tnum">{sys.panel.w} × {sys.panel.h}</dd>
        <dt>Gezeichnet</dt>
        <dd class="tnum">{sys.panel.renderW} × {sys.panel.renderH}, 2× hochskaliert</dd>
        <dt>Bildwiederholung</dt>
        <dd class="tnum">{num(sys.panel.hz, 1)} Hz</dd>
        <dt>Pixeltakt</dt>
        <dd class="tnum">{num(sys.panel.pclk, 1)} MHz</dd>
        <dt>Hochskalier-Interrupt</dt>
        <dd class="tnum">{num(sys.panel.isr)} % von Kern 1</dd>
        <dt>Aussetzer</dt>
        <dd class="tnum">{sys.panel.desync}</dd>
        {#if sys.panel.mode}<dt>Modus</dt><dd>{sys.panel.mode}</dd>{/if}
      </dl>
    </Card>

    <Card>
      <h2 class="card-title">
        <Icon name="touch_app" size={20} />Touch
        <span class="end">
          <Chip tone={sys.touch.ok ? 'primary' : 'error'} icon={sys.touch.ok ? 'check_circle' : 'error'}>
            {sys.touch.ok ? 'erkannt' : 'nicht gefunden'}
          </Chip>
        </span>
      </h2>
      {#if sys.touch.ok}
        <dl class="kv">
          <dt>Controller</dt>
          <dd>{sys.touch.chip} (I²C {sys.touch.addr})</dd>
          <dt>Firmware</dt>
          <dd class="tnum">{sys.touch.fw}</dd>
          <dt>Auflösung</dt>
          <dd class="tnum">{sys.touch.w} × {sys.touch.h}</dd>
          <dt>Konfiguration</dt>
          <dd class="tnum">Version {sys.touch.cfg}</dd>
          <dt>Berührungen</dt>
          <dd class="tnum">{num(sys.touch.touches ?? 0)} seit dem Start</dd>
        </dl>
      {:else}
        <p class="muted">Kein GT911 auf dem I²C-Bus (SDA {sys.pins.sda}, SCL {sys.pins.scl}).</p>
      {/if}
    </Card>

    <Card>
      <h2 class="card-title"><Icon name="memory" size={20} />Chip</h2>
      <div class="temp">
        <Icon name="thermostat" size={32} />
        <div>
          <div class="big tnum">{sys.chip.temp === null ? '–' : `${num(sys.chip.temp, 1)} °C`}</div>
          <div class="muted small">Chiptemperatur (interner Sensor, Messbereich −10 bis 80 °C)</div>
        </div>
      </div>
      <dl class="kv">
        <dt>Modell</dt>
        <dd>{sys.chip.model} rev {num(sys.chip.rev, 2)}</dd>
        <dt>Kerne und Takt</dt>
        <dd class="tnum">{sys.chip.cores} × {sys.chip.mhz} MHz</dd>
        <dt>Funk</dt>
        <dd>{[sys.chip.wifi && 'WLAN 2,4 GHz', sys.chip.ble && 'Bluetooth LE'].filter(Boolean).join(', ')}</dd>
        <dt>Flash</dt>
        <dd class="tnum">{sys.flash.mb} MB · {sys.flash.mode} · {sys.flash.freq.replace('m', ' MHz')}</dd>
        <dt>Flash-ID</dt>
        <dd class="tnum">{sys.flash.id}</dd>
        <dt>PSRAM</dt>
        <dd class="tnum">{kb(sys.psram.kb)} · {sys.psram.mode} · {sys.psram.mhz} MHz</dd>
      </dl>
    </Card>

    <Card>
      <h2 class="card-title"><Icon name="lan" size={20} />MAC-Adressen</h2>
      <ul class="list">
        {#each [['WLAN', sys.mac.wifi], ['Setup-WLAN (Access Point)', sys.mac.ap], ['Bluetooth', sys.mac.bt]] as [k, v] (k)}
          <li class="list-item">
            <span class="text">
              <span class="supporting">{k}</span>
              <span class="headline tnum mono">{v}</span>
            </span>
            {#if canCopy}
              <IconButton icon={copied === v ? 'check' : 'content_copy'} label="{k} kopieren" onclick={() => copy(v)} />
            {/if}
          </li>
        {/each}
      </ul>
    </Card>

    <Card>
      <h2 class="card-title"><Icon name="cable" size={20} />Pins</h2>
      <dl class="kv">
        <dt>Panel DE / VSYNC / HSYNC / PCLK</dt>
        <dd class="tnum">{sys.pins.de} / {sys.pins.vsync} / {sys.pins.hsync} / {sys.pins.pclk}</dd>
        {#each dataPins as [k, pins] (k)}
          <dt>Daten {k}</dt>
          <dd class="tnum">{pins.join(', ')}</dd>
        {/each}
        <dt>Hintergrundlicht</dt>
        <dd class="tnum">GPIO {sys.pins.backlight} (PWM)</dd>
        <dt>Touch SDA / SCL / RST</dt>
        <dd class="tnum">{sys.pins.sda} / {sys.pins.scl} / {sys.pins.touchRst}</dd>
      </dl>
    </Card>

    <Card>
      <h2 class="card-title"><Icon name="build" size={20} />Firmware</h2>
      <dl class="kv">
        <dt>Version</dt>
        <dd class="tnum">{sys.build.v}</dd>
        <dt>Gebaut</dt>
        <dd class="tnum">{sys.build.date} {sys.build.time}</dd>
        <dt>ESP-IDF</dt>
        <dd class="tnum">{sys.build.idf}</dd>
        <dt>Compiler</dt>
        <dd class="tnum">{sys.build.compiler}</dd>
        <dt>NVS-Einträge</dt>
        <dd class="tnum">{sys.nvs.used} belegt, {sys.nvs.free} frei</dd>
      </dl>
    </Card>

    <Card class="wide">
      <h2 class="card-title"><Icon name="storage" size={20} />Partitionstabelle</h2>
      <div class="scroll">
        <table>
          <thead>
            <tr><th>Name</th><th>Art</th><th>Adresse</th><th class="num">Größe</th><th>Inhalt</th></tr>
          </thead>
          <tbody>
            {#each sys.partitions as p (p.label)}
              <tr class:run={p.running}>
                <td>{p.label}</td>
                <td>{p.type}</td>
                <td class="tnum mono">{hex(p.offset)}</td>
                <td class="num tnum">{size(p.size)}</td>
                <td class="tnum">
                  {#if p.v !== undefined}
                    {p.v || 'leer'}{p.running ? ` · läuft (${size(sys.appSize)} belegt)` : ''}{p.nextBoot && !p.running ? ' · nächster Start' : ''}
                  {/if}
                </td>
              </tr>
            {/each}
          </tbody>
        </table>
      </div>
    </Card>
  </div>
{/if}

<style>
  .waiting {
    display: grid;
    place-items: center;
    padding: 48px 0;
  }
  .hero {
    display: flex;
    align-items: center;
    gap: 20px;
  }
  .board {
    font: var(--md-sys-typescale-headline-small);
    font-weight: 750;
    font-stretch: 110%;
  }
  .temp {
    display: flex;
    align-items: center;
    gap: 12px;
    margin-bottom: 16px;
    color: var(--md-sys-color-tertiary);
  }
  .temp .muted {
    color: var(--md-sys-color-on-surface-variant);
  }
  .big {
    font: var(--md-sys-typescale-headline-medium);
    font-weight: 750;
    color: var(--md-sys-color-on-surface);
  }
  .mono {
    font-family: ui-monospace, 'Cascadia Mono', Consolas, monospace;
    font-size: 0.95em;
  }
  .scroll {
    overflow-x: auto;
  }
  table {
    width: 100%;
    border-collapse: collapse;
  }
  th,
  td {
    padding: 8px 12px;
    text-align: left;
    white-space: nowrap;
  }
  th {
    font: var(--md-sys-typescale-label-large);
    color: var(--md-sys-color-on-surface-variant);
  }
  tbody tr {
    border-top: 1px solid var(--md-sys-color-outline-variant);
  }
  tr.run td {
    color: var(--md-sys-color-primary);
    font-weight: 650;
  }
  .num {
    text-align: right;
  }
</style>
