// Selbsttests der Engine-Bausteine ohne Hardware (--test), laufen auch in der CI.
// gestures: Gestenerkennung (Tippen, langer Druck) mit nachgestellten Touch-Abläufen wie vom GT911.
// redact: Passwort-Filter der Diagnose-Ausgaben (Blockgrenzen, JSON-Schreibweise, mehrere Geheimnisse).
// stage: Bühne und Figuren-Bausteine (Determinismus, Rechnen ohne Zeichnen, Übergänge, Farbreihen, Animationen).

#include <cstring>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "pixelia/engine/assets.h"
#include "pixelia/engine/light.h"
#include "pixelia/engine/color.h"
#include "pixelia/engine/camera.h"
#include "pixelia/engine/gesture.h"
#include "pixelia/engine/overlay.h"
#include "pixelia/engine/png.h"
#include "pixelia/engine/redact.h"
#include "pixelia/engine/save.h"
#include "pixelia/engine/sprite.h"
#include "pixelia/engine/stage.h"
#include "pixelia/engine/place.h"
#include "testdorf/test_place.h"
#include "testdorf/testdorf.h"
#include "sim.h"

namespace sim {

namespace {

using pixelia::gesture::Config;
using pixelia::gesture::Event;
using pixelia::gesture::Recognizer;
using pixelia::gesture::Type;

std::vector<Event> drain(Recognizer& r) {
    std::vector<Event> out;
    Event e;
    while (r.poll(&e)) out.push_back(e);
    return out;
}

// Finger von t0 bis t1 halten, alle step ms ein Messwert (GT911-Abfrage alle 20 ms); dx/dy = Zittern pro Messwert
void hold(Recognizer& r, int x, int y, uint32_t t0, uint32_t t1, uint32_t step = 20, int jitter = 0, int fingers = 1) {
    int k = 0;
    for (uint32_t t = t0; t <= t1; t += step, ++k) {
        int j = jitter ? ((k % 2) ? jitter : -jitter) : 0;
        r.touch(fingers, x + j, y - j, t);
    }
}

struct Case {
    const char* name;
    std::function<std::string()> run;  // leer = bestanden, sonst Fehlerbeschreibung
};

std::string expect(const std::vector<Event>& ev, std::initializer_list<Type> types) {
    std::vector<Type> want(types);
    if (ev.size() != want.size()) return "erwartet " + std::to_string(want.size()) + " Ereignis(se), bekommen " +
                                         std::to_string(ev.size());
    for (size_t i = 0; i < want.size(); ++i)
        if (ev[i].type != want[i]) return "Ereignis " + std::to_string(i) + " hat den falschen Typ";
    return {};
}

// Finger in n Schritten von (x0, y0) nach (x1, y1) ziehen und loslassen; rot = Bild um 180° gedreht (Touch wird wie
// in hwtest.cpp mitgedreht, Koordinaten danach in Bildausrichtung)
void drag(Recognizer& r, int x0, int y0, int x1, int y1, uint32_t t0, uint32_t ms, bool rot = false, int fingers = 1) {
    const int n = std::max<int>(2, static_cast<int>(ms / 20));
    int x = x0, y = y0;
    for (int k = 0; k <= n; ++k) {
        x = x0 + (x1 - x0) * k / n;
        y = y0 + (y1 - y0) * k / n;
        int px = x, py = y;
        if (rot) pixelia::gesture::rotate180(&px, &py, 800, 480);
        r.touch(fingers, px, py, t0 + static_cast<uint32_t>(k) * ms / n);
    }
    if (rot) pixelia::gesture::rotate180(&x, &y, 800, 480);
    r.touch(0, x, y, t0 + ms + 20);
}

std::vector<Case> gesture_cases() {
    return {
        {"Wischen nach links ergibt Swipe mit dx < 0",
         [] {
             Recognizer r;
             drag(r, 600, 240, 300, 250, 1000, 300);
             auto ev = drain(r);
             std::string err = expect(ev, {Type::Swipe});
             if (err.empty() && (ev[0].dx >= -250 || std::abs(ev[0].dy) > 20)) err = "Weg falsch";
             return err;
         }},
        {"Wischen mit 180° gedrehtem Bild: Richtung in Bildausrichtung",
         [] {
             // Physisch von links nach rechts gewischt = im gedrehten Bild nach links
             Recognizer r;
             drag(r, 200, 240, 500, 240, 1000, 300, true);
             auto ev = drain(r);
             std::string err = expect(ev, {Type::Swipe});
             if (err.empty() && ev[0].dx >= -250) err = "Richtung nicht gedreht";
             if (err.empty() && (ev[0].x != 599 || ev[0].y != 239)) err = "Aufsetzpunkt nicht gedreht";
             return err;
         }},
        {"Tippen und langer Druck mit 180° gedrehtem Bild am gespiegelten Ort",
         [] {
             Recognizer r;
             int x = 100, y = 50;
             pixelia::gesture::rotate180(&x, &y, 800, 480);
             hold(r, x, y, 1000, 1180);
             r.touch(0, x, y, 1200);
             hold(r, x, y, 2000, 5100);
             r.touch(0, x, y, 5120);
             auto ev = drain(r);
             std::string err = expect(ev, {Type::Tap, Type::LongPress});
             if (err.empty() && (ev[0].x != 699 || ev[0].y != 429 || ev[1].x != 699)) err = "Ort nicht gespiegelt";
             return err;
         }},
        {"Langsames Ziehen, schräges Wischen, zwei Finger und kurzer Weg sind kein Wischen",
         [] {
             Recognizer r;
             drag(r, 600, 240, 300, 240, 1000, 1500);       // zu langsam
             drag(r, 600, 100, 400, 300, 5000, 300);        // schräg
             drag(r, 600, 240, 300, 240, 9000, 300, false, 2);  // zwei Finger
             drag(r, 600, 240, 520, 240, 13000, 200);       // nur 80 Pixel
             auto ev = drain(r);
             return ev.empty() ? std::string() : "Ereignis " + std::to_string(static_cast<int>(ev[0].type)) + " erkannt";
         }},
        {"Kurzes Tippen ergibt Tap",
         [] {
             Recognizer r;
             hold(r, 400, 240, 1000, 1180);
             r.touch(0, 400, 240, 1200);
             auto ev = drain(r);
             std::string err = expect(ev, {Type::Tap});
             if (err.empty() && (ev[0].x != 400 || ev[0].y != 240)) err = "Ort falsch";
             return err;
         }},
        {"3 s still halten ergibt genau einen langen Druck, danach kein Tap",
         [] {
             Recognizer r;
             hold(r, 100, 100, 0, 2980);
             if (!drain(r).empty()) return std::string("Ereignis vor 3 s");
             hold(r, 100, 100, 3000, 4500);
             r.touch(0, 100, 100, 4520);
             auto ev = drain(r);
             std::string err = expect(ev, {Type::LongPress});
             if (err.empty() && ev[0].duration_ms != 3000) err = "Dauer " + std::to_string(ev[0].duration_ms) + " ms";
             return err;
         }},
        {"Langer Druck auch ohne neue Messwerte (nur Zeit)",
         [] {
             Recognizer r;
             r.touch(1, 50, 60, 0);
             for (uint32_t t = 20; t <= 3000; t += 20) r.tick(t);
             return expect(drain(r), {Type::LongPress});
         }},
        {"Zittern bis 10 px bricht den langen Druck nicht ab",
         [] {
             Recognizer r;
             hold(r, 300, 200, 0, 3100, 20, 10);
             return expect(drain(r), {Type::LongPress});
         }},
        {"Bewegung über 24 px: kein langer Druck, kein Tap (frei für Wischen)",
         [] {
             Recognizer r;
             hold(r, 300, 200, 0, 400);
             hold(r, 340, 200, 420, 4000);
             r.touch(0, 340, 200, 4020);
             if (r.hold_progress(4020) >= 0) return std::string("Halte-Anzeige nach Bewegung noch aktiv");
             return expect(drain(r), {});
         }},
        {"Zwei Finger: keine Geste",
         [] {
             Recognizer r;
             hold(r, 300, 200, 0, 3500, 20, 0, 2);
             r.touch(0, 300, 200, 3520);
             return expect(drain(r), {});
         }},
        {"Zu lang für Tap, zu kurz für langen Druck: nichts",
         [] {
             Recognizer r;
             hold(r, 300, 200, 0, 1500);
             r.touch(0, 300, 200, 1520);
             return expect(drain(r), {});
         }},
        {"Fortschritt der Halte-Anzeige",
         [] {
             Recognizer r;
             if (r.hold_progress(0) >= 0) return std::string("Anzeige ohne Finger");
             r.touch(1, 10, 10, 1000);
             float p = r.hold_progress(2500);
             if (std::fabs(p - 0.5f) > 0.01f) return "bei 1,5 s " + std::to_string(p) + " statt 0,5";
             r.tick(4000);
             if (r.hold_progress(4000) >= 0) return std::string("Anzeige nach dem langen Druck noch aktiv");
             return std::string();
         }},
        {"Einstellbare Dauer (1 s)",
         [] {
             Recognizer r;
             Config c;
             c.long_press_ms = 1000;
             r.set_config(c);
             hold(r, 10, 10, 0, 980);
             if (!drain(r).empty()) return std::string("zu früh");
             hold(r, 10, 10, 1000, 1100);
             return expect(drain(r), {Type::LongPress});
         }},
        {"Nach langem Druck funktioniert Tippen wieder (Karte schließen)",
         [] {
             Recognizer r;
             hold(r, 10, 10, 0, 3100);
             r.touch(0, 10, 10, 3120);
             hold(r, 600, 400, 5000, 5100);
             r.touch(0, 600, 400, 5120);
             return expect(drain(r), {Type::LongPress, Type::Tap});
         }},
        {"Zeitüberlauf der ms-Uhr (nach 49 Tagen)",
         [] {
             Recognizer r;
             const uint32_t t0 = 0xFFFFFFFFu - 1000;
             r.touch(1, 10, 10, t0);
             r.tick(t0 + 3000);  // läuft über
             return expect(drain(r), {Type::LongPress});
         }},
    };
}

// Text in Stücken der Größe chunk durch einen SecretFilter schicken
struct Filtered {
    std::string out;
    size_t hits = 0;
    size_t secrets = 0;
};
Filtered filter(const std::string& text, const std::vector<std::string>& secrets, size_t chunk, size_t block = 64) {
    Filtered f;
    pixelia::SecretFilter sf([&](const char* d, size_t n) { f.out.append(d, n); return true; }, block);
    for (const auto& s : secrets) sf.add_secret(s.c_str());
    for (size_t i = 0; i < text.size(); i += chunk) sf.write(text.data() + i, std::min(chunk, text.size() - i));
    sf.finish();
    f.hits = sf.hits();
    f.secrets = sf.secrets();
    return f;
}

std::string expect_text(const Filtered& f, const std::string& want, size_t hits) {
    if (f.out != want) return "Ausgabe falsch: \"" + f.out.substr(0, 120) + "\"";
    if (f.hits != hits) return "erwartet " + std::to_string(hits) + " Ersetzung(en), gezählt " + std::to_string(f.hits);
    return {};
}

std::vector<Case> redact_cases() {
    return {
        {"Geheimnis zweimal in einem Block",
         [] {
             auto f = filter("a GeheimPasswort b GeheimPasswort c", {"GeheimPasswort"}, 1000, 4096);
             return expect_text(f, "a ************** b ************** c", 2);
         }},
        {"Geheimnis über Blockgrenzen (Zeichen für Zeichen)",
         [] {
             std::string text(200, 'x');
             text.replace(60, 10, "Sommer2026");
             std::string want(200, 'x');
             want.replace(60, 10, "**********");
             return expect_text(filter(text, {"Sommer2026"}, 1, 64), want, 1);
         }},
        {"Geheimnis über mehreren Blockgrenzen (Stücke zu 7 Zeichen)",
         [] {
             std::string text(500, '-'), want(500, '-');
             for (size_t at : {58u, 127u, 190u, 250u, 492u}) {
                 text.replace(at, 8, "pw123456");
                 want.replace(at, 8, "********");
             }
             return expect_text(filter(text, {"pw123456"}, 7, 64), want, 5);
         }},
        {"JSON-Schreibweise mit Anführungszeichen und Backslash",
         [] {
             auto f = filter(R"({"m":"pa\"ss\\wort9"})", {R"(pa"ss\wort9)"}, 3, 64);
             return expect_text(f, R"({"m":"*************"})", 1);
         }},
        {"Mehrere Geheimnisse, eines Teil des anderen",
         [] {
             auto f = filter("x Haus1234Garten y Haus1234 z", {"Haus1234Garten", "Haus1234"}, 5, 64);
             return expect_text(f, "x ************** y ******** z", 2);
         }},
        {"Kürzeres Geheimnis zuerst eingetragen: längeres trotzdem ganz ersetzt",
         [] {
             auto f = filter("x Haus1234Garten y Haus1234 z", {"Haus1234", "Haus1234Garten"}, 5, 64);
             return expect_text(f, "x ************** y ******** z", 2);
         }},
        {"Leere und zu kurze Geheimnisse werden ignoriert",
         [] {
             auto f = filter("abc ab a", {"", "ab", "abc"}, 2, 64);
             if (f.secrets != 0) return std::string("Geheimnisse gezählt: ") + std::to_string(f.secrets);
             return expect_text(f, "abc ab a", 0);
         }},
        {"Ohne Geheimnisse unverändert (100 KB)",
         [] {
             std::string text;
             for (int i = 0; i < 4000; ++i) text += "I (" + std::to_string(i) + ") log: Zeile ohne Geheimnis\n";
             return expect_text(filter(text, {}, 333, 4096), text, 0);
         }},
        {"Abbruch, wenn das Ziel nichts mehr annimmt",
         [] {
             int calls = 0;
             pixelia::SecretFilter sf([&](const char*, size_t) { return ++calls < 2; }, 64);
             std::string block(100, 'a');
             bool ok1 = sf.write(block.data(), block.size());
             bool ok2 = sf.write(block.data(), block.size());
             bool ok3 = sf.finish();
             if (!ok1 || ok2 || ok3) return std::string("Rückgabewerte falsch");
             return calls == 2 ? std::string() : "Ziel " + std::to_string(calls) + "-mal aufgerufen";
         }},
    };
}

// Bühne: gleiche Schritte = gleiche Bilder, Zeichnen ändert nichts, Übergänge, Sprites, Farbreihen, Animationen
struct StageRun {
    pixelia::Stage stage;
    pixelia::WorldClock clock;
    explicit StageRun(uint64_t seed, const char* start = "scrolltest") {
        Options opt;
        opt.scene = start;
        opt.seed = seed;
        setup_stage(opt, stage);
        stage.set_world_time(false);  // Prüfungen bis 0.5.6: Wetter nur per Befehl (Weltzeit: --test welt)
        clock.advance(0);
        stage.tick(clock);
    }
    void run(int ticks, bool render_each = false) {
        for (int i = 0; i < ticks; ++i) {
            clock.advance(pixelia::kTickUs);
            stage.tick(clock);
            if (render_each) frame();
        }
    }
    // Bild als RGB (so vergleichen sich auch Bilder mit verschiedenen Paletten)
    std::vector<uint8_t> frame() {
        std::vector<uint8_t> idx(pixelia::kScreenPixels), rgb(pixelia::kScreenPixels * 3);
        pixelia::Canvas c(idx.data(), pixelia::kScreenWidth, pixelia::kScreenHeight);
        stage.render(c);
        pixelia::indexed_to_rgb(idx.data(), pixelia::kScreenWidth, pixelia::kScreenHeight, display_palette(stage), 1,
                                false, rgb.data());
        return rgb;
    }
};

// Anteil gleicher Bildpunkte (0 … 1)
double same_share(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
    size_t same = 0;
    for (size_t i = 0; i < a.size(); i += 3) same += a[i] == b[i] && a[i + 1] == b[i + 1] && a[i + 2] == b[i + 2];
    return static_cast<double>(same) * 3 / a.size();
}

std::string percent(double v) { return std::to_string(static_cast<int>(v * 100 + 0.5)) + " %"; }

