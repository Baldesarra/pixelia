// Log-Ringpuffer (siehe logbuf.h).
//
// Aufbau eines Rings: zwei gleiche Köpfe mit Prüfsumme (bricht ein Absturz das Schreiben ab, bleibt einer gültig;
// im PSRAM zusammen genau eine Cache-Zeile) und ein Byte-Ring aus Datensätzen. Datensatz = RecHead (20 Byte) + Text,
// auf 4 Byte aufgerundet. Alle Zugriffe auf den Ring sind 32-bittig (RTC-Speicher). Reihenfolge beim Schreiben:
// Platz schaffen (älteste Datensätze verwerfen), Datensatz schreiben, dann den Kopf; im PSRAM wird beides sofort
// aus dem Cache zurückgeschrieben. Beim Start werden die Datensätze vom ältesten an nachgezählt; was nicht
// zusammenpasst, wird abgeschnitten.
//
// Die RTC-Sicherung ist langsam (gemessen 16 µs pro Zeile, ~0,3 µs pro 32-Bit-Zugriff). Sie wird deshalb nur
// zeilenweise mitgeschrieben, wenn der nächste Start eine andere Version sein kann: im Probelauf nach einem Update
// (Absturz → Rückfall). Bei jedem Software-Neustart kopiert ein Shutdown-Handler die neuesten 6 KB einmalig hinein.

#include "logbuf.h"

#include <algorithm>
#include <atomic>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <new>
#include <sys/time.h>

#include "esp_attr.h"
#include "esp_cpu.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_private/esp_cache_private.h"
#include "esp_random.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/cache_hal.h"

