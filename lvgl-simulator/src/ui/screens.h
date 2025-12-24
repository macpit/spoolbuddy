#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *top_bar;
    lv_obj_t *spoolbuddy_logo;
    lv_obj_t *printer_select;
    lv_obj_t *wifi_signal;
    lv_obj_t *notification_bell;
    lv_obj_t *clock;
    lv_obj_t *bottom_bar;
    lv_obj_t *status_dot;
    lv_obj_t *status;
    lv_obj_t *status_1;
    lv_obj_t *rught_nozzle;
    lv_obj_t *obj0;
    lv_obj_t *obj1;
    lv_obj_t *obj2;
    lv_obj_t *obj3;
    lv_obj_t *obj4;
    lv_obj_t *obj5;
    lv_obj_t *obj6;
    lv_obj_t *obj7;
    lv_obj_t *obj8;
    lv_obj_t *obj9;
    lv_obj_t *obj10;
    lv_obj_t *obj11;
    lv_obj_t *obj12;
    lv_obj_t *ams_setup;
    lv_obj_t *obj13;
    lv_obj_t *encode_tag;
    lv_obj_t *obj14;
    lv_obj_t *settings;
    lv_obj_t *obj15;
    lv_obj_t *catalog;
    lv_obj_t *obj16;
    lv_obj_t *nfc_scale;
    lv_obj_t *obj17;
    lv_obj_t *obj18;
    lv_obj_t *obj19;
    lv_obj_t *obj20;
    lv_obj_t *obj21;
    lv_obj_t *obj22;
    lv_obj_t *left_nozzle;
    lv_obj_t *obj23;
    lv_obj_t *obj24;
    lv_obj_t *obj25;
    lv_obj_t *obj26;
    lv_obj_t *obj27;
    lv_obj_t *obj28;
    lv_obj_t *obj29;
    lv_obj_t *obj30;
    lv_obj_t *obj31;
    lv_obj_t *obj32;
    lv_obj_t *obj33;
    lv_obj_t *obj34;
    lv_obj_t *obj35;
    lv_obj_t *obj36;
    lv_obj_t *obj37;
    lv_obj_t *obj38;
    lv_obj_t *obj39;
    lv_obj_t *obj40;
    lv_obj_t *obj41;
    lv_obj_t *obj42;
    lv_obj_t *obj43;
    lv_obj_t *obj44;
    lv_obj_t *obj45;
    lv_obj_t *obj46;
    lv_obj_t *printer;
    lv_obj_t *print_cover;
    lv_obj_t *printer_label;
    lv_obj_t *printer_label_1;
    lv_obj_t *printer_label_2;
    lv_obj_t *printer_label_3;
    lv_obj_t *obj47;
    lv_obj_t *obj48;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_MAIN = 1,
};

void create_screen_main();
void tick_screen_main();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/