std::vector<Case> stage_cases() {
    using pixelia::Transition;
    return {
        {"Gleicher Startwert und gleiche Wechsel ergeben dieselben Bilder",
         [] {
             StageRun a(7), b(7);
             for (auto* r : {&a, &b}) {
                 r->run(40);
                 r->stage.go(1, Transition::Dissolve);
                 r->run(9);
             }
             if (a.frame() != b.frame()) return std::string("mitten in der Überblendung verschieden");
             for (auto* r : {&a, &b}) {
                 r->run(300);
                 r->stage.go(2, Transition::Pattern);
                 r->run(400);
             }
             return a.frame() == b.frame() ? std::string() : "nach 750 Schritten verschieden";
         }},
        {"Anderer Startwert ergibt einen anderen Ablauf",
         [] {
             StageRun a(7, "testort-wiese"), b(8, "testort-wiese");
             a.run(200);
             b.run(200);
             return a.frame() != b.frame() ? std::string() : "Bilder gleich";
         }},
        {"Zeichnen ändert den Ablauf nicht (jeden Schritt gezeichnet vs. nur am Ende)",
         [] {
             StageRun a(3), b(3);
             for (int k = 0; k < 3; ++k) {
                 a.stage.go((k + 1) % 3, Transition::Dissolve);
                 b.stage.go((k + 1) % 3, Transition::Dissolve);
                 a.run(250, true);
                 b.run(250, false);
             }
             return a.frame() == b.frame() ? std::string() : "Bilder verschieden";
         }},
        {"Unsichtbare Orte rechnen weiter (Wechsel per Schnitt zeigt denselben Stand)",
         [] {
             StageRun a(5, "testort-raum"), b(5, "scrolltest");
             a.run(901);
             b.run(900);
             b.stage.go(2, Transition::Cut);
             b.run(1);
             return a.frame() == b.frame() ? std::string() : "Raum nach 901 Schritten verschieden";
         }},
        {"Überblendung dauert 18 Schritte (nach einem Schritt Vorbereitung), die Mitte zeigt etwa zur Hälfte den neuen Ort",
         [] {
             StageRun a(9), cut(9);
             a.run(30);
             cut.run(30);
             a.stage.go(1, Transition::Dissolve);
             cut.stage.go(1, Transition::Cut);
             a.run(10);  // seit 0.5.8: erst betreten, im nächsten Schritt mischen (Rechenspitze verteilt)
             cut.run(10);
             if (a.stage.transition() != Transition::Dissolve) return std::string("Paletten nicht zusammengelegt");
             const double mid = same_share(a.frame(), cut.frame());
             if (mid < 0.4 || mid > 0.75) return "Mitte: " + percent(mid) + " wie der neue Ort";
             a.run(8);
             cut.run(8);
             if (!a.stage.transitioning()) return std::string("nach 17 Schritten schon fertig");
             const double late = same_share(a.frame(), cut.frame());
             if (late < 0.85) return "Schritt 17: erst " + percent(late);
             a.run(1);
             cut.run(1);
             if (a.stage.transitioning()) return std::string("nach 18 Schritten nicht fertig");
             return a.frame() == cut.frame() ? std::string() : "nach dem Übergang anders als per Schnitt";
         }},
        {"Raster und Überblendung zwischen zwei groben Orten",
         [] {
             for (Transition t : {Transition::Pattern, Transition::Dissolve}) {
                 StageRun a(4, "testort-wiese"), cut(4, "testort-wiese");
                 a.stage.go(2, t);
                 cut.stage.go(2, Transition::Cut);
                 a.run(9);
                 cut.run(9);
                 const double mid = same_share(a.frame(), cut.frame());
                 if (a.stage.transition() != t) return std::string("Übergang ersetzt");
                 if (mid < 0.4 || mid > 0.75) return std::string(pixelia::transition_name(t)) + ": " + percent(mid);
             }
             return std::string();
         }},
        {"Abblenden: in der Mitte schwarz",
         [] {
             StageRun a(2, "testort-wiese");
             a.stage.go(0, Transition::Fade);
             a.run(9);
             const pixelia::Palette& p = a.stage.palette();
             for (int i = 0; i < pixelia::kSceneColors; ++i)
                 if (p[i].r || p[i].g || p[i].b) return "Farbe " + std::to_string(i) + " nicht schwarz";
             return std::string();
         }},
        {"Verdoppeln 200×120 → 400×240",
         [] {
             std::vector<uint8_t> coarse(pixelia::kCoarsePixels), fine(pixelia::kScreenPixels, 0);
             for (int i = 0; i < pixelia::kCoarsePixels; ++i) coarse[i] = static_cast<uint8_t>(i * 7 + i / 200);
             pixelia::Canvas c(fine.data(), pixelia::kScreenWidth, pixelia::kScreenHeight);
             pixelia::upscale2x(coarse.data(), c);
             for (int y = 0; y < pixelia::kScreenHeight; ++y)
                 for (int x = 0; x < pixelia::kScreenWidth; ++x)
                     if (fine[y * pixelia::kScreenWidth + x] != coarse[(y / 2) * pixelia::kCoarseWidth + x / 2])
                         return "falsch bei " + std::to_string(x) + "/" + std::to_string(y);
             return std::string();
         }},
        {"Farbreihe ersetzt die Rampe, Index 0 bleibt durchsichtig, Spiegeln",
         [] {
             const uint8_t px[4] = {0, 20, 21, 5};
             std::vector<uint8_t> buf(8, 9);
             pixelia::Canvas c(buf.data(), 4, 2);
             pixelia::Recolor r;
             r.ramp(20, 40, 4).set(0, 77);
             c.blit({px, 4, 1}, 0, 0, false, r.table());
             c.blit({px, 4, 1}, 0, 1, true, r.table());
             const std::vector<uint8_t> want = {9, 40, 41, 5, 5, 41, 40, 9};
             return buf == want ? std::string() : std::string("Bildpunkte falsch");
         }},
        {"Animation: Schleife, einmal, hin und her",
         [] {
             using pixelia::AnimFrame;
             using pixelia::AnimMode;
             const AnimFrame f[] = {{0, 2}, {1, 1}, {2, 1}};
             auto seq = [&](AnimMode m) {
                 const pixelia::Animation anim{f, 3, m};
                 pixelia::Animator an;
                 an.play(&anim);
                 std::string s;
                 for (int i = 0; i < 9; ++i) {
                     s += static_cast<char>('0' + an.frame());
                     an.tick();
                 }
                 return s;
             };
             if (seq(AnimMode::Loop) != "001200120") return "Schleife " + seq(AnimMode::Loop);
             if (seq(AnimMode::Once) != "001222222") return "einmal " + seq(AnimMode::Once);
             if (seq(AnimMode::PingPong) != "001210012") return "hin und her " + seq(AnimMode::PingPong);
             return std::string();
         }},
        {"Blickrichtungen: drei Ansichten, links gespiegelt",
         [] {
             using pixelia::Facing;
             const auto l = pixelia::view_of(Facing::Left), r = pixelia::view_of(Facing::Right);
             const bool ok = pixelia::view_of(Facing::Down).index == 0 && pixelia::view_of(Facing::Up).index == 1 &&
                             r.index == 2 && !r.flip && l.index == 2 && l.flip;
             return ok ? std::string() : std::string("Zuordnung falsch");
         }},
        // --- Raum und Baukasten (seit 0.5.1) ---
        {"Kacheln: Legende, Boden deckend, Wand mit Durchsicht, versperrte Kacheln",
         [] {
             uint8_t px[3][pixelia::kTilePixels];
             std::fill(px[0], px[0] + 64, 0);
             std::fill(px[1], px[1] + 64, 5);
             std::fill(px[2], px[2] + 64, 0);
             px[2][0] = 9;  // Wand: nur ein Punkt gezeichnet
             const uint8_t flags[3] = {0, 0, pixelia::kTileBlocks};
             const pixelia::TileSet set{&px[0][0], flags, 3};
             pixelia::TileMap m;
             const char* floor[1] = {"..."};
             const char* wall[1] = {" #"};
             m.load(pixelia::kFloorLayer, floor, 1, " .#");
             m.load(pixelia::kWallLayer, wall, 1, " .#");
             std::vector<uint8_t> buf(pixelia::kCoarsePixels, 1);
             pixelia::Canvas c(buf.data(), pixelia::kCoarseWidth, pixelia::kCoarseHeight);
             m.draw(c, set, pixelia::kFloorLayer, true);
             m.draw(c, set, pixelia::kWallLayer, false);
             if (m.at(0, 0, 0) != 1 || m.at(1, 1, 0) != 2 || m.at(1, 0, 0) != 0) return std::string("Legende falsch");
             if (buf[0] != 5 || buf[8] != 9 || buf[9] != 5 || buf[24] != 1) return std::string("Ebenen falsch gezeichnet");
             if (!m.blocks(set, 1, 0) || m.blocks(set, 0, 0) || !m.blocks(set, -1, 0)) return std::string("Sperren falsch");
             return std::string();
         }},
        {"Tiefe: Figur hinter dem Möbel verdeckt, davor sichtbar (auch eingebacken)",
         [] {
             const uint8_t block[16] = {7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7};
             const uint8_t fig[4] = {3, 3, 3, 3};
             auto run = [&](int fig_y, bool baked) {
                 std::vector<uint8_t> buf(pixelia::kCoarsePixels, 0);
                 pixelia::Canvas c(buf.data(), pixelia::kCoarseWidth, pixelia::kCoarseHeight);
                 pixelia::DepthList l;
                 if (baked) c.blit({block, 4, 4}, 10, 10);  // steht schon im Hintergrund
                 l.add_at({block, 4, 4}, 10, 10, 13, false, nullptr, baked);
                 l.add({fig, 2, 2}, 11, fig_y, 0, 1);  // Fußpunkt (11, fig_y)
                 l.draw(c);
                 return buf[(fig_y - 1) * pixelia::kCoarseWidth + 11];
             };
             for (bool baked : {false, true}) {
                 if (run(12, baked) != 7) return std::string(baked ? "eingebacken: " : "") + "Figur dahinter sichtbar";
                 if (run(14, baked) != 3) return std::string(baked ? "eingebacken: " : "") + "Figur davor verdeckt";
             }
             return std::string();
         }},
        {"Tiefe: Hintergrundbild plus Neuzeichnen ist pixelgleich mit allem jedes Bild (alle Testorte, 40 Figuren)",
         [] {
             for (const char* place : {"testort-wiese", "testort-raum", "testort-platz"}) {
                 StageRun r(11, place);
                 r.clock.set_time_of_day(23, 0);  // nachts: Licht an (Figuren im Lichtkreis, Lichtkarte beim Verdoppeln)
                 r.stage.set_follow(false);
                 r.stage.set_load(40);
                 auto& tp = static_cast<pixelia::TestPlace&>(r.stage.scene(r.stage.active()));
                 if (!tp.room().has_background()) return std::string(place) + ": kein Hintergrundbild";
                 std::vector<uint8_t> a(pixelia::kCoarsePixels), b(pixelia::kCoarsePixels);
                 pixelia::Canvas ca(a.data(), pixelia::kCoarseWidth, pixelia::kCoarseHeight);
                 pixelia::Canvas cb(b.data(), pixelia::kCoarseWidth, pixelia::kCoarseHeight);
                 for (int k = 0; k < 60; ++k) {
                     r.run(7);
                     if (k == 20) tp.command("ausbau");
                     if (k == 40) tp.command("baustelle");
                     tp.render(ca);
                     tp.render_direct(cb);
                     if (a != b) return std::string(place) + ": verschieden nach " + std::to_string(k * 7) + " Schritten";
                     // Schichten (0.5.2): Puffer mit Müll vorbelegt, nur unter den bewegten Teilen kopiert
                     std::vector<uint8_t> w(pixelia::kCoarsePixels, 0xEE), fa(pixelia::kScreenPixels),
                         fb(pixelia::kScreenPixels);
                     pixelia::Canvas cw(w.data(), pixelia::kCoarseWidth, pixelia::kCoarseHeight);
                     pixelia::CoarseLayers layers;
                     tp.render_layers(cw, layers);
                     if (!layers.base) return std::string(place) + ": keine Schichten";
                     pixelia::Canvas cfa(fa.data(), pixelia::kScreenWidth, pixelia::kScreenHeight);
                     pixelia::Canvas cfb(fb.data(), pixelia::kScreenWidth, pixelia::kScreenHeight);
                     pixelia::upscale2x(w.data(), cfa, &layers);
                     pixelia::CoarseLayers flat = layers;  // gleiches Licht, ohne Hintergrund-Schicht
                     flat.base = nullptr;
                     pixelia::upscale2x(b.data(), cfb, &flat);
                     if (!layers.light) return std::string(place) + ": keine Lichtkarte";
                     if (fa != fb) return std::string(place) + ": Schichten verdoppelt verschieden, Schritt " + std::to_string(k * 7);
                     pixelia::flatten_layers(w.data(), layers);
                     if (w != b) return std::string(place) + ": Schichten zusammengelegt verschieden";
                 }
             }
             return std::string();
         }},
        {"Ausbaustufen und „im Bau“ ändern das vorgezeichnete Bild, zurück ergibt wieder das gleiche",
         [] {
             pixelia::TestPlace p(*pixelia::find_place("testort-raum"), 1);
             pixelia::WorldClock clock;
             p.init(3);
             p.enter();
             const uint8_t* bg = p.room().background();
             if (!bg) return std::string("kein Hintergrundbild");
             const std::vector<uint8_t> before(bg, bg + pixelia::kCoarsePixels);
             auto now = [&] { return std::vector<uint8_t>(bg, bg + pixelia::kCoarsePixels); };
             p.command("ausbau");
             p.tick(clock);
             if (now() == before) return std::string("Ausbau ohne Wirkung");
             p.command("ausbau");
             p.tick(clock);
             if (now() != before) return std::string("zwei Stufen weiter ist nicht wieder Stufe 0");
             p.command("baustelle");
             p.tick(clock);
             if (now() == before) return std::string("im Bau ohne Wirkung");
             const pixelia::Room& room = p.room();
             const int bed = room.find_part(pixelia::PartKind::Bed), table = room.find_part(pixelia::PartKind::Table);
             int tx, ty;
             if (!room.building(bed) || !room.use_point(bed, 0, &tx, &ty, nullptr))
                 return std::string("Bett im Bau: Arbeitsplatz fehlt");
             if (room.use_point(table, 0, &tx, &ty, nullptr)) return std::string("Tisch im Bau noch benutzbar");
             p.command("baustelle");
             p.tick(clock);
             p.leave();
             if (p.room().has_background()) return std::string("Hintergrund nicht freigegeben");
             return std::string();
         }},
        {"Wege: Breitensuche um Möbel herum, versperrtes Ziel (Bett) erreichbar, Erkennungsstück aufgelöst",
         [] {
             pixelia::TestPlace p(*pixelia::find_place("testort-raum"), 1);
             p.init(5);
             const pixelia::Room& room = p.room();
             auto walk = [&](int x, int y, int gx, int gy) {
                 constexpr int dx[4] = {0, 0, -1, 1}, dy[4] = {1, -1, 0, 0};
                 for (int n = 0; n < 200; ++n) {
                     pixelia::Facing d;
                     if (x == gx && y == gy) return n;
                     if (!room.step_toward(x, y, gx, gy, &d)) return -1;
                     x += dx[static_cast<int>(d)];
                     y += dy[static_cast<int>(d)];
                     if (!(x == gx && y == gy) && !room.walkable(x, y)) return -2;
                 }
                 return -3;
             };
             const int n = walk(3, 10, 20, 6);
             if (n != 21) return "Weg zum Bett: " + std::to_string(n) + " Schritte (erwartet 21)";
             if (walk(20, 6, 21, 6) != 1) return std::string("Bett selbst nicht erreichbar");
             if (walk(5, 6, 10, 9) < 0) return std::string("Tisch nicht erreichbar");
             if (room.walkable(10, 8) || room.walkable(0, 6) || !room.walkable(10, 14))
                 return std::string("Stellfläche, Wand oder Ausgang falsch");
             int token = -1;
             for (int i = 0; i < room.part_count(); ++i)
                 if (room.part(i).part == pixelia::kSignaturePart) token = i;
             if (token < 0 || !room.def(token)) return std::string("Erkennungsstück nicht aufgelöst");
             return std::string();
         }},
        {"Ausgänge: Figuren wechseln über Tür und Bildrand den Ort, keine geht verloren; Blick folgt der Begleiterin",
         [] {
             // alle Testorte (seit 0.5.8 mit Kammer) und wer gerade auf einem Weg unterwegs ist
             auto total = [](StageRun& r) {
                 int n = r.stage.in_transit();
                 for (int i = 1; i < r.stage.count(); ++i)
                     n += static_cast<pixelia::TestPlace&>(r.stage.scene(i)).figure_count();
                 return n;
             };
             StageRun a(21, "testort-wiese");
             const int start = total(a);
             int changes = 0, last = a.stage.active();
             for (int k = 0; k < 400; ++k) {
                 a.run(30);
                 if (a.stage.active() != last) ++changes, last = a.stage.active();
                 int guides = 0;
                 for (int i = 1; i < a.stage.count(); ++i)
                     guides += static_cast<pixelia::TestPlace&>(a.stage.scene(i)).has_guide();
                 for (int i = 0; i < a.stage.in_transit(); ++i) guides += a.stage.transit(i).follow;
                 if (guides != 1) return "Begleiterin " + std::to_string(guides) + "-mal da";
             }
             if (a.stage.travels() < 20) return "nur " + std::to_string(a.stage.travels()) + " Reisen in 400 s";
             if (total(a) != start) return "Figuren " + std::to_string(start) + " → " + std::to_string(total(a));
             if (changes < 3) return "Blick folgte nur " + std::to_string(changes) + "-mal";
             StageRun b(21, "testort-wiese");
             b.stage.set_follow(false);
             b.run(12000);
             if (b.stage.active() != b.stage.find("testort-wiese")) return std::string("ohne Folgen trotzdem gewechselt");
             if (b.stage.travels() != a.stage.travels()) return std::string("Reisen hängen vom Blick ab");
             return std::string();
         }},
        // --- Farbe und Licht (seit 0.5.2) ---
        {"Tageszeit: gleiche Uhr mit Zeitraffer ergibt dieselben Bilder, andere Uhrzeit eine andere Palette",
         [] {
             StageRun a(6, "testort-raum"), b(6, "testort-raum"), c(6, "testort-raum");
             for (auto* r : {&a, &b, &c}) {
                 r->clock.set_time_of_day(r == &c ? 5 : 17, 0);
                 r->clock.set_speed(600);  // 300 Schritte = 100 Weltminuten
             }
             a.run(300, true);
             b.run(300, false);
             c.run(300, false);
             if (a.frame() != b.frame()) return std::string("mit Zeitraffer verschieden");
             return a.frame() != c.frame() ? std::string() : "17 Uhr und 5 Uhr gleich";
         }},
        {"Licht: tagsüber unsichtbar, nachts heller; Systemfarben 240–255 unberührt; Nacht nicht unter der Grenze",
         [] {
             pixelia::TestPlace p(*pixelia::find_place("testort-raum"), 1);
             p.init(1);
             p.enter();
             const auto& pal = p.place_palette();
             const int base = pal.base();
             pixelia::WorldClock clk;
             auto at = [&](int h, int m) {
                 clk.set(0, h, m);
                 p.tick(clk);
                 return p.palette();
             };
             auto sum = [](pixelia::Color c) { return c.r + c.g + c.b; };
             const pixelia::Palette day = at(12, 0), night = at(23, 30);
             for (int i = 1; i < base; ++i) {
                 if (pal.role(i) != pixelia::ColorRole::Normal) continue;
                 if (sum(day[i]) != sum(day[2 * base + i])) return "tagsüber Lichtstufe sichtbar, Farbe " + std::to_string(i);
                 if (sum(night[i]) > 30 && sum(night[2 * base + i]) <= sum(night[i]))
                     return "nachts Lichtstufe nicht heller, Farbe " + std::to_string(i);
             }
             for (int i = pixelia::kSceneColors; i < 256; ++i)
                 if (sum(night[i]) != 0) return "Systemfarbe " + std::to_string(i) + " verändert";
             const pixelia::Ambient a = pixelia::daylight_preset(0).at(23LL * 3600 * 1000000);
             if ((a.r + a.g + a.b) / 3 < pixelia::kDefaultNightFloor - 1) return std::string("Nacht unter der Grenze");
             return p.room().light_count() > 0 && p.room().light_map() ? std::string() : "keine Lichtkarte";
         }},
        {"Überblendung mit wechselnden Paletten: Farben laufen weiter; nachts ähnliche Farben zusammengelegt",
         [] {
             StageRun a(4, "testort-wiese"), n(4, "testort-wiese");
             a.clock.set_time_of_day(14, 0);
             a.clock.set_speed(18000);
             n.clock.set_time_of_day(23, 0);
             for (auto* r : {&a, &n}) {
                 r->run(1);
                 r->stage.go(2, Transition::Dissolve);
                 // seit 0.5.8: Vorbereitung, dann ein Durchgang Mischen je Schritt (höchstens vier)
                 for (int i = 0; i < 12 && !r->stage.transitioning(); ++i) r->run(1);
                 r->run(1);
             }
             if (a.stage.transition() != Transition::Dissolve) return std::string("tagsüber nicht überblendet");
             const pixelia::Palette p1 = a.stage.palette();
             a.run(8);
             const pixelia::Palette p2 = a.stage.palette();
             if (!a.stage.transitioning()) return std::string("Übergang schon vorbei");
             if (std::memcmp(p1.data(), p2.data(), sizeof(p1)) == 0) return std::string("Palette steht während des Übergangs");
             if (n.stage.transition() != Transition::Dissolve) return std::string("nachts abgeblendet");
             // jede Farbe beider Orte hat in der gemeinsamen Palette eine nahe Farbe (Toleranz der Zusammenlegung)
             const pixelia::Palette& m = n.stage.palette();
             for (const pixelia::Palette* p : {&n.stage.scene(1).palette(), &n.stage.scene(2).palette()}) {  // Wiese → Raum
                 for (int i = 0; i < pixelia::kSceneColors; ++i) {
                     int best = 1 << 30;
                     for (int k = 0; k < pixelia::kSceneColors; ++k) {
                         const int d = std::abs(m[k].r - (*p)[i].r) + std::abs(m[k].g - (*p)[i].g) + std::abs(m[k].b - (*p)[i].b);
                         best = std::min(best, d);
                     }
                     if (best > 60) return "Farbe " + std::to_string(i) + " fehlt in der gemeinsamen Palette";
                 }
             }
             return std::string();
         }},
        {"Farbrotation, Farbtabellen (Schatten, Mischung) und Lichtstufen der Tabellen",
         [] {
             pixelia::PlacePalette p(3);
             const pixelia::Color cs[5] = {{200, 40, 40}, {40, 200, 40}, {40, 40, 200}, {120, 120, 120}, {60, 60, 60}};
             for (int i = 0; i < 5; ++i) p.set(1 + i, cs[i]);
             p.add_cycle({1, 3, 5});
             pixelia::Palette o0{}, o5{}, o15{};
             const pixelia::Ambient a;
             p.apply(a, 0, o0);
             p.apply(a, 5, o5);
             p.apply(a, 15, o15);
             if (o5[1].r != o0[2].r || o5[1].g != o0[2].g) return std::string("Rotation um einen Platz falsch");
             if (std::memcmp(&o0[1], &o15[1], 3 * sizeof(pixelia::Color)) != 0) return std::string("nach 15 Schritten nicht zurück");
             uint8_t t[256];
             p.tint_table({0, 0, 0}, 128, t);
             if (t[0] != 0 || t[4] != 5 || t[80 + 4] != 85 || t[250] != 250) return std::string("Schattentabelle falsch");
             std::vector<uint8_t> bl(80 * 80);
             p.blend_table(bl.data());
             if (bl[4 * 80 + 4] != 4 || bl[4 * 80 + 5] != bl[5 * 80 + 4]) return std::string("Mischtabelle falsch");
             return std::string();
         }},
        {"Betreten: nur sichtbare Orte haben ein Hintergrundbild, während der Überblendung beide",
         [] {
             StageRun r(2, "testort-wiese");
             r.stage.set_follow(false);
             const int w = r.stage.find("testort-wiese"), p = r.stage.find("testort-platz");
             auto bg = [&](int i) { return static_cast<pixelia::TestPlace&>(r.stage.scene(i)).room().has_background(); };
             if (!bg(w) || bg(p)) return std::string("Start falsch");
             r.stage.go(p, pixelia::Transition::Dissolve);
             r.run(2);
             if (!bg(w) || !bg(p)) return std::string("Überblendung: nicht beide");
             r.run(30);
             if (bg(w) || !bg(p)) return std::string("nach dem Wechsel: alter Ort behält sein Bild");
             return std::string();
         }},
        // --- seit 0.5.3: Wetter und Partikel, Fensterlicht ---
        {"Wetter: Determinismus mit Tageszeit, Gewitter und Ortswechsel (jeden Schritt gezeichnet vs. nur am Ende)",
         [] {
             StageRun a(9, "testort-wiese"), b(9, "testort-wiese"), c(9, "testort-wiese");
             for (auto* r : {&a, &b, &c}) {
                 r->clock.set_time_of_day(19, 30);
                 r->clock.set_speed(60);
                 r->stage.command("blitze 1 2");
             }
             a.stage.command("wetter gewitter");
             b.stage.command("wetter gewitter");
             c.stage.command("wetter schnee");
             for (auto* r : {&a, &b, &c}) {
                 r->run(700, r == &a);
                 r->stage.go(r->stage.find("testort-raum"));
                 r->run(60, r == &a);
                 r->stage.go(r->stage.find("testort-wiese"));
                 r->run(90, r == &a);
             }
             if (a.stage.weather().flashes() == 0) return std::string("kein Blitz");
             if (a.frame() != b.frame()) return std::string("Zeichnen ändert den Ablauf");
             return a.frame() != c.frame() ? std::string() : "Gewitter und Schnee gleich";
         }},
        {"Wetter: weiche Übergänge (erst Wolken, dann Regen; erst Regen weg, dann Wolken)",
         [] {
             pixelia::Weather w;
             w.init(3);
             w.set(pixelia::WeatherKind::Rain);
             for (int i = 0; i < 60; ++i) w.tick();
             if (w.state().rain != 0 || w.state().cloud < 70) return std::string("Regen vor den Wolken");
             for (int i = 0; i < 2000; ++i) w.tick();
             if (w.state().rain != 255 || !w.settled()) return std::string("Regen nicht voll");
             w.set(pixelia::WeatherKind::Clear);
             int guard = 0;
             while (w.state().rain > 0 && ++guard < 3000) w.tick();
             if (w.state().cloud < 200) return std::string("Wolken vor dem Regen weg");
             for (int i = 0; i < 2000; ++i) w.tick();
             return w.settled() && w.state().cloud < 60 ? std::string() : "nicht aufgeklart";
         }},
        {"Partikel: Obergrenze hält (Regen 300 %), Blitz ändert nur Szenenfarben, 240–255 unberührt",
         [] {
             StageRun r(4, "testort-wiese");
             r.stage.command("dichte 300");
             r.stage.command("wetter regen sofort");
             r.run(120, true);
             const auto& place = static_cast<pixelia::TestPlace&>(r.stage.scene(r.stage.active()));
             if (place.weather().peak() != pixelia::PlaceWeather::kMaxParticles)
                 return "Obergrenze nicht erreicht oder überschritten: " + std::to_string(place.weather().peak());
             const pixelia::Palette before = r.stage.palette();
             r.stage.command("blitz");
             r.run(1);
             const pixelia::Palette flash = r.stage.palette();
             int brighter = 0;
             for (int i = 1; i < pixelia::kSceneColors; ++i)
                 brighter += flash[i].r + flash[i].g + flash[i].b > before[i].r + before[i].g + before[i].b;
             for (int i = pixelia::kSceneColors; i < 256; ++i)
                 if (flash[i].r || flash[i].g || flash[i].b) return "Systemfarbe " + std::to_string(i) + " verändert";
             // Partikel allein auf eine einfarbige Fläche (die Einblendungen der Orte nutzen 240–255 mit Absicht)
             std::vector<uint8_t> idx(pixelia::kScreenPixels, 5);
             pixelia::Canvas cv(idx.data(), pixelia::kScreenWidth, pixelia::kScreenHeight);
             place.weather().draw(cv, pixelia::CoarseLayers{}, nullptr);
             int drawn = 0;
             for (uint8_t v : idx) {
                 if (v >= pixelia::kSceneColors) return std::string("Partikel in Systemfarben gezeichnet");
                 drawn += v != 5;
             }
             if (drawn < 500) return std::string("kaum Partikel gezeichnet");
             return brighter > 100 ? std::string() : "Blitz nicht hell genug";
         }},
        {"Fensterlicht: kein Licht innen = kein Fensterschein außen; Innen und Außen bleiben stimmig",
         [] {
             StageRun r(6, "testort-wiese");
             r.stage.set_follow(false);
             r.clock.set_time_of_day(22, 0);
             r.run(2);
             const int w = r.stage.find("testort-wiese"), raum = r.stage.find("testort-raum");
             auto place = [&](int i) -> pixelia::TestPlace& { return static_cast<pixelia::TestPlace&>(r.stage.scene(i)); };
             // beleuchtete Bildpunkte (Lichtstufe 1 oder 2) im Bild
             auto lit = [&] {
                 std::vector<uint8_t> idx(pixelia::kScreenPixels);
                 pixelia::Canvas cv(idx.data(), pixelia::kScreenWidth, pixelia::kScreenHeight);
                 r.stage.render(cv);
                 const int base = place(r.stage.active()).place_palette().base();
                 int n = 0;
                 for (uint8_t v : idx) n += v >= base && v < pixelia::kSceneColors;
                 return n;
             };
             if (lit() == 0 || place(w).room().light_count() != 2) return std::string("Fenster leuchten nicht");
             r.stage.command("innenlicht aus");
             r.run(5);
             if (lit() != 0 || place(w).room().light_count() != 0) return std::string("Fensterschein trotz Licht aus");
             // seit 0.5.9 Teil 2 zeigt ein unsichtbarer Ort nichts (seine Lichtkarte baut er beim Betreten): hier zählt
             // der Weltzustand
             if (place(raum).lights_on()) return std::string("unsichtbarer Raum: Licht noch an");
             r.stage.go(raum, pixelia::Transition::Cut);
             r.run(2);
             if (lit() != 0) return std::string("Raum hell trotz Licht aus");
             r.stage.command("innenlicht an");
             r.run(1);
             if (lit() == 0) return std::string("Raum dunkel trotz Licht an");
             r.stage.go(w, pixelia::Transition::Cut);
             r.run(2);
             return lit() > 0 && place(w).lights_on() ? std::string() : "Wiese: Fenster nach Wechsel dunkel";
         }},
    };
}

