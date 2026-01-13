// =============================================================================
// ui_settings.c - Settings Screen Tab and Menu Handlers
// =============================================================================
// Handles settings tab switching and menu row navigation.
// =============================================================================

#include "ui_internal.h"
#include "screens.h"
#include <string.h>

// =============================================================================
// Tab Switching
// =============================================================================

void select_settings_tab(int tab_index) {
    // Tab button objects
    lv_obj_t *tabs[] = {
        objects.settings_screen_tabs_network,
        objects.settings_screen_tabs_printers,
        objects.settings_screen_tabs_hardware,
        objects.settings_screen_tabs_system
    };
    // Tab content objects
    lv_obj_t *contents[] = {
        objects.settings_screen_tabs_network_content,
        objects.settings_screen_tabs_printers_content,
        objects.settings_screen_tabs_hardware_content,
        objects.settings_screen_tabs_system_content
    };

    for (int i = 0; i < 4; i++) {
        if (tabs[i]) {
            if (i == tab_index) {
                // Selected tab - green background, black text
                lv_obj_set_style_bg_color(tabs[i], lv_color_hex(0xff00ff00), LV_PART_MAIN);
                lv_obj_t *label = lv_obj_get_child(tabs[i], 0);
                if (label) lv_obj_set_style_text_color(label, lv_color_hex(0xff000000), LV_PART_MAIN);
            } else {
                // Unselected tab - dark background, gray text
                lv_obj_set_style_bg_color(tabs[i], lv_color_hex(0xff252525), LV_PART_MAIN);
                lv_obj_t *label = lv_obj_get_child(tabs[i], 0);
                if (label) lv_obj_set_style_text_color(label, lv_color_hex(0xff888888), LV_PART_MAIN);
            }
        }
        if (contents[i]) {
            if (i == tab_index) {
                lv_obj_remove_flag(contents[i], LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(contents[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

static void tab_network_handler(lv_event_t *e) { select_settings_tab(0); }
static void tab_printers_handler(lv_event_t *e) { select_settings_tab(1); }
static void tab_hardware_handler(lv_event_t *e) { select_settings_tab(2); }
static void tab_system_handler(lv_event_t *e) { select_settings_tab(3); }

// =============================================================================
// Settings Menu Row Handlers
// =============================================================================

// Settings menu row click handler - gets title from first label child
static void settings_row_click_handler(lv_event_t *e) {
    lv_obj_t *row = lv_event_get_target(e);
    // Find label child to get the title
    uint32_t child_count = lv_obj_get_child_count(row);
    for (uint32_t i = 0; i < child_count; i++) {
        lv_obj_t *child = lv_obj_get_child(row, i);
        if (lv_obj_check_type(child, &lv_label_class)) {
            const char *text = lv_label_get_text(child);
            if (text && strlen(text) > 0) {
                navigate_to_settings_detail(text);
                return;
            }
        }
    }
    navigate_to_settings_detail("Settings");
}

// Wire click handlers for all child rows in a content area
static void wire_content_rows(lv_obj_t *content) {
    if (!content) return;
    uint32_t child_count = lv_obj_get_child_count(content);
    for (uint32_t i = 0; i < child_count; i++) {
        lv_obj_t *child = lv_obj_get_child(content, i);
        if (child) {
            lv_obj_add_flag(child, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_remove_flag(child, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
            // Add pressed style for visual feedback
            lv_obj_set_style_bg_color(child, lv_color_hex(0xff3d3d3d), LV_PART_MAIN | LV_STATE_PRESSED);
            lv_obj_add_event_cb(child, settings_row_click_handler, LV_EVENT_CLICKED, NULL);
        }
    }
}

// =============================================================================
// Settings Detail Title (no longer used - removed in new EEZ design)
// =============================================================================

void update_settings_detail_title(void) {
    // No longer needed - new EEZ design has dedicated screens with static titles
}

// =============================================================================
// Back Button Handler (shared by detail screens)
// =============================================================================

static void settings_detail_back_handler(lv_event_t *e) {
    pending_settings_tab = -1;  // Don't change tab
    pendingScreen = SCREEN_ID_SETTINGS_SCREEN;
}

// =============================================================================
// Wire Functions
// =============================================================================

void wire_settings_buttons(void) {
    // Back button - find first child of top bar if it exists
    if (objects.settings_network_screen_top_bar_icon_back) {
        lv_obj_add_flag(objects.settings_network_screen_top_bar_icon_back, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(objects.settings_network_screen_top_bar_icon_back, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        lv_obj_set_style_opa(objects.settings_network_screen_top_bar_icon_back, 180, LV_PART_MAIN | LV_STATE_PRESSED);
        extern void back_click_handler(lv_event_t *e);
        lv_obj_add_event_cb(objects.settings_network_screen_top_bar_icon_back, back_click_handler, LV_EVENT_CLICKED, NULL);
    }

    // Tab buttons - make clickable and add pressed style for feedback
    lv_obj_t *tabs[] = {
        objects.settings_screen_tabs_network,
        objects.settings_screen_tabs_printers,
        objects.settings_screen_tabs_hardware,
        objects.settings_screen_tabs_system
    };
    void (*handlers[])(lv_event_t*) = {tab_network_handler, tab_printers_handler, tab_hardware_handler, tab_system_handler};
    for (int i = 0; i < 4; i++) {
        if (tabs[i]) {
            lv_obj_add_flag(tabs[i], LV_OBJ_FLAG_CLICKABLE);
            lv_obj_remove_flag(tabs[i], LV_OBJ_FLAG_SCROLL_ON_FOCUS);
            lv_obj_set_style_bg_color(tabs[i], lv_color_hex(0xff3d3d3d), LV_PART_MAIN | LV_STATE_PRESSED);
            lv_obj_add_event_cb(tabs[i], handlers[i], LV_EVENT_CLICKED, NULL);
        }
    }

    // Wire menu rows in each tab content
    wire_content_rows(objects.settings_screen_tabs_network_content);
    wire_content_rows(objects.settings_screen_tabs_printers_content);
    wire_content_rows(objects.settings_screen_tabs_hardware_content);
    wire_content_rows(objects.settings_screen_tabs_system_content);

    // Initialize with first tab selected, hide others
    select_settings_tab(0);
}

void wire_settings_detail_buttons(void) {
    // No longer used - new EEZ design has dedicated screens
}

void wire_settings_subpage_buttons(lv_obj_t *back_btn) {
    if (back_btn) {
        lv_obj_add_flag(back_btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(back_btn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        lv_obj_set_style_opa(back_btn, 180, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_add_event_cb(back_btn, settings_detail_back_handler, LV_EVENT_CLICKED, NULL);
    }
}
