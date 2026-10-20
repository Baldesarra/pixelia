// Webserver: Einrichtungsseite, JSON-API, WebSocket, Update-Upload, Captive Portal (siehe web.h).
//
// Seiten: / liefert die Weboberfläche (web/, eingebettet über web_assets.h), auch im Setup-Modus (seit 0.3.1).
//   GET  /setup               einfache Einrichtungsseite (Notfallseite ohne Build-Schritt, immer erreichbar)
//
// API (alle Antworten JSON, Eingaben als Formulardaten application/x-www-form-urlencoded):
//   GET  /api/status          Gesamtstatus (auch per WebSocket /ws jede Sekunde)
//   GET  /api/changelog       Versionsverlauf dieser Firmware (CHANGELOG.json, gzip; seit 0.4.0)
//   GET  /api/wifi            gespeicherte Netze (nur Namen) und letzter Scan
//   POST /api/wifi/scan       neu scannen
//   POST /api/wifi/add        ssid, pass: speichern und verbinden
//   POST /api/wifi/remove     ssid
//   POST /api/settings        rot (0|180), bright (1–100), lp (langer Druck in ms, 1000–10000), ui (Farbwahl, JSON)
//   POST /api/ota             Rohdaten der .bin (application/octet-stream)
//   POST /api/ota/ack         Hinweis auf zurückgerolltes Update gelesen
//   POST /api/reboot
//   POST /api/speedtest       Daten empfangen und verwerfen (Messung der Übertragungsrate)
//
// Schutz (seit 0.3.2, auth.h): GET /api/auth (Stand), POST /api/login (pin → token), /api/logout, /api/pin (new,
// leer = aus), /api/factory-reset (confirm=1), GET /api/backup (Sicherung als Datei, ohne Passwörter und PIN).
// Mit PIN brauchen alle ändernden Endpunkte außer /api/ota/ack, /api/wifi/scan und /api/speedtest eine Anmeldung;
// /api/wifi/add im Setup-Modus nicht. Lesen bleibt offen (wie das Diagnosepaket: ohne Passwörter).
//
// Fernwartung (seit 0.2.2, web_diag.cpp): /api/log, WebSocket /ws/log, /api/cmd, /api/perf, /api/resets,
// /api/coredump, /api/coredump/summary, /api/coredump/erase, /api/diag (Diagnosepaket).
// Info und Geräte (seit 0.3.1, web_info.cpp): /api/system, /api/clients, /api/clients/name.

#include "web.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>

#include "app.h"
#include "auth.h"
#include "diag.h"
#include "esp_app_desc.h"
#include "esp_heap_caps.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "json.h"
#include "logbuf.h"
#include "net.h"
#include "ota.h"
#include "settings.h"
#include "web_assets.h"
#include "web_internal.h"

extern const char kSetupHtmlStart[] asm("_binary_setup_html_start");
extern const char kSetupHtmlEnd[] asm("_binary_setup_html_end");

namespace app::web {

namespace {
constexpr const char* TAG = "web";
httpd_handle_t s_server = nullptr;
std::atomic<int> s_ws_clients{0};
}  // namespace

namespace detail {

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
    j.obj("set").num("rot", set.rotate180 ? 180 : 0).num("bright", set.brightness).num("lp", set.long_press_ms);
    j.str("ui", set.ui).end_obj();
    j.obj("auth").boolean("pin", auth::enabled()).num("lockS", auth::lock_seconds()).end_obj();

    // Fernwartung (0.2.2): Startnummer, letzter Resetgrund, Absturzbericht, Fehler/Warnungen seit dem Start
    const log::Info li = log::info();
    diag::Crash c;
    const bool crashed = diag::crash(&c);
    j.obj("diag").num("boot", diag::boot()).str("reset", sysinfo::reset_reason_text(esp_reset_reason()));
    j.num("errors", li.stats.errors).num("warnings", li.stats.warnings).num("logLines", li.next - li.first);
    if (crashed) {
        j.obj("crash").str("task", c.task).str("reason", c.reason).str("v", c.version).end_obj();
    } else {
        j.null("crash");
    }
    j.end_obj();
    j.end_obj();
    return std::move(j.text());
}

// --- Hilfen -------------------------------------------------------------------------------------------------

httpd_handle_t server() { return s_server; }

esp_err_t send_json(httpd_req_t* req, const std::string& body, const char* status) {
    httpd_resp_set_status(req, status);
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, body.data(), static_cast<ssize_t>(body.size()));
}

