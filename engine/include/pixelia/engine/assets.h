#pragma once
// Asset-Quelle und Grafik-Speicher (seit 0.5.5): Speicher-Stufen für Grafiken (Projektplan Kapitel 14).
//
// 1. Ablage: Pakete (*.pxa) liegen eingebettet in der Firmware, in LittleFS oder auf der microSD-Karte. Eine Quelle
//    (AssetSource) liefert eine ganze Paketdatei; welche Quelle gilt, entscheidet die Firmware (Rückfall auf
//    eingebettet, wenn eine Datei fehlt). Gezeichnet wird nie direkt aus einer Quelle.
// 2. Grafik-Speicher (AssetStore): geladene Pakete im Grafik-Speicher (memory.h, auf dem Gerät PSRAM) mit Budget.
//    Die Grundausstattung bleibt immer geladen (angeheftet), Ortspakete werden beim Betreten angeheftet, Nachbarorte
//    vorab geladen; über dem Budget fallen die am längsten unbenutzten, nicht angehefteten Pakete heraus.
// 3. Zeichnen: Orte halten Kennungen (AssetId) und lösen sie beim Betreten in Zeiger auf. Die Zeiger gelten, solange
//    das Paket angeheftet ist (die Bühne heftet die Pakete betretener Orte an, stage.h).
//
// Laden: Auf dem Gerät arbeitet eine eigene Task auf Kern 0 die Anfragen ab (work()), geweckt über set_async(); die
// Bühne wartet beim Abblenden in der dunklen Mitte, bis das Ziel geladen ist. Ohne set_async() lädt want() sofort
// im Aufrufer (Simulator, Tests: deterministisch).
//
// Format PXA1 (Details und Werkzeug: docs/STIL.md Kapitel 9, tools/asset_pack.py), alles Little Endian:
//   Kopf 20 Byte: "PXA1", u16 Einträge, u16 Bytes der Namen, u32 Bytes der Daten, u32 Bytes entpackt,
//                 u32 CRC-32 über alles nach dem Kopf
//   je Eintrag 20 Byte: u32 Kennung, u8 Art, u8 Packung, u16 Name (Versatz), u32 Versatz, u32 gepackt, u32 entpackt
//   Namen (nullterminiert), dann die Daten. Packung 0 = roh, 1 = PackBits (wie /api/screen).
//   Bild: u16 Breite, u16 Höhe, u16 Bilder, u8 Zustände, u8 eigene Farben, i16 Fußpunkt x, i16 Fußpunkt y,
//         eigene Farben (je 3 Byte RGB), dann Bild für Bild Breite × Höhe Paletten-Indizes (0 = durchsichtig).
//         Bilder je Zustand = Bilder / Zustände, Zustand für Zustand hintereinander (Lampe: aus, an).
//         Eigene Farben stehen im Bild als Indizes kOwnColorBase + k und werden beim Zeichnen umgefärbt.
//   Kacheln: u16 Anzahl, dann Anzahl Eigenschaften (tiles.h), dann je Kachel 64 Indizes
//   Farbreihe: 256 Byte Umfärbe-Tabelle (sprite.h, Recolor)
//   Palette: u8 Anzahl, dann je Farbe 3 Byte RGB (Grundfarben ab Index 0)
//   Daten: beliebige Bytes

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string_view>

#include "pixelia/engine/color.h"
#include "pixelia/engine/sprite.h"
#include "pixelia/engine/tiles.h"

