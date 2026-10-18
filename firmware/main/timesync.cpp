// Echte Uhrzeit per NTP (Projektplan 5, Weltzeit): Zeitzone Europa/Berlin mit Sommerzeit.
// Die Systemzeit übersteht Software-Neustarts (z. B. nach einem Update), erst ein Stromausfall löscht sie.

#include <atomic>
#include <cstdlib>
#include <ctime>
#include <sys/time.h>

#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "net.h"

namespace app::timesync {

namespace {
constexpr const char* TAG = "zeit";
// POSIX-Regel für Europa/Berlin: MEZ, Sommerzeit vom letzten Sonntag im März 2 Uhr bis letzten Sonntag im Oktober 3 Uhr
constexpr const char* kTimezone = "CET-1CEST,M3.5.0,M10.5.0/3";
constexpr time_t kValidAfter = 1735689600;  // 2025-01-01: davor ist die Uhr sicher nicht gestellt

std::atomic<uint32_t> s_syncs{0};
bool s_started = false;

void on_sync(struct timeval*) {
    uint32_t n = ++s_syncs;
    if (n == 1) log_status();
}

// Tage seit 1970-01-01 für ein Kalenderdatum (proleptischer gregorianischer Kalender)
int64_t days_from_civil(int y, unsigned m, unsigned d) {
    y -= m <= 2;
    const int64_t era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<int64_t>(doe) - 719468;
}
}  // namespace

void init() {
    setenv("TZ", kTimezone, 1);
    tzset();
    esp_sntp_config_t cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(2, ESP_SNTP_SERVER_LIST("de.pool.ntp.org", "pool.ntp.org"));
    cfg.start = false;
    cfg.sync_cb = on_sync;
    esp_netif_sntp_init(&cfg);
    if (valid()) ESP_LOGI(TAG, "Uhrzeit aus der Zeit vor dem Neustart übernommen");
}

void on_connected() {
    if (s_started) return;
    s_started = true;
    esp_netif_sntp_start();
    ESP_LOGI(TAG, "NTP-Abgleich gestartet (de.pool.ntp.org)");
}

bool valid() { return time(nullptr) > kValidAfter; }

bool local_us(int64_t* out) {
    timeval tv;
    gettimeofday(&tv, nullptr);
    if (tv.tv_sec <= kValidAfter) return false;
    tm lt;
    localtime_r(&tv.tv_sec, &lt);
    int64_t days = days_from_civil(lt.tm_year + 1900, static_cast<unsigned>(lt.tm_mon + 1), static_cast<unsigned>(lt.tm_mday));
    int64_t secs = days * 86400 + lt.tm_hour * 3600 + lt.tm_min * 60 + lt.tm_sec;
    *out = secs * 1000000 + tv.tv_usec;
    return true;
}

void log_status() {
    if (!valid()) {
        ESP_LOGI(TAG, "Uhrzeit noch nicht gestellt (%lu NTP-Abgleiche)", static_cast<unsigned long>(s_syncs.load()));
        return;
    }
    time_t now = time(nullptr);
    tm lt;
    localtime_r(&now, &lt);
    char buf[40];
    strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M:%S %Z", &lt);
    ESP_LOGI(TAG, "Uhrzeit %s (%lu NTP-Abgleiche)", buf, static_cast<unsigned long>(s_syncs.load()));
}

uint32_t sync_count() { return s_syncs; }

}  // namespace app::timesync
