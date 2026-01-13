#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;
lv_obj_t *tick_value_change_obj;
uint32_t active_theme_index = 0;

void create_screen_main_screen() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main_screen = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 800, 480);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // top_bar
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.top_bar = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 800, 44);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3d3d3d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // spoolbuddy_logo
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.spoolbuddy_logo = obj;
                    lv_obj_set_pos(obj, -8, -1);
                    lv_obj_set_size(obj, 173, 46);
                    lv_image_set_src(obj, &img_spoolbuddy_logo_dark);
                    lv_image_set_scale(obj, 200);
                    lv_obj_set_style_align(obj, LV_ALIGN_TOP_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // top_bar_printer_select
                    lv_obj_t *obj = lv_dropdown_create(parent_obj);
                    objects.top_bar_printer_select = obj;
                    lv_obj_set_pos(obj, 325, 2);
                    lv_obj_set_size(obj, 150, 39);
                    lv_dropdown_set_options(obj, "");
                    lv_dropdown_set_selected(obj, 0);
                }
                {
                    // top_bar_wifi_signal
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.top_bar_wifi_signal = obj;
                    lv_obj_set_pos(obj, 698, 10);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_image_set_src(obj, &img_signal);
                    lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // top_bar_notification_bell
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.top_bar_notification_bell = obj;
                    lv_obj_set_pos(obj, 662, 11);
                    lv_obj_set_size(obj, 24, 24);
                    lv_image_set_src(obj, &img_bell);
                    lv_image_set_scale(obj, 50);
                }
                {
                    // top_bar_clock
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.top_bar_clock = obj;
                    lv_obj_set_pos(obj, 737, 12);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
        {
            // bottom_bar
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.bottom_bar = obj;
            lv_obj_set_pos(obj, 0, 450);
            lv_obj_set_size(obj, 800, 30);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xfffaaa05), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_TOP, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // bottom_bar_message_dot
                    lv_obj_t *obj = lv_led_create(parent_obj);
                    objects.bottom_bar_message_dot = obj;
                    lv_obj_set_pos(obj, 13, 7);
                    lv_obj_set_size(obj, 12, 12);
                    lv_led_set_color(obj, lv_color_hex(0xfffaaa05));
                    lv_led_set_brightness(obj, 0);
                }
                {
                    // bottom_bar_message
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.bottom_bar_message = obj;
                    lv_obj_set_pos(obj, 33, 6);
                    lv_obj_set_size(obj, 754, 16);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
        {
            // main_screen_ams_right_nozzle
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.main_screen_ams_right_nozzle = obj;
            lv_obj_set_pos(obj, 402, 319);
            lv_obj_set_size(obj, 385, 127);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_arc_width(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_rounded(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff796666), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // main_screen_ams_right_nozzle_indicator
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_screen_ams_right_nozzle_indicator = obj;
                    lv_obj_set_pos(obj, -14, -17);
                    lv_obj_set_size(obj, 12, 12);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "R");
                }
                {
                    // main_screen_ams_right_nozzle_text
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_screen_ams_right_nozzle_text = obj;
                    lv_obj_set_pos(obj, 2, -17);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, 12);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Right Nozzle");
                }
                {
                    // main_screen_ams_ht-a
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.main_screen_ams_ht_a = obj;
                    lv_obj_set_pos(obj, -14, 50);
                    lv_obj_set_size(obj, 47, 50);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_arc_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_rounded(obj, false, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // main_screen_ams_ht-a_text
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_screen_ams_ht_a_text = obj;
                            lv_obj_set_pos(obj, -14, -17);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "HT-A");
                        }
                        {
                            // main_screen_ams_ht-a_slot
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_screen_ams_ht_a_slot = obj;
                            lv_obj_set_pos(obj, -11, -1);
                            lv_obj_set_size(obj, 23, 24);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff726e6e), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_grad_stop(obj, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_main_stop(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff352a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                    }
                }
                {
                    // main_screen_ams_ext-1
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.main_screen_ams_ext_1 = obj;
                    lv_obj_set_pos(obj, 40, 50);
                    lv_obj_set_size(obj, 47, 50);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_arc_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_rounded(obj, false, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // main_screen_ams_ext-1_text
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_screen_ams_ext_1_text = obj;
                            lv_obj_set_pos(obj, -14, -17);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Ext-1");
                        }
                        {
                            // main_screen_ams_ext-1_slot
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_screen_ams_ext_1_slot = obj;
                            lv_obj_set_pos(obj, -11, -1);
                            lv_obj_set_size(obj, 23, 24);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff726e6e), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff352a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_grad_stop(obj, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_main_stop(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                    }
                }
                {
                    // main_screen_ams_b
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.main_screen_ams_b = obj;
                    lv_obj_set_pos(obj, -14, -2);
                    lv_obj_set_size(obj, 120, 50);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_arc_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_rounded(obj, false, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // main_screen_ams_b_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_screen_ams_b_label = obj;
                            lv_obj_set_pos(obj, 31, -19);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "B");
                        }
                        {
                            // main_screen_ams_b_slot_1
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_screen_ams_b_slot_1 = obj;
                            lv_obj_set_pos(obj, -17, -3);
                            lv_obj_set_size(obj, 23, 24);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffec0a0a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // main_screen_ams_b_slot_2
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_screen_ams_b_slot_2 = obj;
                            lv_obj_set_pos(obj, 11, -3);
                            lv_obj_set_size(obj, 23, 24);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0a40ec), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // main_screen_ams_b_slot_3
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_screen_ams_b_slot_3 = obj;
                            lv_obj_set_pos(obj, 39, -3);
                            lv_obj_set_size(obj, 23, 24);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffece90a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // main_screen_ams_b_slot_4
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_screen_ams_b_slot_4 = obj;
                            lv_obj_set_pos(obj, 68, -3);
                            lv_obj_set_size(obj, 23, 24);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff146819), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                    }
                }
            }
        }
        {
            // main_screen_button_ams_setup
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.main_screen_button_ams_setup = obj;
            lv_obj_set_pos(obj, 507, 49);
            lv_obj_set_size(obj, 137, 122);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff796666), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // main_screen_button_ams_setup_icon
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.main_screen_button_ams_setup_icon = obj;
                    lv_obj_set_pos(obj, 2, 2);
                    lv_obj_set_size(obj, 93, 79);
                    lv_image_set_src(obj, &img_amssetup);
                    lv_image_set_scale(obj, 180);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_HOR, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff5f5b5b), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_stop(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_outline_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_outline_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_outline_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // main_screen_button_ams_setup_label
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_screen_button_ams_setup_label = obj;
                    lv_obj_set_pos(obj, 2, 49);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "AMS Setup");
                }
            }
        }
        {
            // main_screen_button_encode_tag
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.main_screen_button_encode_tag = obj;
            lv_obj_set_pos(obj, 657, 49);
            lv_obj_set_size(obj, 130, 122);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff796666), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // main_screen_button_encode_tag_icon
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.main_screen_button_encode_tag_icon = obj;
                    lv_obj_set_pos(obj, -1, 2);
                    lv_obj_set_size(obj, 93, 79);
                    lv_image_set_src(obj, &img_encoding);
                    lv_image_set_scale(obj, 150);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_HOR, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff5f5b5b), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_stop(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_outline_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_outline_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_outline_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // main_screen_button_encode_tag_label
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_screen_button_encode_tag_label = obj;
                    lv_obj_set_pos(obj, 0, 49);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Encode Tag");
                }
            }
        }
        {
            // main_screen_button_settings
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.main_screen_button_settings = obj;
            lv_obj_set_pos(obj, 657, 182);
            lv_obj_set_size(obj, 130, 126);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff796666), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // main_screen_button_settings_icon
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.main_screen_button_settings_icon = obj;
                    lv_obj_set_pos(obj, -1, 2);
                    lv_obj_set_size(obj, 93, 83);
                    lv_image_set_src(obj, &img_settings);
                    lv_image_set_scale(obj, 150);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_HOR, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff5f5b5b), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_stop(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_outline_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_outline_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_outline_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // main_screen_button_settings_label
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_screen_button_settings_label = obj;
                    lv_obj_set_pos(obj, 0, 50);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Settings");
                }
            }
        }
        {
            // main_screen_button_catalog
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.main_screen_button_catalog = obj;
            lv_obj_set_pos(obj, 507, 180);
            lv_obj_set_size(obj, 137, 129);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff796666), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // main_screen_button_catalog_icon
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.main_screen_button_catalog_icon = obj;
                    lv_obj_set_pos(obj, 2, 2);
                    lv_obj_set_size(obj, 93, 83);
                    lv_image_set_src(obj, &img_catalog);
                    lv_image_set_scale(obj, 150);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_HOR, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff5f5b5b), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_stop(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_outline_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_outline_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_outline_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // main_screen_button_catalog_label
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_screen_button_catalog_label = obj;
                    lv_obj_set_pos(obj, 2, 50);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Catalog");
                }
            }
        }
        {
            // main_screen_nfc_scale
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.main_screen_nfc_scale = obj;
            lv_obj_set_pos(obj, 11, 179);
            lv_obj_set_size(obj, 483, 130);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff796666), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // main_screen_nfc_scale_nfc_logo
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.main_screen_nfc_scale_nfc_logo = obj;
                    lv_obj_set_pos(obj, -17, -14);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_image_set_src(obj, &img_nfc);
                    lv_image_set_scale(obj, 175);
                    lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // main_screen_nfc_scale_nfc_label
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_screen_nfc_scale_nfc_label = obj;
                    lv_obj_set_pos(obj, 7, 78);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 125, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Ready");
                }
                {
                    // main_screen_nfc_scale_scale_icon
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.main_screen_nfc_scale_scale_icon = obj;
                    lv_obj_set_pos(obj, 369, -18);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_image_set_src(obj, &img_scale);
                    lv_image_set_scale(obj, 190);
                    lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // main_screen_nfc_scale_scale_label
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_screen_nfc_scale_scale_label = obj;
                    lv_obj_set_pos(obj, 382, 76);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 125, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Ready");
                }
                {
                    // main_screen_nfc_scale_messages
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.main_screen_nfc_scale_messages = obj;
                    lv_obj_set_pos(obj, 83, -8);
                    lv_obj_set_size(obj, 276, 102);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_width(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_rounded(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff3d3d3d), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // main_screen_nfc_scale_message
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_screen_nfc_scale_message = obj;
                            lv_obj_set_pos(obj, 41, 13);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff808080), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Place spool on scale\nto scan & weigh...");
                        }
                    }
                }
            }
        }
        {
            // main_screen_ams_left_nozzle
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.main_screen_ams_left_nozzle = obj;
            lv_obj_set_pos(obj, 10, 319);
            lv_obj_set_size(obj, 385, 127);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff796666), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // main_screen_ams_left_nozzle_indicator
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_screen_ams_left_nozzle_indicator = obj;
                    lv_obj_set_pos(obj, -16, -17);
                    lv_obj_set_size(obj, 12, 12);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "L");
                }
                {
                    // main_screen_ams_left_nozzle_label
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_screen_ams_left_nozzle_label = obj;
                    lv_obj_set_pos(obj, 0, -17);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, 12);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Left Nozzle");
                }
                {
                    // main_screen_ams_a
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.main_screen_ams_a = obj;
                    lv_obj_set_pos(obj, -16, -2);
                    lv_obj_set_size(obj, 120, 50);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_arc_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_rounded(obj, false, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // main_screen_ams_a_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_screen_ams_a_label = obj;
                            lv_obj_set_pos(obj, 32, -18);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text(obj, "A");
                        }
                        {
                            // main_screen_ams_a_slot_1
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_screen_ams_a_slot_1 = obj;
                            lv_obj_set_pos(obj, -17, -3);
                            lv_obj_set_size(obj, 23, 24);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffec0a0a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // main_screen_ams_a_slot_2
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_screen_ams_a_slot_2 = obj;
                            lv_obj_set_pos(obj, 11, -3);
                            lv_obj_set_size(obj, 23, 24);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0a40ec), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // main_screen_ams_a_slot_3
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_screen_ams_a_slot_3 = obj;
                            lv_obj_set_pos(obj, 39, -3);
                            lv_obj_set_size(obj, 23, 24);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffece90a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // main_screen_ams_a_slot_4
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_screen_ams_a_slot_4 = obj;
                            lv_obj_set_pos(obj, 68, -3);
                            lv_obj_set_size(obj, 23, 24);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff146819), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                    }
                }
                {
                    // main_screen_ams_c
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.main_screen_ams_c = obj;
                    lv_obj_set_pos(obj, 111, -2);
                    lv_obj_set_size(obj, 120, 50);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_arc_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_rounded(obj, false, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff2f3237), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // main_screen_ams_c_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_screen_ams_c_label = obj;
                            lv_obj_set_pos(obj, 32, -18);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "C");
                        }
                        {
                            // main_screen_ams_c_slot_1
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_screen_ams_c_slot_1 = obj;
                            lv_obj_set_pos(obj, -17, -3);
                            lv_obj_set_size(obj, 23, 24);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffec0a0a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // main_screen_ams_c_slot_2
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_screen_ams_c_slot_2 = obj;
                            lv_obj_set_pos(obj, 11, -3);
                            lv_obj_set_size(obj, 23, 24);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0a40ec), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // main_screen_ams_c_slot_3
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_screen_ams_c_slot_3 = obj;
                            lv_obj_set_pos(obj, 39, -3);
                            lv_obj_set_size(obj, 23, 24);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffece90a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // main_screen_ams_c_slot_4
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_screen_ams_c_slot_4 = obj;
                            lv_obj_set_pos(obj, 68, -3);
                            lv_obj_set_size(obj, 23, 24);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff146819), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                    }
                }
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.obj0 = obj;
                    lv_obj_set_pos(obj, 240, -2);
                    lv_obj_set_size(obj, 120, 50);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_arc_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_rounded(obj, false, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // main_screen_ams_d_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_screen_ams_d_label = obj;
                            lv_obj_set_pos(obj, 31, -18);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "D");
                        }
                        {
                            // main_screen_ams_d_slot_1
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_screen_ams_d_slot_1 = obj;
                            lv_obj_set_pos(obj, -17, -3);
                            lv_obj_set_size(obj, 23, 24);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffec0a0a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // main_screen_ams_d_slot_2
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_screen_ams_d_slot_2 = obj;
                            lv_obj_set_pos(obj, 11, -3);
                            lv_obj_set_size(obj, 23, 24);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0a40ec), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // main_screen_ams_d_slot_3
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_screen_ams_d_slot_3 = obj;
                            lv_obj_set_pos(obj, 39, -3);
                            lv_obj_set_size(obj, 23, 24);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffece90a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // main_screen_ams_d_slot_4
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_screen_ams_d_slot_4 = obj;
                            lv_obj_set_pos(obj, 68, -3);
                            lv_obj_set_size(obj, 23, 24);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff146819), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                    }
                }
                {
                    // main_screen_ht_b
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.main_screen_ht_b = obj;
                    lv_obj_set_pos(obj, -16, 50);
                    lv_obj_set_size(obj, 47, 50);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_arc_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_rounded(obj, false, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // main_screen_ht_b_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_screen_ht_b_label = obj;
                            lv_obj_set_pos(obj, -14, -17);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "HT-B");
                        }
                        {
                            // main_screen_ht_b_slot
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_screen_ht_b_slot = obj;
                            lv_obj_set_pos(obj, -10, -1);
                            lv_obj_set_size(obj, 23, 24);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff726e6e), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_grad_stop(obj, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_main_stop(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff352a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                    }
                }
                {
                    // main_screen_ext_2
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.main_screen_ext_2 = obj;
                    lv_obj_set_pos(obj, 38, 50);
                    lv_obj_set_size(obj, 47, 50);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_arc_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_rounded(obj, false, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // main_screen_ext_2_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_screen_ext_2_label = obj;
                            lv_obj_set_pos(obj, -14, -17);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Ext-2");
                        }
                        {
                            // main_screen_ext_2_slot
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_screen_ext_2_slot = obj;
                            lv_obj_set_pos(obj, -11, -1);
                            lv_obj_set_size(obj, 23, 24);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff726e6e), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff352a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_grad_stop(obj, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_main_stop(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                    }
                }
            }
        }
        {
            // main_screen_printer
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.main_screen_printer = obj;
            lv_obj_set_pos(obj, 11, 49);
            lv_obj_set_size(obj, 484, 122);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff796666), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff2f3237), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // main_screen_printer_print_cover
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.main_screen_printer_print_cover = obj;
                    lv_obj_set_pos(obj, -11, -13);
                    lv_obj_set_size(obj, 70, 70);
                    lv_image_set_src(obj, &img_filament_spool);
                    lv_image_set_scale(obj, 100);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff3d3d3d), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_opa(obj, 128, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // main_screen_printer_printer_name_label
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_screen_printer_printer_name_label = obj;
                    lv_obj_set_pos(obj, 70, -6);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
                {
                    // main_screen_printer_printer_status
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_screen_printer_printer_status = obj;
                    lv_obj_set_pos(obj, 70, 27);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
                {
                    // main_screen_printer_filename
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_screen_printer_filename = obj;
                    lv_obj_set_pos(obj, -13, 62);
                    lv_obj_set_size(obj, 353, 16);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
                {
                    // main_screen_printer_eta
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_screen_printer_eta = obj;
                    lv_obj_set_pos(obj, 385, 35);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
                {
                    // main_screen_printer_progress_bar
                    lv_obj_t *obj = lv_bar_create(parent_obj);
                    objects.main_screen_printer_progress_bar = obj;
                    lv_obj_set_pos(obj, -17, 80);
                    lv_obj_set_size(obj, 467, 15);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00ff00), LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                }
                {
                    // main_screen_printer_time_left
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_screen_printer_time_left = obj;
                    lv_obj_set_pos(obj, 385, 62);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
    }
    
    tick_screen_main_screen();
}

void tick_screen_main_screen() {
}

void create_screen_ams_overview() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.ams_overview = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 800, 480);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // ams_screen_top_bar
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.ams_screen_top_bar = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 800, 44);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3d3d3d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // ams_screen_top_bar_logo
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.ams_screen_top_bar_logo = obj;
                    lv_obj_set_pos(obj, -8, -1);
                    lv_obj_set_size(obj, 173, 43);
                    lv_image_set_src(obj, &img_spoolbuddy_logo_dark);
                    lv_image_set_scale(obj, 200);
                    lv_obj_set_style_align(obj, LV_ALIGN_TOP_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // ams_screen_top_bar_printer_select
                    lv_obj_t *obj = lv_dropdown_create(parent_obj);
                    objects.ams_screen_top_bar_printer_select = obj;
                    lv_obj_set_pos(obj, 325, 2);
                    lv_obj_set_size(obj, 150, 39);
                    lv_dropdown_set_options(obj, "");
                    lv_dropdown_set_selected(obj, 0);
                }
                {
                    // ams_screen_top_bar_wifi_signal
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.ams_screen_top_bar_wifi_signal = obj;
                    lv_obj_set_pos(obj, 698, 10);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_image_set_src(obj, &img_signal);
                    lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // ams_screen_top_bar_notofication_bell
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.ams_screen_top_bar_notofication_bell = obj;
                    lv_obj_set_pos(obj, 662, 11);
                    lv_obj_set_size(obj, 24, 24);
                    lv_image_set_src(obj, &img_bell);
                    lv_image_set_scale(obj, 50);
                }
                {
                    // ams_screen_top_bar_clock
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.ams_screen_top_bar_clock = obj;
                    lv_obj_set_pos(obj, 737, 12);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
        {
            // ams_screen_bottom_bar
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.ams_screen_bottom_bar = obj;
            lv_obj_set_pos(obj, 0, 450);
            lv_obj_set_size(obj, 800, 30);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xfffaaa05), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_TOP, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // ams_screen_bottom_bar_led
                    lv_obj_t *obj = lv_led_create(parent_obj);
                    objects.ams_screen_bottom_bar_led = obj;
                    lv_obj_set_pos(obj, 13, 7);
                    lv_obj_set_size(obj, 12, 12);
                    lv_led_set_color(obj, lv_color_hex(0xfffaaa05));
                    lv_led_set_brightness(obj, 0);
                }
                {
                    // ams_screen_bottom_bar_message
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.ams_screen_bottom_bar_message = obj;
                    lv_obj_set_pos(obj, 30, 5);
                    lv_obj_set_size(obj, 696, 16);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
        {
            // ams_screen_button_home
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.ams_screen_button_home = obj;
            lv_obj_set_pos(obj, 728, 49);
            lv_obj_set_size(obj, 60, 60);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff796666), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // ams_screen_button_home_image
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.ams_screen_button_home_image = obj;
                    lv_obj_set_pos(obj, -15, -6);
                    lv_obj_set_size(obj, 50, 40);
                    lv_image_set_src(obj, &img_home);
                    lv_image_set_scale(obj, 100);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_HOR, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff5f5b5b), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_stop(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_outline_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_outline_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_outline_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // ams_screen_button_home_label
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.ams_screen_button_home_label = obj;
                    lv_obj_set_pos(obj, 0, 23);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Home");
                }
            }
        }
        {
            // ams_screen_button_encode_tag
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.ams_screen_button_encode_tag = obj;
            lv_obj_set_pos(obj, 728, 116);
            lv_obj_set_size(obj, 60, 60);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff796666), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // ams_screen_button_encode_tag_icon
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.ams_screen_button_encode_tag_icon = obj;
                    lv_obj_set_pos(obj, -15, -6);
                    lv_obj_set_size(obj, 50, 40);
                    lv_image_set_src(obj, &img_encoding);
                    lv_image_set_scale(obj, 100);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_HOR, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff5f5b5b), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_stop(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_outline_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_outline_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_outline_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // ams_screen_button_encode_tag_label
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.ams_screen_button_encode_tag_label = obj;
                    lv_obj_set_pos(obj, 0, 23);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Encode");
                }
            }
        }
        {
            // ams_screen_button_settings
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.ams_screen_button_settings = obj;
            lv_obj_set_pos(obj, 729, 249);
            lv_obj_set_size(obj, 60, 60);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff796666), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // ams_screen_button_settings_icon
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.ams_screen_button_settings_icon = obj;
                    lv_obj_set_pos(obj, -15, -6);
                    lv_obj_set_size(obj, 50, 40);
                    lv_image_set_src(obj, &img_settings);
                    lv_image_set_scale(obj, 110);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_HOR, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff5f5b5b), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_stop(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_outline_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_outline_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_outline_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // ams_screen_button_settings_label
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.ams_screen_button_settings_label = obj;
                    lv_obj_set_pos(obj, 0, 23);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Settings");
                }
            }
        }
        {
            // ams_screen_button_catalog
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.ams_screen_button_catalog = obj;
            lv_obj_set_pos(obj, 729, 182);
            lv_obj_set_size(obj, 60, 60);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff796666), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // ams_screen_button_catalog_icon
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.ams_screen_button_catalog_icon = obj;
                    lv_obj_set_pos(obj, -15, -6);
                    lv_obj_set_size(obj, 50, 40);
                    lv_image_set_src(obj, &img_catalog);
                    lv_image_set_scale(obj, 100);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_HOR, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff5f5b5b), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_stop(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_outline_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_outline_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_outline_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // ams_screen_button_catalog_label
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.ams_screen_button_catalog_label = obj;
                    lv_obj_set_pos(obj, 0, 23);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Catalog");
                }
            }
        }
        {
            // ams_screen_ams_panel
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.ams_screen_ams_panel = obj;
            lv_obj_set_pos(obj, 10, 49);
            lv_obj_set_size(obj, 712, 393);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3d3d3d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // ams_screen_ams_panel_label
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.ams_screen_ams_panel_label = obj;
                    lv_obj_set_pos(obj, -14, -17);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "AMS Units");
                }
                {
                    // ams_screen_ams_panel_amd_d
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.ams_screen_ams_panel_amd_d = obj;
                    lv_obj_set_pos(obj, -14, 185);
                    lv_obj_set_size(obj, 225, 175);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff545151), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_stop(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_main_opa(obj, 128, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_opa(obj, 128, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff3d3d3d), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // ams_screen_ams_panel_amd_d_indicator
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_amd_d_indicator = obj;
                            lv_obj_set_pos(obj, -16, -16);
                            lv_obj_set_size(obj, 12, 12);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, " ");
                        }
                        {
                            // ams_screen_ams_panel_amd_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_amd_label = obj;
                            lv_obj_set_pos(obj, 1, -15);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "AMS D");
                        }
                        {
                            // ams_screen_ams_panel_amd_d_labe-Humidity
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_amd_d_labe_humidity = obj;
                            lv_obj_set_pos(obj, 170, -14);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_amd_d_icon_humidity
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_amd_d_icon_humidity = obj;
                            lv_obj_set_pos(obj, 116, -16);
                            lv_obj_set_size(obj, 21, 14);
                            lv_image_set_src(obj, &img_humidity);
                            lv_image_set_scale(obj, 60);
                        }
                        {
                            // ams_screen_ams_panel_amd_d_label_humidity
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_amd_d_label_humidity = obj;
                            lv_obj_set_pos(obj, 133, -14);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_amd_d_slot_1
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_amd_d_slot_1 = obj;
                            lv_obj_set_pos(obj, -6, 47);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_clean);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_amd_d_slot_1_color
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_amd_d_slot_1_color = obj;
                            lv_obj_set_pos(obj, -6, 47);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_fill);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xfff70303), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_amd_d_slot_2
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_amd_d_slot_2 = obj;
                            lv_obj_set_pos(obj, 46, 48);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_clean);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_amd_d_slot_2_color
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_amd_d_slot_2_color = obj;
                            lv_obj_set_pos(obj, 46, 48);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_fill);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff3603f7), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_amd_d_slot_3
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_amd_d_slot_3 = obj;
                            lv_obj_set_pos(obj, 100, 48);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_clean);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_amd_d_slot_3_color
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_amd_d_slot_3_color = obj;
                            lv_obj_set_pos(obj, 100, 48);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_fill);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff509405), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_amd_d_slot_4
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_amd_d_slot_4 = obj;
                            lv_obj_set_pos(obj, 155, 48);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_clean);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_amd_d_slot_4_color
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_amd_d_slot_4_color = obj;
                            lv_obj_set_pos(obj, 155, 48);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_fill);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xfffad607), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_amd_d_slot_1_label_material
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_amd_d_slot_1_label_material = obj;
                            lv_obj_set_pos(obj, 0, 20);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_amd_d_slot_2_label_material
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_amd_d_slot_2_label_material = obj;
                            lv_obj_set_pos(obj, 52, 20);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_amd_d_slot_3_label_material
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_amd_d_slot_3_label_material = obj;
                            lv_obj_set_pos(obj, 105, 20);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_amd_d_slot_4_label_material
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_amd_d_slot_4_label_material = obj;
                            lv_obj_set_pos(obj, 157, 20);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_amd_d_slot_2_label_slotname
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_amd_d_slot_2_label_slotname = obj;
                            lv_obj_set_pos(obj, 55, 106);
                            lv_obj_set_size(obj, 18, 11);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "D2");
                        }
                        {
                            // ams_screen_ams_panel_amd_d_slot_3_label_slotname
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_amd_d_slot_3_label_slotname = obj;
                            lv_obj_set_pos(obj, 108, 106);
                            lv_obj_set_size(obj, 18, 11);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "D3");
                        }
                        {
                            // ams_screen_ams_panel_amd_d_slot_4_label_slotname
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_amd_d_slot_4_label_slotname = obj;
                            lv_obj_set_pos(obj, 162, 106);
                            lv_obj_set_size(obj, 18, 11);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "D4");
                        }
                        {
                            // ams_screen_ams_panel_amd_d_slot_1_label_fill_level
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_amd_d_slot_1_label_fill_level = obj;
                            lv_obj_set_pos(obj, 0, 123);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_amd_d_slot_2_label_fill_level
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_amd_d_slot_2_label_fill_level = obj;
                            lv_obj_set_pos(obj, 54, 123);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_amd_d_slot_3_label_fill_level
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_amd_d_slot_3_label_fill_level = obj;
                            lv_obj_set_pos(obj, 107, 123);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_amd_d_slot_4_label_fill_level
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_amd_d_slot_4_label_fill_level = obj;
                            lv_obj_set_pos(obj, 161, 123);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_amd_d_slot_1_label_slotname
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_amd_d_slot_1_label_slotname = obj;
                            lv_obj_set_pos(obj, 2, 107);
                            lv_obj_set_size(obj, 18, 11);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "D1");
                        }
                        {
                            // ams_screen_ams_panel_amd_d_icon_thermometer
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_amd_d_icon_thermometer = obj;
                            lv_obj_set_pos(obj, 155, -17);
                            lv_obj_set_size(obj, 21, 14);
                            lv_image_set_src(obj, &img_thermometer);
                            lv_image_set_scale(obj, 95);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff1967ea), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                    }
                }
                {
                    // ams_screen_ams_panel_ams_a
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.ams_screen_ams_panel_ams_a = obj;
                    lv_obj_set_pos(obj, -16, 3);
                    lv_obj_set_size(obj, 225, 175);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff545151), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_stop(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_main_opa(obj, 128, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_opa(obj, 128, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff3d3d3d), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // ams_screen_ams_panel_ams_a_indicator
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_a_indicator = obj;
                            lv_obj_set_pos(obj, -16, -16);
                            lv_obj_set_size(obj, 12, 12);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, " ");
                        }
                        {
                            // ams_screen_ams_panel_ams_a_label_name
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_a_label_name = obj;
                            lv_obj_set_pos(obj, 1, -15);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "AMS A");
                        }
                        {
                            // ams_screen_ams_panel_ams_a_label_temperature
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_a_label_temperature = obj;
                            lv_obj_set_pos(obj, 170, -14);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ams_a_icon_humidity
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_a_icon_humidity = obj;
                            lv_obj_set_pos(obj, 116, -16);
                            lv_obj_set_size(obj, 21, 14);
                            lv_image_set_src(obj, &img_humidity);
                            lv_image_set_scale(obj, 60);
                        }
                        {
                            // ams_screen_ams_panel_ams_a_label_humidity
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_a_label_humidity = obj;
                            lv_obj_set_pos(obj, 133, -14);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ams_a_icon_thermometer
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_a_icon_thermometer = obj;
                            lv_obj_set_pos(obj, 155, -17);
                            lv_obj_set_size(obj, 21, 14);
                            lv_image_set_src(obj, &img_thermometer);
                            lv_image_set_scale(obj, 95);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff1967ea), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_ams_a_slot_1
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_a_slot_1 = obj;
                            lv_obj_set_pos(obj, -6, 47);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_clean);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_ams_a_slot_1_color
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_a_slot_1_color = obj;
                            lv_obj_set_pos(obj, -6, 47);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_fill);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xfff70303), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_ams_a_slot_2
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_a_slot_2 = obj;
                            lv_obj_set_pos(obj, 46, 48);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_clean);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_ams_a_slot_2_color
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_a_slot_2_color = obj;
                            lv_obj_set_pos(obj, 46, 48);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_fill);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff3603f7), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_ams_a_slot_3
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_a_slot_3 = obj;
                            lv_obj_set_pos(obj, 100, 48);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_clean);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_ams_a_slot_3_color
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_a_slot_3_color = obj;
                            lv_obj_set_pos(obj, 100, 48);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_fill);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff509405), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_ams_a_slot_4
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_a_slot_4 = obj;
                            lv_obj_set_pos(obj, 155, 49);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_clean);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_ams_a_slot_4_color
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_a_slot_4_color = obj;
                            lv_obj_set_pos(obj, 155, 49);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_fill);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xfffad607), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_ams_a_slot_1_label_material
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_a_slot_1_label_material = obj;
                            lv_obj_set_pos(obj, 0, 20);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ams_a_slot_2_label_material
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_a_slot_2_label_material = obj;
                            lv_obj_set_pos(obj, 52, 20);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ams_a_slot_3_label_material
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_a_slot_3_label_material = obj;
                            lv_obj_set_pos(obj, 105, 20);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ams_a_slot_4_label_material
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_a_slot_4_label_material = obj;
                            lv_obj_set_pos(obj, 157, 20);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ams_a_slot_2_label_slot_name
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_a_slot_2_label_slot_name = obj;
                            lv_obj_set_pos(obj, 55, 106);
                            lv_obj_set_size(obj, 18, 11);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "A2");
                        }
                        {
                            // ams_screen_ams_panel_ams_a_slot_3_label_slot_name
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_a_slot_3_label_slot_name = obj;
                            lv_obj_set_pos(obj, 108, 106);
                            lv_obj_set_size(obj, 18, 11);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "A3");
                        }
                        {
                            // ams_screen_ams_panel_ams_a_slot_4_label_slot_name
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_a_slot_4_label_slot_name = obj;
                            lv_obj_set_pos(obj, 162, 106);
                            lv_obj_set_size(obj, 18, 11);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "A4");
                        }
                        {
                            // ams_screen_ams_panel_ams_a_slot_1_label_slot_name_label_fill_level
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_a_slot_1_label_slot_name_label_fill_level = obj;
                            lv_obj_set_pos(obj, 0, 123);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ams_a_slot_2_label_slot_name_label_fill_level
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_a_slot_2_label_slot_name_label_fill_level = obj;
                            lv_obj_set_pos(obj, 54, 123);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ams_a_slot_3_label_slot_name_label_fill_level
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_a_slot_3_label_slot_name_label_fill_level = obj;
                            lv_obj_set_pos(obj, 107, 123);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ams_a_slot_4_label_slot_name_label_fill_level
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_a_slot_4_label_slot_name_label_fill_level = obj;
                            lv_obj_set_pos(obj, 161, 123);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ams_a_slot_1_label_slot_name
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_a_slot_1_label_slot_name = obj;
                            lv_obj_set_pos(obj, 1, 105);
                            lv_obj_set_size(obj, 18, 11);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "A1");
                        }
                    }
                }
                {
                    // ams_screen_ams_panel_ht_a
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.ams_screen_ams_panel_ht_a = obj;
                    lv_obj_set_pos(obj, 219, 185);
                    lv_obj_set_size(obj, 108, 175);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff545151), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_stop(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_main_opa(obj, 128, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_opa(obj, 128, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff3d3d3d), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // ams_screen_ams_panel_ht_a_indicator
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ht_a_indicator = obj;
                            lv_obj_set_pos(obj, -16, -16);
                            lv_obj_set_size(obj, 12, 12);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, " ");
                        }
                        {
                            // ams_screen_ams_panel_ht_a_label_name
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ht_a_label_name = obj;
                            lv_obj_set_pos(obj, 1, -15);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "HT-A");
                        }
                        {
                            // ams_screen_ams_panel_ht_a_label_temperature
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ht_a_label_temperature = obj;
                            lv_obj_set_pos(obj, 50, 136);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ht_a_con_humidity
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ht_a_con_humidity = obj;
                            lv_obj_set_pos(obj, -8, 134);
                            lv_obj_set_size(obj, 21, 14);
                            lv_image_set_src(obj, &img_humidity);
                            lv_image_set_scale(obj, 60);
                        }
                        {
                            // ams_screen_ams_panel_ht_a_label_humidity
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ht_a_label_humidity = obj;
                            lv_obj_set_pos(obj, 10, 136);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ht_a_label_material
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ht_a_label_material = obj;
                            lv_obj_set_pos(obj, 19, 20);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ht_a_label_fill_level
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ht_a_label_fill_level = obj;
                            lv_obj_set_pos(obj, 22, 107);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "85%");
                        }
                        {
                            // ams_screen_ams_panel_ht_a_slot
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ht_a_slot = obj;
                            lv_obj_set_pos(obj, 14, 47);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_clean);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_ht_a_slot_color
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ht_a_slot_color = obj;
                            lv_obj_set_pos(obj, 14, 47);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_fill);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xfff70303), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_ht_a_icon_thermometer
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ht_a_icon_thermometer = obj;
                            lv_obj_set_pos(obj, 33, 133);
                            lv_obj_set_size(obj, 21, 14);
                            lv_image_set_src(obj, &img_thermometer);
                            lv_image_set_scale(obj, 95);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff1967ea), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                    }
                }
                {
                    // ams_screen_ams_panel_ht_b
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.ams_screen_ams_panel_ht_b = obj;
                    lv_obj_set_pos(obj, 336, 185);
                    lv_obj_set_size(obj, 108, 175);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff545151), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_stop(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_main_opa(obj, 128, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_opa(obj, 128, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff3d3d3d), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // ams_screen_ams_panel_ht_b_indicator
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ht_b_indicator = obj;
                            lv_obj_set_pos(obj, -16, -16);
                            lv_obj_set_size(obj, 12, 12);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, " ");
                        }
                        {
                            // ams_screen_ams_panel_ht_b_label_name
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ht_b_label_name = obj;
                            lv_obj_set_pos(obj, 1, -15);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "HT-B");
                        }
                        {
                            // ams_screen_ams_panel_ht_b_label_temperature
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ht_b_label_temperature = obj;
                            lv_obj_set_pos(obj, 50, 136);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ht_b_icon_humidity
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ht_b_icon_humidity = obj;
                            lv_obj_set_pos(obj, -8, 134);
                            lv_obj_set_size(obj, 21, 14);
                            lv_image_set_src(obj, &img_humidity);
                            lv_image_set_scale(obj, 60);
                        }
                        {
                            // ams_screen_ams_panel_ht_b_label_humidity
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ht_b_label_humidity = obj;
                            lv_obj_set_pos(obj, 10, 136);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ht_b_label_material
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ht_b_label_material = obj;
                            lv_obj_set_pos(obj, 19, 20);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ht_b_label_fill_level
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ht_b_label_fill_level = obj;
                            lv_obj_set_pos(obj, 22, 107);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "85%");
                        }
                        {
                            // ams_screen_ams_panel_ht_b_slot
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ht_b_slot = obj;
                            lv_obj_set_pos(obj, 14, 47);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_clean);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_ht_b_slot_color
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ht_b_slot_color = obj;
                            lv_obj_set_pos(obj, 14, 47);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_fill);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xfff70303), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_ht_b_icon_t
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ht_b_icon_t = obj;
                            lv_obj_set_pos(obj, 33, 133);
                            lv_obj_set_size(obj, 21, 14);
                            lv_image_set_src(obj, &img_thermometer);
                            lv_image_set_scale(obj, 95);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff1967ea), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                    }
                }
                {
                    // ams_screen_ams_panel_ext_1
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.ams_screen_ams_panel_ext_1 = obj;
                    lv_obj_set_pos(obj, 454, 185);
                    lv_obj_set_size(obj, 108, 175);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff545151), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_stop(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_main_opa(obj, 128, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_opa(obj, 128, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff3d3d3d), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // ams_screen_ams_panel_ext_1_indicator
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ext_1_indicator = obj;
                            lv_obj_set_pos(obj, -16, -16);
                            lv_obj_set_size(obj, 12, 12);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, " ");
                        }
                        {
                            // ams_screen_ams_panel_ext_1_label_name
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ext_1_label_name = obj;
                            lv_obj_set_pos(obj, 1, -15);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "EXT-1");
                        }
                        {
                            // ams_screen_ams_panel_ext_1_label_empty
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ext_1_label_empty = obj;
                            lv_obj_set_pos(obj, 9, 12);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "<empty>");
                        }
                        {
                            // ams_screen_ams_panel_ext_1_icon_empty
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ext_1_icon_empty = obj;
                            lv_obj_set_pos(obj, 0, 41);
                            lv_obj_set_size(obj, 66, 55);
                            lv_image_set_src(obj, &img_circle_empty);
                            lv_image_set_scale(obj, 25);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                    }
                }
                {
                    // ams_screen_ams_panel_ext_2
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.ams_screen_ams_panel_ext_2 = obj;
                    lv_obj_set_pos(obj, 570, 185);
                    lv_obj_set_size(obj, 108, 175);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff545151), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_stop(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_main_opa(obj, 128, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_opa(obj, 128, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff3d3d3d), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // ams_screen_ams_panel_ext_2_indicator
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ext_2_indicator = obj;
                            lv_obj_set_pos(obj, -16, -16);
                            lv_obj_set_size(obj, 12, 12);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, " ");
                        }
                        {
                            // ams_screen_ams_panel_ext_2_label_name
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ext_2_label_name = obj;
                            lv_obj_set_pos(obj, 1, -15);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "EXT-2");
                        }
                        {
                            // ams_screen_ams_panel_ext_2_label_empty
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ext_2_label_empty = obj;
                            lv_obj_set_pos(obj, 9, 12);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "<empty>");
                        }
                        {
                            // ams_screen_ams_panel_ext_2_icon_empty
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ext_2_icon_empty = obj;
                            lv_obj_set_pos(obj, -1, 41);
                            lv_obj_set_size(obj, 66, 55);
                            lv_image_set_src(obj, &img_circle_empty);
                            lv_image_set_scale(obj, 25);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                    }
                }
                {
                    // ams_screen_ams_panel_ams_b
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.ams_screen_ams_panel_ams_b = obj;
                    lv_obj_set_pos(obj, 219, 3);
                    lv_obj_set_size(obj, 225, 175);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff545151), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_stop(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_main_opa(obj, 128, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_opa(obj, 128, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff3d3d3d), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // ams_screen_ams_panel_ams_b_indicator
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_b_indicator = obj;
                            lv_obj_set_pos(obj, -16, -16);
                            lv_obj_set_size(obj, 12, 12);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, " ");
                        }
                        {
                            // ams_screen_ams_panel_ams_b_label_name
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_b_label_name = obj;
                            lv_obj_set_pos(obj, 1, -15);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "AMS B");
                        }
                        {
                            // ams_screen_ams_panel_ams_b_label_temperature
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_b_label_temperature = obj;
                            lv_obj_set_pos(obj, 170, -14);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ams_b_icon_humidity
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_b_icon_humidity = obj;
                            lv_obj_set_pos(obj, 116, -16);
                            lv_obj_set_size(obj, 21, 14);
                            lv_image_set_src(obj, &img_humidity);
                            lv_image_set_scale(obj, 60);
                        }
                        {
                            // ams_screen_ams_panel_ams_b_labe_humidity
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_b_labe_humidity = obj;
                            lv_obj_set_pos(obj, 133, -14);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ams_b_slot_1
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_b_slot_1 = obj;
                            lv_obj_set_pos(obj, -6, 47);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_clean);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_ams_b_slot_1_color
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_b_slot_1_color = obj;
                            lv_obj_set_pos(obj, -6, 47);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_fill);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xfff70303), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_ams_b_slot_2
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_b_slot_2 = obj;
                            lv_obj_set_pos(obj, 46, 48);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_clean);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_ams_b_slot_2_color
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_b_slot_2_color = obj;
                            lv_obj_set_pos(obj, 46, 48);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_fill);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff3603f7), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_ams_b_slot_3
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_b_slot_3 = obj;
                            lv_obj_set_pos(obj, 100, 48);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_clean);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_ams_b_slot_3_color
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_b_slot_3_color = obj;
                            lv_obj_set_pos(obj, 100, 48);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_fill);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff509405), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_ams_b_slot_4
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_b_slot_4 = obj;
                            lv_obj_set_pos(obj, 155, 49);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_clean);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_ams_b_slot_4_color
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_b_slot_4_color = obj;
                            lv_obj_set_pos(obj, 155, 49);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_fill);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xfffad607), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_ams_b_slot_1_label_material
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_b_slot_1_label_material = obj;
                            lv_obj_set_pos(obj, 0, 20);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ams_b_slot_2_label_material
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_b_slot_2_label_material = obj;
                            lv_obj_set_pos(obj, 52, 20);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ams_b_slot_3_label_material
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_b_slot_3_label_material = obj;
                            lv_obj_set_pos(obj, 105, 20);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ams_b_slot_4_label_material
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_b_slot_4_label_material = obj;
                            lv_obj_set_pos(obj, 157, 20);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ams_b_slot_1_label_slot_name
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_b_slot_1_label_slot_name = obj;
                            lv_obj_set_pos(obj, 2, 107);
                            lv_obj_set_size(obj, 18, 11);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "B1");
                        }
                        {
                            // ams_screen_ams_panel_ams_b_slot_2_label_slot_name
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_b_slot_2_label_slot_name = obj;
                            lv_obj_set_pos(obj, 55, 106);
                            lv_obj_set_size(obj, 18, 11);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "B2");
                        }
                        {
                            // ams_screen_ams_panel_ams_b_slot_3_label_slot_name
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_b_slot_3_label_slot_name = obj;
                            lv_obj_set_pos(obj, 108, 106);
                            lv_obj_set_size(obj, 18, 11);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "B3");
                        }
                        {
                            // ams_screen_ams_panel_ams_b_slot_4_label_slot_name
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_b_slot_4_label_slot_name = obj;
                            lv_obj_set_pos(obj, 162, 106);
                            lv_obj_set_size(obj, 18, 11);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "B4");
                        }
                        {
                            // ams_screen_ams_panel_ams_b_slot_1_label_fill_level
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_b_slot_1_label_fill_level = obj;
                            lv_obj_set_pos(obj, 0, 123);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ams_b_slot_2_label_fill_level
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_b_slot_2_label_fill_level = obj;
                            lv_obj_set_pos(obj, 54, 123);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ams_b_slot_3_label_fill_level
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_b_slot_3_label_fill_level = obj;
                            lv_obj_set_pos(obj, 107, 123);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ams_b_slot_4_label_fill_level
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_b_slot_4_label_fill_level = obj;
                            lv_obj_set_pos(obj, 161, 123);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ams_b_icon_temperature
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_b_icon_temperature = obj;
                            lv_obj_set_pos(obj, 155, -17);
                            lv_obj_set_size(obj, 21, 14);
                            lv_image_set_src(obj, &img_thermometer);
                            lv_image_set_scale(obj, 95);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff1967ea), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                    }
                }
                {
                    // ams_screen_ams_panel_ams_c
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.ams_screen_ams_panel_ams_c = obj;
                    lv_obj_set_pos(obj, 454, 3);
                    lv_obj_set_size(obj, 225, 175);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff545151), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_stop(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_main_opa(obj, 128, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_grad_opa(obj, 128, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff3d3d3d), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // ams_screen_ams_panel_ams_c_indicator
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_c_indicator = obj;
                            lv_obj_set_pos(obj, -16, -16);
                            lv_obj_set_size(obj, 12, 12);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, " ");
                        }
                        {
                            // ams_screen_ams_panel_ams_c_label_name
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_c_label_name = obj;
                            lv_obj_set_pos(obj, 1, -15);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "AMS C");
                        }
                        {
                            // ams_screen_ams_panel_ams_c_label_temperature
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_c_label_temperature = obj;
                            lv_obj_set_pos(obj, 170, -14);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ams_c_icon_humidity
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_c_icon_humidity = obj;
                            lv_obj_set_pos(obj, 116, -16);
                            lv_obj_set_size(obj, 21, 14);
                            lv_image_set_src(obj, &img_humidity);
                            lv_image_set_scale(obj, 60);
                        }
                        {
                            // ams_screen_ams_panel_ams_c_label_humidity
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_c_label_humidity = obj;
                            lv_obj_set_pos(obj, 133, -14);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ams_c_slot_1
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_c_slot_1 = obj;
                            lv_obj_set_pos(obj, -6, 47);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_clean);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_ams_c_slot_1_color
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_c_slot_1_color = obj;
                            lv_obj_set_pos(obj, -6, 47);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_fill);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xfff70303), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_ams_c_slot_2
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_c_slot_2 = obj;
                            lv_obj_set_pos(obj, 46, 48);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_clean);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_ams_c_slot_2_color
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_c_slot_2_color = obj;
                            lv_obj_set_pos(obj, 46, 48);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_fill);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff3603f7), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_ams_c_slot_3
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_c_slot_3 = obj;
                            lv_obj_set_pos(obj, 100, 48);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_clean);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_ams_c_slot_3_color
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_c_slot_3_color = obj;
                            lv_obj_set_pos(obj, 100, 48);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_fill);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff509405), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_ams_c_slot_4
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_c_slot_4 = obj;
                            lv_obj_set_pos(obj, 155, 49);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_clean);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_ams_c_slot_4_color
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_c_slot_4_color = obj;
                            lv_obj_set_pos(obj, 155, 49);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_fill);
                            lv_image_set_scale(obj, 400);
                            lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xfffad607), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // ams_screen_ams_panel_ams_c_slot_1_label_material
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_c_slot_1_label_material = obj;
                            lv_obj_set_pos(obj, 0, 20);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ams_c_slot_2_label_material
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_c_slot_2_label_material = obj;
                            lv_obj_set_pos(obj, 52, 20);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ams_c_slot_3_label_material
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_c_slot_3_label_material = obj;
                            lv_obj_set_pos(obj, 105, 20);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ams_c_slot_4_label_material
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_c_slot_4_label_material = obj;
                            lv_obj_set_pos(obj, 157, 20);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ams_c_slot_2_label_slot_name
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_c_slot_2_label_slot_name = obj;
                            lv_obj_set_pos(obj, 55, 106);
                            lv_obj_set_size(obj, 18, 11);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "C2");
                        }
                        {
                            // ams_screen_ams_panel_ams_c_slot_3_label_slot_name
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_c_slot_3_label_slot_name = obj;
                            lv_obj_set_pos(obj, 108, 106);
                            lv_obj_set_size(obj, 18, 11);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "C3");
                        }
                        {
                            // ams_screen_ams_panel_ams_c_slot_4_label_slot_name
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_c_slot_4_label_slot_name = obj;
                            lv_obj_set_pos(obj, 162, 106);
                            lv_obj_set_size(obj, 18, 11);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "C4");
                        }
                        {
                            // ams_screen_ams_panel_ams_c_slot_1_label_fill_level
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_c_slot_1_label_fill_level = obj;
                            lv_obj_set_pos(obj, 0, 123);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ams_c_slot_2_label_fill_level
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_c_slot_2_label_fill_level = obj;
                            lv_obj_set_pos(obj, 54, 123);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ams_c_slot_3_label_fill_level
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_c_slot_3_label_fill_level = obj;
                            lv_obj_set_pos(obj, 107, 123);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ams_c_slot_4_label_fill_level
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_c_slot_4_label_fill_level = obj;
                            lv_obj_set_pos(obj, 161, 123);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // ams_screen_ams_panel_ams_c_slot_1_label_slot_name
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_c_slot_1_label_slot_name = obj;
                            lv_obj_set_pos(obj, 2, 107);
                            lv_obj_set_size(obj, 18, 11);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "C1");
                        }
                        {
                            // ams_screen_ams_panel_ams_c_icon_temperature
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.ams_screen_ams_panel_ams_c_icon_temperature = obj;
                            lv_obj_set_pos(obj, 155, -17);
                            lv_obj_set_size(obj, 21, 14);
                            lv_image_set_src(obj, &img_thermometer);
                            lv_image_set_scale(obj, 95);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff1967ea), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                    }
                }
            }
        }
    }
    
    tick_screen_ams_overview();
}

