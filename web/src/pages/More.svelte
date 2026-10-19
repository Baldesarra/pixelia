<script lang="ts">
  // Seite „Mehr“ (nur Handy): Ziele, die nicht in die Navigationsleiste passen
  import { Card, Icon, type IconName } from '../lib/m3';

  const links: { href: string; icon: IconName; title: string; text: string }[] = [
    { href: '#/info', icon: 'info', title: 'Info', text: 'Laufzeit, Netz, Uhrzeit, Speicherbelegung' },
    { href: '#/hardware', icon: 'developer_board', title: 'Hardware', text: 'Board, Panel, Touch, Chip, Pins, Partitionen' },
    { href: '#/geraete', icon: 'devices', title: 'Verbundene Geräte', text: 'Welche Browser auf Pixelia zugreifen' },
    { href: '#/darstellung', icon: 'palette', title: 'Darstellung', text: 'Farben und Hell/Dunkel der Oberfläche' },
    { href: '/setup', icon: 'build', title: 'Einfache Einrichtungsseite', text: 'Notfallseite ohne Extras (WLAN, Update, Diagnose)' },
  ];
</script>

<Card>
  <ul class="list">
    {#each links as l (l.href)}
      <li>
        <a class="list-item link" href={l.href}>
          <span class="lead"><Icon name={l.icon} /></span>
          <span class="text">
            <span class="headline">{l.title}</span>
            <span class="supporting">{l.text}</span>
          </span>
          <Icon name="chevron_right" />
        </a>
      </li>
    {/each}
  </ul>
</Card>

<style>
  .link {
    text-decoration: none;
    transition: background-color var(--m3-effects);
  }
  .link:hover {
    background: color-mix(in srgb, var(--md-sys-color-on-surface) 8%, transparent);
  }
</style>
