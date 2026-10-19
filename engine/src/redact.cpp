#include "pixelia/engine/redact.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <utility>

namespace pixelia {

namespace {
// Speicher sicher überschreiben (der Optimierer darf das nicht weglassen)
void wipe(std::string& s) {
    volatile char* p = s.data();
    for (size_t i = 0; i < s.size(); ++i) p[i] = 0;
    s.clear();
}
}  // namespace

SecretFilter::SecretFilter(Sink sink, size_t block) : sink_(std::move(sink)), block_(std::max<size_t>(block, 64)) {
    // Einmal passend reservieren (auf dem Gerät landen Blöcke ab 16 KB im PSRAM statt im internen RAM)
    pending_.reserve(block_ + 256);
}

SecretFilter::~SecretFilter() {
    for (auto& p : patterns_) wipe(p);
    wipe(pending_);
}

void SecretFilter::variants(const char* secret, std::vector<std::string>* out) {
    std::string raw(secret);
    std::string json;
    for (unsigned char c : raw) {
        if (c == '"' || c == '\\') {
            json += '\\';
            json += static_cast<char>(c);
        } else if (c == '\n') {
            json += "\\n";
        } else if (c < 0x20 || c == 0x7f) {
            char b[8];
            std::snprintf(b, sizeof(b), "\\u%04x", c);
            json += b;
        } else {
            json += static_cast<char>(c);
        }
    }
    out->push_back(raw);
    if (json != raw) out->push_back(json);
    wipe(json);
    wipe(raw);
}

void SecretFilter::add_secret(const char* secret) {
    if (!secret || std::strlen(secret) < kMinSecret) return;
    ++secrets_;
    variants(secret, &patterns_);
    sort_longest_first(&patterns_);
    keep_ = patterns_.front().size() - 1;
}

void SecretFilter::sort_longest_first(std::vector<std::string>* patterns) {
    // Längere zuerst: ist ein Passwort Teil eines anderen, bliebe sonst der Rest des längeren lesbar
    std::stable_sort(patterns->begin(), patterns->end(),
                     [](const std::string& a, const std::string& b) { return a.size() > b.size(); });
}

size_t SecretFilter::redact(char* buf, size_t len, const std::vector<std::string>& patterns) {
    size_t hits = 0;
    for (const auto& p : patterns) {
        if (p.empty() || p.size() > len) continue;
        const char* end = buf + len;
        char* pos = buf;
        while (pos + p.size() <= end) {
            char* found = static_cast<char*>(std::memchr(pos, p[0], static_cast<size_t>(end - pos) - p.size() + 1));
            if (!found) break;
            if (std::memcmp(found, p.data(), p.size()) == 0) {
                std::memset(found, '*', p.size());
                ++hits;
                pos = found + p.size();
            } else {
                pos = found + 1;
            }
        }
    }
    return hits;
}

bool SecretFilter::flush(bool all) {
    if (!ok_) return false;
    if (pending_.empty()) return true;
    hits_ += redact(pending_.data(), pending_.size(), patterns_);
    // Die letzten keep_ Zeichen zurückhalten: ein Geheimnis, das dort beginnt, ist noch nicht vollständig da
    const size_t send = all ? pending_.size() : (pending_.size() > keep_ ? pending_.size() - keep_ : 0);
    if (send) {
        ok_ = sink_(pending_.data(), send);
        pending_.erase(0, send);
    }
    return ok_;
}

bool SecretFilter::write(const char* data, size_t len) {
    if (!ok_) return false;
    pending_.append(data, len);
    if (pending_.size() >= block_ + keep_) return flush(false);
    return true;
}

bool SecretFilter::finish() { return flush(true); }

}  // namespace pixelia
