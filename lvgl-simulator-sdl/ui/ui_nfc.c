/**
 * NFC UI - Updates scan_result screen with NFC tag data
 */

#include <stdio.h>
#include <string.h>
#include "lvgl.h"
#include "screens.h"

// External FFI functions from Rust nfc_bridge_manager
extern bool nfc_is_initialized(void);
extern bool nfc_tag_present(void);
extern uint8_t nfc_get_uid_len(void);
extern uint8_t nfc_get_uid(uint8_t *buf, uint8_t buf_len);
extern uint8_t nfc_get_uid_hex(uint8_t *buf, uint8_t buf_len);

static bool last_tag_present = false;
static char uid_str[48] = {0};

/**
 * Poll NFC status and update scan_result screen
 * Call this from the main UI tick
 */
void ui_nfc_update(void) {
    if (!nfc_is_initialized()) {
        return;
    }

    bool tag_present = nfc_tag_present();

    // Only update on state change
    if (tag_present != last_tag_present) {
        last_tag_present = tag_present;

        if (tag_present) {
            // Get UID as hex string
            uint8_t hex_buf[32];
            uint8_t len = nfc_get_uid_hex(hex_buf, sizeof(hex_buf) - 1);
            hex_buf[len] = '\0';
            snprintf(uid_str, sizeof(uid_str), "Tag: %s", (char*)hex_buf);

            // Update scan screen labels if they exist
            if (objects.scan_screen_main_panel_top_panel_label_message) {
                lv_label_set_text(objects.scan_screen_main_panel_top_panel_label_message, "NFC Tag Detected!");
            }
            if (objects.scan_screen_main_panel_top_panel_label_status) {
                lv_label_set_text(objects.scan_screen_main_panel_top_panel_label_status, uid_str);
            }
            if (objects.scan_screen_main_panel_top_panel_icon_ok) {
                lv_obj_clear_flag(objects.scan_screen_main_panel_top_panel_icon_ok, LV_OBJ_FLAG_HIDDEN);
            }
        } else {
            // Tag removed
            if (objects.scan_screen_main_panel_top_panel_label_message) {
                lv_label_set_text(objects.scan_screen_main_panel_top_panel_label_message, "Place spool on scale\nto scan & weigh...");
            }
            if (objects.scan_screen_main_panel_top_panel_label_status) {
                lv_label_set_text(objects.scan_screen_main_panel_top_panel_label_status, "Waiting for tag...");
            }
            if (objects.scan_screen_main_panel_top_panel_icon_ok) {
                lv_obj_add_flag(objects.scan_screen_main_panel_top_panel_icon_ok, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

/**
 * Check if NFC tag is currently present
 */
bool ui_nfc_tag_present(void) {
    return nfc_is_initialized() && nfc_tag_present();
}

/**
 * Get current tag UID as string (or empty if no tag)
 */
const char* ui_nfc_get_uid_str(void) {
    return uid_str;
}
