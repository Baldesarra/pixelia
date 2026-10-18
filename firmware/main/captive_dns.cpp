// Mini-DNS für das Captive Portal: beantwortet im Setup-WLAN jede A-Anfrage mit 192.168.4.1.
// So landet jede Adresse (auch die Prüfadressen von Android, iOS und Windows) auf der Einrichtungsseite.
// Lauscht nur auf der Adresse des Access Points, das Heimnetz bleibt unberührt.

#include <atomic>
#include <cstring>

#include "app.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "net.h"

namespace app::captive_dns {

namespace {
constexpr const char* TAG = "dns";
std::atomic<bool> s_run{false};
std::atomic<bool> s_alive{false};

// Antwort in den Puffer der Anfrage schreiben; 0 = nicht antworten
int build_reply(uint8_t* p, int n, int cap, uint32_t ip_be) {
    if (n < 12 || (p[2] & 0x80) || ((p[2] >> 3) & 0x0F) != 0) return 0;  // nur Standardanfragen
    if (((p[4] << 8) | p[5]) != 1) return 0;                               // genau eine Frage
    int i = 12;
    while (i < n && p[i] != 0) {
        if (p[i] & 0xC0) return 0;  // Kompression in Fragen kommt praktisch nicht vor
        i += p[i] + 1;
    }
    if (i + 5 > n) return 0;
    ++i;
    const int qtype = (p[i] << 8) | p[i + 1];
    const int qclass = ((p[i + 2] << 8) | p[i + 3]) & 0x7FFF;
    i += 4;

    p[2] = static_cast<uint8_t>(0x84 | (p[2] & 0x01));  // Antwort, autoritativ, RD aus der Anfrage
    p[3] = 0x80;                                         // Rekursion verfügbar, kein Fehler
    std::memset(p + 6, 0, 6);                            // Zähler Antworten/Autorität/Zusatz
    int len = i;                                         // Frage bleibt, Zusatzdaten (EDNS) fallen weg
    if (qtype == 1 && qclass == 1) {
        if (len + 16 > cap) return 0;
        const auto* ip = reinterpret_cast<const uint8_t*>(&ip_be);
        const uint8_t answer[16] = {0xC0, 0x0C, 0, 1, 0, 1, 0, 0, 0, 30, 0, 4, ip[0], ip[1], ip[2], ip[3]};
        std::memcpy(p + len, answer, sizeof(answer));
        len += sizeof(answer);
        p[7] = 1;
    }
    return len;  // andere Typen (z. B. AAAA): leere Antwort
}

void dns_task(void*) {
    const uint32_t ip_be = inet_addr(wifi::kApIp);
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(53);
    addr.sin_addr.s_addr = ip_be;
    timeval tv{1, 0};
    if (sock < 0 || setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0 ||
        bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        ESP_LOGE(TAG, "DNS-Port 53 nicht verfügbar (errno %d)", errno);
        if (sock >= 0) close(sock);
        s_alive = false;
        vTaskDelete(nullptr);
        return;
    }
    ESP_LOGI(TAG, "Captive-DNS aktiv");
    uint8_t buf[512];
    uint32_t answered = 0;
    while (s_run) {
        sockaddr_in from{};
        socklen_t from_len = sizeof(from);
        int n = recvfrom(sock, buf, sizeof(buf), 0, reinterpret_cast<sockaddr*>(&from), &from_len);
        if (n <= 0) continue;
        int len = build_reply(buf, n, sizeof(buf), ip_be);
        if (len > 0 && sendto(sock, buf, len, 0, reinterpret_cast<sockaddr*>(&from), from_len) > 0) ++answered;
    }
    close(sock);
    ESP_LOGI(TAG, "Captive-DNS beendet (%lu Antworten)", static_cast<unsigned long>(answered));
    s_alive = false;
    vTaskDelete(nullptr);
}
}  // namespace

void start() {
    if (s_alive) {
        s_run = true;
        return;
    }
    s_run = true;
    s_alive = true;
    xTaskCreatePinnedToCore(dns_task, "dns", 3072, nullptr, 4, nullptr, kSystemCore);
}

void stop() { s_run = false; }

bool running() { return s_alive && s_run; }

}  // namespace app::captive_dns
