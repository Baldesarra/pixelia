#pragma once
// Ereignis-Bus (seit 0.5.8, SETTING.md Kapitel 15): Welt und Orte melden, was passiert ist (Jahreszeitenabend,
// Erstling, Ankunft einer Figur, Licht erloschen …); wer sich dafür interessiert, liest mit. Ein Ort greift nie direkt
// auf einen anderen zu, er liest den Bus.
//
// - Ereignisse sind kleine feste Datensätze (kein Heap) in einem Ringpuffer mit fortlaufender Nummer.
// - Zugestellt wird an Takt-Grenzen: Was während eines Schritts gemeldet wird, sehen alle Leser ab dem nächsten
//   Schritt (Stage::tick ruft deliver() zu Beginn), in der Reihenfolge der Meldungen. So hängt nichts davon ab, in
//   welcher Reihenfolge die Orte rechnen, und unsichtbare Orte lesen dasselbe wie sichtbare.
// - Leser melden sich mit einer Maske der Arten an und haben je einen Lesezeiger. Wer zu langsam liest, verliert die
//   ältesten Ereignisse (gezählt in lost()).

#include <array>
#include <cstdint>

namespace pixelia {

enum class EventKind : uint8_t {
    SeasonEvening,  // Jahreszeitenabend (Kalender, Kurzmeldung)
    SeasonBegins,   // neue Jahreszeit (Kalender, Kurzmeldung)
    First,          // Erstling: erster Schnee, erstes Gewitter … (Kalender, Kurzmeldung)
    Arrived,        // Figur ist an einem Ort angekommen (place, from, who)
    Departed,       // Figur ist aufgebrochen (place = Ziel, from, who; mit Reisezeit: jetzt unterwegs)
    StateChanged,   // Ortszustand hat die Stufe gewechselt (place, a = Index in PlaceStates, b = neue Stufe)
    WeatherChanged, // Wetter-Simulation hat neues Wetter gewählt (a = Art, b = Stärke)
    SkillUp,        // Fähigkeit eine Stufe weiter (seit 0.5.9; who, a = Fähigkeit, b = Stufe)
    BondChanged,    // Beziehung hat die Stufe gewechselt (seit 0.5.9; who, other, a = Stufe, b = 1 näher, 0 entfernt)
    Stock,          // Warenbuch (seit 0.5.10, nur an Takt-Grenzen): a = 0 Lagerplatz leer, 1 voll, 2 Notbestand (auch
                    // Kurzmeldung), 3 Gemeinschaftsaufgabe beginnt (Aufgabenliste); place, text für die Chronik
    Project,        // Projekt (seit 0.5.10 Teil 3, nur an Takt-Grenzen): a = 0 Teilaufgabe fertig (b = Schritt), 1 Projekt
                    // fertig; place, text für die Chronik
    kCount,
};
const char* event_kind_name(EventKind k);  // Kennung für Protokoll und Befehle

inline constexpr uint32_t event_bit(EventKind k) { return 1u << static_cast<int>(k); }
inline constexpr uint32_t kCalendarEvents =
    event_bit(EventKind::SeasonEvening) | event_bit(EventKind::SeasonBegins) | event_bit(EventKind::First);
inline constexpr uint32_t kAllEvents = (1u << static_cast<int>(EventKind::kCount)) - 1;

struct WorldEvent {
    EventKind kind = EventKind::First;
    uint8_t a = 0, b = 0;         // je nach Art (Kalender: Jahreszeit, Erstling)
    uint16_t who = 0;             // Figur (Kennung), 0 = keine
    uint16_t other = 0;           // zweite Figur (Beziehung), 0 = keine
    int32_t day = 0;              // Welttag
    const char* place = nullptr;  // Kennung des Ortes (feste Zeichenkette der Ortsdaten), nullptr = die Welt
    const char* from = nullptr;   // Ankunft und Aufbruch: Herkunft
    bool lead = false;            // die Figur ist die Hauptfigur (Begleiterin)
    char text[56] = {};           // Kurzmeldung (UTF-8), leer = keine
};

class EventBus {
public:
    static constexpr int kQueue = 32;  // Ereignisse im Ringpuffer (je Schritt meist keins, selten mehr als zwei)
    static constexpr int kReaders = 8;

    // Leser anmelden: Index, −1 = voll. Ein neuer Leser sieht nur, was ab dem nächsten Schritt zugestellt wird
    int subscribe(uint32_t mask);
    void unsubscribe(int reader);
    // Melden: sichtbar ab dem nächsten deliver()
    void publish(const WorldEvent& e);
    // Takt-Grenze: alles bisher Gemeldete wird lesbar
    void deliver() { visible_ = written_; }
    // Nächstes zugestelltes Ereignis des Lesers (nach seiner Maske); false = keins mehr
    bool poll(int reader, WorldEvent* out);
    uint32_t published() const { return written_; }
    uint32_t lost() const { return lost_; }
    void clear();

private:
    std::array<WorldEvent, kQueue> q_{};
    uint32_t written_ = 0;  // fortlaufende Nummer des nächsten Ereignisses
    uint32_t visible_ = 0;  // bis hierhin zugestellt
    struct Reader {
        uint32_t mask = 0;
        uint32_t next = 0;
        bool used = false;
    };
    std::array<Reader, kReaders> r_{};
    uint32_t lost_ = 0;
};

}  // namespace pixelia
