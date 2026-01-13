// =============================================================================
// ui.c - Core UI Management
// =============================================================================
// Main screen management, navigation, and event loop.
// This is the core module that coordinates all other UI modules.
// =============================================================================

#if defined(EEZ_FOR_LVGL)
#include <eez/core/vars.h>
#endif

#include "ui.h"
#include "ui_internal.h"
#include "ui_nfc.h"
#include "ui_nfc_card.h"
#include "screens.h"
#include "images.h"
#include "actions.h"
#include "vars.h"
#include <stdio.h>
#include <string.h>

#ifdef ESP_PLATFORM
#include "esp_log.h"
static const char *TAG = "ui";
#define UI_LOGI(fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)
#else
#define UI_LOGI(fmt, ...) printf("[ui] " fmt "\n", ##__VA_ARGS__)
#endif

// =============================================================================
// IMPORTANT: STALE POINTER WARNING
// =============================================================================
//
// The EEZ-generated `objects` struct contains pointers to ALL widgets across
// ALL screens. However, we only keep ONE screen in memory at a time to save RAM.
// When a screen is deleted via delete_all_screens(), its child widget pointers
// in `objects` become STALE (pointing to freed memory).
//
// RULE: Only access `objects.xxx` if the parent screen is currently active.
//
// SAFE pattern:
//   int screen_id = currentScreen + 1;
//   if (screen_id == SCREEN_ID_SETTINGS) {
//       // Safe: settings screen is active, its children exist
//       lv_label_set_text(objects.obj230, "text");
//   }
//
// UNSAFE pattern (will crash or corrupt memory):
//   if (objects.wifi_signal_sd_wifi) {  // WRONG: pointer is non-NULL but FREED
//       lv_obj_set_style_...            // Accessing freed memory!
//   }
//
// The NULL check doesn't help because delete_all_screens() only NULLs the
// screen pointers, not every child widget pointer.
//
// When adding new code that accesses objects:
// 1. Identify which screen owns the object
// 2. Check if that screen is currently active before accessing
// 3. Or access only in screen-specific wire_*() / create_*() functions
//
// =============================================================================

#if defined(EEZ_FOR_LVGL)

void ui_init() {
    eez_flow_init(assets, sizeof(assets), (lv_obj_t **)&objects, sizeof(objects), images, sizeof(images), actions);
}

void ui_tick() {
    eez_flow_tick();
    tick_screen(g_currentScreen);
}

#else

// =============================================================================
// Shared Global Variables
// =============================================================================

int16_t currentScreen = -1;
enum ScreensEnum pendingScreen = 0;
enum ScreensEnum previousScreen = SCREEN_ID_MAIN_SCREEN;
const char *pending_settings_detail_title = NULL;
int pending_settings_tab = -1;  // -1 = no change, 0-3 = select tab

// =============================================================================
// Internal Helpers
// =============================================================================

static lv_obj_t *getLvglObjectFromIndex(int32_t index) {
    if (index == -1) return 0;
    return ((lv_obj_t **)&objects)[index];
}

// =============================================================================
// Screen Loading
// =============================================================================

void loadScreen(enum ScreensEnum screenId) {
    currentScreen = screenId - 1;
    lv_obj_t *screen = NULL;

    // Map screen IDs to screen objects
    switch (screenId) {
        case SCREEN_ID_MAIN_SCREEN: screen = objects.main_screen; break;
        case SCREEN_ID_AMS_OVERVIEW: screen = objects.ams_overview; break;
        case SCREEN_ID_SCAN_RESULT: screen = objects.scan_result; break;
        case SCREEN_ID_SPOOL_DETAILS: screen = objects.spool_details; break;
        case SCREEN_ID_SETTINGS_SCREEN: screen = objects.settings_screen; break;
        case SCREEN_ID_SETTINGS_WIFI_SCREEN: screen = objects.settings_wifi_screen; break;
        case SCREEN_ID_SETTINGS_PRINTER_ADD_SCREEN: screen = objects.settings_printer_add_screen; break;
        case SCREEN_ID_SETTINGS_DISPLAY_SCREEN: screen = objects.settings_display_screen; break;
        case SCREEN_ID_SETTINGS_UPDATE_SCREEN: screen = objects.settings_update_screen; break;
        default: screen = getLvglObjectFromIndex(currentScreen); break;
    }

    if (screen) {
        lv_screen_load(screen);
        lv_obj_invalidate(screen);
        lv_refr_now(NULL);
    }
}

// =============================================================================
// Navigation Event Handlers
// =============================================================================

static void ams_setup_click_handler(lv_event_t *e) {
    pendingScreen = SCREEN_ID_AMS_OVERVIEW;
}

