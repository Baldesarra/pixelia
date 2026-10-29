#pragma once
// Figuren (seit 0.5.9, SETTING.md Kapitel 3 und 5): der Datensatz jeder Figur, Teil des Weltzustands (Umgebung,
// environment.h) und des Speicherstands (save.h).
//
// - Stammdaten (Name, Alter, Rolle, Herkunft, Ziel, Schlaftyp), Charakterzüge, Interessen (gewichtet), Fähigkeiten in
//   Stufen mit Erfahrungswert, Beziehungen in Stufen, Erinnerungen mit Zeitstempel, Zuhause (Haushalt), Besitz und
//   Aussehen (Ebenen mit Farbreihen; der Baukasten dazu kommt mit 0.5.9 Teil 2). Nur feste Felder, kein Heap.
// - Wo eine Figur ist, steht genau einmal: im Feld where (an einem Ort oder auf einer Verbindung). Eine Übergabe ändert
//   dieses eine Feld in einem Schritt. So kann es keine doppelte und keine verlorene Figur geben (Bühne: Prüfung
//   check_figures, Simulator: --test speicher).
// - Fähigkeiten: Stufen unbekannt, Anfänger, geübt, Meister; intern zählt ein Erfahrungswert bis zur nächsten Stufe.
//   Wer wenig kann, macht Fehlversuche (try_skill: misslungen, mit Makel, gelungen): Auch daraus lernt man, nur
//   weniger. Gezeigt-Bekommen bringt mehr und stärkt die Beziehung zwischen beiden (teach). Die Startstufen legt 0.6.x
//   fest (Zeitraffer-Bericht), die Schwellen stehen in kSkillNeed.
// - Beziehungen (nur das Modell, Inhalte ab 0.6.x): fremd → bekannt → vertraut → hilft, dazu Familie, Zuneigung,
//   Rivalität; Punkte bis zur nächsten Stufe, beidseitig gebucht (FigureBook::bond).
// - Erinnerungen mit Weltzeit und Gewicht; ist der Platz voll, weicht die unwichtigste, bei Gleichstand die älteste.
// - Ortsbezüge sind Nummern in der Ortsliste der Welt (place.h, place_index); der Speicherstand schreibt Kennungen und
//   ordnet beim Laden neu zu, damit neue Orte alte Stände nicht verschieben.

#include <array>
#include <cstdint>
#include <string_view>

#include "pixelia/engine/calendar.h"
#include "pixelia/engine/events.h"
#include "pixelia/engine/rng.h"

