// Selbsttests der Engine-Bausteine ohne Hardware (--test), laufen auch in der CI.
// Heute: Gestenerkennung (Tippen, langer Druck) mit nachgestellten Touch-Abläufen wie vom GT911.

#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

#include "pixelia/engine/gesture.h"
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

}  // namespace

int run_tests(const std::string& which) {
    if (which != "gestures" && which != "all") {
        std::printf("FEHLER: Test \"%s\" gibt es nicht (gestures, all)\n", which.c_str());
        return 2;
    }
    int failed = 0, total = 0;
    for (const auto& c : gesture_cases()) {
        ++total;
        std::string err = c.run();
        if (err.empty()) {
            std::printf("  ok    %s\n", c.name);
        } else {
            ++failed;
            std::printf("  FEHLER %s: %s\n", c.name, err.c_str());
        }
    }
    std::printf("sim: Gestentest %d/%d bestanden\n", total - failed, total);
    return failed ? 1 : 0;
}

}  // namespace sim
