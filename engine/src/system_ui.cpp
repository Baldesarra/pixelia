#include "pixelia/engine/system_ui.h"

#include <algorithm>
#include <cstdio>

#include "pixelia/engine/font.h"

namespace pixelia::ui {

namespace {

constexpr int kW = kScreenWidth;
constexpr int kH = kScreenHeight;

void text_right(Canvas& c, int right, int y, std::string_view s, uint8_t color, int scale = 1) {
    font::draw(c, right - font::text_width(s, scale), y, s, color, scale);
}

// Rahmen aus zwei abgerundeten Flächen: außen Kontur, innen Füllung
void framed(Canvas& c, int x, int y, int w, int h, uint8_t fill, uint8_t border, int radius = 3) {
    round_rect(c, x, y, w, h, border, radius);
    round_rect(c, x + 1, y + 1, w - 2, h - 2, fill, radius > 0 ? radius - 1 : 0);
}

void badge(Canvas& c, int x, int y, const char* digit) {
    round_rect(c, x, y, 13, 13, kPrimary, 3);
    font::draw_centered(c, x + 7, y + 3 - font::kCapTop + 0, digit, kOnPrimary);
}

void progress_bar(Canvas& c, int x, int y, int w, int h, uint32_t done, uint32_t total) {
    round_rect(c, x, y, w, h, kOutline, 2);
    round_rect(c, x + 1, y + 1, w - 2, h - 2, kSurfaceHigh, 1);
    if (total == 0) return;
    int fill = static_cast<int>(static_cast<uint64_t>(w - 2) * std::min(done, total) / total);
    if (fill > 0) round_rect(c, x + 1, y + 1, fill, h - 2, kPrimary, 1);
}

std::string kb(uint32_t bytes) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>((bytes + 512) / 1024));
    return buf;
}

}  // namespace

void apply_palette(Palette& p) {
    p[kBg] = {20, 18, 24};
    p[kSurface] = {31, 28, 38};
    p[kSurfaceHigh] = {46, 42, 56};
    p[kOutline] = {74, 68, 90};
    p[kTextDim] = {176, 169, 191};
    p[kText] = {236, 231, 245};
    p[kPrimary] = {196, 181, 255};
    p[kPrimaryDim] = {107, 79, 216};
    p[kOnPrimary] = {42, 26, 110};
    p[kSuccess] = {125, 220, 150};
    p[kWarning] = {255, 210, 120};
    p[kError] = {255, 128, 128};
    p[kQrDark] = {0, 0, 0};
    p[kQrLight] = {255, 255, 255};
    p[kShadow] = {8, 7, 10};
    p[kWhite] = {255, 255, 255};
}

uint8_t tone_color(Tone tone) {
    switch (tone) {
        case Tone::Success: return kSuccess;
        case Tone::Warning: return kWarning;
        case Tone::Error: return kError;
        default: return kPrimary;
    }
}

void round_rect(Canvas& c, int x, int y, int w, int h, uint8_t color, int r) {
    if (w <= 0 || h <= 0) return;
    r = std::min({r, w / 2, h / 2});
    for (int yy = 0; yy < h; ++yy) {
        int inset = yy < r ? r - yy : (yy >= h - r ? yy - (h - 1 - r) : 0);
        c.fill_rect(x + inset, y + yy, w - 2 * inset, 1, color);
    }
}

void draw_qr(Canvas& c, const qr::Code& code, int x, int y, int scale, int quiet) {
    const int total = (code.size + 2 * quiet) * scale;
    c.fill_rect(x, y, total, total, kQrLight);
    for (int my = 0; my < code.size; ++my)
        for (int mx = 0; mx < code.size; ++mx)
            if (code.dark(mx, my)) c.fill_rect(x + (mx + quiet) * scale, y + (my + quiet) * scale, scale, scale, kQrDark);
}

void SetupScreen::set(const SetupInfo& info) {
    std::string key = info.ap_ssid + '\n' + info.ap_password + '\n' + info.ap_url;
    if (key != qr_key_) {
        qr::encode(qr::wifi_payload(info.ap_ssid, info.ap_password), qr::Ecc::Medium, &wifi_qr_);
        qr::encode(info.ap_url, qr::Ecc::Medium, &url_qr_);
        qr_key_ = std::move(key);
    }
    info_ = info;
}

