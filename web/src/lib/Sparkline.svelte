<script lang="ts">
  // Kleiner Verlauf als SVG-Linie mit Fläche (Leistungsanzeige). series: eine oder mehrere Reihen gleicher Länge,
  // neueste zuletzt; fehlende Werte (null) unterbrechen die Linie. limit: gestrichelte Grenzlinie (z. B. Budget).
  let {
    series,
    min = 0,
    max,
    limit,
    points = 60,
    label,
  }: {
    series: (number | null)[][];
    min?: number;
    max?: number;
    limit?: number;
    points?: number;
    label: string;
  } = $props();

  const W = 240;
  const H = 56;
  const top = $derived(
    max ?? Math.max(min + 1, limit ?? 0, ...series.flat().filter((v): v is number => v != null)) * 1.1,
  );
  const y = (v: number) => H - 2 - ((Math.min(Math.max(v, min), top) - min) / (top - min || 1)) * (H - 4);
  const x = (i: number, n: number) => W - (n - 1 - i) * (W / (points - 1));

  function path(vals: (number | null)[]): string {
    let d = '';
    let pen = false;
    vals.forEach((v, i) => {
      if (v == null) return (pen = false);
      d += `${pen ? 'L' : 'M'}${x(i, vals.length).toFixed(1)},${y(v).toFixed(1)}`;
      pen = true;
    });
    return d;
  }
  // Fläche nur unter der ersten Reihe, solange sie lückenlos ist
  const area = $derived.by(() => {
    const v = series[0] ?? [];
    if (v.length < 2 || v.some((p) => p == null)) return '';
    return `${path(v)}L${x(v.length - 1, v.length)},${H}L${x(0, v.length)},${H}Z`;
  });
</script>

<svg class="spark" viewBox="0 0 {W} {H}" preserveAspectRatio="none" role="img" aria-label={label}>
  {#if area}<path class="area" d={area} />{/if}
  {#if limit != null}<line class="limit" x1="0" x2={W} y1={y(limit)} y2={y(limit)} />{/if}
  {#each series as s, k (k)}
    <path class="line s{k}" d={path(s)} />
  {/each}
</svg>

<style>
  .spark {
    display: block;
    width: 100%;
    height: 56px;
    overflow: visible;
  }
  .line {
    fill: none;
    stroke-width: 2;
    stroke-linejoin: round;
    stroke-linecap: round;
    vector-effect: non-scaling-stroke;
  }
  .s0 {
    stroke: var(--md-sys-color-primary);
  }
  .s1 {
    stroke: var(--md-sys-color-tertiary);
  }
  .area {
    fill: color-mix(in srgb, var(--md-sys-color-primary) 14%, transparent);
  }
  .limit {
    stroke: var(--md-sys-color-error);
    stroke-width: 1;
    stroke-dasharray: 4 4;
    vector-effect: non-scaling-stroke;
  }
</style>