void tick_screen_ams_overview() {
}

void create_screen_scan_result() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.scan_result = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 800, 480);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // scan_screen_top_bar
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.scan_screen_top_bar = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 800, 44);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3d3d3d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // scan_screen_top_bar_icon_back
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.scan_screen_top_bar_icon_back = obj;
                    lv_obj_set_pos(obj, 5, 1);
                    lv_obj_set_size(obj, 48, 42);
                    lv_image_set_src(obj, &img_back);
                    lv_image_set_scale(obj, 80);
                }
                {
                    // scan_screen_top_bar_logo
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.scan_screen_top_bar_logo = obj;
                    lv_obj_set_pos(obj, 37, 1);
                    lv_obj_set_size(obj, 173, 43);
                    lv_image_set_src(obj, &img_spoolbuddy_logo_dark);
                    lv_image_set_scale(obj, 200);
                    lv_obj_set_style_align(obj, LV_ALIGN_TOP_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // scan_screen_top_bar_printer_select
                    lv_obj_t *obj = lv_dropdown_create(parent_obj);
                    objects.scan_screen_top_bar_printer_select = obj;
                    lv_obj_set_pos(obj, 325, 2);
                    lv_obj_set_size(obj, 150, 39);
                    lv_dropdown_set_options(obj, "");
                    lv_dropdown_set_selected(obj, 0);
                }
                {
                    // scan_screen_top_bar_icon_wifi_signal
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.scan_screen_top_bar_icon_wifi_signal = obj;
                    lv_obj_set_pos(obj, 698, 10);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_image_set_src(obj, &img_signal);
                    lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // scan_screen_top_bar_icon_notification_bell
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.scan_screen_top_bar_icon_notification_bell = obj;
                    lv_obj_set_pos(obj, 662, 11);
                    lv_obj_set_size(obj, 24, 24);
                    lv_image_set_src(obj, &img_bell);
                    lv_image_set_scale(obj, 50);
                }
                {
                    // scan_screen_top_bar_label_clock
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.scan_screen_top_bar_label_clock = obj;
                    lv_obj_set_pos(obj, 737, 12);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
        {
            // scan_screen_main_panel
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.scan_screen_main_panel = obj;
            lv_obj_set_pos(obj, 25, 50);
            lv_obj_set_size(obj, 751, 418);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3d3d3d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // scan_screen_main_panel_top_panel
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.scan_screen_main_panel_top_panel = obj;
                    lv_obj_set_pos(obj, -3, -7);
                    lv_obj_set_size(obj, 706, 63);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 50, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // scan_screen_main_panel_top_panel_label_message
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.scan_screen_main_panel_top_panel_label_message = obj;
                            lv_obj_set_pos(obj, 44, 11);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text(obj, "NFC tag read successfully");
                        }
                        {
                            // scan_screen_main_panel_top_panel_label_status
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.scan_screen_main_panel_top_panel_label_status = obj;
                            lv_obj_set_pos(obj, 44, -8);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Spool Detected");
                        }
                        {
                            // scan_screen_main_panel_top_panel_icon_ok
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.scan_screen_main_panel_top_panel_icon_ok = obj;
                            lv_obj_set_pos(obj, -9, -8);
                            lv_obj_set_size(obj, 38, 35);
                            lv_image_set_src(obj, &img_ok);
                            lv_image_set_scale(obj, 255);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                    }
                }
                {
                    // scan_screen_main_panel_spool_panel
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.scan_screen_main_panel_spool_panel = obj;
                    lv_obj_set_pos(obj, -3, 66);
                    lv_obj_set_size(obj, 706, 72);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff2f3237), LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // scan_screen_main_panel_spool_panel_icon_spool
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.scan_screen_main_panel_spool_panel_icon_spool = obj;
                            lv_obj_set_pos(obj, -7, -7);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_clean);
                            lv_image_set_scale(obj, 300);
                        }
                        {
                            // scan_screen_main_panel_spool_panel_icon_spool_color
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.scan_screen_main_panel_spool_panel_icon_spool_color = obj;
                            lv_obj_set_pos(obj, -7, -7);
                            lv_obj_set_size(obj, 32, 42);
                            lv_image_set_src(obj, &img_spool_fill);
                            lv_image_set_scale(obj, 300);
                            lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xfffad607), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // scan_screen_main_panel_spool_panel_label_weight
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.scan_screen_main_panel_spool_panel_label_weight = obj;
                            lv_obj_set_pos(obj, 46, 23);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_radius(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // scan_screen_main_panel_spool_panel_label_filament
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.scan_screen_main_panel_spool_panel_label_filament = obj;
                            lv_obj_set_pos(obj, 46, -11);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // scan_screen_main_panel_spool_panel_label_filament_color
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.scan_screen_main_panel_spool_panel_label_filament_color = obj;
                            lv_obj_set_pos(obj, 46, 6);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // scan_screen_main_panel_spool_panel_label_k_factor
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.scan_screen_main_panel_spool_panel_label_k_factor = obj;
                            lv_obj_set_pos(obj, 265, -11);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffaca7a7), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "K Factor");
                        }
                        {
                            // scan_screen_main_panel_spool_panel_label_k_factor_value
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.scan_screen_main_panel_spool_panel_label_k_factor_value = obj;
                            lv_obj_set_pos(obj, 266, 5);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // scan_screen_main_panel_spool_panel_label_k_profile
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.scan_screen_main_panel_spool_panel_label_k_profile = obj;
                            lv_obj_set_pos(obj, 370, -11);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffaca7a7), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "K Profile");
                        }
                        {
                            // scan_screen_main_panel_spool_panel_label_k_profile_value
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.scan_screen_main_panel_spool_panel_label_k_profile_value = obj;
                            lv_obj_set_pos(obj, 371, 5);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // scan_screen_main_panel_spool_panel_label_weight_percentage
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.scan_screen_main_panel_spool_panel_label_weight_percentage = obj;
                            lv_obj_set_pos(obj, 93, 23);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_radius(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                    }
                }
                {
                    // scan_screen_main_panel_ams_panel
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.scan_screen_main_panel_ams_panel = obj;
                    lv_obj_set_pos(obj, -3, 150);
                    lv_obj_set_size(obj, 706, 178);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff2f3237), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // scan_screen_main_panel_ams_panel_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.scan_screen_main_panel_ams_panel_label = obj;
                            lv_obj_set_pos(obj, -9, -16);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text(obj, "Assign to AMS slot");
                        }
                        {
                            // scan_screen_main_panel_ams_panel_ht_a
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            objects.scan_screen_main_panel_ams_panel_ht_a = obj;
                            lv_obj_set_pos(obj, 506, 3);
                            lv_obj_set_size(obj, 78, 64);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                            lv_obj_set_style_arc_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_arc_rounded(obj, false, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_arc_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_layout(obj, LV_LAYOUT_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_width(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_y(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_spread(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff2f3237), LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // scan_screen_main_panel_ams_panel_ht_a_label_name
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ht_a_label_name = obj;
                                    lv_obj_set_pos(obj, -14, -17);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "HT-A");
                                }
                                {
                                    // scan_screen_main_panel_ams_panel_ht_a_slot_color
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ht_a_slot_color = obj;
                                    lv_obj_set_pos(obj, 1, 2);
                                    lv_obj_set_size(obj, 30, 30);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff726e6e), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_grad_stop(obj, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_main_stop(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff352a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "");
                                }
                                {
                                    // scan_screen_main_panel_ams_panel_ht_a_indicator
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ht_a_indicator = obj;
                                    lv_obj_set_pos(obj, 33, -15);
                                    lv_obj_set_size(obj, 12, 12);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, " ");
                                }
                            }
                        }
                        {
                            // scan_screen_main_panel_ams_panel_ams_a
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            objects.scan_screen_main_panel_ams_panel_ams_a = obj;
                            lv_obj_set_pos(obj, -9, 3);
                            lv_obj_set_size(obj, 245, 65);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                            lv_obj_set_style_arc_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_arc_rounded(obj, false, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_arc_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_layout(obj, LV_LAYOUT_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_width(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_y(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_spread(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // scan_screen_main_panel_ams_panel_ams_a_label_name
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ams_a_label_name = obj;
                                    lv_obj_set_pos(obj, -11, -13);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "A");
                                }
                                {
                                    // scan_screen_main_panel_ams_panel_ams_a_slot_1
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ams_a_slot_1 = obj;
                                    lv_obj_set_pos(obj, 11, -13);
                                    lv_obj_set_size(obj, 45, 45);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffec0a0a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "");
                                }
                                {
                                    // scan_screen_main_panel_ams_panel_ams_a_slot_2
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ams_a_slot_2 = obj;
                                    lv_obj_set_pos(obj, 64, -13);
                                    lv_obj_set_size(obj, 45, 45);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0a40ec), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "");
                                }
                                {
                                    // scan_screen_main_panel_ams_panel_ams_a_slot_3
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ams_a_slot_3 = obj;
                                    lv_obj_set_pos(obj, 116, -13);
                                    lv_obj_set_size(obj, 45, 45);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffece90a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "");
                                }
                                {
                                    // scan_screen_main_panel_ams_panel_ams_a_slot_4
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ams_a_slot_4 = obj;
                                    lv_obj_set_pos(obj, 169, -13);
                                    lv_obj_set_size(obj, 45, 45);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffd0bdbb), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "");
                                }
                                {
                                    // scan_screen_main_panel_ams_panel_ams_a_indicator
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ams_a_indicator = obj;
                                    lv_obj_set_pos(obj, -12, 13);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "    ");
                                }
                            }
                        }
                        {
                            // scan_screen_main_panel_ams_panel_ams_b
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            objects.scan_screen_main_panel_ams_panel_ams_b = obj;
                            lv_obj_set_pos(obj, 249, 2);
                            lv_obj_set_size(obj, 245, 65);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                            lv_obj_set_style_arc_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_arc_rounded(obj, false, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_arc_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_layout(obj, LV_LAYOUT_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff2f3237), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_width(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_y(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_spread(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // scan_screen_main_panel_ams_panel_ams_b_label_name
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ams_b_label_name = obj;
                                    lv_obj_set_pos(obj, -11, -14);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "B");
                                }
                                {
                                    // scan_screen_main_panel_ams_panel_ams_b_slot_1
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ams_b_slot_1 = obj;
                                    lv_obj_set_pos(obj, 11, -13);
                                    lv_obj_set_size(obj, 45, 45);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffec0a0a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "");
                                }
                                {
                                    // scan_screen_main_panel_ams_panel_ams_b_slot_2
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ams_b_slot_2 = obj;
                                    lv_obj_set_pos(obj, 64, -13);
                                    lv_obj_set_size(obj, 45, 45);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0a40ec), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "");
                                }
                                {
                                    // scan_screen_main_panel_ams_panel_ams_b_slot_3
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ams_b_slot_3 = obj;
                                    lv_obj_set_pos(obj, 116, -13);
                                    lv_obj_set_size(obj, 45, 45);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffece90a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "");
                                }
                                {
                                    // scan_screen_main_panel_ams_panel_ams_b_slot_4
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ams_b_slot_4 = obj;
                                    lv_obj_set_pos(obj, 169, -13);
                                    lv_obj_set_size(obj, 45, 45);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffd0bdbb), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "");
                                }
                                {
                                    // scan_screen_main_panel_ams_panel_ams_b_indicator
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ams_b_indicator = obj;
                                    lv_obj_set_pos(obj, -13, 14);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "    ");
                                }
                            }
                        }
                        {
                            // scan_screen_main_panel_ams_panel_ams_c
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            objects.scan_screen_main_panel_ams_panel_ams_c = obj;
                            lv_obj_set_pos(obj, -9, 80);
                            lv_obj_set_size(obj, 245, 65);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                            lv_obj_set_style_arc_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_arc_rounded(obj, false, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_arc_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_layout(obj, LV_LAYOUT_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff2f3237), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_width(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_y(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_spread(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // scan_screen_main_panel_ams_panel_ams_c_label_name
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ams_c_label_name = obj;
                                    lv_obj_set_pos(obj, -11, -13);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "C");
                                }
                                {
                                    // scan_screen_main_panel_ams_panel_ams_c_slot_1
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ams_c_slot_1 = obj;
                                    lv_obj_set_pos(obj, 11, -13);
                                    lv_obj_set_size(obj, 45, 45);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffec0a0a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "");
                                }
                                {
                                    // scan_screen_main_panel_ams_panel_ams_c_slot_2
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ams_c_slot_2 = obj;
                                    lv_obj_set_pos(obj, 64, -13);
                                    lv_obj_set_size(obj, 45, 45);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0a40ec), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "");
                                }
                                {
                                    // scan_screen_main_panel_ams_panel_ams_c_slot_3
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ams_c_slot_3 = obj;
                                    lv_obj_set_pos(obj, 116, -13);
                                    lv_obj_set_size(obj, 45, 45);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffece90a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "");
                                }
                                {
                                    // scan_screen_main_panel_ams_panel_ams_c_slot_4
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ams_c_slot_4 = obj;
                                    lv_obj_set_pos(obj, 169, -13);
                                    lv_obj_set_size(obj, 45, 45);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffd0bdbb), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "");
                                }
                                {
                                    // scan_screen_main_panel_ams_panel_ams_c_indicator
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ams_c_indicator = obj;
                                    lv_obj_set_pos(obj, -13, 14);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "    ");
                                }
                            }
                        }
                        {
                            // scan_screen_main_panel_ams_panel_ams_d
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            objects.scan_screen_main_panel_ams_panel_ams_d = obj;
                            lv_obj_set_pos(obj, 249, 80);
                            lv_obj_set_size(obj, 245, 65);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                            lv_obj_set_style_arc_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_arc_rounded(obj, false, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_arc_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_layout(obj, LV_LAYOUT_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff2f3237), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_width(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_y(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_spread(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // scan_screen_main_panel_ams_panel_ams_d_label_name
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ams_d_label_name = obj;
                                    lv_obj_set_pos(obj, -11, -13);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "D");
                                }
                                {
                                    // scan_screen_main_panel_ams_panel_ams_d_slot_1
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ams_d_slot_1 = obj;
                                    lv_obj_set_pos(obj, 11, -13);
                                    lv_obj_set_size(obj, 45, 45);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffec0a0a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "");
                                }
                                {
                                    // scan_screen_main_panel_ams_panel_ams_d_slot_2
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ams_d_slot_2 = obj;
                                    lv_obj_set_pos(obj, 64, -13);
                                    lv_obj_set_size(obj, 45, 45);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0a40ec), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "");
                                }
                                {
                                    // scan_screen_main_panel_ams_panel_ams_d_slot_3
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ams_d_slot_3 = obj;
                                    lv_obj_set_pos(obj, 116, -13);
                                    lv_obj_set_size(obj, 45, 45);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffece90a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "");
                                }
                                {
                                    // scan_screen_main_panel_ams_panel_ams_d_slot_4
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ams_d_slot_4 = obj;
                                    lv_obj_set_pos(obj, 169, -13);
                                    lv_obj_set_size(obj, 45, 45);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffd0bdbb), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "");
                                }
                                {
                                    // scan_screen_main_panel_ams_panel_ams_d_indicator
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ams_d_indicator = obj;
                                    lv_obj_set_pos(obj, -11, 14);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "    ");
                                }
                            }
                        }
                        {
                            // scan_screen_main_panel_ams_panel_ht_b
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            objects.scan_screen_main_panel_ams_panel_ht_b = obj;
                            lv_obj_set_pos(obj, 507, 80);
                            lv_obj_set_size(obj, 78, 64);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                            lv_obj_set_style_arc_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_arc_rounded(obj, false, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_arc_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_layout(obj, LV_LAYOUT_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_width(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_y(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_spread(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // scan_screen_main_panel_ams_panel_ht_b_label_name
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ht_b_label_name = obj;
                                    lv_obj_set_pos(obj, -14, -17);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "HT-B");
                                }
                                {
                                    // scan_screen_main_panel_ams_panel_ht_b_slot
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ht_b_slot = obj;
                                    lv_obj_set_pos(obj, 1, 2);
                                    lv_obj_set_size(obj, 30, 30);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff726e6e), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_grad_stop(obj, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_main_stop(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff352a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "");
                                }
                                {
                                    // scan_screen_main_panel_ams_panel_ht_b_indicator
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ht_b_indicator = obj;
                                    lv_obj_set_pos(obj, 30, -15);
                                    lv_obj_set_size(obj, 15, 12);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "    ");
                                }
                            }
                        }
                        {
                            // scan_screen_main_panel_ams_panel_ext_l
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            objects.scan_screen_main_panel_ams_panel_ext_l = obj;
                            lv_obj_set_pos(obj, 594, 3);
                            lv_obj_set_size(obj, 78, 64);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                            lv_obj_set_style_arc_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_arc_rounded(obj, false, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_arc_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_layout(obj, LV_LAYOUT_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_width(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_y(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_spread(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // scan_screen_main_panel_ams_panel_ext_l_label_name
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ext_l_label_name = obj;
                                    lv_obj_set_pos(obj, -14, -17);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "EXT-L");
                                }
                                {
                                    // scan_screen_main_panel_ams_panel_ext_l_slot
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ext_l_slot = obj;
                                    lv_obj_set_pos(obj, 1, 2);
                                    lv_obj_set_size(obj, 30, 30);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff726e6e), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_grad_stop(obj, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_main_stop(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff352a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "");
                                }
                                {
                                    // scan_screen_main_panel_ams_panel_ext_l_indicator
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ext_l_indicator = obj;
                                    lv_obj_set_pos(obj, 33, -15);
                                    lv_obj_set_size(obj, 12, 12);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, " ");
                                }
                            }
                        }
                        {
                            // scan_screen_main_panel_ams_panel_ext_r
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            objects.scan_screen_main_panel_ams_panel_ext_r = obj;
                            lv_obj_set_pos(obj, 594, 80);
                            lv_obj_set_size(obj, 78, 64);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                            lv_obj_set_style_arc_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_arc_rounded(obj, false, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_arc_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_layout(obj, LV_LAYOUT_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_width(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_y(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_spread(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // scan_screen_main_panel_ams_panel_ext_r_label_name
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ext_r_label_name = obj;
                                    lv_obj_set_pos(obj, -14, -17);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "EXT-R");
                                }
                                {
                                    // scan_screen_main_panel_ams_panel_ext_r_slot
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ext_r_slot = obj;
                                    lv_obj_set_pos(obj, 1, 2);
                                    lv_obj_set_size(obj, 30, 30);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff726e6e), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffbab1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_grad_stop(obj, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_main_stop(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff352a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "");
                                }
                                {
                                    // scan_screen_main_panel_ams_panel_ext_r_indicator
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.scan_screen_main_panel_ams_panel_ext_r_indicator = obj;
                                    lv_obj_set_pos(obj, 28, -15);
                                    lv_obj_set_size(obj, 17, 12);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "    ");
                                }
                            }
                        }
                    }
                }
                {
                    // scan_screen_button_assign_save
                    lv_obj_t *obj = lv_button_create(parent_obj);
                    objects.scan_screen_button_assign_save = obj;
                    lv_obj_set_pos(obj, 2, 338);
                    lv_obj_set_size(obj, 706, 50);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // scan_screen_button_assign_save_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.scan_screen_button_assign_save_label = obj;
                            lv_obj_set_pos(obj, 0, 0);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Assign & Save");
                        }
                    }
                }
            }
        }
    }
    
    tick_screen_scan_result();
}

