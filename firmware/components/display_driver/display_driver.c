/**
 * SpoolBuddy Display Driver for CrowPanel Advance 7.0"
 * 800x480 RGB LCD with GT911 touch controller
 * Uses LVGL 9.x and ESP-IDF RGB LCD driver
 */

#include "display_driver.h"
#include "lvgl.h"
#include "ui.h"  // EEZ generated UI

#include <string.h>
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "display";

// Display dimensions
#define DISPLAY_WIDTH   800
#define DISPLAY_HEIGHT  480

// CrowPanel Advance 7.0" RGB pins
#define PIN_PCLK    39
#define PIN_HSYNC   40
#define PIN_VSYNC   41
#define PIN_DE      42

// RGB565 data pins (from working firmware)
#define PIN_B0      21
#define PIN_B1      47
#define PIN_B2      48
#define PIN_B3      45
#define PIN_B4      38
#define PIN_G0      9
#define PIN_G1      10
#define PIN_G2      11
#define PIN_G3      12
#define PIN_G4      13
#define PIN_G5      14
#define PIN_R0      7
#define PIN_R1      17
#define PIN_R2      18
#define PIN_R3      3
#define PIN_R4      46

// Touch I2C pins (directly connected on CrowPanel)
#define TOUCH_I2C_PORT      I2C_NUM_0
#define TOUCH_I2C_SDA       15
#define TOUCH_I2C_SCL       16
#define GT911_ADDR          0x5D

// Backlight control - CrowPanel uses GPIO1 and GPIO2
#define PIN_BACKLIGHT1      1
#define PIN_BACKLIGHT2      2

// Panel handle
static esp_lcd_panel_handle_t panel_handle = NULL;

// LVGL display
static lv_display_t *display = NULL;

// Draw buffers (in internal SRAM for reliability)
// For RGB565, each pixel is 2 bytes. LVGL 9.x uses byte buffers.
#define DRAW_BUF_LINES  40
static uint8_t draw_buf1[DISPLAY_WIDTH * DRAW_BUF_LINES * 2];  // RGB565 = 2 bytes/pixel
static uint8_t draw_buf2[DISPLAY_WIDTH * DRAW_BUF_LINES * 2];  // RGB565 = 2 bytes/pixel

// Touch state
static bool touch_pressed = false;
static int16_t touch_x = 0;
static int16_t touch_y = 0;

// Tick timer
static uint32_t tick_start = 0;

/**
 * LVGL flush callback - copies rendered pixels to display
 */
static int flush_count = 0;

// Expose for debugging
int get_flush_count(void) { return flush_count; }

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    flush_count++;
    // Always log flushes after the first 5 if they're for a new screen (y1 == 0 could indicate full redraw)
    bool is_likely_full_redraw = (area->y1 == 0 && area->x1 == 0);
    if (flush_count <= 10 || is_likely_full_redraw) {
        ESP_LOGI(TAG, "flush_cb #%d: area=(%ld,%ld)-(%ld,%ld), panel=%p, active=%p",
                 flush_count, (long)area->x1, (long)area->y1, (long)area->x2, (long)area->y2,
                 panel_handle, lv_screen_active());
    }

    if (panel_handle == NULL) {
        ESP_LOGE(TAG, "flush_cb: panel_handle is NULL!");
        lv_display_flush_ready(disp);
        return;
    }

    // Get the panel's framebuffer
    void *fb = NULL;
    esp_lcd_rgb_panel_get_frame_buffer(panel_handle, 1, &fb);

    if (flush_count <= 5) {
        ESP_LOGI(TAG, "flush_cb: fb=%p", fb);
    }

    if (fb == NULL) {
        ESP_LOGE(TAG, "flush_cb: framebuffer is NULL!");
        lv_display_flush_ready(disp);
        return;
    }

    // Copy the rendered area to framebuffer
    uint16_t *fb16 = (uint16_t *)fb;
    uint16_t *src = (uint16_t *)px_map;
    int width = area->x2 - area->x1 + 1;
    int height = area->y2 - area->y1 + 1;

    if (flush_count <= 5) {
        ESP_LOGI(TAG, "flush_cb #%d: copying %d rows, width=%d, src=%p",
                 flush_count, height, width, src);
    }

    for (int y = area->y1; y <= area->y2; y++) {
        uint16_t *dst_row = fb16 + y * DISPLAY_WIDTH + area->x1;
        memcpy(dst_row, src, width * sizeof(uint16_t));
        src += width;

        // Log every 10th row for first few flushes
        if (flush_count <= 3 && (y - area->y1) % 10 == 0) {
            ESP_LOGI(TAG, "flush_cb #%d: row %d done", flush_count, y);
        }
    }

    if (flush_count <= 5) {
        ESP_LOGI(TAG, "flush_cb #%d: memcpy done, calling flush_ready", flush_count);
    }

    lv_display_flush_ready(disp);

    if (flush_count <= 5) {
        ESP_LOGI(TAG, "flush_cb #%d: flush_ready returned", flush_count);
    }
}

