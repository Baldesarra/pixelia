#include "pixelia/engine/room.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <new>

#include "pixelia/engine/light.h"
#include "pixelia/engine/memory.h"

namespace pixelia {

namespace {

constexpr int8_t kStepX[4] = {0, 0, -1, 1};  // Reihenfolge wie Facing: unten, oben, links, rechts
constexpr int8_t kStepY[4] = {1, -1, 0, 0};
constexpr uint8_t kUnseen = 0xFF;

Facing mirrored(Facing f) {
    return f == Facing::Left ? Facing::Right : f == Facing::Right ? Facing::Left : f;
}

Rect unite(const Rect& a, const Rect& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    const int x0 = std::min(a.x, b.x), y0 = std::min(a.y, b.y);
    const int x1 = std::max(a.x + a.w, b.x + b.w), y1 = std::max(a.y + a.h, b.y + b.h);
    return {x0, y0, x1 - x0, y1 - y0};
}

}  // namespace

struct Room::Storage {
    TileMap map;
    std::array<RoomPart, kMaxParts> parts{};
    std::array<Exit, kMaxExits> exits{};
    int part_count = 0;
    int exit_count = 0;
    uint8_t layer_row[kTileLayers]{};
    uint8_t walk[kTilesY][kTilesX]{};  // 1 = frei
    uint8_t next[kTileCells]{};        // Breitensuche: Richtung zum Ziel
    uint16_t queue[kTileCells]{};
    DepthList list;
    // Licht (seit 0.5.2): Kreise, Stufen der Palette, Raster
    std::array<LightCircle, Room::kMaxLights> lights{};
    int light_count = 0;
    int light_base = 0, light_levels = 1;
    LightStyle light_style = LightStyle::Fine;
    std::array<uint8_t, kCoarseHeight> lx0{}, lx1{};  // je Zeile der Bereich mit Licht
    // Neubau in Zeilen (seit 0.5.9 Teil 3): Tabellen, nächste Zeile (−1 = fertig), Ziel, Bereiche des zweiten Puffers
    LightPlan plan;
    int build_row = -1;
    bool build_back = false;   // in den zweiten Puffer (betreten)
    bool build_again = false;  // während des Durchgangs geändert: danach noch einer
    std::array<uint8_t, kCoarseHeight> bx0{}, bx1{};
    uint32_t builds = 0;  // fertige Durchgänge (Prüfungen)
    // Füllungen der Lagerplätze (seit 0.5.10 Teil 2b): Teil, Stück, Zahl; seit Teil 3b ein Bereich der Stellplätze
    // [first, first + count), damit ein Teil mehrere Waren zeigt (Materialstapel einer Baustelle)
    struct Fill {
        int part = -1;
        const PartStage* piece = nullptr;
        int n = 0;
        int first = 0;
        int count = 0;
    };
    std::array<Fill, Room::kMaxFills> fills{};
    uint32_t bakes = 0;
};

Room::~Room() {
    leave();
    if (lm_) asset_free(lm_);
    if (lmb_) asset_free(lmb_);
    if (st_) {
        st_->~Storage();
        asset_free(st_);
    }
}

bool Room::init(const TileSet* tiles, const PartCatalog* catalog) {
    tiles_ = tiles;
    catalog_ = catalog;
    if (!st_) {
        void* mem = asset_alloc(sizeof(Storage));
        if (!mem) return false;
        st_ = new (mem) Storage;
    }
    return true;
}

void Room::load(const RoomDef& def) {
    if (!st_) return;
    for (int l = 0; l < kTileLayers; ++l) {
        st_->map.load(l, def.layers[l], kTilesY, def.legend);
        st_->layer_row[l] = def.layer_rows[l];
    }
    st_->part_count = std::min<int>(def.part_count, kMaxParts);
    for (int i = 0; i < st_->part_count; ++i) st_->parts[i] = def.parts[i];
    st_->exit_count = std::min<int>(def.exit_count, kMaxExits);
    for (int i = 0; i < st_->exit_count; ++i) st_->exits[i] = def.exits[i];
    st_->fills.fill({});
    ticks_ = 0;
    invalidate();
}

TileMap& Room::tiles() { return st_->map; }
const TileMap& Room::tiles() const { return st_->map; }

void Room::set_layer_row(int layer, uint8_t row) {
    if (!st_ || layer < 0 || layer >= kTileLayers) return;
    st_->layer_row[layer] = row;
    invalidate();
}

void Room::invalidate() {
    // sofort neu vorzeichnen: Änderungen sind selten, und das nächste Bild zeigt schon den neuen Stand
    rebuild_walk();
    bake();
}

int Room::part_count() const { return st_ ? st_->part_count : 0; }
const RoomPart& Room::part(int i) const { return st_->parts[i]; }

int Room::add_part(const RoomPart& p) {
    if (!st_ || st_->part_count >= kMaxParts) return -1;
    st_->parts[st_->part_count] = p;
    invalidate();
    return st_->part_count++;
}

void Room::remove_part(int i) {
    if (!st_ || i < 0 || i >= st_->part_count) return;
    for (int k = i + 1; k < st_->part_count; ++k) st_->parts[k - 1] = st_->parts[k];
    --st_->part_count;
    for (auto& f : st_->fills) {
        if (f.part == i) f = {};
        if (f.part > i) --f.part;
    }
    invalidate();
}

void Room::set_part(int i, const RoomPart& p) {
    if (!st_ || i < 0 || i >= st_->part_count) return;
    st_->parts[i] = p;
    invalidate();
}

void Room::set_stage(int i, int stage) {
    if (!st_ || i < 0 || i >= st_->part_count) return;
    st_->parts[i].stage = static_cast<uint8_t>(std::clamp(stage, 0, 255));
    invalidate();
}

void Room::set_state(int i, int state) {
    if (!st_ || i < 0 || i >= st_->part_count) return;
    const auto s = static_cast<uint8_t>(std::clamp(state, 0, 255));
    if (st_->parts[i].state == s) return;
    // nur unbewegte Teile stehen im Hintergrundbild; seit 0.5.10 Teil 3b nur ihr Rechteck mit den Stücken neu
    Rect dirty{};
    auto grow = [&](const Sprite& sp, int x, int y, int, bool) { dirty = unite(dirty, {x, y, sp.width, sp.height}); };
    const PartStage* l = look(i);
    const bool was_static = l && !moving(*l);
    if (was_static) {
        dirty = image_rect(i);
        for_pieces(i, grow);
    }
    st_->parts[i].state = s;
    l = look(i);
    if (l && !moving(*l)) {
        dirty = unite(dirty, image_rect(i));
        for_pieces(i, grow);
    }
    if (!dirty.empty()) bake_rect(dirty);
}

void Room::set_building(int i, bool on) {
    if (!st_ || i < 0 || i >= st_->part_count) return;
    uint8_t& f = st_->parts[i].flags;
    f = static_cast<uint8_t>(on ? (f | kPartBuilding) : (f & ~kPartBuilding));
    invalidate();
}

bool Room::building(int i) const {
    return st_ && i >= 0 && i < st_->part_count && (st_->parts[i].flags & kPartBuilding);
}

int Room::resolve(const RoomPart& p) const {
    if (!catalog_) return -1;
    int id = p.part;
    if (p.part == kSignaturePart) id = catalog_->signature ? catalog_->signature(p.owner) : -1;
    return id >= 0 && id < catalog_->count ? id : -1;
}

const PartDef* Room::def(int i) const {
    if (!st_ || i < 0 || i >= st_->part_count) return nullptr;
    const int id = resolve(st_->parts[i]);
    return id < 0 ? nullptr : &catalog_->parts[id];
}

int Room::stage_count(int i) const {
    const PartDef* d = def(i);
    return d ? d->stage_count : 0;
}

const PartStage* Room::stage(int i) const {
    const PartDef* d = def(i);
    if (!d || d->stage_count == 0) return nullptr;
    return &d->stages[std::min<int>(st_->parts[i].stage, d->stage_count - 1)];
}

const PartStage* Room::look(int i) const {
    const PartStage* s = stage(i);
    if (!s || !(st_->parts[i].flags & kPartBuilding)) return s;
    const PartDef* d = def(i);
    return d->building ? d->building : catalog_->site;  // nullptr: im Bau ohne Bild, nichts zu sehen
}

const uint8_t* Room::remap_of(const RoomPart& p, const PartStage& look) const {
    if (&look == catalog_->site) return nullptr;  // Materialstapel hat eigene Farben
    return p.row > 0 && p.row < catalog_->row_count ? catalog_->rows[p.row].table() : nullptr;
}

int Room::find_part(PartKind kind, int start) const {
    for (int i = std::max(start, 0); i < part_count(); ++i)
        if (const PartDef* d = def(i); d && d->kind == kind) return i;
    return -1;
}

Rect Room::part_rect(int i) const {
    const PartStage* s = stage(i);
    if (!s) return {};
    const RoomPart& p = st_->parts[i];
    if (p.flags & kPartOnSlot) return image_rect(i);
    return {p.x * kTileSize, p.y * kTileSize, s->w * kTileSize, s->h * kTileSize};
}

Rect Room::image_rect(int i) const {
    const PartStage* l = look(i);
    int x, y, depth;
    if (!l || !l->sheet.pixels || !locate(i, *l, &x, &y, &depth)) return {};
    return {x, y, l->sheet.width, l->sheet.height};
}

PartLayer Room::layer_of(int i) const {
    for (int n = 0; n < 4 && i >= 0 && i < part_count(); ++n) {
        const RoomPart& p = st_->parts[i];
        if (!(p.flags & kPartOnSlot)) {
            const PartDef* d = def(i);
            return d ? d->layer : PartLayer::Standing;
        }
        i = p.x;  // Träger
    }
    return PartLayer::Standing;
}

bool Room::slot_point(int i, int k, int* x, int* y) const {
    const PartStage* l = look(i);
    if (!l || k < 0 || k >= l->slot_count || !l->sheet.pixels) return false;
    int px, py, depth;
    if (!locate(i, *l, &px, &py, &depth)) return false;
    const SlotPoint& sp = l->slots[k];
    *x = px + ((st_->parts[i].flags & kPartFlipped) ? l->sheet.width - 1 - sp.x : sp.x);
    *y = py + sp.y;
    return true;
}

int Room::slot_item(int host, int k) const {
    for (int i = 0; i < part_count(); ++i) {
        const RoomPart& p = st_->parts[i];
        if ((p.flags & kPartOnSlot) && p.x == host && p.y == k) return i;
    }
    return -1;
}

int Room::slot_count(int i) const {
    const PartStage* l = look(i);
    return l ? l->slot_count : 0;
}

template <class Fill, class F>
void Room::for_fill(const Fill& f, int from, int to, F&& fn) const {
    if (f.part < 0 || !f.piece || f.n <= 0 || !f.piece->sheet.pixels) return;
    const PartStage* hl = look(f.part);
    if (!hl || !hl->sheet.pixels) return;
    int hx, hy, hd;
    if (!locate(f.part, *hl, &hx, &hy, &hd)) return;
    const bool hflip = st_->parts[f.part].flags & kPartFlipped;
    const SpriteSheet& sh = f.piece->sheet;
    const int variants = std::max(sh.states, 1);
    to = std::min({to, f.n, f.count, static_cast<int>(hl->slot_count) - f.first});
    for (int j = std::max(from, 0); j < to; ++j) {
        const int k = f.first + j;  // Stellplatz
        const SlotPoint& sp = hl->slots[k];
        const Sprite s = sh.frame(j % variants, 0);
        const int fx = hx + (hflip ? hl->sheet.width - 1 - sp.x : sp.x);
        // Spielart und Spiegeln nach dem Stellplatz: jeder Stapel sieht gleich aus, egal wie er gewachsen ist
        const bool flip = ((static_cast<unsigned>(k) * 0x9E37u) >> 5) & 1u;
        fn(s, fx - s.width / 2, hy + sp.y - s.height + 1, hd + 1, flip);
    }
}

template <class F>
void Room::for_pieces(int i, F&& fn) const {
    // Bereiche in der Reihenfolge ihrer Stellplätze: spätere liegen vorn, egal in welcher Reihenfolge sie entstanden
    int done = -1;
    for (;;) {
        const Storage::Fill* next = nullptr;
        for (const auto& f : st_->fills)
            if (f.part == i && f.first > done && (!next || f.first < next->first)) next = &f;
        if (!next) return;
        for_fill(*next, 0, kMaxParts, fn);
        done = next->first;
    }
}

void Room::set_fill(int i, const PartStage* piece, int n, int first, int count) {
    if (!st_ || i < 0 || i >= st_->part_count || first < 0 || count <= 0) return;
    n = piece ? std::max(n, 0) : 0;
    Storage::Fill* f = nullptr;
    Storage::Fill* free = nullptr;
    for (auto& x : st_->fills) {
        if (x.part == i && x.first == first) f = &x;
        if (x.part < 0 && !free) free = &x;
    }
    if (!f) {
        if (n == 0 || !free) return;
        f = free;
        *f = {i, piece, 0, first, count};
    }
    if (f->piece == piece && f->n == n && f->count == count) return;
    // neu zu zeichnen: bei gleichem Stück und Bereich nur die Stellplätze zwischen alter und neuer Zahl, sonst alle
    const bool same = f->piece == piece && f->count == count;
    const int lo = same ? std::min(f->n, n) : 0;
    Rect dirty{};
    auto grow = [&](const Sprite& s, int x, int y, int, bool) { dirty = unite(dirty, {x, y, s.width, s.height}); };
    for_fill(*f, lo, kMaxParts, grow);  // alter Stand: was verschwindet
    f->piece = piece;
    f->n = n;
    f->count = count;
    for_fill(*f, lo, kMaxParts, grow);  // neuer Stand: was dazukommt
    if (n == 0) *f = {};
    if (!dirty.empty()) bake_rect(dirty);
}

int Room::fill_shown(int i, int first) const {
    if (!st_) return 0;
    int shown = 0;
    for (const auto& x : st_->fills)
        if (x.part == i && x.piece && (first < 0 || x.first == first))
            shown += std::max(0, std::min({x.n, x.count, slot_count(i) - x.first}));
    return shown;
}

uint32_t Room::bakes() const { return st_ ? st_->bakes : 0; }

bool Room::use_point(int i, int k, int* tx, int* ty, UsePoint* out) const {
    const PartStage* s = look(i);
    if (!s || k < 0 || k >= s->use_count || s == catalog_->site) return false;
    const RoomPart& p = st_->parts[i];
    UsePoint u = s->uses[k];
    if (p.flags & kPartFlipped) {
        u.dx = static_cast<int8_t>(s->w - 1 - u.dx);
        u.facing = mirrored(u.facing);
    }
    if (tx) *tx = p.x + u.dx;
    if (ty) *ty = p.y + u.dy;
    if (out) *out = u;
    return true;
}

int Room::exit_count() const { return st_ ? st_->exit_count : 0; }
const Exit& Room::exit(int i) const { return st_->exits[i]; }

int Room::exit_at(int tx, int ty) const {
    for (int i = 0; i < exit_count(); ++i) {
        const Exit& e = st_->exits[i];
        if (tx >= e.x && tx < e.x + e.w && ty >= e.y && ty < e.y + e.h) return i;
    }
    return -1;
}

bool Room::walkable(int tx, int ty) const {
    if (!st_ || tx < 0 || ty < 0 || tx >= kTilesX || ty >= kTilesY) return false;
    return st_->walk[ty][tx] != 0;
}

void Room::rebuild_walk() {
    if (!st_) return;
    for (int y = 0; y < kTilesY; ++y)
        for (int x = 0; x < kTilesX; ++x) st_->walk[y][x] = tiles_ && st_->map.blocks(*tiles_, x, y) ? 0 : 1;
    for (int i = 0; i < st_->part_count; ++i) {
        const PartDef* d = def(i);
        const PartStage* s = stage(i);
        if (!d || !s || !d->blocks || (st_->parts[i].flags & kPartOnSlot)) continue;
        const RoomPart& p = st_->parts[i];
        for (int y = p.y; y < p.y + s->h && y < kTilesY; ++y)
            for (int x = p.x; x < p.x + s->w && x < kTilesX; ++x) st_->walk[y][x] = 0;
    }
}

bool Room::step_toward(int fx, int fy, int tx, int ty, Facing* dir) const {
    if (!st_ || (fx == tx && fy == ty)) return false;
    if (tx < 0 || ty < 0 || tx >= kTilesX || ty >= kTilesY || fx < 0 || fy < 0 || fx >= kTilesX || fy >= kTilesY)
        return false;
    // Vom Ziel aus suchen: jede gefundene Kachel merkt sich die Richtung, die einen Schritt näher ans Ziel führt
    std::memset(st_->next, kUnseen, sizeof(st_->next));
    int head = 0, tail = 0;
    st_->queue[tail++] = static_cast<uint16_t>(ty * kTilesX + tx);
    st_->next[ty * kTilesX + tx] = 0;
    const int from = fy * kTilesX + fx;
    while (head < tail) {
        const int c = st_->queue[head++];
        const int cx = c % kTilesX, cy = c / kTilesX;
        for (int d = 0; d < 4; ++d) {
            const int nx = cx + kStepX[d], ny = cy + kStepY[d];
            if (nx < 0 || ny < 0 || nx >= kTilesX || ny >= kTilesY) continue;
            const int n = ny * kTilesX + nx;
            if (st_->next[n] != kUnseen) continue;
            if (n != from && !st_->walk[ny][nx]) continue;
            st_->next[n] = static_cast<uint8_t>(d ^ 1);  // Gegenrichtung: von n zurück nach c
            if (n == from) {
                if (dir) *dir = static_cast<Facing>(d ^ 1);
                return true;
            }
            st_->queue[tail++] = static_cast<uint16_t>(n);
        }
    }
    return false;
}

int Room::path(int fx, int fy, int tx, int ty, Facing* dirs, int max) const {
    if (fx == tx && fy == ty) return 0;
    if (!st_ || tx < 0 || ty < 0 || tx >= kTilesX || ty >= kTilesY || fx < 0 || fy < 0 || fx >= kTilesX ||
        fy >= kTilesY)
        return -1;
    // wie step_toward: vom Ziel aus suchen, bis der Start gefunden ist; dann den Richtungen bis zum Ziel folgen
    std::memset(st_->next, kUnseen, sizeof(st_->next));
    int head = 0, tail = 0;
    st_->queue[tail++] = static_cast<uint16_t>(ty * kTilesX + tx);
    st_->next[ty * kTilesX + tx] = 0;
    const int from = fy * kTilesX + fx;
    bool found = false;
    while (head < tail && !found) {
        const int c = st_->queue[head++];
        const int cx = c % kTilesX, cy = c / kTilesX;
        for (int d = 0; d < 4; ++d) {
            const int nx = cx + kStepX[d], ny = cy + kStepY[d];
            if (nx < 0 || ny < 0 || nx >= kTilesX || ny >= kTilesY) continue;
            const int n = ny * kTilesX + nx;
            if (st_->next[n] != kUnseen) continue;
            if (n != from && !st_->walk[ny][nx]) continue;
            st_->next[n] = static_cast<uint8_t>(d ^ 1);
            if (n == from) {
                found = true;
                break;
            }
            st_->queue[tail++] = static_cast<uint16_t>(n);
        }
    }
    if (!found) return -1;
    int steps = 0, x = fx, y = fy;
    while ((x != tx || y != ty) && steps < kTileCells) {
        const int d = st_->next[y * kTilesX + x];
        if (dirs && steps < max) dirs[steps] = static_cast<Facing>(d);
        x += kStepX[d];
        y += kStepY[d];
        ++steps;
    }
    return steps;
}

void Room::place(const RoomPart& p, int fw, int fh, const PartStage& look, int* x, int* y, int* depth) const {
    const int bottom = (p.y + fh) * kTileSize;
    *x = p.x * kTileSize + (fw * kTileSize - look.sheet.width) / 2 + ((p.flags & kPartFlipped) ? -look.ox : look.ox);
    *y = bottom - look.sheet.height + look.oy;
    *depth = bottom - 1;
}

bool Room::locate(int i, const PartStage& lk, int* x, int* y, int* depth, int nest) const {
    const RoomPart& p = st_->parts[i];
    if (!(p.flags & kPartOnSlot)) {
        const PartStage* s = stage(i);
        if (!s) return false;
        place(p, s->w, s->h, lk, x, y, depth);
        return true;
    }
    // auf einem Stellplatz: Fußpunkt des Trägers, knapp vor ihm
    const int host = p.x;
    if (nest > 3 || host >= i || host < 0) return false;  // Träger steht immer davor (keine Schleifen)
    const PartStage* hl = look(host);
    if (!hl || p.y >= hl->slot_count) return false;
    int hx, hy, hd;
    if (!locate(host, *hl, &hx, &hy, &hd, nest + 1)) return false;
    const SlotPoint& sp = hl->slots[p.y];
    const int fx = hx + ((st_->parts[host].flags & kPartFlipped) ? hl->sheet.width - 1 - sp.x : sp.x);
    *x = fx - lk.sheet.width / 2;
    *y = hy + sp.y - lk.sheet.height + 1;
    *depth = hd + 1;
    return true;
}

Sprite Room::frame_of(const RoomPart& p, const PartStage& s) const {
    // Zustandsbilder (seit 0.5.5): Zustand des Teils wählt seine Bilder, deren Animation läuft mit den Schritten
    const int frames = std::max(s.sheet.frames_per_state(), 1);
    const int f = frames > 1 ? static_cast<int>(ticks_ / std::max<int>(s.frame_ticks, 1) % frames) : 0;
    return s.sheet.frame(p.state, f);
}

void Room::draw_static(Canvas& c, bool standing, const Rect* clip) const {
    int tx0 = 0, ty0 = 0, tx1 = kTilesX, ty1 = kTilesY;
    Rect cl{0, 0, c.width(), c.height()};
    if (clip) {
        // auf ganze Kacheln erweitert: die Kachelebenen decken dann genau wie beim ganzen Bild
        auto down = [](int v) { return v >= 0 ? v / kTileSize : -((-v + kTileSize - 1) / kTileSize); };
        tx0 = std::clamp(down(clip->x), 0, kTilesX);
        ty0 = std::clamp(down(clip->y), 0, kTilesY);
        tx1 = std::clamp(down(clip->x + clip->w + kTileSize - 1), 0, kTilesX);
        ty1 = std::clamp(down(clip->y + clip->h + kTileSize - 1), 0, kTilesY);
        cl = {tx0 * kTileSize, ty0 * kTileSize, (tx1 - tx0) * kTileSize, (ty1 - ty0) * kTileSize};
        if (cl.empty()) return;
        c.fill_rect(cl.x, cl.y, cl.w, cl.h, 0);
    } else {
        c.clear(0);
    }
    if (tiles_)
        for (int l = 0; l < kTileLayers; ++l) {
            const int row = st_->layer_row[l];
            const uint8_t* remap = catalog_ && row > 0 && row < catalog_->row_count ? catalog_->rows[row].table()
                                                                                     : nullptr;
            st_->map.draw(c, *tiles_, l, l == kFloorLayer, remap, tx0, ty0, tx1, ty1);
        }
    auto put = [&](const Sprite& s, int x, int y, bool flip, const uint8_t* remap) {
        if (clip)
            c.blit_clipped(s, x, y, flip, remap, cl);
        else
            c.blit(s, x, y, flip, remap);
    };
    // flache Teile in Listenreihenfolge: erst Boden, dann Wand; Stücke einer Füllung gleich nach ihrem Träger
    for (int pass = 0; pass < 2; ++pass) {
        const PartLayer want = pass == 0 ? PartLayer::Floor : PartLayer::Wall;
        for (int i = 0; i < st_->part_count; ++i) {
            const PartDef* d = def(i);
            const PartStage* s = stage(i);
            const PartStage* l = look(i);
            if (!d || !s || !l || !l->sheet.pixels || layer_of(i) != want || moving(*l)) continue;
            const RoomPart& p = st_->parts[i];
            int x, y, depth;
            if (!locate(i, *l, &x, &y, &depth)) continue;
            put(frame_of(p, *l), x, y, p.flags & kPartFlipped, remap_of(p, *l));
            for_pieces(i, [&](const Sprite& ps, int px, int py, int, bool f) { put(ps, px, py, f, nullptr); });
        }
    }
    if (!standing) return;
    DepthList& list = st_->list;
    list.clear();
    for (int i = 0; i < st_->part_count; ++i) {
        const PartDef* d = def(i);
        const PartStage* s = stage(i);
        const PartStage* l = look(i);
        if (!d || !s || !l || !l->sheet.pixels || layer_of(i) != PartLayer::Standing || moving(*l)) continue;
        const RoomPart& p = st_->parts[i];
        int x, y, depth;
        if (!locate(i, *l, &x, &y, &depth)) continue;
        list.add_at(frame_of(p, *l), x, y, depth, p.flags & kPartFlipped, remap_of(p, *l));
        for_pieces(i, [&](const Sprite& ps, int px, int py, int pd, bool f) { list.add_at(ps, px, py, pd, f); });
    }
    if (clip)
        list.draw_clipped(c, cl);
    else
        list.draw(c, true);
}

void Room::bake() {
    if (!st_ || !bg_) return;
    Canvas c(bg_, kCoarseWidth, kCoarseHeight);
    draw_static(c, true);  // ohne Licht: das kommt beim Verdoppeln aus der Lichtkarte
    ++st_->bakes;
}

void Room::bake_rect(Rect r) {
    if (!st_ || !bg_) return;
    Canvas c(bg_, kCoarseWidth, kCoarseHeight);
    draw_static(c, true, &r);
    ++st_->bakes;
}

void Room::start_lights() {
    // betreten: in den zweiten Puffer, sichtbar erst, wenn fertig; unsichtbar an Ort und Stelle (zeichnet niemand)
    Storage& s = *st_;
    s.build_again = false;
    s.build_back = bg_ != nullptr;
    uint8_t*& to = s.build_back ? lmb_ : lm_;
    if (!to) {
        to = static_cast<uint8_t*>(asset_alloc(kCoarsePixels));
        if (!to) {
            s.build_row = -1;
            return;
        }
        std::memset(to, 0, kCoarsePixels);
        if (!s.build_back) {
            s.lx0.fill(0);
            s.lx1.fill(0);
        }
    }
    s.build_row = 0;
}

int Room::build_lights(int budget) {
    if (!st_ || st_->build_row < 0) return 0;
    Storage& s = *st_;
    uint8_t* to = s.build_back ? lmb_ : lm_;
    uint8_t* x0 = s.build_back ? s.bx0.data() : s.lx0.data();
    uint8_t* x1 = s.build_back ? s.bx1.data() : s.lx1.data();
    int used = 0;
    while (s.build_row < kCoarseHeight) {
        const int y = s.build_row;
        const int cost = light_row_cost(s.plan, y, kCoarseWidth);
        if (used > 0 && used + cost > budget) return used;
        build_light_row(to + y * kCoarseWidth, kCoarseWidth, y, s.plan, &x0[y], &x1[y]);
        used += cost;
        ++s.build_row;
    }
    if (s.build_back) {
        std::swap(lm_, lmb_);
        std::swap(s.lx0, s.bx0);
        std::swap(s.lx1, s.bx1);
    }
    s.build_row = -1;
    ++s.builds;
    if (s.build_again) start_lights();
    return used;
}

bool Room::lights_pending() const { return st_ && st_->build_row >= 0; }
uint32_t Room::light_builds() const { return st_ ? st_->builds : 0; }

void Room::tick() { ++ticks_; }

void Room::enter() {
    if (!st_) return;
    if (!bg_) bg_ = static_cast<uint8_t*>(asset_alloc(kCoarsePixels));
    bake();
}

void Room::leave() {
    if (bg_) asset_free(bg_);
    bg_ = nullptr;
    // unsichtbar braucht es keinen zweiten Puffer: ein Neubau dorthinein beginnt an Ort und Stelle neu
    if (st_ && st_->build_row >= 0 && st_->build_back) start_lights();
    if (lmb_) asset_free(lmb_);
    lmb_ = nullptr;
}

void Room::set_lights(const LightCircle* lights, int n, int base, int levels, LightStyle style) {
    if (!st_) return;
    Storage& s = *st_;
    n = std::clamp(n, 0, kMaxLights);
    s.light_base = base;  // nur fürs Zeichnen
    auto same = [](const LightCircle& a, const LightCircle& b) {
        return a.x == b.x && a.y == b.y && a.rx == b.rx && a.ry == b.ry && a.strength == b.strength && a.inner == b.inner;
    };
    if (n == s.light_count && levels == s.light_levels && style == s.light_style &&
        std::equal(lights, lights + n, s.lights.begin(), same) && (n == 0 || lm_ || s.build_row >= 0))
        return;  // gleiche Lichter: keine neue Karte
    s.light_count = n;
    std::copy_n(lights, n, s.lights.begin());
    s.light_levels = levels;
    s.light_style = style;
    if (n == 0) {  // aus: sofort, ohne Neubau
        if (lm_) asset_free(lm_);
        if (lmb_) asset_free(lmb_);
        lm_ = lmb_ = nullptr;
        s.build_row = -1;
        s.build_again = false;
        return;
    }
    plan_light_map(&s.plan, lights, n, levels, style);
    // läuft schon ein Durchgang, endet er mit den neuen Tabellen; danach noch einer (so wird jeder fertig)
    if (s.build_row >= 0)
        s.build_again = true;
    else
        start_lights();
}

int Room::light_count() const { return st_ ? st_->light_count : 0; }

void Room::set_shadow_table(const uint8_t* table) {
    if (st_) st_->list.set_shadow_table(table);
}

void Room::set_shadow_color(uint8_t c) {
    if (st_) st_->list.set_shadow_color(c);
}

void Room::draw_inspect(Canvas& fine, uint8_t blocked, uint8_t part, uint8_t use, uint8_t exit) const {
    if (!st_) return;
    constexpr int k = kCoarseScale;
    auto frame = [&](int x, int y, int w, int h, uint8_t c) {
        fine.fill_rect(x, y, w, 1, c);
        fine.fill_rect(x, y + h - 1, w, 1, c);
        fine.fill_rect(x, y, 1, h, c);
        fine.fill_rect(x + w - 1, y, 1, h, c);
    };
    for (int y = 0; y < kTilesY; ++y)
        for (int x = 0; x < kTilesX; ++x)
            if (!st_->walk[y][x]) fine.fill_rect((x * kTileSize + 3) * k, (y * kTileSize + 3) * k, 2 * k, 2 * k, blocked);
    for (int i = 0; i < st_->part_count; ++i) {
        const Rect r = part_rect(i);
        if (!r.empty()) frame(r.x * k + 1, r.y * k + 1, r.w * k - 2, r.h * k - 2, part);
        for (int u = 0;; ++u) {
            int tx, ty;
            UsePoint up;
            if (!use_point(i, u, &tx, &ty, &up)) break;
            const int cx = (tx * kTileSize + 4) * k, cy = (ty * kTileSize + 4) * k;
            fine.fill_rect(cx - 3, cy - 1, 7, 3, use);
            fine.fill_rect(cx - 1, cy - 3, 3, 7, use);
            // Blickrichtung als kurzer Strich
            const int dx = up.facing == Facing::Left ? -1 : up.facing == Facing::Right ? 1 : 0;
            const int dy = up.facing == Facing::Up ? -1 : up.facing == Facing::Down ? 1 : 0;
            for (int s = 4; s < 9; ++s) fine.fill_rect(cx + dx * s, cy + dy * s, 1, 1, use);
        }
        // Stellplätze (seit 0.5.8): Fußpunkt als kleines Kreuz
        for (int k2 = 0;; ++k2) {
            int sx, sy;
            if (!slot_point(i, k2, &sx, &sy)) break;
            fine.fill_rect(sx * k - 2, sy * k + 1, 6, 1, use);
            fine.fill_rect(sx * k, sy * k - 1, 2, 5, use);
        }
    }
    for (int i = 0; i < st_->exit_count; ++i) {
        const Exit& e = st_->exits[i];
        const int x = e.x * kTileSize * k, y = e.y * kTileSize * k, w = e.w * kTileSize * k, h = e.h * kTileSize * k;
        frame(x, y, w, h, exit);
        frame(x + 1, y + 1, w - 2, h - 2, exit);
    }
}

int Room::hit_part(int x, int y, int* depth_out) const {
    if (!st_) return -1;
    int best = -1, best_depth = 0;
    for (int i = 0; i < st_->part_count; ++i) {
        const PartDef* d = def(i);
        const PartStage* s = stage(i);
        const PartStage* l = look(i);
        if (!d || !s || !l || !l->sheet.pixels) continue;
        const RoomPart& p = st_->parts[i];
        int px, py, depth;
        if (!locate(i, *l, &px, &py, &depth)) continue;
        const bool flat = layer_of(i) != PartLayer::Standing;
        // gleiche Tiefe: das später eingetragene Teil liegt vorn (stabile Sortierung)
        auto take = [&](const Sprite& fr, int ix, int iy, int dp, bool flip) {
            const int sx = x - ix, sy = y - iy;
            if (sx < 0 || sy < 0 || sx >= fr.width || sy >= fr.height) return;
            if (!fr.pixels[sy * fr.width + (flip ? fr.width - 1 - sx : sx)]) return;
            if (flat) dp = kFlatDepth;
            if (best < 0 || dp >= best_depth) {
                best = i;
                best_depth = dp;
            }
        };
        take(frame_of(p, *l), px, py, depth, p.flags & kPartFlipped);
        for_pieces(i, take);  // Stücke einer Füllung gehören zum Lagerplatz
    }
    if (depth_out) *depth_out = best_depth;
    return best;
}

DepthList& Room::begin(Canvas& c, bool with_background, CoarseLayers* layers) const {
    DepthList& list = st_->list;
    const bool bg = with_background && bg_;
    if (layers) layers->base = nullptr;
    if (!bg) {
        draw_static(c, false);
    } else if (!layers) {
        std::memcpy(c.pixels(), bg_, kCoarsePixels);
    }
    list.clear();
    if (bg && layers) {
        layers->base = bg_;  // die Liste kopiert den Hintergrund nur unter die bewegten Teile
        list.use_layers(layers);
    }
    // Licht: Figuren gleichmäßig nach ihrem Fußpunkt, alles andere hebt das Verdoppeln nach der Lichtkarte
    const uint8_t* lm = light_on_ ? lm_ : nullptr;  // tagsüber sind alle Lichtstufen gleich: nichts zu tun
    if (lm) list.use_light(lm, st_->light_base);
    if (layers) {
        layers->light = lm;
        if (lm) {
            layers->lx0 = st_->lx0;
            layers->lx1 = st_->lx1;
            layers->light_base = static_cast<uint8_t>(st_->light_base);
        }
    }
    for (int i = 0; i < st_->part_count; ++i) {
        const PartDef* d = def(i);
        const PartStage* s = stage(i);
        const PartStage* l = look(i);
        if (!d || !s || !l || !l->sheet.pixels) continue;  // Bild fehlt (Paket nicht geladen)
        const bool anim = moving(*l);
        const bool flat = layer_of(i) != PartLayer::Standing;
        if (flat && !anim) continue;  // steht im Hintergrund bzw. wurde eben gezeichnet
        const RoomPart& p = st_->parts[i];
        int x, y, depth;
        if (!locate(i, *l, &x, &y, &depth)) continue;
        list.add_at(frame_of(p, *l), x, y, flat ? kFlatDepth : depth, p.flags & kPartFlipped, remap_of(p, *l),
                    bg && !anim);
        for_pieces(i, [&](const Sprite& ps, int px, int py, int pd, bool f) {
            list.add_at(ps, px, py, flat ? kFlatDepth : pd, f, nullptr, bg && !anim);
        });
    }
    return list;
}

}  // namespace pixelia