void tick_screen_scan_result() {
}

void create_screen_spool_details() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.spool_details = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 800, 480);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // spool_screen_top_bar
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.spool_screen_top_bar = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 800, 44);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3d3d3d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // spool_screen_top_bar_icon_back
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.spool_screen_top_bar_icon_back = obj;
                    lv_obj_set_pos(obj, 5, 1);
                    lv_obj_set_size(obj, 48, 42);
                    lv_image_set_src(obj, &img_back);
                    lv_image_set_scale(obj, 80);
                }
                {
                    // spool_screen_top_bar_logo
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.spool_screen_top_bar_logo = obj;
                    lv_obj_set_pos(obj, 37, 1);
                    lv_obj_set_size(obj, 173, 43);
                    lv_image_set_src(obj, &img_spoolbuddy_logo_dark);
                    lv_image_set_scale(obj, 200);
                    lv_obj_set_style_align(obj, LV_ALIGN_TOP_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // spool_screen_top_bar_printer_select
                    lv_obj_t *obj = lv_dropdown_create(parent_obj);
                    objects.spool_screen_top_bar_printer_select = obj;
                    lv_obj_set_pos(obj, 325, 2);
                    lv_obj_set_size(obj, 150, 39);
                    lv_dropdown_set_options(obj, "");
                    lv_dropdown_set_selected(obj, 0);
                }
                {
                    // spool_screen_top_bar_icon_wifi_signal
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.spool_screen_top_bar_icon_wifi_signal = obj;
                    lv_obj_set_pos(obj, 698, 10);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_image_set_src(obj, &img_signal);
                    lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // spool_screen_top_bar_icon_notifiastion_bell
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.spool_screen_top_bar_icon_notifiastion_bell = obj;
                    lv_obj_set_pos(obj, 662, 11);
                    lv_obj_set_size(obj, 24, 24);
                    lv_image_set_src(obj, &img_bell);
                    lv_image_set_scale(obj, 50);
                }
                {
                    // spool_screen_top_bar_label_clock
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.spool_screen_top_bar_label_clock = obj;
                    lv_obj_set_pos(obj, 737, 12);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
        {
            // spool_screen_main_panel
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.spool_screen_main_panel = obj;
            lv_obj_set_pos(obj, 29, 66);
            lv_obj_set_size(obj, 751, 380);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3d3d3d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // spool_screen_main_panel_button_edit
                    lv_obj_t *obj = lv_button_create(parent_obj);
                    objects.spool_screen_main_panel_button_edit = obj;
                    lv_obj_set_pos(obj, 236, 295);
                    lv_obj_set_size(obj, 230, 50);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // spool_screen_main_panel_button_edit_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.spool_screen_main_panel_button_edit_label = obj;
                            lv_obj_set_pos(obj, 0, 0);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Edit");
                        }
                    }
                }
                {
                    // spool_screen_main_panel_button_remove
                    lv_obj_t *obj = lv_button_create(parent_obj);
                    objects.spool_screen_main_panel_button_remove = obj;
                    lv_obj_set_pos(obj, 473, 295);
                    lv_obj_set_size(obj, 230, 50);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // spool_screen_main_panel_button_remove_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.spool_screen_main_panel_button_remove_label = obj;
                            lv_obj_set_pos(obj, 0, 0);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Remove");
                        }
                    }
                }
                {
                    // spool_screen_main_panel_button_assign
                    lv_obj_t *obj = lv_button_create(parent_obj);
                    objects.spool_screen_main_panel_button_assign = obj;
                    lv_obj_set_pos(obj, -3, 295);
                    lv_obj_set_size(obj, 230, 50);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // spool_screen_main_panel_button_assign_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.spool_screen_main_panel_button_assign_label = obj;
                            lv_obj_set_pos(obj, 0, 0);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Assign Slot");
                        }
                    }
                }
                {
                    // spool_screen_main_panel_middle_panel
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.spool_screen_main_panel_middle_panel = obj;
                    lv_obj_set_pos(obj, -3, 66);
                    lv_obj_set_size(obj, 706, 77);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // spool_screen_main_panel_middle_panel_label_name
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.spool_screen_main_panel_middle_panel_label_name = obj;
                            lv_obj_set_pos(obj, -7, -14);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffaca7a7), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Print Settings");
                        }
                        {
                            // spool_screen_main_panel_middle_panel_label_nozzle
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.spool_screen_main_panel_middle_panel_label_nozzle = obj;
                            lv_obj_set_pos(obj, -8, 7);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffaca7a7), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Nozzle");
                        }
                        {
                            // spool_screen_main_panel_middle_panel_label_bed
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.spool_screen_main_panel_middle_panel_label_bed = obj;
                            lv_obj_set_pos(obj, 103, 7);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffaca7a7), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Bed");
                        }
                        {
                            // spool_screen_main_panel_middle_panel_label_speed
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.spool_screen_main_panel_middle_panel_label_speed = obj;
                            lv_obj_set_pos(obj, 195, 7);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffaca7a7), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Max. Speed");
                        }
                        {
                            // spool_screen_main_panel_middle_panel_label_nozzle_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.spool_screen_main_panel_middle_panel_label_nozzle_label = obj;
                            lv_obj_set_pos(obj, -7, 25);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // spool_screen_main_panel_middle_panel_label_bed_value
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.spool_screen_main_panel_middle_panel_label_bed_value = obj;
                            lv_obj_set_pos(obj, 103, 25);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // spool_screen_main_panel_middle_panel_label_speed_value
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.spool_screen_main_panel_middle_panel_label_speed_value = obj;
                            lv_obj_set_pos(obj, 195, 25);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // spool_screen_main_panel_middle_panel_label_k_profile
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.spool_screen_main_panel_middle_panel_label_k_profile = obj;
                            lv_obj_set_pos(obj, 318, 9);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffaca7a7), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "K Profile");
                        }
                        {
                            // spool_screen_main_panel_middle_panel_label_k_profile_value
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.spool_screen_main_panel_middle_panel_label_k_profile_value = obj;
                            lv_obj_set_pos(obj, 319, 25);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                    }
                }
                {
                    // spool_screen_main_panel_top
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.spool_screen_main_panel_top = obj;
                    lv_obj_set_pos(obj, -3, -9);
                    lv_obj_set_size(obj, 706, 66);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // spool_screen_main_panel_top_icon_spool
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.spool_screen_main_panel_top_icon_spool = obj;
                            lv_obj_set_pos(obj, -8, -10);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_image_set_src(obj, &img_spool_clean);
                        }
                        {
                            // spool_screen_main_panel_top_icon_spool_color
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.spool_screen_main_panel_top_icon_spool_color = obj;
                            lv_obj_set_pos(obj, -8, -10);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_image_set_src(obj, &img_spool_fill);
                            lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xfffad607), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // spool_screen_main_panel_top_label_weight
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.spool_screen_main_panel_top_label_weight = obj;
                            lv_obj_set_pos(obj, 38, -10);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_radius(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // spool_screen_main_panel_top_label_spool_material
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.spool_screen_main_panel_top_label_spool_material = obj;
                            lv_obj_set_pos(obj, 186, -10);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // spool_screen_main_panel_top_label_color
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.spool_screen_main_panel_top_label_color = obj;
                            lv_obj_set_pos(obj, 99, 16);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfffafafa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // spool_screen_main_panel_top_label_spool_vendor
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.spool_screen_main_panel_top_label_spool_vendor = obj;
                            lv_obj_set_pos(obj, 99, -10);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffaca7a7), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // spool_screen_main_panel_top_label_spool_fill_level
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.spool_screen_main_panel_top_label_spool_fill_level = obj;
                            lv_obj_set_pos(obj, 38, 16);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_radius(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                    }
                }
                {
                    // spool_screen_main_panel_bottom
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.spool_screen_main_panel_bottom = obj;
                    lv_obj_set_pos(obj, -3, 154);
                    lv_obj_set_size(obj, 706, 130);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff282b30), LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // spool_screen_main_panel_bottom_label_name
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.spool_screen_main_panel_bottom_label_name = obj;
                            lv_obj_set_pos(obj, -8, -14);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffaca7a7), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Spool Information");
                        }
                        {
                            // spool_screen_main_panel_bottom_label_tag_id
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.spool_screen_main_panel_bottom_label_tag_id = obj;
                            lv_obj_set_pos(obj, -8, 7);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffaca7a7), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Tag ID");
                        }
                        {
                            // spool_screen_main_panel_bottom_label_weight
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.spool_screen_main_panel_bottom_label_weight = obj;
                            lv_obj_set_pos(obj, 180, 7);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffaca7a7), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Initial Weight");
                        }
                        {
                            // spool_screen_main_panel_bottom_label_used
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.spool_screen_main_panel_bottom_label_used = obj;
                            lv_obj_set_pos(obj, 180, 54);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffaca7a7), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Used");
                        }
                        {
                            // spool_screen_main_panel_bottom_label_last_weighed
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.spool_screen_main_panel_bottom_label_last_weighed = obj;
                            lv_obj_set_pos(obj, 439, 7);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffaca7a7), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Last Weighed");
                        }
                        {
                            // spool_screen_main_panel_bottom_label_added
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.spool_screen_main_panel_bottom_label_added = obj;
                            lv_obj_set_pos(obj, -7, 56);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffaca7a7), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Added");
                        }
                        {
                            // spool_screen_main_panel_bottom_label_tag_value
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.spool_screen_main_panel_bottom_label_tag_value = obj;
                            lv_obj_set_pos(obj, -8, 27);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // spool_screen_main_panel_bottom_label_weight_value
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.spool_screen_main_panel_bottom_label_weight_value = obj;
                            lv_obj_set_pos(obj, 180, 27);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // spool_screen_main_panel_bottom_label_used_value
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.spool_screen_main_panel_bottom_label_used_value = obj;
                            lv_obj_set_pos(obj, 180, 74);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // spool_screen_main_panel_bottom_label_last_weighed_value
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.spool_screen_main_panel_bottom_label_last_weighed_value = obj;
                            lv_obj_set_pos(obj, 439, 27);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // spool_screen_main_panel_bottom_label_added_value
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.spool_screen_main_panel_bottom_label_added_value = obj;
                            lv_obj_set_pos(obj, -7, 74);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // spool_screen_main_panel_bottom_label_used_value_percentage
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.spool_screen_main_panel_bottom_label_used_value_percentage = obj;
                            lv_obj_set_pos(obj, 221, 74);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_radius(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                    }
                }
            }
        }
    }
    
    tick_screen_spool_details();
}

