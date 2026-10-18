// Pixelia – Phase 0.1: Hardware-Detektiv und Engine-Prototyp
//
// Ablauf beim Start:
//  1. Systeminfo ins Log (Chip, Flash, PSRAM, Heap, Resetgrund)
//  2. I²C-Scan und GT911-Erkennung, Touch-Abfrage im Hintergrund
//  3. Hintergrundbeleuchtung (PWM) und RGB-Panel auf Kern 1, Testbild „bars“
//  4. Diagnose-Schnittstelle über die serielle Konsole (Befehl „help“), Leistungsmonitor alle 2 s

#include "app.h"
#include "board/backlight.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {
constexpr const char* TAG = "app";
}

extern "C" void app_main() {
    app::sysinfo::log_all();

    app::hwtest::init_i2c_and_scan();
    app::hwtest::start_touch_task();

    ESP_ERROR_CHECK(board::backlight::init());
    board::backlight::set_percent(100);

    board::display::Timing timing;  // Startwerte aus docs/HARDWARE.md
    app::gfx::start(timing);

    app::console::start();
    app::perf::start(2);

    // Platzhalter für den Selbsttest ab 0.2.x: ohne Absturz gestartet = gültig
    vTaskDelay(pdMS_TO_TICKS(3000));
    esp_ota_mark_app_valid_cancel_rollback();
    ESP_LOGI(TAG, "Start abgeschlossen. Befehle: help");
    app::sysinfo::log_heap();
}
