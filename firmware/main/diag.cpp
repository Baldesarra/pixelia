// Diagnose: Startzähler, Reset-Historie, Absturzbericht, Task-Liste, JSON-Bausteine (siehe diag.h).

#include "diag.h"

#include <algorithm>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <memory>
#include <mutex>

#include "app.h"
#include "esp_app_desc.h"
#include "esp_attr.h"
#include "esp_chip_info.h"
#include "esp_core_dump.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_psram.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "json.h"
#include "logbuf.h"
#include "net.h"
#include "nvs.h"
#include "ota.h"
#include "sdkconfig.h"

namespace app::diag {

namespace {
constexpr const char* TAG = "diag";
constexpr const char* kNvs = "diag";
constexpr uint32_t kBeatMagic = 0x54414542;  // „BEAT“
constexpr time_t kValidAfter = 1735689600;
constexpr uint32_t kUnknown = UINT32_MAX;

// Eintrag im NVS (Schlüssel „r00“ … „r15“ = Startnummer % 16). Format nur erweitern, nie umbauen.
struct Record {
    uint8_t format;
    uint8_t reason;
    uint8_t flags;  // Bit 0 Probelauf, 1 Rückfall erkannt, 2 Absturz, 3 Startzeit nachgetragen
    uint8_t reserved;
    uint32_t boot;
    uint32_t t_boot;
    uint32_t prev_up_s;
    uint32_t prev_t;
    uint32_t crash_pc;
    char version[32];
    char part[8];
    char crash_task[16];
    char note[128];
};
constexpr uint8_t kRecordFormat = 1;
constexpr uint8_t kFlagTrial = 1, kFlagRollback = 2, kFlagCrash = 4, kFlagTimeLate = 8;

// Lebenszeichen im RTC-Speicher: übersteht Software-Reset, Absturz und Watchdog, nicht aber einen Stromausfall
struct Beat {
    uint32_t magic, boot, up_s, epoch, check;
};
RTC_NOINIT_ATTR Beat s_beat;

std::mutex s_lock;
uint32_t s_boot = 0;
Record s_current{};
bool s_time_pending = false;
Crash s_crash{};

// CPU-Last pro Task: Laufzeitzähler beim letzten Abruf
struct PrevRun {
    UBaseType_t number;
    uint32_t runtime;
};
PrevRun s_prev_run[48];
int s_prev_count = 0;
int64_t s_prev_run_at = 0;

uint32_t beat_check(const Beat& b) { return (b.magic ^ (b.boot * 2654435761u) ^ (b.up_s * 40503u) ^ b.epoch) + 0x9e3779b9u; }

uint32_t now_epoch() {
    time_t t = time(nullptr);
    return t > kValidAfter ? static_cast<uint32_t>(t) : 0;
}

uint32_t uptime_s() { return static_cast<uint32_t>(esp_timer_get_time() / 1000000); }

void key_for(uint32_t boot, char* key) { std::snprintf(key, 4, "r%02u", static_cast<unsigned>(boot % kResetHistory)); }

void save_record(const Record& r) {
    nvs_handle_t h;
    if (nvs_open(kNvs, NVS_READWRITE, &h) != ESP_OK) return;
    char key[4];
    key_for(r.boot, key);
    esp_err_t err = nvs_set_blob(h, key, &r, sizeof(r));
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    if (err != ESP_OK) ESP_LOGW(TAG, "Reset-Eintrag nicht gespeichert: %s", esp_err_to_name(err));
}

bool load_record(nvs_handle_t h, uint32_t boot, Record* r) {
    char key[4];
    key_for(boot, key);
    size_t len = sizeof(*r);
    return nvs_get_blob(h, key, r, &len) == ESP_OK && len == sizeof(*r) && r->format == kRecordFormat && r->boot == boot;
}

void hex(const uint8_t* data, int bytes, char* out) {
    for (int i = 0; i < bytes; ++i) std::snprintf(out + 2 * i, 3, "%02x", data[i]);
}

const char* exception_name(uint32_t cause) {
    switch (cause) {
        case 0: return "IllegalInstruction";
        case 2: return "InstrFetchError";
        case 3: return "LoadStoreError";
        case 6: return "IntegerDivideByZero";
        case 9: return "LoadStoreAlignment";
        case 12: return "InstrPIFDataError";
        case 13: return "LoadStorePIFDataError";
        case 14: return "InstrPIFAddrError";
        case 15: return "LoadStorePIFAddrError";
        case 20: return "InstFetchProhibited";
        case 28: return "LoadProhibited";
        case 29: return "StoreProhibited";
        default: return nullptr;
    }
}

void read_crash(Crash* c) {
    *c = Crash{};
    size_t addr = 0, size = 0;
    if (esp_core_dump_image_get(&addr, &size) != ESP_OK || size == 0) return;
    c->present = true;
    c->addr = addr;
    c->size = size;
    c->valid = esp_core_dump_image_check() == ESP_OK;
    auto sum = std::make_unique<esp_core_dump_summary_t>();
    if (esp_core_dump_get_summary(sum.get()) == ESP_OK) {
        std::snprintf(c->task, sizeof(c->task), "%.15s", sum->exc_task);
        c->pc = sum->exc_pc;
        c->depth = static_cast<int>(std::min<uint32_t>(sum->exc_bt_info.depth, 16));
        for (int i = 0; i < c->depth; ++i) c->bt[i] = sum->exc_bt_info.bt[i];
        c->bt_corrupted = sum->exc_bt_info.corrupted;
        c->exc_cause = sum->ex_info.exc_cause;
        c->exc_vaddr = sum->ex_info.exc_vaddr;
        std::snprintf(c->sha, sizeof(c->sha), "%.16s", reinterpret_cast<const char*>(sum->app_elf_sha256));
    }
    if (esp_core_dump_get_panic_reason(c->reason, sizeof(c->reason)) != ESP_OK) c->reason[0] = 0;
    // Bei CPU-Ausnahmen steht kein Panic-Text im Bericht: Namen der Xtensa-Ausnahme (EXCCAUSE) einsetzen
    if (!c->reason[0] && c->exc_cause < 64) {
        const char* name = exception_name(c->exc_cause);
        std::snprintf(c->reason, sizeof(c->reason), "%s (Ausnahme %lu) bei Adresse 0x%08lx", name ? name : "CPU-Ausnahme",
                      static_cast<unsigned long>(c->exc_cause), static_cast<unsigned long>(c->exc_vaddr));
    }
    // Zu welcher Version gehört der Bericht? Vergleich mit dem ELF-SHA256 der Firmware in beiden App-Partitionen
    const size_t n = std::strlen(c->sha);
    if (n == 0) return;
    for (const char* label : {"ota_0", "ota_1"}) {
        const esp_partition_t* p = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, label);
        esp_app_desc_t d;
        if (!p || esp_ota_get_partition_description(p, &d) != ESP_OK) continue;
        char sha[65];
        hex(d.app_elf_sha256, 32, sha);
        if (std::strncmp(sha, c->sha, n) == 0) {
            std::snprintf(c->version, sizeof(c->version), "%.31s", d.version);
            std::snprintf(c->part, sizeof(c->part), "%.7s", label);
            return;
        }
    }
}

bool is_crash_reset(esp_reset_reason_t r) {
    return r == ESP_RST_PANIC || r == ESP_RST_INT_WDT || r == ESP_RST_TASK_WDT || r == ESP_RST_WDT;
}

void to_entry(const Record& r, ResetEntry* e) {
    *e = ResetEntry{};
    e->boot = r.boot;
    e->reason = r.reason;
    e->trial = r.flags & kFlagTrial;
    e->rollback = r.flags & kFlagRollback;
    e->crash = r.flags & kFlagCrash;
    e->time_late = r.flags & kFlagTimeLate;
    e->t_boot = r.t_boot;
    e->prev_up_s = r.prev_up_s;
    e->prev_t = r.prev_t;
    e->crash_pc = r.crash_pc;
    std::snprintf(e->version, sizeof(e->version), "%.31s", r.version);
    std::snprintf(e->part, sizeof(e->part), "%.7s", r.part);
    std::snprintf(e->crash_task, sizeof(e->crash_task), "%.15s", r.crash_task);
    std::snprintf(e->note, sizeof(e->note), "%.127s", r.note);
}

void fmt_time(uint32_t epoch, char* out, size_t n) {
    if (!epoch) {
        std::snprintf(out, n, "unbekannt");
        return;
    }
    time_t t = epoch;
    tm lt;
    localtime_r(&t, &lt);
    strftime(out, n, "%Y-%m-%d %H:%M:%S", &lt);
}

void fmt_duration(uint32_t s, char* out, size_t n) {
    if (s == kUnknown) {
        std::snprintf(out, n, "unbekannt");
    } else if (s >= 86400) {
        std::snprintf(out, n, "%u d %u h", static_cast<unsigned>(s / 86400), static_cast<unsigned>(s / 3600 % 24));
    } else if (s >= 3600) {
        std::snprintf(out, n, "%u h %u min", static_cast<unsigned>(s / 3600), static_cast<unsigned>(s / 60 % 60));
    } else {
        std::snprintf(out, n, "%u min %u s", static_cast<unsigned>(s / 60), static_cast<unsigned>(s % 60));
    }
}

const char* state_name(eTaskState s) {
    switch (s) {
        case eRunning: return "läuft";
        case eReady: return "bereit";
        case eBlocked: return "wartet";
        case eSuspended: return "ruht";
        case eDeleted: return "beendet";
        default: return "?";
    }
}

struct TaskRow {
    char name[16];
    int core;  // -1 = beide Kerne
    unsigned prio;
    unsigned stack_free;  // Byte, Tiefstwert seit dem Start der Task
    float cpu;            // % eines Kerns seit dem letzten Abruf
    const char* state;
};

constexpr int kMaxTasks = 40;

// Arbeitslisten im PSRAM statt statisch im knappen internen RAM (nur einfache Strukturen, mit Nullen gefüllt)
struct PsramFree {
    void operator()(void* p) const { heap_caps_free(p); }
};
template <typename T>
std::unique_ptr<T[], PsramFree> psram_array(int n) {
    return std::unique_ptr<T[], PsramFree>(static_cast<T*>(heap_caps_calloc(n, sizeof(T), MALLOC_CAP_SPIRAM)));
}

// Task-Liste mit CPU-Last seit dem letzten Abruf (oder seit dem Start), nach Name sortiert
// Achtung: uxTaskGetSystemState() hält die Kernel-Sperre, während es die Stacks aller Tasks nach dem Füllmuster
// durchsucht (gemessen siehe query_us). In der Zeit warten Tick- und Display-Interrupt auf Kern 1, der Bounce-Buffer
// kommt zu spät: etwa ein Aussetzer (desync) pro Abfrage. Deshalb nur auf ausdrücklichen Wunsch (tasks, ?tasks=1,
// Diagnosepaket), nie regelmäßig.
uint32_t s_task_query_us = 0;

int collect_tasks(TaskRow* rows, int max, uint32_t* window_ms) {
    UBaseType_t n = uxTaskGetNumberOfTasks() + 4;
    auto* st = static_cast<TaskStatus_t*>(heap_caps_malloc(n * sizeof(TaskStatus_t), MALLOC_CAP_DEFAULT));
    if (!st) return 0;
    configRUN_TIME_COUNTER_TYPE total = 0;
    const int64_t q0 = esp_timer_get_time();
    n = uxTaskGetSystemState(st, n, &total);
    s_task_query_us = static_cast<uint32_t>(esp_timer_get_time() - q0);
    std::lock_guard<std::mutex> g(s_lock);
    const int64_t now = esp_timer_get_time();
    const int64_t window = s_prev_run_at ? now - s_prev_run_at : now;
    *window_ms = static_cast<uint32_t>(window / 1000);
    int count = 0;
    for (UBaseType_t i = 0; i < n && count < max; ++i) {
        TaskRow& r = rows[count++];
        std::snprintf(r.name, sizeof(r.name), "%.15s", st[i].pcTaskName);
        r.core = st[i].xCoreID == tskNO_AFFINITY ? -1 : static_cast<int>(st[i].xCoreID);
        r.prio = static_cast<unsigned>(st[i].uxCurrentPriority);
        r.stack_free = static_cast<unsigned>(st[i].usStackHighWaterMark);
        r.state = state_name(st[i].eCurrentState);
        uint32_t prev = 0;
        for (int k = 0; k < s_prev_count; ++k)
            if (s_prev_run[k].number == st[i].xTaskNumber) prev = s_prev_run[k].runtime;
        if (!s_prev_run_at) prev = 0;
        const uint32_t delta = static_cast<uint32_t>(st[i].ulRunTimeCounter) - prev;
        r.cpu = window > 0 ? static_cast<float>(delta * 100.0 / static_cast<double>(window)) : 0;
    }
    s_prev_count = 0;
    for (UBaseType_t i = 0; i < n && s_prev_count < 48; ++i)
        s_prev_run[s_prev_count++] = {st[i].xTaskNumber, static_cast<uint32_t>(st[i].ulRunTimeCounter)};
    s_prev_run_at = now;
    heap_caps_free(st);
    std::sort(rows, rows + count, [](const TaskRow& a, const TaskRow& b) { return std::strcmp(a.name, b.name) < 0; });
    return count;
}
}  // namespace

