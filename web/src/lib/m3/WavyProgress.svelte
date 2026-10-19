<script lang="ts">
  // M3-Expressive-Fortschrittsanzeige, linear: Welle im aktiven Teil, Lücke, flache Spur, Stopp-Punkt am Ende.
  // value 0…1, ohne value unbestimmt (laufender Abschnitt). wavy=false ergibt die flache Variante.
  let {
    value,
    wavy = true,
    label,
    tone = 'primary',
  }: { value?: number; wavy?: boolean; label: string; tone?: 'primary' | 'tertiary' | 'error' } = $props();

  const H = 14; // Höhe der Anzeige
  const A = 3; // Amplitude der Welle
  const L = 40; // Wellenlänge
  const GAP = 4;
  let w = $state(0);
  const uid = $props.id();

  const p = $derived(value === undefined ? 0 : Math.min(1, Math.max(0, value)));
  const active = $derived(value === undefined ? 0 : p * w);
  // Welle eine Wellenlänge breiter als nötig, damit das Verschieben (Animation) nahtlos ist
  const wave = $derived.by(() => {
    const y = H / 2;
    if (!wavy) return `M2 ${y} H${w + L}`;
    let d = `M${-L} ${y}`;
    for (let x = -L; x < w + L; x += L / 2) {
      d += ` q${L / 4} ${(x / (L / 2)) % 2 === 0 ? -2 * A : 2 * A} ${L / 2} 0`;
    }
    return d;
  });
</script>

<div
  class="wp {tone}"
  class:indeterminate={value === undefined}
  bind:clientWidth={w}
  role="progressbar"
  aria-label={label}
  aria-valuemin={0}
  aria-valuemax={100}
  aria-valuenow={value === undefined ? undefined : Math.round(p * 100)}
>
  {#if w > 0}
    <svg width={w} height={H} viewBox="0 0 {w} {H}" style:--w="{w}px">
      <defs>
        <clipPath id="wp-{uid}">
          {#if value === undefined}
            <rect class="run" x="0" y="0" width={w * 0.4} height={H} rx="2" />
          {:else}
            <rect class="clip" x="0" y="0" width={Math.max(0, active)} height={H} />
          {/if}
        </clipPath>
      </defs>
      {#if value !== undefined && active + GAP < w - 2}
        <line class="track" x1={Math.min(w - 2, active + GAP + 2)} y1={H / 2} x2={w - 2} y2={H / 2} />
        <circle class="stop" cx={w - 2} cy={H / 2} r="2" />
      {:else if value === undefined}
        <line class="track" x1="2" y1={H / 2} x2={w - 2} y2={H / 2} />
      {/if}
      <g clip-path="url(#wp-{uid})">
        <path class="wave" class:moving={wavy} d={wave} />
      </g>
    </svg>
  {/if}
</div>

<style>
  .wp {
    width: 100%;
    height: 14px;
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
    display: block;
    overflow: hidden;
  }
  .wave {
    fill: none;
    stroke: var(--c);
    stroke-width: 4px;
    stroke-linecap: round;
  }
  .moving {
    animation: shift 1.4s linear infinite;
  }
  .track {
    stroke: var(--t);
    stroke-width: 4px;
    stroke-linecap: round;
  }
  .stop {
    fill: var(--c);
  }
  .clip {
    transition: width var(--m3-spatial-slow);
  }
  .run {
    animation: run 1.8s cubic-bezier(0.4, 0, 0.2, 1) infinite;
  }
  @keyframes shift {
    to {
      transform: translateX(40px);
    }
  }
  @keyframes run {
    from {
      transform: translateX(calc(var(--w) * -0.45));
    }
    to {
      transform: translateX(calc(var(--w) * 1.05));
    }
  }
  @media (prefers-reduced-motion: reduce) {
    .moving {
      animation: none;
    }
  }
</style>
