// Kopflos-Modus: vorspulen, Screenshot, Kontaktbogen und ein kurzer Bericht.
// Grundlage für den Langzeit-Test der Welten (docs/LEBENDIGKEIT.md, Kapitel 3).

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <vector>

#include "pixelia/engine/color.h"
#include "pixelia/engine/png.h"
#include "pixelia/engine/rng.h"
#include "pixelia/engine/scene.h"
#include "pixelia/engine/system_ui.h"
#include "pixelia/engine/tiny_font.h"
#include "sim.h"

namespace sim {

namespace {

using Clock = std::chrono::steady_clock;
using pixelia::kScreenHeight;
using pixelia::kScreenWidth;

double seconds_since(Clock::time_point t) { return std::chrono::duration<double>(Clock::now() - t).count(); }

bool write_file(const std::string& path, const std::vector<uint8_t>& data) {
    std::filesystem::path p(path);
    if (p.has_parent_path()) std::filesystem::create_directories(p.parent_path());
    std::ofstream f(p, std::ios::binary);
    f.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    return f.good();
}

std::string world_time_text(const pixelia::WorldClock& c) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "Tag %d %02d:%02d", c.day(), c.hour(), c.minute());
    return buf;
}

struct Stats {
    uint64_t ticks = 0;
    double tick_seconds = 0;
    uint64_t renders = 0;
    double render_seconds = 0;
};

void step(pixelia::Scene& scene, pixelia::WorldClock& clock, Stats& st, uint64_t count) {
    auto t0 = Clock::now();
    for (uint64_t i = 0; i < count; ++i) {
        clock.advance(pixelia::kTickUs);
        scene.tick(clock);
    }
    st.ticks += count;
    st.tick_seconds += seconds_since(t0);
}

void render(const pixelia::Scene& scene, std::vector<uint8_t>& frame, Stats& st) {
    auto t0 = Clock::now();
    pixelia::Canvas canvas(frame.data(), kScreenWidth, kScreenHeight);
    scene.render(canvas);
    st.renders++;
    st.render_seconds += seconds_since(t0);
}

// Systemanzeige mit Beispieldaten über bzw. statt der Szene zeichnen (gleicher Code wie auf dem Gerät)
bool render_screen(const std::string& name, std::vector<uint8_t>& frame) {
    namespace ui = pixelia::ui;
    pixelia::Canvas canvas(frame.data(), kScreenWidth, kScreenHeight);
    if (name == "setup") {
        ui::SetupInfo info;
        info.ap_password = "kx7m-q4tr";
        info.clients = 1;
        info.saved_networks = 1;
        info.retry_seconds = 102;
        info.last_error = "Heimnetz: Passwort falsch?";
        ui::SetupScreen screen;
        screen.set(info);
        screen.render(canvas);
    } else if (name == "setup-fehler") {
        ui::SetupInfo info;
        info.ap_password = "kx7m-q4tr";
        info.clients = 2;
        info.saved_networks = 2;
        info.retry_seconds = 95;
        info.last_error = "Heimnetz-Obergeschoss-5G: Verschlüsselung des Routers passt nicht";
        ui::SetupScreen screen;
        screen.set(info);
        screen.render(canvas);
    } else if (name == "setup-leer") {
        ui::SetupInfo info;
        info.ap_password = "kx7m-q4tr";
        ui::SetupScreen screen;
        screen.set(info);
        screen.render(canvas);
    } else if (name == "update") {
        ui::render_update(canvas, {ui::UpdateInfo::Phase::Receiving, "0.2.0", "0.2.1", 312 * 1024, 664 * 1024, ""});
    } else if (name == "update-fertig") {
        ui::render_update(canvas, {ui::UpdateInfo::Phase::Done, "0.2.0", "0.2.1", 664 * 1024, 664 * 1024, ""});
    } else if (name == "failed") {
        ui::render_update(canvas, {ui::UpdateInfo::Phase::Failed, "0.2.0", "", 0, 0,
                                   "Das ist keine Pixelia-Firmware (Projekt „blink“)."});
    } else if (name == "message") {
        ui::render_message(canvas, {"Verbunden",
                                    "WLAN „Heimnetz“, Adresse 192.168.178.42\nIm Browser: http://192.168.178.42/ oder "
                                    "http://pixelia.local\nLanger Druck aufs Display zeigt diese Daten jederzeit.",
                                    ui::Tone::Success});
    } else if (name == "toast") {
        ui::draw_toast(canvas, "Update auf 0.2.1 wurde zurückgerollt: Absturz im Probelauf", ui::Tone::Warning);
    } else if (name == "toast-lang") {
        ui::draw_toast(canvas,
                       "Update auf 0.1.0-2-gcc56269-test-selftest zurückgerollt: Selbsttest nicht bestanden: Engine "
                       "liefert keine Bilder",
                       ui::Tone::Warning);
    } else if (name == "info" || name == "info-setup" || name == "info-offline") {
        // Info-Karte über der laufenden Szene (langer Druck aufs Display)
        ui::InfoCard card;
        card.set(sample_info(name));
        card.render(canvas);
    } else if (name == "hold") {
        ui::draw_hold_indicator(canvas, 0.55f);
    } else if (name == "toast-ip") {
        ui::draw_toast(canvas, "Verbunden mit „Heimnetz“ · 192.168.178.42", ui::Tone::Success);
    } else {
        return false;
    }
    return true;
}

