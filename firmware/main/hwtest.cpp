// Hardware-Detektiv: I²C-Scan, GT911-Touch-Test, Stufentest der Hintergrundbeleuchtung

#include <array>
#include <cstdlib>

#include "app.h"
#include "board/backlight.h"
#include "board/pins.h"
#include "board/touch_gt911.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace app::hwtest {

namespace {
constexpr const char* TAG = "i2c";
constexpr const char* TAG_TOUCH = "touch";
constexpr const char* TAG_BL = "bl";

i2c_master_bus_handle_t s_bus = nullptr;
volatile bool s_touch_ok = false;
volatile bool s_touch_pause = false;
board::touch::Info s_info{};
volatile uint8_t s_corners = 0;
TaskHandle_t s_bl_task = nullptr;

bool probe(uint8_t addr) { return i2c_master_probe(s_bus, addr, 20) == ESP_OK; }

bool find_and_init_touch() {
    for (uint8_t addr : std::array<uint8_t, 2>{board::touch::kAddrPrimary, board::touch::kAddrSecondary}) {
        if (!probe(addr)) continue;
        if (board::touch::init(s_bus, addr, &s_info) == ESP_OK) {
            ESP_LOGI(TAG_TOUCH, "GT911 auf 0x%02X: Produkt \"%s\", Firmware 0x%04X, Auflösung %ux%u, Hersteller 0x%02X, Konfig v%u",
                     addr, s_info.product, s_info.firmware, s_info.x_resolution, s_info.y_resolution, s_info.vendor,
                     s_info.config_version);
            return true;
        }
    }
    return false;
}

// Touch-Koordinaten auf 800×480 abbilden (falls der GT911 anders konfiguriert ist)
void map_point(const board::touch::Point& p, int* x, int* y) {
    int xr = s_info.x_resolution ? s_info.x_resolution : board::kLcdWidth;
    int yr = s_info.y_resolution ? s_info.y_resolution : board::kLcdHeight;
    *x = p.x * board::kLcdWidth / xr;
    *y = p.y * board::kLcdHeight / yr;
}

void touch_task(void*) {
    bool down = false;
    int last_x = -1, last_y = -1;
    int errors = 0;
    TickType_t last_move_log = 0;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(20));
        if (!s_touch_ok || s_touch_pause) continue;
        board::touch::Point pts[5];
        int n = board::touch::read(pts, 5);
        if (n == board::touch::kNoData) continue;
        if (n < 0) {
            if (++errors % 50 == 1) ESP_LOGW(TAG_TOUCH, "Lesefehler (%d)", errors);
            continue;
        }
        if (n == 0) {
            if (down) {
                ESP_LOGI(TAG_TOUCH, "los bei x=%d y=%d", last_x, last_y);
                down = false;
                board::display::set_touch_marker(-1, -1);
            }
            continue;
        }
        int x, y;
        map_point(pts[0], &x, &y);
        if (!down) {
            ESP_LOGI(TAG_TOUCH, "berührt x=%d y=%d (roh %u/%u, Größe %u, Finger %d)", x, y, pts[0].x, pts[0].y,
                     pts[0].size, n);
            down = true;
            last_move_log = xTaskGetTickCount();
            // Eckfelder 100×100 px
            uint8_t bit = 0;
            if (x < 100 && y < 100) bit = 1;
            else if (x >= board::kLcdWidth - 100 && y < 100) bit = 2;
            else if (x < 100 && y >= board::kLcdHeight - 100) bit = 4;
            else if (x >= board::kLcdWidth - 100 && y >= board::kLcdHeight - 100) bit = 8;
            if (bit && !(s_corners & bit)) {
                s_corners |= bit;
                board::display::set_corner_hits(s_corners);
                static const char* names[] = {"oben links", "oben rechts", "unten links", "unten rechts"};
                ESP_LOGI(TAG_TOUCH, "Ecke %s getroffen", names[__builtin_ctz(bit)]);
                if (s_corners == 0x0F) ESP_LOGI(TAG_TOUCH, "Alle vier Ecken getroffen");
            }
        } else if (xTaskGetTickCount() - last_move_log > pdMS_TO_TICKS(500) && (std::abs(x - last_x) > 3 || std::abs(y - last_y) > 3)) {
            ESP_LOGI(TAG_TOUCH, "bewegt x=%d y=%d", x, y);
            last_move_log = xTaskGetTickCount();
        }
        last_x = x;
        last_y = y;
        board::display::set_touch_marker(x, y);
    }
}

