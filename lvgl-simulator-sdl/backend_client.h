/**
 * Backend Client for LVGL Simulator
 * Communicates with the SpoolBuddy Python backend via HTTP
 */

#ifndef BACKEND_CLIENT_H
#define BACKEND_CLIENT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Configuration
#define BACKEND_DEFAULT_URL "http://localhost:3000"
#define BACKEND_POLL_INTERVAL_MS 2000

// AMS Tray data
typedef struct {
    int ams_id;
    int tray_id;
    char tray_type[32];
    char tray_color[16];  // Hex color e.g. "FF0000"
    int remain;           // 0-100 percentage
    int nozzle_temp_min;
    int nozzle_temp_max;
} BackendAmsTray;

// AMS Unit data
typedef struct {
    int id;
    int humidity;         // 0-100 percentage
    int temperature;      // Celsius
    int extruder;         // 0=right, 1=left for dual nozzle
    BackendAmsTray trays[4];
    int tray_count;
} BackendAmsUnit;

// Printer state from backend
typedef struct {
    char serial[32];
    char name[64];
    char gcode_state[32];
    int print_progress;       // 0-100
    int layer_num;
    int total_layer_num;
    char subtask_name[128];
    int remaining_time;       // minutes

    // Detailed status
    int stg_cur;              // Current stage number (-1 = idle)
    char stg_cur_name[64];    // Human-readable stage name

    // AMS data
    BackendAmsUnit ams_units[8];
    int ams_unit_count;

    // Active tray indicators
    int tray_now;             // Legacy single nozzle
    int tray_now_left;        // Dual nozzle left
    int tray_now_right;       // Dual nozzle right
    int active_extruder;      // Currently printing extruder

    // Tray reading state (RFID scanning)
    int tray_reading_bits;    // Bitmask of trays being read (-1 = unknown)

    bool connected;
} BackendPrinterState;

// Device state
typedef struct {
    bool display_connected;
    float last_weight;
    bool weight_stable;
    char current_tag_id[64];
} BackendDeviceState;

// Full backend state
typedef struct {
    BackendPrinterState printers[8];
    int printer_count;
    BackendDeviceState device;
    bool backend_reachable;
    uint32_t last_update_ms;
} BackendState;

// Initialize backend client
// Returns 0 on success, -1 on error
int backend_init(const char *base_url);

// Cleanup backend client
void backend_cleanup(void);

// Set backend URL (can be changed at runtime)
void backend_set_url(const char *base_url);

// Get current backend URL
const char *backend_get_url(void);

// Poll backend for state updates
// Returns 0 on success, -1 on error
int backend_poll(void);

// Send heartbeat to backend (indicates display is connected)
int backend_send_heartbeat(void);

// Send device state to backend (weight, tag)
int backend_send_device_state(float weight, bool stable, const char *tag_id);

// Get current backend state (read-only)
const BackendState *backend_get_state(void);

// Check if backend is reachable
bool backend_is_connected(void);

// Get printer state by serial (simulator-specific)
const BackendPrinterState *backend_get_printer_by_serial(const char *serial);

// Get first connected printer (convenience)
const BackendPrinterState *backend_get_first_printer(void);

// Fetch cover image for a printer to temp file
// Returns path to temp file on success, NULL on failure
// The returned path is valid until the next call
const char *backend_fetch_cover_image(const char *serial);

// =============================================================================
// Firmware-compatible API (allows sharing ui_backend.c with firmware)
// =============================================================================

// Backend connection status (matches firmware BackendStatus)
typedef struct {
    int state;              // 0=Disconnected, 1=Discovering, 2=Connected, 3=Error
    uint8_t server_ip[4];   // Server IP address (valid when state=2)
    uint16_t server_port;   // Server port (valid when state=2)
    uint8_t printer_count;  // Number of printers cached
} BackendStatus;

