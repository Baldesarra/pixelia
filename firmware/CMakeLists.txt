cmake_minimum_required(VERSION 3.22)

# Version ausschließlich aus dem Git-Tag vX.Y.Z (ohne führendes "v").
# Ohne Tag: 0.0.0-dev+<Commit>, damit klar ist, dass es keine veröffentlichte Version ist.
execute_process(
    COMMAND git describe --tags --match "v[0-9]*" --dirty
    WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
    OUTPUT_VARIABLE PIXELIA_GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
if(PIXELIA_GIT_VERSION)
    string(REGEX REPLACE "^v" "" PROJECT_VER "${PIXELIA_GIT_VERSION}")
else()
    execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
        OUTPUT_VARIABLE PIXELIA_GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    if(NOT PIXELIA_GIT_HASH)
        set(PIXELIA_GIT_HASH "unbekannt")
    endif()
    set(PROJECT_VER "0.0.0-dev+${PIXELIA_GIT_HASH}")
endif()

# Nach jedem Commit oder Branchwechsel neu konfigurieren, damit die Version im Image stimmt
set(_pixelia_git "${CMAKE_CURRENT_LIST_DIR}/../.git")
if(EXISTS "${_pixelia_git}/HEAD")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_pixelia_git}/HEAD")
    file(READ "${_pixelia_git}/HEAD" _pixelia_head)
    if(_pixelia_head MATCHES "^ref: ([^\n]+)")
        if(EXISTS "${_pixelia_git}/${CMAKE_MATCH_1}")
            set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_pixelia_git}/${CMAKE_MATCH_1}")
        endif()
    endif()
endif()

# Testvarianten (Rollback-Test) tragen das im Namen, damit sie nie mit echten Versionen verwechselt werden
if(PIXELIA_TEST)
    set(PROJECT_VER "${PROJECT_VER}-test-${PIXELIA_TEST}")
endif()

# Plattformunabhängige Engine liegt außerhalb von firmware/ (auch für den Simulator)
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../engine")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Einheitlich C++20 (Firmware und Simulator)
idf_build_set_property(CXX_COMPILE_OPTIONS "-std=gnu++20" APPEND)

project(pixelia)
