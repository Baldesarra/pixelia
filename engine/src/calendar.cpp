#include "pixelia/engine/calendar.h"

#include <algorithm>
#include <cmath>

namespace pixelia {

// Kalenderrechnung nach Howard Hinnant („chrono-compatible low-level date algorithms“), proleptisch gregorianisch
int64_t days_from_civil(int y, int m, int d) {
    y -= m <= 2;
    const int64_t era = (y >= 0 ? y : y - 399) / 400;
    const int64_t yoe = y - era * 400;
    const int64_t doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const int64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

void civil_from_days(int64_t z, int* y, int* m, int* d) {
    z += 719468;
    const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    const int64_t doe = z - era * 146097;
    const int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const int64_t mp = (5 * doy + 2) / 153;
    *d = static_cast<int>(doy - (153 * mp + 2) / 5 + 1);
    *m = static_cast<int>(mp < 10 ? mp + 3 : mp - 9);
    *y = static_cast<int>(yoe + era * 400 + (*m <= 2));
}

const char* season_name(Season s) {
    constexpr const char* k[] = {"fruehling", "sommer", "herbst", "winter"};
    return k[static_cast<int>(s) & 3];
}

const char* season_title(Season s) {
    constexpr const char* k[] = {"Frühling", "Sommer", "Herbst", "Winter"};
    return k[static_cast<int>(s) & 3];
}

const char* season_genitive(Season s) {
    constexpr const char* k[] = {"des Frühlings", "des Sommers", "des Herbstes", "des Winters"};
    return k[static_cast<int>(s) & 3];
}

// --- Zeitfenster ---
namespace {
constexpr TimeWindow kWindows[kDayParts] = {
    {60, 300},     // Nacht 1–5: alle schlafen
    {300, 450},    // früher Morgen 5–7:30: aufstehen
    {450, 720},    // Vormittag
    {720, 840},    // Mittag 12–14
    {840, 1080},   // Nachmittag
    {1080, 1320},  // Abend 18–22
    {1320, 60},    // später Abend 22–1: zu Bett, je nach Charakter
};
}  // namespace

TimeWindow day_part_window(DayPart p) { return kWindows[static_cast<int>(p) % kDayParts]; }

DayPart day_part(int minute) {
    minute = ((minute % 1440) + 1440) % 1440;
    for (int i = 0; i < kDayParts; ++i)
        if (kWindows[i].contains(minute)) return static_cast<DayPart>(i);
    return DayPart::Night;
}

const char* day_part_name(DayPart p) {
    constexpr const char* k[] = {"nacht", "frueh", "vormittag", "mittag", "nachmittag", "abend", "spaet"};
    return k[static_cast<int>(p) % kDayParts];
}

const char* day_part_title(DayPart p) {
    constexpr const char* k[] = {"Nacht", "früher Morgen", "Vormittag", "Mittag", "Nachmittag", "Abend", "später Abend"};
    return k[static_cast<int>(p) % kDayParts];
}

SleepTimes sleep_times(Chronotype c) {
    switch (c) {
        case Chronotype::Early: return {22 * 60, 5 * 60};
        case Chronotype::Late: return {1 * 60, 7 * 60 + 30};
        default: return {23 * 60, 6 * 60 + 15};
    }
}

int64_t weather_slot_key(int64_t day, int minute) {
    if (minute < kSlotStart[0]) return (day - 1) * 4 + 3;  // 0–5 Uhr: Nacht des Vortags
    int s = 0;
    while (s < 3 && minute >= kSlotStart[s + 1]) ++s;
    return day * 4 + s;
}

SunTimes sun_times(double phase) {
    constexpr double kPi = 3.14159265358979323846;
    constexpr double kNoon = 12 * 60 + 45, kAmplitude = 4 * 60 + 18;
    const double half = (12 * 60 + kAmplitude * std::sin(2 * kPi * phase)) / 2;
    SunTimes s;
    s.rise = static_cast<int>(std::lround(kNoon - half));
    s.set = static_cast<int>(std::lround(kNoon + half));
    return s;
}

// --- Kalender ---
void WorldCalendar::start(int64_t epoch_day, int season_len) {
    st_ = CalendarState{};
    st_.season_len = static_cast<uint8_t>(std::clamp(season_len, kMinSeasonDays, kMaxSeasonDays));
    st_.start_day = static_cast<int32_t>(epoch_day);
    st_.anchor_first = st_.start_day;
    st_.anchor_season = 0;
    st_.anchor_min_last = st_.start_day;
    st_.firsts_year = 1;
    cache_no_ = -1;
    last_day_ = INT64_MIN;
    ++revision_;
}

void WorldCalendar::set_state(const CalendarState& s) {
    st_ = s;
    st_.version = CalendarState::kVersion;
    st_.season_len = static_cast<uint8_t>(std::clamp<int>(st_.season_len, kMinSeasonDays, kMaxSeasonDays));
    cache_no_ = -1;
    last_day_ = INT64_MIN;
}

int64_t WorldCalendar::season_end(int64_t first, int64_t min_last) const {
    int64_t last = std::max<int64_t>(first + st_.season_len - 1, min_last);
    if (st_.season_len == 7)
        while (weekday_of(last) != 0) ++last;  // bis Sonntag: gewechselt wird in der Nacht auf Montag
    return last;
}

void WorldCalendar::season_at(int64_t day, int* no, int64_t* first, int64_t* last) const {
    int n = st_.anchor_season;
    int64_t f = st_.anchor_first, l = season_end(f, st_.anchor_min_last);
    if (cache_no_ >= 0 && day >= cache_first_) {
        n = cache_no_;
        f = cache_first_;
        l = cache_last_;
    }
    while (day > l) {
        f = l + 1;
        l = season_end(f, f);
        ++n;
    }
    if (day >= f) {  // vor dem Anker bleibt es bei der Anker-Jahreszeit (Testuhr zurückgedreht)
        cache_no_ = n;
        cache_first_ = f;
        cache_last_ = l;
    }
    *no = n;
    *first = f;
    *last = l;
}

Season WorldCalendar::season_of(int64_t day) const {
    if (!started()) return Season::Spring;
    int no;
    int64_t f, l;
    season_at(day, &no, &f, &l);
    return static_cast<Season>(no & 3);
}

void WorldCalendar::compute(int64_t day, int minute) {
    CalendarDate& d = date_;
    d.started = started();
    d.epoch_day = day;
    d.minute = minute;
    d.weekday = weekday_of(day);
    if (!d.started) {
        d.day = 1;
        d.year = 1;
        d.season_no = 0;
        d.season = Season::Spring;
        d.day_in_season = 1;
        d.season_days = st_.season_len;
        d.season_first = d.season_last = day;
        d.season_evening = false;
        d.year_phase = 0;
        d.sun = sun_times(0.125);
        return;
    }
    int no;
    int64_t f, l;
    season_at(day, &no, &f, &l);
    d.season_no = no;
    d.season = static_cast<Season>(no & 3);
    d.year = no / 4 + 1;
    d.day = static_cast<int>(std::max<int64_t>(1, day - st_.start_day + 1));
    d.season_first = f;
    d.season_last = l;
    d.season_days = static_cast<int>(l - f + 1);
    d.day_in_season = static_cast<int>(std::clamp<int64_t>(day - f + 1, 1, d.season_days));
    d.season_evening = day == l;
    const double in = (d.day_in_season - 1 + minute / 1440.0) / d.season_days;
    d.year_phase = ((no & 3) + std::clamp(in, 0.0, 1.0)) / 4.0;
    d.sun = sun_times(d.year_phase);
}

void WorldCalendar::update(int64_t day, int minute) {
    if (day == last_day_ && minute == last_minute_) return;
    const bool first_call = last_day_ == INT64_MIN;
    const int64_t before = first_call ? 0 : last_day_ * 1440 + last_minute_;
    const int64_t now = day * 1440 + minute;
    last_day_ = day;
    last_minute_ = minute;
    compute(day, minute);
    if (pending_len_) {
        const int len = pending_len_;
        pending_len_ = 0;
        set_season_length(len);
    }
    if (!started()) return;
    if (st_.firsts_year != date_.year) {  // neues Weltjahr: Erstlinge wieder offen
        st_.firsts_year = date_.year;
        st_.firsts = 0;
        ++revision_;
    }
    if (first_call || now <= before || now - before > 120) return;
    auto crossed = [&](int at) {
        const int64_t t = day * 1440 + at;
        return before < t && t <= now;
    };
    const CalendarDate& d = date_;
    if (d.season_evening && crossed(kEveningMinute))
        push(EventKind::SeasonEvening, First::Snow, std::string("Letzter Abend ") + season_genitive(d.season));
    if (d.day_in_season == 1 && d.season_no > 0 && crossed(kMorningMinute))
        push(EventKind::SeasonBegins, First::Snow,
             std::string("Der ") + season_title(d.season) + " beginnt · " +
                 (d.season == Season::Spring ? "Weltjahr " + std::to_string(d.year) : "Tag " + std::to_string(d.day)));
}

uint16_t WorldCalendar::first_bit(First f) const {
    switch (f) {
        case First::Snow: return 1u << 0;
        case First::Storm: return 1u << 1;
        case First::AutumnFog: return 1u << 2;
        case First::FullMoon: return static_cast<uint16_t>(1u << (4 + static_cast<int>(date_.season)));
    }
    return 0;
}

bool WorldCalendar::has_first(First f) const { return (st_.firsts & first_bit(f)) != 0; }

bool WorldCalendar::first(First f) {
    if (!started() || has_first(f)) return false;
    st_.firsts = static_cast<uint16_t>(st_.firsts | first_bit(f));
    ++revision_;
    std::string text;
    switch (f) {
        case First::Snow: text = "Erster Schnee in diesem Jahr"; break;
        case First::Storm: text = "Erstes Gewitter in diesem Jahr"; break;
        case First::AutumnFog: text = "Erster Herbstnebel"; break;
        case First::FullMoon: text = std::string("Erster Vollmond ") + season_genitive(date_.season); break;
    }
    push(EventKind::First, f, text);
    return true;
}

void WorldCalendar::push(EventKind kind, First f, const std::string& text) {
    if (!bus_) return;
    WorldEvent e;
    e.kind = kind;
    e.a = static_cast<uint8_t>(date_.season);
    e.b = static_cast<uint8_t>(f);
    e.day = date_.day;
    const size_t n = std::min(text.size(), sizeof(e.text) - 1);
    text.copy(e.text, n);
    e.text[n] = 0;
    bus_->publish(e);
}

void WorldCalendar::begin_test() {
    if (testing_) return;
    saved_ = st_;
    testing_ = true;
}

void WorldCalendar::end_test() {
    if (!testing_) return;
    testing_ = false;
    // eine während des Tests geänderte Länge bleibt (Einstellung); neu verankert wird mit dem echten Datum
    const uint8_t len = st_.season_len;
    st_ = saved_;
    pending_len_ = len != saved_.season_len ? len : 0;
    cache_no_ = -1;
    last_day_ = INT64_MIN;
    ++revision_;
}

void WorldCalendar::set_season_length(int days) {
    days = std::clamp(days, kMinSeasonDays, kMaxSeasonDays);
    if (days == st_.season_len) return;
    if (started()) {
        const int64_t today = date_.started ? date_.epoch_day : st_.start_day;
        int no;
        int64_t f, l;
        season_at(today, &no, &f, &l);
        st_.anchor_first = static_cast<int32_t>(f);
        st_.anchor_season = no;
        st_.anchor_min_last = static_cast<int32_t>(today);
    }
    st_.season_len = static_cast<uint8_t>(days);
    cache_no_ = -1;
    if (date_.started) compute(date_.epoch_day, date_.minute);
    ++revision_;
}

}  // namespace pixelia
