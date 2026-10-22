<script lang="ts" generics="T extends string">
  // M3-Expressive „Connected Button Group“ zur Auswahl (ersetzt Segmented Buttons):
  // innere Ecken klein, ausgewählt rund, gedrückt wird der Knopf breiter und die Nachbarn weichen
  import Icon from './Icon.svelte';
  import type { IconName } from './icon-names';

  let {
    options,
    value = $bindable(),
    label,
  }: { options: { value: T; label: string; icon?: IconName }[]; value: T; label: string } = $props();
</script>

<div class="group" role="radiogroup" aria-label={label}>
  {#each options as o (o.value)}
    <button
      role="radio"
      aria-checked={value === o.value}
      class:selected={value === o.value}
      onclick={() => (value = o.value)}
    >
      {#if value === o.value}<Icon name="check" size={18} />{:else if o.icon}<Icon name={o.icon} size={18} />{/if}
      <span>{o.label}</span>
    </button>
  {/each}
</div>

<style>
  /* Volle Breite wie die Knöpfe darunter (seit 0.5.4: die Grenze von 480 px ließ die Gruppe in der 640-px-Spalte
     schmaler als die Knöpfe) */
  .group {
    display: flex;
    gap: 2px;
    width: 100%;
  }
  button {
    position: relative;
    flex: 1 1 0;
    min-width: 0;
    display: inline-flex;
    align-items: center;
    justify-content: center;
    gap: 6px;
    height: 40px;
    padding: 0 12px;
    border: none;
    border-radius: var(--md-sys-shape-corner-small);
    background: var(--md-sys-color-secondary-container);
    color: var(--md-sys-color-on-secondary-container);
    font: var(--md-sys-typescale-label-large);
    cursor: pointer;
    overflow: hidden;
    isolation: isolate;
    transition:
      flex-grow var(--m3-spatial-fast),
      border-radius var(--m3-spatial-fast),
      background-color var(--m3-effects),
      color var(--m3-effects);
  }
  button span {
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }
  button::before {
    content: '';
    position: absolute;
    inset: 0;
    background: currentColor;
    opacity: 0;
    z-index: -1;
    transition: opacity var(--m3-effects);
  }
  button:hover::before {
    opacity: var(--md-sys-state-hover);
  }
  button:active {
    flex-grow: 1.25;
    border-radius: var(--md-sys-shape-corner-extra-small);
  }
  button:first-child {
    border-top-left-radius: 20px;
    border-bottom-left-radius: 20px;
  }
  button:last-child {
    border-top-right-radius: 20px;
    border-bottom-right-radius: 20px;
  }
  button.selected {
    border-radius: 20px;
    background: var(--md-sys-color-secondary);
    color: var(--md-sys-color-on-secondary);
  }
</style>
