idf_component_register(
    SRCS "main.cpp" "sysinfo.cpp" "hwtest.cpp" "gfx.cpp" "perf.cpp" "console.cpp"
         "settings.cpp" "wifi.cpp" "captive_dns.cpp" "timesync.cpp" "web.cpp" "web_diag.cpp" "ota.cpp"
         "input.cpp" "logbuf.cpp" "diag.cpp" "json.cpp"
    INCLUDE_DIRS "."
    EMBED_TXTFILES "web/setup.html"
    REQUIRES board engine
    PRIV_REQUIRES esp_driver_uart esp_driver_i2c esp_psram esp_timer spi_flash app_update esp_app_format
                  espcoredump esp_partition bootloader_support esp_bootloader_format nvs_flash esp_wifi esp_netif
                  esp_event esp_http_server lwip esp_mm hal)

# Testvarianten für den Rollback-Test, nur lokal: idf.py -B build-test -DPIXELIA_TEST=crash|hang|selftest build
# crash = Absturz 20 s nach dem Start im Probelauf, hang = Hänger (Watchdog), selftest = Selbsttest scheitert,
# crash-shift = Absturz wie crash, dazu 64 KB mehr Konstanten: der Log-Puffer im PSRAM liegt dann wie nach einem
# echten Update mit mehr Code an anderer Stelle, nach dem Rückfall muss er aus der RTC-Sicherung kommen
if(PIXELIA_TEST)
    string(TOUPPER "${PIXELIA_TEST}" _pixelia_test)
    if(_pixelia_test STREQUAL "SELFTEST")
        set(_pixelia_test "SELFTEST_FAIL")
    endif()
    if(_pixelia_test STREQUAL "CRASH-SHIFT")
        target_compile_definitions(${COMPONENT_LIB} PRIVATE "PIXELIA_TEST_CRASH=1" "PIXELIA_TEST_SHIFT=1")
    else()
        target_compile_definitions(${COMPONENT_LIB} PRIVATE "PIXELIA_TEST_${_pixelia_test}=1")
    endif()
    message(WARNING "Pixelia-Testvariante: ${PIXELIA_TEST} (nicht veröffentlichen!)")
endif()
