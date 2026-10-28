#pragma once
// Einblendungen über der Welt (seit 0.5.6): Kurzmeldungen mit Warteschlange und Obergrenze, Pixel-Uhr, Karte mit
// Bild und Text (Figur, Gebäude „wer ist drin“, später Museum). Plattformunabhängig wie system_ui.h, gezeichnet in
// 400 × 240 mit den Farben der System-Oberfläche (240–255); das Bild einer Karte in den Farben des sichtbaren Ortes.
// Schriften: Systemschrift, fett und klein aus dem Paket „system“ (font.h), Symbole ebenfalls von dort.

#include <cstdint>
#include <string>
#include <string_view>

#include "pixelia/engine/canvas.h"
#include "pixelia/engine/font.h"
#include "pixelia/engine/system_ui.h"

namespace pixelia {
class AssetStore;
}

namespace pixelia::ui {

// --- Symbole (Paket „system“, Bild „ui/symbole“, 9 × 9, einfarbig) ---
enum class Symbol : uint8_t { Thought, Follow, House, Find, New, Place, Spring, Summer, Autumn, Winter, None = 255 };
bool load_symbols(AssetStore& store);  // false = Paket fehlt (dann ohne Symbole)
void draw_symbol(Canvas& canvas, int x, int y, Symbol symbol, uint8_t color);

// --- Kurzmeldungen ---
// System: Meldungen der Technik (WLAN, PIN, Update), immer und vorn in der Warteschlange.
// World: Meldungen aus der Welt (später der Regisseur), höchstens world_limit pro Stunde (SETTING Kapitel 13).
enum class ToastKind : uint8_t { System, World };
enum class ToastPlace : uint8_t { Top, Bottom };

struct Toast {
    std::string text;
    Tone tone = Tone::Info;
    uint16_t seconds = 6;
    ToastKind kind = ToastKind::System;
    Symbol symbol = Symbol::None;
};

class ToastQueue {
public:
    static constexpr int kCapacity = 6;
    static constexpr int kDefaultWorldLimit = 4;  // je Stunde
    static constexpr uint32_t kMinShowMs = 2000;  // so lange bleibt eine Meldung mindestens, bevor eine neue sie ablöst

