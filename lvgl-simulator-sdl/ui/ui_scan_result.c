/**
 * Scan Result Screen UI - Dynamic AMS display for tag encoding
 * Shows available AMS slots based on the selected printer's configuration
 */

#include "ui_internal.h"
#include "screens.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

// Accent green color - matches progress bar (#00FF00)
#define ACCENT_GREEN 0x00FF00
#define SLOT_BORDER_DEFAULT 0x555555
#define SLOT_BORDER_WIDTH_DEFAULT 2
#define SLOT_BORDER_WIDTH_SELECTED 3

// External scale functions
extern float scale_get_weight(void);
extern bool scale_is_initialized(void);

// External NFC tag data functions (from backend_client)
extern const char *nfc_get_tag_vendor(void);
extern const char *nfc_get_tag_material(void);
extern const char *nfc_get_tag_material_subtype(void);
extern const char *nfc_get_tag_color_name(void);
extern uint32_t nfc_get_tag_color_rgba(void);
extern int nfc_get_tag_spool_weight(void);
extern const char *nfc_get_tag_slicer_filament(void);
extern const char *nfc_get_tag_type(void);
extern bool staging_is_active(void);

// NFC UID function
extern void nfc_get_uid_hex(uint8_t *buf, size_t buf_len);

// SpoolInfo struct (from backend_client.h)
typedef struct {
    char id[64];            // Spool UUID
    char tag_id[64];
    char brand[32];
    char material[32];
    char subtype[32];
    char color_name[32];
    uint32_t color_rgba;
    int label_weight;
    int weight_current;
    char slicer_filament[32];
    char tag_type[32];
    bool valid;
} SpoolInfo;

// SpoolKProfile struct (from backend_client.h)
typedef struct {
    char printer_serial[32];
    int extruder;           // -1 for single-nozzle, 0=right, 1=left
    char k_value[16];       // K-factor value as string
    char name[64];          // Profile name
    int cali_idx;
} SpoolKProfile;

// External spool inventory functions
extern bool spool_exists_by_tag(const char *tag_id);
extern bool spool_get_by_tag(const char *tag_id, SpoolInfo *info);
extern bool spool_get_k_profile_for_printer(const char *spool_id, const char *printer_serial, SpoolKProfile *profile);

// Assignment result enum (from backend_client.h)
typedef enum {
    ASSIGN_RESULT_ERROR = 0,
    ASSIGN_RESULT_CONFIGURED = 1,
    ASSIGN_RESULT_STAGED = 2,
    ASSIGN_RESULT_STAGED_REPLACE = 3,
} AssignResult;

// External AMS assignment functions
extern AssignResult backend_assign_spool_to_tray(const char *printer_serial, int ams_id, int tray_id,
                                                  const char *spool_id);
extern bool backend_cancel_staged_assignment(const char *printer_serial, int ams_id, int tray_id);
extern bool backend_set_tray_calibration(const char *printer_serial, int ams_id, int tray_id,
                                          int cali_idx, const char *filament_id,
                                          const char *nozzle_diameter);

// External staging function
extern void staging_clear(void);

// External tray reading state
extern int backend_get_tray_reading_bits(int printer_index);

// External UI functions
extern void ui_set_status_message(const char *message);

// Screen navigation
extern enum ScreensEnum pendingScreen;

// Currently selected AMS slot for encoding
static int selected_ams_id = -1;      // AMS unit ID (-1 = none)
static int selected_slot_index = -1;  // Slot index within AMS (0-3)
static lv_obj_t *selected_slot_obj = NULL;  // Currently selected slot object

// Waiting for insertion state
static bool waiting_for_insertion = false;
static char waiting_printer_serial[32] = {0};
static int waiting_ams_id = -1;
static int waiting_tray_id = -1;
static double waiting_start_timestamp = 0;  // When we started waiting (for polling)

// Delayed navigation timer
static lv_timer_t *nav_delay_timer = NULL;

// Timer callback for delayed navigation to main screen
static void nav_delay_timer_cb(lv_timer_t *timer) {
    (void)timer;
    printf("[ui_scan_result] Nav delay timer fired, navigating to main screen\n");
    nav_delay_timer = NULL;
    pendingScreen = SCREEN_ID_MAIN_SCREEN;
}

// External polling function
typedef struct {
    double timestamp;
    char serial[32];
    int ams_id;
    int tray_id;
    char spool_id[64];
    bool success;
} AssignmentCompletion;
extern int backend_poll_assignment_completions(double since_timestamp, AssignmentCompletion *events, int max_events);

// Forward declaration
static void ui_scan_result_on_assignment_complete(const char *serial, int ams_id, int tray_id,
                                                   const char *spool_id, bool success);

// Captured tag data (frozen when screen opens)
static bool has_tag_data = false;
static char captured_tag_id[64] = {0};
static char captured_spool_id[64] = {0};  // Spool UUID (for K-profile lookup)
static char captured_vendor[32] = {0};
static char captured_material[32] = {0};
static char captured_subtype[32] = {0};
static char captured_color_name[32] = {0};
static uint32_t captured_color_rgba = 0;
static int captured_spool_weight = 0;
static char captured_slicer_filament[32] = {0};
static char captured_tag_type[32] = {0};
static bool captured_in_inventory = false;

// Helper to convert RGBA packed color to lv_color
static lv_color_t rgba_to_lv_color(uint32_t rgba) {
    uint8_t r = (rgba >> 24) & 0xFF;
    uint8_t g = (rgba >> 16) & 0xFF;
    uint8_t b = (rgba >> 8) & 0xFF;
    return lv_color_make(r, g, b);
}

// Helper to check if a specific slot is being read (RFID scanning)
// Returns true if the slot is being read, false otherwise
static bool is_slot_reading(int printer_index, int ams_id, int tray_id) {
    int reading_bits = backend_get_tray_reading_bits(printer_index);
    if (reading_bits < 0) {
        return false;  // Unknown state
    }

    // Calculate bit position for this slot
    // For regular AMS (0-3): bit = ams_id * 4 + tray_id
    // For AMS-HT (128-135): single slot per unit
    // For external (254, 255): single slot
    int bit_pos;
    if (ams_id <= 3) {
        bit_pos = ams_id * 4 + tray_id;
    } else if (ams_id >= 128 && ams_id <= 135) {
        // AMS-HT units - each is a single slot, starting at bit 16
        bit_pos = 16 + (ams_id - 128);
    } else if (ams_id == 254 || ams_id == 255) {
        // External slots - bits 24 and 25
        bit_pos = 24 + (ams_id - 254);
    } else {
        return false;
    }

    return (reading_bits & (1 << bit_pos)) != 0;
}

