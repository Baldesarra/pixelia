// M3-Expressive-Formen als Polarfunktion r(θ) = 1 + a·cos(k·θ) (Keks, Sonne, Blüte …).
// Alle Formen haben gleich viele Punkte, dadurch lassen sie sich als SVG-Pfad ineinander überblenden (Morphing).
export type ShapeDef = { k: number; a: number; rot?: number };

export const SHAPES = {
  circle: { k: 1, a: 0 },
  oval: { k: 2, a: 0.12 },
  pill: { k: 2, a: 0.2, rot: 45 },
  pentagon: { k: 5, a: 0.09, rot: -90 },
  cookie6: { k: 6, a: 0.07 },
  cookie9: { k: 9, a: 0.06 },
  cookie12: { k: 12, a: 0.045 },
  sunny: { k: 8, a: 0.1 },
  flower: { k: 8, a: 0.16 },
  burst: { k: 12, a: 0.11 },
} satisfies Record<string, ShapeDef>;

export type ShapeName = keyof typeof SHAPES;

const N = 96;

/** SVG-Pfad der Form, Mittelpunkt (c, c), Außenradius r */
export function shapePath(name: ShapeName, c = 50, r = 50): string {
  const s: ShapeDef = SHAPES[name];
  const rot = ((s.rot ?? 0) * Math.PI) / 180;
  const base = r / (1 + s.a);
  let d = '';
  for (let i = 0; i < N; i++) {
    const t = (i / N) * 2 * Math.PI;
    const rr = base * (1 + s.a * Math.cos(s.k * (t - rot)));
    d += `${i ? 'L' : 'M'}${(c + rr * Math.cos(t)).toFixed(2)} ${(c + rr * Math.sin(t)).toFixed(2)}`;
  }
  return d + 'Z';
}
