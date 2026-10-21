#pragma once
// Raum-Baukasten (seit 0.5.1): Ein Ort ist eine Datenliste statt eines gemalten Bildes.
//
// - Teilekatalog (PartDef, von der Welt geliefert): Art, Ebene und Ausbaustufen; jede Stufe hat Bild, Stellfläche
//   und Benutzungspunkte. Teile sind einmal mit einer Platzhalter-Rampe gezeichnet, die Farbe kommt aus einer
//   Farbreihe (Umfärbe-Tabelle, sprite.h). Wände, Böden, Fenster, Türen, Möbel und Deko sind einzelne Teile.
// - Raumliste (RoomPart, 7 Byte je Teil): Teil, Kachel-Lage, Stufe, Farbreihe, Besitzer, gespiegelt. Zur Laufzeit
//   änderbar (Ausbau, Umräumen, Schmücken für ein Ereignis und wieder Aufräumen); der Raum zeichnet dann neu vor.
//   Erkennungsstück: Platzhalter kSignaturePart, den der Katalog über den Besitzer einem Teil zuordnet.
//   Im Bau (kPartBuilding): Statt des fertigen Teils steht dort sein Bau-Bild (z. B. halbfertiges Bett) oder der
//   Materialstapel des Katalogs; die Stellfläche bleibt belegt, benutzbar sind nur die Punkte des Bau-Bildes.
// - Kachelkarte (tiles.h) für Boden, Wände und Deko-Kacheln; jede Ebene mit eigener Farbreihe (Boden, Tapete).
// - Ausgänge: Türen und Bildränder führen in andere Orte (Kachelbereich, Ankunft, Ziel, Übergang); den Wechsel
//   macht die Bühne (Travel in scene.h).
// - Hintergrundbild: Beim Betreten zeichnet der Raum alles Unbewegte einmal vor (kCoarsePixels = 24 KB im
//   Grafik-Speicher), beim Verlassen gibt er es frei. Pro Bild nur Kopie plus bewegte Teile, mit den Figuren nach
//   Tiefe sortiert (depth.h). Ohne Hintergrund (kein Speicher) zeichnet er alles jedes Bild, mit gleichem Ergebnis.
// - Wege: Raster 25 × 15 aus Kacheln, die versperren, und Stellflächen; nächster Schritt per Breitensuche.
// Kein Setting-Inhalt: Was ein Teil darstellt und wer Besitzer ist, legt die Welt fest.

#include <cstdint>

#include "pixelia/engine/canvas.h"
#include "pixelia/engine/depth.h"
#include "pixelia/engine/scene.h"
#include "pixelia/engine/sprite.h"
#include "pixelia/engine/tiles.h"

namespace pixelia {

// Art eines Teils: wofür Figuren es benutzen (Bedeutung im Detail legt die Welt fest)
enum class PartKind : uint8_t { Deco, Seat, Bed, Table, Storage, Work, Light, Door, Window };

enum class PartLayer : uint8_t {
    Floor,     // flach am Boden (Teppich): unter allem, ohne Tiefe
    Wall,      // an der Wand (Bild, Fenster, Tür): unter allem, ohne Tiefe
    Standing,  // steht im Raum: nach Tiefe mit den Figuren sortiert
};

enum class UsePose : uint8_t { Stand, Sit, Lie };

// Benutzungspunkt: wo eine Figur steht oder sitzt, um das Teil zu benutzen
struct UsePoint {
    int8_t dx, dy;   // Kachel relativ zur Lage des Teils; darf außerhalb der Stellfläche liegen (vor dem Tisch)
    Facing facing;   // Blickrichtung bei der Benutzung
    UsePose pose;
    int8_t dz;       // Tiefenversatz der Figur, z. B. +8 = vor der Stuhllehne
};

struct PartStage {
    SpriteSheet sheet;      // Bild(er); mehr als ein Bild = bewegtes Teil, wird nicht vorgezeichnet
    uint8_t w, h;           // Stellfläche in Kacheln (Lage = linke obere Ecke)
    int8_t ox, oy;          // Bildversatz; Standard: unten bündig mit der Stellfläche, waagrecht mittig
    uint8_t frame_ticks;    // Dauer eines Bildes bei bewegten Teilen (Logik-Schritte)
    const UsePoint* uses;
    uint8_t use_count;
};

struct PartDef {
    const char* id;  // Kennung (Daten, später Speicherstand), ASCII
    PartKind kind;
    PartLayer layer;
    bool blocks;     // Stellfläche versperrt den Weg
    const PartStage* stages;  // Ausbaustufen, mindestens eine
    uint8_t stage_count;
    const PartStage* building = nullptr;  // Bild „im Bau“ (gleiche Stellfläche); nullptr = Materialstapel
};

struct PartCatalog {
    const PartDef* parts = nullptr;
    int count = 0;
    const Recolor* rows = nullptr;  // Farbreihen; Reihe 0 = wie gezeichnet
    int row_count = 0;
    // Erkennungsstück des Besitzers: Teil-Nummer, −1 = keins (dann bleibt der Platz leer)
    int (*signature)(uint8_t owner) = nullptr;
    // Materialstapel für Teile im Bau ohne eigenes Bau-Bild (mittig auf der Stellfläche, ohne Farbreihe)
    const PartStage* site = nullptr;
};

inline constexpr uint8_t kSignaturePart = 0xFF;  // Raumliste: „Erkennungsstück von Besitzer owner“
inline constexpr uint8_t kPartFlipped = 1;
inline constexpr uint8_t kPartBuilding = 2;  // im Bau

struct RoomPart {
    uint8_t part;   // Nummer im Katalog oder kSignaturePart
    uint8_t x, y;   // Kachel-Lage
    uint8_t stage;  // Ausbaustufe (darüber: die letzte vorhandene)
    uint8_t row;    // Farbreihe, 0 = wie gezeichnet
    uint8_t owner;  // Besitzer, 0 = niemand
    uint8_t flags;  // kPartFlipped, kPartBuilding
};

enum class ExitKind : uint8_t { Door, Edge };

struct Exit {
    uint8_t x, y, w, h;  // Bereich in Kacheln: wer hier ankommt, geht hinaus
    uint8_t ax, ay;      // Ankunft: Kachel, auf der man von dort kommend erscheint
    Facing facing;       // Blick bei der Ankunft (in den Ort hinein)
    ExitKind kind;
    const char* to;      // Kennung des Zielortes
    uint8_t entry;       // Ausgang im Zielort, an dem man ankommt
    Transition transition;
};

// Vorlage eines Raums (Daten); load() kopiert sie in die änderbare Raumliste
struct RoomDef {
    const char* const* layers[kTileLayers];  // je kTilesY Textzeilen oder nullptr
    const char* legend;                      // Zeichen → Kachel (tiles.h)
    uint8_t layer_rows[kTileLayers];         // Farbreihe je Ebene (Boden, Wand, Deko)
    const RoomPart* parts;
    uint8_t part_count;
    const Exit* exits;
    uint8_t exit_count;
};

class Room {
public:
    static constexpr int kMaxParts = 64;
    static constexpr int kMaxExits = 8;
    static constexpr int kFlatDepth = -1000;  // bewegte Wand- und Bodenteile liegen unter allem