// Update the Assign & Save button state
static void update_button_state(void) {
    lv_obj_t *btn = objects.scan_screen_button_assign_save;
    if (!btn) return;

    bool enabled = has_tag_data && (selected_ams_id >= 0);

    // Reset button label to "Assign" (in case it was changed to "Cancel")
    if (objects.scan_screen_button_assign_save_label) {
        lv_label_set_text(objects.scan_screen_button_assign_save_label, "Assign");
    }

    if (enabled) {
        lv_obj_clear_state(btn, LV_STATE_DISABLED);
        lv_obj_set_style_bg_color(btn, lv_color_hex(ACCENT_GREEN), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(btn, 255, LV_PART_MAIN);
    } else {
        lv_obj_add_state(btn, LV_STATE_DISABLED);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x444444), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(btn, 180, LV_PART_MAIN);
    }
}

// Helper to get AMS slot name for display
static void get_slot_display_name(int ams_id, int tray_id, char *buf, size_t buf_len) {
    if (ams_id <= 3) {
        // Regular AMS: A1, A2, B3, etc.
        snprintf(buf, buf_len, "%c%d", 'A' + ams_id, tray_id + 1);
    } else if (ams_id >= 128 && ams_id <= 135) {
        // AMS-HT: HT-A, HT-B, etc.
        snprintf(buf, buf_len, "HT-%c", 'A' + (ams_id - 128));
    } else if (ams_id == 254) {
        snprintf(buf, buf_len, "EXT-L");
    } else if (ams_id == 255) {
        snprintf(buf, buf_len, "EXT-R");
    } else {
        snprintf(buf, buf_len, "Slot %d", tray_id + 1);
    }
}

// Spinner object for waiting state
static lv_obj_t *waiting_spinner = NULL;

// Show waiting for insertion UI
static void show_waiting_ui(const char *slot_name, bool needs_replace) {
    waiting_for_insertion = true;

    // Record when we started waiting (for polling)
    struct timeval tv;
    gettimeofday(&tv, NULL);
    waiting_start_timestamp = tv.tv_sec + tv.tv_usec / 1000000.0;

    // Update status panel message
    if (objects.scan_screen_main_panel_top_panel_label_message) {
        char msg[80];
        if (needs_replace) {
            snprintf(msg, sizeof(msg), "Replace spool in %s...", slot_name);
        } else {
            snprintf(msg, sizeof(msg), "Insert spool into %s...", slot_name);
        }
        lv_label_set_text(objects.scan_screen_main_panel_top_panel_label_message, msg);
        lv_obj_set_style_text_color(objects.scan_screen_main_panel_top_panel_label_message,
                                     lv_color_hex(0xFFFF00), 0);  // Yellow for waiting
    }

    // Create spinner but keep it HIDDEN until spool is inserted
    if (!waiting_spinner && objects.scan_screen_main_panel_top_panel) {
        waiting_spinner = lv_spinner_create(objects.scan_screen_main_panel_top_panel);
        if (waiting_spinner) {
            lv_obj_set_size(waiting_spinner, 24, 24);
            lv_obj_align(waiting_spinner, LV_ALIGN_RIGHT_MID, -10, 0);
            lv_obj_set_style_arc_width(waiting_spinner, 3, LV_PART_MAIN);
            lv_obj_set_style_arc_width(waiting_spinner, 3, LV_PART_INDICATOR);
            lv_obj_set_style_arc_color(waiting_spinner, lv_color_hex(0xFFFF00), LV_PART_MAIN);
            lv_obj_set_style_arc_color(waiting_spinner, lv_color_hex(ACCENT_GREEN), LV_PART_INDICATOR);
            lv_obj_add_flag(waiting_spinner, LV_OBJ_FLAG_HIDDEN);  // Hidden until spool inserted
            printf("[ui_scan_result] Spinner created (hidden): %p\n", (void*)waiting_spinner);
        }
    }

    // Change button to Cancel
    if (objects.scan_screen_button_assign_save_label) {
        lv_label_set_text(objects.scan_screen_button_assign_save_label, "Cancel");
    }
    if (objects.scan_screen_button_assign_save) {
        lv_obj_set_style_bg_color(objects.scan_screen_button_assign_save,
                                   lv_color_hex(0xFF6600), LV_PART_MAIN);  // Orange for cancel
    }

    printf("[ui_scan_result] Showing waiting UI for slot %s (replace=%d, timestamp=%.3f)\n",
           slot_name, needs_replace, waiting_start_timestamp);
}

// Stop the waiting spinner
static void stop_waiting_animation(void) {
    if (waiting_spinner) {
        lv_obj_delete(waiting_spinner);
        waiting_spinner = NULL;
    }
}

// Handle cancel button click (when waiting for insertion)
static void cancel_waiting(void) {
    if (!waiting_for_insertion) return;

    printf("[ui_scan_result] Cancelling staged assignment\n");

    // Stop animation
    stop_waiting_animation();

    // Cancel the staged assignment on backend
    backend_cancel_staged_assignment(waiting_printer_serial, waiting_ams_id, waiting_tray_id);

    // Reset waiting state
    waiting_for_insertion = false;
    waiting_printer_serial[0] = '\0';
    waiting_ams_id = -1;
    waiting_tray_id = -1;

    // Navigate back
    pendingScreen = SCREEN_ID_MAIN_SCREEN;
}

