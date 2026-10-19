// Fernwartung über WLAN (0.2.2): Log, Live-Log, Befehle, Leistung, Reset-Historie, Absturzbericht, Diagnosepaket.
// Alle Ausgaben mit freiem Text laufen durch den Passwort-Filter (pixelia::SecretFilter): gespeicherte
// WLAN-Passwörter und das Passwort des Setup-WLANs verlassen das Gerät nie, auch nicht über das Log.
//
//   GET  /api/log              Zeilen aus dem Log-Puffer (JSON). Parameter: since=<nr> (nur Zeilen danach),
//                              tail=<n> (die letzten n), boot=<nr> (nur dieser Start), rid=<kennung> (passt sie nicht
//                              mehr, wurde der Puffer neu angelegt: Antwort ab der ältesten Zeile mit reset=true),
//                              max=<Byte> (Text pro Antwort, Standard 48 KB), format=text (Klartext statt JSON)
//   WS   /ws/log               Live-Log: zuerst die letzten 50 Zeilen (tail=<n> bzw. since=<nr>), dann jede neue
//   POST /api/cmd              cmd=<Befehl>[&confirm=1]: wie auf der seriellen Konsole, Ausgabe als Antwort.
//                              Gefährliche Befehle ohne confirm → 409 mit Grund
//   GET  /api/perf             Leistungsmonitor, Speicher, Kosten des Log-Hooks; mit tasks=1 auch die Tasks mit Stack
//                              und CPU-Last (hält die Kernel-Sperre, etwa ein Display-Aussetzer pro Abfrage)
//   GET  /api/resets           Reset-Historie (letzte 16 Starts)
//   GET  /api/coredump         Rohdaten des Absturzberichts (für esp-coredump bzw. tools/crash.ps1)
//   GET  /api/coredump/summary Panic-Grund, Task, PC, Backtrace-Adressen, passende Version
//   POST /api/coredump/erase   Absturzbericht löschen
//   GET  /api/diag             Diagnosepaket: eine JSON-Datei mit allem (Knopf auf der Einrichtungsseite)

#include <algorithm>
#include <atomic>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <memory>
#include <string>
#include <vector>

#include "app.h"
#include "diag.h"
#include "esp_heap_caps.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "json.h"
#include "logbuf.h"
#include "net.h"
#include "pixelia/engine/redact.h"
#include "settings.h"
#include "web_internal.h"

