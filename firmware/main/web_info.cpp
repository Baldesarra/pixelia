// Info, Hardware und verbundene Geräte für die Weboberfläche (0.3.1, Projektplan 4.12 und Kapitel 8).
//
//   GET  /api/system          Board, Panel, Touch, Chip mit Temperatur, Flash, PSRAM, MACs, Pins, Netz (Maske,
//                             Gateway, DNS), Uhrzeit, Partitionstabelle mit Belegung, NVS, Build
//   GET  /api/clients         verbundene Browser (Geräte-ID) und im Setup-Modus die Geräte am Access Point
//   POST /api/clients/name    id, name: eigener Name für ein Gerät (leer = Name löschen)
//
// Browser melden sich über den Status-WebSocket /ws mit Formulardaten (wie die API):
//   hello=<Geräte-ID>&ua=<Browser · System>&page=<Seite>   nach dem Öffnen
//   page=<Seite>                                            bei jedem Seitenwechsel
// Die ID erzeugt der Browser selbst (zufällig, im localStorage). Einen echten Gerätenamen verrät kein Browser, deshalb
// der eigene Name. Gespeichert wird im NVS (Namensraum „clients“): ID, Name, Browser, erste und letzte Verbindung.

#include <algorithm>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>

#include "app.h"
#include "board/pins.h"
#include "driver/temperature_sensor.h"
#include "esp_app_desc.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_http_server.h"
#include "esp_image_format.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_psram.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "json.h"
#include "pixelia/engine/figure.h"
#include "lwip/sockets.h"
#include "net.h"
#include "nvs.h"
#include "settings.h"
#include "web_internal.h"

