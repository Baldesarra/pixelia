// WLAN-Manager: Verbindung mit dem stärksten gespeicherten Netz, Setup-Modus mit Access Point und Captive Portal.
//
// Ablauf (Projektplan 4.12):
//  - Start: scannen und die gespeicherten Netze nach Signalstärke probieren. Klappt keins (oder ist keins
//    gespeichert), startet der Setup-Modus: Access Point „Pixelia-Setup“ (WPA2, Passwort steht auf dem Display),
//    DNS fängt alle Namen ab, das Display zeigt Anleitung und QR-Codes. Alle 2 Minuten ein neuer Versuch im
//    Hintergrund, der Access Point bleibt dabei an.
//  - Verbindung verloren: sofort neu verbinden, dann mit wachsenden Pausen; nach 2 Minuten ohne Erfolg Setup-Modus.
//  - Nach dem Einrichten bleibt der Access Point noch 60 s an, damit die Einrichtungsseite das Ergebnis zeigt.
//  - Passwörter liegen nur im NVS (Namensraum „wifi“) und werden nie ausgegeben, auch nicht im Log.

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <mutex>

#include "app.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "mdns.h"
#include "net.h"
#include "nvs.h"
#include "ui.h"

namespace app::wifi {

namespace {
constexpr const char* TAG = "wlan";
constexpr const char* kNvs = "wifi";
constexpr int kConnectTimeoutMs = 15000;
constexpr int64_t kLostToSetupUs = 120 * 1000000LL;  // so lange nach Verbindungsverlust probieren, dann Setup
constexpr int64_t kBootToSetupUs = 45 * 1000000LL;   // beim Start (Router hängt evtl. noch an der alten Sitzung)
constexpr int64_t kApLingerUs = 60 * 1000000LL;       // Access Point nach dem Einrichten noch offen lassen
constexpr int kMaxScan = 24;
constexpr const char kPortalUri[] = "http://192.168.4.1/";

constexpr EventBits_t kBitGotIp = BIT0;
constexpr EventBits_t kBitDisconnected = BIT1;
constexpr EventBits_t kBitAssociated = BIT2;

struct NetRecord {
    char ssid[33];
    char pass[65];
};
// Gespeicherte Netze, zuletzt erfolgreich benutztes zuerst
struct Store {
    uint8_t version = 1;
    uint8_t count = 0;
    NetRecord nets[kMaxSaved]{};
};

enum class Cmd : uint8_t { Connect, ConnectNew, Scan, Setup };

std::mutex s_lock;  // schützt s_status, s_store, s_scan
Status s_status;
Store s_store;
ScanEntry s_scan[kMaxScan];
int s_scan_count = 0;
std::atomic<uint32_t> s_scan_gen{0};

// nur im WLAN-Task
wifi_ap_record_t s_records[kMaxScan];
uint16_t s_record_count = 0;
int64_t s_next_retry = 0;   // Setup-Modus: nächster Versuch
int64_t s_setup_deadline = 0;  // kein WLAN: bis dahin schnell neu versuchen, danach Setup-Modus (0 = aus)
int64_t s_next_quick = 0;   // nächster schneller Versuch nach Verlust
int s_quick_tries = 0;
int s_burst = 0;            // weitere schnelle Versuche nach dem Eintragen eines Netzes
constexpr int kBurstPauseSeconds = 10;
int64_t s_ap_off_at = 0;    // Access Point nach dem Einrichten abschalten
int64_t s_rssi_at = 0;
bool s_announced = false;   // erste Verbindung nach dem Start gemeldet

QueueHandle_t s_queue = nullptr;
EventGroupHandle_t s_events = nullptr;
std::atomic<uint8_t> s_disc_reason{0};
esp_netif_t* s_sta = nullptr;
esp_netif_t* s_ap = nullptr;
char s_ap_password[16] = "";

int64_t now_us() { return esp_timer_get_time(); }

void set_state(State st) {
    std::lock_guard<std::mutex> g(s_lock);
    s_status.state = st;
}

void set_error(const char* text) {
    std::lock_guard<std::mutex> g(s_lock);
    std::snprintf(s_status.last_error, sizeof(s_status.last_error), "%s", text);
}

const char* reason_text(uint8_t reason) {
    switch (reason) {
        case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
        case WIFI_REASON_HANDSHAKE_TIMEOUT:
        case WIFI_REASON_MIC_FAILURE:
            return "Passwort falsch?";
        case WIFI_REASON_AUTH_FAIL:
            return "Anmeldung abgelehnt (Passwort?)";
        case WIFI_REASON_AUTH_EXPIRE:
        case WIFI_REASON_DISASSOC_DUE_TO_INACTIVITY:
            return "keine Antwort vom Router";
        case WIFI_REASON_NO_AP_FOUND:
        case WIFI_REASON_NO_AP_FOUND_IN_RSSI_THRESHOLD:
            return "nicht gefunden";
        case WIFI_REASON_NO_AP_FOUND_W_COMPATIBLE_SECURITY:
        case WIFI_REASON_NO_AP_FOUND_IN_AUTHMODE_THRESHOLD:
            return "Verschlüsselung passt nicht";
        case WIFI_REASON_ASSOC_FAIL:
        case WIFI_REASON_CONNECTION_FAIL:
            return "Anmeldung abgelehnt";
        case WIFI_REASON_BEACON_TIMEOUT:
            return "Signal verloren";
        case WIFI_REASON_IE_IN_4WAY_DIFFERS:
        case WIFI_REASON_GROUP_CIPHER_INVALID:
        case WIFI_REASON_PAIRWISE_CIPHER_INVALID:
            return "Verschlüsselung des Routers passt nicht";
        case WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT:
            return "Schlüsselaustausch abgebrochen";
        default:
            return "getrennt";
    }
}

// --- Speicher -------------------------------------------------------------------------------------------------

void load_store() {
    nvs_handle_t h;
    if (nvs_open(kNvs, NVS_READONLY, &h) != ESP_OK) return;
    Store st;
    size_t len = sizeof(st);
    if (nvs_get_blob(h, "nets", &st, &len) == ESP_OK && len == sizeof(st) && st.version == 1 && st.count <= kMaxSaved) {
        s_store = st;
    }
    len = sizeof(s_ap_password);
    if (nvs_get_str(h, "ap_pw", s_ap_password, &len) != ESP_OK) s_ap_password[0] = 0;
    nvs_close(h);
}

// Aufruf mit gehaltenem s_lock
void save_store_locked() {
    nvs_handle_t h;
    if (nvs_open(kNvs, NVS_READWRITE, &h) != ESP_OK) return;
    esp_err_t err = nvs_set_blob(h, "nets", &s_store, sizeof(s_store));
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    if (err != ESP_OK) ESP_LOGW(TAG, "WLAN-Liste nicht gespeichert: %s", esp_err_to_name(err));
    s_status.saved = s_store.count;
}

// Passwort des Setup-WLANs: einmal zufällig erzeugt, gut lesbar (ohne 0/O, 1/l/i), steht auf dem Display
void ensure_ap_password() {
    if (std::strlen(s_ap_password) >= 8) return;
    static constexpr char kAlphabet[] = "abcdefghjkmnpqrstuvwxyz23456789";
    for (int i = 0; i < 9; ++i) s_ap_password[i] = i == 4 ? '-' : kAlphabet[esp_random() % (sizeof(kAlphabet) - 1)];
    s_ap_password[9] = 0;
    nvs_handle_t h;
    if (nvs_open(kNvs, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_str(h, "ap_pw", s_ap_password);
        nvs_commit(h);
        nvs_close(h);
    }
    ESP_LOGI(TAG, "Neues Passwort für das Setup-WLAN erzeugt (steht im Setup-Modus auf dem Display)");
}

// Netz an den Anfang der Liste setzen (zuletzt erfolgreich benutzt)
void move_to_front(const char* ssid) {
    std::lock_guard<std::mutex> g(s_lock);
    for (int i = 0; i < s_store.count; ++i) {
        if (std::strcmp(s_store.nets[i].ssid, ssid) != 0) continue;
        if (i == 0) return;
        NetRecord r = s_store.nets[i];
        std::memmove(&s_store.nets[1], &s_store.nets[0], i * sizeof(NetRecord));
        s_store.nets[0] = r;
        save_store_locked();
        return;
    }
}

// --- Anzeige --------------------------------------------------------------------------------------------------

void publish_setup() {
    Status st = status();
    if (!st.ap_active) return;
    pixelia::ui::SetupInfo info;
    info.ap_ssid = kApSsid;
    info.ap_password = s_ap_password;
    info.ap_url = std::string("http://") + kApIp;
    info.clients = st.ap_clients;
    info.saved_networks = st.saved;
    info.retry_seconds = st.state == State::Connecting ? -1 : st.retry_in_s;
    if (st.state == State::Connecting) info.connecting = st.ssid;
    info.last_error = st.last_error;
    ui::show_setup(info);
}

// --- Ereignisse -----------------------------------------------------------------------------------------------

void update_ap_clients() {
    wifi_sta_list_t list{};
    int n = esp_wifi_ap_get_sta_list(&list) == ESP_OK ? list.num : 0;
    std::lock_guard<std::mutex> g(s_lock);
    s_status.ap_clients = n;
}

void on_event(void*, esp_event_base_t base, int32_t id, void* data) {
    if (base == WIFI_EVENT) {
        switch (id) {
            case WIFI_EVENT_STA_CONNECTED:
                xEventGroupSetBits(s_events, kBitAssociated);
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                s_disc_reason = static_cast<wifi_event_sta_disconnected_t*>(data)->reason;
                xEventGroupClearBits(s_events, kBitGotIp | kBitAssociated);
                xEventGroupSetBits(s_events, kBitDisconnected);
                break;
            case WIFI_EVENT_AP_STACONNECTED:
            case WIFI_EVENT_AP_STADISCONNECTED:
                update_ap_clients();
                ESP_LOGI(TAG, "Setup-WLAN: Gerät %s", id == WIFI_EVENT_AP_STACONNECTED ? "angemeldet" : "abgemeldet");
                break;
            default:
                break;
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        auto* e = static_cast<ip_event_got_ip_t*>(data);
        {
            std::lock_guard<std::mutex> g(s_lock);
            std::snprintf(s_status.ip, sizeof(s_status.ip), IPSTR, IP2STR(&e->ip_info.ip));
        }
        xEventGroupSetBits(s_events, kBitGotIp);
    }
}

// --- Scannen und Verbinden (nur im WLAN-Task) ---------------------------------------------------------------

bool do_scan() {
    wifi_scan_config_t cfg{};
    cfg.scan_type = WIFI_SCAN_TYPE_ACTIVE;
    cfg.scan_time.active.min = 60;
    cfg.scan_time.active.max = 120;
    esp_err_t err = esp_wifi_scan_start(&cfg, true);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Scan fehlgeschlagen: %s", esp_err_to_name(err));
        return false;
    }
    uint16_t n = kMaxScan;
    if (esp_wifi_scan_get_ap_records(&n, s_records) != ESP_OK) n = 0;
    esp_wifi_clear_ap_list();
    std::sort(s_records, s_records + n, [](const wifi_ap_record_t& a, const wifi_ap_record_t& b) { return a.rssi > b.rssi; });
    s_record_count = n;

    // Für die Oberfläche: jedes Netz einmal (der stärkste Zugangspunkt zählt), versteckte Netze weglassen
    std::lock_guard<std::mutex> g(s_lock);
    s_scan_count = 0;
    for (int i = 0; i < n && s_scan_count < kMaxScan; ++i) {
        const auto& r = s_records[i];
        const char* ssid = reinterpret_cast<const char*>(r.ssid);
        if (!ssid[0]) continue;
        bool dup = false;
        for (int k = 0; k < s_scan_count; ++k) dup |= std::strcmp(s_scan[k].ssid, ssid) == 0;
        if (dup) continue;
        ScanEntry& e = s_scan[s_scan_count++];
        std::snprintf(e.ssid, sizeof(e.ssid), "%s", ssid);
        e.rssi = r.rssi;
        e.channel = r.primary;
        e.secure = r.authmode != WIFI_AUTH_OPEN;
        e.saved = false;
        for (int k = 0; k < s_store.count; ++k) e.saved |= std::strcmp(s_store.nets[k].ssid, ssid) == 0;
    }
    ESP_LOGI(TAG, "Scan: %u Zugangspunkte, %d Netze", n, s_scan_count);
    s_scan_gen++;
    return true;
}

void reset_link() {
    esp_wifi_disconnect();
    vTaskDelay(pdMS_TO_TICKS(60));
    xEventGroupClearBits(s_events, kBitGotIp | kBitDisconnected | kBitAssociated);
}

const char* auth_name(wifi_auth_mode_t m) {
    switch (m) {
        case WIFI_AUTH_OPEN: return "offen";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA";
        case WIFI_AUTH_WPA2_PSK: return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
        case WIFI_AUTH_WPA3_PSK: return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
        default: return "andere";
    }
}

// Verbindungsarten. Manche Router kommen beim Schlüsselaustausch aus dem Tritt (z. B. Grund 17 „IE weicht ab“,
// gemessen an einer FritzBox im WPA2/WPA3-Mischmodus mit dem WPA3-Kompatibilitätsmodus/RSN-Override von
// ESP-IDF 6.1): Dann folgt ein zweiter Versuch ohne WPA3-Kompatibilität und ohne festen Zugangspunkt.
// Was geklappt hat, merkt sich Pixelia im NVS (Schlüssel „hints“) und probiert es beim nächsten Mal zuerst.
enum Mode : uint8_t { kModeStandard = 0, kModeCompat = 1 };
struct ModeHint {
    char ssid[33];
    uint8_t mode;
};
ModeHint s_hints[kMaxSaved] = {};

void load_hints() {
    nvs_handle_t h;
    if (nvs_open(kNvs, NVS_READONLY, &h) != ESP_OK) return;
    size_t len = sizeof(s_hints);
    if (nvs_get_blob(h, "hints", s_hints, &len) != ESP_OK || len != sizeof(s_hints)) std::memset(s_hints, 0, sizeof(s_hints));
    nvs_close(h);
}

uint8_t mode_hint(const char* ssid) {
    for (const auto& h : s_hints)
        if (h.ssid[0] && std::strcmp(h.ssid, ssid) == 0) return h.mode;
    return kModeStandard;
}

void remember_mode(const char* ssid, uint8_t mode) {
    ModeHint* slot = nullptr;
    for (auto& h : s_hints)
        if (!slot && (!h.ssid[0] || std::strcmp(h.ssid, ssid) == 0)) slot = &h;
    if (!slot) {
        std::memmove(&s_hints[1], &s_hints[0], sizeof(ModeHint) * (kMaxSaved - 1));
        slot = &s_hints[0];
    }
    if (std::strcmp(slot->ssid, ssid) == 0 && slot->mode == mode) return;
    std::snprintf(slot->ssid, sizeof(slot->ssid), "%s", ssid);
    slot->mode = mode;
    nvs_handle_t h;
    if (nvs_open(kNvs, NVS_READWRITE, &h) != ESP_OK) return;
    if (nvs_set_blob(h, "hints", s_hints, sizeof(s_hints)) == ESP_OK) nvs_commit(h);
    nvs_close(h);
}

// Lohnt ein zweiter Versuch mit der anderen Verbindungsart? Nicht, wenn das Netz gar nicht da war.
bool worth_other_mode(uint8_t reason, bool associated) {
    switch (reason) {
        case WIFI_REASON_NO_AP_FOUND:
        case WIFI_REASON_NO_AP_FOUND_IN_RSSI_THRESHOLD:
        case WIFI_REASON_BEACON_TIMEOUT:
            return false;
        default:
            return associated || reason != 0;
    }
}

bool connect_to(const NetRecord& n, const wifi_ap_record_t* ap, uint8_t mode, bool* retry_other) {
    *retry_other = false;
    {
        std::lock_guard<std::mutex> g(s_lock);
        s_status.state = State::Connecting;
        std::snprintf(s_status.ssid, sizeof(s_status.ssid), "%s", n.ssid);
        s_status.ip[0] = 0;
    }
    publish_setup();
    const char* how = mode == kModeCompat ? ", nur WPA2-kompatibel" : "";
    if (ap) ESP_LOGI(TAG, "Verbinde mit „%s“ (%d dBm, Kanal %d, %s%s)", n.ssid, ap->rssi, ap->primary, auth_name(ap->authmode), how);
    else ESP_LOGI(TAG, "Verbinde mit „%s“ (nicht im Scan, evtl. verstecktes Netz%s)", n.ssid, how);

    reset_link();
    s_disc_reason = 0;
    wifi_config_t cfg{};
    std::memcpy(cfg.sta.ssid, n.ssid, strnlen(n.ssid, 32));
    std::memcpy(cfg.sta.password, n.pass, strnlen(n.pass, 64));
    // Mit Passwort mindestens WPA, ohne Passwort auch offene Netze
    cfg.sta.threshold.authmode = n.pass[0] ? WIFI_AUTH_WPA_PSK : WIFI_AUTH_OPEN;
    cfg.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
    if (ap && mode == kModeStandard) {
        cfg.sta.bssid_set = true;
        std::memcpy(cfg.sta.bssid, ap->bssid, 6);
        cfg.sta.channel = ap->primary;
        cfg.sta.scan_method = WIFI_FAST_SCAN;
    } else {
        cfg.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
        cfg.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
    }
    if (mode == kModeCompat) {
        cfg.sta.disable_wpa3_compatible_mode = 1;
        cfg.sta.pmf_cfg.required = false;
    }
    esp_wifi_set_config(WIFI_IF_STA, &cfg);
    esp_err_t err = esp_wifi_connect();
    EventBits_t bits = 0;
    if (err == ESP_OK) {
        bits = xEventGroupWaitBits(s_events, kBitGotIp | kBitDisconnected, pdFALSE, pdFALSE,
                                   pdMS_TO_TICKS(kConnectTimeoutMs));
    }
    if (bits & kBitGotIp) {
        wifi_ap_record_t info{};
        esp_wifi_sta_get_ap_info(&info);
        std::lock_guard<std::mutex> g(s_lock);
        s_status.state = State::Connected;
        s_status.rssi = info.rssi;
        s_status.channel = info.primary;
        s_status.last_error[0] = 0;
        s_status.connected_since_s = static_cast<uint32_t>(now_us() / 1000000);
        return true;
    }
    const bool associated = (xEventGroupGetBits(s_events) & kBitAssociated) != 0;
    const uint8_t reason = s_disc_reason;
    const char* why = err != ESP_OK               ? esp_err_to_name(err)
                      : (bits & kBitDisconnected) ? reason_text(reason)
                      : associated                ? "keine IP-Adresse (DHCP)"
                                                  : "keine Antwort";
    char text[64];
    std::snprintf(text, sizeof(text), "%.24s: %s", n.ssid, why);
    set_error(text);
    ESP_LOGW(TAG, "Verbindung mit „%s“ fehlgeschlagen: %s (Grund %u)", n.ssid, why, reason);
    reset_link();
    *retry_other = err == ESP_OK && worth_other_mode(reason, associated);
    return false;
}

// Ein Netz verbinden: zuerst die Verbindungsart, die zuletzt geklappt hat, bei Problemen im Schlüsselaustausch
// gleich noch die andere
bool connect_net(const NetRecord& n, const wifi_ap_record_t* ap) {
    const uint8_t first = mode_hint(n.ssid);
    bool retry_other = false;
    if (connect_to(n, ap, first, &retry_other)) return true;
    if (!retry_other) return false;
    const uint8_t second = first == kModeStandard ? kModeCompat : kModeStandard;
    if (!connect_to(n, ap, second, &retry_other)) return false;
    remember_mode(n.ssid, second);
    ESP_LOGW(TAG, "„%s“ klappt nur %s, wird ab jetzt so verbunden", n.ssid,
             second == kModeCompat ? "ohne WPA3-Kompatibilitätsmodus" : "im Standardmodus");
    return true;
}

// Alle gespeicherten Netze probieren: zuerst die gefundenen nach Signalstärke, dann nicht gefundene (versteckt)
bool try_saved() {
    Store st;
    {
        std::lock_guard<std::mutex> g(s_lock);
        st = s_store;
    }
    if (st.count == 0) return false;
    set_state(State::Connecting);
    publish_setup();
    if (!do_scan()) s_record_count = 0;

    struct Cand {
        int net;
        int rec;
    };
    Cand cand[kMaxSaved];
    int nc = 0;
    bool found[kMaxSaved] = {};
    for (int i = 0; i < st.count; ++i) {
        for (int r = 0; r < s_record_count; ++r) {
            if (std::strcmp(reinterpret_cast<const char*>(s_records[r].ssid), st.nets[i].ssid) == 0) {
                cand[nc++] = {i, r};  // Liste ist nach Signal sortiert: erster Treffer = stärkster
                found[i] = true;
                break;
            }
        }
    }
    // nach Signalstärke sortieren (Einfügesortierung: höchstens 5 Einträge)
    for (int i = 1; i < nc; ++i)
        for (int k = i; k > 0 && s_records[cand[k].rec].rssi > s_records[cand[k - 1].rec].rssi; --k) std::swap(cand[k], cand[k - 1]);
    for (int i = 0; i < nc; ++i) {
        if (connect_net(st.nets[cand[i].net], &s_records[cand[i].rec])) {
            move_to_front(st.nets[cand[i].net].ssid);
            return true;
        }
    }
    for (int i = 0; i < st.count; ++i) {
        if (found[i]) continue;
        if (connect_net(st.nets[i], nullptr)) {
            move_to_front(st.nets[i].ssid);
            return true;
        }
    }
    if (nc == 0 && s_record_count > 0) set_error("Kein gespeichertes WLAN in Reichweite");
    return false;
}

// --- Setup-Modus ----------------------------------------------------------------------------------------------

void enter_setup() {
    bool already;
    {
        std::lock_guard<std::mutex> g(s_lock);
        already = s_status.ap_active;
    }
    s_next_retry = now_us() + kRetrySeconds * 1000000LL;
    s_setup_deadline = 0;
    s_ap_off_at = 0;
    if (!already) {
        // Kanal: wo das stärkste gespeicherte Netz funkt (weniger Unterbrechung beim späteren Verbinden)
        uint8_t channel = 6;
        for (int r = 0; r < s_record_count; ++r) {
            bool saved = false;
            {
                std::lock_guard<std::mutex> g(s_lock);
                for (int k = 0; k < s_store.count; ++k)
                    saved |= std::strcmp(s_store.nets[k].ssid, reinterpret_cast<const char*>(s_records[r].ssid)) == 0;
            }
            if (saved) {
                channel = s_records[r].primary;
                break;
            }
        }
        wifi_config_t ap{};
        std::snprintf(reinterpret_cast<char*>(ap.ap.ssid), sizeof(ap.ap.ssid), "%s", kApSsid);
        ap.ap.ssid_len = static_cast<uint8_t>(std::strlen(kApSsid));
        std::snprintf(reinterpret_cast<char*>(ap.ap.password), sizeof(ap.ap.password), "%s", s_ap_password);
        ap.ap.authmode = WIFI_AUTH_WPA2_PSK;
        ap.ap.max_connection = 4;
        ap.ap.channel = channel;
        esp_wifi_set_mode(WIFI_MODE_APSTA);
        esp_wifi_set_config(WIFI_IF_AP, &ap);
        // DHCP-Option 114 (RFC 8910): moderne Geräte öffnen die Einrichtungsseite dann direkt
        esp_netif_dhcps_stop(s_ap);
        esp_netif_dhcps_option(s_ap, ESP_NETIF_OP_SET, ESP_NETIF_CAPTIVEPORTAL_URI, const_cast<char*>(kPortalUri),
                               sizeof(kPortalUri) - 1);
        esp_netif_dhcps_start(s_ap);
        captive_dns::start();
        {
            std::lock_guard<std::mutex> g(s_lock);
            s_status.ap_active = true;
            s_status.ap_clients = 0;
        }
        ESP_LOGW(TAG, "Setup-Modus: WLAN „%s“ auf Kanal %u, Einrichtung unter http://%s", kApSsid, channel, kApIp);
    }
    {
        std::lock_guard<std::mutex> g(s_lock);
        s_status.state = State::Setup;
    }
    publish_setup();
}

void leave_setup() {
    {
        std::lock_guard<std::mutex> g(s_lock);
        if (!s_status.ap_active) return;
        s_status.ap_active = false;
        s_status.ap_clients = 0;
    }
    captive_dns::stop();
    esp_wifi_set_mode(WIFI_MODE_STA);
    ui::hide_setup();
    ESP_LOGI(TAG, "Setup-Modus beendet");
}

void on_connected() {
    Status st = status();
    ESP_LOGI(TAG, "Verbunden mit „%s“: IP %s, %d dBm, Kanal %u, Name „%s“ bzw. http://%s.local", st.ssid, st.ip,
             st.rssi, st.channel, kHostname, kMdnsHost);
    s_setup_deadline = 0;
    s_quick_tries = 0;
    s_rssi_at = now_us();
    timesync::on_connected();
    char body[224];
    std::snprintf(body, sizeof(body),
                  "WLAN „%s“, Adresse %s\nIm Browser: http://%s/ oder http://%s.local\nLanger Druck aufs Display "
                  "zeigt diese Daten jederzeit.",
                  st.ssid, st.ip, st.ip, kMdnsHost);
    if (st.ap_active) {
        // Einrichtung abgeschlossen: Ergebnis zeigen, Access Point noch kurz offen lassen, falls jemand zuschaut
        ui::hide_setup();
        ui::show_message({"Verbunden", body, pixelia::ui::Tone::Success}, 20);
        if (st.ap_clients > 0) {
            s_ap_off_at = now_us() + kApLingerUs;
            ESP_LOGI(TAG, "Setup-WLAN bleibt noch %lld s offen", kApLingerUs / 1000000);
        } else {
            leave_setup();
        }
    } else {
        // Mit IP: in fremden WLANs (Handy-Hotspot, andere Router) ist sie oft der einzige Weg zur Seite
        char text[96];
        std::snprintf(text, sizeof(text), "Verbunden mit „%.24s“ · %s", st.ssid, st.ip);
        ui::toast(text, pixelia::ui::Tone::Success, s_announced ? 6 : 10);
    }
    s_announced = true;
}

// Ein Durchgang über alle gespeicherten Netze, danach Zustand passend setzen
void attempt() {
    if (try_saved()) {
        on_connected();
        return;
    }
    Status st = status();
    if (st.ap_active) {
        // Direkt nach dem Eintragen eines Netzes noch zweimal kurz hintereinander probieren, sonst alle 2 Minuten
        const int pause = s_burst > 0 ? kBurstPauseSeconds : kRetrySeconds;
        if (s_burst > 0) --s_burst;
        s_next_retry = now_us() + pause * 1000000LL;
        set_state(State::Setup);
        publish_setup();
    } else if (s_setup_deadline) {
        set_state(State::Connecting);
    } else {
        enter_setup();
    }
}

void start_mdns() {
    esp_err_t err = mdns_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "mDNS nicht verfügbar: %s", esp_err_to_name(err));
        return;
    }
    mdns_hostname_set(kMdnsHost);
    mdns_instance_name_set("Pixelia");
    mdns_service_add("Pixelia", "_http", "_tcp", 80, nullptr, 0);
}

void wifi_task(void*) {
    {
        Status st = status();
        if (st.saved == 0) {
            ESP_LOGW(TAG, "Noch kein WLAN gespeichert");
            enter_setup();
        } else {
            s_setup_deadline = now_us() + kBootToSetupUs;
            s_next_quick = 0;
        }
    }
    for (;;) {
        Cmd cmd;
        if (xQueueReceive(s_queue, &cmd, pdMS_TO_TICKS(1000)) == pdTRUE) {
            switch (cmd) {
                case Cmd::Scan:
                    do_scan();
                    break;
                case Cmd::Setup:
                    if (status().state == State::Connected) reset_link();
                    enter_setup();
                    break;
                case Cmd::ConnectNew:
                    s_burst = 2;
                    [[fallthrough]];
                case Cmd::Connect:
                    if (status().state != State::Connected) attempt();
                    break;
            }
        }
        const int64_t now = now_us();
        Status st = status();

        if (st.state == State::Connected) {
            if (xEventGroupGetBits(s_events) & kBitDisconnected) {
                ESP_LOGW(TAG, "Verbindung zu „%s“ verloren: %s (Grund %u)", st.ssid, reason_text(s_disc_reason),
                         s_disc_reason.load());
                {
                    std::lock_guard<std::mutex> g(s_lock);
                    s_status.ip[0] = 0;
                }
                set_state(State::Connecting);
                s_setup_deadline = now + kLostToSetupUs;
                s_quick_tries = 0;
                s_next_quick = now;
                ui::toast("WLAN getrennt, verbinde neu …", pixelia::ui::Tone::Warning, 5);
            } else {
                if (s_ap_off_at && now >= s_ap_off_at) {
                    s_ap_off_at = 0;
                    leave_setup();
                }
                if (now - s_rssi_at > 10 * 1000000LL) {
                    s_rssi_at = now;
                    wifi_ap_record_t info{};
                    if (esp_wifi_sta_get_ap_info(&info) == ESP_OK) {
                        std::lock_guard<std::mutex> g(s_lock);
                        s_status.rssi = info.rssi;
                    }
                }
            }
            continue;
        }

        if (s_setup_deadline) {
            // Ohne Verbindung (nach Verlust oder beim Start): sofort, dann nach 5, 10, 20, 30 s … neu probieren;
            // klappt es bis zur Frist nicht (2 Minuten bzw. 45 s beim Start), folgt der Setup-Modus
            if (now >= s_next_quick) {
                attempt();
                if (status().state == State::Connected) continue;
                static constexpr int kBackoff[] = {5, 10, 20, 30};
                int pause = kBackoff[std::min(s_quick_tries, 3)];
                ++s_quick_tries;
                s_next_quick = now_us() + pause * 1000000LL;
                if (now_us() > s_setup_deadline) {
                    ESP_LOGW(TAG, "Kein gespeichertes WLAN erreichbar, starte Setup-Modus");
                    enter_setup();
                }
            }
            continue;
        }

        if (st.ap_active) {
            int remaining = static_cast<int>((s_next_retry - now) / 1000000);
            {
                std::lock_guard<std::mutex> g(s_lock);
                s_status.retry_in_s = s_status.saved > 0 ? std::max(0, remaining) : -1;
            }
            if (st.saved > 0 && now >= s_next_retry) attempt();
            publish_setup();
        }
    }
}

}  // namespace

