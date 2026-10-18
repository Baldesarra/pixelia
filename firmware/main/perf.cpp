// Leistungsmonitor: misst alle 2 s Bildrate, Zeichenzeit, Interrupt-Last, CPU-Last pro Kern und freien Speicher.
// Schreibt auf Wunsch eine kompakte Zeile ins Log (Befehl „perf“) und hält die letzte Messung für die
// Weboberfläche bereit. Später per Schalter unter „Experimentell“ (Backlog).

#include <cinttypes>
#include <mutex>

#include "app.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

namespace app::perf {

namespace {
constexpr const char* TAG = "perf";
constexpr int kMeasureS = 2;
volatile int s_interval_s = 2;
std::mutex s_lock;
Snapshot s_snap{};

void perf_task(void*) {
    int64_t t_prev = esp_timer_get_time();
    board::display::Stats d_prev = board::display::stats(true);
    gfx::EngineStats e_prev = gfx::engine_stats(true);
    uint32_t idle_prev[2] = {ulTaskGetIdleRunTimeCounterForCore(0), ulTaskGetIdleRunTimeCounterForCore(1)};
    int since_log = 0;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(kMeasureS * 1000));
        int64_t t_now = esp_timer_get_time();
        board::display::Stats d = board::display::stats(true);
        gfx::EngineStats e = gfx::engine_stats(true);
        uint32_t idle[2] = {ulTaskGetIdleRunTimeCounterForCore(0), ulTaskGetIdleRunTimeCounterForCore(1)};
        double dt_us = static_cast<double>(t_now - t_prev);

        if (dt_us > 0) {
            Snapshot s{};
            s.lcd_hz = static_cast<float>((d.vsyncs - d_prev.vsyncs) * 1e6 / dt_us);
            s.isr_pct = static_cast<float>((d.fill_cycles - d_prev.fill_cycles) * 100.0 /
                                           (dt_us * CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ));
            double isr_max_us = static_cast<double>(d.fill_max_cycles) / CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;
            s.desync = d.desyncs - d_prev.desyncs;
            for (int c = 0; c < 2; ++c) {
                double idle_pct = (idle[c] - idle_prev[c]) * 100.0 / dt_us;
                s.cpu[c] = static_cast<float>(idle_pct > 100 ? 0 : 100 - idle_pct);
            }
            s.ram_kb = heap_caps_get_free_size(MALLOC_CAP_INTERNAL) >> 10;
            s.ram_min_kb = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL) >> 10;
            s.ram_blk_kb = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL) >> 10;
            s.psram_kb = heap_caps_get_free_size(MALLOC_CAP_SPIRAM) >> 10;
            s.psram_min_kb = heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM) >> 10;

            char engine[160] = "";
            if (board::display::mode() == board::display::Mode::Engine) {
                uint32_t frames = e.frames - e_prev.frames;
                s.fps = static_cast<float>((d.swaps - d_prev.swaps) * 1e6 / dt_us);
                s.render_ms = frames ? static_cast<float>((e.render_us_sum - e_prev.render_us_sum) / 1000.0 / frames) : 0;
                double logic_ms = frames ? (e.logic_us_sum - e_prev.logic_us_sum) / 1000.0 / frames : 0;
                snprintf(engine, sizeof(engine),
                         " | fps %.1f render %.2f/%.2fms logic %.2fms ticks %" PRIu32 "/%" PRIu32 "/%" PRIu32
                         " sprites %d buf %s | welt Tag %d %02d:%02d",
                         s.fps, s.render_ms, e.render_us_max / 1000.0, logic_ms, e.tick_hist[0] - e_prev.tick_hist[0],
                         e.tick_hist[1] - e_prev.tick_hist[1], e.tick_hist[2] - e_prev.tick_hist[2], e.sprites,
                         e.buffers_in_psram ? "psram" : "int", e.world_day, e.world_minute / 60, e.world_minute % 60);
            }
            {
                std::lock_guard<std::mutex> g(s_lock);
                s_snap = s;
            }
            int interval = s_interval_s;
            since_log += kMeasureS;
            if (interval > 0 && since_log >= interval) {
                since_log = 0;
                ESP_LOGI(TAG, "%s | lcd %.2fHz pclk %.2fMHz isr %.1f%% (max %.0fus) desync %" PRIu32 "%s | cpu0 %.0f%% cpu1 %.0f%% | ram %uk (min %uk, blk %uk) psram %uk",
                         board::display::mode_name(board::display::mode()), s.lcd_hz,
                         board::display::timing().pclk_hz / 1e6, s.isr_pct, isr_max_us, s.desync, engine, s.cpu[0],
                         s.cpu[1], s.ram_kb, s.ram_min_kb, s.ram_blk_kb, s.psram_kb);
            }
        }
        t_prev = t_now;
        d_prev = d;
        e_prev = e;
        idle_prev[0] = idle[0];
        idle_prev[1] = idle[1];
    }
}
}  // namespace

void start(int interval_s) {
    s_interval_s = interval_s;
    xTaskCreatePinnedToCore(perf_task, "perf", 4096, nullptr, 2, nullptr, kSystemCore);
}

void set_interval(int interval_s) { s_interval_s = interval_s < 0 ? 0 : interval_s; }
int interval() { return s_interval_s; }

Snapshot snapshot() {
    std::lock_guard<std::mutex> g(s_lock);
    return s_snap;
}

}  // namespace app::perf
