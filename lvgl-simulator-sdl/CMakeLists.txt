cmake_minimum_required(VERSION 3.14)
project(spoolbuddy_simulator C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Option to enable/disable backend client (requires libcurl)
option(ENABLE_BACKEND_CLIENT "Enable HTTP backend client (requires libcurl)" ON)

# Disable LVGL examples and demos (not needed for simulator)
set(LV_CONF_BUILD_DISABLE_EXAMPLES ON CACHE BOOL "" FORCE)
set(LV_CONF_BUILD_DISABLE_DEMOS ON CACHE BOOL "" FORCE)

# Find SDL2 - handle macOS Homebrew and Linux
if(APPLE)
    # macOS with Homebrew
    execute_process(
        COMMAND brew --prefix sdl2
        OUTPUT_VARIABLE SDL2_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(SDL2_INCLUDE_DIRS "${SDL2_PREFIX}/include")
    set(SDL2_LIBRARIES "-L${SDL2_PREFIX}/lib -lSDL2")
else()
    # Linux - use pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
endif()

# Backend client dependencies (optional)
if(ENABLE_BACKEND_CLIENT)
    # Find libcurl for HTTP backend communication
    find_package(CURL QUIET)
    if(NOT CURL_FOUND)
        message(WARNING "libcurl not found - backend client disabled. Install with: brew install curl")
        set(ENABLE_BACKEND_CLIENT OFF)
    else()
        # Find or fetch cJSON for JSON parsing
        find_package(cJSON QUIET)
        if(NOT cJSON_FOUND)
            # Fetch cJSON if not found
            include(FetchContent)
            FetchContent_Declare(
                cJSON
                GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
                GIT_TAG v1.7.17
            )
            set(ENABLE_CJSON_TEST OFF CACHE BOOL "" FORCE)
            set(BUILD_SHARED_AND_STATIC_LIBS OFF CACHE BOOL "" FORCE)
            FetchContent_MakeAvailable(cJSON)
            # After MakeAvailable, cjson_SOURCE_DIR is set automatically
            set(CJSON_INCLUDE_DIR ${cjson_SOURCE_DIR})
            message(STATUS "cJSON fetched to: ${CJSON_INCLUDE_DIR}")
        else()
            # System cJSON found
            get_target_property(CJSON_INCLUDE_DIR cjson INTERFACE_INCLUDE_DIRECTORIES)
        endif()
        add_compile_definitions(ENABLE_BACKEND_CLIENT)
    endif()
endif()

# LVGL configuration
set(LV_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE STRING "" FORCE)
add_compile_definitions(LV_CONF_INCLUDE_SIMPLE)
add_compile_definitions(LV_LVGL_H_INCLUDE_SIMPLE)

# LVGL library
add_subdirectory(lvgl)

# Collect UI source files
file(GLOB UI_SOURCES "ui/*.c")

# Main executable sources
set(SIMULATOR_SOURCES main.c ${UI_SOURCES})
if(ENABLE_BACKEND_CLIENT)
    list(APPEND SIMULATOR_SOURCES backend_client.c)
endif()

add_executable(simulator ${SIMULATOR_SOURCES})

# Include directories (esp_stubs FIRST to override ESP-IDF headers)
target_include_directories(simulator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/esp_stubs
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/lvgl
    ${SDL2_INCLUDE_DIRS}
)

if(ENABLE_BACKEND_CLIENT)
    message(STATUS "Backend client enabled, cJSON include: ${CJSON_INCLUDE_DIR}")
    target_include_directories(simulator PRIVATE
        ${CURL_INCLUDE_DIRS}
        ${CJSON_INCLUDE_DIR}
    )
endif()

# Link libraries
target_link_libraries(simulator
    lvgl
    ${SDL2_LIBRARIES}
    m
)

# Add pthread on non-Apple platforms
if(NOT APPLE)
    target_link_libraries(simulator pthread)
endif()

# Backend client libraries
if(ENABLE_BACKEND_CLIENT)
    target_link_libraries(simulator ${CURL_LIBRARIES} cjson)
endif()

# Suppress warnings from generated code
target_compile_options(simulator PRIVATE -w)
