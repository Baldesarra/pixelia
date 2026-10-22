#include "pixelia/engine/assets.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <new>

#include "pixelia/engine/memory.h"
#include "pixelia/engine/png.h"

namespace pixelia {

namespace {

uint16_t rd16(const uint8_t* p) { return static_cast<uint16_t>(p[0] | p[1] << 8); }
uint32_t rd32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) | static_cast<uint32_t>(p[1]) << 8 | static_cast<uint32_t>(p[2]) << 16 |
           static_cast<uint32_t>(p[3]) << 24;
}

constexpr int kImageHeader = 12;

}  // namespace

bool unpack_bits(const uint8_t* src, size_t n, uint8_t* dst, size_t out) {
    size_t i = 0, o = 0;
    while (i < n) {
        const uint8_t c = src[i++];
        if (c < 128) {
            const size_t len = c + 1u;
            if (i + len > n || o + len > out) return false;
            std::memcpy(dst + o, src + i, len);
            i += len;
            o += len;
        } else if (c > 128) {
            const size_t len = 257u - c;
            if (i >= n || o + len > out) return false;
            std::memset(dst + o, src[i++], len);
            o += len;
        }
    }
    return o == out;
}

void AssetBlob::release() {
    if (owned) asset_free(owned);
    owned = nullptr;
    data = nullptr;
    size = 0;
}

bool EmbeddedSource::load(const char* file, AssetBlob* out) {
    for (int i = 0; i < kEmbeddedAssetCount; ++i)
        if (std::strcmp(kEmbeddedAssets[i].file, file) == 0) {
            *out = AssetBlob{kEmbeddedAssets[i].data, kEmbeddedAssets[i].size, nullptr};
            return true;
        }
    return false;
}

bool FileSource::load(const char* file, AssetBlob* out) {
    char path[96];
    std::snprintf(path, sizeof(path), "%s/%s", dir_, file);
    std::FILE* f = std::fopen(path, "rb");
    if (!f) return false;
    long size = -1;
    if (std::fseek(f, 0, SEEK_END) == 0) size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    bool ok = false;
    if (size > 0) {
        auto* buf = static_cast<uint8_t*>(asset_alloc(static_cast<size_t>(size)));
        if (buf && std::fread(buf, 1, static_cast<size_t>(size), f) == static_cast<size_t>(size)) {
            *out = AssetBlob{buf, static_cast<size_t>(size), buf};
            ok = true;
        } else if (buf) {
            asset_free(buf);
        }
    }
    std::fclose(f);
    return ok;
}

// --- Paket aus einer Datei: prüfen und entpacken ---

namespace {

// Speicher eines geladenen Pakets: Eintragstabelle, Namen, entpackte Daten (je auf 4 Byte ausgerichtet)
struct Decoded {
    uint8_t* mem = nullptr;
    const AssetEntry* entries = nullptr;
    int count = 0;
    uint32_t bytes = 0;
};

bool decode_pack(const uint8_t* f, size_t size, Decoded* out) {
    if (size < kPackHeaderBytes || std::memcmp(f, "PXA1", 4) != 0) return false;
    const int count = rd16(f + 4);
    const uint32_t names = rd16(f + 6);
    const uint32_t data = rd32(f + 8);
    const uint32_t unpacked = rd32(f + 12);
    const size_t dir = kPackHeaderBytes;
    const size_t name_at = dir + static_cast<size_t>(count) * kPackEntryBytes;
    const size_t data_at = name_at + names;
    if (data_at + data != size) return false;
    if (crc32(f + kPackHeaderBytes, size - kPackHeaderBytes) != rd32(f + 16)) return false;
    if (unpacked > (64u << 20)) return false;

    const size_t table = sizeof(AssetEntry) * static_cast<size_t>(count);
    const size_t total = table + names + unpacked + 4u * static_cast<size_t>(count) + 4;
    auto* mem = static_cast<uint8_t*>(asset_alloc(total));
    if (!mem) return false;
    auto* entries = reinterpret_cast<AssetEntry*>(mem);
    for (int i = 0; i < count; ++i) new (&entries[i]) AssetEntry{};
    char* name_mem = reinterpret_cast<char*>(mem + table);
    std::memcpy(name_mem, f + name_at, names);
    if (names) name_mem[names - 1] = 0;
    size_t at = (table + names + 3) & ~size_t{3};
    for (int i = 0; i < count; ++i) {
        const uint8_t* e = f + dir + static_cast<size_t>(i) * kPackEntryBytes;
        const uint8_t codec = e[5];
        const uint32_t name = rd16(e + 6), off = rd32(e + 8), packed = rd32(e + 12), size_out = rd32(e + 16);
        if (off + packed > data || at + size_out > total || name >= std::max<uint32_t>(names, 1)) {
            asset_free(mem);
            return false;
        }
        const uint8_t* src = f + data_at + off;
        uint8_t* dst = mem + at;
        const bool ok = codec == 0   ? (packed == size_out && (std::memcpy(dst, src, packed), true))
                        : codec == 1 ? unpack_bits(src, packed, dst, size_out)
                                     : false;
        if (!ok) {
            asset_free(mem);
            return false;
        }
        entries[i] = AssetEntry{rd32(e), static_cast<AssetType>(e[4]), dst, size_out, names ? name_mem + name : ""};
        at = (at + size_out + 3) & ~size_t{3};
    }
    out->mem = mem;
    out->entries = entries;
    out->count = count;
    out->bytes = static_cast<uint32_t>(total);
    return true;
}

}  // namespace

