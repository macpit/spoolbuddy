#ifndef EEZ_LVGL_UI_IMAGES_H
#define EEZ_LVGL_UI_IMAGES_H

#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_img_dsc_t img_ams;
extern const lv_img_dsc_t img_bell_preview;
extern const lv_img_dsc_t img_encode_preview;
extern const lv_img_dsc_t img_humidity_mockup;
extern const lv_img_dsc_t img_humidity;
extern const lv_img_dsc_t img_nfc_preview;
extern const lv_img_dsc_t img_power_preview;
extern const lv_img_dsc_t img_setting_preview;
extern const lv_img_dsc_t img_spool_base;
extern const lv_img_dsc_t img_spool_clean;
extern const lv_img_dsc_t img_spool_fill;
extern const lv_img_dsc_t img_spool_frame;
extern const lv_img_dsc_t img_temperature;
extern const lv_img_dsc_t img_weight_preview;
extern const lv_img_dsc_t img_logo_28x28_preview;
extern const lv_img_dsc_t img_spool_mask;
extern const lv_img_dsc_t img_temp_mockup;
extern const lv_img_dsc_t img_spoolbuddy_logo_light;
extern const lv_img_dsc_t img_spoolbuddy_logo_transparent;
extern const lv_img_dsc_t img_signal;
extern const lv_img_dsc_t img_nfc;
extern const lv_img_dsc_t img_ams_setup;
extern const lv_img_dsc_t img_encoding;
extern const lv_img_dsc_t img_catalog;
extern const lv_img_dsc_t img_settings;
extern const lv_img_dsc_t img_spoolbuddy_logo_dark;
extern const lv_img_dsc_t img_filament_spool;
extern const lv_img_dsc_t img_bell;
extern const lv_img_dsc_t img_dot;
extern const lv_img_dsc_t img_scale;

#ifndef EXT_IMG_DESC_T
#define EXT_IMG_DESC_T
typedef struct _ext_img_desc_t {
    const char *name;
    const lv_img_dsc_t *img_dsc;
} ext_img_desc_t;
#endif

extern const ext_img_desc_t images[30];


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_IMAGES_H*/