const char* state_name(State s) {
    switch (s) {
        case State::Off: return "aus";
        case State::Connecting: return "verbinde";
        case State::Connected: return "verbunden";
        case State::Setup: return "setup";
    }
    return "?";
}

void start() {
    s_events = xEventGroupCreate();
    s_queue = xQueueCreate(8, sizeof(Cmd));
    load_store();
    load_hints();
    s_status.saved = s_store.count;
    ensure_ap_password();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    s_sta = esp_netif_create_default_wifi_sta();
    s_ap = esp_netif_create_default_wifi_ap();
    esp_netif_set_hostname(s_sta, kHostname);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    // Zugangsdaten verwaltet Pixelia selbst; der Treiber soll keine eigene Kopie im NVS ablegen
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, on_event, nullptr);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, on_event, nullptr);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    esp_wifi_set_country_code("DE", true);
    ESP_ERROR_CHECK(esp_wifi_start());
    // Netzbetrieb: Weboberfläche soll sofort reagieren, Stromsparen lohnt sich am Netzteil nicht
    esp_wifi_set_ps(WIFI_PS_NONE);
    start_mdns();
    timesync::init();

    ESP_LOGI(TAG, "WLAN gestartet, %d Netz(e) gespeichert, Hostname „%s“", s_store.count, kHostname);
    xTaskCreatePinnedToCore(wifi_task, "wlan", 4096, nullptr, 5, nullptr, kSystemCore);
}

