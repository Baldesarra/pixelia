// Hardware-Detektiv: I²C-Scan, GT911-Touch-Test, Stufentest der Hintergrundbeleuchtung

#include <array>
#include <cstdlib>

#include "app.h"
#include "board/backlight.h"
#include "board/pins.h"
#include "board/touch_gt911.h"
#include "input.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "esp_timer.h"
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

// Touch-Ergebnisse bleiben gespeichert, damit sie später per Befehl „touch“ abgefragt werden können
struct TouchRecord {
    uint32_t touches = 0;
    int corner_xy[4][2] = {{-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}};
    int min_x = 9999, min_y = 9999, max_x = -1, max_y = -1;
    int last_x = -1, last_y = -1;
};
TouchRecord s_rec;
volatile bool s_down = false;  // gerade berührt (für den Touch-Test der Weboberfläche)
constexpr const char* kCornerNames[] = {"oben links", "oben rechts", "unten links", "unten rechts"};

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

// Touch-Koordinaten auf 800×480 in Bildausrichtung abbilden
// (falls der GT911 anders konfiguriert ist, und passend zur 180°-Drehung des Bildes)
void map_point(const board::touch::Point& p, int* x, int* y) {
    int xr = s_info.x_resolution ? s_info.x_resolution : board::kLcdWidth;
    int yr = s_info.y_resolution ? s_info.y_resolution : board::kLcdHeight;
    *x = p.x * board::kLcdWidth / xr;
    *y = p.y * board::kLcdHeight / yr;
    if (board::display::rotation180()) {
        *x = board::kLcdWidth - 1 - *x;
        *y = board::kLcdHeight - 1 - *y;
    }
}

void touch_task(void*) {
    bool down = false;
    int last_x = -1, last_y = -1;
    int errors = 0;
    TickType_t last_move_log = 0;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(20));
        input::tick();
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
                s_down = false;
                board::display::set_touch_marker(-1, -1);
                input::on_touch(0, last_x, last_y);
            }
            continue;
        }
        int x, y;
        map_point(pts[0], &x, &y);
        input::on_touch(n, x, y);
        if (!down) {
            ESP_LOGI(TAG_TOUCH, "berührt x=%d y=%d (roh %u/%u, Größe %u, Finger %d)", x, y, pts[0].x, pts[0].y,
                     pts[0].size, n);
            down = true;
            s_down = true;
            last_move_log = xTaskGetTickCount();
            s_rec.touches++;
            // Eckfelder 100×100 px
            uint8_t bit = 0;
            if (x < 100 && y < 100) bit = 1;
            else if (x >= board::kLcdWidth - 100 && y < 100) bit = 2;
            else if (x < 100 && y >= board::kLcdHeight - 100) bit = 4;
            else if (x >= board::kLcdWidth - 100 && y >= board::kLcdHeight - 100) bit = 8;
            if (bit) {
                int idx = __builtin_ctz(bit);
                s_rec.corner_xy[idx][0] = x;
                s_rec.corner_xy[idx][1] = y;
            }
            if (bit && !(s_corners & bit)) {
                s_corners |= bit;
                board::display::set_corner_hits(s_corners);
                ESP_LOGI(TAG_TOUCH, "Ecke %s getroffen", kCornerNames[__builtin_ctz(bit)]);
                if (s_corners == 0x0F) ESP_LOGI(TAG_TOUCH, "Alle vier Ecken getroffen");
            }
        } else if (xTaskGetTickCount() - last_move_log > pdMS_TO_TICKS(500) && (std::abs(x - last_x) > 3 || std::abs(y - last_y) > 3)) {
            ESP_LOGI(TAG_TOUCH, "bewegt x=%d y=%d", x, y);
            last_move_log = xTaskGetTickCount();
        }
        last_x = x;
        last_y = y;
        s_rec.last_x = x;
        s_rec.last_y = y;
        if (x < s_rec.min_x) s_rec.min_x = x;
        if (x > s_rec.max_x) s_rec.max_x = x;
        if (y < s_rec.min_y) s_rec.min_y = y;
        if (y > s_rec.max_y) s_rec.max_y = y;
        board::display::set_touch_marker(x, y);
    }
}

