<script lang="ts" module>
  // Bestätigungsdialog für die ganze Oberfläche: await confirmDialog({...}) liefert true/false.
  // App.svelte bindet <Dialog /> einmal ein.
  import type { IconName } from './icon-names';

  type Ask = { title: string; text?: string; ok?: string; cancel?: string; icon?: IconName; danger?: boolean };
  let current = $state<(Ask & { resolve: (v: boolean) => void }) | null>(null);

  export function confirmDialog(a: Ask): Promise<boolean> {
    current?.resolve(false);
    return new Promise((resolve) => (current = { ...a, resolve }));
  }
</script>

<script lang="ts">
  import Button from './Button.svelte';
  import Icon from './Icon.svelte';

  let dlg = $state<HTMLDialogElement>();

  $effect(() => {
    if (!dlg) return;
    if (current && !dlg.open) dlg.showModal();
    if (!current && dlg.open) dlg.close();
  });

  function done(v: boolean) {
    current?.resolve(v);
    current = null;
  }
</script>

<dialog bind:this={dlg} oncancel={() => done(false)} onclick={(e) => e.target === dlg && done(false)}>
  {#if current}
    <div class="body">
      {#if current.icon}<span class="icon" class:danger={current.danger}><Icon name={current.icon} size={28} /></span>{/if}
      <h2>{current.title}</h2>
      {#if current.text}<p>{current.text}</p>{/if}
      <div class="actions">
        <Button variant="text" onclick={() => done(false)}>{current.cancel ?? 'Abbrechen'}</Button>
        <Button variant={current.danger ? 'filled' : 'tonal'} class={current.danger ? 'danger' : ''} onclick={() => done(true)}>
          {current.ok ?? 'OK'}
        </Button>
      </div>
    </div>
  {/if}
</dialog>

<style>
  dialog {
    width: min(100% - 48px, 400px);
    padding: 0;
    border: 0;
    border-radius: var(--md-sys-shape-corner-extra-large);
    background: var(--md-sys-color-surface-container-high);
    color: var(--md-sys-color-on-surface);
  }
  dialog[open] {
    animation: pop var(--m3-spatial-fast) both;
  }
  dialog::backdrop {
    background: rgb(0 0 0 / 0.32);
  }
  @keyframes pop {
    from {
      opacity: 0;
      transform: scale(0.85);
    }
  }
  .body {
    display: grid;
    gap: 16px;
    padding: 24px;
  }
  .icon {
    justify-self: center;
    color: var(--md-sys-color-secondary);
  }
  .icon.danger {
    color: var(--md-sys-color-error);
  }
  h2 {
    margin: 0;
    font: var(--md-sys-typescale-headline-small);
    text-align: center;
  }
  p {
    margin: 0;
    color: var(--md-sys-color-on-surface-variant);
    font: var(--md-sys-typescale-body-medium);
  }
  .actions {
    display: flex;
    justify-content: flex-end;
    gap: 8px;
    margin-top: 8px;
  }
</style>
