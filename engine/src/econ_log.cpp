// Wirtschafts-Protokoll (seit 0.5.10 Teil 4): CSV je Welttag und Tabelle; siehe econ_log.h.

#include "pixelia/engine/econ_log.h"

#include <dirent.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "pixelia/engine/environment.h"
#include "pixelia/engine/ledger.h"
#include "pixelia/engine/memory.h"
#include "pixelia/engine/place.h"
#include "pixelia/engine/png.h"
#include "pixelia/engine/projects.h"
#include "pixelia/engine/stage.h"
#include "pixelia/engine/world_clock.h"

namespace pixelia {

namespace {

const char* const kTableNames[] = {"bewegungen", "projekte", "bedarf", "bestand_lager_tag", "wetter_tag", "verluste"};
static_assert(sizeof(kTableNames) / sizeof(kTableNames[0]) == EconomyLog::kTables);
const char* const kColumns[] = {
    "welttag,zeit,stufe,art,ware,menge,von,nach,person,grund,vorhaben,stueck",
    "welttag,zeit,projekt,ort,art,abschnitt,beginn_tag,beginn_zeit,dauer_min,geplant_min,personen_min,warten_material,"
    "warten_faehigkeit,warten_arbeitskraft,warten_wetter,warten_feierabend,warten_werkzeug,verbaut,werkzeug",
    "welttag,zeit,person,lager,zweck,ware,gewollt,bekommen,fehlmenge",
    "welttag,lager,ware,menge,obergrenze,reserviert",
    "welttag,morgens,mittags,abends,nachts",
    "welttag,tabelle,zeilen,bytes",  // seit 0.5.11 (B08): Zeilen, die nicht mehr in den Tag passten
};
constexpr int kWeatherMinutes[4] = {6 * 60, 12 * 60, 18 * 60, 22 * 60};  // morgens, mittags, abends, nachts
constexpr size_t kHead = 7;  // Eintrag im Puffer: Tabelle (1), Welttag (4), Länge (2), dann der Text
constexpr uint8_t kWritten = 0xFF, kDropped = 0xFE;  // statt der Tabelle: angehängt bzw. Tag zu voll (B08)
constexpr uint32_t kFresh = 0xFFFFFFFFu;            // Datei gab es noch nicht
constexpr const char* kJournal = "journal.pxj";
constexpr const char* kMarker = "zeitlinie.txt";

int64_t floor_div(int64_t a, int64_t b) { return a >= 0 ? a / b : (a - b + 1) / b; }

int make_dir(const char* p) {
#ifdef _WIN32
    return mkdir(p);
#else
    return mkdir(p, 0775);
#endif
}

// Datei „t00003-bewegungen.csv“: Welttag, sonst −1
int32_t file_day(const char* name) {
    if (name[0] != 't' || std::strlen(name) < 8 || name[6] != '-') return -1;
    int32_t d = 0;
    for (int i = 1; i < 6; ++i) {
        if (name[i] < '0' || name[i] > '9') return -1;
        d = d * 10 + (name[i] - '0');
    }
    return std::strstr(name, ".csv") ? d : -1;
}

int64_t now_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

void put32(uint8_t* p, uint32_t v) {
    for (int k = 0; k < 4; ++k) p[k] = static_cast<uint8_t>(v >> (8 * k));
}
uint32_t get32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) | static_cast<uint32_t>(p[1]) << 8 | static_cast<uint32_t>(p[2]) << 16 |
           static_cast<uint32_t>(p[3]) << 24;
}

// Pfade und Kopfzeilen im Grafik-Speicher statt auf dem Stack (seit 0.5.11 Teil b2): Am Gerät haben Grafik- und
// Lade-Task je 6 KB Stack, und littlefs braucht beim Lesen selbst einiges. Mit bis zu 2,6 KB Puffern ineinander lief
// die Grafik-Task beim Öffnen des Protokolls über und zerstörte den Heap (Absturz im Probelauf). Ohne Speicher bleibt
// der Puffer leer (n = 0): jede Funktion bricht dann wie bei einem Dateifehler ab
struct Buf {
    explicit Buf(size_t bytes) : p(static_cast<char*>(asset_alloc(bytes))), n(p ? bytes : 0) {
        if (p) p[0] = 0;
    }
    ~Buf() { asset_free(p); }
    Buf(const Buf&) = delete;
    Buf& operator=(const Buf&) = delete;
    explicit operator bool() const { return p != nullptr; }
    char* p;
    size_t n;
};

long file_size(const char* path) {
    struct stat sb;
    return stat(path, &sb) == 0 ? static_cast<long>(sb.st_size) : -1;
}

void day_path(char* out, size_t cap, const char* dir, int32_t day, int table) {
    std::snprintf(out, cap, "%.300s/t%05ld-%s.csv", dir, static_cast<long>(std::clamp<int32_t>(day, 0, 99999)),
                  kTableNames[table]);
}

int header_text(int table, const char* source, char* out, size_t cap) {
    const int n = std::snprintf(out, cap, "#pixelia-wirtschaft,format=%d,tabelle=%s,quelle=%s\n%s\n",
                                EconomyLog::kFormat, kTableNames[table], source, kColumns[table]);
    return std::min<int>(n, static_cast<int>(cap) - 1);
}

// Datei auf len Byte kürzen (halb angehängter Teil, B04)
bool cut_to(const char* path, uint32_t len) {
#ifdef _WIN32
    const int fd = _open(path, _O_RDWR | _O_BINARY);
    if (fd < 0) return false;
    const bool ok = _chsize(fd, static_cast<long>(len)) == 0;
    _close(fd);
    return ok;
#else
    return truncate(path, static_cast<off_t>(len)) == 0;
#endif
}

