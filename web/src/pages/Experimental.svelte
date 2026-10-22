<script lang="ts">
  // Seite „Experimentell“ (seit 0.4.2, exper.h): Schalter (Standard aus, auf dem Gerät gespeichert); seit 0.5.4 mit
  // Unterseiten: Testorte (Ort, Übergang, Tageszeit, Wetter), Panel und Stromsparen (Probelauf: ohne Bestätigung nach
  // 15 s zurück), ältere Version von GitHub. Die Gerätetests stehen auf der eigenen Seite Gerätetests.
  import { onMount } from 'svelte';
  import { Button, ButtonGroup, Card, Icon, Slider, Switch, TextField, WavyProgress, confirmDialog, theme, type Design } from '../lib/m3';
  import { get, post, type GhState, type PanelValues } from '../lib/api';
  import { device } from '../lib/status.svelte';
  import { exp } from '../lib/exp.svelte';
  import { num } from '../lib/format';
  import { cmpVersion, dateDe, parseFw } from '../lib/changelog';
  import HubList, { type HubGroup } from '../lib/HubList.svelte';

  let { part = 'start' }: { part?: 'start' | 'orte' | 'panel' | 'version' } = $props();

  const ex = $derived(exp.ex);
  const busy = $derived(exp.busy);
  const msg = $derived(exp.msg);
  const act = exp.act;
  const load = exp.load;
  onMount(exp.poll);

  const subpages = $derived<HubGroup[]>([
    {
      title: 'Bereiche',
      items: [
        {
          href: '#/experimentell/orte',
          icon: 'map',
          title: 'Testorte',
          sub: ex?.places?.length ? `${ex.places[ex.place ?? 0]?.title ?? ''} · Tageszeit, Wetter, Übergänge` : 'Ort, Tageszeit, Wetter, Übergänge',
        },
        { href: '#/experimentell/panel', icon: 'tune', title: 'Panel und Stromsparen', sub: ex?.panel.eco ? 'Stromsparmodus an' : 'Pixeltakt, Austastzeiten, Stromsparmodus' },
        { href: '#/experimentell/version', icon: 'history', title: 'Ältere Version installieren', sub: 'Zurück auf eine frühere Version von GitHub' },
      ],
    },
  ]);
  // Rückblick (Easter Egg, auch über 7× Tippen aufs Pixelia-Zeichen): immer sichtbar, auch mit Schalter aus
  const designs: { value: Design; label: string }[] = [
    { value: 'm3', label: 'Material' },
    { value: 'pixel', label: 'Pixelia' },
    { value: 'fahrplan', label: 'Fahrplan' },
  ];

  const story: HubGroup[] = [
    {
      title: 'So entstand Pixelia',
      items: [{ href: '#/experimentell/geschichte', icon: 'auto_stories', title: 'Rückblick', sub: 'Von der Idee bis heute, mit den frühen Skizzen' }],
    },
  ];

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
    if ([p.pclk, ...keys.map((k) => p[k])].some((v) => !Number.isFinite(v))) return (exp.msg = { ok: false, text: 'Bitte nur Zahlen eintragen.' });
    await act(panelData(p));
  }
  const setEco = (on: boolean) => act(panelData({ pclk: ex!.panel.pclk, eco: on }));
  const setPol = (k: 'hlow' | 'vlow' | 'neg', on: boolean) => act(panelData({ pclk: ex!.panel.pclk, [k]: on }));

  // --- Orte (seit 0.5.0) -----------------------------------------------------------------------------------
  const transitions = [
    { value: 'abblenden', label: 'Abblenden' },
    { value: 'ueberblendung', label: 'Überblendung' },
    { value: 'raster', label: 'Raster' },
    { value: 'schnitt', label: 'Schnitt' },
  ];
  // Testuhr (seit 0.5.2): echte Uhrzeit oder Zeitraffer, bis die Weltzeit kommt (0.5.6)
  const speeds = [
    { value: '0', label: 'Echt' },
    { value: '1', label: '1×' },
    { value: '60', label: '60×' },
    { value: '600', label: '600×' },
  ];
  // Wetter (seit 0.5.3): Teststeuerung, bis die Wettersimulation kommt; Übergänge weich (Wolken ziehen auf …)
  const weathers = [
    { value: 'sonne', label: 'Sonne', icon: 'sunny' },
    { value: 'bewoelkt', label: 'Bewölkt', icon: 'cloud' },
    { value: 'regen', label: 'Regen', icon: 'rainy' },
    { value: 'schnee', label: 'Schnee', icon: 'weather_snowy' },
    { value: 'nebel', label: 'Nebel', icon: 'foggy' },
    { value: 'gewitter', label: 'Gewitter', icon: 'thunderstorm' },
    { value: 'wind', label: 'Wind', icon: 'air' },
  ] as const;
  const hhmm = (m: number) => `${String(Math.floor(m / 60)).padStart(2, '0')}:${String(m % 60).padStart(2, '0')}`;

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
    location.hash = '#/update'; // Fortschritt, Abbrechen und Probelauf zeigt die Seite Update
  }

  const s = $derived(device.data);