// RGB-Bild mit Hilfsfunktionen zum Beschriften
struct RgbImage {
    int w, h;
    std::vector<uint8_t> px;
    RgbImage(int width, int height, uint8_t r, uint8_t g, uint8_t b) : w(width), h(height), px(width * height * 3) {
        for (size_t i = 0; i < px.size(); i += 3) {
            px[i] = r;
            px[i + 1] = g;
            px[i + 2] = b;
        }
    }
    void set(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        if (x < 0 || y < 0 || x >= w || y >= h) return;
        uint8_t* p = &px[(static_cast<size_t>(y) * w + x) * 3];
        p[0] = r;
        p[1] = g;
        p[2] = b;
    }
    void rect(int x, int y, int rw, int rh, uint8_t r, uint8_t g, uint8_t b) {
        for (int yy = y; yy < y + rh; ++yy)
            for (int xx = x; xx < x + rw; ++xx) set(xx, yy, r, g, b);
    }
    void text(const char* s, int x, int y, int scale, uint8_t r, uint8_t g, uint8_t b) {
        pixelia::tiny_font::draw(s, x, y, scale, [&](int px_, int py_) { set(px_, py_, r, g, b); });
    }
    void blit(const uint8_t* rgb, int bw, int bh, int x, int y) {
        for (int yy = 0; yy < bh; ++yy)
            std::copy_n(rgb + static_cast<size_t>(yy) * bw * 3, bw * 3, &px[(static_cast<size_t>(y + yy) * w + x) * 3]);
    }
};

int contact_sheet(const Options& opt, pixelia::Scene& scene, pixelia::WorldClock& clock, Stats& st,
                  std::vector<uint8_t>& frame) {
    const int n = std::max(1, opt.moments);
    // Zeitraffer: Ohne eigene Angabe läuft der Kontaktbogen mit 60x (eine Weltminute pro Sekunde)
    if (opt.speed == 1.0) clock.set_speed(60.0);
    const int64_t start = clock.world_us();
    const int64_t span = static_cast<int64_t>(opt.span_days * pixelia::WorldClock::kUsPerDay);
    pixelia::Rng rng(opt.seed * 0x9E3779B97F4A7C15ULL + 24);
    std::vector<int64_t> moments(n);
    for (auto& m : moments) m = start + static_cast<int64_t>((static_cast<double>(rng.next()) / 4294967296.0) * span);
    std::sort(moments.begin(), moments.end());

    const int cols = std::min(n, 6);
    const int rows = (n + cols - 1) / cols;
    const int gap = 6, header = 26, label_h = 0;
    RgbImage sheet(gap + cols * (kScreenWidth + gap), header + rows * (kScreenHeight + label_h + gap), 22, 22, 28);
    char title[96];
    std::snprintf(title, sizeof(title), "PIXELIA KONTAKTBOGEN  %s  SEED %llu  %d MOMENTE", opt.scene.c_str(),
                  static_cast<unsigned long long>(opt.seed), n);
    for (char* p = title; *p; ++p) *p = static_cast<char>(std::toupper(static_cast<unsigned char>(*p)));
    sheet.text(title, gap, 8, 2, 230, 230, 236);

    std::vector<uint8_t> rgb(static_cast<size_t>(kScreenWidth) * kScreenHeight * 3);
    for (int i = 0; i < n; ++i) {
        // vorspulen bis zum Moment (Schritte mit der Zeitraffer-Weite)
        int64_t per_tick = std::max<int64_t>(1, static_cast<int64_t>(pixelia::kTickUs * clock.speed()));
        int64_t remaining = moments[i] - clock.world_us();
        if (remaining > 0) step(scene, clock, st, static_cast<uint64_t>((remaining + per_tick - 1) / per_tick));
        render(scene, frame, st);
        pixelia::indexed_to_rgb(frame.data(), kScreenWidth, kScreenHeight, scene.palette(), 1, opt.device_colors,
                                rgb.data());
        int x = gap + (i % cols) * (kScreenWidth + gap);
        int y = header + (i / cols) * (kScreenHeight + label_h + gap);
        sheet.blit(rgb.data(), kScreenWidth, kScreenHeight, x, y);
        char label[32];
        std::snprintf(label, sizeof(label), "TAG %d %02d:%02d", clock.day(), clock.hour(), clock.minute());
        int tw = pixelia::tiny_font::text_width(label) * 2;
        sheet.rect(x + 4, y + kScreenHeight - 18, tw + 8, 14, 0, 0, 0);
        sheet.text(label, x + 8, y + kScreenHeight - 15, 2, 255, 255, 255);
    }
    if (!write_file(opt.contact_sheet, pixelia::encode_png_rgb(sheet.px.data(), sheet.w, sheet.h))) {
        std::printf("FEHLER: %s nicht schreibbar\n", opt.contact_sheet.c_str());
        return 1;
    }
    std::printf("sim: Kontaktbogen %s (%dx%d, %d Momente über %.1f Tag(e), Zeitraffer %gx)\n",
                opt.contact_sheet.c_str(), sheet.w, sheet.h, n, opt.span_days, clock.speed());
    return 0;
}

}  // namespace

