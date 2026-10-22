# Weboberfläche (web/, Svelte): tools/web_embed.py baut sie bei Bedarf und erzeugt web_assets.cpp (gzip-Daten)
set(PIXELIA_WEB_DIR "${CMAKE_CURRENT_LIST_DIR}/../../web")
set(PIXELIA_WEB_ASSETS "${CMAKE_CURRENT_BINARY_DIR}/web_assets.cpp")
# Versionsverlauf: tools/release_notes.py --embed erzeugt aus CHANGELOG.json den Block für /api/changelog
set(PIXELIA_CHANGELOG "${CMAKE_CURRENT_LIST_DIR}/../../CHANGELOG.json")
set(PIXELIA_CHANGELOG_BLOCK "${CMAKE_CURRENT_BINARY_DIR}/changelog_block.cpp")

idf_component_register(
    SRCS "main.cpp" "sysinfo.cpp" "hwtest.cpp" "gfx.cpp" "perf.cpp" "console.cpp"
         "settings.cpp" "wifi.cpp" "captive_dns.cpp" "timesync.cpp" "web.cpp" "web_diag.cpp" "web_info.cpp" "web_exp.cpp" "exper.cpp" "ota.cpp" "github.cpp"
         "input.cpp" "logbuf.cpp" "assets.cpp" "diag.cpp" "json.cpp" "auth.cpp" "${PIXELIA_WEB_ASSETS}" "${PIXELIA_CHANGELOG_BLOCK}"
    INCLUDE_DIRS "."
    EMBED_TXTFILES "web/setup.html"
    REQUIRES board engine
    PRIV_REQUIRES esp_driver_uart esp_driver_i2c esp_psram esp_timer spi_flash app_update esp_app_format
                  espcoredump esp_partition bootloader_support esp_bootloader_format nvs_flash esp_wifi esp_netif
                  esp_event esp_http_server lwip esp_mm hal esp_driver_tsens mbedtls esp_http_client esp_driver_dma
                  fatfs sdmmc esp_driver_sdspi esp_driver_spi joltwallet__littlefs)

if(NOT CMAKE_BUILD_EARLY_EXPANSION)
file(GLOB_RECURSE _web_src CONFIGURE_DEPENDS "${PIXELIA_WEB_DIR}/src/*")
file(GLOB _web_dist CONFIGURE_DEPENDS "${PIXELIA_WEB_DIR}/dist/*")
idf_build_get_property(_python PYTHON)
add_custom_command(
    OUTPUT "${PIXELIA_WEB_ASSETS}"
    COMMAND "${_python}" "${CMAKE_CURRENT_LIST_DIR}/../../tools/web_embed.py"
            --web "${PIXELIA_WEB_DIR}" --out "${PIXELIA_WEB_ASSETS}"
    DEPENDS ${_web_src} ${_web_dist} "${PIXELIA_WEB_DIR}/index.html" "${PIXELIA_WEB_DIR}/vite.config.ts"
            "${PIXELIA_WEB_DIR}/package.json" "${CMAKE_CURRENT_LIST_DIR}/../../tools/web_embed.py"
    COMMENT "Weboberfläche einbetten"
    VERBATIM)
add_custom_command(
    OUTPUT "${PIXELIA_CHANGELOG_BLOCK}"
    COMMAND "${_python}" "${CMAKE_CURRENT_LIST_DIR}/../../tools/release_notes.py" --embed "${PIXELIA_CHANGELOG_BLOCK}"
    DEPENDS "${PIXELIA_CHANGELOG}" "${CMAKE_CURRENT_LIST_DIR}/../../tools/release_notes.py"
    COMMENT "Versionsverlauf einbetten"
    VERBATIM)
endif()

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
