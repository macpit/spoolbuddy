// =============================================================================
// ui_hardware.c - Hardware Settings Screens (NFC Reader, Scale)
// =============================================================================
// Programmatically creates NFC and Scale detail screens.
// =============================================================================

#include "ui_internal.h"
#include "screens.h"
#include "images.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// =============================================================================
// External Hardware Functions
// =============================================================================

// NFC functions
extern bool nfc_is_initialized(void);
extern bool nfc_tag_present(void);
extern uint8_t nfc_get_uid_hex(uint8_t *buf, uint8_t buf_len);

// Scale functions (from ui_scale.c)
extern float scale_get_weight(void);
extern bool scale_is_initialized(void);
extern bool scale_is_stable(void);
extern int32_t scale_tare(void);
extern int32_t scale_calibrate(float known_weight);

// =============================================================================
// Screen Objects (stored for updates)
// =============================================================================

// Splash Screen objects
static lv_obj_t *splash_screen = NULL;
static lv_obj_t *splash_logo = NULL;
static lv_obj_t *splash_spinner = NULL;
static lv_timer_t *splash_timer = NULL;
static int splash_stage = 0;  // Animation stage

// NFC Screen objects
static lv_obj_t *nfc_screen = NULL;
static lv_obj_t *nfc_screen_top_bar_icon_back = NULL;
static lv_obj_t *nfc_screen_top_bar_clock = NULL;
static lv_obj_t *nfc_screen_status_value = NULL;
static lv_obj_t *nfc_screen_uid_value = NULL;
static lv_obj_t *nfc_screen_tag_type_value = NULL;
static lv_obj_t *nfc_screen_tag_panel = NULL;

// Scale Calibration Screen objects (full screen per mockup)
static lv_obj_t *scale_calibration_screen = NULL;
static lv_obj_t *scale_cal_top_bar_icon_back = NULL;
static lv_obj_t *scale_cal_top_bar_clock = NULL;
static lv_obj_t *scale_cal_content = NULL;         // Content area (for scrolling)
static lv_obj_t *scale_cal_status_card = NULL;
static lv_obj_t *scale_cal_status_icon = NULL;
static lv_obj_t *scale_cal_status_text = NULL;
static lv_obj_t *scale_cal_status_subtitle = NULL;
static lv_obj_t *scale_cal_weight_input = NULL;
static lv_obj_t *scale_cal_weight_label = NULL;    // Live weight display
static lv_obj_t *scale_cal_status_label = NULL;    // Operation status
static lv_obj_t *scale_cal_keyboard = NULL;
static lv_timer_t *scale_cal_timer = NULL;
static int scale_cal_weight_value = 500;           // Default calibration weight

// Update timer
static lv_timer_t *hardware_update_timer = NULL;

// Forward declarations
static void scale_cal_timer_cb(lv_timer_t *timer);

// =============================================================================
// Common Colors
// =============================================================================

#define COLOR_BG_DARK       0x1a1a1a
#define COLOR_BG_PANEL      0x2d2d2d
#define COLOR_BORDER        0x3d3d3d
#define COLOR_TEXT_PRIMARY  0xffffff
#define COLOR_TEXT_SECONDARY 0x888888
#define COLOR_ACCENT_GREEN  0x00ff00
#define COLOR_ACCENT_YELLOW 0xffff00
#define COLOR_ACCENT_RED    0xff4444
#define COLOR_ACCENT_BLUE   0x0088ff

// =============================================================================
// Splash Screen
// =============================================================================

// Animation callback for logo fade-in and scale
static void splash_logo_anim_cb(void *var, int32_t value) {
    lv_obj_t *obj = (lv_obj_t *)var;
    lv_obj_set_style_opa(obj, value, LV_PART_MAIN);
}

static void splash_logo_scale_cb(void *var, int32_t value) {
    lv_obj_t *obj = (lv_obj_t *)var;
    lv_image_set_scale(obj, value);
}