namespace app::web::detail {

namespace {
constexpr const char* TAG = "web";
constexpr const char* kNvsNs = "clients";
constexpr int kMaxClients = 12;
constexpr uint32_t kSessionMagic = 0x56454453;  // „SDEV“
constexpr uint32_t kValidEpoch = 1735689600;   // 2025-01-01: davor ist die Uhr nicht gestellt
constexpr int64_t kSaveEveryUs = 10LL * 60 * 1000000;

// Im NVS (ein Blob): pro Gerät 92 Byte
struct Stored {
    char id[17];
    char name[25];
    char agent[38];
    uint32_t first;  // erste Verbindung (Epoche, 0 = Uhr war nicht gestellt)
    uint32_t last;   // letzte Aktivität (Epoche)
    uint32_t visits;
};

struct Client {
    Stored s{};
    char page[17] = "";
    char ip[40] = "";
    int fd = -1;            // offene WebSocket-Sitzung, -1 = offline
    int64_t seen_us = -1;   // letzte Aktivität seit diesem Start (esp_timer), -1 = noch nicht
};

struct Session {
    uint32_t magic = kSessionMagic;
    int slot = -1;
};

Client s_clients[kMaxClients];
bool s_loaded = false;
int64_t s_saved_us = 0;

uint32_t epoch_now() {
    const time_t t = time(nullptr);
    return t > kValidEpoch ? static_cast<uint32_t>(t) : 0;
}

// Kürzen ohne ein UTF-8-Zeichen zu zerschneiden, Steuerzeichen weglassen
void copy(char* dst, size_t n, const std::string& src) {
    size_t len = 0;
    for (char ch : src) {
        if (static_cast<unsigned char>(ch) < 32) continue;
        if (len + 1 >= n) break;
        dst[len++] = ch;
    }
    if (len < src.size()) {
        size_t start = len;
        while (start > 0 && (static_cast<unsigned char>(dst[start - 1]) & 0xC0) == 0x80) --start;
        if (start > 0) {
            const unsigned char lead = dst[start - 1];
            const size_t need = lead >= 0xF0 ? 4 : lead >= 0xE0 ? 3 : lead >= 0xC0 ? 2 : 1;
            if (len - (start - 1) < need) len = start - 1;
        }
    }
    dst[len] = 0;
}

void load() {
    if (s_loaded) return;
    s_loaded = true;
    nvs_handle_t h;
    if (nvs_open(kNvsNs, NVS_READONLY, &h) != ESP_OK) return;
    Stored list[kMaxClients] = {};
    size_t len = sizeof(list);
    if (nvs_get_blob(h, "list", list, &len) == ESP_OK) {
        const int n = std::min<int>(kMaxClients, static_cast<int>(len / sizeof(Stored)));
        for (int i = 0; i < n; ++i) {
            list[i].id[sizeof(list[i].id) - 1] = 0;
            list[i].name[sizeof(list[i].name) - 1] = 0;
            list[i].agent[sizeof(list[i].agent) - 1] = 0;
            s_clients[i].s = list[i];
        }
    }
    nvs_close(h);
}

void save() {
    s_saved_us = esp_timer_get_time();
    if (settings::frozen()) return;  // Werksreset läuft
    Stored list[kMaxClients] = {};
    int n = 0;
    for (const Client& c : s_clients) {
        if (c.s.id[0]) list[n++] = c.s;
    }
    nvs_handle_t h;
    if (nvs_open(kNvsNs, NVS_READWRITE, &h) != ESP_OK) return;
    esp_err_t err = nvs_set_blob(h, "list", list, n * sizeof(Stored));
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    if (err != ESP_OK) ESP_LOGW(TAG, "Geräteliste nicht gespeichert: %s", esp_err_to_name(err));
}

int find(const char* id) {
    for (int i = 0; i < kMaxClients; ++i) {
        if (s_clients[i].s.id[0] && std::strcmp(s_clients[i].s.id, id) == 0) return i;
    }
    return -1;
}

// Freier Platz, sonst das am längsten inaktive Gerät (unbenannte zuerst, nie ein verbundenes)
int free_slot() {
    int best = -1;
    for (int i = 0; i < kMaxClients; ++i) {
        const Client& c = s_clients[i];
        if (!c.s.id[0]) return i;
        if (c.fd >= 0) continue;
        if (best < 0) {
            best = i;
            continue;
        }
        const Client& b = s_clients[best];
        const bool named = c.s.name[0], bnamed = b.s.name[0];
        if (named != bnamed ? !named : c.s.last < b.s.last) best = i;
    }
    return best;
}

void peer_ip(int fd, char* out, size_t n) {
    out[0] = 0;
    sockaddr_storage addr{};
    socklen_t len = sizeof(addr);
    if (lwip_getpeername(fd, reinterpret_cast<sockaddr*>(&addr), &len) != 0) return;
    if (addr.ss_family == AF_INET) {
        inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in*>(&addr)->sin_addr, out, n);
        return;
    }
#if CONFIG_LWIP_IPV6
    if (addr.ss_family == AF_INET6) {
        const auto* a6 = reinterpret_cast<sockaddr_in6*>(&addr);
        // IPv4 über einen IPv6-Socket (::ffff:a.b.c.d): nur den IPv4-Teil zeigen
        const uint8_t* b = reinterpret_cast<const uint8_t*>(&a6->sin6_addr);
        static constexpr uint8_t kMapped[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff};
        if (std::memcmp(b, kMapped, 12) == 0) {
            std::snprintf(out, n, "%u.%u.%u.%u", b[12], b[13], b[14], b[15]);
        } else {
            inet_ntop(AF_INET6, &a6->sin6_addr, out, n);
        }
    }
#endif
}

void session_closed(void* p) {
    auto* s = static_cast<Session*>(p);
    if (s->slot >= 0 && s->slot < kMaxClients) {
        Client& c = s_clients[s->slot];
        c.fd = -1;
        c.seen_us = esp_timer_get_time();
        if (const uint32_t now = epoch_now()) c.s.last = now;
    }
    delete s;
}

void touch(Client& c) {
    c.seen_us = esp_timer_get_time();
    if (const uint32_t now = epoch_now()) {
        if (!c.s.first) c.s.first = now;
        c.s.last = now;
    }
}

// --- Hardware -----------------------------------------------------------------------------------------------

temperature_sensor_handle_t s_tsens = nullptr;

float chip_temperature() {
    if (!s_tsens) {
        temperature_sensor_config_t cfg = TEMPERATURE_SENSOR_CONFIG_DEFAULT(10, 80);
        if (temperature_sensor_install(&cfg, &s_tsens) != ESP_OK || temperature_sensor_enable(s_tsens) != ESP_OK) {
            s_tsens = nullptr;
            return -999;
        }
    }
    float c = -999;
    temperature_sensor_get_celsius(s_tsens, &c);
    return c;
}

// Größe des laufenden Programms (einmal ermittelt: liest den Image-Kopf aus dem Flash)
uint32_t app_image_size() {
    static uint32_t size = 0;
    if (size) return size;
    const esp_partition_t* run = esp_ota_get_running_partition();
    if (!run) return 0;
    const esp_partition_pos_t pos{run->address, run->size};
    esp_image_metadata_t meta{};
    if (esp_image_get_metadata(&pos, &meta) == ESP_OK) size = meta.image_len;
    return size;
}

void mac_str(esp_mac_type_t type, char* out, size_t n) {
    uint8_t m[6] = {};
    esp_read_mac(m, type);
    std::snprintf(out, n, "%02x:%02x:%02x:%02x:%02x:%02x", m[0], m[1], m[2], m[3], m[4], m[5]);
}

void ip4(Json& j, const char* k, const esp_ip4_addr_t& a) {
    char b[16];
    std::snprintf(b, sizeof(b), IPSTR, IP2STR(&a));
    j.str(k, a.addr ? b : "");
}

const char* part_type(const esp_partition_t* p) {
    if (p->type == ESP_PARTITION_TYPE_APP) return p->subtype == ESP_PARTITION_SUBTYPE_APP_FACTORY ? "app" : "ota";
    switch (p->subtype) {
        case ESP_PARTITION_SUBTYPE_DATA_OTA: return "otadata";
        case ESP_PARTITION_SUBTYPE_DATA_NVS: return "nvs";
        case ESP_PARTITION_SUBTYPE_DATA_COREDUMP: return "coredump";
        case ESP_PARTITION_SUBTYPE_DATA_LITTLEFS: return "littlefs";
        case ESP_PARTITION_SUBTYPE_DATA_SPIFFS: return "spiffs";
        default: return "data";
    }
}

std::string hardware_json() {
    const esp_app_desc_t* app = esp_app_get_description();
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    uint32_t flash_size = 0, flash_id = 0;
    esp_flash_get_size(nullptr, &flash_size);
    esp_flash_read_id(nullptr, &flash_id);
    const perf::Snapshot p = perf::snapshot();
    char b[24];

    Json j;
    j.obj();
    j.str("board", "Sunton ESP32-8048S050C_I");

    j.obj("panel").str("type", "5\" TFT, RGB565 parallel").num("w", board::kLcdWidth).num("h", board::kLcdHeight);
    j.num("renderW", board::kLcdWidth / 2).num("renderH", board::kLcdHeight / 2);
    j.num("hz", p.lcd_hz, 2).num("pclk", p.pclk_mhz, 2).num("isr", p.isr_pct, 1).num("desync", p.desync);
    j.str("mode", p.mode ? p.mode : "").end_obj();

    board::touch::Info ti{};
    uint32_t touches = 0;
    const bool touch_ok = hwtest::touch_info(&ti, &touches);
    j.obj("touch").boolean("ok", touch_ok);
    if (touch_ok) {
        std::snprintf(b, sizeof(b), "GT%s", ti.product);
        j.str("chip", b);
        std::snprintf(b, sizeof(b), "0x%02X", ti.address);
        j.str("addr", b);
        std::snprintf(b, sizeof(b), "0x%04X", ti.firmware);
        j.str("fw", b).num("w", ti.x_resolution).num("h", ti.y_resolution).num("cfg", ti.config_version);
        j.num("touches", touches);
    }
    j.end_obj();

    j.obj("chip").str("model", "ESP32-S3").num("rev", chip.revision / 100.0, 2).num("cores", chip.cores);
    j.num("mhz", CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);
    const float t = chip_temperature();
    if (t > -100) j.num("temp", t, 1);
    else j.null("temp");
    j.boolean("wifi", chip.features & CHIP_FEATURE_WIFI_BGN).boolean("ble", chip.features & CHIP_FEATURE_BLE);
    j.end_obj();

    char mac_sta[20], mac_ap[20], mac_bt[20];
    mac_str(ESP_MAC_WIFI_STA, mac_sta, sizeof(mac_sta));
    mac_str(ESP_MAC_WIFI_SOFTAP, mac_ap, sizeof(mac_ap));
    mac_str(ESP_MAC_BT, mac_bt, sizeof(mac_bt));
    j.obj("mac").str("wifi", mac_sta).str("ap", mac_ap).str("bt", mac_bt).end_obj();

#if CONFIG_ESPTOOLPY_FLASHMODE_QIO
    const char* flash_mode = "QIO";
#elif CONFIG_ESPTOOLPY_FLASHMODE_QOUT
    const char* flash_mode = "QOUT";
#elif CONFIG_ESPTOOLPY_FLASHMODE_DOUT
    const char* flash_mode = "DOUT";
#else
    const char* flash_mode = "DIO";
#endif
    std::snprintf(b, sizeof(b), "0x%06" PRIX32, flash_id);
    j.obj("flash").num("mb", flash_size >> 20).str("id", b).str("mode", flash_mode).str("freq", CONFIG_ESPTOOLPY_FLASHFREQ);
    j.end_obj();
#if CONFIG_SPIRAM_MODE_OCT
    const char* psram_mode = "Octal";
#else
    const char* psram_mode = "Quad";
#endif
    j.obj("psram").num("kb", esp_psram_is_initialized() ? esp_psram_get_size() >> 10 : 0).str("mode", psram_mode);
    j.num("mhz", CONFIG_SPIRAM_SPEED).end_obj();
    j.obj("heap").num("intTotal", heap_caps_get_total_size(MALLOC_CAP_INTERNAL) >> 10);
    j.num("psramTotal", heap_caps_get_total_size(MALLOC_CAP_SPIRAM) >> 10).end_obj();

    // Pinbelegung (aus board/pins.h)
    j.obj("pins").num("de", board::kLcdDe).num("vsync", board::kLcdVsync).num("hsync", board::kLcdHsync);
    j.num("pclk", board::kLcdPclk).arr("data");
    for (gpio_num_t g : board::kLcdData) j.num(nullptr, g);
    j.end_arr().num("backlight", board::kBacklight).num("sda", board::kTouchSda).num("scl", board::kTouchScl);
    j.num("touchRst", board::kTouchRst).end_obj();

    j.obj("build").str("v", app->version).str("date", app->date).str("time", app->time).str("idf", app->idf_ver);
    j.str("compiler", __VERSION__).end_obj();

    // Partitionen mit Belegung des laufenden Programms
    const esp_partition_t* run = esp_ota_get_running_partition();
    const esp_partition_t* next_boot = esp_ota_get_boot_partition();
    j.num("appSize", app_image_size()).num("appPart", run ? run->size : 0);
    j.arr("partitions");
    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, nullptr);
    for (; it; it = esp_partition_next(it)) {
        const esp_partition_t* pt = esp_partition_get(it);
        j.obj().str("label", pt->label).str("type", part_type(pt)).num("offset", pt->address).num("size", pt->size);
        if (pt->type == ESP_PARTITION_TYPE_APP) {
            esp_app_desc_t d;
            j.str("v", esp_ota_get_partition_description(pt, &d) == ESP_OK ? d.version : "");
            j.boolean("running", run && run->address == pt->address);
            j.boolean("nextBoot", next_boot && next_boot->address == pt->address);
        }
        j.end_obj();
    }
    esp_partition_iterator_release(it);
    j.end_arr();