// Printer info (matches firmware BackendPrinterInfo exactly - 188 bytes)
typedef struct {
    char name[32];              // 32 bytes, offset 0
    char serial[20];            // 20 bytes, offset 32
    char gcode_state[16];       // 16 bytes, offset 52
    char subtask_name[64];      // 64 bytes, offset 68
    char stg_cur_name[48];      // 48 bytes, offset 132
    uint16_t remaining_time_min; // 2 bytes, offset 180
    uint8_t print_progress;     // 1 byte, offset 182
    int8_t stg_cur;             // 1 byte, offset 183
    bool connected;             // 1 byte, offset 184
    uint8_t _pad[3];            // 3 bytes padding
} BackendPrinterInfo;

// AMS tray info (matches firmware AmsTrayCInfo)
typedef struct {
    char tray_type[16];     // Material type
    uint32_t tray_color;    // RGBA packed (0xRRGGBBAA)
    uint8_t remain;         // 0-100 percentage
} AmsTrayCInfo;

// AMS unit info (matches firmware AmsUnitCInfo)
typedef struct {
    int id;                 // AMS unit ID
    int humidity;           // -1 if not available
    int16_t temperature;    // Celsius * 10, -1 if not available
    int8_t extruder;        // -1=unknown, 0=right, 1=left
    uint8_t tray_count;     // Number of trays (1-4)
    AmsTrayCInfo trays[4];  // Tray data
} AmsUnitCInfo;

// Firmware-compatible backend functions
void backend_get_status(BackendStatus *status);
int backend_get_printer(int index, BackendPrinterInfo *info);  // Firmware-compatible
int backend_get_ams_count(int printer_index);
int backend_get_ams_unit(int printer_index, int ams_index, AmsUnitCInfo *info);
int backend_get_tray_now(int printer_index);
int backend_get_tray_now_left(int printer_index);
int backend_get_tray_now_right(int printer_index);
int backend_get_active_extruder(int printer_index);
int backend_get_tray_reading_bits(int printer_index);  // Bitmask of trays being read (-1 if unknown)
int backend_has_cover(void);
const uint8_t* backend_get_cover_data(uint32_t *size_out);

// Time functions (simulator provides system time)
int time_get_hhmm(void);
int time_is_synced(void);

// =============================================================================
// Functions implemented in ui.c (simulator's own implementation)
// =============================================================================

void sync_printers_from_backend(void);

// =============================================================================
// Spool Inventory functions (calls backend API)
// =============================================================================

