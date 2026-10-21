// Selbsttests der Engine-Bausteine ohne Hardware (--test), laufen auch in der CI.
// gestures: Gestenerkennung (Tippen, langer Druck) mit nachgestellten Touch-Abläufen wie vom GT911.
// redact: Passwort-Filter der Diagnose-Ausgaben (Blockgrenzen, JSON-Schreibweise, mehrere Geheimnisse).
// stage: Bühne und Figuren-Bausteine (Determinismus, Rechnen ohne Zeichnen, Übergänge, Farbreihen, Animationen).

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

#include "pixelia/engine/color.h"
#include "pixelia/engine/gesture.h"
#include "pixelia/engine/redact.h"
#include "pixelia/engine/sprite.h"
#include "pixelia/engine/stage.h"
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