namespace pixelia {

// --- Fähigkeiten (SETTING.md Kapitel 5) ---
enum class Skill : uint8_t {
    Cooking, Baking, Woodwork, Felling, Joinery, Carpentry, Masonry, Smithing, Repair, Basketry, Spinning, Weaving,
    Sewing, Gardening, Farming, Herbs, Mushrooms, Animals, Milking, Beekeeping, Fishing, Hunting, Mining, Music,
    Storytelling, Letters, Railway,
    kCount
};
inline constexpr int kSkills = static_cast<int>(Skill::kCount);
const char* skill_id(Skill s);     // kochen, backen … (Speicherstand, Befehle)
const char* skill_title(Skill s);  // Kochen, Backen …
bool parse_skill(std::string_view id, Skill* out);

enum class SkillLevel : uint8_t { Unknown, Beginner, Practiced, Master };
inline constexpr int kSkillLevels = 4;
const char* skill_level_title(SkillLevel l);  // unbekannt, Anfänger, geübt, Meister
// Erfahrung bis zur nächsten Stufe (Meister: keine weitere)
inline constexpr uint16_t kSkillNeed[kSkillLevels] = {20, 300, 1500, 0};

struct SkillState {
    SkillLevel level = SkillLevel::Unknown;
    uint16_t xp = 0;  // Erfahrung in der laufenden Stufe (0 … kSkillNeed − 1)
};

// Ausgang eines Versuchs: misslungen (Material verdorben), mit Makel (Brot zu dunkel, Stuhl wackelt), gelungen
enum class Outcome : uint8_t { Failed, Flawed, Success };
const char* outcome_title(Outcome o);
struct SkillTry {
    Outcome outcome = Outcome::Failed;
    bool level_up = false;
    SkillLevel level = SkillLevel::Unknown;  // danach
};

// --- Beziehungen ---
enum class Bond : uint8_t { Stranger, Known, Familiar, Helps };  // fremd, bekannt, vertraut, hilft
inline constexpr int kBonds = 4;
const char* bond_title(Bond b);
// Punkte bis zur nächsten Stufe (hilft: keine weitere)
inline constexpr uint16_t kBondNeed[kBonds] = {10, 60, 200, 0};
enum RelationFlag : uint8_t { kRelFamily = 1, kRelAffection = 2, kRelRivalry = 4 };
struct Relation {
    uint16_t other = 0;  // Kennung der anderen Figur, 0 = frei
    Bond bond = Bond::Stranger;
    uint8_t flags = 0;    // RelationFlag
    uint16_t points = 0;  // in der laufenden Stufe
};

// --- Erinnerungen (LEBENDIGKEIT.md Regel 10) ---
enum class MemoryKind : uint8_t {
    None,
    First,      // ein erstes Mal (arg = Fähigkeit bzw. Erstling)
    Failure,    // Fehlversuch, der hängen bleibt (Gretas erstes Brot)
    SkillUp,    // neue Stufe einer Fähigkeit (arg = Fähigkeit, b = Stufe)
    BondUp,     // Beziehung eine Stufe weiter (who, b = Stufe)
    BondDown,
    Visit,      // zu Besuch gewesen bzw. Besuch gehabt (who, place)
    Gift,       // etwas bekommen oder gebracht (who, item)
    Event,      // Ereignis der Welt (arg = Art)
    kCount
};
const char* memory_kind_name(MemoryKind k);
struct Memory {
    MemoryKind kind = MemoryKind::None;
    uint8_t weight = 0;    // Gewicht 0–255: was weicht, wenn der Platz voll ist
    uint8_t arg = 0, b = 0;
    uint16_t who = 0;      // andere Figur, 0 = keine
    uint8_t place = 0xFF;  // Ort (Nummer in der Ortsliste der Welt), 0xFF = keiner
    uint8_t pad = 0;
    int32_t day = 0;       // Welttag
    uint16_t minute = 0;   // Minute des Tages
    uint16_t item = 0;     // Gegenstand (ab 0.5.10 Warenbuch), 0 = keiner
};

// --- Aussehen (Ebenen mit Farbreihen, Baukasten in 0.5.9 Teil 2) ---
enum class LookLayer : uint8_t { Body, Legs, Torso, Hair, Hat, Tool, kCount };
inline constexpr int kLookLayers = static_cast<int>(LookLayer::kCount);
// Statur (seit 0.5.9 Teil 5, Ansatz A „Chibi mit Statur“): der Baukasten streckt oder staucht Rumpf und Beine beim
// Zusammensetzen, der Kopf bleibt gleich. Alles 0 = Standard (alte Speicherstände ohne Statur)
struct Stature {
    int8_t height = 0;  // -1 klein, 0 mittel, 1 groß, 2 sehr groß (je Stufe Rumpf und Beine eine Zeile)
    int8_t width = 0;   // -1 schmal, 0 mittel, 1 breit
    uint8_t belly = 0;  // 0 … 2
    uint8_t stoop = 0;  // 1 = gebeugt (Rumpf kürzer, von der Seite Kopf und Schultern vor)
    bool operator==(const Stature&) const = default;
};
// in die gültigen Stufen (Speicherstand, Daten eines Gebiets)
constexpr Stature clamp_stature(Stature s) {
    auto lim = [](int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; };
    return {static_cast<int8_t>(lim(s.height, -1, 2)), static_cast<int8_t>(lim(s.width, -1, 1)),
            static_cast<uint8_t>(lim(s.belly, 0, 2)), static_cast<uint8_t>(lim(s.stoop, 0, 1))};
}
struct FigureLook {
    std::array<uint8_t, kLookLayers> part{};  // Variante je Ebene, 0 = keine
    std::array<uint8_t, kLookLayers> row{};   // Farbreihe je Ebene
    Stature build{};                          // Speicherstand: hinten im Datensatz der Figur (seit Teil 5)
};

// --- Wo die Figur ist ---
struct Whereabouts {
    enum class Kind : uint8_t { Nowhere, Place, Link };
    static constexpr uint8_t kAnyEntry = 0xFF;  // irgendwo am Ort (Start, Laden)
    Kind kind = Kind::Nowhere;
    uint8_t place = 0xFF;      // Ort bzw. Ziel der Verbindung (Nummer in der Ortsliste der Welt)
    uint8_t from = 0xFF;       // Verbindung: Herkunft
    uint8_t entry = kAnyEntry;  // Ausgang des Ziels, an dem man ankommt
    uint8_t from_exit = 0;      // benutzter Ausgang
    uint8_t transition = 3;     // Übergang des Ausgangs (scene.h, Transition), Standard Abblenden
    int64_t depart_us = 0, arrive_us = 0;  // Verbindung: Weltzeit
};

// --- Tätigkeit als Zeitblock (seit 0.5.9 Teil 2: Stufen der Simulation, SETTING.md Kapitel 15) ---
// Los um start_us von Kachel (sx, sy), am Ziel (gx, gy) um walk_us (steps Kacheln Weg), fertig um end_us. Der Planer
// des Ortes entscheidet sie (Scene::decide) und bucht ihre Wirkung an ihrem Ende (Scene::finish), sichtbar oder nicht
// derselbe Code: gleiche Wirkung, andere Darstellung. Die volle Stufe zeigt nur, wo die Figur gerade ist.
// Hinausgehen (Leave) endet immer an einer Takt-Grenze (ganze Weltsekunde): Bis dahin wartet die Figur an der Tür,
// dann übergibt die Bühne sie in einem Schritt.
enum class DoingKind : uint8_t { None, Idle, Wander, Use, Leave, kCount };  // None: gleich neu entscheiden
const char* doing_kind_name(DoingKind k);  // keine, steht, geht, benutzt, geht hinaus
// Wozu (seit 0.5.10 Teil 2, Aufgabenliste tasks.h): Der Planer bucht an seinem Ende danach (Holen, Bringen, Herstellen,
// Essen), auch auf dem Weg dorthin steht es auf der Karte. Board: wartet bis zur Takt-Grenze und schaut, ob es etwas zu
// tun gibt; Stow: legt ab, was ohne Aufgabe in der Trage liegt
enum class DoingAim : uint8_t { None, Board, Fetch, Bring, Return, Produce, Eat, Stow, kCount };
struct Doing {
    DoingKind kind = DoingKind::None;
    DoingAim aim = DoingAim::None;  // Speicherstand: hinten im Datensatz (TAET)
    uint8_t part = 0xFF, k = 0;  // Use: Teil (Nummer in der Raumliste) und Benutzungspunkt
    uint8_t exit = 0xFF;         // Leave: Ausgang
    uint8_t sx = 0, sy = 0;      // Start-Kachel
    uint8_t gx = 0, gy = 0;      // Ziel-Kachel (danach steht die Figur dort)
    uint16_t steps = 0;          // Weglänge in Kacheln (0 = steht schon dort)
    uint32_t seq = 0;            // laufende Nummer der Entscheidung (Zufall je Figur, Darstellung erkennt den Wechsel)
    int64_t start_us = 0, walk_us = 0, end_us = 0;  // Weltzeit
    int64_t since_us = 0;        // am Ort seit (Ankunft)
};

enum FigureFlag : uint8_t { kFigLead = 1, kFigGuest = 2 };

struct FigureRecord {
    static constexpr int kName = 16, kKey = 16, kTraits = 3, kInterests = 4, kRelations = 24, kMemories = 32,
                         kPossessions = 8;
    uint16_t id = 0;  // eindeutig, bleibt für immer (1 …); 0 = frei
    uint8_t flags = 0;
    uint8_t age = 0;
    std::array<char, kName> name{};
    std::array<char, kKey> role{};    // Kennung der Rolle (Daten)
    std::array<char, kKey> origin{};  // Herkunft und Ziel (Kennungen, Texte in den Daten)
    std::array<char, kKey> goal{};
    std::array<char, kKey> home{};    // Haushalt (place.h, PlaceDef::house), leer = keiner
    Chronotype chrono = Chronotype::Normal;
    std::array<std::array<char, 12>, kTraits> traits{};
    struct Interest {
        std::array<char, 12> key{};
        uint8_t weight = 0;  // 0–255, verändert sich langsam
    };
    std::array<Interest, kInterests> interests{};
    FigureLook look{};
    Whereabouts where{};
    Doing doing{};  // seit 0.5.9 Teil 2: laufende Tätigkeit (nur am Ort; Speicherstand: Abschnitt TAET)
    std::array<SkillState, kSkills> skills{};
    std::array<Relation, kRelations> relations{};
    std::array<Memory, kMemories> memories{};
    struct Possession {
        std::array<char, 14> key{};  // Kennung des Gegenstands (Erkennungsstück, Werkzeug), leer = frei
        uint8_t count = 0;
        uint8_t state = 0;           // Zustand 0–255 (bis zum Warenbuch 0.5.10)
    };
    std::array<Possession, kPossessions> owns{};