// --- Asset-Pakete und Grafik-Speicher (seit 0.5.5) ---

// Quelle aus dem Speicher (für kaputte Pakete)
struct MemorySource : pixelia::AssetSource {
    std::vector<uint8_t> data;
    const char* file = "";
    const char* name() const override { return "test"; }
    bool load(const char* f, pixelia::AssetBlob* out) override {
        if (std::strcmp(f, file) != 0) return false;
        *out = pixelia::AssetBlob{data.data(), data.size(), nullptr};
        return true;
    }
};

std::vector<Case> asset_cases() {
    using pixelia::AssetStore;
    return {
        {"Pakete: alle eingebetteten lesbar, Prüfsumme erkennt Fehler, kaputtes Paket fällt auf die Ersatzquelle zurück",
         [] {
             AssetStore store;
             for (int i = 0; i < pixelia::kEmbeddedAssetCount; ++i) {
                 const auto p = store.probe(pixelia::embedded_source(), pixelia::kEmbeddedAssets[i].file);
                 if (!p.ok || p.entries == 0) return std::string(pixelia::kEmbeddedAssets[i].file) + " nicht lesbar";
             }
             MemorySource bad;
             bad.file = "raum.pxa";
             pixelia::AssetBlob blob;
             pixelia::embedded_source().load("raum.pxa", &blob);
             bad.data.assign(blob.data, blob.data + blob.size);
             bad.data[bad.data.size() - 7] ^= 0x40;
             if (store.probe(bad, "raum.pxa").ok) return std::string("Prüfsumme übersehen");
             store.set_sources(&bad, &pixelia::embedded_source());
             store.want("raum", true);
             pixelia::AssetStore::PackInfo info[4];
             const int n = store.list(info, 4);
             if (n != 1 || !store.loaded("raum") || std::strcmp(info[0].source, "eingebettet") != 0)
                 return std::string("kein Rückfall auf eingebettet");
             return std::string();
         }},
        {"Kennungen: gleich im Werkzeug und in der Engine, Bilder mit Zuständen und Fußpunkt",
         [] {
             if (pixelia::asset_id("teil/lampe") != 0xdda8106du) return std::string("FNV-1a weicht ab");
             AssetStore& store = pixelia::asset_store();
             store.load_base("grund");  // auch ohne vorherige Bühnen-Tests
             pixelia::ImageAsset lamp, fig;
             pixelia::TileSet tiles;
             if (!store.image(pixelia::asset_id("figur/test"), &fig) || fig.sheet.frames != 15 ||
                 fig.sheet.pivot_x != 8 || fig.sheet.pivot_y != 30)
                 return std::string("Figur fehlt oder falsch");
             if (!store.tiles(pixelia::asset_id("kacheln/testorte"), &tiles) || tiles.count != 19 || !tiles.flag(10) ||
                 tiles.flag(1))
                 return std::string("Kacheln fehlen oder Eigenschaften falsch");
             int colors = 0;
             if (!store.palette(pixelia::asset_id("palette/testorte"), &colors) || colors != 80)
                 return std::string("Palette fehlt");
             const uint8_t* row = store.recolor(pixelia::asset_id("farbreihe/holz"));
             if (!row || row[20] != 64 || row[23] != 67 || row[24] != 24) return std::string("Farbreihe Holz falsch");
             AssetStore own;
             own.set_sources(&pixelia::embedded_source(), nullptr);
             own.want("moebel", true);
             if (!own.image(pixelia::asset_id("teil/lampe"), &lamp) || lamp.sheet.states != 2 ||
                 lamp.sheet.frames_per_state() != 2)
                 return std::string("Lampe ohne Zustände");
             const pixelia::Sprite off = lamp.sheet.frame(0, 1), on = lamp.sheet.frame(1, 0);
             if (std::memcmp(off.pixels, on.pixels, static_cast<size_t>(off.width) * off.height) == 0)
                 return std::string("Lampe aus = an");
             return std::string();
         }},
        {"Grafik-Speicher: Budget verdrängt das am längsten unbenutzte Paket, angeheftete und Grundausstattung bleiben",
         [] {
             AssetStore store;
             store.set_sources(&pixelia::embedded_source(), nullptr);
             store.load_base("grund");
             auto bytes = [&](const char* f) { return store.probe(pixelia::embedded_source(), f).bytes; };
             // Platz für alle drei Pakete außer einem Byte: das dritte verdrängt eines
             store.set_budget(store.stats().used + bytes("moebel.pxa") + bytes("wiese.pxa") + bytes("raum.pxa") - 1);
             store.pin("moebel");
             store.want("wiese", false);
             store.want("raum", false);  // passt nicht mehr: „wiese“ muss weichen
             const auto s = store.stats();
             if (!store.loaded("grund") || !store.loaded("moebel")) return std::string("angeheftetes Paket verdrängt");
             if (store.loaded("wiese") || !store.loaded("raum") || s.evictions != 1)
                 return std::string("falsch verdrängt: ") + std::to_string(s.evictions);
             if (s.used > s.budget) return std::string("über dem Budget");
             store.unpin("moebel");
             // eben verlassen zählt als eben benutzt: es weicht das nur vorgeladene „raum“
             store.want("wiese", true);
             if (!store.loaded("moebel") || !store.loaded("wiese") || store.loaded("raum"))
                 return std::string("Reihenfolge beim Verdrängen falsch");
             return std::string();
         }},
        {"Laden auf eigener Task: Abblenden wartet in der dunklen Mitte, Überblendung beginnt erst nach dem Laden",
         [] {
             AssetStore& store = pixelia::asset_store();
             StageRun r(7, "testort-platz");
             r.stage.set_follow(false);
             const int raum = r.stage.find("testort-raum"), wiese = r.stage.find("testort-wiese");
             store.set_async([] {});  // Lade-Task, die erst auf Zuruf arbeitet
             store.drop_unpinned();   // „raum“ und „wiese“ sind nicht geladen
             auto restore = [&] {
                 while (store.work()) {
                 }
                 store.set_async(nullptr);
             };
             r.stage.go(raum, pixelia::Transition::Fade);
             r.run(30);
             const auto black = r.frame();
             const bool dark = std::all_of(black.begin(), black.end(), [](uint8_t v) { return v == 0; });
             if (!r.stage.holding() || r.stage.entered(raum) || !dark) {
                 restore();
                 return std::string("Abblenden wartet nicht im Dunkeln");
             }
             while (store.work()) {
             }
             r.run(20);
             if (r.stage.transitioning() || !r.stage.entered(raum) || r.stage.holds() == 0) {
                 restore();
                 return std::string("nach dem Laden nicht weiter");
             }
             store.drop_unpinned();  // „wiese“ (vorgeladen) wieder weg
             r.stage.go(wiese, pixelia::Transition::Dissolve);
             r.run(10);
             if (r.stage.transitioning() || r.stage.active() != raum) {
                 restore();
                 return std::string("Überblendung ohne geladenes Ziel begonnen");
             }
             while (store.work()) {
             }
             r.run(2);
             const bool started = r.stage.transitioning() && r.stage.active() == wiese;
             restore();
             return started ? std::string() : "Überblendung beginnt nach dem Laden nicht";
         }},
        {"Zustandsbilder: Lampe aus/an folgt der Lichtquelle (Raum), Laterne auf dem Platz ebenso",
         [] {
             StageRun r(3, "testort-raum");
             auto lamp_state = [&](const char* place) {
                 const auto& room = static_cast<const pixelia::TestPlace&>(r.stage.scene(r.stage.find(place))).room();
                 for (int i = 0; i < room.part_count(); ++i)
                     if (room.def(i) && std::strcmp(room.def(i)->id, "lampe") == 0) return static_cast<int>(room.part(i).state);
                 return -1;
             };
             if (lamp_state("testort-raum") != 1 || lamp_state("testort-platz") != 1) return std::string("Lampe aus trotz Licht");
             r.stage.command("innenlicht aus");
             r.run(1);
             if (lamp_state("testort-raum") != 0) return std::string("Lampe an trotz Licht aus");
             r.stage.command("innenlicht an");
             r.run(1);
             return lamp_state("testort-raum") == 1 ? std::string() : "Lampe bleibt aus";
         }},
    };
}