esp_err_t reply(httpd_req_t* req, bool ok, const char* msg, const char* status) {
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

bool query_value(httpd_req_t* req, const char* key, std::string* out) {
    const size_t len = httpd_req_get_url_query_len(req);
    if (len == 0 || len > 512) return false;
    std::string q(len + 1, '\0');
    if (httpd_req_get_url_query_str(req, q.data(), q.size()) != ESP_OK) return false;
    q.resize(len);
    return form_value(q, key, out);
}

bool authorized(httpd_req_t* req, const std::string* body) {
    if (!auth::enabled()) return true;
    char hdr[auth::kMaxLen + 2] = "";
    if (httpd_req_get_hdr_value_str(req, "X-Pixelia-Token", hdr, sizeof(hdr)) == ESP_OK && auth::token_valid(hdr)) {
        return true;
    }
    std::string pin;
    if (httpd_req_get_hdr_value_str(req, "X-Pixelia-Pin", hdr, sizeof(hdr)) == ESP_OK) pin = hdr;
    else if (!body || !form_value(*body, "pin", &pin)) return false;
    const bool ok = auth::check(pin.c_str()) == auth::Result::Ok;
    std::fill(pin.begin(), pin.end(), 0);
    std::fill(std::begin(hdr), std::end(hdr), 0);
    return ok;
}

esp_err_t deny(httpd_req_t* req) {
    const int lock = auth::lock_seconds();
    Json j;
    j.obj().boolean("ok", false).boolean("auth", true);
    j.str("msg", lock ? "Zu viele Fehlversuche, bitte warten" : "PIN nötig");
    j.num("lockS", lock).num("left", auth::attempts_left()).end_obj();
    return send_json(req, j.text(), lock ? "429 Too Many Requests" : "401 Unauthorized");
}

void add(const char* uri, httpd_method_t method, esp_err_t (*handler)(httpd_req_t*), bool ws,
         esp_err_t (*ws_open)(httpd_req_t*)) {
    httpd_uri_t u{};
    u.uri = uri;
    u.method = method;
    u.handler = handler;
    u.is_websocket = ws;
    u.ws_post_handshake_cb = ws_open;
    esp_err_t err = httpd_register_uri_handler(s_server, &u);
    if (err != ESP_OK) ESP_LOGE(TAG, "URI %s nicht registriert: %s", uri, esp_err_to_name(err));
}

}  // namespace detail

namespace {
using namespace detail;

bool setup_active() { return wifi::status().ap_active; }

// --- WebSocket ----------------------------------------------------------------------------------------------

// An alle WebSocket-Clients des Status senden (nicht an /ws/log); nur im Kontext des Server-Tasks aufrufen
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
        if (is_log_client(clients[i])) continue;
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

// Neue Verbindung: gleich den aktuellen Stand schicken. Bis 0.2.1 stand das im Handler unter HTTP_GET, den ESP-IDF 6.1
// beim Handschlag aber nicht mehr aufruft: Der Status kam deshalb nie per WebSocket, die Seite lief über ihre
// Ersatz-Abfrage alle 2 s (beim Live-Log in 0.2.2 aufgefallen)
esp_err_t ws_open(httpd_req_t*) {
    s_ws_clients++;
    auto* text = new std::string(status_json());
    if (httpd_queue_work(s_server, broadcast_work, text) != ESP_OK) delete text;
    return ESP_OK;
}

// Nachrichten vom Browser: Anmeldung und Seitenwechsel für „Verbundene Geräte“ (web_info.cpp)
esp_err_t ws_handler(httpd_req_t* req) {
    httpd_ws_frame_t frame{};
    char buf[257];
    if (httpd_ws_recv_frame(req, &frame, 0) != ESP_OK) return ESP_FAIL;
    if (frame.len >= sizeof(buf)) return ESP_OK;
    frame.payload = reinterpret_cast<uint8_t*>(buf);
    if (httpd_ws_recv_frame(req, &frame, frame.len) != ESP_OK) return ESP_FAIL;
    if (frame.type == HTTPD_WS_TYPE_TEXT && frame.len > 0) client_message(req, std::string(buf, frame.len));
    return ESP_OK;
}

// --- Seiten und API -----------------------------------------------------------------------------------------

esp_err_t setup_get(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, kSetupHtmlStart, kSetupHtmlEnd - kSetupHtmlStart - 1);
}

