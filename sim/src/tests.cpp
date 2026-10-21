// Selbsttests der Engine-Bausteine ohne Hardware (--test), laufen auch in der CI.
// gestures: Gestenerkennung (Tippen, langer Druck) mit nachgestellten Touch-Abläufen wie vom GT911.
// redact: Passwort-Filter der Diagnose-Ausgaben (Blockgrenzen, JSON-Schreibweise, mehrere Geheimnisse).
// stage: Bühne und Figuren-Bausteine (Determinismus, Rechnen ohne Zeichnen, Übergänge, Farbreihen, Animationen).

#include <cstring>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

#include "pixelia/engine/light.h"
#include "pixelia/engine/color.h"
#include "pixelia/engine/gesture.h"
#include "pixelia/engine/redact.h"
#include "pixelia/engine/sprite.h"
#include "pixelia/engine/stage.h"
#include "pixelia/engine/test_places.h"
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

std::vector<Case> gesture_cases() {
    return {
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
        {"Überblendung dauert 18 Schritte, die Mitte zeigt etwa zur Hälfte den neuen Ort",
         [] {
             StageRun a(9), cut(9);
             a.run(30);
             cut.run(30);
             a.stage.go(1, Transition::Dissolve);
             cut.stage.go(1, Transition::Cut);
             a.run(9);
             cut.run(9);
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
             pixelia::TestPlace p(pixelia::TestPlace::Kind::Room);
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
             pixelia::TestPlace p(pixelia::TestPlace::Kind::Room);
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
             auto total = [](StageRun& r) {
                 int n = 0;
                 for (const char* s : {"testort-wiese", "testort-raum", "testort-platz"})
                     n += static_cast<pixelia::TestPlace&>(r.stage.scene(r.stage.find(s))).figure_count();
                 return n;
             };
             StageRun a(21, "testort-wiese");
             const int start = total(a);
             int changes = 0, last = a.stage.active();
             for (int k = 0; k < 400; ++k) {
                 a.run(30);
                 if (a.stage.active() != last) ++changes, last = a.stage.active();
                 int guides = 0;
                 for (const char* s : {"testort-wiese", "testort-raum", "testort-platz"})
                     guides += static_cast<pixelia::TestPlace&>(a.stage.scene(a.stage.find(s))).has_guide();
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
             pixelia::TestPlace p(pixelia::TestPlace::Kind::Room);
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
                 r->run(2);
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
             r.run(1);
             if (lit() != 0 || place(w).room().light_count() != 0) return std::string("Fensterschein trotz Licht aus");
             if (place(raum).room().light_count() != 0 || place(raum).lights_on())
                 return std::string("unsichtbarer Raum: Licht noch an");
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
    if (which != "gestures" && which != "redact" && which != "stage" && which != "all") {
        std::printf("FEHLER: Test \"%s\" gibt es nicht (gestures, redact, stage, all)\n", which.c_str());
        return 2;
    }
    int failed = 0;
    if (which == "gestures" || which == "all") failed += run_group("Gestentest", gesture_cases());
    if (which == "redact" || which == "all") failed += run_group("Passwort-Filter", redact_cases());
    if (which == "stage" || which == "all") failed += run_group("Bühne", stage_cases());
    return failed ? 1 : 0;
}

}  // namespace sim
