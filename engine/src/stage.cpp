#include "pixelia/engine/stage.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "pixelia/engine/light.h"
#include "pixelia/engine/overlay.h"
#include "pixelia/engine/place.h"
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

Stage::Stage() : assets_(&asset_store()) {
    // Ereignis-Bus (seit 0.5.8): Kalender und Ortszustände melden dorthin, Kurzmeldungen liest die Anzeige
    env_.calendar.set_bus(&env_.events);
    env_.states.set_bus(&env_.events);
    env_.figures.set_bus(&env_.events);
    toast_reader_ = env_.events.subscribe(kCalendarEvents);
    // Chronik (seit 0.5.9): Kalender, neue Stufen von Fähigkeiten und Beziehungen
    chron_reader_ = env_.events.subscribe(kCalendarEvents | event_bit(EventKind::SkillUp) | event_bit(EventKind::BondChanged));
    place_of_.fill(-1);
}

Stage::~Stage() {
    for (int i = 0; i < count_; ++i) pin_scene(i, false);
}

int Stage::add(std::unique_ptr<Scene> scene) {
    if (count_ >= kMaxScenes || !scene) return -1;
    scenes_[count_] = std::move(scene);
    scenes_[count_]->set_environment(&env_);
    place_of_[count_] = static_cast<int16_t>(place_index(scenes_[count_]->name()));
    return count_++;
}

void Stage::set_assets(AssetStore* store) {
    for (int i = 0; i < count_; ++i) pin_scene(i, false);
    assets_ = store;
    prefetched_ = -1;
    sync_pins();
}

bool Stage::scene_ready(int i) const {
    if (!assets_ || i < 0 || i >= count_) return true;
    const char* names[8];
    const int n = scenes_[i]->packs(names, 8);
    for (int k = 0; k < n; ++k)
        if (!assets_->ready(names[k])) return false;
    return true;
}

void Stage::pin_scene(int i, bool on) {
    const uint32_t bit = 1u << i;
    if (on == ((pinned_ & bit) != 0)) return;
    pinned_ = on ? pinned_ | bit : pinned_ & ~bit;
    if (!assets_) return;
    const char* names[8];
    const int n = scenes_[i]->packs(names, 8);
    for (int k = 0; k < n; ++k) {
        if (on)
            assets_->pin(names[k]);
        else
            assets_->unpin(names[k]);
    }
}

void Stage::sync_pins() {
    for (int i = 0; i < count_; ++i) pin_scene(i, i == active_ || i == from_ || i == pending_);
}

void Stage::prefetch_neighbors() {
    prefetched_ = active_;
    if (!assets_ || count_ == 0) return;
    const char* near[8];
    const int n = scenes_[active_]->neighbors(near, 8);
    for (int k = 0; k < n; ++k) {
        const int j = find(near[k]);
        if (j < 0 || j == active_) continue;
        const char* names[8];
        const int m = scenes_[j]->packs(names, 8);
        for (int p = 0; p < m; ++p) assets_->want(names[p], false);
    }
}

void Stage::init(uint64_t seed) {
    seed_ = seed;
    env_.weather.init(seed);
    env_.weather_sim.init(seed ^ 0x57E77E5157ULL);
    world_minute_ = INT64_MIN;
    override_key_ = kNoOverride;
    sim_applied_ = false;
    state_minute_ = INT64_MIN;
    env_.world_minute = INT64_MIN;
    env_.events.clear();
    env_.chronicle.clear();
    env_.visits.clear();
    arming_ = -1;
    transit_n_ = 0;
    wake_pending_ = false;
    saved_minute_ = INT64_MIN;
    caught_up_ = skipped_ = 0;
    past_key_ = INT64_MIN;
    grid_ = INT64_MIN;  // Stufen der Simulation (seit 0.5.9 Teil 2): der erste Schritt setzt alle an
    decisions_ = reseats_ = 0;
    last_grid_steps_ = 0;
    for (int i = 0; i < count_; ++i) scenes_[i]->init(seed + static_cast<uint64_t>(i) * 0x9E3779B97F4A7C15ULL);
    populate_figures();  // seit 0.5.9: die Figuren gehören der Welt, die Orte zeigen sie
    adopt_figures();
    if (active_ >= count_) active_ = 0;
    from_ = -1;
    pending_ = -1;
    step_ = 0;
    transitions_ = 0;
    travels_ = 0;
    entered_ = 0;
    hold_ = false;
    holds_ = 0;
    prefetched_ = -1;
    sync_pins();
    sync_entered();
}