// --- Grafik-Speicher ---

void AssetStore::set_sources(AssetSource* primary, AssetSource* fallback) {
    std::lock_guard<std::mutex> g(lock_);
    primary_ = primary;
    fallback_ = fallback == primary ? nullptr : fallback;
}

AssetStore::Pack* AssetStore::get(const char* name) {
    const AssetId id = asset_id(name);
    for (int i = 0; i < count_; ++i)
        if (packs_[i].id == id) return &packs_[i];
    if (count_ >= kMaxPacks) return nullptr;
    Pack& p = packs_[count_++];
    p = Pack{};
    p.name = name;
    p.id = id;
    return &p;
}

const AssetStore::Pack* AssetStore::get(const char* name) const {
    const AssetId id = asset_id(name);
    for (int i = 0; i < count_; ++i)
        if (packs_[i].id == id) return &packs_[i];
    return nullptr;
}

bool AssetStore::read_pack(const char* name, Pack* out, const char** source) {
    char file[40];
    std::snprintf(file, sizeof(file), "%s.pxa", name);
    AssetSource* srcs[2] = {primary_, fallback_};
    for (AssetSource* src : srcs) {
        if (!src) continue;
        const uint64_t t0 = now();
        AssetBlob blob;
        if (!src->load(file, &blob)) continue;
        const uint64_t t1 = now();
        Decoded d;
        const bool ok = decode_pack(blob.data, blob.size, &d);
        const uint64_t t2 = now();
        const auto file_bytes = static_cast<uint32_t>(blob.size);
        blob.release();
        if (!ok) continue;  // kaputt: nächste Quelle versuchen
        out->mem = d.mem;
        out->entries = d.entries;
        out->count = d.count;
        out->bytes = d.bytes;
        out->file_bytes = file_bytes;
        out->read_us = static_cast<uint32_t>(t1 - t0);
        out->unpack_us = static_cast<uint32_t>(t2 - t1);
        *source = src->name();
        return true;
    }
    return false;
}

void AssetStore::free_pack(Pack& p) {
    if (p.mem) asset_free(p.mem);
    used_ -= p.bytes;
    p.mem = nullptr;
    p.entries = nullptr;
    p.count = 0;
    p.bytes = 0;
    p.state = State::Unknown;
    ++generation_;
}

void AssetStore::evict_for(uint32_t bytes, const Pack* keep) {
    while (used_ + bytes > budget_) {
        Pack* victim = nullptr;
        for (int i = 0; i < count_; ++i) {
            Pack& p = packs_[i];
            if (&p == keep || p.base || p.pins || p.state != State::Ready || !p.mem) continue;
            if (!victim || p.last_use < victim->last_use) victim = &p;
        }
        if (!victim) {
            ++over_budget_;  // alles angeheftet: trotzdem laden, die Messung zeigt es
            return;
        }
        free_pack(*victim);
        ++evictions_;
    }
}

