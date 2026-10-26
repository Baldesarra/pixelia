# Pixelia-Simulator: gleiche Engine und Szenen wie auf dem Gerät, am PC.
# - Fenster-Modus mit SDL3 (per FetchContent geladen, fest auf eine geprüfte Version)
# - Kopflos-Modus für CI und Langzeit-Tests (Screenshot, Zeitraffer, Kontaktbogen), braucht kein SDL
cmake_minimum_required(VERSION 3.22)
project(pixelia_sim C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(PIXELIA_SIM_WINDOW "Fenster-Modus mit SDL3 bauen" ON)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../engine ${CMAKE_BINARY_DIR}/engine)

add_executable(pixelia-sim src/main.cpp src/headless.cpp src/tests.cpp src/world.cpp)
target_link_libraries(pixelia-sim PRIVATE pixelia_engine)
if(MINGW)
    # Ohne mitgelieferte DLLs lauffähig (libc++, libunwind)
    target_link_options(pixelia-sim PRIVATE -static)
endif()

if(PIXELIA_SIM_WINDOW)
    include(FetchContent)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(SDL3
        URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.16/SDL3-3.4.16.tar.gz
        URL_HASH SHA256=7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(SDL3)
    target_sources(pixelia-sim PRIVATE src/window.cpp)
    target_link_libraries(pixelia-sim PRIVATE SDL3::SDL3-static)
    target_compile_definitions(pixelia-sim PRIVATE PIXELIA_SIM_WINDOW=1)
endif()