// Dateien bzw. Unterordner eines Ordners
template <class F>
void for_each_file(const char* dir, F f) {
    DIR* d = opendir(dir);
    if (!d) return;
    Buf path(640);
    if (!path) {
        closedir(d);
        return;
    }
    while (dirent* e = readdir(d)) {
        std::snprintf(path.p, path.n, "%.320s/%.200s", dir, e->d_name);
        struct stat sb;
        if (stat(path.p, &sb) == 0 && S_ISREG(sb.st_mode)) f(e->d_name, static_cast<long>(sb.st_size));
    }
    closedir(d);
}
template <class F>
void for_each_dir(const char* dir, F f) {
    DIR* d = opendir(dir);
    if (!d) return;
    Buf path(640);
    if (!path) {
        closedir(d);
        return;
    }
    while (dirent* e = readdir(d)) {
        if (e->d_name[0] == '.') continue;
        std::snprintf(path.p, path.n, "%.320s/%.200s", dir, e->d_name);
        struct stat sb;
        if (stat(path.p, &sb) == 0 && S_ISDIR(sb.st_mode)) f(e->d_name);
    }
    closedir(d);
}

int32_t oldest_day(const char* dir) {
    int32_t best = -1;
    for_each_file(dir, [&](const char* name, long) {
        const int32_t d = file_day(name);
        if (d >= 0 && (best < 0 || d < best)) best = d;
    });
    return best;
}

// alle Dateien eines Tages löschen (erst die Namen sammeln, dann löschen); false = keine oder Fehler
bool delete_day(const char* dir, int32_t day, uint32_t* deleted) {
    char names[EconomyLog::kTables + 3][40];
    int n = 0;
    for_each_file(dir, [&](const char* name, long) {
        if (file_day(name) == day && n < EconomyLog::kTables + 3) std::snprintf(names[n++], sizeof(names[0]), "%.39s", name);
    });
    Buf path(360);
    bool ok = n > 0 && path;
    if (!path) n = 0;
    for (int i = 0; i < n; ++i) {
        std::snprintf(path.p, path.n, "%.300s/%.39s", dir, names[i]);
        if (std::remove(path.p) == 0) ++*deleted;
        else ok = false;
    }
    return ok;
}

void remove_all(const char* dir) {
    char names[16][64];
    for (;;) {
        int n = 0;
        for_each_file(dir, [&](const char* name, long) {
            if (n < 16) std::snprintf(names[n++], sizeof(names[0]), "%.63s", name);
        });
        Buf path(400);
        if (!n || !path) return;
        int gone = 0;
        for (int i = 0; i < n; ++i) {
            std::snprintf(path.p, path.n, "%.320s/%.63s", dir, names[i]);
            gone += std::remove(path.p) == 0;
        }
        if (!gone) return;
    }
}

// Schreibfehler für die Prüfungen (B04); sonst wirkungslos
EconomyLog::Fault g_fault;
int g_opens = 0, g_closes = 0;

FILE* f_open(const char* path, const char* mode) {
    if (g_fault.open_fail >= 0 && g_opens++ == g_fault.open_fail) return nullptr;
    return std::fopen(path, mode);
}
bool f_write(FILE* f, const void* p, size_t n) {
    if (g_fault.write_budget >= 0) {
        const size_t k = std::min<size_t>(n, static_cast<size_t>(g_fault.write_budget));
        g_fault.write_budget -= static_cast<long>(k);
        if (k < n) {
            if (k) std::fwrite(p, 1, k, f);  // halbe Zeile in der Datei
            return false;
        }
    }
    return std::fwrite(p, 1, n, f) == n;
}
bool f_close(FILE* f) {
    const bool flushed = std::fflush(f) == 0;
    const bool closed = std::fclose(f) == 0;
    if (g_fault.close_fail >= 0 && g_closes++ == g_fault.close_fail) return false;
    return flushed && closed;
}

// Anhängen einer Datei mit Größenprüfung (B04): before = Größe davor (kFresh = neu), bytes = was dazukommt
enum class Put { Fail, Written, There, Skipped };
template <class W>
Put put_file(const char* path, uint32_t before, uint32_t bytes, W write) {
    const long cur = file_size(path);
    const uint32_t base = before == kFresh ? 0 : before;
    const uint64_t full = static_cast<uint64_t>(base) + bytes;
    if (cur < 0 && before != kFresh && before != 0) return Put::Skipped;  // inzwischen gelöscht (älteste Tage)
    if (cur >= 0 && static_cast<uint64_t>(cur) == full && bytes) return Put::There;
    if (cur >= 0 && (static_cast<uint64_t>(cur) < base || static_cast<uint64_t>(cur) > full)) return Put::Skipped;
    if (cur > static_cast<long>(base) && !cut_to(path, base)) return Put::Fail;  // halb angehängt: alte Größe
    FILE* f = f_open(path, base == 0 ? "wb" : "ab");
    if (!f) return Put::Fail;
    bool ok = write(f);
    ok = f_close(f) && ok;
    if (ok) return Put::Written;
    if (base == 0) std::remove(path);
    else cut_to(path, base);
    return Put::Fail;
}

// offene Zeilen einer Datei (Tabelle, Tag) im ausstehenden Puffer, in Reihenfolge
template <class F>
void each_row(const uint8_t* pend, size_t len, uint8_t table, int32_t day, F f) {
    for (size_t j = 0; j + kHead <= len;) {
        const size_t lj = pend[j + 5] | (pend[j + 6] << 8);
        int32_t dj;
        std::memcpy(&dj, pend + j + 1, 4);
        if (pend[j] == table && dj == day) f(pend + j + kHead, lj);
        j += kHead + lj;
    }
}

}  // namespace

struct EconomyLog::Group {
    uint8_t table;
    uint8_t state;  // 0 offen, 1 geschrieben bzw. schon da, 2 fehlgeschlagen
    uint16_t rows;
    int32_t day;
    uint32_t before;  // Größe der Datei davor, kFresh = gab es nicht (dann steht die Kopfzeile vorn)
    uint32_t bytes;   // Kopfzeile + Zeilen
};

