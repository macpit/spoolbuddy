/**
 * Scan Result Screen UI - Dynamic AMS display for tag encoding
 * Shows available AMS slots based on the selected printer's configuration
 */

#include "ui_internal.h"
#include "screens.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>

// External scale functions
extern float scale_get_weight(void);
extern bool scale_is_initialized(void);

// Currently selected AMS slot for encoding
static int selected_ams_id = -1;      // AMS unit ID (-1 = none)
static int selected_slot_index = -1;  // Slot index within AMS (0-3)

// Helper to convert RGBA packed color to lv_color
static lv_color_t rgba_to_lv_color(uint32_t rgba) {
    uint8_t r = (rgba >> 24) & 0xFF;
    uint8_t g = (rgba >> 16) & 0xFF;
    uint8_t b = (rgba >> 8) & 0xFF;
    return lv_color_make(r, g, b);
}

// Slot click handler - stores the selected slot for encoding
static void slot_click_handler(lv_event_t *e) {
    lv_obj_t *slot = lv_event_get_target(e);
    int32_t ams_id = (int32_t)(intptr_t)lv_event_get_user_data(e);

    // Find which slot index was clicked (stored in object's user data)
    int slot_idx = (int)(intptr_t)lv_obj_get_user_data(slot);

    selected_ams_id = ams_id;
    selected_slot_index = slot_idx;

    printf("[ui_scan_result] Selected AMS %ld, slot %ld for encoding\n", (long)ams_id, (long)slot_idx);

    // Visual feedback - highlight selected slot
    // TODO: Add border highlight to selected slot, remove from others
}

