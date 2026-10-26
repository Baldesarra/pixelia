// Update von GitHub: Prüfung über die Releases-API, Installation per HTTPS (siehe github.h).

#include "github.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <mutex>
#include <string>
#include <vector>

#include "app.h"
#include "esp_app_desc.h"
#include "esp_crt_bundle.h"
#include "esp_heap_caps.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "json.h"
#include "net.h"
#include "nvs.h"
#include "ota.h"

namespace app::github {

namespace {
constexpr const char* TAG = "github";
constexpr const char* kNvs = "pixelia";
constexpr const char* kNvsKey = "upd_ch";
constexpr int kPerPage = 10;                 // reicht für einige Vorschauen zwischen zwei fertigen Versionen
constexpr size_t kMaxResponse = 384 * 1024;  // Antwort der API (PSRAM); gemessen ~8 KB pro Release
constexpr size_t kMaxNotes = 12 * 1024;      // Release-Notes pro Version für die Vorschau
constexpr size_t kChunk = 4096;              // Download in Stücken wie beim Upload
constexpr uint32_t kAutoEvery = 24 * 3600;   // Prüfung im Hintergrund höchstens einmal täglich
constexpr uint64_t kAutoTimerUs = 5ULL * 60 * 1000000;  // so oft schaut der Zeitgeber nach, ob es so weit ist

// --- Versionen ----------------------------------------------------------------------------------------------

struct Ver {
    int major = 0, minor = 0, patch = 0;
    bool pre = false;
    int beta = 0;
};

// „0.4.1“, „0.4.1-beta.2“; mit dev=true auch Entwicklungsstände aus git describe („0.4.0-5-gabc1234-dirty“ → 0.4.0)
bool parse_version(const char* s, Ver* v, bool dev = false) {
    int n = 0;
    if (std::sscanf(s, "%d.%d.%d%n", &v->major, &v->minor, &v->patch, &n) != 3) return false;
    s += n;
    v->pre = false;
    v->beta = 0;
    if (std::strncmp(s, "-beta.", 6) == 0) {
        char* end = nullptr;
        v->beta = static_cast<int>(std::strtol(s + 6, &end, 10));
        if (end == s + 6) return false;
        v->pre = true;
        s = end;
    }
    return *s == 0 || (dev && *s == '-');
}

// < 0: a älter als b (wie version_key in tools/release_notes.py)
int compare(const Ver& a, const Ver& b) {
    if (a.major != b.major) return a.major - b.major;
    if (a.minor != b.minor) return a.minor - b.minor;
    if (a.patch != b.patch) return a.patch - b.patch;
    if (a.pre != b.pre) return a.pre ? -1 : 1;
    return a.beta - b.beta;
}

// --- JSON lesen (nur was die Releases-API liefert) ----------------------------------------------------------

class Reader {
public:
    Reader(const char* p, size_t n) : p_(p), end_(p + n) {}

    bool ok() const { return ok_; }
    bool eat(char c) {
        ws();
        if (p_ < end_ && *p_ == c) {
            ++p_;
            return true;
        }
        return false;
    }
    bool expect(char c) { return eat(c) || fail(); }

