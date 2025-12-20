/**
 * LVGL Driver Configuration for SpoolBuddy PC Simulator
 * Using SDL2 for display and input
 */

#ifndef LV_DRV_CONF_H
#define LV_DRV_CONF_H

#include "lv_conf.h"

/*********************
 * DELAY INTERFACE
 *********************/
#define LV_DRV_DELAY_INCLUDE <stdint.h>
#define LV_DRV_DELAY_US(us)
#define LV_DRV_DELAY_MS(ms)

/*********************
 * DISPLAY INTERFACE
 *********************/

/* No SPI display */
#define LV_DRV_DISP_SPI_CS(val)
#define LV_DRV_DISP_SPI_WR_BYTE(data)
#define LV_DRV_DISP_SPI_WR_ARRAY(adr, n)

/*********************
 * DISPLAY DRIVERS
 *********************/

/* SDL for PC simulator */
#define USE_SDL             1
#if USE_SDL
    #define SDL_HOR_RES     800
    #define SDL_VER_RES     480
    #define SDL_ZOOM        1
    #define SDL_INCLUDE_PATH <SDL2/SDL.h>
    #define SDL_DOUBLE_BUFFERED 0
#endif

/* Disable other display drivers */
#define USE_MONITOR         0
#define USE_WINDOWS         0
#define USE_GTK             0
#define USE_SDL_GPU         0
#define USE_FBDEV           0
#define USE_DRM             0
#define USE_R61581          0
#define USE_ST7565          0
#define USE_GC9A01          0
#define USE_SSD1963         0
#define USE_ILI9341         0
#define USE_SHARP_MIP       0
#define USE_UC1610          0

/**********************
 * INPUT DEVICES
 **********************/

/* SDL mouse and keyboard */
#define USE_MOUSE           1
#define USE_MOUSEWHEEL      1
#define USE_KEYBOARD        1

/* Disable other input drivers */
#define USE_XPT2046         0
#define USE_FT5406EE8       0
#define USE_FT6X36          0
#define USE_AD_TOUCH        0
#define USE_EVDEV           0
#define USE_LIBINPUT        0

#endif /* LV_DRV_CONF_H */
