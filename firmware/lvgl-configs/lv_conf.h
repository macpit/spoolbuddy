/**
 * LVGL 9.x Configuration for SpoolBuddy
 * Display: 800x480 RGB565
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/

/* Color depth: 16 bit for RGB565 display */
#define LV_COLOR_DEPTH 16

/*====================
   MEMORY SETTINGS
 *====================*/

/* Use ESP-IDF heap allocator which can use PSRAM */
#define LV_USE_STDLIB_MALLOC LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_CLIB

/* Memory pool size for internal allocator (used if not CLIB) */
#define LV_MEM_SIZE (180U * 1024U)

/*====================
   HAL SETTINGS
 *====================*/

/* Default display refresh period in milliseconds */
#define LV_DEF_REFR_PERIOD 10

/* Default Dot Per Inch */
#define LV_DPI_DEF 130

/* Enable OS abstraction layer */
#define LV_USE_OS LV_OS_NONE

/*====================
   FEATURE CONFIGURATION
 *====================*/

/* Drawing engine features */
#define LV_USE_DRAW_SW 1

/* Enable vector graphics */
#define LV_USE_VECTOR_GRAPHIC 0

/* Enable matrix transforms */
#define LV_USE_MATRIX 0

/*====================
   LOGGING
 *====================*/

#define LV_USE_LOG 1
#if LV_USE_LOG
    #define LV_LOG_LEVEL LV_LOG_LEVEL_INFO
    #define LV_LOG_PRINTF 1
#endif

/*====================
   ASSERTS
 *====================*/

#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0

/*====================
   FONT USAGE
 *====================*/

/* Montserrat fonts - enable what we need */
#define LV_FONT_MONTSERRAT_8     0
#define LV_FONT_MONTSERRAT_10    1
#define LV_FONT_MONTSERRAT_12    1
#define LV_FONT_MONTSERRAT_14    1
#define LV_FONT_MONTSERRAT_16    1
#define LV_FONT_MONTSERRAT_18    1
#define LV_FONT_MONTSERRAT_20    1
#define LV_FONT_MONTSERRAT_22    0
#define LV_FONT_MONTSERRAT_24    1
#define LV_FONT_MONTSERRAT_26    0
#define LV_FONT_MONTSERRAT_28    1
#define LV_FONT_MONTSERRAT_30    0
#define LV_FONT_MONTSERRAT_32    0
#define LV_FONT_MONTSERRAT_34    0
#define LV_FONT_MONTSERRAT_36    0
#define LV_FONT_MONTSERRAT_38    0
#define LV_FONT_MONTSERRAT_40    0
#define LV_FONT_MONTSERRAT_42    0
#define LV_FONT_MONTSERRAT_44    0
#define LV_FONT_MONTSERRAT_46    0
#define LV_FONT_MONTSERRAT_48    0

/* Other built-in fonts */
#define LV_FONT_MONTSERRAT_12_SUBPX      0
#define LV_FONT_MONTSERRAT_28_COMPRESSED 0
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW 0
#define LV_FONT_SIMSUN_16_CJK            0
#define LV_FONT_UNSCII_8                 0
#define LV_FONT_UNSCII_16                0

/* Default font */
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/* Enable FreeType */
#define LV_USE_FREETYPE 0

/*====================
   TEXT SETTINGS
 *====================*/

#define LV_TXT_ENC LV_TXT_ENC_UTF8
#define LV_TXT_BREAK_CHARS " ,.;:-_)]}"
#define LV_TXT_LINE_BREAK_LONG_LEN 0
#define LV_TXT_COLOR_CMD "#"

/*====================
   WIDGETS
 *====================*/

#define LV_USE_ARC        1
#define LV_USE_BAR        1
#define LV_USE_BUTTON     1
#define LV_USE_BUTTONMATRIX 1
#define LV_USE_CANVAS     1
#define LV_USE_CHECKBOX   1
#define LV_USE_DROPDOWN   1
#define LV_USE_IMAGE      1
#define LV_USE_LABEL      1
#define LV_USE_LINE       1
#define LV_USE_ROLLER     1
#define LV_USE_SLIDER     1
#define LV_USE_SWITCH     1
#define LV_USE_TEXTAREA   1
#define LV_USE_TABLE      1
#define LV_USE_SCALE      1

/* Extra widgets */
#define LV_USE_ANIMIMAGE  0
#define LV_USE_CALENDAR   0
#define LV_USE_CHART      1
#define LV_USE_COLORWHEEL 0
#define LV_USE_IMAGEBUTTON 1
#define LV_USE_KEYBOARD   1
#define LV_USE_LED        1
#define LV_USE_LIST       1
#define LV_USE_MENU       0
#define LV_USE_METER      1
#define LV_USE_MSGBOX     1
#define LV_USE_SPAN       0
#define LV_USE_SPINBOX    0
#define LV_USE_SPINNER    1
#define LV_USE_TABVIEW    0
#define LV_USE_TILEVIEW   0
#define LV_USE_WIN        0

/*====================
   THEMES
 *====================*/

#define LV_USE_THEME_DEFAULT 1
#define LV_USE_THEME_SIMPLE  1

/*====================
   LAYOUTS
 *====================*/

#define LV_USE_FLEX 1
#define LV_USE_GRID 1

/*====================
   OTHERS
 *====================*/

/* Performance monitor - disabled for production */
#define LV_USE_PERF_MONITOR 0

/* Memory monitor - disabled for production */
#define LV_USE_MEM_MONITOR 0

/* File system */
#define LV_USE_FS_STDIO 0
#define LV_USE_FS_POSIX 0
#define LV_USE_FS_WIN32 0
#define LV_USE_FS_FATFS 0

/* Image decoders */
#define LV_USE_LODEPNG 0
#define LV_USE_LIBPNG  0
#define LV_USE_BMP     0
#define LV_USE_SJPG    0
#define LV_USE_GIF     0
#define LV_USE_RLE     0

/* Others */
#define LV_USE_SNAPSHOT    0
#define LV_USE_SYSMON      0
#define LV_USE_PROFILER    0
#define LV_USE_MONKEY      0
#define LV_USE_GRIDNAV     0
#define LV_USE_FRAGMENT    0
#define LV_USE_OBSERVER    1
#define LV_USE_IME_PINYIN  0

#define LV_BUILD_EXAMPLES  0

/*====================
   CUSTOM STUBS
 *====================*/

/* lv_deinit is not generated when LV_MEM_CUSTOM=1, but Rust binding needs it.
 * We provide a stub implementation in components/lvgl_stubs/lv_deinit_stub.c */
#if LV_MEM_CUSTOM == 1
    void lv_deinit(void);
#endif

#endif /* LV_CONF_H */