struct EconomyLog::DayUse {
    int32_t day;
    uint32_t used;
    uint32_t rows[3], bytes[3];
};

void EconomyLog::set_fault(const Fault& f) {
    g_fault = f;
    g_opens = g_closes = 0;
}

const char* EconomyLog::table_name(int t) { return t >= 0 && t < kTables ? kTableNames[t] : "?"; }

EconomyLog::~EconomyLog() { detach(); }

bool EconomyLog::attach(Environment& env, size_t buffer_bytes, const char* source) {
    detach();
    cur_ = static_cast<uint8_t*>(asset_alloc(buffer_bytes));
    pend_ = static_cast<uint8_t*>(asset_alloc(buffer_bytes));
    groups_ = static_cast<Group*>(asset_alloc(sizeof(Group) * kMaxGroups));
    use_ = static_cast<DayUse*>(asset_alloc(sizeof(DayUse) * kMaxDays));
    deltas_ = static_cast<Delta*>(asset_alloc(sizeof(Delta) * kMaxDeltas));
    if (!cur_ || !pend_ || !groups_ || !use_ || !deltas_) {
        detach();
        return false;
    }
    cap_ = buffer_bytes;
    cur_len_ = pend_len_ = 0;
    planned_ = -1;
    journal_ = false;
    delta_n_ = 0;
    env_ = &env;
    source_ = source ? source : "simulator";
    day_ = INT32_MIN;
    last_minute_ = -1;
    std::fill(std::begin(weather_), std::end(weather_), 0xFF);
    env.ledger.set_listener([](void* c, const BookingRecord& r) { static_cast<EconomyLog*>(c)->on_booking(r); },
                            [](void* c, const DemandRecord& r) { static_cast<EconomyLog*>(c)->on_demand(r); }, this);
    env.projects.set_listener([](void* c, const ProjectRecord& r) { static_cast<EconomyLog*>(c)->on_project(r); }, this);
    return true;
}

// B10 (seit 0.5.11): Zähler nach Besitzer getrennt. Der Erzeuger (Kern 1: row, Buchungen, cut) zählt in pst_, der
// Verbraucher (Kern 0: prepare, flush, open, scan) in st_. Jeder veröffentlicht seine Kopie kurz unter stats_m_,
// stats() liest nur die Kopien. Keine Dateizugriffe unter der Sperre.
void EconomyLog::publish(bool producer) {
    std::lock_guard<std::mutex> g(stats_m_);
    ++pub_seq_;
    if (producer) {
        pub_prod_ = pst_;
        prod_seq_ = pub_seq_;
    } else {
        pub_cons_ = st_;
        cons_seq_ = pub_seq_;
    }
}

EconomyLog::Stats EconomyLog::stats() const {
    std::lock_guard<std::mutex> g(stats_m_);
    Stats r = pub_cons_;
    r.rows = pub_prod_.rows;
    r.late = pub_prod_.late;
    r.lost = pub_prod_.lost + pub_cons_.lost;
    r.buffered = prod_seq_ > cons_seq_ ? pub_prod_.buffered : pub_cons_.buffered;  // wer zuletzt den Puffer änderte
    return r;
}

void EconomyLog::detach() {
    if (env_) {
        env_->ledger.set_listener(nullptr, nullptr, nullptr);
        env_->projects.set_listener(nullptr, nullptr);
    }
    env_ = nullptr;
    if (cur_) asset_free(cur_);
    if (pend_) asset_free(pend_);
    if (groups_) asset_free(groups_);
    if (use_) asset_free(use_);
    if (deltas_) asset_free(deltas_);
    cur_ = pend_ = nullptr;
    groups_ = nullptr;
    use_ = nullptr;
    deltas_ = nullptr;
    cap_ = cur_len_ = pend_len_ = 0;
}

int32_t EconomyLog::day_of(int64_t at_us) const {
    // Welttag ab Tag 1 des Kalenders; ohne Kalender (Simulator, Prüfungen) der Tag der Uhr ab 1
    const int64_t d = floor_div(at_us, WorldClock::kUsPerDay);
    if (env_ && env_->calendar.started()) return static_cast<int32_t>(d - env_->calendar.state().start_day + 1);
    return static_cast<int32_t>(d + 1);
}

void EconomyLog::minute_text(int64_t at_us, char* out) const {
    const int64_t s = floor_div(at_us, WorldClock::kUsPerSecond);
    const int sec = static_cast<int>(s - floor_div(s, 86400) * 86400);
    std::snprintf(out, 12, "%02u:%02u:%02u", static_cast<uint8_t>(sec / 3600 % 24), static_cast<uint8_t>(sec / 60 % 60),
                  static_cast<uint8_t>(sec % 60));
}

void EconomyLog::row(Table t, int32_t day, const char* text, int len) {
    ++pst_.rows;
    if (len <= 0 || !cur_) return;
    len = std::min(len, 0xFFFF);
    if (cur_len_ + kHead + len > cap_) {
        ++pst_.lost;  // Puffer voll (wants_save hätte vorher gespeichert)
        return;
    }
    uint8_t* p = cur_ + cur_len_;
    p[0] = t;
    std::memcpy(p + 1, &day, 4);
    p[5] = static_cast<uint8_t>(len & 0xFF);
    p[6] = static_cast<uint8_t>(len >> 8);
    std::memcpy(p + kHead, text, len);
    cur_len_ += kHead + len;
}

