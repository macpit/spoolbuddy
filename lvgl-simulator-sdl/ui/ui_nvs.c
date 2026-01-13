// =============================================================================
// ui_nvs.c - NVS Persistence Functions
// =============================================================================
// Handles saving and loading printer configuration to/from ESP32 NVS flash.
// On simulator, data is kept in memory only (not persisted).
// =============================================================================

#include "ui_internal.h"
#include <stdio.h>

#ifdef ESP_PLATFORM
// =============================================================================
// ESP32 Implementation - Real NVS Storage
// =============================================================================

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

static const char *TAG = "ui_nvs";

// NVS namespace and keys
#define PRINTERS_NVS_NAMESPACE "printers"
#define PRINTERS_NVS_KEY_COUNT "count"
#define PRINTERS_NVS_KEY_DATA "data"

void save_printers_to_nvs(void) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(PRINTERS_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS for printers: %s", esp_err_to_name(err));
        return;
    }

    // Save count
    err = nvs_set_i32(handle, PRINTERS_NVS_KEY_COUNT, saved_printer_count);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save printer count: %s", esp_err_to_name(err));
        nvs_close(handle);
        return;
    }

    // Save printer data as blob
    if (saved_printer_count > 0) {
        err = nvs_set_blob(handle, PRINTERS_NVS_KEY_DATA, saved_printers,
                          sizeof(SavedPrinter) * saved_printer_count);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to save printer data: %s", esp_err_to_name(err));
        }
    }

    err = nvs_commit(handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Saved %d printers to NVS", saved_printer_count);
    }

    nvs_close(handle);
}

void load_printers_from_nvs(void) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(PRINTERS_NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "No saved printers found in NVS");
        return;
    }

    // Load count
    int32_t count = 0;
    err = nvs_get_i32(handle, PRINTERS_NVS_KEY_COUNT, &count);
    if (err != ESP_OK || count <= 0) {
        ESP_LOGI(TAG, "No printers saved");
        nvs_close(handle);
        return;
    }

    if (count > MAX_PRINTERS) count = MAX_PRINTERS;

    // Load printer data
    size_t required_size = sizeof(SavedPrinter) * count;
    err = nvs_get_blob(handle, PRINTERS_NVS_KEY_DATA, saved_printers, &required_size);
    if (err == ESP_OK) {
        saved_printer_count = count;
        ESP_LOGI(TAG, "Loaded %d printers from NVS", saved_printer_count);
    } else {
        ESP_LOGE(TAG, "Failed to load printer data: %s", esp_err_to_name(err));
    }

    nvs_close(handle);
}

#else
// =============================================================================
// Simulator Implementation - In-Memory Only (no persistence)
// =============================================================================

void save_printers_to_nvs(void) {
    // Simulator: data kept in memory only
    printf("[ui_nvs] Simulator: saved_printer_count=%d (in-memory only)\n", saved_printer_count);
}

void load_printers_from_nvs(void) {
    // Simulator: nothing to load, printers come from backend
    printf("[ui_nvs] Simulator: no persistent storage, printers sync from backend\n");
}

#endif
