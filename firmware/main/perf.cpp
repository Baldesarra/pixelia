// Leistungsmonitor: schreibt alle N Sekunden eine kompakte Zeile ins Log
// (Bildrate, Zeichenzeit, Interrupt-Last, CPU-Last pro Kern, freier Speicher).
// Später auch in der Weboberfläche und per Schalter abschaltbar (Experimentell).

#include <cinttypes>

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
volatile int s_interval_s = 2;

void perf_task(void*) {
    int64_t t_prev = esp_timer_get_time();
    board::display::Stats d_prev = board::display::stats(true);
    gfx::EngineStats e_prev = gfx::engine_stats(true);
    uint32_t idle_prev[2] = {ulTaskGetIdleRunTimeCounterForCore(0), ulTaskGetIdleRunTimeCounterForCore(1)};
    for (;;) {
        int interval = s_interval_s;
        vTaskDelay(pdMS_TO_TICKS(interval > 0 ? interval * 1000 : 500));
        int64_t t_now = esp_timer_get_time();
        board::display::Stats d = board::display::stats(true);
        gfx::EngineStats e = gfx::engine_stats(true);
        uint32_t idle[2] = {ulTaskGetIdleRunTimeCounterForCore(0), ulTaskGetIdleRunTimeCounterForCore(1)};
        double dt_us = static_cast<double>(t_now - t_prev);

        if (interval > 0 && dt_us > 0) {
            double hz = (d.vsyncs - d_prev.vsyncs) * 1e6 / dt_us;
            double isr_pct = (d.fill_cycles - d_prev.fill_cycles) * 100.0 / (dt_us * CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);
            double isr_max_us = static_cast<double>(d.fill_max_cycles) / CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;
            uint32_t desync = d.desyncs - d_prev.desyncs;
            double cpu[2];
            for (int c = 0; c < 2; ++c) {
                double idle_pct = (idle[c] - idle_prev[c]) * 100.0 / dt_us;
                cpu[c] = idle_pct > 100 ? 0 : 100 - idle_pct;
            }
            unsigned ram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL) >> 10;
            unsigned ram_min = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL) >> 10;
            unsigned ram_blk = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL) >> 10;
            unsigned psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM) >> 10;

            char engine[160] = "";
            if (board::display::mode() == board::display::Mode::Engine) {
                uint32_t frames = e.frames - e_prev.frames;
                double fps = (d.swaps - d_prev.swaps) * 1e6 / dt_us;
                double render_ms = frames ? (e.render_us_sum - e_prev.render_us_sum) / 1000.0 / frames : 0;
                double logic_ms = frames ? (e.logic_us_sum - e_prev.logic_us_sum) / 1000.0 / frames : 0;
                snprintf(engine, sizeof(engine),
                         " | fps %.1f render %.2f/%.2fms logic %.2fms ticks %" PRIu32 "/%" PRIu32 "/%" PRIu32
                         " sprites %d buf %s",
                         fps, render_ms, e.render_us_max / 1000.0, logic_ms, e.tick_hist[0] - e_prev.tick_hist[0],
                         e.tick_hist[1] - e_prev.tick_hist[1], e.tick_hist[2] - e_prev.tick_hist[2], e.sprites,
                         e.buffers_in_psram ? "psram" : "int");
            }
            ESP_LOGI(TAG, "%s | lcd %.2fHz pclk %.2fMHz isr %.1f%% (max %.0fus) desync %" PRIu32 "%s | cpu0 %.0f%% cpu1 %.0f%% | ram %uk (min %uk, blk %uk) psram %uk",
                     board::display::mode_name(board::display::mode()), hz, board::display::timing().pclk_hz / 1e6,
                     isr_pct, isr_max_us, desync, engine, cpu[0], cpu[1], ram, ram_min, ram_blk, psram);
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

}  // namespace app::perf