void init() {
    nvs_handle_t h;
    uint32_t boot = 0;
    if (nvs_open(kNvs, NVS_READWRITE, &h) == ESP_OK) {
        nvs_get_u32(h, "boot", &boot);
        ++boot;
        nvs_set_u32(h, "boot", boot);
        nvs_commit(h);
        nvs_close(h);
    }
    s_boot = boot;
    log::set_boot(static_cast<uint16_t>(boot % log::kBootUnknown));

    read_crash(&s_crash);

    const esp_reset_reason_t reason = esp_reset_reason();
    Record r{};
    r.format = kRecordFormat;
    r.reason = static_cast<uint8_t>(reason);
    r.boot = boot;
    const uint32_t now = now_epoch();
    r.t_boot = now ? now - uptime_s() : 0;
    s_time_pending = r.t_boot == 0;
    const Beat b = s_beat;
    if (b.magic == kBeatMagic && b.check == beat_check(b) && b.boot + 1 == boot) {
        r.prev_up_s = b.up_s;
        r.prev_t = b.epoch;
    } else {
        r.prev_up_s = kUnknown;
        r.prev_t = 0;
    }
    const esp_app_desc_t* app = esp_app_get_description();
    std::snprintf(r.version, sizeof(r.version), "%.31s", app->version);
    const esp_partition_t* run = esp_ota_get_running_partition();
    std::snprintf(r.part, sizeof(r.part), "%.7s", run ? run->label : "?");
    const ota::Status o = ota::status();
    if (o.trial) r.flags |= kFlagTrial;
    if (is_crash_reset(reason) && s_crash.present) {
        r.flags |= kFlagCrash;
        std::snprintf(r.crash_task, sizeof(r.crash_task), "%.15s", s_crash.task);
        r.crash_pc = s_crash.pc;
        std::snprintf(r.note, sizeof(r.note), "%.127s", s_crash.reason);
    }
    if (o.rollback_new) {
        r.flags |= kFlagRollback;
        std::snprintf(r.note, sizeof(r.note), "Update auf %.31s zurückgerollt: %.64s", o.rollback_version,
                      o.rollback_reason);
    }
    save_record(r);
    {
        std::lock_guard<std::mutex> g(s_lock);
        s_current = r;
    }
    char up[24];
    fmt_duration(r.prev_up_s, up, sizeof(up));
    ESP_LOGI(TAG, "Start Nr. %lu: %s, Version %s aus %s, davor %s gelaufen%s%s", static_cast<unsigned long>(boot),
             sysinfo::reset_reason_text(reason), r.version, r.part, up, r.note[0] ? " | " : "", r.note);
    heartbeat();
}

