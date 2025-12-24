#if defined(EEZ_FOR_LVGL)
#include <eez/core/vars.h>
#endif

#include "ui.h"
#include "screens.h"
#include "images.h"
#include "actions.h"
#include "vars.h"

#if defined(EEZ_FOR_LVGL)

void ui_init() {
    eez_flow_init(assets, sizeof(assets), (lv_obj_t **)&objects, sizeof(objects), images, sizeof(images), actions);
}

void ui_tick() {
    eez_flow_tick();
    tick_screen(g_currentScreen);
}

#else

#include <string.h>

static int16_t currentScreen = -1;
static enum ScreensEnum pendingScreen = 0;
static enum ScreensEnum previousScreen = SCREEN_ID_MAIN;

static lv_obj_t *getLvglObjectFromIndex(int32_t index) {
    if (index == -1) return 0;
    return ((lv_obj_t **)&objects)[index];
}

void loadScreen(enum ScreensEnum screenId) {
    currentScreen = screenId - 1;
    lv_obj_t *screen = getLvglObjectFromIndex(currentScreen);
    if (screen) {
        lv_screen_load(screen);
        lv_obj_invalidate(screen);
        lv_refr_now(NULL);
    }
}

// Button event handlers
static void ams_setup_click_handler(lv_event_t *e) {
    pendingScreen = SCREEN_ID_AMS_OVERVIEW;
}

static void home_click_handler(lv_event_t *e) {
    pendingScreen = SCREEN_ID_MAIN;
}

static void encode_tag_click_handler(lv_event_t *e) {
    pendingScreen = SCREEN_ID_SCAN_RESULT;
}

static void catalog_click_handler(lv_event_t *e) {
    pendingScreen = SCREEN_ID_SPOOL_DETAILS;
}

static void settings_click_handler(lv_event_t *e) {
    // TODO: Navigate to settings screen when it exists
}

static void back_click_handler(lv_event_t *e) {
    pendingScreen = previousScreen;
}

// Wire up buttons for each screen
static void wire_main_buttons(void) {
    lv_obj_add_event_cb(objects.ams_setup, ams_setup_click_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.encode_tag, encode_tag_click_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.catalog, catalog_click_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.settings, settings_click_handler, LV_EVENT_CLICKED, NULL);
}

static void wire_ams_overview_buttons(void) {
    lv_obj_add_event_cb(objects.ams_setup_1, home_click_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.encode_tag_1, encode_tag_click_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.catalog_1, catalog_click_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.settings_1, settings_click_handler, LV_EVENT_CLICKED, NULL);
}

static void wire_scan_result_buttons(void) {
    // Back button is first child of top_bar_2 - make it clickable
    lv_obj_t *back_btn = lv_obj_get_child(objects.top_bar_2, 0);
    if (back_btn) {
        lv_obj_add_flag(back_btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(back_btn, back_click_handler, LV_EVENT_CLICKED, NULL);
    }
}

static void wire_spool_details_buttons(void) {
    // Back button is first child of top_bar_3 - make it clickable
    lv_obj_t *back_btn = lv_obj_get_child(objects.top_bar_3, 0);
    if (back_btn) {
        lv_obj_add_flag(back_btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(back_btn, back_click_handler, LV_EVENT_CLICKED, NULL);
    }
}

// Delete all screens to free memory
static void delete_all_screens(void) {
    if (objects.main) {
        lv_obj_delete(objects.main);
        objects.main = NULL;
    }
    if (objects.ams_overview) {
        lv_obj_delete(objects.ams_overview);
        objects.ams_overview = NULL;
    }
    if (objects.scan_result) {
        lv_obj_delete(objects.scan_result);
        objects.scan_result = NULL;
    }
    if (objects.spool_details) {
        lv_obj_delete(objects.spool_details);
        objects.spool_details = NULL;
    }
}

void ui_init() {
    // Initialize theme
    lv_display_t *dispp = lv_display_get_default();
    if (dispp) {
        lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), true, LV_FONT_DEFAULT);
        lv_display_set_theme(dispp, theme);
    }

    // Create main screen
    create_screen_main();
    wire_main_buttons();
    loadScreen(SCREEN_ID_MAIN);
}

void ui_tick() {
    if (pendingScreen != 0) {
        enum ScreensEnum screen = pendingScreen;
        pendingScreen = 0;

        // Track previous screen for back navigation
        enum ScreensEnum currentScreenId = (enum ScreensEnum)(currentScreen + 1);
        if (screen != previousScreen) {
            previousScreen = currentScreenId;
        }

        // Delete old screen and create new one
        delete_all_screens();

        switch (screen) {
            case SCREEN_ID_MAIN:
                create_screen_main();
                wire_main_buttons();
                break;
            case SCREEN_ID_AMS_OVERVIEW:
                create_screen_ams_overview();
                wire_ams_overview_buttons();
                break;
            case SCREEN_ID_SCAN_RESULT:
                create_screen_scan_result();
                wire_scan_result_buttons();
                break;
            case SCREEN_ID_SPOOL_DETAILS:
                create_screen_spool_details();
                wire_spool_details_buttons();
                break;
        }

        loadScreen(screen);
    }

    tick_screen(currentScreen);
}

#endif
