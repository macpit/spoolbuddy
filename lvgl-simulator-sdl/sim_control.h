/**
 * Simulator Control Functions
 * Keyboard controls for NFC and Scale simulation
 */

#ifndef SIM_CONTROL_H
#define SIM_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

// NFC Control (defined in sim_mocks.c)
void sim_set_nfc_tag_present(bool present);
void sim_set_nfc_uid(uint8_t *uid, uint8_t len);
bool sim_get_nfc_tag_present(void);

// Scale Control (defined in ui/ui_scale.c)
void sim_set_scale_weight(float weight);
void sim_set_scale_initialized(bool initialized);
void sim_set_scale_stable(bool stable);
float sim_get_scale_weight(void);

// Print help for keyboard controls
void sim_print_help(void);

#endif // SIM_CONTROL_H