</script>

{#if !ex}
  <Card><p class="muted">{exp.failed ? 'Pixelia antwortet nicht.' : 'Lade …'}</p></Card>
{:else}
  <div class="page-grid">
    {#if part === 'start'}
      <Card class="wide">
        <div class="head">
          <Icon name="science" size={32} />
          <div class="grow">
            <h2 class="card-title">Experimentell</h2>
            <p>
              Vorläufige Funktionen zum Ausprobieren. Gerätetests gibt es ohne Schalter unter System → Labor.
            </p>
          </div>
          <Switch label="Experimentell" checked={ex.on} disabled={busy} onchange={toggle} />
        </div>
        {#if msg}<p class={msg.ok ? 'ok-text' : 'err-text'}>{msg.text}</p>{/if}
      </Card>
    {:else if msg}
      <p class="wide {msg.ok ? 'ok-text' : 'err-text'}">{msg.text}</p>
    {/if}

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

    {#if !ex.on}
      {#if part !== 'start'}
        <Card class="wide">
          <p class="muted">Experimentell ist aus. Auf der Seite <a href="#/experimentell">Experimentell</a> einschalten.</p>
        </Card>
      {/if}
    {:else if part === 'start'}
      <div class="wide"><HubList groups={subpages} /></div>
      <Card class="wide">
        <h2 class="card-title"><Icon name="palette" size={20} />Design (Vorschau)</h2>
        <p class="muted">
          Anderes Aussehen, gleiche Seiten und Einstellungen. Gilt für alle Browser, solange Experimentell an ist.
        </p>
        <ButtonGroup options={designs} bind:value={theme.design} label="Design" />
      </Card>
    {:else if part === 'orte'}
    {#if ex.places?.length}
      <Card class="wide">
        <h2 class="card-title"><Icon name="map" size={20} />Ort und Figuren</h2>
        <p class="muted">
          Wiese, Raum und Platz hängen über Tür und Bildrand zusammen; der weiße Pfeil zeigt die Figur, der das Bild folgt. Ohne Experimentell zeigt das Display wieder die Landschaft.
        </p>
        <div class="setting">
          <span>Ort</span>
          <div class="choices">
            {#each ex.places as p, i (i)}
              <Button variant={(ex.place ?? 0) === i ? 'filled' : 'tonal'} disabled={busy} onclick={() => act({ a: 'place', n: i })}>{p.title}</Button>
            {/each}
          </div>
        </div>
        <div class="setting">
          <span>Übergang beim Wechsel</span>
          <div class="choices">
            {#each transitions as t (t.value)}
              <Button variant={(ex.trans ?? 'abblenden') === t.value ? 'filled' : 'tonal'} disabled={busy} onclick={() => act({ a: 'trans', v: t.value })}>{t.label}</Button>
            {/each}
          </div>
        </div>
        <div class="setting">
          <span class="row"><span>Figuren je Testort</span><span class="tnum">{ex.figures ? ex.figures : 'Standard'}</span></span>
          <Slider label="Figuren je Testort" min={1} max={64} value={ex.figures || 6} onchange={(v) => act({ a: 'figures', n: v })} />
        </div>
        <div class="actions">
          <span class="pol"><Switch label="Rundgang" checked={!!ex.tour} disabled={busy} onchange={(v) => act({ a: 'tour', s: v ? 20 : 0 })} />Rundgang: alle 20 s der nächste Ort</span>
          {#if ex.figures}<Button variant="text" icon="restart_alt" onclick={() => act({ a: 'figures', n: 0 })}>Figuren: Standard</Button>{/if}
        </div>
      </Card>

      <Card class="wide">
        <h2 class="card-title"><Icon name="schedule" size={20} />Tageszeit</h2>
        <div class="setting">
          <span class="row"><span>Tageszeit (Testuhr)</span><span class="tnum">{hhmm(ex.minute ?? 720)}</span></span>
          <Slider label="Tageszeit" min={0} max={1425} step={15} value={ex.minute ?? 720} onchange={(v) => act({ a: 'time', v })} />
        </div>
        <div class="setting">
          <span>Zeitraffer</span>
          <ButtonGroup
            label="Zeitraffer"
            options={speeds}
            bind:value={() => (ex?.real ? '0' : String(ex?.speed ?? 1)), (v: string) => act(v === '0' ? { a: 'time', v: 'real' } : { a: 'speed', v: Number(v) })}
          />
        </div>
        <p class="muted small">Licht und Himmel folgen der Testuhr; Uhrzeit stellen oder Zeitraffer hält die echte Uhr an.</p>
      </Card>

      <Card class="wide">
        <h2 class="card-title"><Icon name="rainy" size={20} />Wetter</h2>
        <div class="setting">
          <span class="row"><span>Wetter</span><span class="tnum">{ex.particles ? `${ex.particles} Partikel` : ''}</span></span>
          <div class="choices">
            {#each weathers as w (w.value)}
              <Button variant={ex.weather === w.value ? 'filled' : 'tonal'} icon={w.icon} disabled={busy}
                onclick={() => act({ a: 'weather', v: w.value, p: ex?.weatherPct ?? 100 })}>{w.label}</Button>
            {/each}
          </div>
        </div>
        <div class="setting">
          <span class="row"><span>Stärke des Wetters</span><span class="tnum">{ex.weatherPct ?? 100} %</span></span>
          <Slider label="Stärke des Wetters" min={10} max={100} step={10} value={ex.weatherPct ?? 100}
            onchange={(v) => act({ a: 'weather', v: ex?.weather ?? 'sonne', p: v })} />
        </div>
        <div class="actions">
          <span class="pol"><Switch label="Licht innen" checked={ex.innen !== false} disabled={busy} onchange={() => act({ a: 'cmd', v: 'innenlicht um' })} />Licht innen (Lampe im Testraum)</span>
          <Button variant="tonal" icon="bolt" disabled={busy} onclick={() => act({ a: 'cmd', v: 'blitz' })}>Blitz</Button>
        </div>
        <p class="muted small">Gilt für alle Orte und wechselt weich.</p>
      </Card>

      <Card class="wide">
        <h2 class="card-title"><Icon name="construction" size={20} />Baukasten</h2>
        <div class="actions">
          <span class="pol"><Switch label="Blick folgt" checked={ex.follow !== false} disabled={busy} onchange={(v) => act({ a: 'follow', v: v ? 1 : 0 })} />Blick folgt der Begleiterin</span>
          <span class="pol"><Switch label="Prüfansicht" checked={!!ex.inspect} disabled={busy} onchange={(v) => act({ a: 'inspect', v: v ? 1 : 0 })} />Prüfansicht</span>
        </div>
        <div class="actions">
          <Button variant="tonal" icon="upgrade" disabled={busy} onclick={() => act({ a: 'cmd', v: 'ausbau' })}>Ausbau</Button>
          <Button variant="tonal" icon="construction" disabled={busy} onclick={() => act({ a: 'cmd', v: 'baustelle' })}>Im Bau</Button>
        </div>
        <p class="muted small">
          Ausbau: nächste Stufe im Testort. Im Bau: Möbel als Baustelle. Prüfansicht: Stellflächen (lila), Benutzungspunkte (grün), Ausgänge (gelb), versperrt (Punkte).
        </p>
      </Card>
    {/if}
    {:else if part === 'panel'}
    <Card>
      <div class="head">
        <Icon name="energy_savings_leaf" size={24} />
        <div class="grow">
          <h2 class="card-title">Stromsparmodus</h2>
          <p class="muted">
            Halber Pixeltakt ({num(ex.std.hz / 2, 0)} statt {num(ex.std.hz, 0)} Hz) bei gleichen 30 fps und weniger Last. Manche Panels flimmern dabei oder bleiben schwarz.
          </p>
        </div>
        <Switch label="Stromsparmodus" checked={ex.panel.eco} disabled={busy || ex.trialS >= 0} onchange={setEco} />
      </div>
    </Card>

    <Card class="wide">
      <h2 class="card-title"><Icon name="tune" size={20} />Panel-Timing</h2>
      <p class="muted">
        Werte in Takten. Falsche Werte machen das Bild unruhig oder schwarz, deshalb läuft jede Änderung {ex.trialNeed} s auf Probe. Standard: {num(ex.std.pclk / 1000, 1)} MHz, H {ex.std.hp}/{ex.std.hb}/{ex.std.hf}, V {ex.std.vp}/{ex.std.vb}/{ex.std.vf} = {num(ex.std.hz, 1)} Hz.
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
    {:else}
    <Card variant="outlined" class="wide">
      <h2 class="card-title"><Icon name="history" size={20} />Ältere Version installieren</h2>
      <p>
        <b>Achtung:</b> Einstellungen, die die ältere Version nicht kennt, können verloren gehen. Versionen vor 0.4.1 updaten nicht selbst von GitHub.
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
    {#if part === 'start'}
      <div class="wide"><HubList groups={story} /></div>
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
  /* Orte: Beschriftung über Knopfgruppe bzw. Regler (wie auf der Seite System) */
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
  /* Auswahl als umbrechende Knöpfe (Ort, Übergang, Wetter) */
  .choices {
    display: flex;
    flex-wrap: wrap;
    gap: 8px;
  }
  p.wide {
    margin: 0;
  }
</style>
