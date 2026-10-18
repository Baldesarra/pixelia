idf_component_register(
    SRCS "main.cpp" "sysinfo.cpp" "hwtest.cpp" "gfx.cpp" "perf.cpp" "console.cpp"
    INCLUDE_DIRS "."
    REQUIRES board engine
    PRIV_REQUIRES esp_driver_uart esp_driver_i2c esp_psram esp_timer spi_flash app_update esp_app_format
                  espcoredump esp_partition)