constexpr int kBlLevels[] = {100, 70, 40, 20, 10, 5, 1, 0};

// Flash-Last wie bei Spielständen: seit 0.5.5 Dateien in LittleFS, geschrieben von der Lade-Task (assets.cpp, dort
// genug Stack); bis 0.5.4 roh in „storage“, das würde LittleFS zerstören. Wartet, bis der Test fertig ist.
void flash_stress_impl(int seconds) {
    if (!assets::flash_load(seconds)) ESP_LOGE("flash", "Flash-Last nicht gelaufen (LittleFS bzw. Lade-Task)");
}

void backlight_steps_once() {
    for (int level : kBlLevels) {
        board::backlight::set_percent(level);
        ESP_LOGI(TAG_BL, "Helligkeit %d %%", level);
        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}

void backlight_task(void*) {
    for (;;) backlight_steps_once();
}

// Geführter Ablauf im Tempo des Users. Die weißen Quadrate oben links zeigen die Stufe an:
//  (Ecken antippen) → 1: Szene 60 Hz → 1: Helligkeitsstufen → 2/3/4: Pixeltakt 30/36/40 MHz
//  → 5: Flash-Schreiben während der Animation → 1: zurück auf 60 Hz
void tour_stage(int marks, uint32_t pclk_hz, const char* text) {
    board::display::set_pclk(pclk_hz);
    gfx::set_marks(marks);
    ESP_LOGI("tour", "Stufe %d: %s (Pixeltakt %.1f MHz)", marks, text, pclk_hz / 1e6);
}

void tour_task(void* arg) {
    constexpr uint32_t kBase = 24600000;
    bool with_corners = arg == nullptr;
    board::display::set_swap_interval(2);
    if (with_corners) {
        ESP_LOGI("tour", "Start: vier Ecken antippen");
        reset_corner_hits();
        board::display::set_mode(board::display::Mode::Touch);
        while (s_corners != 0x0F) vTaskDelay(pdMS_TO_TICKS(100));
        vTaskDelay(pdMS_TO_TICKS(1500));
        tour_stage(1, kBase, "Engine-Szene");
        board::display::set_mode(board::display::Mode::Engine);
        vTaskDelay(pdMS_TO_TICKS(12000));
    } else {
        // Ohne Ecken: Szene läuft, der Ablauf startet erst, wenn der User aufs Display tippt
        tour_stage(0, kBase, "warte auf Tippen");
        board::display::set_mode(board::display::Mode::Engine);
        uint32_t touches = s_rec.touches;
        while (s_rec.touches == touches) vTaskDelay(pdMS_TO_TICKS(100));
        tour_stage(1, kBase, "Engine-Szene");
        vTaskDelay(pdMS_TO_TICKS(4000));
    }
    ESP_LOGI("tour", "Helligkeitsstufen");
    backlight_steps_once();
    board::backlight::set_percent(100);
    vTaskDelay(pdMS_TO_TICKS(1500));
    tour_stage(2, 30000000, "höherer Pixeltakt");
    vTaskDelay(pdMS_TO_TICKS(6000));
    tour_stage(3, 36000000, "höherer Pixeltakt");
    vTaskDelay(pdMS_TO_TICKS(6000));
    tour_stage(4, 40000000, "höherer Pixeltakt");
    vTaskDelay(pdMS_TO_TICKS(6000));
    tour_stage(5, kBase, "Flash-Schreiben");
    vTaskDelay(pdMS_TO_TICKS(1000));
    flash_stress(8);
    vTaskDelay(pdMS_TO_TICKS(1000));
    tour_stage(1, kBase, "fertig");
    ESP_LOGI("tour", "Fertig");
    vTaskDelete(nullptr);
}

void wait_for_tap() {
    uint32_t touches = s_rec.touches;
    while (s_rec.touches == touches) vTaskDelay(pdMS_TO_TICKS(100));
}

// Zweiter geführter Test, zwei Teile, jeweils Start per Tippen:
//  Teil 1, Quadrate 1..6: PWM-Frequenz 200 Hz, 500 Hz, 1 kHz, 2 kHz, 5 kHz, 10 kHz,
//          je 100 % → 50 % → 15 % → 100 % (Welche Frequenz dimmt, statt auszugehen?)
//  Teil 2: 1 Quadrat normal, 2 Quadrate Flash-Schreiben, 1 Quadrat normal (A/B-Vergleich bei gleichem Takt)
void tour2_task(void*) {
    constexpr uint32_t kFreqs[] = {200, 500, 1000, 2000, 5000, 10000};
    uint32_t old_freq = board::backlight::frequency();
    board::display::set_swap_interval(2);
    tour_stage(0, 24600000, "Test 2, warte auf Tippen (Helligkeit)");
    board::display::set_mode(board::display::Mode::Engine);
    wait_for_tap();
    for (int i = 0; i < 6; ++i) {
        gfx::set_marks(i + 1);
        esp_err_t err = board::backlight::set_frequency(kFreqs[i]);
        ESP_LOGI("tour", "Stufe %d: PWM %lu Hz (%s)", i + 1, static_cast<unsigned long>(kFreqs[i]), esp_err_to_name(err));
        board::backlight::set_percent(100);
        vTaskDelay(pdMS_TO_TICKS(1000));
        board::backlight::set_percent(50);
        vTaskDelay(pdMS_TO_TICKS(2000));
        board::backlight::set_percent(15);
        vTaskDelay(pdMS_TO_TICKS(2000));
        board::backlight::set_percent(100);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    board::backlight::set_frequency(old_freq);
    board::backlight::set_percent(100);
    gfx::set_marks(0);
    ESP_LOGI("tour", "Teil 1 fertig, warte auf Tippen (Flüssigkeit)");
    wait_for_tap();
    gfx::set_marks(1);
    ESP_LOGI("tour", "A: normal");
    vTaskDelay(pdMS_TO_TICKS(10000));
    gfx::set_marks(2);
    ESP_LOGI("tour", "B: Flash-Schreiben");
    flash_stress(10);
    gfx::set_marks(1);
    ESP_LOGI("tour", "A: normal");
    vTaskDelay(pdMS_TO_TICKS(10000));
    gfx::set_marks(0);
    ESP_LOGI("tour", "Fertig");
    vTaskDelete(nullptr);
}
}  // namespace

void start_tour2() { xTaskCreatePinnedToCore(tour2_task, "tour2", 3072, nullptr, 3, nullptr, kSystemCore); }

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

void flash_stress(int seconds) { flash_stress_impl(seconds); }

void start_tour(bool with_corners) {
    // Argument nullptr = mit Ecken, sonst Start per Tippen
    static int no_corners_tag;
    xTaskCreatePinnedToCore(tour_task, "tour", 3072, with_corners ? nullptr : &no_corners_tag, 3, nullptr, kSystemCore);
}

void start_touch_task() { xTaskCreatePinnedToCore(touch_task, "touch", 4096, nullptr, 5, nullptr, kSystemCore); }

TouchLive touch_live() {
    return TouchLive{s_touch_ok, s_down, s_rec.last_x, s_rec.last_y, s_corners, s_rec.touches};
}

void reset_corner_hits() {
    s_corners = 0;
    s_rec = TouchRecord{};
    board::display::set_corner_hits(0);
}

bool touch_info(board::touch::Info* info, uint32_t* touches) {
    *info = s_info;
    *touches = s_rec.touches;
    return s_touch_ok;
}

void log_touch_status() {
    ESP_LOGI(TAG_TOUCH, "Status: %s, %lu Berührungen, Ecken %u/4, Bereich x %d..%d y %d..%d, zuletzt %d/%d",
             s_touch_ok ? "GT911 ok" : "kein GT911", static_cast<unsigned long>(s_rec.touches),
             static_cast<unsigned>(__builtin_popcount(s_corners)), s_rec.min_x, s_rec.max_x, s_rec.min_y, s_rec.max_y,
             s_rec.last_x, s_rec.last_y);
    for (int i = 0; i < 4; ++i) {
        if (s_rec.corner_xy[i][0] >= 0) {
            ESP_LOGI(TAG_TOUCH, "  Ecke %s: x=%d y=%d", kCornerNames[i], s_rec.corner_xy[i][0], s_rec.corner_xy[i][1]);
        } else {
            ESP_LOGI(TAG_TOUCH, "  Ecke %s: nicht getroffen", kCornerNames[i]);
        }
    }
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
