#ifndef UI_INTERNAL_H
#define UI_INTERNAL_H

#include <lvgl/lvgl.h>
#include "screens.h"

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Shared Type Definitions
// =============================================================================

// WiFi status from Rust
typedef struct {
    int state;       // 0=Uninitialized, 1=Disconnected, 2=Connecting, 3=Connected, 4=Error
    uint8_t ip[4];   // IP address when connected
    int8_t rssi;     // Signal strength in dBm (when connected)
} WifiStatus;

// WiFi scan result from Rust
typedef struct {
    char ssid[33];   // SSID (null-terminated)
    int8_t rssi;     // Signal strength in dBm
    uint8_t auth_mode; // 0=Open, 1=WEP, 2=WPA, 3=WPA2, 4=WPA3
} WifiScanResult;

// Printer discovery result from Rust
typedef struct {
    char name[64];      // Printer name (null-terminated)
    char serial[32];    // Serial number (null-terminated)
    char ip[16];        // IP address as string (null-terminated)
    char model[32];     // Model name (null-terminated)
} PrinterDiscoveryResult;

// Saved printer configuration
#define MAX_PRINTERS 8

typedef struct {
    char name[32];
    char serial[20];
    char access_code[12];
    char ip_address[16];
    int mqtt_state;  // 0=Disconnected, 1=Connecting, 2=Connected
} SavedPrinter;

// =============================================================================
// Extern Functions (implemented in Rust)
// =============================================================================

// WiFi functions
extern int wifi_connect(const char *ssid, const char *password);
extern void wifi_get_status(WifiStatus *status);
extern int wifi_disconnect(void);
extern int wifi_is_connected(void);
extern int wifi_get_ssid(char *buf, int buf_len);
extern int wifi_scan(WifiScanResult *results, int max_results);
extern int8_t wifi_get_rssi(void);

// Printer discovery
extern int printer_discover(PrinterDiscoveryResult *results, int max_results);

// =============================================================================
// Backend Client Types and Functions (for server communication)
// =============================================================================

// Backend connection status
typedef struct {
    int state;              // 0=Disconnected, 1=Discovering, 2=Connected, 3=Error
    uint8_t server_ip[4];   // Server IP address (valid when state=2)
    uint16_t server_port;   // Server port (valid when state=2)
    uint8_t printer_count;  // Number of printers cached
} BackendStatus;

// Printer info from backend (must match Rust PrinterInfo struct exactly)
typedef struct {
    char name[32];              // 32 bytes
    char serial[20];            // 20 bytes
    char ip_address[20];        // 20 bytes - for settings sync
    char access_code[16];       // 16 bytes - for settings sync
    char gcode_state[16];       // 16 bytes
    char subtask_name[64];      // 64 bytes
    char stg_cur_name[48];      // 48 bytes - detailed stage name
    uint16_t remaining_time_min; // 2 bytes
    uint8_t print_progress;     // 1 byte
    int8_t stg_cur;             // 1 byte - stage number (-1 = idle)
    bool connected;             // 1 byte
    uint8_t _pad[3];            // 3 bytes padding
} BackendPrinterInfo;

// Backend client functions (implemented in Rust)
extern void backend_get_status(BackendStatus *status);
extern int backend_get_printer(int index, BackendPrinterInfo *info);
extern int backend_set_url(const char *url);
extern int backend_discover_server(void);
extern int backend_is_connected(void);
extern int backend_get_printer_count(void);
extern int backend_has_cover(void);
extern const uint8_t* backend_get_cover_data(uint32_t *size_out);

// =============================================================================
// AMS Data Types and Functions (implemented in Rust)
// =============================================================================

// AMS tray info from backend
typedef struct {
    char tray_type[16];     // Material type (e.g., "PLA", "PETG")
    uint32_t tray_color;    // RGBA packed (0xRRGGBBAA)
    uint8_t remain;         // 0-100 percentage
} AmsTrayCInfo;

// AMS unit info from backend
typedef struct {
    int id;                 // AMS unit ID (0-3 for regular, 128-135 for HT)
    int humidity;           // -1 if not available, otherwise 0-100%
    int16_t temperature;    // Celsius * 10, -1 if not available
    int8_t extruder;        // -1 if not available, 0=right, 1=left
    uint8_t tray_count;     // Number of trays (1-4)
    AmsTrayCInfo trays[4];  // Tray data
} AmsUnitCInfo;