namespace pixelia {

using AssetId = uint32_t;

// Kennung aus dem Namen (FNV-1a, 32 Bit); gleich im Werkzeug (tools/asset_pack.py)
constexpr AssetId asset_id(std::string_view name) {
    uint32_t h = 2166136261u;
    for (char c : name) {
        h ^= static_cast<uint8_t>(c);
        h *= 16777619u;
    }
    return h;
}

enum class AssetType : uint8_t { Image = 1, Tiles = 2, Recolor = 3, Palette = 4, Data = 5 };

inline constexpr uint8_t kOwnColorBase = 224;  // eigene Farben eines Bildes: 224 … 239 (höchstens 16)
inline constexpr int kMaxOwnColors = 16;
inline constexpr int kPackHeaderBytes = 20;
inline constexpr int kPackEntryBytes = 20;

// PackBits entpacken; false = Daten passen nicht zur erwarteten Größe
bool unpack_bits(const uint8_t* src, size_t n, uint8_t* dst, size_t out);

// Ein Eintrag eines geladenen Pakets (entpackt im Grafik-Speicher)
struct AssetEntry {
    AssetId id = 0;
    AssetType type = AssetType::Data;
    const uint8_t* data = nullptr;
    uint32_t size = 0;
    const char* name = "";
};

// Bild als Bildstreifen (sprite.h); own = eigene Farben (RGB), own_count = 0: nur Pixelia-Farben
struct ImageAsset {
    SpriteSheet sheet;
    const uint8_t* own = nullptr;
    int own_count = 0;
};

// --- Ablage ---

// Inhalt einer Paketdatei: data gilt bis release(); owned != nullptr = im Grafik-Speicher kopiert
struct AssetBlob {
    const uint8_t* data = nullptr;
    size_t size = 0;
    uint8_t* owned = nullptr;
    void release();
};

class AssetSource {
public:
    virtual ~AssetSource() = default;
    virtual const char* name() const = 0;  // eingebettet, littlefs, sd
    // Ganze Datei (z. B. „raum.pxa“); false = gibt es nicht oder Lesefehler
    virtual bool load(const char* file, AssetBlob* out) = 0;
};

// In die Firmware bzw. den Simulator eingebettete Pakete (erzeugt tools/asset_pack.py beim Build)
struct EmbeddedAsset {
    const char* file;
    const uint8_t* data;
    uint32_t size;
};
extern const EmbeddedAsset kEmbeddedAssets[];
extern const int kEmbeddedAssetCount;

class EmbeddedSource : public AssetSource {
public:
    const char* name() const override { return "eingebettet"; }
    bool load(const char* file, AssetBlob* out) override;  // ohne Kopie
};

// Dateien in einem Ordner (stdio): auf dem Gerät LittleFS (/data/…) oder SD (/sd/…), im Simulator ein PC-Ordner
class FileSource : public AssetSource {
public:
    FileSource(const char* label, const char* dir) : label_(label), dir_(dir) {}
    const char* name() const override { return label_; }
    const char* dir() const { return dir_; }
    bool load(const char* file, AssetBlob* out) override;

private:
    const char* label_;
    const char* dir_;
};

// --- Grafik-Speicher ---

class AssetStore {
public:
    static constexpr int kMaxPacks = 24;

    enum class State : uint8_t { Unknown, Queued, Loading, Ready, Failed };

    struct PackInfo {
        char name[16];
        State state;
        uint8_t pins;
        bool base;           // Grundausstattung (nie verdrängt)
        uint32_t bytes;      // im Grafik-Speicher (entpackt)
        uint32_t file_bytes; // gepackt in der Ablage
        uint32_t read_us;    // Lesen aus der Ablage
        uint32_t unpack_us;  // Prüfen und Entpacken
        uint32_t wait_us;    // von der Anfrage bis geladen (inkl. Warteschlange)
        const char* source;  // Quelle des letzten Ladens
        uint16_t loads;
    };
    struct Stats {
        uint32_t used = 0, budget = 0, peak = 0;
        uint32_t loads = 0, evictions = 0, failures = 0, over_budget = 0;
        uint32_t generation = 0;
        int packs = 0;
    };

    // Quelle (primary) und Rückfall, wenn die Datei dort fehlt (eingebettet); vor dem ersten Laden setzen
    void set_sources(AssetSource* primary, AssetSource* fallback);
    AssetSource* source() const { return primary_; }
    void set_budget(uint32_t bytes) { budget_ = bytes; }
    void set_clock(uint64_t (*now_us)()) { now_ = now_us; }
    // Laden auf einer eigenen Task: wake() weckt sie, sie ruft work(), bis es false liefert. nullptr = synchron
    void set_async(void (*wake)()) { wake_ = wake; }
    bool async() const { return wake_ != nullptr; }