Status status() {
    std::lock_guard<std::mutex> g(s_lock);
    return s_status;
}

const char* ap_password() { return s_ap_password; }

int saved_networks(char (*ssids)[33], int max) {
    std::lock_guard<std::mutex> g(s_lock);
    int n = std::min<int>(max, s_store.count);
    for (int i = 0; i < n; ++i) std::snprintf(ssids[i], 33, "%s", s_store.nets[i].ssid);
    return n;
}

esp_err_t add_network(const char* ssid, const char* password) {
    size_t sl = ssid ? std::strlen(ssid) : 0, pl = password ? std::strlen(password) : 0;
    if (sl == 0 || sl > 32) return ESP_ERR_INVALID_ARG;
    if (pl != 0 && (pl < 8 || pl > 64)) return ESP_ERR_INVALID_SIZE;
    {
        std::lock_guard<std::mutex> g(s_lock);
        int idx = -1;
        for (int i = 0; i < s_store.count; ++i)
            if (std::strcmp(s_store.nets[i].ssid, ssid) == 0) idx = i;
        if (idx < 0) {
            // neu: vorne einfügen, bei voller Liste fällt das am längsten unbenutzte weg
            if (s_store.count == kMaxSaved) --s_store.count;
            std::memmove(&s_store.nets[1], &s_store.nets[0], s_store.count * sizeof(NetRecord));
            ++s_store.count;
        } else {
            NetRecord r = s_store.nets[idx];
            std::memmove(&s_store.nets[1], &s_store.nets[0], idx * sizeof(NetRecord));
            s_store.nets[0] = r;
        }
        NetRecord& r = s_store.nets[0];
        std::memset(&r, 0, sizeof(r));
        std::memcpy(r.ssid, ssid, sl);
        std::memcpy(r.pass, password ? password : "", pl);
        save_store_locked();
        s_status.last_error[0] = 0;
    }
    ESP_LOGI(TAG, "WLAN „%s“ gespeichert (%s)", ssid, pl ? "mit Passwort" : "offen");
    Cmd cmd = Cmd::ConnectNew;
    xQueueSend(s_queue, &cmd, 0);
    return ESP_OK;
}

