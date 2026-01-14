/**
 * @file ui_backend.c
 * @brief Backend server communication UI integration
 *
 * Updates UI elements with printer status from the SpoolBuddy backend server.
 * Called periodically from ui_tick() to refresh displayed data.
 *
 * This file is shared between firmware and simulator.
 */

#include "screens.h"
#include <lvgl.h>
#include <stdio.h>
#include <string.h>

#ifdef ESP_PLATFORM
// Firmware: use ESP-IDF and Rust FFI backend
#include "ui_internal.h"
#include "esp_log.h"
static const char *TAG = "ui_backend";
#else
// Simulator: use libcurl backend with compatibility API
#include "../backend_client.h"
#define ESP_LOGI(tag, fmt, ...) printf("[%s] " fmt "\n", tag, ##__VA_ARGS__)
static const char *TAG = "ui_backend";
// Variables shared with ui.c
extern int16_t currentScreen;
// Functions from ui_nfc_card.c
extern void ui_nfc_card_show_popup(void);
extern bool ui_nfc_card_popup_visible(void);
#endif

// Update counter for rate limiting UI updates
static int backend_update_counter = 0;
// Track previous screen to detect navigation
static int previous_screen = -1;
// Flag to update more frequently when data is stale
static bool needs_data_refresh = true;
// Last displayed time (to avoid redundant updates)
static int last_time_hhmm = -1;
// Last printer count for dropdown update tracking
static int last_printer_count = -1;
static uint8_t last_connected_mask = 0;  // Bitmask of connected printers (up to 8)
// Cover image state
static bool cover_displayed = false;
static lv_image_dsc_t cover_img_dsc;
// Selected printer index (changed via dropdown)
static int selected_printer_index = 0;
// Track if selected printer is dual-nozzle (default false, detected from AMS data)
static bool selected_printer_is_dual_nozzle = false;
// Mapping from dropdown index to actual printer index (only connected printers in dropdown)
static int dropdown_to_printer_index[8] = {0, 1, 2, 3, 4, 5, 6, 7};
static int dropdown_printer_count = 0;

// Dynamic UI labels (created on main screen - must be reset when screen changes)
static lv_obj_t *status_eta_label = NULL;      // ETA on status row
static lv_obj_t *progress_pct_label = NULL;    // Percentage on progress bar
static lv_obj_t *last_main_screen = NULL;      // Track main screen to detect recreations
// LED animation state (must reset when main screen is recreated)
static bool led_anim_active = false;

// Forward declarations
static void update_main_screen_backend_status(BackendStatus *status);
static void update_clock_displays(void);
static void update_printer_dropdowns(BackendStatus *status);
static void update_cover_image(void);
static void update_ams_display(void);
static void update_ams_overview_display(void);
static void update_notification_bell(void);
static void update_status_bar(void);
static void update_settings_menu_indicator(void);
static void reset_main_screen_dynamic_state(void);  // Reset stale pointers on screen recreation

/**
 * @brief Update UI elements with backend printer status
 *
 * This function is called periodically from ui_tick() to refresh the UI
 * with the latest printer status from the backend server.
 */
void update_backend_ui(void) {
    // Get current screen ID
    int screen_id = currentScreen + 1;  // Convert to ScreensEnum (1-based)

    // Detect any screen change
    bool screen_changed = (screen_id != previous_screen);

    // Force immediate update when navigating to main screen
    bool force_update = (screen_id == SCREEN_ID_MAIN_SCREEN && screen_changed);
    if (force_update) {
        needs_data_refresh = true;
    }
    previous_screen = screen_id;

    // Always update notification bell on screen change (no rate limiting)
    // This ensures the notification dot appears on all screens
    if (screen_changed) {
        update_notification_bell();
        // Force dropdown update on next tick for the new screen
        // Each screen has its own dropdown which needs to be populated
        last_printer_count = -1;
        last_connected_mask = 0;
    }

    // Update status bar on every tick (not rate-limited) for responsive staging updates
    update_status_bar();

    // Rate limiting for other updates:
    // - Every 20 ticks (~100ms) when waiting for data
    // - Every 100 ticks (~500ms) when we have data
    int rate_limit = needs_data_refresh ? 20 : 100;
    if (!force_update && ++backend_update_counter < rate_limit) {
        return;
    }
    backend_update_counter = 0;

    // Get backend connection status
    BackendStatus status;
    backend_get_status(&status);

    // Check if we got valid data
    if (status.state == 2 && status.printer_count > 0) {
        needs_data_refresh = false;
    }

    // Update based on current screen
    if (screen_id == SCREEN_ID_MAIN_SCREEN) {
        update_main_screen_backend_status(&status);
        update_cover_image();
        update_ams_display();
    } else if (screen_id == SCREEN_ID_AMS_OVERVIEW) {
        update_ams_overview_display();
    }

    // Update clock on all screens
    update_clock_displays();

    // Update printer dropdowns
    update_printer_dropdowns(&status);

    // Sync saved_printers with backend data (for settings page)
    sync_printers_from_backend();

    // Update notification bell periodically (in addition to screen changes)
    update_notification_bell();

    // Note: update_status_bar() is called before rate limiting for responsiveness

    // Update settings menu firmware indicator
    update_settings_menu_indicator();
}

/**
 * @brief Format remaining time as human-readable string
 */
static void format_remaining_time(char *buf, size_t buf_size, uint16_t minutes) {
    if (minutes >= 60) {
        int hours = minutes / 60;
        int mins = minutes % 60;
        if (mins > 0) {
            snprintf(buf, buf_size, "%dh %dm left", hours, mins);
        } else {
            snprintf(buf, buf_size, "%dh left", hours);
        }
    } else if (minutes > 0) {
        snprintf(buf, buf_size, "%dm left", minutes);
    } else {
        buf[0] = '\0';  // Empty string
    }
}

/**
 * @brief Update the main screen with backend status
 */
static void update_main_screen_backend_status(BackendStatus *status) {
    char buf[64];

    // Check if main screen exists and is actually the active screen
    // This prevents crashes during screen transitions
    if (!objects.main_screen || lv_scr_act() != objects.main_screen) {
        return;
    }

    // Reset all dynamic objects if main screen was recreated
    // These static pointers become stale when screen is deleted and recreated
    if (objects.main_screen != last_main_screen) {
        reset_main_screen_dynamic_state();
        last_main_screen = objects.main_screen;
    }

    // Update printer labels if we have printer data
    if (status->state == 2 && status->printer_count > 0) {
        BackendPrinterInfo printer;

        // Update first printer info
        if (backend_get_printer(selected_printer_index, &printer) == 0) {
            // printer_label = Printer name
            if (objects.main_screen_printer_printer_name_label) {
                lv_label_set_text(objects.main_screen_printer_printer_name_label,
                    printer.name[0] ? printer.name : printer.serial);
            }

            // printer_label_1 = Status (stage name, no percentage)
            if (objects.main_screen_printer_printer_status) {
                if (printer.connected) {
                    // Use stg_cur_name if available
                    if (printer.stg_cur_name[0]) {
                        snprintf(buf, sizeof(buf), "%s", printer.stg_cur_name);
                    } else {
                        // Format gcode_state nicely
                        const char *state_str = printer.gcode_state;
                        if (strcmp(state_str, "IDLE") == 0) {
                            snprintf(buf, sizeof(buf), "Idle");
                        } else if (strcmp(state_str, "RUNNING") == 0) {
                            snprintf(buf, sizeof(buf), "Printing");
                        } else if (strcmp(state_str, "PAUSE") == 0 || strcmp(state_str, "PAUSED") == 0) {
                            snprintf(buf, sizeof(buf), "Paused");
                        } else if (strcmp(state_str, "FINISH") == 0) {
                            snprintf(buf, sizeof(buf), "Finished");
                        } else if (state_str[0]) {
                            snprintf(buf, sizeof(buf), "%s", state_str);
                        } else {
                            snprintf(buf, sizeof(buf), "Idle");
                        }
                    }
                    lv_obj_set_style_text_color(objects.main_screen_printer_printer_status,
                        lv_color_hex(0x00ff00), LV_PART_MAIN);
                } else {
                    snprintf(buf, sizeof(buf), "Offline");
                    lv_obj_set_style_text_color(objects.main_screen_printer_printer_status,
                        lv_color_hex(0xff8800), LV_PART_MAIN);
                }
                lv_label_set_text(objects.main_screen_printer_printer_status, buf);
            }

            // ETA on status row (completion time like "15:45")
            if (objects.main_screen_printer && printer.connected && printer.remaining_time_min > 0) {
                if (!status_eta_label) {
                    status_eta_label = lv_label_create(objects.main_screen_printer);
                    lv_obj_set_style_text_font(status_eta_label, &lv_font_montserrat_14, 0);
                    lv_obj_set_style_text_color(status_eta_label, lv_color_hex(0xfafafa), 0);
                }
                // Calculate ETA: current time + remaining minutes
                int time_hhmm = time_get_hhmm();
                if (time_hhmm >= 0) {
                    int hour = (time_hhmm >> 8) & 0xFF;
                    int minute = time_hhmm & 0xFF;
                    int total_min = hour * 60 + minute + printer.remaining_time_min;
                    int eta_hour = (total_min / 60) % 24;
                    int eta_min = total_min % 60;
                    snprintf(buf, sizeof(buf), "%02d:%02d", eta_hour, eta_min);
                    lv_label_set_text(status_eta_label, buf);
                    lv_obj_set_pos(status_eta_label, 400, 27);
                }
            } else if (status_eta_label) {
                lv_label_set_text(status_eta_label, "");
            }

            // printer_label_2 = File name (subtask_name)
            if (objects.main_screen_printer_filename) {
                if (printer.connected && printer.subtask_name[0]) {
                    lv_label_set_text(objects.main_screen_printer_filename, printer.subtask_name);
                } else {
                    lv_label_set_text(objects.main_screen_printer_filename, "");
                }
            }

            // obj49 = Time remaining (inline with filename at y=62)
            if (objects.main_screen_printer_time_left) {
                if (printer.connected && printer.remaining_time_min > 0) {
                    format_remaining_time(buf, sizeof(buf), printer.remaining_time_min);
                    lv_label_set_text(objects.main_screen_printer_time_left, buf);
                } else {
                    lv_label_set_text(objects.main_screen_printer_time_left, "");
                }
            }

            // Progress bar with percentage label
            if (objects.main_screen_printer_progress_bar) {
                if (printer.connected) {
                    lv_bar_set_value(objects.main_screen_printer_progress_bar, printer.print_progress, LV_ANIM_OFF);

                    // Check if printer is actively printing (not idle)
                    bool is_printing = (strcmp(printer.gcode_state, "RUNNING") == 0 ||
                                       strcmp(printer.gcode_state, "PAUSE") == 0 ||
                                       strcmp(printer.gcode_state, "PAUSED") == 0 ||
                                       printer.print_progress > 0);

                    if (is_printing) {
                        if (!progress_pct_label) {
                            progress_pct_label = lv_label_create(objects.main_screen_printer_progress_bar);
                            lv_obj_set_style_text_font(progress_pct_label, &lv_font_montserrat_12, 0);
                            lv_obj_center(progress_pct_label);
                        }
                        // Dynamic text color based on progress
                        if (printer.print_progress < 50) {
                            lv_obj_set_style_text_color(progress_pct_label, lv_color_hex(0xffffff), 0);
                        } else {
                            lv_obj_set_style_text_color(progress_pct_label, lv_color_hex(0x000000), 0);
                        }
                        snprintf(buf, sizeof(buf), "%d%%", printer.print_progress);
                        lv_label_set_text(progress_pct_label, buf);
                        lv_obj_center(progress_pct_label);
                    } else {
                        // Idle - hide percentage
                        if (progress_pct_label) {
                            lv_label_set_text(progress_pct_label, "");
                        }
                    }
                } else {
                    lv_bar_set_value(objects.main_screen_printer_progress_bar, 0, LV_ANIM_OFF);
                    if (progress_pct_label) {
                        lv_label_set_text(progress_pct_label, "");
                    }
                }
            }
        }
    } else if (status->state != 2) {
        // Not connected to backend server
        if (objects.main_screen_printer_printer_name_label) {
            lv_label_set_text(objects.main_screen_printer_printer_name_label, "No Server");
        }
        if (objects.main_screen_printer_printer_status) {
            lv_label_set_text(objects.main_screen_printer_printer_status, "Offline");
        }
        if (objects.main_screen_printer_filename) {
            lv_label_set_text(objects.main_screen_printer_filename, "");
        }
        if (objects.main_screen_printer_time_left) {
            lv_label_set_text(objects.main_screen_printer_time_left, "");
        }
    }
}

/**
 * @brief Update clock display for current screen only
 *
 * IMPORTANT: Only updates the clock for the currently active screen.
 * Other screen objects are STALE after delete_all_screens() - checking
 * for NULL doesn't help because they point to freed memory.
 */
