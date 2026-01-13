/**
 * @file ui_update.c
 * @brief Firmware Update UI handling
 *
 * Manages the Settings -> System -> Firmware Updates page.
 * Shows current version, available updates, and allows triggering OTA.
 */

#include "screens.h"
#include <lvgl.h>
#include <stdio.h>
#include <string.h>

#include "ui_internal.h"

#ifdef ESP_PLATFORM
#include "esp_log.h"
#define UPDATE_LOGI(fmt, ...) ESP_LOGI("ui_update", fmt, ##__VA_ARGS__)
#else
// Simulator: OTA functions provided by sim_mocks.c
#define UPDATE_LOGI(fmt, ...) printf("[ui_update] " fmt "\n", ##__VA_ARGS__)
extern int ota_is_update_available(void);
extern int ota_get_current_version(char *buf, int len);
extern int ota_get_update_version(char *buf, int len);
extern int ota_get_state(void);
extern int ota_get_progress(void);
extern int ota_check_for_update(void);
extern int ota_start_update(void);
#endif

// Dynamic UI elements
static lv_obj_t *update_btn = NULL;
static lv_obj_t *progress_bar = NULL;
static lv_obj_t *progress_label = NULL;

// Track if we're on the update screen
static bool on_update_screen = false;

/**
 * @brief Check button click handler
 */
static void on_check_btn_clicked(lv_event_t *e) {
    (void)e;
    UPDATE_LOGI( "Check for updates clicked");

    // Update status to show checking
    if (objects.settings_update_screen_top_bar_content_panel_label_status_value) {
        lv_label_set_text(objects.settings_update_screen_top_bar_content_panel_label_status_value, "Checking...");
    }

    // Trigger check
    ota_check_for_update();
}

/**
 * @brief Update button click handler
 */
static void on_update_btn_clicked(lv_event_t *e) {
    (void)e;
    UPDATE_LOGI( "Update Now clicked");

    // Update status
    if (objects.settings_update_screen_top_bar_content_panel_label_status_value) {
        lv_label_set_text(objects.settings_update_screen_top_bar_content_panel_label_status_value, "Starting update...");
    }

    // Start OTA
    ota_start_update();
}

/**
 * @brief Wire up buttons on the firmware update page
 */
void wire_update_buttons(void) {
    // Wire check button
    if (objects.settings_update_screen_top_bar_content_panel_button_check) {
        lv_obj_add_event_cb(objects.settings_update_screen_top_bar_content_panel_button_check, on_check_btn_clicked, LV_EVENT_CLICKED, NULL);
    }
}

/**
 * @brief Create dynamic UI elements for update page
 */
