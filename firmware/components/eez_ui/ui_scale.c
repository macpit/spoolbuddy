// =============================================================================
// ui_scale.c - Scale Settings Screen Handlers
// =============================================================================
// NOTE: Scale screen has been removed from the new EEZ design.
// These functions are stubbed out for compatibility.
// =============================================================================

#include "ui_internal.h"
#include "screens.h"
#include <stdio.h>

// =============================================================================
// Scale Functions (Rust FFI on ESP32, stubs on simulator)
// =============================================================================

#ifdef ESP_PLATFORM
// ESP32: External Rust FFI functions (from scale_manager.rs)
extern float scale_get_weight(void);
extern int32_t scale_get_raw(void);
extern bool scale_is_initialized(void);
extern bool scale_is_stable(void);
extern int32_t scale_tare(void);
extern int32_t scale_calibrate(float known_weight_grams);
extern int32_t scale_get_tare_offset(void);
#else
// Simulator: Mock scale functions with controllable state
static float mock_scale_weight = 850.0f;
static int32_t mock_scale_raw = 85000;
static int32_t mock_scale_tare_offset = 0;
static bool mock_scale_initialized = true;
static bool mock_scale_stable = true;

float scale_get_weight(void) { return mock_scale_weight; }
int32_t scale_get_raw(void) { return mock_scale_raw; }
bool scale_is_initialized(void) { return mock_scale_initialized; }
bool scale_is_stable(void) { return mock_scale_stable; }
int32_t scale_tare(void) { mock_scale_tare_offset = mock_scale_raw; return 0; }
int32_t scale_calibrate(float known_weight_grams) { (void)known_weight_grams; return 0; }
int32_t scale_get_tare_offset(void) { return mock_scale_tare_offset; }

// Simulator control functions
void sim_set_scale_weight(float weight) {
    mock_scale_weight = weight;
    mock_scale_raw = (int32_t)(weight * 100);
}

void sim_set_scale_initialized(bool initialized) {
    mock_scale_initialized = initialized;
}

void sim_set_scale_stable(bool stable) {
    mock_scale_stable = stable;
}

float sim_get_scale_weight(void) {
    return mock_scale_weight;
}
#endif

// =============================================================================
// UI Update Functions (stubbed - no scale screen in new design)
// =============================================================================

void update_scale_ui(void) {
    // No scale screen in new EEZ design - nothing to update
}

// =============================================================================
// Wire Functions (stubbed - no scale screen in new design)
// =============================================================================

void wire_scale_buttons(void) {
    // No scale screen in new EEZ design - nothing to wire
}