// Timer callback for splash screen progression
static void splash_timer_cb(lv_timer_t *timer) {
    splash_stage++;

    if (splash_stage == 1) {
        // Stage 1: Fade in logo with scale animation
        if (splash_logo) {
            lv_anim_t anim_fade;
            lv_anim_init(&anim_fade);
            lv_anim_set_var(&anim_fade, splash_logo);
            lv_anim_set_values(&anim_fade, 0, 255);
            lv_anim_set_duration(&anim_fade, 800);
            lv_anim_set_exec_cb(&anim_fade, splash_logo_anim_cb);
            lv_anim_set_path_cb(&anim_fade, lv_anim_path_ease_out);
            lv_anim_start(&anim_fade);

            // Scale animation (zoom in slightly)
            lv_anim_t anim_scale;
            lv_anim_init(&anim_scale);
            lv_anim_set_var(&anim_scale, splash_logo);
            lv_anim_set_values(&anim_scale, 200, 280);  // 78% to 110% (256 = 100%)
            lv_anim_set_duration(&anim_scale, 800);
            lv_anim_set_exec_cb(&anim_scale, splash_logo_scale_cb);
            lv_anim_set_path_cb(&anim_scale, lv_anim_path_ease_out);
            lv_anim_start(&anim_scale);
        }
    } else if (splash_stage == 2) {
        // Stage 2: Fade in spinner
        if (splash_spinner) {
            lv_anim_t anim;
            lv_anim_init(&anim);
            lv_anim_set_var(&anim, splash_spinner);
            lv_anim_set_values(&anim, 0, 255);
            lv_anim_set_duration(&anim, 500);
            lv_anim_set_exec_cb(&anim, splash_logo_anim_cb);
            lv_anim_start(&anim);
        }
    } else if (splash_stage >= 5) {
        // Stage 5+: Transition to main screen
        if (splash_timer) {
            lv_timer_delete(splash_timer);
            splash_timer = NULL;
        }
        pendingScreen = SCREEN_ID_MAIN_SCREEN;
    }
}

