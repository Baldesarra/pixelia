// Diagnose-Schnittstelle über die serielle Konsole (UART0, 115200 Baud).
// Eine Zeile = ein Befehl. Gedacht für tools/monitor.ps1 -Send, funktioniert aber auch im Terminal.

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "app.h"
#include "board/backlight.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace app::console {

namespace {
constexpr const char* TAG = "cmd";
constexpr uart_port_t kUart = UART_NUM_0;

void print_help() {
    ESP_LOGI(TAG, "Befehle:");
    ESP_LOGI(TAG, "  info | heap | scan | touch | touchrst | reboot");
    ESP_LOGI(TAG, "  rot <0|180>                                   Bild drehen");
    ESP_LOGI(TAG, "  tour [tap] | flash <Sekunden> | marks <n>      geführter Test (tap: Start per Tippen), Flash-Last");
    ESP_LOGI(TAG, "  pat <black|bars|frame|motion|touch|engine>   Testbild/Modus");
    ESP_LOGI(TAG, "  bl <0-100> | bl cycle | bl stop | blf <Hz>      Hintergrundbeleuchtung");
    ESP_LOGI(TAG, "  pclk <MHz>                                    Pixeltakt zur Laufzeit");
    ESP_LOGI(TAG, "  timing <hfp> <hpw> <hbp> <vfp> <vpw> <vbp>     Porches (Panel neu anlegen)");
    ESP_LOGI(TAG, "  pol <hsync_idle_low> <vsync_idle_low> <pclk_neg>  Polaritäten 0/1");
    ESP_LOGI(TAG, "  bounce <Zeilen> | clk <160|240>                Bounce-Buffer, Taktquelle");
    ESP_LOGI(TAG, "  swap <n> | sprites <n> | buf <int|psram>       Engine-Prototyp");
    ESP_LOGI(TAG, "  time <HH:MM> | speed <Faktor>                Weltzeit stellen, Zeitraffer");
    ESP_LOGI(TAG, "  perf <Sekunden> | perf off                     Leistungsmonitor");
}

void log_timing(const board::display::Timing& t) {
    ESP_LOGI(TAG, "Timing: pclk %.2f MHz, H %u/%u/%u, V %u/%u/%u, pol %d/%d/%d, bounce %d, clk %s", t.pclk_hz / 1e6,
             t.hsync_front, t.hsync_pulse, t.hsync_back, t.vsync_front, t.vsync_pulse, t.vsync_back, t.hsync_idle_low,
             t.vsync_idle_low, t.pclk_active_neg, t.bounce_lines, t.pll240 ? "PLL240" : "PLL160");
}

void handle(char* line) {
    char* argv[8] = {};
    int argc = 0;
    for (char* tok = std::strtok(line, " \t"); tok && argc < 8; tok = std::strtok(nullptr, " \t")) argv[argc++] = tok;
    if (argc == 0) return;
    const char* cmd = argv[0];
    auto is = [&](const char* name) { return std::strcmp(cmd, name) == 0; };
    using board::display::Mode;

    if (is("help") || is("?")) {
        print_help();
    } else if (is("info")) {
        sysinfo::log_all();
        log_timing(board::display::timing());
    } else if (is("heap")) {
        sysinfo::log_heap();
    } else if (is("scan")) {
        hwtest::scan();
    } else if (is("touchrst")) {
        hwtest::touch_reset_test();
    } else if (is("touch")) {
        hwtest::log_touch_status();
    } else if (is("tour2")) {
        hwtest::start_tour2();
    } else if (is("tour")) {
        hwtest::start_tour(!(argc >= 2 && std::strcmp(argv[1], "tap") == 0));
    } else if (is("flash") && argc >= 2) {
        hwtest::flash_stress(std::atoi(argv[1]));
    } else if (is("marks") && argc >= 2) {
        gfx::set_marks(std::atoi(argv[1]));
    } else if (is("time") && argc >= 2) {
        int h = 0, m = 0;
        if (std::sscanf(argv[1], "%d:%d", &h, &m) >= 1 && h >= 0 && h < 24 && m >= 0 && m < 60) {
            gfx::set_time_of_day(h, m);
            ESP_LOGI(TAG, "Weltzeit %02d:%02d", h, m);
        } else {
            ESP_LOGW(TAG, "Uhrzeit als HH:MM angeben");
        }
    } else if (is("speed") && argc >= 2) {
        gfx::set_speed(static_cast<float>(std::atof(argv[1])));
        ESP_LOGI(TAG, "Zeitraffer %.1fx", gfx::speed());
    } else if (is("rot") && argc >= 2) {
        board::display::set_rotation180(std::atoi(argv[1]) == 180);
        ESP_LOGI(TAG, "Drehung %d°", board::display::rotation180() ? 180 : 0);
    } else if (is("reboot")) {
        ESP_LOGI(TAG, "Neustart");
        vTaskDelay(pdMS_TO_TICKS(100));
        esp_restart();
    } else if (is("pat") && argc >= 2) {
        Mode m;
        if (!board::display::mode_from_name(argv[1], &m)) {
            ESP_LOGW(TAG, "Unbekanntes Testbild: %s", argv[1]);
            return;
        }
        if (m == Mode::Touch) hwtest::reset_corner_hits();
        board::display::set_mode(m);
        ESP_LOGI(TAG, "Testbild: %s", argv[1]);
    } else if (is("bl") && argc >= 2) {
        if (std::strcmp(argv[1], "cycle") == 0) {
            hwtest::backlight_cycle(true);
        } else if (std::strcmp(argv[1], "stop") == 0) {
            hwtest::backlight_cycle(false);
        } else {
            hwtest::backlight_cycle(false);
            board::backlight::set_percent(std::atoi(argv[1]));
            ESP_LOGI(TAG, "Helligkeit %d %%", board::backlight::percent());
        }
    } else if (is("blf") && argc >= 2) {
        esp_err_t err = board::backlight::set_frequency(static_cast<uint32_t>(std::atoi(argv[1])));
        ESP_LOGI(TAG, "PWM-Frequenz %lu Hz: %s", static_cast<unsigned long>(board::backlight::frequency()), esp_err_to_name(err));
    } else if (is("pclk") && argc >= 2) {
        auto hz = static_cast<uint32_t>(std::atof(argv[1]) * 1e6);
        esp_err_t err = board::display::set_pclk(hz);
        ESP_LOGI(TAG, "Pixeltakt %.2f MHz: %s (Bild %.2f Hz laut Timing)", hz / 1e6, esp_err_to_name(err),
                 1e6 / board::display::frame_period_us());
    } else if (is("timing") && argc >= 7) {
        board::display::Timing t = board::display::timing();
        t.hsync_front = static_cast<uint16_t>(std::atoi(argv[1]));
        t.hsync_pulse = static_cast<uint16_t>(std::atoi(argv[2]));
        t.hsync_back = static_cast<uint16_t>(std::atoi(argv[3]));
        t.vsync_front = static_cast<uint16_t>(std::atoi(argv[4]));
        t.vsync_pulse = static_cast<uint16_t>(std::atoi(argv[5]));
        t.vsync_back = static_cast<uint16_t>(std::atoi(argv[6]));
        log_timing(t);
        gfx::request_reconfigure(t);
    } else if (is("pol") && argc >= 4) {
        board::display::Timing t = board::display::timing();
        t.hsync_idle_low = std::atoi(argv[1]) != 0;
        t.vsync_idle_low = std::atoi(argv[2]) != 0;
        t.pclk_active_neg = std::atoi(argv[3]) != 0;
        log_timing(t);
        gfx::request_reconfigure(t);
    } else if (is("bounce") && argc >= 2) {
        board::display::Timing t = board::display::timing();
        t.bounce_lines = std::atoi(argv[1]);
        log_timing(t);
        gfx::request_reconfigure(t);
    } else if (is("clk") && argc >= 2) {
        board::display::Timing t = board::display::timing();
        t.pll240 = std::atoi(argv[1]) == 240;
        log_timing(t);
        gfx::request_reconfigure(t);
    } else if (is("swap") && argc >= 2) {
        board::display::set_swap_interval(std::atoi(argv[1]));
        ESP_LOGI(TAG, "Tauschintervall %d Bild(er)", board::display::swap_interval());
    } else if (is("sprites") && argc >= 2) {
        gfx::set_sprites(std::atoi(argv[1]));
        ESP_LOGI(TAG, "Sprites %d", std::atoi(argv[1]));
    } else if (is("buf") && argc >= 2) {
        bool psram = std::strcmp(argv[1], "psram") == 0;
        gfx::request_buffers(psram);
        ESP_LOGI(TAG, "Engine-Puffer: %s", psram ? "PSRAM" : "intern");
    } else if (is("perf") && argc >= 2) {
        perf::set_interval(std::strcmp(argv[1], "off") == 0 ? 0 : std::atoi(argv[1]));
        ESP_LOGI(TAG, "Leistungsmonitor: %s", perf::interval() ? "an" : "aus");
    } else {
        ESP_LOGW(TAG, "Unbekannt: %s (help für Liste)", cmd);
    }
}

void console_task(void*) {
    char line[128];
    size_t len = 0;
    for (;;) {
        uint8_t ch;
        if (uart_read_bytes(kUart, &ch, 1, portMAX_DELAY) != 1) continue;
        if (ch == '\r' || ch == '\n') {
            if (len == 0) continue;
            line[len] = 0;
            ESP_LOGI(TAG, "> %s", line);
            handle(line);
            len = 0;
        } else if ((ch == 8 || ch == 127) && len > 0) {
            --len;
        } else if (ch >= 32 && len < sizeof(line) - 1) {
            line[len++] = static_cast<char>(ch);
        }
    }
}
}  // namespace

void start() {
    // Nur Empfang über den Treiber; die Log-Ausgabe läuft unverändert weiter
    ESP_ERROR_CHECK(uart_driver_install(kUart, 1024, 0, 0, nullptr, 0));
    xTaskCreatePinnedToCore(console_task, "console", 4096, nullptr, 4, nullptr, kSystemCore);
}

}  // namespace app::console