void heartbeat() {
    const uint32_t now = now_epoch();
    Beat b{kBeatMagic, s_boot, uptime_s(), now, 0};
    b.check = beat_check(b);
    s_beat = b;
    log::sync_clock();
    bool save = false;
    Record r;
    {
        std::lock_guard<std::mutex> g(s_lock);
        if (s_time_pending && now) {
            // Uhrzeit erst jetzt bekannt (NTP nach dem Einschalten): Startzeit zurückrechnen und nachtragen
            s_current.t_boot = now - uptime_s();
            s_current.flags |= kFlagTimeLate;
            s_time_pending = false;
            r = s_current;
            save = true;
        }
    }
    if (save) save_record(r);
}

uint32_t boot() { return s_boot; }

int resets(ResetEntry* out, int max) {
    nvs_handle_t h;
    if (nvs_open(kNvs, NVS_READONLY, &h) != ESP_OK) return 0;
    int n = 0;
    for (uint32_t i = 0; i < kResetHistory && n < max && i < s_boot; ++i) {
        Record r;
        if (load_record(h, s_boot - i, &r)) to_entry(r, &out[n++]);
    }
    nvs_close(h);
    return n;
}

bool crash(Crash* out) {
    std::lock_guard<std::mutex> g(s_lock);
    *out = s_crash;
    return s_crash.present;
}

