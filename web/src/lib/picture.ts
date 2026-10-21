// Eigenes Testbild (seit 0.5.0, Seite Experimentell): Der Browser dekodiert die Bilddatei, bringt sie pixelgenau auf
// 400 × 240 und bildet sie auf höchstens 240 Farben ab (Indizes 240–255 gehören der System-Oberfläche des Geräts).
// Ergebnis wie POST /api/picture erwartet (web_exp.cpp): 96 000 Indizes, dann die Palette 240 × RGB888.

export const PIC_W = 400;
export const PIC_H = 240;
const MAX_COLORS = 240;

export type Picture = {
  body: Blob;
  rgba: ImageData; // so, wie das Display es zeigt (nach Anpassung und Farbabbildung)
  srcW: number;
  srcH: number;
  colors: number; // Farben im Ergebnis
  srcColors: number; // Farben im angepassten Bild vor einer Reduktion
};

export async function preparePicture(file: Blob): Promise<Picture> {
  const bmp = await createImageBitmap(file);
  const canvas = document.createElement('canvas');
  canvas.width = PIC_W;
  canvas.height = PIC_H;
  const ctx = canvas.getContext('2d', { willReadFrequently: true })!;
  ctx.fillStyle = '#000';
  ctx.fillRect(0, 0, PIC_W, PIC_H);
  // Pixelgenau: vergrößern nur ganzzahlig, verkleinern ohne Glätten (800 × 480 → 400 × 240 nimmt jedes zweite Pixel)
  let s = Math.min(PIC_W / bmp.width, PIC_H / bmp.height);
  if (s >= 1) s = Math.floor(s);
  const w = Math.round(bmp.width * s);
  const h = Math.round(bmp.height * s);
  ctx.imageSmoothingEnabled = false;
  ctx.drawImage(bmp, Math.floor((PIC_W - w) / 2), Math.floor((PIC_H - h) / 2), w, h);
  const [srcW, srcH] = [bmp.width, bmp.height];
  bmp.close();

  const img = ctx.getImageData(0, 0, PIC_W, PIC_H);
  const d = img.data;
  const n = PIC_W * PIC_H;
  // Farbtiefe so lange verringern, bis höchstens 240 Farben übrig sind; Farbe eines Eimers = Mittel seiner Pixel
  let srcColors = 0;
  for (let shift = 0; shift <= 6; shift++) {
    const buckets = new Map<number, number>();
    for (let i = 0; i < n; i++) {
      const k = ((d[i * 4] >> shift) << 16) | ((d[i * 4 + 1] >> shift) << 8) | (d[i * 4 + 2] >> shift);
      if (!buckets.has(k)) {
        buckets.set(k, buckets.size);
        if (buckets.size > MAX_COLORS) break;
      }
    }
    if (shift === 0) srcColors = buckets.size > MAX_COLORS ? countColors(d, n) : buckets.size;
    if (buckets.size > MAX_COLORS) continue;

    const out = new Uint8Array(n + MAX_COLORS * 3);
    const sum = new Float64Array(buckets.size * 4);
    for (let i = 0; i < n; i++) {
      const k = ((d[i * 4] >> shift) << 16) | ((d[i * 4 + 1] >> shift) << 8) | (d[i * 4 + 2] >> shift);
      const idx = buckets.get(k)!;
      out[i] = idx;
      sum[idx * 4] += d[i * 4];
      sum[idx * 4 + 1] += d[i * 4 + 1];
      sum[idx * 4 + 2] += d[i * 4 + 2];
      sum[idx * 4 + 3]++;
    }
    const pal = out.subarray(n);
    for (let c = 0; c < buckets.size; c++) {
      for (let ch = 0; ch < 3; ch++) pal[c * 3 + ch] = Math.round(sum[c * 4 + ch] / sum[c * 4 + 3]);
    }
    for (let i = 0; i < n; i++) {
      const c = out[i];
      d[i * 4] = pal[c * 3];
      d[i * 4 + 1] = pal[c * 3 + 1];
      d[i * 4 + 2] = pal[c * 3 + 2];
      d[i * 4 + 3] = 255;
    }
    return { body: new Blob([out], { type: 'application/octet-stream' }), rgba: img, srcW, srcH, colors: buckets.size, srcColors };
  }
  throw new Error('Farben ließen sich nicht reduzieren');
}

function countColors(d: Uint8ClampedArray, n: number): number {
  const seen = new Set<number>();
  for (let i = 0; i < n; i++) seen.add((d[i * 4] << 16) | (d[i * 4 + 1] << 8) | d[i * 4 + 2]);
  return seen.size;
}
