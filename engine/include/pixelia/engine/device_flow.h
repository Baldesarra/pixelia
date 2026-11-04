#pragma once
// Abläufe der Firmware ohne SDK-Aufrufe (seit 0.5.11 „Fester Boden“, Teil c): Die Firmware ruft das SDK und gibt
// dem Ablauf nur Messwerte und Ergebnisse; so sind die Entscheidungen im Simulator prüfbar (--test geraet).
//
// - UpdateGate (B05): genau ein Update zur Zeit. Reservieren geht kurz unter einer Sperre von Idle nach Preparing,
//   vor jedem langsamen Schritt (Löschen); nur der Besitzer darf schreiben, abschließen und abbrechen. Nach dem
//   Abschluss bleibt RebootPending bis zum Neustart.
// - TrialMachine (V07, V08): Selbsttest im Probelauf. Erwartete Panelrate aus dem aktiven Timing (Stromsparmodus
//   30 Hz, Standard 60 Hz), Raten über die gemessene Zeit; „bestätigt“ erst nach Erfolg des Bestätigens, sonst
//   „gesund, aber nicht dauerhaft bestätigt“ mit begrenzten neuen Versuchen; ein gescheiterter Rückfall bleibt
//   als Zustand sichtbar.
// - SaveMark (B23): Änderungskennzeichen mit Revision. Quittiert wird nur, was nachweislich gespeichert ist; eine
//   Änderung während des Speicherns bleibt offen.

#include <atomic>
#include <cstdint>
#include <mutex>

namespace pixelia {

class UpdateGate {
public:
    enum class Phase : uint8_t { Idle, Preparing, Writing, RebootPending };

    uint32_t reserve();             // Idle → Preparing; Besitzer-Nummer, 0 = schon belegt
    bool start_writing(uint32_t id);  // Preparing → Writing
    bool owns(uint32_t id) const;   // Vorbereiten oder Schreiben durch diesen Besitzer
    bool release(uint32_t id);      // Fehler oder Abbruch: zurück zu Idle; false = nicht der Besitzer
    bool done(uint32_t id);         // geprüft und als Start gesetzt: RebootPending bis zum Neustart
    Phase phase() const;

private:
    mutable std::mutex m_;
    Phase phase_ = Phase::Idle;
    uint32_t owner_ = 0;
    uint32_t next_ = 1;
};

struct TrialSample {
    int64_t now_us = 0;     // monotone Zeit
    uint32_t vsyncs = 0;    // Panel-Bilder seit dem Start (Zähler)
    uint32_t frames = 0;    // Engine-Bilder seit dem Start (Zähler)
    double panel_hz = 60;   // erwartete Panelrate aus dem gerade aktiven Timing
    int swap = 2;           // Panel-Bilder je Engine-Bild
    bool net_ok = true;     // WLAN verbunden oder Setup-Modus
    bool engine_fail = false;  // Testvariante: Engine gilt als ausgefallen
};

class TrialMachine {
public:
    enum class Phase : uint8_t {
        Running,         // misst
        Confirming,      // gesund, Bestätigen angefordert
        Unconfirmed,     // gesund, Bestätigen gescheitert, neuer Versuch geplant
        Passed,          // dauerhaft bestätigt (Ende)
        ConfirmFailed,   // gesund, aber nicht dauerhaft bestätigt, keine Versuche mehr (Ende)
        RollingBack,     // nicht bestanden, Rückfall angefordert
        RollbackFailed,  // Rückfall gescheitert, Version läuft weiter im Probelauf (Ende)
    };
    enum class Action : uint8_t { None, MarkValid, Rollback };

    struct Config {
        int need_s = 60;            // so lange muss die Version gesund laufen
        int max_s = 120;            // danach gilt der Test als nicht bestanden
        int slack_s = 5;            // so viele Sekunden dürfen fehlen
        double panel_min = 5.0 / 6;  // Anteil der erwarteten Panelrate (wie bisher 50 von 60)
        double engine_min = 2.0 / 3;  // Anteil der erwarteten Engine-Rate (wie bisher 20 von 30)
        int confirm_tries = 6;      // Bestätigen höchstens so oft (Abstände 5, 10, 20, 40, 60 s)
        int rollback_tries = 3;     // Rückfall höchstens so oft (Abstand 10 s)
    };

    TrialMachine() = default;
    explicit TrialMachine(const Config& c) : cfg_(c) {}

    void start(const TrialSample& s);
    // Einmal je Sekunde; sagt, ob jetzt bestätigt oder zurückgefallen werden soll
    Action step(const TrialSample& s);
    // Ergebnis von esp_ota_mark_app_valid_cancel_rollback (true = ESP_OK)
    void confirm_result(bool ok, int64_t now_us, int error_code = 0);
    // Rückfall kehrt nur bei einem Fehler zurück
    void rollback_failed(int64_t now_us, int error_code = 0);

    Phase phase() const { return phase_; }
    bool finished() const {
        return phase_ == Phase::Passed || phase_ == Phase::ConfirmFailed || phase_ == Phase::RollbackFailed;
    }
    int elapsed_s() const { return elapsed_s_; }
    int ok_s() const { return static_cast<int>(ok_us_ / 1000000); }
    int confirm_attempts() const { return confirm_attempts_; }
    int rollback_attempts() const { return rollback_attempts_; }
    int last_error() const { return last_error_; }
    double panel_rate() const { return panel_rate_; }    // zuletzt gemessen (Hz)
    double engine_rate() const { return engine_rate_; }  // zuletzt gemessen (Bilder/s)
    double expected_panel() const { return expected_panel_; }
    const char* note() const { return note_; }  // kurzer Satz für Status und Log

private:
    void set_note(const char* fmt, ...);

    Config cfg_{};
    Phase phase_ = Phase::Running;
    int64_t start_us_ = 0, prev_us_ = 0, retry_at_us_ = 0;
    uint32_t prev_vs_ = 0, prev_fr_ = 0;
    int64_t ok_us_ = 0;
    int elapsed_s_ = 0;
    int confirm_attempts_ = 0, rollback_attempts_ = 0, last_error_ = 0;
    double panel_rate_ = 0, engine_rate_ = 0, expected_panel_ = 0;
    char note_[80] = "läuft";
};

// Ändern und Speichern dürfen in verschiedenen Tasks laufen (atomar); speichern soll nur einer zur Zeit
class SaveMark {
public:
    void touch() { rev_.fetch_add(1, std::memory_order_acq_rel); }                 // geändert
    uint32_t begin() const { return rev_.load(std::memory_order_acquire); }       // vor dem Lesen der Werte merken
    void saved(uint32_t rev) {                                                    // nur nach bestätigtem Schreiben
        uint32_t cur = saved_.load(std::memory_order_acquire);
        while (static_cast<int32_t>(rev - cur) > 0 &&
               !saved_.compare_exchange_weak(cur, rev, std::memory_order_acq_rel)) {
        }
    }
    bool dirty() const { return rev_.load(std::memory_order_acquire) != saved_.load(std::memory_order_acquire); }

private:
    std::atomic<uint32_t> rev_{0}, saved_{0};
};

const char* trial_phase_name(TrialMachine::Phase p);

}  // namespace pixelia