static void home_click_handler(lv_event_t *e) {
    pendingScreen = SCREEN_ID_MAIN_SCREEN;
}

static void encode_tag_click_handler(lv_event_t *e) {
    pendingScreen = SCREEN_ID_SCAN_RESULT;
}

static void catalog_click_handler(lv_event_t *e) {
    pendingScreen = SCREEN_ID_SPOOL_DETAILS;
}

static void settings_click_handler(lv_event_t *e) {
    pendingScreen = SCREEN_ID_SETTINGS_SCREEN;
}

// Exported for use by ui_settings.c
void back_click_handler(lv_event_t *e) {
    pendingScreen = previousScreen;
}

// =============================================================================
// Navigation Routing
// =============================================================================

void navigate_to_settings_detail(const char *title) {
    pending_settings_detail_title = title;

    // Route to specific settings screens based on title
    // Note: New EEZ design has fewer screens - some are consolidated
    if (strcmp(title, "WiFi Network") == 0 || strcmp(title, "WiFi") == 0) {
        pendingScreen = SCREEN_ID_SETTINGS_WIFI_SCREEN;
    } else if (strcmp(title, "Add Printer") == 0 || strcmp(title, "Printers") == 0) {
        pendingScreen = SCREEN_ID_SETTINGS_PRINTER_ADD_SCREEN;
    } else if (strcmp(title, "Display") == 0) {
        pendingScreen = SCREEN_ID_SETTINGS_DISPLAY_SCREEN;
    } else if (strcmp(title, "Firmware Update") == 0 || strcmp(title, "Check for Updates") == 0) {
        pendingScreen = SCREEN_ID_SETTINGS_UPDATE_SCREEN;
    } else {
        // Fallback to main settings screen for unsupported detail pages
        pendingScreen = SCREEN_ID_SETTINGS_SCREEN;
    }
}

// =============================================================================
// Screen Wiring Functions
// =============================================================================

void wire_main_buttons(void) {
    if (objects.main_screen_button_ams_setup) {
        lv_obj_add_event_cb(objects.main_screen_button_ams_setup, ams_setup_click_handler, LV_EVENT_CLICKED, NULL);
    }
    if (objects.main_screen_button_encode_tag) {
        lv_obj_add_event_cb(objects.main_screen_button_encode_tag, encode_tag_click_handler, LV_EVENT_CLICKED, NULL);
    }
    if (objects.main_screen_button_catalog) {
        lv_obj_add_event_cb(objects.main_screen_button_catalog, catalog_click_handler, LV_EVENT_CLICKED, NULL);
    }
    if (objects.main_screen_button_settings) {
        lv_obj_add_event_cb(objects.main_screen_button_settings, settings_click_handler, LV_EVENT_CLICKED, NULL);
    }

    // Wire printer selection dropdown
    wire_printer_dropdown();

    // Hide static AMS placeholder content immediately
    init_main_screen_ams();
}

void wire_ams_overview_buttons(void) {
    if (objects.ams_screen_button_home) {
        lv_obj_add_event_cb(objects.ams_screen_button_home, home_click_handler, LV_EVENT_CLICKED, NULL);
    }
    if (objects.ams_screen_button_encode_tag) {
        lv_obj_add_event_cb(objects.ams_screen_button_encode_tag, encode_tag_click_handler, LV_EVENT_CLICKED, NULL);
    }
    if (objects.ams_screen_button_catalog) {
        lv_obj_add_event_cb(objects.ams_screen_button_catalog, catalog_click_handler, LV_EVENT_CLICKED, NULL);
    }
    if (objects.ams_screen_button_settings) {
        lv_obj_add_event_cb(objects.ams_screen_button_settings, settings_click_handler, LV_EVENT_CLICKED, NULL);
    }

    // Wire printer selection dropdown
    wire_ams_printer_dropdown();
}

