#include "pixelia/engine/stage.h"

#include <algorithm>
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

// Lichtstufen der vier feinen Bildpunkte eines groben (Grundfarbe s, Lichtwert v), in a0 a1 / b0 b1
inline void lit4(uint8_t s, int v, int base, LightStyle style, int fx, int fy, uint8_t* a, uint8_t* b) {
    a[0] = static_cast<uint8_t>(s + light_level(v, style, fx, fy) * base);
    a[1] = static_cast<uint8_t>(s + light_level(v, style, fx + 1, fy) * base);
    b[0] = static_cast<uint8_t>(s + light_level(v, style, fx, fy + 1) * base);
    b[1] = static_cast<uint8_t>(s + light_level(v, style, fx + 1, fy + 1) * base);
}

// Licht im Bereich [x0, x1) der groben Zeile y auf die beiden schon verdoppelten feinen Zeilen a und b
void light_span(uint8_t* a, uint8_t* b, int y, int x0, int x1, const CoarseLayers& L) {
    const uint8_t* m = L.light + y * kCoarseWidth;
    const int base = L.light_base;
    const auto style = static_cast<LightStyle>(L.light_style);
    for (int x = x0; x < x1; ++x) {
        const int fx = 2 * x;
        const uint8_t s = a[fx];
        if (m[x] && s && s < base) lit4(s, m[x], base, style, fx, 2 * y, a + fx, b + fx);
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
        // Licht (light.h) erst hier: so kann das Raster fein sein (2 × 2 Displaypunkte)
        if (light && layers->lx1[y] > layers->lx0[y]) {
            std::memcpy(row2, row, kScreenWidth);
            light_span(row, row2, y, layers->lx0[y], layers->lx1[y], *layers);
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
    return count_++;
}

void Stage::init(uint64_t seed) {
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
    slots_ = 0;
    map_from_.fill(0);
    map_to_.fill(0);
    auto add = [&](int si, std::array<uint8_t, 256>& map) {
        const Scene& s = *scenes_[si];
        const Palette& p = s.palette();
        const int n = std::clamp(s.colors(), 1, kSceneColors);
        for (int i = 0; i < n; ++i) {
            const Color c = p[i];
            int slot = -1;
            for (int k = 0; k < slots_; ++k) {
                const Color m = palette_[k];
                if (m.r == c.r && m.g == c.g && m.b == c.b) {
                    slot = k;
                    break;
                }
            }
            if (slot < 0) {
                if (slots_ == kSceneColors) return false;
                slot = slots_++;
                palette_[slot] = c;
                slot_scene_[slot] = static_cast<uint8_t>(si);
                slot_index_[slot] = static_cast<uint8_t>(i);
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
    s.render_layers(coarse, layers_);
    upscale2x(main_, fine, &layers_);
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
        b.render_layers(c, layers_);
        flatten_layers(main_, layers_);
        nc = main_;
    } else {
        b.render(fine);
    }
    if (a.scale() == kCoarseScale) {
        Canvas c(aux_, kCoarseWidth, kCoarseHeight);
        a.render_layers(c, aux_layers_);
        flatten_layers(aux_, aux_layers_);
        oc = aux_;
    } else {
        a.render(fine);
    }
    compose(fine, oc, nc);
}

void Stage::compose(Canvas& fine, const uint8_t* oc, const uint8_t* nc) const {
    const int level = step_ * 256 / ticks_;
    const uint32_t salt = mix(transitions_ * 0x9E3779B1u);
    const bool pattern = trans_ == Transition::Pattern;
    uint8_t* r0 = rows_.data();
    uint8_t* r1 = r0 + kScreenWidth;
    for (int y = 0; y < kCoarseHeight; ++y) {
        const uint8_t* f0 = fine.row(2 * y);
        const uint8_t* f1 = fine.row(2 * y + 1);
        const uint8_t* orow = oc ? oc + y * kCoarseWidth : nullptr;
        const uint8_t* nrow = nc ? nc + y * kCoarseWidth : nullptr;
        for (int x = 0; x < kCoarseWidth; ++x) {
            const int thr = pattern ? kBayer4[y & 3][x & 3] * 16 + 8
                                    : static_cast<int>(mix(static_cast<uint32_t>(y * kCoarseWidth + x) ^ salt) & 0xFF);
            const bool to_new = thr < level;
            const uint8_t* src = to_new ? nrow : orow;
            const uint8_t* map = to_new ? map_to_.data() : map_from_.data();
            const int fx = 2 * x;
            const CoarseLayers& L = to_new ? layers_ : aux_layers_;
            const uint8_t s = src ? src[x] : 0;
            const int lv = src && L.light && s && s < L.light_base ? L.light[y * kCoarseWidth + x] : 0;
            if (lv) {
                uint8_t a[2], b[2];
                lit4(s, lv, L.light_base, static_cast<LightStyle>(L.light_style), fx, 2 * y, a, b);
                r0[fx] = map[a[0]];
                r0[fx + 1] = map[a[1]];
                r1[fx] = map[b[0]];
                r1[fx + 1] = map[b[1]];
            } else if (src) {
                const uint8_t v = map[s];
                r0[fx] = r0[fx + 1] = r1[fx] = r1[fx + 1] = v;
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