std::vector<Case> overlay_cases() {
    using pixelia::Camera;
    using pixelia::CameraHome;
    using pixelia::CameraSettings;
    using pixelia::Hit;
    namespace ui = pixelia::ui;
    return {
        {"Figur antippen trifft sie, ihre durchsichtige Ecke nicht",
         [] {
             StageRun r(5, "testort-wiese");
             r.run(60);
             pixelia::FigureInfo f[64];
             const int n = r.stage.scene(r.stage.active()).figures(f, 64);
             int checked = 0;
             for (int i = 0; i < n; ++i) {
                 int x, y, h;
                 if (!r.stage.scene(r.stage.active()).figure_mark(f[i].id, &x, &y, &h)) continue;
                 // Brust: 9 grobe Bildpunkte über dem Fußpunkt; vorn kann eine andere Figur stehen (dann tiefer)
                 const Hit a = r.stage.hit(x, y - 18);
                 if (a.kind != Hit::Kind::Figure) return "Figur " + std::to_string(f[i].id) + " nicht getroffen";
                 if (a.id != f[i].id && a.y < y) return std::string("weiter hinten stehende Figur getroffen");
                 // linke obere Ecke der Zelle (16 × 32 grob) ist durchsichtig: dort darf diese Figur nicht treffen
                 const Hit b = r.stage.hit(x - 15, y - 58);
                 if (b.kind == Hit::Kind::Figure && b.id == f[i].id) return std::string("durchsichtiger Punkt trifft");
                 ++checked;
             }
             return checked >= 3 ? std::string() : "zu wenige Figuren geprüft";
         }},
        {"Karte einer Figur: Name, Tätigkeit, Ort, Gedanke, Markierung",
         [] {
             StageRun r(5, "testort-wiese");
             r.run(30);
             Hit h;
             h.kind = Hit::Kind::Figure;
             h.id = r.stage.lead();
             ui::Card c;
             if (!r.stage.describe(h, &c)) return std::string("keine Karte");
             if (c.title != "Begleiterin" || c.subtitle.empty() || c.where != "Testort Wiese" || c.quote.empty())
                 return "Inhalt falsch: " + c.title + " / " + c.where;
             return c.mark_x >= 0 && c.figure == h.id && c.picture.sprite.pixels ? std::string() : "Markierung oder Bild fehlt";
         }},
        {"Tür antippen: Karte zeigt, wer im Raum dahinter ist",
         [] {
             StageRun r(5, "testort-wiese");
             r.run(30);
             for (int y = 0; y < pixelia::kScreenHeight; y += 4)
                 for (int x = 0; x < pixelia::kScreenWidth; x += 4) {
                     const Hit h = r.stage.hit(x, y);
                     ui::Card c;
                     if (h.kind != Hit::Kind::Part || !r.stage.describe(h, &c) || !c.place) continue;
                     if (std::strcmp(c.place, "testort-raum") != 0) return std::string("falscher Ort ") + c.place;
                     if (c.where != "→ Testort Raum") return "Ziel fehlt: " + c.where;
                     if (c.hint != "Betreten" || !ui::card_button(c, ui::CardStyle{}).w) return std::string("Knopf Betreten fehlt");
                     return c.line_count == 1 && c.lines[0].rfind("Drin: ", 0) == 0 ? std::string() : "keine Bewohner";
                 }
             return std::string("keine Tür gefunden");
         }},
        {"Jeder Testort: alle Ausgänge per Tippen erreichbar (Karte mit Ziel und Knopf)",
         [] {
             for (const char* name : {"testort-wiese", "testort-raum", "testort-platz"}) {
                 StageRun r(5, name);
                 r.run(30);
                 const auto& room = static_cast<const pixelia::TestPlace&>(r.stage.scene(r.stage.active())).room();
                 for (int e = 0; e < room.exit_count(); ++e) {
                     const char* to = room.exit(e).to;
                     bool found = false;
                     for (int y = 0; y < pixelia::kScreenHeight && !found; y += 4)
                         for (int x = 0; x < pixelia::kScreenWidth && !found; x += 4) {
                             const Hit h = r.stage.hit(x, y);
                             ui::Card c;
                             if (h.kind == Hit::Kind::None || h.kind == Hit::Kind::Figure || !r.stage.describe(h, &c)) continue;
                             found = c.place && std::strcmp(c.place, to) == 0 && ui::card_button(c, ui::CardStyle{}).w &&
                                     c.where.rfind("→ ", 0) == 0;
                         }
                     if (!found) return std::string(name) + ": kein Weg nach " + to;
                 }
             }
             return std::string();
         }},
        {"Standard Hauptfigur: Anzeige geht mit ihr durch die Ausgänge",
         [] {
             StageRun r(11, "testort-wiese");
             Camera cam;
             const uint16_t lead = r.stage.lead();
             int moves = 0, last = r.stage.find_figure(lead);
             for (int t = 0; t < 9000; ++t) {
                 r.run(1);
                 cam.tick(r.stage);
                 const int at = r.stage.find_figure(lead);
                 if (at >= 0 && at != last) ++moves, last = at;
             }
             r.run(60);
             const int at = r.stage.find_figure(lead);
             if (moves == 0) return std::string("Hauptfigur ist nie gegangen");
             return at < 0 || at == r.stage.active() ? std::string() : "Anzeige nicht bei der Hauptfigur";
         }},
        {"Folgen: Anzeige bleibt bei einer anderen Figur, auch über Ausgänge",
         [] {
             StageRun r(13, "testort-wiese");
             Camera cam;
             cam.tick(r.stage);
             pixelia::FigureInfo f[8];
             const int n = r.stage.scene(r.stage.active()).figures(f, 8);
             if (n < 2) return std::string("zu wenige Figuren");
             const uint16_t who = f[1].id;
             cam.follow(r.stage, who);
             int moves = 0, last = r.stage.find_figure(who), wrong = 0;
             for (int t = 0; t < 20000; ++t) {
                 r.run(1);
                 cam.tick(r.stage);
                 cam.input();  // „Zuschauer“ berührt regelmäßig: keine Rückkehr
                 const int at = r.stage.find_figure(who);
                 if (at >= 0 && at != last) ++moves, last = at;
                 // 2 s nach einem Wechsel muss die Anzeige dort sein
                 if (at >= 0 && at != r.stage.active() && !r.stage.transitioning()) ++wrong;
                 else wrong = 0;
                 if (wrong > 60) return "Anzeige bleibt nicht bei Figur " + std::to_string(who);
             }
             return cam.mode() == Camera::Mode::Follow ? std::string() : "Modus verloren";
         }},
        {"Frei: Wischen wechselt den Ort, nach der Ruhezeit zurück; Rückkehr „nie“ bleibt",
         [] {
             StageRun r(7, "testort-wiese");
             Camera cam;
             CameraSettings s;
             s.return_s = 2;
             cam.set_settings(s);
             cam.tick(r.stage);
             r.run(30);
             const int home = r.stage.active();
             cam.swipe(r.stage, +1);
             r.run(40);
             if (cam.mode() != Camera::Mode::Free || r.stage.active() == home) return std::string("Wischen ohne Wirkung");
             for (int t = 0; t < 3 * 30; ++t) {
                 r.run(1);
                 cam.tick(r.stage);
             }
             r.run(40);
             if (cam.mode() != Camera::Mode::Home) return std::string("keine Rückkehr");
             if (r.stage.active() != r.stage.find_figure(r.stage.lead())) return std::string("nicht bei der Hauptfigur");
             s.return_s = 0;
             cam.set_settings(s);
             cam.tick(r.stage);
             cam.swipe(r.stage, -1);
             const int free_place = (home + r.stage.count() - 1) % r.stage.count();
             for (int t = 0; t < 600 * 30 / 10; ++t) {
                 r.run(1);
                 cam.tick(r.stage);
             }
             return cam.mode() == Camera::Mode::Free && r.stage.active() == free_place ? std::string()
                                                                                      : "Rückkehr trotz „nie“";
         }},
        {"Standard fester Ort: Anzeige bleibt dort",
         [] {
             StageRun r(7, "testort-wiese");
             Camera cam;
             CameraSettings s;
             s.home = CameraHome::Place;
             s.place = static_cast<int8_t>(r.stage.find("testort-platz"));
             cam.set_settings(s);
             for (int t = 0; t < 4000; ++t) {
                 r.run(1);
                 cam.tick(r.stage);
             }
             return r.stage.active() == s.place && !r.stage.transitioning() ? std::string() : "Ort verlassen";
         }},
        {"Kurzmeldungen: höchstens 4 aus der Welt pro Stunde, Technik geht vor",
         [] {
             ui::ToastQueue q;
             ui::Toast w;
             w.kind = ui::ToastKind::World;
             w.text = "Welt";
             w.seconds = 20;
             int ok = q.push(w, 1000);
             q.update(1000);  // Weltmeldung sichtbar
             for (int i = 1; i < 6; ++i) ok += q.push(w, 1000 + i * 100);
             if (ok != 4 || q.dropped() != 2) return "angenommen " + std::to_string(ok);
             ui::Toast sys;
             sys.text = "System";
             if (!q.push(sys, 1600)) return std::string("Systemmeldung verworfen");
             const ui::Toast* t = q.update(1600);
             if (!t || t->text != "Welt") return std::string("sichtbare Meldung zu früh abgelöst");
             t = q.update(3100);  // nach 2 s löst die Systemmeldung ab
             if (!t || t->text != "System") return std::string("Systemmeldung wartet");
             return q.push(w, 1000 + 3600u * 1000u + 10) ? std::string() : "nach einer Stunde noch gesperrt";
         }},
        {"Pixel-Uhr und Karte zeichnen ohne Absturz in allen Ecken und Arten",
         [] {
             StageRun r(5, "testort-wiese");
             r.run(30);
             std::vector<uint8_t> px(pixelia::kScreenPixels);
             pixelia::Canvas c(px.data(), pixelia::kScreenWidth, pixelia::kScreenHeight);
             for (int k = 0; k < 4; ++k)
                 for (int sec = 0; sec < 2; ++sec) {
                     ui::ClockStyle st;
                     st.corner = static_cast<ui::Corner>(k);
                     st.seconds = sec;
                     const ui::Rect a = ui::draw_clock(c, 23, 59, 59, st, true);
                     if (a.x < 0 || a.y < 0 || a.x + a.w > pixelia::kScreenWidth || a.y + a.h > pixelia::kScreenHeight)
                         return std::string("Uhr außerhalb");
                 }
             Hit h;
             h.kind = Hit::Kind::Figure;
             h.id = r.stage.lead();
             ui::Card card;
             r.stage.describe(h, &card);
             for (int l = 0; l < 3; ++l) {
                 ui::CardStyle cs;
                 cs.layout = static_cast<ui::CardLayout>(l);
                 ui::render_card(c, card, cs);
             }
             return std::string();
         }},
    };
}

using pixelia::Hit;
namespace ui = pixelia::ui;

// --- Orte und Gebiete (seit 0.5.8): Module, Orts-Graph, Verbindungen, Fenster mit Stellplätzen, Mond im Fenster ---

