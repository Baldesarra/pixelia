// Pixelia – Firmware
//
// Ablauf beim Start:
//  0. Log-Ringpuffer übernehmen (Zeilen vor einem Neustart oder Absturz bleiben per WLAN abrufbar)
//  1. Systeminfo ins Log (Chip, Flash, PSRAM, Heap, Resetgrund), Einstellungen aus dem NVS
//  2. Update-Zustand: Probelauf einer neuen Version? Wurde ein Update zurückgerollt? Startzähler, Reset-Historie
//  3. I²C-Scan und GT911-Erkennung, Touch-Abfrage im Hintergrund
//  4. Hintergrundbeleuchtung und RGB-Panel auf Kern 1, Engine (30 fps) mit Systemanzeigen
//  5. Diagnose über die serielle Konsole (Befehl „help“), Leistungsmessung alle 2 s
//  6. WLAN (oder Setup-Modus), mDNS, NTP, Webserver auf Kern 0
//  7. Im Probelauf: Selbsttest, danach gilt die neue Version dauerhaft

#include "app.h"
#include "auth.h"
#include "board/backlight.h"
#include "diag.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "logbuf.h"
#include "net.h"
#include "exper.h"
#include "github.h"
#include "ota.h"
#include "settings.h"
#include "web.h"

namespace {
constexpr const char* TAG = "app";
}

extern "C" void app_main() {
    // Tiefstwert des freien Speichers ab hier messen: Der beim Systemstart reservierte DMA-Pool (32 KB,
    // SPIRAM_MALLOC_RESERVE_INTERNAL) meldet sonst einen Tiefstwert von 0 und der Wert wäre immer 31 KB zu niedrig
    heap_caps_monitor_local_minimum_free_size_start();
    app::timesync::set_timezone();
    app::log::init();
    app::sysinfo::log_all();
    app::settings::init();
    app::exper::init();
    app::auth::init();
    app::ota::init();
    app::github::init();
    app::diag::init();
    app::sysinfo::log_heap_step("Einstellungen");

    app::hwtest::init_i2c_and_scan();
    app::hwtest::start_touch_task();

    const app::settings::Values set = app::settings::get();
    ESP_ERROR_CHECK(board::backlight::init());
    board::backlight::set_percent(set.brightness);
    ESP_LOGI("bl", "Hintergrundbeleuchtung: PWM %lu Hz, 12 Bit, %d %%",
             static_cast<unsigned long>(board::backlight::frequency()), board::backlight::percent());

    // Geprüfte Werte aus docs/HARDWARE.md: Panel 60 Hz, jedes Engine-Bild zweimal zeigen = 30 fps.
    // Eigene Werte von der Seite Experimentell nur, wenn bestätigt gespeichert (exper.h)
    const board::display::Timing timing = app::exper::boot_timing();
    board::display::set_rotation180(set.rotate180);
    board::display::set_swap_interval(app::exper::boot_swap());
    board::display::set_mode(board::display::Mode::Engine);
    app::gfx::start(timing);

    app::console::start();
    app::perf::start(2);
    vTaskDelay(pdMS_TO_TICKS(500));  // Engine-Puffer anlegen lassen, dann WLAN (Speicherspitzen nicht überlagern)
    app::sysinfo::log_heap_step("Display und Engine");

    app::wifi::start();
    app::sysinfo::log_heap_step("WLAN");
    app::web::start();
    app::sysinfo::log_heap_step("Webserver");

    app::ota::start_selftest();
    ESP_LOGI(TAG, "Start abgeschlossen. Befehle: help, Weboberfläche: http://%s.local", app::wifi::kMdnsHost);
}
