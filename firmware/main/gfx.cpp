// Grafik-Task auf Kern 1: legt das Panel an (Interrupt landet so auf Kern 1) und lässt im Engine-Modus die Szene
// laufen (gleicher Szenen-Code wie im Simulator, siehe engine/ und sim/).

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>

#include "app.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pixelia/engine/color.h"
#include "pixelia/engine/scene.h"
#include "pixelia/engine/system_ui.h"
#include "net.h"
#include "ui.h"

namespace app::gfx {

namespace {
constexpr const char* TAG = "gfx";
constexpr size_t kBufBytes = pixelia::kScreenPixels;
constexpr uint64_t kSeed = 0x5049584C4941ULL;  // „PIXLIA“

enum class Request : uint8_t { None, Reconfigure, Buffers };

volatile Request s_request = Request::None;
board::display::Timing s_req_timing;
// Standard: Engine-Puffer im PSRAM (spart 192 KB internen RAM, Messung siehe docs/HARDWARE.md)
volatile bool s_req_psram = true;
volatile int s_sprites = 48;
volatile int s_marks = 0;
volatile int s_req_minute_of_day = -1;  // Uhrzeit stellen (vom Konsolen-Task, angewendet im Grafik-Task)
volatile float s_speed = 1.0f;

uint8_t* s_buf[2] = {nullptr, nullptr};
bool s_buf_psram = false;
pixelia::Scene* s_scene = nullptr;
pixelia::WorldClock s_clock;
uint16_t s_pal565[256];

portMUX_TYPE s_stats_lock = portMUX_INITIALIZER_UNLOCKED;
EngineStats s_stats{};

// Weltzeit folgt der echten Uhrzeit (NTP), bis jemand per Befehl Uhrzeit oder Zeitraffer verstellt
volatile bool s_follow_real = true;
int64_t s_next_real_sync = 0;

// Systemanzeigen: von allen Tasks geschrieben (s_ui), im Grafik-Task als Kopie benutzt (s_ui_local)
struct UiState {
    uint32_t version = 0;
    bool setup = false;
    pixelia::ui::SetupInfo setup_info;
    bool update = false;
    pixelia::ui::UpdateInfo update_info;
    int64_t update_until = 0;  // 0 = bis auf Weiteres
    bool message = false;
    pixelia::ui::MessageInfo message_info;
    int64_t message_until = 0;
    std::string toast;
    pixelia::ui::Tone toast_tone = pixelia::ui::Tone::Info;
    int64_t toast_until = 0;
    bool info = false;
    pixelia::ui::InfoCardData info_data;
    int64_t info_until = 0;
};
std::mutex s_ui_lock;
UiState s_ui;
UiState s_ui_local;
pixelia::ui::SetupScreen s_setup_screen;
pixelia::ui::InfoCard s_info_card;
uint16_t s_ui_pal565[16];
// Halte-Anzeige in Promille, -1 = aus. Eigener Wert statt UiState: ändert sich bei jedem Touch-Abfrage-Takt
std::atomic<int> s_hold_permille{-1};

// Echte Uhrzeit in die Weltzeit übernehmen: große Abweichung (erster Abgleich, Sommerzeit) springt,
// kleine Gangunterschiede werden sanft nachgeführt, damit Bewegungen gleichmäßig bleiben
void follow_real_time(int64_t now) {
    if (!s_follow_real || now < s_next_real_sync) return;
    s_next_real_sync = now + 1000000;
    int64_t local;
    if (!timesync::local_us(&local)) return;
    int64_t diff = local - s_clock.world_us();
    if (std::llabs(diff) > 2000000) {
        bool first = s_clock.world_us() < 365LL * pixelia::WorldClock::kUsPerDay;
        s_clock.set_world_us(local);
        ESP_LOGI(TAG, "Weltzeit %s echte Uhrzeit: %02d:%02d", first ? "übernimmt die" : "springt auf die",
                 s_clock.hour(), s_clock.minute());
    } else {
        s_clock.set_world_us(s_clock.world_us() + diff / 8);
    }
}

// Ganzseitige Systemanzeige, die gerade gilt: 0 = keine (Welt läuft), 1 = Update, 2 = Meldung, 3 = Setup
int active_screen(int64_t now) {
    const UiState& u = s_ui_local;
    if (u.update && (u.update_until == 0 || now < u.update_until)) return 1;
    if (u.message && now < u.message_until) return 2;
    if (u.setup) return 3;
    return 0;
}

void draw_screen(pixelia::Canvas& canvas, int screen) {
    const UiState& u = s_ui_local;
    if (screen == 1) pixelia::ui::render_update(canvas, u.update_info);
    else if (screen == 2) pixelia::ui::render_message(canvas, u.message_info);
    else s_setup_screen.render(canvas);
}

// Was zuletzt in welchem Puffer gezeichnet wurde (0 = Welt). Systemanzeigen sind unbewegt: Steht im Hinterpuffer
// schon dieselbe Anzeige, muss nichts neu gezeichnet werden (spart Rechenzeit und PSRAM-Bandbreite).
uint64_t s_buf_key[2] = {0, 0};

// Info-Karte und Halte-Anzeige über das fertige Bild legen (nicht über das Update)
void draw_overlays(pixelia::Canvas& canvas, int screen, bool info, int hold) {
    if (screen == 1) return;
    if (info) s_info_card.render(canvas);
    if (hold >= 0) pixelia::ui::draw_hold_indicator(canvas, hold / 1000.0f);
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
    s_buf_key[0] = s_buf_key[1] = 0;
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
        s_scene = pixelia::make_scene("scrolltest").release();
        s_scene->init(kSeed);
        ESP_LOGI(TAG, "Szene %s, Weltzeit Tag %d %02d:%02d", s_scene->name(), s_clock.day(), s_clock.hour(),
                 s_clock.minute());
    }
    board::display::attach_buffers(s_buf[0], s_buf[1]);
    return true;
}

void gfx_task(void* arg) {
    auto* timing = static_cast<board::display::Timing*>(arg);
    ESP_ERROR_CHECK(board::display::init(*timing));
    delete timing;

    pixelia::Palette ui_pal{};
    pixelia::ui::apply_palette(ui_pal);
    for (int i = 0; i < 16; ++i) s_ui_pal565[i] = pixelia::to_rgb565(ui_pal[pixelia::ui::kFirstIndex + i]);

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
        int minute = s_req_minute_of_day;
        if (minute >= 0) {
            s_clock.set_time_of_day(minute / 60, minute % 60);
            s_req_minute_of_day = -1;
        }
        s_clock.set_speed(s_speed);

        if (board::display::mode() != board::display::Mode::Engine) {
            vTaskDelay(pdMS_TO_TICKS(20));
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

        // Logik-Schritte aus der Zahl der angezeigten Panel-Bilder: im Normalfall genau ein Schritt pro
        // Engine-Bild, nach einem verpassten Bild zwei. Kein Verhaken mit einer zweiten Uhr möglich.
        int64_t t0 = esp_timer_get_time();
        int swap = board::display::swap_interval();
        uint32_t frames = board::display::frame_count();
        uint32_t elapsed = frames - last_frames + frame_rem;
        last_frames = frames;
        int ticks = static_cast<int>(elapsed / swap);
        frame_rem = elapsed % swap;
        if (ticks > 4) ticks = 4;  // Überlast: nicht endlos nachholen
        const int64_t step_us = static_cast<int64_t>(board::display::frame_period_us()) * swap;
        s_scene->set_load(s_sprites);
        follow_real_time(t0);
        for (int i = 0; i < ticks; ++i) {
            s_clock.advance(step_us);
            s_scene->tick(s_clock);
        }

        // Systemanzeigen übernehmen (Kopie, damit andere Tasks nie auf den Grafik-Task warten müssen)
        bool ui_changed = false;
        {
            std::lock_guard<std::mutex> g(s_ui_lock);
            if (s_ui.version != s_ui_local.version) {
                s_ui_local = s_ui;
                ui_changed = true;
            }
        }
        if (ui_changed && s_ui_local.setup) s_setup_screen.set(s_ui_local.setup_info);
        if (ui_changed && s_ui_local.info) s_info_card.set(s_ui_local.info_data);

        int64_t t1 = esp_timer_get_time();
        pixelia::Canvas canvas(back, pixelia::kScreenWidth, pixelia::kScreenHeight);
        const int screen = active_screen(t1);
        const int bi = back == s_buf[0] ? 0 : 1;
        const bool info = s_ui_local.info && t1 < s_ui_local.info_until;
        const int hold = s_hold_permille.load(std::memory_order_relaxed);
        if (screen == 0) {
            s_scene->render(canvas);
            const UiState& u = s_ui_local;
            if (!u.toast.empty() && t1 < u.toast_until) pixelia::ui::draw_toast(canvas, u.toast, u.toast_tone);
            draw_overlays(canvas, screen, info, hold);
            s_buf_key[bi] = 0;
        } else {
            // Halte-Anzeige in 1-%-Schritten: so oft muss eine unbewegte Anzeige höchstens neu gezeichnet werden
            const uint64_t key = static_cast<uint64_t>(screen) << 40 |
                                 static_cast<uint64_t>(hold < 0 ? 0 : hold / 10 + 1) << 32 |
                                 static_cast<uint64_t>(info) << 31 | static_cast<uint64_t>(s_marks & 0x7F) << 24 |
                                 (s_ui_local.version & 0x00FFFFFFu);
            if (s_buf_key[bi] != key) {
                draw_screen(canvas, screen);
                draw_overlays(canvas, screen, info, hold);
                s_buf_key[bi] = key;
            }
        }
        for (int i = 0; i < s_marks; ++i) canvas.fill_rect(4 + i * 10, 4, 6, 6, pixelia::ui::kWhite);
        // Indizes 240–255 gehören der System-Oberfläche (engine/system_ui.h)
        const pixelia::Palette& pal = s_scene->palette();
        for (int i = 0; i < pixelia::ui::kFirstIndex; ++i) s_pal565[i] = pixelia::to_rgb565(pal[i]);
        std::memcpy(&s_pal565[pixelia::ui::kFirstIndex], s_ui_pal565, sizeof(s_ui_pal565));
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
        s_stats.world_minute = s_clock.hour() * 60 + s_clock.minute();
        s_stats.world_day = s_clock.day();
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
void set_time_of_day(int hour, int minute) {
    s_follow_real = false;
    s_req_minute_of_day = (hour % 24) * 60 + minute % 60;
}
void set_speed(float factor) {
    s_speed = factor > 0 ? factor : 1.0f;
    if (s_speed != 1.0f) s_follow_real = false;
}
float speed() { return s_speed; }
void set_real_time(bool on) {
    if (on) {
        s_speed = 1.0f;
        s_next_real_sync = 0;
    }
    s_follow_real = on;
}
bool real_time() { return s_follow_real; }

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

namespace app::ui {

namespace {
int64_t until(int seconds) { return esp_timer_get_time() + static_cast<int64_t>(seconds) * 1000000; }
}  // namespace

void show_setup(const pixelia::ui::SetupInfo& info) {
    std::lock_guard<std::mutex> g(gfx::s_ui_lock);
    if (gfx::s_ui.setup && gfx::s_ui.setup_info == info) return;
    gfx::s_ui.setup = true;
    gfx::s_ui.setup_info = info;
    ++gfx::s_ui.version;
}

void hide_setup() {
    std::lock_guard<std::mutex> g(gfx::s_ui_lock);
    if (!gfx::s_ui.setup) return;
    gfx::s_ui.setup = false;
    ++gfx::s_ui.version;
}

void show_update(const pixelia::ui::UpdateInfo& info) {
    std::lock_guard<std::mutex> g(gfx::s_ui_lock);
    gfx::s_ui.update = true;
    gfx::s_ui.update_info = info;
    // Fehlermeldung nach 12 s ausblenden, die alte Version läuft ja weiter
    gfx::s_ui.update_until = info.phase == pixelia::ui::UpdateInfo::Phase::Failed ? until(12) : 0;
    ++gfx::s_ui.version;
}

void hide_update() {
    std::lock_guard<std::mutex> g(gfx::s_ui_lock);
    gfx::s_ui.update = false;
    ++gfx::s_ui.version;
}

void show_message(const pixelia::ui::MessageInfo& info, int seconds) {
    std::lock_guard<std::mutex> g(gfx::s_ui_lock);
    gfx::s_ui.message = true;
    gfx::s_ui.message_info = info;
    gfx::s_ui.message_until = until(seconds);
    ++gfx::s_ui.version;
}

void hide_message() {
    std::lock_guard<std::mutex> g(gfx::s_ui_lock);
    gfx::s_ui.message = false;
    ++gfx::s_ui.version;
}

void toast(std::string_view text, pixelia::ui::Tone tone, int seconds) {
    std::lock_guard<std::mutex> g(gfx::s_ui_lock);
    gfx::s_ui.toast = text;
    gfx::s_ui.toast_tone = tone;
    gfx::s_ui.toast_until = until(seconds);
    ++gfx::s_ui.version;
}

void show_info(const pixelia::ui::InfoCardData& data, int seconds) {
    std::lock_guard<std::mutex> g(gfx::s_ui_lock);
    gfx::s_ui.info = true;
    gfx::s_ui.info_data = data;
    gfx::s_ui.info_until = until(seconds);
    ++gfx::s_ui.version;
}

void refresh_info(const pixelia::ui::InfoCardData& data) {
    std::lock_guard<std::mutex> g(gfx::s_ui_lock);
    if (!gfx::s_ui.info || gfx::s_ui.info_data == data) return;
    gfx::s_ui.info_data = data;
    ++gfx::s_ui.version;
}

void hide_info() {
    std::lock_guard<std::mutex> g(gfx::s_ui_lock);
    if (!gfx::s_ui.info) return;
    gfx::s_ui.info = false;
    ++gfx::s_ui.version;
}

bool info_visible() {
    std::lock_guard<std::mutex> g(gfx::s_ui_lock);
    return gfx::s_ui.info && esp_timer_get_time() < gfx::s_ui.info_until;
}

bool update_visible() {
    std::lock_guard<std::mutex> g(gfx::s_ui_lock);
    const auto& u = gfx::s_ui;
    return u.update && (u.update_until == 0 || esp_timer_get_time() < u.update_until);
}

void set_hold_progress(float progress) {
    gfx::s_hold_permille.store(progress < 0 ? -1 : static_cast<int>(progress * 1000.0f), std::memory_order_relaxed);
}

}  // namespace app::ui