// Assign button click handler - sends filament + K-profile to printer
static void assign_button_click_handler(lv_event_t *e) {
    (void)e;

    printf("[ui_scan_result] === ASSIGN BUTTON CLICKED ===\n");

    // If waiting for insertion, this is a cancel click
    if (waiting_for_insertion) {
        printf("[ui_scan_result] Currently waiting - treating as cancel\n");
        cancel_waiting();
        return;
    }

    printf("[ui_scan_result] Assign: ams_id=%d, slot=%d, spool_id=%s, in_inventory=%d\n",
           selected_ams_id, selected_slot_index, captured_spool_id, captured_in_inventory);

    // Validate we have everything needed
    if (!has_tag_data || selected_ams_id < 0 || !captured_in_inventory || !captured_spool_id[0]) {
        printf("[ui_scan_result] Cannot assign: missing data (has_tag=%d, ams=%d, in_inv=%d, spool_id=%s)\n",
               has_tag_data, selected_ams_id, captured_in_inventory, captured_spool_id);
        return;
    }

    // Get selected printer serial
    int printer_idx = get_selected_printer_index();
    if (printer_idx < 0) {
        printf("[ui_scan_result] Cannot assign: no printer selected\n");
        return;
    }

    BackendPrinterInfo printer_info = {0};
    if (backend_get_printer(printer_idx, &printer_info) != 0 || !printer_info.serial[0]) {
        printf("[ui_scan_result] Cannot assign: failed to get printer info\n");
        return;
    }

    printf("[ui_scan_result] Assigning to printer %s, AMS %d, tray %d\n",
           printer_info.serial, selected_ams_id, selected_slot_index);

    // Step 1: Assign spool to tray (sends filament settings or stages)
    AssignResult assign_result = backend_assign_spool_to_tray(printer_info.serial, selected_ams_id,
                                                               selected_slot_index, captured_spool_id);

    if (assign_result == ASSIGN_RESULT_STAGED || assign_result == ASSIGN_RESULT_STAGED_REPLACE) {
        // Assignment staged - show waiting UI
        char slot_name[16];
        get_slot_display_name(selected_ams_id, selected_slot_index, slot_name, sizeof(slot_name));

        // Store waiting state
        strncpy(waiting_printer_serial, printer_info.serial, sizeof(waiting_printer_serial) - 1);
        waiting_ams_id = selected_ams_id;
        waiting_tray_id = selected_slot_index;

        // Show appropriate message based on whether replacement is needed
        bool needs_replace = (assign_result == ASSIGN_RESULT_STAGED_REPLACE);
        show_waiting_ui(slot_name, needs_replace);
        return;  // Don't navigate away - wait for insertion
    }

    if (assign_result == ASSIGN_RESULT_ERROR) {
        printf("[ui_scan_result] Failed to assign spool to tray\n");
        // Continue anyway - calibration might still work
    }

    // Step 2: Set K-profile calibration if we have one for this printer
    printf("[ui_scan_result] Step 2: Looking up K-profile for spool=%s, printer=%s\n",
           captured_spool_id, printer_info.serial);

    SpoolKProfile k_profile = {0};
    bool has_k_profile = spool_get_k_profile_for_printer(captured_spool_id, printer_info.serial, &k_profile);

    printf("[ui_scan_result] K-profile lookup result: found=%d, cali_idx=%d, k_value=%s\n",
           has_k_profile, k_profile.cali_idx, k_profile.k_value);

    if (has_k_profile && k_profile.cali_idx >= 0) {
        printf("[ui_scan_result] >>> SENDING K-PROFILE: cali_idx=%d, filament=%s, name=%s\n",
               k_profile.cali_idx, captured_slicer_filament, k_profile.name);

        bool cal_ok = backend_set_tray_calibration(printer_info.serial, selected_ams_id,
                                                    selected_slot_index, k_profile.cali_idx,
                                                    captured_slicer_filament, "0.4");
        printf("[ui_scan_result] K-profile send result: %s\n", cal_ok ? "SUCCESS" : "FAILED");
    } else {
        printf("[ui_scan_result] No K-profile to send (found=%d, cali_idx=%d)\n",
               has_k_profile, k_profile.cali_idx);
    }

    // Step 3: Clear staging (tag has been processed)
    staging_clear();

    // Step 4: Set status bar message and navigate back to main screen
    char slot_name[16];
    get_slot_display_name(selected_ams_id, selected_slot_index, slot_name, sizeof(slot_name));
    char status_msg[128];
    if (captured_subtype[0] && strcmp(captured_subtype, "Unknown") != 0) {
        snprintf(status_msg, sizeof(status_msg), "Slot %s: %s %s %s %s",
                 slot_name, captured_vendor, captured_material, captured_subtype, captured_color_name);
    } else {
        snprintf(status_msg, sizeof(status_msg), "Slot %s: %s %s %s",
                 slot_name, captured_vendor, captured_material, captured_color_name);
    }
    ui_set_status_message(status_msg);

    printf("[ui_scan_result] Assignment complete, returning to main screen\n");
    pendingScreen = SCREEN_ID_MAIN_SCREEN;
}

// Clear selection highlight from a slot (restore default border)
static void clear_slot_selection(lv_obj_t *slot) {
    if (!slot) return;
    lv_obj_set_style_border_width(slot, SLOT_BORDER_WIDTH_DEFAULT, LV_PART_MAIN);
    lv_obj_set_style_border_color(slot, lv_color_hex(SLOT_BORDER_DEFAULT), LV_PART_MAIN);
}

// Apply selection highlight to a slot
static void apply_slot_selection(lv_obj_t *slot) {
    if (!slot) return;
    lv_obj_set_style_border_width(slot, SLOT_BORDER_WIDTH_SELECTED, LV_PART_MAIN);
    lv_obj_set_style_border_color(slot, lv_color_hex(ACCENT_GREEN), LV_PART_MAIN);
}

// Slot click handler - stores the selected slot for encoding
static void slot_click_handler(lv_event_t *e) {
    lv_obj_t *slot = lv_event_get_target(e);
    int32_t ams_id = (int32_t)(intptr_t)lv_event_get_user_data(e);

    // Find which slot index was clicked (stored in object's user data)
    int slot_idx = (int)(intptr_t)lv_obj_get_user_data(slot);

    // Clear previous selection
    if (selected_slot_obj) {
        clear_slot_selection(selected_slot_obj);
    }

    // Set new selection
    selected_ams_id = ams_id;
    selected_slot_index = slot_idx;
    selected_slot_obj = slot;

    // Apply visual highlight
    apply_slot_selection(slot);

    printf("[ui_scan_result] Selected AMS %d, slot %d for encoding\n", ams_id, slot_idx);

    // Update button state
    update_button_state();
}