void tick_screen_spool_details() {
}

void create_screen_settings_screen() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.settings_screen = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 800, 480);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // settings_screen_top_bar_
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.settings_screen_top_bar_ = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 800, 44);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3d3d3d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // settings_screen_top_bar_logo
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.settings_screen_top_bar_logo = obj;
                    lv_obj_set_pos(obj, 55, 1);
                    lv_obj_set_size(obj, 173, 43);
                    lv_image_set_src(obj, &img_spoolbuddy_logo_dark);
                    lv_image_set_scale(obj, 200);
                    lv_obj_set_style_align(obj, LV_ALIGN_TOP_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // settings_screen_top_bar_printer_select
                    lv_obj_t *obj = lv_dropdown_create(parent_obj);
                    objects.settings_screen_top_bar_printer_select = obj;
                    lv_obj_set_pos(obj, 325, 2);
                    lv_obj_set_size(obj, 150, 39);
                    lv_dropdown_set_options(obj, "");
                    lv_dropdown_set_selected(obj, 0);
                }
                {
                    // settings_screen_top_bar_icon_wifi_signal
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.settings_screen_top_bar_icon_wifi_signal = obj;
                    lv_obj_set_pos(obj, 698, 10);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_image_set_src(obj, &img_signal);
                    lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // settings_screen_top_bar_icon_notification_bell
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.settings_screen_top_bar_icon_notification_bell = obj;
                    lv_obj_set_pos(obj, 662, 11);
                    lv_obj_set_size(obj, 24, 24);
                    lv_image_set_src(obj, &img_bell);
                    lv_image_set_scale(obj, 50);
                }
                {
                    // settings_screen_top_bar_label_clock
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.settings_screen_top_bar_label_clock = obj;
                    lv_obj_set_pos(obj, 737, 12);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
                {
                    // settings_network_screen_top_bar_icon_back
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.settings_network_screen_top_bar_icon_back = obj;
                    lv_obj_set_pos(obj, 5, 1);
                    lv_obj_set_size(obj, 48, 42);
                    lv_image_set_src(obj, &img_back);
                    lv_image_set_scale(obj, 80);
                }
            }
        }
        {
            // settings_screen_tabs
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.settings_screen_tabs = obj;
            lv_obj_set_pos(obj, 0, 44);
            lv_obj_set_size(obj, 800, 40);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff252525), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // settings_screen_tabs_network
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.settings_screen_tabs_network = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, 200, 40);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // settings_screen_tabs_network_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_screen_tabs_network_label = obj;
                            lv_obj_set_pos(obj, 60, 10);
                            lv_obj_set_size(obj, 80, 20);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Network");
                        }
                    }
                }
                {
                    // settings_screen_tabs_printers
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.settings_screen_tabs_printers = obj;
                    lv_obj_set_pos(obj, 200, 0);
                    lv_obj_set_size(obj, 200, 40);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff252525), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // settings_screen_tabs_printers_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_screen_tabs_printers_label = obj;
                            lv_obj_set_pos(obj, 60, 10);
                            lv_obj_set_size(obj, 80, 20);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff888888), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Printers");
                        }
                    }
                }
                {
                    // settings_screen_tabs_hardware
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.settings_screen_tabs_hardware = obj;
                    lv_obj_set_pos(obj, 400, 0);
                    lv_obj_set_size(obj, 200, 40);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff252525), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // settings_screen_tabs_hardware_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_screen_tabs_hardware_label = obj;
                            lv_obj_set_pos(obj, 60, 10);
                            lv_obj_set_size(obj, 80, 20);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff888888), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Hardware");
                        }
                    }
                }
                {
                    // settings_screen_tabs_system
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.settings_screen_tabs_system = obj;
                    lv_obj_set_pos(obj, 600, 0);
                    lv_obj_set_size(obj, 200, 40);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff252525), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // settings_screen_tabs_system_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_screen_tabs_system_label = obj;
                            lv_obj_set_pos(obj, 60, 10);
                            lv_obj_set_size(obj, 80, 20);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff888888), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "System");
                        }
                    }
                }
            }
        }
        {
            // settings_screen_tabs_network_content
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.settings_screen_tabs_network_content = obj;
            lv_obj_set_pos(obj, 0, 84);
            lv_obj_set_size(obj, 800, 396);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // settings_screen_tabs_network_content_wifi
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.settings_screen_tabs_network_content_wifi = obj;
                    lv_obj_set_pos(obj, 15, 10);
                    lv_obj_set_size(obj, 770, 50);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // settings_screen_tabs_network_content_wifi_label_name
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_screen_tabs_network_content_wifi_label_name = obj;
                            lv_obj_set_pos(obj, 45, 15);
                            lv_obj_set_size(obj, 200, 20);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "WiFi Network");
                        }
                        {
                            // settings_screen_tabs_network_content_wifi_label_ssid
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_screen_tabs_network_content_wifi_label_ssid = obj;
                            lv_obj_set_pos(obj, 550, 7);
                            lv_obj_set_size(obj, 150, 20);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff888888), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "SpoolBuddy_5G");
                        }
                        {
                            // settings_screen_tabs_network_content_wifi_icon_select
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_screen_tabs_network_content_wifi_icon_select = obj;
                            lv_obj_set_pos(obj, 725, 15);
                            lv_obj_set_size(obj, 20, 24);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff666666), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, ">");
                        }
                        {
                            // settings_screen_tabs_network_content_wifi_icon_wifi
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.settings_screen_tabs_network_content_wifi_icon_wifi = obj;
                            lv_obj_set_pos(obj, -23, -9);
                            lv_obj_set_size(obj, 70, 69);
                            lv_image_set_src(obj, &img_wifi);
                            lv_image_set_scale(obj, 20);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // settings_screen_tabs_network_content_wifi_label_ip_address
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_screen_tabs_network_content_wifi_label_ip_address = obj;
                            lv_obj_set_pos(obj, 550, 27);
                            lv_obj_set_size(obj, 150, 20);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff888888), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "192.168.1.42");
                        }
                    }
                }
            }
        }
        {
            // settings_screen_tabs_printers_content
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.settings_screen_tabs_printers_content = obj;
            lv_obj_set_pos(obj, 0, 84);
            lv_obj_set_size(obj, 800, 396);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // settings_screen_tabs_printers_content_add_printer
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.settings_screen_tabs_printers_content_add_printer = obj;
                    lv_obj_set_pos(obj, 15, 10);
                    lv_obj_set_size(obj, 770, 50);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // settings_screen_tabs_printers_content_add_printer_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_screen_tabs_printers_content_add_printer_label = obj;
                            lv_obj_set_pos(obj, 45, 15);
                            lv_obj_set_size(obj, 200, 20);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Add Printer");
                        }
                        {
                            // settings_screen_tabs_printers_content_add_printer_label_select
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_screen_tabs_printers_content_add_printer_label_select = obj;
                            lv_obj_set_pos(obj, 725, 15);
                            lv_obj_set_size(obj, 20, 24);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff666666), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, ">");
                        }
                        {
                            // settings_screen_tabs_printers_content_add_printer_icon_add
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.settings_screen_tabs_printers_content_add_printer_icon_add = obj;
                            lv_obj_set_pos(obj, -38, -25);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_image_set_src(obj, &img_add);
                            lv_image_set_scale(obj, 80);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                    }
                }
                {
                    // settings_screen_tabs_printers_content_printer_1
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.settings_screen_tabs_printers_content_printer_1 = obj;
                    lv_obj_set_pos(obj, 15, 70);
                    lv_obj_set_size(obj, 770, 50);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // settings_screen_tabs_printers_content_printer_1_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_screen_tabs_printers_content_printer_1_label = obj;
                            lv_obj_set_pos(obj, 45, 16);
                            lv_obj_set_size(obj, 200, 20);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "H2D-1");
                        }
                        {
                            // settings_screen_tabs_printers_content_printer_1_label_online
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_screen_tabs_printers_content_printer_1_label_online = obj;
                            lv_obj_set_pos(obj, 550, 15);
                            lv_obj_set_size(obj, 150, 20);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff888888), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Online");
                        }
                        {
                            // settings_screen_tabs_printers_content_printer_1_label_select
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_screen_tabs_printers_content_printer_1_label_select = obj;
                            lv_obj_set_pos(obj, 725, 15);
                            lv_obj_set_size(obj, 20, 24);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff666666), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, ">");
                        }
                        {
                            // settings_screen_tabs_printers_content_printer_1_icon
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.settings_screen_tabs_printers_content_printer_1_icon = obj;
                            lv_obj_set_pos(obj, -38, -25);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_image_set_src(obj, &img_3d_cube);
                            lv_image_set_scale(obj, 80);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                    }
                }
            }
        }
        {
            // settings_screen_tabs_hardware_content
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.settings_screen_tabs_hardware_content = obj;
            lv_obj_set_pos(obj, 0, 84);
            lv_obj_set_size(obj, 800, 396);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // settings_screen_tabs_hardware_content_nfc
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.settings_screen_tabs_hardware_content_nfc = obj;
                    lv_obj_set_pos(obj, 15, 10);
                    lv_obj_set_size(obj, 770, 50);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // settings_screen_tabs_hardware_content_nfc_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_screen_tabs_hardware_content_nfc_label = obj;
                            lv_obj_set_pos(obj, 45, 15);
                            lv_obj_set_size(obj, 200, 20);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "NFC Reader");
                        }
                        {
                            // settings_screen_tabs_hardware_content_nfc_label_type
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_screen_tabs_hardware_content_nfc_label_type = obj;
                            lv_obj_set_pos(obj, 550, 15);
                            lv_obj_set_size(obj, 150, 20);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff888888), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "PN5180");
                        }
                        {
                            // settings_screen_tabs_hardware_content_nfc_label_select
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_screen_tabs_hardware_content_nfc_label_select = obj;
                            lv_obj_set_pos(obj, 725, 15);
                            lv_obj_set_size(obj, 20, 24);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff666666), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, ">");
                        }
                        {
                            // settings_screen_tabs_hardware_content_nfc_icon_nfc
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.settings_screen_tabs_hardware_content_nfc_icon_nfc = obj;
                            lv_obj_set_pos(obj, -38, -25);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_image_set_src(obj, &img_nfc);
                            lv_image_set_scale(obj, 75);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                    }
                }
                {
                    // settings_screen_tabs_hardware_content_scale
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.settings_screen_tabs_hardware_content_scale = obj;
                    lv_obj_set_pos(obj, 15, 70);
                    lv_obj_set_size(obj, 770, 50);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // settings_screen_tabs_hardware_content_scale_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_screen_tabs_hardware_content_scale_label = obj;
                            lv_obj_set_pos(obj, 45, 15);
                            lv_obj_set_size(obj, 200, 20);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Scale");
                        }
                        {
                            // settings_screen_tabs_hardware_content_scale_label_type
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_screen_tabs_hardware_content_scale_label_type = obj;
                            lv_obj_set_pos(obj, 550, 15);
                            lv_obj_set_size(obj, 150, 20);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff888888), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "NAU7802");
                        }
                        {
                            // settings_screen_tabs_hardware_content_scale_label_select
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_screen_tabs_hardware_content_scale_label_select = obj;
                            lv_obj_set_pos(obj, 725, 15);
                            lv_obj_set_size(obj, 20, 24);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff666666), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, ">");
                        }
                        {
                            // settings_screen_tabs_hardware_content_scale_icon
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.settings_screen_tabs_hardware_content_scale_icon = obj;
                            lv_obj_set_pos(obj, -38, -25);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_image_set_src(obj, &img_scale_2);
                            lv_image_set_scale(obj, 75);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                    }
                }
                {
                    // settings_screen_tabs_display_content
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.settings_screen_tabs_display_content = obj;
                    lv_obj_set_pos(obj, 15, 130);
                    lv_obj_set_size(obj, 770, 50);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // settings_screen_tabs_display_content_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_screen_tabs_display_content_label = obj;
                            lv_obj_set_pos(obj, 45, 15);
                            lv_obj_set_size(obj, 200, 20);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Display");
                        }
                        {
                            // settings_screen_tabs_display_content_label_resolution
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_screen_tabs_display_content_label_resolution = obj;
                            lv_obj_set_pos(obj, 550, 15);
                            lv_obj_set_size(obj, 150, 20);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff888888), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "7.0\" 800x480");
                        }
                        {
                            // settings_screen_tabs_display_content_label_select
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_screen_tabs_display_content_label_select = obj;
                            lv_obj_set_pos(obj, 725, 15);
                            lv_obj_set_size(obj, 20, 24);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff666666), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, ">");
                        }
                        {
                            // settings_screen_tabs_display_content_icon
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.settings_screen_tabs_display_content_icon = obj;
                            lv_obj_set_pos(obj, -38, -25);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_image_set_src(obj, &img_display);
                            lv_image_set_scale(obj, 75);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                    }
                }
            }
        }
        {
            // settings_screen_tabs_system_content
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.settings_screen_tabs_system_content = obj;
            lv_obj_set_pos(obj, 0, 84);
            lv_obj_set_size(obj, 800, 396);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // settings_screen_tabs_system_content_firmware
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.settings_screen_tabs_system_content_firmware = obj;
                    lv_obj_set_pos(obj, 15, 10);
                    lv_obj_set_size(obj, 770, 50);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // settings_screen_tabs_system_content_firmware_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_screen_tabs_system_content_firmware_label = obj;
                            lv_obj_set_pos(obj, 37, 15);
                            lv_obj_set_size(obj, 200, 20);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Firmware Version");
                        }
                        {
                            // settings_screen_tabs_system_content_firmware_label_version
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_screen_tabs_system_content_firmware_label_version = obj;
                            lv_obj_set_pos(obj, 602, 17);
                            lv_obj_set_size(obj, 79, 20);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff888888), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // settings_screen_tabs_system_content_firmware_label_select
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_screen_tabs_system_content_firmware_label_select = obj;
                            lv_obj_set_pos(obj, 715, 15);
                            lv_obj_set_size(obj, 20, 24);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff666666), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, ">");
                        }
                        {
                            // settings_screen_tabs_system_content_firmware_icon
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.settings_screen_tabs_system_content_firmware_icon = obj;
                            lv_obj_set_pos(obj, -38, -25);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_image_set_src(obj, &img_firmware);
                            lv_image_set_scale(obj, 80);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                    }
                }
                {
                    // settings_screen_tabs_system_content_reset
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.settings_screen_tabs_system_content_reset = obj;
                    lv_obj_set_pos(obj, 15, 70);
                    lv_obj_set_size(obj, 770, 50);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // settings_screen_tabs_system_content_reset_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_screen_tabs_system_content_reset_label = obj;
                            lv_obj_set_pos(obj, 37, 15);
                            lv_obj_set_size(obj, 200, 20);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Factory Reset");
                        }
                        {
                            // settings_screen_tabs_system_content_reset_label_select
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_screen_tabs_system_content_reset_label_select = obj;
                            lv_obj_set_pos(obj, 715, 15);
                            lv_obj_set_size(obj, 20, 24);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff666666), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, ">");
                        }
                        {
                            // settings_screen_tabs_system_content_reset_icon
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.settings_screen_tabs_system_content_reset_icon = obj;
                            lv_obj_set_pos(obj, -38, -25);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_image_set_src(obj, &img_reset);
                            lv_image_set_scale(obj, 80);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                    }
                }
                {
                    // settings_screen_tabs_about_content
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.settings_screen_tabs_about_content = obj;
                    lv_obj_set_pos(obj, 15, 130);
                    lv_obj_set_size(obj, 770, 50);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // settings_screen_tabs_about_content_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_screen_tabs_about_content_label = obj;
                            lv_obj_set_pos(obj, 37, 15);
                            lv_obj_set_size(obj, 200, 20);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "About");
                        }
                        {
                            // settings_screen_tabs_about_content_label_select
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_screen_tabs_about_content_label_select = obj;
                            lv_obj_set_pos(obj, 715, 15);
                            lv_obj_set_size(obj, 20, 24);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff666666), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, ">");
                        }
                        {
                            // settings_screen_tabs_about_content_icon
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.settings_screen_tabs_about_content_icon = obj;
                            lv_obj_set_pos(obj, -38, -25);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_image_set_src(obj, &img_about);
                            lv_image_set_scale(obj, 80);
                            lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                    }
                }
            }
        }
    }
    
    tick_screen_settings_screen();
}