    Room() = default;
    ~Room();
    Room(const Room&) = delete;
    Room& operator=(const Room&) = delete;

    // Arbeitsspeicher (~8 KB) im Grafik-Speicher anlegen; false = kein Platz
    bool init(const TileSet* tiles, const PartCatalog* catalog);
    bool ready() const { return st_ != nullptr; }
    void load(const RoomDef& def);

    // Kachelkarte; nach Änderungen invalidate() aufrufen. Alle Änderungen wirken sofort: Wege neu, und hat der Raum
    // ein Hintergrundbild, wird es gleich neu vorgezeichnet (nur auf der Grafik-Task ändern, wie das Zeichnen)
    TileMap& tiles();
    const TileMap& tiles() const;
    void set_layer_row(int layer, uint8_t row);
    void invalidate();

    // Raumliste
    int part_count() const;
    const RoomPart& part(int i) const;
    int add_part(const RoomPart& p);  // Index, −1 = voll
    void remove_part(int i);
    void set_part(int i, const RoomPart& p);
    void set_stage(int i, int stage);
    void set_building(int i, bool on);  // Zustand „im Bau“
    bool building(int i) const;
    int stage_count(int i) const;
    const PartDef* def(int i) const;  // nullptr: unbekannt oder Erkennungsstück ohne Zuordnung
    const PartStage* stage(int i) const;  // fertige Ausbaustufe (Stellfläche)
    const PartStage* look(int i) const;   // was zu sehen ist: Stufe, Bau-Bild oder Materialstapel
    int find_part(PartKind kind, int start = 0) const;  // −1 = keins
    Rect part_rect(int i) const;  // Stellfläche in groben Bildpunkten
    // Benutzungspunkt k von Teil i (beim Spiegeln mitgespiegelt); false = gibt es nicht
    bool use_point(int i, int k, int* tx, int* ty, UsePoint* out) const;

    // Ausgänge
    int exit_count() const;
    const Exit& exit(int i) const;
    int exit_at(int tx, int ty) const;  // −1 = keiner

    // Wege
    bool walkable(int tx, int ty) const;
    // Nächster Schritt von (fx, fy) nach (tx, ty) über freie Kacheln (Breitensuche, Ziel darf versperrt sein, z. B.
    // ein Bett); false = schon da oder kein Weg
    bool step_toward(int fx, int fy, int tx, int ty, Facing* dir) const;

    void tick();   // Bilder bewegter Teile
    void enter();  // Hintergrundbild anlegen und vorzeichnen
    void leave();  // Hintergrundbild freigeben
    bool has_background() const { return bg_ != nullptr; }
    const uint8_t* background() const { return bg_; }  // vorgezeichnetes Bild (Prüfungen), nullptr = keins
    void set_shadow_color(uint8_t c);

    // Prüfansicht in 400 × 240: versperrte Kacheln (Punkt), Stellflächen, Benutzungspunkte, Ausgänge (Rahmen)
    void draw_inspect(Canvas& fine, uint8_t blocked, uint8_t part, uint8_t use, uint8_t exit) const;

    // Zeichnen, Schritt 1: Hintergrund ins grobe Bild, Teile in die Tiefenliste. Danach fügt der Ort seine Figuren
    // hinzu und ruft draw(c) der Liste. with_background = false zeichnet alles direkt (Test der Gleichheit).
    DepthList& begin(Canvas& c, bool with_background = true) const;

private:
    struct Storage;
    int resolve(const RoomPart& p) const;  // Teil-Nummer, −1 = nichts
    // Lage des Bildes: unten bündig mit der Stellfläche fw × fh, waagrecht mittig
    void place(const RoomPart& p, int fw, int fh, const PartStage& look, int* x, int* y, int* depth) const;
    const uint8_t* remap_of(const RoomPart& p, const PartStage& look) const;
    Sprite frame_of(const PartStage& s) const;
    void draw_static(Canvas& c, bool standing) const;
    void rebuild_walk();
    void bake();

    const TileSet* tiles_ = nullptr;
    const PartCatalog* catalog_ = nullptr;
    Storage* st_ = nullptr;
    uint8_t* bg_ = nullptr;
    uint32_t ticks_ = 0;
};

}  // namespace pixelia
