// =============================================================================
// ui_wifi.c - WiFi Settings Handlers
// =============================================================================
// Handles WiFi configuration, scanning, and connection UI.
// =============================================================================

#include "ui_internal.h"
#include "screens.h"
#include <stdio.h>
#include <string.h>

#ifdef ESP_PLATFORM
#include "esp_log.h"
static const char *TAG = "ui_wifi";
#endif

// =============================================================================
// Module State
// =============================================================================

static lv_obj_t *wifi_keyboard = NULL;
static lv_obj_t *wifi_focused_ta = NULL;
static lv_obj_t *wifi_scan_list = NULL;

// Static storage for scan results (must persist for button callbacks)
static WifiScanResult wifi_scan_results_storage[16];

// =============================================================================
// Internal Helpers
// =============================================================================

static void wifi_hide_keyboard(void) {
    if (wifi_keyboard) {
        lv_obj_add_flag(wifi_keyboard, LV_OBJ_FLAG_HIDDEN);
    }
    if (objects.settings_wifi_screen) {
        lv_obj_scroll_to_y(objects.settings_wifi_screen, 0, LV_ANIM_ON);
    }
    wifi_focused_ta = NULL;
}

static void wifi_keyboard_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        wifi_hide_keyboard();
    }
}

static void ensure_wifi_keyboard(void) {
    if (wifi_keyboard) return;
    if (!objects.settings_wifi_screen) return;

    wifi_keyboard = lv_keyboard_create(objects.settings_wifi_screen);
    if (!wifi_keyboard) return;

    lv_obj_set_size(wifi_keyboard, 800, 220);
    lv_obj_align(wifi_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(wifi_keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(wifi_keyboard, wifi_keyboard_event_cb, LV_EVENT_ALL, NULL);
}

// =============================================================================
// Button State Updates
// =============================================================================

void update_wifi_connect_btn_state(void) {
    if (!objects.settings_wifi_screen_content_panel_button_connect_) return;

    WifiStatus status;
    wifi_get_status(&status);

    lv_obj_t *label = lv_obj_get_child(objects.settings_wifi_screen_content_panel_button_connect_, 0);

    // Configure label to not wrap text
    if (label && lv_obj_check_type(label, &lv_label_class)) {
        lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
        lv_obj_set_width(label, LV_SIZE_CONTENT);
        lv_obj_center(label);
    }

    // If connected, always show "Disconnect" and enable
    if (status.state == 3) {
        if (label && lv_obj_check_type(label, &lv_label_class)) {
            lv_label_set_text(label, "Disconnect");
            lv_obj_set_style_text_color(label, lv_color_hex(0xffffffff), LV_PART_MAIN);
        }
        lv_obj_set_style_bg_color(objects.settings_wifi_screen_content_panel_button_connect_, lv_color_hex(0xffff5555), LV_PART_MAIN);
        lv_obj_remove_state(objects.settings_wifi_screen_content_panel_button_connect_, LV_STATE_DISABLED);
        return;
    }

    // If connecting, show "Connecting..." and disable
    if (status.state == 2) {
        if (label && lv_obj_check_type(label, &lv_label_class)) {
            lv_label_set_text(label, "Connecting...");
            lv_obj_set_style_text_color(label, lv_color_hex(0xff000000), LV_PART_MAIN);
        }
        lv_obj_set_style_bg_color(objects.settings_wifi_screen_content_panel_button_connect_, lv_color_hex(0xffffaa00), LV_PART_MAIN);
        lv_obj_add_state(objects.settings_wifi_screen_content_panel_button_connect_, LV_STATE_DISABLED);
        return;
    }

    // Disconnected - check if user has entered an SSID
    const char *ssid = "";
    if (objects.settings_wifi_screen_content_panel_input_ssid) {
        ssid = lv_textarea_get_text(objects.settings_wifi_screen_content_panel_input_ssid);
    }

    bool has_ssid = ssid && strlen(ssid) > 0;

    if (label && lv_obj_check_type(label, &lv_label_class)) {
        lv_label_set_text(label, "Connect");
    }

    if (has_ssid) {
        // SSID entered - enable Connect button
        lv_obj_set_style_bg_color(objects.settings_wifi_screen_content_panel_button_connect_, lv_color_hex(0xff00ff00), LV_PART_MAIN);
        if (label) lv_obj_set_style_text_color(label, lv_color_hex(0xff000000), LV_PART_MAIN);
        lv_obj_remove_state(objects.settings_wifi_screen_content_panel_button_connect_, LV_STATE_DISABLED);
    } else {
        // No SSID - disable Connect button
        lv_obj_set_style_bg_color(objects.settings_wifi_screen_content_panel_button_connect_, lv_color_hex(0xff404040), LV_PART_MAIN);
        if (label) lv_obj_set_style_text_color(label, lv_color_hex(0xff888888), LV_PART_MAIN);
        lv_obj_add_state(objects.settings_wifi_screen_content_panel_button_connect_, LV_STATE_DISABLED);
    }
}

// =============================================================================
// Event Handlers
// =============================================================================

static void wifi_textarea_value_changed_handler(lv_event_t *e) {
    update_wifi_connect_btn_state();
}

static void wifi_textarea_click_handler(lv_event_t *e) {
    lv_obj_t *ta = lv_event_get_target(e);
    if (!ta) return;

    ensure_wifi_keyboard();

    if (wifi_keyboard) {
        wifi_focused_ta = ta;
        lv_keyboard_set_textarea(wifi_keyboard, ta);
        lv_obj_remove_flag(wifi_keyboard, LV_OBJ_FLAG_HIDDEN);

        if (objects.settings_wifi_screen) {
            int32_t ta_y = lv_obj_get_y(ta);
            lv_obj_scroll_to_y(objects.settings_wifi_screen, ta_y - 20, LV_ANIM_ON);
        }
    }
}

static void wifi_connect_click_handler(lv_event_t *e) {
    // Hide keyboard first
    wifi_hide_keyboard();

    // Check if already connected - disconnect instead
    WifiStatus status;
    wifi_get_status(&status);
    if (status.state == 3) {
        // Already connected, disconnect
        wifi_disconnect();
        if (objects.settings_wifi_screen_content_panel_label_status) {
            lv_label_set_text(objects.settings_wifi_screen_content_panel_label_status, "Status: Disconnected");
        }
        update_wifi_ui_state();
        return;
    }

    // Get SSID and password from text inputs
    const char *ssid = "";
    const char *password = "";

    if (objects.settings_wifi_screen_content_panel_input_ssid) {
        ssid = lv_textarea_get_text(objects.settings_wifi_screen_content_panel_input_ssid);
    }
    if (objects.settings_wifi_screen_content_panel_input_password) {
        password = lv_textarea_get_text(objects.settings_wifi_screen_content_panel_input_password);
    }

    // Validate SSID
    if (ssid == NULL || strlen(ssid) == 0) {
        if (objects.settings_wifi_screen_content_panel_label_status) {
            lv_label_set_text(objects.settings_wifi_screen_content_panel_label_status, "Status: Enter SSID");
        }
        return;
    }

    // Update status to show connecting
    if (objects.settings_wifi_screen_content_panel_label_status) {
        lv_label_set_text(objects.settings_wifi_screen_content_panel_label_status, "Status: Connecting...");
        lv_obj_invalidate(objects.settings_wifi_screen_content_panel_label_status);
        lv_refr_now(NULL);
    }

    // Call Rust WiFi connect function
    wifi_connect(ssid, password ? password : "");

    // Update UI state after connection attempt
    update_wifi_ui_state();
}

static void wifi_scan_list_btn_handler(lv_event_t *e) {
    const char *ssid = (const char *)lv_event_get_user_data(e);
    if (ssid && objects.settings_wifi_screen_content_panel_input_ssid) {
        lv_textarea_set_text(objects.settings_wifi_screen_content_panel_input_ssid, ssid);
    }
    // Close the scan list
    if (wifi_scan_list) {
        lv_obj_delete(wifi_scan_list);
        wifi_scan_list = NULL;
    }
}

static void wifi_scan_click_handler(lv_event_t *e) {
    wifi_hide_keyboard();

    // Close existing scan list if open
    if (wifi_scan_list) {
        lv_obj_delete(wifi_scan_list);
        wifi_scan_list = NULL;
    }

    // Create popup on the SCREEN
    lv_obj_t *screen = lv_screen_active();
    if (!screen) return;

    // Show scanning progress modal with spinner
    wifi_scan_list = lv_obj_create(screen);
    lv_obj_set_size(wifi_scan_list, 420, 150);
    lv_obj_center(wifi_scan_list);
    lv_obj_move_foreground(wifi_scan_list);
    lv_obj_set_style_bg_color(wifi_scan_list, lv_color_hex(0xff1a1a1a), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(wifi_scan_list, 255, LV_PART_MAIN);
    lv_obj_set_style_border_color(wifi_scan_list, lv_color_hex(0xff00ff00), LV_PART_MAIN);
    lv_obj_set_style_border_width(wifi_scan_list, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(wifi_scan_list, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_all(wifi_scan_list, 20, LV_PART_MAIN);
    lv_obj_set_flex_flow(wifi_scan_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(wifi_scan_list, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(wifi_scan_list, 15, LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(wifi_scan_list);
    lv_label_set_text(title, "Scanning Networks...");
    lv_obj_set_style_text_color(title, lv_color_hex(0xff00ff00), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, LV_PART_MAIN);

    lv_obj_t *spinner = lv_spinner_create(wifi_scan_list);
    lv_obj_set_size(spinner, 40, 40);
    lv_spinner_set_anim_params(spinner, 1000, 200);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(0xff00ff00), LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(0xff333333), LV_PART_MAIN);

    // Force display update before blocking scan call
    lv_refr_now(NULL);

    // Perform WiFi scan
    memset(wifi_scan_results_storage, 0, sizeof(wifi_scan_results_storage));
    int count = wifi_scan(wifi_scan_results_storage, 16);
    if (count < 0) count = 0;

    // Delete scanning modal
    lv_obj_delete(wifi_scan_list);
    wifi_scan_list = NULL;

    // Update status label
    if (objects.settings_wifi_screen_content_panel_label_status) {
        char buf[64];
        if (count == 0) {
            lv_label_set_text(objects.settings_wifi_screen_content_panel_label_status, "Status: No networks found");
        } else {
            snprintf(buf, sizeof(buf), "Found %d networks", count);
            lv_label_set_text(objects.settings_wifi_screen_content_panel_label_status, buf);
        }
    }

    // Create results popup
    wifi_scan_list = lv_obj_create(screen);
    int popup_height = (count == 0) ? 180 : 320;
    lv_obj_set_size(wifi_scan_list, 420, popup_height);
    lv_obj_center(wifi_scan_list);
    lv_obj_move_foreground(wifi_scan_list);
    lv_obj_set_style_bg_color(wifi_scan_list, lv_color_hex(0xff1a1a1a), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(wifi_scan_list, 255, LV_PART_MAIN);
    lv_obj_set_style_border_color(wifi_scan_list, count == 0 ? lv_color_hex(0xffffaa00) : lv_color_hex(0xff00ff00), LV_PART_MAIN);
    lv_obj_set_style_border_width(wifi_scan_list, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(wifi_scan_list, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_all(wifi_scan_list, 15, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(wifi_scan_list, lv_color_hex(0xff000000), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(wifi_scan_list, 200, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(wifi_scan_list, 30, LV_PART_MAIN);
    lv_obj_set_style_shadow_offset_y(wifi_scan_list, 10, LV_PART_MAIN);
    lv_obj_set_flex_flow(wifi_scan_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(wifi_scan_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(wifi_scan_list, 8, LV_PART_MAIN);
    lv_obj_clear_flag(wifi_scan_list, LV_OBJ_FLAG_SCROLL_ELASTIC);

    // Title
    title = lv_label_create(wifi_scan_list);
    if (count == 0) {
        lv_label_set_text(title, "No Networks Found");
        lv_obj_set_style_text_color(title, lv_color_hex(0xffffaa00), LV_PART_MAIN);
    } else {
        char title_buf[32];
        snprintf(title_buf, sizeof(title_buf), "Found %d Network%s", count, count == 1 ? "" : "s");
        lv_label_set_text(title, title_buf);
        lv_obj_set_style_text_color(title, lv_color_hex(0xff00ff00), LV_PART_MAIN);
    }
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, LV_PART_MAIN);

    // Show message if no networks found
    if (count == 0) {
        lv_obj_t *msg = lv_label_create(wifi_scan_list);
        lv_label_set_text(msg, "Make sure WiFi is enabled\non your router and try again.");
        lv_obj_set_style_text_color(msg, lv_color_hex(0xffaaaaaa), LV_PART_MAIN);
        lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    }

    // Network buttons
    for (int i = 0; i < count && i < 8; i++) {
        lv_obj_t *btn = lv_button_create(wifi_scan_list);
        lv_obj_set_size(btn, 380, 36);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0xff2d2d2d), LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0xff3d3d3d), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_radius(btn, 6, LV_PART_MAIN);

        // Use the static storage SSID pointer for callback
        lv_obj_add_event_cb(btn, wifi_scan_list_btn_handler, LV_EVENT_CLICKED,
                            (void*)wifi_scan_results_storage[i].ssid);

        // SSID label
        lv_obj_t *ssid_label = lv_label_create(btn);
        lv_label_set_text(ssid_label, wifi_scan_results_storage[i].ssid);
        lv_obj_set_style_text_color(ssid_label, lv_color_hex(0xffffffff), LV_PART_MAIN);
        lv_obj_align(ssid_label, LV_ALIGN_LEFT_MID, 5, 0);

        // Signal strength indicator with bars
        lv_obj_t *rssi_label = lv_label_create(btn);
        char rssi_buf[24];
        int8_t rssi = wifi_scan_results_storage[i].rssi;
        const char *bars = rssi > -50 ? "||||" : rssi > -65 ? "|||" : rssi > -75 ? "||" : "|";
        snprintf(rssi_buf, sizeof(rssi_buf), "%s %ddBm", bars, rssi);
        lv_label_set_text(rssi_label, rssi_buf);
        lv_obj_set_style_text_color(rssi_label, rssi > -50 ? lv_color_hex(0xff00ff00) :
                                                 rssi > -65 ? lv_color_hex(0xff88ff00) :
                                                 rssi > -75 ? lv_color_hex(0xffffaa00) :
                                                              lv_color_hex(0xffff5555), LV_PART_MAIN);
        lv_obj_align(rssi_label, LV_ALIGN_RIGHT_MID, -5, 0);
    }

    // Close button at the bottom
    lv_obj_t *close_btn = lv_button_create(wifi_scan_list);
    lv_obj_set_size(close_btn, 120, 36);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xff444444), LV_PART_MAIN);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xff555555), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(close_btn, 6, LV_PART_MAIN);
    lv_obj_add_event_cb(close_btn, wifi_scan_list_btn_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_t *close_label = lv_label_create(close_btn);
    lv_label_set_text(close_label, "Close");
    lv_obj_set_style_text_color(close_label, lv_color_hex(0xffffffff), LV_PART_MAIN);
    lv_obj_center(close_label);
}

// =============================================================================
// WiFi UI State Update
// =============================================================================

void update_wifi_ui_state(void) {
    WifiStatus status;
    wifi_get_status(&status);

    // Update WiFi settings screen elements (only if WiFi screen is active)
    if (!objects.settings_wifi_screen) {
        // Skip WiFi screen updates if not on WiFi screen
        goto update_settings_tab;
    }

    // Update status label
    if (objects.settings_wifi_screen_content_panel_label_status) {
        char buf[64];
        switch (status.state) {
            case 0: // Uninitialized
                lv_label_set_text(objects.settings_wifi_screen_content_panel_label_status, "Status: WiFi not ready");
                break;
            case 1: // Disconnected
                lv_label_set_text(objects.settings_wifi_screen_content_panel_label_status, "Status: Disconnected");
                break;
            case 2: // Connecting
                lv_label_set_text(objects.settings_wifi_screen_content_panel_label_status, "Status: Connecting...");
                break;
            case 3: // Connected
                snprintf(buf, sizeof(buf), "Connected: %d.%d.%d.%d",
                         status.ip[0], status.ip[1], status.ip[2], status.ip[3]);
                lv_label_set_text(objects.settings_wifi_screen_content_panel_label_status, buf);
                break;
            case 4: // Error
                lv_label_set_text(objects.settings_wifi_screen_content_panel_label_status, "Status: Connection failed");
                break;
            default:
                lv_label_set_text(objects.settings_wifi_screen_content_panel_label_status, "Status: Unknown");
                break;
        }
    }

    // Update Connect button via dedicated function
    update_wifi_connect_btn_state();

    // Update SSID input with connected SSID if connected
    if (status.state == 3 && objects.settings_wifi_screen_content_panel_input_ssid) {
        char ssid_buf[64];
        if (wifi_get_ssid(ssid_buf, sizeof(ssid_buf)) > 0) {
            const char *current = lv_textarea_get_text(objects.settings_wifi_screen_content_panel_input_ssid);
            if (current == NULL || strlen(current) == 0) {
                lv_textarea_set_text(objects.settings_wifi_screen_content_panel_input_ssid, ssid_buf);
            }
        }
    }

    // Update Scan button state - enabled when not connected/connecting
    if (objects.settings_wifi_screen_content_panel_button_scan_) {
        lv_obj_t *label = lv_obj_get_child(objects.settings_wifi_screen_content_panel_button_scan_, 0);
        // Enable scan when: Uninitialized (0), Disconnected (1), or Error (4)
        if (status.state == 0 || status.state == 1 || status.state == 4) {
            lv_obj_remove_state(objects.settings_wifi_screen_content_panel_button_scan_, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(objects.settings_wifi_screen_content_panel_button_scan_, lv_color_hex(0xff00ff00), LV_PART_MAIN);
            if (label) lv_obj_set_style_text_color(label, lv_color_hex(0xff000000), LV_PART_MAIN);
        } else {
            lv_obj_add_state(objects.settings_wifi_screen_content_panel_button_scan_, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(objects.settings_wifi_screen_content_panel_button_scan_, lv_color_hex(0xff252525), LV_PART_MAIN);
            if (label) lv_obj_set_style_text_color(label, lv_color_hex(0xff666666), LV_PART_MAIN);
        }
    }

update_settings_tab:
    // Update network tab elements on the main settings screen
    // These objects only exist when settings screen is created

    // Update SSID label
    if (objects.settings_screen && objects.settings_screen_tabs_network_content_wifi_label_ssid) {
        char ssid_buf[64];
        if (status.state == 3 && wifi_get_ssid(ssid_buf, sizeof(ssid_buf)) > 0) {
            lv_label_set_text(objects.settings_screen_tabs_network_content_wifi_label_ssid, ssid_buf);
        } else if (status.state == 2) {
            lv_label_set_text(objects.settings_screen_tabs_network_content_wifi_label_ssid, "Connecting...");
        } else {
            lv_label_set_text(objects.settings_screen_tabs_network_content_wifi_label_ssid, "Not connected");
        }
    }

    // Update WiFi icon
    if (objects.settings_screen && objects.settings_screen_tabs_network_content_wifi_icon_wifi) {
        if (status.state == 3) {
            // Connected - green icon, full opacity
            lv_obj_set_style_image_recolor(objects.settings_screen_tabs_network_content_wifi_icon_wifi, lv_color_hex(0xff00ff00), LV_PART_MAIN);
            lv_obj_set_style_image_recolor_opa(objects.settings_screen_tabs_network_content_wifi_icon_wifi, 255, LV_PART_MAIN);
            lv_obj_set_style_opa(objects.settings_screen_tabs_network_content_wifi_icon_wifi, 255, LV_PART_MAIN);
        } else if (status.state == 2) {
            // Connecting - yellow icon, full opacity
            lv_obj_set_style_image_recolor(objects.settings_screen_tabs_network_content_wifi_icon_wifi, lv_color_hex(0xffffaa00), LV_PART_MAIN);
            lv_obj_set_style_image_recolor_opa(objects.settings_screen_tabs_network_content_wifi_icon_wifi, 255, LV_PART_MAIN);
            lv_obj_set_style_opa(objects.settings_screen_tabs_network_content_wifi_icon_wifi, 255, LV_PART_MAIN);
        } else {
            // Disconnected - dimmed (30% opacity)
            lv_obj_set_style_image_recolor_opa(objects.settings_screen_tabs_network_content_wifi_icon_wifi, 0, LV_PART_MAIN);
            lv_obj_set_style_opa(objects.settings_screen_tabs_network_content_wifi_icon_wifi, 80, LV_PART_MAIN);
        }
    }

    // Update IP address label
    if (objects.settings_screen && objects.settings_screen_tabs_network_content_wifi_label_ip_address) {
        if (status.state == 3) {
            char ip_buf[24];
            snprintf(ip_buf, sizeof(ip_buf), "%d.%d.%d.%d",
                     status.ip[0], status.ip[1], status.ip[2], status.ip[3]);
            lv_label_set_text(objects.settings_screen_tabs_network_content_wifi_label_ip_address, ip_buf);
        } else {
            lv_label_set_text(objects.settings_screen_tabs_network_content_wifi_label_ip_address, "---");
        }
    }
}

// =============================================================================
// Screen Cleanup Helper (called by ui_core.c)
// =============================================================================

void ui_wifi_cleanup(void) {
    wifi_keyboard = NULL;
    wifi_focused_ta = NULL;
    wifi_scan_list = NULL;
}

// =============================================================================
// Module Initialization (called when WiFi screen is loaded)
// =============================================================================

void wire_wifi_settings_buttons(void) {
    if (!objects.settings_wifi_screen) return;

    // Reset module state when screen is created
    wifi_keyboard = NULL;
    wifi_focused_ta = NULL;

    // Wire textarea click events to show keyboard
    if (objects.settings_wifi_screen_content_panel_input_ssid) {
        lv_obj_add_flag(objects.settings_wifi_screen_content_panel_input_ssid, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(objects.settings_wifi_screen_content_panel_input_ssid, wifi_textarea_click_handler, LV_EVENT_CLICKED, NULL);
        // Update connect button when SSID changes
        lv_obj_add_event_cb(objects.settings_wifi_screen_content_panel_input_ssid, wifi_textarea_value_changed_handler, LV_EVENT_VALUE_CHANGED, NULL);
    }
    if (objects.settings_wifi_screen_content_panel_input_password) {
        lv_obj_add_flag(objects.settings_wifi_screen_content_panel_input_password, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(objects.settings_wifi_screen_content_panel_input_password, wifi_textarea_click_handler, LV_EVENT_CLICKED, NULL);
        lv_textarea_set_password_mode(objects.settings_wifi_screen_content_panel_input_password, true);
    }

    // Connect button
    if (objects.settings_wifi_screen_content_panel_button_connect_) {
        lv_obj_add_flag(objects.settings_wifi_screen_content_panel_button_connect_, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(objects.settings_wifi_screen_content_panel_button_connect_, wifi_connect_click_handler, LV_EVENT_CLICKED, NULL);
    }

    // Scan button
    if (objects.settings_wifi_screen_content_panel_button_scan_) {
        lv_obj_add_flag(objects.settings_wifi_screen_content_panel_button_scan_, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(objects.settings_wifi_screen_content_panel_button_scan_, wifi_scan_click_handler, LV_EVENT_CLICKED, NULL);
    }

    // Update initial state
    update_wifi_ui_state();
    // Set initial connect button state
    update_wifi_connect_btn_state();
}