const assets::Asset* find_asset(const char* uri) {
    const size_t len = std::strcspn(uri, "?#");
    for (size_t i = 0; i < assets::kCount; ++i) {
        const assets::Asset& a = assets::kAssets[i];
        if (std::strlen(a.path) == len && std::strncmp(a.path, uri, len) == 0) return &a;
    }
    return nullptr;
}

// Datei der Weboberfläche senden: gzip-komprimiert, mit ETag (Inhalts-Hash). Der Browser fragt jedes Mal nach
// (no-cache) und bekommt nach einem Update die neuen Dateien, sonst nur „304 Not Modified“.
esp_err_t send_asset(httpd_req_t* req, const assets::Asset& a) {
    char tag[24] = "";
    if (httpd_req_get_hdr_value_str(req, "If-None-Match", tag, sizeof(tag)) == ESP_OK && std::strcmp(tag, a.etag) == 0) {
        httpd_resp_set_status(req, "304 Not Modified");
        httpd_resp_set_hdr(req, "ETag", a.etag);
        return httpd_resp_send(req, nullptr, 0);
    }
    httpd_resp_set_type(req, a.mime);
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_set_hdr(req, "ETag", a.etag);
    if (a.gzip) httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    return httpd_resp_send(req, reinterpret_cast<const char*>(a.data), static_cast<ssize_t>(a.size));
}

esp_err_t asset_get(httpd_req_t* req) {
    const assets::Asset* a = find_asset(req->uri);
    if (!a) return httpd_resp_send_404(req);
    return send_asset(req, *a);
}

// Versionsverlauf dieser Firmware (CHANGELOG.json, gzip wie die Oberfläche, ETag = Inhalts-Hash)
esp_err_t changelog_get(httpd_req_t* req) { return send_asset(req, assets::kChangelog); }

// Startseite: die Oberfläche, auch im Setup-Modus (sie startet dann auf der WLAN-Seite, seit 0.3.1).
// Ohne eingebettete Oberfläche die einfache Einrichtungsseite, die außerdem immer unter /setup erreichbar bleibt.
esp_err_t root_get(httpd_req_t* req) {
    const assets::Asset* a = find_asset("/index.html");
    if (!a) return setup_get(req);
    return send_asset(req, *a);
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
    // Im Setup-Modus ohne PIN: Ins Setup-WLAN kommt nur, wer das Passwort auf dem Display sieht (wie bei der
    // Touch-Geste), und so bleibt ein umgezogenes Gerät mit vergessenem PIN ohne Werksreset einrichtbar
    if (!setup_active() && !authorized(req, &body)) {
        std::fill(body.begin(), body.end(), 0);
        return deny(req);
    }
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
    if (!authorized(req, &body)) return deny(req);
    if (wifi::remove_network(ssid.c_str()) != ESP_OK) return reply(req, false, "Nicht gespeichert");
    return reply(req, true, "Entfernt");
}

esp_err_t settings_post(httpd_req_t* req) {
    std::string body, v;
    if (!read_body(req, &body, 512)) return reply(req, false, "Ungültige Anfrage");
    if (!authorized(req, &body)) return deny(req);
    if (form_value(body, "ui", &v) && !settings::set_ui(v)) return reply(req, false, "Farbwahl ungültig");
    if (form_value(body, "rot", &v)) settings::set_rotation180(v == "180");
    if (form_value(body, "bright", &v)) settings::set_brightness(std::atoi(v.c_str()));
    if (form_value(body, "lp", &v)) settings::set_long_press_ms(std::atoi(v.c_str()));
    return reply(req, true, "Gespeichert");
}

// --- Anmeldung, PIN, Werksreset, Sicherung (0.3.2) -----------------------------------------------------------

esp_err_t auth_get(httpd_req_t* req) {
    char tok[auth::kTokenLen + 2] = "";
    const bool has = httpd_req_get_hdr_value_str(req, "X-Pixelia-Token", tok, sizeof(tok)) == ESP_OK;
    Json j;
    j.obj().boolean("pin", auth::enabled()).boolean("ok", !auth::enabled() || (has && auth::token_valid(tok)));
    j.num("lockS", auth::lock_seconds()).num("left", auth::attempts_left()).end_obj();
    return send_json(req, j.text());
}

