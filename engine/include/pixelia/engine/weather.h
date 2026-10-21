#pragma once
// Wetter (seit 0.5.3): ein Wetter für alle Orte; jeder Ort zeigt es auf seine Art (PlaceWeather).
//
// - Das Ziel (Art und Stärke) kommt von außen: bis 0.5.6 die Teststeuerung (Seite Experimentell → Orte, Konsole
//   „wetter“, Simulator --cmd), danach die Wettersimulation der Welt (SETTING.md Kapitel 9). Wie bei der Tageszeit
//   rechnet die Engine nur aus, was zu sehen ist.
// - Weiche Übergänge: erst ziehen Wolken auf, dann setzt Regen bzw. Schnee langsam ein; beim Aufklaren hört erst der
//   Niederschlag auf, dann reißen die Wolken auf. Blitze erst bei kräftigem Regen.
// - Böen und Blitze aus eigenem Zufall (Startwert der Bühne): deterministisch.
// - apply() ändert das Umgebungslicht (Wolken dunkler und blasser, Himmel grau, Dunst bei Nebel, Blitz hell). Über
//   PlacePalette::apply wirkt das nur auf die Szenenfarben; 240–255 (System-Oberfläche) bleiben unberührt.
//
// Am Ort (PlaceWeather): Partikel (Regen mit Spritzern, Schnee, Blätter im Wind), fein gezeichnet nach dem
// Verdoppeln; Schleier über Farbtabellen (Nebelbänke, Regenschleier, Wolkenschatten), fein gerastert beim
// Verdoppeln (canvas.h, CoarseLayers). Innenräume zeigen Partikel nur in den Fenstern (Glasfarben mit Rolle Himmel).
// Partikel gibt es nur, solange der Ort betreten ist: Unsichtbare Orte kosten nichts, beim Betreten ist das Bild
// sofort voll (Vorlauf).

#include <cstdint>
#include <string_view>

#include "pixelia/engine/canvas.h"
#include "pixelia/engine/light.h"
#include "pixelia/engine/particles.h"
#include "pixelia/engine/rng.h"

namespace pixelia {

enum class WeatherKind : uint8_t { Clear, Cloudy, Rain, Snow, Fog, Storm, Wind };
inline constexpr int kWeatherKinds = 7;
const char* weather_name(WeatherKind k);   // Kennung: sonne, bewoelkt, regen, schnee, nebel, gewitter, wind
const char* weather_title(WeatherKind k);  // sichtbarer Name
bool parse_weather(std::string_view name, WeatherKind* out);

// Aussehen zum Vergleichen (Befehle „blitzstil“ an der Bühne, „nebelstil“ am Ort; Entscheidung des Users)
enum class FlashStyle : uint8_t { Dimmed, Sheet, Short };  // gedämpft, Wetterleuchten, kurz und heller
enum class FogStyle : uint8_t { Drift, Layers, Billow };    // Schwaden von oben, mehrere Schleier, wabernd

// Wetter gerade, je 0 … 255
struct WeatherState {
    uint8_t cloud = 0;  // Bewölkung
    uint8_t rain = 0;
    uint8_t snow = 0;
    uint8_t fog = 0;
    uint8_t storm = 0;  // Gewitter (Blitze)
    uint8_t wind = 0;   // mit Böen
    int8_t wind_dir = 1;  // +1 nach rechts, −1 nach links
    uint8_t flash = 0;    // Helligkeit des Blitzes gerade
};

class Weather {
public:
    // Abstand der Blitze bei vollem Gewitter: zufällig 12–30 s (Entscheidung des Users 2026-10-21), bei
    // schwächerem Gewitter entsprechend länger
    static constexpr int kFlashMinSeconds = 12, kFlashMaxSeconds = 30;

    void init(uint64_t seed);  // Sonne, wenig Wind
    // Neues Ziel, weicher Übergang (Stärke 0 … 255)
    void set(WeatherKind kind, int strength = 255);
    // Sofort ohne Übergang (Vergleichsbilder, Prüfungen)
    void set_now(WeatherKind kind, int strength = 255);
    WeatherKind kind() const { return kind_; }
    int strength() const { return strength_; }
    void set_wind_dir(int dir) { s_.wind_dir = static_cast<int8_t>(dir < 0 ? -1 : 1); }
    void set_flash_seconds(int min_s, int max_s);
    void set_flash_style(FlashStyle s) { flash_style_ = s; }
    void strike();  // Blitz sofort (Prüfungen)

    void tick();
    const WeatherState& state() const { return s_; }
    bool settled() const;  // Übergang fertig
    uint32_t flashes() const { return flashes_; }
    // Windversatz seit init() in 1/256 feinen Bildpunkten (Nebel und Wolkenschatten ziehen damit)
    int64_t drift() const { return drift_; }
    // Umgebungslicht verändern; outdoor = draußen, sonst drinnen (weniger Wirkung, die Fenster zeigen den Himmel)
    void apply(Ambient& a, bool outdoor) const;

private:
    struct Channels {
        int32_t cloud, rain, snow, fog, storm, wind;  // 8.8 Festkomma
    };
    void goals(WeatherKind kind, int strength, Channels* out) const;
    void publish();

