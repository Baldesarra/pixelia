<script lang="ts">
  // WLAN-Signal als vier Balken (Bewertung aus format.ts: ≥ −55 sehr gut, ≥ −67 gut, ≥ −75 mittel, sonst schwach)
  import { signal } from './format';

  let { rssi, size = 20 }: { rssi: number; size?: number } = $props();
  const s = $derived(signal(rssi));
</script>

<span class="bars" style:--h="{size}px" role="img" aria-label="Signal {s.text} ({rssi} dBm)" title="{rssi} dBm · {s.text}">
  {#each [1, 2, 3, 4] as b (b)}<span class:on={b <= s.bars} style:height="{b * 25}%"></span>{/each}
</span>

<style>
  .bars {
    display: inline-flex;
    align-items: flex-end;
    gap: calc(var(--h) / 10);
    height: var(--h);
    flex: none;
  }
  span span {
    width: calc(var(--h) / 4.4);
    border-radius: calc(var(--h) / 12);
    background: var(--md-sys-color-surface-container-highest);
    transition: background-color var(--m3-effects);
  }
  span span.on {
    background: var(--md-sys-color-primary);
  }
</style>
