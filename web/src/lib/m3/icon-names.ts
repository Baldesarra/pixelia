// Benutzte Material Symbols (Rounded). Nur diese landen im Bundle (Vite-Plugin in vite.config.ts).
// „-fill“ = gefüllte Variante, z. B. für das aktive Ziel in der Navigation.
export const ICONS = [
  'dashboard',
  'dashboard-fill',
  'palette',
  'palette-fill',
  'settings',
  'settings-fill',
  'wifi',
  'wifi_off',
  'speed',
  'memory',
  'schedule',
  'system_update_alt',
  'warning',
  'check_circle',
  'error',
  'bug_report',
  'light_mode',
  'dark_mode',
  'brightness_auto',
  'open_in_new',
  'check',
  'router',
  'restart_alt',
] as const;

export type IconName = (typeof ICONS)[number];