namespace app::web::detail {

namespace {
constexpr const char* TAG = "web";
constexpr uint32_t kLogClientMagic = 0x43474F4C;  // „LOGC“
constexpr size_t kDefaultMax = 48 * 1024;
constexpr size_t kWsFrameMax = 12 * 1024;
constexpr uint32_t kWsTail = 50;
constexpr size_t kCmdOut = 16 * 1024;
constexpr int kCmdTimeoutMs = 20000;
// Große Zwischenpuffer (Rahmen, gestreamte Antworten) bewusst ≥ 16 KB: malloc legt sie dann in den PSRAM
// (CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL = 16384), sonst sank der interne RAM im Test auf 99 KB
constexpr size_t kBigAlloc = 16 * 1024 + 512;
constexpr size_t kStreamBlock = 16 * 1024;

struct LogClient {
    uint32_t magic = kLogClientMagic;
    uint32_t next = 0;  // nächste zu sendende Nummer
    uint32_t rid = 0;
};
std::atomic<int> s_log_clients{0};
std::atomic<bool> s_push_queued{false};
std::atomic<uint32_t> s_pushed_next{0};

// --- Passwort-Filter ----------------------------------------------------------------------------------------

void add_secrets(pixelia::SecretFilter& f) {
    wifi::for_each_secret([](const char* s, void* ctx) { static_cast<pixelia::SecretFilter*>(ctx)->add_secret(s); }, &f);
}

// Muster für einmaliges Filtern ganzer Puffer (WebSocket-Rahmen, Befehlsausgaben)
struct Patterns {
    std::vector<std::string> list;
    Patterns() {
        wifi::for_each_secret(
            [](const char* s, void* ctx) {
                if (std::strlen(s) >= pixelia::SecretFilter::kMinSecret)
                    pixelia::SecretFilter::variants(s, static_cast<std::vector<std::string>*>(ctx));
            },
            &list);
        pixelia::SecretFilter::sort_longest_first(&list);
    }
    ~Patterns() {
        for (auto& p : list) std::fill(p.begin(), p.end(), 0);
    }
    size_t apply(std::string& s) const { return pixelia::SecretFilter::redact(s.data(), s.size(), list); }
};

pixelia::SecretFilter::Sink chunk_sink(httpd_req_t* req) {
    return [req](const char* d, size_t n) { return httpd_resp_send_chunk(req, d, static_cast<ssize_t>(n)) == ESP_OK; };
}

// --- Log-Zeilen ---------------------------------------------------------------------------------------------

void line_json(std::string& out, const log::Line& l) {
    char head[112];
    std::snprintf(head, sizeof(head), "{\"n\":%" PRIu32 ",\"b\":%u,\"t\":%" PRIu32 ",\"u\":%" PRIu32 ",\"l\":\"%c\"%s,\"m\":\"",
                  l.seq, l.boot, l.epoch, l.up_ms, l.level, l.truncated ? ",\"cut\":true" : "");
    out += head;
    Json::escape(out, l.text, l.len);
    out += "\"}";
}

struct ReadCtx {
    pixelia::SecretFilter* filter = nullptr;  // gestreamt (HTTP) …
    std::string* out = nullptr;               // … oder gesammelt (WebSocket)
    bool text = false;
    bool first = true;
    int boot = -1;  // nur dieser Start, -1 = alle
    uint32_t last = 0;
    uint32_t count = 0;
    std::string buf;
};

bool read_cb(const log::Line& l, void* p) {
    auto* c = static_cast<ReadCtx*>(p);
    c->last = l.seq;
    if (c->boot >= 0 && l.boot != c->boot) return true;
    c->buf.clear();
    if (c->text) {
        char line[log::kMaxText + 64];
        int n = std::snprintf(line, sizeof(line), "%" PRIu32 " ", l.seq);
        log::format(l, line + n, sizeof(line) - n);
        c->buf += line;
        c->buf += '\n';
    } else {
        if (!c->first) c->buf += ',';
        line_json(c->buf, l);
    }
    c->first = false;
    ++c->count;
    if (c->filter) return c->filter->write(c->buf);
    c->out->append(c->buf);
    return true;
}

uint32_t parse_u32(const std::string& v) { return static_cast<uint32_t>(std::strtoul(v.c_str(), nullptr, 10)); }

// GET /api/log
esp_err_t log_get(httpd_req_t* req) {
    const log::Info info = log::info();
    std::string v;
    uint32_t from = info.first;
    bool since_given = false, reset = false;
    if (query_value(req, "since", &v)) {
        from = parse_u32(v) + 1;
        since_given = true;
    }
    if (query_value(req, "tail", &v)) {
        const uint32_t n = parse_u32(v);
        from = info.next > n ? info.next - n : 0;
    }
    if (query_value(req, "rid", &v) && parse_u32(v) != info.rid) {
        reset = true;
        from = info.first;
    }
    const bool lost = since_given && !reset && from < info.first;
    if (from < info.first) from = info.first;
    ReadCtx ctx;
    if (query_value(req, "boot", &v)) ctx.boot = std::atoi(v.c_str());
    size_t max = kDefaultMax;
    if (query_value(req, "max", &v)) max = std::clamp<size_t>(parse_u32(v), 1024, log::kRingBytes);
    ctx.text = query_value(req, "format", &v) && v == "text";
    ctx.last = from ? from - 1 : 0;

    httpd_resp_set_type(req, ctx.text ? "text/plain; charset=utf-8" : "application/json; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    pixelia::SecretFilter filter(chunk_sink(req), kStreamBlock);
    add_secrets(filter);
    ctx.filter = &filter;
    if (!ctx.text) {
        char head[192];
        std::snprintf(head, sizeof(head),
                      "{\"rid\":%" PRIu32 ",\"boot\":%u,\"first\":%" PRIu32 ",\"next\":%" PRIu32 ",\"from\":%" PRIu32
                      ",\"reset\":%s,\"lost\":%s,\"lines\":[",
                      info.rid, info.boot, info.first, info.next, from, reset ? "true" : "false", lost ? "true" : "false");
        filter.write(head, std::strlen(head));
    }
    const uint32_t next = log::read(from, max, read_cb, &ctx);
    if (!ctx.text) {
        char tail[96];
        std::snprintf(tail, sizeof(tail), "],\"count\":%" PRIu32 ",\"last\":%" PRIu32 ",\"more\":%s}", ctx.count, ctx.last,
                      next < log::next_seq() ? "true" : "false");
        filter.write(tail, std::strlen(tail));
    }
    if (!filter.finish()) return ESP_FAIL;
    return httpd_resp_send_chunk(req, nullptr, 0);
}

// --- Live-Log per WebSocket ---------------------------------------------------------------------------------

void push_work(void*) {
    s_push_queued = false;
    size_t fds = 8;
    int clients[8];
    if (httpd_get_client_list(server(), &fds, clients) != ESP_OK) return;
    const log::Info info = log::info();
    Patterns pats;
    int n = 0;
    bool behind = false;
    for (size_t i = 0; i < fds; ++i) {
        if (httpd_ws_get_fd_info(server(), clients[i]) != HTTPD_WS_CLIENT_WEBSOCKET) continue;
        auto* c = static_cast<LogClient*>(httpd_sess_get_ctx(server(), clients[i]));
        if (!c || c->magic != kLogClientMagic) continue;
        ++n;
        if (c->rid != info.rid) {  // Puffer neu angelegt: von vorn
            c->rid = info.rid;
            c->next = info.first;
        }
        if (c->next >= info.next) continue;
        std::string frame;
        frame.reserve(kBigAlloc);  // ab 16 KB legt ESP-IDF Speicher in den PSRAM statt in den internen RAM
        char head[64];
        std::snprintf(head, sizeof(head), "{\"rid\":%" PRIu32 ",\"lines\":[", info.rid);
        frame += head;
        ReadCtx ctx;
        ctx.out = &frame;
        const uint32_t next = log::read(c->next, kWsFrameMax, read_cb, &ctx);
        std::snprintf(head, sizeof(head), "],\"next\":%" PRIu32 "}", next);
        frame += head;
        pats.apply(frame);
        httpd_ws_frame_t f{};
        f.type = HTTPD_WS_TYPE_TEXT;
        f.payload = reinterpret_cast<uint8_t*>(frame.data());
        f.len = frame.size();
        if (httpd_ws_send_frame_async(server(), clients[i], &f) == ESP_OK) c->next = next;
        if (c->next < info.next) behind = true;
    }
    (void)n;  // gezählt wird beim Öffnen und Schließen der Sitzung (free_ctx)
    s_pushed_next = behind ? 0 : info.next;
}

void queue_push() {
    if (s_push_queued.exchange(true)) return;
    if (httpd_queue_work(server(), push_work, nullptr) != ESP_OK) s_push_queued = false;
}

// Schaut fünfmal pro Sekunde nach neuen Zeilen; der Hook selbst sendet nie (er darf nicht blockieren)
void logpush_task(void*) {
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(200));
        if (s_log_clients > 0 && log::next_seq() != s_pushed_next) queue_push();
    }
}

// Nach dem Handschlag: Sitzung als Log-Client markieren (Startpunkt aus tail bzw. since), Rückstand gleich senden
esp_err_t ws_log_open(httpd_req_t* req) {
    const log::Info info = log::info();
    auto* c = new LogClient;
    c->rid = info.rid;
    std::string v;
    uint32_t tail = kWsTail;
    if (query_value(req, "tail", &v)) tail = parse_u32(v);
    c->next = info.next > tail ? info.next - tail : 0;
    if (query_value(req, "since", &v)) c->next = parse_u32(v) + 1;
    if (c->next < info.first) c->next = info.first;
    req->sess_ctx = c;
    req->free_ctx = [](void* p) {
        delete static_cast<LogClient*>(p);
        s_log_clients--;
    };
    s_log_clients++;
    s_pushed_next = 0;
    queue_push();
    return ESP_OK;
}

esp_err_t ws_log_handler(httpd_req_t* req) {
    // Nachrichten vom Client werden nicht gebraucht: lesen und verwerfen
    httpd_ws_frame_t frame{};
    uint8_t buf[64];
    if (httpd_ws_recv_frame(req, &frame, 0) != ESP_OK) return ESP_FAIL;
    if (frame.len >= sizeof(buf)) return ESP_OK;
    frame.payload = buf;
    return httpd_ws_recv_frame(req, &frame, frame.len);
}

// --- Befehle ------------------------------------------------------------------------------------------------

esp_err_t cmd_post(httpd_req_t* req) {
    std::string body, cmd, v;
    if (!read_body(req, &body, 600) || !form_value(body, "cmd", &cmd)) return reply(req, false, "Befehl fehlt (cmd=…)");
    if (!authorized(req, &body)) return reply(req, false, "PIN fehlt oder falsch", "401 Unauthorized");
    // Steuerzeichen raus, Leerzeichen am Rand weg
    cmd.erase(std::remove_if(cmd.begin(), cmd.end(), [](char c) { return static_cast<unsigned char>(c) < 32; }), cmd.end());
    while (!cmd.empty() && cmd.back() == ' ') cmd.pop_back();
    while (!cmd.empty() && cmd.front() == ' ') cmd.erase(0, 1);
    if (cmd.empty() || cmd.size() > 200) return reply(req, false, "Befehl leer oder zu lang (höchstens 200 Zeichen)");
    const bool confirmed = form_value(body, "confirm", &v) && (v == "1" || v == "ja" || v == "true" || v == "yes");

    std::unique_ptr<char, decltype(&heap_caps_free)> out(static_cast<char*>(heap_caps_malloc(kCmdOut, MALLOC_CAP_SPIRAM)),
                                                         heap_caps_free);
    if (!out) return reply(req, false, "Kein Speicher frei", "500 Internal Server Error");
    char why[160];
    const int64_t t0 = esp_timer_get_time();
    const console::Result r = console::run_remote(cmd.c_str(), confirmed, out.get(), kCmdOut, why, sizeof(why), kCmdTimeoutMs);
    const double ms = (esp_timer_get_time() - t0) / 1000.0;

    Json j;
    j.text().reserve(kBigAlloc);
    j.obj();
    const char* status = "200 OK";
    switch (r) {
        case console::Result::Done:
            j.boolean("ok", true);
            break;
        case console::Result::Unknown:
            j.boolean("ok", false).str("msg", "Unbekannter Befehl oder falsche Angaben (help listet alle Befehle)");
            status = "400 Bad Request";
            break;
        case console::Result::NeedsConfirm: {
            char msg[240];
            std::snprintf(msg, sizeof(msg), "Der Befehl %s. Zum Ausführen mit confirm=1 wiederholen.", why);
            j.boolean("ok", false).boolean("needsConfirm", true).str("msg", msg);
            status = "409 Conflict";
            break;
        }
        case console::Result::Busy:
            j.boolean("ok", false).str("msg", "Es läuft noch ein anderer Befehl, bitte gleich noch einmal");
            status = "503 Service Unavailable";
            break;
        case console::Result::Timeout:
            j.boolean("ok", false).str("msg", "Befehl läuft noch (nach 20 s keine Antwort), Ausgabe im Log");
            status = "504 Gateway Timeout";
            break;
    }
    j.str("cmd", cmd.c_str()).str("out", out.get()).num("ms", ms, 0).end_obj();
    std::string& resp = j.text();
    Patterns().apply(resp);
    return send_json(req, resp, status);
}

// --- Leistung, Resets, Absturzbericht ----------------------------------------------------------------------

// Task-Liste nur mit ?tasks=1 (kostet etwa einen Display-Aussetzer, siehe diag.cpp)
esp_err_t perf_get(httpd_req_t* req) {
    std::string v;
    return send_json(req, diag::perf_json(query_value(req, "tasks", &v) && v == "1"));
}
esp_err_t resets_get(httpd_req_t* req) { return send_json(req, diag::resets_json()); }
esp_err_t coredump_summary_get(httpd_req_t* req) { return send_json(req, diag::crash_json()); }

// Rohdaten in Stücken an fn (für den Download und das Diagnosepaket)
template <typename Fn>
bool read_coredump(const diag::Crash& c, size_t chunk, Fn&& fn) {
    const esp_partition_t* p =
        esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_COREDUMP, nullptr);
    if (!p || c.size > p->size) return false;
    std::unique_ptr<uint8_t, decltype(&heap_caps_free)> buf(static_cast<uint8_t*>(heap_caps_malloc(chunk, MALLOC_CAP_DEFAULT)),
                                                            heap_caps_free);
    if (!buf) return false;
    for (size_t off = 0; off < c.size; off += chunk) {
        const size_t n = std::min<size_t>(chunk, c.size - off);
        if (esp_partition_read(p, off, buf.get(), n) != ESP_OK) return false;
        if (!fn(buf.get(), n)) return false;
    }
    return true;
}