/**
 * Read GT911 touch data
 */
static bool read_gt911_touch(int16_t *x, int16_t *y)
{
    uint8_t buf[7];
    uint8_t reg_addr[2] = {0x81, 0x4E};  // Touch status register

    // Write register address
    if (i2c_master_write_to_device(TOUCH_I2C_PORT, GT911_ADDR, reg_addr, 2, 10) != ESP_OK) {
        return false;
    }

    // Read touch data
    if (i2c_master_read_from_device(TOUCH_I2C_PORT, GT911_ADDR, buf, 7, 10) != ESP_OK) {
        return false;
    }

    // Check if touch is valid
    uint8_t status = buf[0];
    if ((status & 0x80) == 0 || (status & 0x0F) == 0) {
        // Clear status flag
        uint8_t clear[3] = {0x81, 0x4E, 0x00};
        i2c_master_write_to_device(TOUCH_I2C_PORT, GT911_ADDR, clear, 3, 10);
        return false;
    }

    // Extract coordinates (little endian)
    *x = buf[2] | (buf[3] << 8);
    *y = buf[4] | (buf[5] << 8);

    // Bounds check
    if (*x >= DISPLAY_WIDTH) *x = DISPLAY_WIDTH - 1;
    if (*y >= DISPLAY_HEIGHT) *y = DISPLAY_HEIGHT - 1;

    // Clear status flag
    uint8_t clear[3] = {0x81, 0x4E, 0x00};
    i2c_master_write_to_device(TOUCH_I2C_PORT, GT911_ADDR, clear, 3, 10);

    return true;
}

/**
 * LVGL touch input callback
 */
static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    if (read_gt911_touch(&touch_x, &touch_y)) {
        data->point.x = touch_x;
        data->point.y = touch_y;
        data->state = LV_INDEV_STATE_PRESSED;
        touch_pressed = true;
    } else {
        data->point.x = touch_x;  // Report last known position
        data->point.y = touch_y;
        data->state = LV_INDEV_STATE_RELEASED;
        touch_pressed = false;
    }
}

/**
 * Initialize I2C for touch controller
 */
static esp_err_t init_touch_i2c(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = TOUCH_I2C_SDA,
        .scl_io_num = TOUCH_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };

    esp_err_t err = i2c_param_config(TOUCH_I2C_PORT, &conf);
    if (err != ESP_OK) return err;

    return i2c_driver_install(TOUCH_I2C_PORT, conf.mode, 0, 0, 0);
}

/**
 * Initialize RGB LCD panel
 */
