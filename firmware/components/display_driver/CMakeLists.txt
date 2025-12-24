# SpoolBuddy Display Driver Component
# Initializes RGB LCD, touch, and LVGL 9.x

idf_component_register(
    SRCS "display_driver.c"
    INCLUDE_DIRS "."
    REQUIRES lvgl eez_ui driver esp_lcd esp_timer
)

# Include LVGL configuration
target_include_directories(${COMPONENT_LIB} PRIVATE
    "${PROJECT_DIR}/lvgl-configs"
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    LV_LVGL_H_INCLUDE_SIMPLE
    LV_CONF_INCLUDE_SIMPLE
)