// Helper to set up a single slot with proper border
static void setup_slot(lv_obj_t *slot, int ams_id, int slot_idx, AmsTrayCInfo *tray) {
    if (!slot) return;

    // Store slot index in user data for click handler
    lv_obj_set_user_data(slot, (void*)(intptr_t)slot_idx);

    // Remove any existing click callbacks to prevent accumulation
    lv_obj_remove_event_cb(slot, slot_click_handler);

    // Clear any existing children (stripe objects from previous state)
    lv_obj_clean(slot);

    // Make clickable
    lv_obj_add_flag(slot, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(slot, slot_click_handler, LV_EVENT_CLICKED, (void*)(intptr_t)ams_id);

    // Set color from tray data
    bool is_empty = !tray || tray->tray_color == 0;
    if (!is_empty) {
        lv_obj_set_style_bg_color(slot, rgba_to_lv_color(tray->tray_color), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(slot, 255, LV_PART_MAIN);
    } else {
        // Empty slot - darker background with striping pattern
        lv_obj_set_style_bg_color(slot, lv_color_hex(0x2a2a2a), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(slot, 255, LV_PART_MAIN);

        // Add 3 diagonal stripe rectangles for visual indication of empty slot
        // Use fixed width (slots are typically 32-40px wide)
        for (int i = 0; i < 3; i++) {
            lv_obj_t *stripe = lv_obj_create(slot);
            lv_obj_remove_style_all(stripe);
            lv_obj_set_size(stripe, 48, 3);  // Fixed width to cover slot
            lv_obj_set_pos(stripe, -4, 8 + i * 12);
            lv_obj_set_style_bg_color(stripe, lv_color_hex(0x3a3a3a), 0);
            lv_obj_set_style_bg_opa(stripe, 255, 0);
            lv_obj_set_style_transform_rotation(stripe, -200, 0);  // Slight angle
            lv_obj_clear_flag(stripe, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        }
    }

    // Set default border (gray, visible)
    lv_obj_set_style_border_width(slot, SLOT_BORDER_WIDTH_DEFAULT, LV_PART_MAIN);
    lv_obj_set_style_border_color(slot, lv_color_hex(SLOT_BORDER_DEFAULT), LV_PART_MAIN);
    lv_obj_set_style_border_opa(slot, 255, LV_PART_MAIN);
}

// Indicator size constants
#define INDICATOR_SIZE 16


// Helper to update L/R indicator on AMS panel (same as ui_backend.c)
static void update_extruder_indicator(lv_obj_t *indicator, int8_t extruder, bool is_dual_nozzle) {
    if (!indicator) return;

    if (!is_dual_nozzle) {
        lv_obj_add_flag(indicator, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    if (extruder == 1) {
        // Left extruder - green badge with "L"
        lv_label_set_text(indicator, "L");
        lv_obj_set_size(indicator, INDICATOR_SIZE, INDICATOR_SIZE);
        lv_obj_set_style_bg_color(indicator, lv_color_hex(ACCENT_GREEN), 0);
        lv_obj_set_style_bg_opa(indicator, 255, 0);
        lv_obj_set_style_text_color(indicator, lv_color_hex(0x000000), 0);
        lv_obj_set_style_text_font(indicator, &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_align(indicator, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_pad_top(indicator, 2, 0);
        lv_obj_set_style_radius(indicator, 2, 0);
        lv_obj_clear_flag(indicator, LV_OBJ_FLAG_HIDDEN);
    } else if (extruder == 0) {
        // Right extruder - green badge with "R"
        lv_label_set_text(indicator, "R");
        lv_obj_set_size(indicator, INDICATOR_SIZE, INDICATOR_SIZE);
        lv_obj_set_style_bg_color(indicator, lv_color_hex(ACCENT_GREEN), 0);
        lv_obj_set_style_bg_opa(indicator, 255, 0);
        lv_obj_set_style_text_color(indicator, lv_color_hex(0x000000), 0);
        lv_obj_set_style_text_font(indicator, &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_align(indicator, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_pad_top(indicator, 2, 0);
        lv_obj_set_style_radius(indicator, 2, 0);
        lv_obj_clear_flag(indicator, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(indicator, LV_OBJ_FLAG_HIDDEN);
    }
}

// Helper to set up a single-slot AMS (HT or EXT)
static void setup_single_slot_ams(lv_obj_t *container, lv_obj_t *slot, lv_obj_t *indicator,
                                   int ams_id, AmsUnitCInfo *unit, bool is_dual_nozzle) {
    if (!container) return;

    lv_obj_clear_flag(container, LV_OBJ_FLAG_HIDDEN);

    if (unit && unit->tray_count > 0) {
        setup_slot(slot, ams_id, 0, &unit->trays[0]);
        update_extruder_indicator(indicator, unit->extruder, is_dual_nozzle);
    } else {
        setup_slot(slot, ams_id, 0, NULL);
        update_extruder_indicator(indicator, -1, is_dual_nozzle);
    }
}

// Helper to set up a 4-slot AMS (A, B, C, D)
static void setup_quad_slot_ams(lv_obj_t *container, lv_obj_t *slots[4], lv_obj_t *indicator,
                                 int ams_id, AmsUnitCInfo *unit, bool is_dual_nozzle) {
    if (!container) return;

    lv_obj_clear_flag(container, LV_OBJ_FLAG_HIDDEN);

    // Update L/R indicator
    update_extruder_indicator(indicator, unit ? unit->extruder : -1, is_dual_nozzle);

    // Setup all 4 slots
    for (int i = 0; i < 4; i++) {
        if (slots[i]) {
            lv_obj_clear_flag(slots[i], LV_OBJ_FLAG_HIDDEN);
            if (unit && i < unit->tray_count) {
                setup_slot(slots[i], ams_id, i, &unit->trays[i]);
            } else {
                setup_slot(slots[i], ams_id, i, NULL);
            }
        }
    }
}

// Hide all AMS panels
static void hide_all_ams_panels(void) {
    // Regular AMS units (A-D)
    if (objects.scan_screen_main_panel_ams_panel_ams_a)
        lv_obj_add_flag(objects.scan_screen_main_panel_ams_panel_ams_a, LV_OBJ_FLAG_HIDDEN);
    if (objects.scan_screen_main_panel_ams_panel_ams_b)
        lv_obj_add_flag(objects.scan_screen_main_panel_ams_panel_ams_b, LV_OBJ_FLAG_HIDDEN);
    if (objects.scan_screen_main_panel_ams_panel_ams_c)
        lv_obj_add_flag(objects.scan_screen_main_panel_ams_panel_ams_c, LV_OBJ_FLAG_HIDDEN);
    if (objects.scan_screen_main_panel_ams_panel_ams_d)
        lv_obj_add_flag(objects.scan_screen_main_panel_ams_panel_ams_d, LV_OBJ_FLAG_HIDDEN);

    // HT AMS units
    if (objects.scan_screen_main_panel_ams_panel_ht_a)
        lv_obj_add_flag(objects.scan_screen_main_panel_ams_panel_ht_a, LV_OBJ_FLAG_HIDDEN);
    if (objects.scan_screen_main_panel_ams_panel_ht_b)
        lv_obj_add_flag(objects.scan_screen_main_panel_ams_panel_ht_b, LV_OBJ_FLAG_HIDDEN);

    // External spool slots
    if (objects.scan_screen_main_panel_ams_panel_ext_l)
        lv_obj_add_flag(objects.scan_screen_main_panel_ams_panel_ext_l, LV_OBJ_FLAG_HIDDEN);
    if (objects.scan_screen_main_panel_ams_panel_ext_r)
        lv_obj_add_flag(objects.scan_screen_main_panel_ams_panel_ext_r, LV_OBJ_FLAG_HIDDEN);
}

// Capture tag data when screen opens (freezes the data)
static void capture_tag_data(void) {
    has_tag_data = false;

    // Get tag UID
    nfc_get_uid_hex((uint8_t*)captured_tag_id, sizeof(captured_tag_id));

    if (!staging_is_active() || captured_tag_id[0] == '\0') {
        // No tag - clear captured data
        captured_tag_id[0] = '\0';
        captured_spool_id[0] = '\0';
        captured_vendor[0] = '\0';
        captured_material[0] = '\0';
        captured_subtype[0] = '\0';
        captured_color_name[0] = '\0';
        captured_color_rgba = 0;
        captured_spool_weight = 0;
        captured_slicer_filament[0] = '\0';
        captured_tag_type[0] = '\0';
        captured_in_inventory = false;
        return;
    }

    has_tag_data = true;

    // Check if in inventory and get inventory data
    SpoolInfo inventory_spool = {0};
    captured_in_inventory = spool_get_by_tag(captured_tag_id, &inventory_spool);

    if (captured_in_inventory && inventory_spool.valid) {
        // Use inventory data
        strncpy(captured_spool_id, inventory_spool.id, sizeof(captured_spool_id) - 1);
        strncpy(captured_vendor, inventory_spool.brand, sizeof(captured_vendor) - 1);
        strncpy(captured_material, inventory_spool.material, sizeof(captured_material) - 1);
        strncpy(captured_subtype, inventory_spool.subtype, sizeof(captured_subtype) - 1);
        strncpy(captured_color_name, inventory_spool.color_name, sizeof(captured_color_name) - 1);
        captured_color_rgba = inventory_spool.color_rgba;
        captured_spool_weight = inventory_spool.label_weight;
        strncpy(captured_slicer_filament, inventory_spool.slicer_filament, sizeof(captured_slicer_filament) - 1);
        strncpy(captured_tag_type, inventory_spool.tag_type, sizeof(captured_tag_type) - 1);

        printf("[ui_scan_result] Using inventory data: %s (id=%s), vendor=%s, material=%s, color=%s\n",
               captured_tag_id, captured_spool_id, captured_vendor, captured_material, captured_color_name);
    } else {
        // Use NFC tag data - spool not in inventory
        captured_spool_id[0] = '\0';
        const char *str;

        str = nfc_get_tag_vendor();
        if (str) strncpy(captured_vendor, str, sizeof(captured_vendor) - 1);
        else captured_vendor[0] = '\0';

        str = nfc_get_tag_material();
        if (str) strncpy(captured_material, str, sizeof(captured_material) - 1);
        else captured_material[0] = '\0';

        str = nfc_get_tag_material_subtype();
        if (str) strncpy(captured_subtype, str, sizeof(captured_subtype) - 1);
        else captured_subtype[0] = '\0';

        str = nfc_get_tag_color_name();
        if (str) strncpy(captured_color_name, str, sizeof(captured_color_name) - 1);
        else captured_color_name[0] = '\0';

        captured_color_rgba = nfc_get_tag_color_rgba();
        captured_spool_weight = nfc_get_tag_spool_weight();

        str = nfc_get_tag_slicer_filament();
        if (str) strncpy(captured_slicer_filament, str, sizeof(captured_slicer_filament) - 1);
        else captured_slicer_filament[0] = '\0';

        str = nfc_get_tag_type();
        if (str) strncpy(captured_tag_type, str, sizeof(captured_tag_type) - 1);
        else captured_tag_type[0] = '\0';

        printf("[ui_scan_result] Using NFC tag data: %s, vendor=%s, material=%s, color=%s\n",
               captured_tag_id, captured_vendor, captured_material, captured_color_name);
    }
}

// Populate status panel (top green bar) - only show icon and tag (STATIC data)
static void populate_status_panel(void) {
    // Always show the checkmark icon
    if (objects.scan_screen_main_panel_top_panel_icon_ok) {
        lv_obj_clear_flag(objects.scan_screen_main_panel_top_panel_icon_ok, LV_OBJ_FLAG_HIDDEN);
    }

    // Show tag ID in message label - vertically centered
    if (objects.scan_screen_main_panel_top_panel_label_message) {
        if (has_tag_data) {
            char tag_str[80];
            snprintf(tag_str, sizeof(tag_str), "Tag: %s", captured_tag_id);
            lv_label_set_text(objects.scan_screen_main_panel_top_panel_label_message, tag_str);
            printf("[ui_scan_result] Status message set to: %s (static)\n", tag_str);
        } else {
            lv_label_set_text(objects.scan_screen_main_panel_top_panel_label_message, "No Tag");
            printf("[ui_scan_result] Status message set to: No Tag (static)\n");
        }
        // Vertically center the label within parent panel (override EEZ absolute position)
        lv_obj_align(objects.scan_screen_main_panel_top_panel_label_message, LV_ALIGN_LEFT_MID, 44, 0);
    }

    // Clear the status label (only show icon + tag, no secondary text)
    if (objects.scan_screen_main_panel_top_panel_label_status) {
        lv_label_set_text(objects.scan_screen_main_panel_top_panel_label_status, "");
    }
}

// Populate spool panel with captured tag data
static void populate_spool_panel(void) {
    printf("[ui_scan_result] populate_spool_panel: has_tag_data=%d\n", has_tag_data);

    if (!has_tag_data) {
        // No tag - show placeholder
        if (objects.scan_screen_main_panel_spool_panel_label_filament)
            lv_label_set_text(objects.scan_screen_main_panel_spool_panel_label_filament, "No spool");
        if (objects.scan_screen_main_panel_spool_panel_label_filament_color)
            lv_label_set_text(objects.scan_screen_main_panel_spool_panel_label_filament_color, "");
        if (objects.scan_screen_main_panel_spool_panel_label_k_factor_value)
            lv_label_set_text(objects.scan_screen_main_panel_spool_panel_label_k_factor_value, "-");
        if (objects.scan_screen_main_panel_spool_panel_label_k_profile_value)
            lv_label_set_text(objects.scan_screen_main_panel_spool_panel_label_k_profile_value, "-");
        return;
    }

    // Build filament label: "Material Subtype" (e.g., "PLA Basic")
    char filament_str[64];
    if (captured_subtype[0] && strcmp(captured_subtype, "Unknown") != 0) {
        snprintf(filament_str, sizeof(filament_str), "%s %s", captured_material, captured_subtype);
    } else if (captured_material[0]) {
        snprintf(filament_str, sizeof(filament_str), "%s", captured_material);
    } else {
        snprintf(filament_str, sizeof(filament_str), "Unknown");
    }

    // Set filament label
    if (objects.scan_screen_main_panel_spool_panel_label_filament) {
        lv_label_set_text(objects.scan_screen_main_panel_spool_panel_label_filament, filament_str);
        printf("[ui_scan_result] Filament label: %s\n", filament_str);
    }

    // Set color label - only color name (no vendor to avoid overlap)
    if (objects.scan_screen_main_panel_spool_panel_label_filament_color) {
        if (captured_color_name[0]) {
            lv_label_set_text(objects.scan_screen_main_panel_spool_panel_label_filament_color, captured_color_name);
            printf("[ui_scan_result] Color label: %s\n", captured_color_name);
        } else {
            lv_label_set_text(objects.scan_screen_main_panel_spool_panel_label_filament_color, "");
        }
    }

    // Set spool icon color
    if (objects.scan_screen_main_panel_spool_panel_icon_spool_color && captured_color_rgba != 0) {
        lv_obj_set_style_image_recolor(objects.scan_screen_main_panel_spool_panel_icon_spool_color,
                                        rgba_to_lv_color(captured_color_rgba), 0);
        lv_obj_set_style_image_recolor_opa(objects.scan_screen_main_panel_spool_panel_icon_spool_color, 255, 0);
    }

    // K-profile: Look up pressure advance calibration for this spool on selected printer
    bool k_profile_found = false;
    SpoolKProfile k_profile = {0};

    if (captured_in_inventory && captured_spool_id[0]) {
        // Get selected printer serial
        int printer_idx = get_selected_printer_index();
        if (printer_idx >= 0) {
            BackendPrinterInfo printer_info = {0};
            if (backend_get_printer(printer_idx, &printer_info) == 0 && printer_info.serial[0]) {
                // Look up K-profile for this spool on this printer
                k_profile_found = spool_get_k_profile_for_printer(captured_spool_id,
                                                                   printer_info.serial, &k_profile);
                printf("[ui_scan_result] K-profile lookup: spool=%s printer=%s found=%d\n",
                       captured_spool_id, printer_info.serial, k_profile_found);
            }
        }
    }

    // Set K factor value
    if (objects.scan_screen_main_panel_spool_panel_label_k_factor_value) {
        if (k_profile_found && k_profile.k_value[0]) {
            lv_label_set_text(objects.scan_screen_main_panel_spool_panel_label_k_factor_value, k_profile.k_value);
            printf("[ui_scan_result] K factor: %s\n", k_profile.k_value);
        } else {
            lv_label_set_text(objects.scan_screen_main_panel_spool_panel_label_k_factor_value, "-");
            printf("[ui_scan_result] K factor: - (no profile)\n");
        }
    }

    // Set K profile label (profile name from calibration)
    if (objects.scan_screen_main_panel_spool_panel_label_k_profile_value) {
        if (k_profile_found && k_profile.name[0]) {
            lv_label_set_text(objects.scan_screen_main_panel_spool_panel_label_k_profile_value, k_profile.name);
            printf("[ui_scan_result] K profile: %s\n", k_profile.name);
        } else {
            lv_label_set_text(objects.scan_screen_main_panel_spool_panel_label_k_profile_value, "-");
            printf("[ui_scan_result] K profile: -\n");
        }
    }
}

// Initialize the scan result screen with dynamic AMS data
void ui_scan_result_init(void) {
    int printer_idx = get_selected_printer_index();
    bool is_dual_nozzle = is_selected_printer_dual_nozzle();

    // Reset selection state
    selected_ams_id = -1;
    selected_slot_index = -1;
    selected_slot_obj = NULL;

    // Reset waiting state
    waiting_for_insertion = false;
    waiting_printer_serial[0] = '\0';
    waiting_ams_id = -1;
    waiting_tray_id = -1;
    waiting_spinner = NULL;  // Will be created fresh if needed

    // Clean up any pending navigation timer
    if (nav_delay_timer) {
        lv_timer_delete(nav_delay_timer);
        nav_delay_timer = NULL;
    }

    // Capture tag data (freeze it for this screen session)
    capture_tag_data();

    // Populate status panel (top bar with icon and message)
    populate_status_panel();

    // Populate spool panel with captured tag data
    populate_spool_panel();

    // Hide all AMS panels first
    hide_all_ams_panels();

    // Disable button by default
    update_button_state();

    if (printer_idx < 0) {
        // No printer selected - show message
        if (objects.scan_screen_main_panel_ams_panel_label) {
            lv_label_set_text(objects.scan_screen_main_panel_ams_panel_label, "No printer selected");
        }
        return;
    }

    // Get AMS count for selected printer
    int ams_count = backend_get_ams_count(printer_idx);
    printf("[ui_scan_result] printer_idx=%d, ams_count=%d, dual_nozzle=%d\n",
           printer_idx, ams_count, is_dual_nozzle);

    if (objects.scan_screen_main_panel_ams_panel_label) {
        lv_label_set_text(objects.scan_screen_main_panel_ams_panel_label, "Assign to AMS Slot");
    }

    // Collect AMS units by ID for proper ordering
    AmsUnitCInfo ams_units[256];
    bool has_ams[256] = {false};
    memset(ams_units, 0, sizeof(ams_units));

    for (int i = 0; i < ams_count; i++) {
        AmsUnitCInfo unit;
        int result = backend_get_ams_unit(printer_idx, i, &unit);
        if (result != 0) continue;

        has_ams[unit.id] = true;
        ams_units[unit.id] = unit;
        printf("[ui_scan_result] AMS unit: id=%d, extruder=%d, tray_count=%d\n",
               unit.id, unit.extruder, unit.tray_count);
    }

    // Layout order:
    // Row 1: AMS-A (0), AMS-B (1), HT-A (128), HT-B (129)
    // Row 2: AMS-C (2), AMS-D (3), EXT-L (254), EXT-R (255)

    // AMS A (ID 0)
    if (has_ams[0]) {
        lv_obj_t *slots[4] = {
            objects.scan_screen_main_panel_ams_panel_ams_a_slot_1,
            objects.scan_screen_main_panel_ams_panel_ams_a_slot_2,
            objects.scan_screen_main_panel_ams_panel_ams_a_slot_3,
            objects.scan_screen_main_panel_ams_panel_ams_a_slot_4
        };
        setup_quad_slot_ams(objects.scan_screen_main_panel_ams_panel_ams_a, slots,
                            objects.scan_screen_main_panel_ams_panel_ams_a_indicator,
                            0, &ams_units[0], is_dual_nozzle);
    }

    // AMS B (ID 1)
    if (has_ams[1]) {
        lv_obj_t *slots[4] = {
            objects.scan_screen_main_panel_ams_panel_ams_b_slot_1,
            objects.scan_screen_main_panel_ams_panel_ams_b_slot_2,
            objects.scan_screen_main_panel_ams_panel_ams_b_slot_3,
            objects.scan_screen_main_panel_ams_panel_ams_b_slot_4
        };
        setup_quad_slot_ams(objects.scan_screen_main_panel_ams_panel_ams_b, slots,
                            objects.scan_screen_main_panel_ams_panel_ams_b_indicator,
                            1, &ams_units[1], is_dual_nozzle);
    }

    // HT-A (ID 128) - after AMS A and B on row 1
    if (has_ams[128]) {
        setup_single_slot_ams(objects.scan_screen_main_panel_ams_panel_ht_a,
                             objects.scan_screen_main_panel_ams_panel_ht_a_slot_color,
                             objects.scan_screen_main_panel_ams_panel_ht_a_indicator,
                             128, &ams_units[128], is_dual_nozzle);
    }

    // HT-B (ID 129) - after HT-A on row 1
    if (has_ams[129]) {
        setup_single_slot_ams(objects.scan_screen_main_panel_ams_panel_ht_b,
                             objects.scan_screen_main_panel_ams_panel_ht_b_slot,
                             objects.scan_screen_main_panel_ams_panel_ht_b_indicator,
                             129, &ams_units[129], is_dual_nozzle);
    }

    // AMS C (ID 2) - start of row 2
    if (has_ams[2]) {
        lv_obj_t *slots[4] = {
            objects.scan_screen_main_panel_ams_panel_ams_c_slot_1,
            objects.scan_screen_main_panel_ams_panel_ams_c_slot_2,
            objects.scan_screen_main_panel_ams_panel_ams_c_slot_3,
            objects.scan_screen_main_panel_ams_panel_ams_c_slot_4
        };
        setup_quad_slot_ams(objects.scan_screen_main_panel_ams_panel_ams_c, slots,
                            objects.scan_screen_main_panel_ams_panel_ams_c_indicator,
                            2, &ams_units[2], is_dual_nozzle);
    }

    // AMS D (ID 3)
    if (has_ams[3]) {
        lv_obj_t *slots[4] = {
            objects.scan_screen_main_panel_ams_panel_ams_d_slot_1,
            objects.scan_screen_main_panel_ams_panel_ams_d_slot_2,
            objects.scan_screen_main_panel_ams_panel_ams_d_slot_3,
            objects.scan_screen_main_panel_ams_panel_ams_d_slot_4
        };
        setup_quad_slot_ams(objects.scan_screen_main_panel_ams_panel_ams_d, slots,
                            objects.scan_screen_main_panel_ams_panel_ams_d_indicator,
                            3, &ams_units[3], is_dual_nozzle);
    }

    // EXT-L (ID 254) - always show for dual-nozzle printers
    if (has_ams[254]) {
        setup_single_slot_ams(objects.scan_screen_main_panel_ams_panel_ext_l,
                             objects.scan_screen_main_panel_ams_panel_ext_l_slot,
                             objects.scan_screen_main_panel_ams_panel_ext_l_indicator,
                             254, &ams_units[254], is_dual_nozzle);
    } else if (is_dual_nozzle && objects.scan_screen_main_panel_ams_panel_ext_l) {
        // Show empty EXT-L for dual-nozzle printers
        lv_obj_clear_flag(objects.scan_screen_main_panel_ams_panel_ext_l, LV_OBJ_FLAG_HIDDEN);
        setup_slot(objects.scan_screen_main_panel_ams_panel_ext_l_slot, 254, 0, NULL);
        update_extruder_indicator(objects.scan_screen_main_panel_ams_panel_ext_l_indicator, 1, is_dual_nozzle);
    }

    // EXT-R (ID 255) - always show for dual-nozzle printers
    if (has_ams[255]) {
        setup_single_slot_ams(objects.scan_screen_main_panel_ams_panel_ext_r,
                             objects.scan_screen_main_panel_ams_panel_ext_r_slot,
                             objects.scan_screen_main_panel_ams_panel_ext_r_indicator,
                             255, &ams_units[255], is_dual_nozzle);
    } else if (is_dual_nozzle && objects.scan_screen_main_panel_ams_panel_ext_r) {
        // Show empty EXT-R for dual-nozzle printers
        lv_obj_clear_flag(objects.scan_screen_main_panel_ams_panel_ext_r, LV_OBJ_FLAG_HIDDEN);
        setup_slot(objects.scan_screen_main_panel_ams_panel_ext_r_slot, 255, 0, NULL);
        update_extruder_indicator(objects.scan_screen_main_panel_ams_panel_ext_r_indicator, 0, is_dual_nozzle);
    }
}

// Update scan result screen (called from ui_tick)
// Polling interval for assignment completions (in ticks, ~33ms per tick)
static uint32_t last_completion_poll_tick = 0;
#define COMPLETION_POLL_INTERVAL_TICKS 30  // ~1 second

void ui_scan_result_update(void) {
    // Update weight display with live weight from scale
    if (objects.scan_screen_main_panel_spool_panel_label_weight) {
        if (scale_is_initialized()) {
            float weight = scale_get_weight();
            char weight_str[32];
            snprintf(weight_str, sizeof(weight_str), "%.0fg", weight);
            lv_label_set_text(objects.scan_screen_main_panel_spool_panel_label_weight, weight_str);
        } else {
            lv_label_set_text(objects.scan_screen_main_panel_spool_panel_label_weight, "---g");
        }
    }

    // Update weight percentage if we have spool weight data
    if (objects.scan_screen_main_panel_spool_panel_label_weight_percentage && captured_spool_weight > 0) {
        float current_weight = scale_is_initialized() ? scale_get_weight() : 0;
        // Assume 250g empty spool weight as default
        int empty_weight = 250;
        float filament_weight = current_weight - empty_weight;
        if (filament_weight < 0) filament_weight = 0;
        int percentage = (int)((filament_weight / captured_spool_weight) * 100);
        if (percentage > 100) percentage = 100;
        if (percentage < 0) percentage = 0;

        char pct_str[16];
        snprintf(pct_str, sizeof(pct_str), "%d%%", percentage);
        lv_label_set_text(objects.scan_screen_main_panel_spool_panel_label_weight_percentage, pct_str);
    }

    // Poll for assignment completions when waiting for insertion
    if (waiting_for_insertion) {
        uint32_t now = lv_tick_get();
        if (now - last_completion_poll_tick >= COMPLETION_POLL_INTERVAL_TICKS) {
            last_completion_poll_tick = now;

            // Check if the slot is being read (RFID scanning)
            int printer_idx = get_selected_printer_index();
            bool slot_is_reading = is_slot_reading(printer_idx, waiting_ams_id, waiting_tray_id);

            // Show/hide spinner based on reading state
            if (slot_is_reading && waiting_spinner) {
                // Slot is being read - show spinner if hidden
                if (lv_obj_has_flag(waiting_spinner, LV_OBJ_FLAG_HIDDEN)) {
                    lv_obj_clear_flag(waiting_spinner, LV_OBJ_FLAG_HIDDEN);
                    printf("[ui_scan_result] Slot reading started - showing spinner\n");
                    if (objects.scan_screen_main_panel_top_panel_label_message) {
                        lv_label_set_text(objects.scan_screen_main_panel_top_panel_label_message, "Reading slot...");
                    }
                }
            }

            // Poll for completions since we started waiting
            AssignmentCompletion events[4];
            int count = backend_poll_assignment_completions(waiting_start_timestamp, events, 4);

            for (int i = 0; i < count; i++) {
                // Check if this event matches our waiting assignment
                if (strcmp(events[i].serial, waiting_printer_serial) == 0 &&
                    events[i].ams_id == waiting_ams_id &&
                    events[i].tray_id == waiting_tray_id) {

                    printf("[ui_scan_result] Received assignment completion via polling: success=%d\n",
                           events[i].success);

                    // Call the completion handler
                    ui_scan_result_on_assignment_complete(events[i].serial, events[i].ams_id,
                                                          events[i].tray_id, events[i].spool_id,
                                                          events[i].success);
                    break;
                }
            }
        }
    }
}

// Get currently selected slot info
int ui_scan_result_get_selected_ams(void) {
    return selected_ams_id;
}

int ui_scan_result_get_selected_slot(void) {
    return selected_slot_index;
}

// Check if a slot is selected and tag data is available
bool ui_scan_result_can_assign(void) {
    return has_tag_data && (selected_ams_id >= 0);
}

// Get captured tag ID
const char *ui_scan_result_get_tag_id(void) {
    return captured_tag_id;
}

// Wire up the assign button click handler
void ui_scan_result_wire_assign_button(void) {
    if (objects.scan_screen_button_assign_save) {
        // Remove any existing callback to prevent accumulation
        lv_obj_remove_event_cb(objects.scan_screen_button_assign_save, assign_button_click_handler);

        lv_obj_add_flag(objects.scan_screen_button_assign_save, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(objects.scan_screen_button_assign_save, assign_button_click_handler,
                           LV_EVENT_CLICKED, NULL);
        printf("[ui_scan_result] Assign button wired\n");
    }
}

// Check if we're waiting for insertion
bool ui_scan_result_is_waiting(void) {
    return waiting_for_insertion;
}

// Handle assignment completion from WebSocket/polling
// Called when backend sends assignment_complete message
static void ui_scan_result_on_assignment_complete(const char *serial, int ams_id, int tray_id,
                                                   const char *spool_id, bool success) {
    printf("[ui_scan_result] Assignment complete: serial=%s, ams=%d, tray=%d, success=%d\n",
           serial, ams_id, tray_id, success);

    // Check if this matches our waiting assignment
    if (!waiting_for_insertion) {
        printf("[ui_scan_result] Not waiting for insertion, ignoring\n");
        return;
    }

    if (strcmp(serial, waiting_printer_serial) != 0 ||
        ams_id != waiting_ams_id || tray_id != waiting_tray_id) {
        printf("[ui_scan_result] Assignment doesn't match our waiting slot, ignoring\n");
        return;
    }

    // Show the spinner now that spool is inserted
    if (waiting_spinner) {
        lv_obj_clear_flag(waiting_spinner, LV_OBJ_FLAG_HIDDEN);
        lv_obj_invalidate(waiting_spinner);
        printf("[ui_scan_result] Spinner unhidden\n");
    } else {
        printf("[ui_scan_result] WARNING: waiting_spinner is NULL!\n");
    }

    // Update message to show we're configuring
    if (objects.scan_screen_main_panel_top_panel_label_message) {
        lv_label_set_text(objects.scan_screen_main_panel_top_panel_label_message, "Configuring slot...");
    }

    // Reset waiting state (keep spinner visible during processing)
    waiting_for_insertion = false;
    waiting_printer_serial[0] = '\0';
    waiting_ams_id = -1;
    waiting_tray_id = -1;

    if (success) {
        // Show success message briefly
        if (objects.scan_screen_main_panel_top_panel_label_message) {
            lv_label_set_text(objects.scan_screen_main_panel_top_panel_label_message, "Slot configured!");
            lv_obj_set_style_text_color(objects.scan_screen_main_panel_top_panel_label_message,
                                         lv_color_hex(ACCENT_GREEN), 0);  // Green for success
        }

        // Set K-profile calibration if available (now that slot is configured)
        printf("[ui_scan_result] Setting K-profile after staged completion...\n");
        int printer_idx = get_selected_printer_index();
        printf("[ui_scan_result] printer_idx=%d, captured_spool_id=%s\n", printer_idx, captured_spool_id);

        if (printer_idx >= 0 && captured_spool_id[0]) {
            BackendPrinterInfo printer_info = {0};
            if (backend_get_printer(printer_idx, &printer_info) == 0) {
                printf("[ui_scan_result] Looking up K-profile: spool=%s, printer=%s\n",
                       captured_spool_id, printer_info.serial);

                SpoolKProfile k_profile = {0};
                bool has_k_profile = spool_get_k_profile_for_printer(captured_spool_id,
                                                                       printer_info.serial, &k_profile);

                printf("[ui_scan_result] K-profile lookup: found=%d, cali_idx=%d\n",
                       has_k_profile, k_profile.cali_idx);

                if (has_k_profile && k_profile.cali_idx >= 0) {
                    printf("[ui_scan_result] >>> SENDING K-PROFILE (staged): cali_idx=%d, filament=%s\n",
                           k_profile.cali_idx, captured_slicer_filament);
                    bool cal_ok = backend_set_tray_calibration(printer_info.serial, ams_id, tray_id,
                                                  k_profile.cali_idx, captured_slicer_filament, "0.4");
                    printf("[ui_scan_result] K-profile send result: %s\n", cal_ok ? "SUCCESS" : "FAILED");
                } else {
                    printf("[ui_scan_result] No K-profile to send after staged completion\n");
                }
            } else {
                printf("[ui_scan_result] Failed to get printer info\n");
            }
        } else {
            printf("[ui_scan_result] Cannot set K-profile: printer_idx=%d, spool_id=%s\n",
                   printer_idx, captured_spool_id);
        }

        // Clear staging (tag has been processed)
        staging_clear();

        // Set status bar message to show what was done
        char slot_name[16];
        get_slot_display_name(ams_id, tray_id, slot_name, sizeof(slot_name));
        char status_msg[128];
        if (captured_subtype[0] && strcmp(captured_subtype, "Unknown") != 0) {
            snprintf(status_msg, sizeof(status_msg), "Slot %s: %s %s %s %s",
                     slot_name, captured_vendor, captured_material, captured_subtype, captured_color_name);
        } else {
            snprintf(status_msg, sizeof(status_msg), "Slot %s: %s %s %s",
                     slot_name, captured_vendor, captured_material, captured_color_name);
        }
        ui_set_status_message(status_msg);

        // Navigate back to main screen
        printf("[ui_scan_result] Assignment complete, navigating to main screen\n");
        pendingScreen = SCREEN_ID_MAIN_SCREEN;
    } else {
        // Show error message
        stop_waiting_animation();  // Clean up spinner for error case
        if (objects.scan_screen_main_panel_top_panel_label_message) {
            lv_label_set_text(objects.scan_screen_main_panel_top_panel_label_message, "Configuration failed");
            lv_obj_set_style_text_color(objects.scan_screen_main_panel_top_panel_label_message,
                                         lv_color_hex(0xFF0000), 0);  // Red for error
        }
    }
}