static esp_err_t init_rgb_panel(void)
{
    ESP_LOGI(TAG, "=== RGB PANEL INIT ===");
    ESP_LOGI(TAG, "Resolution: %dx%d", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    ESP_LOGI(TAG, "Pixel clock: 14MHz");
    ESP_LOGI(TAG, "PCLK=%d HSYNC=%d VSYNC=%d DE=%d", PIN_PCLK, PIN_HSYNC, PIN_VSYNC, PIN_DE);
    ESP_LOGI(TAG, "B: %d,%d,%d,%d,%d", PIN_B0, PIN_B1, PIN_B2, PIN_B3, PIN_B4);
    ESP_LOGI(TAG, "G: %d,%d,%d,%d,%d,%d", PIN_G0, PIN_G1, PIN_G2, PIN_G3, PIN_G4, PIN_G5);
    ESP_LOGI(TAG, "R: %d,%d,%d,%d,%d", PIN_R0, PIN_R1, PIN_R2, PIN_R3, PIN_R4);

    esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .timings = {
            .pclk_hz = 14000000,  // 14MHz pixel clock (from working firmware)
            .h_res = DISPLAY_WIDTH,
            .v_res = DISPLAY_HEIGHT,
            .hsync_pulse_width = 48,
            .hsync_back_porch = 20,
            .hsync_front_porch = 40,
            .vsync_pulse_width = 4,
            .vsync_back_porch = 20,
            .vsync_front_porch = 20,
            .flags = {
                .pclk_active_neg = true,
            },
        },
        .data_width = 16,
        .num_fbs = 1,
        .bounce_buffer_size_px = 10 * DISPLAY_WIDTH,  // Bounce buffer for PSRAM
        .psram_trans_align = 64,
        .hsync_gpio_num = PIN_HSYNC,
        .vsync_gpio_num = PIN_VSYNC,
        .de_gpio_num = PIN_DE,
        .pclk_gpio_num = PIN_PCLK,
        .disp_gpio_num = -1,
        .data_gpio_nums = {
            PIN_B0, PIN_B1, PIN_B2, PIN_B3, PIN_B4,
            PIN_G0, PIN_G1, PIN_G2, PIN_G3, PIN_G4, PIN_G5,
            PIN_R0, PIN_R1, PIN_R2, PIN_R3, PIN_R4,
        },
        .flags = {
            .fb_in_psram = true,
        },
    };

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    // Turn on display (from working firmware)
    esp_err_t err = esp_lcd_panel_disp_on_off(panel_handle, true);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "esp_lcd_panel_disp_on_off failed: %d (continuing anyway)", err);
    }

    ESP_LOGI(TAG, "RGB panel initialized");
    return ESP_OK;
}

/**
 * Initialize backlight
 * CrowPanel uses both GPIO1 and GPIO2 for backlight control
 * Also tries I2C commands for v1.3+ boards
 */
