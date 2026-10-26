<script lang="ts">
  // Karte „Weltkalender“ (seit 0.5.7, Seite Welt): Jahreszeit und Tag, nächster Wechsel, Sonne, Mond, Wetter der
  // Simulation, Erstlinge und die Länge einer Jahreszeit. Stand aus /api/world alle 10 s.
  import { onDestroy } from 'svelte';
  import { ButtonGroup, Card, Icon, type IconName } from './m3';
  import { get, post } from './api';

  type World = {
    valid: boolean;
    started: boolean;
    test: boolean;
    start?: string;
    day?: number;
    year?: number;
    season?: string;
    seasonTitle?: string;
    dayInSeason?: number;
    seasonDays?: number;
    evening?: boolean;
    seasonTo?: string;
    next?: string;
    nextFrom?: string;
    sunrise?: string;
    sunset?: string;
    dayPart?: string;
    moon?: { title: string; illum: number; waxing: boolean; up: boolean; visible: boolean };
    firsts?: string[];
    seasonLen: number;
    weather: { kind: string; title: string; pct: number; override: boolean };
    forecast?: { slot: string; kind: string; title: string; pct: number }[];
  };

  let w = $state<World | null>(null);
  async function load() {
    w = await get<World>('/api/world').catch(() => w);
  }
  load();
  const timer = window.setInterval(load, 10000);
  onDestroy(() => clearInterval(timer));

  const seasonIcon: Record<string, IconName> = {
    fruehling: 'local_florist',
    sommer: 'sunny',
    herbst: 'eco',
    winter: 'ac_unit',
  };
  const weatherIcon: Record<string, IconName> = {
    sonne: 'sunny',
    bewoelkt: 'cloud',
    regen: 'rainy',
    schnee: 'weather_snowy',
    nebel: 'foggy',
    gewitter: 'thunderstorm',
    wind: 'air',
  };
  const days = ['So', 'Mo', 'Di', 'Mi', 'Do', 'Fr', 'Sa'];
  // „So 15.11.“ aus „2026-11-15“
  function day(iso?: string) {
    if (!iso) return '–';
    const [y, m, d] = iso.split('-').map(Number);
    return `${days[new Date(y, m - 1, d).getDay()]} ${d}.${m}.`;
  }
  const cap = (s: string) => s.charAt(0).toUpperCase() + s.slice(1);
  const moonState = $derived(
    !w?.moon ? '' : w.moon.visible ? 'am Himmel' : !w.moon.up ? 'untergegangen' : 'nicht zu sehen',
  );

  // Länge einer Jahreszeit: gilt sofort, bis das Gerät sie bestätigt
  let lenLocal = $state<string | null>(null);
  const len = $derived(lenLocal ?? String(w?.seasonLen ?? 7));
  function setLen(v: string) {
    lenLocal = v;
    void post('/api/settings', { seasonLen: v })
      .then(load)
      .catch(() => null)
      .finally(() => setTimeout(() => (lenLocal = null), 2500));
  }
</script>

<Card>
  <h2 class="card-title"><Icon name="calendar_month" size={20} />Weltkalender</h2>
  {#if !w}
    <p class="muted">Lade …</p>
  {:else if !w.started}
    <p class="muted">
      {w.valid ? 'Der Kalender startet gleich.' : 'Der Kalender startet, sobald das Gerät die Uhrzeit kennt (WLAN mit Internet).'}
    </p>
  {:else}
    <div class="hero">
      <span class="badge"><Icon name={seasonIcon[w.season ?? ''] ?? 'calendar_month'} size={28} /></span>
      <div>
        <b>{w.seasonTitle}, Tag {w.dayInSeason} von {w.seasonDays}</b>
        <span class="muted">Weltjahr {w.year} · Welttag {w.day}{w.test ? ' · Testuhr' : ''}</span>
      </div>
    </div>
    <dl class="kv">
      <dt>Jahreszeitenabend</dt>
      <dd>{w.evening ? 'heute' : day(w.seasonTo)}</dd>
      <dt>{w.next} ab</dt>
      <dd>{day(w.nextFrom)}</dd>
      <dt>Sonne</dt>
      <dd class="tnum">{w.sunrise}–{w.sunset}</dd>
      <dt>Mond</dt>
      <dd>{cap(w.moon?.title ?? '')}, {w.moon?.illum} % · {moonState}</dd>
      <dt>Wetter</dt>
      <dd class="weather">
        <Icon name={weatherIcon[w.weather.kind] ?? 'cloud'} size={18} />{w.weather.title}{w.weather.kind !== 'sonne'
          ? ` ${w.weather.pct} %`
          : ''}{w.weather.override ? ' · Test' : ''}
      </dd>
      {#if w.forecast?.length}
        <dt>Danach</dt>
        <dd class="forecast">
          {#each w.forecast as f, i (i)}
            <span title={f.title}><Icon name={weatherIcon[f.kind] ?? 'cloud'} size={18} />{f.slot}</span>
          {/each}
        </dd>
      {/if}
    </dl>
    {#if w.firsts?.length}
      <div class="firsts">
        {#each w.firsts as f (f)}<span class="chip">{f}</span>{/each}
      </div>
    {/if}
  {/if}
  <div class="setting">
    <span>Tage je Jahreszeit</span>
    <ButtonGroup
      label="Länge einer Jahreszeit"
      options={[
        { value: '3', label: '3' },
        { value: '7', label: '7' },
        { value: '14', label: '14' },
        { value: '28', label: '28' },
      ]}
      bind:value={() => len, setLen}
    />
    <span class="muted small">Bei 7 Tagen wechselt sie in der Nacht auf Montag, sonst am Ende des letzten Tages.</span>
  </div>
</Card>

<style>
  .hero {
    display: flex;
    gap: 14px;
    align-items: center;
    margin-bottom: 14px;
  }
  .hero div {
    display: grid;
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
    background: var(--md-sys-color-tertiary-container);
    color: var(--md-sys-color-on-tertiary-container);
  }
  .weather {
    display: inline-flex;
    justify-content: flex-end;
    align-items: center;
    gap: 6px;
  }
  .forecast {
    display: flex;
    flex-wrap: wrap;
    justify-content: flex-end;
    gap: 4px 12px;
  }
  .forecast span {
    display: inline-flex;
    align-items: center;
    gap: 4px;
    font-weight: 500;
  }
  .firsts {
    display: flex;
    flex-wrap: wrap;
    gap: 6px;
    margin-top: 14px;
  }
  .chip {
    padding: 2px 10px;
    border-radius: var(--md-sys-shape-corner-full);
    background: var(--md-sys-color-surface-container-highest);
    font: var(--md-sys-typescale-label-medium);
  }
  .setting {
    display: grid;
    gap: 10px;
    margin-top: 18px;
    color: var(--md-sys-color-on-surface-variant);
  }
  .small {
    font-size: 0.8125rem;
  }
</style>
