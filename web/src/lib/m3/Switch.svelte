<script lang="ts">
  // M3-Expressive-Schalter: Spur 52×32, Griff wächst beim Einschalten (16 → 24 px), Häkchen im Griff.
  // Baut auf <input type=checkbox role=switch> auf (Tastatur und Bildschirmleser von selbst).
  import Icon from './Icon.svelte';

  let {
    checked = $bindable(false),
    label,
    disabled = false,
    onchange,
  }: { checked?: boolean; label: string; disabled?: boolean; onchange?: (v: boolean) => void } = $props();
</script>

<label class="switch" class:on={checked} class:disabled>
  <input type="checkbox" role="switch" bind:checked {disabled} aria-label={label} onchange={() => onchange?.(checked)} />
  <span class="track" aria-hidden="true">
    <span class="handle">{#if checked}<Icon name="check" size={16} />{/if}</span>
  </span>
</label>

<style>
  .switch {
    position: relative;
    display: inline-flex;
    flex: none;
    cursor: pointer;
  }
  .disabled {
    opacity: 0.38;
    cursor: default;
  }
  input {
    position: absolute;
    inset: 0;
    margin: 0;
    opacity: 0;
    cursor: inherit;
  }
  .track {
    display: flex;
    align-items: center;
    width: 52px;
    height: 32px;
    box-sizing: border-box;
    border: 2px solid var(--md-sys-color-outline);
    border-radius: var(--md-sys-shape-corner-full);
    background: var(--md-sys-color-surface-container-highest);
    transition: background-color var(--m3-effects-fast), border-color var(--m3-effects-fast);
  }
  .handle {
    display: grid;
    place-items: center;
    width: 16px;
    height: 16px;
    margin-left: 6px;
    border-radius: 50%;
    background: var(--md-sys-color-outline);
    color: var(--md-sys-color-on-primary-container);
    transition:
      width var(--m3-spatial-fast),
      height var(--m3-spatial-fast),
      margin var(--m3-spatial-fast),
      background-color var(--m3-effects-fast);
  }
  .on .track {
    border-color: var(--md-sys-color-primary);
    background: var(--md-sys-color-primary);
  }
  .on .handle {
    width: 24px;
    height: 24px;
    margin-left: 22px;
    background: var(--md-sys-color-on-primary);
  }
  input:focus-visible + .track {
    outline: 2px solid var(--md-sys-color-secondary);
    outline-offset: 2px;
  }
</style>
