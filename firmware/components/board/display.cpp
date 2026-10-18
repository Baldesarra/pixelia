#include "board/display.h"

#include <cstring>

#include "board/pins.h"
#include "esp_attr.h"
#include "esp_cpu.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

namespace board::display {

namespace {

constexpr const char* TAG = "lcd";
constexpr int kW = kLcdWidth;
constexpr int kH = kLcdHeight;
constexpr int kSrcW = 400;
constexpr int kSrcH = 240;

// RGB565-Farben der Testbilder
constexpr uint16_t kBlack = 0x0000, kWhite = 0xFFFF, kRed = 0xF800, kGreen = 0x07E0, kBlue = 0x001F;
constexpr uint16_t kYellow = 0xFFE0, kCyan = 0x07FF, kSlate = 0x2945, kDark = 0x18E3;

esp_lcd_panel_handle_t s_panel = nullptr;
Timing s_timing;
SemaphoreHandle_t s_back_free = nullptr;
portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;

// Zustand, den der Interrupt liest (alles im internen RAM)
DRAM_ATTR volatile Mode s_mode = Mode::Bars;
DRAM_ATTR volatile int s_expected_fills = kH / 8;
DRAM_ATTR volatile int s_motion_x = 0;
DRAM_ATTR volatile int s_motion_y = 0;
DRAM_ATTR volatile int s_touch_x = -1;
DRAM_ATTR volatile int s_touch_y = -1;
DRAM_ATTR volatile uint8_t s_corners = 0;
DRAM_ATTR uint16_t* volatile s_grad = nullptr;  // 4 Verlaufszeilen für das Farbbalken-Testbild
DRAM_ATTR volatile bool s_rot180 = false;

// Engine-Doppelpuffer und Paletten (je 256 Einträge, Farbe doppelt für 2 Pixel)
DRAM_ATTR uint32_t s_lut[2][256];
DRAM_ATTR volatile int s_lut_front = 0;
DRAM_ATTR uint8_t* volatile s_front = nullptr;
DRAM_ATTR uint8_t* volatile s_back = nullptr;
DRAM_ATTR volatile bool s_pending = false;
DRAM_ATTR volatile int s_swap_interval = 1;
DRAM_ATTR volatile uint32_t s_frames_since_swap = 0;

// Statistik
DRAM_ATTR volatile uint32_t s_vsyncs = 0, s_frames = 0, s_swaps = 0, s_fills = 0;
DRAM_ATTR volatile uint32_t s_fill_cycles = 0, s_fill_max = 0, s_desyncs = 0, s_fills_since_vsync = 0;

IRAM_ATTR void fill16(uint16_t* d, uint16_t c, int n) {
    if (n <= 0) return;
    if (reinterpret_cast<uintptr_t>(d) & 2) {
        *d++ = c;
        --n;
    }
    uint32_t v = c | (static_cast<uint32_t>(c) << 16);
    auto* p = reinterpret_cast<uint32_t*>(d);
    int pairs = n >> 1;
    for (int i = 0; i < pairs; ++i) p[i] = v;
    if (n & 1) d[n - 1] = c;
}

// Testbild 1 (Prüfbild): weißer 1-Pixel-Rand ganz außen, 5 px schwarzer Abstand, oben Balken Rot-Grün-Blau-Weiß
// mit gelbem Quadrat oben links (Lage/Spiegelung), unten Verläufe Rot/Grün/Blau/Grau (vertauschte Datenleitungen
// zeigen sich als Sprünge oder falsche Farben).
constexpr int kInnerX0 = 6;              // gerade, damit 32-Bit-Kopien ausgerichtet sind
constexpr int kInnerW = kW - 2 * kInnerX0;  // 788
constexpr int kGradY0 = kH / 2;
constexpr int kGradH = 58;

IRAM_ATTR void line_bars(uint16_t* d, int y) {
    if (y == 0 || y == kH - 1) {
        fill16(d, kWhite, kW);
        return;
    }
    fill16(d, kBlack, kW);
    d[0] = kWhite;
    d[kW - 1] = kWhite;
    if (y < kInnerX0 || y >= kH - kInnerX0) return;
    if (y < kH / 2) {
        constexpr int bw = kInnerW / 4;  // 197
        fill16(d + kInnerX0, kRed, bw);
        fill16(d + kInnerX0 + bw, kGreen, bw);
        fill16(d + kInnerX0 + 2 * bw, kBlue, bw);
        fill16(d + kInnerX0 + 3 * bw, kWhite, kInnerW - 3 * bw);
        if (y >= 20 && y < 60) fill16(d + 20, kYellow, 40);
        return;
    }
    // Verläufe sind vorberechnet: pro Pixel rechnen war zu langsam für den Interrupt
    const uint16_t* grad = s_grad;
    int strip = (y - kGradY0) / kGradH;
    if (grad == nullptr || strip > 3) return;
    const auto* src = reinterpret_cast<const uint32_t*>(grad + strip * kInnerW);
    auto* dst = reinterpret_cast<uint32_t*>(d + kInnerX0);
    for (int i = 0; i < kInnerW / 2; ++i) dst[i] = src[i];
}

void build_gradients() {
    if (s_grad) return;
    auto* g = static_cast<uint16_t*>(heap_caps_malloc(4 * kInnerW * sizeof(uint16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_32BIT));
    if (!g) return;
    for (int x = 0; x < kInnerW; ++x) {
        auto v5 = static_cast<uint16_t>(x * 32 / kInnerW);  // 0..31
        auto v6 = static_cast<uint16_t>(x * 64 / kInnerW);  // 0..63
        g[0 * kInnerW + x] = v5 << 11;
        g[1 * kInnerW + x] = v6 << 5;
        g[2 * kInnerW + x] = v5;
        g[3 * kInnerW + x] = (v5 << 11) | (v6 << 5) | v5;
    }
    s_grad = g;
}

// Testbild 2: weißer 1-Pixel-Rahmen am Rand, roter Rahmen 10 px innen, grünes Kreuz, gelbes Quadrat oben links
IRAM_ATTR void line_frame(uint16_t* d, int y) {
    if (y == 0 || y == kH - 1) {
        fill16(d, kWhite, kW);
        return;
    }
    fill16(d, kBlack, kW);
    if (y == 10 || y == kH - 11) {
        fill16(d + 10, kRed, kW - 20);
    } else if (y > 10 && y < kH - 11) {
        d[10] = kRed;
        d[kW - 11] = kRed;
    }
    if (y == kH / 2) fill16(d + 1, kGreen, kW - 2);
    d[kW / 2] = kGreen;
    if (y >= 30 && y < 70) fill16(d + 30, kYellow, 40);
    d[0] = kWhite;
    d[kW - 1] = kWhite;
}

// Testbild 3: senkrechter weißer und waagerechter cyanfarbener Balken wandern
IRAM_ATTR void line_motion(uint16_t* d, int y) {
    int by = s_motion_y;
    if (y >= by && y < by + 16) {
        fill16(d, kCyan, kW);
    } else {
        fill16(d, kSlate, kW);
    }
    int bx = s_motion_x;
    int w = kW - bx < 16 ? kW - bx : 16;
    fill16(d + bx, kWhite, w);
}

// Testbild 4: Touch-Test mit vier Eckfeldern (rot = offen, grün = getroffen) und Fadenkreuz
IRAM_ATTR void line_touch(uint16_t* d, int y) {
    fill16(d, kDark, kW);
    uint8_t hits = s_corners;
    if (y < 100) {
        fill16(d, (hits & 1) ? kGreen : kRed, 100);
        fill16(d + kW - 100, (hits & 2) ? kGreen : kRed, 100);
    } else if (y >= kH - 100) {
        fill16(d, (hits & 4) ? kGreen : kRed, 100);
        fill16(d + kW - 100, (hits & 8) ? kGreen : kRed, 100);
    }
    int tx = s_touch_x, ty = s_touch_y;
    if (tx >= 0 && tx < kW) {
        if (y == ty) {
            fill16(d, kWhite, kW);
        } else {
            d[tx] = kWhite;
        }
    }
}

// Engine: Palette nachschlagen und 2× hochskalieren (jede Quellzeile ergibt zwei Bildzeilen).
// Bei 180°-Drehung kommen die Quellzeilen von unten. Gelesen wird immer aufsteigend (PSRAM-Cache mag das),
// geschrieben wird von rechts nach links in den internen Bounce-Buffer.
IRAM_ATTR void fill_engine(uint16_t* dst, int y0, int lines, bool rot) {
    const uint8_t* fb = s_front;
    if (fb == nullptr) {
        fill16(dst, kBlack, lines * kW);
        return;
    }
    const uint32_t* lut = s_lut[s_lut_front];
    for (int y = y0; y < y0 + lines; y += 2) {
        int sy = rot ? kSrcH - 1 - (y >> 1) : (y >> 1);
        const auto* src = reinterpret_cast<const uint32_t*>(fb + sy * kSrcW);
        auto* a = reinterpret_cast<uint32_t*>(dst);
        uint32_t* b = a + kW / 2;
        if (rot) {
            uint32_t* ea = a + kSrcW - 4;  // letzte 4 Ausgabeworte der Zeile
            uint32_t* eb = b + kSrcW - 4;
            for (int i = 0; i < kSrcW / 4; ++i) {
                uint32_t s = src[i];
                uint32_t p0 = lut[s & 0xff];
                uint32_t p1 = lut[(s >> 8) & 0xff];
                uint32_t p2 = lut[(s >> 16) & 0xff];
                uint32_t p3 = lut[s >> 24];
                ea[3] = p0;
                ea[2] = p1;
                ea[1] = p2;
                ea[0] = p3;
                eb[3] = p0;
                eb[2] = p1;
                eb[1] = p2;
                eb[0] = p3;
                ea -= 4;
                eb -= 4;
            }
        } else {
            for (int i = 0; i < kSrcW / 4; ++i) {
                uint32_t s = src[i];
                uint32_t p0 = lut[s & 0xff];
                uint32_t p1 = lut[(s >> 8) & 0xff];
                uint32_t p2 = lut[(s >> 16) & 0xff];
                uint32_t p3 = lut[s >> 24];
                a[0] = p0;
                a[1] = p1;
                a[2] = p2;
                a[3] = p3;
                b[0] = p0;
                b[1] = p1;
                b[2] = p2;
                b[3] = p3;
                a += 4;
                b += 4;
            }
        }
        dst += 2 * kW;
    }
}

// Zeile spiegeln (für die 180°-Drehung der Testbilder)
IRAM_ATTR void reverse_line(uint16_t* d) {
    auto* p = reinterpret_cast<uint32_t*>(d);
    for (int i = 0, j = kW / 2 - 1; i < j; ++i, --j) {
        uint32_t a = p[i], b = p[j];
        p[i] = (b >> 16) | (b << 16);
        p[j] = (a >> 16) | (a << 16);
    }
}

IRAM_ATTR bool on_bounce_empty(esp_lcd_panel_handle_t, void* bounce_buf, int pos_px, int len_bytes, void*) {
    uint32_t t0 = esp_cpu_get_cycle_count();
    auto* dst = static_cast<uint16_t*>(bounce_buf);
    int y0 = pos_px / kW;
    int lines = len_bytes / (kW * 2);
    Mode m = s_mode;
    bool rot = s_rot180;
    if (m == Mode::Engine) {
        fill_engine(dst, y0, lines, rot);
    } else {
        for (int y = y0; y < y0 + lines; ++y, dst += kW) {
            int sy = rot ? kH - 1 - y : y;
            if (m == Mode::Bars) {
                line_bars(dst, sy);
            } else if (m == Mode::Frame) {
                line_frame(dst, sy);
            } else if (m == Mode::Motion) {
                line_motion(dst, sy);
            } else if (m == Mode::Touch) {
                line_touch(dst, sy);
            } else {
                fill16(dst, kBlack, kW);
            }
            if (rot) reverse_line(dst);
        }
    }
    s_fills = s_fills + 1;
    s_fills_since_vsync = s_fills_since_vsync + 1;
    uint32_t dt = esp_cpu_get_cycle_count() - t0;
    s_fill_cycles += dt;
    if (dt > s_fill_max) s_fill_max = dt;
    return false;
}

// Bildende (letzter Bounce-Buffer des Bildes befüllt): hier wird der Engine-Puffer getauscht
IRAM_ATTR bool on_frame_complete(esp_lcd_panel_handle_t, const esp_lcd_rgb_panel_event_data_t*, void*) {
    s_frames = s_frames + 1;
    int mx = s_motion_x + 8;
    s_motion_x = mx >= kW ? mx - kW : mx;
    int my = s_motion_y + 4;
    s_motion_y = my >= kH ? my - kH : my;

    BaseType_t woken = pdFALSE;
    portENTER_CRITICAL_ISR(&s_lock);
    uint32_t since = s_frames_since_swap + 1;
    if (s_pending && since >= static_cast<uint32_t>(s_swap_interval)) {
        uint8_t* t = s_front;
        s_front = s_back;
        s_back = t;
        s_lut_front ^= 1;
        s_pending = false;
        since = 0;
        s_swaps = s_swaps + 1;
        portEXIT_CRITICAL_ISR(&s_lock);
        xSemaphoreGiveFromISR(s_back_free, &woken);
    } else {
        portEXIT_CRITICAL_ISR(&s_lock);
    }
    s_frames_since_swap = since;
    return woken == pdTRUE;
}

IRAM_ATTR bool on_vsync(esp_lcd_panel_handle_t, const esp_lcd_rgb_panel_event_data_t*, void*) {
    s_vsyncs = s_vsyncs + 1;
    uint32_t n = s_fills_since_vsync;
    s_fills_since_vsync = 0;
    if (n != static_cast<uint32_t>(s_expected_fills) && s_vsyncs > 4) s_desyncs = s_desyncs + 1;
    return false;
}

esp_err_t create_panel(const Timing& t) {
    if (t.bounce_lines <= 0 || (t.bounce_lines & 1) || kH % t.bounce_lines) {
        ESP_LOGE(TAG, "Bounce-Zeilen %d ungültig (gerade, Teiler von %d)", t.bounce_lines, kH);
        return ESP_ERR_INVALID_ARG;
    }
    esp_lcd_rgb_panel_config_t cfg = {};
    cfg.clk_src = t.pll240 ? LCD_CLK_SRC_PLL240M : LCD_CLK_SRC_PLL160M;
    cfg.timings.pclk_hz = t.pclk_hz;
    cfg.timings.h_res = kW;
    cfg.timings.v_res = kH;
    cfg.timings.hsync_pulse_width = t.hsync_pulse;
    cfg.timings.hsync_back_porch = t.hsync_back;
    cfg.timings.hsync_front_porch = t.hsync_front;
    cfg.timings.vsync_pulse_width = t.vsync_pulse;
    cfg.timings.vsync_back_porch = t.vsync_back;
    cfg.timings.vsync_front_porch = t.vsync_front;
    cfg.timings.flags.hsync_idle_low = t.hsync_idle_low;
    cfg.timings.flags.vsync_idle_low = t.vsync_idle_low;
    cfg.timings.flags.pclk_active_neg = t.pclk_active_neg;
    cfg.data_width = 16;
    cfg.in_color_format = LCD_COLOR_FMT_RGB565;
    cfg.bounce_buffer_size_px = static_cast<size_t>(kW) * t.bounce_lines;
    cfg.hsync_gpio_num = kLcdHsync;
    cfg.vsync_gpio_num = kLcdVsync;
    cfg.de_gpio_num = kLcdDe;
    cfg.pclk_gpio_num = kLcdPclk;
    cfg.disp_gpio_num = GPIO_NUM_NC;
    for (int i = 0; i < ESP_LCD_RGB_BUS_WIDTH_MAX; ++i) cfg.data_gpio_nums[i] = i < 16 ? kLcdData[i] : GPIO_NUM_NC;
    cfg.flags.no_fb = 1;

    s_expected_fills = kH / t.bounce_lines;
    s_desyncs = 0;
    esp_err_t err = esp_lcd_new_rgb_panel(&cfg, &s_panel);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Panel anlegen fehlgeschlagen: %s", esp_err_to_name(err));
        s_panel = nullptr;
        return err;
    }
    esp_lcd_rgb_panel_event_callbacks_t cbs = {};
    cbs.on_vsync = on_vsync;
    cbs.on_bounce_empty = on_bounce_empty;
    cbs.on_frame_buf_complete = on_frame_complete;
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(s_panel, &cbs, nullptr));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
    s_timing = t;
    ESP_LOGI(TAG, "Panel %dx%d, Pixeltakt %.2f MHz (%s), H %u/%u/%u, V %u/%u/%u, Bounce %d Zeilen, Bild %.2f Hz (Kern %d)",
             kW, kH, t.pclk_hz / 1e6, t.pll240 ? "PLL240" : "PLL160", t.hsync_front, t.hsync_pulse, t.hsync_back,
             t.vsync_front, t.vsync_pulse, t.vsync_back, t.bounce_lines, 1e6 / frame_period_us(), xPortGetCoreID());
    return ESP_OK;
}

}  // namespace

