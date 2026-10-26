<script lang="ts">
  // Seite „Display“ (Einstellungen → Gerät): Bilddrehung, Helligkeit, Dauer des langen Drucks für die Info-Karte,
  // seit 0.5.6 die Pixel-Uhr (an/aus, Ecke, Sekunden)
  import { ButtonGroup, Card, Icon, LoadingIndicator, Slider, Switch } from '../lib/m3';
  import { device } from '../lib/status.svelte';
  import { post } from '../lib/api';
  import { num } from '../lib/format';

  const s = $derived(device.data);

  let bright = $state(80);
  let lpS = $state(3);
  let touchedAt = 0;
  let brightTimer = 0;
  $effect(() => {
    if (!s || performance.now() - touchedAt < 3000) return;
    bright = s.set.bright;
    lpS = Math.round(s.set.lp / 100) / 10;
  });
  function setBright(v: number) {
    touchedAt = performance.now();
    clearTimeout(brightTimer);
    brightTimer = window.setTimeout(() => post('/api/settings', { bright: v }).catch(() => null), 150);
  }
  function setLp(v: number) {
    touchedAt = performance.now();
    void post('/api/settings', { lp: Math.round(v * 1000) }).catch(() => null);
  }
  // Pixel-Uhr: Ecke als oben/unten und links/rechts (kurze, gleich breite Knöpfe)
  let clkLocal = $state<{ on?: boolean; corner?: number; sec?: boolean }>({});
  const clkOn = $derived(clkLocal.on ?? s?.set.clk ?? true);
  const corner = $derived(clkLocal.corner ?? s?.set.clkCorner ?? 0);
  const clkSec = $derived(clkLocal.sec ?? s?.set.clkSec ?? false);
  function setClock(patch: { on?: boolean; corner?: number; sec?: boolean }) {
    clkLocal = { ...clkLocal, ...patch };
    const p: Record<string, string> = {};
    if (patch.on !== undefined) p.clk = patch.on ? '1' : '0';
    if (patch.corner !== undefined) p.clkCorner = String(patch.corner);
    if (patch.sec !== undefined) p.clkSec = patch.sec ? '1' : '0';
    void post('/api/settings', p)
      .catch(() => null)
      .finally(() => setTimeout(() => (clkLocal = {}), 2500));
  }
  // 0 oben rechts, 1 oben links, 2 unten rechts, 3 unten links
  const vert = $derived(corner >= 2 ? 'unten' : 'oben');
  const horiz = $derived(corner % 2 === 1 ? 'links' : 'rechts');
  const cornerOf = (v: string, h: string) => (v === 'unten' ? 2 : 0) + (h === 'links' ? 1 : 0);

  // Nachtmodus (seit 0.5.7): 22–6 Uhr wie die gedimmte Uhr, Nacht-Helligkeit
  let nightLocal = $state<boolean | null>(null);
  const nightOn = $derived(nightLocal ?? s?.set.night ?? true);
  let nightBr = $state(25);
  let nightTouched = 0;
  let nightTimer = 0;
  $effect(() => {
    if (!s || performance.now() - nightTouched < 3000) return;
    nightBr = s.set.nightBr ?? 25;
  });
  function setNight(on: boolean) {
    nightLocal = on;
    void post('/api/settings', { night: on ? '1' : '0' })
      .catch(() => null)
      .finally(() => setTimeout(() => (nightLocal = null), 2500));
  }
  // Nachtzeiten (Wunsch des Users): Beginn 18–24 Uhr (24 = Mitternacht), Ende 4–10 Uhr
  let nightFrom = $state(22);
  let nightTo = $state(6);
  $effect(() => {
    if (!s || performance.now() - nightTouched < 3000) return;
    nightFrom = s.set.nightFrom === 0 ? 24 : (s.set.nightFrom ?? 22);
    nightTo = s.set.nightTo ?? 6;
  });
  function setNightHours() {
    nightTouched = performance.now();
    void post('/api/settings', { nightFrom: nightFrom % 24, nightTo }).catch(() => null);
  }
  function setNightBr(v: number) {
    nightTouched = performance.now();
    clearTimeout(nightTimer);
    nightTimer = window.setTimeout(() => post('/api/settings', { nightBr: v }).catch(() => null), 150);
  }

  let rotLocal = $state<'0' | '180' | null>(null);
  const rot = $derived(rotLocal ?? (String(s?.set.rot ?? 0) as '0' | '180'));
  function setRot(v: '0' | '180') {
    rotLocal = v;
    void post('/api/settings', { rot: v })
      .catch(() => null)
      .finally(() => setTimeout(() => (rotLocal = null), 2500));
  }
