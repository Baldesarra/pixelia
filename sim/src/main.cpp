// Pixelia-Simulator: Einstieg und Befehlszeile

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "pixelia/engine/scene.h"
#include "sim.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace {

void print_help() {
    std::printf(
        "Pixelia-Simulator\n"
        "\n"
        "Aufruf: pixelia-sim [Optionen]\n"
        "  --headless              ohne Fenster (CI, Langzeit-Tests)\n"
        "  --scene NAME            Szene (%s)\n"
        "  --seed N                Startwert für den Zufall (Standard 1)\n"
        "  --day N                 Welttag beim Start (Standard 0)\n"
        "  --time HH:MM            Uhrzeit beim Start (Standard 12:00)\n"
        "  --speed X               Zeitraffer, 1 = Echtzeit, 60 = eine Weltminute pro Sekunde\n"
        "  --frames N              N Logik-Schritte vorspulen (30 pro Sekunde)\n"
        "  --screenshot DATEI      PNG speichern (kopflos)\n"
        "  --scale N               Vergrößerung des Screenshots (Standard 2 = wie das Gerät)\n"
        "  --contact-sheet DATEI   Kontaktbogen mit zufälligen Momenten (kopflos)\n"
        "  --moments N             Zahl der Momente im Kontaktbogen (Standard 24)\n"
        "  --span-days X           Zeitraum der Momente in Tagen (Standard 1)\n"
        "  --load N                Lastregler der Szene (z. B. Zahl der Sprites)\n"
        "  --true-colors           Palettenfarben statt Display-Farben (RGB565)\n"
        "  --run-seconds X         Fenster nach X Sekunden schließen (mit --screenshot: Bild beim Schließen)\n"
        "  --screen NAME           Systemanzeige mit Beispieldaten als Screenshot (kopflos): setup, setup-leer,\n"
        "                          setup-fehler, update, update-fertig, failed, message, toast, toast-lang\n"
        "\n"
        "Fenster: Leertaste Pause, +/- Zeitraffer, T eine Stunde weiter, S Screenshot, Esc Ende\n",
        pixelia::scene_names());
}

bool parse_time(const char* s, int* h, int* m) {
    return std::sscanf(s, "%d:%d", h, m) == 2 && *h >= 0 && *h < 24 && *m >= 0 && *m < 60;
}

}  // namespace

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);  // Umlaute in der Windows-Konsole
#endif
    sim::Options opt;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() -> const char* {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "FEHLER: %s braucht einen Wert\n", a.c_str());
                std::exit(2);
            }
            return argv[++i];
        };
        if (a == "--help" || a == "-h") {
            print_help();
            return 0;
        } else if (a == "--headless") {
            opt.headless = true;
        } else if (a == "--scene") {
            opt.scene = next();
        } else if (a == "--seed") {
            opt.seed = std::strtoull(next(), nullptr, 0);
        } else if (a == "--day") {
            opt.day = std::atoi(next());
        } else if (a == "--time") {
            if (!parse_time(next(), &opt.hour, &opt.minute)) {
                std::fprintf(stderr, "FEHLER: Uhrzeit als HH:MM angeben\n");
                return 2;
            }
        } else if (a == "--speed") {
            opt.speed = std::atof(next());
        } else if (a == "--frames") {
            opt.frames = std::atol(next());
        } else if (a == "--screenshot") {
            opt.screenshot = next();
        } else if (a == "--scale") {
            opt.scale = std::atoi(next());
        } else if (a == "--contact-sheet") {
            opt.contact_sheet = next();
        } else if (a == "--moments") {
            opt.moments = std::atoi(next());
        } else if (a == "--span-days") {
            opt.span_days = std::atof(next());
        } else if (a == "--load") {
            opt.load = std::atoi(next());
        } else if (a == "--true-colors") {
            opt.device_colors = false;
        } else if (a == "--screen") {
            opt.screen = next();
        } else if (a == "--run-seconds") {
            opt.run_seconds = std::atof(next());
        } else {
            std::fprintf(stderr, "FEHLER: unbekannte Option %s (--help zeigt alle)\n", a.c_str());
            return 2;
        }
    }
    if (!pixelia::make_scene(opt.scene.c_str())) {
        std::fprintf(stderr, "FEHLER: Szene \"%s\" gibt es nicht (%s)\n", opt.scene.c_str(), pixelia::scene_names());
        return 2;
    }
    if (opt.headless) return sim::run_headless(opt);
#if PIXELIA_SIM_WINDOW
    return sim::run_window(opt);
#else
    std::fprintf(stderr, "FEHLER: ohne SDL gebaut, nur --headless möglich\n");
    return 2;
#endif
}
