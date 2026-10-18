#include "board/touch_gt911.h"

#include <cstring>

#include "board/pins.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace board::touch {

namespace {
constexpr uint16_t kRegConfigVersion = 0x8047;
constexpr uint16_t kRegProductId = 0x8140;
constexpr uint16_t kRegStatus = 0x814E;
constexpr uint16_t kRegPoints = 0x814F;
constexpr int kTimeoutMs = 50;

i2c_master_dev_handle_t s_dev = nullptr;

esp_err_t read_reg(uint16_t reg, uint8_t* buf, size_t len) {
    uint8_t addr[2] = {static_cast<uint8_t>(reg >> 8), static_cast<uint8_t>(reg & 0xff)};
    return i2c_master_transmit_receive(s_dev, addr, 2, buf, len, kTimeoutMs);
}

esp_err_t write_reg8(uint16_t reg, uint8_t value) {
    uint8_t buf[3] = {static_cast<uint8_t>(reg >> 8), static_cast<uint8_t>(reg & 0xff), value};
    return i2c_master_transmit(s_dev, buf, 3, kTimeoutMs);
}
}  // namespace

void hold_reset(bool low) {
    static bool configured = false;
    if (!configured) {
        // Pegel vor dem Umschalten auf Ausgang setzen, damit kein ungewollter Reset-Puls entsteht
        gpio_set_level(kTouchRst, low ? 0 : 1);
        gpio_config_t io = {};
        io.pin_bit_mask = 1ULL << kTouchRst;
        io.mode = GPIO_MODE_OUTPUT;
        gpio_config(&io);
        configured = true;
    }
    gpio_set_level(kTouchRst, low ? 0 : 1);
}

void hardware_reset() {
    hold_reset(true);
    vTaskDelay(pdMS_TO_TICKS(20));
    hold_reset(false);
    // GT911 braucht nach dem Reset etwa 50 ms, bis er auf I²C antwortet
    vTaskDelay(pdMS_TO_TICKS(100));
}

esp_err_t init(i2c_master_bus_handle_t bus, uint8_t address, Info* info) {
    if (s_dev) {
        i2c_master_bus_rm_device(s_dev);
        s_dev = nullptr;
    }
    i2c_device_config_t dev = {};
    dev.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev.device_address = address;
    dev.scl_speed_hz = 400000;
    esp_err_t err = i2c_master_bus_add_device(bus, &dev, &s_dev);
    if (err != ESP_OK) return err;

    uint8_t id[11] = {};
    err = read_reg(kRegProductId, id, sizeof(id));
    if (err != ESP_OK) return err;
    uint8_t cfg_ver = 0;
    read_reg(kRegConfigVersion, &cfg_ver, 1);
    if (info) {
        std::memset(info, 0, sizeof(*info));
        info->address = address;
        std::memcpy(info->product, id, 4);
        info->firmware = static_cast<uint16_t>(id[4] | (id[5] << 8));
        info->x_resolution = static_cast<uint16_t>(id[6] | (id[7] << 8));
        info->y_resolution = static_cast<uint16_t>(id[8] | (id[9] << 8));
        info->vendor = id[10];
        info->config_version = cfg_ver;
    }
    return write_reg8(kRegStatus, 0);
}

int read(Point* points, int max_points) {
    if (!s_dev) return -1;
    uint8_t status = 0;
    if (read_reg(kRegStatus, &status, 1) != ESP_OK) return -1;
    if (!(status & 0x80)) return kNoData;
    int n = status & 0x0F;
    if (n > 5) n = 5;
    if (n > max_points) n = max_points;
    if (n > 0) {
        uint8_t buf[5 * 8];
        if (read_reg(kRegPoints, buf, static_cast<size_t>(n) * 8) != ESP_OK) return -1;
        for (int i = 0; i < n; ++i) {
            const uint8_t* p = buf + i * 8;
            points[i].id = p[0];
            points[i].x = static_cast<uint16_t>(p[1] | (p[2] << 8));
            points[i].y = static_cast<uint16_t>(p[3] | (p[4] << 8));
            points[i].size = static_cast<uint16_t>(p[5] | (p[6] << 8));
        }
    }
    write_reg8(kRegStatus, 0);
    return n;
}

}  // namespace board::touch