esp_err_t login_post(httpd_req_t* req) {
    std::string body, pin;
    if (!read_body(req, &body, 256)) return reply(req, false, "Ungültige Anfrage");
    form_value(body, "pin", &pin);
    char token[auth::kTokenLen + 1] = "";
    const auth::Result r = auth::check(pin.c_str(), token);
    std::fill(body.begin(), body.end(), 0);
    std::fill(pin.begin(), pin.end(), 0);
    if (r == auth::Result::Ok) {
        Json j;
        j.obj().boolean("ok", true).str("msg", "Angemeldet").str("token", token).end_obj();
        return send_json(req, j.text());
    }
    const int lock = auth::lock_seconds();
    char msg[96];
    if (lock) std::snprintf(msg, sizeof(msg), "Zu viele Fehlversuche. Nächster Versuch in %d s", lock);
    else if (r == auth::Result::Wrong) {
        const int left = auth::attempts_left();
        std::snprintf(msg, sizeof(msg), "PIN falsch, noch %d Versuch%s", left, left == 1 ? "" : "e");
    } else std::snprintf(msg, sizeof(msg), "Gesperrt");
    Json j;
    j.obj().boolean("ok", false).boolean("auth", true).str("msg", msg).num("lockS", lock);
    j.num("left", auth::attempts_left()).end_obj();
    return send_json(req, j.text(), lock ? "429 Too Many Requests" : "401 Unauthorized");
}

esp_err_t logout_post(httpd_req_t* req) {
    char tok[auth::kTokenLen + 2] = "";
    if (httpd_req_get_hdr_value_str(req, "X-Pixelia-Token", tok, sizeof(tok)) == ESP_OK) auth::logout(tok);
    return reply(req, true, "Abgemeldet");
}

// PIN setzen, ändern (Feld „new“) oder ausschalten (leer). Mit PIN nur angemeldet.
esp_err_t pin_post(httpd_req_t* req) {
    std::string body, pin;
    if (!read_body(req, &body, 256) || !form_value(body, "new", &pin)) return reply(req, false, "Neuer PIN fehlt");
    if (!authorized(req, &body)) {
        std::fill(body.begin(), body.end(), 0);
        std::fill(pin.begin(), pin.end(), 0);
        return deny(req);
    }
    char token[auth::kTokenLen + 1] = "";
    const esp_err_t err = auth::set(pin.c_str(), token);
    const bool on = !pin.empty();
    std::fill(body.begin(), body.end(), 0);
    std::fill(pin.begin(), pin.end(), 0);
    if (err == ESP_ERR_INVALID_SIZE) return reply(req, false, "Der PIN braucht 4 bis 32 Zeichen");
    if (err == ESP_ERR_INVALID_ARG) return reply(req, false, "Der PIN enthält ungültige Zeichen");
    if (err != ESP_OK) return reply(req, false, "PIN nicht gespeichert", "500 Internal Server Error");
    Json j;
    j.obj().boolean("ok", true).str("msg", on ? "PIN gespeichert" : "PIN ausgeschaltet");
    if (on) j.str("token", token);
    j.end_obj();
    return send_json(req, j.text());
}

esp_err_t factory_reset_post(httpd_req_t* req) {
    std::string body, v;
    read_body(req, &body, 256);
    if (!authorized(req, &body)) return deny(req);
    if (!form_value(body, "confirm", &v) || v != "1") return reply(req, false, "Bestätigung fehlt (confirm=1)");
    reply(req, true, "Werksreset, Neustart im Setup-Modus");
    settings::factory_reset("Weboberfläche");
    return ESP_OK;
}

// Sicherung der Einstellungen als Datei. Enthält nie WLAN-Passwörter oder den PIN (auch nicht als Hash): nur die
// Namen der Netze zur Erinnerung und ob ein PIN gesetzt war. Import macht die Oberfläche über /api/settings.
esp_err_t backup_get(httpd_req_t* req) {
    const settings::Values set = settings::get();
    char saved[wifi::kMaxSaved][33];
    const int ns = wifi::saved_networks(saved, wifi::kMaxSaved);
    char date[16] = "";
    if (timesync::valid()) {
        time_t now = time(nullptr);
        tm lt;
        localtime_r(&now, &lt);
        strftime(date, sizeof(date), "%Y-%m-%d", &lt);
    }
    Json j;
    j.obj().str("pixelia", "settings").num("format", 1).str("v", esp_app_get_description()->version);
    j.str("date", date).str("host", wifi::kMdnsHost);
    j.obj("set").num("rot", set.rotate180 ? 180 : 0).num("bright", set.brightness).num("lp", set.long_press_ms);
    j.str("ui", set.ui).end_obj();
    client_names_json(j);
    j.arr("wifi");
    for (int i = 0; i < ns; ++i) j.str(nullptr, saved[i]);
    j.end_arr();
    j.boolean("pin", auth::enabled()).end_obj();
    char disp[80];
    std::snprintf(disp, sizeof(disp), "attachment; filename=\"pixelia-einstellungen%s%s.json\"", date[0] ? "-" : "",
                  date);
    httpd_resp_set_hdr(req, "Content-Disposition", disp);
    return send_json(req, j.text());
}

