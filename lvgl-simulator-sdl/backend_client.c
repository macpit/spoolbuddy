/**
 * Backend Client for LVGL Simulator
 * Uses libcurl to communicate with the SpoolBuddy Python backend
 */

#include "backend_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <curl/curl.h>

// cJSON header location varies: Homebrew uses cjson/cJSON.h, FetchContent uses cJSON.h
#if __has_include(<cjson/cJSON.h>)
#include <cjson/cJSON.h>
#else
#include "cJSON.h"
#endif

// Backend state
static BackendState g_state = {0};
static char g_base_url[256] = BACKEND_DEFAULT_URL;
static CURL *g_curl = NULL;

// NFC state (synced from real device via backend, or toggled with 'N' key)
static bool g_nfc_initialized = true;
static bool g_nfc_tag_present = false;
static uint8_t g_nfc_uid[7] = {0x87, 0x0D, 0x51, 0x00, 0x00, 0x00, 0x00};
static uint8_t g_nfc_uid_len = 4;

// Decoded tag data - synced from backend
static char g_tag_vendor[32] = "";
static char g_tag_material[32] = "";
static char g_tag_material_subtype[32] = "";
static char g_tag_color_name[32] = "";
static uint32_t g_tag_color_rgba = 0;
static int g_tag_spool_weight = 0;
static char g_tag_type[32] = "";
static char g_tag_slicer_filament[32] = "";

// Staging state - separate from raw NFC tag detection
// UI should use staging_is_active() for popup control
static bool g_staging_active = false;
static float g_staging_remaining = 0;

// When staging is cleared locally, prevent poll from overwriting for a few seconds
static bool g_staging_cleared_locally = false;
static time_t g_staging_cleared_time = 0;
#define STAGING_CLEAR_HOLDOFF_SEC 3

// Response buffer for curl
typedef struct {
    char *data;
    size_t size;
} ResponseBuffer;

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    ResponseBuffer *buf = (ResponseBuffer *)userp;

    char *ptr = realloc(buf->data, buf->size + realsize + 1);
    if (!ptr) {
        fprintf(stderr, "[backend] realloc failed\n");
        return 0;
    }

    buf->data = ptr;
    memcpy(&(buf->data[buf->size]), contents, realsize);
    buf->size += realsize;
    buf->data[buf->size] = 0;

    return realsize;
}

int backend_init(const char *base_url) {
    if (base_url) {
        strncpy(g_base_url, base_url, sizeof(g_base_url) - 1);
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);
    g_curl = curl_easy_init();

    if (!g_curl) {
        fprintf(stderr, "[backend] Failed to init curl\n");
        return -1;
    }

    memset(&g_state, 0, sizeof(g_state));
    printf("[backend] Initialized with URL: %s\n", g_base_url);
    return 0;
}

void backend_cleanup(void) {
    if (g_curl) {
        curl_easy_cleanup(g_curl);
        g_curl = NULL;
    }
    curl_global_cleanup();
    printf("[backend] Cleanup complete\n");
}

void backend_set_url(const char *base_url) {
    if (base_url) {
        strncpy(g_base_url, base_url, sizeof(g_base_url) - 1);
        printf("[backend] URL set to: %s\n", g_base_url);
    }
}

const char *backend_get_url(void) {
    return g_base_url;
}

// Parse AMS tray from JSON
static void parse_ams_tray(cJSON *tray_json, BackendAmsTray *tray) {
    cJSON *item;

    item = cJSON_GetObjectItem(tray_json, "ams_id");
    tray->ams_id = item ? item->valueint : 0;

    item = cJSON_GetObjectItem(tray_json, "tray_id");
    tray->tray_id = item ? item->valueint : 0;

    item = cJSON_GetObjectItem(tray_json, "tray_type");
    if (item && item->valuestring) {
        strncpy(tray->tray_type, item->valuestring, sizeof(tray->tray_type) - 1);
    }

    item = cJSON_GetObjectItem(tray_json, "tray_color");
    if (item && item->valuestring) {
        strncpy(tray->tray_color, item->valuestring, sizeof(tray->tray_color) - 1);
    }

    item = cJSON_GetObjectItem(tray_json, "remain");
    tray->remain = item ? item->valueint : 0;

    item = cJSON_GetObjectItem(tray_json, "nozzle_temp_min");
    tray->nozzle_temp_min = item ? item->valueint : 0;

    item = cJSON_GetObjectItem(tray_json, "nozzle_temp_max");
    tray->nozzle_temp_max = item ? item->valueint : 0;
}

// Parse AMS unit from JSON
static void parse_ams_unit(cJSON *unit_json, BackendAmsUnit *unit) {
    cJSON *item;

    item = cJSON_GetObjectItem(unit_json, "id");
    unit->id = item ? item->valueint : 0;

    item = cJSON_GetObjectItem(unit_json, "humidity");
    unit->humidity = item && !cJSON_IsNull(item) ? item->valueint : -1;

    item = cJSON_GetObjectItem(unit_json, "temperature");
    unit->temperature = item && !cJSON_IsNull(item) ? item->valueint : -1;

    item = cJSON_GetObjectItem(unit_json, "extruder");
    unit->extruder = item && !cJSON_IsNull(item) ? item->valueint : -1;

    cJSON *trays = cJSON_GetObjectItem(unit_json, "trays");
    unit->tray_count = 0;
    if (trays && cJSON_IsArray(trays)) {
        cJSON *tray_json;
        cJSON_ArrayForEach(tray_json, trays) {
            if (unit->tray_count < 4) {
                parse_ams_tray(tray_json, &unit->trays[unit->tray_count]);
                unit->tray_count++;
            }
        }
    }
}

