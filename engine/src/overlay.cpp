#include "pixelia/engine/overlay.h"

#include <algorithm>
#include <cstdio>

#include "pixelia/engine/assets.h"
#include "pixelia/engine/ledger.h"
#include "pixelia/engine/projects.h"

namespace pixelia::ui {

namespace {

constexpr int kW = kScreenWidth;
constexpr int kH = kScreenHeight;
SpriteSheet s_symbols{};

// Schatten nur dort, wo er neben und unter der Karte zu sehen ist (eine Fläche darunter kostet im PSRAM zu viel)
void shadowed(Canvas& c, int x, int y, int w, int h, int radius) {
    c.fill_rect(x - 1, y + 2 + radius, 1, h - 2 * radius, kShadow);
    c.fill_rect(x + w, y + 2 + radius, 1, h - 2 * radius, kShadow);
    c.fill_rect(x + radius, y + h, w - 2 * radius, 2, kShadow);
    framed(c, x, y, w, h, kSurface, kOutline, radius);
}

// Bild vergrößert zeichnen (Farbe 0 durchsichtig), beschnitten auf clip
void blit_scaled(Canvas& c, const CardPicture& p, int x, int y, const Rect& clip) {
    const Sprite& s = p.sprite;
    if (!s.pixels) return;
    const int k = std::max(1, p.scale);
    for (int sy = 0; sy < s.height; ++sy)
        for (int sx = 0; sx < s.width; ++sx) {
            uint8_t v = s.pixels[sy * s.width + (p.flip ? s.width - 1 - sx : sx)];
            if (!v) continue;
            if (p.remap) v = p.remap[v];
            if (v < p.light_base) v = static_cast<uint8_t>(v + p.light_add);
            const int px = x + sx * k, py = y + sy * k;
            const int x0 = std::max(px, clip.x), x1 = std::min(px + k, clip.x + clip.w);
            const int y0 = std::max(py, clip.y), y1 = std::min(py + k, clip.y + clip.h);
            if (x0 < x1 && y0 < y1) c.fill_rect(x0, y0, x1 - x0, y1 - y0, v);
        }
}

// Bildkasten: Hintergrund, Bild unten mittig (Figuren stehen auf dem Boden des Kastens)
void picture_box(Canvas& c, const CardPicture& p, int x, int y, int w, int h) {
    framed(c, x, y, w, h, kSurfaceHigh, kOutline, 3);
    const int pw = p.sprite.width * std::max(1, p.scale), ph = p.sprite.height * std::max(1, p.scale);
    blit_scaled(c, p, x + (w - pw) / 2, y + h - 3 - ph, {x + 1, y + 1, w - 2, h - 2});
}

// Markierung über der Figur in der Welt: kleiner Pfeil nach unten
void mark(Canvas& c, const Card& card) {
    if (card.mark_x < 0) return;
    const int x = card.mark_x, y = card.mark_y - card.mark_h - 12;
    for (int r = 0; r < 6; ++r) c.fill_rect(x - 6 + r, y + r, 13 - 2 * r, 1, kBg);
    for (int r = 0; r < 4; ++r) c.fill_rect(x - 4 + r, y + 1 + r, 9 - 2 * r, 1, kPrimary);
}

int wrapped(Canvas& c, int x, int y, int w, std::string_view s, uint8_t color, int max_lines, font::Style st) {
    const int n = std::min(font::wrapped_lines(w, s, 1, nullptr, st), max_lines);
    font::draw_wrapped(c, x, y, w, s, color, 1, 0, max_lines, st);
    return n;
}

void footer(Canvas& c, const Card& card, const CardStyle& st, const Rect& r);

void hint(Canvas& c, int x, int y, const Card& card, const CardStyle& st, bool right_aligned, int right) {
    if (card.hint.empty()) return;
    const int sym = card.hint_symbol != Symbol::None && s_symbols.pixels ? 11 : 0;
    const int w = sym + font::text_width(card.hint, 1, st.small);
    const int x0 = right_aligned ? right - w : x;
    const int ty = y + font::cap_top(font::Style::Normal) - font::cap_top(st.small) +
                   (font::cap_height(font::Style::Normal) - font::cap_height(st.small)) / 2;
    if (sym) draw_symbol(c, x0, y + 1, card.hint_symbol, kTextDim);
    font::draw(c, x0 + sym, ty, card.hint, kTextDim, 1, st.small);
}

void title(Canvas& c, int x, int y, int w, const Card& card, const CardStyle& st) {
    font::draw(c, x, y, font::ellipsize(card.title, w, st.title_scale, st.title), kText, st.title_scale, st.title);
}

int title_h(const CardStyle& st) { return font::line_height(st.title, st.title_scale); }

// Tätigkeit und Ort untereinander, je mit Symbol davor; gibt das untere Ende zurück
int info_lines(Canvas& c, int x, int y, int w, int max_y, const Card& card) {
    const std::string* text[2] = {&card.subtitle, &card.where};
    const Symbol sym[2] = {card.subtitle_symbol, card.where_symbol};
    for (int k = 0; k < 2; ++k) {
        if (text[k]->empty() || y + font::kLineHeight > max_y) continue;
        int tx = x;
        if (sym[k] != Symbol::None && s_symbols.pixels) {
            draw_symbol(c, x, y + 1, sym[k], kTextDim);
            tx += 12;
        }
        const int lines = std::max(1, std::min(2, (max_y - y) / font::kLineHeight));
        y += wrapped(c, tx, y, x + w - tx, *text[k], kTextDim, lines, font::Style::Normal) * font::kLineHeight;
    }
    return y;
}

// Stufe in einer Zeile (Wahl des Users 2026-10-28): Name links, rechts drei Segmente; das nächste füllt sich mit der
// Erfahrung (dunkler). Gibt die Höhe zurück
int meter(Canvas& c, int x, int y, int w, const CardMeter& m) {
    constexpr int kSeg = 10, kGap = 1, kW = 3 * kSeg + 2 * kGap;
    font::draw(c, x, y, font::ellipsize(m.label, w - kW - 6), kTextDim);
    const int my = y + font::kCapTop + (font::kCapHeight - 5) / 2;
    for (int k = 0; k < 3; ++k) {
        const int sx = x + w - kW + k * (kSeg + kGap);
        c.fill_rect(sx, my, kSeg, 5, kSurfaceHigh);
        const int fill = k < m.level ? kSeg : k == m.level ? m.progress * kSeg / 255 : 0;
        if (fill) c.fill_rect(sx, my, fill, 5, k < m.level ? kPrimary : kPrimaryDim);
    }
    return font::kLineHeight;
}

// Punkt eines Schritts (7 × 7): fertig gefüllt, läuft als Ring mit gefüllter linker Hälfte, offen als grauer Ring
void step_dot(Canvas& c, int x, int y, uint8_t state) {
    static constexpr const char* kDot[7] = {"..###..", ".#ooo#.", "#ooooo#", "#ooooo#", "#ooooo#", ".#ooo#.", "..###.."};
    const uint8_t ink = state == 0 ? kSurfaceHigh : kPrimary;
    for (int j = 0; j < 7; ++j)
        for (int i = 0; i < 7; ++i) {
            const char m = kDot[j][i];
            if (m == '#' || (m == 'o' && (state == 2 || (state == 1 && i <= 3)))) c.fill_rect(x + i, y + j, 1, 1, ink);
        }
}

// Teilaufgaben als senkrechte Leiste (seit 0.5.10): Punkt, Name, beim laufenden Schritt darunter das Material; gibt
// das untere Ende zurück
int steps(Canvas& c, int x, int y, int w, int max_y, const Card& card) {
    const int lh = font::kLineHeight;
    const int tx = x + 11, tw = w - 11;
    int prev_dot = -1;
    uint8_t prev_state = 0;
    for (int i = 0; i < card.step_count && y + lh <= max_y; ++i) {
        const CardStep& s = card.steps[i];
        const int dot = y + font::kCapTop;
        if (prev_dot >= 0)  // Linie vom vorigen Punkt zu diesem; fertige Strecke in der Akzentfarbe
            c.fill_rect(x + 3, prev_dot + 7, 1, dot - prev_dot - 7, prev_state == 2 ? kPrimary : kSurfaceHigh);
        step_dot(c, x, dot, s.state);
        font::draw(c, tx, y, font::ellipsize(s.title, tw), s.state == 1 ? kText : kTextDim);
        y += lh;
        for (int k = 0; s.state == 1 && k < s.detail_count && y + lh <= max_y; ++k) {
            font::draw(c, tx + 4, y, font::ellipsize(s.detail[k], tw - 4), kTextDim);
            y += lh;
        }
        y += 2;
        prev_dot = dot;
        prev_state = s.state;
    }
    return y + 2;
}

// Gedanke bzw. Beschreibung und Angaben untereinander; gibt das untere Ende zurück
int body(Canvas& c, int x, int y, int w, int max_y, const Card& card, const CardStyle& st) {
    const int lh = font::kLineHeight;
    if (card.step_count > 0) y = steps(c, x, y, w, max_y, card);
    if (!card.quote.empty()) {
        int tx = x;
        if (card.kind == Card::Kind::Figure && s_symbols.pixels) {
            draw_symbol(c, x, y + 1, Symbol::Thought, kPrimary);
            tx += 13;
        }
        const int n = wrapped(c, tx, y, x + w - tx, card.quote, kText, std::max(1, (max_y - y) / lh), font::Style::Normal);
        y += n * lh + 4;
    }
    for (int i = 0; i < card.line_count && y + lh <= max_y; ++i) {
        const int n = wrapped(c, x, y, w, card.lines[i], kTextDim, std::max(1, (max_y - y) / lh), font::Style::Normal);
        y += n * lh;
    }
    for (int i = 0; i < card.meter_count && y + lh <= max_y; ++i) y += meter(c, x, y, w, card.meters[i]);
    return y;
}

constexpr int kSideX = 244, kSideY = 24, kSideW = 150, kSideH = 192;

// Fuß der Seitenkarte: Knopf „Folgen“ (Figur) bzw. „Betreten“ (Tür), Wünsche des Users 2026-10-22; sonst der Hinweis
void footer(Canvas& c, const Card& card, const CardStyle& st, const Rect& r) {
    const Rect b = card_button(card, st);
    if (!b.w) {
        hint(c, r.x + 8, r.y + r.h - 16, card, st, false, 0);
        return;
    }
    round_rect(c, b.x, b.y, b.w, b.h, kPrimary, 4);
    const int sym = card.hint_symbol != Symbol::None && s_symbols.pixels ? 12 : 0;
    const int x = b.x + (b.w - sym - font::text_width(card.hint, 1, font::Style::Bold)) / 2;
    if (sym) draw_symbol(c, x, b.y + (b.h - 9) / 2, card.hint_symbol, kOnPrimary);
    font::draw(c, x + sym, b.y + (b.h - font::kCapHeight) / 2 - font::kCapTop, card.hint, kOnPrimary, 1, font::Style::Bold);
}

}  // namespace

Rect card_button(const Card& card, const CardStyle& st) {
    if (st.layout != CardLayout::Side || (!card.figure && !card.place) || card.hint.empty()) return {};
    return {kSideX + 8, kSideY + kSideH - 28, kSideW - 16, 20};
}

// --- Symbole ---

bool load_symbols(AssetStore& store) {
    if (s_symbols.pixels) return true;
    if (!store.load_base("system")) return false;
    ImageAsset img;
    if (!store.image(asset_id("ui/symbole"), &img)) return false;
    s_symbols = img.sheet;
    return true;
}

void draw_symbol(Canvas& c, int x, int y, Symbol symbol, uint8_t color) {
    if (!s_symbols.pixels || symbol == Symbol::None || static_cast<int>(symbol) >= s_symbols.frames) return;
    const Sprite s = s_symbols.frame(static_cast<int>(symbol));
    for (int yy = 0; yy < s.height; ++yy)
        for (int xx = 0; xx < s.width; ++xx)
            if (s.pixels[yy * s.width + xx] == 1) {
                const int px = x + xx, py = y + yy;
                if (px >= 0 && py >= 0 && px < c.width() && py < c.height()) c.row(py)[px] = color;
            }
}

// --- Kurzmeldungen ---

bool ToastQueue::push(const Toast& t, uint32_t now) {
    if (t.kind == ToastKind::World) {
        if (world_last_hour(now) >= world_limit_) {
            ++dropped_;
            return false;
        }
    }
    if (count_ >= kCapacity) {
        // voll: die älteste wartende Weltmeldung weicht einer Systemmeldung, sonst verwerfen
        int victim = -1;
        for (int k = showing_ ? 1 : 0; k < count_; ++k)
            if (items_[(head_ + k) % kCapacity].kind == ToastKind::World) {
                victim = k;
                break;
            }
        if (t.kind == ToastKind::World || victim < 0) {
            ++dropped_;
            return false;
        }
        for (int k = victim; k < count_ - 1; ++k) items_[(head_ + k) % kCapacity] = items_[(head_ + k + 1) % kCapacity];
        --count_;
    }
    if (t.kind == ToastKind::World) {
        world_at_[world_n_ % kWorldLog] = now;
        ++world_n_;
    }
    // System vorn (hinter der sichtbaren), Welt hinten anstellen
    int pos = count_;
    if (t.kind == ToastKind::System) {
        pos = showing_ ? 1 : 0;
        while (pos < count_ && items_[(head_ + pos) % kCapacity].kind == ToastKind::System) ++pos;
        for (int k = count_; k > pos; --k) items_[(head_ + k) % kCapacity] = items_[(head_ + k - 1) % kCapacity];
    }
    items_[(head_ + pos) % kCapacity] = t;
    ++count_;
    // gleiche Systemmeldung sichtbar (z. B. erneut „Verbunden“): ablösen, statt zu warten
    if (showing_ && t.kind == ToastKind::System && items_[head_].kind == ToastKind::System) shown_at_ = now - 600000;
    return true;
}

const Toast* ToastQueue::update(uint32_t now) {
    if (showing_) {
        const uint32_t age = now - shown_at_;
        const bool expired = age >= items_[head_].seconds * 1000u;
        const bool replaced = count_ > 1 && items_[(head_ + 1) % kCapacity].kind == ToastKind::System && age >= kMinShowMs;
        if (expired || replaced) {
            head_ = (head_ + 1) % kCapacity;
            --count_;
            showing_ = false;
            ++version_;
        }
    }
    if (!showing_ && count_ > 0) {
        showing_ = true;
        shown_at_ = now;
        ++version_;
    }
    return current();
}

int ToastQueue::world_last_hour(uint32_t now) const {
    int n = 0;
    for (int k = 0; k < std::min(world_n_, kWorldLog); ++k)
        if (now - world_at_[k] < 3600u * 1000u) ++n;
    return n;
}

void ToastQueue::clear() {
    head_ = count_ = 0;
    showing_ = false;
    ++version_;
}

void draw_toast(Canvas& c, const Toast& t, ToastPlace place) {
    if (place == ToastPlace::Bottom) {
        draw_toast(c, t.text, t.tone);
        return;
    }
    // oben: schmale Leiste mit Punkt in der Farbe des Tons, eine Zeile (seit 0.5.7 bis drei, getrennt durch \n)
    constexpr int kMaxText = kW - 120, kLines = 3, kLineH = 10;
    std::string lines[kLines];
    int n = 0, tw = 0;
    for (size_t pos = 0; pos <= t.text.size() && n < kLines;) {
        size_t end = t.text.find('\n', pos);
        if (end == std::string::npos) end = t.text.size();
        lines[n] = font::ellipsize(std::string_view(t.text).substr(pos, end - pos), kMaxText);
        tw = std::max(tw, font::text_width(lines[n++]));
        pos = end + 1;
    }
    const int w = tw + 26, h = 15 + (n - 1) * kLineH;
    const int x = (kW - w) / 2, y = 5;
    round_rect(c, x - 1, y + 1, w + 2, h + 1, kShadow, 4);
    framed(c, x, y, w, h, kSurface, kOutline, 4);
    round_rect(c, x + 7, y + 5, 5, 5, tone_color(t.tone), 1);
    for (int i = 0; i < n; ++i)
        font::draw(c, x + 17, y + 4 - font::kCapTop + i * kLineH, lines[i], i ? kTextDim : kText);
}

// --- Uhr ---

namespace {
void clock_text(char* buf, size_t n, int h, int m, int s, bool seconds) {
    if (seconds) std::snprintf(buf, n, "%02d:%02d:%02d", h, m, s);
    else std::snprintf(buf, n, "%02d:%02d", h, m);
}
}  // namespace

Rect clock_rect(const ClockStyle& st) {
    char buf[16];
    clock_text(buf, sizeof(buf), 88, 88, 88, st.seconds);  // breiteste Ziffern
    const int sym = st.symbol != Symbol::None && s_symbols.pixels ? 12 : 0;
    const int w = font::text_width(buf, 1, st.face) + 10 + sym, h = font::cap_height(st.face) + 8;
    const bool right = st.corner == Corner::TopRight || st.corner == Corner::BottomRight;
    const bool top = st.corner == Corner::TopRight || st.corner == Corner::TopLeft;
    return {right ? kW - w - 5 : 5, top ? 5 : kH - h - 5, w, h};
}

Rect draw_clock(Canvas& c, int hour, int minute, int second, const ClockStyle& st, bool colon) {
    const Rect r = clock_rect(st);
    char buf[16];
    clock_text(buf, sizeof(buf), hour, minute, second, st.seconds);
    round_rect(c, r.x - 1, r.y + 1, r.w + 2, r.h + 1, kShadow, 3);
    framed(c, r.x, r.y, r.w, r.h, st.dim ? kBg : kSurface, st.dim ? kSurfaceHigh : kOutline, 3);
    const uint8_t ink = st.dim ? kOutline : kText;
    if (st.symbol != Symbol::None && s_symbols.pixels)
        draw_symbol(c, r.x + 5, r.y + (r.h - 9) / 2, st.symbol, st.dim ? kOutline : st.symbol_color);
    // Ziffern einzeln, damit der blinkende Doppelpunkt nichts verschiebt
    const int y = r.y + 4 - font::cap_top(st.face);
    int x = r.x + r.w - 5 - font::text_width(buf, 1, st.face);
    for (const char* p = buf; *p; ++p) {
        const char ch[2] = {*p, 0};
        if (*p != ':' || colon || st.seconds) font::draw(c, x, y, ch, ink, 1, st.face);
        x += font::text_width(ch, 1, st.face) + 1;
    }
    return r;
}

void draw_place_label(Canvas& c, std::string_view title) {
    const int w = font::text_width(title) + 14, h = 15;
    round_rect(c, 4, 6, w + 2, h + 1, kShadow, 3);
    framed(c, 5, 5, w, h, kSurface, kOutline, 3);
    font::draw(c, 12, 5 + 4 - font::kCapTop, title, kText);
}

// --- Karte ---

Rect card_rect(const Card& card, const CardStyle& st) {
    switch (st.layout) {
        case CardLayout::Center: return {50, 66, 300, 104};
        case CardLayout::Bar: return {6, kH - 82, kW - 12, 76};
        default: break;
    }
    (void)card;
    return {kSideX, kSideY, kSideW, kSideH};
}

void render_card(Canvas& c, const Card& card, const CardStyle& st) {
    const Rect r = card_rect(card, st);
    const bool exhibit = card.kind == Card::Kind::Exhibit;
    const int pw = card.picture.sprite.width * std::max(1, card.picture.scale);
    const int ph = card.picture.sprite.height * std::max(1, card.picture.scale);
    if (st.layout == CardLayout::Center) {
        // Welt abdunkeln (Raster), Karte in der Mitte
        for (int y = 0; y < kH; ++y) {
            uint8_t* row = c.row(y);
            for (int x = y & 1; x < kW; x += 2) row[x] = kShadow;
        }
    } else {
        mark(c, card);
    }
    shadowed(c, r.x, r.y, r.w, r.h, 5);
    const int pad = 8;
    if (st.layout == CardLayout::Side) {
        int y = r.y + pad;
        int tx = r.x + pad;
        if (card.picture.sprite.pixels) {
            if (exhibit || pw > 56) {
                // großes Bild (Museum): oben über die ganze Breite
                const int bh = std::min(ph + 8, 96);
                picture_box(c, card.picture, r.x + pad, y, r.w - 2 * pad, bh);
                y += bh + 6;
            } else {
                const int bw = std::max(pw + 8, 40), bh = std::max(ph + 6, 40);
                picture_box(c, card.picture, r.x + pad, y, bw, bh);
                tx += bw + 6;
                title(c, tx, y + 2, r.x + r.w - pad - tx, card, st);
                info_lines(c, tx, y + 2 + title_h(st) + 2, r.x + r.w - pad - tx, y + bh + 2, card);
                y += bh + 6;
                tx = r.x + pad;
                c.fill_rect(r.x + 1, y - 3, r.w - 2, 1, kOutline);
                body(c, tx, y + 2, r.w - 2 * pad, r.y + r.h - 32, card, st);
                footer(c, card, st, r);
                return;
            }
        }
        title(c, tx, y, r.w - 2 * pad, card, st);
        y = info_lines(c, tx, y + title_h(st), r.w - 2 * pad, r.y + r.h - 40, card);
        c.fill_rect(r.x + 1, y + 2, r.w - 2, 1, kOutline);
        body(c, tx, y + 7, r.w - 2 * pad, r.y + r.h - 32, card, st);
        footer(c, card, st, r);
        return;
    }
    // Mitte und Leiste: Bild links, Text rechts
    const int bh = st.layout == CardLayout::Bar ? r.h - 2 * 6 : std::min(std::max(ph + 6, 40), r.h - 2 * pad);
    const int bw = std::max(pw + 8, 40);
    const int top = st.layout == CardLayout::Bar ? r.y + 6 : r.y + pad;
    int tx = r.x + pad;
    if (card.picture.sprite.pixels) {
        picture_box(c, card.picture, tx, top, bw, bh);
        tx += bw + 8;
    }
    const int tw = r.x + r.w - pad - tx;
    if (st.layout == CardLayout::Bar) {
        // eine Zeile: Titel · Tätigkeit, darunter der Gedanke, Hinweis rechts unten
        title(c, tx, top + 1, tw / 2, card, st);
        const int nx = tx + std::min(font::text_width(card.title, st.title_scale, st.title), tw / 2) + 8;
        font::draw(c, nx, top + 1 + (title_h(st) - font::kLineHeight) / 2, font::ellipsize(card.subtitle, r.x + r.w - pad - nx), kTextDim);
        body(c, tx, top + title_h(st) + 4, tw, r.y + r.h - 18, card, st);
        hint(c, 0, r.y + r.h - 15, card, st, true, r.x + r.w - pad);
        return;
    }
    title(c, tx, top + 1, tw, card, st);
    int y = info_lines(c, tx, top + 1 + title_h(st), tw, r.y + r.h - 30, card);
    c.fill_rect(tx, y + 2, tw, 1, kOutline);
    body(c, tx, y + 7, tw, r.y + r.h - 20, card, st);
    hint(c, 0, r.y + r.h - 15, card, st, true, r.x + r.w - pad);
}

void store_card(const Ledger& l, int store, int slots, Card* card) {
    card->kind = Card::Kind::Building;
    card->title = l.store_title(store);
    const StoreKind kind = l.store_kind(store);
    uint8_t g[Card::kMaxLines];
    uint16_t q[Card::kMaxLines];
    const int n = l.contents(store, g, q, Card::kMaxLines);
    char buf[64];
    if (n >= 2) {
        // mehrere Waren: kurze Liste, je Ware Stand im Dorf in einem Wort oder was davon reserviert ist
        std::snprintf(buf, sizeof(buf), "%s · %d/%d", store_kind_title(kind), l.load(store), l.capacity(store));
        card->subtitle = buf;
        static constexpr const char* kLevel[] = {"", " · Notbestand", " · knapp", " · gut", " · reichlich"};
        for (int k = 0; k < n; ++k) {
            const int res = l.reserved(store, g[k]);
            if (res > 0)
                std::snprintf(buf, sizeof(buf), "%s %u · davon %d reserviert", l.good(g[k]).title, q[k], res);
            else
                std::snprintf(buf, sizeof(buf), "%s %u%s", l.good(g[k]).title, q[k],
                              kLevel[static_cast<int>(l.level(g[k]))]);
            card->add_line(buf);
        }
        return;
    }
    // eine Ware in Zeilen; leer: die Ware aus der ersten Regel
    card->subtitle = store_kind_title(kind);
    int good = n == 1 ? g[0] : -1;
    if (const StoreDef* d = l.store_def(store); good < 0 && d && d->rule_count > 0) good = l.find_good(d->rules[0].good);
    if (good < 0) {
        card->add_line("Leer");
        return;
    }
    const int qty = l.qty(store, good);
    std::snprintf(buf, sizeof(buf), "%s: %d von %d", l.good(good).title, qty, l.capacity(store));
    card->add_line(buf);
    const int h = l.reach_hours(store, good);
    if (qty == 0)
        card->add_line("Leer");
    else if (h < 0)
        card->add_line("Gerade kein Verbrauch");
    else if (h < 1)
        card->add_line("Reicht keine Stunde mehr");
    else if (h < 24)
        card->add_line((std::snprintf(buf, sizeof(buf), "Reicht noch etwa %d Std.", h), buf));
    else if (h < 48)
        card->add_line("Reicht noch 1 Tag");
    else
        card->add_line((std::snprintf(buf, sizeof(buf), "Reicht noch %d Tage", h / 24), buf));
    // heute dazu und weg, je eine Zeile (eine passt nicht in die Breite der Karte)
    std::snprintf(buf, sizeof(buf), "Heute %s: %d", kind == StoreKind::Producer ? "gemacht" : "gebracht",
                  l.in_today(store, good));
    card->add_line(buf);
    std::snprintf(buf, sizeof(buf), "Heute %s: %d", kind == StoreKind::House ? "verbraucht" : "geholt",
                  l.out_today(store, good));
    card->add_line(buf);
    if (const int res = l.reserved(store, good); res > 0)
        card->add_line((std::snprintf(buf, sizeof(buf), "%d davon reserviert", res), buf));
    if (const int back = qty - slots; slots > 0 && back > 0)
        card->add_line((std::snprintf(buf, sizeof(buf), back == 1 ? "1 liegt hinten" : "%d liegen hinten", back), buf));
}

void project_card(const ProjectList& pl, const Project& p, const Ledger& l, const char* worker, Card* card) {
    const ProjectDef& d = pl.def(p.def);
    card->kind = Card::Kind::Building;
    card->title = d.title;
    char buf[64];
    const int steps = std::min<int>(d.step_count, Card::kMaxSteps);
    std::snprintf(buf, sizeof(buf), "Im Bau · Schritt %d von %d", std::min<int>(p.step + 1, d.step_count),
                  d.step_count);
    card->subtitle = buf;
    card->step_count = 0;
    for (int i = 0; i < steps; ++i) {
        const ProjectStepDef& s = d.steps[i];
        CardStep& cs = card->steps[card->step_count++];
        cs.title = s.label ? s.label : s.title;
        cs.state = i < p.step ? 2 : i == p.step ? 1 : 0;
        cs.detail_count = 0;
        if (i != p.step) continue;
        for (int k = 0; k < s.need_count && k < Project::kMaxNeeds && cs.detail_count < CardStep::kMaxDetails; ++k) {
            const int g = l.find_good(s.needs[k].good);
            std::snprintf(buf, sizeof(buf), "%s %u / %u", g >= 0 ? l.good(g).title : s.needs[k].good,
                          static_cast<unsigned>(p.used[k]), static_cast<unsigned>(s.needs[k].qty));
            cs.detail[cs.detail_count++] = buf;
        }
    }
    // Warten in einem Satz, sonst wer arbeitet; dazu die Arbeit bis zum Ende des Projekts (eine Person)
    pl.wait_text(p, l, buf, sizeof(buf));
    if (buf[0]) {
        if (buf[0] >= 'a' && buf[0] <= 'z') buf[0] = static_cast<char>(buf[0] - 'a' + 'A');
        card->add_line(buf);
    } else if (worker && worker[0]) {
        card->add_line(std::string(worker) + " arbeitet");
    }
    uint32_t left = 0;
    for (int i = p.step; i < d.step_count; ++i) left += d.steps[i].minutes * 60u;
    left -= std::min(left, p.work_s);
    const unsigned mins = (left + 59) / 60;
    if (mins == 0) return;
    if (mins < 60)
        std::snprintf(buf, sizeof(buf), "Noch etwa %u Min. Arbeit", std::max(5u, (mins + 4) / 5 * 5));
    else
        std::snprintf(buf, sizeof(buf), "Noch etwa %u Std. Arbeit", (mins + 30) / 60);
    card->add_line(buf);
}

}  // namespace pixelia::ui
