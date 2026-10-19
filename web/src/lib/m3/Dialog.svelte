<script lang="ts" module>
  // Dialoge für die ganze Oberfläche (App.svelte bindet <Dialog /> einmal ein):
  // - await confirmDialog({...}) liefert true/false
  // - await promptDialog({...}) liefert den eingegebenen Text oder null (Abbrechen). Mit check() bleibt der Dialog
  //   offen, solange check einen Fehlertext liefert (z. B. „PIN falsch, noch 3 Versuche“).
  import type { HTMLInputAttributes } from 'svelte/elements';
  import type { IconName } from './icon-names';

  type Ask = { title: string; text?: string; ok?: string; cancel?: string; icon?: IconName; danger?: boolean };
  type Field = {
    label: string;
    type?: 'text' | 'password';
    autocomplete?: HTMLInputAttributes['autocomplete'];
    inputmode?: 'text' | 'numeric';
    supporting?: string;
    check?: (value: string) => Promise<string | null>;
  };
  let current = $state<(Ask & { field?: Field; resolve: (v: string | null) => void }) | null>(null);

  export function confirmDialog(a: Ask): Promise<boolean> {
    current?.resolve(null);
    return new Promise((resolve) => (current = { ...a, resolve: (v) => resolve(v !== null) }));
  }

  export function promptDialog(a: Ask & { field: Field }): Promise<string | null> {
    current?.resolve(null);
    return new Promise((resolve) => (current = { ...a, resolve }));
  }
</script>

<script lang="ts">
  import Button from './Button.svelte';
  import Icon from './Icon.svelte';
  import TextField from './TextField.svelte';

  let dlg = $state<HTMLDialogElement>();
  let value = $state('');
  let error = $state('');
  let busy = $state(false);
  let input = $state<HTMLInputElement>();

  $effect(() => {
    if (!dlg) return;
    if (current && !dlg.open) {
      value = '';
      error = '';
      busy = false;
      dlg.showModal();
      if (current.field) queueMicrotask(() => input?.focus());
    }
    if (!current && dlg.open) dlg.close();
  });

  function cancel() {
    current?.resolve(null);
    current = null;
  }

  async function accept() {
    const c = current;
    if (!c || busy) return;
    if (c.field?.check) {
      busy = true;
      const err = await c.field.check(value).catch(() => 'Keine Verbindung zu Pixelia');
      busy = false;
      if (err) {
        error = err;
        input?.select();
        return;
      }
    }
    c.resolve(c.field ? value : '');
    current = null;
  }
</script>

<dialog bind:this={dlg} oncancel={cancel} onclick={(e) => e.target === dlg && cancel()}>
  {#if current}
    <form
      class="body"
      onsubmit={(e) => {
        e.preventDefault();
        void accept();
      }}
    >
      {#if current.icon}<span class="icon" class:danger={current.danger}><Icon name={current.icon} size={28} /></span>{/if}
      <h2>{current.title}</h2>
      {#if current.text}<p>{current.text}</p>{/if}
      {#if current.field}
        <TextField
          label={current.field.label}
          type={current.field.type ?? 'text'}
          autocomplete={current.field.autocomplete}
          inputmode={current.field.inputmode}
          supporting={current.field.supporting}
          {error}
          bind:value
          bind:input
        />
      {/if}
      <div class="actions">
        <Button variant="text" type="button" onclick={cancel}>{current.cancel ?? 'Abbrechen'}</Button>
        <Button
          variant={current.danger ? 'filled' : current.field ? 'filled' : 'tonal'}
          class={current.danger ? 'danger' : ''}
          type="submit"
          disabled={busy}
        >
          {current.ok ?? 'OK'}
        </Button>
      </div>
    </form>
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
