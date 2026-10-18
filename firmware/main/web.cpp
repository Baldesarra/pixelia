// Webserver: Einrichtungsseite, JSON-API, WebSocket, Update-Upload, Captive Portal (siehe web.h).
//
// API (alle Antworten JSON, Eingaben als Formulardaten application/x-www-form-urlencoded):
//   GET  /api/status          Gesamtstatus (auch per WebSocket /ws jede Sekunde)
//   GET  /api/wifi            gespeicherte Netze (nur Namen) und letzter Scan
//   POST /api/wifi/scan       neu scannen
//   POST /api/wifi/add        ssid, pass: speichern und verbinden
//   POST /api/wifi/remove     ssid
//   POST /api/settings        rot (0|180), bright (1–100)
//   POST /api/ota             Rohdaten der .bin (application/octet-stream)
//   POST /api/ota/ack         Hinweis auf zurückgerolltes Update gelesen
//   POST /api/reboot
//   POST /api/speedtest       Daten empfangen und verwerfen (Messung der Übertragungsrate)

#include "web.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>

#include "app.h"
#include "esp_app_desc.h"
#include "esp_heap_caps.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "net.h"
#include "ota.h"
#include "settings.h"

extern const char kSetupHtmlStart[] asm("_binary_setup_html_start");
extern const char kSetupHtmlEnd[] asm("_binary_setup_html_end");

