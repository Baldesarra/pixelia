<script lang="ts">
  // Seite „Display“ (Einstellungen → Gerät): Bilddrehung, Helligkeit, Dauer des langen Drucks für die Info-Karte
  import { ButtonGroup, Card, Icon, LoadingIndicator, Slider } from '../lib/m3';
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
  </div>
{/if}

<style>
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
