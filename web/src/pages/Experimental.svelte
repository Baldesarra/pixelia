<script lang="ts">
  // Seite „Experimentell“ (seit 0.4.2, exper.h): Schalter (Standard aus, auf dem Gerät gespeichert), Stromsparmodus und
  // Panel-Timing im Probelauf (ohne Bestätigung nach 15 s zurück), Displaytests, Touch-Test, ältere Version von GitHub.
  import { onMount } from 'svelte';
  import { Button, Card, Chip, Icon, Switch, TextField, WavyProgress, confirmDialog } from '../lib/m3';
  import { get, post, type ExpState, type GhState, type PanelValues } from '../lib/api';
  import { device } from '../lib/status.svelte';
  import { num } from '../lib/format';
  import { cmpVersion, dateDe, parseFw } from '../lib/changelog';

  let ex = $state<ExpState | null>(null);
  let failed = $state(false);
  let msg = $state<{ ok: boolean; text: string } | null>(null);
  let busy = $state(false);

  async function load() {
    const r = await get<ExpState>('/api/exp').catch(() => null);
    if (r) [ex, failed] = [r, false];
    else failed = true;
  }

  // Abfrage: 4×/s beim Touch-Test, 1×/s während Probe oder Testbild, sonst alle 5 s
  onMount(() => {
    void load();
    let stop = false;
    const loop = async () => {
      while (!stop) {
        const fast = ex?.mode === 'touch' ? 250 : ex && (ex.trialS >= 0 || ex.testS >= 0) ? 1000 : 5000;
        await new Promise((r) => setTimeout(r, fast));
        if (!document.hidden && !stop) await load();
      }
    };
    void loop();
    return () => (stop = true);
  });

  async function act(data: Record<string, string | number>, okText?: string) {
    busy = true;
    msg = null;
    const r = await post('/api/exp', data).catch(() => null);
    busy = false;
    if (!r?.ok) msg = { ok: false, text: r?.msg ?? 'Pixelia antwortet nicht' };
    else if (okText) msg = { ok: true, text: okText };
    await load();
    return !!r?.ok;
  }

  async function toggle(on: boolean) {
    if (!on && ex?.saved && !same(ex.saved, ex.std)) {
      const ok = await confirmDialog({
        title: 'Experimentell ausschalten?',
        text: 'Eigene Panel-Werte und der Stromsparmodus werden dabei gelöscht, das Panel läuft wieder mit den Standardwerten.',
        ok: 'Ausschalten',
        icon: 'science',
      });
      if (!ok) return load();
    }
    await act({ a: on ? 'on' : 'off' });
  }

  // --- Panel-Timing und Stromsparmodus --------------------------------------------------------------------
  const keys = ['hp', 'hb', 'hf', 'vp', 'vb', 'vf'] as const;
  let form = $state<Record<string, string>>({ pclk: '', hp: '', hb: '', hf: '', vp: '', vb: '', vf: '' });
  let formFrom = '';
  const same = (a: PanelValues, b: PanelValues) =>
    a.pclk === b.pclk && keys.every((k) => a[k] === b[k]) && a.hlow === b.hlow && a.vlow === b.vlow && a.neg === b.neg && a.eco === b.eco;
  // Formular aus den aktiven Werten füllen, sobald sie sich auf dem Gerät ändern (nicht beim Tippen)
  $effect(() => {
    if (!ex) return;
    const key = JSON.stringify(ex.panel);
    if (key === formFrom) return;
    formFrom = key;
    form = { pclk: String(ex.panel.pclk / 1000), ...Object.fromEntries(keys.map((k) => [k, String(ex!.panel[k])])) };
  });
  const formHz = $derived.by(() => {
    const f = (k: string) => Number(form[k]?.replace(',', '.'));
    const ht = 800 + f('hp') + f('hb') + f('hf');
    const vt = 480 + f('vp') + f('vb') + f('vf');
    const hz = (f('pclk') * 1e6) / (ht * vt) / (ex?.panel.eco ? 2 : 1);
    return Number.isFinite(hz) && hz > 0 ? hz : null;
  });

  function panelData(p: Partial<PanelValues> & { pclk: number }): Record<string, string | number> {
    const b = ex!.panel;
    const v = { ...b, ...p };
    return {
      a: 'panel',
      pclk: v.pclk,
      ...Object.fromEntries(keys.map((k) => [k, v[k]])),
      hlow: v.hlow ? 1 : 0,
      vlow: v.vlow ? 1 : 0,
      neg: v.neg ? 1 : 0,
      eco: v.eco ? 1 : 0,
    };
  }

  async function tryForm() {
    const p: Partial<PanelValues> & { pclk: number } = { pclk: Math.round(Number(form.pclk.replace(',', '.')) * 1000) };
    for (const k of keys) p[k] = Math.round(Number(form[k]));
    if ([p.pclk, ...keys.map((k) => p[k])].some((v) => !Number.isFinite(v))) return (msg = { ok: false, text: 'Bitte nur Zahlen eintragen.' });
    await act(panelData(p));
  }
  const setEco = (on: boolean) => act(panelData({ pclk: ex!.panel.pclk, eco: on }));
  const setPol = (k: 'hlow' | 'vlow' | 'neg', on: boolean) => act(panelData({ pclk: ex!.panel.pclk, [k]: on }));

  // --- Displaytests ---------------------------------------------------------------------------------------
  const tests = [
    { mode: 'bars', label: 'Farbbalken', icon: 'palette' },
    { mode: 'frame', label: 'Rahmen', icon: 'crop_square' },
    { mode: 'motion', label: 'Bewegung', icon: 'motion_photos_on' },
  ] as const;

  // --- Ältere Version von GitHub ---------------------------------------------------------------------------
  let gh = $state<GhState | null>(null);
  let ghBusy = $state(false);
  let ghMsg = $state('');
  async function loadOld() {
    ghBusy = true;
    ghMsg = '';
    let st = await get<GhState>('/api/update').catch(() => null);
    if (st && st.checkedS < 0) {
      const r = await post('/api/update/check').catch(() => null);
      if (!r?.ok) ghMsg = r?.msg ?? 'Pixelia antwortet nicht';
      for (let i = 0; r?.ok && i < 40; i++) {
        await new Promise((res) => setTimeout(res, 1000));
        st = await get<GhState>('/api/update').catch(() => null);
        if (st && st.job !== 'check') break;
      }
    }
    if (st?.err) ghMsg = st.err;
    gh = st;
    ghBusy = false;
  }
  async function installOld(v: string, size: number) {
    const ok = await confirmDialog({
      title: `Ältere Version ${v} installieren?`,
      text:
        `Pixelia lädt ${v} (${num(size / 1024)} KB) von GitHub und startet damit neu. Einstellungen, die ${v} noch nicht kennt, ` +
        `können dabei verloren gehen. ${cmpVersion(v, '0.4.1') < 0 ? `Achtung: ${v} kann selbst nicht von GitHub updaten, zurück geht es nur über „Aus Datei“ auf der Seite System. ` : ''}` +
        'Danach folgt wie immer der Probelauf mit Selbsttest.',
      ok: 'Trotzdem installieren',
      icon: 'warning',
    });
    if (!ok) return;
    const r = await post('/api/update/install', { v, old: 1 }).catch(() => null);
    if (!r?.ok) return (ghMsg = r?.msg ?? 'Pixelia antwortet nicht');
    location.hash = '#/system'; // Fortschritt, Abbrechen und Probelauf zeigt die Seite System
  }

  const cornerNames = ['oben links', 'oben rechts', 'unten links', 'unten rechts'];
  const s = $derived(device.data);
