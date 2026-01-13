#ifndef EEZ_LVGL_UI_IMAGES_H
#define EEZ_LVGL_UI_IMAGES_H

#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_image_dsc_t img_humidity;
extern const lv_image_dsc_t img_spool_base;
extern const lv_image_dsc_t img_spool_clean;
extern const lv_image_dsc_t img_spool_fill;
extern const lv_image_dsc_t img_spool_frame;
extern const lv_image_dsc_t img_spool_mask;
extern const lv_image_dsc_t img_spoolbuddy_logo_light;
extern const lv_image_dsc_t img_signal;
extern const lv_image_dsc_t img_nfc;
extern const lv_image_dsc_t img_encoding;
extern const lv_image_dsc_t img_catalog;
extern const lv_image_dsc_t img_settings;
extern const lv_image_dsc_t img_filament_spool;
extern const lv_image_dsc_t img_bell;
extern const lv_image_dsc_t img_dot;
extern const lv_image_dsc_t img_scale;
extern const lv_image_dsc_t img_amssetup;
extern const lv_image_dsc_t img_spool;
extern const lv_image_dsc_t img_spoolbuddy_logo_dark;
extern const lv_image_dsc_t img_back;
extern const lv_image_dsc_t img_ok;
extern const lv_image_dsc_t img_home;
extern const lv_image_dsc_t img_circle_empty;
extern const lv_image_dsc_t img_wifi;
extern const lv_image_dsc_t img_ip_address;
extern const lv_image_dsc_t img_mqtt;
extern const lv_image_dsc_t img_add;
extern const lv_image_dsc_t img_3d_cube;
extern const lv_image_dsc_t img_scale_2;
extern const lv_image_dsc_t img_display;
extern const lv_image_dsc_t img_calibrate;
extern const lv_image_dsc_t img_firmware;
extern const lv_image_dsc_t img_update;
extern const lv_image_dsc_t img_reset;
extern const lv_image_dsc_t img_about;
extern const lv_image_dsc_t img_thermometer;
extern const lv_image_dsc_t img_wifi_signal_1;
extern const lv_image_dsc_t img_wifi_signal_2;
extern const lv_image_dsc_t img_wifi_signal_3;
extern const lv_image_dsc_t img_wifi_signal_4;

#ifndef EXT_IMG_DESC_T
#define EXT_IMG_DESC_T
typedef struct _ext_img_desc_t {
    const char *name;
    const lv_image_dsc_t *img_dsc;
} ext_img_desc_t;
#endif

extern const ext_img_desc_t images[40];


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_IMAGES_H*/