void backlight_task(void*) {
    static const int kLevels[] = {100, 70, 40, 20, 10, 5, 1, 0};
    for (;;) {
        for (int level : kLevels) {
            board::backlight::set_percent(level);
            ESP_LOGI(TAG_BL, "Helligkeit %d %%", level);
            vTaskDelay(pdMS_TO_TICKS(1500));
        }
    }
}
}  // namespace

void init_i2c_and_scan() {
    i2c_master_bus_config_t cfg = {};
    cfg.i2c_port = -1;
    cfg.sda_io_num = board::kTouchSda;
    cfg.scl_io_num = board::kTouchScl;
    cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    cfg.glitch_ignore_cnt = 7;
    cfg.flags.enable_internal_pullup = 1;
    esp_err_t err = i2c_new_master_bus(&cfg, &s_bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I²C-Bus auf SDA %d / SCL %d nicht angelegt: %s", board::kTouchSda, board::kTouchScl,
                 esp_err_to_name(err));
        return;
    }
    scan();
    s_touch_ok = find_and_init_touch();
    if (!s_touch_ok) {
        ESP_LOGW(TAG_TOUCH, "GT911 antwortet nicht, Reset über GPIO %d und neuer Versuch", board::kTouchRst);
        board::touch::hardware_reset();
        scan();
        s_touch_ok = find_and_init_touch();
        if (!s_touch_ok) ESP_LOGE(TAG_TOUCH, "Kein GT911 gefunden");
    }
}

void scan() {
    if (!s_bus) return;
    int found = 0;
    for (uint8_t addr = 0x08; addr < 0x78; ++addr) {
        if (probe(addr)) {
            ESP_LOGI(TAG, "Gerät auf 0x%02X (SDA %d, SCL %d)", addr, board::kTouchSda, board::kTouchScl);
            ++found;
        }
    }
    ESP_LOGI(TAG, "Scan fertig: %d Gerät(e)", found);
}

void touch_reset_test() {
    if (!s_bus) return;
    s_touch_pause = true;
    vTaskDelay(pdMS_TO_TICKS(50));
    uint8_t addr = s_info.address ? s_info.address : board::touch::kAddrPrimary;
    bool before = probe(addr);
    board::touch::hold_reset(true);
    vTaskDelay(pdMS_TO_TICKS(30));
    bool during = probe(board::touch::kAddrPrimary) || probe(board::touch::kAddrSecondary);
    board::touch::hold_reset(false);
    vTaskDelay(pdMS_TO_TICKS(150));
    bool after_primary = probe(board::touch::kAddrPrimary);
    bool after_secondary = probe(board::touch::kAddrSecondary);
    ESP_LOGI(TAG_TOUCH, "RST-Test GPIO %d: vorher %s, bei RST=low %s, danach 0x5D %s / 0x14 %s", board::kTouchRst,
             before ? "da" : "weg", during ? "da" : "weg", after_primary ? "da" : "weg", after_secondary ? "da" : "weg");
    if (before && !during && (after_primary || after_secondary)) {
        ESP_LOGI(TAG_TOUCH, "RST-Leitung bestätigt (GPIO %d)", board::kTouchRst);
    } else {
        ESP_LOGW(TAG_TOUCH, "RST-Leitung nicht eindeutig bestätigt");
    }
    s_touch_ok = find_and_init_touch();
    s_touch_pause = false;
}

void start_touch_task() { xTaskCreatePinnedToCore(touch_task, "touch", 4096, nullptr, 5, nullptr, kSystemCore); }

void reset_corner_hits() {
    s_corners = 0;
    board::display::set_corner_hits(0);
}

void backlight_cycle(bool on) {
    if (on && !s_bl_task) {
        xTaskCreatePinnedToCore(backlight_task, "blcycle", 3072, nullptr, 3, &s_bl_task, kSystemCore);
    } else if (!on && s_bl_task) {
        vTaskDelete(s_bl_task);
        s_bl_task = nullptr;
        board::backlight::set_percent(100);
        ESP_LOGI(TAG_BL, "Stufentest beendet, Helligkeit 100 %%");
    }
}

}  // namespace app::hwtest