esp_err_t coredump_get(httpd_req_t* req) {
    diag::Crash c;
    if (!diag::crash(&c)) return reply(req, false, "Kein Absturzbericht vorhanden", "404 Not Found");
    char disp[96];
    std::snprintf(disp, sizeof(disp), "attachment; filename=\"pixelia-coredump-%s-start%" PRIu32 ".bin\"",
                  c.sha[0] ? c.sha : "unbekannt", diag::boot());
    httpd_resp_set_type(req, "application/octet-stream");
    httpd_resp_set_hdr(req, "Content-Disposition", disp);
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    const bool ok = read_coredump(c, 4096, [&](const uint8_t* d, size_t n) {
        return httpd_resp_send_chunk(req, reinterpret_cast<const char*>(d), static_cast<ssize_t>(n)) == ESP_OK;
    });
    if (!ok) return ESP_FAIL;
    return httpd_resp_send_chunk(req, nullptr, 0);
}

esp_err_t coredump_erase_post(httpd_req_t* req) {
    std::string body;
    read_body(req, &body, 256);
    if (!authorized(req, &body)) return reply(req, false, "PIN fehlt oder falsch", "401 Unauthorized");
    diag::Crash c;
    if (!diag::crash(&c)) return reply(req, true, "Es war kein Absturzbericht vorhanden");
    const esp_err_t err = diag::crash_erase();
    return reply(req, err == ESP_OK, err == ESP_OK ? "Absturzbericht gelöscht" : esp_err_to_name(err));
}