    void set_world_limit(int per_hour) { world_limit_ = per_hour < 0 ? 0 : per_hour; }
    int world_limit() const { return world_limit_; }
    // false = verworfen (Obergrenze der Welt erreicht, Warteschlange voll)
    bool push(const Toast& toast, uint32_t now_ms);
    // Zeit fortschreiben; gibt die sichtbare Meldung zurück (nullptr = keine)
    const Toast* update(uint32_t now_ms);
    const Toast* current() const { return showing_ ? &items_[head_] : nullptr; }
    uint32_t shown_since() const { return shown_at_; }
    int waiting() const { return showing_ ? count_ - 1 : count_; }
    int world_last_hour(uint32_t now_ms) const;
    uint32_t dropped() const { return dropped_; }
    uint32_t version() const { return version_; }  // ändert sich, wenn eine andere Meldung sichtbar wird
    void clear();

private:
    Toast items_[kCapacity];
    int head_ = 0, count_ = 0;
    bool showing_ = false;
    uint32_t shown_at_ = 0;
    int world_limit_ = kDefaultWorldLimit;
    static constexpr int kWorldLog = 16;
    uint32_t world_at_[kWorldLog] = {};
    int world_n_ = 0;
    uint32_t dropped_ = 0;
    uint32_t version_ = 0;
};

// Kurzmeldung zeichnen (unten wie bis 0.5.5 oder oben als schmale Leiste; oben seit 0.5.7 bis drei Zeilen, getrennt\n// durch \\n, ab der zweiten gedämpft)
void draw_toast(Canvas& canvas, const Toast& toast, ToastPlace place);

// --- Pixel-Uhr ---
enum class Corner : uint8_t { TopRight, TopLeft, BottomRight, BottomLeft };
struct ClockStyle {
    bool on = true;
    Corner corner = Corner::TopRight;
    bool seconds = false;
    font::Style face = font::Style::Bold;
    bool dim = false;  // nachts gedimmt (Wunsch des Users 2026-10-22: 22–6 Uhr)
    // Symbol links neben der Uhrzeit (seit 0.5.7: Jahreszeit, Wunsch des Users 2026-10-26), None = keins
    Symbol symbol = Symbol::None;
    uint8_t symbol_color = kText;
};
// Ortsname nach einem Wechsel (oben links, kurz eingeblendet)
void draw_place_label(Canvas& canvas, std::string_view title);
// colon: Doppelpunkt sichtbar (ohne Sekunden blinkt er im Sekundentakt). Gibt die belegte Fläche zurück.
Rect clock_rect(const ClockStyle& style);
Rect draw_clock(Canvas& canvas, int hour, int minute, int second, const ClockStyle& style, bool colon);

// --- Karte ---
enum class CardLayout : uint8_t { Side, Center, Bar };
struct CardStyle {
    CardLayout layout = CardLayout::Side;
    font::Style title = font::Style::Bold;
    int title_scale = 1;
    font::Style small = font::Style::Small;  // Hinweise (Entscheidung des Users 2026-10-22: klein 3×5)
};

struct CardPicture {
    Sprite sprite{};
    const uint8_t* remap = nullptr;  // Farbreihe (Figuren)
    bool flip = false;
    int scale = 2;
    // Lichtstufe (light.h): Grundfarben unter light_base um light_add verschieben, z. B. nachts auf die hellste Stufe
    uint8_t light_base = 0;
    uint8_t light_add = 0;
};

// Anzeige einer Stufe auf der Karte (seit 0.5.9: Fähigkeiten einer Figur), Stufe 1–3 und Fortschritt zur nächsten
struct CardMeter {
    std::string label;
    uint8_t level = 0;     // 0–3
    uint8_t progress = 0;  // 0–255 zur nächsten Stufe (bei 3: voll)
};

struct Card {
    enum class Kind : uint8_t { None, Figure, Building, Exhibit };
    static constexpr int kMaxLines = 4;
    static constexpr int kMaxMeters = 3;
    Kind kind = Kind::None;
    std::string title;
    std::string subtitle;  // was die Figur gerade tut (Teile: Zustand, z. B. „im Bau“)
    std::string where;     // Ort bzw. Fundort
    // Symbole vor Tätigkeit und Ort: ohne (Entscheidung des Users 2026-10-22), Feld bleibt für später
    Symbol subtitle_symbol = Symbol::None;
    Symbol where_symbol = Symbol::None;
    std::string quote;     // Gedanke (Figur, ab 0.6.x aus der Simulation) bzw. Beschreibung (Museum)
    std::string hint;      // z. B. „Halten: Folgen“
    std::string lines[kMaxLines];  // Angaben, z. B. „Drin: …“
    int line_count = 0;
    CardMeter meters[kMaxMeters];  // unter den Angaben, je eine Zeile (so viele, wie Platz haben)
    int meter_count = 0;
    Symbol hint_symbol = Symbol::None;
    CardPicture picture;
    int mark_x = -1, mark_y = -1, mark_h = 0;  // Markierung in der Welt (fein): Fußpunkt und Höhe, −1 = keine
    uint16_t figure = 0;                         // Figur der Karte (Folgen), 0 = keine
    const char* place = nullptr;                 // Teil führt in diesen Ort (Tür): die Bühne ergänzt, wer drin ist
    void add_line(std::string_view text) {
        if (line_count < kMaxLines) lines[line_count++] = std::string(text);
    }
};
void render_card(Canvas& canvas, const Card& card, const CardStyle& style);
// Fläche der Karte (Tippen auf die Karte schließt sie nicht, sondern bleibt bei ihr)
Rect card_rect(const Card& card, const CardStyle& style);
// Knopf unten auf der Seitenkarte (seit 0.5.6): „Folgen“ bei einer Figur, „Betreten“ bei einer Tür; w = 0: keiner
Rect card_button(const Card& card, const CardStyle& style);

}  // namespace pixelia::ui