void EconomyLog::on_booking(const BookingRecord& r) {
    const Ledger& l = env_->ledger;
    char t[12], fb[12], tb[12], holder[12] = "", item[12] = "", line[200];
    minute_text(r.info.at_us, t);
    const char* from = r.from >= 0 ? l.store_key(r.from, fb, sizeof(fb)) : "";
    const char* to = r.to >= 0 ? l.store_key(r.to, tb, sizeof(tb)) : "";
    if (r.info.holder)
        std::snprintf(holder, sizeof(holder), "%c%u", r.info.holder >= ProjectList::kFirstId ? 'p' : 'a',
                      static_cast<unsigned>(r.info.holder));
    if (r.item) std::snprintf(item, sizeof(item), "%08lx", static_cast<unsigned long>(r.item));
    // Stufe: nachgeholt, voll (ein Lager am betretenen Ort) oder grob
    const char* stage = "grob";
    if (catching_) stage = "nachgeholt";
    else if ((r.from >= 0 && l.store_place(r.from) == visible_) || (r.to >= 0 && l.store_place(r.to) == visible_))
        stage = "voll";
    const int n = std::snprintf(line, sizeof(line), "%ld,%s,%s,%s,%s,%u,%s,%s,%u,%s,%s,%s\n",
                                static_cast<long>(day_of(r.info.at_us)), t, stage, booking_id(r.kind),
                                l.good(r.good).id, static_cast<unsigned>(r.qty), from, to,
                                static_cast<unsigned>(r.info.who), r.info.reason ? r.info.reason : "", holder, item);
    row(Moves, day_of(r.info.at_us), line, std::min<int>(n, sizeof(line) - 1));
    // Tagesaufnahme genau um Mitternacht (seit 0.5.11): Buchungen des neuen Tages im selben Schritt merken, day_rows
    // rechnet sie heraus; eine Buchung für einen schon aufgenommenen Tag zählt als late
    const int32_t day = day_of(r.info.at_us);
    if (day_ != INT32_MIN && day > day_) {
        if (r.from >= 0) note_delta(r.from, r.good, day, -static_cast<int>(r.qty));
        if (r.to >= 0) note_delta(r.to, r.good, day, r.qty);
    } else if (day_ != INT32_MIN && day < day_) {
        ++pst_.late;
    }
}

void EconomyLog::note_delta(int store, int good, int32_t day, int qty) {
    for (int i = 0; i < delta_n_; ++i)
        if (deltas_[i].store == store && deltas_[i].good == good && deltas_[i].day == day) {
            deltas_[i].qty += qty;
            return;
        }
    if (!deltas_ || delta_n_ >= kMaxDeltas) {
        ++pst_.late;  // Tagesaufnahme kann abweichen
        return;
    }
    deltas_[delta_n_++] = Delta{static_cast<int16_t>(store), static_cast<uint8_t>(good), day, qty};
}

void EconomyLog::on_demand(const DemandRecord& r) {
    const Ledger& l = env_->ledger;
    char t[12], sb[12], line[160];
    minute_text(r.at_us, t);
    const char* store = r.store >= 0 ? l.store_key(r.store, sb, sizeof(sb)) : "";
    const int n = std::snprintf(line, sizeof(line), "%ld,%s,%u,%s,%s,%s,%u,%u,%u\n",
                                static_cast<long>(day_of(r.at_us)), t, static_cast<unsigned>(r.who), store,
                                r.reason ? r.reason : "", l.good(r.good).id, static_cast<unsigned>(r.want),
                                static_cast<unsigned>(r.got), static_cast<unsigned>(r.want - r.got));
    row(Demand, day_of(r.at_us), line, std::min<int>(n, sizeof(line) - 1));
}

void EconomyLog::on_project(const ProjectRecord& r) {
    const Ledger& l = env_->ledger;
    char t[12], bt[12], built[96] = "", tools[48] = "", line[400];
    minute_text(r.end_us, t);
    minute_text(r.begin_us, bt);
    int k = 0;
    for (int i = 0; i < r.good_count && k < static_cast<int>(sizeof(built)) - 1; ++i)
        k += std::snprintf(built + k, sizeof(built) - k, "%s%s:%u", i ? " " : "", l.good(r.goods[i]).id,
                           static_cast<unsigned>(r.qty[i]));
    k = 0;
    for (int i = 0; i < r.tool_count && k < static_cast<int>(sizeof(tools)) - 1; ++i)
        k += std::snprintf(tools + k, sizeof(tools) - k, "%s%s", i ? " " : "", l.good(r.tools[i]).id);
    const int64_t dur = std::max<int64_t>(0, (r.end_us - r.begin_us) / WorldClock::kUsPerMinute);
    const auto wm = [&](ProjectWait w) { return static_cast<unsigned>(r.wait_s[static_cast<int>(w)] / 60); };
    const int n = std::snprintf(
        line, sizeof(line), "%ld,%s,%s,%s,%s,%s,%ld,%s,%lld,%u,%u,%u,%u,%u,%u,%u,%u,%s,%s\n",
        static_cast<long>(day_of(r.end_us)), t, r.def->id, r.def->place ? r.def->place : "",
        project_kind_id(r.def->kind), r.step ? r.step->id : "gesamt", static_cast<long>(day_of(r.begin_us)), bt,
        static_cast<long long>(dur), static_cast<unsigned>(r.planned_s / 60), static_cast<unsigned>(r.person_s / 60),
        wm(ProjectWait::Material), wm(ProjectWait::Skill), wm(ProjectWait::Worker), wm(ProjectWait::Weather),
        wm(ProjectWait::Rest), wm(ProjectWait::Tool), built, tools);
    row(Projects, day_of(r.end_us), line, std::min<int>(n, sizeof(line) - 1));
}

