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
 * Poll NFC status - NO LONGER updates scan_result screen labels
 * The scan_result screen uses STATIC captured data from ui_scan_result_init()
 * This function only tracks state for ui_nfc_tag_present() and ui_nfc_get_uid_str()
 */
void ui_nfc_update(void) {
    if (!nfc_is_initialized()) {
        return;
    }

    bool tag_present = nfc_tag_present();

    // Only update internal state on state change
    if (tag_present != last_tag_present) {
        last_tag_present = tag_present;

        if (tag_present) {
            // Get UID as hex string for internal tracking only
            uint8_t hex_buf[32];
            uint8_t len = nfc_get_uid_hex(hex_buf, sizeof(hex_buf) - 1);
            hex_buf[len] = '\0';
            snprintf(uid_str, sizeof(uid_str), "Tag: %s", (char*)hex_buf);
        } else {
            uid_str[0] = '\0';
        }
        // NOTE: Do NOT update scan_result screen labels here!
        // The scan_result screen captures tag data statically in ui_scan_result_init()
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
