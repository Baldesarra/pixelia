// Abläufe der Firmware ohne SDK-Aufrufe (siehe device_flow.h)

#include "pixelia/engine/device_flow.h"

#include <cstdarg>
#include <cstdio>

namespace pixelia {

// ---- UpdateGate ----

uint32_t UpdateGate::reserve() {
    std::lock_guard<std::mutex> g(m_);
    if (phase_ != Phase::Idle) return 0;
    phase_ = Phase::Preparing;
    owner_ = next_++;
    if (next_ == 0) next_ = 1;  // 0 heißt „kein Besitzer“
    return owner_;
}

bool UpdateGate::start_writing(uint32_t id) {
    std::lock_guard<std::mutex> g(m_);
    if (id == 0 || id != owner_ || phase_ != Phase::Preparing) return false;
    phase_ = Phase::Writing;
    return true;
}

bool UpdateGate::owns(uint32_t id) const {
    std::lock_guard<std::mutex> g(m_);
    return id != 0 && id == owner_ && (phase_ == Phase::Preparing || phase_ == Phase::Writing);
}

bool UpdateGate::release(uint32_t id) {
    std::lock_guard<std::mutex> g(m_);
    if (id == 0 || id != owner_ || (phase_ != Phase::Preparing && phase_ != Phase::Writing)) return false;
    phase_ = Phase::Idle;
    owner_ = 0;
    return true;
}

bool UpdateGate::done(uint32_t id) {
    std::lock_guard<std::mutex> g(m_);
    if (id == 0 || id != owner_ || phase_ != Phase::Writing) return false;
    phase_ = Phase::RebootPending;
    return true;
}

UpdateGate::Phase UpdateGate::phase() const {
    std::lock_guard<std::mutex> g(m_);
    return phase_;
}

// ---- TrialMachine ----

namespace {
constexpr int kConfirmGapS[] = {5, 10, 20, 40, 60};
constexpr int kRollbackGapS = 10;
}  // namespace

void TrialMachine::set_note(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(note_, sizeof(note_), fmt, ap);
    va_end(ap);
}

void TrialMachine::start(const TrialSample& s) {
    phase_ = Phase::Running;
    start_us_ = prev_us_ = s.now_us;
    prev_vs_ = s.vsyncs;
    prev_fr_ = s.frames;
    ok_us_ = 0;
    elapsed_s_ = 0;
    confirm_attempts_ = rollback_attempts_ = last_error_ = 0;
    expected_panel_ = s.panel_hz;
    set_note("läuft");
}

TrialMachine::Action TrialMachine::step(const TrialSample& s) {
    switch (phase_) {
        case Phase::Unconfirmed:
            if (s.now_us < retry_at_us_) return Action::None;
            phase_ = Phase::Confirming;
            return Action::MarkValid;
        case Phase::RollingBack:
            if (s.now_us < retry_at_us_) return Action::None;
            return Action::Rollback;
        case Phase::Running: break;
        default: return Action::None;  // wartet auf ein Ergebnis oder ist fertig
    }
    const int64_t dt = s.now_us - prev_us_;
    if (dt < 200000) return Action::None;  // zu kurz für eine Rate (Task kam zu früh dran)
    const double sec = static_cast<double>(dt) / 1e6;
    panel_rate_ = static_cast<double>(s.vsyncs - prev_vs_) / sec;  // Zähler laufen über: Differenz ohne Vorzeichen
    engine_rate_ = static_cast<double>(s.frames - prev_fr_) / sec;
    expected_panel_ = s.panel_hz;
    prev_us_ = s.now_us;
    prev_vs_ = s.vsyncs;
    prev_fr_ = s.frames;
    elapsed_s_ = static_cast<int>((s.now_us - start_us_) / 1000000);

    const double expected_engine = s.panel_hz / (s.swap > 0 ? s.swap : 1);
    const bool panel_ok = panel_rate_ >= cfg_.panel_min * s.panel_hz;
    const bool engine_ok = !s.engine_fail && engine_rate_ >= cfg_.engine_min * expected_engine;
    if (panel_ok && engine_ok) ok_us_ += dt;
    if (!panel_ok) {
        set_note("Display liefert keine Bilder (%.0f von %.0f Hz)", panel_rate_, s.panel_hz);
    } else if (!engine_ok) {
        set_note("Engine liefert keine Bilder (%.0f von %.0f/s)", engine_rate_, expected_engine);
    } else if (!s.net_ok) {
        set_note("weder WLAN noch Setup-Modus");
    } else {
        set_note("läuft (Panel %.0f Hz, Engine %.0f/s)", panel_rate_, engine_rate_);
    }

    if (elapsed_s_ >= cfg_.need_s && ok_s() >= cfg_.need_s - cfg_.slack_s && s.net_ok) {
        phase_ = Phase::Confirming;
        return Action::MarkValid;
    }
    if (elapsed_s_ >= cfg_.max_s) {
        phase_ = Phase::RollingBack;
        ++rollback_attempts_;
        return Action::Rollback;
    }
    return Action::None;
}

void TrialMachine::confirm_result(bool ok, int64_t now_us, int error_code) {
    if (phase_ != Phase::Confirming) return;
    ++confirm_attempts_;
    if (ok) {
        phase_ = Phase::Passed;
        set_note("bestanden");
        return;
    }
    last_error_ = error_code;
    if (confirm_attempts_ >= cfg_.confirm_tries) {
        phase_ = Phase::ConfirmFailed;
        set_note("gesund, aber nicht dauerhaft bestätigt (Fehler 0x%x)", static_cast<unsigned>(error_code));
        return;
    }
    const int i = confirm_attempts_ - 1;
    const int gap = kConfirmGapS[i < 5 ? i : 4];
    phase_ = Phase::Unconfirmed;
    retry_at_us_ = now_us + static_cast<int64_t>(gap) * 1000000;
    set_note("gesund, Bestätigen fehlgeschlagen (Fehler 0x%x), neuer Versuch in %d s",
             static_cast<unsigned>(error_code), gap);
}

void TrialMachine::rollback_failed(int64_t now_us, int error_code) {
    if (phase_ != Phase::RollingBack) return;
    last_error_ = error_code;
    if (rollback_attempts_ >= cfg_.rollback_tries) {
        phase_ = Phase::RollbackFailed;
        set_note("Rückfall nicht möglich (Fehler 0x%x), Version läuft weiter im Probelauf",
                 static_cast<unsigned>(error_code));
        return;
    }
    ++rollback_attempts_;
    retry_at_us_ = now_us + static_cast<int64_t>(kRollbackGapS) * 1000000;
    set_note("Rückfall fehlgeschlagen (Fehler 0x%x), neuer Versuch in %d s", static_cast<unsigned>(error_code),
             kRollbackGapS);
}

const char* trial_phase_name(TrialMachine::Phase p) {
    switch (p) {
        case TrialMachine::Phase::Running: return "läuft";
        case TrialMachine::Phase::Confirming: return "bestätigen";
        case TrialMachine::Phase::Unconfirmed: return "unbestätigt";
        case TrialMachine::Phase::Passed: return "bestanden";
        case TrialMachine::Phase::ConfirmFailed: return "nicht bestätigt";
        case TrialMachine::Phase::RollingBack: return "Rückfall";
        case TrialMachine::Phase::RollbackFailed: return "Rückfall gescheitert";
    }
    return "?";
}

}  // namespace pixelia