    bool lead() const { return flags & kFigLead; }
    SkillLevel level(Skill s) const { return skills[static_cast<int>(s)].level; }
    const Relation* relation(uint16_t other) const;
    int memory_count() const;
};

// Figur aus den Daten eines Gebiets (Startbesetzung, Zuzug): was beim Anlegen feststeht
struct FigureDef {
    const char* name;
    uint8_t age;
    const char* role;
    const char* origin;
    const char* goal;
    const char* home;   // Haushalt
    const char* place;  // Ort beim Start (Kennung)
    Chronotype chrono;
    const char* traits[FigureRecord::kTraits];
    struct Want {
        const char* key;
        uint8_t weight;
    } interests[FigureRecord::kInterests];
    struct Start {
        Skill skill;
        SkillLevel level;
    } skills[6];  // Startstufen (Rest unbekannt)
    FigureLook look;
    bool lead;
};

// Alle Figuren der Welt
class FigureBook {
public:
    static constexpr int kMax = 48;  // Endausbau 30–40 Bewohner plus Gäste (SETTING.md Kapitel 3)

    FigureBook() = default;
    ~FigureBook();
    FigureBook(const FigureBook&) = delete;
    FigureBook& operator=(const FigureBook&) = delete;
    void clear();
    // Anlegen: Kennung id (0 = die nächste freie); nullptr = voll bzw. Kennung vergeben
    FigureRecord* add(const FigureDef& def, uint8_t place, uint16_t id = 0);
    FigureRecord* add_record(const FigureRecord& r);  // Laden: Datensatz wie gespeichert
    int count() const { return n_; }
    FigureRecord& at(int i) { return f_[i]; }
    const FigureRecord& at(int i) const { return f_[i]; }
    FigureRecord* find(uint16_t id);
    const FigureRecord* find(uint16_t id) const;
    uint16_t lead() const;  // Hauptfigur, 0 = keine
    uint16_t next_id() const { return next_; }
    void set_next_id(uint16_t id) { next_ = id; }

