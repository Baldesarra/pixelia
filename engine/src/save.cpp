#include "pixelia/engine/save.h"

#include <cstdio>
#include <cstring>
#include <utility>

#include "pixelia/engine/png.h"

namespace pixelia {

// --- Schreiben ---

void SaveWriter::put(const void* p, size_t n) {
    if (!ok_ || n_ + n > cap_) {
        ok_ = false;
        return;
    }
    std::memcpy(b_ + n_, p, n);
    n_ += n;
}

void SaveWriter::u16(uint16_t v) {
    const uint8_t b[2] = {static_cast<uint8_t>(v), static_cast<uint8_t>(v >> 8)};
    put(b, 2);
}

void SaveWriter::u32(uint32_t v) {
    const uint8_t b[4] = {static_cast<uint8_t>(v), static_cast<uint8_t>(v >> 8), static_cast<uint8_t>(v >> 16),
                          static_cast<uint8_t>(v >> 24)};
    put(b, 4);
}

void SaveWriter::u64(uint64_t v) {
    u32(static_cast<uint32_t>(v));
    u32(static_cast<uint32_t>(v >> 32));
}

void SaveWriter::str(std::string_view s) {
    const size_t n = s.size() < 255 ? s.size() : 255;
    u8(static_cast<uint8_t>(n));
    put(s.data(), n);
}

void SaveWriter::patch32(size_t at, uint32_t v) {
    if (!ok_ || at + 4 > n_) return;
    for (int k = 0; k < 4; ++k) b_[at + k] = static_cast<uint8_t>(v >> (8 * k));
}

void SaveWriter::begin(uint32_t generation, int64_t world_us) {
    n_ = 0;
    ok_ = true;
    chunk_at_ = 0;
    u32(kSaveMagic);
    u32(0);  // CRC
    u16(kSaveFormat);
    u16(kSaveHeader);
    u32(generation);
    i64(world_us);
    u32(0);  // Länge
    u32(0);  // frei
}

void SaveWriter::close_chunk() {
    if (chunk_at_ == 0) return;
    patch32(chunk_at_ + 6, static_cast<uint32_t>(n_ - chunk_at_ - 10));
    chunk_at_ = 0;
}

void SaveWriter::chunk(const char tag[5], uint16_t version) {
    close_chunk();
    chunk_at_ = n_;
    put(tag, 4);
    u16(version);
    u32(0);  // Länge, beim Schließen
}

size_t SaveWriter::record_begin() {
    const size_t at = n_;
    u16(0);
    return at;
}

void SaveWriter::record_end(size_t at) {
    const size_t len = n_ - at - 2;
    if (!ok_ || len > 0xFFFF) {
        ok_ = false;
        return;
    }
    b_[at] = static_cast<uint8_t>(len);
    b_[at + 1] = static_cast<uint8_t>(len >> 8);
}

size_t SaveWriter::finish() {
    close_chunk();
    if (!ok_ || n_ < kSaveHeader) return 0;
    patch32(24, static_cast<uint32_t>(n_));
    patch32(4, crc32(b_ + 8, n_ - 8));
    return n_;
}

// --- Lesen ---

bool SaveReader::take(size_t k) {
    if (!ok_ || i_ + k > n_) {
        ok_ = false;
        i_ = n_;
        return false;
    }
    return true;
}

uint8_t SaveReader::u8() {
    if (!take(1)) return 0;
    return p_[i_++];
}

uint16_t SaveReader::u16() {
    if (!take(2)) return 0;
    const uint16_t v = static_cast<uint16_t>(p_[i_] | (p_[i_ + 1] << 8));
    i_ += 2;
    return v;
}

uint32_t SaveReader::u32() {
    if (!take(4)) return 0;
    uint32_t v = 0;
    for (int k = 0; k < 4; ++k) v |= static_cast<uint32_t>(p_[i_ + k]) << (8 * k);
    i_ += 4;
    return v;
}

uint64_t SaveReader::u64() {
    const uint64_t lo = u32();
    return lo | (static_cast<uint64_t>(u32()) << 32);
}

std::string_view SaveReader::view() {
    const size_t n = u8();
    if (!take(n)) return {};
    const std::string_view s(reinterpret_cast<const char*>(p_ + i_), n);
    i_ += n;
    return s;
}

size_t SaveReader::str(char* out, size_t cap) {
    const std::string_view s = view();
    if (cap == 0) return s.size();
    const size_t k = s.size() < cap - 1 ? s.size() : cap - 1;
    std::memcpy(out, s.data(), k);
    std::memset(out + k, 0, cap - k);
    return s.size();
}

SaveReader SaveReader::sub(size_t len) {
    if (!take(len)) {
        SaveReader bad;
        bad.ok_ = false;
        return bad;
    }
    SaveReader r(p_ + i_, len);
    i_ += len;
    return r;
}

SaveReader SaveReader::record() { return sub(u16()); }

bool save_check(const uint8_t* data, size_t n, SaveHeader* h, const char** why) {
    auto fail = [&](const char* w) {
        if (why) *why = w;
        return false;
    };
    if (!data || n < kSaveHeader) return fail("zu kurz");
    SaveReader r(data, n);
    if (r.u32() != kSaveMagic) return fail("kein Pixelia-Speicherstand");
    const uint32_t crc = r.u32();
    SaveHeader hd;
    hd.format = r.u16();
    const uint16_t head = r.u16();
    hd.generation = r.u32();
    hd.world_us = r.i64();
    hd.bytes = r.u32();
    if (head != kSaveHeader) return fail("unbekannter Kopf");
    if (hd.bytes != n) return fail("Länge stimmt nicht (abgebrochen?)");
    if (crc32(data + 8, n - 8) != crc) return fail("Prüfsumme stimmt nicht");
    if (hd.format == 0 || hd.format > kSaveFormat) return fail("neueres Format");
    if (h) *h = hd;
    if (why) *why = "";
    return true;
}

bool SaveChunks::next(char tag[5], uint16_t* version, SaveReader* data) {
    if (r_.left() < 10) return false;
    for (int k = 0; k < 4; ++k) tag[k] = static_cast<char>(r_.u8());
    tag[4] = 0;
    *version = r_.u16();
    const uint32_t len = r_.u32();
    if (len > r_.left()) return false;  // kaputt: der Rest zählt nicht (die CRC hat das schon ausgeschlossen)
    *data = r_.sub(len);
    return data->ok();
}

// --- Ablage ---

SaveSlots::SaveSlots(const char* dir, const char* name) {
    std::snprintf(a_, sizeof(a_), "%s/%s-a.pxw", dir, name);
    std::snprintf(b_, sizeof(b_), "%s/%s-b.pxw", dir, name);
}

bool SaveSlots::read(int slot, std::vector<uint8_t>* out, SaveHeader* h) const {
    FILE* f = std::fopen(path(slot), "rb");
    if (!f) return false;
    std::fseek(f, 0, SEEK_END);
    const long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    bool ok = size >= kSaveHeader && size < 4 * 1024 * 1024;
    if (ok) {
        out->resize(static_cast<size_t>(size));
        ok = std::fread(out->data(), 1, out->size(), f) == out->size();
    }
    std::fclose(f);
    return ok && save_check(out->data(), out->size(), h);
}

bool SaveSlots::load(std::vector<uint8_t>* out, SaveHeader* h, int* slot) const {
    std::vector<uint8_t> d[2];
    SaveHeader hd[2];
    const bool v0 = read(0, &d[0], &hd[0]), v1 = read(1, &d[1], &hd[1]);
    if (!v0 && !v1) return false;
    const int s = !v0 ? 1 : !v1 ? 0 : hd[1].generation > hd[0].generation ? 1 : 0;
    *out = std::move(d[s]);
    if (h) *h = hd[s];
    if (slot) *slot = s;
    newest_ = s;
    return true;
}

uint32_t SaveSlots::generation() const {
    std::vector<uint8_t> d;
    SaveHeader h;
    return load(&d, &h) ? h.generation : 0;
}

bool SaveSlots::write(const uint8_t* data, size_t n, int* slot) const {
    // in den älteren bzw. ungültigen Platz: der neueste gültige bleibt unberührt, bis dieser fertig ist. Welcher das
    // ist, merkt sich die Ablage nach dem Laden bzw. Schreiben; sonst liest sie beide einmal
    int s = newest_ >= 0 ? 1 - newest_ : 0;
    if (newest_ < 0) {
        std::vector<uint8_t> d;
        SaveHeader h0, h1;
        const bool v0 = read(0, &d, &h0), v1 = read(1, &d, &h1);
        s = !v0 ? 0 : !v1 ? 1 : h0.generation <= h1.generation ? 0 : 1;
    }
    FILE* f = std::fopen(path(s), "wb");
    bool ok = f != nullptr;
    if (f) {
        ok = std::fwrite(data, 1, n, f) == n;
        ok = std::fflush(f) == 0 && ok;
        ok = std::fclose(f) == 0 && ok;
    }
    newest_ = ok ? s : -1;  // fehlgeschlagen: beim nächsten Mal beide neu prüfen
    if (slot) *slot = s;
    return ok;
}

}  // namespace pixelia