    nvs_stats_t nvs{};
    nvs_get_stats(nullptr, &nvs);
    j.obj("nvs").num("used", nvs.used_entries).num("free", nvs.free_entries).num("total", nvs.total_entries).end_obj();

    // Netz: Adresse, Maske, Gateway, DNS
    const wifi::Status w = wifi::status();
    j.obj("net").str("host", wifi::kHostname).str("mdns", wifi::kMdnsHost);
    esp_netif_t* sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_ip_info_t ip{};
    if (sta && esp_netif_get_ip_info(sta, &ip) == ESP_OK) {
        ip4(j, "ip", ip.ip);
        ip4(j, "mask", ip.netmask);
        ip4(j, "gw", ip.gw);
    }
    esp_netif_dns_info_t dns{};
    if (sta && esp_netif_get_dns_info(sta, ESP_NETIF_DNS_MAIN, &dns) == ESP_OK && dns.ip.type == ESP_IPADDR_TYPE_V4) {
        ip4(j, "dns", dns.ip.u_addr.ip4);
    }
    wifi_ap_record_t ap{};
    if (w.state == wifi::State::Connected && esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
        std::snprintf(b, sizeof(b), "%02x:%02x:%02x:%02x:%02x:%02x", ap.bssid[0], ap.bssid[1], ap.bssid[2], ap.bssid[3],
                      ap.bssid[4], ap.bssid[5]);
        j.str("bssid", b).str("phy", ap.phy_11n ? "802.11n" : ap.phy_11g ? "802.11g" : "802.11b");
    }
    const int64_t up_s = esp_timer_get_time() / 1000000;
    if (w.state == wifi::State::Connected) j.num("connectedS", static_cast<double>(up_s - w.connected_since_s));
    j.end_obj();

