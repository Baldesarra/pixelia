<script lang="ts" module>
  import type { IconName } from './m3/icon-names';

  export type HubItem = {
    href: string;
    icon: IconName;
    title: string;
    /** zweite Zeile, z. B. der aktuelle Zustand („verbunden mit …“) */
    sub?: string;
    /** Hinweis-Ton der zweiten Zeile (z. B. Absturzbericht vorhanden) */
    tone?: 'error' | 'primary';
    /** kleiner Punkt am Symbol (z. B. neue Version) */
    badge?: boolean;
    /** eigene Seite außerhalb der Oberfläche (z. B. /setup), öffnet neu */
    ext?: boolean;
  };
  export type HubGroup = { title: string; items: HubItem[] };
</script>

<script lang="ts">
  // Liste mit Kategorien (Seiten System, Einstellungen, Experimentell): Gruppentitel in der Primärfarbe, darunter
  // Zeilen als zusammenhängende Kacheln (oben und unten stärker gerundet), Symbol im Tonal-Feld, Zustand als zweite
  // Zeile, Pfeil nach rechts. In der breiten Ansicht stehen die Gruppen nebeneinander.
  import Icon from './m3/Icon.svelte';

  let { groups }: { groups: HubGroup[] } = $props();
</script>

<div class="hub">
  {#each groups as g (g.title)}
    {#if g.items.length}
      <section>
        <h2>{g.title}</h2>
        <ul>
          {#each g.items as it (it.href)}
            <li>
              <a href={it.href} target={it.ext ? '_blank' : undefined} rel={it.ext ? 'noopener' : undefined}>
                <span class="ic"><Icon name={it.icon} />{#if it.badge}<span class="badge"></span>{/if}</span>
                <span class="tx">
                  <span class="t">{it.title}</span>
                  {#if it.sub}<span class="s {it.tone ?? ''}">{it.sub}</span>{/if}
                </span>
                <span class="end"><Icon name={it.ext ? 'open_in_new' : 'chevron_right'} /></span>
              </a>
            </li>
          {/each}
        </ul>
      </section>
    {/if}
  {/each}
</div>

<style>
  .hub {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(min(100%, 360px), 1fr));
    gap: 8px 16px;
    align-items: start;
  }
  h2 {
    margin: 12px 0 8px 4px;
    font: var(--md-sys-typescale-label-large);
    font-weight: 700;
    color: var(--md-sys-color-primary);
  }
  ul {
    display: grid;
    gap: 3px;
    margin: 0;
    padding: 0;
    list-style: none;
  }
  a {
    display: flex;
    align-items: center;
    gap: 16px;
    min-height: 72px;
    padding: 10px 16px;
    border-radius: var(--md-sys-shape-corner-extra-small);
    background: var(--md-sys-color-surface-container);
    color: var(--md-sys-color-on-surface);
    text-decoration: none;
    transition:
      background-color var(--m3-effects),
      border-radius var(--m3-spatial-fast);
  }
  li:first-child a {
    border-top-left-radius: var(--md-sys-shape-corner-extra-large);
    border-top-right-radius: var(--md-sys-shape-corner-extra-large);
  }
  li:last-child a {
    border-bottom-left-radius: var(--md-sys-shape-corner-extra-large);
    border-bottom-right-radius: var(--md-sys-shape-corner-extra-large);
  }
  a:hover {
    background: color-mix(in srgb, var(--md-sys-color-on-surface) 6%, var(--md-sys-color-surface-container));
  }
  a:active {
    border-radius: var(--md-sys-shape-corner-extra-large);
  }
  .ic {
    position: relative;
    display: grid;
    place-items: center;
    flex: none;
    width: 44px;
    height: 44px;
    border-radius: var(--md-sys-shape-corner-medium);
    background: var(--md-sys-color-secondary-container);
    color: var(--md-sys-color-on-secondary-container);
  }
  .badge {
    position: absolute;
    top: 6px;
    right: 6px;
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: var(--md-sys-color-error);
  }
  .tx {
    flex: 1;
    min-width: 0;
    display: grid;
    gap: 2px;
  }
  .t {
    font: var(--md-sys-typescale-title-medium);
    font-weight: 650;
  }
  .s {
    font: var(--md-sys-typescale-body-medium);
    color: var(--md-sys-color-on-surface-variant);
  }
  .s.error {
    color: var(--md-sys-color-error);
  }
  .s.primary {
    color: var(--md-sys-color-primary);
  }
  .end {
    display: grid;
    color: var(--md-sys-color-on-surface-variant);
  }
</style>
