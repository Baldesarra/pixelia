// Systeminfo: Chip, Flash, PSRAM, Speicher, Resetgrund, Partition, Absturzbericht

#include "app.h"
#include "esp_app_desc.h"
#include "esp_chip_info.h"
#include "esp_core_dump.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_ota_ops.h"
#include "esp_psram.h"
#include "esp_system.h"
#include "sdkconfig.h"

namespace app::sysinfo {

namespace {
constexpr const char* TAG = "sys";

const char* reset_reason_text(esp_reset_reason_t r) {
    switch (r) {
        case ESP_RST_POWERON: return "Einschalten";
        case ESP_RST_EXT: return "externer Reset";
        case ESP_RST_SW: return "Software-Neustart";
        case ESP_RST_PANIC: return "Absturz (Panic)";
        case ESP_RST_INT_WDT: return "Interrupt-Watchdog";
        case ESP_RST_TASK_WDT: return "Task-Watchdog";
        case ESP_RST_WDT: return "Watchdog";
        case ESP_RST_DEEPSLEEP: return "Aufwachen aus Tiefschlaf";
        case ESP_RST_BROWNOUT: return "Unterspannung (Brownout)";
        case ESP_RST_SDIO: return "SDIO";
        case ESP_RST_USB: return "USB";
        case ESP_RST_JTAG: return "JTAG";
        default: return "unbekannt";
    }
}
}  // namespace

void log_all() {
    const esp_app_desc_t* app = esp_app_get_description();
    ESP_LOGI(TAG, "Pixelia %s (ESP-IDF %s, gebaut %s %s)", app->version, app->idf_ver, app->date, app->time);

    esp_chip_info_t chip;
    esp_chip_info(&chip);
    ESP_LOGI(TAG, "Chip ESP32-S3 Rev. v%d.%d, %d Kerne, %d MHz,%s%s%s", chip.revision / 100, chip.revision % 100,
             chip.cores, CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ, (chip.features & CHIP_FEATURE_WIFI_BGN) ? " WLAN" : "",
             (chip.features & CHIP_FEATURE_BLE) ? " BLE" : "",
             (chip.features & CHIP_FEATURE_EMB_PSRAM) ? " PSRAM im Chip" : "");

    uint8_t mac[6] = {};
    esp_efuse_mac_get_default(mac);
    ESP_LOGI(TAG, "MAC %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    uint32_t flash_size = 0, flash_id = 0;
    esp_flash_get_size(nullptr, &flash_size);
    esp_flash_read_id(nullptr, &flash_id);
    ESP_LOGI(TAG, "Flash %lu MB, ID 0x%06lx, Modus %s %s", static_cast<unsigned long>(flash_size >> 20),
             static_cast<unsigned long>(flash_id),
#if CONFIG_ESPTOOLPY_FLASHMODE_QIO
             "QIO",
#elif CONFIG_ESPTOOLPY_FLASHMODE_DIO
             "DIO",
#else
             "?",
#endif
             CONFIG_ESPTOOLPY_FLASHFREQ);

    if (esp_psram_is_initialized()) {
        ESP_LOGI(TAG, "PSRAM %u KB, %s %d MHz, XIP aus PSRAM: %s", static_cast<unsigned>(esp_psram_get_size() >> 10),
#if CONFIG_SPIRAM_MODE_OCT
                 "Octal",
#else
                 "Quad",
#endif
                 CONFIG_SPIRAM_SPEED,
#if CONFIG_SPIRAM_XIP_FROM_PSRAM
                 "ja"
#else
                 "nein"
#endif
        );
    } else {
        ESP_LOGE(TAG, "PSRAM nicht initialisiert!");
    }

    esp_reset_reason_t rr = esp_reset_reason();
    ESP_LOGI(TAG, "Resetgrund: %s (%d)", reset_reason_text(rr), static_cast<int>(rr));

    const esp_partition_t* part = esp_ota_get_running_partition();
    if (part) {
        ESP_LOGI(TAG, "Läuft aus %s @0x%06lx (%lu KB)", part->label, static_cast<unsigned long>(part->address),
                 static_cast<unsigned long>(part->size >> 10));
    }

    size_t dump_addr = 0, dump_size = 0;
    if (esp_core_dump_image_get(&dump_addr, &dump_size) == ESP_OK && dump_size > 0) {
        ESP_LOGW(TAG, "Absturzbericht im Flash vorhanden (%u Byte)", static_cast<unsigned>(dump_size));
    } else {
        ESP_LOGI(TAG, "Kein Absturzbericht im Flash");
    }
    log_heap();
}

void log_heap() {
    ESP_LOGI(TAG, "RAM intern frei %u KB (min %u KB, größter Block %u KB), DMA-fähig %u KB, PSRAM frei %u KB",
             static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) >> 10),
             static_cast<unsigned>(heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL) >> 10),
             static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL) >> 10),
             static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_DMA) >> 10),
             static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) >> 10));
}

}  // namespace app::sysinfo