namespace app::web {

namespace {
constexpr const char* TAG = "web";
httpd_handle_t s_server = nullptr;
std::atomic<int> s_ws_clients{0};

// --- JSON ---------------------------------------------------------------------------------------------------

// Minimaler JSON-Schreiber: Kommas und Escaping automatisch
class Json {
public:
    Json& obj(const char* k = nullptr) { return open(k, '{'); }
    Json& arr(const char* k = nullptr) { return open(k, '['); }
    Json& end_obj() { return close('}'); }
    Json& end_arr() { return close(']'); }
    Json& str(const char* k, const char* v) {
        key(k);
        quote(v);
        return *this;
    }
    Json& num(const char* k, double v, int decimals = 0) {
        key(k);
        char b[32];
        std::snprintf(b, sizeof(b), "%.*f", decimals, v);
        s_ += b;
        return *this;
    }
    Json& boolean(const char* k, bool v) {
        key(k);
        s_ += v ? "true" : "false";
        return *this;
    }
    Json& null(const char* k) {
        key(k);
        s_ += "null";
        return *this;
    }
    std::string& text() { return s_; }

private:
    Json& open(const char* k, char c) {
        key(k);
        s_ += c;
        first_[++depth_] = true;
        return *this;
    }
    Json& close(char c) {
        s_ += c;
        --depth_;
        return *this;
    }
    void key(const char* k) {
        if (depth_ >= 0) {
            if (!first_[depth_]) s_ += ',';
            first_[depth_] = false;
        }
        if (k) {
            quote(k);
            s_ += ':';
        }
    }
    void quote(const char* v) {
        s_ += '"';
        for (const char* p = v; *p; ++p) {
            auto c = static_cast<unsigned char>(*p);
            if (c == '"' || c == '\\') {
                s_ += '\\';
                s_ += static_cast<char>(c);
            } else if (c < 0x20) {
                char b[8];
                std::snprintf(b, sizeof(b), "\\u%04x", c);
                s_ += b;
            } else {
                s_ += static_cast<char>(c);
            }
        }
        s_ += '"';
    }
    std::string s_;
    bool first_[12] = {};
    int depth_ = -1;
};

std::string status_json() {
    const esp_app_desc_t* app = esp_app_get_description();
    wifi::Status w = wifi::status();
    ota::Status o = ota::status();
    perf::Snapshot p = perf::snapshot();
    settings::Values set = settings::get();

    Json j;
    j.obj();
    j.str("v", app->version).str("built", (std::string(app->date) + " " + app->time).c_str()).str("idf", app->idf_ver);
    j.str("part", o.running).num("uptime", static_cast<double>(esp_timer_get_time() / 1000000));
    j.str("host", wifi::kMdnsHost);

    j.obj("wifi");
    j.str("state", wifi::state_name(w.state)).str("ssid", w.ssid).num("rssi", w.rssi).num("ch", w.channel);
    j.str("ip", w.ip).boolean("ap", w.ap_active).num("apClients", w.ap_clients).num("retry", w.retry_in_s);
    j.str("err", w.last_error).num("saved", w.saved);
    j.end_obj();

    char clock[40] = "";
    if (timesync::valid()) {
        time_t now = time(nullptr);
        tm lt;
        localtime_r(&now, &lt);
        strftime(clock, sizeof(clock), "%d.%m.%Y %H:%M:%S", &lt);
    }
    j.obj("time").boolean("valid", timesync::valid()).str("local", clock).end_obj();

    j.obj("perf");
    j.num("fps", p.fps, 1).num("render", p.render_ms, 2).num("isr", p.isr_pct, 1).num("lcd", p.lcd_hz, 2);
    j.num("cpu0", p.cpu[0]).num("cpu1", p.cpu[1]).num("desync", p.desync);
    j.num("ram", p.ram_kb).num("ramMin", p.ram_min_kb).num("ramBlk", p.ram_blk_kb);
    j.num("psram", p.psram_kb).num("psramMin", p.psram_min_kb);
    j.end_obj();

    j.obj("ota");
    j.boolean("busy", o.busy).num("bytes", o.bytes).num("total", o.total).str("newV", o.new_version);
    j.str("err", o.last_error).boolean("trial", o.trial).num("trialS", o.trial_elapsed_s);
    j.num("trialNeed", ota::kTrialSeconds).str("trialNote", o.trial_note).boolean("passed", o.trial_passed);
    j.end_obj();

    if (o.rollback) {
        j.obj("rollback").str("v", o.rollback_version).str("why", o.rollback_reason).end_obj();
    } else {
        j.null("rollback");
    }
    j.obj("set").num("rot", set.rotate180 ? 180 : 0).num("bright", set.brightness).end_obj();
    j.end_obj();
    return std::move(j.text());
}

// --- Hilfen -------------------------------------------------------------------------------------------------

esp_err_t send_json(httpd_req_t* req, const std::string& body, const char* status = "200 OK") {
    httpd_resp_set_status(req, status);
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, body.data(), static_cast<ssize_t>(body.size()));
}

esp_err_t reply(httpd_req_t* req, bool ok, const char* msg, const char* status = nullptr) {
    Json j;
    j.obj().boolean("ok", ok).str("msg", msg).end_obj();
    return send_json(req, j.text(), status ? status : (ok ? "200 OK" : "400 Bad Request"));
}

// Formulardaten lesen (klein, nur für Einstellungen und WLAN)
bool read_body(httpd_req_t* req, std::string* out, size_t max) {
    if (req->content_len > max) return false;
    out->resize(req->content_len);
    size_t got = 0;
    int timeouts = 0;
    while (got < req->content_len) {
        int r = httpd_req_recv(req, out->data() + got, req->content_len - got);
        if (r == HTTPD_SOCK_ERR_TIMEOUT && ++timeouts < 3) continue;
        if (r <= 0) return false;
        got += r;
    }
    return true;
}

int hex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// Wert zu key aus „a=1&b=2“, URL-dekodiert
bool form_value(const std::string& body, const char* key, std::string* out) {
    const size_t kl = std::strlen(key);
    size_t pos = 0;
    while (pos <= body.size()) {
        size_t amp = body.find('&', pos);
        if (amp == std::string::npos) amp = body.size();
        if (amp - pos > kl && body.compare(pos, kl, key) == 0 && body[pos + kl] == '=') {
            out->clear();
            for (size_t i = pos + kl + 1; i < amp; ++i) {
                char c = body[i];
                if (c == '+') {
                    *out += ' ';
                } else if (c == '%' && i + 2 < amp && hex(body[i + 1]) >= 0 && hex(body[i + 2]) >= 0) {
                    *out += static_cast<char>(hex(body[i + 1]) * 16 + hex(body[i + 2]));
                    i += 2;
                } else {
                    *out += c;
                }
            }
            return true;
        }
        if (amp - pos == kl && body.compare(pos, kl, key) == 0) {
            out->clear();
            return true;
        }
        pos = amp + 1;
    }
    return false;
}

bool setup_active() { return wifi::status().ap_active; }

// --- WebSocket ----------------------------------------------------------------------------------------------

// An alle WebSocket-Clients senden; nur im Kontext des Server-Tasks aufrufen
void broadcast_in_server(const std::string& text) {
    size_t fds = 8;
    int clients[8];
    if (httpd_get_client_list(s_server, &fds, clients) != ESP_OK) return;
    httpd_ws_frame_t frame{};
    frame.type = HTTPD_WS_TYPE_TEXT;
    frame.payload = reinterpret_cast<uint8_t*>(const_cast<char*>(text.data()));
    frame.len = text.size();
    int n = 0;
    for (size_t i = 0; i < fds; ++i) {
        if (httpd_ws_get_fd_info(s_server, clients[i]) != HTTPD_WS_CLIENT_WEBSOCKET) continue;
        ++n;
        httpd_ws_send_frame_async(s_server, clients[i], &frame);
    }
    s_ws_clients = n;
}

void broadcast_work(void* arg) {
    auto* text = static_cast<std::string*>(arg);
    broadcast_in_server(*text);
    delete text;
}

void status_task(void*) {
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        if (s_ws_clients <= 0 || ota::status().busy) continue;
        auto* text = new std::string(status_json());
        if (httpd_queue_work(s_server, broadcast_work, text) != ESP_OK) delete text;
    }
}