    Rng rng_;
    WeatherKind kind_ = WeatherKind::Clear;
    int strength_ = 255;
    Channels goal_{}, cur_{};
    int32_t gust_ = 256 << 8, gust_goal_ = 256 << 8;
    int gust_wait_ = 0;
    int flash_min_ = kFlashMinSeconds, flash_max_ = kFlashMaxSeconds;
    int32_t flash_wait_ = -1;  // bis zum nächsten Blitz, Schritte × 255 (−1 = neu auslosen)
    int flash_step_ = -1;
    bool double_flash_ = false;
    FlashStyle flash_style_ = FlashStyle::Dimmed;
    uint32_t flashes_ = 0;
    int64_t drift_ = 0;
    WeatherState s_{};
};

// Wie ein Ort das Wetter zeigt: Grundfarben der Partikel, Fenster drinnen, Ziele der Schleier
struct WeatherLook {
    bool outdoor = true;
    uint8_t rain = 0, splash = 0, snow = 0, leaf[2] = {0, 0};  // Grundfarben (gezeichnet im Licht des Ortes)
    // drinnen: Glasfarben der Fenster (Rolle Himmel); nur dort zeigen sich die Partikel
    uint8_t window_first = 0, window_count = 0;
    Color fog{206, 210, 214};     // Nebelbänke
    Color veil{214, 218, 226};    // Regenschleier
    Color shadow{36, 40, 66};     // Wolkenschatten
};

// Aussehen zum Vergleichen (Befehle „regen“, „schnee“ am Ort; Entscheidung des Users nach Vergleichsbildern)
enum class RainStyle : uint8_t { Streaks, Drops, Fine };
enum class SnowStyle : uint8_t { Mixed, Small, Large };

class PlaceWeather {
public:
    static constexpr int kMaxParticles = 512;  // Obergrenze je Ort (16 Byte je Partikel, 8 KB)

    // Farbtabellen der Schleier aus der Palette des Ortes (nach jeder Änderung der Palette)
    void configure(const WeatherLook& look, const PlacePalette& pal);
    // Betreten: Speicher anlegen und das Wetter vorlaufen lassen (Bild sofort voll); Verlassen: freigeben
    void enter(const Weather& w, uint64_t seed);
    void leave();
    bool active() const { return pool_.ready(); }
    void tick(const Weather& w);

    void set_rain_style(RainStyle s) { rain_style_ = s; }
    void set_snow_style(SnowStyle s) { snow_style_ = s; }
    // Dichte in Prozent der Vorgabe (Vergleiche); Nebel: Anteil der Bänke
    void set_density(int percent) { density_ = percent < 10 ? 10 : percent > 300 ? 300 : percent; }
    void set_fog_scale(int percent) { fog_scale_ = percent < 0 ? 0 : percent > 200 ? 200 : percent; }
    void set_fog_style(FogStyle s) { fog_style_ = s; }

    int particles() const { return pool_.size(); }
    int peak() const { return peak_; }

    // Zeichnen, Schritt 1 (vor dem Verdoppeln): Schleier in die Schichten eintragen (oder austragen)
    void layers(CoarseLayers& L, const Weather& w, uint64_t tick) const;
    // Schritt 2 (nach dem Verdoppeln): Partikel fein, im Licht des Ortes (L.light); remap = gemeinsame Palette einer
    // Überblendung (nullptr = keine)
    void draw(Canvas& fine, const CoarseLayers& L, const uint8_t* remap) const;

private:
    void spawn(const WeatherState& s);
    void step();
    bool masked(uint8_t px) const;  // drinnen: nur über Glas zeichnen

    WeatherLook look_{};
    int base_ = 80;
    Rng rng_;
    ParticlePool pool_;
    RainStyle rain_style_ = RainStyle::Streaks;
    SnowStyle snow_style_ = SnowStyle::Mixed;
    FogStyle fog_style_ = FogStyle::Drift;
    int density_ = 100, fog_scale_ = 100;
    int32_t rain_acc_ = 0, snow_acc_ = 0, leaf_acc_ = 0;  // Bruchteile beim Entstehen, 8.8
    int peak_ = 0;
    int wind_vx_ = 0;  // Windversatz je Schritt (1/256 feine Bildpunkte)
    uint8_t fog_[256]{}, veil_[256]{}, shadow_[256]{};
    mutable uint8_t grid_[CoarseLayers::kVeilW * CoarseLayers::kVeilH]{};
    mutable uint8_t noise_[CoarseLayers::kVeilW * CoarseLayers::kVeilH]{};  // zweite Lage (Nebel)
};

}  // namespace pixelia