static void update_clock_displays(void) {
    int time_hhmm = time_get_hhmm();
    int screen_id = currentScreen + 1;

    // Only update if time changed or first valid time
    if (time_hhmm < 0 || time_hhmm == last_time_hhmm) {
        return;
    }
    last_time_hhmm = time_hhmm;

    int hour = (time_hhmm >> 8) & 0xFF;
    int minute = time_hhmm & 0xFF;

    char time_str[8];
    snprintf(time_str, sizeof(time_str), "%02d:%02d", hour, minute);

    // Update clock for CURRENT screen only - other screen objects are stale!
    lv_obj_t *clock = NULL;

    switch (screen_id) {
        case SCREEN_ID_MAIN_SCREEN:
            clock = objects.top_bar_clock;
            break;
        case SCREEN_ID_AMS_OVERVIEW:
            clock = objects.ams_screen_top_bar_clock;
            break;
        case SCREEN_ID_SCAN_RESULT:
            clock = objects.scan_screen_top_bar_label_clock;
            break;
        case SCREEN_ID_SPOOL_DETAILS:
            clock = objects.spool_screen_top_bar_label_clock;
            break;
        case SCREEN_ID_SETTINGS_SCREEN:
            clock = objects.settings_screen_top_bar_label_clock;
            break;
        case SCREEN_ID_SETTINGS_WIFI_SCREEN:
            clock = objects.settings_wifi_screen_top_bar_label_clock;
            break;
        case SCREEN_ID_SETTINGS_PRINTER_ADD_SCREEN:
            clock = objects.settings_printer_add_screen_top_bar_label_clock;
            break;
        case SCREEN_ID_SETTINGS_DISPLAY_SCREEN:
            clock = objects.settings_display_screen_top_bar_label_clock;
            break;
        case SCREEN_ID_SETTINGS_UPDATE_SCREEN:
            clock = objects.settings_update_screen_top_bar_label_clock;
            break;
    }

    if (clock) {
        lv_label_set_text(clock, time_str);
    }
}

/**
 * @brief Get the printer dropdown for the current screen
 *
 * IMPORTANT: Only returns the dropdown for the currently active screen.
 * Other screen objects are STALE after delete_all_screens().
 */
static lv_obj_t* get_current_printer_dropdown(int screen_id) {
    switch (screen_id) {
        case SCREEN_ID_MAIN_SCREEN:
            return objects.top_bar_printer_select;
        case SCREEN_ID_AMS_OVERVIEW:
            return objects.ams_screen_top_bar_printer_select;
        case SCREEN_ID_SCAN_RESULT:
            return objects.scan_screen_top_bar_printer_select;
        case SCREEN_ID_SPOOL_DETAILS:
            return objects.spool_screen_top_bar_printer_select;
        case SCREEN_ID_SETTINGS_SCREEN:
            return objects.settings_screen_top_bar_printer_select;
        case SCREEN_ID_SETTINGS_WIFI_SCREEN:
            return objects.settings_wifi_screen_top_bar_printer_select;
        case SCREEN_ID_SETTINGS_PRINTER_ADD_SCREEN:
            return objects.settings_printer_add_screen_top_bar_printer_select;
        case SCREEN_ID_SETTINGS_DISPLAY_SCREEN:
            return objects.settings_display_screen_top_bar_printer_select;
        case SCREEN_ID_SETTINGS_UPDATE_SCREEN:
            return objects.settings_update_screen_top_bar_printer_select;
        default:
            return NULL;
    }
}

/**
 * @brief Update printer selection dropdown for current screen
 *
 * IMPORTANT: Only updates the dropdown for the currently active screen.
 * Other screen objects are STALE after delete_all_screens().
 */
static void update_printer_dropdowns(BackendStatus *status) {
    // Build current connected mask to detect connection status changes
    uint8_t connected_mask = 0;
    for (int i = 0; i < status->printer_count && i < 8; i++) {
        BackendPrinterInfo printer;
        if (backend_get_printer(i, &printer) == 0) {
            if (printer.connected) {
                connected_mask |= (1 << i);
            }
        }
    }

    // Only update when printer count OR connection status changes
    if (status->printer_count == last_printer_count && connected_mask == last_connected_mask) {
        return;
    }

    last_printer_count = status->printer_count;
    last_connected_mask = connected_mask;

    // Build options string with connected printer names and track mapping
    char options[256] = "";
    int pos = 0;
    dropdown_printer_count = 0;

    for (int i = 0; i < status->printer_count && i < 8; i++) {
        BackendPrinterInfo printer;
        if (backend_get_printer(i, &printer) == 0 && printer.connected) {
            // Track mapping: dropdown index -> actual printer index
            dropdown_to_printer_index[dropdown_printer_count] = i;
            dropdown_printer_count++;

            if (pos > 0) {
                options[pos++] = '\n';
            }
            const char *name = printer.name[0] ? printer.name : printer.serial;
            int len = strlen(name);
            if (pos + len < sizeof(options) - 1) {
                strcpy(&options[pos], name);
                pos += len;
            }
        }
    }

    // If no connected printers, show placeholder
    if (pos == 0) {
        strcpy(options, "No Printers");
    }

    // Update ONLY the current screen's dropdown - other screen objects are stale!
    int screen_id = currentScreen + 1;
    lv_obj_t *dropdown = get_current_printer_dropdown(screen_id);
    if (dropdown) {
        lv_dropdown_set_options(dropdown, options);

        // Set selected index to match selected_printer_index
        // Find which dropdown index corresponds to the selected printer
        int dropdown_idx = 0;
        for (int i = 0; i < dropdown_printer_count; i++) {
            if (dropdown_to_printer_index[i] == selected_printer_index) {
                dropdown_idx = i;
                break;
            }
        }
        lv_dropdown_set_selected(dropdown, dropdown_idx);
    }
}

// Cover image dimensions (must match backend COVER_SIZE - 70x70 as per EEZ design)
#define COVER_WIDTH 70
#define COVER_HEIGHT 70

/**
 * @brief Update cover image from downloaded raw RGB565 data
 *
 * EEZ design specifies:
 * - Size: 70x70
 * - Border: 2px, color #3d3d3d
 * - Shadow: width=5, offset 2x2, spread=2, opa=100
 */
