#include "pixelia/engine/stage.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "pixelia/engine/light.h"
#include "pixelia/engine/system_ui.h"

namespace pixelia {

static_assert(ui::kFirstIndex == kSceneColors, "Szenenfarben und System-Oberfläche teilen sich die Palette");
static_assert(kCoarseWidth % 2 == 0, "upscale2x verarbeitet zwei grobe Bildpunkte auf einmal");

namespace {

constexpr uint8_t kBayer4[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};
constexpr const char* kTransitionNames[kTransitionKinds] = {"schnitt", "ueberblendung", "raster", "abblenden"};

// Gut gemischte 32-Bit-Zahl (Reihenfolge der Bildpunkte bei der Überblendung)
uint32_t mix(uint32_t h) {
    h ^= h >> 16;
    h *= 0x7FEB352Du;
    h ^= h >> 15;
    h *= 0x846CA68Bu;
    h ^= h >> 16;
    return h;
}

Color scaled(Color c, int k256) {
    return {static_cast<uint8_t>(c.r * k256 >> 8), static_cast<uint8_t>(c.g * k256 >> 8),
            static_cast<uint8_t>(c.b * k256 >> 8)};
}

}  // namespace

const char* transition_name(Transition t) {
    const int i = static_cast<int>(t);
    return i >= 0 && i < kTransitionKinds ? kTransitionNames[i] : "?";
}

bool parse_transition(std::string_view name, Transition* out) {
    for (int i = 0; i < kTransitionKinds; ++i) {
        if (name == kTransitionNames[i]) {
            *out = static_cast<Transition>(i);
            return true;
        }
    }
    return false;
}

namespace {
inline void double_span(const uint8_t* src, uint32_t* out, int x0, int x1) {
    for (int x = x0; x < x1; x += 2) {
        const uint32_t a = src[x], b = src[x + 1];
        out[x / 2] = a | a << 8 | b << 16 | b << 24;  // little-endian: a a b b
    }
}

// Grundfarbe s im Licht: Stufen der vier feinen Bildpunkte aus der Lichtkarte (p, je 2 Bit), in a0 a1 / b0 b1
inline void lit4(uint8_t s, uint8_t p, const uint8_t* off, uint8_t* a, uint8_t* b) {
    a[0] = static_cast<uint8_t>(s + off[p & 3]);
    a[1] = static_cast<uint8_t>(s + off[p >> 2 & 3]);
    b[0] = static_cast<uint8_t>(s + off[p >> 4 & 3]);
    b[1] = static_cast<uint8_t>(s + off[p >> 6]);
}

// Verschiebung je Lichtstufe 0 … 3
inline std::array<uint8_t, 4> level_offsets(int base) {
    return {0, static_cast<uint8_t>(base), static_cast<uint8_t>(2 * base), static_cast<uint8_t>(3 * base)};
}

// Licht im Bereich [x0, x1) der groben Zeile y auf die beiden schon verdoppelten feinen Zeilen a und b
void light_span(uint8_t* a, uint8_t* b, int y, int x0, int x1, const CoarseLayers& L) {
    const uint8_t* m = L.light + y * kCoarseWidth;
    const int base = L.light_base;
    const auto off = level_offsets(base);
    for (int x = x0; x < x1; ++x) {
        const uint8_t p = m[x];
        const int fx = 2 * x;
        const uint8_t s = a[fx];
        if (p && s && s < base) lit4(s, p, off.data(), a + fx, b + fx);
    }
}

// Bitmuster des 4×4-Rasters für 8 feine Bildpunkte: kVeilMask[Anteil][Zeile & 3], Bit k = Bildpunkt k gesetzt
struct VeilMasks {
    uint8_t m[17][4];
    constexpr VeilMasks() : m{} {
        for (int lv = 0; lv <= 16; ++lv)
            for (int r = 0; r < 4; ++r)
                for (int k = 0; k < 8; ++k)
                    if (kBayer4[r][k & 3] < lv) m[lv][r] |= static_cast<uint8_t>(1u << k);
    }
};
constexpr VeilMasks kVeilMask{};

// Schleier (Wetter, seit 0.5.3) auf die beiden feinen Zeilen der groben Zeile y: je Feld ein Anteil 0 … 16, fein
// gerastert; nach dem Licht, die Farbtabelle behält die Lichtstufe
void veil_rows(uint8_t* a, uint8_t* b, int y, const CoarseLayers& L) {
    constexpr int kSpan = CoarseLayers::kVeilCell * kCoarseScale;  // 8 feine Bildpunkte je Feld
    static_assert(kSpan == 8, "Bitmuster für 8 Bildpunkte");
    const uint8_t* g = L.veil_grid + (y / CoarseLayers::kVeilCell) * CoarseLayers::kVeilW;
    const uint8_t* t = L.veil_table;
    const int ra = (2 * y) & 3, rb = (2 * y + 1) & 3;
    for (int c = 0; c < CoarseLayers::kVeilW; ++c) {
        const int lv = g[c] > 16 ? 16 : g[c];
        if (!lv) continue;
        uint8_t* pa = a + c * kSpan;
        uint8_t* pb = b + c * kSpan;
        // nur die gesetzten Bildpunkte: bei schwachem Schleier wenige
        for (unsigned m = kVeilMask.m[lv][ra]; m; m &= m - 1) {
            const int k = __builtin_ctz(m);
            pa[k] = t[pa[k]];
        }
        for (unsigned m = kVeilMask.m[lv][rb]; m; m &= m - 1) {
            const int k = __builtin_ctz(m);
            pb[k] = t[pb[k]];
        }
    }
}
}  // namespace

void upscale2x(const uint8_t* coarse, Canvas& fine, const CoarseLayers* layers) {
    // Zeile im (schnellen) Stapel aufbauen, dann zweimal am Stück kopieren: auf dem Gerät liegt fine im PSRAM.
    // Gemessen (0.5.2, „bench mem“): 96 KB in den PSRAM schreiben kostet allein 2,4 ms (CPU wie DMA), die Schleife
    // selbst ist nicht der Engpass.
    alignas(4) uint8_t row[kScreenWidth];
    alignas(4) uint8_t row2[kScreenWidth];
    auto* out = reinterpret_cast<uint32_t*>(row);
    const uint8_t* base = layers ? layers->base : nullptr;
    const bool light = layers && layers->light;
    const bool veil = layers && layers->veil_table && layers->veil_grid;
    bool veil_row = false;
    for (int y = 0; y < kCoarseHeight; ++y) {
        const uint8_t* src = coarse + y * kCoarseWidth;
        if (base) {
            const uint8_t* b = base + y * kCoarseWidth;
            const int x0 = layers->x0[y], x1 = layers->x1[y];
            double_span(b, out, 0, x0);
            double_span(src, out, x0, x1);
            double_span(b, out, x1, kCoarseWidth);
        } else {
            double_span(src, out, 0, kCoarseWidth);
        }
        // Licht (light.h) und Schleier (Wetter) erst hier: so kann das Raster fein sein (2 × 2 Displaypunkte)
        const bool lit = light && layers->lx1[y] > layers->lx0[y];
        if (veil && y % CoarseLayers::kVeilCell == 0) {
            const uint8_t* g = layers->veil_grid + (y / CoarseLayers::kVeilCell) * CoarseLayers::kVeilW;
            veil_row = false;  // Feldzeile ohne Schleier: der schnelle Weg
            for (int c = 0; c < CoarseLayers::kVeilW && !veil_row; ++c) veil_row = g[c] != 0;
        }
        const bool veiled = veil && veil_row;
        if (lit || veiled) {
            std::memcpy(row2, row, kScreenWidth);
            if (lit) light_span(row, row2, y, layers->lx0[y], layers->lx1[y], *layers);
            if (veiled) veil_rows(row, row2, y, *layers);
            std::memcpy(fine.row(2 * y), row, kScreenWidth);
            std::memcpy(fine.row(2 * y + 1), row2, kScreenWidth);
            continue;
        }
        std::memcpy(fine.row(2 * y), row, kScreenWidth);
        std::memcpy(fine.row(2 * y + 1), row, kScreenWidth);
    }
}

void flatten_layers(uint8_t* coarse, const CoarseLayers& layers) {
    if (!layers.base) return;
    for (int y = 0; y < kCoarseHeight; ++y) {
        uint8_t* d = coarse + y * kCoarseWidth;
        const uint8_t* b = layers.base + y * kCoarseWidth;
        std::memcpy(d, b, layers.x0[y]);
        std::memcpy(d + layers.x1[y], b + layers.x1[y], kCoarseWidth - layers.x1[y]);
    }
}

int Stage::add(std::unique_ptr<Scene> scene) {
    if (count_ >= kMaxScenes || !scene) return -1;
    scenes_[count_] = std::move(scene);
    scenes_[count_]->set_environment(&env_);
    return count_++;
}

void Stage::init(uint64_t seed) {
    env_.weather.init(seed);
    for (int i = 0; i < count_; ++i) scenes_[i]->init(seed + static_cast<uint64_t>(i) * 0x9E3779B97F4A7C15ULL);
    if (active_ >= count_) active_ = 0;
    from_ = -1;
    pending_ = -1;
    step_ = 0;
    transitions_ = 0;
    travels_ = 0;
    entered_ = 0;
    sync_entered();
}

void Stage::sync_entered() {
    for (int i = 0; i < count_; ++i) {
        const bool want = i == active_ || i == from_;
        const bool have = entered_ & (1u << i);
        if (want == have) continue;
        if (want) {
            scenes_[i]->enter();
            entered_ |= 1u << i;
        } else {
            scenes_[i]->leave();
            entered_ &= ~(1u << i);
        }
    }
}

int Stage::find(std::string_view name) const {
    for (int i = 0; i < count_; ++i)
        if (name == scenes_[i]->name()) return i;
    return -1;
}

void Stage::set_active(int index) {
    if (index < 0 || index >= count_) return;
    active_ = index;
    from_ = -1;
    pending_ = -1;
    sync_entered();
}

void Stage::go(int index, Transition t) {
    if (index < 0 || index >= count_) return;
    pending_ = index;
    pending_trans_ = t;
}

void Stage::set_transition_ticks(int ticks) { ticks_ = std::clamp(ticks, 1, 300); }

void Stage::set_inspect(bool on) {
    inspect_ = on;
    for (int i = 0; i < count_; ++i) scenes_[i]->set_inspect(on);
}

bool Stage::command(const char* cmd) {
    if (!cmd) return false;
    char word[16] = {};
    int n = 0;
    if (std::sscanf(cmd, "wetter %15s", word) == 1) {
        WeatherKind k;
        if (!parse_weather(word, &k)) return false;
        int pct = 100;
        std::sscanf(cmd, "wetter %*s %d", &pct);
        const int strength = std::clamp(pct, 0, 100) * 255 / 100;
        if (std::strstr(cmd, "sofort"))
            env_.weather.set_now(k, strength);
        else
            env_.weather.set(k, strength);
        return true;
    }
    if (std::sscanf(cmd, "wind %15s", word) == 1) {
        const std::string_view w(word);
        if (w != "links" && w != "rechts") return false;
        env_.weather.set_wind_dir(w == "links" ? -1 : 1);
        return true;
    }
    if (std::sscanf(cmd, "blitzstil %15s", word) == 1) {
        const std::string_view w(word);
        if (w != "a" && w != "b" && w != "c") return false;
        env_.weather.set_flash_style(w == "a" ? FlashStyle::Dimmed : w == "b" ? FlashStyle::Sheet : FlashStyle::Short);
        return true;
    }
    if (std::strcmp(cmd, "blitz") == 0) {
        env_.weather.strike();
        return true;
    }
    int m = 0;
    if (const int k = std::sscanf(cmd, "blitze %d %d", &n, &m); k >= 1) {
        env_.weather.set_flash_seconds(n, k == 2 ? m : n);
        return true;
    }
    return count_ > 0 && scenes_[active_]->command(cmd);
}

void Stage::set_load(int load) {
    if (count_) scenes_[active_]->set_load(load);
}

void Stage::start_transition(int to, Transition t) {
    if (to == active_) return;
    from_ = -1;  // laufenden Übergang abschließen
    ++transitions_;
    if (t == Transition::Cut || ticks_ < 2) {
        active_ = to;
        sync_entered();
        return;
    }
    from_ = active_;
    active_ = to;
    step_ = 0;
    trans_ = t;
    sync_entered();  // vor merge_palettes: ein Ort darf seine Palette beim Betreten setzen
    if (t != Transition::Fade) {
        const bool both_fine = scenes_[from_]->scale() != kCoarseScale && scenes_[to]->scale() != kCoarseScale;
        if (both_fine || !merge_palettes()) trans_ = Transition::Fade;
    }
}

void Stage::deliver_travels() {
    // Reihenfolge der Orte fest: gehört zum deterministischen Ablauf
    for (int i = 0; i < count_; ++i) {
        Travel t;
        for (int n = 0; n < 16 && scenes_[i]->take_travel(&t); ++n) {
            const int to = t.to ? find(t.to) : -1;
            if (to < 0) {
                t.entry = t.from_exit;
                scenes_[i]->arrive(t);
                continue;
            }
            ++travels_;
            scenes_[to]->arrive(t);
            if (t.follow && follow_ && i == active_ && from_ < 0 && pending_ < 0) start_transition(to, t.transition);
        }
    }
}

bool Stage::merge_palettes() {
    // Erst exakt; reichen die 240 Farben nicht (nachts, drinnen ↔ draußen mit Wetter), ähnliche Farben zusammenlegen:
    // für die 0,6 s der Überblendung unsichtbar, am Ende gilt wieder die eigene Palette (seit 0.5.3, vorher Abblenden)
    constexpr int kTolerance[] = {0, 12, 30};
    for (int t : kTolerance)
        if (merge_palettes(t, false)) return true;
    return merge_palettes(60, true);
}

// Ein Durchgang mit Toleranz (Summe der Abstände je Kanal); last: ist alles belegt, die nächste Farbe nehmen
bool Stage::merge_palettes(int tolerance, bool last) {
    slots_ = 0;
    map_from_.fill(0);
    map_to_.fill(0);
    auto add = [&](int si, std::array<uint8_t, 256>& map) {
        const Scene& s = *scenes_[si];
        const Palette& p = s.palette();
        const int n = std::clamp(s.colors(), 1, kSceneColors);
        for (int i = 0; i < n; ++i) {
            const Color c = p[i];
            int slot = -1, best = 1 << 30;
            for (int k = 0; k < slots_; ++k) {
                const Color m = palette_[k];
                const int d = std::abs(m.r - c.r) + std::abs(m.g - c.g) + std::abs(m.b - c.b);
                if (d < best) {
                    best = d;
                    slot = k;
                    if (d <= tolerance) break;  // erste passende reicht
                }
            }
            if (best > tolerance) {
                if (slots_ < kSceneColors) {
                    slot = slots_++;
                    palette_[slot] = c;
                    slot_scene_[slot] = static_cast<uint8_t>(si);
                    slot_index_[slot] = static_cast<uint8_t>(i);
                } else if (!last) {
                    return false;
                }
            }
            map[i] = static_cast<uint8_t>(slot);
        }
        return true;
    };
    return add(from_, map_from_) && add(active_, map_to_);
}

int Stage::visible() const {
    if (from_ >= 0 && trans_ == Transition::Fade && step_ < ticks_ / 2) return from_;
    return active_;
}

void Stage::update_palette() {
    if (trans_ == Transition::Fade) {
        const int half = ticks_ / 2;
        const int k = step_ < half ? (half - step_) * 256 / half : (step_ - half) * 256 / (ticks_ - half);
        const Palette& p = scenes_[visible()]->palette();
        for (int i = 0; i < kSceneColors; ++i) palette_[i] = scaled(p[i], k);
        return;
    }
    // Farbwechsel der Szenen (Tageszeit, Farbrotation) laufen während der Überblendung weiter
    for (int k = 0; k < slots_; ++k) palette_[k] = scenes_[slot_scene_[k]]->palette()[slot_index_[k]];
    for (int k = slots_; k < kSceneColors; ++k) palette_[k] = {0, 0, 0};
}

void Stage::tick(const WorldClock& clock) {
    if (pending_ >= 0) {
        const int to = pending_;
        pending_ = -1;
        start_transition(to, pending_trans_);
    }
    env_.weather.tick();  // vor den Orten: alle sehen im selben Schritt dasselbe Wetter
    for (int i = 0; i < count_; ++i) scenes_[i]->tick(clock);
    const bool was = from_ >= 0;
    if (from_ >= 0 && ++step_ >= ticks_) from_ = -1;
    deliver_travels();
    if (was && from_ < 0) sync_entered();
    if (from_ >= 0) update_palette();
}

void Stage::set_buffers(uint8_t* main, uint8_t* aux) {
    main_ = main;
    aux_ = aux;
}

void Stage::use_own_buffers() {
    own_ = std::make_unique<uint8_t[]>(2 * kCoarsePixels);
    set_buffers(own_.get(), own_.get() + kCoarsePixels);
}

const Palette& Stage::palette() const {
    if (from_ >= 0) return palette_;
    return scenes_[active_]->palette();
}

void Stage::draw_scene(const Scene& s, Canvas& fine) const {
    if (s.scale() != kCoarseScale) {
        s.render(fine);
        return;
    }
    if (!main_) {
        fine.clear(0);
        return;
    }
    Canvas coarse(main_, kCoarseWidth, kCoarseHeight);
    layers_.veil_table = nullptr;  // trägt nur ein Ort mit Wetter ein
    layers_.veil_grid = nullptr;
    s.render_layers(coarse, layers_);
    upscale2x(main_, fine, &layers_);
    s.render_fine(fine, layers_, nullptr);
}

void Stage::render(Canvas& fine) const {
    if (count_ == 0) {
        fine.clear(0);
        return;
    }
    if (from_ < 0 || trans_ == Transition::Fade) {
        const Scene& s = *scenes_[visible()];
        draw_scene(s, fine);
        if (from_ < 0) s.render_overlay(fine);
        return;
    }
    // Überblendung: höchstens eine der beiden Szenen ist fein (sonst Abblenden), sie zeichnet direkt ins Bild
    const Scene& a = *scenes_[from_];
    const Scene& b = *scenes_[active_];
    if (!main_ || !aux_) {
        fine.clear(0);
        return;
    }
    const uint8_t* oc = nullptr;
    const uint8_t* nc = nullptr;
    if (b.scale() == kCoarseScale) {
        Canvas c(main_, kCoarseWidth, kCoarseHeight);
        b.render_layers(c, layers_);  // Hintergrund liest compose() direkt aus der Schicht
        nc = main_;
    } else {
        b.render(fine);
    }
    if (a.scale() == kCoarseScale) {
        Canvas c(aux_, kCoarseWidth, kCoarseHeight);
        a.render_layers(c, aux_layers_);
        oc = aux_;
    } else {
        a.render(fine);
    }
    compose(fine, oc, nc);
    // Wetter des Ziels über dem ganzen Bild (Schleier gibt es während der Überblendung nicht)
    if (nc) b.render_fine(fine, layers_, map_to_.data());
}

void Stage::compose(Canvas& fine, const uint8_t* oc, const uint8_t* nc) const {
    const int level = step_ * 256 / ticks_;
    const uint32_t salt = mix(transitions_ * 0x9E3779B1u);
    const bool pattern = trans_ == Transition::Pattern;
    const bool lit = layers_.light || aux_layers_.light;  // sonst der schnelle Weg ohne Licht
    uint8_t* r0 = rows_.data();
    uint8_t* r1 = r0 + kScreenWidth;
    for (int y = 0; y < kCoarseHeight; ++y) {
        const uint8_t* f0 = fine.row(2 * y);
        const uint8_t* f1 = fine.row(2 * y + 1);
        const uint8_t* orow = oc ? oc + y * kCoarseWidth : nullptr;
        const uint8_t* nrow = nc ? nc + y * kCoarseWidth : nullptr;
        // außerhalb der Bereiche mit bewegten Teilen direkt aus dem Hintergrund (spart zwei Kopien je Bild)
        const uint8_t* obase = orow && aux_layers_.base ? aux_layers_.base + y * kCoarseWidth : orow;
        const uint8_t* nbase = nrow && layers_.base ? layers_.base + y * kCoarseWidth : nrow;
        const int ox0 = aux_layers_.base ? aux_layers_.x0[y] : 0, ox1 = aux_layers_.base ? aux_layers_.x1[y] : 0;
        const int nx0 = layers_.base ? layers_.x0[y] : 0, nx1 = layers_.base ? layers_.x1[y] : 0;
        for (int x = 0; x < kCoarseWidth; ++x) {
            const int thr = pattern ? kBayer4[y & 3][x & 3] * 16 + 8
                                    : static_cast<int>(mix(static_cast<uint32_t>(y * kCoarseWidth + x) ^ salt) & 0xFF);
            const bool to_new = thr < level;
            const uint8_t* src = to_new ? (x >= nx0 && x < nx1 ? nrow : nbase) : (x >= ox0 && x < ox1 ? orow : obase);
            const uint8_t* map = to_new ? map_to_.data() : map_from_.data();
            const int fx = 2 * x;
            if (src && !lit) {
                const uint8_t v = map[src[x]];
                r0[fx] = r0[fx + 1] = r1[fx] = r1[fx + 1] = v;
            } else if (src) {
                // mit Licht (abends und nachts)
                const CoarseLayers& L = to_new ? layers_ : aux_layers_;
                const uint8_t s = src[x];
                const uint8_t lp = L.light && s && s < L.light_base ? L.light[y * kCoarseWidth + x] : 0;
                uint8_t a[2] = {s, s}, b[2] = {s, s};
                if (lp) lit4(s, lp, level_offsets(L.light_base).data(), a, b);
                r0[fx] = map[a[0]];
                r0[fx + 1] = map[a[1]];
                r1[fx] = map[b[0]];
                r1[fx + 1] = map[b[1]];
            } else {
                // feine Szene: steht schon im Bild, nur auf die gemeinsame Palette umsetzen
                r0[fx] = map[f0[fx]];
                r0[fx + 1] = map[f0[fx + 1]];
                r1[fx] = map[f1[fx]];
                r1[fx + 1] = map[f1[fx + 1]];
            }
        }
        std::memcpy(fine.row(2 * y), r0, kScreenWidth);
        std::memcpy(fine.row(2 * y + 1), r1, kScreenWidth);
    }
}

}  // namespace pixelia
