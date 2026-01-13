/**
 * NFC Card UI - Main screen NFC/Scale card management
 */

#ifndef UI_NFC_CARD_H
#define UI_NFC_CARD_H

#include <stdbool.h>

/**
 * Initialize the NFC card state (call when main screen loads)
 */
void ui_nfc_card_init(void);

/**
 * Clean up NFC card dynamic elements (call when leaving main screen)
 */
void ui_nfc_card_cleanup(void);

/**
 * Update NFC card UI based on tag/scale state
 * Call this periodically when main screen is active
 */
void ui_nfc_card_update(void);

#endif // UI_NFC_CARD_H