// Parse printer state from JSON
static void parse_printer_state(cJSON *state_json, BackendPrinterState *printer) {
    cJSON *item;

    item = cJSON_GetObjectItem(state_json, "gcode_state");
    if (item && item->valuestring) {
        strncpy(printer->gcode_state, item->valuestring, sizeof(printer->gcode_state) - 1);
    }

    item = cJSON_GetObjectItem(state_json, "print_progress");
    printer->print_progress = item && !cJSON_IsNull(item) ? item->valueint : 0;

    item = cJSON_GetObjectItem(state_json, "layer_num");
    printer->layer_num = item && !cJSON_IsNull(item) ? item->valueint : 0;

    item = cJSON_GetObjectItem(state_json, "total_layer_num");
    printer->total_layer_num = item && !cJSON_IsNull(item) ? item->valueint : 0;

    item = cJSON_GetObjectItem(state_json, "subtask_name");
    if (item && item->valuestring) {
        strncpy(printer->subtask_name, item->valuestring, sizeof(printer->subtask_name) - 1);
    }

    item = cJSON_GetObjectItem(state_json, "mc_remaining_time");
    printer->remaining_time = item && !cJSON_IsNull(item) ? item->valueint : 0;

    // Detailed status (stg_cur)
    item = cJSON_GetObjectItem(state_json, "stg_cur");
    printer->stg_cur = item && !cJSON_IsNull(item) ? item->valueint : -1;

    item = cJSON_GetObjectItem(state_json, "stg_cur_name");
    memset(printer->stg_cur_name, 0, sizeof(printer->stg_cur_name));
    if (item && cJSON_IsString(item) && item->valuestring) {
        strncpy(printer->stg_cur_name, item->valuestring, sizeof(printer->stg_cur_name) - 1);
    }

    // Active tray indicators
    item = cJSON_GetObjectItem(state_json, "tray_now");
    printer->tray_now = item && !cJSON_IsNull(item) ? item->valueint : -1;

    item = cJSON_GetObjectItem(state_json, "tray_now_left");
    printer->tray_now_left = item && !cJSON_IsNull(item) ? item->valueint : -1;

    item = cJSON_GetObjectItem(state_json, "tray_now_right");
    printer->tray_now_right = item && !cJSON_IsNull(item) ? item->valueint : -1;

    item = cJSON_GetObjectItem(state_json, "active_extruder");
    printer->active_extruder = item && !cJSON_IsNull(item) ? item->valueint : -1;

    item = cJSON_GetObjectItem(state_json, "tray_reading_bits");
    printer->tray_reading_bits = item && !cJSON_IsNull(item) ? item->valueint : -1;

    // Parse AMS units
    cJSON *ams_units = cJSON_GetObjectItem(state_json, "ams_units");
    printer->ams_unit_count = 0;
    if (ams_units && cJSON_IsArray(ams_units)) {
        cJSON *unit_json;
        cJSON_ArrayForEach(unit_json, ams_units) {
            if (printer->ams_unit_count < 8) {
                parse_ams_unit(unit_json, &printer->ams_units[printer->ams_unit_count]);
                printer->ams_unit_count++;
            }
        }
    }
}

