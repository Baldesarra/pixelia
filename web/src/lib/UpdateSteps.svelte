<script lang="ts" module>
  export type StepState = 'fertig' | 'jetzt' | 'warn' | 'fehler' | 'offen';
</script>

<script lang="ts">
  // Schritt-Leiste eines Updates (seit 0.5.11, Wahl des Users, Skizze docs/skizzen/0511-web-entwuerfe.png):
  // Übertragen bzw. Laden → Prüfen → Neustart → Selbsttest, erledigte mit Haken, der laufende hervorgehoben,
  // „warn“ orange (Antwort fehlt, Bestätigen klappt nicht), „fehler“ rot (Rückfall)
  import { Icon } from './m3';

  let { first = 'Übertragen', states }: { first?: string; states: StepState[] } = $props();
  const names = $derived([first, 'Prüfen', 'Neustart', 'Selbsttest']);
</script>

<ol class="steps" aria-label="Ablauf des Updates">
  {#each names as n, k (k)}
    <li class={states[k] ?? 'offen'} aria-current={states[k] === 'jetzt' ? 'step' : undefined}>
      <span class="dot">
        {#if states[k] === 'fertig'}<Icon name="check" size={16} />{:else if states[k] === 'warn' || states[k] === 'fehler'}!{:else}{k + 1}{/if}
      </span>
      <span class="nm">{n}</span>
    </li>
  {/each}
</ol>

<style>
  .steps {
    display: grid;
    grid-template-columns: repeat(4, 1fr);
    gap: 4px;
    margin: 4px 0 16px;
    padding: 0;
    list-style: none;
  }
  li {
    position: relative;
    display: grid;
    justify-items: center;
    gap: 6px;
  }
  /* Verbindungslinie zum vorigen Schritt, farbig, wenn der vorige erledigt ist */
  li + li::before {
    content: '';
    position: absolute;
    top: 13px;
    right: calc(50% + 18px);
    width: calc(100% - 32px);
    height: 2px;
    background: var(--md-sys-color-outline-variant);
  }
  li.fertig + li::before {
    background: var(--md-sys-color-primary);
  }
  .dot {
    display: grid;
    place-items: center;
    width: 28px;
    height: 28px;
    border-radius: 50%;
    font: var(--md-sys-typescale-label-large);
    font-weight: 700;
    background: var(--md-sys-color-surface-container-highest);
    color: var(--md-sys-color-on-surface-variant);
  }
  .fertig .dot {
    background: var(--md-sys-color-primary);
    color: var(--md-sys-color-on-primary);
  }
  .jetzt .dot {
    background: var(--md-sys-color-tertiary);
    color: var(--md-sys-color-on-tertiary);
    box-shadow: 0 0 0 4px var(--md-sys-color-tertiary-container);
  }
  /* Orange wie „Update vorhanden“ (feste Töne, Orange gibt es im Schema nicht) */
  .warn .dot {
    background: #ffb68d;
    color: #341100;
    box-shadow: 0 0 0 4px #ffdbc8;
  }
  :global([data-theme='dark']) .warn .dot {
    box-shadow: 0 0 0 4px #723600;
  }
  .fehler .dot {
    background: var(--md-sys-color-error);
    color: var(--md-sys-color-on-error);
    box-shadow: 0 0 0 4px var(--md-sys-color-error-container);
  }
  .nm {
    font: var(--md-sys-typescale-label-medium);
    color: var(--md-sys-color-on-surface-variant);
    text-align: center;
  }
  .jetzt .nm,
  .warn .nm,
  .fehler .nm {
    color: var(--md-sys-color-on-surface);
    font-weight: 700;
  }
</style>
