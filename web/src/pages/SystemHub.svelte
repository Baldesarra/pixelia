<script lang="ts">
  // Seite „System“ (seit 0.5.4 eine Liste mit Kategorien): Wartung, Technik, Labor. Jede Zeile zeigt den aktuellen
  // Zustand und öffnet eine eigene Seite (Zurück-Pfeil oben führt hierher).
  import HubList, { type HubGroup } from '../lib/HubList.svelte';
  import { device } from '../lib/status.svelte';
  import { duration, kb, num } from '../lib/format';

  const s = $derived(device.data);
  const groups = $derived<HubGroup[]>([
    {
      title: 'Wartung',
      items: [
        s?.ota.avail
          ? { href: '#/update', icon: 'system_update_alt', title: 'Update', sub: `Neue Version ${s.ota.avail} verfügbar`, tone: 'primary', badge: true }
          : { href: '#/update', icon: 'system_update_alt', title: 'Update', sub: s ? `Installiert: Version ${s.v}` : 'Von GitHub oder aus Datei' },
        { href: '#/versionen', icon: 'history', title: 'Versionsverlauf', sub: 'Was in jeder Version neu ist, kurz gefasst' },
        { href: '#/sicherung', icon: 'settings_backup_restore', title: 'Sicherung', sub: 'Einstellungen sichern und wieder einspielen' },
        { href: '#/neustart', icon: 'restart_alt', title: 'Neustart und Werksreset', sub: s ? `Läuft seit ${duration(s.uptime)}` : '' },
      ],
    },
    {
      title: 'Technik',
      items: [
        { href: '#/live', icon: 'monitoring', title: 'Leistung und Live-Bild', sub: s ? `${num(s.perf.fps, 1)} fps · ${kb(s.perf.ram)} RAM frei` : '' },
        { href: '#/log', icon: 'terminal', title: 'Live-Log', sub: s ? `${s.diag.errors} Fehler · ${s.diag.warnings} Warnungen · Befehle` : 'Log und Befehle' },
        s?.diag.crash
          ? { href: '#/diagnose', icon: 'monitor_heart', title: 'Diagnose', sub: `Absturzbericht vorhanden (Task „${s.diag.crash.task}“)`, tone: 'error' }
          : { href: '#/diagnose', icon: 'monitor_heart', title: 'Diagnose', sub: s ? `Start Nr. ${s.diag.boot} · ${s.diag.reset}` : 'Abstürze, Neustarts, Diagnosepaket' },
        { href: '#/geraet', icon: 'developer_board', title: 'Gerät und Speicher', sub: 'Laufzeit, Speicher, Panel, Chip, Pins, Partitionen' },
      ],
    },
    {
      title: 'Labor',
      items: [
        { href: '#/tests', icon: 'touch_app', title: 'Gerätetests', sub: 'Touch-Test, Testbilder, eigenes Bild aufs Display' },
        { href: '#/experimentell', icon: 'science', title: 'Experimentell', sub: s?.set.exp ? 'An · Testorte, Panel, ältere Version' : 'Aus · Testorte, Panel, ältere Version' },
        { href: '/setup', icon: 'emergency_home', title: 'Notfallseite', sub: 'Einfache Einrichtung ohne Extras (WLAN, Update)', ext: true },
      ],
    },
  ]);
</script>

<HubList {groups} />
