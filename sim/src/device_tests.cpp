// Prüfungen zu den Abläufen der Firmware (--test geraet, seit 0.5.11 „Fester Boden“ Teil c): Selbsttest im
// Probelauf (V07, V08), Update-Sperre (B05) und Änderungskennzeichen (B23) ohne SDK, mit der richtigen Erwartung.

#include <atomic>
#include <cstdio>
#include <functional>
#include <string>
#include <thread>
#include <vector>

#include "pixelia/engine/device_flow.h"
#include "sim.h"

namespace sim {
namespace {

using pixelia::SaveMark;
using pixelia::TrialMachine;
using pixelia::TrialSample;
using pixelia::UpdateGate;
using Phase = TrialMachine::Phase;
using Action = TrialMachine::Action;

struct Case {
    const char* name;
    std::function<std::string()> run;
};

// Gerät im Probelauf: Panel und Engine zählen mit den angegebenen Raten, die Selbsttest-Task kommt alle step_ms dran
struct Rig {
    double panel_hz = 60;  // erwartet (aus dem Timing)
    int swap = 2;
    double real_panel = 60;   // tatsächlich
    double real_engine = 30;  // tatsächlich
    bool net = true;
    int64_t now = 5000000;    // Start nicht bei 0
    double vs = 0, fr = 0;
    uint32_t vs_base = 0;     // Zählerstand beim Start (Überlauf prüfen)