    j.obj("time").boolean("valid", timesync::valid()).num("syncs", timesync::sync_count()).num("epoch", epoch_now());
    j.str("tz", "Europa/Berlin").end_obj();
    j.end_obj();
    return std::move(j.text());
}

// --- Endpunkte ----------------------------------------------------------------------------------------------

esp_err_t system_get(httpd_req_t* req) { return send_json(req, hardware_json()); }

esp_err_t clients_get(httpd_req_t* req) {
    load();
    const int64_t now = esp_timer_get_time();
    const int self = httpd_req_to_sockfd(req);
    char self_ip[40];
    peer_ip(self, self_ip, sizeof(self_ip));
    Json j;
    j.obj().num("epoch", epoch_now()).str("you", self_ip).arr("list");
    for (const Client& c : s_clients) {
        if (!c.s.id[0]) continue;
        const bool online = c.fd >= 0 && httpd_ws_get_fd_info(server(), c.fd) == HTTPD_WS_CLIENT_WEBSOCKET;
        j.obj().str("id", c.s.id).str("name", c.s.name).str("agent", c.s.agent).str("ip", c.ip);
        j.str("page", online ? c.page : "").boolean("online", online).num("first", c.s.first).num("last", c.s.last);
        if (c.seen_us >= 0) j.num("agoS", online ? 0 : static_cast<double>((now - c.seen_us) / 1000000));
        else j.null("agoS");
        j.num("visits", c.s.visits).end_obj();
    }
    j.end_arr();

    // Setup-Modus: Geräte am eigenen Access Point (MAC, IP vom DHCP-Server)
    j.arr("ap");
    wifi_sta_list_t sta{};
    if (wifi::status().ap_active && esp_wifi_ap_get_sta_list(&sta) == ESP_OK && sta.num > 0) {
        esp_netif_pair_mac_ip_t pairs[ESP_WIFI_MAX_CONN_NUM] = {};
        const int n = std::min<int>(sta.num, ESP_WIFI_MAX_CONN_NUM);
        for (int i = 0; i < n; ++i) std::memcpy(pairs[i].mac, sta.sta[i].mac, 6);
        esp_netif_t* apif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
        if (apif) esp_netif_dhcps_get_clients_by_mac(apif, n, pairs);
        for (int i = 0; i < n; ++i) {
            char mac[20], ip[16];
            const uint8_t* m = sta.sta[i].mac;
            std::snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x", m[0], m[1], m[2], m[3], m[4], m[5]);
            std::snprintf(ip, sizeof(ip), IPSTR, IP2STR(&pairs[i].ip));
            j.obj().str("mac", mac).str("ip", pairs[i].ip.addr ? ip : "").num("rssi", sta.sta[i].rssi).end_obj();
        }
    }
    j.end_arr().end_obj();
    return send_json(req, j.text());
}

