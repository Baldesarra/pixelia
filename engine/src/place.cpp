#include "pixelia/engine/place.h"

#include <cstdio>
#include <cstring>
#include <new>

#include "pixelia/engine/memory.h"

namespace pixelia {

const PlaceDef* find_place(std::string_view id) {
    const AreaDef* const* areas = nullptr;
    const int n = world_areas(&areas);
    for (int a = 0; a < n; ++a)
        for (int i = 0; i < areas[a]->count; ++i)
            if (id == areas[a]->places[i]->id) return areas[a]->places[i];
    return nullptr;
}

int place_index(std::string_view id) {
    const AreaDef* const* areas = nullptr;
    const int n = world_areas(&areas);
    int k = 0;
    for (int a = 0; a < n; ++a)
        for (int i = 0; i < areas[a]->count; ++i, ++k)
            if (id == areas[a]->places[i]->id) return k;
    return -1;
}

const PlaceDef* place_at(int index) {
    const AreaDef* const* areas = nullptr;
    const int n = world_areas(&areas);
    for (int a = 0; a < n && index >= 0; index -= areas[a]->count, ++a)
        if (index < areas[a]->count) return areas[a]->places[index];
    return nullptr;
}

std::unique_ptr<Scene> make_place(std::string_view id) {
    const AreaDef* const* areas = nullptr;
    const int n = world_areas(&areas);
    int k = 0;
    for (int a = 0; a < n; ++a)
        for (int i = 0; i < areas[a]->count; ++i, ++k)
            if (id == areas[a]->places[i]->id) return areas[a]->make(*areas[a]->places[i], k);
    return nullptr;
}

const char* exit_kind_title(ExitKind k) {
    switch (k) {
        case ExitKind::Door: return "Tür";
        case ExitKind::Edge: return "Weg";
        case ExitKind::Path: return "Feldweg";
        case ExitKind::Stairs: return "Treppe";
        case ExitKind::Station: return "Bahnhof";
        case ExitKind::Mine: return "Stolleneingang";
    }
    return "Weg";
}

bool exit_open(const Exit& e, std::string_view here, const Environment& env, const char** why) {
    const char* reason = nullptr;
    // Fahrplan: Halt ist der Ort mit dem Ausgang, im Fahrzeug selbst das Ziel des Ausgangs
    const std::string_view stop = e.needs && here == e.needs ? std::string_view(e.to ? e.to : "") : here;
    if ((e.when & kOpenDayOnly) && env.sky.dark() >= 160) reason = "Nur bei Tag begehbar.";
    else if ((e.when & kOpenAfterBuild) && !(e.needs && env.flags.has(e.needs))) reason = "Erst nach dem Bau begehbar.";
    else if ((e.when & kOpenWithTool) && !(e.needs && env.flags.has(e.needs))) reason = "Nur mit Werkzeug.";
    else if ((e.when & kOpenTimetable) && !(e.needs && env.vehicles.stands_at(e.needs, stop, env.world_minute)))
        reason = "Fährt nur nach Fahrplan.";
    if (why) *why = reason;
    return reason == nullptr;
}

void PlaceGraph::build() {
    std::array<const PlaceDef*, kMaxPlaces> list{};
    int n = 0;
    const AreaDef* const* areas = nullptr;
    const int na = world_areas(&areas);
    for (int a = 0; a < na; ++a)
        for (int i = 0; i < areas[a]->count && n < kMaxPlaces; ++i) list[n++] = areas[a]->places[i];
    build(list.data(), n);
}

void PlaceGraph::build(const PlaceDef* const* places, int count) {
    n_ = e_ = 0;
    for (int i = 0; i < count && n_ < kMaxPlaces; ++i) defs_[n_++] = places[i];
    for (int p = 0; p < n_; ++p) {
        const RoomDef* r = defs_[p]->room;
        for (int x = 0; r && x < r->exit_count && e_ < kMaxEdges; ++x) {
            const Exit& ex = r->exits[x];
            Edge& e = edges_[e_++];
            e = Edge{static_cast<uint8_t>(p), 0xFF, static_cast<uint8_t>(x), ex.entry, ex.kind, ex.travel, ex.when};
            for (int q = 0; q < n_; ++q)
                if (ex.to && std::strcmp(ex.to, defs_[q]->id) == 0) e.to = static_cast<uint8_t>(q);
        }
    }
    build_hops();
}

const char* PlaceGraph::check(char* buf, int n) const {
    buf[0] = 0;
    for (int i = 0; i < e_; ++i) {
        const Edge& e = edges_[i];
        const PlaceDef& from = *defs_[e.from];
        const Exit& ex = from.room->exits[e.exit];
        if (e.to == 0xFF) {
            std::snprintf(buf, n, "%s: Ausgang %d führt nach %s, den Ort gibt es nicht", from.id, e.exit,
                          ex.to ? ex.to : "(nichts)");
            return buf;
        }
        const PlaceDef& to = *defs_[e.to];
        if (!to.room || e.entry >= to.room->exit_count) {
            std::snprintf(buf, n, "%s: Ausgang %d kommt in %s an Ausgang %d an, den es nicht gibt", from.id, e.exit,
                          to.id, e.entry);
            return buf;
        }
        // Rückweg: der Ausgang, an dem man ankommt, führt zurück, und zwar an den Ausgang, durch den man kam
        const Exit& back = to.room->exits[e.entry];
        if (!back.to || std::strcmp(back.to, from.id) != 0 || back.entry != e.exit) {
            std::snprintf(buf, n, "%s → %s: kein Rückweg (Ausgang %d dort führt nach %s)", from.id, to.id, e.entry,
                          back.to ? back.to : "(nichts)");
            return buf;
        }
        if (back.kind != ex.kind || back.travel != ex.travel) {
            std::snprintf(buf, n, "%s ↔ %s: beide Seiten brauchen dieselbe Art und Reisezeit", from.id, to.id);
            return buf;
        }
    }
    for (int a = 0; a < n_; ++a)
        for (int b = 0; b < n_; ++b)
            if (a != b && hops(a, b) < 0) {
                std::snprintf(buf, n, "von %s ist %s nicht erreichbar", defs_[a]->id, defs_[b]->id);
                return buf;
            }
    return buf;
}

void PlaceGraph::build_hops() {
    // Breitensuche von jedem Ort aus
    for (int a = 0; a < kMaxPlaces; ++a) hops_[a].fill(-1);
    for (int a = 0; a < n_; ++a) {
        std::array<int8_t, kMaxPlaces>& dist = hops_[a];
        std::array<uint8_t, kMaxPlaces> queue{};
        int head = 0, tail = 0;
        dist[a] = 0;
        queue[tail++] = static_cast<uint8_t>(a);
        while (head < tail) {
            const int c = queue[head++];
            for (int i = 0; i < e_; ++i) {
                const Edge& e = edges_[i];
                if (e.from != c || e.to == 0xFF || dist[e.to] >= 0) continue;
                dist[e.to] = static_cast<int8_t>(dist[c] + 1);
                queue[tail++] = e.to;
            }
        }
    }
}

int PlaceGraph::hops(int a, int b) const {
    if (a < 0 || b < 0 || a >= n_ || b >= n_) return -1;
    return hops_[a][b];
}

const PlaceGraph& world_graph() {
    static PlaceGraph* g = [] {
        void* p = asset_alloc(sizeof(PlaceGraph));
        PlaceGraph* x = p ? new (p) PlaceGraph() : new PlaceGraph();
        x->build();
        return x;
    }();
    return *g;
}

}  // namespace pixelia