</script>

{#if !ex}
  <Card><p class="muted">{failed ? 'Pixelia antwortet nicht.' : 'Lade …'}</p></Card>
{:else}
  <div class="page-grid">
    <Card class="wide">
      <div class="head">
        <Icon name="science" size={32} />
        <div class="grow">
          <h2 class="card-title">Experimentell</h2>
          <p>
            Funktionen zum Ausprobieren und für die Fehlersuche, jede mit „(Vorschau)“ gekennzeichnet. Standard aus, die Wahl
            bleibt auf dem Gerät gespeichert.
          </p>
        </div>
        <Switch label="Experimentell" checked={ex.on} disabled={busy} onchange={toggle} />
      </div>
      {#if msg}<p class={msg.ok ? 'ok-text' : 'err-text'}>{msg.text}</p>{/if}
    </Card>

    {#if ex.safeBoot}
      <Card variant="error" class="wide">
        <div class="alert">
          <Icon name="warning" />
          <p class="grow">
            Pixelia wurde zweimal kurz nach dem Start neu gestartet und läuft deshalb mit den Standardwerten des Panels. Die
            gespeicherten eigenen Werte ({num(ex.saved.hz, 1)} Hz{ex.saved.eco ? ', Stromsparmodus' : ''}) gelten erst wieder
            nach „Ausprobieren“ und Bestätigen.
          </p>
          <Button variant="tonal" onclick={() => act({ a: 'reset' }, 'Standardwerte gespeichert')}>Werte löschen</Button>
        </div>
      </Card>
    {/if}

    {#if ex.trialS >= 0}
      <Card variant="primary" class="wide">
        <h2 class="card-title"><Icon name="tune" size={20} />Neue Panel-Werte auf Probe</h2>
        <p>
          Ist das Bild auf dem Display in Ordnung? Ohne Bestätigung stellt Pixelia in <b class="tnum">{ex.trialS} s</b> die vorigen
          Werte wieder her.
        </p>
        <WavyProgress value={ex.trialS / ex.trialNeed} label="Zeit bis zum Rückfall" />
        <div class="actions">
          <Button icon="check" disabled={busy} onclick={() => act({ a: 'confirm' }, 'Panel-Werte gespeichert')}>Behalten</Button>
          <Button variant="tonal" icon="undo" disabled={busy} onclick={() => act({ a: 'revert' }, 'Vorige Werte wiederhergestellt')}>Zurück</Button>
        </div>
      </Card>
    {/if}

    {#if ex.on}
      <Card>
        <div class="head">
          <Icon name="energy_savings_leaf" size={24} />
          <div class="grow">
            <h2 class="card-title">Stromsparmodus (Vorschau)</h2>
            <p class="muted">
              Panel mit halbem Pixeltakt ({num(ex.std.hz / 2, 0)} statt {num(ex.std.hz, 0)} Hz), jedes Bild der Welt wird nur
              einmal gezeigt. Die Welt bleibt bei 30 fps, der Hochskalier-Interrupt und der Speicherverkehr halbieren sich.
              Manche Panels flimmern dabei oder bleiben schwarz.
            </p>
          </div>
          <Switch label="Stromsparmodus" checked={ex.panel.eco} disabled={busy || ex.trialS >= 0} onchange={setEco} />
        </div>
      </Card>

      <Card>
        <h2 class="card-title"><Icon name="monitor" size={20} />Displaytests (Vorschau)</h2>
        <p class="muted">Testbilder auf dem Display; nach {60} s läuft die Welt von selbst weiter.</p>
        <div class="actions">
          {#each tests as t (t.mode)}
            <Button variant={ex.mode === t.mode ? 'filled' : 'tonal'} disabled={busy || ex.trialS >= 0} onclick={() => act({ a: 'test', mode: t.mode })}>
              {t.label}
            </Button>
          {/each}
          {#if ex.mode !== 'engine'}
            <Button variant="text" icon="close" onclick={() => act({ a: 'test', mode: 'engine' })}>Zurück zur Welt</Button>
          {/if}
        </div>
        {#if ex.testS >= 0}<p class="muted small tnum">Testbild „{ex.mode}“ noch {ex.testS} s</p>{/if}
      </Card>

      <Card class="wide">
        <h2 class="card-title">
          <Icon name="touch_app" size={20} />Touch-Test (Vorschau)
          {#if ex.mode === 'touch'}<span class="end"><Chip pulse tone="primary">läuft · noch {ex.testS} s</Chip></span>{/if}
        </h2>
        {#if !ex.touch.ok}
          <p class="err-text">Kein Touch-Controller gefunden.</p>
        {:else}
          <p class="muted">
            Display berühren: Der Punkt zeigt die gemessene Stelle hier und auf dem Display. Alle vier Ecken antippen, um die
            Ränder zu prüfen.
          </p>
          <div class="touch" class:live={ex.mode === 'touch'}>
            {#each [0, 1, 2, 3] as c (c)}
              <span class="corner c{c}" class:hit={(ex.touch.corners >> c) & 1} title={cornerNames[c]}></span>
            {/each}
            {#if ex.touch.x >= 0}
              <span class="finger" class:down={ex.touch.down} style:left="{(ex.touch.x / 800) * 100}%" style:top="{(ex.touch.y / 480) * 100}%"></span>
            {/if}
          </div>
          <p class="muted small tnum">
            {ex.touch.x >= 0 ? `Letzter Punkt x ${ex.touch.x}, y ${ex.touch.y}` : 'Noch keine Berührung'} · {ex.touch.n} Berührungen seit
            dem Start · Ecken {[0, 1, 2, 3].filter((c) => (ex!.touch.corners >> c) & 1).length} von 4
          </p>
          <div class="actions">
            <Button variant={ex.mode === 'touch' ? 'tonal' : 'filled'} icon="touch_app" disabled={busy || ex.trialS >= 0} onclick={() => act({ a: 'test', mode: 'touch' })}>
              {ex.mode === 'touch' ? 'Neu starten' : 'Touch-Test starten'}
            </Button>
            {#if ex.mode === 'touch'}<Button variant="text" icon="close" onclick={() => act({ a: 'test', mode: 'engine' })}>Beenden</Button>{/if}
          </div>
        {/if}
      </Card>

      <Card class="wide">
        <h2 class="card-title"><Icon name="tune" size={20} />Panel-Timing (Vorschau)</h2>
        <p class="muted">
          Pixeltakt und Austastzeiten des RGB-Panels in Takten (Sync-Impuls, Schwarzschulter hinten und vorn). Falsche Werte machen das Bild unruhig oder schwarz; jede Änderung läuft
          deshalb erst {ex.trialNeed} s auf Probe. Standard: {num(ex.std.pclk / 1000, 1)} MHz, H {ex.std.hp}/{ex.std.hb}/{ex.std.hf}, V
          {ex.std.vp}/{ex.std.vb}/{ex.std.vf} = {num(ex.std.hz, 1)} Hz.
        </p>
        <div class="fields">
          <TextField label="Takt (MHz)" size={6} inputmode="decimal" bind:value={form.pclk} />
          <TextField label="H Sync" size={6} inputmode="numeric" bind:value={form.hp} />
          <TextField label="H hinten" size={6} inputmode="numeric" bind:value={form.hb} />
          <TextField label="H vorn" size={6} inputmode="numeric" bind:value={form.hf} />
          <TextField label="V Sync" size={6} inputmode="numeric" bind:value={form.vp} />
          <TextField label="V hinten" size={6} inputmode="numeric" bind:value={form.vb} />
          <TextField label="V vorn" size={6} inputmode="numeric" bind:value={form.vf} />
        </div>
        <div class="pols">
          <span class="pol"><Switch label="H-Sync aktiv low" checked={ex.panel.hlow} disabled={busy || ex.trialS >= 0} onchange={(v) => setPol('hlow', v)} />H-Sync in Ruhe low</span>
          <span class="pol"><Switch label="V-Sync aktiv low" checked={ex.panel.vlow} disabled={busy || ex.trialS >= 0} onchange={(v) => setPol('vlow', v)} />V-Sync in Ruhe low</span>
          <span class="pol"><Switch label="Takt fallende Flanke" checked={ex.panel.neg} disabled={busy || ex.trialS >= 0} onchange={(v) => setPol('neg', v)} />Daten bei fallender Flanke</span>
        </div>
        <p class="tnum">
          Ergibt {formHz ? `${num(formHz, 1)} Hz` : '–'}{ex.panel.eco ? ' (mit Stromsparmodus)' : ''} · läuft gerade: {num(ex.panel.hz, 1)} Hz, Bild der
          Welt {ex.panel.swap}× gezeigt
        </p>
        <div class="actions">
          <Button icon="tune" disabled={busy || ex.trialS >= 0} onclick={tryForm}>Ausprobieren</Button>
          <Button variant="tonal" icon="restart_alt" disabled={busy || (same(ex.panel, ex.std) && same(ex.saved, ex.std))} onclick={() => act({ a: 'reset' }, 'Standardwerte gespeichert')}>
            Standardwerte
          </Button>
        </div>
        <p class="muted small">
          Fluchtweg ohne Browser: Läuft das Bild nach dem Speichern nicht, Pixelia zweimal innerhalb einer Minute nach dem Start
          vom Strom trennen. Der dritte Start nimmt die Standardwerte.
        </p>
      </Card>

      <Card variant="outlined" class="wide">
        <h2 class="card-title"><Icon name="history" size={20} />Ältere Version installieren (Vorschau)</h2>
        <p>
          <b>Achtung:</b> Zurück auf eine ältere Version hilft, wenn eine neue Version Probleme macht. Einstellungen, die die
          ältere Version noch nicht kennt, können verloren gehen. Versionen vor 0.4.1 können nicht selbst von GitHub updaten.
        </p>
        {#if !gh}
          <div class="actions">
            <Button variant="tonal" icon="cloud_download" disabled={ghBusy} onclick={loadOld}>{ghBusy ? 'Lade …' : 'Versionen von GitHub laden'}</Button>
          </div>
        {:else if !gh.old?.length}
          <p class="muted">Keine ältere Version ab 0.4.0 im Kanal {gh.ch === 'stable' ? 'Stabil' : 'Vorschau'} gefunden.</p>
        {:else}
          <ul class="list">
            {#each gh.old as o (o.v)}
              <li class="list-item">
                <span class="lead"><Icon name="history" /></span>
                <span class="text">
                  <span class="headline">Pixelia {o.v}{o.pre ? ' (Vorschau)' : ''}</span>
                  <span class="supporting tnum">{dateDe(o.date)} · {num(o.size / 1024)} KB</span>
                </span>
                <Button variant="text" disabled={!!s?.ota.busy || cmpVersion(o.v, parseFw(s?.v ?? '0').base) >= 0} onclick={() => installOld(o.v, o.size)}>Installieren</Button>
              </li>
            {/each}
          </ul>
        {/if}
        {#if ghMsg}<p class="err-text">{ghMsg}</p>{/if}
      </Card>
    {/if}
  </div>
{/if}

<style>
  .head {
    display: flex;
    align-items: flex-start;
    gap: 16px;
  }
  .head p {
    margin: 4px 0 0;
  }
  .grow {
    flex: 1;
    min-width: 0;
  }
  .alert {
    display: flex;
    align-items: center;
    gap: 12px;
    flex-wrap: wrap;
  }
  .fields {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(120px, 1fr));
    gap: 8px 12px;
    margin: 12px 0;
  }
  .pols {
    display: flex;
    flex-wrap: wrap;
    gap: 12px 24px;
  }
  .pol {
    display: inline-flex;
    align-items: center;
    gap: 10px;
  }
  .touch {
    position: relative;
    aspect-ratio: 5 / 3;
    max-width: 480px;
    margin: 12px 0;
    border-radius: var(--md-sys-shape-corner-medium);
    background: var(--md-sys-color-surface-container-highest);
    outline: 2px dashed var(--md-sys-color-outline-variant);
    overflow: hidden;
  }
  .touch.live {
    outline-color: var(--md-sys-color-primary);
  }
  .corner {
    position: absolute;
    width: 12.5%;
    height: 20.8%;
    background: color-mix(in srgb, var(--md-sys-color-outline) 25%, transparent);
  }
  .corner.hit {
    background: color-mix(in srgb, var(--md-sys-color-primary) 55%, transparent);
  }
  .c0 {
    left: 0;
    top: 0;
  }
  .c1 {
    right: 0;
    top: 0;
  }
  .c2 {
    left: 0;
    bottom: 0;
  }
  .c3 {
    right: 0;
    bottom: 0;
  }
  .finger {
    position: absolute;
    width: 24px;
    height: 24px;
    margin: -12px 0 0 -12px;
    border-radius: 50%;
    border: 3px solid var(--md-sys-color-tertiary);
    transition:
      left 0.2s linear,
      top 0.2s linear;
  }
  .finger.down {
    background: var(--md-sys-color-tertiary);
  }
</style>