void EconomyLog::day_rows(int32_t day) {
    // Bestände je Lagerplatz und Ware am Ende des Tages (nur ungleich null, dafür alle: seit 0.5.11 bis zu jeder Ware
    // des Warenbuchs, vorher 24 je Lagerplatz, B12), dann das Wetter des Tages
    const Ledger& l = env_->ledger;
    char kb[12], line[120];
    for (int s = 0; l.ready() && s < l.stores(); ++s) {
        uint8_t g[Ledger::kMaxGoods];
        uint16_t q[Ledger::kMaxGoods];
        const int k = l.contents(s, g, q, Ledger::kMaxGoods);
        const char* key = l.store_key(s, kb, sizeof(kb));
        // seit 0.5.11: Bestand um Mitternacht = jetzt minus die Buchungen späterer Tage (auch Waren, die jetzt leer
        // sind); Waren in ihrer Reihenfolge
        int extra[kMaxDeltas];
        int ne = 0;
        for (int d = 0; d < delta_n_; ++d) {
            const Delta& x = deltas_[d];
            if (x.store != s || x.day <= day || std::find(g, g + k, x.good) != g + k) continue;
            bool dup = false;
            for (int e = 0; e < ne; ++e) dup = dup || deltas_[extra[e]].good == x.good;
            if (!dup) extra[ne++] = d;
        }
        std::sort(extra, extra + ne, [&](int a, int b) { return deltas_[a].good < deltas_[b].good; });
        for (int i = 0, e = 0; i < k || e < ne;) {
            const bool take_extra = e < ne && (i >= k || deltas_[extra[e]].good < g[i]);
            const int good = take_extra ? deltas_[extra[e++]].good : g[i];
            int qty = take_extra ? 0 : q[i++];
            for (int d = 0; d < delta_n_; ++d)
                if (deltas_[d].store == s && deltas_[d].good == good && deltas_[d].day > day) qty -= deltas_[d].qty;
            if (qty <= 0) {
                if (qty < 0) ++pst_.late;
                continue;
            }
            const int n = std::snprintf(line, sizeof(line), "%ld,%s,%s,%u,%d,%d\n", static_cast<long>(day), key,
                                        l.good(good).id, static_cast<unsigned>(qty), l.capacity(s), l.reserved(s, good));
            row(StockDay, day, line, std::min<int>(n, sizeof(line) - 1));
        }
    }
    const auto w = [&](int i) { return weather_[i] == 0xFF ? "" : weather_name(static_cast<WeatherKind>(weather_[i])); };
    const int n = std::snprintf(line, sizeof(line), "%ld,%s,%s,%s,%s\n", static_cast<long>(day), w(0), w(1), w(2), w(3));
    row(WeatherDay, day, line, std::min<int>(n, sizeof(line) - 1));
    std::fill(std::begin(weather_), std::end(weather_), 0xFF);
}

void EconomyLog::step(const Stage& stage) {
    if (!env_) return;
    const int a = stage.active();
    visible_ = a >= 0 && a < stage.count() ? place_index(stage.scene(a).name()) : -1;
    catching_ = stage.catching_up() > 0;
    const int64_t at = stage.world_us();
    const int32_t day = day_of(at);
    const int64_t m = floor_div(at, WorldClock::kUsPerMinute);
    const int minute = static_cast<int>(m - floor_div(m, 1440) * 1440);
    if (day_ != INT32_MIN && day > day_) {
        day_rows(day_);  // Tageswechsel (auch nach einem Sprung: der zuletzt bekannte Tag)
        last_minute_ = -1;
    }
    if (day_ == INT32_MIN || day != day_) last_minute_ = minute;  // erster Schritt bzw. neuer Tag: von hier an
    // Wetter beim Überschreiten von 6, 12, 18 und 22 Uhr
    for (int i = 0; i < 4; ++i)
        if (last_minute_ < kWeatherMinutes[i] && minute >= kWeatherMinutes[i])
            weather_[i] = static_cast<uint8_t>(env_->weather.kind());
    if (day_ == INT32_MIN || day >= day_) day_ = day;
    last_minute_ = minute;
    // Buchungen späterer Tage, die nicht mehr nach dem laufenden Tag kommen, sind aufgenommen
    int w = 0;
    for (int d = 0; d < delta_n_; ++d)
        if (deltas_[d].day > day_) deltas_[w++] = deltas_[d];
    delta_n_ = w;
    publish(true);
}

void EconomyLog::cut() {
    if (!cur_len_) return;
    if (!pend_len_) {
        std::swap(cur_, pend_);
        pend_len_ = cur_len_;
    } else {
        // das letzte Anhängen schlug fehl: dahinter anhängen, was passt
        size_t k = 0;
        while (k + kHead <= cur_len_) {
            const size_t len = kHead + (cur_[k + 5] | (cur_[k + 6] << 8));
            if (pend_len_ + len > cap_) {
                for (size_t j = k; j + kHead <= cur_len_; j += kHead + (cur_[j + 5] | (cur_[j + 6] << 8))) ++pst_.lost;
                break;
            }
            std::memcpy(pend_ + pend_len_, cur_ + k, len);
            pend_len_ += len;
            k += len;
        }
    }
    cur_len_ = 0;
    pst_.buffered = static_cast<uint32_t>(pend_len_);
    publish(true);
}

// --- Anhängen (seit 0.5.11: Plan, Journal, Größenprüfung je Datei) ---