esp_err_t crash_erase() {
    esp_err_t err = esp_core_dump_image_erase();
    std::lock_guard<std::mutex> g(s_lock);
    read_crash(&s_crash);
    ESP_LOGW(TAG, "Absturzbericht gelöscht: %s", esp_err_to_name(err));
    return err;
}

// --- JSON ---------------------------------------------------------------------------------------------------

std::string log_info_json() {
    const log::Info i = log::info();
    const log::Stats& s = i.stats;
    const double mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;
    auto avg_us = [&](uint64_t cyc, uint32_t n) { return n ? static_cast<double>(cyc) / n / mhz : 0.0; };
    char addr[16];
    std::snprintf(addr, sizeof(addr), "%p", i.address);
    Json j;
    j.obj();
    j.num("rid", i.rid).num("first", i.first).num("next", i.next).num("lines", i.next - i.first);
    j.num("used", static_cast<double>(i.used)).num("capacity", static_cast<double>(i.capacity));
    j.num("rtcUsed", static_cast<double>(i.rtc_used)).num("rtcCapacity", static_cast<double>(i.rtc_capacity));
    j.num("rtcFirst", i.rtc_first).boolean("rtcMirror", i.mirror).str("origin", i.origin).num("prevLines", i.prev_lines);
    j.str("addr", addr);
    j.num("boot", i.boot);
    j.obj("hook");
    j.num("calls", s.calls).num("stored", s.stored).num("serialOnly", s.serial_only).num("truncated", s.truncated);
    j.num("errors", s.errors).num("warnings", s.warnings);
    j.num("fmtUs", avg_us(s.fmt_cycles, s.calls), 1).num("storeUs", avg_us(s.store_cycles, s.stored), 1);
    j.num("psramUs", avg_us(s.main_cycles, s.stored), 1).num("rtcUs", avg_us(s.rtc_cycles, s.stored), 1);
    j.num("uartUs", avg_us(s.uart_cycles, s.calls), 0);
    j.num("storeMaxUs", s.store_max_cycles / mhz, 1).num("totalMaxUs", s.total_max_cycles / mhz, 0);
    j.end_obj();
    j.end_obj();
    return std::move(j.text());
}