// --- Diagnosepaket ------------------------------------------------------------------------------------------

void base64(const uint8_t* d, size_t n, std::string& out) {
    static constexpr char kAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    for (size_t i = 0; i < n; i += 3) {
        const uint32_t v = (d[i] << 16) | (i + 1 < n ? d[i + 1] << 8 : 0) | (i + 2 < n ? d[i + 2] : 0);
        out += kAlphabet[(v >> 18) & 63];
        out += kAlphabet[(v >> 12) & 63];
        out += i + 1 < n ? kAlphabet[(v >> 6) & 63] : '=';
        out += i + 2 < n ? kAlphabet[v & 63] : '=';
    }
}

esp_err_t diag_get(httpd_req_t* req) {
    const int64_t t0 = esp_timer_get_time();
    char stamp[32] = "", created[32] = "";
    const time_t now = time(nullptr);
    if (now > 1735689600) {
        tm lt;
        localtime_r(&now, &lt);
        strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M", &lt);
        strftime(created, sizeof(created), "%Y-%m-%d %H:%M:%S", &lt);
    } else {
        std::snprintf(stamp, sizeof(stamp), "ohne-uhrzeit");
    }
    char disp[112];
    std::snprintf(disp, sizeof(disp), "attachment; filename=\"pixelia-diag-%s-start%" PRIu32 ".json\"", stamp, diag::boot());
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    httpd_resp_set_hdr(req, "Content-Disposition", disp);
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");

    pixelia::SecretFilter f(chunk_sink(req), kStreamBlock);
    add_secrets(f);
    std::string s;
    {
        Json j;
        j.text().reserve(kBigAlloc);
        j.obj().num("pixeliaDiag", 1).str("created", created).num("epoch", now > 1735689600 ? now : 0);
        j.str("host", wifi::kMdnsHost);
        j.str("hinweis", "Diagnosepaket von Pixelia: Status, Versionen, Einstellungen, Reset-Historie, Leistung, "
                         "Log-Puffer und Absturzbericht. Enthält keine Passwörter (siehe secretCheck am Ende).");
        j.raw("status", status_json());
        j.raw("system", diag::system_json());
        const settings::Values set = settings::get();
        char names[wifi::kMaxSaved][33];
        const int ns = wifi::saved_networks(names, wifi::kMaxSaved);
        j.obj("settings").num("rot", set.rotate180 ? 180 : 0).num("bright", set.brightness).num("lp", set.long_press_ms);
        j.arr("wifiSaved");
        for (int i = 0; i < ns; ++i) j.str(nullptr, names[i]);
        j.end_arr().boolean("pinSet", false).end_obj();
        j.raw("perf", diag::perf_json(true));
        j.raw("resets", diag::resets_json());
        s = std::move(j.text());  // Objekt bleibt offen, Rest folgt gestreamt
    }
    bool ok = f.write(s);

    // Absturzbericht: Zusammenfassung und Rohdaten (Base64, für tools/crash.ps1 -File)
    diag::Crash c;
    const bool crashed = diag::crash(&c);
    s = ",\"coredump\":{\"summary\":" + diag::crash_json() + ",\"raw\":";
    ok = ok && f.write(s);
    if (crashed) {
        ok = ok && f.write("\"", 1);
        std::string b64;
        ok = ok && read_coredump(c, 3 * 1024, [&](const uint8_t* d, size_t n) {
                 b64.clear();
                 base64(d, n, b64);
                 return f.write(b64);
             });
        ok = ok && f.write("\"", 1);
    } else {
        ok = ok && f.write("null", 4);
    }

    // Log-Puffer komplett
    s = "},\"log\":{\"info\":" + diag::log_info_json() + ",\"lines\":[";
    ok = ok && f.write(s);
    ReadCtx ctx;
    ctx.filter = &f;
    if (ok) log::read(0, log::kRingBytes, read_cb, &ctx);
    ok = ok && f.write("]}", 2);
    ok = ok && f.finish();

    // Prüfergebnis ganz am Ende (enthält nur Zahlen, läuft nicht mehr durch den Filter)
    char tail[160];
    std::snprintf(tail, sizeof(tail),
                  ",\"secretCheck\":{\"secrets\":%u,\"redacted\":%u},\"logLines\":%" PRIu32 ",\"ms\":%.0f}",
                  static_cast<unsigned>(f.secrets()), static_cast<unsigned>(f.hits()), ctx.count,
                  (esp_timer_get_time() - t0) / 1000.0);
    ok = ok && httpd_resp_send_chunk(req, tail, static_cast<ssize_t>(std::strlen(tail))) == ESP_OK;
    if (!ok) {
        ESP_LOGW(TAG, "Diagnosepaket abgebrochen");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Diagnosepaket erstellt: %" PRIu32 " Log-Zeilen, %u Geheimnisse geprüft, %u ersetzt, %.0f ms", ctx.count,
             static_cast<unsigned>(f.secrets()), static_cast<unsigned>(f.hits()), (esp_timer_get_time() - t0) / 1000.0);
    return httpd_resp_send_chunk(req, nullptr, 0);
}

}  // namespace

bool is_log_client(int fd) {
    auto* c = static_cast<LogClient*>(httpd_sess_get_ctx(server(), fd));
    return c && c->magic == kLogClientMagic;
}

void register_diag() {
    add("/api/log", HTTP_GET, log_get);
    add("/ws/log", HTTP_GET, ws_log_handler, true, ws_log_open);
    add("/api/cmd", HTTP_POST, cmd_post);
    add("/api/perf", HTTP_GET, perf_get);
    add("/api/resets", HTTP_GET, resets_get);
    add("/api/coredump", HTTP_GET, coredump_get);
    add("/api/coredump/summary", HTTP_GET, coredump_summary_get);
    add("/api/coredump/erase", HTTP_POST, coredump_erase_post);
    add("/api/diag", HTTP_GET, diag_get);
    xTaskCreatePinnedToCore(logpush_task, "logpush", 3072, nullptr, 2, nullptr, kSystemCore);
}

}  // namespace app::web::detail
