# Pixelia-Engine: plattformunabhängig (keine ESP-IDF-Header).
# Unter ESP-IDF als Komponente eingebunden, sonst als normale Bibliothek (Simulator).
set(PIXELIA_ENGINE_SRCS
    src/assets.cpp
    src/calendar.cpp
    src/canvas.cpp
    src/color.cpp
    src/depth.cpp
    src/environment.cpp
    src/font.cpp
    src/gesture.cpp
    src/light.cpp
    src/memory.cpp
    src/overlay.cpp
    src/camera.cpp
    src/png.cpp
    src/qr.cpp
    src/redact.cpp
    src/room.cpp
    src/scenes.cpp
    src/scroll_test.cpp
    src/sky.cpp
    src/sprite.cpp
    src/stage.cpp
    src/system_ui.cpp
    src/test_places.cpp
    src/tiles.cpp
    src/tiny_font.cpp
    src/weather.cpp
    src/weather_sim.cpp)

# Eingebettete Asset-Pakete (seit 0.5.5): tools/asset_pack.py baut sie aus assets/**/pakete.json und den PNGs
set(PIXELIA_ASSETS_DIR "${CMAKE_CURRENT_LIST_DIR}/../assets")
set(PIXELIA_ASSET_TOOL "${CMAKE_CURRENT_LIST_DIR}/../tools/asset_pack.py")
set(PIXELIA_ASSET_BLOBS "${CMAKE_CURRENT_BINARY_DIR}/asset_blobs.cpp")

if(ESP_PLATFORM)
    idf_component_register(SRCS ${PIXELIA_ENGINE_SRCS} "${PIXELIA_ASSET_BLOBS}"
                           INCLUDE_DIRS include)
    if(NOT CMAKE_BUILD_EARLY_EXPANSION)
        idf_build_get_property(_pixelia_python PYTHON)
    endif()
else()
    cmake_minimum_required(VERSION 3.22)
    project(pixelia_engine CXX)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(_pixelia_python "${Python3_EXECUTABLE}")
    add_library(pixelia_engine STATIC ${PIXELIA_ENGINE_SRCS} "${PIXELIA_ASSET_BLOBS}")
    target_include_directories(pixelia_engine PUBLIC include)
    target_compile_features(pixelia_engine PUBLIC cxx_std_20)
endif()

if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    file(GLOB_RECURSE _pixelia_asset_files CONFIGURE_DEPENDS "${PIXELIA_ASSETS_DIR}/*")
    add_custom_command(
        OUTPUT "${PIXELIA_ASSET_BLOBS}"
        COMMAND "${_pixelia_python}" "${PIXELIA_ASSET_TOOL}" build --cpp "${PIXELIA_ASSET_BLOBS}"
        DEPENDS ${_pixelia_asset_files} "${PIXELIA_ASSET_TOOL}"
        COMMENT "Asset-Pakete bauen"
        VERBATIM)
endif()