esp_err_t clients_name_post(httpd_req_t* req) {
    std::string body, id, name;
    if (!read_body(req, &body, 256) || !form_value(body, "id", &id)) return reply(req, false, "Geräte-ID fehlt");
    if (!authorized(req, &body)) return deny(req);
    form_value(body, "name", &name);
    load();
    int i = find(id.c_str());
    // Import einer Sicherung (create=1): Namen auch für Browser übernehmen, die sich seit dem Werksreset noch nicht
    // gemeldet haben. Sie bekommen ihren Namen zurück, sobald sie die Seite wieder öffnen.
    std::string create;
    if (i < 0 && form_value(body, "create", &create) && create == "1" && !name.empty() && id.size() >= 8 &&
        id.size() <= 16) {
        i = free_slot();
        if (i >= 0) {
            s_clients[i] = Client{};
            copy(s_clients[i].s.id, sizeof(s_clients[i].s.id), id);
        }
    }
    if (i < 0) return reply(req, false, "Gerät unbekannt");
    copy(s_clients[i].s.name, sizeof(s_clients[i].s.name), name);
    save();
    return reply(req, true, name.empty() ? "Name entfernt" : "Gespeichert");
}

}  // namespace

// Nachricht eines Browsers über /ws (im Server-Task)
void client_message(httpd_req_t* req, const std::string& msg) {
    load();
    std::string v;
    auto* sess = static_cast<Session*>(req->sess_ctx);
    if (sess && sess->magic != kSessionMagic) return;  // fremde Sitzung (z. B. Live-Log)

    if (form_value(msg, "hello", &v)) {
        if (v.size() < 8 || v.size() > 16) return;
        int i = find(v.c_str());
        bool fresh = false;
        if (i < 0) {
            i = free_slot();
            if (i < 0) return;
            s_clients[i] = Client{};
            copy(s_clients[i].s.id, sizeof(s_clients[i].s.id), v);
            fresh = true;
        }
        Client& c = s_clients[i];
        if (form_value(msg, "ua", &v)) copy(c.s.agent, sizeof(c.s.agent), v);
        c.fd = httpd_req_to_sockfd(req);
        peer_ip(c.fd, c.ip, sizeof(c.ip));
        // Neue Seite derselben Browser-Sitzung: alte Zuordnung lösen
        for (int k = 0; k < kMaxClients; ++k) {
            if (k != i && s_clients[k].fd == c.fd) s_clients[k].fd = -1;
        }
        c.s.visits++;
        touch(c);
        if (!sess) {
            sess = new Session;
            req->sess_ctx = sess;
            req->free_ctx = session_closed;
        }
        sess->slot = i;
        if (fresh) ESP_LOGI(TAG, "Neues Gerät verbunden: %s (%s)", c.s.agent, c.ip);
        if (fresh || esp_timer_get_time() - s_saved_us > kSaveEveryUs) save();
    }
    if (sess && sess->slot >= 0 && form_value(msg, "page", &v)) {
        Client& c = s_clients[sess->slot];
        copy(c.page, sizeof(c.page), v);
        touch(c);
    }
}