void tick_screen_settings_screen() {
}

void create_screen_settings_wifi_screen() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.settings_wifi_screen = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 800, 480);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_ofs_x(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // settings_wifi_screen_top_bar
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.settings_wifi_screen_top_bar = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 800, 44);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3d3d3d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // settings_wifi_screen_top_bar_logo
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.settings_wifi_screen_top_bar_logo = obj;
                    lv_obj_set_pos(obj, 44, 1);
                    lv_obj_set_size(obj, 173, 43);
                    lv_image_set_src(obj, &img_spoolbuddy_logo_dark);
                    lv_image_set_scale(obj, 200);
                    lv_obj_set_style_align(obj, LV_ALIGN_TOP_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // settings_wifi_screen_top_bar_printer_select
                    lv_obj_t *obj = lv_dropdown_create(parent_obj);
                    objects.settings_wifi_screen_top_bar_printer_select = obj;
                    lv_obj_set_pos(obj, 325, 2);
                    lv_obj_set_size(obj, 150, 39);
                    lv_dropdown_set_options(obj, "");
                    lv_dropdown_set_selected(obj, 0);
                }
                {
                    // settings_wifi_screen_top_bar_icon_wifi_signal
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.settings_wifi_screen_top_bar_icon_wifi_signal = obj;
                    lv_obj_set_pos(obj, 698, 10);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_image_set_src(obj, &img_signal);
                    lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // settings_wifi_screen_top_bar_icon_notification_bell
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.settings_wifi_screen_top_bar_icon_notification_bell = obj;
                    lv_obj_set_pos(obj, 662, 11);
                    lv_obj_set_size(obj, 24, 24);
                    lv_image_set_src(obj, &img_bell);
                    lv_image_set_scale(obj, 50);
                }
                {
                    // settings_wifi_screen_top_bar_label_clock
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.settings_wifi_screen_top_bar_label_clock = obj;
                    lv_obj_set_pos(obj, 737, 12);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
                {
                    // settings_wifi_screen_top_bar_icon_back
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.settings_wifi_screen_top_bar_icon_back = obj;
                    lv_obj_set_pos(obj, 5, 1);
                    lv_obj_set_size(obj, 48, 42);
                    lv_image_set_src(obj, &img_back);
                    lv_image_set_scale(obj, 80);
                }
            }
        }
        {
            // settings_wifi_screen_content_
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.settings_wifi_screen_content_ = obj;
            lv_obj_set_pos(obj, 0, 44);
            lv_obj_set_size(obj, 800, 436);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // settings_wifi_screen_content_panel_
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.settings_wifi_screen_content_panel_ = obj;
                    lv_obj_set_pos(obj, 0, 10);
                    lv_obj_set_size(obj, 765, 343);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_arc_width(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_rounded(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff796666), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // settings_wifi_screen_content_panel_label_wifi
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_wifi_screen_content_panel_label_wifi = obj;
                            lv_obj_set_pos(obj, 16, -7);
                            lv_obj_set_size(obj, 300, 30);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_decor(obj, LV_TEXT_DECOR_UNDERLINE, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "WiFi Network");
                        }
                        {
                            // settings_wifi_screen_content_panel_label_ssid
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_wifi_screen_content_panel_label_ssid = obj;
                            lv_obj_set_pos(obj, 16, 24);
                            lv_obj_set_size(obj, 100, 25);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_label_set_text(obj, "SSID");
                        }
                        {
                            // settings_wifi_screen_content_panel_input_ssid
                            lv_obj_t *obj = lv_textarea_create(parent_obj);
                            objects.settings_wifi_screen_content_panel_input_ssid = obj;
                            lv_obj_set_pos(obj, 16, 54);
                            lv_obj_set_size(obj, 400, 42);
                            lv_textarea_set_max_length(obj, 128);
                            lv_textarea_set_placeholder_text(obj, "Enter network name");
                            lv_textarea_set_one_line(obj, true);
                            lv_textarea_set_password_mode(obj, false);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_ON_FOCUS|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_x(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_spread(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // settings_wifi_screen_content_panel_label_password
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_wifi_screen_content_panel_label_password = obj;
                            lv_obj_set_pos(obj, 16, 114);
                            lv_obj_set_size(obj, 100, 25);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_label_set_text(obj, "Password");
                        }
                        {
                            // settings_wifi_screen_content_panel_input_password
                            lv_obj_t *obj = lv_textarea_create(parent_obj);
                            objects.settings_wifi_screen_content_panel_input_password = obj;
                            lv_obj_set_pos(obj, 16, 144);
                            lv_obj_set_size(obj, 400, 42);
                            lv_textarea_set_max_length(obj, 128);
                            lv_textarea_set_placeholder_text(obj, "Enter password");
                            lv_textarea_set_one_line(obj, true);
                            lv_textarea_set_password_mode(obj, false);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_ON_FOCUS|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_x(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_spread(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // settings_wifi_screen_content_panel_label_status
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_wifi_screen_content_panel_label_status = obj;
                            lv_obj_set_pos(obj, 16, 214);
                            lv_obj_set_size(obj, 300, 25);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_label_set_text(obj, "Status: Connected");
                        }
                        {
                            // settings_wifi_screen_content_panel_button_connect_
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.settings_wifi_screen_content_panel_button_connect_ = obj;
                            lv_obj_set_pos(obj, 213, 250);
                            lv_obj_set_size(obj, 150, 50);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_ON_FOCUS|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // settings_wifi_screen_content_panel_button_connect_label
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.settings_wifi_screen_content_panel_button_connect_label = obj;
                                    lv_obj_set_pos(obj, -1, 1);
                                    lv_obj_set_size(obj, 68, 18);
                                    lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "Connect");
                                }
                            }
                        }
                        {
                            // settings_wifi_screen_content_panel_button_scan_
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.settings_wifi_screen_content_panel_button_scan_ = obj;
                            lv_obj_set_pos(obj, 16, 250);
                            lv_obj_set_size(obj, 180, 50);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_ON_FOCUS|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // settings_wifi_screen_content_panel_button_scan_label
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.settings_wifi_screen_content_panel_button_scan_label = obj;
                                    lv_obj_set_pos(obj, 11, 6);
                                    lv_obj_set_size(obj, 118, 16);
                                    lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                                    lv_label_set_text(obj, "Scan Networks");
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    tick_screen_settings_wifi_screen();
}

void tick_screen_settings_wifi_screen() {
}

void create_screen_settings_printer_add_screen() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.settings_printer_add_screen = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 800, 480);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // settings_printer_add_screen_top_bar
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.settings_printer_add_screen_top_bar = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 800, 44);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3d3d3d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // settings_printer_add_screen_top_bar_logo
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.settings_printer_add_screen_top_bar_logo = obj;
                    lv_obj_set_pos(obj, 55, 1);
                    lv_obj_set_size(obj, 173, 43);
                    lv_image_set_src(obj, &img_spoolbuddy_logo_dark);
                    lv_image_set_scale(obj, 200);
                    lv_obj_set_style_align(obj, LV_ALIGN_TOP_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // settings_printer_add_screen_top_bar_printer_select
                    lv_obj_t *obj = lv_dropdown_create(parent_obj);
                    objects.settings_printer_add_screen_top_bar_printer_select = obj;
                    lv_obj_set_pos(obj, 325, 2);
                    lv_obj_set_size(obj, 150, 39);
                    lv_dropdown_set_options(obj, "");
                    lv_dropdown_set_selected(obj, 0);
                }
                {
                    // settings_printer_add_screen_top_bar_icon_wifi_signal
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.settings_printer_add_screen_top_bar_icon_wifi_signal = obj;
                    lv_obj_set_pos(obj, 698, 10);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_image_set_src(obj, &img_signal);
                    lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // settings_printer_add_screen_top_bar_icon_notification_bell
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.settings_printer_add_screen_top_bar_icon_notification_bell = obj;
                    lv_obj_set_pos(obj, 662, 11);
                    lv_obj_set_size(obj, 24, 24);
                    lv_image_set_src(obj, &img_bell);
                    lv_image_set_scale(obj, 50);
                }
                {
                    // settings_printer_add_screen_top_bar_label_clock
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.settings_printer_add_screen_top_bar_label_clock = obj;
                    lv_obj_set_pos(obj, 737, 12);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
                {
                    // settings_printer_add_screen_top_bar_icon_back
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.settings_printer_add_screen_top_bar_icon_back = obj;
                    lv_obj_set_pos(obj, 5, 1);
                    lv_obj_set_size(obj, 48, 42);
                    lv_image_set_src(obj, &img_back);
                    lv_image_set_scale(obj, 80);
                }
            }
        }
        {
            // settings_printer_add_screen_panel
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.settings_printer_add_screen_panel = obj;
            lv_obj_set_pos(obj, 0, 44);
            lv_obj_set_size(obj, 800, 436);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // settings_printer_add_screen_panel_panel
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.settings_printer_add_screen_panel_panel = obj;
                    lv_obj_set_pos(obj, 0, 10);
                    lv_obj_set_size(obj, 765, 339);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_arc_width(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_rounded(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff796666), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // settings_printer_add_screen_panel_panel_label_add
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_printer_add_screen_panel_panel_label_add = obj;
                            lv_obj_set_pos(obj, 16, -6);
                            lv_obj_set_size(obj, 300, 30);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_decor(obj, LV_TEXT_DECOR_UNDERLINE, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Add Printer");
                        }
                        {
                            // settings_printer_add_screen_panel_panel_label_name
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_printer_add_screen_panel_panel_label_name = obj;
                            lv_obj_set_pos(obj, 18, 69);
                            lv_obj_set_size(obj, 150, 25);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_label_set_text(obj, "Printer Name");
                        }
                        {
                            // settings_printer_add_screen_panel_panel_input_name
                            lv_obj_t *obj = lv_textarea_create(parent_obj);
                            objects.settings_printer_add_screen_panel_panel_input_name = obj;
                            lv_obj_set_pos(obj, 18, 94);
                            lv_obj_set_size(obj, 200, 42);
                            lv_textarea_set_max_length(obj, 128);
                            lv_textarea_set_placeholder_text(obj, "e.g. X1C-1");
                            lv_textarea_set_one_line(obj, true);
                            lv_textarea_set_password_mode(obj, false);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_ON_FOCUS|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_x(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // settings_printer_add_screen_panel_panel_label_serial
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_printer_add_screen_panel_panel_label_serial = obj;
                            lv_obj_set_pos(obj, 18, 158);
                            lv_obj_set_size(obj, 150, 25);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_label_set_text(obj, "Serial Number");
                        }
                        {
                            // settings_printer_add_screen_panel_panel_input_serial
                            lv_obj_t *obj = lv_textarea_create(parent_obj);
                            objects.settings_printer_add_screen_panel_panel_input_serial = obj;
                            lv_obj_set_pos(obj, 18, 184);
                            lv_obj_set_size(obj, 200, 42);
                            lv_textarea_set_max_length(obj, 128);
                            lv_textarea_set_placeholder_text(obj, "Enter serial number");
                            lv_textarea_set_one_line(obj, true);
                            lv_textarea_set_password_mode(obj, false);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_ON_FOCUS|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_x(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // settings_printer_add_screen_panel_panel_label_code
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_printer_add_screen_panel_panel_label_code = obj;
                            lv_obj_set_pos(obj, 243, 158);
                            lv_obj_set_size(obj, 150, 25);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_label_set_text(obj, "Access Code");
                        }
                        {
                            // settings_printer_add_screen_panel_panel_input_code
                            lv_obj_t *obj = lv_textarea_create(parent_obj);
                            objects.settings_printer_add_screen_panel_panel_input_code = obj;
                            lv_obj_set_pos(obj, 243, 184);
                            lv_obj_set_size(obj, 197, 42);
                            lv_textarea_set_max_length(obj, 128);
                            lv_textarea_set_placeholder_text(obj, "8-digit code");
                            lv_textarea_set_one_line(obj, true);
                            lv_textarea_set_password_mode(obj, false);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_ON_FOCUS|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_x(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // settings_printer_add_screen_panel_panel_button_add
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.settings_printer_add_screen_panel_panel_button_add = obj;
                            lv_obj_set_pos(obj, 18, 247);
                            lv_obj_set_size(obj, 180, 50);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_ON_FOCUS|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // settings_printer_add_screen_panel_panel_button_add_label
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.settings_printer_add_screen_panel_panel_button_add_label = obj;
                                    lv_obj_set_pos(obj, 27, 6);
                                    lv_obj_set_size(obj, 87, 16);
                                    lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                                    lv_label_set_text(obj, "Add Printer");
                                }
                            }
                        }
                        {
                            // settings_printer_add_screen_panel_panel_button_scan
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.settings_printer_add_screen_panel_panel_button_scan = obj;
                            lv_obj_set_pos(obj, 16, 24);
                            lv_obj_set_size(obj, 100, 30);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_ON_FOCUS|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // settings_printer_add_screen_panel_panel_button_scan_label
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.settings_printer_add_screen_panel_panel_button_scan_label = obj;
                                    lv_obj_set_pos(obj, -4, -3);
                                    lv_obj_set_size(obj, 71, 13);
                                    lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                                    lv_label_set_text(obj, "Discover");
                                }
                            }
                        }
                        {
                            // settings_printer_add_screen_panel_panel_label_ip_address
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_printer_add_screen_panel_panel_label_ip_address = obj;
                            lv_obj_set_pos(obj, 243, 69);
                            lv_obj_set_size(obj, 150, 25);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_label_set_text(obj, "IP Address");
                        }
                        {
                            // settings_printer_add_screen_panel_panel_input_ip_address
                            lv_obj_t *obj = lv_textarea_create(parent_obj);
                            objects.settings_printer_add_screen_panel_panel_input_ip_address = obj;
                            lv_obj_set_pos(obj, 243, 94);
                            lv_obj_set_size(obj, 200, 42);
                            lv_textarea_set_max_length(obj, 128);
                            lv_textarea_set_placeholder_text(obj, "e.g. 192.168.1.100");
                            lv_textarea_set_one_line(obj, true);
                            lv_textarea_set_password_mode(obj, false);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_ON_FOCUS|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_x(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                    }
                }
            }
        }
    }
    
    tick_screen_settings_printer_add_screen();
}

