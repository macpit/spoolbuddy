#include "actions.h"
#include "screens.h"
#include "ui.h"

void action_go_to_ams_overview(lv_event_t *e) {
    (void)e;
    loadScreen(SCREEN_ID_AMS_OVERVIEW);
}

void action_go_to_home(lv_event_t *e) {
    (void)e;
    loadScreen(SCREEN_ID_MAIN);
}

void action_encode_tag(lv_event_t *e) {
    (void)e;
    // TODO: Navigate to NFC encode screen
}

void action_go_to_settings(lv_event_t *e) {
    (void)e;
    // TODO: Navigate to settings screen
}

void action_go_to_catalog(lv_event_t *e) {
    (void)e;
    // TODO: Navigate to catalog screen
}
