<script lang="ts">
  // Stufe einer Fähigkeit wie auf der Karte am Display: drei Segmente, das nächste füllt sich mit der Erfahrung
  let { level, progress = 0, title = '' }: { level: number; progress?: number; title?: string } = $props();
</script>

<span class="meter" role="img" aria-label={title} {title}>
  {#each [0, 1, 2] as k (k)}
    <span class="seg"><span style:width={k < level ? '100%' : k === level ? `${progress}%` : '0'} class:part={k >= level}></span></span>
  {/each}
</span>

<style>
  .meter {
    display: inline-flex;
    gap: 2px;
    flex: none;
  }
  .seg {
    display: block;
    width: 12px;
    height: 6px;
    border-radius: 1px;
    background: var(--md-sys-color-surface-container-highest);
    overflow: hidden;
  }
  .seg span {
    display: block;
    height: 100%;
    background: var(--md-sys-color-primary);
  }
  .seg span.part {
    background: color-mix(in srgb, var(--md-sys-color-primary) 45%, transparent);
  }
</style>