static void create_update_ui_elements(void) {
    if (!objects.settings_update_screen_top_bar_content_panel || update_btn) {
        return; // Already created or no parent
    }

    // Use the EEZ panel as parent
    lv_obj_t *parent = objects.settings_update_screen_top_bar_content_panel;

    // Create "Update Now" button (initially hidden)
    update_btn = lv_button_create(parent);
    lv_obj_set_pos(update_btn, 16, 200);
    lv_obj_set_size(update_btn, 152, 50);
    lv_obj_set_style_bg_color(update_btn, lv_color_hex(0x00BFFF), 0);
    lv_obj_add_flag(update_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(update_btn, on_update_btn_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_label = lv_label_create(update_btn);
    lv_label_set_text(btn_label, "Update Now");
    lv_obj_center(btn_label);

    // Create progress bar (initially hidden)
    progress_bar = lv_bar_create(parent);
    lv_obj_set_pos(progress_bar, 16, 260);
    lv_obj_set_size(progress_bar, 350, 20);
    lv_bar_set_range(progress_bar, 0, 100);
    lv_bar_set_value(progress_bar, 0, LV_ANIM_OFF);
    lv_obj_add_flag(progress_bar, LV_OBJ_FLAG_HIDDEN);

    // Progress label
    progress_label = lv_label_create(parent);
    lv_obj_set_pos(progress_label, 16, 285);
    lv_obj_set_style_text_font(progress_label, &lv_font_montserrat_12, 0);
    lv_label_set_text(progress_label, "");
    lv_obj_add_flag(progress_label, LV_OBJ_FLAG_HIDDEN);

    UPDATE_LOGI( "Created update UI elements");
}

/**
 * @brief Update the firmware update UI
 *
 * Called periodically to refresh the update page with current state.
 */
void update_firmware_ui(void) {
    // Check if we're on the update screen
    int screen_id = currentScreen + 1;
    bool now_on_update = (screen_id == SCREEN_ID_SETTINGS_UPDATE_SCREEN);

    // Create UI elements when entering screen
    if (now_on_update && !on_update_screen) {
        create_update_ui_elements();
    }
    on_update_screen = now_on_update;

    if (!now_on_update) {
        return;
    }

    char buf[32];

    // Update current version
    // TODO: Add "update_current_version" label in EEZ and update this reference
    // if (objects.update_current_version) {
    //     ota_get_current_version(buf, sizeof(buf));
    //     char version_str[40];
    //     snprintf(version_str, sizeof(version_str), "v%s", buf);
    //     lv_label_set_text(objects.update_current_version, version_str);
    // }

    // Get OTA state
    int state = ota_get_state();
    int progress = ota_get_progress();
    int update_available = ota_is_update_available();

    // Update current version
    if (objects.settings_update_screen_top_bar_content_panel_label_version_value) {
        ota_get_current_version(buf, sizeof(buf));
        char version_str[40];
        snprintf(version_str, sizeof(version_str), "v%s", buf);
        lv_label_set_text(objects.settings_update_screen_top_bar_content_panel_label_version_value, version_str);
    }

    // Update latest version
    if (objects.settings_update_screen_top_bar_content_panel_label_latest_value) {
        if (update_available) {
            ota_get_update_version(buf, sizeof(buf));
            char version_str[64];
            snprintf(version_str, sizeof(version_str), "v%s", buf);
            lv_label_set_text(objects.settings_update_screen_top_bar_content_panel_label_latest_value, version_str);
            lv_obj_set_style_text_color(objects.settings_update_screen_top_bar_content_panel_label_latest_value, lv_color_hex(0x00FF00), 0);
        } else if (state == 1) {  // Checking
            lv_label_set_text(objects.settings_update_screen_top_bar_content_panel_label_latest_value, "Checking...");
            lv_obj_set_style_text_color(objects.settings_update_screen_top_bar_content_panel_label_latest_value, lv_color_hex(0xfafafa), 0);
        } else {
            lv_label_set_text(objects.settings_update_screen_top_bar_content_panel_label_latest_value, "Up to date");
            lv_obj_set_style_text_color(objects.settings_update_screen_top_bar_content_panel_label_latest_value, lv_color_hex(0x888888), 0);
        }
    }

    // Update status
    if (objects.settings_update_screen_top_bar_content_panel_label_status_value) {
        const char *status_text = "Ready";
        uint32_t status_color = 0xfafafa;

        switch (state) {
            case 0: // Idle
                if (update_available) {
                    status_text = "Update ready to install";
                    status_color = 0x00BFFF;
                } else {
                    status_text = "No updates available";
                }
                break;
            case 1: // Checking
                status_text = "Checking for updates...";
                status_color = 0xFFAA00;
                break;
            case 2: // Downloading
                snprintf(buf, sizeof(buf), "Downloading... %d%%", progress >= 0 ? progress : 0);
                status_text = buf;
                status_color = 0x00BFFF;
                break;
            case 3: // Validating
                status_text = "Validating firmware...";
                status_color = 0x00BFFF;
                break;
            case 4: // Flashing
                snprintf(buf, sizeof(buf), "Installing... %d%%", progress >= 0 ? progress : 0);
                status_text = buf;
                status_color = 0x00BFFF;
                break;
            case 5: // Complete
                status_text = "Update complete! Rebooting...";
                status_color = 0x00FF00;
                break;
            case 6: // Error
                status_text = "Update failed";
                status_color = 0xFF4444;
                break;
        }

        lv_label_set_text(objects.settings_update_screen_top_bar_content_panel_label_status_value, status_text);
        lv_obj_set_style_text_color(objects.settings_update_screen_top_bar_content_panel_label_status_value, lv_color_hex(status_color), 0);
    }

    // Show/hide Update button
    if (update_btn) {
        if (update_available && state == 0) {
            lv_obj_clear_flag(update_btn, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(update_btn, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // Show/hide progress bar
    if (progress_bar && progress_label) {
        if (state == 2 || state == 4) {  // Downloading or Flashing
            lv_obj_clear_flag(progress_bar, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(progress_label, LV_OBJ_FLAG_HIDDEN);
            lv_bar_set_value(progress_bar, progress >= 0 ? progress : 0, LV_ANIM_ON);

            const char *action = (state == 2) ? "Downloading" : "Installing";
            snprintf(buf, sizeof(buf), "%s firmware...", action);
            lv_label_set_text(progress_label, buf);
        } else {
            lv_obj_add_flag(progress_bar, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(progress_label, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // Disable check button during update
    if (objects.settings_update_screen_top_bar_content_panel_button_check) {
        if (state > 0 && state < 5) {
            lv_obj_add_state(objects.settings_update_screen_top_bar_content_panel_button_check, LV_STATE_DISABLED);
        } else {
            lv_obj_clear_state(objects.settings_update_screen_top_bar_content_panel_button_check, LV_STATE_DISABLED);
        }
    }
}
