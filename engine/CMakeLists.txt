# Pixelia-Engine: plattformunabhängig (keine ESP-IDF-Header).
# Unter ESP-IDF als Komponente eingebunden, sonst als normale Bibliothek (Simulator).
set(PIXELIA_ENGINE_SRCS
    src/canvas.cpp
    src/color.cpp
    src/depth.cpp
    src/font.cpp
    src/gesture.cpp
    src/light.cpp
    src/memory.cpp
    src/png.cpp
    src/qr.cpp
    src/redact.cpp
    src/room.cpp
    src/scenes.cpp
    src/scroll_test.cpp
    src/sprite.cpp
    src/stage.cpp
    src/system_ui.cpp
    src/test_places.cpp
    src/tiles.cpp
    src/tiny_font.cpp)

if(ESP_PLATFORM)
    idf_component_register(SRCS ${PIXELIA_ENGINE_SRCS}
                           INCLUDE_DIRS include)
else()
    cmake_minimum_required(VERSION 3.22)
    project(pixelia_engine CXX)
    add_library(pixelia_engine STATIC ${PIXELIA_ENGINE_SRCS})
    target_include_directories(pixelia_engine PUBLIC include)
    target_compile_features(pixelia_engine PUBLIC cxx_std_20)
endif()
