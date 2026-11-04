// Prüfungen zu 0.5.11 „Fester Boden“ (--test boden): die Befunde der Code-Prüfungen vom 31.10. und 1.11.2026 (andere
// KI, Berichte außerhalb des Repos) als normale Tests mit der richtigen Erwartung. Die Gegenproben der Prüfung zeigten
// den Fehler; hier muss er weg sein. Teil a: Simulation und Warenbuch (V01–V05, B12–B15, B17, B18, P-F). Teil b:
// Speichern und Protokoll (B01, B02, B04, B08, B09, Tagesaufnahme).

#include <algorithm>
#include <array>
#include <thread>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <string>
#include <vector>

#include "pixelia/engine/assets.h"
#include "pixelia/engine/econ_log.h"
#include "pixelia/engine/events.h"
#include "pixelia/engine/figure.h"
#include "pixelia/engine/ledger.h"
#include "pixelia/engine/place.h"
#include "pixelia/engine/png.h"
#include "pixelia/engine/projects.h"
#include "pixelia/engine/save.h"
#include "pixelia/engine/stage.h"
#include "pixelia/engine/tasks.h"
#include "pixelia/engine/world_clock.h"
#include "sim.h"

namespace sim {
namespace {

using pixelia::Booking;
using pixelia::EconomyDef;
using pixelia::EventBus;
using pixelia::GoodDef;
using pixelia::Ledger;
using pixelia::SaveReader;
using pixelia::SaveWriter;
using pixelia::StoreDef;
using pixelia::StoreKind;
using pixelia::SupplyWait;
using pixelia::Task;
using pixelia::TaskKind;
using pixelia::TaskList;
using pixelia::WorldEvent;

struct Case {
    const char* name;
    std::function<std::string()> run;
};

std::string checked(const Ledger& l) {
    char buf[200];
    const char* e = l.check(buf, sizeof(buf));
    return e ? std::string(e) : std::string();
}

pixelia::BookingInfo at(int64_t us) {
    pixelia::BookingInfo i;
    i.at_us = us;
    i.reason = "test";
    return i;
}

// Bühne mit dem Testdorf (wie in ledger_tests.cpp)
struct Run {
    pixelia::Stage stage;
    pixelia::WorldClock clock;
    explicit Run(uint64_t seed, const char* start = "testort-raum", int hour = 16, int minute = 0, double speed = 1.0) {
        Options opt;
        opt.scene = start;
        opt.seed = seed;
        setup_stage(opt, stage);
        stage.set_world_time(true);
        stage.set_follow(false);
        clock.set_time_of_day(hour, minute);
        clock.set_speed(speed);
        clock.advance(0);
        stage.tick(clock);
    }
    void run(int ticks) {
        for (int i = 0; i < ticks; ++i) {
            clock.advance(pixelia::kTickUs);
            stage.tick(clock);
        }
    }
    pixelia::Environment& env() { return stage.environment(); }
    std::vector<uint8_t> save(uint32_t gen = 1) const {
        std::vector<uint8_t> b(96 * 1024);
        b.resize(stage.save_state(b.data(), b.size(), gen));
        return b;
    }
};

// eigene Wirtschaft mit 96 Waren und fünf Lagern (wie die Gegenproben der Prüfung)
struct Wide {
    std::array<std::string, Ledger::kMaxGoods> names;
    std::array<GoodDef, Ledger::kMaxGoods> goods{};
    std::array<StoreDef, 5> stores{};
    EconomyDef def;
    Ledger ledger;
    explicit Wide(bool unique = false, uint16_t cap = 1000, int good_count = Ledger::kMaxGoods) {
        for (int i = 0; i < good_count; ++i) {
            names[i] = "w" + std::to_string(i);
            goods[i] = GoodDef{names[i].c_str(), names[i].c_str()};
        }
        goods[0].unique = unique;
        static const char* const kIds[] = {"s0", "s1", "s2", "s3", "s4"};
        for (int s = 0; s < 5; ++s) stores[s] = StoreDef{kIds[s], kIds[s], "testort-wiese", StoreKind::Community, cap};
        def.goods = goods.data();
        def.good_count = static_cast<uint8_t>(good_count);
        def.stores = stores.data();
        def.store_count = 5;
        const EconomyDef* defs[] = {&def};
        ledger.build(defs, 1);
    }
};

// Warenbuch als Speicherstand-Abschnitte; resv ersetzt den Abschnitt RESV (eigene, auch fehlerhafte Daten)
std::vector<uint8_t> ledger_bytes(const Ledger& l, const std::function<void(SaveWriter&)>& resv = nullptr) {
    std::vector<uint8_t> b(96 * 1024);
    SaveWriter w(b.data(), b.size());
    w.begin(1, 0);
    w.chunk("WARE", 1);
    l.save_goods(w);
    w.chunk("LAGR", 1);
    l.save_stock(w);
    w.chunk("STCK", 1);
    l.save_items(w);
    w.chunk("RESV", 1);
    if (resv)
        resv(w);
    else
        l.save_reservations(w);
    b.resize(w.finish());
    return b;
}

Ledger::LoadStats load_into(Ledger& l, const std::vector<uint8_t>& b) {
    l.clear();
    Ledger::LoadStats st;
    pixelia::SaveChunks c(b.data(), b.size());
    char tag[5];
    uint16_t v = 0;
    SaveReader d(nullptr, 0);
    while (c.next(tag, &v, &d)) {
        if (std::strcmp(tag, "WARE") == 0) l.load_goods(d, v, &st);
        if (std::strcmp(tag, "LAGR") == 0) l.load_stock(d, v, &st);
        if (std::strcmp(tag, "STCK") == 0) l.load_items(d, v, &st);
        if (std::strcmp(tag, "RESV") == 0) l.load_reservations(d, v, &st);
    }
    l.finish_load(&st);
    return st;
}

// Dorf mit den Orten des Testdorfs (Wege: Raum–Wiese, Raum–Kammer, Wiese–Platz) und eigenen Lagern
struct Village {
    GoodDef goods[2] = {{"holz", "Holz"}, {"brot", "Brot"}};
    pixelia::StockRule rule[1] = {{"holz", 4, 10}};
    StoreDef stores[7] = {
        {"haus", "Haus", "testort-raum", StoreKind::House, 20, -1, rule, 1},
        {"k_platz", "Kiste Platz", "testort-platz", StoreKind::Community, 100},
        {"k_wiese", "Kiste Wiese", "testort-wiese", StoreKind::Community, 100},
        {"k_kammer", "Kiste Kammer", "testort-kammer", StoreKind::Community, 100},
        {"e_wiese", "Holzplatz", "testort-wiese", StoreKind::Producer, 100},
        {"haus2", "Haus 2", "testort-raum", StoreKind::House, 20, -1, rule, 1, "k_platz"},  // Quelle eingetragen
        {"site", "Baustelle", "testort-kammer", StoreKind::Site, 100},
    };
    pixelia::ProjectNeed needs[1] = {{"holz", 6}};
    pixelia::ProjectStepDef steps[1] = {{"stapeln", "Holz stapeln", "Stapeln", needs, 1, pixelia::Skill::Joinery,
                                         pixelia::SkillLevel::Unknown, 10}};
    pixelia::ProjectDef projects[1] = {{"stapel", "Stapel bauen", pixelia::ProjectKind::Furniture, "testort-kammer",
                                        "site", nullptr, -1, steps, 1, false, true}};
    EconomyDef def{goods, 2, stores, 7, nullptr, 0, projects, 1};
    Ledger l;
    pixelia::FigureBook book;
    TaskList tasks;
    int64_t now = 0;
    Village() {
        const EconomyDef* defs[] = {&def};
        l.build(defs, 1);
    }
    int store(const char* id) const { return l.find_store(id); }
    void put(const char* id, int n) { l.add(store(id), l.find_good("holz"), n, Booking::Produce, at(now)); }
    void step() {
        now += pixelia::Stage::kGridUs;
        tasks.step(now, 0, l, book, nullptr);
    }
    // Quelle der Holen-Aufgabe zu diesem Lager, −1 = keine
    int fetch_from(const char* to) const {
        for (int i = 0; i < tasks.count(); ++i)
            if (tasks.at(i).kind == TaskKind::Fetch && tasks.at(i).to == store(to)) return tasks.at(i).from;
        return -1;
    }
    std::string name(int s) const { return s >= 0 ? l.store_def(s)->id : "keine"; }
};

// vier Orte für die Wege (B15): A–B direkt über eine Brücke (erst nach dem Bau), Umweg A–C–D–B mit einem Steg in C–D
using pixelia::ExitKind;
using pixelia::Facing;
using pixelia::Transition;
constexpr pixelia::Exit kRA[] = {
    {24, 9, 1, 2, 23, 9, Facing::Left, ExitKind::Path, "b15-b", 0, Transition::Fade, 0, pixelia::kOpenAfterBuild, "bruecke"},
    {0, 9, 1, 2, 1, 9, Facing::Right, ExitKind::Path, "b15-c", 0, Transition::Fade}};
constexpr pixelia::Exit kRB[] = {
    {0, 9, 1, 2, 1, 9, Facing::Right, ExitKind::Path, "b15-a", 0, Transition::Fade, 0, pixelia::kOpenAfterBuild, "bruecke"},
    {24, 9, 1, 2, 23, 9, Facing::Left, ExitKind::Path, "b15-d", 1, Transition::Fade}};
constexpr pixelia::Exit kRC[] = {
    {24, 9, 1, 2, 23, 9, Facing::Left, ExitKind::Path, "b15-a", 1, Transition::Fade},
    {0, 9, 1, 2, 1, 9, Facing::Right, ExitKind::Path, "b15-d", 0, Transition::Fade, 0, pixelia::kOpenAfterBuild, "steg"}};
constexpr pixelia::Exit kRD[] = {
    {24, 9, 1, 2, 23, 9, Facing::Left, ExitKind::Path, "b15-c", 1, Transition::Fade, 0, pixelia::kOpenAfterBuild, "steg"},
    {0, 9, 1, 2, 1, 9, Facing::Right, ExitKind::Path, "b15-b", 1, Transition::Fade}};
constexpr pixelia::RoomDef kRoomA = {{nullptr, nullptr, nullptr}, "", {0, 0, 0}, nullptr, 0, kRA, 2};
constexpr pixelia::RoomDef kRoomB = {{nullptr, nullptr, nullptr}, "", {0, 0, 0}, nullptr, 0, kRB, 2};
constexpr pixelia::RoomDef kRoomC = {{nullptr, nullptr, nullptr}, "", {0, 0, 0}, nullptr, 0, kRC, 2};
constexpr pixelia::RoomDef kRoomD = {{nullptr, nullptr, nullptr}, "", {0, 0, 0}, nullptr, 0, kRD, 2};
const pixelia::PlaceDef kPA = {"b15-a", "A", pixelia::kPlaceOutdoor, &kRoomA, nullptr, 0, false,
                               {256, 256, 256}, 256, {0, 0, 0}, pixelia::GlassRole::None, nullptr, 0, nullptr, 0};
const pixelia::PlaceDef kPB = {"b15-b", "B", pixelia::kPlaceOutdoor, &kRoomB, nullptr, 0, false,
                               {256, 256, 256}, 256, {0, 0, 0}, pixelia::GlassRole::None, nullptr, 0, nullptr, 0};
const pixelia::PlaceDef kPC = {"b15-c", "C", pixelia::kPlaceOutdoor, &kRoomC, nullptr, 0, false,
                               {256, 256, 256}, 256, {0, 0, 0}, pixelia::GlassRole::None, nullptr, 0, nullptr, 0};
const pixelia::PlaceDef kPD = {"b15-d", "D", pixelia::kPlaceOutdoor, &kRoomD, nullptr, 0, false,
                               {256, 256, 256}, 256, {0, 0, 0}, pixelia::GlassRole::None, nullptr, 0, nullptr, 0};

// Bestände nach Kennung von Lagerplatz und Ware (Tragen entstehen nach dem Laden in anderer Reihenfolge)
std::string stock_text(const Ledger& l) {
    std::vector<std::string> rows;
    char key[12], line[96];
    for (int s = 0; s < l.stores(); ++s)
        for (int g = 0; g < l.goods(); ++g)
            if (l.qty(s, g) || l.reserved(s, g)) {
                std::snprintf(line, sizeof(line), "%s/%s=%d(%d)", l.store_key(s, key, sizeof(key)), l.good(g).id,
                              l.qty(s, g), l.reserved(s, g));
                rows.push_back(line);
            }
    std::sort(rows.begin(), rows.end());
    std::string out;
    for (const std::string& r : rows) out += r + " ";
    return out;
}

// 48 Figuren (FigureBook::kMax) an einem Ort, über den Speicherstand auf freie Kacheln gesetzt
std::string fill_figures(Run& a, const char* place) {
    pixelia::FigureBook& book = a.env().figures;
    const int p = pixelia::place_index(place);
    pixelia::FigureDef def{};
    def.name = "Messfigur";
    while (book.count() < pixelia::FigureBook::kMax && book.add(def, static_cast<uint8_t>(p))) {
    }
    for (int k = 0; k < book.count(); ++k) {
        book.at(k).where = pixelia::Whereabouts{};
        book.at(k).where.kind = pixelia::Whereabouts::Kind::Place;
        book.at(k).where.place = static_cast<uint8_t>(p);
        const uint32_t seq = book.at(k).doing.seq;
        book.at(k).doing = pixelia::Doing{};
        book.at(k).doing.seq = seq;
    }
    const std::vector<uint8_t> s = a.save();
    if (!a.stage.load_state(s.data(), s.size())) return "Laden";
    return book.count() == pixelia::FigureBook::kMax ? std::string() : std::string("keine 48 Figuren");
}

// --- Teil b: Speichern und Protokoll ---

using pixelia::EconomyLog;
using pixelia::Stage;
namespace fs = std::filesystem;

// Speicherstand als Abschnitte (roh), zum gezielten Verändern; build setzt Länge und Prüfsumme neu
struct RawChunk {
    std::string tag;
    uint16_t version;
    std::vector<uint8_t> data;
};
std::vector<RawChunk> raw_chunks(const std::vector<uint8_t>& s) {
    std::vector<RawChunk> out;
    for (size_t at = pixelia::kSaveHeader; at + 10 <= s.size();) {
        RawChunk c;
        c.tag.assign(reinterpret_cast<const char*>(s.data() + at), 4);
        c.version = static_cast<uint16_t>(s[at + 4] | s[at + 5] << 8);
        const uint32_t len = SaveReader(s.data() + at + 6, 4).u32();
        c.data.assign(s.begin() + static_cast<long>(at + 10), s.begin() + static_cast<long>(at + 10 + len));
        out.push_back(c);
        at += 10 + len;
    }
    return out;
}
void put_u32(std::vector<uint8_t>& b, size_t at, uint32_t v) {
    for (int k = 0; k < 4; ++k) b[at + k] = static_cast<uint8_t>(v >> (8 * k));
}
std::vector<uint8_t> build_save(const std::vector<uint8_t>& orig, const std::vector<RawChunk>& cs,
                                const std::vector<uint8_t>& tail = {}) {
    std::vector<uint8_t> b(orig.begin(), orig.begin() + pixelia::kSaveHeader);
    for (const RawChunk& c : cs) {
        b.insert(b.end(), c.tag.begin(), c.tag.end());
        b.push_back(static_cast<uint8_t>(c.version));
        b.push_back(static_cast<uint8_t>(c.version >> 8));
        const size_t at = b.size();
        b.resize(at + 4);
        put_u32(b, at, static_cast<uint32_t>(c.data.size()));
        b.insert(b.end(), c.data.begin(), c.data.end());
    }
    b.insert(b.end(), tail.begin(), tail.end());
    put_u32(b, 24, static_cast<uint32_t>(b.size()));
    put_u32(b, 4, pixelia::crc32(b.data() + 8, b.size() - 8));
    return b;
}
RawChunk* chunk_of(std::vector<RawChunk>& cs, const char* tag) {
    for (RawChunk& c : cs)
        if (c.tag == tag) return &c;
    return nullptr;
}
// Datensätze eines Abschnitts mit Anzahl (u16) und Datensätzen mit Länge (FIGS)
std::vector<std::vector<uint8_t>> records_of(const std::vector<uint8_t>& d) {
    std::vector<std::vector<uint8_t>> out;
    size_t at = 2;
    const int n = d[0] | d[1] << 8;
    for (int k = 0; k < n && at + 2 <= d.size(); ++k) {
        const size_t len = d[at] | d[at + 1] << 8;
        out.emplace_back(d.begin() + static_cast<long>(at + 2), d.begin() + static_cast<long>(at + 2 + len));
        at += 2 + len;
    }
    return out;
}
std::vector<uint8_t> records_data(const std::vector<std::vector<uint8_t>>& rs, int count = -1) {
    const int n = count < 0 ? static_cast<int>(rs.size()) : count;
    std::vector<uint8_t> d = {static_cast<uint8_t>(n), static_cast<uint8_t>(n >> 8)};
    for (const auto& r : rs) {
        d.push_back(static_cast<uint8_t>(r.size()));
        d.push_back(static_cast<uint8_t>(r.size() >> 8));
        d.insert(d.end(), r.begin(), r.end());
    }
    return d;
}

std::vector<uint8_t> read_all(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}
fs::path fresh_dir(const char* name) {
    const fs::path d = fs::path(".logs") / name;
    std::error_code ec;
    fs::remove_all(d, ec);
    fs::remove_all(d.string() + "-alt", ec);
    fs::create_directories(d, ec);
    return d;
}
// Inhalt aller CSV-Dateien eines Ordners (Name → Bytes), zum Vergleich zweier Läufe
std::map<std::string, std::vector<uint8_t>> csv_files(const fs::path& d) {
    std::map<std::string, std::vector<uint8_t>> m;
    std::error_code ec;
    for (const auto& e : fs::directory_iterator(d, ec))
        if (e.path().extension() == ".csv") m[e.path().filename().string()] = read_all(e.path());
    return m;
}
uint64_t dir_bytes(const fs::path& d) {
    uint64_t n = 0;
    std::error_code ec;
    for (const auto& e : fs::recursive_directory_iterator(d, ec))
        if (e.is_regular_file()) n += e.file_size();
    return n;
}

// kleines Warenbuch für das Protokoll: ein Lager, zwei Waren; Buchungen an zwei Tagen (Bewegungen und Bedarf)
struct LogBench {
    GoodDef goods[2] = {{"holz", "Holz"}, {"brot", "Brot"}};
    StoreDef stores[1] = {{"kiste", "Kiste", "testort-platz", StoreKind::Community, 60000}};
    EconomyDef def{goods, 2, stores, 1};
    pixelia::Environment env;
    EconomyLog log;
    explicit LogBench(size_t buffer = 64 * 1024) {
        const EconomyDef* defs[] = {&def};
        env.ledger.build(defs, 1);
        log.attach(env, buffer, "simulator");
    }
    // n Buchungen am Welttag day (ab 1), abwechselnd Holz und Brot, dazu je fünfte ein Bedarf
    void book(int day, int n, int from = 0) {
        for (int k = from; k < from + n; ++k) {
            const int64_t us = (day - 1) * pixelia::WorldClock::kUsPerDay + (k + 1) * pixelia::WorldClock::kUsPerSecond;
            env.ledger.add(0, k % 2, 1 + k % 3, Booking::Produce, at(us));
            if (k % 5 == 0) env.ledger.demand(1, 0, 2, 1, at(us));
        }
    }
};

// Bilanz eines Protokoll-Ordners wie tools/wirtschaft_bilanz.py: Bestand am Vortag + Buchungen = Bestand; "" = gut
std::string balance(const fs::path& d, int* checked = nullptr) {
    auto lines = [&](const fs::path& p) {
        std::vector<std::vector<std::string>> out;
        std::ifstream in(p);
        bool head = true;
        for (std::string line; std::getline(in, line);) {
            if (line.empty() || line[0] == '#') continue;
            if (head) {
                head = false;
                continue;
            }
            std::vector<std::string> f;
            size_t a = 0;
            for (size_t b; (b = line.find(',', a)) != std::string::npos; a = b + 1) f.push_back(line.substr(a, b - a));
            f.push_back(line.substr(a));
            out.push_back(f);
        }
        return out;
    };
    std::map<int, std::map<std::string, int>> stock, delta;
    std::error_code ec;
    for (const auto& e : fs::directory_iterator(d, ec)) {
        const std::string n = e.path().filename().string();
        if (n.size() < 7 || n[0] != 't') continue;
        const int day = std::atoi(n.c_str() + 1);
        if (n.find("bestand_lager_tag") != std::string::npos) {
            auto& s = stock[day];
            for (const auto& f : lines(e.path())) s[f[1] + "/" + f[2]] = std::atoi(f[3].c_str());
        } else if (n.find("bewegungen") != std::string::npos) {
            for (const auto& f : lines(e.path())) {
                const int q = std::atoi(f[5].c_str());
                if (!f[6].empty()) delta[day][f[6] + "/" + f[4]] -= q;
                if (!f[7].empty()) delta[day][f[7] + "/" + f[4]] += q;
            }
        }
    }
    int prev = -1, n = 0;
    for (const auto& [day, s] : stock) {
        if (prev >= 0) {
            std::map<std::string, int> want = stock[prev];
            for (int k = prev + 1; k <= day; ++k)
                for (const auto& [key, q] : delta[k]) want[key] += q;
            for (const auto& [key, q] : s) want.try_emplace(key, 0);
            for (const auto& [key, q] : want) {
                ++n;
                const auto it = s.find(key);
                const int got = it == s.end() ? 0 : it->second;
                if (got != q)
                    return "Tag " + std::to_string(day) + " " + key + ": erwartet " + std::to_string(q) + ", Bestand " +
                           std::to_string(got);
            }
        }
        prev = day;
    }
    if (checked) *checked = n;
    return std::string();
}

// Speicherstand im Format 0.5.10 (ohne Erstmalig-Merker hinter der Statur, ohne KENN): so schrieb ihn 0.5.10
std::vector<uint8_t> as_0510(Run& r) {
    for (int k = 0; k < r.env().figures.count(); ++k) r.env().figures.at(k).first_ok = r.env().figures.at(k).first_fail = 0;
    const std::vector<uint8_t> s = r.save();
    std::vector<RawChunk> cs = raw_chunks(s);
    RawChunk* figs = chunk_of(cs, "FIGS");
    auto recs = records_of(figs->data);
    for (auto& rec : recs) rec.pop_back();  // Anzahl der Merker (0)
    figs->data = records_data(recs);
    cs.erase(std::remove_if(cs.begin(), cs.end(), [](const RawChunk& c) { return c.tag == "KENN"; }), cs.end());
    return build_save(s, cs);
}

// Ein PXA1-Paket mit einem Eintrag (für B02): Kopf, Eintrag, Daten; crc stimmt immer
std::vector<uint8_t> pack_bytes(uint32_t data_field, uint32_t off, uint32_t packed, uint32_t size_out,
                                const std::vector<uint8_t>& payload, uint8_t type = 2) {
    std::vector<uint8_t> b(pixelia::kPackHeaderBytes + pixelia::kPackEntryBytes, 0);
    std::memcpy(b.data(), "PXA1", 4);
    b[4] = 1;  // ein Eintrag, keine Namen
    put_u32(b, 8, data_field);
    put_u32(b, 12, size_out);
    uint8_t* e = b.data() + pixelia::kPackHeaderBytes;
    const uint32_t id = pixelia::asset_id("b02");
    std::memcpy(e, &id, 4);
    e[4] = type;
    e[5] = 0;  // ungepackt
    put_u32(b, pixelia::kPackHeaderBytes + 8, off);
    put_u32(b, pixelia::kPackHeaderBytes + 12, packed);
    put_u32(b, pixelia::kPackHeaderBytes + 16, size_out);
    b.insert(b.end(), payload.begin(), payload.end());
    put_u32(b, 16, pixelia::crc32(b.data() + pixelia::kPackHeaderBytes, b.size() - pixelia::kPackHeaderBytes));
    return b;
}
struct MemSource : pixelia::AssetSource {
    std::vector<uint8_t> data;
    const char* name() const override { return "speicher"; }
    bool load(const char*, pixelia::AssetBlob* out) override {
        *out = pixelia::AssetBlob{data.data(), data.size(), nullptr};
        return true;
    }
};

std::vector<Case> cases() {
    return {
        // --- V01 Ereignis-Bus ---
        {"V01 Ereignis-Bus: läuft der Ring über, liest niemand eine Meldung vor ihrer Zustellung (Gegenprobe der Prüfung)",
         [] {
             EventBus bus;
             const int rd = bus.subscribe(pixelia::kAllEvents);
             const int q = EventBus::kQueue;
             for (int i = 0; i < q; ++i) {
                 WorldEvent e;
                 e.who = static_cast<uint16_t>(i + 1);
                 bus.publish(e);
             }
             bus.deliver();
             WorldEvent future;
             future.who = 999;
             bus.publish(future);
             std::vector<int> got;
             for (WorldEvent e; bus.poll(rd, &e);) got.push_back(e.who);
             if (std::find(got.begin(), got.end(), 999) != got.end()) return std::string("vor der Zustellung gelesen");
             if (static_cast<int>(got.size()) != q - 1 || got.front() != 2 || got.back() != q || bus.lost() != 1)
                 return "gelesen " + std::to_string(got.size()) + ", verloren " + std::to_string(bus.lost());
             bus.deliver();
             WorldEvent e;
             if (!bus.poll(rd, &e) || e.who != 999) return std::string("nach der Zustellung nicht gelesen");
             if (bus.poll(rd, &e)) return std::string("doppelt gelesen");
             return std::string();
         }},
        {"V01 Ereignis-Bus: Ring knapp unter, genau und über voll, Überlauf vor der Zustellung; Verluste genau gezählt",
         [] {
             const int q = EventBus::kQueue;
             for (const int c : {q - 1, q, q + 1, 2 * q + 5}) {
                 EventBus bus;
                 const int rd = bus.subscribe(pixelia::kAllEvents);
                 for (int i = 0; i < c; ++i) {
                     WorldEvent e;
                     e.who = static_cast<uint16_t>(i + 1);
                     bus.publish(e);
                 }
                 bus.deliver();
                 int n = 0, first = 0, last = 0;
                 for (WorldEvent e; bus.poll(rd, &e);) {
                     if (!n) first = e.who;
                     if (n && e.who != last + 1) return "Lücke bei " + std::to_string(c) + " Meldungen";
                     last = e.who;
                     ++n;
                 }
                 if (n != std::min(c, q) || first != std::max(1, c - q + 1) ||
                     static_cast<int>(bus.lost()) != std::max(0, c - q))
                     return std::to_string(c) + " Meldungen: " + std::to_string(n) + " gelesen ab " + std::to_string(first) +
                            ", verloren " + std::to_string(bus.lost());
             }
             EventBus bus;
             const int rd = bus.subscribe(pixelia::kAllEvents);
             for (int i = 0; i < q + 5; ++i) {
                 WorldEvent e;
                 e.who = static_cast<uint16_t>(i + 1);
                 bus.publish(e);
             }
             WorldEvent e;
             if (bus.poll(rd, &e)) return std::string("Überlauf vor der Zustellung: trotzdem gelesen");
             bus.deliver();
             int n = 0, first = 0;
             while (bus.poll(rd, &e)) first = n++ ? first : e.who;
             if (n != q || first != 6 || bus.lost() != 5)
                 return "nach dem Überlauf: " + std::to_string(n) + " ab " + std::to_string(first) + ", verloren " +
                        std::to_string(bus.lost());
             return std::string();
         }},
        {"V01 Ereignis-Bus: drei verschieden schnelle Leser, Meldungen auch zwischen ihren Abfragen; jede genau einmal gelesen oder verloren",
         [] {
             EventBus bus;
             int rd[3];
             for (int& r : rd) r = bus.subscribe(pixelia::kAllEvents);
             const int every[3] = {1, 5, 17};
             int published = 0, delivered = 0, last[3] = {0, 0, 0};
             long reads = 0;
             std::string err;
             const auto drain = [&](int i) {
                 for (WorldEvent e; bus.poll(rd[i], &e);) {
                     if (e.who <= last[i] || e.who > delivered) err = "Leser " + std::to_string(i) + ": Meldung " +
                                                                      std::to_string(e.who) + " zur falschen Zeit";
                     last[i] = e.who;
                     ++reads;
                 }
             };
             for (int s = 0; s < 400 && err.empty(); ++s) {
                 const int n = (s * 7) % 23;
                 for (int k = 0; k < n; ++k) {
                     WorldEvent e;
                     e.who = static_cast<uint16_t>(++published);
                     bus.publish(e);
                     if (k == n / 2) drain(0);  // mitten im Schritt: sieht nur Zugestelltes
                 }
                 bus.deliver();
                 delivered = published;
                 for (int i = 0; i < 3; ++i)
                     if (s % every[i] == 0) drain(i);
             }
             bus.deliver();
             delivered = published;
             for (int i = 0; i < 3; ++i) drain(i);
             if (!err.empty()) return err;
             if (reads + static_cast<long>(bus.lost()) != 3L * published)
                 return std::to_string(reads) + " gelesen + " + std::to_string(bus.lost()) + " verloren ≠ 3 × " +
                        std::to_string(published);
             return bus.lost() > 0 ? std::string() : std::string("langsamer Leser hat nichts verloren (Prüfung zu schwach)");
         }},
        {"V01 48 Figuren im Zeitraffer 3600: kein Ereignis verloren, die Chronik liest an jeder Takt-Grenze",
         [] {
             Run a(51, "scrolltest", 7, 0, 1.0);
             if (std::string e = fill_figures(a, "testort-wiese"); !e.empty()) return e;
             a.clock.set_speed(3600.0);
             const uint32_t before = a.env().events.published();
             for (int t = 0; t < 30 * 20; ++t) {  // 20 Weltstunden, je Schritt 120 Takt-Grenzen
                 a.run(1);
                 for (WorldEvent e; a.stage.poll_event(&e);) {
                 }
             }
             const pixelia::EventBus& bus = a.env().events;
             std::printf("  info  48 Figuren, 20 Weltstunden: %u Meldungen, höchstens %u zwischen zwei Zustellungen (Platz %d)\n",
                         bus.published() - before, bus.peak(), EventBus::kQueue);
             if (bus.lost()) return "verloren: " + std::to_string(bus.lost());
             if (bus.peak() * 2 > static_cast<uint32_t>(EventBus::kQueue)) return std::string("weniger als die Hälfte Reserve");
             return checked(a.env().ledger);
         }},
        // --- V02 Reservierungsnummern ---
        {"V02 Reservierungen: nach dem Umlauf der Nummern nie eine belegte; Speichern und Laden danach behält die Zuordnung",
         [] {
             Wide f;
             Ledger& l = f.ledger;
             l.add(0, 0, 10, Booking::Produce, {});
             l.add(1, 0, 10, Booking::Produce, {});
             const uint16_t old = l.reserve(0, 0, 1, 10);
             for (int i = 0; i < 65534; ++i) {
                 const uint16_t r = l.reserve(1, 0, 1, 20);
                 if (!r || r == old) return "belegte oder keine Kennung bei " + std::to_string(i);
                 l.release(r);
             }
             const uint16_t next = l.reserve(1, 0, 2, 30);
             if (!next || next == old) return std::string("nach dem Umlauf die belegte Kennung");
             if (l.reservation_left(next) != 2 || l.reservation_left(old) != 1) return std::string("falsche Mengen");
             Wide g;
             const Ledger::LoadStats st = load_into(g.ledger, ledger_bytes(l));
             if (st.rejected || g.ledger.reservation_left(old) != 1 || g.ledger.reservation_left(next) != 2)
                 return std::string("nach dem Laden andere Zuordnung");
             if (std::string e = checked(g.ledger); !e.empty()) return "geladen: " + e;
             l.release(next);
             if (l.reserved(0, 0) != 1 || l.reserved(1, 0) != 0) return std::string("falsche Reservierung freigegeben");
             return checked(l);
         }},
        {"V02 Reservierungen: eine doppelte Kennung im Speicherstand wird abgelehnt; die Bilanz-Prüfung bleibt sauber",
         [] {
             Wide f;
             f.ledger.add(0, 0, 5, Booking::Produce, {});
             const std::vector<uint8_t> b = ledger_bytes(f.ledger, [](SaveWriter& w) {
                 w.u16(8);
                 w.u16(2);
                 for (int k = 0; k < 2; ++k) {
                     const size_t r = w.record_begin();
                     w.u16(7);
                     w.u16(static_cast<uint16_t>(40 + k));
                     w.str("s0");
                     w.str("w0");
                     w.u16(1);
                     w.record_end(r);
                 }
             });
             Wide g;
             const Ledger::LoadStats st = load_into(g.ledger, b);
             if (st.rejected != 1 || st.reservations != 1) return "übernommen " + std::to_string(st.reservations) +
                                                                   ", abgelehnt " + std::to_string(st.rejected);
             if (g.ledger.reservation_left(7) != 1 || g.ledger.reserved(0, 0) != 1) return std::string("Menge falsch");
             return checked(g.ledger);
         }},
        // --- V03 Bestandstabelle ---
        {"V03 Bestandstabelle: 384 einmal benutzte, leere Einträge werden an der Takt-Grenze frei; ein neues Lager nimmt Ware an (Gegenprobe)",
         [] {
             Wide f;
             Ledger& l = f.ledger;
             l.step(0, 0, nullptr);
             for (int s = 0; s < 4; ++s)
                 for (int g = 0; g < Ledger::kMaxGoods; ++g)
                     if (l.add(s, g, 1, Booking::Produce, {}) != 1 || l.take(s, g, 1, Booking::Consume, {}) != 1)
                         return std::string("Buchung");
             if (l.entries() != Ledger::kMaxStock) return "belegt " + std::to_string(l.entries());
             l.step(2 * pixelia::WorldClock::kUsPerDay, 2, nullptr);
             if (l.entries() != 0 || l.entries_freed() != Ledger::kMaxStock)
                 return "nach zwei Tagen belegt " + std::to_string(l.entries());
             if (l.add(4, 0, 1, Booking::Produce, {}) != 1) return std::string("neues Lager nimmt nichts an");
             return checked(l);
         }},
        {"V03 Bestandstabelle: Verbrauch der letzten 24 Stunden, Reservierung und Gemeldetes halten einen leeren Eintrag",
         [] {
             Wide f;
             Ledger& l = f.ledger;
             const int64_t h = 60 * pixelia::WorldClock::kUsPerMinute;
             l.step(0, 0, nullptr);
             l.add(0, 0, 5, Booking::Produce, at(h));
             l.take(0, 0, 5, Booking::Consume, at(h));
             l.add(1, 1, 3, Booking::Produce, at(h));
             const uint16_t r = l.reserve(1, 1, 3, 9);
             l.step(2 * h, 0, nullptr);
             if (l.out_24h(0, 0) != 5 || l.use_24h(0) != 5 || l.entries() != 2) return std::string("zu früh freigegeben");
             l.step(26 * h, 1, nullptr);
             if (l.out_24h(0, 0) != 0 || l.entries() != 1) return "nach 25 Stunden belegt " + std::to_string(l.entries());
             if (l.reservation_left(r) != 3 || l.total(1) != 3) return std::string("Reservierung oder Summe verändert");
             return checked(l);
         }},
        {"V03 Testdorf 30 Weltstunden: an der Takt-Grenze dieselbe Tabelle wie nach einem Neustart, danach dasselbe Warenbuch",
         [] {
             Run a(31, "testort-raum", 6, 0, 600.0);
             a.run(30 * 180);  // 30 Weltstunden
             const int64_t g = pixelia::Stage::kGridUs;
             a.clock.set_world_us((a.clock.world_us() / g + 1) * g);
             a.stage.tick(a.clock);
             const std::vector<uint8_t> s = a.save();
             Run b(31, "testort-raum");
             if (!b.stage.load_state(s.data(), s.size())) return std::string("Laden");
             b.clock.set_world_us(a.clock.world_us());
             b.clock.set_speed(600.0);
             if (a.env().ledger.entries() != b.env().ledger.entries())
                 return "Einträge " + std::to_string(a.env().ledger.entries()) + " ohne, " +
                        std::to_string(b.env().ledger.entries()) + " mit Neustart";
             std::printf("  info  Testdorf nach 30 Weltstunden: %d Einträge belegt, %u wieder frei geworden\n",
                         a.env().ledger.entries(), a.env().ledger.entries_freed());
             a.run(30 * 60);
             b.run(30 * 60);
             for (Run* r : {&a, &b}) {
                 r->clock.set_world_us((r->clock.world_us() / g + 1) * g);
                 r->stage.tick(r->clock);
             }
             const Ledger &x = a.env().ledger, &y = b.env().ledger;
             if (x.entries() != y.entries()) return std::string("Einträge weichen nach weiteren 10 Stunden ab");
             const std::string dx = stock_text(x), dy = stock_text(y);
             if (dx != dy) return "ohne Neustart: " + dx + " | mit Neustart: " + dy;
             return checked(y);
         }},
        // --- V04 Einzelstück-Transport ---
        {"V04 Einzelstück: Transport scheitert an voller Tabelle, vollem oder ungültigem Ziel und ändert nichts (Gegenprobe)",
         [] {
             Wide f(true);
             Ledger& l = f.ledger;
             for (int s = 0; s < 4; ++s)
                 for (int g = 0; g < Ledger::kMaxGoods; ++g)
                     if (l.add(s, g, 1, Booking::Produce, at(s * 100 + g)) != 1) return std::string("Buchung");
             const uint16_t res = l.reserve(0, 0, 1, 10);
             const uint32_t v = l.version();
             const auto same = [&]() -> std::string {
                 if (l.qty(0, 0) != 1 || l.qty(4, 0) != 0 || l.reservation_left(res) != 1 || l.reserved(0, 0) != 1)
                     return "Menge oder Reservierung verändert";
                 int at0 = 0, at4 = 0;
                 for (int i = 0; i < l.items(); ++i) {
                     at0 += l.item_at(i).good == 0 && l.item_at(i).store == 0;
                     at4 += l.item_at(i).good == 0 && l.item_at(i).store == 4;
                 }
                 if (at0 != 1 || at4 != 0) return "Einzelstück gewandert";
                 return l.version() == v ? checked(l) : std::string("Buchungszähler verändert");
             };
             if (l.move(0, 4, 0, 1, {}, res) != 0) return std::string("Tabelle voll: trotzdem bewegt");
             if (std::string e = same(); !e.empty()) return "Tabelle voll: " + e;
             if (l.move(0, 99, 0, 1, {}, res) != 0) return std::string("ungültiges Ziel: bewegt");
             if (std::string e = same(); !e.empty()) return "ungültiges Ziel: " + e;
             Wide s(true, 2);
             Ledger& m = s.ledger;
             m.add(0, 0, 1, Booking::Produce, at(1));
             m.add(1, 1, 2, Booking::Produce, at(2));  // Ziel voll
             const uint16_t r2 = m.reserve(0, 0, 1, 10);
             if (m.move(0, 1, 0, 1, {}, r2) != 0 || m.reservation_left(r2) != 1 || m.reserved(0, 0) != 1)
                 return std::string("Ziel voll: Reservierung verändert");
             return checked(m);
         }},
        {"V04 Einzelstück: ein Teil gelingt, verbraucht genau so viel Reservierung; Abbuchen ebenso",
         [] {
             Wide f(true, 3);
             Ledger& l = f.ledger;
             l.add(0, 0, 3, Booking::Produce, at(1));
             l.add(1, 1, 2, Booking::Produce, at(2));  // Ziel hat Platz für eins
             const uint16_t res = l.reserve(0, 0, 2, 10);
             if (l.move(0, 1, 0, 3, {}, res) != 1) return std::string("nicht genau eins bewegt");
             if (l.reservation_left(res) != 1 || l.reserved(0, 0) != 1 || l.qty(0, 0) != 2 || l.qty(1, 0) != 1)
                 return std::string("Reservierung passt nicht zur Teilmenge");
             if (l.take(0, 0, 1, Booking::Consume, {}, res) != 1 || l.reservation_left(res) != 0 || l.reserved(0, 0) != 0)
                 return std::string("Abbuchen mit Reservierung");
             if (l.take(0, 0, 5, Booking::Consume, {}) != 1 || l.qty(0, 0) != 0) return std::string("freies Stück");
             return checked(l);
         }},
        // --- V05 Nachschub ohne Quelle, P-F Gründe ---
        {"V05 Ohne Quelle wählt Pixelia: nächstes Lager mit genug Vorrat, Gleichstand nach Kennung, Erzeuger zählen mit",
         [] {
             struct Want {
                 std::vector<std::pair<const char*, int>> stock;
                 const char* expect;
             };
             const Want cases[] = {
                 {{{"k_platz", 20}}, "k_platz"},                       // die einzige (zwei Wege weit)
                 {{{"k_platz", 20}, {"k_wiese", 20}}, "k_wiese"},       // die nähere
                 {{{"k_wiese", 20}, {"k_kammer", 20}}, "k_kammer"},     // gleich weit: kleinere Kennung
                 {{{"k_wiese", 20}, {"e_wiese", 20}}, "e_wiese"},       // Erzeuger zählt mit
                 {{{"k_wiese", 2}, {"k_platz", 20}}, "k_platz"},        // weniger als die halbe Ladung: lieber weiter
                 {{}, nullptr},                                         // nirgends Vorrat
             };
             for (const Want& w : cases) {
                 Village v;
                 for (const auto& [id, n] : w.stock) v.put(id, n);
                 v.step();
                 const int got = v.fetch_from("haus");
                 if (got != (w.expect ? v.store(w.expect) : -1))
                     return std::string("erwartet ") + (w.expect ? w.expect : "keine") + ", gewählt " + v.name(got);
                 if (!w.expect && v.tasks.supply_wait(v.store("haus")) != SupplyWait::NoSource)
                     return std::string("Grund fehlt: keine Quelle");
                 if (std::string e = checked(v.l); !e.empty()) return e;
             }
             return std::string();
         }},
        {"V05 Eingetragene Quelle bleibt verbindlich: ist sie leer, wird gewartet (auch wenn anderswo Vorrat liegt); Projekte wie Aufgaben",
         [] {
             Village v;
             v.put("k_wiese", 20);
             v.step();
             if (v.fetch_from("haus2") >= 0) return std::string("leere eingetragene Quelle umgangen");
             if (v.tasks.supply_wait(v.store("haus2")) != SupplyWait::SourceEmpty) return std::string("Grund: Quelle leer");
             if (v.fetch_from("haus") != v.store("k_wiese")) return std::string("ohne Quelle nicht gewählt");
             // Projekt ohne Quelle: dieselbe Wahl (Kammer: Wiese zwei Wege, Platz drei)
             Village p;
             p.put("k_platz", 20);
             p.put("k_wiese", 20);
             pixelia::ProjectList pl;
             const EconomyDef* defs[] = {&p.def};
             pl.build(defs, 1, p.l);
             pl.start_new();
             p.now += pixelia::Stage::kGridUs;
             pl.step(p.now, 0, p.l, p.tasks, p.book, nullptr, false);
             if (pl.count() != 1) return std::string("Projekt nicht begonnen");
             if (p.fetch_from("site") != p.store("k_wiese")) return "Projekt holt von " + p.name(p.fetch_from("site"));
             return checked(p.l);
         }},
        {"P-F Gründe ohne Nachschub: Quelle leer, lohnt noch nicht, keine Quelle; Zähler je Takt-Grenze; kommt Vorrat, geht es weiter",
         [] {
             Village v;
             v.put("haus", 3);  // unter low (4), aber nicht leer
             v.put("k_wiese", 2);
             v.step();
             v.step();
             if (v.tasks.supply_wait(v.store("haus2")) != SupplyWait::SourceEmpty ||
                 v.tasks.supply_wait(v.store("haus")) != SupplyWait::NotWorth)
                 return std::string("Gründe: ") + pixelia::supply_wait_id(v.tasks.supply_wait(v.store("haus2"))) + ", " +
                        pixelia::supply_wait_id(v.tasks.supply_wait(v.store("haus")));
             if (v.tasks.supply_waits(SupplyWait::SourceEmpty) != 2 || v.tasks.supply_waits(SupplyWait::NotWorth) != 2)
                 return std::string("Zähler je Takt-Grenze");
             int good = -1;
             v.tasks.supply_wait(v.store("haus2"), &good);
             if (good != v.l.find_good("holz")) return std::string("Ware zum Grund fehlt");
             v.put("k_platz", 30);
             v.step();
             if (v.fetch_from("haus2") != v.store("k_platz") || v.fetch_from("haus") != v.store("k_platz"))
                 return std::string("nach dem Vorrat kein Nachschub");
             if (v.tasks.supply_wait(v.store("haus2")) != SupplyWait::None) return std::string("Grund bleibt stehen");
             return checked(v.l);
         }},
        {"Daten prüfen: eine unbekannte Quelle oder Ware in den Lagerplätzen wird gemeldet; das Testdorf ist sauber",
         [] {
             Village v;
             char buf[160];
             if (const char* e = v.l.data_problem(buf, sizeof(buf))) return std::string("ohne Fehler gemeldet: ") + e;
             Village bad;
             bad.stores[5].source = "k_plat";
             const EconomyDef* defs[] = {&bad.def};
             bad.l.build(defs, 1);
             const char* e = bad.l.data_problem(buf, sizeof(buf));
             if (!e || !std::strstr(e, "haus2") || !std::strstr(e, "k_plat")) return std::string("Tippfehler nicht gemeldet");
             Run a(5);
             if (const char* x = a.env().ledger.data_problem(buf, sizeof(buf))) return std::string("Testdorf: ") + x;
             if (const char* x = a.env().projects.data_problem(buf, sizeof(buf))) return std::string("Testdorf: ") + x;
             return std::string();
         }},
        // --- B12, B13 ---
        {"B12 Tagesprotokoll: ein Lager mit 25, 30 und 96 Waren steht vollständig in bestand_lager_tag",
         [] {
             for (const int n : {25, 30, Ledger::kMaxGoods}) {
                 Wide f(false, 1000, n);
                 pixelia::Environment env;
                 const EconomyDef* defs[] = {&f.def};
                 env.ledger.build(defs, 1);
                 for (int g = 0; g < n; ++g) env.ledger.add(0, g, 1 + g % 5, Booking::Produce, {});
                 pixelia::EconomyLog log;
                 if (!log.attach(env, 128 * 1024, "simulator")) return std::string("Puffer");
                 const std::filesystem::path dir = std::filesystem::path(".logs") / "test-boden-b12";
                 std::error_code ec;
                 std::filesystem::remove_all(dir, ec);
                 std::filesystem::create_directories(dir, ec);
                 log.day_rows(3);
                 log.cut();
                 if (!log.flush(dir.string().c_str())) return std::string("Anhängen");
                 std::ifstream in(dir / "t00003-bestand_lager_tag.csv");
                 int rows = 0, sum = 0;
                 for (std::string line; std::getline(in, line);) {
                     if (line.rfind("3,s0,", 0) != 0) continue;
                     ++rows;
                     sum += std::atoi(line.c_str() + line.find(',', 5) + 1);
                 }
                 int want = 0;
                 for (int g = 0; g < n; ++g) want += 1 + g % 5;
                 if (rows != n || sum != want)
                     return std::to_string(n) + " Waren: " + std::to_string(rows) + " Zeilen, Summe " + std::to_string(sum);
             }
             return std::string();
         }},
        // --- B10 (Teil c) ---
        {"B10 Zähler des Protokolls: Erzeugen und Anhängen in zwei Threads, ein dritter liest; nichts läuft rückwärts",
         [] {
             Wide f(false, 1000, 8);
             pixelia::Environment env;
             const EconomyDef* defs[] = {&f.def};
             env.ledger.build(defs, 1);
             pixelia::EconomyLog log;
             if (!log.attach(env, 128 * 1024, "simulator")) return std::string("Puffer");
             const std::filesystem::path dir = std::filesystem::path(".logs") / "test-boden-b10";
             std::error_code ec;
             std::filesystem::remove_all(dir, ec);
             std::filesystem::create_directories(dir, ec);
             const std::string d = dir.string();
             // Übergabe wie am Gerät: 0 frei, 1 gepackt (cut), 2 hängt an (flush)
             std::atomic<int> state{0};
             std::atomic<bool> done{false};
             std::string err;
             std::thread prod([&] {
                 for (int day = 1; day <= 40; ++day) {
                     for (int k = 0; k < 50; ++k) env.ledger.add(0, k % 8, 1, Booking::Produce, {});
                     log.day_rows(day);
                     while (state.load() != 0) std::this_thread::yield();
                     log.cut();
                     state = 1;
                 }
                 done = true;
             });
             std::thread cons([&] {
                 while (!done || state.load() == 1) {
                     if (state.load() != 1) {
                         std::this_thread::yield();
                         continue;
                     }
                     state = 2;
                     log.flush(d.c_str());
                     state = 0;
                 }
             });
             uint32_t rows = 0, written = 0;
             int reads = 0;
             while (!done && err.empty()) {
                 const pixelia::EconomyLog::Stats st = log.stats();
                 if (st.rows < rows || st.written < written) err = "Zähler rückwärts";
                 if (st.written > st.rows) err = "mehr geschrieben als erzeugt";
                 rows = st.rows, written = st.written;
                 ++reads;
             }
             prod.join();
             cons.join();
             if (!err.empty()) return err;
             while (state.load() != 0) {
             }
             const pixelia::EconomyLog::Stats st = log.stats();
             if (st.written != st.rows || st.lost) return "am Ende " + std::to_string(st.written) + " von " +
                                                          std::to_string(st.rows) + " geschrieben, verloren " + std::to_string(st.lost);
             return reads > 10 ? std::string() : std::string("zu wenige Lesungen");
         }},
        {"B13 Laden: Lager, die es nicht mehr gibt, landen im Gemeinschaftslager nur, soweit Platz ist; der eigene Bestand zuerst; Bilanz exakt",
         [] {
             const GoodDef goods[] = {{"holz", "Holz"}};
             const StoreDef before[] = {{"neu", "Neu", "testort-platz", StoreKind::Community, 100},
                                        {"alt1", "Alt 1", "testort-platz", StoreKind::Community, 60000},
                                        {"alt2", "Alt 2", "testort-platz", StoreKind::Community, 60000}};
             const EconomyDef da{goods, 1, before, 3};
             const EconomyDef db{goods, 1, before, 1};  // nur noch „neu“
             Ledger a, b;
             const EconomyDef* pa[] = {&da};
             const EconomyDef* pb[] = {&db};
             a.build(pa, 1);
             b.build(pb, 1);
             a.add(1, 0, 40000, Booking::Produce, {});
             a.add(2, 0, 40000, Booking::Produce, {});
             a.add(0, 0, 30, Booking::Produce, {});
             const Ledger::LoadStats st = load_into(b, ledger_bytes(a));
             if (b.qty(0, 0) != 100 || b.total(0) != 100) return "im Lager " + std::to_string(b.qty(0, 0));
             if (st.lost != 80030 - 100 || st.moved != 70)
                 return "verloren " + std::to_string(st.lost) + ", umgelegt " + std::to_string(st.moved);
             return checked(b);
         }},
        // --- B14 Reisen ---
        {"B14 48 Figuren brechen an derselben Takt-Grenze über den Feldweg auf: keine kommt vor der Zeit an, jede genau einmal, auch mit Neustart",
         [] {
             Run a(61, "scrolltest", 10, 0, 1.0);
             if (std::string e = fill_figures(a, "testort-wiese"); !e.empty()) return e;
             pixelia::FigureBook& book = a.env().figures;
             const int platz = pixelia::place_index("testort-platz");
             const int64_t g = pixelia::Stage::kGridUs, go = (a.clock.world_us() / g + 2) * g;
             for (int k = 0; k < book.count(); ++k) {
                 pixelia::Doing& d = book.at(k).doing;
                 d.kind = pixelia::DoingKind::Leave;
                 d.exit = 2;  // Feldweg nach dem Platz, 30 Weltsekunden
                 d.walk_us = go;
                 d.end_us = go;
             }
             const int rd = a.env().events.subscribe(pixelia::event_bit(pixelia::EventKind::Arrived));
             while (a.clock.world_us() < go + 2 * g) a.run(1);
             if (a.stage.in_transit() != pixelia::FigureBook::kMax)
                 return "unterwegs " + std::to_string(a.stage.in_transit()) + " von 48";
             const std::vector<uint8_t> s = a.save();
             Run b(61, "scrolltest");
             pixelia::Stage::LoadReport rep;
             if (!b.stage.load_state(s.data(), s.size(), &rep) || rep.transit != pixelia::FigureBook::kMax)
                 return "nach dem Laden unterwegs " + std::to_string(rep.transit);
             b.clock.set_world_us(a.clock.world_us());
             const int rb = b.env().events.subscribe(pixelia::event_bit(pixelia::EventKind::Arrived));
             int arrived[2] = {0, 0};
             const auto early = [&](Run& r) {
                 for (int k = 0; k < r.env().figures.count(); ++k) {
                     const pixelia::FigureRecord& f = r.env().figures.at(k);
                     if (f.where.kind == pixelia::Whereabouts::Kind::Place && f.where.place == platz &&
                         r.clock.world_us() < go + 30 * g)
                         return true;
                 }
                 return false;
             };
             while (a.clock.world_us() < go + 30 * g) {  // bis zur Takt-Grenze der Ankunft, danach gehen sie weiter
                 a.run(1);
                 b.run(1);
                 if (early(a) || early(b)) return std::string("vor der Reisezeit angekommen");
                 for (WorldEvent e; a.env().events.poll(rd, &e);) ++arrived[0];
                 for (WorldEvent e; b.env().events.poll(rb, &e);) ++arrived[1];
             }
             for (Run* r : {&a, &b}) {
                 int here = 0;
                 for (int k = 0; k < r->env().figures.count(); ++k)
                     here += r->env().figures.at(k).where.place == platz;
                 if (here != pixelia::FigureBook::kMax || r->stage.in_transit() != 0)
                     return "am Platz " + std::to_string(here) + ", unterwegs " + std::to_string(r->stage.in_transit());
                 char buf[160];
                 if (const char* e = r->stage.check_figures(buf, sizeof(buf))) return std::string(e);
             }
             if (arrived[0] != pixelia::FigureBook::kMax || arrived[1] != pixelia::FigureBook::kMax)
                 return "Ankünfte " + std::to_string(arrived[0]) + " / " + std::to_string(arrived[1]);
             // V01: 48 Aufbrüche an einer Takt-Grenze sind der Grund für 128 Plätze im Ring (vorher 32)
             std::printf("  info  48 Aufbrüche an einer Takt-Grenze: höchstens %u Meldungen zwischen zwei Zustellungen, verloren %u\n",
                         a.env().events.peak(), a.env().events.lost());
             if (a.env().events.peak() < 48 || a.env().events.lost()) return std::string("Ereignisse der Aufbrüche fehlen");
             return checked(b.env().ledger);
         }},
        // --- B15 Wege ---
        {"B15 Wege: ist die Brücke zu, zählt der offene Umweg; ist auch der Steg zu, nennt die Karte den Grund",
         [] {
             const pixelia::PlaceDef* list[] = {&kPA, &kPB, &kPC, &kPD};
             pixelia::PlaceGraph gr;
             gr.build(list, 4);
             char buf[160];
             const char* err = gr.check(buf, sizeof(buf));
             if (err && *err) return std::string("Graph: ") + err;
             pixelia::Environment env;
             env.flags.set("steg", true);
             int8_t d[pixelia::PlaceGraph::kMaxPlaces];
             gr.open_hops_to(1, env, d);
             if (gr.hops(0, 1) != 1 || d[0] != 3 || d[2] != 2 || d[3] != 1)
                 return "Brücke zu: A " + std::to_string(d[0]) + ", C " + std::to_string(d[2]) + ", D " + std::to_string(d[3]);
             if (gr.closed_why(0, 1, env)) return std::string("Umweg offen, trotzdem ein Grund");
             env.flags.set("bruecke", true);
             gr.open_hops_to(1, env, d);
             if (d[0] != 1) return std::string("Brücke offen: nicht der direkte Weg");
             env.flags.set("bruecke", false);
             env.flags.set("steg", false);
             gr.open_hops_to(1, env, d);
             int8_t from[pixelia::PlaceGraph::kMaxPlaces];
             gr.open_hops_from(0, env, from);
             if (d[0] != -1 || from[2] != 1 || from[1] != -1) return std::string("alles zu: trotzdem ein Weg");
             const char* why = gr.closed_why(0, 1, env);
             if (!why || std::strcmp(why, "Erst nach dem Bau begehbar.") != 0)
                 return std::string("Grund: ") + (why ? why : "keiner");
             return std::string();
         }},
        {"B15 Testdorf nachts: der Feldweg ist zu, Figuren gehen über den Rand zum Platz und warten nie auf einen Weg",
         [] {
             Run a(71, "testort-wiese", 23, 0, 60.0);
             const pixelia::PlaceGraph& gr = pixelia::world_graph();
             const int wiese = pixelia::place_index("testort-wiese"), platz = pixelia::place_index("testort-platz");
             if (gr.closed_why(wiese, platz, a.env())) return std::string("nachts kein Weg zum Platz");
             int8_t d[pixelia::PlaceGraph::kMaxPlaces];
             gr.open_hops_to(platz, a.env(), d);
             for (int p = 0; p < gr.places(); ++p)
                 if (d[p] != gr.hops(p, platz)) return std::string("offene Wege kürzer oder länger als die Daten");
             a.run(30 * 120);  // 2 Weltstunden nachts
             char buf[160];
             if (const char* e = a.stage.check_figures(buf, sizeof(buf))) return std::string(e);
             return checked(a.env().ledger);
         }},
        // --- B17 Projekte ---
        {"B17 Projektdaten: unbekannte Ware, zu viele Bedarfe, falsche Baustelle sperren das Projekt mit Meldung; gültige laufen weiter",
         [] {
             struct Bad {
                 const char* what;
                 std::function<void(Village&)> spoil;
                 const char* expect;
             };
             static pixelia::ProjectNeed five[5] = {{"holz", 1}, {"holz", 1}, {"holz", 1}, {"holz", 1}, {"holz", 1}};
             const Bad bad[] = {
                 {"Ware", [](Village& v) { v.needs[0].good = "brettt"; }, "Schritt stapeln: Ware „brettt“ gibt es nicht"},
                 {"Bedarfe", [](Village& v) { v.steps[0].needs = five; v.steps[0].need_count = 5; }, "5 Bedarfe"},
                 {"Baustelle", [](Village& v) { v.projects[0].site = "nix"; }, "Baustelle „nix“"},
                 {"Baustelle ist kein Bauplatz", [](Village& v) { v.projects[0].site = "k_kammer"; }, "Baustelle „k_kammer“"},
                 {"Quelle", [](Village& v) { v.projects[0].source = "k_plat"; }, "Quelle „k_plat“"},
             };
             for (const Bad& b : bad) {
                 Village v;
                 b.spoil(v);
                 pixelia::ProjectList pl;
                 const EconomyDef* defs[] = {&v.def};
                 v.l.build(defs, 1);
                 pl.build(defs, 1, v.l);
                 char buf[160];
                 const char* e = pl.data_problem(buf, sizeof(buf));
                 if (!e || !std::strstr(e, "Projekt stapel") || !std::strstr(e, b.expect))
                     return std::string(b.what) + ": Meldung „" + (e ? e : "keine") + "“";
                 pl.start_new();
                 if (pl.count() != 0 || pl.test_start(0, 0, v.l, v.tasks)) return std::string(b.what) + ": trotzdem begonnen";
             }
             Village ok;
             pixelia::ProjectList pl;
             const EconomyDef* defs[] = {&ok.def};
             pl.build(defs, 1, ok.l);
             char buf[160];
             if (const char* e = pl.data_problem(buf, sizeof(buf))) return std::string("gültig gemeldet: ") + e;
             pl.start_new();
             return pl.count() == 1 ? std::string() : std::string("gültiges Projekt beginnt nicht");
         }},
        // --- B18 Erstmalig-Merker ---
        {"B18 Erstes Mal: Gezeigtes verdeckt das eigene erste Gelingen nicht, Vergessen macht kein zweites erstes Mal",
         [] {
             using pixelia::MemoryKind;
             using pixelia::Skill;
             pixelia::FigureBook book;
             pixelia::FigureDef def{};
             def.name = "Greta";
             const uint16_t t = book.add(def, 0)->id;
             def.name = "Lisl";
             const uint16_t s = book.add(def, 0)->id;
             book.learn(t, Skill::Baking, 3000, 1, 600);  // Meisterin
             if (!book.teach(t, s, Skill::Baking, 30, 1, 610)) return std::string("zeigen geht nicht");
             const auto count = [&](MemoryKind k, uint16_t who) {
                 int n = 0;
                 for (const pixelia::Memory& m : book.find(s)->memories)
                     n += m.kind == k && m.arg == static_cast<uint8_t>(Skill::Baking) && m.who == who;
                 return n;
             };
             if (count(MemoryKind::Taught, t) != 1 || count(MemoryKind::First, t) != 0)
                 return std::string("Gezeigtes ist kein eigenes Gelingen");
             const uint32_t bit = 1u << static_cast<int>(Skill::Baking);
             pixelia::Rng rng(7);
             int successes = 0;
             for (int i = 0; i < 200 && successes < 3; ++i)
                 successes += book.try_skill(s, Skill::Baking, 0, rng, 2, 600).outcome == pixelia::Outcome::Success;
             if (successes < 3) return std::string("kein Gelingen in 200 Versuchen");
             if (count(MemoryKind::First, 0) != 1 || !(book.find(s)->first_ok & bit))
                 return "erstes Gelingen " + std::to_string(count(MemoryKind::First, 0)) + "-mal erinnert";
             // vergessen (der Platz wird frei): danach darf kein zweites erstes Mal entstehen
             for (pixelia::Memory& m : book.find(s)->memories)
                 if (m.kind == MemoryKind::First) m = pixelia::Memory{};
             for (int i = 0; i < 50; ++i) book.try_skill(s, Skill::Baking, 0, rng, 4, 600);
             if (count(MemoryKind::First, 0) != 0) return std::string("nach dem Vergessen ein zweites erstes Mal");
             if (!(book.find(s)->first_ok & bit)) return std::string("Merker weg");
             // alle Plätze mit Wichtigerem belegt: das erste Gelingen zählt trotzdem (der Merker, nicht die Erinnerung)
             def.name = "Jakob";
             const uint16_t j = book.add(def, 0)->id;
             for (int i = 0; i < pixelia::FigureRecord::kMemories; ++i) {
                 pixelia::Memory m;
                 m.kind = MemoryKind::Event;
                 m.weight = 250;
                 m.day = 3;
                 m.minute = static_cast<uint16_t>(i);
                 book.remember(j, m);
             }
             book.learn(j, Skill::Baking, 400, 3, 600);
             for (int i = 0; i < 200 && !(book.find(j)->first_ok & bit); ++i) book.try_skill(j, Skill::Baking, 0, rng, 5, 600);
             return (book.find(j)->first_ok & bit) ? std::string() : std::string("volle Erinnerung: erstes Gelingen fehlt");
         }},
        {"B18 Erstmalig-Merker überstehen Speichern und Laden; ein Stand aus 0.5.9 leitet sie aus den Erinnerungen ab",
         [] {
             Run a(81);
             pixelia::FigureRecord& f = a.env().figures.at(2);
             f.first_ok |= 1u << static_cast<int>(pixelia::Skill::Baking);
             f.first_fail |= 1u << static_cast<int>(pixelia::Skill::Smithing);
             const uint16_t id = f.id;
             const std::vector<uint8_t> s = a.save();
             Run b(82);
             if (!b.stage.load_state(s.data(), s.size())) return std::string("Laden");
             const pixelia::FigureRecord* g = b.env().figures.find(id);
             if (!g || g->first_ok != f.first_ok || g->first_fail != f.first_fail) return std::string("Merker verloren");
             std::vector<uint8_t> old;
             if (FILE* fp = std::fopen("sim/testdata/welt-0.5.9.pxw", "rb")) {
                 uint8_t buf[4096];
                 for (size_t n; (n = std::fread(buf, 1, sizeof(buf), fp)) > 0;) old.insert(old.end(), buf, buf + n);
                 std::fclose(fp);
             }
             if (old.empty()) return std::string("Vorlage sim/testdata/welt-0.5.9.pxw fehlt");
             Run c(83);
             if (!c.stage.load_state(old.data(), old.size())) return std::string("alter Stand lädt nicht");
             for (int k = 0; k < c.env().figures.count(); ++k) {
                 const pixelia::FigureRecord& x = c.env().figures.at(k);
                 uint32_t ok = 0, fail = 0;
                 for (const pixelia::Memory& m : x.memories) {
                     if (m.kind == pixelia::MemoryKind::First && m.who) return std::string("Gezeigtes noch als erstes Mal");
                     if (m.kind == pixelia::MemoryKind::First) ok |= 1u << m.arg;
                     if (m.kind == pixelia::MemoryKind::Failure) fail |= 1u << m.arg;
                 }
                 if (x.first_ok != ok || x.first_fail != fail) return std::string("aus dem alten Stand falsch abgeleitet");
             }
             return std::string();
         }},
        // --- Teil b: Speichern und Protokoll ---
        {"B01 Gegenprobe: FIGS kündigt eine Figur an, ohne Datensatz (Prüfsumme stimmt): abgelehnt, die Welt bleibt byte-gleich",
         [] {
             Run a(91);
             const std::vector<uint8_t> before = a.save();
             std::vector<uint8_t> data(1024);
             SaveWriter w(data.data(), data.size());
             w.begin(10, 0);
             w.chunk("WELT", 1);
             w.u64(91);
             w.i64(0);
             w.u16(1);
             w.chunk("FIGS", 1);
             w.u16(1);  // verspricht eine Figur
             data.resize(w.finish());
             Stage::LoadReport rep;
             if (!pixelia::save_check(data.data(), data.size(), nullptr)) return std::string("Kopf oder CRC falsch gebaut");
             if (a.stage.load_state(data.data(), data.size(), &rep)) return std::string("geladen");
             if (a.save() != before) return std::string("Welt verändert");
             if (std::strcmp(rep.chunk, "FIGS") != 0) return std::string("Grund ohne Abschnitt: ") + rep.why;
             return std::string();
         }},
        {"B01 Abgeschnitten, zu lang, Reste, doppelt, neuere Version, Figur doppelt, ohne WELT: jedes abgelehnt mit Grund, die Welt bleibt",
         [] {
             Run a(92);
             a.run(300);
             const std::vector<uint8_t> s = a.save();
             const std::vector<uint8_t> before = a.save();
             using Mut = std::function<std::vector<uint8_t>()>;
             const std::vector<std::pair<const char*, Mut>> muts = {
                 {"Datensatz abgeschnitten", [&] { auto cs = raw_chunks(s); chunk_of(cs, "FIGS")->data.pop_back(); return build_save(s, cs); }},
                 {"Abschnitt länger als die Datei", [&] { auto b = s; put_u32(b, b.size() - 4, 0); auto cs = raw_chunks(s);
                     b = build_save(s, cs, {'X', 'X', 'X', 'X', 1, 0, 200, 0, 0, 0, 1, 2}); return b; }},
                 {"Reste hinter dem letzten Abschnitt", [&] { return build_save(s, raw_chunks(s), {1, 2, 3}); }},
                 {"Reste in einem Abschnitt", [&] { auto cs = raw_chunks(s); chunk_of(cs, "WELT")->data.push_back(0); return build_save(s, cs); }},
                 {"Abschnitt doppelt", [&] { auto cs = raw_chunks(s); cs.push_back(*chunk_of(cs, "FIGS")); return build_save(s, cs); }},
                 {"neuere Version", [&] { auto cs = raw_chunks(s); chunk_of(cs, "LAGR")->version = 2; return build_save(s, cs); }},
                 {"Figur doppelt", [&] { auto cs = raw_chunks(s); auto recs = records_of(chunk_of(cs, "FIGS")->data);
                     recs[1][0] = recs[0][0]; recs[1][1] = recs[0][1]; chunk_of(cs, "FIGS")->data = records_data(recs); return build_save(s, cs); }},
                 {"Figur ohne Kennung", [&] { auto cs = raw_chunks(s); auto recs = records_of(chunk_of(cs, "FIGS")->data);
                     recs[2][0] = recs[2][1] = 0; chunk_of(cs, "FIGS")->data = records_data(recs); return build_save(s, cs); }},
                 {"mehr Figuren als Platz", [&] { auto cs = raw_chunks(s); auto recs = records_of(chunk_of(cs, "FIGS")->data);
                     const auto one = recs[0]; recs.clear();
                     for (int k = 1; k <= pixelia::FigureBook::kMax + 1; ++k) { auto r = one; r[0] = static_cast<uint8_t>(k); r[1] = 0; recs.push_back(r); }
                     chunk_of(cs, "FIGS")->data = records_data(recs); return build_save(s, cs); }},
                 {"ohne WELT", [&] { auto cs = raw_chunks(s); cs.erase(cs.begin()); return build_save(s, cs); }},
             };
             for (const auto& [what, make] : muts) {
                 const std::vector<uint8_t> bad = make();
                 Stage::LoadReport rep;
                 if (a.stage.load_state(bad.data(), bad.size(), &rep)) return std::string(what) + ": geladen";
                 if (!rep.why || !rep.why[0]) return std::string(what) + ": ohne Grund";
                 if (Stage::check_state(bad.data(), bad.size())) return std::string(what) + ": Prüfung allein nimmt ihn an";
                 if (a.save() != before) return std::string(what) + ": Welt verändert";
             }
             return std::string();
         }},
        {"B01 Erlaubt bleibt: unbekannter Abschnitt, fehlende Abschnitte (alte Stände), längere Datensätze (neue Felder hinten), Stand 0.5.9 und 0.5.10",
         [] {
             Run a(93);
             a.run(300);
             const std::vector<uint8_t> s = a.save();
             auto loads = [&](const std::vector<uint8_t>& b, const char* what) -> std::string {
                 Run c(94);
                 Stage::LoadReport rep;
                 if (!c.stage.load_state(b.data(), b.size(), &rep))
                     return std::string(what) + ": abgelehnt (" + rep.why + " " + rep.chunk + ")";
                 return std::string();
             };
             std::string e;
             {
                 auto cs = raw_chunks(s);
                 cs.insert(cs.begin() + 2, RawChunk{"XTRA", 7, {1, 2, 3, 4, 5}});
                 if (!(e = loads(build_save(s, cs), "unbekannter Abschnitt")).empty()) return e;
             }
             for (const char* drop : {"TAET", "AUFG", "PROJ", "KENN", "WARE", "LAGR", "STCK", "RESV", "CHRO", "KAL "}) {
                 auto cs = raw_chunks(s);
                 cs.erase(std::remove_if(cs.begin(), cs.end(), [&](const RawChunk& c) { return c.tag == drop; }), cs.end());
                 if (!(e = loads(build_save(s, cs), drop)).empty()) return e;
             }
             {
                 auto cs = raw_chunks(s);
                 for (const char* tag : {"FIGS", "TAET", "LAGR"}) {
                     auto recs = records_of(chunk_of(cs, tag)->data);
                     if (std::string(tag) == "TAET") {
                         auto& d = chunk_of(cs, tag)->data;  // TAET: erst die Takt-Grenze (8 Byte), dann die Liste
                         std::vector<uint8_t> list(d.begin() + 8, d.end());
                         recs = records_of(list);
                         for (auto& r : recs) r.insert(r.end(), {9, 9});
                         auto nd = records_data(recs);
                         d.resize(8);
                         d.insert(d.end(), nd.begin(), nd.end());
                         continue;
                     }
                     for (auto& r : recs) r.insert(r.end(), {9, 9, 9});
                     chunk_of(cs, tag)->data = records_data(recs);
                 }
                 if (!(e = loads(build_save(s, cs), "längere Datensätze")).empty()) return e;
             }
             Run b(95);
             const std::vector<uint8_t> old = as_0510(b);
             if (!(e = loads(old, "Stand 0.5.10")).empty()) return e;
             Run c(96), d(97);
             Stage::LoadReport rc;
             c.stage.load_state(old.data(), old.size(), &rc);
             d.stage.load_state(old.data(), old.size());
             if (rc.identity || c.stage.world_id() != d.stage.world_id() || c.stage.line_id() != d.stage.line_id() ||
                 c.stage.world_id() != pixelia::derive_world_id(c.stage.seed()))
                 return std::string("Stand 0.5.10: Kennung nicht stabil aus dem Startwert");
             const std::vector<uint8_t> v059 = read_all("sim/testdata/welt-0.5.9.pxw");
             if (v059.empty()) return std::string("Vorlage welt-0.5.9.pxw fehlt");
             return loads(v059, "Stand 0.5.9");
         }},
        {"B01 Zufällige Bytefehler mit neu gerechneter Prüfsumme (400 Fälle): nie ein Absturz; abgelehnt = Welt byte-gleich, angenommen = neuer Stand gültig",
         [] {
             Run a(98);
             a.run(300);
             const std::vector<uint8_t> s = a.save();
             uint32_t x = 0x9E3779B9u;
             auto rnd = [&]() { return x = x * 1664525u + 1013904223u; };
             int accepted = 0, rejected = 0;
             for (int k = 0; k < 400; ++k) {
                 std::vector<uint8_t> b = s;
                 const int flips = 1 + static_cast<int>(rnd() % 3);
                 for (int f = 0; f < flips; ++f) b[pixelia::kSaveHeader + rnd() % (b.size() - pixelia::kSaveHeader)] ^= static_cast<uint8_t>(1u << (rnd() % 8));
                 put_u32(b, 4, pixelia::crc32(b.data() + 8, b.size() - 8));
                 Run c(99);
                 const std::vector<uint8_t> before = c.save();
                 if (!c.stage.load_state(b.data(), b.size())) {
                     ++rejected;
                     if (c.save() != before) return "Fall " + std::to_string(k) + ": abgelehnt, aber Welt verändert";
                     continue;
                 }
                 ++accepted;
                 const std::vector<uint8_t> again = c.save();
                 Stage::LoadReport rep;
                 if (!Stage::check_state(again.data(), again.size(), &rep))
                     return "Fall " + std::to_string(k) + ": neuer Stand ungültig (" + rep.why + ")";
                 c.run(30);  // weiterlaufen ohne Absturz
             }
             if (!accepted || !rejected) return "angenommen " + std::to_string(accepted) + ", abgelehnt " + std::to_string(rejected);
             return std::string();
         }},
        {"B01 Zwei Plätze: ist der neueste inhaltlich kaputt, lädt der ältere, neue Zeitlinie; das nächste Speichern überschreibt den kaputten",
         [] {
             const fs::path dir = fresh_dir("test-boden-b01-plaetze");
             const pixelia::SaveSlots slots(dir.string().c_str(), "welt");
             Run a(101);
             a.run(200);
             const std::vector<uint8_t> good = a.save(5);
             a.run(200);
             auto cs = raw_chunks(a.save(6));
             chunk_of(cs, "FIGS")->data.pop_back();  // CRC stimmt, Inhalt nicht
             const std::vector<uint8_t> bad = build_save(a.save(6), cs);
             int sa = -1, sb = -1;
             if (!slots.write(good.data(), good.size(), &sa) || !slots.write(bad.data(), bad.size(), &sb) || sa == sb)
                 return std::string("Plätze nicht geschrieben");
             Run b(102);
             const std::vector<uint8_t> fresh = b.save();
             const uint32_t line = b.stage.line_id();
             Stage::LoadReport rep;
             if (!b.stage.load_slots(slots, &rep)) return std::string("nichts geladen: ") + rep.why;
             if (rep.slot != sa || !rep.fallback || rep.rejected_gen != 6 || rep.next_generation != 6)
                 return "Platz " + std::to_string(rep.slot) + ", Rückfall " + std::to_string(rep.fallback);
             Run ref(103);
             ref.stage.load_state(good.data(), good.size());
             if (b.stage.line_id() == ref.stage.line_id() || b.stage.world_id() != ref.stage.world_id())
                 return std::string("Rückfall ohne neue Zeitlinie");
             (void)fresh;
             (void)line;
             // Chronik (Wahl des Users 2026-11-03): sobald die Weltzeit läuft, steht der Rückfall drin
             b.run(60);
             const pixelia::Chronicle& ch = b.stage.environment().chronicle;
             bool noted = false;
             for (int k = 0; k < ch.count(); ++k)
                 noted = noted || std::strstr(ch.at(k).text, "Vorigen Speicherstand geladen") != nullptr;
             if (!noted) return std::string("Rückfall nicht in der Chronik");
             const std::vector<uint8_t> next = b.save(7);
             int sn = -1;
             if (!slots.write(next.data(), next.size(), &sn) || sn != sb) return std::string("überschreibt den geladenen Platz");
             Run c(104);
             if (!c.stage.load_slots(slots, &rep) || rep.fallback || rep.generation != 7)
                 return std::string("danach nicht der neue Stand");
             // beide kaputt: nichts geladen, Welt bleibt
             std::vector<uint8_t> broken = next;
             broken[40] ^= 0xFF;
             const pixelia::SaveSlots two((dir / "zwei").string().c_str(), "welt");
             fs::create_directories(dir / "zwei");
             two.write(broken.data(), broken.size());
             two.write(bad.data(), bad.size());
             Run d(105);
             const std::vector<uint8_t> was = d.save();
             if (d.stage.load_slots(two, &rep) || d.save() != was) return std::string("kaputte Plätze geladen");
             return std::string();
         }},
        {"B09 Sicherung einspielen: neue Zeitlinie im Stand (KENN ersetzt bzw. beim Stand 0.5.9 angehängt), Generation neu, gültig",
         [] {
             Run a(111);
             std::vector<uint8_t> s = a.save(3);
             const size_t n0 = s.size();
             s.resize(n0 + 64);
             Stage::LoadReport rep;
             size_t n = Stage::prepare_import(s.data(), n0, s.size(), 20, 0xABCD1234u, &rep);
             if (n != n0) return std::string("mit KENN: Länge ") + std::to_string(n) + " " + rep.why;
             Run b(112);
             if (!b.stage.load_state(s.data(), n, &rep) || rep.generation != 20 || b.stage.line_id() != 0xABCD1234u ||
                 b.stage.world_id() != a.stage.world_id())
                 return std::string("mit KENN: nicht übernommen");
             std::vector<uint8_t> old = read_all("sim/testdata/welt-0.5.9.pxw");
             const size_t o0 = old.size();
             old.resize(o0 + 64);
             n = Stage::prepare_import(old.data(), o0, old.size(), 21, 0x5555AAAAu, &rep);
             if (n != o0 + 18) return std::string("0.5.9: nicht angehängt ") + rep.why;
             Run c(113);
             if (!c.stage.load_state(old.data(), n, &rep) || !rep.identity || c.stage.line_id() != 0x5555AAAAu ||
                 c.stage.world_id() != pixelia::derive_world_id(c.stage.seed()))
                 return std::string("0.5.9: Kennung falsch");
             std::vector<uint8_t> old2 = read_all("sim/testdata/welt-0.5.9.pxw");
             if (Stage::prepare_import(old2.data(), old2.size(), old2.size(), 22, 1, &rep)) return std::string("ohne Platz angehängt");
             auto cs = raw_chunks(a.save(3));
             chunk_of(cs, "CHRO")->data.push_back(1);
             std::vector<uint8_t> bad = build_save(a.save(3), cs);
             const size_t b0 = bad.size();
             bad.resize(b0 + 64);
             if (Stage::prepare_import(bad.data(), b0, bad.size(), 23, 1, &rep)) return std::string("kaputten Stand vorbereitet");
             return std::string();
         }},
        {"B04 Schreibfehler beim Öffnen, mitten in einer Zeile, nach mehreren Zeilen, beim Schließen: nichts geht verloren, nichts doppelt, keine halbe Zeile",
         [] {
             const fs::path ref = fresh_dir("test-boden-b04-ref");
             {
                 LogBench r;
                 r.book(1, 40);
                 r.book(2, 25);
                 r.log.cut();
                 if (!r.log.flush(ref.string().c_str())) return std::string("Vergleich nicht geschrieben");
             }
             const auto want = csv_files(ref);
             struct F {
                 const char* what;
                 EconomyLog::Fault f;
             };
             const F faults[] = {{"Öffnen", {0, -1, -1}},       {"zweite Datei öffnen", {1, -1, -1}},
                                 {"mitten in der Kopfzeile", {-1, 30, -1}}, {"mitten in einer Zeile", {-1, 900, -1}},
                                 {"nach mehreren Zeilen", {-1, 2500, -1}},  {"Schließen", {-1, -1, 0}},
                                 {"drittes Schließen", {-1, -1, 2}}};
             for (const F& f : faults) {
                 const fs::path dir = fresh_dir("test-boden-b04");
                 LogBench b;
                 b.book(1, 40);
                 b.book(2, 25);
                 b.log.cut();
                 EconomyLog::set_fault(f.f);
                 const bool ok1 = b.log.flush(dir.string().c_str());
                 EconomyLog::set_fault({});
                 const EconomyLog::Stats s1 = b.log.stats();
                 if (ok1) return std::string(f.what) + ": Fehler nicht gemeldet";
                 if (!b.log.pending()) return std::string(f.what) + ": Zeilen nicht mehr im Puffer";
                 // „geschrieben“ zählt nur bestätigte Zeilen: so viele, wie in vollständigen Dateien stehen
                 uint32_t whole = 0;
                 for (const auto& [name, bytes] : csv_files(dir)) {
                     if (want.at(name) != bytes) return std::string(f.what) + ": " + name + " halb geschrieben";
                     whole += static_cast<uint32_t>(std::count(bytes.begin(), bytes.end(), '\n')) - 2;
                 }
                 if (s1.written != whole) return std::string(f.what) + ": geschrieben " + std::to_string(s1.written) + " statt " + std::to_string(whole);
                 b.book(2, 5, 25);  // weiter gebucht, dann der nächste Stand
                 b.log.cut();
                 if (!b.log.flush(dir.string().c_str())) return std::string(f.what) + ": zweites Anhängen";
                 LogBench r2;
                 r2.book(1, 40);
                 r2.book(2, 30);
                 r2.log.cut();
                 const fs::path ref2 = fresh_dir("test-boden-b04-ref2");
                 r2.log.flush(ref2.string().c_str());
                 if (csv_files(dir) != csv_files(ref2)) return std::string(f.what) + ": Dateien danach nicht wie ohne Fehler";
             }
             return std::string();
         }},
        {"B04 Journal: Strom weg nach dem Speicherstand (auch mitten im Anhängen) → beim Start genau einmal nachgetragen; vor dem Speicherstand → verworfen",
         [] {
             const fs::path ref = fresh_dir("test-boden-b04j-ref");
             {
                 LogBench r;
                 r.book(1, 30);
                 r.book(2, 20);
                 r.log.cut();
                 r.log.flush(ref.string().c_str());
             }
             const auto want = csv_files(ref);
             for (int mode = 0; mode < 3; ++mode) {  // 0 nach dem Stand, 1 mitten im Anhängen, 2 vor dem Stand
                 const fs::path dir = fresh_dir("test-boden-b04j");
                 {
                     LogBench b;
                     b.log.open(dir.string().c_str(), 7, 8, 0);
                     b.book(1, 30);
                     b.book(2, 20);
                     b.log.cut();
                     if (!b.log.prepare(dir.string().c_str(), 5)) return std::string("kein Journal");
                     if (mode == 1) {  // Strom weg mitten im Anhängen: eine Datei halb, eine ganz
                         const auto half = want.at("t00001-bewegungen.csv");
                         std::ofstream(dir / "t00001-bewegungen.csv", std::ios::binary)
                             .write(reinterpret_cast<const char*>(half.data()), static_cast<std::streamsize>(half.size() / 2));
                         const auto whole = want.at("t00001-bedarf.csv");
                         std::ofstream(dir / "t00001-bedarf.csv", std::ios::binary)
                             .write(reinterpret_cast<const char*>(whole.data()), static_cast<std::streamsize>(whole.size()));
                     }
                 }  // Strom weg: Puffer verloren
                 LogBench boot;
                 boot.log.open(dir.string().c_str(), 7, 8, mode == 2 ? 4 : 5);
                 const EconomyLog::Stats st = boot.log.stats();
                 if (fs::exists(dir / "journal.pxj")) return std::string("Journal bleibt liegen");
                 if (mode == 2) {
                     if (!csv_files(dir).empty() || st.discarded == 0) return std::string("vor dem Stand: nicht verworfen");
                     continue;
                 }
                 if (csv_files(dir) != want) return "Fall " + std::to_string(mode) + ": nicht genau einmal nachgetragen";
                 if (!st.recovered) return std::string("nichts als nachgetragen gezählt");
                 // zweiter Start mit demselben (gelöschten) Journal ändert nichts
                 LogBench again;
                 again.log.open(dir.string().c_str(), 7, 8, 5);
                 if (csv_files(dir) != want) return std::string("zweiter Start verändert");
             }
             // Journal einer anderen Zeitlinie: verworfen
             const fs::path dir = fresh_dir("test-boden-b04j");
             {
                 LogBench b;
                 b.log.open(dir.string().c_str(), 7, 8, 0);
                 b.book(1, 10);
                 b.log.cut();
                 b.log.prepare(dir.string().c_str(), 5);
             }
             LogBench other;
             other.log.open(dir.string().c_str(), 7, 9, 5);
             if (other.log.stats().discarded == 0 || other.log.stats().archives != 1) return std::string("fremde Zeitlinie nachgetragen");
             return std::string();
         }},
        {"B08 Ein Tag über 256 KB: weitere Zeilen gezählt statt geschrieben, Vermerk im Tag; insgesamt nie über 1 MB, auch mitten im Anhängen",
         [] {
             const fs::path dir = fresh_dir("test-boden-b08");
             LogBench b(256 * 1024);
             b.log.open(dir.string().c_str(), 1, 1, 0);
             uint64_t peak = 0;
             for (int round = 0; round < 6; ++round) {
                 b.book(3, 2000, round * 2000);
                 b.log.cut();
                 b.log.flush(dir.string().c_str());
                 peak = std::max<uint64_t>(peak, dir_bytes(dir));
             }
             const EconomyLog::Stats s = b.log.stats();
             uint64_t day3 = 0;
             for (const char* t : {"bewegungen", "projekte", "bedarf"}) {
                 std::error_code ec;
                 const auto sz = fs::file_size(dir / ("t00003-" + std::string(t) + ".csv"), ec);
                 if (!ec) day3 += sz;
             }
             if (!s.dropped || day3 > EconomyLog::kMaxDayBytes || !fs::exists(dir / "t00003-verluste.csv"))
                 return "Tag 3: " + std::to_string(day3) + " Byte, verworfen " + std::to_string(s.dropped);
             // viele Tage: die ältesten gehen, die Grenze hält jederzeit
             for (int day = 4; day < 40; ++day) {
                 b.book(day, 1200);
                 b.log.cut();
                 if (!b.log.flush(dir.string().c_str())) return "Tag " + std::to_string(day) + ": Anhängen";
                 peak = std::max<uint64_t>(peak, dir_bytes(dir));
             }
             if (peak > EconomyLog::kMaxBytes) return "über 1 MB: " + std::to_string(peak);
             if (b.log.stats().first_day <= 3) return std::string("älteste Tage nicht gelöscht");
             // Gegenprobe der Prüfung: ein fremder Tag knapp über 1 MB liegt schon im Ordner
             const fs::path q = fresh_dir("test-boden-b08q");
             {
                 std::ofstream big(q / "t00001-bewegungen.csv", std::ios::binary);
                 std::string x(EconomyLog::kMaxBytes + 1, 'x');
                 big << x;
             }
             LogBench c;
             c.book(2, 5);
             c.log.cut();
             c.log.flush(q.string().c_str());
             if (dir_bytes(q) > EconomyLog::kMaxBytes) return "Gegenprobe: " + std::to_string(dir_bytes(q));
             return std::string();
         }},
        {"B09 Zeitlinien: anderer Stand → alter Ordner nach wirtschaft-alt/1, neue Marke; alte Zeitlinien zählen zur Grenze und gehen zuerst",
         [] {
             const fs::path dir = fresh_dir("test-boden-b09");
             {
                 LogBench a(256 * 1024);
                 a.log.open(dir.string().c_str(), 1, 10, 0);
                 for (int day = 1; day < 25; ++day) {
                     a.book(day, 1000);
                     a.log.cut();
                     a.log.flush(dir.string().c_str());
                 }
             }
             const uint64_t old_bytes = dir_bytes(dir);
             LogBench same;
             same.log.open(dir.string().c_str(), 1, 10, 0);
             if (same.log.stats().archives != 0) return std::string("gleiche Zeitlinie archiviert");
             LogBench b(256 * 1024);
             b.log.open(dir.string().c_str(), 1, 11, 0);
             if (b.log.stats().archives != 1 || !fs::exists(dir.string() + "-alt/1/zeitlinie.txt") ||
                 !csv_files(dir).empty())
                 return std::string("nicht archiviert");
             if (b.log.stats().archive_bytes < old_bytes - 100) return std::string("Archiv nicht gezählt");
             std::ifstream m(dir / "zeitlinie.txt");
             std::string mark;
             std::getline(m, mark);
             if (mark.find("linie=0000000b") == std::string::npos) return "Marke: " + mark;
             uint64_t peak = 0;
             for (int day = 1; day < 15; ++day) {
                 b.book(day, 1000);
                 b.log.cut();
                 b.log.flush(dir.string().c_str());
                 peak = std::max<uint64_t>(peak, dir_bytes(dir) + dir_bytes(dir.string() + "-alt"));
             }
             if (peak > EconomyLog::kMaxBytes) return "zusammen über 1 MB: " + std::to_string(peak);
             if (b.log.stats().first_day != 1) return std::string("neue Zeitlinie verlor Tage, bevor das Archiv weg war");
             if (b.log.stats().archive_bytes >= old_bytes) return std::string("Archiv nicht zuerst gelöscht");
             return std::string();
         }},
        {"Tagesaufnahme: Bestand am Vortag + Buchungen = Bestand am Tagesende, im Zeitraffer über 3 Mitternächte (Buchungen im Schritt nach 0 Uhr)",
         [] {
             const fs::path dir = fresh_dir("test-boden-tag");
             Run a(121, "testort-platz", 20, 0, 600.0);
             EconomyLog log;
             if (!log.attach(a.env(), 256 * 1024, "simulator")) return std::string("Puffer");
             for (int i = 0; i < 3 * 4320 + 500; ++i) {
                 a.run(1);
                 log.step(a.stage);
             }
             log.cut();
             log.flush(dir.string().c_str());
             int checked = 0;
             const std::string e = balance(dir, &checked);
             if (!e.empty()) return e;
             if (checked < 20) return "nur " + std::to_string(checked) + " Posten";
             if (log.stats().late) return "Buchungen für einen schon aufgenommenen Tag: " + std::to_string(log.stats().late);
             return std::string();
         }},
        {"B02 Grafik-Paket: Versatz nahe 2³², riesige Länge, Größe hinter dem Ende, Bild 65535³: abgelehnt ohne Zugriff außerhalb",
         [] {
             const std::vector<uint8_t> payload(16, 7);
             const struct {
                 const char* what;
                 std::vector<uint8_t> pack;
             } bad[] = {
                 {"Versatz 0xFFFFFFF0 + 32", pack_bytes(16, 0xFFFFFFF0u, 32, 32, payload)},
                 {"Länge 0xFFFFFFFF", pack_bytes(16, 0, 0xFFFFFFFFu, 16, payload)},
                 {"Datenlänge falsch", pack_bytes(0xFFFFFFF0u, 0, 16, 16, payload)},
                 {"Ausgabe größer als angegeben", pack_bytes(16, 0, 16, 32, payload)},
             };
             for (const auto& b : bad) {
                 MemSource src;
                 src.data = b.pack;
                 pixelia::AssetStore store;
                 store.set_sources(&src, nullptr);
                 store.want("b02", true);
                 if (store.loaded("b02")) return std::string(b.what) + ": geladen";
             }
             // gültiges Paket mit einem Bild, dessen Kopf 65535 × 65535 × 65535 verspricht
             std::vector<uint8_t> img(16, 0);
             img[0] = img[1] = img[2] = img[3] = img[4] = img[5] = 0xFF;
             img[6] = 1;
             MemSource src;
             src.data = pack_bytes(16, 0, 16, 16, img, static_cast<uint8_t>(pixelia::AssetType::Image));
             pixelia::AssetStore store;
             store.set_sources(&src, nullptr);
             store.want("b02", true);
             if (!store.loaded("b02")) return std::string("gültiges Paket nicht geladen");
             pixelia::ImageAsset out;
             if (store.image(pixelia::asset_id("b02"), &out)) return std::string("riesiges Bild angenommen");
             return std::string();
         }},
    };
}

}  // namespace

int run_boden_tests() {
    int failed = 0;
    const auto list = cases();
    for (const auto& c : list) {
        const std::string err = c.run();
        if (err.empty()) {
            std::printf("  ok    %s\n", c.name);
        } else {
            ++failed;
            std::printf("  FEHLER %s: %s\n", c.name, err.c_str());
        }
    }
    std::printf("sim: Fester Boden (0.5.11) %d/%d bestanden\n", static_cast<int>(list.size()) - failed,
                static_cast<int>(list.size()));
    return failed;
}

}  // namespace sim
