idf_component_register(
    SRCS "main.cpp" "sysinfo.cpp" "hwtest.cpp" "gfx.cpp" "perf.cpp" "console.cpp"
         "settings.cpp" "wifi.cpp" "captive_dns.cpp" "timesync.cpp" "web.cpp" "ota.cpp"
         "input.cpp"
    INCLUDE_DIRS "."
    EMBED_TXTFILES "web/setup.html"
    REQUIRES board engine
    PRIV_REQUIRES esp_driver_uart esp_driver_i2c esp_psram esp_timer spi_flash app_update esp_app_format
                  espcoredump esp_partition bootloader_support esp_bootloader_format nvs_flash esp_wifi esp_netif
                  esp_event esp_http_server lwip)

# Testvarianten für den Rollback-Test, nur lokal: idf.py -B build-test -DPIXELIA_TEST=crash|hang|selftest build
# crash = Absturz 20 s nach dem Start im Probelauf, hang = Hänger (Watchdog), selftest = Selbsttest scheitert
if(PIXELIA_TEST)
    string(TOUPPER "${PIXELIA_TEST}" _pixelia_test)
    if(_pixelia_test STREQUAL "SELFTEST")
        set(_pixelia_test "SELFTEST_FAIL")
    endif()
    target_compile_definitions(${COMPONENT_LIB} PRIVATE "PIXELIA_TEST_${_pixelia_test}=1")
    message(WARNING "Pixelia-Testvariante: ${PIXELIA_TEST} (nicht veröffentlichen!)")
endif()