esp_err_t ws_handler(httpd_req_t* req) {
    if (req->method == HTTP_GET) {
        // Handschlag erledigt: gleich den aktuellen Stand schicken
        s_ws_clients++;
        auto* text = new std::string(status_json());
        if (httpd_queue_work(s_server, broadcast_work, text) != ESP_OK) delete text;
        return ESP_OK;
    }
    httpd_ws_frame_t frame{};
    uint8_t buf[64];
    if (httpd_ws_recv_frame(req, &frame, 0) != ESP_OK) return ESP_FAIL;
    if (frame.len >= sizeof(buf)) return ESP_OK;  // Nachrichten vom Browser werden noch nicht gebraucht
    frame.payload = buf;
    return httpd_ws_recv_frame(req, &frame, frame.len);
}

// --- Seiten und API -----------------------------------------------------------------------------------------

esp_err_t root_get(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, kSetupHtmlStart, kSetupHtmlEnd - kSetupHtmlStart - 1);
}

esp_err_t status_get(httpd_req_t* req) { return send_json(req, status_json()); }

esp_err_t wifi_json(httpd_req_t* req, bool fresh) {
    char saved[wifi::kMaxSaved][33];
    int ns = wifi::saved_networks(saved, wifi::kMaxSaved);
    static wifi::ScanEntry scan[24];  // nur im Server-Task benutzt
    int n = wifi::scan(scan, 24, fresh);
    Json j;
    j.obj().arr("saved");
    for (int i = 0; i < ns; ++i) j.str(nullptr, saved[i]);
    j.end_arr().arr("scan");
    for (int i = 0; i < n; ++i) {
        j.obj().str("ssid", scan[i].ssid).num("rssi", scan[i].rssi).num("ch", scan[i].channel);
        j.boolean("secure", scan[i].secure).boolean("saved", scan[i].saved).end_obj();
    }
    j.end_arr().end_obj();
    return send_json(req, j.text());
}

esp_err_t wifi_get(httpd_req_t* req) { return wifi_json(req, false); }
esp_err_t wifi_scan_post(httpd_req_t* req) { return wifi_json(req, true); }

esp_err_t wifi_add_post(httpd_req_t* req) {
    std::string body, ssid, pass;
    if (!read_body(req, &body, 512) || !form_value(body, "ssid", &ssid)) return reply(req, false, "WLAN-Name fehlt");
    form_value(body, "pass", &pass);
    esp_err_t err = wifi::add_network(ssid.c_str(), pass.c_str());
    // Passwort sofort aus dem Speicher tilgen
    std::fill(body.begin(), body.end(), 0);
    std::fill(pass.begin(), pass.end(), 0);
    if (err == ESP_ERR_INVALID_SIZE) return reply(req, false, "Das Passwort muss 8 bis 64 Zeichen lang sein");
    if (err != ESP_OK) return reply(req, false, "WLAN-Name ungültig (1 bis 32 Zeichen)");
    if (wifi::status().state == wifi::State::Connected) {
        return reply(req, true, "Gespeichert. Pixelia nutzt das Netz, sobald es das stärkste verfügbare ist.");
    }
    return reply(req, true, "Gespeichert. Pixelia verbindet sich jetzt, das Setup-WLAN kann dabei kurz abbrechen.");
}

esp_err_t wifi_remove_post(httpd_req_t* req) {
    std::string body, ssid;
    if (!read_body(req, &body, 256) || !form_value(body, "ssid", &ssid)) return reply(req, false, "WLAN-Name fehlt");
    if (wifi::remove_network(ssid.c_str()) != ESP_OK) return reply(req, false, "Nicht gespeichert");
    return reply(req, true, "Entfernt");
}

esp_err_t settings_post(httpd_req_t* req) {
    std::string body, v;
    if (!read_body(req, &body, 256)) return reply(req, false, "Ungültige Anfrage");
    if (form_value(body, "rot", &v)) settings::set_rotation180(v == "180");
    if (form_value(body, "bright", &v)) settings::set_brightness(std::atoi(v.c_str()));
    return reply(req, true, "Gespeichert");
}