    // Text lesen (out = nullptr: überspringen); Escapes inkl. \uXXXX → UTF-8
    bool string(std::string* out) {
        if (!expect('"')) return false;
        if (out) out->clear();
        while (p_ < end_) {
            char c = *p_++;
            if (c == '"') return true;
            if (c != '\\') {
                if (out) out->push_back(c);
                continue;
            }
            if (p_ >= end_) break;
            char e = *p_++;
            switch (e) {
                case 'n': c = '\n'; break;
                case 'r': c = '\r'; break;
                case 't': c = '\t'; break;
                case 'b': c = '\b'; break;
                case 'f': c = '\f'; break;
                case 'u': {
                    unsigned cp = hex4();
                    if (cp >= 0xD800 && cp < 0xDC00 && end_ - p_ >= 6 && p_[0] == '\\' && p_[1] == 'u') {
                        p_ += 2;
                        unsigned lo = hex4();
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                    }
                    if (out) utf8(out, cp);
                    continue;
                }
                default: c = e; break;  // \" \\ \/
            }
            if (out) out->push_back(c);
        }
        return fail();
    }
    bool boolean(bool* out) {
        ws();
        if (end_ - p_ >= 4 && std::strncmp(p_, "true", 4) == 0) {
            p_ += 4;
            *out = true;
            return true;
        }
        if (end_ - p_ >= 5 && std::strncmp(p_, "false", 5) == 0) {
            p_ += 5;
            *out = false;
            return true;
        }
        return skip();  // null o. Ä.
    }
    bool number(double* out) {
        ws();
        char tmp[32];
        size_t n = 0;
        while (p_ < end_ && n + 1 < sizeof(tmp) && std::strchr("+-0123456789.eE", *p_)) tmp[n++] = *p_++;
        tmp[n] = 0;
        if (!n) return skip();
        *out = std::strtod(tmp, nullptr);
        return true;
    }
    // beliebigen Wert überspringen
    bool skip(int depth = 0) {
        if (depth > 24) return fail();
        ws();
        if (p_ >= end_) return fail();
        char c = *p_;
        if (c == '"') return string(nullptr);
        if (c == '{' || c == '[') {
            const char close = c == '{' ? '}' : ']';
            ++p_;
            if (eat(close)) return true;
            do {
                if (c == '{' && (!string(nullptr) || !expect(':'))) return false;
                if (!skip(depth + 1)) return false;
            } while (eat(','));
            return expect(close);
        }
        while (p_ < end_ && !std::strchr(",}] \t\r\n", *p_)) ++p_;  // Zahl, true, false, null
        return true;
    }

private:
    void ws() {
        while (p_ < end_ && (*p_ == ' ' || *p_ == '\n' || *p_ == '\r' || *p_ == '\t')) ++p_;
    }
    bool fail() {
        ok_ = false;
        p_ = end_;
        return false;
    }
    unsigned hex4() {
        unsigned v = 0;
        for (int i = 0; i < 4 && p_ < end_; ++i, ++p_) {
            char h = *p_;
            v = v * 16 + (h >= '0' && h <= '9' ? h - '0' : (h | 0x20) >= 'a' && (h | 0x20) <= 'f' ? (h | 0x20) - 'a' + 10 : 0);
        }
        return v;
    }
    static void utf8(std::string* out, unsigned cp) {
        if (cp < 0x80) {
            out->push_back(static_cast<char>(cp));
        } else if (cp < 0x800) {
            out->push_back(static_cast<char>(0xC0 | (cp >> 6)));
            out->push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            out->push_back(static_cast<char>(0xE0 | (cp >> 12)));
            out->push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out->push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            out->push_back(static_cast<char>(0xF0 | (cp >> 18)));
            out->push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            out->push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out->push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }

    const char* p_;
    const char* end_;
    bool ok_ = true;
};

// --- Zustand ------------------------------------------------------------------------------------------------

enum class Job : unsigned char { None, Check, Install };

struct Target {
    char v[32] = "";
    char date[11] = "";
    Ver ver;
    bool pre = false;
    uint32_t size = 0;
    std::string url;
};

std::mutex s_lock;
Channel s_channel = Channel::Stable;
std::atomic<Job> s_job{Job::None};
char s_error[192] = "";
// letzte Prüfung
int64_t s_checked_us = -1;
Channel s_checked_channel = Channel::Stable;
char s_latest[32] = "";        // neueste Version im Kanal (auch wenn nicht neuer als installiert)
std::string s_releases = "[]";  // neuere Versionen als JSON-Array (bei Bedarf im PSRAM)
Target s_target;                // neueste Version im Kanal, wenn neuer als installiert
// Ältere Versionen ab 0.4.0 (Seite Experimentell, „Ältere Version installieren“), ohne Release-Notes
std::vector<Target> s_older;
std::string s_older_json = "[]";
// Abbrechen (Knopf in der Oberfläche) und Ergebnis
std::atomic<bool> s_cancel{false};
bool s_cancelled = false;
std::atomic<bool> s_test_break{false};  // seit 0.5.7: nächster Download trennt einmal nach 300 KB (Befehl „update abriss“)
// Prüfung im Hintergrund (NVS upd_auto, upd_last = Unix-Zeit der letzten Prüfung, upd_new = gefundene Version)
bool s_auto = true;
uint32_t s_last_unix = 0;
bool s_last_auto = false;        // letzte Prüfung kam aus dem Hintergrund
char s_avail[32] = "";           // neueste neuere Version (bleibt über Neustarts), leer = keine
std::atomic<uint32_t> s_seq{0};  // ändert sich mit jedem neuen Stand (Oberfläche lädt /api/update dann neu)
esp_timer_handle_t s_auto_timer = nullptr;
// Limit der API (aus der letzten Antwort)
int s_rate_left = -1, s_rate_limit = -1;
long s_rate_reset = 0;  // Unix-Zeit

const char* channel_name(Channel c) { return c == Channel::Preview ? "preview" : "stable"; }

void set_error(const char* text) {
    std::lock_guard<std::mutex> g(s_lock);
    std::snprintf(s_error, sizeof(s_error), "%s", text);
    ++s_seq;
}

void nvs_save(const char* key, const char* str, int u8 = -1, uint32_t u32 = 0) {
    nvs_handle_t h;
    if (nvs_open(kNvs, NVS_READWRITE, &h) != ESP_OK) return;
    if (str) nvs_set_str(h, key, str);
    else if (u8 >= 0) nvs_set_u8(h, key, static_cast<uint8_t>(u8));
    else nvs_set_u32(h, key, u32);
    nvs_commit(h);
    nvs_close(h);
}

unsigned internal_free_kb() { return heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024; }

// --- HTTPS ----------------------------------------------------------------------------------------------------

struct Http {
    esp_http_client_handle_t h = nullptr;
    int rate_left = -1, rate_limit = -1, retry_after = -1;
    long rate_reset = 0;
    ~Http() { reset(); }
    void reset() {
        if (!h) return;
        esp_http_client_close(h);
        esp_http_client_cleanup(h);
        h = nullptr;
    }
};

esp_err_t on_event(esp_http_client_event_t* e) {
    if (e->event_id != HTTP_EVENT_ON_HEADER || !e->user_data) return ESP_OK;
    auto* x = static_cast<Http*>(e->user_data);
    if (!strcasecmp(e->header_key, "x-ratelimit-remaining")) x->rate_left = std::atoi(e->header_value);
    else if (!strcasecmp(e->header_key, "x-ratelimit-limit")) x->rate_limit = std::atoi(e->header_value);
    else if (!strcasecmp(e->header_key, "x-ratelimit-reset")) x->rate_reset = std::atol(e->header_value);
    else if (!strcasecmp(e->header_key, "retry-after")) x->retry_after = std::atoi(e->header_value);
    return ESP_OK;
}

// Anfrage öffnen, Weiterleitungen folgen (auch auf andere Server); Statuscode oder -1 mit Fehlertext.
// from > 0: nur ab diesem Byte (HTTP-Range, seit 0.5.7: Download weitermachen), Antwort 206
int open(Http& x, const char* url, bool api, int64_t* length, char* error, size_t error_len, int64_t from = 0) {
    char ua[48];
    std::snprintf(ua, sizeof(ua), "Pixelia/%s", esp_app_get_description()->version);
    esp_http_client_config_t c{};
    c.url = url;
    c.event_handler = on_event;
    c.user_data = &x;
    c.crt_bundle_attach = esp_crt_bundle_attach;
    c.timeout_ms = 15000;
    c.buffer_size = 4096;     // Kopfzeilen der Antwort (die Weiterleitung trägt eine lange signierte Adresse)
    c.buffer_size_tx = 4096;  // Anfragezeile mit dieser Adresse
    c.user_agent = ua;
    c.keep_alive_enable = false;
    x.h = esp_http_client_init(&c);
    if (!x.h) {
        std::snprintf(error, error_len, "Kein Speicher für die Verbindung");
        return -1;
    }
    if (api) {
        esp_http_client_set_header(x.h, "Accept", "application/vnd.github+json");
        esp_http_client_set_header(x.h, "X-GitHub-Api-Version", "2022-11-28");
    }
    char range[40];
    if (from > 0) {
        std::snprintf(range, sizeof(range), "bytes=%lld-", static_cast<long long>(from));
        esp_http_client_set_header(x.h, "Range", range);
    }
    for (int hop = 0; hop < 5; ++hop) {
        esp_err_t err = esp_http_client_open(x.h, 0);
        if (err != ESP_OK) {
            std::snprintf(error, error_len, "Keine Verbindung zu GitHub (%s). Hat das WLAN Internet?", esp_err_to_name(err));
            return -1;
        }
        int64_t len = esp_http_client_fetch_headers(x.h);
        if (len < 0) {
            std::snprintf(error, error_len, "GitHub antwortet nicht (%s)", esp_err_to_name(static_cast<esp_err_t>(-len)));
            return -1;
        }
        int status = esp_http_client_get_status_code(x.h);
        if (status == 301 || status == 302 || status == 303 || status == 307 || status == 308) {
            esp_http_client_flush_response(x.h, nullptr);
            esp_http_client_close(x.h);
            if (esp_http_client_set_redirection(x.h) != ESP_OK) {
                std::snprintf(error, error_len, "Weiterleitung ohne Ziel (HTTP %d)", status);
                return -1;
            }
            continue;
        }
        *length = len;
        return status;
    }
    std::snprintf(error, error_len, "Zu viele Weiterleitungen");
    return -1;
}

// Lesen mit Wiederholung bei Zeitüberschreitung; 0 = Ende, < 0 = Fehler
int read_some(Http& x, char* buf, int len) {
    for (int tries = 0; tries < 3; ++tries) {
        int r = esp_http_client_read(x.h, buf, len);
        if (r != -ESP_ERR_HTTP_EAGAIN) return r;
    }
    return -1;
}

// Meldung für 403/429 der API (Limit ohne Token)
void rate_error(const Http& x, int status, char* error, size_t error_len) {
    char when[48] = "später";
    if (x.retry_after > 0) {
        std::snprintf(when, sizeof(when), "in %d s", x.retry_after);
    } else if (x.rate_reset > 0 && timesync::valid()) {
        long wait = x.rate_reset - static_cast<long>(time(nullptr));
        time_t at = x.rate_reset;
        tm lt;
        localtime_r(&at, &lt);
        if (wait > 0) {
            std::snprintf(when, sizeof(when), "um %02d:%02d Uhr (in %ld min)", lt.tm_hour, lt.tm_min, (wait + 59) / 60);
        }
    }
    if (status == 429 || x.rate_left == 0) {
        std::snprintf(error, error_len,
                      "GitHub-Limit erreicht: höchstens %d Abfragen pro Stunde je Internetanschluss. Wieder möglich %s.",
                      x.rate_limit > 0 ? x.rate_limit : 60, when);
    } else {
        std::snprintf(error, error_len, "GitHub verweigert die Abfrage (HTTP %d). Bitte %s erneut versuchen.", status, when);
    }
}

// --- Prüfung --------------------------------------------------------------------------------------------------

struct Release {
    char v[32] = "";
    Ver ver;
    bool pre = false, draft = false;
    char date[11] = "";
    uint32_t size = 0;
    std::string asset, url;  // Update-Datei pixelia-vX.Y.Z.bin
    std::string notes;       // gekürzt, ohne Abschnitt „Installation“
};

// Release-Notes für die Vorschau: Installationshinweise abschneiden, Länge begrenzen (nicht mitten im Zeichen)
void trim_notes(std::string* s) {
    size_t cut = s->find("\n### Installation");
    if (cut != std::string::npos) s->resize(cut);
    if (s->size() > kMaxNotes) {
        size_t n = kMaxNotes;
        while (n > 0 && (static_cast<unsigned char>((*s)[n]) & 0xC0) == 0x80) --n;
        s->resize(n);
    }
    while (!s->empty() && std::strchr(" \r\n", s->back())) s->pop_back();
}

bool parse_assets(Reader& r, Release* rel) {
    if (!r.expect('[')) return false;
    if (r.eat(']')) return true;
    std::string key, name, url;
    do {
        double size = 0;
        name.clear();
        url.clear();
        if (!r.expect('{')) return false;
        if (!r.eat('}')) {
            do {
                if (!r.string(&key) || !r.expect(':')) return false;
                bool ok = key == "name"                   ? r.string(&name)
                          : key == "browser_download_url" ? r.string(&url)
                          : key == "size"                 ? r.number(&size)
                                                          : r.skip();
                if (!ok) return false;
            } while (r.eat(','));
            if (!r.expect('}')) return false;
        }
        // Update-Datei (nicht Factory-Image, nicht ELF); passt sie zur Version, prüft check_job
        const bool bin = name.size() > 13 && name.compare(0, 9, "pixelia-v") == 0 &&
                         name.compare(name.size() - 4, 4, ".bin") == 0 && name.find("-factory") == std::string::npos;
        if (bin) {
            rel->asset = name;
            rel->url = url;
            rel->size = static_cast<uint32_t>(size);
        }
    } while (r.eat(','));
    return r.expect(']');
}

bool parse_release(Reader& r, Release* rel, std::string* key, std::string* text) {
    if (!r.expect('{')) return false;
    if (r.eat('}')) return true;
    std::string tag;
    do {
        if (!r.string(key) || !r.expect(':')) return false;
        bool ok = true;
        if (*key == "tag_name") {
            ok = r.string(&tag);
            const char* t = tag.c_str();
            if (*t == 'v') ++t;
            std::snprintf(rel->v, sizeof(rel->v), "%s", t);
        } else if (*key == "prerelease") {
            ok = r.boolean(&rel->pre);
        } else if (*key == "draft") {
            ok = r.boolean(&rel->draft);
        } else if (*key == "published_at") {
            ok = r.string(text);
            if (ok) std::snprintf(rel->date, sizeof(rel->date), "%.10s", text->c_str());
        } else if (*key == "body") {
            ok = r.string(&rel->notes);
        } else if (*key == "assets") {
            ok = parse_assets(r, rel);
        } else {
            ok = r.skip();
        }
        if (!ok) return false;
    } while (r.eat(','));
    return r.expect('}');
}

void check_job() {
    char error[192] = "";
    const int64_t t0 = esp_timer_get_time();
    const Channel ch = channel();
    Ver installed;
    const char* me = esp_app_get_description()->version;
    const bool known = parse_version(me, &installed, true);

    char url[128];
    std::snprintf(url, sizeof(url), "https://api.github.com/repos/%s/releases?per_page=%d", kRepo, kPerPage);
    Http x;
    int64_t len = 0;
    const unsigned ram_before = internal_free_kb();
    int status = open(x, url, true, &len, error, sizeof(error));
    const unsigned ram_tls = internal_free_kb();
    {
        std::lock_guard<std::mutex> g(s_lock);
        if (x.rate_left >= 0) {
            s_rate_left = x.rate_left;
            s_rate_limit = x.rate_limit;
            s_rate_reset = x.rate_reset;
        }
    }
    if (status < 0) return set_error(error);
    if (status == 403 || status == 429) {
        rate_error(x, status, error, sizeof(error));
        ESP_LOGW(TAG, "%s", error);
        return set_error(error);
    }
    if (status != 200) {
        std::snprintf(error, sizeof(error), "GitHub antwortet mit HTTP %d", status);
        return set_error(error);
    }

    // Antwort komplett ins PSRAM (Länge oft unbekannt: chunked)
    auto* buf = static_cast<char*>(heap_caps_malloc(kMaxResponse, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!buf) return set_error("Kein PSRAM frei für die Antwort");
    size_t n = 0;
    for (;;) {
        if (n == kMaxResponse) {
            heap_caps_free(buf);
            return set_error("Antwort von GitHub zu groß");
        }
        int r = read_some(x, buf + n, static_cast<int>(kMaxResponse - n));
        if (r < 0) {
            heap_caps_free(buf);
            return set_error("Verbindung zu GitHub abgebrochen");
        }
        if (r == 0) break;
        n += r;
    }
    if (!esp_http_client_is_complete_data_received(x.h)) {
        heap_caps_free(buf);
        return set_error("Antwort von GitHub unvollständig");
    }

    // Lesen: Notizen und Schlüssel in Puffern ≥ 16 KB, damit sie im PSRAM liegen (SPIRAM_MALLOC_ALWAYSINTERNAL)
    std::vector<Release> found;
    std::vector<Target> older;
    std::string key, text;
    text.reserve(64);
    Reader r(buf, n);
    int total = 0;
    char latest[32] = "";
    Ver latest_ver;
    if (r.expect('[') && !r.eat(']')) {
        do {
            Release rel;
            rel.notes.reserve(20 * 1024);
            if (!parse_release(r, &rel, &key, &text)) break;
            ++total;
            Ver v;
            if (rel.draft || !parse_version(rel.v, &v) || !rel.size || rel.asset != "pixelia-v" + std::string(rel.v) + ".bin") continue;
            if (ch == Channel::Stable && (rel.pre || v.pre)) continue;
            rel.ver = v;
            if (!latest[0] || compare(v, latest_ver) > 0) {
                latest_ver = v;
                std::snprintf(latest, sizeof(latest), "%s", rel.v);
            }
            if (known && compare(v, installed) <= 0) {
                // Ältere Version ab 0.4.0 (kann über GitHub zurück); die installierte selbst nicht
                if (compare(v, installed) < 0 && compare(v, Ver{0, 4, 0}) >= 0) {
                    Target t;
                    std::snprintf(t.v, sizeof(t.v), "%s", rel.v);
                    std::snprintf(t.date, sizeof(t.date), "%s", rel.date);
                    t.pre = rel.pre;
                    t.size = rel.size;
                    t.url = rel.url;
                    t.ver = v;
                    older.push_back(std::move(t));
                }
                continue;
            }
            trim_notes(&rel.notes);
            found.push_back(std::move(rel));
        } while (r.eat(','));
    }
    heap_caps_free(buf);
    if (!r.ok()) return set_error("Antwort von GitHub nicht lesbar");
    std::sort(found.begin(), found.end(), [](const Release& a, const Release& b) { return compare(a.ver, b.ver) > 0; });

    Json j;
    j.text().reserve(24 * 1024);  // PSRAM
    j.arr();
    for (const Release& rel : found) {
        j.obj().str("v", rel.v).boolean("pre", rel.pre).str("date", rel.date).num("size", rel.size);
        j.str_n("notes", rel.notes.data(), rel.notes.size()).end_obj();
    }
    j.end_arr();
    std::sort(older.begin(), older.end(), [](const Target& a, const Target& b) { return compare(a.ver, b.ver) > 0; });
    Json jo;
    jo.arr();
    for (const Target& t : older) jo.obj().str("v", t.v).boolean("pre", t.pre).str("date", t.date).num("size", t.size).end_obj();
    jo.end_arr();
    const uint32_t now_unix = timesync::valid() ? static_cast<uint32_t>(time(nullptr)) : 0;
    {
        std::lock_guard<std::mutex> g(s_lock);
        s_older = std::move(older);
        s_older_json = std::move(jo.text());
        if (now_unix) s_last_unix = now_unix;
        std::snprintf(s_avail, sizeof(s_avail), "%s", found.empty() ? "" : found[0].v);
        ++s_seq;
        s_releases = std::move(j.text());
        s_checked_us = esp_timer_get_time();
        s_checked_channel = ch;
        std::snprintf(s_latest, sizeof(s_latest), "%s", latest);
        s_target = Target{};
        if (!found.empty()) {
            std::snprintf(s_target.v, sizeof(s_target.v), "%s", found[0].v);
            s_target.pre = found[0].pre;
            s_target.size = found[0].size;
            s_target.url = found[0].url;
        }
        s_error[0] = 0;
    }
    if (now_unix) nvs_save("upd_last", nullptr, -1, now_unix);
    nvs_save("upd_new", found.empty() ? "" : found[0].v);
    ESP_LOGI(TAG, "Prüfung (%s%s): %d Releases, %u KB Antwort in %lld ms, neueste %s, %d neuer als %s; Limit %d/%d; "
             "RAM %u → %u KB mit TLS",
             channel_name(ch), s_last_auto ? ", Hintergrund" : "", total, static_cast<unsigned>(n / 1024), (esp_timer_get_time() - t0) / 1000,
             latest[0] ? latest : "–", static_cast<int>(found.size()), me, x.rate_left, x.rate_limit, ram_before, ram_tls);
}

// --- Installation ---------------------------------------------------------------------------------------------

void install_job(Target target) {
    char error[192] = "";
    const int64_t t0 = esp_timer_get_time();
    unsigned ram_min = internal_free_kb();
    unsigned psram_min = heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024;
    float fps_min = 99;
    ESP_LOGW(TAG, "Update auf %s von GitHub (%u KB)", target.v, static_cast<unsigned>(target.size / 1024));

    Http x;
    int64_t len = 0;
    int status = open(x, target.url.c_str(), false, &len, error, sizeof(error));
    if (status < 0) return set_error(error);
    if (s_cancel) {
        s_cancelled = true;
        ESP_LOGW(TAG, "Update von GitHub vor dem Download abgebrochen");
        return set_error("");
    }
    if (status != 200) {
        std::snprintf(error, sizeof(error), "Download von GitHub fehlgeschlagen (HTTP %d)", status);
        return set_error(error);
    }
    if (len > 0 && static_cast<uint64_t>(len) != target.size) {
        std::snprintf(error, sizeof(error), "Datei hat %lld Byte, laut Release %lu", len, static_cast<unsigned long>(target.size));
        return set_error(error);
    }
    const int64_t t_conn = esp_timer_get_time();

    auto* buf = static_cast<uint8_t*>(heap_caps_malloc(kChunk, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    if (!buf) return set_error("Kein Speicher frei");
    size_t got = 0, filled = 0;
    bool started = false;
    int64_t last_sample = 0;
    // seit 0.5.7: Ohne Fortschritt (WLAN-Aussetzer) nach kStallUs neu verbinden und per HTTP-Range ab dem letzten
    // Byte weitermachen, höchstens kResumes Mal (0.4.1 → 0.4.2 hing einmal vier Minuten bei 432 KB)
    constexpr int64_t kStallUs = 30LL * 1000000;
    constexpr int kResumes = 5;
    int resumes = 0;
    int64_t last_progress = esp_timer_get_time();
    auto resume = [&](const char* why) -> bool {
        if (resumes >= kResumes || s_cancel) {
            std::snprintf(error, sizeof(error), "Download nach %u KB abgebrochen (%s, %d Versuche)",
                          static_cast<unsigned>(got / 1024), why, resumes + 1);
            return false;
        }
        ++resumes;
        ESP_LOGW(TAG, "Download bei %u KB: %s, neuer Versuch %d/%d ab Byte %u", static_cast<unsigned>(got / 1024), why,
                 resumes, kResumes, static_cast<unsigned>(got));
        x.reset();
        vTaskDelay(pdMS_TO_TICKS(2000 * resumes));
        int64_t rest = 0;
        char err2[128] = "";
        const int st = open(x, target.url.c_str(), false, &rest, err2, sizeof(err2), static_cast<int64_t>(got));
        if (st == 206 && (rest <= 0 || static_cast<uint64_t>(rest) == target.size - got)) {
            last_progress = esp_timer_get_time();
            return true;
        }
        if (st == 200) {
            // Server ignoriert Range: von vorn lesen und das Vorhandene überspringen
            char skip[512];
            for (size_t done = 0; done < got;) {
                const int r = read_some(x, skip, static_cast<int>(std::min(sizeof(skip), got - done)));
                if (r <= 0) {
                    std::snprintf(error, sizeof(error), "Download nach %u KB abgebrochen (Überspringen)", static_cast<unsigned>(got / 1024));
                    return false;
                }
                done += static_cast<size_t>(r);
            }
            last_progress = esp_timer_get_time();
            return true;
        }
        std::snprintf(error, sizeof(error), "Download nach %u KB abgebrochen (%s)", static_cast<unsigned>(got / 1024),
                      st < 0 ? err2 : "Weitermachen nicht möglich");
        return false;
    };
    while (got < target.size) {
        if (s_cancel) {
            std::snprintf(error, sizeof(error), "abgebrochen nach %u KB", static_cast<unsigned>(got / 1024));
            break;
        }
        if (got > 300 * 1024 && s_test_break.exchange(false)) {  // Prüfung: Verbindung mitten im Download trennen
            if (!resume("Test: Verbindung getrennt")) break;
            continue;
        }
        const size_t want = std::min(kChunk - filled, static_cast<size_t>(target.size - got));
        const int r = esp_http_client_read(x.h, reinterpret_cast<char*>(buf + filled), static_cast<int>(want));
        if (r <= 0) {
            const bool timeout = r == -ESP_ERR_HTTP_EAGAIN;
            if (timeout && esp_timer_get_time() - last_progress < kStallUs) continue;  // noch warten
            if (!resume(timeout ? "30 s ohne Fortschritt" : r == 0 ? "Verbindung beendet" : "Lesefehler")) break;
            continue;
        }
        last_progress = esp_timer_get_time();
        got += r;
        filled += r;
        if (!started) {
            if (filled < ota::kHeaderBytes && got < target.size) continue;
            // Vor dem ersten Schreiben: Pixelia-Image für diesen Chip und genau die Version aus dem Release
            esp_app_desc_t ad;
            if (!ota::check_header(buf, filled, &ad, error, sizeof(error))) break;
            if (std::strcmp(ad.version, target.v) != 0) {
                std::snprintf(error, sizeof(error), "Datei enthält Version %.31s statt %.31s", ad.version, target.v);
                break;
            }
            if (ota::begin(buf, filled, target.size, error, sizeof(error), "github") != ESP_OK) break;
            started = true;
        }
        if (filled == kChunk || got == target.size) {
            esp_err_t err = ota::write(buf, filled);
            if (err != ESP_OK) {
                std::snprintf(error, sizeof(error), "Schreiben fehlgeschlagen: %s", esp_err_to_name(err));
                break;
            }
            filled = 0;
        }
        const int64_t now = esp_timer_get_time();
        if (now - last_sample > 500000) {
            last_sample = now;
            ram_min = std::min(ram_min, internal_free_kb());
            psram_min = std::min(psram_min, static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
            const float fps = perf::snapshot().fps;
            if (fps > 0) fps_min = std::min(fps_min, fps);
        }
    }
    heap_caps_free(buf);
    if (s_cancel) {
        if (started) ota::cancel(error, true);
        ESP_LOGW(TAG, "Update von GitHub %s, die laufende Version bleibt (%.1f s)", error, (esp_timer_get_time() - t0) / 1e6);
        s_cancelled = true;
        return set_error("");
    }
    if (error[0] || !started) {
        if (started) ota::cancel(error[0] ? error : "Download unvollständig");
        set_error(error[0] ? error : "Download unvollständig");
        ESP_LOGE(TAG, "Update von GitHub fehlgeschlagen: %s", error);
        return;
    }
    if (ota::finish(error, sizeof(error)) != ESP_OK) return set_error(error);
    const double secs = (esp_timer_get_time() - t0) / 1e6;
    ESP_LOGW(TAG, "Update von GitHub: %u KB in %.1f s (Verbindung %.1f s, %.0f KB/s), RAM-Tiefstwert %u KB, PSRAM %u KB, "
             "fps min %.1f, Stack frei %u Byte, %d× weitergemacht; Neustart",
             static_cast<unsigned>(target.size / 1024), secs, (t_conn - t0) / 1e6,
             target.size / 1024.0 / ((esp_timer_get_time() - t_conn) / 1e6), ram_min, psram_min, fps_min,
             static_cast<unsigned>(uxTaskGetStackHighWaterMark(nullptr)), resumes);
    ota::restart_soon(1500);
}

Target s_install_target;

void job_task(void*) {
    if (s_job == Job::Check) {
        check_job();
    } else {
        Target t;
        {
            std::lock_guard<std::mutex> g(s_lock);
            t = s_install_target;
        }
        install_job(std::move(t));
    }
    s_job = Job::None;
    ++s_seq;
    vTaskDelete(nullptr);
}

// Gemeinsame Voraussetzungen; startet die Task
bool launch(Job job, char* error, size_t error_len) {
    const wifi::Status w = wifi::status();
    if (w.ap_active || w.state != wifi::State::Connected) {
        std::snprintf(error, error_len, "%s", w.ap_active ? "Im Setup-Modus hat Pixelia kein Internet. Erst das WLAN einrichten."
                                                         : "Pixelia ist gerade nicht mit dem WLAN verbunden.");
        return false;
    }
    Job none = Job::None;
    if (!s_job.compare_exchange_strong(none, job)) {
        std::snprintf(error, error_len, "%s", none == Job::Check ? "Die Prüfung läuft schon" : "Die Installation läuft schon");
        return false;
    }
    set_error("");
    s_cancel = false;
    s_cancelled = false;
    if (job == Job::Check) s_last_auto = false;
    // 8 KB Stack für TLS (Handschlag mit Zertifikatsprüfung); Puffer von mbedTLS liegen im PSRAM (sdkconfig)
    if (xTaskCreatePinnedToCore(job_task, "github", 8192, nullptr, 3, nullptr, kSystemCore) != pdPASS) {
        s_job = Job::None;
        std::snprintf(error, error_len, "Kein Speicher für die Update-Task");
        return false;
    }
    return true;
}
// Zeitgeber alle 5 min: im Hintergrund prüfen, wenn eingeschaltet, mit dem Internet verbunden, Uhrzeit bekannt und die
// letzte Prüfung mindestens einen Tag her ist. Nie während eines Updates oder Probelaufs. Installiert nichts.
void auto_tick(void*) {
    if (esp_timer_get_time() < 120LL * 1000000 || !timesync::valid()) return;
    {
        std::lock_guard<std::mutex> g(s_lock);
        if (!s_auto || static_cast<uint32_t>(time(nullptr)) - s_last_unix < kAutoEvery) return;
    }
    const ota::Status o = ota::status();
    if (o.busy || o.trial) return;
    char error[160];
    if (launch(Job::Check, error, sizeof(error))) {
        s_last_auto = true;
        ESP_LOGI(TAG, "Prüfung im Hintergrund (einmal täglich)");
    }
}
}  // namespace

void init() {
    nvs_handle_t h;
    uint8_t v = 0, au = 1;
    uint32_t last = 0;
    char avail[32] = "";
    size_t len = sizeof(avail);
    if (nvs_open(kNvs, NVS_READONLY, &h) == ESP_OK) {
        nvs_get_u8(h, kNvsKey, &v);
        nvs_get_u8(h, "upd_auto", &au);
        nvs_get_u32(h, "upd_last", &last);
        if (nvs_get_str(h, "upd_new", avail, &len) != ESP_OK) avail[0] = 0;
        nvs_close(h);
    }
    // Gefundene Version nur merken, wenn sie noch neuer ist als die laufende (sonst wurde sie inzwischen installiert)
    Ver a, me;
    if (avail[0] && (!parse_version(avail, &a) || !parse_version(esp_app_get_description()->version, &me, true) ||
                     compare(a, me) <= 0)) {
        avail[0] = 0;
    }
    {
        std::lock_guard<std::mutex> g(s_lock);
        s_channel = v == 1 ? Channel::Preview : Channel::Stable;
        s_auto = au != 0;
        s_last_unix = last;
        std::snprintf(s_avail, sizeof(s_avail), "%s", avail);
    }
    esp_timer_create_args_t args{};
    args.callback = auto_tick;
    args.name = "upd-auto";
    if (esp_timer_create(&args, &s_auto_timer) == ESP_OK) esp_timer_start_periodic(s_auto_timer, kAutoTimerUs);
}

void set_auto(bool on) {
    nvs_save("upd_auto", nullptr, on ? 1 : 0);
    std::lock_guard<std::mutex> g(s_lock);
    if (s_auto != on) ESP_LOGI(TAG, "Prüfung im Hintergrund %s", on ? "an" : "aus");
    s_auto = on;
    ++s_seq;
}

void test_break() { s_test_break.store(true); }

bool cancel_install() {
    if (s_job != Job::Install) return false;
    s_cancel = true;
    return true;
}

std::string available() {
    std::lock_guard<std::mutex> g(s_lock);
    return s_avail;
}

uint32_t seq() { return s_seq; }

Channel channel() {
    std::lock_guard<std::mutex> g(s_lock);
    return s_channel;
}

void set_channel(Channel c) {
    nvs_handle_t h;
    if (nvs_open(kNvs, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u8(h, kNvsKey, c == Channel::Preview ? 1 : 0);
        nvs_commit(h);
        nvs_close(h);
    }
    std::lock_guard<std::mutex> g(s_lock);
    if (s_channel != c) {
        ESP_LOGI(TAG, "Kanal: %s", channel_name(c));
        s_avail[0] = 0;  // gilt für den alten Kanal; die nächste Prüfung sucht neu
        ++s_seq;
    }
    s_channel = c;
}

bool start_check(char* error, size_t error_len) { return launch(Job::Check, error, error_len); }

bool start_install(const char* version, bool older, char* error, size_t error_len) {
    {
        std::lock_guard<std::mutex> g(s_lock);
        const Target* t = nullptr;
        if (!older && s_target.v[0] && std::strcmp(s_target.v, version) == 0) t = &s_target;
        for (const Target& o : s_older) {
            if (older && std::strcmp(o.v, version) == 0) t = &o;
        }
        if (!t) {
            std::snprintf(error, error_len, "Version %.31s ist nicht das Ergebnis der letzten Prüfung, bitte neu suchen", version);
            return false;
        }
        s_install_target = *t;
    }
    if (older) ESP_LOGW(TAG, "Ältere Version %s angefordert (Seite Experimentell)", version);
    const ota::Status o = ota::status();
    if (o.busy || o.trial) {
        std::snprintf(error, error_len, "%s", o.busy ? "Es läuft bereits ein Update" : "Der Probelauf der aktuellen Version läuft noch, bitte kurz warten");
        return false;
    }
    return launch(Job::Install, error, error_len);
}

std::string state_json() {
    const wifi::Status w = wifi::status();
    std::lock_guard<std::mutex> g(s_lock);
    Json j;
    j.text().reserve(s_releases.size() + 512);
    const Job job = s_job;
    j.obj().str("ch", channel_name(s_channel)).str("job", job == Job::Check ? "check" : job == Job::Install ? "install" : "");
    j.str("err", s_error).boolean("cancelled", s_cancelled).boolean("net", !w.ap_active && w.state == wifi::State::Connected);
    j.boolean("auto", s_auto).boolean("lastAuto", s_last_auto).num("lastT", s_last_unix).str("avail", s_avail);
    j.num("seq", s_seq.load());
    j.num("checkedS", s_checked_us < 0 ? -1 : static_cast<double>((esp_timer_get_time() - s_checked_us) / 1000000));
    j.str("checkedCh", channel_name(s_checked_channel)).str("latest", s_latest);
    j.raw("rel", s_releases);
    j.raw("old", s_older_json);
    j.obj("rate").num("left", s_rate_left).num("limit", s_rate_limit);
    long reset_s = -1;
    if (s_rate_reset > 0 && timesync::valid()) reset_s = std::max(0L, s_rate_reset - static_cast<long>(time(nullptr)));
    j.num("resetS", static_cast<double>(reset_s)).end_obj();
    j.end_obj();
    return std::move(j.text());
}

}  // namespace app::github