std::string perf_json(bool with_tasks) {
    const perf::Snapshot p = perf::snapshot();
    Json j;
    j.obj();
    j.num("uptime", uptime_s()).num("at", p.at_s).str("mode", p.mode ? p.mode : "?");
    j.obj("lcd").num("hz", p.lcd_hz, 2).num("pclk", p.pclk_mhz, 2).num("isr", p.isr_pct, 1);
    j.num("isrMaxUs", p.isr_max_us).num("desync", p.desync).end_obj();
    if (p.engine) {
        j.obj("engine").num("fps", p.fps, 1).num("render", p.render_ms, 2).num("renderMax", p.render_max_ms, 2);
        j.num("logic", p.logic_ms, 2).num("logicMax", p.logic_max_ms, 2).num("holds", p.holds);
        j.arr("ticks").num(nullptr, p.ticks[0]).num(nullptr, p.ticks[1]).num(nullptr, p.ticks[2]).end_arr();
        char wt[16];
        std::snprintf(wt, sizeof(wt), "%02d:%02d", p.world_minute / 60, p.world_minute % 60);
        j.num("sprites", p.sprites).str("buf", p.buf_psram ? "psram" : "intern").num("worldDay", p.world_day);
        j.str("worldTime", wt).end_obj();
    } else {
        j.null("engine");
    }
    j.arr("cpu").num(nullptr, p.cpu[0]).num(nullptr, p.cpu[1]).end_arr();
    j.obj("heap").num("ram", p.ram_kb).num("ramMin", p.ram_min_kb).num("ramBlk", p.ram_blk_kb).num("dma", p.dma_kb);
    j.num("psram", p.psram_kb).num("psramMin", p.psram_min_kb).num("psramBlk", p.psram_blk_kb).end_obj();
    const wifi::Status w = wifi::status();
    j.obj("wifi").str("state", wifi::state_name(w.state)).num("rssi", w.rssi).num("ch", w.channel).end_obj();
    j.raw("log", log_info_json());
    // Grafik-Speicher (seit 0.5.5): Belegung, Laden, letztes Paket mit Zeit von der Anfrage bis geladen
    const assets::Summary a = assets::summary();
    j.obj("assets").num("used", a.used >> 10).num("budget", a.budget >> 10).num("peak", a.peak >> 10);
    j.num("packs", a.packs).num("loads", a.loads).num("evictions", a.evictions).num("failures", a.failures);
    j.str("source", a.source).boolean("littlefs", a.littlefs).boolean("sd", a.sd);
    j.str("last", a.last).num("lastMs", a.last_us / 1000.0, 2).str("lastSource", a.last_source ? a.last_source : "");
    j.end_obj();
    if (!with_tasks) {
        j.null("tasks").end_obj();
        return std::move(j.text());
    }

    auto rows = psram_array<TaskRow>(kMaxTasks);
    uint32_t window_ms = 0;
    const int n = rows ? collect_tasks(rows.get(), kMaxTasks, &window_ms) : 0;
    j.num("taskWindowMs", window_ms).num("taskQueryUs", s_task_query_us);
    j.arr("tasks");
    for (int i = 0; i < n; ++i) {
        j.obj().str("name", rows[i].name).num("core", rows[i].core).num("prio", rows[i].prio);
        j.num("stackFree", rows[i].stack_free).num("cpu", rows[i].cpu, 1).str("state", rows[i].state).end_obj();
    }
    j.end_arr();
    j.end_obj();
    return std::move(j.text());
}