</script>

{#if !s}
  <div class="waiting"><LoadingIndicator size={64} label="Lade" /></div>
{:else}
  <div class="page-grid">
    <Card class="wide">
      <h2 class="card-title"><Icon name="light_mode" size={20} />Anzeige</h2>
      <div class="setting">
        <span>Bilddrehung</span>
        <ButtonGroup
          label="Bilddrehung"
          options={[
            { value: '0', label: '0° normal' },
            { value: '180', label: '180°', icon: 'mobile_rotate' },
          ]}
          bind:value={() => rot, setRot}
        />
      </div>
      <div class="setting">
        <span class="row"><span>Helligkeit</span><span class="tnum">{bright} %</span></span>
        <Slider label="Helligkeit" min={1} max={100} unit=" %" bind:value={bright} oninput={setBright} />
      </div>
      <div class="setting">
        <span class="row"><span>Langer Druck (Info-Karte)</span><span class="tnum">{num(lpS, 1)} s</span></span>
        <Slider label="Dauer langer Druck" min={1} max={10} step={0.5} unit=" s" bind:value={lpS} onchange={setLp} />
      </div>
    </Card>
    <Card>
      <h2 class="card-title"><Icon name="schedule" size={20} />Pixel-Uhr</h2>
      <div class="switch-row">
        <span>Uhr anzeigen <span class="muted">Ortszeit, davor die Jahreszeit; antippen zeigt Datum und Mond</span></span>
        <Switch label="Pixel-Uhr" checked={clkOn} onchange={(v) => setClock({ on: v })} />
      </div>
      <div class="setting">
        <span>Ecke</span>
        <ButtonGroup
          label="Oben oder unten"
          options={[
            { value: 'oben', label: 'Oben' },
            { value: 'unten', label: 'Unten' },
          ]}
          bind:value={() => vert, (v) => setClock({ corner: cornerOf(v, horiz) })}
        />
        <ButtonGroup
          label="Links oder rechts"
          options={[
            { value: 'links', label: 'Links' },
            { value: 'rechts', label: 'Rechts' },
          ]}
          bind:value={() => horiz, (h) => setClock({ corner: cornerOf(vert, h) })}
        />
      </div>
      <div class="switch-row">
        <span>Sekunden <span class="muted">sonst blinkt der Doppelpunkt</span></span>
        <Switch label="Sekunden" checked={clkSec} onchange={(v) => setClock({ sec: v })} />
      </div>
    </Card>
    <Card>
      <h2 class="card-title"><Icon name="bedtime" size={20} />Nachtmodus</h2>
      <div class="switch-row">
        <span
          >Nachts dimmen <span class="muted"
            >{nightFrom % 24}–{nightTo} Uhr, auch die Uhr wird gedimmt; Berühren hellt 1 Minute auf{nightOn
              ? s.set.nightNow
                ? ' · gerade Nacht'
                : ' · gerade Tag'
              : ''}</span
          ></span
        >
        <Switch label="Nachtmodus" checked={nightOn} onchange={setNight} />
      </div>
      <div class="setting">
        <span class="row"><span>Nacht-Helligkeit</span><span class="tnum">{nightBr} %</span></span>
        <Slider label="Nacht-Helligkeit" min={1} max={100} unit=" %" bind:value={nightBr} oninput={setNightBr} />
      </div>
      <div class="setting">
        <span class="row"><span>Nacht beginnt</span><span class="tnum">{nightFrom % 24} Uhr</span></span>
        <Slider label="Nacht beginnt" min={18} max={24} unit=" Uhr" bind:value={nightFrom} onchange={setNightHours} />
      </div>
      <div class="setting">
        <span class="row"><span>Nacht endet</span><span class="tnum">{nightTo} Uhr</span></span>
        <Slider label="Nacht endet" min={4} max={10} unit=" Uhr" bind:value={nightTo} onchange={setNightHours} />
      </div>
    </Card>
  </div>
{/if}

<style>
  .switch-row {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    margin-bottom: 18px;
  }
  .switch-row .muted {
    display: block;
    color: var(--md-sys-color-on-surface-variant);
    font-size: 0.875rem;
  }
  .waiting {
    display: grid;
    place-items: center;
    padding: 48px 0;
  }
  .setting {
    display: grid;
    gap: 10px;
    margin-bottom: 20px;
    color: var(--md-sys-color-on-surface-variant);
  }
  .row {
    display: flex;
    justify-content: space-between;
  }
</style>