int EconomyLog::plan(const char* dir) {
    planned_ = 0;
    journal_ = false;
    st_.full = false;
    if (!pend_len_ || !groups_) return 0;
    // 1. Tagesgrenze (B08): je Tag, was in seinen Dateien steht, plus die neuen Zeilen in Reihenfolge; was nicht mehr
    //    passt, wird gezählt statt geschrieben (Tagesbestände, Wetter und Vermerke zählen nicht mit)
    DayUse* use = use_;
    int days = 0;
    Buf path(320);
    if (!path) return 0;
    for (size_t k = 0; k + kHead <= pend_len_;) {
        const size_t len = pend_[k + 5] | (pend_[k + 6] << 8);
        const uint8_t t = pend_[k];
        if (t <= Demand) {
            int32_t day;
            std::memcpy(&day, pend_ + k + 1, 4);
            int i = 0;
            while (i < days && use[i].day != day) ++i;
            if (i == days && days < kMaxDays) {
                use[i] = DayUse{day, 0, {0, 0, 0}, {0, 0, 0}};
                for (int c = Moves; c <= Demand; ++c) {  // neue Dateien mit ihrer Kopfzeile
                    day_path(path.p, path.n, dir, day, c);
                    const long size = file_size(path.p);
                    use[i].used += static_cast<uint32_t>(size >= 0 ? size : header_text(c, source_, path.p, path.n));
                }
                ++days;
            }
            if (i < days) {
                if (use[i].used + kHead + len > kMaxDayBytes) {
                    pend_[k] = kDropped;
                    ++use[i].rows[t];
                    use[i].bytes[t] += static_cast<uint32_t>(len);
                    ++st_.dropped;
                } else {
                    use[i].used += static_cast<uint32_t>(len);
                }
            }
        }
        k += kHead + len;
    }
    // Vermerk je Tag und Tabelle mit verworfenen Zeilen (direkt in den ausstehenden Puffer, soweit Platz ist)
    for (int i = 0; i < days; ++i)
        for (int c = Moves; c <= Demand; ++c) {
            if (!use[i].rows[c]) continue;
            char line[96];
            const int n = std::snprintf(line, sizeof(line), "%ld,%s,%u,%u\n", static_cast<long>(use[i].day),
                                        kTableNames[c], static_cast<unsigned>(use[i].rows[c]),
                                        static_cast<unsigned>(use[i].bytes[c]));
            if (pend_len_ + kHead + n > cap_) continue;
            uint8_t* p = pend_ + pend_len_;
            p[0] = Lost;
            std::memcpy(p + 1, &use[i].day, 4);
            p[5] = static_cast<uint8_t>(n);
            p[6] = 0;
            std::memcpy(p + kHead, line, n);
            pend_len_ += kHead + n;
        }
    // 2. Gruppen je (Tabelle, Tag) in der Reihenfolge des ersten Auftretens, mit der Größe der Datei davor
    int last = -1;
    for (size_t k = 0; k + kHead <= pend_len_;) {
        const size_t len = pend_[k + 5] | (pend_[k + 6] << 8);
        const uint8_t t = pend_[k];
        k += kHead + len;
        if (t >= kTables) continue;
        int32_t day;
        std::memcpy(&day, pend_ + k - kHead - len + 1, 4);
        int g = last >= 0 && groups_[last].table == t && groups_[last].day == day ? last : -1;
        for (int i = 0; g < 0 && i < planned_; ++i)
            if (groups_[i].table == t && groups_[i].day == day) g = i;
        if (g < 0) {
            if (planned_ >= kMaxGroups) continue;  // bleibt für das nächste Mal
            g = planned_++;
            groups_[g] = Group{t, 0, 0, day, 0, 0};
        }
        ++groups_[g].rows;
        groups_[g].bytes += static_cast<uint32_t>(len);
        last = g;
    }
    Buf head(400);
    if (!head) return planned_ = 0;
    for (int g = 0; g < planned_; ++g) {
        day_path(path.p, path.n, dir, groups_[g].day, groups_[g].table);
        const long size = file_size(path.p);
        groups_[g].before = size < 0 ? kFresh : static_cast<uint32_t>(size);
        if (size < 0) groups_[g].bytes += static_cast<uint32_t>(header_text(groups_[g].table, source_, head.p, head.n));
    }
    return planned_;
}

bool EconomyLog::make_room(const char* dir, uint32_t need, int groups) {
    // Grenze vor dem Anhängen (B08): älteste Tage löschen, alte Zeitlinien zuerst, nie einen Tag, an den gerade
    // angehängt wird; mit set_free_space bleibt kReserveBytes frei für die Weltstände
    Buf alt(320);
    Buf sub(340);
    if (!alt || !sub) return false;
    std::snprintf(alt.p, alt.n, "%.300s-alt", dir);
    for (int guard = 0; guard < 2000; ++guard) {
        scan(dir);
        const uint64_t total = static_cast<uint64_t>(st_.bytes) + st_.archive_bytes + need;
        const int64_t free = free_fn_ ? free_fn_(free_ctx_) : INT64_MAX;
        if (total <= kMaxBytes && free - static_cast<int64_t>(need) >= static_cast<int64_t>(kReserveBytes)) return true;
        if (st_.archives) {
            // älteste alte Zeitlinie: ihr ältester Tag, ist sie leer, der Ordner selbst
            int oldest = 0;
            for_each_dir(alt.p, [&](const char* name) {
                const int n = std::atoi(name);
                if (n > 0 && (!oldest || n < oldest)) oldest = n;
            });
            std::snprintf(sub.p, sub.n, "%s/%d", alt.p, oldest);
            const int32_t day = oldest_day(sub.p);
            if (day < 0) {
                remove_all(sub.p);
                if (rmdir(sub.p) != 0) st_.cleanup_error = true;
            } else if (!delete_day(sub.p, day, &st_.deleted)) {
                st_.cleanup_error = true;
                return false;
            }
            continue;
        }
        if (!st_.files) break;
        bool busy = false;
        for (int g = 0; g < groups; ++g) busy = busy || groups_[g].day == st_.first_day;
        if (busy) break;
        if (!delete_day(dir, st_.first_day, &st_.deleted)) {
            st_.cleanup_error = true;
            return false;
        }
    }
    return false;
}

