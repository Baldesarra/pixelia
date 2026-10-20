<script lang="ts">
  // Ein Eintrag des Versionsverlaufs: Kopf (Version, Titel, Datum, Markierung) und alle Abschnitte mit Stichpunkten.
  // Der Bereich vor dem Doppelpunkt („WLAN: …“) wird als kleines Etikett gezeigt. Zugeklappt nur der Kopf mit Zahlen.
  import { Icon } from './m3';
  import { dateDe, REPO_URL, SECTIONS, type ChangeEntry } from './changelog';

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

  const sections = $derived(SECTIONS.map((s) => ({ ...s, items: entry[s.key] ?? [] })).filter((s) => s.items.length));
  const count = $derived(sections.reduce((n, s) => n + s.items.length, 0));
  const split = (item: string) => {
    const i = item.indexOf(': ');
    return i > 0 && i < 24 ? [item.slice(0, i), item.slice(i + 2)] : ['', item];
  };
</script>

<article class="entry" class:open>
  {#snippet head()}
    <span class="title">
      <span class="v tnum">{entry.v}</span>
      {#if entry.t}<span class="t">{entry.t}</span>{/if}
      {#if mark}<span class="mark">{mark}</span>{/if}
    </span>
    <span class="sub">
      {entry.d ? dateDe(entry.d) : 'in Arbeit'}{entry.s ? ` · ${entry.s}` : ''}{open ? '' : ` · ${count} Änderungen`}
    </span>
  {/snippet}
  {#if toggle}
    <button class="head" type="button" aria-expanded={open} onclick={() => (open = !open)}>
      {@render head()}<span class="chev"><Icon name="keyboard_arrow_down" /></span>
    </button>
  {:else}
    <div class="head">{@render head()}</div>
  {/if}

  {#if open}
    {#each sections as s (s.key)}
      <h4 class="sec {s.key}"><Icon name={s.icon} size={18} />{s.label}</h4>
      <ul>
        {#each s.items as item, k (k)}
          {@const [area, text] = split(item)}
          <li>{#if area}<span class="area">{area}</span>{/if}{text}</li>
        {/each}
      </ul>
    {/each}
    {#if entry.pr?.length}
      <p class="prs">
        Pull Requests:
        {#each entry.pr as n, k (n)}{k ? ', ' : ' '}<a href="{REPO_URL}/pull/{n}" target="_blank" rel="noopener">#{n}</a>{/each}
      </p>
    {/if}
  {/if}
</article>

<style>
  .head {
    position: relative;
    display: grid;
    gap: 2px;
    width: 100%;
    padding: 0 40px 0 0;
    border: 0;
    background: none;
    color: inherit;
    font: inherit;
    text-align: left;
  }
  button.head {
    cursor: pointer;
  }
  .title {
    display: flex;
    flex-wrap: wrap;
    align-items: baseline;
    gap: 4px 10px;
  }
  .v {
    font: var(--md-sys-typescale-title-large);
    font-weight: 700;
    font-stretch: 110%;
    color: var(--md-sys-color-primary);
  }
  .t {
    font: var(--md-sys-typescale-title-medium);
    font-weight: 600;
  }
  .mark {
    align-self: center;
    padding: 2px 10px;
    border-radius: var(--md-sys-shape-corner-full);
    background: var(--md-sys-color-primary);
    color: var(--md-sys-color-on-primary);
    font: var(--md-sys-typescale-label-medium);
    font-weight: 650;
  }
  .sub {
    color: var(--md-sys-color-on-surface-variant);
    font: var(--md-sys-typescale-body-medium);
  }
  .chev {
    position: absolute;
    top: 4px;
    right: 4px;
    display: grid;
    color: var(--md-sys-color-on-surface-variant);
    transition: transform var(--m3-spatial-fast);
  }
  .open .chev {
    transform: rotate(180deg);
  }
  .sec {
    display: flex;
    align-items: center;
    gap: 6px;
    margin: 16px 0 6px;
    font: var(--md-sys-typescale-label-large);
    font-weight: 650;
    color: var(--md-sys-color-on-surface-variant);
  }
  .sec.n {
    color: var(--md-sys-color-primary);
  }
  .sec.f {
    color: var(--md-sys-color-tertiary);
  }
  .sec.r {
    color: var(--md-sys-color-error);
  }
  ul {
    display: grid;
    gap: 6px;
    margin: 0;
    padding: 0 0 0 4px;
    list-style: none;
  }
  li {
    position: relative;
    padding-left: 14px;
    line-height: 1.45;
  }
  li::before {
    content: '';
    position: absolute;
    left: 0;
    top: 0.62em;
    width: 5px;
    height: 5px;
    border-radius: 2px;
    background: var(--md-sys-color-outline);
  }
  .area {
    display: inline-block;
    margin-right: 6px;
    padding: 0 6px;
    border-radius: var(--md-sys-shape-corner-extra-small);
    background: var(--md-sys-color-secondary-container);
    color: var(--md-sys-color-on-secondary-container);
    font: var(--md-sys-typescale-label-medium);
    font-weight: 600;
    line-height: 1.6;
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
