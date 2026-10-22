<script lang="ts">
  // Ein Eintrag des Versionsverlaufs (seit 0.5.4 nach Vorbild des Users): Kopf mit der Version als Chip, daneben Titel
  // und Unterzeile, darunter die Abschnitte Neu, Verbessert, Behoben, Entfernt als Etiketten mit Stichpunkten
  // „**Stichwort:** Satz“. Gezeigt wird die Kurzfassung (k) mit dem Link „Alle Details auf GitHub“; Einträge ohne
  // Kurzfassung (Update-Dateien und Releases vor 0.5.4) zeigen wie früher alle Abschnitte.
  import { Icon } from './m3';
  import { dateDe, releaseUrl, REPO_URL, SECTIONS, type ChangeEntry } from './changelog';

  let {
    entry,
    mark = '',
    open = $bindable(true),
    toggle = false,
  }: {
    entry: ChangeEntry;
    /** Markierung im Kopf, z. B. „Installiert“ */
    mark?: string;
    open?: boolean;
    /** Kopf klickbar zum Auf- und Zuklappen */
    toggle?: boolean;
  } = $props();

  const k = $derived(entry.k);
  const src = $derived((k ?? entry) as Record<string, string[] | undefined>);
  const sections = $derived(
    (k ? SECTIONS.slice(0, 4) : SECTIONS).map((s) => ({ ...s, items: src[s.key] ?? [] })).filter((s) => s.items.length),
  );
  const sub = $derived([k?.s ?? entry.s, entry.d ? dateDe(entry.d) : 'in Arbeit'].filter(Boolean).join(' · '));
  const split = (item: string) => {
    const i = item.indexOf(': ');
    return i > 0 && i < 32 ? [item.slice(0, i), item.slice(i + 2)] : ['', item];
  };
</script>

<article class="entry">
  {#snippet head()}
    <span class="v tnum">{entry.v}</span>
    <span class="title">
      <span class="t">{entry.t || `Pixelia ${entry.v}`}{#if mark}<span class="mark">{mark}</span>{/if}</span>
      <span class="sub">{sub}</span>
    </span>
  {/snippet}
  {#if toggle}
    <button class="head" type="button" aria-expanded={open} onclick={() => (open = !open)}>
      {@render head()}<span class="chev" class:up={open}><Icon name="keyboard_arrow_down" /></span>
    </button>
  {:else}
    <div class="head">{@render head()}</div>
  {/if}

  {#if open}
    {#each sections as s (s.key)}
      <section class="sec {s.key}">
        <h4><Icon name={s.icon} size={16} />{s.label}</h4>
        <ul>
          {#each s.items as item, n (n)}
            {@const [area, text] = split(item)}
            <li>{#if area}<b>{area}:</b>{' '}{/if}{text}</li>
          {/each}
        </ul>
      </section>
    {/each}
    {#if k}
      <a class="more" href={releaseUrl(entry)} target="_blank" rel="noopener">Alle Details auf GitHub<Icon name="open_in_new" size={16} /></a>
    {:else if entry.pr?.length}
      <p class="prs">
        Pull Requests:
        {#each entry.pr as n, i (n)}{i ? ', ' : ' '}<a href="{REPO_URL}/pull/{n}" target="_blank" rel="noopener">#{n}</a>{/each}
      </p>
    {/if}
  {/if}
</article>

<style>
  .head {
    display: flex;
    align-items: center;
    gap: 14px;
    width: 100%;
    padding: 0;
    border: 0;
    background: none;
    color: inherit;
    font: inherit;
    text-align: left;
  }
  button.head {
    cursor: pointer;
  }
  .v {
    flex: none;
    padding: 8px 12px;
    border-radius: var(--md-sys-shape-corner-medium);
    background: var(--md-sys-color-primary-container);
    color: var(--md-sys-color-on-primary-container);
    font: var(--md-sys-typescale-title-medium);
    font-weight: 750;
  }
  .title {
    flex: 1;
    min-width: 0;
    display: grid;
    gap: 2px;
  }
  .t {
    font: var(--md-sys-typescale-title-medium);
    font-weight: 700;
    font-stretch: 105%;
  }
  .mark {
    display: inline-block;
    margin-left: 8px;
    padding: 1px 8px;
    border-radius: var(--md-sys-shape-corner-full);
    background: var(--md-sys-color-primary);
    color: var(--md-sys-color-on-primary);
    font: var(--md-sys-typescale-label-small);
    font-weight: 650;
    vertical-align: 2px;
  }
  .sub {
    color: var(--md-sys-color-on-surface-variant);
    font: var(--md-sys-typescale-body-medium);
  }
  .chev {
    display: grid;
    color: var(--md-sys-color-on-surface-variant);
    transition: transform var(--m3-spatial-fast);
  }
  .chev.up {
    transform: rotate(180deg);
  }
  /* Farbe je Abschnitt: Neu = Primärfarbe kräftig (M3 Expressive), Verbessert = Tertiär, Behoben = Grün,
     Entfernt = Rot, Geprüft = Sekundär, Hinweise = Bernstein */
  .n {
    --bg: var(--md-sys-color-primary);
    --fg: var(--md-sys-color-on-primary);
  }
  .i {
    --bg: var(--md-sys-color-tertiary-container);
    --fg: var(--md-sys-color-on-tertiary-container);
  }
  .f {
    --bg: var(--pixelia-color-success-container);
    --fg: var(--pixelia-color-on-success-container);
  }
  .r {
    --bg: var(--md-sys-color-error-container);
    --fg: var(--md-sys-color-on-error-container);
  }
  .g {
    --bg: var(--md-sys-color-secondary-container);
    --fg: var(--md-sys-color-on-secondary-container);
  }
  .h {
    --bg: var(--pixelia-color-warning-container);
    --fg: var(--pixelia-color-on-warning-container);
  }
  .sec {
    margin-top: 16px;
  }
  h4 {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    margin: 0 0 8px;
    padding: 3px 12px 3px 8px;
    border-radius: var(--md-sys-shape-corner-full);
    background: var(--bg);
    color: var(--fg);
    font: var(--md-sys-typescale-label-medium);
    font-weight: 750;
    letter-spacing: 0.06em;
    text-transform: uppercase;
  }
  ul {
    display: grid;
    gap: 8px;
    margin: 0;
    padding-left: 22px;
  }
  li {
    font: var(--md-sys-typescale-body-large);
    line-height: 1.45;
  }
  li::marker {
    color: var(--md-sys-color-on-surface-variant);
  }
  li b {
    font-weight: 700;
  }
  .more {
    display: inline-flex;
    align-items: center;
    gap: 4px;
    margin-top: 16px;
    color: var(--md-sys-color-primary);
    font: var(--md-sys-typescale-label-large);
    font-weight: 650;
    text-decoration: none;
  }
  .more:hover {
    text-decoration: underline;
  }
  .prs {
    margin: 16px 0 0;
    color: var(--md-sys-color-on-surface-variant);
  }
  .prs a {
    color: var(--md-sys-color-primary);
    font-weight: 600;
  }
</style>