// AMS backend functions
extern int backend_get_ams_count(int printer_index);
extern int backend_get_ams_unit(int printer_index, int ams_index, AmsUnitCInfo *info);
extern int backend_get_tray_now(int printer_index);
extern int backend_get_tray_now_left(int printer_index);
extern int backend_get_tray_now_right(int printer_index);
extern int backend_get_active_extruder(int printer_index);  // -1=unknown, 0=right, 1=left

// Time manager functions (implemented in Rust)
// Returns hour in upper 8 bits, minute in lower 8 bits, or -1 if not synced
extern int time_get_hhmm(void);
extern int time_is_synced(void);

// OTA manager functions (implemented in Rust)
// Returns 1 if update available, 0 otherwise
extern int ota_is_update_available(void);
// Get current firmware version (copies to buf, returns length)
extern int ota_get_current_version(char *buf, int buf_len);
// Get available update version (copies to buf, returns length)
extern int ota_get_update_version(char *buf, int buf_len);
// Get OTA state: 0=Idle, 1=Checking, 2=Downloading, 3=Validating, 4=Flashing, 5=Complete, 6=Error
extern int ota_get_state(void);
// Get download/flash progress (0-100), -1 if not in progress state
extern int ota_get_progress(void);
// Trigger update check (non-blocking)
extern int ota_check_for_update(void);
// Start OTA update (non-blocking)
extern int ota_start_update(void);

// =============================================================================
// Shared Global Variables (defined in ui_core.c)
// =============================================================================

extern int16_t currentScreen;
extern enum ScreensEnum pendingScreen;
extern enum ScreensEnum previousScreen;
extern const char *pending_settings_detail_title;
extern int pending_settings_tab;

// =============================================================================
// Shared Printer State (defined in ui_printer.c)
// =============================================================================

extern SavedPrinter saved_printers[MAX_PRINTERS];
extern int saved_printer_count;
extern int editing_printer_index;

// =============================================================================
// Module Functions - ui_core.c
// =============================================================================

void loadScreen(enum ScreensEnum screenId);
void navigate_to_settings_detail(const char *title);
void delete_all_screens(void);

// =============================================================================
// Module Functions - ui_nvs.c
// =============================================================================

void save_printers_to_nvs(void);
void load_printers_from_nvs(void);

// =============================================================================
// Module Functions - ui_wifi.c
// =============================================================================

void wire_wifi_settings_buttons(void);
void update_wifi_ui_state(void);
void update_wifi_connect_btn_state(void);
void ui_wifi_cleanup(void);

// =============================================================================
// Module Functions - ui_printer.c
// =============================================================================

void wire_printer_add_buttons(void);
void wire_printer_edit_buttons(void);
void wire_printers_tab(void);
void update_printers_list(void);
void update_printer_edit_ui(void);
void ui_printer_cleanup(void);
void sync_printers_from_backend(void);  // Sync saved_printers with backend data

// =============================================================================
// Module Functions - ui_settings.c
// =============================================================================

void wire_settings_buttons(void);
void wire_settings_detail_buttons(void);
void wire_settings_subpage_buttons(lv_obj_t *back_btn);
void select_settings_tab(int tab_index);
void update_settings_detail_title(void);

// =============================================================================
// Module Functions - ui_scale.c
// =============================================================================

void wire_scale_buttons(void);
void update_scale_ui(void);

// =============================================================================
// Module Functions - ui_backend.c
// =============================================================================

void update_backend_ui(void);
void wire_printer_dropdown(void);
void wire_ams_printer_dropdown(void);
void init_main_screen_ams(void);      // Hide static AMS content immediately on screen load
int get_selected_printer_index(void);
bool is_selected_printer_dual_nozzle(void);
void reset_notification_state(void);  // Call before deleting screens
void reset_backend_ui_state(void);    // Reset all dynamic UI state when screens deleted

// =============================================================================
// Module Functions - ui_update.c
// =============================================================================

void wire_update_buttons(void);
void update_firmware_ui(void);

// =============================================================================
// Module Functions - ui_scan_result.c
// =============================================================================

void ui_scan_result_init(void);
void ui_scan_result_update(void);
int ui_scan_result_get_selected_ams(void);
int ui_scan_result_get_selected_slot(void);

// =============================================================================
// Module Functions - ui_core.c (wiring)
// =============================================================================

void wire_main_buttons(void);
void wire_ams_overview_buttons(void);
void wire_scan_result_buttons(void);
void wire_spool_details_buttons(void);

#ifdef __cplusplus
}
#endif

#endif // UI_INTERNAL_H
