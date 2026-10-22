// Seite Experimentell und Live-Vorschau (seit 0.4.2):
//   GET  /api/exp     Schalter, Panel-Werte (aktiv, gespeichert, Standard), Probe, Testbild, Touch-Test
//   POST /api/exp     a=on|off|panel|confirm|revert|reset|test|picoff (mit PIN; revert ohne, macht nur sicherer);
//                     Orte (seit 0.5.0): a=place&n=<Index>, a=trans&v=<Übergang>, a=tour&s=<Sekunden>, a=figures&n=
//                     seit 0.5.1: a=cmd&v=ausbau|baustelle, a=follow&v=0|1, a=inspect&v=0|1
//                     seit 0.5.2 (Testuhr): a=time&v=<Minute des Tages>|real, a=speed&v=<Zeitraffer 1–3600>
//   GET  /api/screen  aktuelles Bild 400×240 als Paletten-Indizes, PackBits-gepackt (Aufbau siehe screen_get)
//   POST /api/picture eigenes Testbild zeigen (seit 0.5.0, Aufbau siehe picture_post)

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "app.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "exper.h"
#include "pixelia/engine/canvas.h"
#include "pixelia/engine/weather.h"
#include "ui.h"
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
    if (a == "picoff") {
        ui::hide_picture();
        return reply(req, true, "Testbild beendet");
    }
    // Orte (Vorschau, seit 0.5.0): nur mit Experimentell an
    if (a == "place" || a == "trans" || a == "tour" || a == "figures" || a == "cmd" || a == "follow" ||
        a == "inspect" || a == "time" || a == "speed" || a == "weather") {
        if (!exper::enabled()) return reply(req, false, "Experimentell ist aus");
        if (a == "place") {
            const int n = field(body, "n", -1);
            if (n < 0 || n >= gfx::place_count()) return reply(req, false, "Ort unbekannt");
            gfx::go_place(n);
            return reply(req, true, gfx::place_title(n));
        }
        if (a == "trans") {
            std::string v;
            pixelia::Transition t;
            if (!form_value(body, "v", &v) || !pixelia::parse_transition(v, &t))
                return reply(req, false, "Übergang unbekannt");
            gfx::set_transition(t);
            return reply(req, true, "Übergang gesetzt");
        }
        if (a == "tour") {
            gfx::set_tour(field(body, "s", 0));
            return reply(req, true, gfx::tour() ? "Rundgang an" : "Rundgang aus");
        }
        if (a == "cmd") {  // seit 0.5.1: ausbau, baustelle; seit 0.5.3 jeder Bühnenbefehl (innenlicht um, blitz …)
            std::string v;
            if (!form_value(body, "v", &v) || !gfx::place_command(v.c_str()))
                return reply(req, false, "Befehl nicht angenommen");
            return reply(req, true, v == "ausbau"      ? "Nächste Ausbaustufe"
                                    : v == "baustelle" ? "Im Bau umgeschaltet"
                                                       : "Befehl gesendet");
        }
        if (a == "weather") {  // seit 0.5.3: Wetter-Teststeuerung, v = Art, p = Stärke in %
            std::string v;
            pixelia::WeatherKind k;
            if (!form_value(body, "v", &v) || !pixelia::parse_weather(v, &k)) return reply(req, false, "Wetter unbekannt");
            const std::string cmd = "wetter " + v + " " + std::to_string(std::clamp(field(body, "p", 100), 0, 100));
            gfx::place_command(cmd.c_str());
            return reply(req, true, "Wetter gesetzt");
        }
        if (a == "follow") {
            gfx::set_follow(field(body, "v", 1) != 0);
            return reply(req, true, gfx::follow() ? "Blick folgt" : "Blick bleibt");
        }
        if (a == "inspect") {
            gfx::set_inspect(field(body, "v", 0) != 0);
            return reply(req, true, gfx::inspect() ? "Prüfansicht an" : "Prüfansicht aus");
        }
        if (a == "time") {  // Testuhr bis zur Weltzeit (0.5.6)
            std::string v;
            form_value(body, "v", &v);
            if (v == "real") {
                gfx::set_real_time(true);
                return reply(req, true, "Echte Uhrzeit");
            }
            const int m = field(body, "v", -1);
            if (m < 0 || m >= 1440) return reply(req, false, "Uhrzeit ungültig");
            gfx::set_time_of_day(m / 60, m % 60);
            return reply(req, true, "Uhrzeit gestellt");
        }
        if (a == "speed") {
            const int f = field(body, "v", 1);
            if (f < 1 || f > 3600) return reply(req, false, "Zeitraffer 1–3600");
            gfx::set_speed(static_cast<float>(f));
            return reply(req, true, f == 1 ? "Zeitraffer aus" : "Zeitraffer an");
        }
        gfx::set_figures(field(body, "n", 0));
        return reply(req, true, "Figuren gesetzt");
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

// Eigenes Testbild (seit 0.5.0): Rumpf = 400×240 Paletten-Indizes (0–239), dann die Palette 240 × RGB888, zusammen
// 96 720 Byte. Der Browser dekodiert das Bild und bildet es auf die Palette ab, hier wird nur direkt ins PSRAM
// empfangen und an den Grafik-Task übergeben. Nichts landet im Flash.
constexpr size_t kPicPalette = pixelia::ui::kFirstIndex * 3;
constexpr size_t kPicBytes = kPixels + kPicPalette;

esp_err_t picture_post(httpd_req_t* req) {
    if (!authorized(req)) return deny(req);
    // seit 0.5.4 ohne Experimentell (Seite Gerätetests)
    if (req->content_len != kPicBytes) return reply(req, false, "Bild muss 400 × 240 groß sein", "400 Bad Request");
    auto* buf = static_cast<uint8_t*>(heap_caps_malloc(kPicBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!buf) return reply(req, false, "Kein Speicher", "503 Service Unavailable");
    const int64_t t0 = esp_timer_get_time();
    size_t got = 0;
    int timeouts = 0;
    while (got < kPicBytes) {
        int r = httpd_req_recv(req, reinterpret_cast<char*>(buf) + got, kPicBytes - got);
        if (r == HTTPD_SOCK_ERR_TIMEOUT && ++timeouts < 3) continue;
        if (r <= 0) {
            heap_caps_free(buf);
            return reply(req, false, "Übertragung abgebrochen");
        }
        got += r;
    }
    // Indizes 240–255 gehören der System-Oberfläche
    for (size_t i = 0; i < kPixels; ++i) {
        if (buf[i] >= pixelia::ui::kFirstIndex) buf[i] = 0;
    }
    char error[96];
    exper::start_test(board::display::Mode::Engine, error, sizeof(error));  // laufenden Displaytest beenden
    ui::show_picture(buf, reinterpret_cast<const pixelia::Color*>(buf + kPixels), ui::kPictureMaxSeconds);
    ESP_LOGI(TAG, "Testbild empfangen (%u Byte in %lu ms), bis Tippen oder höchstens %d min",
             static_cast<unsigned>(kPicBytes), static_cast<unsigned long>((esp_timer_get_time() - t0) / 1000),
             ui::kPictureMaxSeconds / 60);
    return reply(req, true, "Testbild auf dem Display");
}
}  // namespace

void register_exp() {
    add("/api/exp", HTTP_GET, exp_get);
    add("/api/exp", HTTP_POST, exp_post);
    add("/api/screen", HTTP_GET, screen_get);
    add("/api/picture", HTTP_POST, picture_post);
}

}  // namespace app::web::detail