pixelia::ui::InfoCardData sample_info(const std::string& variant) {
    pixelia::ui::InfoCardData d;
    d.version = "0.2.1";
    if (variant == "info-setup") {
        d.mode = pixelia::ui::InfoCardData::Mode::Setup;
        d.ap_ssid = "Pixelia-Setup";
        d.ap_password = "kx7m-q4tr";
        d.ap_ip = "192.168.4.1";
        d.ap_clients = 1;
    } else if (variant == "info-offline") {
        d.mode = pixelia::ui::InfoCardData::Mode::Offline;
        d.status = "Verbinde mit „Heimnetz“ …\nZuletzt: Passwort falsch?";
    } else {
        d.ip = "192.168.178.42";
        d.ssid = "Heimnetz";
        d.rssi = -63;
    }
    return d;
}

int run_headless(const Options& opt) {
    auto scene = pixelia::make_scene(opt.scene.c_str());
    scene->init(opt.seed);
    if (opt.load >= 0) scene->set_load(opt.load);
    pixelia::WorldClock clock;
    clock.set(opt.day, opt.hour, opt.minute);
    clock.set_speed(opt.speed);
    std::printf("sim: Szene %s, Startwert %llu, Start %s, Zeitraffer %gx\n", scene->name(),
                static_cast<unsigned long long>(opt.seed), world_time_text(clock).c_str(), clock.speed());

    Stats st;
    std::vector<uint8_t> frame(pixelia::kScreenPixels);
    // Ein Schritt ohne Weltzeit bringt die Szene auf die Startzeit (Palette, Licht), ohne die Uhr zu verstellen
    clock.advance(0);
    scene->tick(clock);
    if (opt.frames > 0) step(*scene, clock, st, static_cast<uint64_t>(opt.frames));

    int rc = 0;
    if (!opt.screenshot.empty()) {
        render(*scene, frame, st);
        pixelia::Palette palette = scene->palette();
        pixelia::ui::apply_palette(palette);
        if (!opt.screen.empty() && !render_screen(opt.screen, frame)) {
            std::printf("FEHLER: Systemanzeige \"%s\" gibt es nicht (setup, setup-leer, setup-fehler, update, "
                        "update-fertig, failed, message, toast, toast-lang, toast-ip, info, info-setup, "
                        "info-offline, hold)\n", opt.screen.c_str());
            return 2;
        }
        const int s = std::max(1, opt.scale);
        std::vector<uint8_t> rgb(static_cast<size_t>(kScreenWidth) * s * kScreenHeight * s * 3);
        pixelia::indexed_to_rgb(frame.data(), kScreenWidth, kScreenHeight, palette, s, opt.device_colors,
                                rgb.data());
        if (write_file(opt.screenshot, pixelia::encode_png_rgb(rgb.data(), kScreenWidth * s, kScreenHeight * s))) {
            std::printf("sim: Screenshot %s (%dx%d, %s)\n", opt.screenshot.c_str(), kScreenWidth * s,
                        kScreenHeight * s, world_time_text(clock).c_str());
        } else {
            std::printf("FEHLER: %s nicht schreibbar\n", opt.screenshot.c_str());
            rc = 1;
        }
    }
    if (!opt.contact_sheet.empty()) rc |= contact_sheet(opt, *scene, clock, st, frame);
    if (opt.screenshot.empty() && opt.contact_sheet.empty() && opt.frames == 0) {
        std::printf("sim: nichts zu tun (--frames, --screenshot oder --contact-sheet angeben)\n");
    }

    std::printf("sim: Bericht: %llu Logik-Schritte (%.2f µs/Schritt), %llu Bilder (%.1f µs/Bild), Weltzeit jetzt %s\n",
                static_cast<unsigned long long>(st.ticks), st.ticks ? st.tick_seconds * 1e6 / st.ticks : 0.0,
                static_cast<unsigned long long>(st.renders), st.renders ? st.render_seconds * 1e6 / st.renders : 0.0,
                world_time_text(clock).c_str());
    return rc;
}

}  // namespace sim
