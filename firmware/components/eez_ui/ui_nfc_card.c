/**
 * NFC Card UI - Main screen NFC/Scale card management
 * Shows a popup when NFC tag is detected
 */

#include "ui_nfc_card.h"
#include "screens.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>

// External Rust FFI functions - NFC
extern bool nfc_is_initialized(void);
extern bool nfc_tag_present(void);
extern uint8_t nfc_get_uid_hex(uint8_t *buf, uint8_t buf_len);

// Decoded tag data (Rust FFI on ESP32, mock on simulator)
extern const char* nfc_get_tag_vendor(void);
extern const char* nfc_get_tag_material(void);
extern const char* nfc_get_tag_color_name(void);
extern uint32_t nfc_get_tag_color_rgba(void);

// External Rust FFI functions - Scale
extern float scale_get_weight(void);
extern bool scale_is_initialized(void);
extern bool scale_is_stable(void);

// Screen navigation
extern enum ScreensEnum pendingScreen;

// Static state
static bool last_tag_present = false;

// Popup elements
static lv_obj_t *tag_popup = NULL;
static lv_obj_t *popup_tag_label = NULL;
static lv_obj_t *popup_weight_label = NULL;

// Button click handlers
static void popup_close_handler(lv_event_t *e) {
    (void)e;
    if (tag_popup) {
        lv_obj_delete(tag_popup);
        tag_popup = NULL;
        popup_tag_label = NULL;
        popup_weight_label = NULL;
    }
}

static void configure_ams_click_handler(lv_event_t *e) {
    (void)e;
    // Close popup first
    popup_close_handler(NULL);
    // Navigate to scan_result screen (Encode Tag)
    pendingScreen = SCREEN_ID_SCAN_RESULT;
}

static void add_spool_click_handler(lv_event_t *e) {
    (void)e;
    // TODO: Navigate to add spool screen or show dialog
    printf("[ui_nfc_card] Add Spool clicked - not yet implemented\n");
}

// Close popup if open
static void close_popup(void) {
    if (tag_popup) {
        lv_obj_delete(tag_popup);
        tag_popup = NULL;
        popup_tag_label = NULL;
        popup_weight_label = NULL;
    }
}

