// Grafik-Task auf Kern 1: legt das Panel an (Interrupt landet so auf Kern 1) und rendert im Engine-Modus
// den Vollbild-Scrolling-Test in 400×240.

#include <cstring>

#include "app.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pixelia/engine/scroll_test.h"

namespace app::gfx {

namespace {
constexpr const char* TAG = "gfx";
constexpr size_t kBufBytes = pixelia::kScreenPixels;

enum class Request : uint8_t { None, Reconfigure, Buffers };

volatile Request s_request = Request::None;
board::display::Timing s_req_timing;
// Standard: Engine-Puffer im PSRAM (spart 192 KB internen RAM, Messung siehe docs/HARDWARE.md)
volatile bool s_req_psram = true;
volatile int s_sprites = 48;
volatile int s_marks = 0;
volatile bool s_tick_by_vsync = true;

uint8_t* s_buf[2] = {nullptr, nullptr};
bool s_buf_psram = false;
pixelia::ScrollTest* s_scene = nullptr;
uint16_t s_pal565[256];

portMUX_TYPE s_stats_lock = portMUX_INITIALIZER_UNLOCKED;
EngineStats s_stats{};

uint16_t to565(pixelia::Color c) {
    return static_cast<uint16_t>(((c.r & 0xF8) << 8) | ((c.g & 0xFC) << 3) | (c.b >> 3));
}

void free_buffers() {
    board::display::detach_buffers();
    // Zwei Bilder warten, damit der Interrupt die alten Puffer sicher nicht mehr liest
    vTaskDelay(pdMS_TO_TICKS(2 * board::display::frame_period_us() / 1000 + 5));
    for (auto& b : s_buf) {
        heap_caps_free(b);
        b = nullptr;
    }
}

bool alloc_buffers(bool psram) {
    uint32_t caps = psram ? (MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) : (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    for (auto& b : s_buf) b = static_cast<uint8_t*>(heap_caps_aligned_alloc(4, kBufBytes, caps));
    if (!s_buf[0] || !s_buf[1]) {
        for (auto& b : s_buf) {
            heap_caps_free(b);
            b = nullptr;
        }
        return false;
    }
    s_buf_psram = psram;
    std::memset(s_buf[0], 0, kBufBytes);
    std::memset(s_buf[1], 0, kBufBytes);
    return true;
}

bool ensure_engine_ready(bool psram) {
    if (s_buf[0] && s_buf_psram == psram) return true;
    if (s_buf[0]) free_buffers();
    if (!alloc_buffers(psram)) {
        ESP_LOGW(TAG, "2× %u Byte im %s nicht verfügbar", static_cast<unsigned>(kBufBytes), psram ? "PSRAM" : "internen RAM");
        if (psram || !alloc_buffers(true)) {
            ESP_LOGE(TAG, "Keine Engine-Puffer");
            return false;
        }
    }
    ESP_LOGI(TAG, "Engine-Puffer: 2× %u Byte im %s", static_cast<unsigned>(kBufBytes),
             s_buf_psram ? "PSRAM" : "internen RAM");
    app::sysinfo::log_heap();
    if (!s_scene) {
        s_scene = new pixelia::ScrollTest();
        s_scene->init(0x5049584C4941ULL);  // „PIXLIA“
    }
    board::display::attach_buffers(s_buf[0], s_buf[1]);
    return true;
}

void gfx_task(void* arg) {
    auto* timing = static_cast<board::display::Timing*>(arg);
    ESP_ERROR_CHECK(board::display::init(*timing));
    delete timing;

    int64_t last = esp_timer_get_time();
    int64_t acc = 0;
    uint32_t last_frames = board::display::frame_count();
    uint32_t frame_rem = 0;
    for (;;) {
        Request req = s_request;
        if (req == Request::Reconfigure) {
            bool had = s_buf[0] != nullptr;
            if (had) free_buffers();
            board::display::reconfigure(s_req_timing);
            s_request = Request::None;
            if (had && board::display::mode() == board::display::Mode::Engine) ensure_engine_ready(s_buf_psram);
            continue;
        }
        if (req == Request::Buffers) {
            if (s_buf[0]) free_buffers();
            s_request = Request::None;
            if (board::display::mode() == board::display::Mode::Engine) ensure_engine_ready(s_req_psram);
            continue;
        }

        if (board::display::mode() != board::display::Mode::Engine) {
            vTaskDelay(pdMS_TO_TICKS(20));
            last = esp_timer_get_time();
            acc = 0;
            last_frames = board::display::frame_count();
            frame_rem = 0;
            continue;
        }
        if (!s_buf[0] && !ensure_engine_ready(s_req_psram)) {
            board::display::set_mode(board::display::Mode::Black);
            continue;
        }

        uint8_t* back = board::display::acquire_back_buffer(200);
        if (!back) continue;

        int64_t t0 = esp_timer_get_time();
        int swap = board::display::swap_interval();
        int want = 0;
        if (s_tick_by_vsync) {
            // Logik-Schritte aus der Zahl der angezeigten Panel-Bilder: im Normalfall genau ein Schritt pro
            // Engine-Bild, nach einem verpassten Bild zwei. Kein Verhaken mit einer zweiten Uhr möglich.
            uint32_t frames = board::display::frame_count();
            uint32_t elapsed = frames - last_frames + frame_rem;
            last_frames = frames;
            want = static_cast<int>(elapsed / swap);
            frame_rem = elapsed % swap;
        } else {
            // Alte Variante zum Vergleich: Echtzeit-Akkumulator mit Schrittweite = Bilddauer × Tauschintervall
            int64_t step = static_cast<int64_t>(board::display::frame_period_us()) * swap;
            acc += t0 - last;
            want = static_cast<int>(acc / step);
            acc -= want * step;
        }
        last = t0;
        if (want > 4) want = 4;  // Überlast: nicht endlos nachholen
        s_scene->set_sprite_count(s_sprites);
        for (int i = 0; i < want; ++i) s_scene->tick();
        int ticks = want;

        int64_t t1 = esp_timer_get_time();
        pixelia::Canvas canvas(back, pixelia::kScreenWidth, pixelia::kScreenHeight);
        s_scene->render(canvas);
        for (int i = 0; i < s_marks; ++i) canvas.fill_rect(4 + i * 10, 4, 6, 6, 255);
        const pixelia::Palette& pal = s_scene->palette();
        for (int i = 0; i < 256; ++i) s_pal565[i] = to565(pal[i]);
        int64_t t2 = esp_timer_get_time();
        board::display::present(s_pal565);

        uint32_t render_us = static_cast<uint32_t>(t2 - t1);
        portENTER_CRITICAL(&s_stats_lock);
        s_stats.frames++;
        s_stats.render_us_sum += render_us;
        if (render_us > s_stats.render_us_max) s_stats.render_us_max = render_us;
        s_stats.logic_us_sum += static_cast<uint64_t>(t1 - t0);
        s_stats.logic_ticks += ticks;
        s_stats.tick_hist[ticks < 2 ? ticks : 2]++;
        portEXIT_CRITICAL(&s_stats_lock);
    }
}
}  // namespace

void start(const board::display::Timing& timing) {
    auto* arg = new board::display::Timing(timing);
    xTaskCreatePinnedToCore(gfx_task, "gfx", 6144, arg, 10, nullptr, kGfxCore);
}

void request_reconfigure(const board::display::Timing& timing) {
    s_req_timing = timing;
    s_request = Request::Reconfigure;
}

void request_buffers(bool psram) {
    s_req_psram = psram;
    s_request = Request::Buffers;
}

void set_sprites(int n) { s_sprites = n; }
void set_marks(int n) { s_marks = n; }
void set_tick_by_vsync(bool on) { s_tick_by_vsync = on; }

EngineStats engine_stats(bool reset_max) {
    portENTER_CRITICAL(&s_stats_lock);
    EngineStats s = s_stats;
    if (reset_max) s_stats.render_us_max = 0;
    portEXIT_CRITICAL(&s_stats_lock);
    s.buffers_in_psram = s_buf_psram;
    s.buffers_ready = s_buf[0] != nullptr;
    s.sprites = s_sprites;
    return s;
}

}  // namespace app::gfx