bool EconomyLog::write_journal(const char* dir, uint32_t generation, int groups) {
    Buf path(320);
    Buf head(400);
    if (!path || !head) return false;
    std::snprintf(path.p, path.n, "%.300s/%s", dir, kJournal);
    // zweimal durch den Plan: erst die Prüfsumme, dann schreiben
    uint32_t crc = 0, payload = 0;
    for (int pass = 0; pass < 2; ++pass) {
        FILE* f = nullptr;
        if (pass == 1) {
            f = f_open(path.p, "wb");
            if (!f) return false;
        }
        bool ok = true;
        auto put = [&](const void* p, size_t n) {
            if (pass == 0) {
                crc = crc32(static_cast<const uint8_t*>(p), n, crc);
                payload += static_cast<uint32_t>(n);
            } else {
                ok = f_write(f, p, n) && ok;
            }
        };
        uint8_t top[28];
        std::memcpy(top, "PXJ1", 4);
        put32(top + 4, crc);
        put32(top + 8, world_);
        put32(top + 12, line_);
        put32(top + 16, generation);
        put32(top + 20, static_cast<uint32_t>(groups));
        put32(top + 24, payload);
        if (pass == 1) put(top, sizeof(top));
        for (int g = 0; g < groups; ++g) {
            const Group& gr = groups_[g];
            uint8_t gh[16];
            gh[0] = gr.table;
            gh[1] = 0;
            gh[2] = static_cast<uint8_t>(gr.rows);
            gh[3] = static_cast<uint8_t>(gr.rows >> 8);
            put32(gh + 4, static_cast<uint32_t>(gr.day));
            put32(gh + 8, gr.before);
            put32(gh + 12, gr.bytes);
            put(gh, sizeof(gh));
            if (gr.before == kFresh) put(head.p, header_text(gr.table, source_, head.p, head.n));
            each_row(pend_, pend_len_, gr.table, gr.day, [&](const uint8_t* p, size_t n) { put(p, n); });
        }
        if (pass == 1) {
            ok = f_close(f) && ok;
            if (!ok) std::remove(path.p);
            st_.journal_bytes = ok ? payload + 28 : 0;
            return ok;
        }
    }
    return false;
}

bool EconomyLog::prepare(const char* dir, uint32_t generation) {
    const Publisher pub(*this);  // B10: Zähler des Verbrauchers am Ende veröffentlichen
    if (!pend_len_) return false;
    make_dir(dir);
    const int groups = plan(dir);
    if (!groups) return false;
    uint32_t need = 0;
    for (int g = 0; g < groups; ++g) need += groups_[g].bytes;
    if (!make_room(dir, 2 * need + 64, groups)) {
        st_.full = true;
        planned_ = 0;  // nichts anhängen: die Zeilen bleiben (läuft der Puffer voll, zählt lost)
        return false;
    }
    journal_ = write_journal(dir, generation, groups);
    return journal_;
}

void EconomyLog::abandon() {
    // Speicherstand nicht geschrieben: alles bleibt im Puffer und kommt mit dem nächsten Stand (das Journal gilt dann
    // nicht, open() verwirft es über die Generation). Was der Tag nicht mehr fasste, bleibt verworfen, sein Vermerk
    // steht im Puffer
    planned_ = -1;
    journal_ = false;
}

bool EconomyLog::flush(const char* dir) {
    const Publisher pub(*this);  // B10: Zähler des Verbrauchers am Ende veröffentlichen
    if (!pend_len_) return true;
    const int64_t t0 = now_ms();
    make_dir(dir);
    if (planned_ < 0) {  // ohne prepare (Simulator): Plan und Platz jetzt, ohne Journal
        const int groups = plan(dir);
        uint32_t need = 0;
        for (int g = 0; g < groups; ++g) need += groups_[g].bytes;
        if (groups && !make_room(dir, need, groups)) {
            st_.full = true;
            planned_ = 0;
        }
    }
    Buf path(320);
    Buf head(400);
    if (!path || !head) return false;  // Plan und Zeilen bleiben für das nächste Mal
    bool ok = !st_.full;
    uint32_t written = 0;
    for (int g = 0; g < planned_; ++g) {
        Group& gr = groups_[g];
        day_path(path.p, path.n, dir, gr.day, gr.table);
        const Put r = put_file(path.p, gr.before, gr.bytes, [&](FILE* f) {
            bool w = true;
            if (gr.before == kFresh) w = f_write(f, head.p, header_text(gr.table, source_, head.p, head.n));
            each_row(pend_, pend_len_, gr.table, gr.day, [&](const uint8_t* p, size_t n) { w = w && f_write(f, p, n); });
            return w;
        });
        gr.state = r == Put::Fail ? 2 : 1;
        if (r == Put::Fail) ok = false;
        if (r == Put::Written) written += gr.rows;
        if (r == Put::Skipped) st_.lost += gr.rows;
    }
    // geschriebene (und schon vorhandene) Zeilen und verworfene weg; die fehlgeschlagener Dateien bleiben
    for (int g = 0; g < planned_; ++g)
        if (groups_[g].state == 1)
            for (size_t k = 0; k + kHead <= pend_len_; k += kHead + (pend_[k + 5] | (pend_[k + 6] << 8))) {
                int32_t day;
                std::memcpy(&day, pend_ + k + 1, 4);
                if (pend_[k] == groups_[g].table && day == groups_[g].day) pend_[k] = kWritten;
            }
    size_t w = 0;
    for (size_t k = 0; k + kHead <= pend_len_;) {
        const size_t len = kHead + (pend_[k + 5] | (pend_[k + 6] << 8));
        if (pend_[k] < kTables) {
            std::memmove(pend_ + w, pend_ + k, len);
            w += len;
        }
        k += len;
    }
    pend_len_ = w;
    if (ok && journal_) {
        std::snprintf(path.p, path.n, "%.300s/%s", dir, kJournal);
        std::remove(path.p);  // bleibt es (Fehler), stimmt es noch: open() fände alles schon da
    }
    planned_ = -1;
    journal_ = false;
    st_.error = !ok;
    st_.written += written;
    st_.buffered = static_cast<uint32_t>(pend_len_);
    scan(dir);
    st_.flush_ms = static_cast<uint32_t>(now_ms() - t0);
    return ok;
}