esp_err_t init(const Timing& timing) {
    if (s_back_free == nullptr) s_back_free = xSemaphoreCreateBinary();
    build_gradients();
    return create_panel(timing);
}

esp_err_t reconfigure(const Timing& timing) {
    if (s_panel) {
        ESP_ERROR_CHECK(esp_lcd_panel_del(s_panel));
        s_panel = nullptr;
    }
    return create_panel(timing);
}

const Timing& timing() { return s_timing; }

esp_err_t set_pclk(uint32_t hz) {
    if (!s_panel) return ESP_ERR_INVALID_STATE;
    esp_err_t err = esp_lcd_rgb_panel_set_pclk(s_panel, hz);
    if (err == ESP_OK) s_timing.pclk_hz = hz;
    return err;
}

uint32_t frame_period_us() {
    const Timing& t = s_timing;
    uint64_t htotal = kW + t.hsync_pulse + t.hsync_back + t.hsync_front;
    uint64_t vtotal = kH + t.vsync_pulse + t.vsync_back + t.vsync_front;
    return t.pclk_hz ? static_cast<uint32_t>(htotal * vtotal * 1000000ULL / t.pclk_hz) : 0;
}

int fills_per_frame() { return s_expected_fills; }

void set_mode(Mode mode) { s_mode = mode; }
Mode mode() { return s_mode; }