void tick_screen_settings_printer_add_screen() {
}

void create_screen_settings_display_screen() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.settings_display_screen = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 800, 480);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // settings_display_screen_top_bar
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.settings_display_screen_top_bar = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 800, 44);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3d3d3d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // settings_display_screen_top_bar_icon_back
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.settings_display_screen_top_bar_icon_back = obj;
                    lv_obj_set_pos(obj, 5, 1);
                    lv_obj_set_size(obj, 48, 42);
                    lv_image_set_src(obj, &img_back);
                    lv_image_set_scale(obj, 80);
                }
                {
                    // settings_display_screen_top_bar_logo
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.settings_display_screen_top_bar_logo = obj;
                    lv_obj_set_pos(obj, 55, 1);
                    lv_obj_set_size(obj, 173, 43);
                    lv_image_set_src(obj, &img_spoolbuddy_logo_dark);
                    lv_image_set_scale(obj, 200);
                    lv_obj_set_style_align(obj, LV_ALIGN_TOP_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // settings_display_screen_top_bar_printer_select
                    lv_obj_t *obj = lv_dropdown_create(parent_obj);
                    objects.settings_display_screen_top_bar_printer_select = obj;
                    lv_obj_set_pos(obj, 325, 2);
                    lv_obj_set_size(obj, 150, 39);
                    lv_dropdown_set_options(obj, "H2D-1\nX1C-1\nX1C-2");
                    lv_dropdown_set_selected(obj, 0);
                }
                {
                    // settings_display_screen_top_bar_icon_wifi_signal
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.settings_display_screen_top_bar_icon_wifi_signal = obj;
                    lv_obj_set_pos(obj, 698, 10);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_image_set_src(obj, &img_signal);
                    lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // settings_display_screen_top_bar_icon_notification_bell
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.settings_display_screen_top_bar_icon_notification_bell = obj;
                    lv_obj_set_pos(obj, 662, 11);
                    lv_obj_set_size(obj, 24, 24);
                    lv_image_set_src(obj, &img_bell);
                    lv_image_set_scale(obj, 50);
                }
                {
                    // settings_display_screen_top_bar_label_clock
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.settings_display_screen_top_bar_label_clock = obj;
                    lv_obj_set_pos(obj, 737, 12);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "10:23");
                }
            }
        }
        {
            // settings_display_screen_content
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.settings_display_screen_content = obj;
            lv_obj_set_pos(obj, 0, 44);
            lv_obj_set_size(obj, 800, 436);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // settings_display_screen_content_panel
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.settings_display_screen_content_panel = obj;
                    lv_obj_set_pos(obj, 0, 10);
                    lv_obj_set_size(obj, 765, 217);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_arc_width(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_rounded(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff796666), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // settings_display_screen_content_panel_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_display_screen_content_panel_label = obj;
                            lv_obj_set_pos(obj, 18, -7);
                            lv_obj_set_size(obj, 300, 30);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_text_decor(obj, LV_TEXT_DECOR_UNDERLINE, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Display Settings");
                        }
                        {
                            // settings_display_screen_content_panel_label_resolution
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_display_screen_content_panel_label_resolution = obj;
                            lv_obj_set_pos(obj, 18, 32);
                            lv_obj_set_size(obj, 87, 25);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_label_set_text(obj, "Resolution:");
                        }
                        {
                            // settings_display_screen_content_panel_label_panel
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_display_screen_content_panel_label_panel = obj;
                            lv_obj_set_pos(obj, 18, 63);
                            lv_obj_set_size(obj, 87, 25);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_label_set_text(obj, "Panel:");
                        }
                        {
                            // settings_display_screen_content_panel_label_brightness
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_display_screen_content_panel_label_brightness = obj;
                            lv_obj_set_pos(obj, 18, 94);
                            lv_obj_set_size(obj, 150, 25);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_label_set_text(obj, "Brightness");
                        }
                        {
                            // settings_display_screen_content_panel_label_timeout
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_display_screen_content_panel_label_timeout = obj;
                            lv_obj_set_pos(obj, 16, 139);
                            lv_obj_set_size(obj, 150, 25);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_label_set_text(obj, "Screen Timeout");
                        }
                        {
                            // settings_display_screen_content_panel_label_brightness_slider
                            lv_obj_t *obj = lv_slider_create(parent_obj);
                            objects.settings_display_screen_content_panel_label_brightness_slider = obj;
                            lv_obj_set_pos(obj, 18, 119);
                            lv_obj_set_size(obj, 150, 10);
                            lv_slider_set_value(obj, 25, LV_ANIM_OFF);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_x(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // settings_display_screen_content_panel_label_timeout_slider
                            lv_obj_t *obj = lv_slider_create(parent_obj);
                            objects.settings_display_screen_content_panel_label_timeout_slider = obj;
                            lv_obj_set_pos(obj, 16, 164);
                            lv_obj_set_size(obj, 150, 10);
                            lv_slider_set_range(obj, 0, 900);
                            lv_slider_set_value(obj, 300, LV_ANIM_OFF);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_x(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // settings_display_screen_content_panel_label_resolution_value
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_display_screen_content_panel_label_resolution_value = obj;
                            lv_obj_set_pos(obj, 105, 32);
                            lv_obj_set_size(obj, 87, 25);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_label_set_text(obj, "");
                        }
                        {
                            // settings_display_screen_content_panel_label_panel_value
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_display_screen_content_panel_label_panel_value = obj;
                            lv_obj_set_pos(obj, 105, 63);
                            lv_obj_set_size(obj, 87, 25);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_label_set_text(obj, "");
                        }
                    }
                }
            }
        }
    }
    
    tick_screen_settings_display_screen();
}