void wire_scan_result_buttons(void) {
    // Back button - make it clickable
    if (objects.scan_screen_top_bar_icon_back) {
        lv_obj_add_flag(objects.scan_screen_top_bar_icon_back, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(objects.scan_screen_top_bar_icon_back, back_click_handler, LV_EVENT_CLICKED, NULL);
    }
}

void wire_spool_details_buttons(void) {
    // Back button - make it clickable
    if (objects.spool_screen_top_bar_icon_back) {
        lv_obj_add_flag(objects.spool_screen_top_bar_icon_back, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(objects.spool_screen_top_bar_icon_back, back_click_handler, LV_EVENT_CLICKED, NULL);
    }
}

// =============================================================================
// Screen Lifecycle
// =============================================================================

void delete_all_screens(void) {
    // Clear module state via cleanup functions
    ui_wifi_cleanup();
    ui_printer_cleanup();
    ui_nfc_card_cleanup();       // Clear NFC card dynamic elements
    reset_notification_state();  // Clear notification dots before deleting screens
    reset_backend_ui_state();    // Clear all dynamic UI state (AMS widgets, labels, etc.)

    lv_obj_t **screens[] = {
        &objects.main_screen,
        &objects.ams_overview,
        &objects.scan_result,
        &objects.spool_details,
        &objects.settings_screen,
        &objects.settings_wifi_screen,
        &objects.settings_printer_add_screen,
        &objects.settings_display_screen,
        &objects.settings_update_screen,
    };
    for (size_t i = 0; i < sizeof(screens)/sizeof(screens[0]); i++) {
        if (*screens[i]) {
            lv_obj_delete(*screens[i]);
            *screens[i] = NULL;
        }
    }
}

// =============================================================================
// Main Entry Points
// =============================================================================

void ui_init() {
    // Load saved printers from NVS
    load_printers_from_nvs();

    // Initialize theme
    lv_display_t *dispp = lv_display_get_default();
    if (dispp) {
        lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), true, LV_FONT_DEFAULT);
        lv_display_set_theme(dispp, theme);
    }

    // Create main screen
    create_screen_main_screen();
    wire_main_buttons();
    ui_nfc_card_init();
    loadScreen(SCREEN_ID_MAIN_SCREEN);
}

