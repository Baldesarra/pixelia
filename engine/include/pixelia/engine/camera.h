#pragma once
// Kamera-Modi (seit 0.5.6, SETTING.md Kapitel 13; der Regisseur kommt mit der ersten Ereigniswelle 0.6.x).
//
// Standard („Zuhause“ der Kamera, in der Weboberfläche einstellbar):
// - Hauptfigur: die Anzeige folgt ihr von Ort zu Ort (später Sykora, heute die Begleiterin der Testorte).
// - Figur: dasselbe mit einer frei gewählten Figur („Folge X“).
// - Ort: die Anzeige bleibt an einem festen Ort.
// Vom Standard weg führen:
// - „Frei“: Wischen oder die Weboberfläche wechseln den Ort, niemand wird verfolgt.
// - „Folgen“: langes Halten auf einer Figur (oder die Weboberfläche) folgt dieser Figur.
// Nach einer einstellbaren Zeit ohne Eingabe (Standard 10 min, 0 = nie) kehrt die Kamera zum Standard zurück.
// Die Kamera rechnet in Logik-Schritten und sagt der Bühne nur, wohin sie gehen und wem der Blick folgt
// (Stage::go, Stage::set_follow_who); die Bühne bleibt deterministisch.

#include <cstdint>

#include "pixelia/engine/scene.h"

namespace pixelia {

class Stage;

enum class CameraHome : uint8_t { Lead, Figure, Place };

struct CameraSettings {
    CameraHome home = CameraHome::Lead;
    uint16_t figure = 0;      // bei Figure
    int8_t place = 0;         // bei Place (Index der Bühne)
    uint16_t return_s = 600;  // Rückkehr zum Standard nach so vielen Sekunden ohne Eingabe, 0 = nie

    bool operator==(const CameraSettings&) const = default;
};

class Camera {
public:
    enum class Mode : uint8_t { Home, Free, Follow };

    void set_settings(const CameraSettings& s);  // wirkt sofort: zurück zum Standard
    const CameraSettings& settings() const { return settings_; }
    Mode mode() const { return mode_; }
    // Figur, der die Anzeige gerade folgt (0 = niemandem)
    uint16_t target(const Stage& stage) const;
    uint32_t idle_ticks() const { return idle_; }
    // Sekunden bis zur Rückkehr (−1 = keine geplant)
    int return_in_s() const;

    void input() { idle_ = 0; }                      // jede Berührung
    void swipe(Stage& stage, int dir);               // +1 nächster, −1 vorheriger Ort; Modus Frei
    void go(Stage& stage, int place, Transition t = kDefaultTransition);  // zu einem Ort; Modus Frei
    void follow(Stage& stage, uint16_t figure);      // Modus Folgen; springt zum Ort der Figur
    void home(Stage& stage);                         // zurück zum Standard
    void tick(Stage& stage);                         // je Logik-Schritt nach Stage::tick

private:
    void apply(Stage& stage);
    void seek(Stage& stage, uint16_t figure);

    CameraSettings settings_{};
    Mode mode_ = Mode::Home;
    uint16_t follow_ = 0;  // Modus Folgen
    uint32_t idle_ = 0;
    uint32_t check_ = 0;
    bool dirty_ = true;
};

}  // namespace pixelia