// Tippen auf Ausgang e des sichtbaren Ortes: Karte mit Ziel; true, wenn eine Stelle im Bereich (eine Kachel
// Spielraum) dorthin führt. Figuren können im Weg stehen, deshalb der ganze Bereich
bool tap_exit(StageRun& r, int e) {
    const auto& room = static_cast<const pixelia::TestPlace&>(r.stage.scene(r.stage.active())).room();
    const pixelia::Exit& x = room.exit(e);
    constexpr int k = pixelia::kTileSize * pixelia::kCoarseScale;
    for (int ty = x.y - 1; ty <= x.y + x.h; ++ty)
        for (int tx = x.x - 1; tx <= x.x + x.w; ++tx)
            for (int dy = 2; dy < k; dy += 5)
                for (int dx = 2; dx < k; dx += 5) {
                    const int fx = tx * k + dx, fy = ty * k + dy;
                    if (fx < 0 || fy < 0 || fx >= pixelia::kScreenWidth || fy >= pixelia::kScreenHeight) continue;
                    const Hit h = r.stage.hit(fx, fy);
                    ui::Card c;
                    if (h.kind == Hit::Kind::None || h.kind == Hit::Kind::Figure || !r.stage.describe(h, &c)) continue;
                    if (h.kind == Hit::Kind::Exit && h.id != e) continue;
                    if (c.place && std::strcmp(c.place, x.to) == 0 && ui::card_button(c, ui::CardStyle{}).w) return true;
                }
    return false;
}

// „Betreten“/„Hingehen“ auf der Karte: die Anzeige wechselt (wie gfx.cpp über die Kamera)
bool press_go(StageRun& r, const char* to) {
    const int t = r.stage.find(to);
    if (t < 0) return false;
    r.stage.go(t);
    for (int i = 0; i < 120 && (r.stage.active() != t || r.stage.transitioning()); ++i) r.run(1);
    return r.stage.active() == t && !r.stage.transitioning();
}

// Mondpunkte im Bild: Index der Mondfarbe des Ortes (beim Abblenden bleiben die Indizes)
int moon_pixels(StageRun& r, int color) {
    std::vector<uint8_t> idx(pixelia::kScreenPixels);
    pixelia::Canvas c(idx.data(), pixelia::kScreenWidth, pixelia::kScreenHeight);
    r.stage.render(c);
    int n = 0;
    for (uint8_t v : idx) n += v == color;
    return n;
}

std::vector<Case> place_cases() {
    return {
        {"Module: jeder Ort der Welt hat Daten, eine Szene und eine eindeutige Kennung",
         [] {
             const pixelia::AreaDef* const* areas = nullptr;
             const int n = pixelia::world_areas(&areas);
             if (n < 1) return std::string("keine Gebiete");
             int places = 0;
             for (int a = 0; a < n; ++a)
                 for (int i = 0; i < areas[a]->count; ++i, ++places) {
                     const pixelia::PlaceDef* d = areas[a]->places[i];
                     if (pixelia::find_place(d->id) != d) return std::string("doppelte Kennung ") + d->id;
                     auto s = pixelia::make_place(d->id);
                     if (!s || std::strcmp(s->name(), d->id) != 0) return std::string("keine Szene für ") + d->id;
                 }
             if (places < 4) return "nur " + std::to_string(places) + " Orte";
             return std::string();
         }},
        {"Orts-Graph: jede Verbindung hat einen Rückweg, jeder Ort erreicht jeden in beide Richtungen",
         [] {
             pixelia::PlaceGraph g;
             g.build();
             char buf[160];
             const char* err = g.check(buf, sizeof(buf));
             if (err[0]) return std::string(err);
             if (g.edges() < 8) return "nur " + std::to_string(g.edges()) + " Verbindungen";
             for (int a = 0; a < g.places(); ++a)
                 for (int b = 0; b < g.places(); ++b)
                     if (g.hops(a, b) < 0 || g.hops(b, a) < 0) return std::string("nicht verbunden: ") + g.place(a).id;
             return std::string();
         }},
        {"Jede Verbindung per Tippen hin und zurück (Karte, Betreten/Hingehen, Rückweg im Zielort)",
         [] {
             pixelia::PlaceGraph g;
             g.build();
             for (int i = 0; i < g.edges(); ++i) {
                 const auto& e = g.edge(i);
                 const char* from = g.place(e.from).id;
                 const char* to = g.place(e.to).id;
                 StageRun r(7, from);
                 r.stage.set_follow(false);
                 r.run(20);
                 if (!tap_exit(r, e.exit)) return std::string(from) + ": Ausgang " + std::to_string(e.exit) + " nicht antippbar";
                 if (!press_go(r, to)) return std::string(from) + " → " + to + ": nicht angekommen";
                 r.run(10);
                 if (!tap_exit(r, e.entry)) return std::string(to) + ": Rückweg nach " + from + " nicht antippbar";
                 if (!press_go(r, from)) return std::string(to) + " → " + from + ": nicht zurück";
             }
             return std::string();
         }},
        {"Feldweg: 30 Weltsekunden unterwegs (keinem Ort zugehörig), nur bei Tag; Karte nennt Reisezeit und Grund",
         [] {
             // tagsüber: irgendwann ist jemand auf dem Feldweg unterwegs und kommt nach 30 s an
             StageRun d(3, "testort-wiese");
             d.stage.set_world_time(true);
             d.clock.set_time_of_day(11, 0);
             d.stage.set_follow(false);
             // seit 0.5.9 je Figur aus dem Weltzustand (where = Verbindung), nicht mehr „irgendwer unterwegs“
             int seen = 0, longest = 0;
             std::vector<int> run(0x10000, 0);
             for (int t = 0; t < 30 * 600; ++t) {
                 d.run(1);
                 const pixelia::FigureBook& book = d.stage.environment().figures;
                 for (int k = 0; k < book.count(); ++k) {
                     const pixelia::FigureRecord& f = book.at(k);
                     int& r = run[f.id];
                     r = f.where.kind == pixelia::Whereabouts::Kind::Link ? r + 1 : 0;
                     longest = std::max(longest, r);
                     seen += r == 1;
                 }
             }
             if (seen == 0) return std::string("in 10 min niemand auf dem Feldweg");
             if (longest < 29 * 30 || longest > 31 * 30) return "unterwegs " + std::to_string(longest / 30) + " s";
             // nachts: niemand nimmt ihn, die Karte sagt warum
             StageRun n(3, "testort-wiese");
             n.stage.set_world_time(true);
             n.clock.set_time_of_day(23, 30);
             n.stage.set_follow(false);
             for (int t = 0; t < 30 * 600; ++t) {
                 n.run(1);
                 if (n.stage.in_transit() > 0) return std::string("nachts auf dem Feldweg");
             }
             const auto& room = static_cast<const pixelia::TestPlace&>(n.stage.scene(n.stage.active())).room();
             for (int e = 0; e < room.exit_count(); ++e) {
                 if (room.exit(e).kind != pixelia::ExitKind::Path) continue;
                 const int k = pixelia::kTileSize * pixelia::kCoarseScale;
                 const Hit h{Hit::Kind::Exit, static_cast<uint16_t>(e), 0, 0, 0, 0};
                 ui::Card c;
                 (void)k;
                 if (!n.stage.describe(h, &c)) return std::string("keine Karte zum Feldweg");
                 if (c.subtitle.find("30 Sekunden") == std::string::npos) return "Reisezeit fehlt: " + c.subtitle;
                 if (c.quote.find("Nur bei Tag") == std::string::npos) return "Grund fehlt: " + c.quote;
                 return std::string();
             }
             return std::string("kein Feldweg an der Wiese");
         }},
        {"Fensterbank: Kerze und Blumentöpfe stehen auf den Stellplätzen, flach an der Wand, versperren nichts",
         [] {
             StageRun r(2, "testort-raum");
             const auto& room = static_cast<const pixelia::TestPlace&>(r.stage.scene(r.stage.active())).room();
             int items = 0;
             for (int i = 0; i < room.part_count(); ++i) {
                 const pixelia::RoomPart& p = room.part(i);
                 if (!(p.flags & pixelia::kPartOnSlot)) continue;
                 ++items;
                 int sx, sy;
                 if (!room.slot_point(p.x, p.y, &sx, &sy)) return "Stellplatz fehlt bei Teil " + std::to_string(i);
                 const pixelia::Rect img = room.image_rect(i);
                 if (img.empty() || img.y + img.h - 1 != sy || std::abs(img.x + img.w / 2 - sx) > 1)
                     return "Teil " + std::to_string(i) + " steht nicht auf dem Stellplatz";
                 if (room.layer_of(i) != pixelia::PartLayer::Wall) return std::string("nicht auf der Ebene des Fensters");
                 if (room.slot_item(p.x, p.y) != i) return std::string("Stellplatz doppelt belegt");
             }
             if (items != 3) return "Dinge auf Stellplätzen: " + std::to_string(items);
             for (int x = 0; x < pixelia::kTilesX; ++x)
                 if (!room.walkable(x, 6) && x > 0 && x < pixelia::kTilesX - 1 && x != 21 && x != 22 && x != 2 && x != 3 &&
                     x != 12 && x != 13)  // Bett, Schrank, Ofen (seit 0.5.8 Teil 2)
                     return "Reihe vor den Fenstern versperrt bei " + std::to_string(x);
             return std::string();
         }},
        {"Mond im Fenster: ganz in einer Scheibe, bei Voll-, Halbmond und Sichel; Lage folgt dem Stand am Himmel",
         [] {
             for (int phase : {50, 25, 10}) {
                 int last_x = -1;
                 for (int lage : {10, 40, 60, 90}) {
                     StageRun r(4, "testort-raum");
                     r.stage.set_world_time(true);
                     r.clock.set_time_of_day(23, 0);
                     r.stage.command("wetter sonne sofort");
                     char cmd[32];
                     std::snprintf(cmd, sizeof(cmd), "mond %d 70 %d", phase, lage);
                     r.stage.command(cmd);
                     r.run(3);
                     pixelia::SkyView v;
                     if (!r.stage.scene(r.stage.active()).sky_view(&v)) return std::string("Raum ohne Himmel");
                     int cx, cy, rad;
                     pixelia::Rect pane;
                     if (!r.stage.environment().sky.place(v, &cx, &cy, &rad, &pane))
                         return "Mond nicht sichtbar (Phase " + std::to_string(phase) + ")";
                     if (cx - rad < pane.x || cx + rad >= pane.x + pane.w || cy - rad < pane.y || cy + rad >= pane.y + pane.h)
                         return std::string("Mond ragt aus der Scheibe");
                     if (rad < 5) return "Mond zu klein: " + std::to_string(rad);
                     if (cx < last_x) return std::string("Lage läuft nicht von Osten nach Westen");
                     last_x = cx;
                     if (moon_pixels(r, v.moon) < 10) return "Mond nicht im Bild (Phase " + std::to_string(phase) + ")";
                 }
             }
             return std::string();
         }},
        {"Mond beim Ortswechsel: blendet mit dem Bild aus (Abblenden) und bleibt bei der Überblendung",
         [] {
             StageRun r(4, "testort-raum");
             r.stage.set_world_time(true);
             r.clock.set_time_of_day(23, 0);
             r.stage.command("wetter sonne sofort");
             r.stage.command("mond 50 70 30");
             r.stage.set_follow(false);
             r.run(3);
             pixelia::SkyView v;
             r.stage.scene(r.stage.active()).sky_view(&v);
             r.stage.go(r.stage.find("testort-kammer"), pixelia::Transition::Fade);
             r.run(3);
             if (!r.stage.transitioning()) return std::string("kein Übergang");
             if (moon_pixels(r, v.moon) < 10) return std::string("Mond verschwindet beim Abblenden");
             r.run(60);
             r.stage.go(r.stage.find("testort-raum"), pixelia::Transition::Dissolve);
             r.run(4);
             if (!r.stage.transitioning() || r.stage.transition() != pixelia::Transition::Dissolve)
                 return std::string("keine Überblendung");
             // gemeinsame Palette: Mondpunkte haben die Farbe des Mondes
             std::vector<uint8_t> idx(pixelia::kScreenPixels);
             pixelia::Canvas c(idx.data(), pixelia::kScreenWidth, pixelia::kScreenHeight);
             r.stage.render(c);
             const pixelia::Color m = r.stage.scene(r.stage.active()).palette()[v.moon];
             int n = 0;
             for (uint8_t i : idx) {
                 const pixelia::Color p = r.stage.palette()[i];
                 n += i < pixelia::kSceneColors && std::abs(p.r - m.r) + std::abs(p.g - m.g) + std::abs(p.b - m.b) <= 30;  // Toleranz der gemeinsamen Palette
             }
             return n >= 4 ? std::string() : "Mond fehlt in der Überblendung (" + std::to_string(n) + ")";
         }},
    };
}

// --- Ortszustände, Ereignis-Bus, Fahrzeuge, unter Tage, Rechenspitzen (seit 0.5.8 Teil 2) ---

// Bühne mit laufender Weltzeit im Zeitraffer (speed: Weltsekunden je echter Sekunde)
void world_run(StageRun& r, int hour, int minute, double speed) {
    r.stage.set_world_time(true);
    r.stage.set_follow(false);
    r.clock.set_time_of_day(hour, minute);
    r.clock.set_speed(speed);
}

// Zustände als Text zum Vergleichen (filter: nur diese Hausregeln, leer = alle)
std::string states_text(const pixelia::PlaceStates& st, bool rules_only) {
    std::string s;
    for (int i = 0; i < st.count(); ++i) {
        const pixelia::StateDef& d = st.def(i);
        if (rules_only && d.rule != pixelia::StateRule::Evening && d.rule != pixelia::StateRule::Hearth) continue;
        char buf[96];
        std::snprintf(buf, sizeof(buf), "%s/%s:%d,%d,%d,%d ", st.place(i), d.id, st.phase(i), st.value(i), st.heat(i),
                      st.on(i) ? 1 : 0);
        s += buf;
    }
    return s;
}

// Ort unter Tage (nur für Bühnen-Tests, kein Keller im Testdorf): Kammer-Raum, Nässe ohne Himmel
constexpr pixelia::StateDef kUnderStates[] = {{.kind = pixelia::StateKind::Wetness, .id = "boden"}};
const pixelia::PlaceDef kUnderground = {"test-stollen", "Test-Stollen", pixelia::kPlaceUnderground,
                                        pixelia::testdorf::kKammer.room, "raum", 1, false, {256, 256, 256}, 200,
                                        {0, 0, 0}, pixelia::GlassRole::Sky, kUnderStates, 1, nullptr, 0};

// Kleine Welt mit Fahrzeug: zwei Halte, ein Wagen, ein Weg zwischen den Halten
constexpr pixelia::VehicleStop kStops[] = {{"halt-a", 5, 10}, {"halt-b", 5, 10}};
constexpr pixelia::VehicleDef kWagon = {"test-wagen", kStops, 2, 0};
constexpr pixelia::Exit kExitsA[] = {
    {12, 0, 2, 1, 12, 1, pixelia::Facing::Down, pixelia::ExitKind::Station, "test-wagen", 0,
     pixelia::Transition::Fade, 0, pixelia::kOpenTimetable, "test-wagen"},
    {24, 9, 1, 2, 23, 9, pixelia::Facing::Left, pixelia::ExitKind::Path, "halt-b", 1, pixelia::Transition::Fade, 60}};
constexpr pixelia::Exit kExitsB[] = {
    {12, 0, 2, 1, 12, 1, pixelia::Facing::Down, pixelia::ExitKind::Station, "test-wagen", 1,
     pixelia::Transition::Fade, 0, pixelia::kOpenTimetable, "test-wagen"},
    {0, 9, 1, 2, 1, 9, pixelia::Facing::Right, pixelia::ExitKind::Path, "halt-a", 1, pixelia::Transition::Fade, 60}};
constexpr pixelia::Exit kExitsW[] = {
    {0, 9, 1, 2, 1, 9, pixelia::Facing::Right, pixelia::ExitKind::Station, "halt-a", 0, pixelia::Transition::Fade, 0,
     pixelia::kOpenTimetable, "test-wagen"},
    {24, 9, 1, 2, 23, 9, pixelia::Facing::Left, pixelia::ExitKind::Station, "halt-b", 0, pixelia::Transition::Fade, 0,
     pixelia::kOpenTimetable, "test-wagen"}};
