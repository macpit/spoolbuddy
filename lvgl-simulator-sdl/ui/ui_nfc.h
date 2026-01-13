/**
 * NFC UI - Updates scan_result screen with NFC tag data
 */

#ifndef UI_NFC_H
#define UI_NFC_H

#include <stdbool.h>

/**
 * Poll NFC status and update scan_result screen
 * Call this from the main UI tick when scan_result screen is active
 */
void ui_nfc_update(void);

/**
 * Check if NFC tag is currently present
 */
bool ui_nfc_tag_present(void);

/**
 * Get current tag UID as string (or empty if no tag)
 */
const char* ui_nfc_get_uid_str(void);

#endif // UI_NFC_H
