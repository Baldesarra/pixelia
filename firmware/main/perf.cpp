// Leistungsmonitor: misst alle 2 s Bildrate, Zeichenzeit, Interrupt-Last, CPU-Last pro Kern und freien Speicher.
// Schreibt auf Wunsch eine kompakte Zeile ins Log (Befehl „perf“) und hält die letzte Messung für die
// Weboberfläche und /api/perf bereit. Später per Schalter unter „Experimentell“ (Backlog).
// Die Zeile alle 2 s geht nur auf die serielle Konsole, in den Log-Puffer (WLAN) nur eine pro Minute, sonst
// bestünde der Puffer nach einer halben Stunde nur noch aus Leistungszeilen.

#include <cinttypes>
#include <mutex>

#include "app.h"
#include "diag.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "logbuf.h"
#include "sdkconfig.h"

namespace app::perf {

namespace {
constexpr const char* TAG = "perf";
constexpr int kMeasureS = 2;
constexpr int kRingEveryS = 60;
volatile int s_interval_s = 2;
std::mutex s_lock;
Snapshot s_snap{};

void perf_task(void*) {
    int64_t t_prev = esp_timer_get_time();
    board::display::Stats d_prev = board::display::stats(true);
    gfx::EngineStats e_prev = gfx::engine_stats(true);
    uint32_t idle_prev[2] = {ulTaskGetIdleRunTimeCounterForCore(0), ulTaskGetIdleRunTimeCounterForCore(1)};
    int since_log = 0;
    int since_ring = kRingEveryS;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(kMeasureS * 1000));
        diag::heartbeat();
        int64_t t_now = esp_timer_get_time();
        board::display::Stats d = board::display::stats(true);
        gfx::EngineStats e = gfx::engine_stats(true);
        uint32_t idle[2] = {ulTaskGetIdleRunTimeCounterForCore(0), ulTaskGetIdleRunTimeCounterForCore(1)};
        double dt_us = static_cast<double>(t_now - t_prev);

        if (dt_us > 0) {
            Snapshot s{};
            s.at_s = static_cast<uint32_t>(t_now / 1000000);
            s.mode = board::display::mode_name(board::display::mode());
            s.lcd_hz = static_cast<float>((d.vsyncs - d_prev.vsyncs) * 1e6 / dt_us);
            s.pclk_mhz = static_cast<float>(board::display::timing().pclk_hz / 1e6);
            s.isr_pct = static_cast<float>((d.fill_cycles - d_prev.fill_cycles) * 100.0 /
                                           (dt_us * CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ));
            s.isr_max_us = static_cast<float>(d.fill_max_cycles) / CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;
            s.desync = d.desyncs - d_prev.desyncs;
            for (int c = 0; c < 2; ++c) {
                double idle_pct = (idle[c] - idle_prev[c]) * 100.0 / dt_us;
                s.cpu[c] = static_cast<float>(idle_pct > 100 ? 0 : 100 - idle_pct);
            }
            s.ram_kb = heap_caps_get_free_size(MALLOC_CAP_INTERNAL) >> 10;
            s.ram_min_kb = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL) >> 10;
            s.ram_blk_kb = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL) >> 10;
            s.dma_kb = heap_caps_get_free_size(MALLOC_CAP_DMA) >> 10;
            s.psram_kb = heap_caps_get_free_size(MALLOC_CAP_SPIRAM) >> 10;
            s.psram_min_kb = heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM) >> 10;
            s.psram_blk_kb = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM) >> 10;

            char engine[160] = "";
            if (board::display::mode() == board::display::Mode::Engine) {
                uint32_t frames = e.frames - e_prev.frames;
                s.engine = true;
                s.fps = static_cast<float>((d.swaps - d_prev.swaps) * 1e6 / dt_us);
                s.render_ms = frames ? static_cast<float>((e.render_us_sum - e_prev.render_us_sum) / 1000.0 / frames) : 0;
                s.render_max_ms = static_cast<float>(e.render_us_max / 1000.0);
                s.logic_ms = frames ? static_cast<float>((e.logic_us_sum - e_prev.logic_us_sum) / 1000.0 / frames) : 0;
                s.logic_max_ms = static_cast<float>(e.logic_us_max / 1000.0);
                for (int k = 0; k < 3; ++k) s.ticks[k] = e.tick_hist[k] - e_prev.tick_hist[k];
                s.sprites = e.sprites;
                s.buf_psram = e.buffers_in_psram;
                s.world_day = e.world_day;
                s.world_minute = e.world_minute;
                snprintf(engine, sizeof(engine),
                         " | fps %.1f render %.2f/%.2fms logic %.2f/%.2fms ticks %" PRIu32 "/%" PRIu32 "/%" PRIu32
                         " sprites %d buf %s | welt Tag %d %02d:%02d",
                         s.fps, s.render_ms, s.render_max_ms, s.logic_ms, s.logic_max_ms, s.ticks[0], s.ticks[1], s.ticks[2], e.sprites,
                         e.buffers_in_psram ? "psram" : "int", e.world_day, e.world_minute / 60, e.world_minute % 60);
            }
            {
                std::lock_guard<std::mutex> g(s_lock);
                s_snap = s;
            }
            int interval = s_interval_s;
            since_log += kMeasureS;
            since_ring += kMeasureS;
            if (interval > 0 && since_log >= interval) {
                since_log = 0;
                // Nur jede Minute auch in den Log-Puffer (siehe oben)
                const bool to_ring = since_ring >= kRingEveryS;
                if (to_ring) since_ring = 0;
                log::SerialOnly serial_only(!to_ring);
                ESP_LOGI(TAG, "%s | lcd %.2fHz pclk %.2fMHz isr %.1f%% (max %.0fus) desync %" PRIu32 "%s | cpu0 %.0f%% cpu1 %.0f%% | ram %uk (min %uk, blk %uk) psram %uk",
                         s.mode, s.lcd_hz, s.pclk_mhz, s.isr_pct, s.isr_max_us, s.desync, engine, s.cpu[0], s.cpu[1],
                         s.ram_kb, s.ram_min_kb, s.ram_blk_kb, s.psram_kb);
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