// Fetch JSON from URL
static cJSON *fetch_json(const char *url) {
    if (!g_curl) return NULL;

    ResponseBuffer buf = {0};
    buf.data = malloc(1);
    buf.size = 0;

    // Reset curl options to ensure GET (previous calls may have set POST)
    curl_easy_reset(g_curl);
    curl_easy_setopt(g_curl, CURLOPT_URL, url);
    curl_easy_setopt(g_curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(g_curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(g_curl, CURLOPT_TIMEOUT, 2L);
    curl_easy_setopt(g_curl, CURLOPT_CONNECTTIMEOUT, 1L);

    CURLcode res = curl_easy_perform(g_curl);

    if (res != CURLE_OK) {
        free(buf.data);
        return NULL;
    }

    cJSON *json = cJSON_Parse(buf.data);
    free(buf.data);
    return json;
}

int backend_send_heartbeat(void) {
    char url[512];
    snprintf(url, sizeof(url), "%s/api/display/heartbeat", g_base_url);

    cJSON *json = fetch_json(url);
    if (json) {
        cJSON_Delete(json);
        return 0;
    }
    return -1;
}

int backend_send_device_state(float weight, bool stable, const char *tag_id) {
    if (!g_curl) return -1;

    char url[512];
    if (tag_id && tag_id[0]) {
        snprintf(url, sizeof(url), "%s/api/display/state?weight=%.1f&stable=%s&tag_id=%s",
                 g_base_url, weight, stable ? "true" : "false", tag_id);
    } else {
        snprintf(url, sizeof(url), "%s/api/display/state?weight=%.1f&stable=%s",
                 g_base_url, weight, stable ? "true" : "false");
    }

    ResponseBuffer response = {0};

    curl_easy_reset(g_curl);
    curl_easy_setopt(g_curl, CURLOPT_URL, url);
    curl_easy_setopt(g_curl, CURLOPT_POST, 1L);
    curl_easy_setopt(g_curl, CURLOPT_POSTFIELDS, "");
    curl_easy_setopt(g_curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(g_curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(g_curl, CURLOPT_TIMEOUT, 2L);

    CURLcode res = curl_easy_perform(g_curl);
    free(response.data);

    return (res == CURLE_OK) ? 0 : -1;
}

int backend_poll(void) {
    // Send heartbeat first
    backend_send_heartbeat();

    // Fetch printer states
    char url[512];
    snprintf(url, sizeof(url), "%s/api/printers", g_base_url);

    cJSON *json = fetch_json(url);
    if (!json) {
        g_state.backend_reachable = false;
        return -1;
    }

    g_state.backend_reachable = true;
    g_state.printer_count = 0;

    // Parse printer list
    if (cJSON_IsArray(json)) {
        cJSON *printer_json;
        cJSON_ArrayForEach(printer_json, json) {
            if (g_state.printer_count >= 8) break;

            BackendPrinterState *printer = &g_state.printers[g_state.printer_count];
            memset(printer, 0, sizeof(*printer));

            cJSON *item = cJSON_GetObjectItem(printer_json, "serial");
            if (item && item->valuestring) {
                strncpy(printer->serial, item->valuestring, sizeof(printer->serial) - 1);
            }

            item = cJSON_GetObjectItem(printer_json, "name");
            if (item && item->valuestring) {
                strncpy(printer->name, item->valuestring, sizeof(printer->name) - 1);
            }

            item = cJSON_GetObjectItem(printer_json, "connected");
            printer->connected = item ? cJSON_IsTrue(item) : false;

            // Parse state fields directly from printer object (not nested)
            // The backend returns state fields at top level, not in a "state" object
            parse_printer_state(printer_json, printer);

            g_state.printer_count++;
        }
    }

    cJSON_Delete(json);

    // Fetch device status (includes real device's tag data)
    snprintf(url, sizeof(url), "%s/api/display/status", g_base_url);
    json = fetch_json(url);
    if (json) {
        cJSON *item = cJSON_GetObjectItem(json, "connected");
        g_state.device.display_connected = item ? cJSON_IsTrue(item) : false;

        // Sync NFC state from staging system
        // Use staging_remaining to determine if tag is "present" - more stable than raw tag_data
        cJSON *staging_remaining = cJSON_GetObjectItem(json, "staging_remaining");
        cJSON *tag_data = cJSON_GetObjectItem(json, "tag_data");

        float remaining = staging_remaining ? staging_remaining->valuedouble : 0;
        // Only check if staging is active (remaining > 0), NOT if tag_data exists
        bool has_staged_tag = remaining > 0;

        // Update global staging state (used by UI directly)
        // But respect local clear holdoff to prevent race condition
        if (g_staging_cleared_locally) {
            time_t now = time(NULL);
            if (difftime(now, g_staging_cleared_time) < STAGING_CLEAR_HOLDOFF_SEC) {
                // Within holdoff period - don't overwrite cleared state
                printf("[backend] Ignoring staging update (holdoff active: %.0fs remaining)\n",
                       STAGING_CLEAR_HOLDOFF_SEC - difftime(now, g_staging_cleared_time));
            } else {
                // Holdoff expired - resume normal updates
                g_staging_cleared_locally = false;
                g_staging_active = has_staged_tag;
                g_staging_remaining = remaining;
            }
        } else {
            g_staging_active = has_staged_tag;
            g_staging_remaining = remaining;
        }

        printf("[backend] Staging: remaining=%.1fs, has_staged_tag=%s\n",
               remaining, has_staged_tag ? "YES" : "no");

        if (has_staged_tag) {
            // Real device has a tag - sync to simulator
            bool was_present = g_nfc_tag_present;
            g_nfc_tag_present = true;
            if (!was_present) {
                printf("[backend] NFC tag synced from device - popup should appear\n");
            }

            item = cJSON_GetObjectItem(tag_data, "uid");
            if (item && item->valuestring) {
                // Parse UID hex string into bytes
                const char *uid_str = item->valuestring;
                g_nfc_uid_len = 0;
                for (int i = 0; uid_str[i] && uid_str[i+1] && g_nfc_uid_len < 7; i += 2) {
                    if (uid_str[i] == ':') { i--; continue; }
                    char hex[3] = {uid_str[i], uid_str[i+1], 0};
                    g_nfc_uid[g_nfc_uid_len++] = (uint8_t)strtol(hex, NULL, 16);
                }
            }

            item = cJSON_GetObjectItem(tag_data, "vendor");
            if (item && item->valuestring) strncpy(g_tag_vendor, item->valuestring, sizeof(g_tag_vendor) - 1);

            item = cJSON_GetObjectItem(tag_data, "material");
            if (item && item->valuestring) strncpy(g_tag_material, item->valuestring, sizeof(g_tag_material) - 1);

            item = cJSON_GetObjectItem(tag_data, "subtype");
            if (item && item->valuestring) strncpy(g_tag_material_subtype, item->valuestring, sizeof(g_tag_material_subtype) - 1);

            item = cJSON_GetObjectItem(tag_data, "color_name");
            if (item && item->valuestring) strncpy(g_tag_color_name, item->valuestring, sizeof(g_tag_color_name) - 1);

            item = cJSON_GetObjectItem(tag_data, "color_rgba");
            if (item) g_tag_color_rgba = (uint32_t)item->valuedouble;  // Use valuedouble for large unsigned values

            item = cJSON_GetObjectItem(tag_data, "spool_weight");
            if (item) g_tag_spool_weight = item->valueint;

            item = cJSON_GetObjectItem(tag_data, "tag_type");
            if (item && item->valuestring) strncpy(g_tag_type, item->valuestring, sizeof(g_tag_type) - 1);

            item = cJSON_GetObjectItem(tag_data, "slicer_filament");
            if (item && item->valuestring) strncpy(g_tag_slicer_filament, item->valuestring, sizeof(g_tag_slicer_filament) - 1);
        } else {
            // Staging expired or no tag - clear simulator NFC state
            if (g_nfc_tag_present) {
                printf("[backend] Staging expired (remaining=%.1fs) - closing popup\n", remaining);
                g_nfc_tag_present = false;
                g_tag_vendor[0] = '\0';
                g_tag_material[0] = '\0';
                g_tag_material_subtype[0] = '\0';
                g_tag_color_name[0] = '\0';
                g_tag_color_rgba = 0;
                g_tag_spool_weight = 0;
                g_tag_type[0] = '\0';
                g_tag_slicer_filament[0] = '\0';
            }
        }

        cJSON_Delete(json);
    }

    return 0;
}

const BackendState *backend_get_state(void) {
    return &g_state;
}

bool backend_is_connected(void) {
    return g_state.backend_reachable;
}

const BackendPrinterState *backend_get_printer_by_serial(const char *serial) {
    for (int i = 0; i < g_state.printer_count; i++) {
        if (strcmp(g_state.printers[i].serial, serial) == 0) {
            return &g_state.printers[i];
        }
    }
    return NULL;
}

const BackendPrinterState *backend_get_first_printer(void) {
    for (int i = 0; i < g_state.printer_count; i++) {
        if (g_state.printers[i].connected) {
            return &g_state.printers[i];
        }
    }
    // Return first printer even if not connected
    if (g_state.printer_count > 0) {
        return &g_state.printers[0];
    }
    return NULL;
}

// Static buffer for cover image path
static char g_cover_path[256] = "/tmp/spoolbuddy_cover.png";
static char g_cover_serial[32] = "";

// Write callback for file download
static size_t write_file_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    FILE *fp = (FILE *)userp;
    return fwrite(contents, size, nmemb, fp);
}

const char *backend_fetch_cover_image(const char *serial) {
    if (!g_curl || !serial) return NULL;

    // Check if we already have this cover cached
    if (strcmp(g_cover_serial, serial) == 0) {
        // Check if file exists
        FILE *fp = fopen(g_cover_path, "r");
        if (fp) {
            fclose(fp);
            return g_cover_path;
        }
    }

    char url[512];
    snprintf(url, sizeof(url), "%s/api/printers/%s/cover", g_base_url, serial);

    FILE *fp = fopen(g_cover_path, "wb");
    if (!fp) {
        fprintf(stderr, "[backend] Failed to open temp file for cover image\n");
        return NULL;
    }

    curl_easy_setopt(g_curl, CURLOPT_URL, url);
    curl_easy_setopt(g_curl, CURLOPT_WRITEFUNCTION, write_file_callback);
    curl_easy_setopt(g_curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(g_curl, CURLOPT_TIMEOUT, 5L);

    CURLcode res = curl_easy_perform(g_curl);
    fclose(fp);

    // Reset write callback for JSON fetching
    curl_easy_setopt(g_curl, CURLOPT_WRITEFUNCTION, write_callback);

    if (res != CURLE_OK) {
        fprintf(stderr, "[backend] Failed to fetch cover image: %s\n", curl_easy_strerror(res));
        remove(g_cover_path);
        g_cover_serial[0] = '\0';
        return NULL;
    }

    // Check HTTP response code
    long http_code = 0;
    curl_easy_getinfo(g_curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200) {
        fprintf(stderr, "[backend] Cover image HTTP error: %ld\n", http_code);
        remove(g_cover_path);
        g_cover_serial[0] = '\0';
        return NULL;
    }

    strncpy(g_cover_serial, serial, sizeof(g_cover_serial) - 1);
    printf("[backend] Fetched cover image for %s\n", serial);
    return g_cover_path;
}

// =============================================================================
// Firmware-compatible API implementation
// These functions adapt the simulator's data structures to match firmware
// =============================================================================

// Parse hex color string to RGBA uint32_t (RRGGBBAA format)
static uint32_t parse_hex_color_rgba(const char *hex) {
    if (!hex || hex[0] == '\0') return 0;
    if (hex[0] == '#') hex++;
    uint32_t color = 0;
    int len = strlen(hex);
    for (int i = 0; i < len && i < 8; i++) {
        char c = hex[i];
        int digit = 0;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
        color = (color << 4) | digit;
    }
    // If only 6 chars (RGB), add full alpha
    if (len == 6) color = (color << 8) | 0xFF;
    return color;
}

void backend_get_status(BackendStatus *status) {
    if (!status) return;
    memset(status, 0, sizeof(*status));

    if (g_state.backend_reachable) {
        status->state = 2;  // Connected
        status->printer_count = g_state.printer_count;
        // IP/port not used in simulator
    } else {
        status->state = 0;  // Disconnected
    }
}

int backend_get_printer(int index, BackendPrinterInfo *info) {
    if (!info || index < 0 || index >= g_state.printer_count) {
        return -1;
    }

    memset(info, 0, sizeof(*info));
    BackendPrinterState *src = &g_state.printers[index];

    // Copy with size limits matching firmware struct
    strncpy(info->name, src->name, sizeof(info->name) - 1);
    strncpy(info->serial, src->serial, sizeof(info->serial) - 1);
    strncpy(info->gcode_state, src->gcode_state, sizeof(info->gcode_state) - 1);
    strncpy(info->subtask_name, src->subtask_name, sizeof(info->subtask_name) - 1);
    strncpy(info->stg_cur_name, src->stg_cur_name, sizeof(info->stg_cur_name) - 1);

    info->remaining_time_min = src->remaining_time;
    info->print_progress = src->print_progress;
    info->stg_cur = src->stg_cur;
    info->connected = src->connected;

    return 0;
}

int backend_get_ams_count(int printer_index) {
    if (printer_index < 0 || printer_index >= g_state.printer_count) {
        return 0;
    }
    return g_state.printers[printer_index].ams_unit_count;
}

int backend_get_ams_unit(int printer_index, int ams_index, AmsUnitCInfo *info) {
    if (!info || printer_index < 0 || printer_index >= g_state.printer_count) {
        return -1;
    }

    BackendPrinterState *printer = &g_state.printers[printer_index];
    if (ams_index < 0 || ams_index >= printer->ams_unit_count) {
        return -1;
    }

    memset(info, 0, sizeof(*info));
    BackendAmsUnit *src = &printer->ams_units[ams_index];

    info->id = src->id;
    info->humidity = src->humidity;
    info->temperature = src->temperature * 10;  // Firmware uses Celsius * 10
    info->extruder = src->extruder;
    info->tray_count = src->tray_count;

    for (int i = 0; i < src->tray_count && i < 4; i++) {
        strncpy(info->trays[i].tray_type, src->trays[i].tray_type, sizeof(info->trays[i].tray_type) - 1);
        info->trays[i].tray_color = parse_hex_color_rgba(src->trays[i].tray_color);
        info->trays[i].remain = src->trays[i].remain;
    }

    return 0;
}

int backend_get_tray_now(int printer_index) {
    if (printer_index < 0 || printer_index >= g_state.printer_count) {
        return -1;
    }
    return g_state.printers[printer_index].tray_now;
}

int backend_get_tray_now_left(int printer_index) {
    if (printer_index < 0 || printer_index >= g_state.printer_count) {
        return -1;
    }
    return g_state.printers[printer_index].tray_now_left;
}

int backend_get_tray_now_right(int printer_index) {
    if (printer_index < 0 || printer_index >= g_state.printer_count) {
        return -1;
    }
    return g_state.printers[printer_index].tray_now_right;
}

int backend_get_active_extruder(int printer_index) {
    if (printer_index < 0 || printer_index >= g_state.printer_count) {
        return -1;
    }
    return g_state.printers[printer_index].active_extruder;
}

int backend_get_tray_reading_bits(int printer_index) {
    if (printer_index < 0 || printer_index >= g_state.printer_count) {
        return -1;
    }
    return g_state.printers[printer_index].tray_reading_bits;
}

// Cover image handling - simulator uses file-based approach
static uint8_t *g_cover_data = NULL;
static uint32_t g_cover_data_size = 0;

int backend_has_cover(void) {
    // Check if we have a cached cover file
    FILE *fp = fopen(g_cover_path, "r");
    if (fp) {
        fclose(fp);
        return 1;
    }
    return 0;
}

const uint8_t* backend_get_cover_data(uint32_t *size_out) {
    // Simulator uses file-based covers, not raw data
    // Return NULL - ui_backend.c will need to handle this differently
    if (size_out) *size_out = 0;
    return NULL;
}

int time_get_hhmm(void) {
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    if (tm) {
        return (tm->tm_hour << 8) | tm->tm_min;
    }
    return -1;
}

int time_is_synced(void) {
    return 1;  // Simulator always has valid time
}

// =============================================================================
// Spool Inventory API
// =============================================================================

bool spool_exists_by_tag(const char *tag_id) {
    if (!tag_id || !g_curl) return false;

    char url[512];
    snprintf(url, sizeof(url), "%s/api/spools", g_base_url);

    ResponseBuffer response = {0};

    curl_easy_reset(g_curl);
    curl_easy_setopt(g_curl, CURLOPT_URL, url);
    curl_easy_setopt(g_curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(g_curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(g_curl, CURLOPT_TIMEOUT, 5L);

    CURLcode res = curl_easy_perform(g_curl);

    bool found = false;
    if (res == CURLE_OK && response.data) {
        cJSON *json = cJSON_Parse(response.data);
        if (json && cJSON_IsArray(json)) {
            int count = cJSON_GetArraySize(json);
            for (int i = 0; i < count; i++) {
                cJSON *spool = cJSON_GetArrayItem(json, i);
                cJSON *tid = cJSON_GetObjectItem(spool, "tag_id");
                if (tid && tid->valuestring && strcmp(tid->valuestring, tag_id) == 0) {
                    found = true;
                    break;
                }
            }
        }
        cJSON_Delete(json);
    }

    free(response.data);
    return found;
}

bool spool_get_by_tag(const char *tag_id, SpoolInfo *info) {
    if (!tag_id || !info || !g_curl) {
        if (info) info->valid = false;
        return false;
    }

    memset(info, 0, sizeof(SpoolInfo));

    char url[512];
    snprintf(url, sizeof(url), "%s/api/spools", g_base_url);

    ResponseBuffer response = {0};

    curl_easy_reset(g_curl);
    curl_easy_setopt(g_curl, CURLOPT_URL, url);
    curl_easy_setopt(g_curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(g_curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(g_curl, CURLOPT_TIMEOUT, 5L);

    CURLcode res = curl_easy_perform(g_curl);

    bool found = false;
    if (res == CURLE_OK && response.data) {
        cJSON *json = cJSON_Parse(response.data);
        if (json && cJSON_IsArray(json)) {
            int count = cJSON_GetArraySize(json);
            for (int i = 0; i < count; i++) {
                cJSON *spool = cJSON_GetArrayItem(json, i);
                cJSON *tid = cJSON_GetObjectItem(spool, "tag_id");
                if (tid && tid->valuestring && strcmp(tid->valuestring, tag_id) == 0) {
                    // Found - extract fields
                    cJSON *id_field = cJSON_GetObjectItem(spool, "id");
                    if (id_field && id_field->valuestring) {
                        strncpy(info->id, id_field->valuestring, sizeof(info->id) - 1);
                    }
                    strncpy(info->tag_id, tag_id, sizeof(info->tag_id) - 1);

                    cJSON *field;
                    field = cJSON_GetObjectItem(spool, "brand");
                    if (field && field->valuestring) strncpy(info->brand, field->valuestring, sizeof(info->brand) - 1);

                    field = cJSON_GetObjectItem(spool, "material");
                    if (field && field->valuestring) strncpy(info->material, field->valuestring, sizeof(info->material) - 1);

                    field = cJSON_GetObjectItem(spool, "subtype");
                    if (field && field->valuestring) strncpy(info->subtype, field->valuestring, sizeof(info->subtype) - 1);

                    field = cJSON_GetObjectItem(spool, "color_name");
                    if (field && field->valuestring) strncpy(info->color_name, field->valuestring, sizeof(info->color_name) - 1);

                    field = cJSON_GetObjectItem(spool, "rgba");
                    if (field && field->valuestring) {
                        // Handle both RRGGBB (6 chars) and RRGGBBAA (8 chars) formats
                        char rgba_padded[16] = {0};
                        size_t len = strlen(field->valuestring);
                        strncpy(rgba_padded, field->valuestring, sizeof(rgba_padded) - 1);
                        if (len == 6) {
                            // Pad with FF for full alpha
                            strcat(rgba_padded, "FF");
                        }
                        info->color_rgba = (uint32_t)strtoul(rgba_padded, NULL, 16);
                        printf("[backend] spool_get_by_tag: rgba string='%s' (padded='%s') -> color_rgba=0x%08X\n",
                               field->valuestring, rgba_padded, info->color_rgba);
                    }

                    field = cJSON_GetObjectItem(spool, "label_weight");
                    if (field && cJSON_IsNumber(field)) info->label_weight = field->valueint;

                    field = cJSON_GetObjectItem(spool, "weight_current");
                    if (field && cJSON_IsNumber(field)) info->weight_current = field->valueint;

                    field = cJSON_GetObjectItem(spool, "slicer_filament");
                    if (field && field->valuestring) strncpy(info->slicer_filament, field->valuestring, sizeof(info->slicer_filament) - 1);

                    field = cJSON_GetObjectItem(spool, "tag_type");
                    if (field && field->valuestring) strncpy(info->tag_type, field->valuestring, sizeof(info->tag_type) - 1);

                    info->valid = true;
                    found = true;
                    break;
                }
            }
        }
        cJSON_Delete(json);
    }

    free(response.data);
    return found;
}

bool spool_add_to_inventory(const char *tag_id, const char *vendor, const char *material,
                            const char *subtype, const char *color_name, uint32_t color_rgba,
                            int label_weight, int weight_current, const char *data_origin,
                            const char *tag_type, const char *slicer_filament) {
    if (!g_curl) {
        printf("[backend] spool_add_to_inventory: curl not initialized\n");
        return false;
    }

    char url[512];
    snprintf(url, sizeof(url), "%s/api/spools", g_base_url);

    // Build JSON body
    cJSON *json = cJSON_CreateObject();
    if (tag_id) cJSON_AddStringToObject(json, "tag_id", tag_id);
    cJSON_AddStringToObject(json, "material", material ? material : "Unknown");
    if (subtype && subtype[0]) cJSON_AddStringToObject(json, "subtype", subtype);
    if (vendor) cJSON_AddStringToObject(json, "brand", vendor);
    if (color_name) cJSON_AddStringToObject(json, "color_name", color_name);

    // Convert RGBA to hex string (RRGGBBAA format)
    char rgba_hex[16];
    snprintf(rgba_hex, sizeof(rgba_hex), "%08X", color_rgba);
    cJSON_AddStringToObject(json, "rgba", rgba_hex);

    cJSON_AddNumberToObject(json, "label_weight", label_weight);
    cJSON_AddNumberToObject(json, "weight_new", label_weight);  // New spool, same as label
    if (weight_current > 0) {
        cJSON_AddNumberToObject(json, "weight_current", weight_current);
    }
    if (data_origin && data_origin[0]) cJSON_AddStringToObject(json, "data_origin", data_origin);
    if (tag_type && tag_type[0]) cJSON_AddStringToObject(json, "tag_type", tag_type);
    if (slicer_filament && slicer_filament[0]) cJSON_AddStringToObject(json, "slicer_filament", slicer_filament);

    char *body = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    if (!body) {
        printf("[backend] spool_add_to_inventory: failed to create JSON\n");
        return false;
    }

    ResponseBuffer response = {0};

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_reset(g_curl);
    curl_easy_setopt(g_curl, CURLOPT_URL, url);
    curl_easy_setopt(g_curl, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(g_curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(g_curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(g_curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(g_curl, CURLOPT_TIMEOUT, 5L);

    CURLcode res = curl_easy_perform(g_curl);

    long http_code = 0;
    curl_easy_getinfo(g_curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    free(body);

    bool success = (res == CURLE_OK && http_code == 201);

    if (success) {
        printf("[backend] Spool added to inventory: tag=%s\n", tag_id);
    } else {
        printf("[backend] Failed to add spool: HTTP %ld, curl %d\n", http_code, res);
        if (response.data) {
            printf("[backend] Response: %s\n", response.data);
        }
    }

    free(response.data);
    return success;
}

// Get K-profiles for a spool by spool ID
int spool_get_k_profiles(const char *spool_id, SpoolKProfile *profiles, int max_profiles) {
    if (!spool_id || !profiles || max_profiles <= 0 || !g_curl) {
        return 0;
    }

    char url[512];
    snprintf(url, sizeof(url), "%s/api/spools/%s/k-profiles", g_base_url, spool_id);

    ResponseBuffer response = {0};

    curl_easy_reset(g_curl);
    curl_easy_setopt(g_curl, CURLOPT_URL, url);
    curl_easy_setopt(g_curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(g_curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(g_curl, CURLOPT_TIMEOUT, 5L);

    CURLcode res = curl_easy_perform(g_curl);

    int count = 0;
    if (res == CURLE_OK && response.data) {
        cJSON *json = cJSON_Parse(response.data);
        if (json && cJSON_IsArray(json)) {
            int array_size = cJSON_GetArraySize(json);
            for (int i = 0; i < array_size && count < max_profiles; i++) {
                cJSON *item = cJSON_GetArrayItem(json, i);
                if (!item) continue;

                SpoolKProfile *p = &profiles[count];
                memset(p, 0, sizeof(SpoolKProfile));
                p->extruder = -1;  // Default: single-nozzle
                p->cali_idx = -1;

                cJSON *field;
                field = cJSON_GetObjectItem(item, "printer_serial");
                if (field && field->valuestring) {
                    strncpy(p->printer_serial, field->valuestring, sizeof(p->printer_serial) - 1);
                }

                field = cJSON_GetObjectItem(item, "extruder");
                if (field && cJSON_IsNumber(field)) {
                    p->extruder = field->valueint;
                } else if (field && cJSON_IsNull(field)) {
                    p->extruder = -1;
                }

                field = cJSON_GetObjectItem(item, "k_value");
                if (field && field->valuestring) {
                    strncpy(p->k_value, field->valuestring, sizeof(p->k_value) - 1);
                }

                field = cJSON_GetObjectItem(item, "name");
                if (field && field->valuestring) {
                    strncpy(p->name, field->valuestring, sizeof(p->name) - 1);
                }

                field = cJSON_GetObjectItem(item, "cali_idx");
                if (field && cJSON_IsNumber(field)) {
                    p->cali_idx = field->valueint;
                }

                count++;
            }
        }
        cJSON_Delete(json);
    }

    free(response.data);
    printf("[backend] spool_get_k_profiles(%s): found %d profiles\n", spool_id, count);
    return count;
}

// Get K-profile for a spool matching a specific printer
bool spool_get_k_profile_for_printer(const char *spool_id, const char *printer_serial, SpoolKProfile *profile) {
    if (!spool_id || !printer_serial || !profile) {
        printf("[backend] spool_get_k_profile_for_printer: invalid params (spool=%s, serial=%s)\n",
               spool_id ? spool_id : "NULL", printer_serial ? printer_serial : "NULL");
        return false;
    }

    printf("[backend] Looking for K-profile: spool=%s, printer=%s\n", spool_id, printer_serial);

    SpoolKProfile profiles[16];
    int count = spool_get_k_profiles(spool_id, profiles, 16);

    for (int i = 0; i < count; i++) {
        printf("[backend] K-profile %d: printer_serial='%s', cali_idx=%d, k_value=%s\n",
               i, profiles[i].printer_serial, profiles[i].cali_idx, profiles[i].k_value);
        if (strcmp(profiles[i].printer_serial, printer_serial) == 0) {
            memcpy(profile, &profiles[i], sizeof(SpoolKProfile));
            printf("[backend] Found matching K-profile: cali_idx=%d\n", profile->cali_idx);
            return true;
        }
    }

    printf("[backend] No matching K-profile found for printer %s\n", printer_serial);
    return false;
}

// =============================================================================
// AMS Slot Assignment functions
// =============================================================================

AssignResult backend_assign_spool_to_tray(const char *printer_serial, int ams_id, int tray_id,
                                           const char *spool_id) {
    if (!printer_serial || !spool_id || !g_curl) {
        printf("[backend] assign_spool_to_tray: invalid params\n");
        return ASSIGN_RESULT_ERROR;
    }

    // POST /api/printers/{serial}/ams/{ams_id}/tray/{tray_id}/assign
    char url[512];
    snprintf(url, sizeof(url), "%s/api/printers/%s/ams/%d/tray/%d/assign",
             g_base_url, printer_serial, ams_id, tray_id);

    // Build JSON payload: {"spool_id": "..."}
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "spool_id", spool_id);
    char *json_str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    if (!json_str) {
        printf("[backend] assign_spool_to_tray: failed to create JSON\n");
        return ASSIGN_RESULT_ERROR;
    }

    printf("[backend] assign_spool_to_tray: POST %s\n", url);
    printf("[backend] payload: %s\n", json_str);

    ResponseBuffer response = {0};
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_reset(g_curl);
    curl_easy_setopt(g_curl, CURLOPT_URL, url);
    curl_easy_setopt(g_curl, CURLOPT_POST, 1L);
    curl_easy_setopt(g_curl, CURLOPT_POSTFIELDS, json_str);
    curl_easy_setopt(g_curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(g_curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(g_curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(g_curl, CURLOPT_TIMEOUT, 10L);

    CURLcode res = curl_easy_perform(g_curl);

    long http_code = 0;
    curl_easy_getinfo(g_curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    free(json_str);

    AssignResult result = ASSIGN_RESULT_ERROR;

    if (res == CURLE_OK && http_code == 200 && response.data) {
        // Parse JSON response: {"status": "configured"|"staged", "message": "...", "needs_replacement": bool}
        cJSON *resp_json = cJSON_Parse(response.data);
        if (resp_json) {
            cJSON *status = cJSON_GetObjectItem(resp_json, "status");
            cJSON *needs_replacement = cJSON_GetObjectItem(resp_json, "needs_replacement");
            if (status && cJSON_IsString(status)) {
                if (strcmp(status->valuestring, "configured") == 0) {
                    result = ASSIGN_RESULT_CONFIGURED;
                } else if (strcmp(status->valuestring, "staged") == 0) {
                    // Check if needs replacement
                    if (needs_replacement && cJSON_IsTrue(needs_replacement)) {
                        result = ASSIGN_RESULT_STAGED_REPLACE;
                    } else {
                        result = ASSIGN_RESULT_STAGED;
                    }
                }
            }
            cJSON_Delete(resp_json);
        }
    }

    if (response.data) free(response.data);

    printf("[backend] assign_spool_to_tray: result=%d, http=%ld, assign_result=%d\n",
           res, http_code, result);

    return result;
}

bool backend_cancel_staged_assignment(const char *printer_serial, int ams_id, int tray_id) {
    if (!printer_serial || !g_curl) {
        printf("[backend] cancel_staged_assignment: invalid params\n");
        return false;
    }

    // POST /api/printers/{serial}/ams/{ams_id}/tray/{tray_id}/cancel-staged
    char url[512];
    snprintf(url, sizeof(url), "%s/api/printers/%s/ams/%d/tray/%d/cancel-staged",
             g_base_url, printer_serial, ams_id, tray_id);

    printf("[backend] cancel_staged_assignment: POST %s\n", url);

    ResponseBuffer response = {0};
    curl_easy_reset(g_curl);
    curl_easy_setopt(g_curl, CURLOPT_URL, url);
    curl_easy_setopt(g_curl, CURLOPT_POST, 1L);
    curl_easy_setopt(g_curl, CURLOPT_POSTFIELDS, "");
    curl_easy_setopt(g_curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(g_curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(g_curl, CURLOPT_TIMEOUT, 5L);

    CURLcode res = curl_easy_perform(g_curl);

    long http_code = 0;
    curl_easy_getinfo(g_curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (response.data) free(response.data);

    bool success = (res == CURLE_OK && http_code == 204);
    printf("[backend] cancel_staged_assignment: result=%d, http=%ld, success=%d\n",
           res, http_code, success);

    return success;
}

int backend_poll_assignment_completions(double since_timestamp, AssignmentCompletion *events, int max_events) {
    if (!events || max_events <= 0 || !g_curl) {
        return 0;
    }

    // GET /api/printers/assignment-completions?since=<timestamp>
    char url[512];
    snprintf(url, sizeof(url), "%s/api/printers/assignment-completions?since=%.6f",
             g_base_url, since_timestamp);

    // Only log occasionally to avoid spam (every ~10 calls)
    static int poll_count = 0;
    bool should_log = (poll_count % 10 == 0);
    poll_count++;

    if (should_log) {
        printf("[backend] Polling for completions since %.3f\n", since_timestamp);
    }

    ResponseBuffer response = {0};
    curl_easy_reset(g_curl);
    curl_easy_setopt(g_curl, CURLOPT_URL, url);
    curl_easy_setopt(g_curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(g_curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(g_curl, CURLOPT_TIMEOUT, 2L);

    CURLcode res = curl_easy_perform(g_curl);

    long http_code = 0;
    curl_easy_getinfo(g_curl, CURLINFO_RESPONSE_CODE, &http_code);

    int count = 0;
    if (res == CURLE_OK && http_code == 200 && response.data) {
        cJSON *json = cJSON_Parse(response.data);
        if (json && cJSON_IsArray(json)) {
            int array_size = cJSON_GetArraySize(json);
            for (int i = 0; i < array_size && count < max_events; i++) {
                cJSON *item = cJSON_GetArrayItem(json, i);
                if (!item) continue;

                AssignmentCompletion *evt = &events[count];
                memset(evt, 0, sizeof(*evt));

                cJSON *ts = cJSON_GetObjectItem(item, "timestamp");
                cJSON *serial = cJSON_GetObjectItem(item, "serial");
                cJSON *ams_id = cJSON_GetObjectItem(item, "ams_id");
                cJSON *tray_id = cJSON_GetObjectItem(item, "tray_id");
                cJSON *spool_id = cJSON_GetObjectItem(item, "spool_id");
                cJSON *success = cJSON_GetObjectItem(item, "success");

                if (ts && cJSON_IsNumber(ts)) evt->timestamp = ts->valuedouble;
                if (serial && cJSON_IsString(serial))
                    strncpy(evt->serial, serial->valuestring, sizeof(evt->serial) - 1);
                if (ams_id && cJSON_IsNumber(ams_id)) evt->ams_id = ams_id->valueint;
                if (tray_id && cJSON_IsNumber(tray_id)) evt->tray_id = tray_id->valueint;
                if (spool_id && cJSON_IsString(spool_id))
                    strncpy(evt->spool_id, spool_id->valuestring, sizeof(evt->spool_id) - 1);
                if (success && cJSON_IsBool(success)) evt->success = cJSON_IsTrue(success);

                count++;
            }
            cJSON_Delete(json);
        }
    }

    if (count > 0) {
        printf("[backend] Found %d assignment completion(s)!\n", count);
        for (int i = 0; i < count; i++) {
            printf("[backend]   Completion %d: serial=%s, ams=%d, tray=%d, success=%d\n",
                   i, events[i].serial, events[i].ams_id, events[i].tray_id, events[i].success);
        }
    }

    if (response.data) free(response.data);
    return count;
}

bool backend_set_tray_calibration(const char *printer_serial, int ams_id, int tray_id,
                                   int cali_idx, const char *filament_id,
                                   const char *nozzle_diameter) {
    if (!printer_serial || !g_curl) {
        printf("[backend] set_tray_calibration: invalid params\n");
        return false;
    }

    // POST /api/printers/{serial}/ams/{ams_id}/tray/{tray_id}/calibration
    char url[512];
    snprintf(url, sizeof(url), "%s/api/printers/%s/ams/%d/tray/%d/calibration",
             g_base_url, printer_serial, ams_id, tray_id);

    // Build JSON payload
    cJSON *json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "cali_idx", cali_idx);
    cJSON_AddStringToObject(json, "filament_id", filament_id ? filament_id : "");
    cJSON_AddStringToObject(json, "nozzle_diameter", nozzle_diameter ? nozzle_diameter : "0.4");
    char *json_str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    if (!json_str) {
        printf("[backend] set_tray_calibration: failed to create JSON\n");
        return false;
    }

    printf("[backend] set_tray_calibration: POST %s\n", url);
    printf("[backend] payload: %s\n", json_str);

    ResponseBuffer response = {0};
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_reset(g_curl);
    curl_easy_setopt(g_curl, CURLOPT_URL, url);
    curl_easy_setopt(g_curl, CURLOPT_POST, 1L);
    curl_easy_setopt(g_curl, CURLOPT_POSTFIELDS, json_str);
    curl_easy_setopt(g_curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(g_curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(g_curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(g_curl, CURLOPT_TIMEOUT, 10L);

    CURLcode res = curl_easy_perform(g_curl);

    long http_code = 0;
    curl_easy_getinfo(g_curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    free(json_str);
    if (response.data) free(response.data);

    bool success = (res == CURLE_OK && (http_code == 200 || http_code == 204));
    printf("[backend] set_tray_calibration: result=%d, http=%ld, success=%d\n",
           res, http_code, success);

    return success;
}

// =============================================================================
// NFC Hardware Simulation (keyboard toggle in simulator)
// =============================================================================

// Note: NFC state variables are declared at top of file for use by backend_poll()

bool nfc_is_initialized(void) {
    return g_nfc_initialized;
}

bool nfc_tag_present(void) {
    return g_nfc_tag_present;
}

bool staging_is_active(void) {
    return g_staging_active;
}

float staging_get_remaining(void) {
    return g_staging_remaining;
}

void staging_clear(void) {
    // Set local state FIRST to ensure UI updates immediately
    // This prevents race condition with poll thread
    g_staging_active = false;
    g_staging_remaining = 0;
    g_staging_cleared_locally = true;
    g_staging_cleared_time = time(NULL);
    printf("[backend] Staging cleared locally (holdoff active)\n");

    // Now send clear request to backend using a separate curl handle
    // (g_curl is used by poll thread and not thread-safe)
    CURL *curl = curl_easy_init();
    if (!curl) return;

    char url[256];
    snprintf(url, sizeof(url), "%s/api/staging/clear", g_base_url);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        printf("[backend] Staging cleared via API\n");
    } else {
        printf("[backend] Warning: API clear failed (%d), but local state already cleared\n", res);
    }
    curl_easy_cleanup(curl);
}

uint8_t nfc_get_uid_len(void) {
    return g_nfc_tag_present ? g_nfc_uid_len : 0;
}

uint8_t nfc_get_uid(uint8_t *buf, uint8_t buf_len) {
    if (!g_nfc_tag_present || buf == NULL) return 0;
    uint8_t len = g_nfc_uid_len < buf_len ? g_nfc_uid_len : buf_len;
    memcpy(buf, g_nfc_uid, len);
    return len;
}

uint8_t nfc_get_uid_hex(uint8_t *buf, uint8_t buf_len) {
    if (!g_nfc_tag_present || buf == NULL || buf_len < 3) return 0;
    int pos = 0;
    for (int i = 0; i < g_nfc_uid_len && pos < buf_len - 3; i++) {
        if (i > 0) buf[pos++] = ':';
        pos += snprintf((char*)&buf[pos], buf_len - pos, "%02X", g_nfc_uid[i]);
    }
    return pos;
}

// Fetch decoded tag data from backend
static void fetch_tag_data_from_backend(const char *tag_uid_hex) {
    if (!g_curl || !tag_uid_hex) return;

    char url[512];
    snprintf(url, sizeof(url), "%s/api/tags/decode?uid=%s", g_base_url, tag_uid_hex);

    ResponseBuffer response = {0};

    curl_easy_reset(g_curl);
    curl_easy_setopt(g_curl, CURLOPT_URL, url);
    curl_easy_setopt(g_curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(g_curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(g_curl, CURLOPT_TIMEOUT, 2L);

    CURLcode res = curl_easy_perform(g_curl);

    if (res == CURLE_OK && response.data) {
        cJSON *json = cJSON_Parse(response.data);
        if (json) {
            cJSON *item;

            item = cJSON_GetObjectItem(json, "vendor");
            if (item && item->valuestring) strncpy(g_tag_vendor, item->valuestring, sizeof(g_tag_vendor) - 1);

            item = cJSON_GetObjectItem(json, "material");
            if (item && item->valuestring) strncpy(g_tag_material, item->valuestring, sizeof(g_tag_material) - 1);

            item = cJSON_GetObjectItem(json, "subtype");
            if (item && item->valuestring) strncpy(g_tag_material_subtype, item->valuestring, sizeof(g_tag_material_subtype) - 1);

            item = cJSON_GetObjectItem(json, "color_name");
            if (item && item->valuestring) strncpy(g_tag_color_name, item->valuestring, sizeof(g_tag_color_name) - 1);

            item = cJSON_GetObjectItem(json, "color_rgba");
            if (item) g_tag_color_rgba = (uint32_t)item->valuedouble;  // Use valuedouble for large unsigned values

            item = cJSON_GetObjectItem(json, "spool_weight");
            if (item) g_tag_spool_weight = item->valueint;

            item = cJSON_GetObjectItem(json, "tag_type");
            if (item && item->valuestring) strncpy(g_tag_type, item->valuestring, sizeof(g_tag_type) - 1);

            cJSON_Delete(json);
            printf("[backend] Tag data fetched: %s %s %s\n", g_tag_vendor, g_tag_material, g_tag_color_name);
        }
    }

    free(response.data);
}

void sim_set_nfc_tag_present(bool present) {
    bool was_present = g_nfc_tag_present;
    g_nfc_tag_present = present;
    printf("[sim] NFC tag %s\n", present ? "DETECTED" : "REMOVED");

    if (present && !was_present) {
        // Tag just appeared - fetch decoded data from backend
        char uid_hex[32];
        nfc_get_uid_hex((uint8_t*)uid_hex, sizeof(uid_hex));
        fetch_tag_data_from_backend(uid_hex);
    } else if (!present) {
        // Tag removed - clear cached data
        g_tag_vendor[0] = '\0';
        g_tag_material[0] = '\0';
        g_tag_material_subtype[0] = '\0';
        g_tag_color_name[0] = '\0';
        g_tag_color_rgba = 0;
        g_tag_spool_weight = 0;
        g_tag_type[0] = '\0';
        g_tag_slicer_filament[0] = '\0';
    }
}

void sim_set_nfc_uid(uint8_t *uid, uint8_t len) {
    g_nfc_uid_len = len < 7 ? len : 7;
    memcpy(g_nfc_uid, uid, g_nfc_uid_len);
}

bool sim_get_nfc_tag_present(void) {
    return g_nfc_tag_present;
}

// Decoded tag data getters
const char* nfc_get_tag_vendor(void) {
    return g_nfc_tag_present ? g_tag_vendor : "";
}

const char* nfc_get_tag_material(void) {
    return g_nfc_tag_present ? g_tag_material : "";
}

const char* nfc_get_tag_material_subtype(void) {
    return g_nfc_tag_present ? g_tag_material_subtype : "";
}

const char* nfc_get_tag_color_name(void) {
    return g_nfc_tag_present ? g_tag_color_name : "";
}

uint32_t nfc_get_tag_color_rgba(void) {
    return g_nfc_tag_present ? g_tag_color_rgba : 0;
}

int nfc_get_tag_spool_weight(void) {
    return g_nfc_tag_present ? g_tag_spool_weight : 0;
}

const char* nfc_get_tag_type(void) {
    return g_nfc_tag_present ? g_tag_type : "";
}

const char* nfc_get_tag_slicer_filament(void) {
    return g_nfc_tag_present ? g_tag_slicer_filament : "";
}

// =============================================================================
// WiFi Stubs (simulator doesn't have real WiFi)
// =============================================================================

// WiFi types (matching ui_internal.h)
typedef struct {
    int state;       // 0=Uninitialized, 1=Disconnected, 2=Connecting, 3=Connected, 4=Error
    uint8_t ip[4];   // IP address when connected
    int8_t rssi;     // Signal strength in dBm (when connected)
} WifiStatus;

typedef struct {
    char ssid[33];   // SSID (null-terminated)
    int8_t rssi;     // Signal strength in dBm
    uint8_t auth_mode; // 0=Open, 1=WEP, 2=WPA, 3=WPA2, 4=WPA3
} WifiScanResult;

static int g_wifi_state = 3;  // Connected
static char g_wifi_ssid[33] = "SimulatorWiFi";

void wifi_get_status(WifiStatus *status) {
    if (status) {
        status->state = g_wifi_state;
        status->ip[0] = 192; status->ip[1] = 168; status->ip[2] = 1; status->ip[3] = 100;
        status->rssi = -45;
    }
}

int wifi_get_ssid(char *buf, int buf_len) {
    strncpy(buf, g_wifi_ssid, buf_len - 1);
    buf[buf_len - 1] = '\0';
    return strlen(buf);
}

int wifi_connect(const char *ssid, const char *password) {
    (void)password;
    printf("[sim] WiFi connect: %s\n", ssid);
    strncpy(g_wifi_ssid, ssid, sizeof(g_wifi_ssid) - 1);
    g_wifi_state = 3;
    return 0;
}

int wifi_disconnect(void) {
    printf("[sim] WiFi disconnect\n");
    g_wifi_state = 1;
    return 0;
}

int wifi_scan(WifiScanResult *results, int max_results) {
    if (max_results < 1) return 0;
    strncpy(results[0].ssid, "SimNetwork1", 32);
    results[0].rssi = -45;
    results[0].auth_mode = 3;

    if (max_results < 2) return 1;
    strncpy(results[1].ssid, "SimNetwork2", 32);
    results[1].rssi = -60;
    results[1].auth_mode = 0;

    return 2;
}

// =============================================================================
// OTA Stubs (simulator doesn't do real OTA)
// =============================================================================

int ota_is_update_available(void) { return 0; }
int ota_get_current_version(char *buf, int buf_len) {
    const char *ver = "0.1.1-sim";
    strncpy(buf, ver, buf_len - 1);
    buf[buf_len - 1] = '\0';
    return strlen(ver);
}
int ota_get_update_version(char *buf, int buf_len) {
    buf[0] = '\0';
    return 0;
}
int ota_get_state(void) { return 0; }
int ota_get_progress(void) { return 0; }
int ota_check_for_update(void) { return 0; }
int ota_start_update(void) { return -1; }