// Für die Sicherung (GET /api/backup): Geräte mit eigenem Namen als Feld „names“
void client_names_json(Json& j) {
    load();
    j.arr("names");
    for (const Client& c : s_clients) {
        if (c.s.id[0] && c.s.name[0]) j.obj().str("id", c.s.id).str("name", c.s.name).end_obj();
    }
    j.end_arr();
}

// --- Kamera (seit 0.5.6): Zustand, Orte, Figuren; Wechseln und Folgen ------------------------------------------

esp_err_t camera_get(httpd_req_t* req) {
    const gfx::CameraState st = gfx::camera_state();
    // Liste im PSRAM (bis 64 Figuren à 28 Byte), der interne RAM bleibt frei
    constexpr int kMax = 64;
    auto* figs = static_cast<pixelia::FigureInfo*>(heap_caps_malloc(kMax * sizeof(pixelia::FigureInfo), MALLOC_CAP_SPIRAM));
    auto* places = static_cast<int*>(heap_caps_malloc(kMax * sizeof(int), MALLOC_CAP_SPIRAM));
    const int n = figs && places ? gfx::figure_list(figs, places, kMax) : 0;
    Json j;
    j.obj().num("mode", st.mode).num("target", st.target).str("targetName", st.target_name).num("place", st.place);
    j.num("returnIn", st.return_in_s);
    j.arr("places");
    for (int i = 0; i < gfx::place_count(); ++i)
        j.obj().str("id", gfx::place_id(i)).str("title", gfx::place_title(i)).end_obj();
    j.end_arr().arr("figures");
    for (int i = 0; i < n; ++i)
        j.obj().num("id", figs[i].id).str("name", figs[i].name).num("place", places[i]).boolean("lead", figs[i].lead).end_obj();
    j.end_arr().end_obj();
    heap_caps_free(figs);
    heap_caps_free(places);
    return send_json(req, j.text());
}