namespace {
constexpr const char* kModeNames[] = {"black", "bars", "frame", "motion", "touch", "engine"};
}

const char* mode_name(Mode mode) { return kModeNames[static_cast<int>(mode)]; }

bool mode_from_name(const char* name, Mode* out) {
    for (int i = 0; i < 6; ++i) {
        if (std::strcmp(name, kModeNames[i]) == 0) {
            *out = static_cast<Mode>(i);
            return true;
        }
    }
    return false;
}

Stats stats(bool reset_max) {
    Stats s;
    s.vsyncs = s_vsyncs;
    s.frames = s_frames;
    s.swaps = s_swaps;
    s.fills = s_fills;
    s.fill_cycles = s_fill_cycles;
    s.fill_max_cycles = s_fill_max;
    s.desyncs = s_desyncs;
    if (reset_max) s_fill_max = 0;
    return s;
}

void set_rotation180(bool on) { s_rot180 = on; }
bool rotation180() { return s_rot180; }

void set_touch_marker(int x, int y) {
    s_touch_y = y;
    s_touch_x = x;
}

void set_corner_hits(uint8_t mask) { s_corners = mask; }

void attach_buffers(uint8_t* a, uint8_t* b) {
    portENTER_CRITICAL(&s_lock);
    s_front = a;
    s_back = b;
    s_pending = false;
    s_frames_since_swap = 0;
    portEXIT_CRITICAL(&s_lock);
    // Hinterpuffer ist sofort frei
    xSemaphoreTake(s_back_free, 0);
    xSemaphoreGive(s_back_free);
}

void detach_buffers() {
    portENTER_CRITICAL(&s_lock);
    s_front = nullptr;
    s_back = nullptr;
    s_pending = false;
    portEXIT_CRITICAL(&s_lock);
    xSemaphoreTake(s_back_free, 0);
}

uint8_t* acquire_back_buffer(uint32_t timeout_ms) {
    if (xSemaphoreTake(s_back_free, pdMS_TO_TICKS(timeout_ms)) != pdTRUE) return nullptr;
    return s_back;
}

void present(const uint16_t* palette565) {
    // Die hintere Palette liest der Interrupt erst nach dem Tausch
    uint32_t* lut = s_lut[s_lut_front ^ 1];
    for (int i = 0; i < 256; ++i) lut[i] = palette565[i] | (static_cast<uint32_t>(palette565[i]) << 16);
    portENTER_CRITICAL(&s_lock);
    s_pending = true;
    portEXIT_CRITICAL(&s_lock);
}

void set_swap_interval(int frames) { s_swap_interval = frames < 1 ? 1 : frames; }
int swap_interval() { return s_swap_interval; }

}  // namespace board::display