esp_err_t remove_network(const char* ssid) {
    std::lock_guard<std::mutex> g(s_lock);
    for (int i = 0; i < s_store.count; ++i) {
        if (std::strcmp(s_store.nets[i].ssid, ssid) != 0) continue;
        std::memmove(&s_store.nets[i], &s_store.nets[i + 1], (s_store.count - i - 1) * sizeof(NetRecord));
        --s_store.count;
        std::memset(&s_store.nets[s_store.count], 0, sizeof(NetRecord));
        save_store_locked();
        ESP_LOGI(TAG, "WLAN „%s“ entfernt", ssid);
        return ESP_OK;
    }
    return ESP_ERR_NOT_FOUND;
}

int scan(ScanEntry* out, int max, bool fresh, int timeout_ms) {
    if (fresh) {
        uint32_t gen = s_scan_gen;
        Cmd cmd = Cmd::Scan;
        xQueueSend(s_queue, &cmd, 0);
        for (int waited = 0; s_scan_gen == gen && waited < timeout_ms; waited += 100) vTaskDelay(pdMS_TO_TICKS(100));
    }
    std::lock_guard<std::mutex> g(s_lock);
    int n = std::min(max, s_scan_count);
    for (int i = 0; i < n; ++i) {
        out[i] = s_scan[i];
        out[i].saved = false;
        for (int k = 0; k < s_store.count; ++k) out[i].saved |= std::strcmp(s_store.nets[k].ssid, out[i].ssid) == 0;
    }
    return n;
}

void reconnect() {
    Cmd cmd = Cmd::Connect;
    xQueueSend(s_queue, &cmd, 0);
}

void force_setup() {
    Cmd cmd = Cmd::Setup;
    xQueueSend(s_queue, &cmd, 0);
}

}  // namespace app::wifi
