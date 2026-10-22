<script lang="ts">
  // Seite „Einstellungen“ (seit 0.5.4, früher „Mehr“): Verbindung, Gerät, Oberfläche als Liste mit Kategorien; jede
  // Zeile zeigt den aktuellen Zustand und öffnet eine eigene Seite.
  import HubList, { type HubGroup } from '../lib/HubList.svelte';
  import { device } from '../lib/status.svelte';
  import { prefs } from '../lib/prefs.svelte';
  import { theme } from '../lib/m3';
  import { num } from '../lib/format';

  const s = $derived(device.data);
  const wifiSub = $derived(
    !s ? '' : s.wifi.ap ? 'Setup-Modus: WLAN einrichten' : s.wifi.state === 'verbunden' ? `Verbunden mit ${s.wifi.ssid}` : 'Nicht verbunden',
  );
  const mode = { system: 'wie das System', light: 'hell', dark: 'dunkel' } as const;
  const groups = $derived<HubGroup[]>([
    {
      title: 'Verbindung',
      items: [
        { href: '#/wlan', icon: 'wifi', title: 'WLAN', sub: wifiSub, tone: s?.wifi.ap ? 'error' : undefined },
        { href: '#/geraete', icon: 'devices', title: 'Verbundene Geräte', sub: 'Welche Browser auf Pixelia zugreifen' },
      ],
    },
    {
      title: 'Gerät',
      items: [
        { href: '#/display', icon: 'light_mode', title: 'Display', sub: s ? `Helligkeit ${s.set.bright} % · ${s.set.rot ? 'gedreht' : 'normal'} · langer Druck ${num(s.set.lp / 1000, 1)} s` : '' },
        { href: '#/schutz', icon: 'shield_lock', title: 'Zugangsschutz', sub: s?.auth?.pin ? 'PIN an' : 'Aus, jeder im WLAN hat Zugriff' },
      ],
    },
    {
      title: 'Oberfläche',
      items: [
        {
          href: '#/darstellung',
          icon: 'palette',
          title: 'Darstellung',
          sub: `${prefs.wide ? 'Breit' : 'Schmal'} · ${mode[theme.mode as keyof typeof mode] ?? 'hell'} · Farben, Schrift`,
        },
      ],
    },
  ]);
</script>

<HubList {groups} />
