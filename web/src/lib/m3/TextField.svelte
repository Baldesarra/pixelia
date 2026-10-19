<script lang="ts">
  // M3-Textfeld (umrandet): schwebendes Label, Hilfs- bzw. Fehlertext, optional ein Knopf am Ende
  import type { Snippet } from 'svelte';
  import type { HTMLInputAttributes } from 'svelte/elements';
  import Icon from './Icon.svelte';
  import type { IconName } from './icon-names';

  let {
    label,
    value = $bindable(''),
    icon,
    supporting,
    error,
    trailing,
    input = $bindable(),
    class: cls = '',
    ...rest
  }: {
    label: string;
    value?: string;
    icon?: IconName;
    supporting?: string;
    error?: string;
    trailing?: Snippet;
    input?: HTMLInputElement;
  } & Omit<HTMLInputAttributes, 'value'> = $props();

  const uid = $props.id();
</script>

<div class="field {cls}" class:error={!!error} class:lead={!!icon}>
  <div class="box">
    {#if icon}<span class="icon"><Icon name={icon} /></span>{/if}
    <input id={uid} bind:value bind:this={input} placeholder=" " aria-invalid={!!error} {...rest} />
    <label for={uid}>{label}</label>
    {#if trailing}<span class="trail">{@render trailing()}</span>{/if}
  </div>
  {#if error || supporting}<div class="support">{error || supporting}</div>{/if}
</div>

<style>
  .field {
    display: grid;
    gap: 4px;
    min-width: 0;
  }
  .box {
    position: relative;
    display: flex;
    align-items: center;
    min-height: 56px;
    border-radius: var(--md-sys-shape-corner-large);
    box-shadow: inset 0 0 0 1px var(--md-sys-color-outline);
    transition:
      box-shadow var(--m3-effects),
      border-radius var(--m3-spatial-fast);
  }
  .box:hover {
    box-shadow: inset 0 0 0 1px var(--md-sys-color-on-surface);
  }
  .box:focus-within {
    box-shadow: inset 0 0 0 2px var(--md-sys-color-primary);
    border-radius: var(--md-sys-shape-corner-medium);
  }
  input {
    flex: 1;
    min-width: 0;
    height: 56px;
    padding: 18px 16px 0;
    border: 0;
    background: none;
    color: var(--md-sys-color-on-surface);
    font: var(--md-sys-typescale-body-large);
    outline: none;
  }
  .lead input {
    padding-left: 0;
  }
  .icon {
    display: grid;
    padding: 0 12px 0 14px;
    color: var(--md-sys-color-on-surface-variant);
  }
  label {
    position: absolute;
    left: 16px;
    top: 50%;
    max-width: calc(100% - 64px);
    overflow: hidden;
    white-space: nowrap;
    text-overflow: ellipsis;
    transform: translateY(-50%);
    color: var(--md-sys-color-on-surface-variant);
    font: var(--md-sys-typescale-body-large);
    pointer-events: none;
    transition:
      transform var(--m3-spatial-fast),
      font var(--m3-effects),
      top var(--m3-spatial-fast),
      color var(--m3-effects);
  }
  .lead label {
    left: 50px;
  }
  input:focus + label,
  input:not(:placeholder-shown) + label {
    top: 8px;
    transform: none;
    font: var(--md-sys-typescale-body-small);
  }
  input:focus + label {
    color: var(--md-sys-color-primary);
  }
  .trail {
    display: flex;
    padding-right: 4px;
  }
  .support {
    padding: 0 16px;
    font: var(--md-sys-typescale-body-small);
    color: var(--md-sys-color-on-surface-variant);
  }
  .error .box {
    box-shadow: inset 0 0 0 2px var(--md-sys-color-error);
  }
  .error .support,
  .error label {
    color: var(--md-sys-color-error);
  }
</style>