    // Beziehung beidseitig ändern (Punkte, auch negativ); Stufenwechsel → Erinnerung beider, Ereignis auf dem Bus
    void bond(uint16_t a, uint16_t b, int points, int32_t day, int minute);
    void set_relation_flag(uint16_t a, uint16_t b, uint8_t flag, bool on);
    // Versuch mit einer Fähigkeit: Erfolg nach Stufe, Erfahrung und Schwierigkeit (0 leicht … 3 schwer)
    SkillTry try_skill(uint16_t who, Skill s, int difficulty, Rng& rng, int32_t day, int minute);
    // Gezeigt bekommen: der Lehrer muss die Fähigkeit mindestens geübt können; minutes gemeinsame Arbeit
    bool teach(uint16_t teacher, uint16_t learner, Skill s, int minutes, int32_t day, int minute);
    // Erfahrung gutschreiben (Zuschauen, Ausprobieren); true = neue Stufe
    bool learn(uint16_t who, Skill s, int xp, int32_t day, int minute);
    // Erinnerung eintragen (weicht die unwichtigste, bei Gleichstand die älteste); false = zu unwichtig
    bool remember(uint16_t who, const Memory& m);

    void set_bus(EventBus* bus) { bus_ = bus; }
    uint32_t version() const { return version_; }  // zählt jede Änderung an Fähigkeiten, Beziehungen, Erinnerungen

private:
    bool ensure();  // Datensätze liegen im Grafik-Speicher (auf dem Gerät PSRAM, je ~1,1 KB), angelegt beim ersten Mal
    Relation* relation_slot(FigureRecord& f, uint16_t other);
    void bond_side(FigureRecord& f, uint16_t other, int points, int32_t day, int minute, bool tell);
    void level_up(FigureRecord& f, Skill s, int32_t day, int minute);
    void publish(EventKind kind, uint16_t who, uint16_t other, uint8_t a, uint8_t b, int32_t day);

    FigureRecord* f_ = nullptr;
    int n_ = 0;
    uint16_t next_ = 1;
    EventBus* bus_ = nullptr;
    uint32_t version_ = 0;
};

// Text in ein festes Feld kopieren (abgeschnitten, immer mit Null am Ende)
template <size_t N>
void set_text(std::array<char, N>& out, std::string_view s) {
    const size_t n = s.size() < N - 1 ? s.size() : N - 1;
    for (size_t i = 0; i < n; ++i) out[i] = s[i];
    for (size_t i = n; i < N; ++i) out[i] = 0;
}

}  // namespace pixelia