static void init_backlight(void)
{
    ESP_LOGI(TAG, "=== BACKLIGHT INIT START ===");

    // GPIO1 backlight
    ESP_LOGI(TAG, "Configuring GPIO%d as output...", PIN_BACKLIGHT1);
    gpio_config_t io_conf1 = {
        .pin_bit_mask = (1ULL << PIN_BACKLIGHT1),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err1 = gpio_config(&io_conf1);
    ESP_LOGI(TAG, "GPIO%d config result: %d", PIN_BACKLIGHT1, err1);
    esp_err_t err1b = gpio_set_level(PIN_BACKLIGHT1, 1);
    ESP_LOGI(TAG, "GPIO%d set HIGH result: %d", PIN_BACKLIGHT1, err1b);

    // GPIO2 backlight
    ESP_LOGI(TAG, "Configuring GPIO%d as output...", PIN_BACKLIGHT2);
    gpio_config_t io_conf2 = {
        .pin_bit_mask = (1ULL << PIN_BACKLIGHT2),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err2 = gpio_config(&io_conf2);
    ESP_LOGI(TAG, "GPIO%d config result: %d", PIN_BACKLIGHT2, err2);
    esp_err_t err2b = gpio_set_level(PIN_BACKLIGHT2, 1);
    ESP_LOGI(TAG, "GPIO%d set HIGH result: %d", PIN_BACKLIGHT2, err2b);

    ESP_LOGI(TAG, "=== BACKLIGHT INIT DONE ===");
}

/**
 * LVGL tick callback
 */
static uint32_t tick_get_cb(void)
{
    return (uint32_t)((esp_timer_get_time() - tick_start) / 1000);
}

/**
 * Initialize display, touch, and LVGL
 */
int display_init(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "SpoolBuddy Display Driver Init");
    ESP_LOGI(TAG, "LVGL 9.x + EEZ Studio UI");
    ESP_LOGI(TAG, "========================================");

    // Record start time for tick
    tick_start = esp_timer_get_time();

    // Initialize backlight
    init_backlight();
    vTaskDelay(pdMS_TO_TICKS(200));  // 200ms delay like working firmware

    // Initialize I2C for touch
    esp_err_t err = init_touch_i2c();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Touch I2C init failed: %d", err);
    } else {
        ESP_LOGI(TAG, "Touch I2C initialized");

        // Try I2C backlight commands (for v1.3+ boards)
        ESP_LOGI(TAG, "=== I2C BACKLIGHT COMMANDS ===");
        uint8_t brightness = 0xFF;
        esp_err_t i2c_err;

        // Scan I2C bus first
        ESP_LOGI(TAG, "Scanning I2C bus...");
        for (uint8_t addr = 0x08; addr < 0x78; addr++) {
            uint8_t dummy;
            if (i2c_master_read_from_device(TOUCH_I2C_PORT, addr, &dummy, 1, 10) == ESP_OK) {
                ESP_LOGI(TAG, "  Found device at 0x%02X", addr);
            }
        }

        // Try 0x30 (STC8H1K28 on v1.3+)
        i2c_err = i2c_master_write_to_device(TOUCH_I2C_PORT, 0x30, &brightness, 1, 100);
        ESP_LOGI(TAG, "I2C 0x30 write result: %d", i2c_err);
        // Try XL9535 GPIO expander at 0x20
        uint8_t xl9535_cfg[] = {0x06, 0x00};  // Config port 0 as output
        uint8_t xl9535_out[] = {0x02, 0xFF};  // Set all outputs high
        i2c_err = i2c_master_write_to_device(TOUCH_I2C_PORT, 0x20, xl9535_cfg, 2, 100);
        ESP_LOGI(TAG, "I2C 0x20 cfg result: %d", i2c_err);
        i2c_err = i2c_master_write_to_device(TOUCH_I2C_PORT, 0x20, xl9535_out, 2, 100);
        ESP_LOGI(TAG, "I2C 0x20 out result: %d", i2c_err);
        // Try 0x24
        i2c_err = i2c_master_write_to_device(TOUCH_I2C_PORT, 0x24, &brightness, 1, 100);
        ESP_LOGI(TAG, "I2C 0x24 write result: %d", i2c_err);
        ESP_LOGI(TAG, "=== I2C BACKLIGHT DONE ===");
    }

    // Initialize RGB panel
    err = init_rgb_panel();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "RGB panel init failed: %d", err);
        return -1;
    }

    // Initialize LVGL
    ESP_LOGI(TAG, "Initializing LVGL 9.x...");
    lv_init();
    lv_tick_set_cb(tick_get_cb);

    // Create display
    display = lv_display_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    if (display == NULL) {
        ESP_LOGE(TAG, "Failed to create LVGL display");
        return -2;
    }

    // Set color format (RGB565)
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);

    // Set draw buffers
    lv_display_set_buffers(display, draw_buf1, draw_buf2,
                           sizeof(draw_buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    // Set flush callback
    lv_display_set_flush_cb(display, flush_cb);

    ESP_LOGI(TAG, "LVGL display created");

    // Create touch input device
    lv_indev_t *indev = lv_indev_create();
    if (indev != NULL) {
        lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(indev, touch_read_cb);
        ESP_LOGI(TAG, "Touch input device created");
    }

    // Initialize EEZ UI
    ESP_LOGI(TAG, "Initializing EEZ UI...");
    ui_init();
    ESP_LOGI(TAG, "EEZ UI initialized");

    ESP_LOGI(TAG, "Display driver init complete!");
    return 0;
}

/**
 * Run LVGL timer handler
 */
static int tick_count = 0;
static int flush_before_timer = 0;

void display_tick(void)
{
    tick_count++;
    flush_before_timer = flush_count;

    if (tick_count <= 10 || tick_count % 200 == 0) {
        ESP_LOGI(TAG, "tick #%d before lv_timer_handler, flush=%d, active=%p",
                 tick_count, flush_count, lv_screen_active());
    }

    lv_timer_handler();

    // Log if any flushes happened during timer_handler
    int flushes_this_tick = flush_count - flush_before_timer;
    if (tick_count <= 10 || tick_count % 200 == 0 || flushes_this_tick > 0) {
        ESP_LOGI(TAG, "tick #%d after lv_timer, flush=%d (+%d this tick), active=%p",
                 tick_count, flush_count, flushes_this_tick, lv_screen_active());
    }

    ui_tick();

    if (tick_count <= 10 || tick_count % 200 == 0) {
        ESP_LOGI(TAG, "tick #%d after ui_tick", tick_count);
    }
}

/**
 * Get elapsed time in milliseconds
 */
uint32_t display_get_tick_ms(void)
{
    return tick_get_cb();
}