static int tick_count = 0;
void ui_tick() {
    tick_count++;
    if (tick_count % 500 == 0) {
        UI_LOGI("ui_tick #%d", tick_count);
    }
    if (pendingScreen != 0) {
        enum ScreensEnum screen = pendingScreen;
        pendingScreen = 0;

        // Track previous screen for back navigation from settings
        // Only update when entering settings from a non-settings screen
        enum ScreensEnum currentScreenId = (enum ScreensEnum)(currentScreen + 1);
        if (screen == SCREEN_ID_SETTINGS_SCREEN) {
            // Only save previous screen if coming from main-level screens
            if (currentScreenId == SCREEN_ID_MAIN_SCREEN ||
                currentScreenId == SCREEN_ID_AMS_OVERVIEW ||
                currentScreenId == SCREEN_ID_SCAN_RESULT ||
                currentScreenId == SCREEN_ID_SPOOL_DETAILS) {
                previousScreen = currentScreenId;
            }
        }

        // Delete old screen and create new one
        delete_all_screens();

        switch (screen) {
            case SCREEN_ID_MAIN_SCREEN:
                create_screen_main_screen();
                wire_main_buttons();
                ui_nfc_card_init();
                break;
            case SCREEN_ID_AMS_OVERVIEW:
                create_screen_ams_overview();
                wire_ams_overview_buttons();
                break;
            case SCREEN_ID_SCAN_RESULT:
                create_screen_scan_result();
                wire_scan_result_buttons();
                ui_scan_result_init();
                break;
            case SCREEN_ID_SPOOL_DETAILS:
                create_screen_spool_details();
                wire_spool_details_buttons();
                break;
            case SCREEN_ID_SETTINGS_SCREEN:
                create_screen_settings_screen();
                wire_settings_buttons();
                wire_printers_tab();
                update_wifi_ui_state();
                if (pending_settings_tab >= 0) {
                    select_settings_tab(pending_settings_tab);
                    pending_settings_tab = -1;
                }
                break;
            case SCREEN_ID_SETTINGS_WIFI_SCREEN:
                create_screen_settings_wifi_screen();
                wire_settings_subpage_buttons(objects.settings_wifi_screen_top_bar_icon_back);
                wire_wifi_settings_buttons();
                break;
            case SCREEN_ID_SETTINGS_PRINTER_ADD_SCREEN:
                create_screen_settings_printer_add_screen();
                wire_settings_subpage_buttons(objects.settings_printer_add_screen_top_bar_icon_back);
                wire_printer_add_buttons();
                break;
            case SCREEN_ID_SETTINGS_DISPLAY_SCREEN:
                create_screen_settings_display_screen();
                wire_settings_subpage_buttons(objects.settings_display_screen_top_bar_icon_back);
                break;
            case SCREEN_ID_SETTINGS_UPDATE_SCREEN:
                create_screen_settings_update_screen();
                wire_settings_subpage_buttons(objects.settings_update_screen_top_bar_icon_back);
                wire_update_buttons();
                break;
        }

        loadScreen(screen);

        // Force immediate backend UI update for the new screen
        // This ensures clock, dropdown, etc. are populated immediately
        update_backend_ui();
    }

    // Poll WiFi status (every ~50 ticks = 250ms)
    static int wifi_poll_counter = 0;
    wifi_poll_counter++;
    if (wifi_poll_counter >= 50) {
        wifi_poll_counter = 0;

        // Update WiFi settings screen if active
        int screen_id = currentScreen + 1;
        if (screen_id == SCREEN_ID_SETTINGS_SCREEN || screen_id == SCREEN_ID_SETTINGS_WIFI_SCREEN) {
            update_wifi_ui_state();
        }

        // Update firmware update screen if active
        update_firmware_ui();

        // Update backend status UI (main screen printer info)
        UI_LOGI("Calling update_backend_ui");
        update_backend_ui();
        UI_LOGI("update_backend_ui returned");

        // Update NFC card on main screen
        if (screen_id == SCREEN_ID_MAIN_SCREEN) {
            ui_nfc_card_update();
        }

        // Update NFC status and weight on scan_result screen
        if (screen_id == SCREEN_ID_SCAN_RESULT) {
            ui_nfc_update();
            ui_scan_result_update();
        }

        // Update WiFi icon for CURRENT screen only (other screen objects are freed)
        WifiStatus status;
        wifi_get_status(&status);

        // Get the WiFi icon for the current screen
        lv_obj_t *wifi_icon = NULL;
        switch (screen_id) {
            case SCREEN_ID_MAIN_SCREEN:
                wifi_icon = objects.top_bar_wifi_signal;
                break;
            case SCREEN_ID_AMS_OVERVIEW:
                wifi_icon = objects.ams_screen_top_bar_wifi_signal;
                break;
            case SCREEN_ID_SCAN_RESULT:
                wifi_icon = objects.scan_screen_top_bar_icon_wifi_signal;
                break;
            case SCREEN_ID_SPOOL_DETAILS:
                wifi_icon = objects.spool_screen_top_bar_icon_wifi_signal;
                break;
            case SCREEN_ID_SETTINGS_SCREEN:
                wifi_icon = objects.settings_screen_top_bar_icon_wifi_signal;
                break;
            case SCREEN_ID_SETTINGS_WIFI_SCREEN:
                wifi_icon = objects.settings_wifi_screen_top_bar_icon_wifi_signal;
                break;
            case SCREEN_ID_SETTINGS_PRINTER_ADD_SCREEN:
                wifi_icon = objects.settings_printer_add_screen_top_bar_icon_wifi_signal;
                break;
            case SCREEN_ID_SETTINGS_DISPLAY_SCREEN:
                wifi_icon = objects.settings_display_screen_top_bar_icon_wifi_signal;
                break;
            case SCREEN_ID_SETTINGS_UPDATE_SCREEN:
                wifi_icon = objects.settings_update_screen_top_bar_icon_wifi_signal;
                break;
            default:
                break;
        }

        // Style the WiFi icon based on connection state and signal strength
        if (wifi_icon) {
            if (status.state == 3) {
                // Connected - color based on RSSI signal strength
                int8_t rssi = status.rssi;
                lv_color_t color;
                if (rssi > -50) {
                    color = lv_color_hex(0xff00ff00);  // Excellent - bright green
                } else if (rssi > -65) {
                    color = lv_color_hex(0xff88ff00);  // Good - yellow-green
                } else if (rssi > -75) {
                    color = lv_color_hex(0xffffaa00);  // Fair - orange/yellow
                } else {
                    color = lv_color_hex(0xffff5555);  // Poor - red
                }
                lv_obj_set_style_image_recolor(wifi_icon, color, LV_PART_MAIN);
                lv_obj_set_style_image_recolor_opa(wifi_icon, 255, LV_PART_MAIN);
                lv_obj_set_style_opa(wifi_icon, 255, LV_PART_MAIN);
            } else if (status.state == 2) {
                // Connecting - yellow, full opacity
                lv_obj_set_style_image_recolor(wifi_icon, lv_color_hex(0xffffaa00), LV_PART_MAIN);
                lv_obj_set_style_image_recolor_opa(wifi_icon, 255, LV_PART_MAIN);
                lv_obj_set_style_opa(wifi_icon, 255, LV_PART_MAIN);
            } else {
                // Disconnected - dimmed (30% opacity), no recolor
                lv_obj_set_style_image_recolor_opa(wifi_icon, 0, LV_PART_MAIN);
                lv_obj_set_style_opa(wifi_icon, 80, LV_PART_MAIN);
            }
        }
    }

    tick_screen(currentScreen);
}

#endif
