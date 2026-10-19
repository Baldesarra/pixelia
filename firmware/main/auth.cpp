// Optionaler PIN: gesalzener Hash im NVS, Sitzungs-Tokens im RAM, Sperre nach Fehlversuchen (siehe auth.h)

#include "auth.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <mutex>

#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "nvs.h"
#include "psa/crypto.h"

namespace app::auth {

namespace {
constexpr const char* TAG = "auth";
constexpr const char* kNamespace = "auth";
constexpr int kSaltLen = 16;
constexpr int kHashLen = 32;
// 1000 Runden SHA-256 (mit Hardware-Beschleunigung etwa 25 ms): bremst das Durchprobieren eines kopierten NVS-Abbilds,
// gegen Raten übers Netz hilft die Sperre
constexpr int kRounds = 1000;
constexpr int kFreeAttempts = 5;
constexpr int kLockFirstS = 30;
constexpr int kLockMaxS = 15 * 60;
constexpr int kTokens = 8;
constexpr int64_t kTokenIdleUs = 24LL * 3600 * 1000000;  // ein Tag ohne Anfrage → neu anmelden

struct Token {
    char text[kTokenLen + 1] = "";
    int64_t last_us = 0;
};

std::mutex s_lock;
bool s_enabled = false;
uint8_t s_salt[kSaltLen];
uint8_t s_hash[kHashLen];
int s_fails = 0;              // Fehlversuche in Folge
int64_t s_locked_until = 0;   // esp_timer-Zeit
Token s_tokens[kTokens];

int64_t now_us() { return esp_timer_get_time(); }

void derive(const uint8_t* salt, const char* pin, uint8_t* out) {
    const size_t pin_len = std::strlen(pin);
    uint8_t buf[kSaltLen + kHashLen + kMaxLen];
    size_t len = 0;
    std::memcpy(buf, salt, kSaltLen);
    std::memcpy(buf + kSaltLen, pin, pin_len);
    len = kSaltLen + pin_len;
    size_t got = 0;
    psa_hash_compute(PSA_ALG_SHA_256, buf, len, out, kHashLen, &got);
    // Runde i: H(Salz | vorheriger Hash | PIN)
    for (int i = 1; i < kRounds; ++i) {
        std::memcpy(buf + kSaltLen, out, kHashLen);
        std::memcpy(buf + kSaltLen + kHashLen, pin, pin_len);
        psa_hash_compute(PSA_ALG_SHA_256, buf, kSaltLen + kHashLen + pin_len, out, kHashLen, &got);
    }
    std::fill(std::begin(buf), std::end(buf), 0);
}

bool equal(const uint8_t* a, const uint8_t* b, size_t n) {
    uint8_t diff = 0;
    for (size_t i = 0; i < n; ++i) diff |= a[i] ^ b[i];
    return diff == 0;
}

void clear_tokens_locked() {
    for (Token& t : s_tokens) t = Token{};
}

void new_token_locked(char* out) {
    uint8_t raw[kTokenLen / 2];
    esp_fill_random(raw, sizeof(raw));
    for (size_t i = 0; i < sizeof(raw); ++i) std::snprintf(out + 2 * i, 3, "%02x", raw[i]);
    // Freien oder am längsten ungenutzten Platz nehmen
    Token* slot = &s_tokens[0];
    for (Token& t : s_tokens) {
        if (!t.text[0]) {
            slot = &t;
            break;
        }
        if (t.last_us < slot->last_us) slot = &t;
    }
    std::memcpy(slot->text, out, kTokenLen + 1);
    slot->last_us = now_us();
}

int lock_seconds_locked() {
    const int64_t left = s_locked_until - now_us();
    return left > 0 ? static_cast<int>((left + 999999) / 1000000) : 0;
}

esp_err_t store(bool on) {
    nvs_handle_t h;
    esp_err_t err = nvs_open(kNamespace, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    if (on) {
        err = nvs_set_blob(h, "salt", s_salt, kSaltLen);
        if (err == ESP_OK) err = nvs_set_blob(h, "hash", s_hash, kHashLen);
    } else {
        err = nvs_erase_all(h);
    }
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}
}  // namespace

void init() {
    psa_crypto_init();  // ESP-IDF startet PSA schon, ein zweiter Aufruf schadet nicht
    std::lock_guard<std::mutex> g(s_lock);
    nvs_handle_t h;
    if (nvs_open(kNamespace, NVS_READONLY, &h) != ESP_OK) {
        ESP_LOGI(TAG, "PIN aus");
        return;
    }
    size_t sl = kSaltLen, hl = kHashLen;
    s_enabled = nvs_get_blob(h, "salt", s_salt, &sl) == ESP_OK && sl == kSaltLen &&
                nvs_get_blob(h, "hash", s_hash, &hl) == ESP_OK && hl == kHashLen;
    nvs_close(h);
    ESP_LOGI(TAG, "PIN %s", s_enabled ? "an" : "aus");
}

bool enabled() {
    std::lock_guard<std::mutex> g(s_lock);
    return s_enabled;
}

int lock_seconds() {
    std::lock_guard<std::mutex> g(s_lock);
    return lock_seconds_locked();
}

int attempts_left() {
    std::lock_guard<std::mutex> g(s_lock);
    return s_fails < kFreeAttempts ? kFreeAttempts - s_fails : 1;
}

Result check(const char* pin, char* token) {
    std::unique_lock<std::mutex> g(s_lock);
    if (!s_enabled) {
        if (token) new_token_locked(token);
        return Result::Ok;
    }
    if (lock_seconds_locked() > 0) return Result::Locked;
    uint8_t salt[kSaltLen], want[kHashLen], got[kHashLen];
    std::memcpy(salt, s_salt, kSaltLen);
    std::memcpy(want, s_hash, kHashLen);
    const size_t len = pin ? std::strlen(pin) : 0;
    const bool plausible = len >= kMinLen && len <= kMaxLen;
    // Rechnen ohne Sperre: andere Anfragen (Status, Token-Prüfung) warten nicht auf die 1000 Runden
    g.unlock();
    if (plausible) derive(salt, pin, got);
    g.lock();
    if (plausible && equal(got, want, kHashLen)) {
        if (s_fails) ESP_LOGI(TAG, "Angemeldet nach %d Fehlversuchen", s_fails);
        s_fails = 0;
        if (token) new_token_locked(token);
        return Result::Ok;
    }
    ++s_fails;
    if (s_fails >= kFreeAttempts) {
        const int shift = std::min(s_fails - kFreeAttempts, 5);
        const int secs = std::min(kLockFirstS << shift, kLockMaxS);
        s_locked_until = now_us() + secs * 1000000LL;
        ESP_LOGW(TAG, "PIN falsch (%d. Fehlversuch in Folge), gesperrt für %d s", s_fails, secs);
    } else {
        ESP_LOGW(TAG, "PIN falsch (%d. Fehlversuch in Folge)", s_fails);
    }
    return Result::Wrong;
}

bool token_valid(const char* token) {
    if (!token || std::strlen(token) != kTokenLen) return false;
    std::lock_guard<std::mutex> g(s_lock);
    if (!s_enabled) return true;
    const int64_t now = now_us();
    for (Token& t : s_tokens) {
        if (!t.text[0]) continue;
        if (now - t.last_us > kTokenIdleUs) {
            t = Token{};
            continue;
        }
        if (equal(reinterpret_cast<const uint8_t*>(t.text), reinterpret_cast<const uint8_t*>(token), kTokenLen)) {
            t.last_us = now;
            return true;
        }
    }
    return false;
}

void logout(const char* token) {
    if (!token) return;
    std::lock_guard<std::mutex> g(s_lock);
    for (Token& t : s_tokens) {
        if (t.text[0] && std::strcmp(t.text, token) == 0) t = Token{};
    }
}

esp_err_t set(const char* pin, char* token) {
    const size_t len = pin ? std::strlen(pin) : 0;
    if (len > 0 && (len < kMinLen || len > kMaxLen)) return ESP_ERR_INVALID_SIZE;
    for (size_t i = 0; i < len; ++i) {
        if (static_cast<unsigned char>(pin[i]) < 32 || pin[i] == 127) return ESP_ERR_INVALID_ARG;
    }
    uint8_t salt[kSaltLen], hash[kHashLen];
    if (len) {
        esp_fill_random(salt, kSaltLen);
        derive(salt, pin, hash);
    }
    std::lock_guard<std::mutex> g(s_lock);
    if (len) {
        std::memcpy(s_salt, salt, kSaltLen);
        std::memcpy(s_hash, hash, kHashLen);
    }
    s_enabled = len > 0;
    s_fails = 0;
    s_locked_until = 0;
    clear_tokens_locked();
    const esp_err_t err = store(s_enabled);
    if (err != ESP_OK) ESP_LOGE(TAG, "PIN nicht gespeichert: %s", esp_err_to_name(err));
    else ESP_LOGW(TAG, "PIN %s", s_enabled ? "gesetzt, alle Sitzungen abgemeldet" : "ausgeschaltet");
    if (token && s_enabled) new_token_locked(token);
    return err;
}

void clear_on_device() {
    ESP_LOGW(TAG, "PIN am Gerät zurückgesetzt");
    set("");
}

}  // namespace app::auth
