#pragma once
// Speicherstand (seit 0.5.9, LEBENDIGKEIT.md Regel 10): Format und Ablage. Was hineinkommt, schreibt die Bühne
// (Stage::save_state, world_save.cpp): Weltzeit, Kalender, Wetter, Ortszustände, Merker, Figuren, Chronik, Besuche.
//
// - Format PXW1, alles Little Endian: Kopf (32 Byte: Kennung, CRC-32 über den Rest, Formatversion, Generation,
//   Weltzeit, Länge), danach Abschnitte (Kennung aus vier Buchstaben, Version, Länge, Daten). Unbekannte Abschnitte
//   überspringt der Leser, fehlende bleiben beim Anfangswert, jeder Abschnitt hat seine eigene Version, Datensätze
//   darin ihre Länge: So laden alte Stände in neuer Firmware weiter (und neue in alter, soweit sie sie kennt).
// - Bezüge auf Orte, Fähigkeiten und Gegenstände stehen als Kennungen in der Datei, nie als Nummern oder Zeiger.
// - Der Leser prüft jede Länge: Eine kaputte oder fremde Datei führt nie zu einem Absturz, höchstens zu einem leeren
//   Abschnitt (ok() wird false, die Werte sind 0).
// - Ablage über zwei Plätze (SaveSlots): Geschrieben wird immer der ältere Platz, geladen der gültige mit der höchsten
//   Generation. Bricht das Schreiben ab (Stromausfall), stimmt die CRC dieses Platzes nicht und der andere gilt weiter.
//   Das braucht kein atomares Umbenennen und gilt so auch für eine SD-Karte.

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace pixelia {

inline constexpr uint32_t kSaveMagic = 0x31575850;  // „PXW1“
inline constexpr uint16_t kSaveFormat = 1;
inline constexpr int kSaveHeader = 32;

struct SaveHeader {
    uint16_t format = 0;
    uint32_t generation = 0;
    int64_t world_us = 0;  // Weltzeit beim Speichern
    uint32_t bytes = 0;    // ganze Datei
};

// Schreibt in einen festen Puffer (auf dem Gerät PSRAM); läuft er über, wird ok() false und finish() liefert 0
class SaveWriter {
public:
    SaveWriter(uint8_t* buf, size_t cap) : b_(buf), cap_(cap) {}
    void begin(uint32_t generation, int64_t world_us);
    void chunk(const char tag[5], uint16_t version);  // beginnt einen Abschnitt (beendet den vorigen)
    void u8(uint8_t v) { put(&v, 1); }
    void u16(uint16_t v);
    void u32(uint32_t v);
    void i32(int32_t v) { u32(static_cast<uint32_t>(v)); }
    void u64(uint64_t v);
    void i64(int64_t v) { u64(static_cast<uint64_t>(v)); }
    void str(std::string_view s);  // Länge (1 Byte, höchstens 255) und Text
    // Datensatz mit Länge (2 Byte): der Leser kann ihn überspringen, auch wenn neue Felder dazukommen
    size_t record_begin();
    void record_end(size_t at);
    size_t finish();  // Länge und CRC in den Kopf; 0 = Puffer zu klein
    bool ok() const { return ok_; }
    size_t size() const { return n_; }

private:
    void put(const void* p, size_t n);
    void close_chunk();
    void patch32(size_t at, uint32_t v);
    uint8_t* b_;
    size_t cap_;
    size_t n_ = 0;
    size_t chunk_at_ = 0;  // 0 = kein offener Abschnitt
    bool ok_ = true;
};

// Liest einen Bereich (Abschnitt, Datensatz) mit Prüfung jeder Länge
class SaveReader {
public:
    SaveReader() = default;
    SaveReader(const uint8_t* data, size_t n) : p_(data), n_(n) {}
    uint8_t u8();
    uint16_t u16();
    uint32_t u32();
    int32_t i32() { return static_cast<int32_t>(u32()); }
    uint64_t u64();
    int64_t i64() { return static_cast<int64_t>(u64()); }
    // Text in ein festes Feld (abgeschnitten, mit Null am Ende); Rückgabe: Länge in der Datei
    size_t str(char* out, size_t cap);
    std::string_view view();       // Text ohne Kopie (gültig, solange die Daten leben)
    SaveReader record();           // nächster Datensatz mit Länge; danach steht der Leser hinter ihm
    SaveReader sub(size_t len);    // die nächsten len Byte als eigener Leser
    bool ok() const { return ok_; }
    bool at_end() const { return i_ >= n_; }
    size_t left() const { return ok_ && i_ < n_ ? n_ - i_ : 0; }

private:
    bool take(size_t k);
    const uint8_t* p_ = nullptr;
    size_t n_ = 0, i_ = 0;
    bool ok_ = true;
};

// Kopf und CRC prüfen; why = Grund in einem Satz
bool save_check(const uint8_t* data, size_t n, SaveHeader* h, const char** why = nullptr);

// Abschnitte einer geprüften Datei der Reihe nach
class SaveChunks {
public:
    SaveChunks(const uint8_t* data, size_t n) : r_(data + kSaveHeader, n > kSaveHeader ? n - kSaveHeader : 0) {}
    // nächster Abschnitt: Kennung (4 Zeichen + Null), Version, Leser über die Daten; false = keiner mehr
    bool next(char tag[5], uint16_t* version, SaveReader* data);

private:
    SaveReader r_;
};

// Zwei Plätze in einem Ordner (LittleFS /data, PC: Simulator-Ordner): <dir>/<name>-a.pxw und -b.pxw
class SaveSlots {
public:
    SaveSlots(const char* dir, const char* name);
    // Neuester gültiger Stand nach out; false = keiner. slot: 0 = a, 1 = b
    bool load(std::vector<uint8_t>* out, SaveHeader* h = nullptr, int* slot = nullptr) const;
    // Schreiben in den älteren Platz (bzw. den ungültigen); die Generation steht schon im Kopf der Daten
    bool write(const uint8_t* data, size_t n, int* slot = nullptr) const;
    // höchste gültige Generation, 0 = keine (die nächste Speicherung nimmt + 1)
    uint32_t generation() const;
    const char* path(int slot) const { return slot ? b_ : a_; }

private:
    bool read(int slot, std::vector<uint8_t>* out, SaveHeader* h) const;
    char a_[96] = {}, b_[96] = {};
    mutable int newest_ = -1;  // Platz mit dem neuesten gültigen Stand, −1 = unbekannt
};

}  // namespace pixelia