constexpr pixelia::RoomDef kRoomA = {{nullptr, nullptr, nullptr}, "", {0, 0, 0}, nullptr, 0, kExitsA, 2};
constexpr pixelia::RoomDef kRoomB = {{nullptr, nullptr, nullptr}, "", {0, 0, 0}, nullptr, 0, kExitsB, 2};
constexpr pixelia::RoomDef kRoomW = {{nullptr, nullptr, nullptr}, "", {0, 0, 0}, nullptr, 0, kExitsW, 2};
const pixelia::PlaceDef kHaltA = {"halt-a", "Halt A", pixelia::kPlaceOutdoor, &kRoomA, nullptr, 0, false,
                                  {256, 256, 256}, 256, {0, 0, 0}, pixelia::GlassRole::None, nullptr, 0, nullptr, 0};
const pixelia::PlaceDef kHaltB = {"halt-b", "Halt B", pixelia::kPlaceOutdoor, &kRoomB, nullptr, 0, false,
                                  {256, 256, 256}, 256, {0, 0, 0}, pixelia::GlassRole::None, nullptr, 0, nullptr, 0};
const pixelia::PlaceDef kWagonPlace = {"test-wagen", "Testwagen", pixelia::kPlaceVehicle, &kRoomW, nullptr, 0, false,
                                       {256, 256, 256}, 256, {0, 0, 0}, pixelia::GlassRole::Sky, nullptr, 0,
                                       nullptr, 0, nullptr, &kWagon};

std::vector<Case> state_cases() {
    using pixelia::PlaceStates;
    using pixelia::StateDef;
    using pixelia::StateKind;
    using pixelia::StateRule;
    using pixelia::LightKind;
    return {
        {"Lichtquellen-Arten: Stärke 1–4 ergibt Radius und Helligkeit, Glut ohne Schein nach außen",
         [] {
             using pixelia::light_kind;
             if (light_kind(LightKind::Tallow).strength != 2 || light_kind(LightKind::OilLamp).strength != 4 ||
                 light_kind(LightKind::Hearth).strength != 3 || light_kind(LightKind::Ember).strength != 1)
                 return std::string("Stärken nicht wie im Setting");
             if (light_kind(LightKind::Ember).outside) return std::string("Glut scheint nach außen");
             int last = 0;
             for (int s = 1; s <= pixelia::kMaxStrength; ++s) {
                 const pixelia::LightCircle c = pixelia::light_circle(10, 10, s);
                 if (c.rx <= last || c.ry == 0 || c.strength == 0) return "Stärke " + std::to_string(s) + " nicht größer";
                 last = c.rx;
             }
             return pixelia::light_circle(0, 0, 4).rx == 60 ? std::string() : std::string("Stärke 4 nicht 60 breit");
         }},
        {"Hausregeln: Abendlicht, Kerze für Heimkehrer (brennt ab), Herdfeuer mit Glut über Nacht",
         [] {
             static constexpr StateDef defs[] = {
                 {.kind = StateKind::Light, .id = "lampe", .light = LightKind::OilLamp, .rule = StateRule::Evening},
                 {.kind = StateKind::Light, .id = "kerze", .light = LightKind::Tallow, .rule = StateRule::Homecomer},
                 {.kind = StateKind::Fire, .id = "ofen", .light = LightKind::Hearth, .rule = StateRule::Hearth}};
             PlaceStates st;
             st.declare("haus", defs, 3, "h", false);
             pixelia::StateInput in;  // Sonne 6:30–19:45, dunkel ab 20:05
             in.lead_known = true;
             std::string log;
             for (int m = 12 * 60; m < 32 * 60; ++m) {
                 const int mod = m % 1440;
                 st.step(mod, in);
                 auto at = [&](int h, int mi) { return mod == h * 60 + mi; };
                 if (at(20, 4) && st.on(0)) return std::string("Lampe vor der Dunkelheit an");
                 if (at(20, 5) && (!st.on(0) || !st.on(1))) return std::string("Lampe oder Kerze um 20:05 aus");
                 if (at(21, 0) && st.outside_strength("haus") != pixelia::kMaxOutsideStrength)
                     return std::string("Fenster nicht voll hell (Summe der Stärken)");
                 if (at(22, 40) && st.on(1)) return std::string("Kerze nach 2½ Stunden nicht abgebrannt");
                 if (at(21, 0) && st.phase(2) != 2) return std::string("Herd um 21 Uhr aus");
                 if (at(22, 30) && (st.phase(2) != 1 || !st.covered(2))) return std::string("Herd um 22:30 nicht abgedeckt");
                 if (at(23, 0) && st.on(0)) return std::string("Lampe nach 23 Uhr an");
                 if (at(5, 59) && st.phase(2) != 1) return "Glut hält die Nacht nicht (" + std::to_string(st.heat(2)) + ")";
                 if (at(5, 45) && !st.on(0)) return std::string("morgens vor Sonnenaufgang kein Licht");
                 if (at(6, 0) && st.phase(2) != 2) return std::string("Herd um 6 Uhr nicht angezündet");
                 if (at(7, 0) && st.on(0)) return std::string("Lampe bei Tag an");
             }
             in.lead_house = "h";  // die Begleiterin ist daheim: keine Kerze
             for (int m = 20 * 60; m < 21 * 60; ++m) st.step(m, in);
             return st.on(1) ? std::string("Kerze brennt, obwohl alle daheim sind") : std::string();
         }},
        {"Nässe und Becken: Regen macht nass und füllt, Trocknen dauert Stunden, Schöpfen leert, unter Tage kein Regen",
         [] {
             static constexpr StateDef out[] = {{.kind = StateKind::Wetness, .id = "boden"},
                                                {.kind = StateKind::Fill, .id = "becken", .capacity = 400, .rain = 120}};
             static constexpr StateDef under[] = {{.kind = StateKind::Wetness, .id = "boden"}};
             PlaceStates st;
             st.declare("platz", out, 2, nullptr, true);
             st.declare("stollen", under, 1, nullptr, false);
             pixelia::StateInput in;
             in.rain = 255;
             for (int m = 600; m < 630; ++m) st.step(m, in);
             if (st.phase(0) != 3 || st.value(0) < 990) return "nach 30 Minuten Regen nicht nass: " + std::to_string(st.value(0));
             if (st.value(1) < 255) return "Becken füllt sich nicht: " + std::to_string(st.value(1));
             if (st.value(2) != 0) return std::string("unter Tage nass vom Regen");
             in.rain = 0;
             for (int m = 630; m < 690; ++m) st.step(m, in);
             if (st.value(0) < 500 || st.value(0) > 700) return "eine Stunde Sonne: " + std::to_string(st.value(0));
             for (int m = 690; m < 900; ++m) st.step(m, in);
             if (st.value(0) != 0) return std::string("nach Stunden noch nass");
             const int before = st.value(1);
             if (st.draw(1, 10) != 10 || st.value(1) != before - 10) return std::string("Schöpfen leert nicht");
             st.set_value(1, 3);
             return st.draw(1, 10) == 3 && st.phase(1) == 0 ? std::string() : std::string("leeres Becken gibt Wasser");
         }},
        {"Ortszustände laufen unsichtbar gleich: Raum sichtbar oder nie gesehen, 8 Weltstunden",
         [] {
             StageRun vis(21, "testort-raum"), inv(21, "scrolltest");
             for (StageRun* r : {&vis, &inv}) {
                 world_run(*r, 15, 0, 600.0);  // 20 Weltsekunden je Schritt
                 r->run(1500);
             }
             const std::string a = states_text(vis.stage.environment().states, false);
             const std::string b = states_text(inv.stage.environment().states, false);
             if (a != b) return "sichtbar: " + a + "\n          unsichtbar: " + b;
             const auto& st = vis.stage.environment().states;
             const int ofen = st.find("testort-raum", "ofen");
             if (ofen < 0 || st.phase(ofen) != 1 || !st.covered(ofen)) return std::string("Ofen um 23:20 nicht abgedeckt");
             return std::string();
         }},
        {"Sprung der Uhr: 6 Weltstunden auf einmal ergeben dieselben Lichter und Feuer wie Minute für Minute",
         [] {
             StageRun step(23, "scrolltest"), jump(23, "scrolltest");
             world_run(step, 16, 0, 600.0);
             world_run(jump, 16, 0, 600.0);
             step.run(1080);  // 6 Stunden
             jump.run(1);
             jump.clock.set_world_us(step.clock.world_us());
             jump.run(1);
             // seit 0.5.9 verteilt sich das Nachholen auf mehrere Schritte (kCatchUpPerTick): weiter bei derselben Weltzeit
             for (int k = 0; k < 4 && jump.stage.catching_up() > 0; ++k) jump.stage.tick(jump.clock);
             step.run(1);
             const std::string a = states_text(step.stage.environment().states, true);
             const std::string b = states_text(jump.stage.environment().states, true);
             return a == b ? std::string() : "Minute für Minute: " + a + "\n          Sprung: " + b;
         }},
        {"Ereignis-Bus: Zustellung an der Takt-Grenze, Leser mit Maske, Überlauf gezählt",
         [] {
             pixelia::EventBus bus;
             const int all = bus.subscribe(pixelia::kAllEvents), cal = bus.subscribe(pixelia::kCalendarEvents);
             pixelia::WorldEvent e;
             e.kind = pixelia::EventKind::Arrived;
             bus.publish(e);
             if (bus.poll(all, &e)) return std::string("vor der Takt-Grenze zugestellt");
             bus.deliver();
             if (!bus.poll(all, &e) || e.kind != pixelia::EventKind::Arrived) return std::string("nicht zugestellt");
             if (bus.poll(cal, &e)) return std::string("Maske greift nicht");
             for (int i = 0; i < pixelia::EventBus::kQueue + 10; ++i) {
                 e.kind = pixelia::EventKind::First;
                 e.day = i;
                 bus.publish(e);
             }
             bus.deliver();
             int n = 0, first = -1;
             while (bus.poll(cal, &e)) {
                 if (first < 0) first = e.day;
                 ++n;
             }
             if (n != pixelia::EventBus::kQueue || first != 10 || bus.lost() != 10)
                 return "Überlauf: " + std::to_string(n) + " gelesen, verloren " + std::to_string(bus.lost());
             return std::string();
         }},
        {"Kerze für Heimkehrer am Fenster: brennt nachts, solange die Begleiterin fort ist; kommt sie heim, ist sie aus",
         [] {
             StageRun r(25, "testort-raum");
             world_run(r, 21, 0, 1.0);
             r.run(3);
             const auto& st = r.stage.environment().states;
             const int kerze = st.find("testort-raum", "kerze");
             if (kerze < 0 || !st.on(kerze)) return std::string("Kerze brennt nicht, obwohl die Begleiterin fort ist");
             auto& raum = static_cast<pixelia::TestPlace&>(r.stage.scene(r.stage.active()));
             if (raum.room().part(14).state != 1) return std::string("Kerze zeigt nicht das Bild „an“");
             pixelia::WorldEvent e;  // wie bei der Ankunft (Bühne meldet sie auf dem Bus)
             e.kind = pixelia::EventKind::Arrived;
             e.place = "testort-kammer";
             e.lead = true;
             r.stage.events().publish(e);
             r.run(31);  // seit 0.5.9 Teil 2: Regeln an der nächsten Takt-Grenze (höchstens 1 Weltsekunde)
             if (st.on(kerze)) return std::string("Kerze brennt weiter, obwohl sie heimgekommen ist");
             return raum.room().part(14).state == 0 ? std::string() : std::string("Kerzenbild bleibt „an“");
         }},
        {"Fenster glimmen orange, wenn im Raum nur der Herd brennt",
         [] {
             StageRun r(29, "testort-wiese");
             world_run(r, 21, 0, 1.0);
             r.run(3);
             auto& wiese = static_cast<pixelia::TestPlace&>(r.stage.scene(r.stage.active()));
             constexpr int kSkyGlass = 72;  // Fensterglas der Wiese (Rolle leuchtend)
             const pixelia::Color lamp = wiese.palette()[kSkyGlass];
             r.stage.command("innenlicht aus");  // Lampe und Kerze aus, der Herd brennt (Hausregel 17–22 Uhr)
             r.run(3);
             const pixelia::Color fire = wiese.palette()[kSkyGlass];
             if (!(fire.g + 30 < lamp.g && fire.r > fire.b + 120))
                 return "Fenster nicht orange: Lampe " + std::to_string(lamp.r) + "/" + std::to_string(lamp.g) + ", Herd " +
                        std::to_string(fire.r) + "/" + std::to_string(fire.g);
             return std::string();
         }},
        {"Fahrzeug nach Fahrplan: steht, fährt, Verbindungen nur am stehenden Fahrzeug offen; Orts-Graph mit Fahrzeug",
         [] {
             using pixelia::vehicle_at;
             if (pixelia::vehicle_cycle(kWagon) != 30) return std::string("Umlauf nicht 30 Minuten");
             const pixelia::VehiclePos p0 = vehicle_at(kWagon, 2), p1 = vehicle_at(kWagon, 7), p2 = vehicle_at(kWagon, 17),
                                       p3 = vehicle_at(kWagon, 30 * 1000 + 2);
             if (!p0.standing || p0.stop != 0 || p1.standing || p1.next != 1 || !p2.standing || p2.stop != 1 ||
                 !p3.standing || p3.stop != 0)
                 return std::string("Fahrplan falsch");
             pixelia::Environment env;
             env.vehicles.add(&kWagon);
             const char* why = nullptr;
             env.world_minute = 2;  // steht an Halt A
             if (!pixelia::exit_open(kExitsA[0], "halt-a", env) || pixelia::exit_open(kExitsB[0], "halt-b", env, &why))
                 return std::string("Einstieg nicht nach Fahrplan");
             if (!why || std::strcmp(why, "Fährt nur nach Fahrplan.") != 0) return std::string("Grund fehlt");
             if (!pixelia::exit_open(kExitsW[0], "test-wagen", env) || pixelia::exit_open(kExitsW[1], "test-wagen", env))
                 return std::string("Ausstieg nicht nach Fahrplan");
             env.world_minute = 7;  // unterwegs
             if (pixelia::exit_open(kExitsW[0], "test-wagen", env) || pixelia::exit_open(kExitsA[0], "halt-a", env))
                 return std::string("offen während der Fahrt");
             const pixelia::PlaceDef* list[] = {&kHaltA, &kHaltB, &kWagonPlace};
             pixelia::PlaceGraph g;
             g.build(list, 3);
             char buf[160];
             const char* err = g.check(buf, sizeof(buf));
             if (err && *err) return std::string("Graph: ") + err;
             return g.hops(0, 2) == 1 && g.hops(2, 1) == 1 ? std::string() : std::string("Wagen nicht im Graph");
         }},
        {"Unter Tage: kein Wetter, kein Himmel, gleiches Licht mittags und nachts, bei Sonne und Gewitter",
         [] {
             pixelia::Environment env;
             pixelia::TestPlace p(kUnderground, 7);
             p.set_environment(&env);
             p.init(5);
             p.enter();
             pixelia::WorldClock clk;
             auto at = [&](int h, pixelia::WeatherKind w) {
                 env.weather.set_now(w);
                 env.weather.tick();
                 clk.set_time_of_day(h, 0);
                 p.tick(clk);
                 return p.palette();
             };
             const pixelia::Palette a = at(12, pixelia::WeatherKind::Clear), b = at(0, pixelia::WeatherKind::Storm);
             for (int i = 0; i < pixelia::kSceneColors; ++i)
                 if (a[i].r != b[i].r || a[i].g != b[i].g || a[i].b != b[i].b) return "Farbe " + std::to_string(i) + " folgt Tageszeit oder Wetter";
             for (int i = 0; i < 40; ++i) p.tick(clk);
             if (p.weather().peak() != 0) return std::string("Partikel unter Tage");
             pixelia::SkyView v;
             if (p.sky_view(&v)) return std::string("Himmel unter Tage");
             const int wet = env.states.find("test-stollen", "boden");
             pixelia::StateInput in;
             in.rain = 255;
             for (int m = 0; m < 60; ++m) env.states.step(m, in);
             return wet >= 0 && env.states.value(wet) == 0 ? std::string() : std::string("unter Tage nass vom Regen");
         }},
        {"Rechenspitzen verteilt: Überblendung betritt das Ziel vor dem Mischen, mischt je Schritt einen Durchgang, Wetter-Vorlauf über Schritte",
         [] {
             StageRun r(27, "testort-wiese");
             r.stage.command("wetter schnee 100 sofort");  // Schnee braucht den längsten Vorlauf
             r.run(5);
             const int platz = r.stage.find("testort-platz");
             r.stage.go(platz, pixelia::Transition::Dissolve);
             for (int i = 0; i < 30 && !r.stage.entered(platz); ++i) r.run(1);
             if (!r.stage.entered(platz) || r.stage.transitioning())
                 return std::string("Ziel nicht vorbereitet bzw. Übergang schon begonnen");
             auto& tp = static_cast<pixelia::TestPlace&>(r.stage.scene(platz));
             const int warm1 = tp.weather().peak();
             int passes = 0;
             while (!r.stage.transitioning() && passes < 12) {
                 r.run(1);
                 ++passes;
             }
             if (!r.stage.transitioning() || passes > 5)
                 return "Übergang beginnt nicht nach Vorbereitung und Mischen (" + std::to_string(passes) + " Schritte)";
             r.run(4);
             const int warm5 = tp.weather().peak();
             return warm5 > warm1 && warm1 > 0 ? std::string()
                                               : "Vorlauf nicht verteilt: " + std::to_string(warm1) + " → " + std::to_string(warm5);
         }},
    };
}