namespace app::log {

namespace {
constexpr const char* TAG = "log";
constexpr uint32_t kMagic = 0x474C5850;  // „PXLG“
constexpr uint32_t kFormat = 1;
constexpr time_t kValidAfter = 1735689600;  // wie timesync: vor 2025 ist die Uhr nicht gestellt
constexpr size_t kIndexSize = 16384;        // mehr als kRingBytes / kleinster Datensatz (24 Byte)

struct RecHead {
    uint16_t size;  // Datensatz gesamt, Vielfaches von 4
    uint8_t level;
    uint8_t flags;  // Bit 0: gekürzt
    uint32_t seq;
    uint32_t up_ms;
    uint32_t epoch;
    uint16_t boot;
    uint16_t len;
};
static_assert(sizeof(RecHead) == 20, "RecHead muss 20 Byte groß sein");
constexpr uint32_t kMaxRecord = (sizeof(RecHead) + kMaxText + 3) & ~3u;
// Puffer im Hook: Kopf + Text + '\n' + Nullbyte, auf Worte gerundet
constexpr size_t kRecWords = (sizeof(RecHead) + kMaxText + 2 + 3) / 4;

struct Head {
    uint32_t magic;
    uint32_t rid;
    uint32_t head, tail, used;
    uint32_t first, next;  // Nummer des ältesten Datensatzes, nächste Nummer
    uint32_t check;        // über alle Felder davor und die Lage (Format, Größe, Adresse)
};
static_assert(sizeof(Head) == 32, "zwei Köpfe sollen genau eine Cache-Zeile (64 Byte) füllen");

struct alignas(64) PsramArea {
    Head hdr[2];  // eine Cache-Zeile, die Daten beginnen auf der nächsten
    uint32_t data[kRingBytes / 4];
};
struct RtcArea {
    Head hdr[2];
    uint32_t data[kRtcBytes / 4];
};

EXT_RAM_NOINIT_ATTR PsramArea s_psram;
RTC_NOINIT_ATTR RtcArea s_rtc;

struct Ring {
    Head* hdr = nullptr;
    volatile uint32_t* data = nullptr;
    uint32_t cap = 0;
    bool cached = false;  // PSRAM: nach dem Schreiben Cache zurückschreiben
    Head h{};             // Arbeitskopie des Kopfes
};

portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
Ring s_main, s_backup;
bool s_mirror = false;        // RTC-Sicherung zeilenweise mitschreiben (Probelauf)
uint32_t* s_index = nullptr;  // Versatz jedes Datensatzes im Hauptpuffer, Platz = Nummer % kIndexSize
bool s_ready = false;
uint16_t s_boot = kBootUnknown;
uint32_t s_boot_first = 0;
int64_t s_epoch_base_ms = 0;  // Uhrzeit in ms beim Start (Laufzeit 0), 0 = unbekannt
Stats s_stats{};
const char* s_origin = "neu";
uint32_t s_prev_lines = 0;
vprintf_like_t s_prev_vprintf = nullptr;

std::atomic<TaskHandle_t> s_capture_task{nullptr};
char* s_cap_buf = nullptr;
size_t s_cap_cap = 0, s_cap_len = 0;
std::atomic<TaskHandle_t> s_serial_task{nullptr};

// --- Ring ---------------------------------------------------------------------------------------------------

uint32_t checksum(const Ring& r, const Head& h) {
    // FNV-1a über die Lage des Rings und alle Felder vor der Prüfsumme
    uint32_t x = 2166136261u;
    auto mix = [&x](uint32_t v) {
        for (int i = 0; i < 4; ++i, v >>= 8) x = (x ^ (v & 0xFF)) * 16777619u;
    };
    mix(kFormat);
    mix(r.cap);
    mix(reinterpret_cast<uint32_t>(r.hdr));
    const auto* w = reinterpret_cast<const uint32_t*>(&h);
    for (size_t i = 0; i < offsetof(Head, check) / 4; ++i) mix(w[i]);
    return x;
}

void words_out(volatile uint32_t* dst, const uint32_t* src, size_t n) {
    for (size_t i = 0; i < n; ++i) dst[i] = src[i];
}

void words_in(uint32_t* dst, const volatile uint32_t* src, size_t n) {
    for (size_t i = 0; i < n; ++i) dst[i] = src[i];
}

void sync(const Ring& r, const volatile void* p, size_t n) {
    // Wie esp_cache_msync(), aber ohne dessen Prüfungen: unter demselben Spinlock wie alle Cache-Operationen von
    // ESP-IDF direkt die Cache-Zeilen zurückschreiben
    if (!r.cached || !n) return;
    esp_cache_sync_ops_enter_critical_section();
    cache_hal_writeback_addr(reinterpret_cast<uint32_t>(p), n);
    esp_cache_sync_ops_exit_critical_section();
}

// n Byte (Vielfaches von 4) ab Versatz off schreiben bzw. lesen, über das Ende des Rings hinweg
void put(const Ring& r, uint32_t off, const uint32_t* src, uint32_t n) {
    const uint32_t a = std::min(n, r.cap - off);
    words_out(r.data + off / 4, src, a / 4);
    sync(r, r.data + off / 4, a);
    if (a < n) {
        words_out(r.data, src + a / 4, (n - a) / 4);
        sync(r, r.data, n - a);
    }
}

void get(const Ring& r, uint32_t off, uint32_t* dst, uint32_t n) {
    const uint32_t a = std::min(n, r.cap - off);
    words_in(dst, r.data + off / 4, a / 4);
    if (a < n) words_in(dst + a / 4, r.data, (n - a) / 4);
}

// Datensatzkopf lesen: erst als Worte, dann per memcpy in die Struktur. Direkt über einen uint32_t-Zeiger in die
// Struktur mit uint16_t-Feldern zu schreiben verletzt die Aliasing-Regeln von C++; der Compiler (-O2) hat dann
// size, boot und len als 0 angenommen (in der Entwicklung so passiert: alle Zeilen leer, nach Neustart 0 gültig).
RecHead read_head(const Ring& r, uint32_t off) {
    uint32_t w[sizeof(RecHead) / 4];
    get(r, off, w, sizeof(RecHead));
    RecHead rh;
    std::memcpy(&rh, w, sizeof(rh));
    return rh;
}

void store_head(Ring& r) {
    r.h.check = checksum(r, r.h);
    const auto* src = reinterpret_cast<const uint32_t*>(&r.h);
    words_out(reinterpret_cast<volatile uint32_t*>(&r.hdr[0]), src, sizeof(Head) / 4);
    words_out(reinterpret_cast<volatile uint32_t*>(&r.hdr[1]), src, sizeof(Head) / 4);
    sync(r, r.hdr, 2 * sizeof(Head));
}

void fresh(Ring& r, uint32_t start_seq) {
    r.h = Head{};
    r.h.magic = kMagic;
    r.h.rid = esp_random();
    r.h.first = r.h.next = start_seq;
    store_head(r);
}

bool head_valid(const Ring& r, const Head& h) {
    return h.magic == kMagic && h.check == checksum(r, h) && h.used <= r.cap && h.head < r.cap && h.tail < r.cap &&
           (h.head | h.tail) % 4 == 0 && h.first <= h.next;
}

bool rec_valid(const RecHead& rh, uint32_t seq, uint32_t room) {
    return rh.size >= sizeof(RecHead) + 1 && rh.size <= kMaxRecord && rh.size % 4 == 0 && rh.size <= room &&
           rh.seq == seq && rh.len <= rh.size - sizeof(RecHead) && rh.len <= kMaxText;
}

// Befund beim Start (wird nach dem Setzen des Hooks ins Log geschrieben)
char s_load_note[2][160];

// Gültigen Kopf übernehmen und alle Datensätze nachzählen; Unstimmiges am Ende abschneiden
bool load(Ring& r, bool fill_index, char* note, size_t note_len) {
    Head cand[2];
    for (int i = 0; i < 2; ++i)
        words_in(reinterpret_cast<uint32_t*>(&cand[i]), reinterpret_cast<volatile uint32_t*>(&r.hdr[i]),
                 sizeof(Head) / 4);
    const bool ok0 = head_valid(r, cand[0]), ok1 = head_valid(r, cand[1]);
    if (!ok0 && !ok1) {
        std::snprintf(note, note_len, "kein gültiger Kopf (Kennung %08lx/%08lx)", static_cast<unsigned long>(cand[0].magic),
                      static_cast<unsigned long>(cand[1].magic));
        return false;
    }
    r.h = (ok0 && (!ok1 || cand[0].next >= cand[1].next)) ? cand[0] : cand[1];

    uint32_t off = r.h.tail, seq = r.h.first, total = 0;
    RecHead bad{};
    bool broken = false;
    while (total < r.h.used) {
        const RecHead rh = read_head(r, off);
        if (!rec_valid(rh, seq, r.h.used - total)) {
            bad = rh;
            broken = true;
            break;
        }
        if (fill_index && s_index) s_index[seq % kIndexSize] = off;
        total += rh.size;
        off = (off + rh.size) % r.cap;
        ++seq;
    }
    int n = std::snprintf(note, note_len, "Kopf %c%c, Nr. %lu–%lu, %lu Byte, %lu Zeilen gültig",
                          ok0 ? 'A' : '-', ok1 ? 'B' : '-', static_cast<unsigned long>(r.h.first),
                          static_cast<unsigned long>(r.h.next), static_cast<unsigned long>(r.h.used),
                          static_cast<unsigned long>(seq - r.h.first));
    if (broken && n > 0 && static_cast<size_t>(n) < note_len)
        std::snprintf(note + n, note_len - n, ", Abbruch bei Versatz %lu (Größe %u, Nr. %lu, Länge %u)",
                      static_cast<unsigned long>(off), bad.size, static_cast<unsigned long>(bad.seq), bad.len);
    r.h.used = total;
    r.h.head = off;
    r.h.next = seq;
    store_head(r);
    return true;
}

// Datensatz (Kopf in rec[0..4], Text dahinter) anhängen; im kritischen Abschnitt aufrufen.
// Gibt den Versatz des neuen Datensatzes zurück.
uint32_t append(Ring& r, const uint32_t* rec, uint32_t size, uint32_t seq) {
    while (r.cap - r.h.used < size) {
        uint32_t w;
        get(r, r.h.tail, &w, 4);
        const uint32_t old = w & 0xFFFF;
        if (old < sizeof(RecHead) + 1 || old > kMaxRecord || old % 4 || old > r.h.used) {
            // Sollte nie passieren: Ring lieber leeren als falsch weiterzählen
            r.h.tail = r.h.head;
            r.h.used = 0;
            r.h.first = seq;
            break;
        }
        r.h.tail = (r.h.tail + old) % r.cap;
        r.h.used -= old;
        ++r.h.first;
    }
    if (r.h.used == 0) r.h.first = seq;
    const uint32_t off = r.h.head;
    put(r, off, rec, size);
    r.h.head = (off + size) % r.cap;
    r.h.used += size;
    r.h.next = seq + 1;
    store_head(r);
    return off;
}

char level_of(const char* text, size_t len) {
    if (len >= 3 && text[1] == ' ' && text[2] == '(' && std::strchr("EWIDV", text[0])) return text[0];
    return '-';
}

uint32_t cycles() { return esp_cpu_get_cycle_count(); }

// Zeile in beide Ringe; rec enthält ab Byte 20 den Text (len Zeichen). Gibt die Takte im Puffer zurück.
uint32_t store(uint32_t* rec, size_t len, bool truncated, bool force_mirror = false) {
    const uint32_t c0 = cycles();
    auto* text = reinterpret_cast<char*>(rec) + sizeof(RecHead);
    RecHead rh{};
    rh.size = static_cast<uint16_t>((sizeof(RecHead) + len + 3) & ~3u);
    rh.level = static_cast<uint8_t>(level_of(text, len));
    rh.flags = truncated ? 1 : 0;
    const int64_t up_us = esp_timer_get_time();
    rh.up_ms = static_cast<uint32_t>(up_us / 1000);
    rh.len = static_cast<uint16_t>(len);

    portENTER_CRITICAL(&s_mux);
    rh.seq = s_main.h.next;
    rh.boot = s_boot;
    rh.epoch = s_epoch_base_ms ? static_cast<uint32_t>((s_epoch_base_ms + up_us / 1000) / 1000) : 0;
    std::memcpy(rec, &rh, sizeof(rh));
    const uint32_t ca = cycles();
    const uint32_t off = append(s_main, rec, rh.size, rh.seq);
    if (s_index) s_index[rh.seq % kIndexSize] = off;
    const uint32_t cb = cycles();
    if (s_mirror || force_mirror) append(s_backup, rec, rh.size, rh.seq);
    const uint32_t cc = cycles();
    s_stats.main_cycles += cb - ca;
    s_stats.rtc_cycles += cc - cb;
    ++s_stats.stored;
    if (truncated) ++s_stats.truncated;
    if (rh.level == 'E') ++s_stats.errors;
    if (rh.level == 'W') ++s_stats.warnings;
    const uint32_t c = cycles() - c0;
    s_stats.store_cycles += c;
    if (c > s_stats.store_max_cycles) s_stats.store_max_cycles = c;
    portEXIT_CRITICAL(&s_mux);
    return c;
}

void capture_append(const char* text, size_t len) {
    if (!s_cap_buf || len + 64 > s_cap_cap) return;
    if (s_cap_len + len + 2 > s_cap_cap) {
        // Voll: vordere Hälfte verwerfen, das Ende (meist das Ergebnis eines Befehls) bleibt erhalten
        static constexpr char kCut[] = "… (Anfang der Ausgabe gekürzt)\n";
        const char* nl = static_cast<const char*>(std::memchr(s_cap_buf + s_cap_len / 2, '\n', s_cap_len - s_cap_len / 2));
        const size_t from = nl ? static_cast<size_t>(nl - s_cap_buf) + 1 : s_cap_len;
        const size_t keep = s_cap_len - from;
        std::memmove(s_cap_buf + sizeof(kCut) - 1, s_cap_buf + from, keep);
        std::memcpy(s_cap_buf, kCut, sizeof(kCut) - 1);
        s_cap_len = sizeof(kCut) - 1 + keep;
    }
    std::memcpy(s_cap_buf + s_cap_len, text, len);
    s_cap_len += len;
    s_cap_buf[s_cap_len++] = '\n';
    s_cap_buf[s_cap_len] = 0;
}

// --- Hook ---------------------------------------------------------------------------------------------------

int hook(const char* fmt, va_list args) {
    if (xPortInIsrContext() || !s_ready) return s_prev_vprintf ? s_prev_vprintf(fmt, args) : vprintf(fmt, args);
    const uint32_t c0 = cycles();
    uint32_t rec[kRecWords];
    char* text = reinterpret_cast<char*>(rec) + sizeof(RecHead);
    va_list copy;
    va_copy(copy, args);
    const int n = vsnprintf(text, kMaxText + 2, fmt, args);
    const uint32_t c1 = cycles();
    if (n < 0) {
        va_end(copy);
        return n;
    }
    const bool fits = n <= static_cast<int>(kMaxText) + 1;  // + '\n'
    size_t len = fits ? static_cast<size_t>(n) : kMaxText;
    while (len && (text[len - 1] == '\n' || text[len - 1] == '\r')) --len;
    if (len > kMaxText) len = kMaxText;

    TaskHandle_t me = xTaskGetCurrentTaskHandle();
    const bool serial_only = s_serial_task.load(std::memory_order_relaxed) == me;
    uint32_t store_c = 0;
    if (!serial_only && len) store_c = store(rec, len, !fits);
    if (s_capture_task.load(std::memory_order_relaxed) == me) capture_append(text, len);

    // Seriell wie bisher (vollständige Zeile, auch wenn sie im Puffer gekürzt ist)
    const uint32_t c2 = cycles();
    if (fits) {
        fwrite(text, 1, static_cast<size_t>(n), stdout);
    } else {
        vprintf(fmt, copy);
    }
    va_end(copy);
    const uint32_t c3 = cycles();

    portENTER_CRITICAL(&s_mux);
    ++s_stats.calls;
    if (serial_only) ++s_stats.serial_only;
    s_stats.fmt_cycles += c1 - c0;
    s_stats.uart_cycles += c3 - c2;
    const uint32_t total = (c1 - c0) + store_c + (c3 - c2);
    if (total > s_stats.total_max_cycles) s_stats.total_max_cycles = total;
    portEXIT_CRITICAL(&s_mux);
    return n;
}

// Datensatz an Versatz off aus dem Hauptpuffer in line kopieren (im kritischen Abschnitt)
bool copy_line(uint32_t seq, Line* line) {
    if (!s_index) return false;
    const uint32_t off = s_index[seq % kIndexSize];
    if (off >= s_main.cap || off % 4) return false;
    const RecHead rh = read_head(s_main, off);
    if (rh.seq != seq || rh.len > kMaxText) return false;
    alignas(4) uint32_t buf[(kMaxText + 3) / 4 + 1];
    get(s_main, (off + sizeof(RecHead)) % s_main.cap, buf, (rh.len + 3) & ~3u);
    std::memcpy(line->text, buf, rh.len);
    line->text[rh.len] = 0;
    line->seq = rh.seq;
    line->up_ms = rh.up_ms;
    line->epoch = rh.epoch;
    line->boot = rh.boot;
    line->level = static_cast<char>(rh.level);
    line->truncated = rh.flags & 1;
    line->len = rh.len;
    return true;
}

// Die neuesten Zeilen (so viele wie hineinpassen) in die RTC-Sicherung kopieren; im kritischen Abschnitt aufrufen
void fill_backup() {
    if (!s_index) {
        fresh(s_backup, s_main.h.next);
        return;
    }
    uint32_t start = s_main.h.next, total = 0;
    while (start > s_main.h.first) {
        uint32_t w;
        get(s_main, s_index[(start - 1) % kIndexSize], &w, 4);
        const uint32_t size = w & 0xFFFF;
        if (total + size > s_backup.cap) break;
        total += size;
        --start;
    }
    fresh(s_backup, start);
    alignas(4) uint32_t rec[kMaxRecord / 4];
    for (uint32_t seq = start; seq < s_main.h.next; ++seq) {
        const uint32_t off = s_index[seq % kIndexSize];
        uint32_t w;
        get(s_main, off, &w, 4);
        const uint32_t size = w & 0xFFFF;
        if (size < sizeof(RecHead) + 1 || size > kMaxRecord) break;
        get(s_main, off, rec, size);
        append(s_backup, rec, size, seq);
    }
}

// Software-Neustart (esp_restart, z. B. nach einem Update): die neuesten Zeilen in die RTC-Sicherung, falls die
// nächste Firmware den PSRAM-Puffer an anderer Stelle hat. Läuft einmal, kurz vor dem Reset (~1 ms).
void on_shutdown() {
    portENTER_CRITICAL(&s_mux);
    fill_backup();
    portEXIT_CRITICAL(&s_mux);
}

}  // namespace

// --- öffentliche Schnittstelle -------------------------------------------------------------------------------

void set_mirror(bool on) {
    portENTER_CRITICAL(&s_mux);
    s_mirror = on;
    // ein: mit den bisherigen Zeilen füllen, ab jetzt jede mitschreiben; aus: leeren, damit nie veraltete Zeilen
    // übernommen werden
    if (on) fill_backup();
    else fresh(s_backup, s_main.h.next);
    portEXIT_CRITICAL(&s_mux);
}

void sync_clock() {
    timeval tv;
    gettimeofday(&tv, nullptr);
    const int64_t up_ms = esp_timer_get_time() / 1000;
    const int64_t base = tv.tv_sec > kValidAfter ? static_cast<int64_t>(tv.tv_sec) * 1000 + tv.tv_usec / 1000 - up_ms : 0;
    portENTER_CRITICAL(&s_mux);
    s_epoch_base_ms = base;
    portEXIT_CRITICAL(&s_mux);
}

void init() {
    s_main.hdr = s_psram.hdr;
    s_main.data = s_psram.data;
    s_main.cap = kRingBytes;
    s_main.cached = true;
    s_backup.hdr = s_rtc.hdr;
    s_backup.data = s_rtc.data;
    s_backup.cap = kRtcBytes;
    s_index = static_cast<uint32_t*>(heap_caps_calloc(kIndexSize, sizeof(uint32_t), MALLOC_CAP_SPIRAM));

    const bool main_ok = load(s_main, true, s_load_note[0], sizeof(s_load_note[0]));
    const bool rtc_ok = load(s_backup, false, s_load_note[1], sizeof(s_load_note[1]));
    if (main_ok) {
        s_origin = "übernommen";
        s_prev_lines = s_main.h.next - s_main.h.first;
    } else if (rtc_ok && s_backup.h.used > 0) {
        // Hauptpuffer liegt an anderer Stelle (andere Version) oder ist verloren: letzte Zeilen aus der Sicherung
        fresh(s_main, s_backup.h.first);
        uint32_t off = s_backup.h.tail;
        alignas(4) uint32_t rec[kMaxRecord / 4];
        for (uint32_t seq = s_backup.h.first; seq < s_backup.h.next; ++seq) {
            const RecHead rh = read_head(s_backup, off);
            get(s_backup, off, rec, rh.size);
            const uint32_t at = append(s_main, rec, rh.size, seq);
            if (s_index) s_index[seq % kIndexSize] = at;
            off = (off + rh.size) % s_backup.cap;
        }
        s_origin = "aus RTC-Sicherung";
        s_prev_lines = s_main.h.next - s_main.h.first;
    } else {
        fresh(s_main, 1);
        s_origin = "neu";
    }
    // Sicherung geleert: sie gilt nur für genau einen Neustart (Probelauf bzw. Shutdown-Handler füllen sie neu)
    fresh(s_backup, s_main.h.next);
    s_boot_first = s_main.h.next;
    sync_clock();
    s_ready = true;
    esp_register_shutdown_handler(on_shutdown);
    s_prev_vprintf = esp_log_set_vprintf(hook);
    ESP_LOGI(TAG, "Log-Puffer %s: %lu Zeilen früherer Starts, %u KB im PSRAM bei %p, %u Byte Sicherung im RTC-Speicher",
             s_origin, static_cast<unsigned long>(s_prev_lines), static_cast<unsigned>(kRingBytes / 1024),
             static_cast<void*>(&s_psram), static_cast<unsigned>(kRtcBytes));
    ESP_LOGI(TAG, "  Befund PSRAM: %s", s_load_note[0]);
    ESP_LOGI(TAG, "  Befund RTC: %s", s_load_note[1]);
}

void set_boot(uint16_t boot) {
    // Zeilen dieses Starts, die vor dem Lesen des Startzählers entstanden sind, nachtragen
    portENTER_CRITICAL(&s_mux);
    s_boot = boot;
    portEXIT_CRITICAL(&s_mux);
    for (uint32_t seq = s_boot_first;; ++seq) {
        bool more = false;
        portENTER_CRITICAL(&s_mux);
        if (seq < s_main.h.next && seq >= s_main.h.first && s_index) {
            more = true;
            const uint32_t off = (s_index[seq % kIndexSize] + 16) % s_main.cap;  // Wort mit boot und len
            uint32_t w;
            get(s_main, off, &w, 4);
            if ((w & 0xFFFF) == kBootUnknown) {
                w = (w & 0xFFFF0000u) | boot;
                put(s_main, off, &w, 4);
            }
        }
        portEXIT_CRITICAL(&s_mux);
        if (!more) break;
    }
    portENTER_CRITICAL(&s_mux);
    uint32_t off = s_backup.h.tail;
    for (uint32_t seq = s_backup.h.first; seq < s_backup.h.next; ++seq) {
        uint32_t w[5];
        get(s_backup, off, w, sizeof(w));
        if (w[1] >= s_boot_first && (w[4] & 0xFFFF) == kBootUnknown) {
            w[4] = (w[4] & 0xFFFF0000u) | boot;
            put(s_backup, (off + 16) % s_backup.cap, &w[4], 4);
        }
        off = (off + (w[0] & 0xFFFF)) % s_backup.cap;
    }
    portEXIT_CRITICAL(&s_mux);
}

uint16_t boot() { return s_boot; }

uint32_t next_seq() {
    portENTER_CRITICAL(&s_mux);
    const uint32_t n = s_main.h.next;
    portEXIT_CRITICAL(&s_mux);
    return n;
}

Info info() {
    Info i{};
    portENTER_CRITICAL(&s_mux);
    i.rid = s_main.h.rid;
    i.first = s_main.h.first;
    i.next = s_main.h.next;
    i.used = s_main.h.used;
    i.capacity = s_main.cap;
    i.rtc_used = s_backup.h.used;
    i.rtc_capacity = s_backup.cap;
    i.rtc_first = s_backup.h.first;
    i.mirror = s_mirror;
    i.boot = s_boot;
    i.stats = s_stats;
    portEXIT_CRITICAL(&s_mux);
    i.origin = s_origin;
    i.prev_lines = s_prev_lines;
    i.address = &s_psram;
    return i;
}

uint32_t read(uint32_t from, size_t max_bytes, LineFn fn, void* ctx) {
    // Line ist groß (≈340 Byte): nicht auf den Stack der aufrufenden Task
    Line* l = new (std::nothrow) Line;
    if (!l) return from;
    uint32_t seq = from;
    size_t bytes = 0;
    for (;;) {
        bool got = false;
        portENTER_CRITICAL(&s_mux);
        if (seq < s_main.h.first) seq = s_main.h.first;
        if (seq < s_main.h.next) got = copy_line(seq, l);
        portEXIT_CRITICAL(&s_mux);
        if (!got) break;
        seq = l->seq + 1;
        bytes += l->len;
        if (!fn(*l, ctx) || bytes >= max_bytes) break;
    }
    delete l;
    return seq;
}

void clear() {
    portENTER_CRITICAL(&s_mux);
    for (Ring* r : {&s_main, &s_backup}) {
        r->h.tail = r->h.head;
        r->h.used = 0;
        r->h.first = r->h.next;
        store_head(*r);
    }
    portEXIT_CRITICAL(&s_mux);
}

size_t format(const Line& line, char* out, size_t out_len) {
    char ts[24] = "---------- --:--:--";
    if (line.epoch) {
        time_t t = line.epoch;
        tm lt;
        localtime_r(&t, &lt);
        strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &lt);
    }
    char boot[8] = "?";
    if (line.boot != kBootUnknown) std::snprintf(boot, sizeof(boot), "%u", line.boot);
    int n = std::snprintf(out, out_len, "%s #%s %.*s%s", ts, boot, static_cast<int>(line.len), line.text,
                          line.truncated ? " …" : "");
    if (n < 0) return 0;
    return std::min(static_cast<size_t>(n), out_len ? out_len - 1 : 0);
}