    TrialSample sample() const {
        TrialSample s;
        s.now_us = now;
        s.vsyncs = vs_base + static_cast<uint32_t>(vs);
        s.frames = static_cast<uint32_t>(fr);
        s.panel_hz = panel_hz;
        s.swap = swap;
        s.net_ok = net;
        return s;
    }
    void advance(int ms) {
        now += static_cast<int64_t>(ms) * 1000;
        vs += real_panel * ms / 1000.0;
        fr += real_engine * ms / 1000.0;
    }
};

// Lässt den Selbsttest laufen, bis er etwas verlangt; confirm/rollback liefern die Ergebnisse des „SDK“
struct Run {
    std::vector<std::pair<int, Action>> actions;  // Sekunde, Aktion
    int passed_at = -1;
    Phase end = Phase::Running;
};

Run drive(Rig& r, TrialMachine& m, const std::function<bool(int)>& confirm_ok, const std::function<bool(int)>& rollback_ok,
          int max_s = 600, int step_ms = 1000) {
    Run out;
    m.start(r.sample());
    int confirms = 0, rollbacks = 0;
    for (int t = 0; t < max_s * 1000 && !m.finished(); t += step_ms) {
        r.advance(step_ms);
        const Action a = m.step(r.sample());
        if (a == Action::None) continue;
        const int sec = static_cast<int>((r.now - 5000000) / 1000000);
        out.actions.push_back({sec, a});
        if (a == Action::MarkValid) {
            if (m.phase() == Phase::Passed) return out;  // darf nicht vor dem Ergebnis bestanden sein
            m.confirm_result(confirm_ok(confirms++), r.now, 0x105);
            if (m.phase() == Phase::Passed) out.passed_at = sec;
        } else {
            if (rollback_ok(rollbacks++)) {  // Neustart: Ablauf endet hier
                out.end = Phase::RollingBack;
                return out;
            }
            m.rollback_failed(r.now, 0x103);
        }
    }
    out.end = m.phase();
    return out;
}

std::string expect_pass(const Run& run, const TrialMachine& m, int min_s, int max_s) {
    if (m.phase() != Phase::Passed) {
        return std::string("nicht bestanden: ") + pixelia::trial_phase_name(m.phase()) + ", " + m.note();
    }
    if (run.passed_at < min_s || run.passed_at > max_s) return "bestanden nach " + std::to_string(run.passed_at) + " s";
    for (const auto& a : run.actions) {
        if (a.second == Action::Rollback) return "Rückfall angefordert";
    }
    return {};
}

std::string expect_rollback(const Run& run, int at_s) {
    if (run.end != Phase::RollingBack) return std::string("kein Rückfall, Ende ") + pixelia::trial_phase_name(run.end);
    for (const auto& a : run.actions) {
        if (a.second == Action::MarkValid) return "Bestätigen angefordert nach " + std::to_string(a.first) + " s";
    }
    if (run.actions.back().first != at_s) return "Rückfall nach " + std::to_string(run.actions.back().first) + " s";
    return {};
}

const auto kOk = [](int) { return true; };
const auto kFail = [](int) { return false; };

std::vector<Case> cases() {
    return {
        {"Standard-Timing 60 Hz, Engine 30/s: bestanden nach 60 s",
         [] {
             Rig r;
             TrialMachine m;
             const Run run = drive(r, m, kOk, kOk);
             return expect_pass(run, m, 60, 60);
         }},
        {"Stromsparmodus 30 Hz (Engine-Bild 1× gezeigt), Engine 30/s: bestanden (V07)",
         [] {
             Rig r;
             r.panel_hz = 30, r.real_panel = 30, r.swap = 1;
             TrialMachine m;
             const Run run = drive(r, m, kOk, kOk);
             if (auto e = expect_pass(run, m, 60, 60); !e.empty()) return e;
             // Gegenprobe zur alten Regel: 30 Panelbilder je Sekunde hätten die feste Grenze 50 nie erreicht
             return m.panel_rate() >= 50 ? std::string("Messung falsch: ") + std::to_string(m.panel_rate()) : std::string();
         }},
        {"Eigenes Timing 45 Hz (2×): bestanden; Stromsparmodus mit 30 Hz erwartet, aber Panel steht: Rückfall",
         [] {
             Rig r;
             r.panel_hz = 45, r.real_panel = 45, r.real_engine = 22.5;
             TrialMachine m;
             if (auto e = expect_pass(drive(r, m, kOk, kOk), m, 60, 60); !e.empty()) return "45 Hz: " + e;
             Rig s;
             s.panel_hz = 30, s.swap = 1, s.real_panel = 0;
             TrialMachine n;
             return expect_rollback(drive(s, n, kOk, kOk), 120);
         }},
        {"60 Hz erwartet, Panel liefert nur 30 Hz: nicht bestanden (kein stilles Absenken der Erwartung)",
         [] {
             Rig r;
             r.real_panel = 30;
             TrialMachine m;
             const Run run = drive(r, m, kOk, kOk);
             if (auto e = expect_rollback(run, 120); !e.empty()) return e;
             return std::string(m.note()).find("Display") == std::string::npos ? std::string("Grund: ") + m.note()
                                                                                : std::string();
         }},
        {"Engine steht (Panel läuft): Rückfall nach 120 s mit Grund „Engine“",
         [] {
             Rig r;
             r.real_engine = 0;
             TrialMachine m;
             const Run run = drive(r, m, kOk, kOk);
             if (auto e = expect_rollback(run, 120); !e.empty()) return e;
             return std::string(m.note()).find("Engine") == std::string::npos ? std::string("Grund: ") + m.note()
                                                                               : std::string();
         }},
        {"Task kommt unregelmäßig dran (alle 1,5 s und alle 0,7 s): Raten über die echte Zeit, bestanden",
         [] {
             for (int step : {1500, 700}) {
                 Rig r;
                 r.panel_hz = 30, r.real_panel = 30, r.swap = 1;
                 TrialMachine m;
                 const Run run = drive(r, m, kOk, kOk, 600, step);
                 if (auto e = expect_pass(run, m, 60, 62); !e.empty()) return std::to_string(step) + " ms: " + e;
             }
             return std::string();
         }},
        {"Überlauf des VSync-Zählers während des Tests: bestanden",
         [] {
             Rig r;
             r.vs_base = 0xFFFFFFFFu - 1000;
             TrialMachine m;
             return expect_pass(drive(r, m, kOk, kOk), m, 60, 60);
         }},
        {"Ohne WLAN: kein Bestätigen; kommt es nach 80 s, bestanden nach 80 s",
         [] {
             Rig r;
             r.net = false;
             TrialMachine m;
             m.start(r.sample());
             for (int s = 1; s <= 80; ++s) {
                 r.advance(1000);
                 if (s == 80) r.net = true;
                 const Action a = m.step(r.sample());
                 if (a != Action::None && s < 80) return "Aktion nach " + std::to_string(s) + " s";
                 if (s == 80 && a != Action::MarkValid) return std::string("kein Bestätigen nach 80 s: ") + m.note();
             }
             return std::string();
         }},
        {"Bestätigen scheitert einmal, dann klappt es: erst danach bestanden (V08)",
         [] {
             Rig r;
             TrialMachine m;
             const Run run = drive(r, m, [](int i) { return i >= 1; }, kOk);
             if (auto e = expect_pass(run, m, 65, 65); !e.empty()) return e;
             return m.confirm_attempts() == 2 ? std::string() : "Versuche " + std::to_string(m.confirm_attempts());
         }},
        {"Bestätigen scheitert dauerhaft: „gesund, aber nicht bestätigt“ nach 6 Versuchen, nie bestanden (V08)",
         [] {
             Rig r;
             TrialMachine m;
             const Run run = drive(r, m, kFail, kOk);
             if (m.phase() != Phase::ConfirmFailed) return std::string("Ende ") + pixelia::trial_phase_name(m.phase());
             if (m.confirm_attempts() != 6) return "Versuche " + std::to_string(m.confirm_attempts());
             if (run.passed_at >= 0) return std::string("als bestanden gemeldet");
             // Abstände 5, 10, 20, 40, 60 s nach dem ersten Versuch bei 60 s
             const int want[] = {60, 65, 75, 95, 135, 195};
             for (int i = 0; i < 6; ++i) {
                 if (run.actions[i].first != want[i]) {
                     return "Versuch " + std::to_string(i + 1) + " nach " + std::to_string(run.actions[i].first) + " s";
                 }
             }
             return std::string(m.note()).find("nicht dauerhaft") == std::string::npos ? std::string("Hinweis: ") + m.note()
                                                                                         : std::string();
         }},
        {"Rückfall scheitert: zwei neue Versuche, dann sichtbarer Zustand statt stillem Ende (V08)",
         [] {
             Rig r;
             r.real_panel = 0;
             TrialMachine m;
             const Run run = drive(r, m, kOk, kFail);
             if (m.phase() != Phase::RollbackFailed) return std::string("Ende ") + pixelia::trial_phase_name(m.phase());
             if (run.actions.size() != 3) return "Rückfall-Aufrufe " + std::to_string(run.actions.size());
             if (run.actions[1].first != 130 || run.actions[2].first != 140) return std::string("Abstände falsch");
             return m.last_error() == 0x103 ? std::string() : std::string("Fehlercode fehlt");
         }},
        {"Rückfall scheitert einmal, dann klappt er",
         [] {
             Rig r;
             r.real_engine = 0;
             TrialMachine m;
             const Run run = drive(r, m, kOk, [](int i) { return i >= 1; });
             return run.end == Phase::RollingBack && run.actions.size() == 2 ? std::string()
                                                                              : std::string("kein zweiter Versuch");
         }},
        {"Update-Sperre: von zwei gleichzeitigen Reservierungen gewinnt genau eine (1000 Runden, B05)",
         [] {
             for (int round = 0; round < 1000; ++round) {
                 UpdateGate g;
                 std::atomic<int> go{0};
                 uint32_t a = 0, b = 0;
                 std::thread ta([&] { while (go.load() == 0) {} a = g.reserve(); });
                 std::thread tb([&] { while (go.load() == 0) {} b = g.reserve(); });
                 go = 1;
                 ta.join();
                 tb.join();
                 if ((a != 0) == (b != 0)) return "Runde " + std::to_string(round) + ": a=" + std::to_string(a) +
                                                  " b=" + std::to_string(b);
             }
             return std::string();
         }},
        {"Update-Sperre: nur der Besitzer schreibt, schließt ab und bricht ab; danach wartet sie auf den Neustart",
         [] {
             UpdateGate g;
             const uint32_t up = g.reserve();
             if (!up || g.reserve() != 0) return std::string("zweite Reservierung möglich");
             if (g.start_writing(up + 1) || g.release(up + 1) || g.done(up)) return std::string("Fremder darf etwas");
             if (!g.start_writing(up) || !g.owns(up)) return std::string("Besitzer darf nicht schreiben");
             if (!g.release(up) || g.phase() != UpdateGate::Phase::Idle) return std::string("Abbruch setzt nicht zurück");
             if (g.owns(up) || g.start_writing(up)) return std::string("alter Besitzer schreibt nach dem Abbruch");
             const uint32_t gh = g.reserve();  // neuer Versuch nach Fehler
             if (!gh || gh == up) return std::string("neuer Versuch ohne neue Nummer");
             if (!g.start_writing(gh) || !g.done(gh)) return std::string("Abschluss scheitert");
             if (g.phase() != UpdateGate::Phase::RebootPending || g.reserve() != 0 || g.release(gh)) {
                 return std::string("nach dem Abschluss wieder frei");
             }
             return std::string();
         }},
        {"Änderungskennzeichen: Fehler beim Schreiben und Änderung während des Schreibens bleiben offen (B23)",
         [] {
             SaveMark m;
             if (m.dirty()) return std::string("frisch geändert");
             m.touch();
             uint32_t rev = m.begin();
             // Schreiben scheitert: nichts quittieren
             if (!m.dirty()) return std::string("nach Fehler sauber");
             rev = m.begin();
             m.touch();  // Änderung zwischen Lesen und Commit
             m.saved(rev);
             if (!m.dirty()) return std::string("Änderung während des Schreibens verloren");
             m.saved(m.begin());
             if (m.dirty()) return std::string("nach Erfolg noch offen");
             m.saved(rev);  // altes Ergebnis kommt spät an
             return m.dirty() ? std::string("spätes altes Ergebnis macht wieder offen") : std::string();
         }},
    };
}

}  // namespace

int run_device_tests() {
    int failed = 0;
    const auto list = cases();
    for (const auto& c : list) {
        const std::string err = c.run();
        if (err.empty()) {
            std::printf("  ok    %s\n", c.name);
        } else {
            ++failed;
            std::printf("  FEHLER %s: %s\n", c.name, err.c_str());
        }
    }
    std::printf("sim: Abläufe der Firmware %d/%d bestanden\n", static_cast<int>(list.size()) - failed,
                static_cast<int>(list.size()));
    return failed;
}

}  // namespace sim