void SetupScreen::render(Canvas& c) const {
    c.clear(kBg);

    // Kopfzeile
    c.fill_rect(0, 0, kW, 26, kSurface);
    c.fill_rect(0, 26, kW, 1, kOutline);
    font::draw(c, 12, 3, "Pixelia einrichten", kText, 2);
    text_right(c, kW - 12, 9, "Setup-Modus", kPrimary);

    // Drei Schritte als Karten
    constexpr int kCardY = 33, kCardH = 182, kCardW = 124, kGap = 6, kMargin = 8;
    const char* titles[3] = {"WLAN verbinden", "Seite öffnen", "WLAN eintragen"};
    const char* digits[3] = {"1", "2", "3"};
    for (int i = 0; i < 3; ++i) {
        int x = kMargin + i * (kCardW + kGap);
        framed(c, x, kCardY, kCardW, kCardH, kSurface, kOutline);
        badge(c, x + 8, kCardY + 8, digits[i]);
        font::draw(c, x + 26, kCardY + 9 - font::kCapTop, titles[i], kText);
    }
    const int cx0 = kMargin + kCardW / 2;
    const int cx1 = cx0 + kCardW + kGap;
    const int x2 = kMargin + 2 * (kCardW + kGap);

    // 1: WLAN-QR, Name und Passwort (groß zum Abtippen)
    constexpr int kQrTop = kCardY + 28;
    constexpr int kScale = 3, kQuiet = 2;
    int qr1 = (wifi_qr_.size + 2 * kQuiet) * kScale;
    if (wifi_qr_.size) draw_qr(c, wifi_qr_, cx0 - qr1 / 2, kQrTop, kScale, kQuiet);
    int ty = kQrTop + qr1 + 5;
    font::draw_centered(c, cx0, ty, info_.ap_ssid, kText);
    if (!info_.ap_password.empty()) {
        font::draw_centered(c, cx0, ty + 14, "Passwort", kTextDim);
        font::draw_centered(c, cx0, ty + 26, info_.ap_password, kPrimary, 2);
    } else {
        font::draw_centered(c, cx0, ty + 14, "ohne Passwort", kTextDim);
    }

    // 2: Adresse der Einrichtungsseite
    int qr2 = (url_qr_.size + 2 * kQuiet) * kScale;
    if (url_qr_.size) draw_qr(c, url_qr_, cx1 - qr2 / 2, kQrTop + (qr1 - qr2) / 2, kScale, kQuiet);
    font::draw_centered(c, cx1, ty, "Öffnet sich meist", kTextDim);
    font::draw_centered(c, cx1, ty + 12, "von selbst. Sonst:", kTextDim);
    std::string_view url = info_.ap_url;
    if (url.substr(0, 7) == "http://") url.remove_prefix(7);
    font::draw_centered(c, cx1, ty + 28, url, kPrimary);

    // 3: Anleitung
    font::draw_wrapped(c, x2 + 9, kCardY + 30, kCardW - 16,
                       "Auf der Seite dein WLAN auswählen und das Passwort eingeben.", kText);
    int lines = font::draw_wrapped(c, x2 + 9, kCardY + 82, kCardW - 16,
                                   "Pixelia verbindet sich danach von selbst. Im Heimnetz dann unter:", kTextDim);
    font::draw(c, x2 + 9, kCardY + 84 + lines * font::kLineHeight, "pixelia.local", kPrimary);
    if (info_.saved_networks > 0) {
        char buf[48];
        std::snprintf(buf, sizeof(buf), "%d WLAN%s gespeichert", info_.saved_networks,
                      info_.saved_networks == 1 ? "" : "s");
        font::draw(c, x2 + 9, kCardY + kCardH - 20, buf, kTextDim);
    }

    // Fußzeile: Geräte am Setup-WLAN und nächster Verbindungsversuch
    constexpr int kFootY = kH - 19;
    char left[48];
    if (info_.clients <= 0) std::snprintf(left, sizeof(left), "Noch kein Gerät verbunden");
    else std::snprintf(left, sizeof(left), "%d Gerät%s verbunden", info_.clients, info_.clients == 1 ? "" : "e");
    c.fill_rect(kMargin + 2, kFootY + 4, 5, 5, info_.clients > 0 ? kSuccess : kOutline);
    font::draw(c, kMargin + 11, kFootY + 2, left, kTextDim);

    std::string right;
    uint8_t right_color = kTextDim;
    if (!info_.connecting.empty()) {
        right = "Verbinde mit „" + info_.connecting + "“ …";
        right_color = kPrimary;
    } else {
        if (!info_.last_error.empty()) {
            right = info_.last_error;
            right_color = kWarning;
        }
        if (info_.retry_seconds >= 0) {
            // Countdown zuerst: Wird gekürzt, trifft es den Fehlertext dahinter
            char buf[40];
            std::snprintf(buf, sizeof(buf), "Neuer Versuch in %d:%02d", info_.retry_seconds / 60,
                          info_.retry_seconds % 60);
            right = right.empty() ? std::string(buf) : std::string(buf) + " · " + right;
        } else if (info_.saved_networks == 0 && right.empty()) {
            right = "Noch kein WLAN gespeichert";
        }
    }
    // Passt der rechte Text nicht neben den linken, hat er Vorrang (Fehler und Countdown sind wichtiger)
    int left_end = kMargin + 11 + font::text_width(left) + 12;
    if (kW - kMargin - font::text_width(right) < left_end) {
        c.fill_rect(0, kFootY, kW, kH - kFootY, kBg);
        right = font::ellipsize(right, kW - 2 * kMargin);
    }
    text_right(c, kW - kMargin, kFootY + 2, right, right_color);
}