void tick_screen_settings_display_screen() {
}

void create_screen_settings_update_screen() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.settings_update_screen = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 800, 480);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // settings_update_screen_top_bar
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.settings_update_screen_top_bar = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 800, 44);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3d3d3d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // settings_update_screen_top_bar_icon_back
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.settings_update_screen_top_bar_icon_back = obj;
                    lv_obj_set_pos(obj, 5, 1);
                    lv_obj_set_size(obj, 48, 42);
                    lv_image_set_src(obj, &img_back);
                    lv_image_set_scale(obj, 80);
                }
                {
                    // settings_update_screen_top_bar_logo
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.settings_update_screen_top_bar_logo = obj;
                    lv_obj_set_pos(obj, 55, 1);
                    lv_obj_set_size(obj, 173, 43);
                    lv_image_set_src(obj, &img_spoolbuddy_logo_dark);
                    lv_image_set_scale(obj, 200);
                    lv_obj_set_style_align(obj, LV_ALIGN_TOP_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // settings_update_screen_top_bar_printer_select
                    lv_obj_t *obj = lv_dropdown_create(parent_obj);
                    objects.settings_update_screen_top_bar_printer_select = obj;
                    lv_obj_set_pos(obj, 325, 2);
                    lv_obj_set_size(obj, 150, 39);
                    lv_dropdown_set_options(obj, "H2D-1\nX1C-1\nX1C-2");
                    lv_dropdown_set_selected(obj, 0);
                }
                {
                    // settings_update_screen_top_bar_icon_wifi_signal
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.settings_update_screen_top_bar_icon_wifi_signal = obj;
                    lv_obj_set_pos(obj, 698, 10);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_image_set_src(obj, &img_signal);
                    lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // settings_update_screen_top_bar_icon_notification_bell
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.settings_update_screen_top_bar_icon_notification_bell = obj;
                    lv_obj_set_pos(obj, 662, 11);
                    lv_obj_set_size(obj, 24, 24);
                    lv_image_set_src(obj, &img_bell);
                    lv_image_set_scale(obj, 50);
                }
                {
                    // settings_update_screen_top_bar_label_clock
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.settings_update_screen_top_bar_label_clock = obj;
                    lv_obj_set_pos(obj, 737, 12);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "10:23");
                }
            }
        }
        {
            // settings_update_screen_top_bar_content
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.settings_update_screen_top_bar_content = obj;
            lv_obj_set_pos(obj, 0, 44);
            lv_obj_set_size(obj, 800, 436);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // settings_update_screen_top_bar_content_panel
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.settings_update_screen_top_bar_content_panel = obj;
                    lv_obj_set_pos(obj, 0, 10);
                    lv_obj_set_size(obj, 765, 232);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_arc_width(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_rounded(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_x(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_ofs_y(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff796666), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // settings_update_screen_top_bar_content_panel_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_update_screen_top_bar_content_panel_label = obj;
                            lv_obj_set_pos(obj, 18, -9);
                            lv_obj_set_size(obj, 300, 30);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_label_set_text(obj, "Firmware Update");
                        }
                        {
                            // settings_update_screen_top_bar_content_panel_label_version
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_update_screen_top_bar_content_panel_label_version = obj;
                            lv_obj_set_pos(obj, 18, 41);
                            lv_obj_set_size(obj, 136, 25);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_label_set_text(obj, "Current Version:");
                        }
                        {
                            // settings_update_screen_top_bar_content_panel_label_latest
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_update_screen_top_bar_content_panel_label_latest = obj;
                            lv_obj_set_pos(obj, 18, 66);
                            lv_obj_set_size(obj, 136, 25);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_label_set_text(obj, "Latest Version:");
                        }
                        {
                            // settings_update_screen_top_bar_content_panel_label_status
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_update_screen_top_bar_content_panel_label_status = obj;
                            lv_obj_set_pos(obj, 18, 95);
                            lv_obj_set_size(obj, 138, 25);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_label_set_text(obj, "Status:");
                        }
                        {
                            // settings_update_screen_top_bar_content_panel_button_check
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.settings_update_screen_top_bar_content_panel_button_check = obj;
                            lv_obj_set_pos(obj, 16, 136);
                            lv_obj_set_size(obj, 152, 50);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_ON_FOCUS|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // settings_update_screen_top_bar_content_panel_button_check_label
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.settings_update_screen_top_bar_content_panel_button_check_label = obj;
                                    lv_obj_set_pos(obj, -14, 7);
                                    lv_obj_set_size(obj, 140, 14);
                                    lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "Check for Updates");
                                }
                            }
                        }
                        {
                            // settings_update_screen_top_bar_content_panel_label_version_value
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_update_screen_top_bar_content_panel_label_version_value = obj;
                            lv_obj_set_pos(obj, 156, 41);
                            lv_obj_set_size(obj, 220, 25);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_label_set_text(obj, "0.1b");
                        }
                        {
                            // settings_update_screen_top_bar_content_panel_label_latest_value
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_update_screen_top_bar_content_panel_label_latest_value = obj;
                            lv_obj_set_pos(obj, 156, 66);
                            lv_obj_set_size(obj, 220, 25);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_label_set_text(obj, "Checking...");
                        }
                        {
                            // settings_update_screen_top_bar_content_panel_label_status_value
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_update_screen_top_bar_content_panel_label_status_value = obj;
                            lv_obj_set_pos(obj, 156, 95);
                            lv_obj_set_size(obj, 220, 25);
                            lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_label_set_text(obj, "Up to date");
                        }
                    }
                }
            }
        }
    }
    
    tick_screen_settings_update_screen();
}

void tick_screen_settings_update_screen() {
}



typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_main_screen,
    tick_screen_ams_overview,
    tick_screen_scan_result,
    tick_screen_spool_details,
    tick_screen_settings_screen,
    tick_screen_settings_wifi_screen,
    tick_screen_settings_printer_add_screen,
    tick_screen_settings_display_screen,
    tick_screen_settings_update_screen,
};
void tick_screen(int screen_index) {
    tick_screen_funcs[screen_index]();
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen_funcs[screenId - 1]();
}

void create_screens() {
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), true, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    
    create_screen_main_screen();
    create_screen_ams_overview();
    create_screen_scan_result();
    create_screen_spool_details();
    create_screen_settings_screen();
    create_screen_settings_wifi_screen();
    create_screen_settings_printer_add_screen();
    create_screen_settings_display_screen();
    create_screen_settings_update_screen();
}
