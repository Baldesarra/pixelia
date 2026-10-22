#pragma once
// Pinbelegung und Panel-Startwerte Sunton ESP32-8048S050C_I.
// Prüfstand und Herkunft der Werte: docs/HARDWARE.md

#include "driver/gpio.h"

namespace board {

// RGB-Panel, 800×480, RGB565 parallel
inline constexpr int kLcdWidth = 800;
inline constexpr int kLcdHeight = 480;
inline constexpr gpio_num_t kLcdDe = GPIO_NUM_40;
inline constexpr gpio_num_t kLcdVsync = GPIO_NUM_41;
inline constexpr gpio_num_t kLcdHsync = GPIO_NUM_39;
inline constexpr gpio_num_t kLcdPclk = GPIO_NUM_42;
// Datenleitungen D0..D15 = B0..B4, G0..G5, R0..R4 (RGB565: Bit 0 = B0)
inline constexpr gpio_num_t kLcdData[16] = {
    GPIO_NUM_8,  GPIO_NUM_3,  GPIO_NUM_46, GPIO_NUM_9,  GPIO_NUM_1,                 // B0..B4
    GPIO_NUM_5,  GPIO_NUM_6,  GPIO_NUM_7,  GPIO_NUM_15, GPIO_NUM_16, GPIO_NUM_4,   // G0..G5
    GPIO_NUM_45, GPIO_NUM_48, GPIO_NUM_47, GPIO_NUM_21, GPIO_NUM_14,               // R0..R4
};

// Hintergrundbeleuchtung (PWM)
inline constexpr gpio_num_t kBacklight = GPIO_NUM_2;

// Touch GT911 (I²C)
inline constexpr gpio_num_t kTouchSda = GPIO_NUM_19;
inline constexpr gpio_num_t kTouchScl = GPIO_NUM_20;
inline constexpr gpio_num_t kTouchRst = GPIO_NUM_38;

// microSD-Halter (SPI2, Standardpins, laut Community-Angabe; seit 0.5.5 im Treiber, ohne Kartenerkennung)
inline constexpr gpio_num_t kSdCs = GPIO_NUM_10;
inline constexpr gpio_num_t kSdMosi = GPIO_NUM_11;
inline constexpr gpio_num_t kSdClk = GPIO_NUM_12;
inline constexpr gpio_num_t kSdMiso = GPIO_NUM_13;

}  // namespace board