std::string resets_json() {
    auto list = psram_array<ResetEntry>(kResetHistory);
    const int n = list ? resets(list.get(), kResetHistory) : 0;
    Json j;
    j.obj().num("boot", s_boot).arr("entries");
    for (int i = 0; i < n; ++i) {
        const ResetEntry& e = list[i];
        char t[24], pt[24], pc[12];
        fmt_time(e.t_boot, t, sizeof(t));
        fmt_time(e.prev_t, pt, sizeof(pt));
        std::snprintf(pc, sizeof(pc), "0x%08" PRIx32, e.crash_pc);
        j.obj().num("boot", e.boot).str("v", e.version).str("part", e.part);
        j.num("reason", e.reason).str("reasonText", sysinfo::reset_reason_text(e.reason));
        j.num("t", e.t_boot).str("time", e.t_boot ? t : "").boolean("timeLate", e.time_late);
        if (e.prev_up_s == kUnknown) j.null("prevUptime");
        else j.num("prevUptime", e.prev_up_s);
        j.num("prevT", e.prev_t).str("prevTime", e.prev_t ? pt : "");
        j.boolean("trial", e.trial).boolean("rollback", e.rollback);
        if (e.crash) j.obj("crash").str("task", e.crash_task).str("pc", pc).end_obj();
        else j.null("crash");
        j.str("note", e.note).end_obj();
    }
    j.end_arr().end_obj();
    return std::move(j.text());
}