esp_err_t camera_post(httpd_req_t* req) {
    std::string body, a, v;
    if (!read_body(req, &body, 128)) return reply(req, false, "Ungültige Anfrage");
    if (!authorized(req, &body)) return deny(req);
    form_value(body, "a", &a);
    form_value(body, "v", &v);
    if (a == "home") gfx::camera_home();
    else if (a == "follow" && std::atoi(v.c_str()) > 0) gfx::camera_follow(static_cast<uint16_t>(std::atoi(v.c_str())));
    else if (a == "go" && std::atoi(v.c_str()) >= 0 && std::atoi(v.c_str()) < gfx::place_count()) gfx::go_place(std::atoi(v.c_str()));
    else return reply(req, false, "Unbekannte Aktion");
    return reply(req, true, "Ok");
}

// --- Weltzeit (seit 0.5.7): Kalender, Sonne, Mond, Wetter der Simulation -----------------------------------------

std::string iso_day(int32_t days) {
    int y, m, d;
    pixelia::civil_from_days(days, &y, &m, &d);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", y, m, d);
    return buf;
}

std::string hhmm(int minute) {
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%02d:%02d", minute / 60 % 24, minute % 60);
    return buf;
}

esp_err_t world_get(httpd_req_t* req) {
    const gfx::WorldInfo w = gfx::world_info();
    const auto season = static_cast<pixelia::Season>(w.season & 3);
    const auto next = static_cast<pixelia::Season>((w.season + 1) & 3);
    Json j;
    j.obj().boolean("valid", w.valid).boolean("started", w.started).boolean("test", w.test);
    if (w.started) {
        j.str("start", iso_day(w.start_day).c_str()).num("day", w.day).num("year", w.year);
        j.str("season", pixelia::season_name(season)).str("seasonTitle", pixelia::season_title(season));
        j.num("dayInSeason", w.day_in_season).num("seasonDays", w.season_days).boolean("evening", w.season_evening);
        j.str("seasonFrom", iso_day(w.season_first).c_str()).str("seasonTo", iso_day(w.season_last).c_str());
        j.str("next", pixelia::season_title(next)).str("nextFrom", iso_day(w.season_last + 1).c_str());
        j.str("sunrise", hhmm(w.sunrise).c_str()).str("sunset", hhmm(w.sunset).c_str());
        j.str("dayPart", pixelia::day_part_title(static_cast<pixelia::DayPart>(w.day_part)));
        j.obj("moon").str("title", w.moon_title).num("illum", (w.moon_illum + 5) / 10).num("phase", w.moon_phase);
        j.boolean("waxing", w.moon_waxing).boolean("up", w.moon_up).boolean("visible", w.moon_visible).end_obj();
        j.arr("firsts");
        constexpr const char* kFirsts[] = {"Erster Schnee", "Erstes Gewitter", "Erster Herbstnebel"};
        for (int i = 0; i < 3; ++i)
            if (w.firsts & (1 << i)) j.str(nullptr, kFirsts[i]);
        for (int s = 0; s < 4; ++s)
            if (w.firsts & (1 << (4 + s)))
                j.str(nullptr, (std::string("Erster Vollmond ") + pixelia::season_genitive(static_cast<pixelia::Season>(s))).c_str());
        j.end_arr();
    }
    j.num("seasonLen", w.season_len);
    j.obj("weather").str("kind", pixelia::weather_name(static_cast<pixelia::WeatherKind>(w.weather)));
    j.str("title", pixelia::weather_title(static_cast<pixelia::WeatherKind>(w.weather))).num("pct", w.weather_pct);
    j.boolean("override", w.weather_override).end_obj();
    if (w.started) {
        // Vorschau der Simulation (seit 0.5.7): die nächsten drei Wetterzeiten; ein Gewitter mittendrin steht nicht drin
        constexpr const char* kSlots[] = {"morgens", "mittags", "abends", "nachts"};
        j.arr("forecast");
        for (int i = 0; i < 3; ++i) {
            const auto k = static_cast<pixelia::WeatherKind>(w.forecast_kind[i]);
            j.obj().str("slot", kSlots[w.forecast_slot[i] & 3]).str("kind", pixelia::weather_name(k));
            j.str("title", pixelia::weather_title(k)).num("pct", w.forecast_pct[i]).end_obj();
        }
        j.end_arr();
    }
    // Speicherstand (seit 0.5.9): zuletzt gespeichert vor ago Sekunden (−1 = seit dem Start noch nicht)
    const gfx::SaveInfo s = gfx::save_info();
    const int64_t now = esp_timer_get_time();
    j.obj("save").num("gen", static_cast<int64_t>(s.generation)).num("bytes", static_cast<int64_t>(s.bytes));
    j.num("ago", s.at_us ? (now - s.at_us) / 1000000 : -1).num("next", s.next_us > now ? (s.next_us - now) / 1000000 : 0);
    j.boolean("loaded", s.loaded).num("packUs", static_cast<int64_t>(s.pack_us)).num("writeMs", static_cast<int64_t>(s.write_ms));
    j.num("figures", s.figures).num("transit", s.transit).num("chronicle", s.chronicle);
    j.num("caughtUp", s.caught_up).num("skipped", s.skipped).num("catchingUp", s.catching_up);
    j.boolean("test", s.test).boolean("waiting", s.waiting).str("error", s.error).end_obj();
    // Figuren und Chronik nur auf Wunsch (?mehr=1): die Grafik-Task füllt die Ansicht im nächsten Bild
    std::string more;
    if (query_value(req, "mehr", &more) && more == "1") {
        static gfx::WorldView* v = static_cast<gfx::WorldView*>(heap_caps_malloc(sizeof(gfx::WorldView), MALLOC_CAP_SPIRAM));
        if (v && gfx::world_view(v)) {
            j.arr("figures");
            for (int i = 0; i < v->figures; ++i) {
                const gfx::FigureRow& f = v->fig[i];
                j.obj().num("id", f.id).str("name", f.name).num("age", f.age).str("place", f.place).str("home", f.home);
                j.boolean("lead", f.lead).boolean("moving", f.moving).num("relations", f.relations).num("memories", f.memories);
                j.arr("skills");
                for (int k = 0; k < f.skill_n; ++k) {
                    j.obj().str("t", pixelia::skill_title(static_cast<pixelia::Skill>(f.skills[k].skill)));
                    j.str("l", pixelia::skill_level_title(static_cast<pixelia::SkillLevel>(f.skills[k].level)));
                    j.num("n", f.skills[k].level).end_obj();
                }
                j.end_arr().end_obj();
            }
            j.end_arr();
            j.arr("chronicle");
            for (int i = 0; i < v->chronicle; ++i)
                j.obj().num("day", v->ch[i].day).str("time", hhmm(v->ch[i].minute).c_str()).num("kind", v->ch[i].kind)
                    .str("text", v->ch[i].text).end_obj();
            j.end_arr();
            j.num("visits", v->visits).num("visitsOpen", v->visits_open);
        }
    }
    j.end_obj();
    return send_json(req, j.text());
}

// Speicherstand jetzt schreiben (seit 0.5.9, Knopf auf der Seite Welt)
esp_err_t world_post(httpd_req_t* req) {
    std::string body, a;
    if (!read_body(req, &body, 128)) return reply(req, false, "Ungültige Anfrage");
    if (!authorized(req, &body)) return deny(req);
    form_value(body, "a", &a);
    if (a != "save") return reply(req, false, "Unbekannte Aktion");
    const gfx::SaveInfo s = gfx::save_info();
    if (s.test) return reply(req, false, "Testuhr läuft: es wird nicht gespeichert");
    if (s.waiting) return reply(req, false, "Die Uhrzeit fehlt noch: es wird noch nicht gespeichert");
    return reply(req, gfx::request_save(), "Speicherstand wird geschrieben");
}

void register_info() {
    add("/api/world", HTTP_GET, world_get);
    add("/api/world", HTTP_POST, world_post);
    add("/api/camera", HTTP_GET, camera_get);
    add("/api/camera", HTTP_POST, camera_post);
    add("/api/system", HTTP_GET, system_get);
    add("/api/clients", HTTP_GET, clients_get);
    add("/api/clients/name", HTTP_POST, clients_name_post);
}

}  // namespace app::web::detail