void render_update(Canvas& c, const UpdateInfo& u) {
    using Phase = UpdateInfo::Phase;
    c.clear(kBg);
    constexpr int kPx = 40, kPy = 46, kPw = 320, kPh = 148;
    framed(c, kPx, kPy, kPw, kPh, kSurface, kOutline, 4);
    const int cx = kW / 2;

    const char* title = "Update wird installiert";
    uint8_t title_color = kText;
    if (u.phase == Phase::Done) {
        title = "Update installiert";
        title_color = kSuccess;
    } else if (u.phase == Phase::Failed) {
        title = "Update fehlgeschlagen";
        title_color = kError;
    }
    font::draw_centered(c, cx, kPy + 12, title, title_color, 2);

    std::string versions = u.from_version.empty() || u.to_version.empty() ? u.from_version + u.to_version
                                                                           : u.from_version + "  →  " + u.to_version;
    if (!versions.empty()) font::draw_centered(c, cx, kPy + 40, font::ellipsize(versions, kPw - 24), kTextDim);

    if (u.phase == Phase::Failed) {
        int lines = font::wrapped_lines(kPw - 40, u.message);
        font::draw_wrapped(c, kPx + 20, kPy + 62, kPw - 40, u.message, kText);
        font::draw_centered(c, cx, kPy + 62 + std::max(lines, 1) * font::kLineHeight + 14,
                            "Die bisherige Version läuft weiter.", kTextDim);
        return;
    }

    uint32_t done = u.phase == Phase::Receiving ? u.bytes : u.total;
    progress_bar(c, kPx + 20, kPy + 62, kPw - 40, 14, done, u.total ? u.total : 1);

    std::string line;
    if (u.phase == Phase::Receiving && u.total) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%u %%  ·  %s von %s KB",
                      static_cast<unsigned>(static_cast<uint64_t>(u.bytes) * 100 / u.total), kb(u.bytes).c_str(),
                      kb(u.total).c_str());
        line = buf;
    } else if (u.phase == Phase::Receiving) {
        line = "Warte auf Daten …";
    } else if (u.phase == Phase::Verifying) {
        line = "Prüfsumme wird geprüft …";
    } else {
        line = "Neustart …";
    }
    font::draw_centered(c, cx, kPy + 84, line, kText);
    if (u.phase != Phase::Done) font::draw_centered(c, cx, kPy + 112, "Bitte nicht ausschalten.", kWarning);
    else font::draw_centered(c, cx, kPy + 112, "Danach folgt ein Probelauf mit Selbsttest.", kTextDim);
}

void render_message(Canvas& c, const MessageInfo& m) {
    c.clear(kBg);
    constexpr int kPx = 30, kPw = 340;
    int lines = std::max(1, font::wrapped_lines(kPw - 40, m.body));
    int ph = 54 + lines * font::kLineHeight + 12;
    int py = (kH - ph) / 2;
    framed(c, kPx, py, kPw, ph, kSurface, kOutline, 4);
    c.fill_rect(kPx + 1, py + 8, 3, ph - 16, tone_color(m.tone));
    const uint8_t title_color = m.tone == Tone::Info ? static_cast<uint8_t>(kText) : tone_color(m.tone);
    font::draw(c, kPx + 20, py + 12, font::ellipsize(m.title, kPw - 40, 2), title_color, 2);
    font::draw_wrapped(c, kPx + 20, py + 46, kPw - 40, m.body, kText);
}

void draw_toast(Canvas& c, std::string_view text, Tone tone) {
    // Lange Meldungen umbrechen (höchstens 3 Zeilen), der Kasten wächst nach oben
    constexpr int kMaxLines = 3, kPadLeft = 18, kPadRight = 8;
    constexpr int kMaxText = kW - 24 - kPadLeft - kPadRight;
    int widest = 0;
    const int lines = std::min(kMaxLines, std::max(1, font::wrapped_lines(kMaxText, text, 1, &widest)));
    const int w = widest + kPadLeft + kPadRight, h = 17 + (lines - 1) * font::kLineHeight;
    const int x = (kW - w) / 2, y = kH - h - 6;
    round_rect(c, x - 1, y + 1, w + 2, h + 1, kShadow, 4);
    framed(c, x, y, w, h, kSurface, kOutline, 4);
    round_rect(c, x + 8, y + 6, 5, 5, tone_color(tone), 1);
    font::draw_wrapped(c, x + kPadLeft, y + 5 - font::kCapTop, kMaxText, text, kText, 1, 0, kMaxLines);
}

}  // namespace pixelia::ui