// Helper to set up a single slot
static void setup_slot(lv_obj_t *slot, int ams_id, int slot_idx, AmsTrayCInfo *tray) {
    if (!slot) return;

    // Store slot index in user data for click handler
    lv_obj_set_user_data(slot, (void*)(intptr_t)slot_idx);

    // Make clickable
    lv_obj_add_flag(slot, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(slot, slot_click_handler, LV_EVENT_CLICKED, (void*)(intptr_t)ams_id);

    // Set color from tray data
    if (tray && tray->tray_color != 0) {
        lv_obj_set_style_bg_color(slot, rgba_to_lv_color(tray->tray_color), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(slot, 255, LV_PART_MAIN);
    } else {
        // Empty slot - dark gray
        lv_obj_set_style_bg_color(slot, lv_color_hex(0x333333), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(slot, 255, LV_PART_MAIN);
    }
}

// Helper to set up a single-slot AMS (HT or EXT)
static void setup_single_slot_ams(lv_obj_t *container, lv_obj_t *slot, int ams_id, AmsUnitCInfo *unit) {
    if (!container) return;

    if (unit && unit->tray_count > 0) {
        lv_obj_clear_flag(container, LV_OBJ_FLAG_HIDDEN);
        setup_slot(slot, ams_id, 0, &unit->trays[0]);
    } else {
        lv_obj_add_flag(container, LV_OBJ_FLAG_HIDDEN);
    }
}

// Helper to set up a 4-slot AMS (A, B, C, D)
static void setup_quad_slot_ams(lv_obj_t *container, lv_obj_t *slots[4], int ams_id, AmsUnitCInfo *unit) {
    if (!container) return;

    if (unit && unit->tray_count > 0) {
        lv_obj_clear_flag(container, LV_OBJ_FLAG_HIDDEN);
        for (int i = 0; i < 4; i++) {
            if (i < unit->tray_count) {
                setup_slot(slots[i], ams_id, i, &unit->trays[i]);
            } else if (slots[i]) {
                // Hide unused slots
                lv_obj_add_flag(slots[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
    } else {
        lv_obj_add_flag(container, LV_OBJ_FLAG_HIDDEN);
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

// Initialize the scan result screen with dynamic AMS data
void ui_scan_result_init(void) {
    int printer_idx = get_selected_printer_index();

    // Reset selection
    selected_ams_id = -1;
    selected_slot_index = -1;

    // Hide all AMS panels first
    hide_all_ams_panels();

    if (printer_idx < 0) {
        // No printer selected - show message
        if (objects.scan_screen_main_panel_ams_panel_label) {
            lv_label_set_text(objects.scan_screen_main_panel_ams_panel_label, "No printer selected");
        }
        return;
    }

    // Get AMS count for selected printer
    int ams_count = backend_get_ams_count(printer_idx);

    if (ams_count == 0) {
        // No AMS - show external spool slot
        if (objects.scan_screen_main_panel_ams_panel_label) {
            lv_label_set_text(objects.scan_screen_main_panel_ams_panel_label, "Select slot to encode:");
        }

        // Show external slot
        if (objects.scan_screen_main_panel_ams_panel_ext_l) {
            lv_obj_clear_flag(objects.scan_screen_main_panel_ams_panel_ext_l, LV_OBJ_FLAG_HIDDEN);
            setup_slot(objects.scan_screen_main_panel_ams_panel_ext_l_slot, 254, 0, NULL);
        }
        return;
    }

    if (objects.scan_screen_main_panel_ams_panel_label) {
        lv_label_set_text(objects.scan_screen_main_panel_ams_panel_label, "Select slot to encode:");
    }

    // Process each AMS unit
    for (int i = 0; i < ams_count; i++) {
        AmsUnitCInfo unit;
        if (backend_get_ams_unit(printer_idx, i, &unit) == 0) {
            continue;  // Failed to get unit info
        }

        // Determine which UI panel to use based on AMS ID
        // 0-3 = AMS A-D, 128-131 = HT A-D, 254 = External Left, 255 = External Right
        switch (unit.id) {
            case 0: {
                lv_obj_t *slots[4] = {
                    objects.scan_screen_main_panel_ams_panel_ams_a_slot_1,
                    objects.scan_screen_main_panel_ams_panel_ams_a_slot_2,
                    objects.scan_screen_main_panel_ams_panel_ams_a_slot_3,
                    objects.scan_screen_main_panel_ams_panel_ams_a_slot_4
                };
                setup_quad_slot_ams(objects.scan_screen_main_panel_ams_panel_ams_a, slots, unit.id, &unit);
                break;
            }
            case 1: {
                lv_obj_t *slots[4] = {
                    objects.scan_screen_main_panel_ams_panel_ams_b_slot_1,
                    objects.scan_screen_main_panel_ams_panel_ams_b_slot_2,
                    objects.scan_screen_main_panel_ams_panel_ams_b_slot_3,
                    objects.scan_screen_main_panel_ams_panel_ams_b_slot_4
                };
                setup_quad_slot_ams(objects.scan_screen_main_panel_ams_panel_ams_b, slots, unit.id, &unit);
                break;
            }
            case 2: {
                lv_obj_t *slots[4] = {
                    objects.scan_screen_main_panel_ams_panel_ams_c_slot_1,
                    objects.scan_screen_main_panel_ams_panel_ams_c_slot_2,
                    objects.scan_screen_main_panel_ams_panel_ams_c_slot_3,
                    objects.scan_screen_main_panel_ams_panel_ams_c_slot_4
                };
                setup_quad_slot_ams(objects.scan_screen_main_panel_ams_panel_ams_c, slots, unit.id, &unit);
                break;
            }
            case 3: {
                lv_obj_t *slots[4] = {
                    objects.scan_screen_main_panel_ams_panel_ams_d_slot_1,
                    objects.scan_screen_main_panel_ams_panel_ams_d_slot_2,
                    objects.scan_screen_main_panel_ams_panel_ams_d_slot_3,
                    objects.scan_screen_main_panel_ams_panel_ams_d_slot_4
                };
                setup_quad_slot_ams(objects.scan_screen_main_panel_ams_panel_ams_d, slots, unit.id, &unit);
                break;
            }
            case 128:  // HT A
                setup_single_slot_ams(objects.scan_screen_main_panel_ams_panel_ht_a,
                                     objects.scan_screen_main_panel_ams_panel_ht_a_slot_color,
                                     unit.id, &unit);
                break;
            case 129:  // HT B
                setup_single_slot_ams(objects.scan_screen_main_panel_ams_panel_ht_b,
                                     objects.scan_screen_main_panel_ams_panel_ht_b_slot,
                                     unit.id, &unit);
                break;
            case 254:  // External Left
                setup_single_slot_ams(objects.scan_screen_main_panel_ams_panel_ext_l,
                                     objects.scan_screen_main_panel_ams_panel_ext_l_slot,
                                     unit.id, &unit);
                break;
            case 255:  // External Right
                setup_single_slot_ams(objects.scan_screen_main_panel_ams_panel_ext_r,
                                     objects.scan_screen_main_panel_ams_panel_ext_r_slot,
                                     unit.id, &unit);
                break;
            default:
                printf("[ui_scan_result] Unknown AMS ID: %d\n", unit.id);
                break;
        }
    }
}

// Update scan result screen (called from ui_tick)
void ui_scan_result_update(void) {
    // Update weight display
    if (objects.scan_screen_main_panel_spool_panel_label_weight) {
        if (scale_is_initialized()) {
            float weight = scale_get_weight();
            char weight_str[32];
            snprintf(weight_str, sizeof(weight_str), "%.1f g", weight);
            lv_label_set_text(objects.scan_screen_main_panel_spool_panel_label_weight, weight_str);
        } else {
            lv_label_set_text(objects.scan_screen_main_panel_spool_panel_label_weight, "--- g");
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