    // Grundausstattung: sofort laden (auch auf dem Gerät im Aufrufer) und nie verdrängen; false = fehlt
    bool load_base(const char* pack);
    // Paket wird gebraucht: soon = gleich (Ortswechsel), sonst Vorladen. name muss dauerhaft gültig sein
    void want(const char* pack, bool soon);
    void pin(const char* pack);    // anheften (= want soon); angeheftete Pakete werden nie verdrängt
    void unpin(const char* pack);
    bool ready(const char* pack) const;   // geladen oder fehlgeschlagen (dann fehlen seine Bilder)
    bool loaded(const char* pack) const;  // wirklich geladen
    // Nicht angeheftete Pakete freigeben (Messung, Quellenwechsel); Grundausstattung bleibt
    void drop_unpinned();

    // Abarbeiten (Lade-Task bzw. synchron): eine Anfrage; false = nichts zu tun
    bool work();

    // Nachschlagen: Zeiger gelten, solange das Paket angeheftet ist
    const AssetEntry* find(AssetId id) const;
    bool image(AssetId id, ImageAsset* out) const;
    bool tiles(AssetId id, TileSet* out) const;
    const uint8_t* recolor(AssetId id) const;                // 256 Byte
    const uint8_t* palette(AssetId id, int* count) const;    // count × RGB
    uint32_t generation() const { return generation_.load(std::memory_order_relaxed); }  // zählt bei jedem Laden und Verdrängen

    Stats stats() const;
    int list(PackInfo* out, int max) const;
    bool last(PackInfo* out) const;  // zuletzt von work() bearbeitetes Paket (Log der Lade-Task)

    // Messung ohne Speichern: Paket aus einer Quelle lesen, prüfen, entpacken und wieder freigeben
    struct Probe {
        bool ok = false;
        uint32_t file_bytes = 0, bytes = 0, read_us = 0, unpack_us = 0;
        int entries = 0;
    };
    Probe probe(AssetSource& src, const char* file);

private:
    struct Pack {
        const char* name = nullptr;
        AssetId id = 0;
        State state = State::Unknown;
        uint8_t pins = 0;
        bool base = false;
        bool soon = false;
        uint8_t* mem = nullptr;  // Einträge + Namen + Daten (entpackt)
        const AssetEntry* entries = nullptr;
        int count = 0;
        uint32_t bytes = 0, file_bytes = 0, read_us = 0, unpack_us = 0, wait_us = 0;
        uint64_t asked = 0;
        uint32_t last_use = 0;
        uint16_t loads = 0;
        const char* source = "";
    };
    Pack* get(const char* name);  // anlegen, falls neu
    const Pack* get(const char* name) const;
    void touch(Pack& p) { p.last_use = ++use_clock_; }
    uint64_t now() const { return now_ ? now_() : 0; }
    // Datei laden und entpacken (ohne Sperre, Quellen vorher unter der Sperre übernommen); false = fehlt oder kaputt
    bool read_pack(const char* name, AssetSource* primary, AssetSource* fallback, Pack* out, const char** source);
    static void info_of(const Pack& p, PackInfo& o);
    void free_pack(Pack& p);
    void evict_for(uint32_t bytes, const Pack* keep);

    mutable std::mutex lock_;
    Pack packs_[kMaxPacks];
    int count_ = 0;
    int last_ = -1;
    AssetSource* primary_ = nullptr;
    AssetSource* fallback_ = nullptr;
    uint32_t budget_ = 2u << 20;
    uint32_t used_ = 0, peak_ = 0;
    uint32_t loads_ = 0, evictions_ = 0, failures_ = 0, over_budget_ = 0;
    std::atomic<uint32_t> generation_{0};
    uint32_t use_clock_ = 0;
    uint64_t (*now_)() = nullptr;
    void (*wake_)() = nullptr;
};

// Grafik-Speicher des Programms (Bühne, Testorte, Firmware); Quelle zu Beginn eingebettet
AssetStore& asset_store();
EmbeddedSource& embedded_source();

}  // namespace pixelia