esp_err_t reboot_post(httpd_req_t* req) {
    std::string body;
    read_body(req, &body, 256);
    if (!authorized(req, &body)) return deny(req);
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
    if (!authorized(req)) {
        // Verbindung danach schließen statt bis zu 6 MB abzuwarten (die Oberfläche prüft die Anmeldung vorher)
        deny(req);
        return ESP_FAIL;
    }
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
}  // namespace

void start() {
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.core_id = kSystemCore;
    cfg.stack_size = 8192;
    cfg.task_priority = 5;
    cfg.max_uri_handlers = 56;  // 0.4.0: 45 belegt
    cfg.max_open_sockets = 7;
    cfg.lru_purge_enable = true;
    cfg.recv_wait_timeout = 10;
    cfg.send_wait_timeout = 10;
    // Captive-Portal-Abfragen kommen in Schüben, Fehlermeldungen dazu sind nur Lärm im Log
    esp_log_level_set("httpd_uri", ESP_LOG_ERROR);
    esp_log_level_set("httpd_txrx", ESP_LOG_ERROR);
    esp_log_level_set("httpd_parse", ESP_LOG_ERROR);
    // Browser, die einschlafen oder hart schließen, kappen den WebSocket ohne Abschied: harmlos, sonst Warnung pro Fall
    esp_log_level_set("httpd_ws", ESP_LOG_ERROR);
    esp_err_t err = httpd_start(&s_server, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Webserver startet nicht: %s", esp_err_to_name(err));
        return;
    }
    add("/", HTTP_GET, root_get);
    add("/setup", HTTP_GET, setup_get);
    for (size_t i = 0; i < assets::kCount; ++i) add(assets::kAssets[i].path, HTTP_GET, asset_get);
    add("/api/status", HTTP_GET, status_get);
    add("/api/changelog", HTTP_GET, changelog_get);
    add("/api/wifi", HTTP_GET, wifi_get);
    add("/api/wifi/scan", HTTP_POST, wifi_scan_post);
    add("/api/wifi/add", HTTP_POST, wifi_add_post);
    add("/api/wifi/remove", HTTP_POST, wifi_remove_post);
    add("/api/settings", HTTP_POST, settings_post);
    add("/api/ota", HTTP_POST, ota_post);
    add("/api/ota/ack", HTTP_POST, ota_ack_post);
    add("/api/reboot", HTTP_POST, reboot_post);
    add("/api/auth", HTTP_GET, auth_get);
    add("/api/login", HTTP_POST, login_post);
    add("/api/logout", HTTP_POST, logout_post);
    add("/api/pin", HTTP_POST, pin_post);
    add("/api/factory-reset", HTTP_POST, factory_reset_post);
    add("/api/backup", HTTP_GET, backup_get);
    add("/api/speedtest", HTTP_POST, speedtest_post);
    add("/ws", HTTP_GET, ws_handler, true, ws_open);
    register_diag();
    register_info();
    for (const char* uri : {"/generate_204", "/gen_204", "/hotspot-detect.html", "/library/test/success.html",
                            "/connecttest.txt", "/ncsi.txt", "/redirect", "/canonical.html", "/success.txt"}) {
        add(uri, HTTP_GET, captive_check);
    }
    httpd_register_err_handler(s_server, HTTPD_404_NOT_FOUND, not_found);
    xTaskCreatePinnedToCore(status_task, "webstatus", 4096, nullptr, 2, nullptr, kSystemCore);
    ESP_LOGI(TAG, "Webserver auf Port 80 gestartet, Oberfläche %u Dateien, %u KB", static_cast<unsigned>(assets::kCount),
             static_cast<unsigned>(assets::kBytes / 1024));
}

int ws_clients() { return s_ws_clients; }

}  // namespace app::web