Capture::Capture(char* buf, size_t cap) {
    s_cap_buf = buf;
    s_cap_cap = cap;
    s_cap_len = 0;
    if (cap) buf[0] = 0;
    s_capture_task = xTaskGetCurrentTaskHandle();
}

Capture::~Capture() {
    s_capture_task = nullptr;
    s_cap_buf = nullptr;
    s_cap_cap = 0;
}

size_t Capture::size() const { return s_cap_len; }

SerialOnly::SerialOnly(bool active) : active_(active) {
    if (active_) s_serial_task = xTaskGetCurrentTaskHandle();
}

SerialOnly::~SerialOnly() {
    if (active_) s_serial_task = nullptr;
}

void bench_store(int n, bool with_rtc, uint32_t* avg_cycles, uint32_t* max_cycles) {
    uint64_t sum = 0;
    uint32_t mx = 0;
    uint32_t rec[kRecWords];
    char* text = reinterpret_cast<char*>(rec) + sizeof(RecHead);
    for (int i = 0; i < n; ++i) {
        int len = std::snprintf(text, kMaxText + 1,
                                "I (%lu) log: Messzeile %d/%d nur im Puffer%s, typische Länge einer Log-Zeile mit Zahlen",
                                static_cast<unsigned long>(esp_timer_get_time() / 1000), i + 1, n,
                                with_rtc ? " und RTC" : "");
        const uint32_t c = store(rec, static_cast<size_t>(len), false, with_rtc);
        sum += c;
        mx = std::max(mx, c);
    }
    // Messzeilen nicht als „Sicherung“ stehen lassen, wenn sie gerade nicht zeilenweise geführt wird
    if (with_rtc && !s_mirror) {
        portENTER_CRITICAL(&s_mux);
        fresh(s_backup, s_main.h.next);
        portEXIT_CRITICAL(&s_mux);
    }
    *avg_cycles = n ? static_cast<uint32_t>(sum / n) : 0;
    *max_cycles = mx;
}

}  // namespace app::log
