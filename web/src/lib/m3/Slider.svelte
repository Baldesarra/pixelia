<script lang="ts">
  // M3-Expressive-Schieberegler: dicke Spur, schmaler Griff mit Lücke, Wert-Blase beim Ziehen.
  // Baut auf <input type=range> auf (Tastatur, Bildschirmleser und Touch funktionieren von selbst).
  let {
    value = $bindable(0),
    min = 0,
    max = 100,
    step = 1,
    label,
    unit = '',
    oninput,
    onchange,
  }: {
    value?: number;
    min?: number;
    max?: number;
    step?: number;
    label: string;
    unit?: string;
    oninput?: (v: number) => void;
    onchange?: (v: number) => void;
  } = $props();

  const p = $derived((value - min) / (max - min || 1));
</script>

<div class="slider" style:--p={p}>
  <div class="track" aria-hidden="true">
    <span class="active"></span>
    <span class="handle"><span class="bubble tnum">{value}{unit}</span></span>
    <span class="rest"></span>
  </div>
  <input
    type="range"
    {min}
    {max}
    {step}
    bind:value
    aria-label={label}
    aria-valuetext="{value}{unit}"
    oninput={() => oninput?.(value)}
    onchange={() => onchange?.(value)}
  />
</div>

<style>
  .slider {
    position: relative;
    height: 44px;
    --handle: 4px;
    --gap: 6px;
  }
  .track {
    position: absolute;
    inset: 0;
    display: flex;
    align-items: center;
    pointer-events: none;
  }
  .active,
  .rest {
    height: 16px;
  }
  .active {
    width: max(0px, calc(var(--p) * (100% - var(--handle) - 2 * var(--gap))));
    border-radius: 8px 2px 2px 8px;
    background: var(--md-sys-color-primary);
  }
  .rest {
    flex: 1;
    border-radius: 2px 8px 8px 2px;
    background: var(--md-sys-color-secondary-container);
  }
  .handle {
    position: relative;
    width: var(--handle);
    height: 44px;
    margin: 0 var(--gap);
    border-radius: 2px;
    background: var(--md-sys-color-primary);
    transition: width var(--m3-spatial-fast);
  }
  .bubble {
    position: absolute;
    left: 50%;
    bottom: calc(100% + 6px);
    padding: 6px 12px;
    border-radius: var(--md-sys-shape-corner-full);
    background: var(--md-sys-color-inverse-surface);
    color: var(--md-sys-color-inverse-on-surface);
    font: var(--md-sys-typescale-label-large);
    white-space: nowrap;
    transform: translateX(-50%) scale(0.6);
    opacity: 0;
    transition:
      transform var(--m3-spatial-fast),
      opacity var(--m3-effects-fast);
  }
  input {
    position: absolute;
    inset: 0;
    width: 100%;
    height: 100%;
    margin: 0;
    opacity: 0;
    cursor: pointer;
  }
  .slider:has(input:active) .handle,
  .slider:has(input:focus-visible) .handle {
    width: 2px;
  }
  .slider:has(input:active) .bubble,
  .slider:has(input:focus-visible) .bubble {
    opacity: 1;
    transform: translateX(-50%) scale(1);
  }
  .slider:has(input:focus-visible) {
    outline: 3px solid var(--md-sys-color-secondary);
    outline-offset: 4px;
    border-radius: 8px;
  }
</style>