esp_err_t reboot_post(httpd_req_t* req) {
    ESP_LOGW(TAG, "Neustart über die Weboberfläche");
    reply(req, true, "Neustart");
    ota::restart_soon(800);
    return ESP_OK;
}

esp_err_t ota_ack_post(httpd_req_t* req) {
    ota::ack_rollback();
    return reply(req, true, "OK");
}

// Update: Kopf sammeln und prüfen, dann in Stücken in die freie App-Partition schreiben
esp_err_t ota_post(httpd_req_t* req) {
    const size_t total = req->content_len;
    if (total < ota::kHeaderBytes) return reply(req, false, "Keine Firmware-Datei empfangen");
    constexpr size_t kChunk = 4096;
    auto* buf = static_cast<uint8_t*>(heap_caps_malloc(kChunk, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    if (!buf) return reply(req, false, "Kein Speicher frei", "500 Internal Server Error");

    size_t got = 0, filled = 0;
    int timeouts = 0;
    char error[128] = "";
    bool started = false;
    bool broken = false;  // Übertragung abgerissen oder hängt
    int64_t t0 = esp_timer_get_time();
    uint32_t last_pct = 0;
    while (got < total) {
        int r = httpd_req_recv(req, reinterpret_cast<char*>(buf + filled), std::min(kChunk - filled, total - got));
        if (r == HTTPD_SOCK_ERR_TIMEOUT && ++timeouts < 5) continue;
        if (r <= 0) {
            std::snprintf(error, sizeof(error), "Verbindung während der Übertragung abgebrochen");
            broken = true;
            break;
        }
        timeouts = 0;
        got += r;
        filled += r;
        if (!started) {
            if (filled < ota::kHeaderBytes && got < total) continue;
            if (ota::begin(buf, filled, total, error, sizeof(error)) != ESP_OK) break;
            started = true;
        }
        if (filled == kChunk || got == total) {
            esp_err_t err = ota::write(buf, filled);
            if (err != ESP_OK) {
                std::snprintf(error, sizeof(error), "Schreiben fehlgeschlagen: %s", esp_err_to_name(err));
                break;
            }
            filled = 0;
            // Fortschritt an alle offenen Seiten (alle 5 %)
            uint32_t pct = static_cast<uint32_t>(static_cast<uint64_t>(got) * 100 / total);
            if (pct >= last_pct + 5 || got == total) {
                last_pct = pct;
                broadcast_in_server(status_json());
            }
        }
    }
    heap_caps_free(buf);
    if (error[0] || !started) {
        if (started) ota::cancel(error);
        ESP_LOGW(TAG, "Update abgelehnt: %s", error);
        if (broken) {
            // Übertragung hängt: Verbindung sofort schließen statt den Rest abzuwarten (blockiert sonst den Server)
            return ESP_FAIL;
        }
        return reply(req, false, error[0] ? error : "Firmware unvollständig");
    }
    if (ota::finish(error, sizeof(error)) != ESP_OK) return reply(req, false, error);
    double secs = (esp_timer_get_time() - t0) / 1e6;
    ESP_LOGI(TAG, "Update empfangen: %u KB in %.1f s (%.0f KB/s)", static_cast<unsigned>(total / 1024), secs,
             total / 1024.0 / (secs > 0 ? secs : 1));
    Json j;
    j.obj().boolean("ok", true).str("msg", "Installiert und geprüft, Neustart").str("v", ota::status().new_version);
    j.end_obj();
    send_json(req, j.text());
    ota::restart_soon(1500);
    return ESP_OK;
}

// Netzwerk-Messung: nimmt beliebige Daten entgegen und verwirft sie (Diagnose der Übertragungsrate ohne Flashen,
// z. B. curl --data-binary @datei http://pixelia/api/speedtest)
esp_err_t speedtest_post(httpd_req_t* req) {
    constexpr size_t kChunk = 4096;
    auto* buf = static_cast<char*>(heap_caps_malloc(kChunk, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    if (!buf) return reply(req, false, "Kein Speicher frei", "500 Internal Server Error");
    const int64_t t0 = esp_timer_get_time();
    size_t got = 0;
    int timeouts = 0, max_gap_ms = 0;
    int64_t last = t0;
    while (got < req->content_len) {
        int r = httpd_req_recv(req, buf, std::min(kChunk, req->content_len - got));
        if (r == HTTPD_SOCK_ERR_TIMEOUT && ++timeouts < 5) continue;
        if (r <= 0) break;
        int64_t now = esp_timer_get_time();
        max_gap_ms = std::max(max_gap_ms, static_cast<int>((now - last) / 1000));
        last = now;
        got += r;
    }
    heap_caps_free(buf);
    const double secs = (esp_timer_get_time() - t0) / 1e6;
    wifi::Status w = wifi::status();
    ESP_LOGI(TAG, "Messung: %u KB in %.1f s (%.0f KB/s), längste Pause %d ms, %d Zeitüberschreitungen, %d dBm",
             static_cast<unsigned>(got / 1024), secs, got / 1024.0 / (secs > 0 ? secs : 1), max_gap_ms, timeouts, w.rssi);
    if (got < req->content_len) return ESP_FAIL;
    Json j;
    j.obj().boolean("ok", true).num("bytes", got).num("seconds", secs, 2).num("kbps", got / 1024.0 / (secs > 0 ? secs : 1));
    j.num("maxGapMs", max_gap_ms).num("timeouts", timeouts).num("rssi", w.rssi).end_obj();
    return send_json(req, j.text());
}

// Captive Portal: Prüfadressen von Android, iOS/macOS, Windows und Firefox auf die Einrichtungsseite umleiten
esp_err_t redirect_to_portal(httpd_req_t* req) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "http://192.168.4.1/");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    // iOS erkennt das Portal nur, wenn die Antwort einen Inhalt hat
    return httpd_resp_send(req, "<a href=\"http://192.168.4.1/\">Pixelia einrichten</a>", HTTPD_RESP_USE_STRLEN);
}

esp_err_t captive_check(httpd_req_t* req) {
    if (setup_active()) return redirect_to_portal(req);
    httpd_resp_set_status(req, "204 No Content");
    return httpd_resp_send(req, nullptr, 0);
}

esp_err_t not_found(httpd_req_t* req, httpd_err_code_t) {
    if (setup_active()) return redirect_to_portal(req);
    httpd_resp_set_status(req, "404 Not Found");
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    return httpd_resp_send(req, "Nicht gefunden", HTTPD_RESP_USE_STRLEN);
}

void add(const char* uri, httpd_method_t method, esp_err_t (*handler)(httpd_req_t*), bool ws = false) {
    httpd_uri_t u{};
    u.uri = uri;
    u.method = method;
    u.handler = handler;
    u.is_websocket = ws;
    esp_err_t err = httpd_register_uri_handler(s_server, &u);
    if (err != ESP_OK) ESP_LOGE(TAG, "URI %s nicht registriert: %s", uri, esp_err_to_name(err));
}
}  // namespace

void start() {
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.core_id = kSystemCore;
    cfg.stack_size = 8192;
    cfg.task_priority = 5;
    cfg.max_uri_handlers = 32;
    cfg.max_open_sockets = 7;
    cfg.lru_purge_enable = true;
    cfg.recv_wait_timeout = 10;
    cfg.send_wait_timeout = 10;
    // Captive-Portal-Abfragen kommen in Schüben, Fehlermeldungen dazu sind nur Lärm im Log
    esp_log_level_set("httpd_uri", ESP_LOG_ERROR);
    esp_log_level_set("httpd_txrx", ESP_LOG_ERROR);
    esp_log_level_set("httpd_parse", ESP_LOG_ERROR);
    esp_err_t err = httpd_start(&s_server, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Webserver startet nicht: %s", esp_err_to_name(err));
        return;
    }
    add("/", HTTP_GET, root_get);
    add("/api/status", HTTP_GET, status_get);
    add("/api/wifi", HTTP_GET, wifi_get);
    add("/api/wifi/scan", HTTP_POST, wifi_scan_post);
    add("/api/wifi/add", HTTP_POST, wifi_add_post);
    add("/api/wifi/remove", HTTP_POST, wifi_remove_post);
    add("/api/settings", HTTP_POST, settings_post);
    add("/api/ota", HTTP_POST, ota_post);
    add("/api/ota/ack", HTTP_POST, ota_ack_post);
    add("/api/reboot", HTTP_POST, reboot_post);
    add("/api/speedtest", HTTP_POST, speedtest_post);
    add("/ws", HTTP_GET, ws_handler, true);
    for (const char* uri : {"/generate_204", "/gen_204", "/hotspot-detect.html", "/library/test/success.html",
                            "/connecttest.txt", "/ncsi.txt", "/redirect", "/canonical.html", "/success.txt"}) {
        add(uri, HTTP_GET, captive_check);
    }
    httpd_register_err_handler(s_server, HTTPD_404_NOT_FOUND, not_found);
    xTaskCreatePinnedToCore(status_task, "webstatus", 4096, nullptr, 2, nullptr, kSystemCore);
    ESP_LOGI(TAG, "Webserver auf Port 80 gestartet");
}

int ws_clients() { return s_ws_clients; }

}  // namespace app::web