void Stage::sync_entered() {
    for (int i = 0; i < count_; ++i) {
        const bool have = entered_ & (1u << i);
        // betreten erst, wenn die Pakete geladen sind (bzw. fehlgeschlagen: dann fehlen nur Bilder)
        const bool want = (i == active_ || i == from_ || i == arming_) && (have || scene_ready(i));
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
    arming_ = -1;
    hold_ = false;
    sync_pins();
    sync_entered();
}

void Stage::go(int index, Transition t) {
    if (index < 0 || index >= count_) return;
    pending_ = index;
    pending_trans_ = t;
    sync_pins();  // Laden beginnt sofort, nicht erst im nächsten Schritt
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
    if (std::strcmp(cmd, "wetter auto") == 0) {
        release_weather();
        return true;
    }
    if (std::sscanf(cmd, "mond %15s", word) == 1) {
        const std::string_view w(word);
        if (w == "a" || w == "b" || w == "c") {
            env_.sky.set_style(w == "a" ? MoonStyle::Halo : w == "b" ? MoonStyle::Plain : MoonStyle::Craters);
        } else if (w == "echt") {
            env_.sky.force(-1);
        } else {
            int phase = 0, up = 80, x = 50;
            if (std::sscanf(cmd, "mond %d %d %d", &phase, &up, &x) < 1) return false;
            env_.sky.force(std::clamp(phase, 0, 100) / 100.0, std::clamp(up, 0, 100) * 256 / 100,
                           std::clamp(x, 0, 100) * 256 / 100);
        }
        world_minute_ = INT64_MIN;  // Himmel sofort neu
        return true;
    }
    if (std::sscanf(cmd, "wetter %15s", word) == 1) {
        WeatherKind k;
        if (!parse_weather(word, &k)) return false;
        // Prüfbefehl: übersteuert die Simulation bis zur nächsten Wetterzeit
        if (world_valid_) world_minute_ = INT64_MIN;
        override_key_ = kPendingOverride;  // Wetterzeit im nächsten Schritt (die Uhr kann eben gestellt sein)
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
    const bool cut = t == Transition::Cut || ticks_ < 2;
    if ((cut || t != Transition::Fade) && !scene_ready(to)) {
        // Schnitt, Überblendung, Raster: erst, wenn das Ziel geladen ist; bis dahin bleibt der alte Ort
        pending_ = to;
        pending_trans_ = t;
        sync_pins();
        return;
    }
    if (!cut && t != Transition::Fade && !entered(to)) {
        // Vorbereitung (seit 0.5.8): erst betreten (Hintergrundbild, Wetter-Vorlauf), im nächsten Schritt die Paletten
        // mischen und beginnen. So fallen die beiden teuren Teile nicht in denselben Schritt
        arming_ = to;
        pending_ = to;
        pending_trans_ = t;
        sync_entered();
        return;
    }
    // Paletten mischen (seit 0.5.8): erst exakt; reichen die 240 Farben nicht (nachts, drinnen ↔ draußen mit Wetter),
    // ähnliche Farben zusammenlegen (für die 0,6 s unsichtbar). Ein Durchgang je Schritt, bis einer passt
    bool fade = false;
    if (!cut && t != Transition::Fade) {
        if (scenes_[active_]->scale() != kCoarseScale && scenes_[to]->scale() != kCoarseScale) {
            fade = true;  // beide fein: über Schwarz
        } else {
            constexpr int kTolerance[] = {0, 12, 30, 60};
            if (merge_to_ != to) {
                merge_to_ = to;
                merge_try_ = 0;
            }
            const int k = std::min(merge_try_, 3);
            if (!merge_palettes(active_, to, kTolerance[k], k == 3)) {
                ++merge_try_;
                arming_ = to;
                pending_ = to;
                pending_trans_ = t;
                return;
            }
        }
    }
    merge_to_ = -1;
    arming_ = -1;
    from_ = -1;  // laufenden Übergang abschließen
    hold_ = false;
    ++transitions_;
    if (cut) {
        active_ = to;
        sync_pins();
        sync_entered();
        return;
    }
    from_ = active_;
    active_ = to;
    step_ = 0;
    trans_ = fade ? Transition::Fade : t;
    sync_pins();
    sync_entered();  // Abblenden: das Ziel wird betreten, sobald es geladen ist
}

void Stage::publish_travel(EventKind kind, int from, int to, const Travel& t) {
    WorldEvent e;
    e.kind = kind;
    e.place = scenes_[to]->name();
    e.from = scenes_[from]->name();
    e.who = t.who;
    e.lead = t.follow;
    e.day = env_.calendar.date().day;
    env_.events.publish(e);
}

// Ein Durchgang mit Toleranz (Summe der Abstände je Kanal); last: ist alles belegt, die nächste Farbe nehmen
bool Stage::merge_palettes(int from, int to, int tolerance, bool last) {
    // Seit 0.5.8 über ein grobes Farbgitter (8 × 8 × 8 Zellen zu je 32 Stufen): gesucht wird nur in den Zellen, die
    // die Toleranz erreichen kann, statt in allen Plätzen (Rechenspitze beim Start einer Überblendung)
    slots_ = 0;
    map_from_.fill(0);
    map_to_.fill(0);
    cell_head_.fill(0xFF);
    const int reach = (tolerance + 31) / 32;
    auto dist = [](const Color& a, const Color& b) { return std::abs(a.r - b.r) + std::abs(a.g - b.g) + std::abs(a.b - b.b); };
    auto add = [&](int si, std::array<uint8_t, 256>& map) {
        const Scene& s = *scenes_[si];
        const Palette& p = s.palette();
        const int n = std::clamp(s.colors(), 1, kSceneColors);
        for (int i = 0; i < n; ++i) {
            const Color c = p[i];
            const int cr = c.r >> 5, cg = c.g >> 5, cb = c.b >> 5;
            int slot = -1, best = 1 << 30;
            for (int r = std::max(0, cr - reach); r <= std::min(7, cr + reach); ++r)
                for (int g = std::max(0, cg - reach); g <= std::min(7, cg + reach); ++g)
                    for (int b = std::max(0, cb - reach); b <= std::min(7, cb + reach); ++b)
                        for (int k = cell_head_[(r * 8 + g) * 8 + b]; k != 0xFF; k = cell_next_[k]) {
                            const int d = dist(palette_[k], c);
                            if (d < best || (d == best && k < slot)) {
                                best = d;
                                slot = k;
                            }
                        }
            if (best > tolerance) {
                if (slots_ < kSceneColors) {
                    slot = slots_++;
                    palette_[slot] = c;
                    slot_scene_[slot] = static_cast<uint8_t>(si);
                    slot_index_[slot] = static_cast<uint8_t>(i);
                    const int cell = (cr * 8 + cg) * 8 + cb;
                    cell_next_[slot] = cell_head_[cell];
                    cell_head_[cell] = static_cast<uint8_t>(slot);
                } else if (!last) {
                    return false;
                } else if (slot < 0) {
                    // alles belegt und nichts in Reichweite: die nächste aller Farben (selten)
                    for (int k = 0; k < slots_; ++k)
                        if (const int d = dist(palette_[k], c); d < best) {
                            best = d;
                            slot = k;
                        }
                }
            }
            map[i] = static_cast<uint8_t>(slot);
        }
        return true;
    };
    return add(from, map_from_) && add(to, map_to_);
}

int Stage::visible() const {
    if (from_ >= 0 && trans_ == Transition::Fade && step_ < ticks_ / 2) return from_;
    return active_;
}

void Stage::update_palette() {
    if (trans_ == Transition::Fade) {
        const int half = ticks_ / 2;
        const int k = hold_         ? 0  // wartet in der dunklen Mitte auf das Ziel
                      : step_ < half ? (half - step_) * 256 / half
                                     : (step_ - half) * 256 / (ticks_ - half);
        const Palette& p = scenes_[visible()]->palette();
        for (int i = 0; i < kSceneColors; ++i) palette_[i] = scaled(p[i], k);
        return;
    }
    // Farbwechsel der Szenen (Tageszeit, Farbrotation) laufen während der Überblendung weiter
    for (int k = 0; k < slots_; ++k) palette_[k] = scenes_[slot_scene_[k]]->palette()[slot_index_[k]];
    for (int k = slots_; k < kSceneColors; ++k) palette_[k] = {0, 0, 0};
}

void Stage::tick(const WorldClock& clock) {
    world_us_ = clock.world_us();
    env_.events.deliver();  // Takt-Grenze: was im letzten Schritt gemeldet wurde, sehen jetzt alle
    read_chronicle();
    if (pending_ >= 0) {
        const int to = pending_;
        pending_ = -1;
        start_transition(to, pending_trans_);  // setzt pending_ wieder, solange das Ziel noch lädt
    }
    // Seit 0.5.9 Teil 2: Tätigkeiten und Übergaben (world_sim.cpp) vor Kalender und Ortszuständen, damit beide Stufen
    // bei jeder Entscheidung dieselbe Umgebung sehen
    advance(world_us_);
    update_world(clock);
    env_.weather.tick();  // vor den Orten: alle sehen im selben Schritt dasselbe Wetter
    // volle Stufe: nur die betretenen Orte zeigen (seit 0.5.9 Teil 2; vorher rechneten alle Orte mit 30 Hz)
    const uint32_t ticked = entered_;
    for (int i = 0; i < count_; ++i)
        if (entered(i)) scenes_[i]->tick(clock);
    const bool was = from_ >= 0;
    if (from_ >= 0) {
        // Abblenden: vor der zweiten Hälfte muss das Ziel betreten sein; sonst in der dunklen Mitte warten
        const bool middle = trans_ == Transition::Fade && step_ + 1 >= ticks_ / 2;
        if (middle && !entered(active_)) sync_entered();
        hold_ = middle && !entered(active_);
        if (hold_)
            ++holds_;
        else if (++step_ >= ticks_)
            from_ = -1;
    }
    if (was && from_ < 0) {
        sync_pins();
        sync_entered();
    }
    if (from_ < 0 && !entered(active_)) sync_entered();  // z. B. nach set_active, sobald geladen
    // eben erst betreten (Mitte des Abblendens, geladen): gleich einmal zeigen, damit Palette und Figuren stimmen
    for (int i = 0; i < count_; ++i)
        if (entered(i) && !(ticked & (1u << i))) scenes_[i]->tick(clock);
    prepare_scenes();
    if (from_ >= 0) update_palette();
    if (from_ < 0 && pending_ < 0 && prefetched_ != active_) prefetch_neighbors();
}

void Stage::prepare_scenes() {
    // Seit 0.5.9 Teil 3: teure Darstellung (Lichtkarten) mit festem Budget je Schritt statt in einem Stück (bis 31 ms
    // nach dem Start, nach Uhrsprüngen, beim ersten Betreten). Sichtbares zuerst, dann ein unsichtbarer Ort nach dem
    // anderen, damit jeder bald fertig ist
    int budget = kPrepareBudget;
    for (int i = 0; i < count_ && budget > 0; ++i)
        if (entered(i)) budget -= scenes_[i]->prepare(budget);
    for (int k = 0; k < count_ && budget > 0; ++k) {
        const int i = (prepare_next_ + k) % count_;
        if (entered(i)) continue;
        if (const int used = scenes_[i]->prepare(budget)) {
            budget -= used;
            prepare_next_ = i;
        }
    }
    last_prepare_ = kPrepareBudget - budget;
}

int Stage::find_figure(uint16_t id) const {
    if (id == 0) return -1;
    // seit 0.5.9 aus dem Weltzustand; Statisten kennt nur ihr Ort
    if (const FigureRecord* f = env_.figures.find(id))
        return f->where.kind == Whereabouts::Kind::Place ? scene_of(f->where.place) : -1;
    for (int i = 0; i < count_; ++i)
        if (scenes_[i]->has_figure(id)) return i;
    return -1;
}

uint16_t Stage::lead() const {
    if (env_.figures.count() > 0) return env_.figures.lead();
    // Orte nennen die Hauptfigur zuerst (Scene::figures)
    FigureInfo f[1];
    for (int i = 0; i < count_; ++i)
        if (scenes_[i]->figures(f, 1) > 0 && f[0].lead) return f[0].id;
    return 0;
}

int Stage::figures(FigureInfo* out, int* place, int max) const {
    int n = 0;
    for (int i = 0; i < count_ && n < max; ++i) {
        const int got = scenes_[i]->figures(out + n, max - n);
        if (place)
            for (int k = 0; k < got; ++k) place[n + k] = i;
        n += got;
    }
    return n;
}

Hit Stage::hit(int x, int y) const {
    if (from_ >= 0 || !entered(active_)) return {};
    return scenes_[active_]->hit(x, y);
}

bool Stage::describe(const Hit& hit, ui::Card* card) const {
    if (hit.kind == Hit::Kind::None || !scenes_[active_]->describe(hit, card)) return false;
    if (card->place) {
        // Tür: Ziel statt des eigenen Ortes zeigen (wohin sie führt), darunter, wer dort ist
        const int to = find(card->place);
        if (to >= 0) {
            card->where = std::string("→ ") + scenes_[to]->title();
            FigureInfo f[8];
            const int n = scenes_[to]->figures(f, 8);
            std::string who = n ? "Drin: " : "Niemand drin";
            for (int k = 0; k < n && k < 4; ++k) who += std::string(k ? ", " : "") + f[k].name;
            if (n > 4) who += " …";
            card->add_line(who);
        }
    }
    return true;
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

void Stage::draw_scene(const Scene& s, Canvas& fine, bool transition) const {
    SkyView view;
    // Mond (seit 0.5.7) über dem Himmel bzw. Fensterglas, hinter den Partikeln. Seit 0.5.8 auch beim Abblenden, wenn
    // der Ort eigene Mondfarben hat (die Farben der Oberfläche würden nicht mit abblenden)
    auto moon = [&] {
        if (env_.sky.moon_visible() && s.sky_view(&view) && (!transition || view.moon >= 0)) env_.sky.draw(fine, view);
    };
    if (s.scale() != kCoarseScale) {
        s.render(fine);
        moon();
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
    moon();
    s.render_fine(fine, layers_, nullptr);
}

bool Stage::shows_new(int x, int y) const {
    // wie compose(): Schwelle je grobem Bildpunkt (zufällig bzw. Raster) gegen den Fortschritt
    const int level = step_ * 256 / ticks_;
    const int thr = trans_ == Transition::Pattern
                        ? kBayer4[y & 3][x & 3] * 16 + 8
                        : static_cast<int>(mix(static_cast<uint32_t>(y * kCoarseWidth + x) ^
                                               mix(transitions_ * 0x9E3779B1u)) &
                                           0xFF);
    return thr < level;
}

namespace {
struct OwnsCtx {
    const Stage* stage;
    bool is_new;
};
}  // namespace

bool Stage::owns_pixel(const void* ctx, int x, int y) {
    const auto* c = static_cast<const OwnsCtx*>(ctx);
    return c->stage->shows_new(x >> 1, y >> 1) == c->is_new;
}

void Stage::draw_moon(const Scene& s, Canvas& fine, bool is_new) const {
    // Überblendung (seit 0.5.8): Mond je Ort nur in dessen Bildpunkten, in den Farben der gemeinsamen Palette
    SkyView view;
    if (!env_.sky.moon_visible() || !s.sky_view(&view) || view.moon < 0) return;
    const OwnsCtx ctx{this, is_new};
    env_.sky.draw(fine, view, is_new ? map_to_.data() : map_from_.data(), &Stage::owns_pixel, &ctx);
}

void Stage::release_weather() {
    override_key_ = kNoOverride;
    sim_applied_ = false;
    world_minute_ = INT64_MIN;  // im nächsten Schritt neu
}

void Stage::update_world(const WorldClock& clock) {
    if (!world_valid_) return;
    const int64_t us = clock.world_us();
    const int64_t day = us >= 0 ? us / WorldClock::kUsPerDay : (us - WorldClock::kUsPerDay + 1) / WorldClock::kUsPerDay;
    const int minute = clock.minute_of_day();
    const int64_t wm = day * 1440 + minute;
    if (wm == world_minute_) {
        if (state_minute_ != INT64_MIN && state_minute_ < wm) step_states(wm);  // Nachholen läuft weiter
        return;
    }
    const bool first = !sim_applied_ && world_minute_ == INT64_MIN && override_key_ == kNoOverride;
    world_minute_ = wm;
    WorldCalendar& cal = env_.calendar;
    if (!cal.started()) {
        cal.start(day);
        ChronicleEntry c;
        c.kind = ChronicleKind::Start;
        c.day = 1;
        c.minute = static_cast<uint16_t>(minute);
        std::snprintf(c.text, sizeof(c.text), "Die Welt beginnt: Tag 1");
        env_.chronicle.add(c);
    }
    cal.update(day, minute);
    if (wake_pending_) note_wake();
    const int64_t key = weather_slot_key(day, minute);
    if (override_key_ == kPendingOverride) override_key_ = key;  // vor dem ersten Schritt übersteuert
    if (override_key_ != kNoOverride && key != override_key_) {
        override_key_ = kNoOverride;  // neue Wetterzeit: die Simulation übernimmt wieder (weich)
        sim_applied_ = false;
    }
    if (override_key_ == kNoOverride) {
        const WeatherPick p = env_.weather_sim.at(cal, day, minute);
        if (!sim_applied_ || p.kind != sim_pick_.kind || p.strength != sim_pick_.strength) {
            if (first)
                env_.weather.set_now(p.kind, p.strength);  // nach dem Start gleich das Wetter von jetzt
            else
                env_.weather.set(p.kind, p.strength);
        }
        if (!sim_applied_ || p.wind_dir != sim_pick_.wind_dir) env_.weather.set_wind_dir(p.wind_dir);
        sim_pick_ = p;
        sim_applied_ = true;
        // Erstlinge aus dem simulierten Wetter, sobald es zu sehen ist
        const WeatherState& s = env_.weather.state();
        const Season season = cal.date().season;  // das Weltjahr beginnt im Frühling: Schnee zählt ab dem Herbst
        if (p.kind == WeatherKind::Snow && s.snow > 80 && (season == Season::Autumn || season == Season::Winter))
            cal.first(First::Snow);
        if (p.kind == WeatherKind::Storm && s.storm > 100) cal.first(First::Storm);
        if (p.kind == WeatherKind::Fog && s.fog > 100 && season == Season::Autumn) cal.first(First::AutumnFog);
    }
    env_.sky.update(cal.date(), clock.unix_s(), env_.weather.state());
    const MoonInfo& m = env_.sky.moon();
    if (m.illum >= 985 && m.up > 0 && env_.sky.dark() >= 256) cal.first(First::FullMoon);
    env_.world_minute = wm;
    step_states(wm);
}

void Stage::step_states(int64_t wm) {
    // Ortszustände (seit 0.5.8): Minute für Minute bis jetzt; beim Start und wenn die Uhr zurückspringt nur die jetzige
    // Minute. Seit 0.5.9 Nachholen nach dem Laden bzw. einem Sprung: bis kCatchUpMinutes (3 Tage), je Schritt höchstens
    // kCatchUpPerTick Minuten (gleiche Wirkung, verteilte Rechenzeit), Regen der verpassten Zeit aus der Wetter-Simulation
    if (state_minute_ == INT64_MIN || wm < state_minute_) state_minute_ = wm - 1;
    if (wm - state_minute_ > kCatchUpMinutes) {
        skipped_ += wm - state_minute_ - kCatchUpMinutes;
        state_minute_ = wm - kCatchUpMinutes;
    }
    StateInput in;
    in.sun = env_.sky.sun();
    const WeatherState& w = env_.weather.state();
    in.rain = std::max<int>(w.rain, w.storm);
    // Trocknen: Sonne und Wind, bei Wolken und Nebel langsamer; im Sommer schneller, im Winter langsamer
    int dry = 96 - w.cloud * 56 / 255 + w.wind * 32 / 255;
    dry = dry * (255 - w.fog * 3 / 4) / 255;
    const Season season = env_.calendar.date().season;
    if (season == Season::Summer) dry = dry * 3 / 2;
    if (season == Season::Winter) dry /= 2;
    in.dry_day = std::max(4, dry);
    in.dry_night = std::max(2, dry / 6);
    // Hauptfigur: in welchem Haushalt (Kerze für Heimkehrer); unterwegs auf einer Verbindung = nicht daheim
    const uint16_t who = lead();
    const int at = find_figure(who);
    if (const FigureRecord* f = env_.figures.find(who); f && f->where.kind != Whereabouts::Kind::Place) {
        in.lead_known = true;  // unterwegs auf einer Verbindung: nicht daheim
    } else if (at >= 0) {
        in.lead_known = true;
        const PlaceDef* d = find_place(scenes_[at]->name());
        in.lead_house = d ? d->house : nullptr;
    } else {
        for (int k = 0; k < transit_n_; ++k) in.lead_known = in.lead_known || transit_[k].t.follow;
    }
    env_.states.set_day(env_.calendar.date().day);
    const int64_t until = std::min(wm, state_minute_ + kCatchUpPerTick);
    for (int64_t m = state_minute_ + 1; m <= until; ++m) {
        const int64_t d = m >= 0 ? m / 1440 : (m - 1439) / 1440;
        const int mod = static_cast<int>(m - d * 1440);
        if (wm - m > 30) {
            // verpasste Zeit: Regen nach der Wetter-Simulation dieser Wetterzeit (einmal je Wetterzeit gerechnet)
            const int64_t key = weather_slot_key(d, mod);
            if (key != past_key_) {
                past_key_ = key;
                const WeatherPick p = env_.weather_sim.at(env_.calendar, d, mod);
                past_rain_ = p.kind == WeatherKind::Rain || p.kind == WeatherKind::Storm ? p.strength : 0;
            }
            StateInput past = in;
            past.rain = past_rain_;
            env_.states.step(mod, past);
            ++caught_up_;
        } else {
            env_.states.step(mod, in);
        }
    }
    state_minute_ = until;
}

void Stage::render(Canvas& fine) const {
    if (count_ == 0) {
        fine.clear(0);
        return;
    }
    if (from_ < 0 || trans_ == Transition::Fade) {
        const int v = visible();
        if (!entered(v)) {  // Pakete noch nicht geladen
            fine.clear(0);
            return;
        }
        const Scene& s = *scenes_[v];
        draw_scene(s, fine, from_ >= 0);
        if (from_ < 0) s.render_overlay(fine);
        return;
    }
    // Überblendung: höchstens eine der beiden Szenen ist fein (sonst Abblenden), sie zeichnet direkt ins Bild
    const Scene& a = *scenes_[from_];
    const Scene& b = *scenes_[active_];
    if (!entered(from_) || !entered(active_)) {  // Wechsel mitten in einem wartenden Abblenden
        if (entered(active_))
            draw_scene(b, fine, true);
        else
            fine.clear(0);
        return;
    }
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
    draw_moon(a, fine, false);
    draw_moon(b, fine, true);
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