void create_splash_screen(void) {
    if (splash_screen) return;

    splash_stage = 0;

    // Create screen with pure black background
    splash_screen = lv_obj_create(NULL);
    lv_obj_set_size(splash_screen, 800, 480);
    lv_obj_set_style_bg_color(splash_screen, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(splash_screen, 255, LV_PART_MAIN);
    lv_obj_set_style_border_width(splash_screen, 0, LV_PART_MAIN);
    lv_obj_clear_flag(splash_screen, LV_OBJ_FLAG_SCROLLABLE);

    // Logo (starts invisible and small)
    splash_logo = lv_image_create(splash_screen);
    lv_image_set_src(splash_logo, &img_spoolbuddy_logo_dark);
    lv_obj_align(splash_logo, LV_ALIGN_CENTER, 0, -30);
    lv_image_set_scale(splash_logo, 200);  // Start at 78%
    lv_obj_set_style_opa(splash_logo, 0, LV_PART_MAIN);  // Start invisible

    // Spinner (starts invisible, below logo)
    splash_spinner = lv_spinner_create(splash_screen);
    lv_obj_set_size(splash_spinner, 50, 50);
    lv_obj_align(splash_spinner, LV_ALIGN_CENTER, 0, 100);
    lv_spinner_set_anim_params(splash_spinner, 1200, 200);
    lv_obj_set_style_arc_color(splash_spinner, lv_color_hex(COLOR_ACCENT_GREEN), LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(splash_spinner, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_arc_width(splash_spinner, 6, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(splash_spinner, 6, LV_PART_MAIN);
    lv_obj_set_style_opa(splash_spinner, 0, LV_PART_MAIN);  // Start invisible

    // Start animation timer (300ms per stage)
    splash_timer = lv_timer_create(splash_timer_cb, 300, NULL);
}

lv_obj_t *get_splash_screen(void) {
    return splash_screen;
}

void cleanup_splash_screen(void) {
    if (splash_timer) {
        lv_timer_delete(splash_timer);
        splash_timer = NULL;
    }
    if (splash_screen) {
        lv_obj_delete(splash_screen);
        splash_screen = NULL;
        splash_logo = NULL;
        splash_spinner = NULL;
    }
    splash_stage = 0;
}

// =============================================================================
// Helper: Create Standard Top Bar
// =============================================================================

static lv_obj_t *create_top_bar(lv_obj_t *parent, const char *title, lv_obj_t **back_btn_out, lv_obj_t **clock_out) {
    // Top bar container
    lv_obj_t *top_bar = lv_obj_create(parent);
    lv_obj_set_pos(top_bar, 0, 0);
    lv_obj_set_size(top_bar, 800, 44);
    lv_obj_set_style_pad_all(top_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(top_bar, 0, LV_PART_MAIN);
    lv_obj_clear_flag(top_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(top_bar, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(top_bar, 255, LV_PART_MAIN);
    lv_obj_set_style_border_color(top_bar, lv_color_hex(COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(top_bar, 1, LV_PART_MAIN);
    lv_obj_set_style_border_side(top_bar, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);

    // Back button
    lv_obj_t *back_btn = lv_image_create(top_bar);
    lv_obj_set_pos(back_btn, 5, 1);
    lv_obj_set_size(back_btn, 48, 42);
    lv_image_set_src(back_btn, &img_back);
    lv_image_set_scale(back_btn, 80);
    lv_obj_add_flag(back_btn, LV_OBJ_FLAG_CLICKABLE);
    if (back_btn_out) *back_btn_out = back_btn;

    // Title
    lv_obj_t *title_label = lv_label_create(top_bar);
    lv_obj_set_pos(title_label, 60, 10);
    lv_label_set_text(title_label, title);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(title_label, lv_color_hex(COLOR_TEXT_PRIMARY), LV_PART_MAIN);

    // Bell/notification icon (same position as main screen: 662)
    lv_obj_t *bell_icon = lv_image_create(top_bar);
    lv_obj_set_pos(bell_icon, 662, 11);
    lv_obj_set_size(bell_icon, 24, 24);
    lv_image_set_src(bell_icon, &img_bell);
    lv_image_set_scale(bell_icon, 50);

    // WiFi signal icon (same position as main screen: 698)
    lv_obj_t *wifi_icon = lv_image_create(top_bar);
    lv_obj_set_pos(wifi_icon, 698, 10);
    lv_obj_set_size(wifi_icon, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_image_set_src(wifi_icon, &img_signal);
    lv_obj_set_style_image_recolor(wifi_icon, lv_color_hex(COLOR_ACCENT_GREEN), LV_PART_MAIN);
    lv_obj_set_style_image_recolor_opa(wifi_icon, 255, LV_PART_MAIN);

    // Clock
    lv_obj_t *clock = lv_label_create(top_bar);
    lv_obj_set_pos(clock, 737, 12);
    lv_label_set_text(clock, "00:00");
    lv_obj_set_style_text_font(clock, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(clock, lv_color_hex(COLOR_TEXT_PRIMARY), LV_PART_MAIN);
    if (clock_out) *clock_out = clock;

    return top_bar;
}

// =============================================================================
// Helper: Create Info Row
// =============================================================================

static lv_obj_t *create_info_row(lv_obj_t *parent, int y, const char *label_text, lv_obj_t **value_out) {
    // Row container
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_pos(row, 0, y);
    lv_obj_set_size(row, 735, 45);
    lv_obj_set_style_bg_color(row, lv_color_hex(COLOR_BG_PANEL), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(row, 255, LV_PART_MAIN);
    lv_obj_set_style_radius(row, 8, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(row, 15, LV_PART_MAIN);
    lv_obj_set_style_pad_right(row, 15, LV_PART_MAIN);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    // Label
    lv_obj_t *label = lv_label_create(row);
    lv_label_set_text(label, label_text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT_SECONDARY), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);

    // Value
    lv_obj_t *value = lv_label_create(row);
    lv_label_set_text(value, "---");
    lv_obj_set_style_text_font(value, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(value, lv_color_hex(COLOR_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_align(value, LV_ALIGN_RIGHT_MID, 0, 0);
    if (value_out) *value_out = value;

    return row;
}

// =============================================================================
// Back Button Handler
// =============================================================================

static void back_btn_handler(lv_event_t *e) {
    (void)e;
    pendingScreen = SCREEN_ID_SETTINGS_SCREEN;
    pending_settings_tab = 2;  // Hardware tab
}

// Back handler for calibration screen (returns to settings -> hardware tab)
static void calibration_back_btn_handler(lv_event_t *e) {
    (void)e;
    // Clean up timer before leaving
    if (scale_cal_timer) {
        lv_timer_delete(scale_cal_timer);
        scale_cal_timer = NULL;
    }
    // Navigate back to settings screen, hardware tab
    pendingScreen = SCREEN_ID_SETTINGS_SCREEN;
    pending_settings_tab = 2;  // Hardware tab
}

// =============================================================================
// Create NFC Screen
// =============================================================================

void create_nfc_screen(void) {
    if (nfc_screen) return;  // Already created

    // Main screen
    nfc_screen = lv_obj_create(NULL);
    lv_obj_set_size(nfc_screen, 800, 480);
    lv_obj_set_style_bg_color(nfc_screen, lv_color_hex(COLOR_BG_DARK), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(nfc_screen, 255, LV_PART_MAIN);

    // Top bar
    create_top_bar(nfc_screen, "NFC Reader", &nfc_screen_top_bar_icon_back, &nfc_screen_top_bar_clock);
    lv_obj_add_event_cb(nfc_screen_top_bar_icon_back, back_btn_handler, LV_EVENT_CLICKED, NULL);

    // Content area
    lv_obj_t *content = lv_obj_create(nfc_screen);
    lv_obj_set_pos(content, 0, 44);
    lv_obj_set_size(content, 800, 436);
    lv_obj_set_style_bg_color(content, lv_color_hex(COLOR_BG_DARK), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(content, 255, LV_PART_MAIN);
    lv_obj_set_style_border_width(content, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(content, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(content, 15, LV_PART_MAIN);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // Hardware Info Panel
    lv_obj_t *panel = lv_obj_create(content);
    lv_obj_set_pos(panel, 0, 0);
    lv_obj_set_size(panel, 765, 180);
    lv_obj_set_style_bg_color(panel, lv_color_hex(COLOR_BG_PANEL), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, 255, LV_PART_MAIN);
    lv_obj_set_style_radius(panel, 12, LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, 15, LV_PART_MAIN);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    // Panel title
    lv_obj_t *panel_title = lv_label_create(panel);
    lv_label_set_text(panel_title, "Hardware Info");
    lv_obj_set_style_text_font(panel_title, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(panel_title, lv_color_hex(COLOR_ACCENT_GREEN), LV_PART_MAIN);
    lv_obj_set_pos(panel_title, 0, 0);

    // Chip row
    lv_obj_t *chip_value;
    create_info_row(panel, 35, "Chip", &chip_value);
    lv_label_set_text(chip_value, "PN5180");

    // Status row
    create_info_row(panel, 90, "Status", &nfc_screen_status_value);

    // Tag Info Panel
    nfc_screen_tag_panel = lv_obj_create(content);
    lv_obj_set_pos(nfc_screen_tag_panel, 0, 195);
    lv_obj_set_size(nfc_screen_tag_panel, 765, 180);
    lv_obj_set_style_bg_color(nfc_screen_tag_panel, lv_color_hex(COLOR_BG_PANEL), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(nfc_screen_tag_panel, 255, LV_PART_MAIN);
    lv_obj_set_style_radius(nfc_screen_tag_panel, 12, LV_PART_MAIN);
    lv_obj_set_style_border_width(nfc_screen_tag_panel, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(nfc_screen_tag_panel, 15, LV_PART_MAIN);
    lv_obj_clear_flag(nfc_screen_tag_panel, LV_OBJ_FLAG_SCROLLABLE);

    // Tag panel title
    lv_obj_t *tag_title = lv_label_create(nfc_screen_tag_panel);
    lv_label_set_text(tag_title, "Tag Information");
    lv_obj_set_style_text_font(tag_title, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(tag_title, lv_color_hex(COLOR_ACCENT_GREEN), LV_PART_MAIN);
    lv_obj_set_pos(tag_title, 0, 0);

    // UID row
    create_info_row(nfc_screen_tag_panel, 35, "Tag UID", &nfc_screen_uid_value);

    // Tag type row
    create_info_row(nfc_screen_tag_panel, 90, "Tag Type", &nfc_screen_tag_type_value);
}

// =============================================================================
// Helper: Create Step Card with Number
// =============================================================================

static lv_obj_t *create_step_card(lv_obj_t *parent, int step_num, const char *text, int y_pos) {
    // Step card (row)
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_pos(card, 0, y_pos);
    lv_obj_set_size(card, 765, 45);
    lv_obj_set_style_bg_color(card, lv_color_hex(COLOR_BG_PANEL), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, 255, LV_PART_MAIN);
    lv_obj_set_style_radius(card, 8, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 10, LV_PART_MAIN);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    // Green circle with number
    lv_obj_t *circle = lv_obj_create(card);
    lv_obj_set_size(circle, 28, 28);
    lv_obj_align(circle, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(circle, lv_color_hex(COLOR_ACCENT_GREEN), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(circle, 255, LV_PART_MAIN);
    lv_obj_set_style_radius(circle, 14, LV_PART_MAIN);  // Full circle
    lv_obj_set_style_border_width(circle, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(circle, 0, LV_PART_MAIN);
    lv_obj_clear_flag(circle, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *num_label = lv_label_create(circle);
    char num_str[4];
    snprintf(num_str, sizeof(num_str), "%d", step_num);
    lv_label_set_text(num_label, num_str);
    lv_obj_set_style_text_font(num_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(num_label, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_center(num_label);

    // Step text
    lv_obj_t *text_label = lv_label_create(card);
    lv_label_set_text(text_label, text);
    lv_obj_set_style_text_font(text_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(text_label, lv_color_hex(COLOR_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_align(text_label, LV_ALIGN_LEFT_MID, 40, 0);

    return card;
}

// =============================================================================
// Calibration Screen Button Handlers
// =============================================================================

static void cal_screen_tare_handler(lv_event_t *e) {
    (void)e;
    int result = scale_tare();
    // Update status in top card
    if (result == 0) {
        if (scale_cal_status_text) {
            lv_label_set_text(scale_cal_status_text, "Scale Zeroed");
        }
        if (scale_cal_status_subtitle) {
            lv_label_set_text(scale_cal_status_subtitle, "Tare complete - ready for calibration");
        }
    } else {
        if (scale_cal_status_text) {
            lv_label_set_text(scale_cal_status_text, "Tare Failed");
            lv_obj_set_style_text_color(scale_cal_status_text, lv_color_hex(COLOR_ACCENT_RED), LV_PART_MAIN);
        }
        if (scale_cal_status_subtitle) {
            lv_label_set_text(scale_cal_status_subtitle, "Device not connected?");
        }
    }
    // Immediately update weight display
    scale_cal_timer_cb(NULL);
}

static void cal_screen_calibrate_handler(lv_event_t *e) {
    (void)e;
    if (scale_cal_weight_input) {
        const char *text = lv_textarea_get_text(scale_cal_weight_input);
        if (text && strlen(text) > 0) {
            float known_weight = (float)atof(text);
            if (known_weight > 0) {
                // Show calibrating status
                if (scale_cal_status_text) {
                    lv_label_set_text(scale_cal_status_text, "Calibrating...");
                    lv_obj_set_style_text_color(scale_cal_status_text, lv_color_hex(COLOR_ACCENT_YELLOW), LV_PART_MAIN);
                }
                if (scale_cal_status_subtitle) {
                    lv_label_set_text(scale_cal_status_subtitle, "Please wait...");
                }

                int result = scale_calibrate(known_weight);
                if (result == 0) {
                    char msg[64];
                    snprintf(msg, sizeof(msg), "Calibrated to %.0fg", known_weight);
                    if (scale_cal_status_text) {
                        lv_label_set_text(scale_cal_status_text, "Scale Calibrated");
                        lv_obj_set_style_text_color(scale_cal_status_text, lv_color_hex(COLOR_TEXT_PRIMARY), LV_PART_MAIN);
                    }
                    if (scale_cal_status_subtitle) {
                        lv_label_set_text(scale_cal_status_subtitle, msg);
                    }
                } else {
                    if (scale_cal_status_text) {
                        lv_label_set_text(scale_cal_status_text, "Calibration Failed");
                        lv_obj_set_style_text_color(scale_cal_status_text, lv_color_hex(COLOR_ACCENT_RED), LV_PART_MAIN);
                    }
                    if (scale_cal_status_subtitle) {
                        lv_label_set_text(scale_cal_status_subtitle, "Device not connected?");
                    }
                }
                // Immediately update weight display
                scale_cal_timer_cb(NULL);
                return;
            }
        }
        // Invalid weight entered
        if (scale_cal_status_text) {
            lv_label_set_text(scale_cal_status_text, "Invalid Weight");
            lv_obj_set_style_text_color(scale_cal_status_text, lv_color_hex(COLOR_ACCENT_RED), LV_PART_MAIN);
        }
        if (scale_cal_status_subtitle) {
            lv_label_set_text(scale_cal_status_subtitle, "Please enter a weight > 0");
        }
    }
}

static void cal_keyboard_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        // Close keyboard and scroll content back
        if (scale_cal_keyboard) {
            lv_obj_add_flag(scale_cal_keyboard, LV_OBJ_FLAG_HIDDEN);
        }
        if (scale_cal_content) {
            lv_obj_scroll_to_y(scale_cal_content, 0, LV_ANIM_ON);
        }
    }
}

static void cal_input_click_handler(lv_event_t *e) {
    (void)e;
    // Show keyboard when input is clicked
    if (scale_cal_keyboard) {
        lv_obj_clear_flag(scale_cal_keyboard, LV_OBJ_FLAG_HIDDEN);
        // Scroll content up so input field is visible above keyboard
        if (scale_cal_content) {
            lv_obj_scroll_to_y(scale_cal_content, 200, LV_ANIM_ON);
        }
    }
}

// Weight display hysteresis for calibration screen
static float scale_cal_last_displayed_weight = 0.0f;
static bool scale_cal_weight_initialized = false;

// Timer callback to update weight display on calibration screen
static void scale_cal_timer_cb(lv_timer_t *timer) {
    (void)timer;
    if (scale_cal_weight_label) {
        float weight = scale_get_weight();

        // Apply 5g hysteresis to reduce visual bouncing
        float diff = weight - scale_cal_last_displayed_weight;
        if (diff < 0) diff = -diff;

        if (!scale_cal_weight_initialized || diff >= 5.0f) {
            scale_cal_last_displayed_weight = weight;
            scale_cal_weight_initialized = true;

            char str[64];
            snprintf(str, sizeof(str), "Current: %.1f g", weight);
            lv_label_set_text(scale_cal_weight_label, str);
        }
        // Always show weight in yellow for visibility
        lv_obj_set_style_text_color(scale_cal_weight_label,
            lv_color_hex(COLOR_ACCENT_YELLOW), LV_PART_MAIN);
    }
}

// =============================================================================
// Create Scale Calibration Screen
// =============================================================================

void create_scale_calibration_screen(void) {
    if (scale_calibration_screen) return;

    // Main screen
    scale_calibration_screen = lv_obj_create(NULL);
    lv_obj_set_size(scale_calibration_screen, 800, 480);
    lv_obj_set_style_bg_color(scale_calibration_screen, lv_color_hex(COLOR_BG_DARK), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scale_calibration_screen, 255, LV_PART_MAIN);

    // Top bar
    create_top_bar(scale_calibration_screen, "Scale Calibration", &scale_cal_top_bar_icon_back, &scale_cal_top_bar_clock);
    lv_obj_add_event_cb(scale_cal_top_bar_icon_back, calibration_back_btn_handler, LV_EVENT_CLICKED, NULL);

    // Content area (scrollable for keyboard)
    scale_cal_content = lv_obj_create(scale_calibration_screen);
    lv_obj_set_pos(scale_cal_content, 0, 44);
    lv_obj_set_size(scale_cal_content, 800, 436);
    lv_obj_set_style_bg_color(scale_cal_content, lv_color_hex(COLOR_BG_DARK), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scale_cal_content, 255, LV_PART_MAIN);
    lv_obj_set_style_border_width(scale_cal_content, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(scale_cal_content, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scale_cal_content, 15, LV_PART_MAIN);
    // Enable vertical scrolling for keyboard visibility
    lv_obj_set_scroll_dir(scale_cal_content, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(scale_cal_content, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t *content = scale_cal_content;

    // Status card (top panel showing calibration state)
    scale_cal_status_card = lv_obj_create(content);
    lv_obj_set_pos(scale_cal_status_card, 0, 0);
    lv_obj_set_size(scale_cal_status_card, 765, 65);
    lv_obj_set_style_bg_color(scale_cal_status_card, lv_color_hex(COLOR_BG_PANEL), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scale_cal_status_card, 255, LV_PART_MAIN);
    lv_obj_set_style_radius(scale_cal_status_card, 12, LV_PART_MAIN);
    lv_obj_set_style_border_width(scale_cal_status_card, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scale_cal_status_card, 12, LV_PART_MAIN);
    lv_obj_clear_flag(scale_cal_status_card, LV_OBJ_FLAG_SCROLLABLE);

    // Checkmark icon (green circle with checkmark)
    scale_cal_status_icon = lv_obj_create(scale_cal_status_card);
    lv_obj_set_size(scale_cal_status_icon, 40, 40);
    lv_obj_align(scale_cal_status_icon, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(scale_cal_status_icon, lv_color_hex(COLOR_ACCENT_GREEN), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scale_cal_status_icon, 255, LV_PART_MAIN);
    lv_obj_set_style_radius(scale_cal_status_icon, 20, LV_PART_MAIN);  // Full circle
    lv_obj_set_style_border_width(scale_cal_status_icon, 0, LV_PART_MAIN);
    lv_obj_clear_flag(scale_cal_status_icon, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *check_label = lv_label_create(scale_cal_status_icon);
    lv_label_set_text(check_label, LV_SYMBOL_OK);
    lv_obj_set_style_text_font(check_label, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(check_label, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_center(check_label);

    // Status text
    scale_cal_status_text = lv_label_create(scale_cal_status_card);
    lv_label_set_text(scale_cal_status_text, "Ready to Calibrate");
    lv_obj_set_style_text_font(scale_cal_status_text, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(scale_cal_status_text, lv_color_hex(COLOR_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_align(scale_cal_status_text, LV_ALIGN_LEFT_MID, 55, -10);

    // Status subtitle
    scale_cal_status_subtitle = lv_label_create(scale_cal_status_card);
    lv_label_set_text(scale_cal_status_subtitle, "Follow the steps below");
    lv_obj_set_style_text_font(scale_cal_status_subtitle, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(scale_cal_status_subtitle, lv_color_hex(COLOR_TEXT_SECONDARY), LV_PART_MAIN);
    lv_obj_align(scale_cal_status_subtitle, LV_ALIGN_LEFT_MID, 55, 10);

    // "CALIBRATION STEPS" section header
    lv_obj_t *steps_header = lv_label_create(content);
    lv_label_set_text(steps_header, "CALIBRATION STEPS");
    lv_obj_set_style_text_font(steps_header, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(steps_header, lv_color_hex(COLOR_TEXT_SECONDARY), LV_PART_MAIN);
    lv_obj_set_pos(steps_header, 0, 75);

    // Step cards
    create_step_card(content, 1, "Remove all items from the scale and press \"Tare\"", 95);
    create_step_card(content, 2, "Place a known weight on scale", 148);
    create_step_card(content, 3, "Enter the exact weight and press \"Calibrate\"", 201);

    // "CALIBRATION WEIGHT (GRAMS)" label
    lv_obj_t *weight_label = lv_label_create(content);
    lv_label_set_text(weight_label, "CALIBRATION WEIGHT (GRAMS)");
    lv_obj_set_style_text_font(weight_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(weight_label, lv_color_hex(COLOR_TEXT_SECONDARY), LV_PART_MAIN);
    lv_obj_set_pos(weight_label, 0, 260);

    // Weight input container
    lv_obj_t *input_container = lv_obj_create(content);
    lv_obj_set_pos(input_container, 0, 280);
    lv_obj_set_size(input_container, 765, 55);
    lv_obj_set_style_bg_color(input_container, lv_color_hex(COLOR_BG_PANEL), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(input_container, 255, LV_PART_MAIN);
    lv_obj_set_style_radius(input_container, 8, LV_PART_MAIN);
    lv_obj_set_style_border_width(input_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(input_container, 10, LV_PART_MAIN);
    lv_obj_clear_flag(input_container, LV_OBJ_FLAG_SCROLLABLE);

    // Weight input field
    scale_cal_weight_input = lv_textarea_create(input_container);
    lv_obj_set_size(scale_cal_weight_input, 200, 35);
    lv_obj_align(scale_cal_weight_input, LV_ALIGN_LEFT_MID, 0, 0);
    lv_textarea_set_text(scale_cal_weight_input, "500");
    lv_textarea_set_one_line(scale_cal_weight_input, true);
    lv_textarea_set_accepted_chars(scale_cal_weight_input, "0123456789.");
    lv_obj_set_style_bg_color(scale_cal_weight_input, lv_color_hex(0x3d3d3d), LV_PART_MAIN);
    lv_obj_set_style_text_color(scale_cal_weight_input, lv_color_hex(COLOR_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_font(scale_cal_weight_input, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_border_color(scale_cal_weight_input, lv_color_hex(COLOR_BORDER), LV_PART_MAIN);
    lv_obj_add_event_cb(scale_cal_weight_input, cal_input_click_handler, LV_EVENT_CLICKED, NULL);

    // Live weight display
    scale_cal_weight_label = lv_label_create(input_container);
    lv_label_set_text(scale_cal_weight_label, "Current: 0.0 g");
    lv_obj_set_style_text_font(scale_cal_weight_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(scale_cal_weight_label, lv_color_hex(COLOR_TEXT_SECONDARY), LV_PART_MAIN);
    lv_obj_align(scale_cal_weight_label, LV_ALIGN_RIGHT_MID, 0, 0);

    // Status message label
    scale_cal_status_label = lv_label_create(content);
    lv_label_set_text(scale_cal_status_label, "");
    lv_obj_set_style_text_font(scale_cal_status_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(scale_cal_status_label, lv_color_hex(COLOR_TEXT_SECONDARY), LV_PART_MAIN);
    lv_obj_set_pos(scale_cal_status_label, 0, 345);

    // Button container (bottom)
    lv_obj_t *btn_container = lv_obj_create(content);
    lv_obj_set_pos(btn_container, 0, 370);
    lv_obj_set_size(btn_container, 765, 55);
    lv_obj_set_style_bg_opa(btn_container, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn_container, 0, LV_PART_MAIN);
    lv_obj_clear_flag(btn_container, LV_OBJ_FLAG_SCROLLABLE);

    // Tare button (gray, left)
    lv_obj_t *tare_btn = lv_button_create(btn_container);
    lv_obj_set_size(tare_btn, 370, 50);
    lv_obj_align(tare_btn, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(tare_btn, lv_color_hex(0x555555), LV_PART_MAIN);
    lv_obj_set_style_bg_color(tare_btn, lv_color_hex(0x444444), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_event_cb(tare_btn, cal_screen_tare_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *tare_label = lv_label_create(tare_btn);
    lv_label_set_text(tare_label, "Tare");
    lv_obj_set_style_text_font(tare_label, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(tare_label, lv_color_hex(COLOR_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_center(tare_label);

    // Calibrate button (green, right)
    lv_obj_t *calibrate_btn = lv_button_create(btn_container);
    lv_obj_set_size(calibrate_btn, 370, 50);
    lv_obj_align(calibrate_btn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(calibrate_btn, lv_color_hex(COLOR_ACCENT_GREEN), LV_PART_MAIN);
    lv_obj_set_style_bg_color(calibrate_btn, lv_color_hex(0x00cc00), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_event_cb(calibrate_btn, cal_screen_calibrate_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *cal_label = lv_label_create(calibrate_btn);
    lv_label_set_text(cal_label, "Calibrate");
    lv_obj_set_style_text_font(cal_label, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(cal_label, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_center(cal_label);

    // Keyboard (hidden by default)
    scale_cal_keyboard = lv_keyboard_create(scale_calibration_screen);
    lv_keyboard_set_mode(scale_cal_keyboard, LV_KEYBOARD_MODE_NUMBER);
    lv_keyboard_set_textarea(scale_cal_keyboard, scale_cal_weight_input);
    lv_obj_add_event_cb(scale_cal_keyboard, cal_keyboard_handler, LV_EVENT_ALL, NULL);
    lv_obj_add_flag(scale_cal_keyboard, LV_OBJ_FLAG_HIDDEN);

    // Start timer for live weight updates
    scale_cal_timer = lv_timer_create(scale_cal_timer_cb, 200, NULL);
    scale_cal_timer_cb(NULL);  // Initial update
}

// =============================================================================
// Get Screen Objects
// =============================================================================

lv_obj_t *get_nfc_screen(void) {
    return nfc_screen;
}

lv_obj_t *get_scale_calibration_screen(void) {
    return scale_calibration_screen;
}

// =============================================================================
// Update Functions
// =============================================================================

void update_nfc_screen(void) {
    if (!nfc_screen || lv_scr_act() != nfc_screen) return;

    bool initialized = nfc_is_initialized();
    bool tag_present = nfc_tag_present();

    // Update status
    if (nfc_screen_status_value) {
        if (!initialized) {
            lv_label_set_text(nfc_screen_status_value, "Not Initialized");
            lv_obj_set_style_text_color(nfc_screen_status_value, lv_color_hex(COLOR_ACCENT_RED), LV_PART_MAIN);
        } else if (tag_present) {
            lv_label_set_text(nfc_screen_status_value, "Tag Detected");
            lv_obj_set_style_text_color(nfc_screen_status_value, lv_color_hex(COLOR_ACCENT_GREEN), LV_PART_MAIN);
        } else {
            lv_label_set_text(nfc_screen_status_value, "Ready");
            lv_obj_set_style_text_color(nfc_screen_status_value, lv_color_hex(COLOR_ACCENT_GREEN), LV_PART_MAIN);
        }
    }

    // Update tag info
    if (tag_present) {
        if (nfc_screen_tag_panel) {
            lv_obj_clear_flag(nfc_screen_tag_panel, LV_OBJ_FLAG_HIDDEN);
        }
        if (nfc_screen_uid_value) {
            uint8_t hex_buf[32];
            uint8_t len = nfc_get_uid_hex(hex_buf, sizeof(hex_buf) - 1);
            hex_buf[len] = '\0';
            lv_label_set_text(nfc_screen_uid_value, (char*)hex_buf);
        }
        if (nfc_screen_tag_type_value) {
            // Could detect tag type based on UID prefix
            lv_label_set_text(nfc_screen_tag_type_value, "NFC-A");
        }
    } else {
        if (nfc_screen_uid_value) {
            lv_label_set_text(nfc_screen_uid_value, "No tag");
        }
        if (nfc_screen_tag_type_value) {
            lv_label_set_text(nfc_screen_tag_type_value, "---");
        }
    }

    // Update clock
    if (nfc_screen_top_bar_clock) {
        int time_hhmm = time_get_hhmm();
        if (time_hhmm >= 0) {
            int hour = (time_hhmm >> 8) & 0xFF;
            int minute = time_hhmm & 0xFF;
            char time_str[8];
            snprintf(time_str, sizeof(time_str), "%02d:%02d", hour, minute);
            lv_label_set_text(nfc_screen_top_bar_clock, time_str);
        }
    }
}

void update_scale_calibration_screen(void) {
    if (!scale_calibration_screen || lv_scr_act() != scale_calibration_screen) return;

    // Update clock
    if (scale_cal_top_bar_clock) {
        int time_hhmm = time_get_hhmm();
        if (time_hhmm >= 0) {
            int hour = (time_hhmm >> 8) & 0xFF;
            int minute = time_hhmm & 0xFF;
            char time_str[8];
            snprintf(time_str, sizeof(time_str), "%02d:%02d", hour, minute);
            lv_label_set_text(scale_cal_top_bar_clock, time_str);
        }
    }

    // Weight is updated via timer (scale_cal_timer_cb)
}

// =============================================================================
// Cleanup
// =============================================================================

void cleanup_hardware_screens(void) {
    if (hardware_update_timer) {
        lv_timer_delete(hardware_update_timer);
        hardware_update_timer = NULL;
    }
    // Delete programmatic screens ONLY if they're not the active screen
    // (During transition TO these screens, we load them first then clean up)
    lv_obj_t *active = lv_scr_act();
    if (nfc_screen && nfc_screen != active) {
        lv_obj_delete(nfc_screen);
        nfc_screen = NULL;
    }
    if (scale_calibration_screen && scale_calibration_screen != active) {
        // Clean up calibration timer first
        if (scale_cal_timer) {
            lv_timer_delete(scale_cal_timer);
            scale_cal_timer = NULL;
        }
        lv_obj_delete(scale_calibration_screen);
        scale_calibration_screen = NULL;
    }
    // Reset other pointers (only if screens were deleted)
    if (!nfc_screen) {
        nfc_screen_top_bar_icon_back = NULL;
        nfc_screen_top_bar_clock = NULL;
        nfc_screen_status_value = NULL;
        nfc_screen_uid_value = NULL;
        nfc_screen_tag_type_value = NULL;
        nfc_screen_tag_panel = NULL;
    }
    if (!scale_calibration_screen) {
        scale_cal_top_bar_icon_back = NULL;
        scale_cal_top_bar_clock = NULL;
        scale_cal_content = NULL;
        scale_cal_status_card = NULL;
        scale_cal_status_icon = NULL;
        scale_cal_status_text = NULL;
        scale_cal_status_subtitle = NULL;
        scale_cal_weight_input = NULL;
        scale_cal_weight_label = NULL;
        scale_cal_status_label = NULL;
        scale_cal_keyboard = NULL;
    }
}