bool AssetStore::load_base(const char* pack) {
    {
        std::lock_guard<std::mutex> g(lock_);
        Pack* p = get(pack);
        if (!p) return false;
        p->base = true;
        if (p->state == State::Ready) return true;
        p->state = State::Loading;
        p->asked = now();
    }
    Pack loaded;
    const char* source = "";
    const bool ok = read_pack(pack, &loaded, &source);
    std::lock_guard<std::mutex> g(lock_);
    Pack* p = get(pack);
    if (!ok) {
        p->state = State::Failed;
        ++failures_;
        return false;
    }
    p->mem = loaded.mem;
    p->entries = loaded.entries;
    p->count = loaded.count;
    p->bytes = loaded.bytes;
    p->file_bytes = loaded.file_bytes;
    p->read_us = loaded.read_us;
    p->unpack_us = loaded.unpack_us;
    p->wait_us = static_cast<uint32_t>(now() - p->asked);
    p->source = source;
    p->state = State::Ready;
    ++p->loads;
    used_ += p->bytes;
    peak_ = std::max(peak_, used_);
    ++loads_;
    ++generation_;
    return true;
}

void AssetStore::want(const char* pack, bool soon) {
    bool wake = false;
    {
        std::lock_guard<std::mutex> g(lock_);
        Pack* p = get(pack);
        if (!p) return;
        touch(*p);
        if (p->state == State::Unknown) {
            p->state = State::Queued;
            p->soon = soon;
            p->asked = now();
            wake = true;
        } else if (p->state == State::Queued && soon && !p->soon) {
            p->soon = true;
            wake = true;
        }
    }
    if (!wake) return;
    if (wake_)
        wake_();
    else
        while (work()) {
        }
}

void AssetStore::pin(const char* pack) {
    {
        std::lock_guard<std::mutex> g(lock_);
        Pack* p = get(pack);
        if (!p) return;
        if (p->pins < 255) ++p->pins;
    }
    want(pack, true);
}

void AssetStore::unpin(const char* pack) {
    std::lock_guard<std::mutex> g(lock_);
    Pack* p = get(pack);
    if (p && p->pins) {
        --p->pins;
        touch(*p);
    }
}

bool AssetStore::ready(const char* pack) const {
    std::lock_guard<std::mutex> g(lock_);
    const Pack* p = get(pack);
    return p && (p->state == State::Ready || p->state == State::Failed);
}

bool AssetStore::loaded(const char* pack) const {
    std::lock_guard<std::mutex> g(lock_);
    const Pack* p = get(pack);
    return p && p->state == State::Ready;
}

void AssetStore::drop_unpinned() {
    std::lock_guard<std::mutex> g(lock_);
    for (int i = 0; i < count_; ++i) {
        Pack& p = packs_[i];
        if (p.base || p.pins) continue;
        if (p.state == State::Ready) free_pack(p);
        if (p.state == State::Failed || p.state == State::Queued) p.state = State::Unknown;
    }
}

bool AssetStore::work() {
    const char* name = nullptr;
    {
        std::lock_guard<std::mutex> g(lock_);
        Pack* next = nullptr;
        for (int i = 0; i < count_; ++i) {
            Pack& p = packs_[i];
            if (p.state != State::Queued) continue;
            // gleich gebraucht vor Vorladen, sonst in der Reihenfolge der Anfragen
            if (!next || (p.soon && !next->soon) || (p.soon == next->soon && p.asked < next->asked)) next = &p;
        }
        if (!next) return false;
        next->state = State::Loading;
        name = next->name;
    }
    Pack loaded;
    const char* source = "";
    const bool ok = read_pack(name, &loaded, &source);  // ohne Sperre: die Bühne zeichnet weiter
    std::lock_guard<std::mutex> g(lock_);
    Pack* p = get(name);
    if (!ok) {
        p->state = State::Failed;
        ++failures_;
        ++generation_;
        return true;
    }
    evict_for(loaded.bytes, p);
    p->mem = loaded.mem;
    p->entries = loaded.entries;
    p->count = loaded.count;
    p->bytes = loaded.bytes;
    p->file_bytes = loaded.file_bytes;
    p->read_us = loaded.read_us;
    p->unpack_us = loaded.unpack_us;
    p->wait_us = static_cast<uint32_t>(now() - p->asked);
    p->source = source;
    p->state = State::Ready;
    ++p->loads;
    used_ += p->bytes;
    peak_ = std::max(peak_, used_);
    ++loads_;
    ++generation_;
    return true;
}

