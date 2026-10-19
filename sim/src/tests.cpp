// Selbsttests der Engine-Bausteine ohne Hardware (--test), laufen auch in der CI.
// gestures: Gestenerkennung (Tippen, langer Druck) mit nachgestellten Touch-Abläufen wie vom GT911.
// redact: Passwort-Filter der Diagnose-Ausgaben (Blockgrenzen, JSON-Schreibweise, mehrere Geheimnisse).

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

#include "pixelia/engine/gesture.h"
#include "pixelia/engine/redact.h"
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
    if (which != "gestures" && which != "redact" && which != "all") {
        std::printf("FEHLER: Test \"%s\" gibt es nicht (gestures, redact, all)\n", which.c_str());
        return 2;
    }
    int failed = 0;
    if (which == "gestures" || which == "all") failed += run_group("Gestentest", gesture_cases());
    if (which == "redact" || which == "all") failed += run_group("Passwort-Filter", redact_cases());
    return failed ? 1 : 0;
}

}  // namespace sim