// --- Weltzustand, Figuren und Speicherstand (seit 0.5.9): Prüfgrößen aus dem Systemplan, Grundlage für 0.5.12 ---

using pixelia::FigureBook;
using pixelia::FigureRecord;
using pixelia::Skill;
using pixelia::SkillLevel;
using Kind = pixelia::Whereabouts::Kind;

std::vector<uint8_t> save_of(const pixelia::Stage& s, uint32_t generation = 1) {
    std::vector<uint8_t> b(256 * 1024);
    b.resize(s.save_state(b.data(), b.size(), generation));
    return b;
}

std::string figures_ok(const pixelia::Stage& s) {
    char buf[160];
    const char* e = s.check_figures(buf, sizeof(buf));
    return e ? std::string(e) : std::string();
}

// Weiter wie a: Weltzeit an, gleiche Uhr, Blick folgt nicht
void resume_like(StageRun& b, const StageRun& a, int64_t plus_us = 0) {
    b.stage.set_world_time(true);
    b.stage.set_follow(false);
    b.clock.set_world_us(a.clock.world_us() + plus_us);
    b.clock.set_speed(1.0);
}

// bis eine Figur auf einer Verbindung ist (höchstens 20 Weltminuten); Kennung, 0 = keine
uint16_t run_until_link(StageRun& r) {
    for (int t = 0; t < 30 * 1200; ++t) {
        r.run(1);
        const FigureBook& book = r.stage.environment().figures;
        for (int k = 0; k < book.count(); ++k)
            if (book.at(k).where.kind == Kind::Link) return book.at(k).id;
    }
    return 0;
}

// Aufenthalt und Tätigkeit aller Figuren als Text (Vergleich zweier Abläufe)
std::string doings_text(const pixelia::Stage& s) {
    std::string t;
    const FigureBook& book = s.environment().figures;
    for (int k = 0; k < book.count(); ++k) {
        const FigureRecord& f = book.at(k);
        const pixelia::Doing& d = f.doing;
        char line[160];
        std::snprintf(line, sizeof(line), "%u:%d@%d %s %d,%d→%d,%d #%u %lld/%lld\n", static_cast<unsigned>(f.id),
                      static_cast<int>(f.where.kind), f.where.place, pixelia::doing_kind_name(d.kind), d.sx, d.sy, d.gx,
                      d.gy, static_cast<unsigned>(d.seq), static_cast<long long>(d.walk_us),
                      static_cast<long long>(d.end_us));
        t += line;
    }
    return t;
}

// erste Zeile, in der zwei Texte abweichen
std::string first_diff(const std::string& a, const std::string& b) {
    size_t k = 0;
    while (k < a.size() && k < b.size() && a[k] == b[k]) ++k;
    const size_t from = a.rfind('\n', k == 0 ? 0 : k - 1);
    const size_t start = from == std::string::npos ? 0 : from + 1;
    return a.substr(start, a.find('\n', start) - start) + " ≠ " + b.substr(start, b.find('\n', start) - start);
}

void fix_crc(std::vector<uint8_t>& d) {
    const uint32_t c = pixelia::crc32(d.data() + 8, d.size() - 8);
    for (int k = 0; k < 4; ++k) d[4 + k] = static_cast<uint8_t>(c >> (8 * k));
}

void write_file(const std::string& path, const uint8_t* p, size_t n) {
    if (FILE* f = std::fopen(path.c_str(), "wb")) {
        if (n) std::fwrite(p, 1, n, f);
        std::fclose(f);
    }
}

constexpr const char* kFixture = "sim/testdata/welt-0.5.9.pxw";