std::string crash_json() {
    Crash c;
    crash(&c);
    Json j;
    j.obj().boolean("present", c.present);
    if (c.present) {
        char b[16];
        j.boolean("valid", c.valid).num("size", c.size);
        std::snprintf(b, sizeof(b), "0x%06" PRIx32, c.addr);
        j.str("addr", b).str("task", c.task);
        std::snprintf(b, sizeof(b), "0x%08" PRIx32, c.pc);
        j.str("pc", b).str("reason", c.reason);
        j.num("excCause", c.exc_cause);
        std::snprintf(b, sizeof(b), "0x%08" PRIx32, c.exc_vaddr);
        j.str("excVaddr", b);
        j.arr("backtrace");
        for (int i = 0; i < c.depth; ++i) {
            std::snprintf(b, sizeof(b), "0x%08" PRIx32, c.bt[i]);
            j.str(nullptr, b);
        }
        j.end_arr().boolean("btCorrupted", c.bt_corrupted).str("elfSha", c.sha);
        j.str("version", c.version).str("part", c.part);
    }
    j.end_obj();
    return std::move(j.text());
}

std::string system_json() {
    const esp_app_desc_t* app = esp_app_get_description();
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    uint8_t mac[6] = {};
    esp_efuse_mac_get_default(mac);
    char macs[20], sha[65];
    std::snprintf(macs, sizeof(macs), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    hex(app->app_elf_sha256, 32, sha);
    uint32_t flash_size = 0, flash_id = 0;
    esp_flash_get_size(nullptr, &flash_size);
    esp_flash_read_id(nullptr, &flash_id);
    nvs_stats_t nvs{};
    nvs_get_stats(nullptr, &nvs);

    Json j;
    j.obj();
    j.obj("app").str("v", app->version).str("date", app->date).str("time", app->time).str("idf", app->idf_ver);
    j.str("elfSha", sha).end_obj();
    j.obj("chip").str("model", "ESP32-S3").num("rev", chip.revision / 100.0, 2).num("cores", chip.cores);
    j.num("mhz", CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ).str("mac", macs).end_obj();
    j.obj("flash").num("mb", flash_size >> 20).num("id", flash_id).end_obj();
    j.obj("psram").num("kb", esp_psram_is_initialized() ? esp_psram_get_size() >> 10 : 0).end_obj();
    j.str("resetReason", sysinfo::reset_reason_text(esp_reset_reason())).num("boot", s_boot);
    const esp_partition_t* run = esp_ota_get_running_partition();
    const esp_partition_t* next_boot = esp_ota_get_boot_partition();
    j.str("running", run ? run->label : "?").str("nextBoot", next_boot ? next_boot->label : "?");
    j.arr("partitions");
    for (const char* label : {"ota_0", "ota_1"}) {
        const esp_partition_t* p = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, label);
        if (!p) continue;
        esp_app_desc_t d;
        esp_ota_img_states_t s;
        const bool has = esp_ota_get_partition_description(p, &d) == ESP_OK;
        const char* state = "–";
        if (esp_ota_get_state_partition(p, &s) == ESP_OK) {
            state = s == ESP_OTA_IMG_VALID            ? "gültig"
                    : s == ESP_OTA_IMG_PENDING_VERIFY ? "Probelauf"
                    : s == ESP_OTA_IMG_INVALID        ? "ungültig"
                    : s == ESP_OTA_IMG_ABORTED        ? "abgebrochen"
                    : s == ESP_OTA_IMG_NEW            ? "neu"
                                                      : "unbestimmt";
        }
        char psha[17] = "";
        if (has) hex(d.app_elf_sha256, 8, psha);
        j.obj().str("label", label).str("v", has ? d.version : "").str("state", state).str("elfSha", psha).end_obj();
    }
    j.end_arr();
    j.obj("nvs").num("used", nvs.used_entries).num("free", nvs.free_entries).num("total", nvs.total_entries).end_obj();
    j.end_obj();
    return std::move(j.text());
}