const AssetEntry* AssetStore::find(AssetId id) const {
    std::lock_guard<std::mutex> g(lock_);
    for (int i = 0; i < count_; ++i) {
        const Pack& p = packs_[i];
        if (p.state != State::Ready) continue;
        for (int k = 0; k < p.count; ++k)
            if (p.entries[k].id == id) return &p.entries[k];
    }
    return nullptr;
}

bool AssetStore::image(AssetId id, ImageAsset* out) const {
    const AssetEntry* e = find(id);
    if (!e || e->type != AssetType::Image || e->size < kImageHeader) return false;
    const uint8_t* d = e->data;
    const int w = rd16(d), h = rd16(d + 2), frames = rd16(d + 4), states = d[6], own = d[7];
    const size_t need = kImageHeader + static_cast<size_t>(own) * 3 + static_cast<size_t>(w) * h * frames;
    if (need > e->size || states < 1 || frames % states != 0 || own > kMaxOwnColors) return false;
    out->sheet = SpriteSheet{d + kImageHeader + own * 3, w, h, frames,
                             static_cast<int16_t>(rd16(d + 8)), static_cast<int16_t>(rd16(d + 10)), states};
    out->own = own ? d + kImageHeader : nullptr;
    out->own_count = own;
    return true;
}

bool AssetStore::tiles(AssetId id, TileSet* out) const {
    const AssetEntry* e = find(id);
    if (!e || e->type != AssetType::Tiles || e->size < 2) return false;
    const int n = rd16(e->data);
    if (2u + static_cast<size_t>(n) * (1 + kTilePixels) > e->size) return false;
    *out = TileSet{e->data + 2 + n, e->data + 2, n};
    return true;
}

const uint8_t* AssetStore::recolor(AssetId id) const {
    const AssetEntry* e = find(id);
    return e && e->type == AssetType::Recolor && e->size == 256 ? e->data : nullptr;
}

const uint8_t* AssetStore::palette(AssetId id, int* count) const {
    const AssetEntry* e = find(id);
    if (!e || e->type != AssetType::Palette || e->size < 1 || 1u + e->data[0] * 3u > e->size) return nullptr;
    if (count) *count = e->data[0];
    return e->data + 1;
}

AssetStore::Stats AssetStore::stats() const {
    std::lock_guard<std::mutex> g(lock_);
    Stats s;
    s.used = used_;
    s.budget = budget_;
    s.peak = peak_;
    s.loads = loads_;
    s.evictions = evictions_;
    s.failures = failures_;
    s.over_budget = over_budget_;
    s.generation = generation_;
    for (int i = 0; i < count_; ++i) s.packs += packs_[i].state == State::Ready;
    return s;
}

int AssetStore::list(PackInfo* out, int max) const {
    std::lock_guard<std::mutex> g(lock_);
    int n = 0;
    for (int i = 0; i < count_ && n < max; ++i) {
        const Pack& p = packs_[i];
        PackInfo& o = out[n++];
        std::snprintf(o.name, sizeof(o.name), "%s", p.name);
        o.state = p.state;
        o.pins = p.pins;
        o.base = p.base;
        o.bytes = p.bytes;
        o.file_bytes = p.file_bytes;
        o.read_us = p.read_us;
        o.unpack_us = p.unpack_us;
        o.wait_us = p.wait_us;
        o.source = p.source;
        o.loads = p.loads;
    }
    return n;
}

AssetStore::Probe AssetStore::probe(AssetSource& src, const char* file) {
    Probe r;
    const uint64_t t0 = now();
    AssetBlob blob;
    if (!src.load(file, &blob)) return r;
    const uint64_t t1 = now();
    Decoded d;
    r.ok = decode_pack(blob.data, blob.size, &d);
    const uint64_t t2 = now();
    r.file_bytes = static_cast<uint32_t>(blob.size);
    r.read_us = static_cast<uint32_t>(t1 - t0);
    r.unpack_us = static_cast<uint32_t>(t2 - t1);
    if (r.ok) {
        r.bytes = d.bytes;
        r.entries = d.count;
        asset_free(d.mem);
    }
    blob.release();
    return r;
}

EmbeddedSource& embedded_source() {
    static EmbeddedSource src;
    return src;
}

AssetStore& asset_store() {
    static AssetStore* store = [] {
        static AssetStore s;
        s.set_sources(&embedded_source(), nullptr);  // die Firmware stellt um (LittleFS, SD)
        return &s;
    }();
    return *store;
}

}  // namespace pixelia
