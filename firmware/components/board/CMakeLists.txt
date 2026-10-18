idf_component_register(
    SRCS "backlight.cpp" "display.cpp" "touch_gt911.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_lcd esp_driver_gpio esp_driver_i2c esp_driver_ledc esp_timer
    PRIV_REQUIRES esp_mm)