// Spool info from inventory
typedef struct {
    char id[64];            // Spool UUID (needed for K-profile lookup)
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

// K-profile (pressure advance calibration) stored with a spool
typedef struct {
    char printer_serial[32];
    int extruder;           // -1 for single-nozzle, 0=right, 1=left
    char k_value[16];       // K-factor value as string (e.g., "0.040")
    char name[64];          // Profile name (e.g., "Bambu PLA Basic")
    int cali_idx;           // Calibration index on printer
} SpoolKProfile;

// Check if a spool with given tag_id exists in inventory
bool spool_exists_by_tag(const char *tag_id);

// Get spool details from inventory by tag_id
// Returns true if found, false otherwise
// Fills in the SpoolInfo struct with data
bool spool_get_by_tag(const char *tag_id, SpoolInfo *info);

// Get K-profiles for a spool
// Returns number of profiles found (0 if none), fills profiles array up to max_profiles
// The spool_id is the UUID from SpoolInfo.id
int spool_get_k_profiles(const char *spool_id, SpoolKProfile *profiles, int max_profiles);

// Get K-profile for a spool matching a specific printer
// Returns true if found, fills profile
bool spool_get_k_profile_for_printer(const char *spool_id, const char *printer_serial, SpoolKProfile *profile);

// Add a new spool to inventory
// Parameters:
//   tag_id: NFC tag UID
//   vendor: Brand name (e.g., "Bambu")
//   material: Material type (e.g., "PLA")
//   subtype: Material subtype (e.g., "Basic", "Matte")
//   color_name: Color name (e.g., "Jade White")
//   color_rgba: RGBA color value
//   label_weight: Weight on spool label in grams (e.g., 1000 for 1kg)
//   weight_current: Current weight from scale in grams
//   data_origin: Origin of data (e.g., "nfc_scan", "manual")
//   tag_type: NFC tag type (e.g., "bambu", "generic")
//   slicer_filament: Slicer filament profile ID (e.g., "GFL99")
// Returns true on success, false on failure
bool spool_add_to_inventory(const char *tag_id, const char *vendor, const char *material,
                            const char *subtype, const char *color_name, uint32_t color_rgba,
                            int label_weight, int weight_current, const char *data_origin,
                            const char *tag_type, const char *slicer_filament);

// =============================================================================
// OTA functions (mocked in simulator - implemented in sim_mocks.c)
// =============================================================================

int ota_is_update_available(void);
int ota_get_current_version(char *buf, int buf_len);
int ota_get_update_version(char *buf, int buf_len);
int ota_get_state(void);
int ota_get_progress(void);
int ota_check_for_update(void);
int ota_start_update(void);

// =============================================================================
// Staging functions (for NFC tag staging system)
// =============================================================================

bool staging_is_active(void);
float staging_get_remaining(void);
void staging_clear(void);  // Clear staging via backend API

// Tag data getters (from staged tag)
const char *nfc_get_tag_vendor(void);
const char *nfc_get_tag_material(void);
const char *nfc_get_tag_material_subtype(void);
const char *nfc_get_tag_color_name(void);
uint32_t nfc_get_tag_color_rgba(void);
int nfc_get_tag_spool_weight(void);
const char *nfc_get_tag_type(void);
const char *nfc_get_tag_slicer_filament(void);

// =============================================================================
// AMS Slot Assignment functions (configure printer AMS with filament/calibration)
// =============================================================================

// Assignment result status
typedef enum {
    ASSIGN_RESULT_ERROR = 0,      // Failed to assign
    ASSIGN_RESULT_CONFIGURED = 1, // Slot configured immediately (spool was present)
    ASSIGN_RESULT_STAGED = 2,     // Assignment staged - waiting for spool insertion
    ASSIGN_RESULT_STAGED_REPLACE = 3,  // Assignment staged - wrong spool in slot, needs replacement
} AssignResult;

// Assign a spool to an AMS tray (sends filament settings to printer)
// Parameters:
//   printer_serial: Printer serial number
//   ams_id: AMS unit ID (0-3 for regular AMS, 128-135 for AMS-HT, 254/255 for external)
//   tray_id: Tray ID within AMS (0-3 for regular AMS, 0 for HT/external)
//   spool_id: Spool UUID from inventory
// Returns:
//   ASSIGN_RESULT_CONFIGURED - slot was configured immediately
//   ASSIGN_RESULT_STAGED - assignment staged, waiting for spool insertion
//   ASSIGN_RESULT_ERROR - failed
AssignResult backend_assign_spool_to_tray(const char *printer_serial, int ams_id, int tray_id,
                                           const char *spool_id);

// Cancel a staged assignment (before spool is inserted)
// Returns true if a staged assignment was cancelled
bool backend_cancel_staged_assignment(const char *printer_serial, int ams_id, int tray_id);

// Assignment completion event (from polling)
typedef struct {
    double timestamp;
    char serial[32];
    int ams_id;
    int tray_id;
    char spool_id[64];
    bool success;
} AssignmentCompletion;

// Poll for assignment completion events
// Returns number of events found (up to max_events), fills events array
// Pass last_timestamp to only get events newer than that
int backend_poll_assignment_completions(double since_timestamp, AssignmentCompletion *events, int max_events);

// Set calibration (K-profile) for an AMS tray
// Parameters:
//   printer_serial: Printer serial number
//   ams_id: AMS unit ID
//   tray_id: Tray ID within AMS
//   cali_idx: Calibration index from SpoolKProfile (-1 for default)
//   filament_id: Filament preset ID (e.g., "GFL99")
//   nozzle_diameter: Nozzle size (e.g., "0.4")
// Returns true on success
bool backend_set_tray_calibration(const char *printer_serial, int ams_id, int tray_id,
                                   int cali_idx, const char *filament_id,
                                   const char *nozzle_diameter);

#ifdef __cplusplus
}
#endif

#endif // BACKEND_CLIENT_H