// --- Konsole ------------------------------------------------------------------------------------------------

void log_tasks() {
    auto rows = psram_array<TaskRow>(kMaxTasks);
    if (!rows) return;
    uint32_t window_ms = 0;
    const int n = collect_tasks(rows.get(), kMaxTasks, &window_ms);
    ESP_LOGI(TAG, "%d Tasks, CPU-Last der letzten %.1f s (100 %% = ein Kern), Abfrage %lu µs unter Kernel-Sperre:", n,
             window_ms / 1000.0, static_cast<unsigned long>(s_task_query_us));
    ESP_LOGI(TAG, "  %-16s %4s %4s %10s %6s  %s", "Name", "Kern", "Prio", "Stack frei", "CPU", "Zustand");
    for (int i = 0; i < n; ++i) {
        char core[4];
        std::snprintf(core, sizeof(core), "%s", rows[i].core < 0 ? "–" : rows[i].core ? "1" : "0");
        ESP_LOGI(TAG, "  %-16s %4s %4u %8u B %5.1f%%  %s", rows[i].name, core, rows[i].prio, rows[i].stack_free,
                 rows[i].cpu, rows[i].state);
    }
}

void log_resets() {
    auto list = psram_array<ResetEntry>(kResetHistory);
    if (!list) return;
    const int n = resets(list.get(), kResetHistory);
    ESP_LOGI(TAG, "Reset-Historie (%d Einträge, neueste zuerst):", n);
    for (int i = 0; i < n; ++i) {
        const ResetEntry& e = list[i];
        char t[24], up[24];
        fmt_time(e.t_boot, t, sizeof(t));
        fmt_duration(e.prev_up_s, up, sizeof(up));
        ESP_LOGI(TAG, "  #%lu %s%s  %-24s %s (%s)  davor %s%s%s%s%s", static_cast<unsigned long>(e.boot), t,
                 e.time_late ? "*" : "", sysinfo::reset_reason_text(e.reason), e.version, e.part, up,
                 e.trial ? ", Probelauf" : "", e.rollback ? ", Rückfall" : "", e.crash ? ", Absturz in " : "",
                 e.crash ? e.crash_task : "");
        if (e.note[0]) ESP_LOGI(TAG, "      %s", e.note);
    }
}

void log_crash() {
    Crash c;
    if (!crash(&c)) {
        ESP_LOGI(TAG, "Kein Absturzbericht im Flash");
        return;
    }
    ESP_LOGW(TAG, "Absturzbericht: %lu Byte, Prüfsumme %s, Version %s (%s), ELF-SHA %s",
             static_cast<unsigned long>(c.size), c.valid ? "ok" : "FALSCH", c.version[0] ? c.version : "unbekannt",
             c.part[0] ? c.part : "?", c.sha);
    ESP_LOGW(TAG, "  Task „%s“, PC 0x%08" PRIx32 ", Ursache %" PRIu32 ", Adresse 0x%08" PRIx32, c.task, c.pc, c.exc_cause,
             c.exc_vaddr);
    if (c.reason[0]) ESP_LOGW(TAG, "  Grund: %s", c.reason);
    char bt[16 * 11 + 1] = "";
    for (int i = 0; i < c.depth; ++i) {
        size_t len = std::strlen(bt);
        std::snprintf(bt + len, sizeof(bt) - len, " 0x%08" PRIx32, c.bt[i]);
    }
    ESP_LOGW(TAG, "  Backtrace:%s%s", bt, c.bt_corrupted ? " (beschädigt)" : "");
}

}  // namespace app::diag
