#pragma once
// Minimaler GT911-Treiber (I²C, ohne INT-Leitung, Abfrage per Polling)

#include <cstdint>

#include "driver/i2c_master.h"
#include "esp_err.h"

namespace board::touch {

inline constexpr uint8_t kAddrPrimary = 0x5D;
inline constexpr uint8_t kAddrSecondary = 0x14;

struct Info {
    uint8_t address;
    char product[5];       // z. B. "911"
    uint16_t firmware;
    uint16_t x_resolution;  // konfigurierte Auflösung
    uint16_t y_resolution;
    uint8_t vendor;
    uint8_t config_version;
};

struct Point {
    uint8_t id;
    uint16_t x;
    uint16_t y;
    uint16_t size;
};

// Reset-Puls über die RST-Leitung (INT bleibt unberührt)
void hardware_reset();
// RST dauerhaft auf low (true) oder high (false), für den Pin-Test
void hold_reset(bool low);

esp_err_t init(i2c_master_bus_handle_t bus, uint8_t address, Info* info);
inline constexpr int kNoData = -2;

// Liefert die Anzahl der Berührungen (0..5), kNoData ohne neue Daten, -1 bei Fehler
int read(Point* points, int max_points);

}  // namespace board::touch
