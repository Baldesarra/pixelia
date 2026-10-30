// Wirtschafts-Protokoll (seit 0.5.10 Teil 4): CSV je Welttag und Tabelle; siehe econ_log.h.

#include "pixelia/engine/econ_log.h"

#include <dirent.h>
#include <sys/stat.h>

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "pixelia/engine/environment.h"
#include "pixelia/engine/ledger.h"
#include "pixelia/engine/memory.h"
#include "pixelia/engine/place.h"
#include "pixelia/engine/projects.h"
#include "pixelia/engine/stage.h"
#include "pixelia/engine/world_clock.h"

namespace pixelia {

namespace {

const char* const kTableNames[] = {"bewegungen", "projekte", "bedarf", "bestand_lager_tag", "wetter_tag"};
static_assert(sizeof(kTableNames) / sizeof(kTableNames[0]) == EconomyLog::kTables);
const char* const kColumns[] = {
    "welttag,zeit,stufe,art,ware,menge,von,nach,person,grund,vorhaben,stueck",
    "welttag,zeit,projekt,ort,art,abschnitt,beginn_tag,beginn_zeit,dauer_min,geplant_min,personen_min,warten_material,"
    "warten_faehigkeit,warten_arbeitskraft,warten_wetter,warten_feierabend,warten_werkzeug,verbaut,werkzeug",
    "welttag,zeit,person,lager,zweck,ware,gewollt,bekommen,fehlmenge",
    "welttag,lager,ware,menge,obergrenze,reserviert",
    "welttag,morgens,mittags,abends,nachts",
};
constexpr int kWeatherMinutes[4] = {6 * 60, 12 * 60, 18 * 60, 22 * 60};  // morgens, mittags, abends, nachts
constexpr size_t kHead = 7;  // Eintrag im Puffer: Tabelle (1), Welttag (4), Länge (2), dann der Text

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

}  // namespace

const char* EconomyLog::table_name(int t) { return t >= 0 && t < kTables ? kTableNames[t] : "?"; }

EconomyLog::~EconomyLog() { detach(); }

bool EconomyLog::attach(Environment& env, size_t buffer_bytes, const char* source) {
    detach();
    cur_ = static_cast<uint8_t*>(asset_alloc(buffer_bytes));
    pend_ = static_cast<uint8_t*>(asset_alloc(buffer_bytes));
    if (!cur_ || !pend_) {
        detach();
        return false;
    }
    cap_ = buffer_bytes;
    cur_len_ = pend_len_ = 0;
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

void EconomyLog::detach() {
    if (env_) {
        env_->ledger.set_listener(nullptr, nullptr, nullptr);
        env_->projects.set_listener(nullptr, nullptr);
    }
    env_ = nullptr;
    if (cur_) asset_free(cur_);
    if (pend_) asset_free(pend_);
    cur_ = pend_ = nullptr;
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
    ++st_.rows;
    if (len <= 0 || !cur_) return;
    len = std::min(len, 0xFFFF);
    if (cur_len_ + kHead + len > cap_) {
        ++st_.lost;  // Puffer voll (wants_save hätte vorher gespeichert)
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
    // Bestände je Lagerplatz und Ware am Ende des Tages (nur ungleich null), dann das Wetter des Tages
    const Ledger& l = env_->ledger;
    char kb[12], line[120];
    for (int s = 0; l.ready() && s < l.stores(); ++s) {
        uint8_t g[24];
        uint16_t q[24];
        const int k = l.contents(s, g, q, 24);
        const char* key = l.store_key(s, kb, sizeof(kb));
        for (int i = 0; i < k; ++i) {
            const int n = std::snprintf(line, sizeof(line), "%ld,%s,%s,%u,%d,%d\n", static_cast<long>(day), key,
                                        l.good(g[i]).id, static_cast<unsigned>(q[i]), l.capacity(s), l.reserved(s, g[i]));
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
                for (size_t j = k; j + kHead <= cur_len_; j += kHead + (cur_[j + 5] | (cur_[j + 6] << 8))) ++st_.lost;
                break;
            }
            std::memcpy(pend_ + pend_len_, cur_ + k, len);
            pend_len_ += len;
            k += len;
        }
    }
    cur_len_ = 0;
    st_.buffered = static_cast<uint32_t>(pend_len_);
}

bool EconomyLog::flush(const char* dir) {
    if (!pend_len_) return true;
    make_dir(dir);  // gibt es ihn schon, schlägt das still fehl
    // je (Tabelle, Tag) die Datei einmal öffnen und alle ihre Zeilen in Reihenfolge anhängen
    bool ok = true;
    uint32_t written = 0;
    char path[320];
    for (size_t k = 0; k + kHead <= pend_len_;) {
        const size_t len = pend_[k + 5] | (pend_[k + 6] << 8);
        const uint8_t t = pend_[k];
        if (t >= kTables) {  // schon geschrieben
            k += kHead + len;
            continue;
        }
        int32_t day;
        std::memcpy(&day, pend_ + k + 1, 4);
        std::snprintf(path, sizeof(path), "%.200s/t%05ld-%s.csv", dir, static_cast<long>(std::clamp<int32_t>(day, 0, 99999)),
                      kTableNames[t]);
        struct stat sb;
        const bool fresh = stat(path, &sb) != 0;
        FILE* f = std::fopen(path, "ab");
        if (!f) {
            ok = false;
            break;
        }
        if (fresh)
            std::fprintf(f, "#pixelia-wirtschaft,format=%d,tabelle=%s,quelle=%s\n%s\n", kFormat, kTableNames[t], source_,
                         kColumns[t]);
        for (size_t j = k; j + kHead <= pend_len_;) {
            const size_t lj = pend_[j + 5] | (pend_[j + 6] << 8);
            int32_t dj;
            std::memcpy(&dj, pend_ + j + 1, 4);
            if (pend_[j] == t && dj == day) {
                ok = std::fwrite(pend_ + j + kHead, 1, lj, f) == lj && ok;
                pend_[j] = 0xFF;
                ++written;
            }
            j += kHead + lj;
        }
        ok = std::fclose(f) == 0 && ok;
        if (!ok) break;
        k += kHead + len;
    }
    st_.error = !ok;
    if (!ok) {
        // Was schon in den Dateien steht, ist markiert (0xFF): nur der Rest bleibt für das nächste Mal
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
    } else {
        pend_len_ = 0;
    }
    st_.written += written;
    st_.buffered = static_cast<uint32_t>(pend_len_);
    enforce(dir);
    return ok;
}

void EconomyLog::scan(const char* dir) {
    st_.files = st_.bytes = 0;
    st_.first_day = st_.last_day = 0;
    DIR* d = opendir(dir);
    if (!d) return;
    char path[320];
    while (dirent* e = readdir(d)) {
        const int32_t day = file_day(e->d_name);
        if (day < 0) continue;
        std::snprintf(path, sizeof(path), "%.100s/%.200s", dir, e->d_name);
        struct stat sb;
        if (stat(path, &sb) != 0) continue;
        if (!st_.files || day < st_.first_day) st_.first_day = day;
        if (!st_.files || day > st_.last_day) st_.last_day = day;
        ++st_.files;
        st_.bytes += static_cast<uint32_t>(sb.st_size);
    }
    closedir(d);
}

bool EconomyLog::enforce(const char* dir) {
    // über kMaxBytes: alle Dateien des ältesten Tages löschen, solange mehr als ein Tag da ist
    for (int guard = 0; guard < 400; ++guard) {
        scan(dir);
        if (st_.bytes <= kMaxBytes || st_.first_day >= st_.last_day) return true;
        // erst die Namen sammeln, dann löschen (nicht während des Lesens des Ordners)
        DIR* d = opendir(dir);
        if (!d) return false;
        char names[kTables + 3][40];
        int n = 0;
        while (dirent* e = readdir(d))
            if (file_day(e->d_name) == st_.first_day && n < kTables + 3)
                std::snprintf(names[n++], sizeof(names[0]), "%.39s", e->d_name);
        closedir(d);
        if (n == 0) return false;
        char path[320];
        for (int i = 0; i < n; ++i) {
            std::snprintf(path, sizeof(path), "%.100s/%.39s", dir, names[i]);
            if (std::remove(path) == 0) ++st_.deleted;
        }
    }
    return false;
}

}  // namespace pixelia
