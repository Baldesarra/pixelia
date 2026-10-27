# Pixelia-Engine: plattformunabhängig (keine ESP-IDF-Header).
# Unter ESP-IDF als Komponente eingebunden, sonst als normale Bibliothek (Simulator).
set(PIXELIA_ENGINE_SRCS
    src/assets.cpp
    src/calendar.cpp
    src/canvas.cpp
    src/chronicle.cpp
    src/color.cpp
    src/depth.cpp
    src/environment.cpp
    src/events.cpp
    src/figure.cpp
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
    src/save.cpp
    src/place.cpp
    src/place_state.cpp
    src/scenes.cpp
    src/scroll_test.cpp
    src/sky.cpp
    src/sprite.cpp
    src/stage.cpp
    src/system_ui.cpp
    src/tiles.cpp
    src/tiny_font.cpp
    src/vehicle.cpp
    src/weather.cpp
    src/weather_sim.cpp
    src/world_save.cpp)

# Welten (seit 0.5.8): Orte und Gebiete als Module in worlds/<gebiet>/, jede .cpp dort wird mitgebaut
if(CMAKE_BUILD_EARLY_EXPANSION)  # ESP-IDF liest die Datei zuerst im Skript-Modus: dort ohne CONFIGURE_DEPENDS
    file(GLOB_RECURSE PIXELIA_WORLD_SRCS "${CMAKE_CURRENT_LIST_DIR}/../worlds/*.cpp")
else()
    file(GLOB_RECURSE PIXELIA_WORLD_SRCS CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/../worlds/*.cpp")
endif()
list(APPEND PIXELIA_ENGINE_SRCS ${PIXELIA_WORLD_SRCS})
set(PIXELIA_WORLDS_DIR "${CMAKE_CURRENT_LIST_DIR}/../worlds")

# Eingebettete Asset-Pakete (seit 0.5.5): tools/asset_pack.py baut sie aus assets/**/pakete.json und den PNGs
set(PIXELIA_ASSETS_DIR "${CMAKE_CURRENT_LIST_DIR}/../assets")
set(PIXELIA_ASSET_TOOL "${CMAKE_CURRENT_LIST_DIR}/../tools/asset_pack.py")
set(PIXELIA_ASSET_BLOBS "${CMAKE_CURRENT_BINARY_DIR}/asset_blobs.cpp")

if(ESP_PLATFORM)
    idf_component_register(SRCS ${PIXELIA_ENGINE_SRCS} "${PIXELIA_ASSET_BLOBS}"
                           INCLUDE_DIRS include "${PIXELIA_WORLDS_DIR}")
    if(NOT CMAKE_BUILD_EARLY_EXPANSION)
        idf_build_get_property(_pixelia_python PYTHON)
    endif()
else()
    cmake_minimum_required(VERSION 3.22)
    project(pixelia_engine CXX)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(_pixelia_python "${Python3_EXECUTABLE}")
    add_library(pixelia_engine STATIC ${PIXELIA_ENGINE_SRCS} "${PIXELIA_ASSET_BLOBS}")
    target_include_directories(pixelia_engine PUBLIC include "${PIXELIA_WORLDS_DIR}")
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