// Create the tag detected popup
static void create_tag_popup(void) {
    if (tag_popup) return;  // Already open

    // Get tag UID
    uint8_t uid_str[32];
    nfc_get_uid_hex(uid_str, sizeof(uid_str));

    // Get weight
    float weight = scale_get_weight();
    bool scale_ok = scale_is_initialized();

    // Create modal background (semi-transparent overlay)
    tag_popup = lv_obj_create(lv_layer_top());
    lv_obj_set_size(tag_popup, 800, 480);
    lv_obj_set_pos(tag_popup, 0, 0);
    lv_obj_set_style_bg_color(tag_popup, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(tag_popup, 180, LV_PART_MAIN);
    lv_obj_set_style_border_width(tag_popup, 0, LV_PART_MAIN);
    lv_obj_clear_flag(tag_popup, LV_OBJ_FLAG_SCROLLABLE);

    // Click on background closes popup
    lv_obj_add_event_cb(tag_popup, popup_close_handler, LV_EVENT_CLICKED, NULL);

    // Create popup card (centered)
    lv_obj_t *card = lv_obj_create(tag_popup);
    lv_obj_set_size(card, 400, 280);
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1a1a1a), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, 255, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, lv_color_hex(0x4CAF50), LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(card, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 20, LV_PART_MAIN);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    // Prevent clicks on card from closing popup
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(card, NULL, LV_EVENT_CLICKED, NULL);  // Absorb click

    // Title
    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, "NFC Tag Detected");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0x4CAF50), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    // Spool icon (loop symbol represents filament spool)
    lv_obj_t *icon_label = lv_label_create(card);
    lv_label_set_text(icon_label, LV_SYMBOL_LOOP);
    lv_obj_set_style_text_font(icon_label, &lv_font_montserrat_28, LV_PART_MAIN);
    lv_obj_set_style_text_color(icon_label, lv_color_hex(0x4CAF50), LV_PART_MAIN);
    lv_obj_align(icon_label, LV_ALIGN_TOP_MID, 0, 35);

    // Tag UID
    popup_tag_label = lv_label_create(card);
    char tag_text[64];
    snprintf(tag_text, sizeof(tag_text), "Tag: %s", uid_str);
    lv_label_set_text(popup_tag_label, tag_text);
    lv_obj_set_style_text_font(popup_tag_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(popup_tag_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_align(popup_tag_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(popup_tag_label, LV_ALIGN_TOP_MID, 0, 70);

    // Weight
    popup_weight_label = lv_label_create(card);
    char weight_text[64];
    if (scale_ok) {
        snprintf(weight_text, sizeof(weight_text), "Weight: %.1fg", weight);
    } else {
        snprintf(weight_text, sizeof(weight_text), "Weight: N/A (scale not ready)");
    }
    lv_label_set_text(popup_weight_label, weight_text);
    lv_obj_set_style_text_font(popup_weight_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(popup_weight_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_align(popup_weight_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(popup_weight_label, LV_ALIGN_TOP_MID, 0, 95);

    // Decoded tag info (vendor, material, color)
    const char *vendor = nfc_get_tag_vendor();
    const char *material = nfc_get_tag_material();
    const char *color_name = nfc_get_tag_color_name();

    lv_obj_t *info_label = lv_label_create(card);
    char info_text[128];
    if (vendor && vendor[0] != '\0') {
        snprintf(info_text, sizeof(info_text), "Vendor: %s\nMaterial: %s\nColor: %s",
                 vendor, material[0] ? material : "Unknown", color_name[0] ? color_name : "Unknown");
    } else {
        snprintf(info_text, sizeof(info_text), "Material: Unknown\nColor: Unknown\n(Tag not decoded)");
    }
    lv_label_set_text(info_label, info_text);
    lv_obj_set_style_text_font(info_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(info_label, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
    lv_obj_set_style_text_align(info_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(info_label, LV_ALIGN_TOP_MID, 0, 125);

    // Buttons container
    lv_obj_t *btn_container = lv_obj_create(card);
    lv_obj_set_size(btn_container, 360, 50);
    lv_obj_align(btn_container, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(btn_container, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn_container, 0, LV_PART_MAIN);
    lv_obj_clear_flag(btn_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // "Add Spool" button
    lv_obj_t *btn_add = lv_btn_create(btn_container);
    lv_obj_set_size(btn_add, 150, 42);
    lv_obj_set_style_bg_color(btn_add, lv_color_hex(0x2D5A27), LV_PART_MAIN);
    lv_obj_set_style_radius(btn_add, 8, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_add, add_spool_click_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *add_label = lv_label_create(btn_add);
    lv_label_set_text(add_label, "Add Spool");
    lv_obj_set_style_text_font(add_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(add_label);

    // "Configure AMS" button
    lv_obj_t *btn_ams = lv_btn_create(btn_container);
    lv_obj_set_size(btn_ams, 170, 42);
    lv_obj_set_style_bg_color(btn_ams, lv_color_hex(0x1E88E5), LV_PART_MAIN);
    lv_obj_set_style_radius(btn_ams, 8, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_ams, configure_ams_click_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *ams_label = lv_label_create(btn_ams);
    lv_label_set_text(ams_label, "Configure AMS");
    lv_obj_set_style_text_font(ams_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(ams_label);
}

// Update weight display in popup if open
static void update_popup_weight(void) {
    if (!popup_weight_label) return;

    float weight = scale_get_weight();
    bool scale_ok = scale_is_initialized();

    char weight_text[64];
    if (scale_ok) {
        snprintf(weight_text, sizeof(weight_text), "Weight: %.1fg", weight);
    } else {
        snprintf(weight_text, sizeof(weight_text), "Weight: N/A (scale not ready)");
    }
    lv_label_set_text(popup_weight_label, weight_text);
}

void ui_nfc_card_init(void) {
    last_tag_present = false;
    close_popup();
}

void ui_nfc_card_cleanup(void) {
    close_popup();
    last_tag_present = false;
}

void ui_nfc_card_update(void) {
    if (!nfc_is_initialized()) return;

    bool tag_present = nfc_tag_present();

    // Tag state changed
    if (tag_present != last_tag_present) {
        last_tag_present = tag_present;

        if (tag_present) {
            // Tag detected - show popup
            create_tag_popup();
        } else {
            // Tag removed - close popup
            close_popup();
        }
    } else if (tag_present && tag_popup) {
        // Tag still present - update weight in popup
        update_popup_weight();
    }

    // Always update scale status label on main screen (shows current weight)
    if (objects.main_screen_nfc_scale_scale_label) {
        if (scale_is_initialized()) {
            float weight = scale_get_weight();
            char weight_str[16];
            snprintf(weight_str, sizeof(weight_str), "%.1fg", weight);
            lv_label_set_text(objects.main_screen_nfc_scale_scale_label, weight_str);
            lv_obj_set_style_text_color(objects.main_screen_nfc_scale_scale_label,
                lv_color_hex(0xFF00FF00), LV_PART_MAIN);
        } else {
            lv_label_set_text(objects.main_screen_nfc_scale_scale_label, "N/A");
            lv_obj_set_style_text_color(objects.main_screen_nfc_scale_scale_label,
                lv_color_hex(0xFFFF6600), LV_PART_MAIN);
        }
    }

    // NFC status always shows "Ready"
    if (objects.main_screen_nfc_scale_nfc_label) {
        lv_label_set_text(objects.main_screen_nfc_scale_nfc_label, "Ready");
    }
}