std::vector<Case> save_cases() {
    return {
        {"Startbesetzung im Weltzustand: 14 Figuren, jede in 20 Weltminuten jederzeit genau einmal (Ort oder Verbindung)",
         [] {
             StageRun r(7, "testort-wiese");
             world_run(r, 11, 0, 1.0);
             const FigureBook& book = r.stage.environment().figures;
             if (book.count() != 14) return "Figuren: " + std::to_string(book.count());
             if (!book.lead() || r.stage.lead() != book.lead()) return std::string("Hauptfigur fehlt");
             int links = 0;
             for (int t = 0; t < 30 * 1200; ++t) {
                 r.run(1);
                 if (std::string e = figures_ok(r.stage); !e.empty()) return "nach " + std::to_string(t) + " Schritten: " + e;
                 links = std::max(links, r.stage.in_transit());
             }
             if (book.count() != 14) return "danach " + std::to_string(book.count()) + " Figuren";
             if (r.stage.travels() < 5 || links == 0)
                 return "kaum Reisen: " + std::to_string(r.stage.travels()) + ", unterwegs höchstens " + std::to_string(links);
             return std::string();
         }},
        {"Fähigkeiten: Unbekanntes misslingt meist, Meister fast nie; aus Fehlversuchen lernt man, erstes Mal bleibt in Erinnerung",
         [] {
             FigureBook book;
             pixelia::FigureDef d{};
             d.name = "Lehrling";
             const uint16_t a = book.add(d, 0)->id;
             d.name = "Meisterin";
             d.skills[0] = {Skill::Baking, SkillLevel::Master};
             const uint16_t m = book.add(d, 0)->id;
             pixelia::Rng rng(5);
             int fail_a = 0, fail_m = 0, early = 0, late = 0;
             for (int i = 0; i < 20; ++i) {
                 fail_a += book.try_skill(a, Skill::Baking, 1, rng, 1, 600).outcome != pixelia::Outcome::Success;
                 fail_m += book.try_skill(m, Skill::Baking, 1, rng, 1, 600).outcome != pixelia::Outcome::Success;
             }
             if (fail_a < 12 || fail_m > 4)
                 return "Fehlversuche von 20: Lehrling " + std::to_string(fail_a) + ", Meisterin " + std::to_string(fail_m);
             for (int i = 0; i < 300; ++i) {
                 const bool ok = book.try_skill(a, Skill::Baking, 1, rng, 2, 600).outcome == pixelia::Outcome::Success;
                 if (i < 60)
                     early += ok;
                 else if (i >= 240)
                     late += ok;
             }
             const FigureRecord& f = *book.find(a);
             if (f.level(Skill::Baking) < SkillLevel::Practiced)
                 return std::string("nach 320 Versuchen erst ") + pixelia::skill_level_title(f.level(Skill::Baking));
             if (late <= early) return "kein Fortschritt: " + std::to_string(early) + " → " + std::to_string(late) + " von 60";
             bool first = false, failure = false, up = false;
             for (const pixelia::Memory& x : f.memories) {
                 first = first || x.kind == pixelia::MemoryKind::First;
                 failure = failure || x.kind == pixelia::MemoryKind::Failure;
                 up = up || x.kind == pixelia::MemoryKind::SkillUp;
             }
             if (!first || !failure || !up) return std::string("Erinnerungen fehlen (erstes Mal, Fehlversuch, Stufe)");
             return std::string();
         }},
        {"Gezeigt bekommen: nur wer es besser kann; Lernen und Beziehung wachsen zwischen beiden",
         [] {
             FigureBook book;
             pixelia::FigureDef d{};
             d.name = "Sykora";
             const uint16_t s = book.add(d, 0)->id;
             d.name = "Konrad";
             d.skills[0] = {Skill::Smithing, SkillLevel::Master};
             const uint16_t k = book.add(d, 0)->id;
             if (book.teach(s, k, Skill::Smithing, 60, 1, 600)) return std::string("Lehrling zeigt der Meisterin");
             if (!book.teach(k, s, Skill::Smithing, 120, 1, 600)) return std::string("Meister kann nicht zeigen");
             const FigureRecord& f = *book.find(s);
             if (f.level(Skill::Smithing) != SkillLevel::Beginner) return std::string("nach 2 Stunden nicht Anfänger");
             const pixelia::Relation* r1 = f.relation(k);
             const pixelia::Relation* r2 = book.find(k)->relation(s);
             if (!r1 || !r2 || r1->bond != pixelia::Bond::Known || r2->bond != pixelia::Bond::Known)
                 return std::string("Beziehung nicht beidseitig „bekannt“");
             return std::string();
         }},
        {"Beziehungen: Stufen hoch und runter, beide Seiten, je Paar ein Ereignis, Erinnerung bei beiden",
         [] {
             FigureBook book;
             pixelia::EventBus bus;
             book.set_bus(&bus);
             const int rd = bus.subscribe(pixelia::event_bit(pixelia::EventKind::BondChanged));
             pixelia::FigureDef d{};
             d.name = "Greta";
             const uint16_t a = book.add(d, 0)->id;
             d.name = "Jakob";
             const uint16_t b = book.add(d, 0)->id;
             book.bond(a, b, 75, 3, 700);  // fremd → bekannt (10) → vertraut (60), 5 übrig
             book.bond(a, b, -30, 4, 700);  // zurück auf bekannt
             bus.deliver();
             int events = 0;
             for (pixelia::WorldEvent e; bus.poll(rd, &e);) events += e.who == a && e.other == b;
             const pixelia::Relation* r1 = book.find(a)->relation(b);
             const pixelia::Relation* r2 = book.find(b)->relation(a);
             if (!r1 || !r2 || r1->bond != pixelia::Bond::Known || r2->bond != pixelia::Bond::Known || r1->points != 35)
                 return std::string("Stufe oder Punkte falsch");
             if (events != 2) return "Ereignisse: " + std::to_string(events);
             int mem = 0;
             for (uint16_t id : {a, b})
                 for (const pixelia::Memory& x : book.find(id)->memories)
                     mem += x.kind == pixelia::MemoryKind::BondUp || x.kind == pixelia::MemoryKind::BondDown;
             return mem == 4 ? std::string() : "Erinnerungen: " + std::to_string(mem);
         }},
        {"Erinnerungen: ist der Platz voll, weicht die unwichtigste, bei Gleichstand die älteste",
         [] {
             FigureBook book;
             pixelia::FigureDef d{};
             d.name = "Martha";
             const uint16_t a = book.add(d, 0)->id;
             for (int i = 0; i < FigureRecord::kMemories; ++i) {
                 pixelia::Memory m;
                 m.kind = pixelia::MemoryKind::Event;
                 m.weight = 50;
                 m.day = 10 + i;
                 book.remember(a, m);
             }
             pixelia::Memory low, same, high;
             low.kind = same.kind = high.kind = pixelia::MemoryKind::Visit;
             low.weight = 10;
             same.weight = 50;
             same.day = 99;
             high.weight = 200;
             high.day = 100;
             if (book.remember(a, low)) return std::string("unwichtige Erinnerung verdrängt eine wichtigere");
             if (!book.remember(a, same) || !book.remember(a, high)) return std::string("nicht eingetragen");
             int oldest = 1 << 30;
             for (const pixelia::Memory& x : book.find(a)->memories) oldest = std::min<int>(oldest, x.day);
             return oldest == 12 ? std::string() : "älteste übrige vom Tag " + std::to_string(oldest) + " statt 12";
         }},
        {"Speicherstand: Speichern, in eine andere Welt laden und wieder speichern ergibt dieselben Bytes",
         [] {
             StageRun a(11, "testort-wiese");
             world_run(a, 11, 0, 1.0);
             a.run(30 * 300);
             FigureBook& book = a.stage.environment().figures;
             book.learn(book.at(3).id, Skill::Baking, 400, 1, 660);
             book.bond(book.at(1).id, book.at(2).id, 70, 1, 660);
             a.stage.environment().flags.set("brunnen", true);
             a.run(2);
             const std::vector<uint8_t> s1 = save_of(a.stage, 4);
             if (s1.empty()) return std::string("Speichern: Puffer zu klein");
             StageRun b(99, "testort-raum");
             pixelia::Stage::LoadReport rep;
             if (!b.stage.load_state(s1.data(), s1.size(), &rep)) return std::string("Laden: ") + rep.why;
             const std::vector<uint8_t> s2 = save_of(b.stage, 4);
             if (s1 != s2) {
                 size_t k = 0;
                 while (k < s1.size() && k < s2.size() && s1[k] == s2[k]) ++k;
                 return "Bytes weichen ab bei " + std::to_string(k) + " von " + std::to_string(s1.size());
             }
             if (rep.figures != 14 || rep.chronicle == 0) return "Bericht: " + std::to_string(rep.figures) + " Figuren";
             if (!b.stage.environment().flags.has("brunnen")) return std::string("Merker fehlt");
             std::string e = figures_ok(b.stage);
             return e.empty() ? std::string() : "nach dem Laden: " + e;
         }},
        {"Neustart mitten auf dem Weg: die Figur auf der Verbindung kommt nach dem Laden genau einmal an",
         [] {
             StageRun a(5, "testort-wiese");
             world_run(a, 11, 0, 1.0);
             const uint16_t who = run_until_link(a);
             if (!who) return std::string("niemand unterwegs");
             const int64_t arrive = a.stage.environment().figures.find(who)->where.arrive_us;
             const std::vector<uint8_t> s = save_of(a.stage);
             StageRun b(5, "testort-wiese");
             if (!b.stage.load_state(s.data(), s.size())) return std::string("Laden");
             resume_like(b, a);
             const int rd = b.stage.events().subscribe(pixelia::event_bit(pixelia::EventKind::Arrived));
             int arrivals = 0;
             for (int t = 0; t < 30 * 60 && b.clock.world_us() < arrive + 2000000; ++t) {
                 b.run(1);
                 if (std::string e = figures_ok(b.stage); !e.empty()) return e;
                 for (pixelia::WorldEvent e; b.stage.events().poll(rd, &e);) arrivals += e.who == who;
             }
             b.run(1);
             for (pixelia::WorldEvent e; b.stage.events().poll(rd, &e);) arrivals += e.who == who;
             const FigureRecord* f = b.stage.environment().figures.find(who);
             if (!f || f->where.kind != Kind::Place) return std::string("nicht angekommen");
             if (arrivals != 1) return "Ankünfte: " + std::to_string(arrivals);
             return b.stage.environment().figures.count() == 14 ? std::string() : std::string("Zahl der Figuren");
         }},
        // --- Stufen der Simulation (seit 0.5.9 Teil 2) ---
        {"P10 gleiche Wirkung: Orte ein- und ausblenden ergibt Differenz 0 zwischen voller und grober Stufe (Echtzeit und Zeitraffer 60)",
         [] {
             // drei Abläufe mit gleichem Startwert: kein Testort zu sehen (alles grob), immer die Wiese, alle 7 Weltsekunden
             // ein anderer Ort (Schnitt, Überblendung, Abblenden: dabei zwei Orte zugleich voll)
             for (const double speed : {1.0, 60.0}) {
                 const int ticks = speed == 1.0 ? 30 * 900 : 30 * 240;  // 15 Weltminuten bzw. 4 Weltstunden
                 std::vector<uint8_t> save[3];
                 std::string doings[3];
                 uint32_t decisions = 0, travels = 0;
                 for (int run = 0; run < 3; ++run) {
                     StageRun r(31, run == 0 ? "scrolltest" : "testort-wiese");
                     world_run(r, 17, 30, speed);
                     const pixelia::Transition kinds[] = {pixelia::Transition::Cut, pixelia::Transition::Dissolve,
                                                          pixelia::Transition::Fade};
                     for (int t = 0, k = 0; t < ticks; t += 7 * 30, ++k) {
                         r.run(std::min(7 * 30, ticks - t));
                         if (run == 2) r.stage.go(1 + (k * 3) % (r.stage.count() - 1), kinds[k % 3]);
                         if (std::string e = figures_ok(r.stage); !e.empty()) return "Lauf " + std::to_string(run) + ": " + e;
                     }
                     // verglichen wird genau an einer Takt-Grenze: dazwischen ist der sichtbare Ort schon weiter
                     const int64_t g = pixelia::Stage::kGridUs;
                     r.clock.set_world_us((r.clock.world_us() / g + 1) * g);
                     r.stage.tick(r.clock);
                     save[run] = save_of(r.stage);
                     doings[run] = doings_text(r.stage);
                     if (run == 0) decisions = r.stage.decisions(), travels = r.stage.travels();
                 }
                 const std::string at = speed == 1.0 ? "Echtzeit: " : "Zeitraffer 60: ";
                 if (decisions < 100 || travels < 3)
                     return at + "zu wenig los (" + std::to_string(decisions) + " Entscheidungen, " +
                            std::to_string(travels) + " Reisen)";
                 for (int run = 1; run < 3; ++run) {
                     if (doings[run] != doings[0])
                         return at + (run == 1 ? "Wiese sichtbar" : "Orte wechselnd") + ": " + first_diff(doings[0], doings[run]);
                     if (save[run] != save[0]) {
                         size_t k = 0;
                         while (k < save[0].size() && k < save[run].size() && save[0][k] == save[run][k]) ++k;
                         return at + "Speicherstand weicht ab bei Byte " + std::to_string(k) + " von " +
                                std::to_string(save[0].size());
                     }
                 }
             }
             return std::string();
         }},
        {"Neustart mitten in einer Übergabe (an der Tür, auf dem Weg): danach an genau einem Ort, Tätigkeiten gehen genau so weiter",
         [] {
             for (const pixelia::DoingKind wait : {pixelia::DoingKind::Leave, pixelia::DoingKind::None}) {
                 // a läuft bis jemand an der Tür wartet (bzw. auf einer Verbindung ist), wird gespeichert und läuft weiter
                 StageRun a(13, "testort-wiese");
                 world_run(a, 11, 0, 1.0);
                 uint16_t who = 0;
                 for (int t = 0; t < 30 * 1200 && !who; ++t) {
                     a.run(1);
                     const FigureBook& book = a.stage.environment().figures;
                     for (int k = 0; k < book.count() && !who; ++k) {
                         const FigureRecord& f = book.at(k);
                         const bool door = f.where.kind == Kind::Place && f.doing.kind == pixelia::DoingKind::Leave &&
                                           a.clock.world_us() >= f.doing.walk_us && a.clock.world_us() < f.doing.end_us;
                         if (wait == pixelia::DoingKind::Leave ? door : f.where.kind == Kind::Link) who = f.id;
                     }
                 }
                 const std::string at = wait == pixelia::DoingKind::Leave ? "an der Tür: " : "auf dem Weg: ";
                 if (!who) return at + "niemand gefunden";
                 const std::vector<uint8_t> s = save_of(a.stage);
                 // b beginnt neu, lädt den Stand und läuft im selben Takt weiter
                 StageRun b(13, "testort-wiese");
                 pixelia::Stage::LoadReport rep;
                 if (!b.stage.load_state(s.data(), s.size(), &rep)) return at + "Laden: " + rep.why;
                 if (rep.doings == 0) return at + "keine Tätigkeit übernommen";
                 resume_like(b, a);
                 if (std::string e = figures_ok(b.stage); !e.empty()) return at + "nach dem Laden: " + e;
                 const int start_place = a.stage.environment().figures.find(who)->where.place;
                 bool moved = false;
                 for (int t = 0; t < 30 * 120; ++t) {
                     a.run(1);
                     b.run(1);
                     if (std::string e = figures_ok(b.stage); !e.empty()) return at + e;
                     const FigureRecord* f = b.stage.environment().figures.find(who);
                     // an der Tür: hinaus (anderer Ort oder Verbindung); auf dem Weg: angekommen (where.place = Ziel)
                     if (wait == pixelia::DoingKind::Leave)
                         moved = moved || f->where.kind == Kind::Link ||
                                 (f->where.kind == Kind::Place && f->where.place != start_place);
                     else
                         moved = moved || f->where.kind == Kind::Place;
                 }
                 if (!moved) return at + "Figur hat den Ort nicht gewechselt";
                 if (doings_text(a.stage) != doings_text(b.stage))
                     return at + "nach 2 Weltminuten verschieden: " + first_diff(doings_text(a.stage), doings_text(b.stage));
             }
             return std::string();
         }},
        {"Stromausfall beim Speichern: abgebrochene oder kaputte Datei, es gilt der vorige Stand; Laden stürzt nie ab",
         [] {
             const std::string dir = "build-sim-headless/test-speicher";
             std::filesystem::create_directories(dir);
             pixelia::SaveSlots slots(dir.c_str(), "welt");
             std::remove(slots.path(0));
             std::remove(slots.path(1));
             StageRun a(9, "testort-wiese");
             world_run(a, 11, 0, 1.0);
             a.run(300);
             const std::vector<uint8_t> s1 = save_of(a.stage, 1);
             int slot = -1;
             if (!slots.write(s1.data(), s1.size(), &slot) || slot != 0) return std::string("erster Platz");
             a.run(300);
             std::vector<uint8_t> s2 = save_of(a.stage, 2);
             auto loads = [&](uint32_t want) {
                 pixelia::SaveSlots fresh(dir.c_str(), "welt");
                 std::vector<uint8_t> d;
                 pixelia::SaveHeader h;
                 return fresh.load(&d, &h) && h.generation == want;
             };
             // abgebrochen nach k Byte, an jeder Stelle: der vorige Stand gilt, und der Leser lehnt die Daten ab
             for (size_t k = 0; k < s2.size(); k += k < 64 ? 1 : 37) {
                 write_file(slots.path(1), s2.data(), k);
                 if (!loads(1)) return "abgebrochen nach " + std::to_string(k) + " Byte: Stand 1 fehlt";
                 StageRun b(9, "testort-wiese");
                 if (b.stage.load_state(s2.data(), k)) return "abgebrochene Daten geladen (" + std::to_string(k) + ")";
             }
             std::vector<uint8_t> bad = s2;
             bad[bad.size() / 2] ^= 0x40;
             write_file(slots.path(1), bad.data(), bad.size());
             if (!loads(1)) return std::string("gekipptes Bit: Stand 1 fehlt");
             pixelia::SaveSlots w(dir.c_str(), "welt");
             if (!w.write(s2.data(), s2.size(), &slot) || slot != 1 || !loads(2)) return std::string("zweiter Platz");
             const std::vector<uint8_t> s3 = save_of(a.stage, 3);
             write_file(slots.path(0), s3.data(), s3.size() / 3);
             if (!loads(2)) return std::string("Stand 2 nach abgebrochenem Stand 3");
             // kaputte Inhalte mit passender Prüfsumme (Fehler im Programm): laden, ohne abzustürzen, Figuren heil
             pixelia::Rng rng(77);
             for (int i = 0; i < 300; ++i) {
                 std::vector<uint8_t> f = s2;
                 const int flips = 1 + static_cast<int>(rng.below(6));
                 for (int k = 0; k < flips; ++k)
                     f[pixelia::kSaveHeader + rng.below(static_cast<uint32_t>(f.size() - pixelia::kSaveHeader))] =
                         static_cast<uint8_t>(rng.next());
                 fix_crc(f);
                 StageRun b(9, "testort-wiese");
                 b.stage.load_state(f.data(), f.size());
                 resume_like(b, a);
                 b.run(3);
                 if (std::string e = figures_ok(b.stage); !e.empty()) return "Versuch " + std::to_string(i) + ": " + e;
             }
             return std::string();
         }},
        {"Alter Stand lädt weiter: Speicherstand 0.5.9 aus dem Repo, fehlende und unbekannte Abschnitte, Ort gibt es nicht mehr",
         [] {
             if (std::getenv("PIXELIA_FIXTURE")) {
                 // Vorlage einmal erzeugen (0.5.9): mit Figur unterwegs, Erinnerungen, Chronik
                 StageRun a(5, "testort-wiese");
                 world_run(a, 11, 0, 1.0);
                 FigureBook& book = a.stage.environment().figures;
                 book.learn(book.at(4).id, Skill::Joinery, 340, 1, 660);
                 book.bond(book.at(4).id, book.at(5).id, 80, 1, 660);
                 if (!run_until_link(a)) return std::string("niemand unterwegs");
                 const std::vector<uint8_t> s = save_of(a.stage, 1);
                 write_file(kFixture, s.data(), s.size());
             }
             std::vector<uint8_t> old;
             if (FILE* f = std::fopen(kFixture, "rb")) {
                 uint8_t buf[4096];
                 for (size_t k; (k = std::fread(buf, 1, sizeof(buf), f)) > 0;) old.insert(old.end(), buf, buf + k);
                 std::fclose(f);
             }
             if (old.empty()) return std::string("Vorlage ") + kFixture + " fehlt";
             StageRun b(1, "testort-wiese");
             pixelia::Stage::LoadReport rep;
             if (!b.stage.load_state(old.data(), old.size(), &rep)) return std::string("Vorlage: ") + rep.why;
             if (rep.figures != 14 || rep.transit < 1) return "Vorlage: " + std::to_string(rep.figures) + " Figuren";
             if (std::string e = figures_ok(b.stage); !e.empty()) return "Vorlage: " + e;
             // nur Weltzeit und ein unbekannter Abschnitt: der Rest bleibt beim Anfangswert
             std::vector<uint8_t> buf(1024);
             pixelia::SaveWriter w(buf.data(), buf.size());
             w.begin(1, b.clock.world_us());
             w.chunk("WELT", 1);
             w.u64(1);
             w.i64(INT64_MIN);
             w.u16(100);
             w.chunk("ZZZZ", 7);
             w.u32(12345);
             buf.resize(w.finish());
             StageRun c(1, "testort-wiese");
             if (!c.stage.load_state(buf.data(), buf.size(), &rep) || rep.skipped_chunks != 1)
                 return std::string("unbekannter Abschnitt nicht übersprungen");
             if (c.stage.environment().figures.count() != 14) return std::string("Startbesetzung ohne Abschnitt FIGS weg");
             // Ort umbenannt (gibt es nicht mehr): seine Figuren kommen in den ersten Ort, keine geht verloren
             std::vector<uint8_t> moved = old;
             const std::string from = "testort-kammer", to = "testort-xammer";
             auto it = std::search(moved.begin(), moved.end(), from.begin(), from.end());
             if (it == moved.end()) return std::string("Kammer nicht in der Vorlage");
             std::copy(to.begin(), to.end(), it);
             fix_crc(moved);
             StageRun m(1, "testort-wiese");
             if (!m.stage.load_state(moved.data(), moved.size(), &rep) || rep.unknown_places == 0)
                 return std::string("unbekannter Ort nicht erkannt");
             if (m.stage.environment().figures.count() != 14) return std::string("Figur verloren");
             std::string e = figures_ok(m.stage);
             return e.empty() ? std::string() : "umbenannter Ort: " + e;
         }},
        {"Nachholen: 10 Stunden aus ergibt dieselben Lichter und Feuer; mehr als 3 Tage: Rest übersprungen, Chronik nennt die Pause",
         [] {
             StageRun a(23, "scrolltest");
             world_run(a, 16, 0, 600.0);
             a.run(3);
             const std::vector<uint8_t> s = save_of(a.stage);
             a.run(1800);  // 10 Stunden Minute für Minute
             StageRun b(23, "scrolltest");
             if (!b.stage.load_state(s.data(), s.size())) return std::string("Laden");
             resume_like(b, a);
             b.run(1);
             for (int k = 0; k < 8 && b.stage.catching_up() > 0; ++k) b.stage.tick(b.clock);
             if (b.stage.catching_up() > 0) return std::string("Nachholen nicht fertig");
             a.run(1);
             const std::string x = states_text(a.stage.environment().states, true);
             const std::string y = states_text(b.stage.environment().states, true);
             if (x != y) return "Minute für Minute: " + x + "\n          nachgeholt: " + y;
             const pixelia::Chronicle& ch = b.stage.environment().chronicle;
             const std::string wake = ch.count() ? ch.at(ch.count() - 1).text : "";
             if (wake.find("Pixelia war 10 Std.") == std::string::npos) return "Chronik: " + wake;
             StageRun c(23, "scrolltest");
             pixelia::Stage::LoadReport rep;
             c.stage.load_state(s.data(), s.size(), &rep);
             resume_like(c, a, 4LL * 24 * 3600 * 1000000);  // 4 Tage und 10 Stunden aus
             for (int k = 0; k < 40 && (k < 2 || c.stage.catching_up() > 0); ++k) c.run(1);
             const int64_t gap = (c.clock.world_us() - rep.saved_us) / 60000000;
             const int64_t skipped = c.stage.skipped(), want = gap - pixelia::Stage::kCatchUpMinutes;
             if (skipped < want - 2 || skipped > want + 2)
                 return "übersprungen " + std::to_string(skipped) + " statt " + std::to_string(want) + " Minuten";
             const pixelia::Chronicle& cc = c.stage.environment().chronicle;
             const std::string w5 = cc.count() ? cc.at(cc.count() - 1).text : "";
             return w5.find("nachgeholt: 3 Tage") != std::string::npos ? std::string() : "Chronik: " + w5;
         }},
        {"Chronik und Besuche: Einträge mit Zeitstempel, neue Stufen erscheinen, alles übersteht Speichern und Laden",
         [] {
             StageRun a(3, "testort-wiese");
             world_run(a, 11, 0, 1.0);
             a.run(30 * 600);
             FigureBook& book = a.stage.environment().figures;
             const uint16_t who = book.at(2).id;
             book.learn(who, Skill::Music, 400, a.stage.environment().calendar.date().day, 671);
             a.run(2);
             const pixelia::Chronicle& ch = a.stage.environment().chronicle;
             bool start = false, skill = false;
             for (int i = 0; i < ch.count(); ++i) {
                 start = start || ch.at(i).kind == pixelia::ChronicleKind::Start;
                 skill = skill || std::string(ch.at(i).text).find("Musizieren jetzt") != std::string::npos;
             }
             if (!start || !skill) return std::string("Chronik ohne Start oder neue Stufe");
             const pixelia::VisitLog& vs = a.stage.environment().visits;
             if (vs.count() == 0) return std::string("keine Besuche in 10 Minuten");
             const std::vector<uint8_t> s = save_of(a.stage);
             StageRun b(4, "testort-raum");
             b.stage.load_state(s.data(), s.size());
             const pixelia::Chronicle& cb = b.stage.environment().chronicle;
             if (cb.count() != ch.count() || std::strcmp(cb.at(cb.count() - 1).text, ch.at(ch.count() - 1).text) != 0)
                 return std::string("Chronik nach dem Laden anders");
             const pixelia::VisitLog& vb = b.stage.environment().visits;
             if (vb.count() != vs.count() || vb.at(0).who != vs.at(0).who || vb.at(0).from != vs.at(0).from)
                 return std::string("Besuche nach dem Laden anders");
             return std::string();
         }},
    };
}

int run_group(const char* title, const std::vector<Case>& cases) {
    int failed = 0;
    for (const auto& c : cases) {
        std::string err = c.run();
        if (err.empty()) {
            std::printf("  ok    %s\n", c.name);
        } else {
            ++failed;
            std::printf("  FEHLER %s: %s\n", c.name, err.c_str());
        }
    }
    const int total = static_cast<int>(cases.size());
    std::printf("sim: %s %d/%d bestanden\n", title, total - failed, total);
    return failed;
}

}  // namespace

int run_tests(const std::string& which) {
    if (which != "gestures" && which != "redact" && which != "stage" && which != "assets" && which != "overlay" &&
        which != "welt" && which != "orte" && which != "speicher" && which != "all") {
        std::printf("FEHLER: Test \"%s\" gibt es nicht (gestures, redact, stage, assets, overlay, welt, orte, speicher, all)\n",
                    which.c_str());
        return 2;
    }
    int failed = 0;
    if (which == "gestures" || which == "all") failed += run_group("Gestentest", gesture_cases());
    if (which == "redact" || which == "all") failed += run_group("Passwort-Filter", redact_cases());
    if (which == "stage" || which == "all") failed += run_group("Bühne", stage_cases());
    if (which == "assets" || which == "all") failed += run_group("Asset-Pakete", asset_cases());
    if (which == "overlay" || which == "all") failed += run_group("Einblendungen, Touch und Kamera", overlay_cases());
    if (which == "welt" || which == "all") failed += run_world_tests();
    if (which == "orte" || which == "all") {
        failed += run_group("Orte und Gebiete", place_cases());
        failed += run_group("Ortszustände, Ereignis-Bus, Fahrzeuge", state_cases());
    }
    if (which == "speicher" || which == "all") failed += run_group("Weltzustand, Figuren und Speicherstand", save_cases());
    return failed ? 1 : 0;
}

}  // namespace sim
