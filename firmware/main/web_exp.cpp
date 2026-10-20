// Seite Experimentell und Live-Vorschau (seit 0.4.2):
//   GET  /api/exp     Schalter, Panel-Werte (aktiv, gespeichert, Standard), Probe, Testbild, Touch-Test
//   POST /api/exp     a=on|off|panel|confirm|revert|reset|test (mit PIN; revert ohne, macht nur sicherer)
//   GET  /api/screen  aktuelles Bild 400×240 als Paletten-Indizes, PackBits-gepackt (Aufbau siehe screen_get)

#include <cstdlib>
#include <cstring>

#include "app.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "exper.h"
#include "pixelia/engine/canvas.h"
#include "web_internal.h"

namespace app::web::detail {

namespace {
constexpr const char* TAG = "web";
constexpr size_t kPixels = pixelia::kScreenPixels;
constexpr size_t kHeader = 16;
constexpr size_t kPalette = 256 * 3;
// PackBits braucht im schlechtesten Fall ein Steuerbyte pro 128 Byte
constexpr size_t kOutMax = kHeader + kPalette + kPixels + kPixels / 128 + 16;

esp_err_t exp_get(httpd_req_t* req) { return send_json(req, exper::state_json()); }

int field(const std::string& body, const char* key, int fallback) {
    std::string v;
    return form_value(body, key, &v) ? std::atoi(v.c_str()) : fallback;
}

esp_err_t exp_post(httpd_req_t* req) {
    std::string body, a;
    if (!read_body(req, &body, 512)) return reply(req, false, "Ungültige Anfrage");
    form_value(body, "a", &a);
    if (a == "revert") {
        exper::revert("im Browser zurückgesetzt");
        return reply(req, true, "Zurückgesetzt");
    }
    if (!authorized(req, &body)) return deny(req);
    char error[128] = "";
    if (a == "on" || a == "off") {
        exper::set_enabled(a == "on");
        return reply(req, true, a == "on" ? "Experimentell an" : "Experimentell aus");
    }
    if (a == "panel") {
        exper::Panel p;
        board::display::Timing& t = p.timing;
        t.pclk_hz = static_cast<uint32_t>(field(body, "pclk", t.pclk_hz / 1000)) * 1000;
        t.hsync_pulse = field(body, "hp", t.hsync_pulse);
        t.hsync_back = field(body, "hb", t.hsync_back);
        t.hsync_front = field(body, "hf", t.hsync_front);
        t.vsync_pulse = field(body, "vp", t.vsync_pulse);
        t.vsync_back = field(body, "vb", t.vsync_back);
        t.vsync_front = field(body, "vf", t.vsync_front);
        t.hsync_idle_low = field(body, "hlow", t.hsync_idle_low) != 0;
        t.vsync_idle_low = field(body, "vlow", t.vsync_idle_low) != 0;
        t.pclk_active_neg = field(body, "neg", t.pclk_active_neg) != 0;
        p.eco = field(body, "eco", 0) != 0;
        if (!exper::try_panel(p, error, sizeof(error))) return reply(req, false, error);
        return reply(req, true, "Probe läuft");
    }
    if (a == "confirm") {
        if (!exper::confirm(error, sizeof(error))) return reply(req, false, error);
        return reply(req, true, "Gespeichert");
    }
    if (a == "reset") {
        exper::reset_panel();
        return reply(req, true, "Standardwerte");
    }
    if (a == "test") {
        std::string m;
        board::display::Mode mode;
        if (!form_value(body, "mode", &m) || !board::display::mode_from_name(m.c_str(), &mode) ||
            mode == board::display::Mode::Black) {
            return reply(req, false, "Testbild unbekannt");
        }
        if (!exper::start_test(mode, error, sizeof(error))) return reply(req, false, error);
        return reply(req, true, "OK");
    }
    return reply(req, false, "Aktion unbekannt");
}

// PackBits: Steuerbyte 0–127 = so viele + 1 Bytes folgen roh, 129–255 = nächstes Byte (257 − n)-mal
size_t packbits(const uint8_t* src, size_t n, uint8_t* out) {
    size_t i = 0, o = 0;
    while (i < n) {
        size_t run = 1;
        while (i + run < n && run < 128 && src[i + run] == src[i]) ++run;
        if (run >= 3) {
            out[o++] = static_cast<uint8_t>(257 - run);
            out[o++] = src[i];
            i += run;
            continue;
        }
        // Roh bis zum nächsten Lauf von mindestens drei gleichen Bytes
        size_t lit = 0;
        while (i + lit < n && lit < 128) {
            if (i + lit + 2 < n && src[i + lit] == src[i + lit + 1] && src[i + lit] == src[i + lit + 2]) break;
            ++lit;
        }
        out[o++] = static_cast<uint8_t>(lit - 1);
        std::memcpy(out + o, src + i, lit);
        o += lit;
        i += lit;
    }
    return o;
}

// Antwort (Little Endian): "PXS1", Breite u16, Höhe u16, Bildnummer u32, Verfahren u8 (1 = PackBits), 3 Byte frei,
// Palette 256 × RGB888, dann die gepackten Indizes. Die Vorschau bremst sich selbst: höchstens alle 200 ms ein neues
// Bild aus dem Grafik-Task (sonst kommt das letzte noch einmal), der Browser fragt erst nach der Antwort wieder.
esp_err_t screen_get(httpd_req_t* req) {
    static uint8_t* out = nullptr;
    if (!out) out = static_cast<uint8_t*>(heap_caps_malloc(kOutMax, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    gfx::Frame f;
    if (!out || !gfx::grab(&f, 150, 200)) {
        return reply(req, false, out ? "Kein Bild (Testbild aktiv?)" : "Kein Speicher", "503 Service Unavailable");
    }
    const int64_t t0 = esp_timer_get_time();
    std::memcpy(out, "PXS1", 4);
    const uint16_t w = pixelia::kScreenWidth, h = pixelia::kScreenHeight;
    std::memcpy(out + 4, &w, 2);
    std::memcpy(out + 6, &h, 2);
    std::memcpy(out + 8, &f.number, 4);
    out[12] = 1;
    out[13] = out[14] = out[15] = 0;
    std::memcpy(out + kHeader, f.palette, kPalette);
    const size_t len = kHeader + kPalette + packbits(f.pixels, kPixels, out + kHeader + kPalette);
    const uint32_t pack_us = static_cast<uint32_t>(esp_timer_get_time() - t0);
    char timing[48];
    std::snprintf(timing, sizeof(timing), "copy;dur=%.2f, pack;dur=%.2f", f.copy_us / 1000.0, pack_us / 1000.0);
    httpd_resp_set_type(req, "application/octet-stream");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, "Server-Timing", timing);
    ESP_LOGD(TAG, "Vorschau: Bild %lu, %u Byte, Kopie %lu µs, Packen %lu µs", static_cast<unsigned long>(f.number),
             static_cast<unsigned>(len), static_cast<unsigned long>(f.copy_us), static_cast<unsigned long>(pack_us));
    return httpd_resp_send(req, reinterpret_cast<const char*>(out), static_cast<ssize_t>(len));
}
}  // namespace

void register_exp() {
    add("/api/exp", HTTP_GET, exp_get);
    add("/api/exp", HTTP_POST, exp_post);
    add("/api/screen", HTTP_GET, screen_get);
}

}  // namespace app::web::detail