void EconomyLog::recover(const char* dir, uint32_t loaded_gen) {
    Buf path(320);
    Buf file(320);
    if (!path || !file) return;  // das Journal bleibt für den nächsten Start
    std::snprintf(path.p, path.n, "%.300s/%s", dir, kJournal);
    const long size = file_size(path.p);
    if (size < 0) return;
    uint8_t* buf = size >= 28 && size < 8 * 1024 * 1024 ? static_cast<uint8_t*>(asset_alloc(static_cast<size_t>(size))) : nullptr;
    bool ok = false;
    if (buf) {
        if (FILE* f = std::fopen(path.p, "rb")) {
            ok = std::fread(buf, 1, static_cast<size_t>(size), f) == static_cast<size_t>(size);
            std::fclose(f);
        }
    }
    ok = ok && std::memcmp(buf, "PXJ1", 4) == 0 && get32(buf + 24) == static_cast<uint32_t>(size - 28) &&
         crc32(buf + 28, static_cast<size_t>(size - 28)) == get32(buf + 4);
    // Ein Stapel gehört zum Stand seiner Generation: Ist der geladene Stand mindestens so neu und aus derselben
    // Zeitlinie, stehen seine Buchungen in der Welt und gehören genau einmal ins Protokoll; sonst wurde der Stand nie
    // geschrieben (die Welt bucht neu) oder er ist aus einer anderen Zeitlinie
    const bool mine = ok && get32(buf + 8) == world_ && get32(buf + 12) == line_ && get32(buf + 16) != 0 &&
                      get32(buf + 16) <= loaded_gen;
    const int groups = ok ? static_cast<int>(get32(buf + 20)) : 0;
    for (size_t at = 28, g = 0; ok && g < static_cast<size_t>(groups) && at + 16 <= static_cast<size_t>(size); ++g) {
        const uint8_t* gh = buf + at;
        const int table = gh[0];
        const uint16_t rows = static_cast<uint16_t>(gh[2] | gh[3] << 8);
        const uint32_t before = get32(gh + 8), bytes = get32(gh + 12);
        at += 16;
        if (table >= kTables || bytes > static_cast<size_t>(size) - at) break;
        if (!mine) {
            st_.discarded += rows;
        } else {
            day_path(file.p, file.n, dir, static_cast<int32_t>(get32(gh + 4)), table);
            const uint8_t* body = buf + at;
            const Put r = put_file(file.p, before, bytes, [&](FILE* f) { return f_write(f, body, bytes); });
            if (r == Put::Written) st_.recovered += rows;
            if (r == Put::Fail || r == Put::Skipped) st_.lost += rows;
        }
        at += bytes;
    }
    if (buf) asset_free(buf);
    std::remove(path.p);
}

void EconomyLog::open(const char* dir, uint32_t world, uint32_t line, uint32_t loaded_gen) {
    const Publisher pub(*this);  // B10: Zähler des Verbrauchers am Ende veröffentlichen
    make_dir(dir);
    world_ = world;
    line_ = line;
    recover(dir, loaded_gen);
    // Zeitlinie des Ordners (B09): passt sie nicht, wandert er nach <dir>-alt/<n> (n = nächste freie Nummer)
    Buf path(320);
    Buf alt(320);
    Buf to(340);
    if (!path || !alt || !to) return;
    std::snprintf(path.p, path.n, "%.300s/%s", dir, kMarker);
    unsigned long w = 0, l = 0;
    bool marked = false;
    if (FILE* f = std::fopen(path.p, "rb")) {
        marked = std::fscanf(f, "#pixelia-zeitlinie,welt=%lx,linie=%lx", &w, &l) == 2;
        std::fclose(f);
    }
    if (marked && (w != world || l != line)) {
        std::snprintf(alt.p, alt.n, "%.300s-alt", dir);
        make_dir(alt.p);
        int n = 0;
        for_each_dir(alt.p, [&](const char* name) { n = std::max(n, std::atoi(name)); });
        std::snprintf(to.p, to.n, "%s/%d", alt.p, n + 1);
        if (std::rename(dir, to.p) == 0) {
            make_dir(dir);
            marked = false;
        } else {
            st_.cleanup_error = true;  // bleibt gemischt: lieber weiter protokollieren als nichts
        }
    }
    if (!marked)
        if (FILE* f = std::fopen(path.p, "wb")) {
            std::fprintf(f, "#pixelia-zeitlinie,welt=%08lx,linie=%08lx\n", static_cast<unsigned long>(world),
                         static_cast<unsigned long>(line));
            std::fclose(f);
        }
    scan(dir);
}

void EconomyLog::scan(const char* dir) {
    const Publisher pub(*this);  // B10: Zähler des Verbrauchers am Ende veröffentlichen
    st_.files = st_.bytes = 0;
    st_.first_day = st_.last_day = 0;
    Buf path(640);
    if (!path) return;
    for_each_file(dir, [&](const char* name, long size) {
        st_.bytes += static_cast<uint32_t>(size);  // alles zählt zur Grenze (auch Marke und Journal)
        const int32_t day = file_day(name);
        if (day < 0) return;
        if (!st_.files || day < st_.first_day) st_.first_day = day;
        if (!st_.files || day > st_.last_day) st_.last_day = day;
        ++st_.files;
    });
    // alte Zeitlinien (B09): Ordner <dir>-alt/<n>
    st_.archives = st_.archive_bytes = 0;
    Buf alt(320);
    std::snprintf(alt.p, alt.n, "%.300s-alt", dir);
    for_each_dir(alt.p, [&](const char* name) {
        if (std::atoi(name) <= 0) return;
        ++st_.archives;
        std::snprintf(path.p, path.n, "%s/%s", alt.p, name);
        for_each_file(path.p, [&](const char*, long size) { st_.archive_bytes += static_cast<uint32_t>(size); });
    });
}

}  // namespace pixelia
