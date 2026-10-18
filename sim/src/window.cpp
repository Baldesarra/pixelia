// Fenster-Modus mit SDL3: zeigt die Szene in 800×480 (2×), Logik mit 30 Schritten pro Sekunde wie auf dem Gerät.
// Die Maus ersetzt den Touch: gleiche Gestenerkennung wie auf dem Gerät (langer Druck = Info-Karte).

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "pixelia/engine/color.h"
#include "pixelia/engine/gesture.h"
#include "pixelia/engine/png.h"
#include "pixelia/engine/scene.h"
#include "sim.h"

namespace sim {

namespace {

void save_screenshot(const pixelia::Scene& scene, const pixelia::Palette& palette, const std::vector<uint8_t>& frame,
                     const pixelia::WorldClock& clock, bool device_colors, std::string path) {
    constexpr int s = 2;
    std::vector<uint8_t> rgb(static_cast<size_t>(pixelia::kScreenWidth) * s * pixelia::kScreenHeight * s * 3);
    pixelia::indexed_to_rgb(frame.data(), pixelia::kScreenWidth, pixelia::kScreenHeight, palette, s, device_colors,
                            rgb.data());
    auto png = pixelia::encode_png_rgb(rgb.data(), pixelia::kScreenWidth * s, pixelia::kScreenHeight * s);
    if (path.empty()) {
        char name[96];
        std::snprintf(name, sizeof(name), "screenshots/%s-tag%d-%02d%02d.png", scene.name(), clock.day(), clock.hour(),
                      clock.minute());
        path = name;
    }
    std::filesystem::path p(path);
    if (p.has_parent_path()) std::filesystem::create_directories(p.parent_path());
    std::ofstream(p, std::ios::binary).write(reinterpret_cast<const char*>(png.data()),
                                              static_cast<std::streamsize>(png.size()));
    std::printf("sim: Screenshot %s\n", path.c_str());
}

}  // namespace

int run_window(const Options& opt) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "FEHLER: SDL startet nicht: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("Pixelia-Simulator", pixelia::kScreenWidth * 2, pixelia::kScreenHeight * 2,
                                          SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = window ? SDL_CreateRenderer(window, nullptr) : nullptr;
    if (!renderer) {
        std::fprintf(stderr, "FEHLER: Fenster: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderVSync(renderer, 1);
    SDL_SetRenderLogicalPresentation(renderer, pixelia::kScreenWidth, pixelia::kScreenHeight,
                                     SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING,
                                             pixelia::kScreenWidth, pixelia::kScreenHeight);
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

    auto scene = pixelia::make_scene(opt.scene.c_str());
    scene->init(opt.seed);
    if (opt.load >= 0) scene->set_load(opt.load);
    pixelia::WorldClock clock;
    clock.set(opt.day, opt.hour, opt.minute);
    clock.set_speed(opt.speed);
    clock.advance(0);
    scene->tick(clock);
    for (long i = 0; i < opt.frames; ++i) {
        clock.advance(pixelia::kTickUs);
        scene->tick(clock);
    }

    std::vector<uint8_t> frame(pixelia::kScreenPixels);
    std::vector<uint8_t> rgb(pixelia::kScreenPixels * 3);
    pixelia::Canvas canvas(frame.data(), pixelia::kScreenWidth, pixelia::kScreenHeight);

    // Maus wie Touch, in Panel-Koordinaten (800×480), damit die Bewegungstoleranz wie auf dem Gerät gilt
    pixelia::gesture::Recognizer gestures;
    pixelia::ui::InfoCard card;
    Uint64 card_until_ms = 0;
    bool mouse_down = false;
    auto mouse = [&](SDL_Event& ev, int fingers) {
        SDL_ConvertEventToRenderCoordinates(renderer, &ev);
        const float fx = ev.type == SDL_EVENT_MOUSE_MOTION ? ev.motion.x : ev.button.x;
        const float fy = ev.type == SDL_EVENT_MOUSE_MOTION ? ev.motion.y : ev.button.y;
        gestures.touch(fingers, static_cast<int>(fx * 2), static_cast<int>(fy * 2),
                       static_cast<uint32_t>(SDL_GetTicks()));
    };
    pixelia::Palette palette{};

    bool running = true, paused = false, want_shot = false;
    Uint64 last = SDL_GetTicksNS();
    const Uint64 started = last;
    Uint64 last_title = 0;
    int64_t acc_us = 0;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (e.type == SDL_EVENT_KEY_DOWN) {
                SDL_Keycode k = e.key.key;
                if (k == SDLK_ESCAPE) running = false;
                else if (k == SDLK_SPACE) paused = !paused;
                else if (k == SDLK_PLUS || k == SDLK_KP_PLUS || k == SDLK_EQUALS) clock.set_speed(clock.speed() * 2);
                else if (k == SDLK_MINUS || k == SDLK_KP_MINUS) clock.set_speed(std::max(1.0, clock.speed() / 2));
                else if (k == SDLK_T) clock.set_world_us(clock.world_us() + 60 * pixelia::WorldClock::kUsPerMinute);
                else if (k == SDLK_S) want_shot = true;
            } else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) {
                mouse_down = true;
                mouse(e, 1);
            } else if (e.type == SDL_EVENT_MOUSE_MOTION && mouse_down) {
                mouse(e, 1);
            } else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && e.button.button == SDL_BUTTON_LEFT) {
                mouse_down = false;
                mouse(e, 0);
            }
        }
        const Uint64 now_ms = SDL_GetTicks();
        gestures.tick(static_cast<uint32_t>(now_ms));
        for (pixelia::gesture::Event g; gestures.poll(&g);) {
            if (g.type == pixelia::gesture::Type::LongPress) {
                card.set(sample_info("info"));
                card_until_ms = now_ms + 20000;
                std::printf("sim: langer Druck bei %d/%d → Info-Karte\n", g.x, g.y);
            } else if (g.type == pixelia::gesture::Type::Tap && now_ms < card_until_ms) {
                card_until_ms = 0;
                std::printf("sim: Tippen → Info-Karte geschlossen\n");
            }
        }

        // Feste Logik-Rate wie auf dem Gerät (30 Schritte pro Sekunde), höchstens 8 nachholen
        Uint64 now = SDL_GetTicksNS();
        acc_us += static_cast<int64_t>((now - last) / 1000);
        last = now;
        int steps = 0;
        while (acc_us >= pixelia::kTickUs && steps < 8) {
            if (!paused) {
                clock.advance(pixelia::kTickUs);
                scene->tick(clock);
            }
            acc_us -= pixelia::kTickUs;
            ++steps;
        }
        if (steps == 8) acc_us = 0;

        scene->render(canvas);
        const bool card_visible = now_ms < card_until_ms;
        if (card_visible) card.render(canvas);
        const float hold = gestures.hold_progress(static_cast<uint32_t>(now_ms));
        if (hold >= 0.25f && !card_visible) pixelia::ui::draw_hold_indicator(canvas, hold);
        palette = scene->palette();
        pixelia::ui::apply_palette(palette);
        if (want_shot) {
            save_screenshot(*scene, palette, frame, clock, opt.device_colors, "");
            want_shot = false;
        }
        if (opt.run_seconds > 0 && (now - started) / 1e9 >= opt.run_seconds) {
            if (!opt.screenshot.empty())
                save_screenshot(*scene, palette, frame, clock, opt.device_colors, opt.screenshot);
            running = false;
        }
        pixelia::indexed_to_rgb(frame.data(), pixelia::kScreenWidth, pixelia::kScreenHeight, palette, 1,
                                opt.device_colors, rgb.data());
        SDL_UpdateTexture(texture, nullptr, rgb.data(), pixelia::kScreenWidth * 3);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        if (now - last_title > 250000000ULL) {
            char title[128];
            std::snprintf(title, sizeof(title), "Pixelia-Simulator – %s – Tag %d %02d:%02d – %gx%s", scene->name(),
                          clock.day(), clock.hour(), clock.minute(), clock.speed(), paused ? " – Pause" : "");
            SDL_SetWindowTitle(window, title);
            last_title = now;
        }
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

}  // namespace sim
