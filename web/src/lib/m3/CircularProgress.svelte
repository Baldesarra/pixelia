<script lang="ts">
  // M3-Expressive-Fortschritt, kreisförmig und wellig (bestimmt). Inhalt (Snippet) steht in der Mitte.
  import type { Snippet } from 'svelte';

  let {
    value,
    size = 120,
    label,
    tone = 'primary',
    children,
  }: { value: number; size?: number; label: string; tone?: 'primary' | 'tertiary' | 'error'; children?: Snippet } =
    $props();

  const STROKE = 8;
  const WAVES = 12;
  const A = 2.2;
  const GAP_DEG = 14;

  const p = $derived(Math.min(1, Math.max(0, value)));
  const R = $derived(size / 2 - STROKE / 2 - A);

  function arc(from: number, to: number, wavy: boolean): string {
    if (to <= from) return '';
    const c = size / 2;
    const steps = Math.max(2, Math.ceil((to - from) / 3));
    let d = '';
    for (let i = 0; i <= steps; i++) {
      const deg = from + ((to - from) * i) / steps;
      const t = ((deg - 90) * Math.PI) / 180;
      const r = R + (wavy ? A * Math.sin((deg * Math.PI * WAVES) / 180) : 0);
      d += `${i ? 'L' : 'M'}${(c + r * Math.cos(t)).toFixed(2)} ${(c + r * Math.sin(t)).toFixed(2)}`;
    }
    return d;
  }

  const active = $derived(arc(0, 360 * p, true));
  const track = $derived(p >= 0.999 ? '' : arc(p > 0 ? 360 * p + GAP_DEG : 0, 360 - (p > 0 ? GAP_DEG : 0), false));
</script>

<div
  class="cp {tone}"
  style:width="{size}px"
  style:height="{size}px"
  role="progressbar"
  aria-label={label}
  aria-valuemin={0}
  aria-valuemax={100}
  aria-valuenow={Math.round(p * 100)}
>
  <svg viewBox="0 0 {size} {size}" width={size} height={size}>
    {#if track}<path class="track" d={track} />{/if}
    {#if active}<path class="active" d={active} />{/if}
  </svg>
  {#if children}<div class="inner">{@render children()}</div>{/if}
</div>

<style>
  .cp {
    position: relative;
    display: inline-grid;
    place-items: center;
    flex: none;
    --c: var(--md-sys-color-primary);
    --t: var(--md-sys-color-secondary-container);
  }
  .tertiary {
    --c: var(--md-sys-color-tertiary);
    --t: var(--md-sys-color-tertiary-container);
  }
  .error {
    --c: var(--md-sys-color-error);
    --t: var(--md-sys-color-error-container);
  }
  svg {
    position: absolute;
    inset: 0;
  }
  path {
    fill: none;
    stroke-width: 8px;
    stroke-linecap: round;
    stroke-linejoin: round;
  }
  .active {
    stroke: var(--c);
  }
  .track {
    stroke: var(--t);
  }
  .inner {
    position: relative;
    text-align: center;
  }
</style>