static void update_cover_image(void) {
    if (!objects.main_screen_printer_print_cover) {
        return;
    }

    if (backend_has_cover()) {
        if (!cover_displayed) {
            // Get cover data from Rust (raw RGB565 pixels)
            uint32_t size = 0;
            const uint8_t *data = backend_get_cover_data(&size);

            // Verify size matches expected RGB565 data (70x70x2 = 9800 bytes)
            uint32_t expected_size = COVER_WIDTH * COVER_HEIGHT * 2;
            if (data && size == expected_size) {
                // Set up image descriptor for raw RGB565 data
                memset(&cover_img_dsc, 0, sizeof(cover_img_dsc));
                cover_img_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
                cover_img_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
                cover_img_dsc.header.w = COVER_WIDTH;
                cover_img_dsc.header.h = COVER_HEIGHT;
                cover_img_dsc.header.stride = COVER_WIDTH * 2;  // RGB565 = 2 bytes per pixel
                cover_img_dsc.data_size = size;
                cover_img_dsc.data = data;

                // Set the image source
                lv_image_set_src(objects.main_screen_printer_print_cover, &cover_img_dsc);

                // Scale 256 = 100% (1:1 mapping for 70x70 image in 70x70 container)
                lv_image_set_scale(objects.main_screen_printer_print_cover, 256);

                // Make fully opaque when showing actual cover
                lv_obj_set_style_opa(objects.main_screen_printer_print_cover, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

                cover_displayed = true;
            }
        }
    } else {
        if (cover_displayed) {
            // No cover available, revert to placeholder
            extern const lv_image_dsc_t img_filament_spool;
            lv_image_set_src(objects.main_screen_printer_print_cover, &img_filament_spool);

            // Restore original scale from EEZ (100 scales the placeholder to fit)
            lv_image_set_scale(objects.main_screen_printer_print_cover, 100);

            // Semi-transparent for placeholder (as per EEZ design)
            lv_obj_set_style_opa(objects.main_screen_printer_print_cover, 128, LV_PART_MAIN | LV_STATE_DEFAULT);

            cover_displayed = false;
        }
    }
}

// =============================================================================
// Dynamic AMS Display - Matches EEZ static design exactly
// =============================================================================

// Track dynamically created AMS containers for cleanup
#define MAX_AMS_WIDGETS 8  // 4 AMS + 2 HT + 2 Ext
static lv_obj_t *ams_widgets_left[MAX_AMS_WIDGETS];
static lv_obj_t *ams_widgets_right[MAX_AMS_WIDGETS];
static int ams_widget_count_left = 0;
static int ams_widget_count_right = 0;
static bool ams_static_hidden = false;

// Dimensions matching EEZ static design exactly
// NOTE: EEZ uses negative positions to account for default LVGL container padding (~15px)
#define SLOT_SIZE 23           // 23x24 in EEZ but using square
#define SLOT_SPACING 28        // Distance between slot centers (28px between slot starts)
#define CONTAINER_4SLOT_W 120  // 4-slot container (regular AMS)
#define CONTAINER_4SLOT_H 50
#define CONTAINER_1SLOT_W 56   // Single slot - TWO fit one 4-slot: (120-8)/2 = 56
#define CONTAINER_1SLOT_H 50
#define ROW_TOP_Y (-2)         // Top row Y (4-slot AMS) - EEZ coordinate
#define ROW_BOTTOM_Y 50        // Bottom row Y (1-slot HT/Ext) - EEZ coordinate
#define LR_BADGE_X (-16)       // L/R badge X position (EEZ)
#define LR_BADGE_Y (-17)       // L/R badge Y position (EEZ)
#define CONTAINER_START_X (-16) // AMS containers aligned with L/R badge (same X)
#define CONTAINER_4SLOT_GAP 7  // Gap between 4-slot containers
#define CONTAINER_1SLOT_GAP 8  // Gap between 1-slot containers

// Accent green color - matches progress bar (#00FF00)
#define ACCENT_GREEN 0x00FF00

/**
 * @brief Get AMS unit name from ID
 */
static void get_ams_unit_name(int id, char *buf, size_t buf_size) {
    if (id >= 0 && id <= 3) {
        snprintf(buf, buf_size, "%c", 'A' + id);
    } else if (id >= 128 && id <= 135) {
        snprintf(buf, buf_size, "HT-%c", 'A' + (id - 128));
    } else if (id == 254) {
        snprintf(buf, buf_size, "Ext-R");
    } else if (id == 255) {
        snprintf(buf, buf_size, "Ext-L");
    } else {
        snprintf(buf, buf_size, "?");
    }
}

/**
 * @brief Calculate global tray index for active tray comparison
 */
static int get_global_tray_index(int ams_id, int tray_idx) {
    if (ams_id >= 0 && ams_id <= 3) {
        return ams_id * 4 + tray_idx;
    } else if (ams_id >= 128 && ams_id <= 135) {
        return 64 + (ams_id - 128);
    } else if (ams_id == 254 || ams_id == 255) {
        return ams_id;
    }
    return -1;
}

/**
 * @brief Create a color slot matching EEZ design
 * Uses lv_obj_create with lv_line_create for empty slot striping (matches simulator)
 */
static lv_obj_t* create_slot(lv_obj_t *parent, int x, int y, uint32_t rgba, bool is_active) {
    // Use container for slot to allow child objects (striping lines)
    lv_obj_t *slot = lv_obj_create(parent);
    lv_obj_set_pos(slot, x, y);
    lv_obj_set_size(slot, SLOT_SIZE, SLOT_SIZE + 1);
    lv_obj_clear_flag(slot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(slot, 0, 0);

    // Extract RGB from RGBA
    uint8_t r = (rgba >> 24) & 0xFF;
    uint8_t g = (rgba >> 16) & 0xFF;
    uint8_t b = (rgba >> 8) & 0xFF;
    uint32_t color_hex = (r << 16) | (g << 8) | b;
    bool is_empty = (rgba == 0);

    if (!is_empty) {
        // Use solid color (no gradient) for better color visibility
        lv_obj_set_style_bg_color(slot, lv_color_hex(color_hex), 0);
        lv_obj_set_style_bg_opa(slot, 255, 0);
        // Add subtle dark border for depth instead of gradient
        lv_obj_set_style_border_width(slot, 1, 0);
        lv_obj_set_style_border_color(slot, lv_color_hex(0x000000), 0);
        lv_obj_set_style_border_opa(slot, 80, 0);
    } else {
        // Empty slot: darker background with striping pattern
        lv_obj_set_style_bg_color(slot, lv_color_hex(0x0a0a0a), 0);
        lv_obj_set_style_bg_opa(slot, 255, 0);

        // Use simple rectangle objects as diagonal stripes (more reliable than lv_line)
        // Create 3 thin rectangles positioned diagonally
        for (int i = 0; i < 3; i++) {
            lv_obj_t *stripe = lv_obj_create(slot);
            lv_obj_remove_style_all(stripe);
            lv_obj_set_size(stripe, SLOT_SIZE + 8, 3);
            lv_obj_set_pos(stripe, -4, 6 + i * 10);  // Offset positions
            lv_obj_set_style_bg_color(stripe, lv_color_hex(0x3a3a3a), 0);
            lv_obj_set_style_bg_opa(stripe, 255, 0);
            lv_obj_set_style_transform_rotation(stripe, -200, 0);  // Slight angle (0.1 deg units)
            lv_obj_clear_flag(stripe, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        }
    }

    lv_obj_set_style_radius(slot, 5, 0);
    lv_obj_set_style_clip_corner(slot, true, 0);

    if (is_active) {
        lv_obj_set_style_border_color(slot, lv_color_hex(ACCENT_GREEN), 0);
        lv_obj_set_style_border_width(slot, 3, 0);
    } else {
        lv_obj_set_style_border_color(slot, lv_color_hex(0xbab1b1), 0);
        lv_obj_set_style_border_width(slot, 2, 0);
    }
    lv_obj_set_style_border_opa(slot, 255, 0);

    return slot;
}

/**
 * @brief Create AMS container matching EEZ design exactly
 * @param tray_now Global active tray index (used to highlight active slot)
 */
static lv_obj_t* create_ams_container(lv_obj_t *parent, AmsUnitCInfo *info, int tray_now) {
    char name_buf[16];
    get_ams_unit_name(info->id, name_buf, sizeof(name_buf));

    int slot_count = info->tray_count > 0 ? info->tray_count : 1;
    bool is_single_slot = (slot_count == 1);

    int width = is_single_slot ? CONTAINER_1SLOT_W : CONTAINER_4SLOT_W;
    int height = is_single_slot ? CONTAINER_1SLOT_H : CONTAINER_4SLOT_H;

    // Create container
    lv_obj_t *container = lv_obj_create(parent);
    lv_obj_set_size(container, width, height);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    // Container styling matching EEZ exactly
    lv_obj_set_style_bg_color(container, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(container, 255, 0);  // Fully opaque
    lv_obj_set_style_layout(container, LV_LAYOUT_NONE, 0);

    // Check if this container has the active slot
    bool container_active = false;
    for (int i = 0; i < slot_count; i++) {
        int global_tray = get_global_tray_index(info->id, i);
        if (global_tray == tray_now) {
            container_active = true;
            break;
        }
    }

    // Container border - accent green if it contains the active slot
    lv_obj_set_style_border_width(container, 3, 0);
    if (container_active) {
        lv_obj_set_style_border_color(container, lv_color_hex(ACCENT_GREEN), 0);
    } else {
        lv_obj_set_style_border_color(container, lv_color_hex(0x3d3d3d), 0);
    }

    // Shadow matching EEZ
    lv_obj_set_style_shadow_width(container, 5, 0);
    lv_obj_set_style_shadow_ofs_x(container, 2, 0);
    lv_obj_set_style_shadow_ofs_y(container, 2, 0);
    lv_obj_set_style_shadow_spread(container, 2, 0);
    lv_obj_set_style_shadow_opa(container, 100, 0);

    // Label
    lv_obj_t *label = lv_label_create(container);
    lv_label_set_text(label, name_buf);
    lv_obj_set_style_text_color(label, lv_color_hex(0xfafafa), 0);
    lv_obj_set_style_text_opa(label, 255, 0);

    if (is_single_slot) {
        // Single slot: label at top-left, slot below - EEZ positions
        lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
        lv_obj_set_pos(label, -14, -17);  // EEZ: HT-A label position

        int global_tray = get_global_tray_index(info->id, 0);
        bool slot_active = (tray_now == global_tray);
        uint32_t color = info->tray_count > 0 ? info->trays[0].tray_color : 0;
        create_slot(container, -10, -1, color, slot_active);  // EEZ: x=-10, y=-1
    } else {
        // 4-slot: label centered at top, slots in a row - EEZ positions
        lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(label, 35, -18);  // EEZ position

        // EEZ slot positions: -17, 11, 39, 68 (spacing of 28px)
        int slot_x_positions[4] = {-17, 11, 39, 68};
        for (int i = 0; i < slot_count && i < 4; i++) {
            int global_tray = get_global_tray_index(info->id, i);
            bool slot_active = (tray_now == global_tray);
            uint32_t color = (i < info->tray_count) ? info->trays[i].tray_color : 0;
            create_slot(container, slot_x_positions[i], -3, color, slot_active);
        }
    }

    return container;
}

/**
 * @brief Hide all children of a container
 */
static void hide_all_children(lv_obj_t *parent) {
    if (!parent) return;
    uint32_t child_count = lv_obj_get_child_count(parent);
    for (uint32_t i = 0; i < child_count; i++) {
        lv_obj_t *child = lv_obj_get_child(parent, i);
        if (child) {
            lv_obj_add_flag(child, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/**
 * @brief Create the "L" or "R" indicator badge - EEZ position (top-left)
 */
static lv_obj_t* create_nozzle_badge(lv_obj_t *parent, const char *letter) {
    lv_obj_t *badge = lv_label_create(parent);
    lv_obj_set_pos(badge, LR_BADGE_X, LR_BADGE_Y);  // EEZ: (-16, -17)
    lv_obj_set_size(badge, 12, 12);
    lv_obj_set_style_bg_color(badge, lv_color_hex(ACCENT_GREEN), 0);
    lv_obj_set_style_bg_opa(badge, 255, 0);
    lv_obj_set_style_text_color(badge, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(badge, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_align(badge, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_opa(badge, 255, 0);
    lv_label_set_text(badge, letter);
    return badge;
}

/**
 * @brief Create the "Left Nozzle" or "Right Nozzle" label - EEZ position (next to badge)
 */
static lv_obj_t* create_nozzle_label(lv_obj_t *parent, const char *text) {
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_pos(label, 0, LR_BADGE_Y);  // EEZ: right of badge, same Y
    lv_obj_set_size(label, LV_SIZE_CONTENT, 12);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_10, 0);
    lv_label_set_text(label, text);
    return label;
}

/**
 * @brief Clear dynamically created AMS widgets
 */
static void clear_ams_widgets(void) {
    for (int i = 0; i < ams_widget_count_left; i++) {
        if (ams_widgets_left[i]) {
            lv_obj_delete(ams_widgets_left[i]);
            ams_widgets_left[i] = NULL;
        }
    }
    ams_widget_count_left = 0;

    for (int i = 0; i < ams_widget_count_right; i++) {
        if (ams_widgets_right[i]) {
            lv_obj_delete(ams_widgets_right[i]);
            ams_widgets_right[i] = NULL;
        }
    }
    ams_widget_count_right = 0;
}


// Store nozzle header objects
static lv_obj_t *left_badge = NULL;
static lv_obj_t *left_label = NULL;
static lv_obj_t *right_badge = NULL;
static lv_obj_t *right_label = NULL;

/**
 * @brief Hide static objects and create nozzle headers
 */
static void setup_ams_containers(void) {
    if (ams_static_hidden) return;

    // Delete old badges/labels if they exist (prevents memory leak on printer change)
    if (left_badge) { lv_obj_delete(left_badge); left_badge = NULL; }
    if (left_label) { lv_obj_delete(left_label); left_label = NULL; }
    if (right_badge) { lv_obj_delete(right_badge); right_badge = NULL; }
    if (right_label) { lv_obj_delete(right_label); right_label = NULL; }

    // Hide all static children
    hide_all_children(objects.main_screen_ams_left_nozzle);
    hide_all_children(objects.main_screen_ams_right_nozzle);

    // Create nozzle headers
    if (objects.main_screen_ams_left_nozzle) {
        left_badge = create_nozzle_badge(objects.main_screen_ams_left_nozzle, "L");
        left_label = create_nozzle_label(objects.main_screen_ams_left_nozzle, "Left Nozzle");
    }
    if (objects.main_screen_ams_right_nozzle) {
        right_badge = create_nozzle_badge(objects.main_screen_ams_right_nozzle, "R");
        right_label = create_nozzle_label(objects.main_screen_ams_right_nozzle, "Right Nozzle");
    }

    ams_static_hidden = true;
}

/**
 * @brief Reset all dynamically created main screen objects
 *
 * Called when navigating back to the main screen to clear stale pointers.
 * These static pointers point to objects on the old (deleted) main screen.
 */
static void reset_main_screen_dynamic_state(void) {
    // Reset labels
    status_eta_label = NULL;
    progress_pct_label = NULL;

    // Reset AMS widgets
    for (int i = 0; i < MAX_AMS_WIDGETS; i++) {
        ams_widgets_left[i] = NULL;
        ams_widgets_right[i] = NULL;
    }
    ams_widget_count_left = 0;
    ams_widget_count_right = 0;
    ams_static_hidden = false;

    // Reset nozzle headers
    left_badge = NULL;
    left_label = NULL;
    right_badge = NULL;
    right_label = NULL;

    // Reset cover image state
    cover_displayed = false;

    // Reset LED animation state (animation is auto-deleted when object is deleted)
    led_anim_active = false;

    // Reset printer dropdown tracking to force update
    last_printer_count = -1;
    last_connected_mask = 0;

    ESP_LOGI(TAG, "Reset main screen dynamic state - cleared stale pointers");
}

/**
 * @brief Update AMS display matching EEZ static design
 */
static void update_ams_display(void) {
    if (!objects.main_screen) {
        return;
    }

    // Ensure AMS container parent objects exist
    if (!objects.main_screen_ams_left_nozzle && !objects.main_screen_ams_right_nozzle) {
        return;  // Neither container exists
    }

    // Setup on first call
    setup_ams_containers();

    // Clear previous dynamic widgets
    clear_ams_widgets();

    // Get AMS data for selected printer
    int ams_count = backend_get_ams_count(selected_printer_index);
    int tray_now = backend_get_tray_now(selected_printer_index);  // Legacy single-nozzle
    int tray_now_left = backend_get_tray_now_left(selected_printer_index);
    int tray_now_right = backend_get_tray_now_right(selected_printer_index);
    int active_extruder = backend_get_active_extruder(selected_printer_index);  // -1=unknown, 0=right, 1=left

    // Determine which tray is ACTIVELY printing (not just loaded)
    // For dual-nozzle printers: active_extruder indicates which nozzle (0=right, 1=left)
    // For single-nozzle printers: active_extruder is -1, use tray_now for right side
    int active_tray_left = -1;
    int active_tray_right = -1;

    // Check if this is a dual-nozzle printer (H2C/H2D)
    // Only use AMS extruder assignment - active_extruder >= 0 is true for single-nozzle too
    bool has_left_extruder_ams = false;
    for (int i = 0; i < ams_count; i++) {
        AmsUnitCInfo check_info;
        if (backend_get_ams_unit(selected_printer_index, i, &check_info) == 0) {
            if (check_info.extruder == 1) {
                has_left_extruder_ams = true;
                break;
            }
        }
    }
    // Dual-nozzle only if AMS units are assigned to left extruder
    bool is_dual_nozzle = has_left_extruder_ams;


    // Update the tracking variable
    selected_printer_is_dual_nozzle = is_dual_nozzle;

    // Handle AMS containers based on single/dual nozzle
    // Single-nozzle: use LEFT container, hide RIGHT
    // Dual-nozzle: use both containers
    if (!is_dual_nozzle) {
        // Single nozzle: hide RIGHT container, use LEFT for all AMS
        if (objects.main_screen_ams_right_nozzle) {
            lv_obj_add_flag(objects.main_screen_ams_right_nozzle, LV_OBJ_FLAG_HIDDEN);
        }
        if (objects.main_screen_ams_left_nozzle) {
            lv_obj_clear_flag(objects.main_screen_ams_left_nozzle, LV_OBJ_FLAG_HIDDEN);
        }
        // Hide L badge, show simple "AMS" label aligned left (where badge was)
        if (left_badge) lv_obj_add_flag(left_badge, LV_OBJ_FLAG_HIDDEN);
        if (left_label) {
            lv_label_set_text(left_label, "AMS");
            lv_obj_set_pos(left_label, LR_BADGE_X, LR_BADGE_Y);  // Align with badge position
        }
    } else {
        // Dual nozzle: show both containers with L/R labels
        if (objects.main_screen_ams_left_nozzle) {
            lv_obj_clear_flag(objects.main_screen_ams_left_nozzle, LV_OBJ_FLAG_HIDDEN);
        }
        if (objects.main_screen_ams_right_nozzle) {
            lv_obj_clear_flag(objects.main_screen_ams_right_nozzle, LV_OBJ_FLAG_HIDDEN);
        }
        // Show L/R badges and full labels
        if (left_badge) lv_obj_clear_flag(left_badge, LV_OBJ_FLAG_HIDDEN);
        if (left_label) {
            lv_label_set_text(left_label, "Left Nozzle");
            lv_obj_set_pos(left_label, 0, LR_BADGE_Y);  // Reset to right of badge
        }
        if (right_badge) lv_obj_clear_flag(right_badge, LV_OBJ_FLAG_HIDDEN);
        if (right_label) lv_label_set_text(right_label, "Right Nozzle");
    }

    if (is_dual_nozzle) {
        // Dual nozzle: only use per-extruder tray values, no fallback to legacy tray_now
        // tray_now_left/right must be explicitly set (>= 0) to show active indicator
        if (active_extruder == 0 && tray_now_right >= 0) {
            active_tray_right = tray_now_right;
        } else if (active_extruder == 1 && tray_now_left >= 0) {
            active_tray_left = tray_now_left;
        }
        // If per-extruder values not set, don't show any slot as active
    } else {
        // Single nozzle: use tray_now for right side (only side shown)
        active_tray_right = tray_now;
    }


    // Separate AMS units by type and nozzle
    // Left nozzle: top row for 4-slot, bottom row for 1-slot
    // Right nozzle: same layout
    // EEZ positions: 4-slot at x=-16, 111, 240 (step ~127); 1-slot at x=-16, 38 (step 54)
    int left_4slot_x = CONTAINER_START_X;
    int left_1slot_x = CONTAINER_START_X;
    int right_4slot_x = CONTAINER_START_X;
    int right_1slot_x = CONTAINER_START_X;

    for (int i = 0; i < ams_count && i < MAX_AMS_WIDGETS; i++) {
        AmsUnitCInfo info;
        if (backend_get_ams_unit(selected_printer_index, i, &info) != 0) {
            continue;
        }

        // For single-nozzle printers, all AMS goes to LEFT side
        // For dual-nozzle, use extruder assignment (0=right, 1=left)
        bool use_left = !is_dual_nozzle || (info.extruder == 1);
        lv_obj_t *parent = use_left ? objects.main_screen_ams_left_nozzle : objects.main_screen_ams_right_nozzle;
        if (!parent) continue;

        // Use the active tray for this extruder (only if it's the active extruder)
        int active_tray = use_left ? active_tray_left : active_tray_right;
        lv_obj_t *widget = create_ams_container(parent, &info, active_tray);

        // Position based on slot count and nozzle
        bool is_single = (info.tray_count <= 1);
        int *x_pos;
        int y_pos;
        int step;

        if (use_left) {
            if (is_single) {
                x_pos = &left_1slot_x;
                y_pos = ROW_BOTTOM_Y;
                step = CONTAINER_1SLOT_W + CONTAINER_1SLOT_GAP;  // 47 + 7 = 54
            } else {
                x_pos = &left_4slot_x;
                y_pos = ROW_TOP_Y;
                step = CONTAINER_4SLOT_W + CONTAINER_4SLOT_GAP;  // 120 + 7 = 127
            }
            if (ams_widget_count_left < MAX_AMS_WIDGETS) {
                ams_widgets_left[ams_widget_count_left++] = widget;
            }
        } else {
            if (is_single) {
                x_pos = &right_1slot_x;
                y_pos = ROW_BOTTOM_Y;
                step = CONTAINER_1SLOT_W + CONTAINER_1SLOT_GAP;
            } else {
                x_pos = &right_4slot_x;
                y_pos = ROW_TOP_Y;
                step = CONTAINER_4SLOT_W + CONTAINER_4SLOT_GAP;
            }
            if (ams_widget_count_right < MAX_AMS_WIDGETS) {
                ams_widgets_right[ams_widget_count_right++] = widget;
            }
        }

        lv_obj_set_pos(widget, *x_pos, y_pos);
        *x_pos += step;
    }

    // Create external spool holder slots
    // Single-nozzle: one "Ext" slot on LEFT
    // Dual-nozzle: EXT-R on right, EXT-L on left
    AmsUnitCInfo ext_info = {
        .id = 254,  // External right (or just external for single-nozzle)
        .humidity = -1,
        .temperature = -1,
        .extruder = 0,
        .tray_count = 1,
        .trays = {{.tray_color = 0}}  // Empty
    };

    if (!is_dual_nozzle) {
        // Single-nozzle: create one "Ext" slot on LEFT side
        if (objects.main_screen_ams_left_nozzle && ams_widget_count_left < MAX_AMS_WIDGETS) {
            lv_obj_t *ext = create_ams_container(objects.main_screen_ams_left_nozzle, &ext_info, active_tray_right);
            lv_obj_set_pos(ext, left_1slot_x, ROW_BOTTOM_Y);
            ams_widgets_left[ams_widget_count_left++] = ext;
        }
    } else {
        // Dual-nozzle: create EXT-R on right and EXT-L on left
        if (objects.main_screen_ams_right_nozzle && ams_widget_count_right < MAX_AMS_WIDGETS) {
            lv_obj_t *ext_r = create_ams_container(objects.main_screen_ams_right_nozzle, &ext_info, active_tray_right);
            lv_obj_set_pos(ext_r, right_1slot_x, ROW_BOTTOM_Y);
            ams_widgets_right[ams_widget_count_right++] = ext_r;
        }

        AmsUnitCInfo ext_l_info = {
            .id = 255,  // External left
            .humidity = -1,
            .temperature = -1,
            .extruder = 1,
            .tray_count = 1,
            .trays = {{.tray_color = 0}}  // Empty
        };
        if (objects.main_screen_ams_left_nozzle && ams_widget_count_left < MAX_AMS_WIDGETS) {
            lv_obj_t *ext_l = create_ams_container(objects.main_screen_ams_left_nozzle, &ext_l_info, active_tray_left);
            lv_obj_set_pos(ext_l, left_1slot_x, ROW_BOTTOM_Y);
            ams_widgets_left[ams_widget_count_left++] = ext_l;
        }
    }
}

// =============================================================================
// AMS Overview Screen Display
// =============================================================================

/**
 * @brief Update a single slot's color based on RGBA value
 */
static void update_slot_color(lv_obj_t *slot_color, uint32_t rgba) {
    if (!slot_color) return;

    uint8_t r = (rgba >> 24) & 0xFF;
    uint8_t g = (rgba >> 16) & 0xFF;
    uint8_t b = (rgba >> 8) & 0xFF;
    uint32_t color_hex = (r << 16) | (g << 8) | b;
    bool is_empty = (rgba == 0);

    // slot_color is an lv_image with img_spool_fill - use image recoloring
    if (is_empty) {
        // Empty/no filament - dark gray
        lv_obj_set_style_image_recolor(slot_color, lv_color_hex(0x1a1a1a), 0);
        lv_obj_set_style_image_recolor_opa(slot_color, 255, 0);
    } else {
        // Apply filament color via image recolor
        lv_obj_set_style_image_recolor(slot_color, lv_color_hex(color_hex), 0);
        lv_obj_set_style_image_recolor_opa(slot_color, 255, 0);
    }
}

// Stripe line storage for slot empty state visualization
// Each slot has 3 diagonal stripe lines
#define STRIPE_COUNT 3

// HT slots (HT-A, HT-B)
static lv_obj_t *ht_a_stripes[STRIPE_COUNT] = {NULL};
static lv_obj_t *ht_b_stripes[STRIPE_COUNT] = {NULL};

// AMS slots (4 per AMS panel, 4 panels = 16 total)
static lv_obj_t *ams_a_slot_stripes[4][STRIPE_COUNT] = {{NULL}};
static lv_obj_t *ams_b_slot_stripes[4][STRIPE_COUNT] = {{NULL}};
static lv_obj_t *ams_c_slot_stripes[4][STRIPE_COUNT] = {{NULL}};
static lv_obj_t *ams_d_slot_stripes[4][STRIPE_COUNT] = {{NULL}};

// Static line points for diagonal stripes on 32x42 slots
static lv_point_precise_t stripe_pts_0[2] = {{0, 12}, {32, 4}};
static lv_point_precise_t stripe_pts_1[2] = {{0, 24}, {32, 16}};
static lv_point_precise_t stripe_pts_2[2] = {{0, 36}, {32, 28}};
static lv_point_precise_t *stripe_pts[STRIPE_COUNT] = {stripe_pts_0, stripe_pts_1, stripe_pts_2};

/**
 * @brief Create stripe lines for an empty slot
 * Creates diagonal lines as visual indicator for empty state
 * @param parent The panel containing the slot
 * @param stripes Array to store created line objects (STRIPE_COUNT elements)
 * @param slot_x X position of the slot within parent
 * @param slot_y Y position of the slot within parent
 */
static void create_slot_stripes(lv_obj_t *parent, lv_obj_t **stripes, int slot_x, int slot_y) {
    if (!parent) return;

    for (int i = 0; i < STRIPE_COUNT; i++) {
        if (!stripes[i]) {
            stripes[i] = lv_line_create(parent);
            lv_line_set_points(stripes[i], stripe_pts[i], 2);
            lv_obj_set_pos(stripes[i], slot_x, slot_y);
            lv_obj_set_style_line_color(stripes[i], lv_color_hex(0x4a4a4a), 0);
            lv_obj_set_style_line_width(stripes[i], 3, 0);
            lv_obj_set_style_line_opa(stripes[i], 255, 0);
        }
    }
}

/**
 * @brief Show or hide stripe lines for slot empty state
 * @param stripes Array of stripe line objects (STRIPE_COUNT elements)
 * @param show True to show stripes (empty slot), false to hide (has filament)
 */
static void set_slot_stripes_visible(lv_obj_t **stripes, bool show) {
    for (int i = 0; i < STRIPE_COUNT; i++) {
        if (stripes[i]) {
            if (show) {
                lv_obj_clear_flag(stripes[i], LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(stripes[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

/**
 * @brief Update the L/R extruder indicator on an AMS panel
 * Matches main screen style: green badge with black text
 * @param indicator The indicator label object
 * @param extruder -1=unknown, 0=right, 1=left
 */
static void update_extruder_indicator(lv_obj_t *indicator, int8_t extruder) {
    if (!indicator) return;

    if (extruder == 1) {
        // Left extruder - green badge with "L" (matching main screen)
        lv_label_set_text(indicator, "L");
        lv_obj_set_style_bg_color(indicator, lv_color_hex(ACCENT_GREEN), 0);
        lv_obj_set_style_bg_opa(indicator, 255, 0);
        lv_obj_set_style_text_color(indicator, lv_color_hex(0x000000), 0);
        lv_obj_set_style_text_font(indicator, &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_align(indicator, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_clear_flag(indicator, LV_OBJ_FLAG_HIDDEN);
    } else if (extruder == 0) {
        // Right extruder - green badge with "R" (matching main screen)
        lv_label_set_text(indicator, "R");
        lv_obj_set_style_bg_color(indicator, lv_color_hex(ACCENT_GREEN), 0);
        lv_obj_set_style_bg_opa(indicator, 255, 0);
        lv_obj_set_style_text_color(indicator, lv_color_hex(0x000000), 0);
        lv_obj_set_style_text_font(indicator, &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_align(indicator, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_clear_flag(indicator, LV_OBJ_FLAG_HIDDEN);
    } else {
        // Unknown - hide indicator
        lv_obj_add_flag(indicator, LV_OBJ_FLAG_HIDDEN);
    }
}

/**
 * @brief Update indicator and shift label position based on dual-nozzle mode
 * When indicator is shown: label at x=1 (after badge)
 * When indicator is hidden: label at x=-16 (where badge was)
 */
static void update_panel_indicator(lv_obj_t *indicator, lv_obj_t *label, int8_t extruder, bool is_dual_nozzle) {
    if (is_dual_nozzle && extruder >= 0) {
        // Show indicator, label stays at normal position
        update_extruder_indicator(indicator, extruder);
        if (label) {
            lv_obj_set_x(label, 1);  // EEZ default position (after badge)
        }
    } else {
        // Hide indicator, shift label left
        if (indicator) {
            lv_obj_add_flag(indicator, LV_OBJ_FLAG_HIDDEN);
        }
        if (label) {
            lv_obj_set_x(label, -16);  // Move to where badge was
        }
    }
}

/**
 * @brief Format fill level, handling 255 (unknown) as "---"
 * Note: 0% is a valid fill level for nearly-empty spools
 * Empty slots should be handled by caller checking tray_color before calling this
 */
static void format_fill_level(char *buf, size_t buf_size, uint8_t remain) {
    if (remain >= 101) {
        // 255 or any invalid value means unknown
        snprintf(buf, buf_size, "---");
    } else {
        // 0-100 are valid percentages
        snprintf(buf, buf_size, "%d%%", remain);
    }
}

/**
 * @brief Update AMS overview screen with backend data
 *
 * Shows/hides AMS panels based on actual data and updates:
 * - Slot colors, materials, fill levels
 * - Humidity and temperature
 * - Active slot indicator
 */
// Track last AMS screen to detect fresh loads for one-time positioning
static lv_obj_t *last_ams_screen = NULL;
static bool ams_row2_positioned = false;

static void update_ams_overview_display(void) {
    // Only update on AMS overview screen
    if (!objects.ams_overview || lv_scr_act() != objects.ams_overview) {
        return;
    }

    ESP_LOGI(TAG, "AMS overview update: panel=%p ht_a=%p ext1=%p ext2=%p",
             (void*)objects.ams_screen_ams_panel,
             (void*)objects.ams_screen_ams_panel_ht_a,
             (void*)objects.ams_screen_ams_panel_ext_1,
             (void*)objects.ams_screen_ams_panel_ext_2);

    // Detect screen recreation - reset positioning flag
    if (objects.ams_overview != last_ams_screen) {
        last_ams_screen = objects.ams_overview;
        ams_row2_positioned = false;
    }

    int ams_count = backend_get_ams_count(selected_printer_index);
    int tray_now = backend_get_tray_now(selected_printer_index);

    // Build a set of which AMS IDs are present
    bool has_ams[256] = {false};
    AmsUnitCInfo ams_data[8];  // Cache AMS data
    int ams_data_count = 0;

    for (int i = 0; i < ams_count && i < 8; i++) {
        AmsUnitCInfo info;
        if (backend_get_ams_unit(selected_printer_index, i, &info) == 0) {
            has_ams[info.id] = true;
            ams_data[ams_data_count++] = info;
        }
    }

    // Hide/show and update AMS A (id=0)
    if (objects.ams_screen_ams_panel_ams_a) {
        if (has_ams[0]) {
            lv_obj_clear_flag(objects.ams_screen_ams_panel_ams_a, LV_OBJ_FLAG_HIDDEN);

            // Find the data for this AMS
            for (int i = 0; i < ams_data_count; i++) {
                if (ams_data[i].id == 0) {
                    AmsUnitCInfo *info = &ams_data[i];

                    // Update L/R extruder indicator and label position
                    update_panel_indicator(
                        objects.ams_screen_ams_panel_ams_a_indicator,
                        objects.ams_screen_ams_panel_ams_a_label_name,
                        info->extruder,
                        selected_printer_is_dual_nozzle
                    );

                    // Update humidity
                    if (objects.ams_screen_ams_panel_ams_a_label_humidity) {
                        if (info->humidity >= 0) {
                            char buf[16];
                            snprintf(buf, sizeof(buf), "%d%%", info->humidity);
                            lv_label_set_text(objects.ams_screen_ams_panel_ams_a_label_humidity, buf);
                        } else {
                            lv_label_set_text(objects.ams_screen_ams_panel_ams_a_label_humidity, "--");
                        }
                    }

                    // Update temperature
                    if (objects.ams_screen_ams_panel_ams_a_label_temperature) {
                        if (info->temperature >= 0) {
                            char buf[16];
                            snprintf(buf, sizeof(buf), "%d°C", info->temperature / 10);
                            lv_label_set_text(objects.ams_screen_ams_panel_ams_a_label_temperature, buf);
                        } else {
                            lv_label_set_text(objects.ams_screen_ams_panel_ams_a_label_temperature, "--");
                        }
                    }

                    // Update slot colors
                    lv_obj_t *slot_colors[] = {
                        objects.ams_screen_ams_panel_ams_a_slot_1_color,
                        objects.ams_screen_ams_panel_ams_a_slot_2_color,
                        objects.ams_screen_ams_panel_ams_a_slot_3_color,
                        objects.ams_screen_ams_panel_ams_a_slot_4_color
                    };
                    lv_obj_t *slot_materials[] = {
                        objects.ams_screen_ams_panel_ams_a_slot_1_label_material,
                        objects.ams_screen_ams_panel_ams_a_slot_2_label_material,
                        objects.ams_screen_ams_panel_ams_a_slot_3_label_material,
                        objects.ams_screen_ams_panel_ams_a_slot_4_label_material
                    };
                    lv_obj_t *slot_fill_levels[] = {
                        objects.ams_screen_ams_panel_ams_a_slot_1_label_slot_name_label_fill_level,
                        objects.ams_screen_ams_panel_ams_a_slot_2_label_slot_name_label_fill_level,
                        objects.ams_screen_ams_panel_ams_a_slot_3_label_slot_name_label_fill_level,
                        objects.ams_screen_ams_panel_ams_a_slot_4_label_slot_name_label_fill_level
                    };
                    lv_obj_t *slots[] = {
                        objects.ams_screen_ams_panel_ams_a_slot_1,
                        objects.ams_screen_ams_panel_ams_a_slot_2,
                        objects.ams_screen_ams_panel_ams_a_slot_3,
                        objects.ams_screen_ams_panel_ams_a_slot_4
                    };
                    // Slot positions from EEZ (relative to panel)
                    int slot_x[] = {-6, 46, 100, 155};
                    int slot_y[] = {47, 48, 48, 49};

                    for (int j = 0; j < 4 && j < info->tray_count; j++) {
                        bool is_empty = (info->trays[j].tray_color == 0);
                        update_slot_color(slot_colors[j], info->trays[j].tray_color);

                        // Create/update stripes for empty slots
                        create_slot_stripes(objects.ams_screen_ams_panel_ams_a, ams_a_slot_stripes[j], slot_x[j], slot_y[j]);
                        set_slot_stripes_visible(ams_a_slot_stripes[j], is_empty);

                        // Update material label
                        if (slot_materials[j]) {
                            if (!is_empty && info->trays[j].tray_type[0]) {
                                lv_label_set_text(slot_materials[j], info->trays[j].tray_type);
                            } else {
                                lv_label_set_text(slot_materials[j], "");
                            }
                        }

                        // Update fill level - empty slots show "---", loaded slots show percentage
                        if (slot_fill_levels[j]) {
                            if (is_empty) {
                                lv_label_set_text(slot_fill_levels[j], "---");
                            } else {
                                char buf[16];
                                format_fill_level(buf, sizeof(buf), info->trays[j].remain);
                                lv_label_set_text(slot_fill_levels[j], buf);
                            }
                        }

                        // Highlight active slot
                        int global_tray = get_global_tray_index(0, j);
                        if (slots[j]) {
                            if (global_tray == tray_now) {
                                lv_obj_set_style_border_color(slots[j], lv_color_hex(ACCENT_GREEN), 0);
                                lv_obj_set_style_border_width(slots[j], 3, 0);
                            } else {
                                lv_obj_set_style_border_color(slots[j], lv_color_hex(0x3d3d3d), 0);
                                lv_obj_set_style_border_width(slots[j], 1, 0);
                            }
                        }
                    }
                    break;
                }
            }
        } else {
            lv_obj_add_flag(objects.ams_screen_ams_panel_ams_a, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // Hide/show and update AMS B (id=1)
    if (objects.ams_screen_ams_panel_ams_b) {
        if (has_ams[1]) {
            lv_obj_clear_flag(objects.ams_screen_ams_panel_ams_b, LV_OBJ_FLAG_HIDDEN);

            for (int i = 0; i < ams_data_count; i++) {
                if (ams_data[i].id == 1) {
                    AmsUnitCInfo *info = &ams_data[i];

                    // Update L/R extruder indicator and label position
                    update_panel_indicator(
                        objects.ams_screen_ams_panel_ams_b_indicator,
                        objects.ams_screen_ams_panel_ams_b_label_name,
                        info->extruder,
                        selected_printer_is_dual_nozzle
                    );

                    if (objects.ams_screen_ams_panel_ams_b_labe_humidity) {
                        if (info->humidity >= 0) {
                            char buf[16];
                            snprintf(buf, sizeof(buf), "%d%%", info->humidity);
                            lv_label_set_text(objects.ams_screen_ams_panel_ams_b_labe_humidity, buf);
                        } else {
                            lv_label_set_text(objects.ams_screen_ams_panel_ams_b_labe_humidity, "--");
                        }
                    }

                    if (objects.ams_screen_ams_panel_ams_b_label_temperature) {
                        if (info->temperature >= 0) {
                            char buf[16];
                            snprintf(buf, sizeof(buf), "%d°C", info->temperature / 10);
                            lv_label_set_text(objects.ams_screen_ams_panel_ams_b_label_temperature, buf);
                        } else {
                            lv_label_set_text(objects.ams_screen_ams_panel_ams_b_label_temperature, "--");
                        }
                    }

                    lv_obj_t *slot_colors[] = {
                        objects.ams_screen_ams_panel_ams_b_slot_1_color,
                        objects.ams_screen_ams_panel_ams_b_slot_2_color,
                        objects.ams_screen_ams_panel_ams_b_slot_3_color,
                        objects.ams_screen_ams_panel_ams_b_slot_4_color
                    };
                    lv_obj_t *slot_materials[] = {
                        objects.ams_screen_ams_panel_ams_b_slot_1_label_material,
                        objects.ams_screen_ams_panel_ams_b_slot_2_label_material,
                        objects.ams_screen_ams_panel_ams_b_slot_3_label_material,
                        objects.ams_screen_ams_panel_ams_b_slot_4_label_material
                    };
                    lv_obj_t *slot_fill_levels[] = {
                        objects.ams_screen_ams_panel_ams_b_slot_1_label_fill_level,
                        objects.ams_screen_ams_panel_ams_b_slot_2_label_fill_level,
                        objects.ams_screen_ams_panel_ams_b_slot_3_label_fill_level,
                        objects.ams_screen_ams_panel_ams_b_slot_4_label_fill_level
                    };
                    lv_obj_t *slots[] = {
                        objects.ams_screen_ams_panel_ams_b_slot_1,
                        objects.ams_screen_ams_panel_ams_b_slot_2,
                        objects.ams_screen_ams_panel_ams_b_slot_3,
                        objects.ams_screen_ams_panel_ams_b_slot_4
                    };
                    // Slot positions from EEZ (same as AMS A)
                    int slot_x[] = {-6, 46, 100, 155};
                    int slot_y[] = {47, 48, 48, 49};

                    for (int j = 0; j < 4 && j < info->tray_count; j++) {
                        bool is_empty = (info->trays[j].tray_color == 0);
                        update_slot_color(slot_colors[j], info->trays[j].tray_color);

                        // Create/update stripes for empty slots
                        create_slot_stripes(objects.ams_screen_ams_panel_ams_b, ams_b_slot_stripes[j], slot_x[j], slot_y[j]);
                        set_slot_stripes_visible(ams_b_slot_stripes[j], is_empty);

                        if (slot_materials[j]) {
                            if (!is_empty && info->trays[j].tray_type[0]) {
                                lv_label_set_text(slot_materials[j], info->trays[j].tray_type);
                            } else {
                                lv_label_set_text(slot_materials[j], "");
                            }
                        }

                        // Update fill level - empty slots show "---", loaded slots show percentage
                        if (slot_fill_levels[j]) {
                            if (is_empty) {
                                lv_label_set_text(slot_fill_levels[j], "---");
                            } else {
                                char buf[16];
                                format_fill_level(buf, sizeof(buf), info->trays[j].remain);
                                lv_label_set_text(slot_fill_levels[j], buf);
                            }
                        }

                        int global_tray = get_global_tray_index(1, j);
                        if (slots[j]) {
                            if (global_tray == tray_now) {
                                lv_obj_set_style_border_color(slots[j], lv_color_hex(ACCENT_GREEN), 0);
                                lv_obj_set_style_border_width(slots[j], 3, 0);
                            } else {
                                lv_obj_set_style_border_color(slots[j], lv_color_hex(0x3d3d3d), 0);
                                lv_obj_set_style_border_width(slots[j], 1, 0);
                            }
                        }
                    }
                    break;
                }
            }
        } else {
            lv_obj_add_flag(objects.ams_screen_ams_panel_ams_b, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // Hide/show and update AMS C (id=2)
    if (objects.ams_screen_ams_panel_ams_c) {
        if (has_ams[2]) {
            lv_obj_clear_flag(objects.ams_screen_ams_panel_ams_c, LV_OBJ_FLAG_HIDDEN);

            for (int i = 0; i < ams_data_count; i++) {
                if (ams_data[i].id == 2) {
                    AmsUnitCInfo *info = &ams_data[i];

                    // Update L/R extruder indicator and label position
                    update_panel_indicator(
                        objects.ams_screen_ams_panel_ams_c_indicator,
                        objects.ams_screen_ams_panel_ams_c_label_name,
                        info->extruder,
                        selected_printer_is_dual_nozzle
                    );

                    if (objects.ams_screen_ams_panel_ams_c_label_humidity) {
                        if (info->humidity >= 0) {
                            char buf[16];
                            snprintf(buf, sizeof(buf), "%d%%", info->humidity);
                            lv_label_set_text(objects.ams_screen_ams_panel_ams_c_label_humidity, buf);
                        } else {
                            lv_label_set_text(objects.ams_screen_ams_panel_ams_c_label_humidity, "--");
                        }
                    }

                    if (objects.ams_screen_ams_panel_ams_c_label_temperature) {
                        if (info->temperature >= 0) {
                            char buf[16];
                            snprintf(buf, sizeof(buf), "%d°C", info->temperature / 10);
                            lv_label_set_text(objects.ams_screen_ams_panel_ams_c_label_temperature, buf);
                        } else {
                            lv_label_set_text(objects.ams_screen_ams_panel_ams_c_label_temperature, "--");
                        }
                    }

                    lv_obj_t *slot_colors[] = {
                        objects.ams_screen_ams_panel_ams_c_slot_1_color,
                        objects.ams_screen_ams_panel_ams_c_slot_2_color,
                        objects.ams_screen_ams_panel_ams_c_slot_3_color,
                        objects.ams_screen_ams_panel_ams_c_slot_4_color
                    };
                    lv_obj_t *slot_materials[] = {
                        objects.ams_screen_ams_panel_ams_c_slot_1_label_material,
                        objects.ams_screen_ams_panel_ams_c_slot_2_label_material,
                        objects.ams_screen_ams_panel_ams_c_slot_3_label_material,
                        objects.ams_screen_ams_panel_ams_c_slot_4_label_material
                    };
                    lv_obj_t *slot_fill_levels[] = {
                        objects.ams_screen_ams_panel_ams_c_slot_1_label_fill_level,
                        objects.ams_screen_ams_panel_ams_c_slot_2_label_fill_level,
                        objects.ams_screen_ams_panel_ams_c_slot_3_label_fill_level,
                        objects.ams_screen_ams_panel_ams_c_slot_4_label_fill_level
                    };
                    lv_obj_t *slots[] = {
                        objects.ams_screen_ams_panel_ams_c_slot_1,
                        objects.ams_screen_ams_panel_ams_c_slot_2,
                        objects.ams_screen_ams_panel_ams_c_slot_3,
                        objects.ams_screen_ams_panel_ams_c_slot_4
                    };
                    // Slot positions from EEZ (same as AMS A)
                    int slot_x[] = {-6, 46, 100, 155};
                    int slot_y[] = {47, 48, 48, 49};

                    for (int j = 0; j < 4 && j < info->tray_count; j++) {
                        bool is_empty = (info->trays[j].tray_color == 0);
                        update_slot_color(slot_colors[j], info->trays[j].tray_color);

                        // Create/update stripes for empty slots
                        create_slot_stripes(objects.ams_screen_ams_panel_ams_c, ams_c_slot_stripes[j], slot_x[j], slot_y[j]);
                        set_slot_stripes_visible(ams_c_slot_stripes[j], is_empty);

                        if (slot_materials[j]) {
                            if (!is_empty && info->trays[j].tray_type[0]) {
                                lv_label_set_text(slot_materials[j], info->trays[j].tray_type);
                            } else {
                                lv_label_set_text(slot_materials[j], "");
                            }
                        }

                        // Update fill level - empty slots show "---", loaded slots show percentage
                        if (slot_fill_levels[j]) {
                            if (is_empty) {
                                lv_label_set_text(slot_fill_levels[j], "---");
                            } else {
                                char buf[16];
                                format_fill_level(buf, sizeof(buf), info->trays[j].remain);
                                lv_label_set_text(slot_fill_levels[j], buf);
                            }
                        }

                        int global_tray = get_global_tray_index(2, j);
                        if (slots[j]) {
                            if (global_tray == tray_now) {
                                lv_obj_set_style_border_color(slots[j], lv_color_hex(ACCENT_GREEN), 0);
                                lv_obj_set_style_border_width(slots[j], 3, 0);
                            } else {
                                lv_obj_set_style_border_color(slots[j], lv_color_hex(0x3d3d3d), 0);
                                lv_obj_set_style_border_width(slots[j], 1, 0);
                            }
                        }
                    }
                    break;
                }
            }
        } else {
            lv_obj_add_flag(objects.ams_screen_ams_panel_ams_c, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // Hide/show and update AMS D (id=3) - note the typo in EEZ: "amd_d"
    if (objects.ams_screen_ams_panel_amd_d) {
        if (has_ams[3]) {
            lv_obj_clear_flag(objects.ams_screen_ams_panel_amd_d, LV_OBJ_FLAG_HIDDEN);

            for (int i = 0; i < ams_data_count; i++) {
                if (ams_data[i].id == 3) {
                    AmsUnitCInfo *info = &ams_data[i];

                    // Update L/R extruder indicator and label position
                    update_panel_indicator(
                        objects.ams_screen_ams_panel_amd_d_indicator,
                        objects.ams_screen_ams_panel_amd_label,
                        info->extruder,
                        selected_printer_is_dual_nozzle
                    );

                    if (objects.ams_screen_ams_panel_amd_d_label_humidity) {
                        if (info->humidity >= 0) {
                            char buf[16];
                            snprintf(buf, sizeof(buf), "%d%%", info->humidity);
                            lv_label_set_text(objects.ams_screen_ams_panel_amd_d_label_humidity, buf);
                        } else {
                            lv_label_set_text(objects.ams_screen_ams_panel_amd_d_label_humidity, "--");
                        }
                    }

                    lv_obj_t *slot_colors[] = {
                        objects.ams_screen_ams_panel_amd_d_slot_1_color,
                        objects.ams_screen_ams_panel_amd_d_slot_2_color,
                        objects.ams_screen_ams_panel_amd_d_slot_3_color,
                        objects.ams_screen_ams_panel_amd_d_slot_4_color
                    };
                    lv_obj_t *slot_materials[] = {
                        objects.ams_screen_ams_panel_amd_d_slot_1_label_material,
                        objects.ams_screen_ams_panel_amd_d_slot_2_label_material,
                        objects.ams_screen_ams_panel_amd_d_slot_3_label_material,
                        objects.ams_screen_ams_panel_amd_d_slot_4_label_material
                    };
                    lv_obj_t *slot_fill_levels[] = {
                        objects.ams_screen_ams_panel_amd_d_slot_1_label_fill_level,
                        objects.ams_screen_ams_panel_amd_d_slot_2_label_fill_level,
                        objects.ams_screen_ams_panel_amd_d_slot_3_label_fill_level,
                        objects.ams_screen_ams_panel_amd_d_slot_4_label_fill_level
                    };
                    lv_obj_t *slots[] = {
                        objects.ams_screen_ams_panel_amd_d_slot_1,
                        objects.ams_screen_ams_panel_amd_d_slot_2,
                        objects.ams_screen_ams_panel_amd_d_slot_3,
                        objects.ams_screen_ams_panel_amd_d_slot_4
                    };

                    for (int j = 0; j < 4 && j < info->tray_count; j++) {
                        bool is_empty = (info->trays[j].tray_color == 0);
                        update_slot_color(slot_colors[j], info->trays[j].tray_color);

                        if (slot_materials[j]) {
                            if (!is_empty && info->trays[j].tray_type[0]) {
                                lv_label_set_text(slot_materials[j], info->trays[j].tray_type);
                            } else {
                                lv_label_set_text(slot_materials[j], "");
                            }
                        }

                        // Update fill level - empty slots show "---", loaded slots show percentage
                        if (slot_fill_levels[j]) {
                            if (is_empty) {
                                lv_label_set_text(slot_fill_levels[j], "---");
                            } else {
                                char buf[16];
                                format_fill_level(buf, sizeof(buf), info->trays[j].remain);
                                lv_label_set_text(slot_fill_levels[j], buf);
                            }
                        }

                        int global_tray = get_global_tray_index(3, j);
                        if (slots[j]) {
                            if (global_tray == tray_now) {
                                lv_obj_set_style_border_color(slots[j], lv_color_hex(ACCENT_GREEN), 0);
                                lv_obj_set_style_border_width(slots[j], 3, 0);
                            } else {
                                lv_obj_set_style_border_color(slots[j], lv_color_hex(0x3d3d3d), 0);
                                lv_obj_set_style_border_width(slots[j], 1, 0);
                            }
                        }
                    }
                    break;
                }
            }
        } else {
            lv_obj_add_flag(objects.ams_screen_ams_panel_amd_d, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // Hide/show and update HT-A (id=128)
    if (objects.ams_screen_ams_panel_ht_a) {
        if (has_ams[128]) {
            lv_obj_clear_flag(objects.ams_screen_ams_panel_ht_a, LV_OBJ_FLAG_HIDDEN);

            for (int i = 0; i < ams_data_count; i++) {
                if (ams_data[i].id == 128) {
                    AmsUnitCInfo *info = &ams_data[i];

                    // Update L/R extruder indicator and label position
                    update_panel_indicator(
                        objects.ams_screen_ams_panel_ht_a_indicator,
                        objects.ams_screen_ams_panel_ht_a_label_name,
                        info->extruder,
                        selected_printer_is_dual_nozzle
                    );

                    if (objects.ams_screen_ams_panel_ht_a_label_humidity) {
                        if (info->humidity >= 0) {
                            char buf[16];
                            snprintf(buf, sizeof(buf), "%d%%", info->humidity);
                            lv_label_set_text(objects.ams_screen_ams_panel_ht_a_label_humidity, buf);
                        } else {
                            lv_label_set_text(objects.ams_screen_ams_panel_ht_a_label_humidity, "--");
                        }
                    }

                    if (objects.ams_screen_ams_panel_ht_a_label_temperature) {
                        if (info->temperature >= 0) {
                            char buf[16];
                            snprintf(buf, sizeof(buf), "%d°C", info->temperature / 10);
                            lv_label_set_text(objects.ams_screen_ams_panel_ht_a_label_temperature, buf);
                        } else {
                            lv_label_set_text(objects.ams_screen_ams_panel_ht_a_label_temperature, "--");
                        }
                    }

                    // Single slot for HT - create stripes on first run
                    create_slot_stripes(objects.ams_screen_ams_panel_ht_a, ht_a_stripes, 14, 47);

                    if (info->tray_count > 0 && info->trays[0].tray_color != 0) {
                        // Has filament - hide stripes
                        set_slot_stripes_visible(ht_a_stripes, false);
                        update_slot_color(objects.ams_screen_ams_panel_ht_a_slot_color, info->trays[0].tray_color);

                        if (objects.ams_screen_ams_panel_ht_a_label_material) {
                            if (info->trays[0].tray_type[0]) {
                                lv_label_set_text(objects.ams_screen_ams_panel_ht_a_label_material, info->trays[0].tray_type);
                            } else {
                                lv_label_set_text(objects.ams_screen_ams_panel_ht_a_label_material, "");
                            }
                        }

                        // Update fill level (255 = unknown -> "---")
                        if (objects.ams_screen_ams_panel_ht_a_label_fill_level) {
                            char buf[16];
                            format_fill_level(buf, sizeof(buf), info->trays[0].remain);
                            lv_label_set_text(objects.ams_screen_ams_panel_ht_a_label_fill_level, buf);
                        }
                    } else {
                        // Empty slot - show stripes
                        set_slot_stripes_visible(ht_a_stripes, true);
                        update_slot_color(objects.ams_screen_ams_panel_ht_a_slot_color, 0);
                        if (objects.ams_screen_ams_panel_ht_a_label_material) {
                            lv_label_set_text(objects.ams_screen_ams_panel_ht_a_label_material, "");
                        }
                        if (objects.ams_screen_ams_panel_ht_a_label_fill_level) {
                            lv_label_set_text(objects.ams_screen_ams_panel_ht_a_label_fill_level, "");
                        }
                    }

                    int global_tray = get_global_tray_index(128, 0);
                    if (objects.ams_screen_ams_panel_ht_a_slot) {
                        if (global_tray == tray_now) {
                            lv_obj_set_style_border_color(objects.ams_screen_ams_panel_ht_a_slot, lv_color_hex(ACCENT_GREEN), 0);
                            lv_obj_set_style_border_width(objects.ams_screen_ams_panel_ht_a_slot, 3, 0);
                        } else {
                            lv_obj_set_style_border_color(objects.ams_screen_ams_panel_ht_a_slot, lv_color_hex(0x3d3d3d), 0);
                            lv_obj_set_style_border_width(objects.ams_screen_ams_panel_ht_a_slot, 1, 0);
                        }
                    }
                    break;
                }
            }
        } else {
            lv_obj_add_flag(objects.ams_screen_ams_panel_ht_a, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // Hide/show and update HT-B (id=129)
    if (objects.ams_screen_ams_panel_ht_b) {
        if (has_ams[129]) {
            lv_obj_clear_flag(objects.ams_screen_ams_panel_ht_b, LV_OBJ_FLAG_HIDDEN);

            for (int i = 0; i < ams_data_count; i++) {
                if (ams_data[i].id == 129) {
                    AmsUnitCInfo *info = &ams_data[i];

                    // Update L/R extruder indicator and label position
                    update_panel_indicator(
                        objects.ams_screen_ams_panel_ht_b_indicator,
                        objects.ams_screen_ams_panel_ht_b_label_name,
                        info->extruder,
                        selected_printer_is_dual_nozzle
                    );

                    if (objects.ams_screen_ams_panel_ht_b_label_humidity) {
                        if (info->humidity >= 0) {
                            char buf[16];
                            snprintf(buf, sizeof(buf), "%d%%", info->humidity);
                            lv_label_set_text(objects.ams_screen_ams_panel_ht_b_label_humidity, buf);
                        } else {
                            lv_label_set_text(objects.ams_screen_ams_panel_ht_b_label_humidity, "--");
                        }
                    }

                    if (objects.ams_screen_ams_panel_ht_b_label_temperature) {
                        if (info->temperature >= 0) {
                            char buf[16];
                            snprintf(buf, sizeof(buf), "%d°C", info->temperature / 10);
                            lv_label_set_text(objects.ams_screen_ams_panel_ht_b_label_temperature, buf);
                        } else {
                            lv_label_set_text(objects.ams_screen_ams_panel_ht_b_label_temperature, "--");
                        }
                    }

                    // Single slot for HT - create stripes on first run
                    create_slot_stripes(objects.ams_screen_ams_panel_ht_b, ht_b_stripes, 14, 47);

                    if (info->tray_count > 0 && info->trays[0].tray_color != 0) {
                        // Has filament - hide stripes
                        set_slot_stripes_visible(ht_b_stripes, false);
                        update_slot_color(objects.ams_screen_ams_panel_ht_b_slot_color, info->trays[0].tray_color);

                        if (objects.ams_screen_ams_panel_ht_b_label_material) {
                            if (info->trays[0].tray_type[0]) {
                                lv_label_set_text(objects.ams_screen_ams_panel_ht_b_label_material, info->trays[0].tray_type);
                            } else {
                                lv_label_set_text(objects.ams_screen_ams_panel_ht_b_label_material, "");
                            }
                        }

                        // Update fill level (255 = unknown -> "---")
                        if (objects.ams_screen_ams_panel_ht_b_label_fill_level) {
                            char buf[16];
                            format_fill_level(buf, sizeof(buf), info->trays[0].remain);
                            lv_label_set_text(objects.ams_screen_ams_panel_ht_b_label_fill_level, buf);
                        }
                    } else {
                        // Empty slot - show stripes
                        set_slot_stripes_visible(ht_b_stripes, true);
                        update_slot_color(objects.ams_screen_ams_panel_ht_b_slot_color, 0);
                        if (objects.ams_screen_ams_panel_ht_b_label_material) {
                            lv_label_set_text(objects.ams_screen_ams_panel_ht_b_label_material, "");
                        }
                        if (objects.ams_screen_ams_panel_ht_b_label_fill_level) {
                            lv_label_set_text(objects.ams_screen_ams_panel_ht_b_label_fill_level, "");
                        }
                    }

                    int global_tray = get_global_tray_index(129, 0);
                    if (objects.ams_screen_ams_panel_ht_b_slot) {
                        if (global_tray == tray_now) {
                            lv_obj_set_style_border_color(objects.ams_screen_ams_panel_ht_b_slot, lv_color_hex(ACCENT_GREEN), 0);
                            lv_obj_set_style_border_width(objects.ams_screen_ams_panel_ht_b_slot, 3, 0);
                        } else {
                            lv_obj_set_style_border_color(objects.ams_screen_ams_panel_ht_b_slot, lv_color_hex(0x3d3d3d), 0);
                            lv_obj_set_style_border_width(objects.ams_screen_ams_panel_ht_b_slot, 1, 0);
                        }
                    }
                    break;
                }
            }
        } else {
            lv_obj_add_flag(objects.ams_screen_ams_panel_ht_b, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // External spools handling
    // - Single-nozzle: show EXT-1 only (relabeled as "Ext"), no L/R indicator
    // - Dual-nozzle: show both EXT-1 (R) and EXT-2 (L) with indicators
    if (objects.ams_screen_ams_panel_ext_1) {
        // EXT-1 always shown (single or dual nozzle)
        lv_obj_clear_flag(objects.ams_screen_ams_panel_ext_1, LV_OBJ_FLAG_HIDDEN);

        if (selected_printer_is_dual_nozzle) {
            // Dual-nozzle: show "R" indicator, label as "EXT-1" at normal position
            update_extruder_indicator(objects.ams_screen_ams_panel_ext_1_indicator, 0);
            if (objects.ams_screen_ams_panel_ext_1_label_name) {
                lv_label_set_text(objects.ams_screen_ams_panel_ext_1_label_name, "EXT-1");
                lv_obj_set_x(objects.ams_screen_ams_panel_ext_1_label_name, 1);  // After badge
            }
        } else {
            // Single-nozzle: hide indicator, label as "Ext" shifted left
            if (objects.ams_screen_ams_panel_ext_1_indicator) {
                lv_obj_add_flag(objects.ams_screen_ams_panel_ext_1_indicator, LV_OBJ_FLAG_HIDDEN);
            }
            if (objects.ams_screen_ams_panel_ext_1_label_name) {
                lv_label_set_text(objects.ams_screen_ams_panel_ext_1_label_name, "Ext");
                lv_obj_set_x(objects.ams_screen_ams_panel_ext_1_label_name, -16);  // Where badge was
            }
        }
    }

    // EXT-2 only for dual-nozzle printers
    if (objects.ams_screen_ams_panel_ext_2) {
        if (selected_printer_is_dual_nozzle) {
            lv_obj_clear_flag(objects.ams_screen_ams_panel_ext_2, LV_OBJ_FLAG_HIDDEN);
            // Dual-nozzle: show "L" indicator, label at normal position
            update_extruder_indicator(objects.ams_screen_ams_panel_ext_2_indicator, 1);
            if (objects.ams_screen_ams_panel_ext_2_label_name) {
                lv_obj_set_x(objects.ams_screen_ams_panel_ext_2_label_name, 1);  // After badge
            }
        } else {
            lv_obj_add_flag(objects.ams_screen_ams_panel_ext_2, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // Update bottom bar message (matching main screen style)
    if (objects.ams_screen_bottom_bar_message) {
        int update_available = ota_is_update_available();

        if (update_available) {
            lv_label_set_text(objects.ams_screen_bottom_bar_message, "Update available! Settings -> Firmware Update");
            lv_obj_set_style_text_color(objects.ams_screen_bottom_bar_message, lv_color_hex(0xFFD700), 0);  // Gold/yellow
        } else {
            lv_label_set_text(objects.ams_screen_bottom_bar_message, "System running");
            lv_obj_set_style_text_color(objects.ams_screen_bottom_bar_message, lv_color_hex(0x666666), 0);  // Muted gray
        }

        // Update LED indicator if present
        if (objects.ams_screen_bottom_bar_led) {
            if (update_available) {
                lv_led_set_color(objects.ams_screen_bottom_bar_led, lv_color_hex(0xFFD700));  // Gold/yellow
            } else {
                lv_led_set_color(objects.ams_screen_bottom_bar_led, lv_color_hex(0x666666));  // Muted gray
                lv_led_set_brightness(objects.ams_screen_bottom_bar_led, 180);  // Dimmed
            }
        }
    }

    // Left-align second row panels (HT-A, HT-B, EXT-1, EXT-2)
    // Create a flex container for row 2 and reparent the panels into it
    static lv_obj_t *row2_container = NULL;

    // Use ams_row2_positioned flag (reset when screen changes, see above)
    if (objects.ams_screen_ams_panel && !ams_row2_positioned) {
        ams_row2_positioned = true;

        // Create a flex container for row 2 panels
        // Position at same x offset as AMS A (-16) to align with row 1
        row2_container = lv_obj_create(objects.ams_screen_ams_panel);
        lv_obj_remove_style_all(row2_container);  // Remove default styles
        lv_obj_set_size(row2_container, 700, 180);  // Wide enough for all panels
        lv_obj_set_pos(row2_container, -16, 185);  // Same x as AMS A, same Y as row 2

        // Set up flex layout - row direction, left-aligned, with gap
        lv_obj_set_flex_flow(row2_container, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row2_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_style_pad_column(row2_container, 8, 0);  // 8px gap between items

        // Make container transparent
        lv_obj_set_style_bg_opa(row2_container, 0, 0);
        lv_obj_set_style_border_width(row2_container, 0, 0);

        lv_obj_t *panels[] = {
            objects.ams_screen_ams_panel_ht_a,
            objects.ams_screen_ams_panel_ht_b,
            objects.ams_screen_ams_panel_ext_1,
            objects.ams_screen_ams_panel_ext_2
        };

        // Reparent all row 2 panels into the flex container
        for (int i = 0; i < 4; i++) {
            if (panels[i]) {
                lv_obj_set_parent(panels[i], row2_container);
                // Clear position-related styles to let flex handle positioning
                lv_obj_remove_local_style_prop(panels[i], LV_STYLE_X, 0);
                lv_obj_remove_local_style_prop(panels[i], LV_STYLE_Y, 0);
                lv_obj_remove_local_style_prop(panels[i], LV_STYLE_ALIGN, 0);
                lv_obj_remove_local_style_prop(panels[i], LV_STYLE_TRANSLATE_X, 0);
                lv_obj_remove_local_style_prop(panels[i], LV_STYLE_TRANSLATE_Y, 0);
            }
        }

        // Force layout update and redraw
        lv_obj_update_layout(row2_container);
        lv_obj_invalidate(objects.ams_overview);
    }
}

// =============================================================================
// Update Notification and Status Bar
// =============================================================================

// Track previous update state and screen to recreate dots on navigation
static int last_update_available = -1;
static int last_bell_screen = -1;

// Notification badge dots (dynamically created)
static lv_obj_t *notification_dots[16] = {NULL};
static int notification_dot_count = 0;

/**
 * @brief Animation callback for pulsing opacity
 */
static void pulse_anim_cb(void *var, int32_t value) {
    lv_obj_set_style_bg_opa((lv_obj_t *)var, (lv_opa_t)value, 0);
}

/**
 * @brief Create a notification dot badge on a bell icon with pulsing animation
 */
static lv_obj_t* create_notification_dot(lv_obj_t *bell) {
    if (!bell) return NULL;

    lv_obj_t *dot = lv_obj_create(lv_obj_get_parent(bell));
    lv_obj_set_size(dot, 8, 8);
    lv_obj_set_style_radius(dot, 4, 0);  // Circle
    lv_obj_set_style_bg_color(dot, lv_color_hex(0xFF4444), 0);  // Red dot
    lv_obj_set_style_bg_opa(dot, 255, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);

    // Position at top-right of bell icon
    lv_coord_t bell_x = lv_obj_get_x(bell);
    lv_coord_t bell_y = lv_obj_get_y(bell);
    lv_coord_t bell_w = lv_obj_get_width(bell);
    lv_obj_set_pos(dot, bell_x + bell_w - 4, bell_y - 2);

    // Add very gentle pulsing animation
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, dot);
    lv_anim_set_values(&anim, 255, 180);  // Very subtle: only 30% fade
    lv_anim_set_time(&anim, 2500);  // 2.5s per direction
    lv_anim_set_playback_time(&anim, 2500);
    lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&anim, pulse_anim_cb);
    lv_anim_start(&anim);

    return dot;
}

/**
 * @brief Clear all notification dots
 */
static void clear_notification_dots(void) {
    for (int i = 0; i < notification_dot_count; i++) {
        if (notification_dots[i]) {
            lv_obj_delete(notification_dots[i]);
            notification_dots[i] = NULL;
        }
    }
    notification_dot_count = 0;
}

/**
 * @brief Reset notification state when screens are deleted
 *
 * Called from delete_all_screens() BEFORE screens are deleted.
 * The notification dots are children of screen objects, so they will be
 * auto-deleted when the parent screen is deleted. We just need to clear
 * our tracking state to avoid dangling pointers.
 */
void reset_notification_state(void) {
    // Don't try to delete - parents will be deleted which auto-deletes children
    for (int i = 0; i < notification_dot_count; i++) {
        notification_dots[i] = NULL;
    }
    notification_dot_count = 0;
    // Reset tracking to force recreation on next screen
    last_bell_screen = -1;
}

/**
 * @brief Reset all backend UI state when screens are deleted
 *
 * Called from delete_all_screens() to clear all stale pointers.
 * This is critical because memory reuse can cause new screens to have
 * the same address as old ones, bypassing the address-based detection.
 */
void reset_backend_ui_state(void) {
    // Force reset on next main screen update by clearing last_main_screen
    last_main_screen = NULL;

    // Also directly reset all dynamic state to be safe
    status_eta_label = NULL;
    progress_pct_label = NULL;

    // Reset AMS widgets - these become invalid when parent screen is deleted
    for (int i = 0; i < MAX_AMS_WIDGETS; i++) {
        ams_widgets_left[i] = NULL;
        ams_widgets_right[i] = NULL;
    }
    ams_widget_count_left = 0;
    ams_widget_count_right = 0;
    ams_static_hidden = false;

    // Reset nozzle headers
    left_badge = NULL;
    left_label = NULL;
    right_badge = NULL;
    right_label = NULL;

    // Reset cover image state
    cover_displayed = false;

    // Reset LED animation state
    led_anim_active = false;

    // Reset slot stripes (become invalid when screen is deleted)
    for (int i = 0; i < STRIPE_COUNT; i++) {
        ht_a_stripes[i] = NULL;
        ht_b_stripes[i] = NULL;
    }
    // Reset AMS slot stripes
    for (int slot = 0; slot < 4; slot++) {
        for (int i = 0; i < STRIPE_COUNT; i++) {
            ams_a_slot_stripes[slot][i] = NULL;
            ams_b_slot_stripes[slot][i] = NULL;
            ams_c_slot_stripes[slot][i] = NULL;
            ams_d_slot_stripes[slot][i] = NULL;
        }
    }

    // Reset AMS overview screen tracking
    last_ams_screen = NULL;
    ams_row2_positioned = false;

    // Reset printer dropdown tracking
    last_printer_count = -1;
    last_connected_mask = 0;

    // Reset clock tracking to force immediate update on new screen
    last_time_hhmm = -1;

    // Reset screen tracking to force screen change detection
    previous_screen = -1;

    // Force past rate limit on next update_backend_ui call
    backend_update_counter = 1000;

    ESP_LOGI(TAG, "Reset backend UI state - cleared all stale pointers");
}

/**
 * @brief Get the bell icon for the current screen
 */
static lv_obj_t* get_current_bell_icon(int screen_id) {
    switch (screen_id) {
        case SCREEN_ID_MAIN_SCREEN:
            return objects.top_bar_notification_bell;
        case SCREEN_ID_AMS_OVERVIEW:
            return objects.ams_screen_top_bar_notofication_bell;  // EEZ typo: "notofication"
        case SCREEN_ID_SCAN_RESULT:
            return objects.scan_screen_top_bar_icon_notification_bell;
        case SCREEN_ID_SPOOL_DETAILS:
            return objects.spool_screen_top_bar_icon_notifiastion_bell;  // EEZ typo: "notifiastion"
        case SCREEN_ID_SETTINGS_SCREEN:
            return objects.settings_screen_top_bar_icon_notification_bell;
        case SCREEN_ID_SETTINGS_WIFI_SCREEN:
            return objects.settings_wifi_screen_top_bar_icon_notification_bell;
        case SCREEN_ID_SETTINGS_PRINTER_ADD_SCREEN:
            return objects.settings_printer_add_screen_top_bar_icon_notification_bell;
        case SCREEN_ID_SETTINGS_DISPLAY_SCREEN:
            return objects.settings_display_screen_top_bar_icon_notification_bell;
        case SCREEN_ID_SETTINGS_UPDATE_SCREEN:
            return objects.settings_update_screen_top_bar_icon_notification_bell;
        default:
            return NULL;
    }
}

/**
 * @brief Update notification bell to show firmware update indicator
 *
 * When a firmware update is available, a red notification dot badge appears
 * on the bell icon to indicate there are important notifications.
 * Only creates dot for the CURRENT screen's bell (other screens are deleted).
 */
static void update_notification_bell(void) {
    int update_available = ota_is_update_available();
    int screen_id = currentScreen + 1;

    // Check if we need to recreate dots (state changed OR screen changed)
    bool needs_update = (update_available != last_update_available) ||
                        (screen_id != last_bell_screen);

    if (!needs_update) {
        return;
    }

    last_update_available = update_available;
    last_bell_screen = screen_id;

    // Clear existing dots (they're invalid after screen change anyway)
    clear_notification_dots();

    if (!update_available) {
        return;
    }

    // Get the bell icon for the current screen only
    lv_obj_t *bell = get_current_bell_icon(screen_id);
    if (bell) {
        lv_obj_t *dot = create_notification_dot(bell);
        if (dot) {
            notification_dots[notification_dot_count++] = dot;
        }
    }
}

/**
 * @brief Animation callback for LED brightness pulsing
 */
static void led_pulse_anim_cb(void *var, int32_t value) {
    lv_led_set_brightness((lv_obj_t *)var, (uint8_t)value);
}

/**
 * @brief Click handler for status bar when staging is active
 */
static void staging_status_click_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    bool staging = staging_is_active();
    printf("[ui_backend] Status bar event: code=%d, staging=%d\n", code, staging);

    if (code == LV_EVENT_CLICKED) {
        if (staging) {
            printf("[ui_backend] Status bar clicked - showing popup\n");
            ui_nfc_card_show_popup();
        } else {
            printf("[ui_backend] Status bar clicked but staging not active!\n");
        }
    } else if (code == LV_EVENT_LONG_PRESSED) {
        if (staging) {
            printf("[ui_backend] Status bar long-pressed - clearing staging\n");
            staging_clear();
        }
    }
}

// Track if click handler is installed and on which object
static bool staging_click_handler_installed = false;
static lv_obj_t *staging_click_handler_target = NULL;

// Clear button for status bar
static lv_obj_t *staging_clear_btn = NULL;

static void staging_clear_btn_click(lv_event_t *e) {
    (void)e;
    printf("[ui_backend] Clear button clicked - clearing staging\n");
    staging_clear();
}

/**
 * @brief Update status bar with important messages
 *
 * Shows firmware update notification in the bottom status bar on the main screen.
 * obj0 = LED indicator dot (left side)
 * status_4 = main log message area (622px wide)
 * status_5 = "View Log >" link (73px at right)
 */
static void update_status_bar(void) {
    // Only update on main screen, and only if it's actually active
    // This prevents crashes during screen transitions
    if (!objects.main_screen || lv_scr_act() != objects.main_screen) {
        return;
    }

    // Update status_4 (main log message area) on main screen
    if (!objects.bottom_bar_message) {
        return;
    }

    int update_available = ota_is_update_available();
    bool staging_active = staging_is_active();

    // Debug: track state changes
    static bool last_staging_state = false;
    if (staging_active != last_staging_state) {
        printf("[status_bar] Staging state changed: %d -> %d\n", last_staging_state, staging_active);
        last_staging_state = staging_active;
    }

    if (staging_active) {
        // Show staging info: material + remaining time + clickable indicator
        float remaining = staging_get_remaining();
        const char *vendor = nfc_get_tag_vendor();
        const char *material = nfc_get_tag_material();
        const char *subtype = nfc_get_tag_material_subtype();

        char status_msg[128];
        if (subtype && subtype[0]) {
            snprintf(status_msg, sizeof(status_msg), LV_SYMBOL_RIGHT " %s %s %s (%.0fs) - tap to view",
                     vendor, material, subtype, remaining);
        } else {
            snprintf(status_msg, sizeof(status_msg), LV_SYMBOL_RIGHT " %s %s (%.0fs) - tap to view",
                     vendor, material, remaining);
        }

        lv_label_set_text(objects.bottom_bar_message, status_msg);
        lv_obj_set_style_text_color(objects.bottom_bar_message, lv_color_hex(0x00FF88), 0);  // Green

        // Make the bottom bar clickable
        // Reinstall handler if target object changed (screen recreated)
        if (objects.bottom_bar && objects.bottom_bar != staging_click_handler_target) {
            lv_obj_add_flag(objects.bottom_bar, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(objects.bottom_bar, staging_status_click_handler, LV_EVENT_CLICKED, NULL);
            lv_obj_add_event_cb(objects.bottom_bar, staging_status_click_handler, LV_EVENT_LONG_PRESSED, NULL);
            staging_click_handler_installed = true;
            staging_click_handler_target = objects.bottom_bar;
            printf("[status_bar] Click handler installed on bottom_bar %p\n", (void*)objects.bottom_bar);
        }

        // Show LED with green color, gentle pulse
        if (objects.bottom_bar_message_dot) {
            lv_obj_clear_flag(objects.bottom_bar_message_dot, LV_OBJ_FLAG_HIDDEN);
            lv_led_set_color(objects.bottom_bar_message_dot, lv_color_hex(0x00FF88));

            if (!led_anim_active) {
                lv_anim_t anim;
                lv_anim_init(&anim);
                lv_anim_set_var(&anim, objects.bottom_bar_message_dot);
                lv_anim_set_values(&anim, 255, 180);
                lv_anim_set_time(&anim, 2000);
                lv_anim_set_playback_time(&anim, 2000);
                lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
                lv_anim_set_exec_cb(&anim, led_pulse_anim_cb);
                lv_anim_start(&anim);
                led_anim_active = true;
            }
        }
    } else if (update_available) {
        // Set message text and color (yellow for update notification)
        lv_label_set_text(objects.bottom_bar_message, "Update available! Settings -> Firmware Update");
        lv_obj_set_style_text_color(objects.bottom_bar_message, lv_color_hex(0xFFD700), 0);  // Gold/yellow

        // Show LED, change color to match message (yellow) and add pulsing
        if (objects.bottom_bar_message_dot) {
            lv_obj_clear_flag(objects.bottom_bar_message_dot, LV_OBJ_FLAG_HIDDEN);
            lv_led_set_color(objects.bottom_bar_message_dot, lv_color_hex(0xFFD700));  // Gold/yellow

            // Add very gentle pulsing animation if not already active
            if (!led_anim_active) {
                lv_anim_t anim;
                lv_anim_init(&anim);
                lv_anim_set_var(&anim, objects.bottom_bar_message_dot);
                lv_anim_set_values(&anim, 255, 180);  // Very subtle: only 30% fade
                lv_anim_set_time(&anim, 2500);  // 2.5s per direction
                lv_anim_set_playback_time(&anim, 2500);
                lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
                lv_anim_set_exec_cb(&anim, led_pulse_anim_cb);
                lv_anim_start(&anim);
                led_anim_active = true;
            }
        }
    } else {
        // Show default status message
        printf("[status_bar] Setting 'System running' (staging=%d, update=%d)\n", staging_active, update_available);
        lv_label_set_text(objects.bottom_bar_message, "System running");
        lv_obj_set_style_text_color(objects.bottom_bar_message, lv_color_hex(0x666666), 0);  // Muted gray

        // Show LED with muted color, no pulsing
        if (objects.bottom_bar_message_dot) {
            // Stop any existing animation
            if (led_anim_active) {
                lv_anim_delete(objects.bottom_bar_message_dot, led_pulse_anim_cb);
                led_anim_active = false;
            }
            lv_obj_clear_flag(objects.bottom_bar_message_dot, LV_OBJ_FLAG_HIDDEN);
            lv_led_set_color(objects.bottom_bar_message_dot, lv_color_hex(0x666666));  // Muted gray, static
            lv_led_set_brightness(objects.bottom_bar_message_dot, 180);  // Dimmed
        }
    }
}

// =============================================================================
// Printer Selection Dropdown Handler
// =============================================================================

/**
 * @brief Event handler for printer selection dropdown
 *
 * When user selects a different printer, update the selected_printer_index
 * and force a refresh of the main screen display.
 */
static void printer_dropdown_changed(lv_event_t *e) {
    lv_obj_t *dropdown = lv_event_get_target(e);
    int dropdown_index = lv_dropdown_get_selected(dropdown);

    // Map dropdown index to actual printer index
    int new_printer_index = dropdown_index;
    if (dropdown_index >= 0 && dropdown_index < dropdown_printer_count) {
        new_printer_index = dropdown_to_printer_index[dropdown_index];
    }

    if (new_printer_index != selected_printer_index) {
        selected_printer_index = new_printer_index;

        // Check if new printer is dual-nozzle by looking at AMS extruder values
        // Only use AMS extruder assignment - active_extruder >= 0 is true for single-nozzle too
        int ams_count = backend_get_ams_count(selected_printer_index);
        selected_printer_is_dual_nozzle = false;
        for (int i = 0; i < ams_count; i++) {
            AmsUnitCInfo info;
            if (backend_get_ams_unit(selected_printer_index, i, &info) == 0) {
                if (info.extruder == 1) {  // Has left extruder AMS
                    selected_printer_is_dual_nozzle = true;
                    break;
                }
            }
        }

        // Force immediate update
        needs_data_refresh = true;
        backend_update_counter = 1000;  // Force past rate limit

        // Reset AMS display state to rebuild with new printer
        ams_static_hidden = false;
        clear_ams_widgets();
    }
}

/**
 * @brief Wire up printer dropdown on main screen
 *
 * Called from wire_main_buttons() in ui.c
 */
void wire_printer_dropdown(void) {
    if (objects.top_bar_printer_select) {
        lv_obj_add_event_cb(objects.top_bar_printer_select, printer_dropdown_changed,
                           LV_EVENT_VALUE_CHANGED, NULL);
    }
}

/**
 * @brief Wire up printer dropdown on AMS overview screen
 *
 * Called from wire_ams_overview_buttons() in ui.c
 */
void wire_ams_printer_dropdown(void) {
    if (objects.ams_screen_top_bar_printer_select) {
        lv_obj_add_event_cb(objects.ams_screen_top_bar_printer_select, printer_dropdown_changed,
                           LV_EVENT_VALUE_CHANGED, NULL);
    }
}

/**
 * @brief Initialize main screen AMS display
 *
 * Called immediately when main screen is created to hide static placeholder
 * content before backend data is available. This prevents the user from
 * seeing stale EEZ placeholder content.
 */
void init_main_screen_ams(void) {
    if (!objects.main_screen) return;

    // Hide static AMS children immediately (they will be replaced with dynamic content)
    hide_all_children(objects.main_screen_ams_left_nozzle);
    hide_all_children(objects.main_screen_ams_right_nozzle);

    // Note: Do NOT set ams_static_hidden = true here!
    // We want setup_ams_containers() to still run and create the nozzle headers.
    // The hide_all_children() above just hides the static EEZ placeholders early.
}

/**
 * @brief Get the currently selected printer index
 */
int get_selected_printer_index(void) {
    return selected_printer_index;
}

/**
 * @brief Check if the currently selected printer is dual-nozzle
 */
bool is_selected_printer_dual_nozzle(void) {
    return selected_printer_is_dual_nozzle;
}

/**
 * @brief Update the settings menu firmware update indicator
 *
 * When on the settings screen, updates the firmware menu item to show
 * update availability with color change and version text.
 */
static void update_settings_menu_indicator(void) {
    int screen_id = currentScreen + 1;

    // Only update on settings screen
    if (screen_id != SCREEN_ID_SETTINGS_SCREEN) {
        return;
    }

    // Update the version label in the firmware menu item
    if (objects.settings_screen_tabs_system_content_firmware_label_version) {
        int update_available = ota_is_update_available();
        char buf[32];

        if (update_available) {
            // Show "Update available!" in yellow/gold
            ota_get_update_version(buf, sizeof(buf));
            char version_str[48];
            snprintf(version_str, sizeof(version_str), "v%s available!", buf);
            lv_label_set_text(objects.settings_screen_tabs_system_content_firmware_label_version, version_str);
            lv_obj_set_style_text_color(objects.settings_screen_tabs_system_content_firmware_label_version,
                                        lv_color_hex(0xFFD700), 0);  // Gold/yellow

            // Also highlight the firmware menu item border
            if (objects.settings_screen_tabs_system_content_firmware) {
                lv_obj_set_style_border_color(objects.settings_screen_tabs_system_content_firmware,
                                              lv_color_hex(0xFFD700), 0);  // Gold border
                lv_obj_set_style_border_width(objects.settings_screen_tabs_system_content_firmware, 2, 0);
            }
        } else {
            // Show current version in muted gray
            ota_get_current_version(buf, sizeof(buf));
            char version_str[40];
            snprintf(version_str, sizeof(version_str), "v%s", buf);
            lv_label_set_text(objects.settings_screen_tabs_system_content_firmware_label_version, version_str);
            lv_obj_set_style_text_color(objects.settings_screen_tabs_system_content_firmware_label_version,
                                        lv_color_hex(0x888888), 0);  // Muted gray

            // Reset firmware menu item border
            if (objects.settings_screen_tabs_system_content_firmware) {
                lv_obj_set_style_border_width(objects.settings_screen_tabs_system_content_firmware, 0, 0);
            }
        }
    }
}
