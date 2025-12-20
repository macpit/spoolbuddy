//! SpoolBuddy LVGL PC Simulator
//!
//! Runs the same LVGL UI as the firmware, but on desktop with SDL2.
//!
//! # Usage
//! ```bash
//! # Interactive mode (requires display)
//! DEP_LV_CONFIG_PATH=$(pwd)/include cargo run --release
//!
//! # Headless mode - renders to BMP file
//! DEP_LV_CONFIG_PATH=$(pwd)/include cargo run --release -- --headless
//! ```

use cstr_core::CString;
use log::info;
use std::time::Instant;
use std::io::Write;

// Display dimensions (same as firmware)
const WIDTH: i16 = 800;
const HEIGHT: i16 = 480;

// Global framebuffer for headless mode
static mut FRAMEBUFFER: [u16; (800 * 480) as usize] = [0u16; (800 * 480) as usize];

// SpoolBuddy logo (97x24)
const LOGO_WIDTH: u32 = 97;
const LOGO_HEIGHT: u32 = 24;
static LOGO_DATA: &[u8] = include_bytes!("../assets/logo.bin");

// Bell icon (20x20)
const BELL_WIDTH: u32 = 20;
const BELL_HEIGHT: u32 = 20;
static BELL_DATA: &[u8] = include_bytes!("../assets/bell.bin");

// NFC icon (72x72)
const NFC_WIDTH: u32 = 72;
const NFC_HEIGHT: u32 = 72;
static NFC_DATA: &[u8] = include_bytes!("../assets/nfc.bin");

// Weight icon (64x64)
const WEIGHT_WIDTH: u32 = 64;
const WEIGHT_HEIGHT: u32 = 64;
static WEIGHT_DATA: &[u8] = include_bytes!("../assets/weight.bin");

// Power icon (12x12)
const POWER_WIDTH: u32 = 12;
const POWER_HEIGHT: u32 = 12;
static POWER_DATA: &[u8] = include_bytes!("../assets/power.bin");

// Setting icon (40x40)
const SETTING_WIDTH: u32 = 40;
const SETTING_HEIGHT: u32 = 40;
static SETTING_DATA: &[u8] = include_bytes!("../assets/setting.bin");

// Encode icon (40x40)
const ENCODE_WIDTH: u32 = 40;
const ENCODE_HEIGHT: u32 = 40;
static ENCODE_DATA: &[u8] = include_bytes!("../assets/encode.bin");

// Image descriptors (static, initialized at runtime)
static mut LOGO_IMG_DSC: lvgl_sys::lv_img_dsc_t = unsafe { std::mem::zeroed() };
static mut BELL_IMG_DSC: lvgl_sys::lv_img_dsc_t = unsafe { std::mem::zeroed() };
static mut NFC_IMG_DSC: lvgl_sys::lv_img_dsc_t = unsafe { std::mem::zeroed() };
static mut WEIGHT_IMG_DSC: lvgl_sys::lv_img_dsc_t = unsafe { std::mem::zeroed() };
static mut POWER_IMG_DSC: lvgl_sys::lv_img_dsc_t = unsafe { std::mem::zeroed() };
static mut SETTING_IMG_DSC: lvgl_sys::lv_img_dsc_t = unsafe { std::mem::zeroed() };
static mut ENCODE_IMG_DSC: lvgl_sys::lv_img_dsc_t = unsafe { std::mem::zeroed() };

fn main() {
    env_logger::Builder::from_env(env_logger::Env::default().default_filter_or("info")).init();

    let args: Vec<String> = std::env::args().collect();
    let headless = args.iter().any(|a| a == "--headless" || a == "-h");

    if headless {
        run_headless();
    } else {
        run_interactive();
    }
}

// Statics for headless mode
static mut HEADLESS_DISP_BUF: lvgl_sys::lv_disp_draw_buf_t = unsafe { std::mem::zeroed() };
static mut HEADLESS_BUF1: [lvgl_sys::lv_color_t; (800 * 480 / 10) as usize] =
    [lvgl_sys::lv_color_t { full: 0 }; (800 * 480 / 10) as usize];
static mut HEADLESS_DISP_DRV: lvgl_sys::lv_disp_drv_t = unsafe { std::mem::zeroed() };

fn run_headless() {
    info!("SpoolBuddy LVGL Simulator - HEADLESS MODE");
    info!("Display: {}x{}", WIDTH, HEIGHT);

    unsafe {
        // Initialize LVGL
        lvgl_sys::lv_init();
        info!("LVGL initialized");

        // Create display buffer
        lvgl_sys::lv_disp_draw_buf_init(
            &raw mut HEADLESS_DISP_BUF,
            HEADLESS_BUF1.as_mut_ptr() as *mut _,
            std::ptr::null_mut(),
            (800 * 480 / 10) as u32,
        );

        // Create display driver with our custom flush callback
        lvgl_sys::lv_disp_drv_init(&raw mut HEADLESS_DISP_DRV);
        HEADLESS_DISP_DRV.hor_res = WIDTH;
        HEADLESS_DISP_DRV.ver_res = HEIGHT;
        HEADLESS_DISP_DRV.flush_cb = Some(headless_flush_cb);
        HEADLESS_DISP_DRV.draw_buf = &raw mut HEADLESS_DISP_BUF;
        let _disp = lvgl_sys::lv_disp_drv_register(&raw mut HEADLESS_DISP_DRV);
        info!("Display driver registered (headless)");

        // Create UI
        create_home_screen();
        info!("UI created");

        // Run a few frames to let LVGL render
        for _ in 0..10 {
            lvgl_sys::lv_tick_inc(10);
            lvgl_sys::lv_timer_handler();
            std::thread::sleep(std::time::Duration::from_millis(10));
        }

        // Save screenshot
        std::fs::create_dir_all("screenshots").unwrap();
        save_framebuffer_as_bmp("screenshots/home.bmp");
        info!("Saved: screenshots/home.bmp");

        // Also save as raw RGB for debugging
        save_framebuffer_as_raw("screenshots/home.raw");
        info!("Saved: screenshots/home.raw");

        info!("Done! Screenshots saved to screenshots/ directory");
    }
}

/// Headless flush callback - copies pixels to our framebuffer
unsafe extern "C" fn headless_flush_cb(
    disp_drv: *mut lvgl_sys::lv_disp_drv_t,
    area: *const lvgl_sys::lv_area_t,
    color_p: *mut lvgl_sys::lv_color_t,
) {
    let x1 = (*area).x1 as usize;
    let y1 = (*area).y1 as usize;
    let x2 = (*area).x2 as usize;
    let y2 = (*area).y2 as usize;

    let width = x2 - x1 + 1;

    // Debug: print first pixel color and area
    let first_color = (*color_p).full;
    eprintln!("Flush area: ({},{}) to ({},{}), first pixel: 0x{:04X}", x1, y1, x2, y2, first_color);

    for row in 0..=(y2 - y1) {
        let src_offset = row * width;
        let dst_offset = (y1 + row) * (WIDTH as usize) + x1;
        for col in 0..width {
            let color = *color_p.add(src_offset + col);
            FRAMEBUFFER[dst_offset + col] = color.full;
        }
    }

    lvgl_sys::lv_disp_flush_ready(disp_drv);
}

fn save_framebuffer_as_bmp(filename: &str) {
    let width = WIDTH as u32;
    let height = HEIGHT as u32;

    // Debug: check first few pixels
    unsafe {
        eprintln!("Framebuffer[0] = 0x{:04X}", FRAMEBUFFER[0]);
        eprintln!("Framebuffer[100] = 0x{:04X}", FRAMEBUFFER[100]);
    }

    // BMP file header (14 bytes) + DIB header (40 bytes) = 54 bytes
    let row_size = ((width * 3 + 3) / 4) * 4;
    let pixel_data_size = row_size * height;
    let file_size = 54 + pixel_data_size;

    let mut file = std::fs::File::create(filename).unwrap();

    // BMP File Header
    file.write_all(b"BM").unwrap();
    file.write_all(&(file_size as u32).to_le_bytes()).unwrap();
    file.write_all(&[0u8; 4]).unwrap();
    file.write_all(&54u32.to_le_bytes()).unwrap();

    // DIB Header (BITMAPINFOHEADER)
    file.write_all(&40u32.to_le_bytes()).unwrap();
    file.write_all(&(width as i32).to_le_bytes()).unwrap();
    file.write_all(&(-(height as i32)).to_le_bytes()).unwrap();
    file.write_all(&1u16.to_le_bytes()).unwrap();
    file.write_all(&24u16.to_le_bytes()).unwrap();
    file.write_all(&0u32.to_le_bytes()).unwrap();
    file.write_all(&(pixel_data_size as u32).to_le_bytes()).unwrap();
    file.write_all(&2835u32.to_le_bytes()).unwrap();
    file.write_all(&2835u32.to_le_bytes()).unwrap();
    file.write_all(&0u32.to_le_bytes()).unwrap();
    file.write_all(&0u32.to_le_bytes()).unwrap();

    // Pixel data (BGR format, rows padded)
    let padding = (row_size - width * 3) as usize;
    let mut first_pixel_printed = false;
    unsafe {
        for y in 0..height {
            for x in 0..width {
                let pixel = FRAMEBUFFER[(y * width + x) as usize];
                // RGB565 to BGR24
                let r = ((pixel >> 11) & 0x1F) as u8;
                let g = ((pixel >> 5) & 0x3F) as u8;
                let b = (pixel & 0x1F) as u8;
                let r8 = ((r as u32 * 255) / 31) as u8;
                let g8 = ((g as u32 * 255) / 63) as u8;
                let b8 = ((b as u32 * 255) / 31) as u8;

                if !first_pixel_printed {
                    eprintln!("First pixel: 0x{:04X} -> r={} g={} b={} -> r8={} g8={} b8={}",
                              pixel, r, g, b, r8, g8, b8);
                    first_pixel_printed = true;
                }

                file.write_all(&[b8, g8, r8]).unwrap();
            }
            for _ in 0..padding {
                file.write_all(&[0u8]).unwrap();
            }
        }
    }
}

// Statics for interactive mode
static mut SDL_DISP_BUF: lvgl_sys::lv_disp_draw_buf_t = unsafe { std::mem::zeroed() };
static mut SDL_BUF1: [lvgl_sys::lv_color_t; (800 * 480 / 10) as usize] =
    [lvgl_sys::lv_color_t { full: 0 }; (800 * 480 / 10) as usize];
static mut SDL_DISP_DRV: lvgl_sys::lv_disp_drv_t = unsafe { std::mem::zeroed() };
static mut SDL_INDEV_DRV: lvgl_sys::lv_indev_drv_t = unsafe { std::mem::zeroed() };

fn run_interactive() {
    info!("SpoolBuddy LVGL Simulator starting...");
    info!("Display: {}x{}", WIDTH, HEIGHT);

    unsafe {
        // Initialize LVGL
        lvgl_sys::lv_init();
        info!("LVGL initialized");

        // Initialize SDL display driver
        lvgl_sys::sdl_init();
        info!("SDL initialized");

        // Create display buffer
        lvgl_sys::lv_disp_draw_buf_init(
            &raw mut SDL_DISP_BUF,
            SDL_BUF1.as_mut_ptr() as *mut _,
            std::ptr::null_mut(),
            (800 * 480 / 10) as u32,
        );

        // Create display driver
        lvgl_sys::lv_disp_drv_init(&raw mut SDL_DISP_DRV);
        SDL_DISP_DRV.hor_res = WIDTH;
        SDL_DISP_DRV.ver_res = HEIGHT;
        SDL_DISP_DRV.flush_cb = Some(lvgl_sys::sdl_display_flush);
        SDL_DISP_DRV.draw_buf = &raw mut SDL_DISP_BUF;
        let _disp = lvgl_sys::lv_disp_drv_register(&raw mut SDL_DISP_DRV);
        info!("Display driver registered");

        // Create input device (mouse)
        lvgl_sys::lv_indev_drv_init(&raw mut SDL_INDEV_DRV);
        SDL_INDEV_DRV.type_ = lvgl_sys::lv_indev_type_t_LV_INDEV_TYPE_POINTER;
        SDL_INDEV_DRV.read_cb = Some(lvgl_sys::sdl_mouse_read);
        let _indev = lvgl_sys::lv_indev_drv_register(&raw mut SDL_INDEV_DRV);
        info!("Mouse input registered");

        // Create UI
        create_home_screen();
        info!("UI created");

        info!("Entering main loop...");
        info!("Controls:");
        info!("  Mouse: Touch input");
        info!("  Close window to quit");

        // Main loop
        let mut last_tick = Instant::now();
        loop {
            let elapsed = last_tick.elapsed().as_millis() as u32;
            if elapsed > 0 {
                lvgl_sys::lv_tick_inc(elapsed);
                last_tick = Instant::now();
            }
            lvgl_sys::lv_timer_handler();
            std::thread::sleep(std::time::Duration::from_millis(5));
        }
    }
}

// Color constants
const COLOR_BG: u32 = 0x1A1A1A;
const COLOR_CARD: u32 = 0x2D2D2D;
const COLOR_BORDER: u32 = 0x3D3D3D;
const COLOR_ACCENT: u32 = 0x00FF00;
const COLOR_WHITE: u32 = 0xFFFFFF;
const COLOR_GRAY: u32 = 0x808080;
const COLOR_STATUS_BAR: u32 = 0x1A1A1A;

/// Create the home screen UI
unsafe fn create_home_screen() {
    let disp = lvgl_sys::lv_disp_get_default();
    let scr = lvgl_sys::lv_disp_get_scr_act(disp);

    // Background
    lvgl_sys::lv_obj_set_style_bg_color(scr, lv_color_hex(COLOR_BG), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(scr, 255, 0);
    set_style_pad_all(scr, 0);

    // === STATUS BAR (44px) ===
    let status_bar = lvgl_sys::lv_obj_create(scr);
    lvgl_sys::lv_obj_set_size(status_bar, 800, 44);
    lvgl_sys::lv_obj_set_pos(status_bar, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_color(status_bar, lv_color_hex(COLOR_STATUS_BAR), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(status_bar, 255, 0);
    lvgl_sys::lv_obj_set_style_border_width(status_bar, 0, 0);
    lvgl_sys::lv_obj_set_style_radius(status_bar, 0, 0);
    lvgl_sys::lv_obj_set_style_pad_left(status_bar, 16, 0);
    lvgl_sys::lv_obj_set_style_pad_right(status_bar, 16, 0);

    // SpoolBuddy logo image (97x24 PNG converted to ARGB8888)
    // Initialize the image descriptor at runtime
    LOGO_IMG_DSC.header._bitfield_1 = lvgl_sys::lv_img_header_t::new_bitfield_1(
        lvgl_sys::LV_IMG_CF_TRUE_COLOR_ALPHA as u32,  // ARGB8888
        0,  // always_zero
        0,  // reserved
        LOGO_WIDTH,
        LOGO_HEIGHT,
    );
    LOGO_IMG_DSC.data_size = (LOGO_WIDTH * LOGO_HEIGHT * 3) as u32;  // RGB565 + Alpha = 3 bytes/pixel
    LOGO_IMG_DSC.data = LOGO_DATA.as_ptr();

    let logo_img = lvgl_sys::lv_img_create(status_bar);
    lvgl_sys::lv_img_set_src(logo_img, &raw const LOGO_IMG_DSC as *const _);
    lvgl_sys::lv_obj_align(logo_img, lvgl_sys::LV_ALIGN_LEFT_MID as u8, 0, 0);

    // Printer selector (center)
    let printer_btn = lvgl_sys::lv_btn_create(status_bar);
    lvgl_sys::lv_obj_set_size(printer_btn, 180, 32);
    lvgl_sys::lv_obj_align(printer_btn, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_color(printer_btn, lv_color_hex(COLOR_CARD), 0);
    lvgl_sys::lv_obj_set_style_radius(printer_btn, 16, 0);

    // Left status dot (green = connected)
    let left_dot = lvgl_sys::lv_obj_create(printer_btn);
    lvgl_sys::lv_obj_set_size(left_dot, 8, 8);
    lvgl_sys::lv_obj_align(left_dot, lvgl_sys::LV_ALIGN_LEFT_MID as u8, 12, 0);
    lvgl_sys::lv_obj_set_style_bg_color(left_dot, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_radius(left_dot, 4, 0);
    lvgl_sys::lv_obj_set_style_border_width(left_dot, 0, 0);

    let printer_label = lvgl_sys::lv_label_create(printer_btn);
    let printer_text = CString::new("X1C-Studio").unwrap();
    lvgl_sys::lv_label_set_text(printer_label, printer_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(printer_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_align(printer_label, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

    // Right status icon (power button, orange = printing)
    POWER_IMG_DSC.header._bitfield_1 = lvgl_sys::lv_img_header_t::new_bitfield_1(
        lvgl_sys::LV_IMG_CF_TRUE_COLOR_ALPHA as u32,
        0, 0,
        POWER_WIDTH,
        POWER_HEIGHT,
    );
    POWER_IMG_DSC.data_size = (POWER_WIDTH * POWER_HEIGHT * 3) as u32;
    POWER_IMG_DSC.data = POWER_DATA.as_ptr();

    let power_img = lvgl_sys::lv_img_create(printer_btn);
    lvgl_sys::lv_img_set_src(power_img, &raw const POWER_IMG_DSC as *const _);
    lvgl_sys::lv_obj_align(power_img, lvgl_sys::LV_ALIGN_RIGHT_MID as u8, -8, 0);
    // Color it orange for "printing" state
    lvgl_sys::lv_obj_set_style_img_recolor(power_img, lv_color_hex(0xFFA500), 0);
    lvgl_sys::lv_obj_set_style_img_recolor_opa(power_img, 255, 0);

    // Right side of status bar: Bell -> WiFi -> Time (from left to right)
    // Layout: [bell+badge] 12px [wifi] 12px [time]

    // Time (rightmost)
    let time_label = lvgl_sys::lv_label_create(status_bar);
    let time_text = CString::new("14:23").unwrap();
    lvgl_sys::lv_label_set_text(time_label, time_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(time_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_align(time_label, lvgl_sys::LV_ALIGN_RIGHT_MID as u8, 0, 0);

    // WiFi icon - 3 bars, bottom-aligned, to left of time
    // Bars: 4px wide, heights 8/12/16, 2px gaps between
    let wifi_x = -50;  // Start position from right
    let wifi_bottom = 8;  // Y offset for bottom alignment
    // Bar 3 (tallest, rightmost)
    let wifi_bar3 = lvgl_sys::lv_obj_create(status_bar);
    lvgl_sys::lv_obj_set_size(wifi_bar3, 4, 16);
    lvgl_sys::lv_obj_align(wifi_bar3, lvgl_sys::LV_ALIGN_RIGHT_MID as u8, wifi_x, wifi_bottom - 8);
    lvgl_sys::lv_obj_set_style_bg_color(wifi_bar3, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(wifi_bar3, 255, 0);
    lvgl_sys::lv_obj_set_style_radius(wifi_bar3, 1, 0);
    lvgl_sys::lv_obj_set_style_border_width(wifi_bar3, 0, 0);
    // Bar 2 (medium)
    let wifi_bar2 = lvgl_sys::lv_obj_create(status_bar);
    lvgl_sys::lv_obj_set_size(wifi_bar2, 4, 12);
    lvgl_sys::lv_obj_align(wifi_bar2, lvgl_sys::LV_ALIGN_RIGHT_MID as u8, wifi_x - 6, wifi_bottom - 6);
    lvgl_sys::lv_obj_set_style_bg_color(wifi_bar2, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(wifi_bar2, 255, 0);
    lvgl_sys::lv_obj_set_style_radius(wifi_bar2, 1, 0);
    lvgl_sys::lv_obj_set_style_border_width(wifi_bar2, 0, 0);
    // Bar 1 (shortest, leftmost)
    let wifi_bar1 = lvgl_sys::lv_obj_create(status_bar);
    lvgl_sys::lv_obj_set_size(wifi_bar1, 4, 8);
    lvgl_sys::lv_obj_align(wifi_bar1, lvgl_sys::LV_ALIGN_RIGHT_MID as u8, wifi_x - 12, wifi_bottom - 4);
    lvgl_sys::lv_obj_set_style_bg_color(wifi_bar1, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(wifi_bar1, 255, 0);
    lvgl_sys::lv_obj_set_style_radius(wifi_bar1, 1, 0);
    lvgl_sys::lv_obj_set_style_border_width(wifi_bar1, 0, 0);

    // Bell icon (20x20)
    BELL_IMG_DSC.header._bitfield_1 = lvgl_sys::lv_img_header_t::new_bitfield_1(
        lvgl_sys::LV_IMG_CF_TRUE_COLOR_ALPHA as u32,
        0, 0,
        BELL_WIDTH,
        BELL_HEIGHT,
    );
    BELL_IMG_DSC.data_size = (BELL_WIDTH * BELL_HEIGHT * 3) as u32;
    BELL_IMG_DSC.data = BELL_DATA.as_ptr();

    let bell_img = lvgl_sys::lv_img_create(status_bar);
    lvgl_sys::lv_img_set_src(bell_img, &raw const BELL_IMG_DSC as *const _);
    lvgl_sys::lv_obj_align(bell_img, lvgl_sys::LV_ALIGN_RIGHT_MID as u8, -82, 0);

    // Notification badge (overlaps top-right of bell)
    let badge = lvgl_sys::lv_obj_create(status_bar);
    lvgl_sys::lv_obj_set_size(badge, 14, 14);
    lvgl_sys::lv_obj_align(badge, lvgl_sys::LV_ALIGN_RIGHT_MID as u8, -70, -8);
    lvgl_sys::lv_obj_set_style_bg_color(badge, lv_color_hex(0xFF4444), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(badge, 255, 0);
    lvgl_sys::lv_obj_set_style_radius(badge, 7, 0);
    lvgl_sys::lv_obj_set_style_border_width(badge, 0, 0);

    let badge_text = lvgl_sys::lv_label_create(badge);
    let badge_str = CString::new("3").unwrap();
    lvgl_sys::lv_label_set_text(badge_text, badge_str.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(badge_text, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_align(badge_text, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

    // === STATUS BAR SEPARATOR ===
    let separator = lvgl_sys::lv_obj_create(scr);
    lvgl_sys::lv_obj_set_size(separator, 800, 1);
    lvgl_sys::lv_obj_set_pos(separator, 0, 44);
    lvgl_sys::lv_obj_set_style_bg_color(separator, lv_color_hex(COLOR_BORDER), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(separator, 255, 0);
    lvgl_sys::lv_obj_set_style_border_width(separator, 0, 0);
    lvgl_sys::lv_obj_set_style_radius(separator, 0, 0);

    // === MAIN CONTENT AREA ===
    let content_y = 52;
    let content_height = 280;
    let card_gap = 8;

    // Button dimensions (defined first so we can calculate left card width)
    let btn_width: i16 = 130;
    let btn_gap: i16 = 8;
    let btn_start_x: i16 = 800 - 16 - btn_width - btn_gap - btn_width;

    // Left column - Printer Card (expanded)
    let left_card_width = btn_start_x - 16 - card_gap; // Fill space up to buttons with gap
    let printer_card = create_card(scr, 16, content_y, left_card_width, 130);

    // Print cover image placeholder (left side)
    let cover_size = 70;
    let cover_img = lvgl_sys::lv_obj_create(printer_card);
    lvgl_sys::lv_obj_set_size(cover_img, cover_size, cover_size);
    lvgl_sys::lv_obj_set_pos(cover_img, 12, 12);
    lvgl_sys::lv_obj_set_style_bg_color(cover_img, lv_color_hex(0x404040), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(cover_img, 255, 0);
    lvgl_sys::lv_obj_set_style_radius(cover_img, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(cover_img, 0, 0);
    // Placeholder icon (simple 3D cube representation)
    let cube_label = lvgl_sys::lv_label_create(cover_img);
    let cube_text = CString::new("3D").unwrap();
    lvgl_sys::lv_label_set_text(cube_label, cube_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(cube_label, lv_color_hex(COLOR_GRAY), 0);
    lvgl_sys::lv_obj_align(cube_label, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

    // Printer name (right of image)
    let text_x = 12 + cover_size + 12;
    let printer_name = lvgl_sys::lv_label_create(printer_card);
    let name_text = CString::new("X1C-Studio").unwrap();
    lvgl_sys::lv_label_set_text(printer_name, name_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(printer_name, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_pos(printer_name, text_x, 16);

    // Status (below printer name, green)
    let status_label = lvgl_sys::lv_label_create(printer_card);
    let status_text = CString::new("Printing").unwrap();
    lvgl_sys::lv_label_set_text(status_label, status_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(status_label, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_pos(status_label, text_x, 38);

    // Filename and time (above progress bar)
    let file_label = lvgl_sys::lv_label_create(printer_card);
    let file_text = CString::new("Benchy.3mf").unwrap();
    lvgl_sys::lv_label_set_text(file_label, file_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(file_label, lv_color_hex(COLOR_GRAY), 0);
    lvgl_sys::lv_obj_set_pos(file_label, 12, 88);

    let time_left = lvgl_sys::lv_label_create(printer_card);
    let time_left_text = CString::new("1h 23m left").unwrap();
    lvgl_sys::lv_label_set_text(time_left, time_left_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(time_left, lv_color_hex(COLOR_GRAY), 0);
    lvgl_sys::lv_obj_align(time_left, lvgl_sys::LV_ALIGN_TOP_RIGHT as u8, -12, 88);

    // Progress bar (full width at bottom) - enhanced with gradient effect
    let progress_width = left_card_width - 24;
    let progress_percent: f32 = 0.6;  // 60%
    let fill_width = (progress_width as f32 * progress_percent) as i16;

    // Background track
    let progress_bg = lvgl_sys::lv_obj_create(printer_card);
    lvgl_sys::lv_obj_set_size(progress_bg, progress_width, 14);
    lvgl_sys::lv_obj_set_pos(progress_bg, 12, 106);
    lvgl_sys::lv_obj_set_style_bg_color(progress_bg, lv_color_hex(0x1A1A1A), 0);
    lvgl_sys::lv_obj_set_style_radius(progress_bg, 7, 0);
    lvgl_sys::lv_obj_set_style_border_color(progress_bg, lv_color_hex(COLOR_BORDER), 0);
    lvgl_sys::lv_obj_set_style_border_width(progress_bg, 1, 0);
    set_style_pad_all(progress_bg, 0);

    // Gradient simulation: darker green base
    let progress_base = lvgl_sys::lv_obj_create(progress_bg);
    lvgl_sys::lv_obj_set_size(progress_base, fill_width, 14);
    lvgl_sys::lv_obj_set_pos(progress_base, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_color(progress_base, lv_color_hex(0x00AA00), 0);  // Darker green
    lvgl_sys::lv_obj_set_style_radius(progress_base, 7, 0);
    lvgl_sys::lv_obj_set_style_border_width(progress_base, 0, 0);
    set_style_pad_all(progress_base, 0);

    // Highlight strip on top (lighter green, creates gradient illusion)
    let progress_highlight = lvgl_sys::lv_obj_create(progress_bg);
    lvgl_sys::lv_obj_set_size(progress_highlight, fill_width - 4, 5);
    lvgl_sys::lv_obj_set_pos(progress_highlight, 2, 2);
    lvgl_sys::lv_obj_set_style_bg_color(progress_highlight, lv_color_hex(0x44FF44), 0);  // Bright green
    lvgl_sys::lv_obj_set_style_bg_opa(progress_highlight, 180, 0);
    lvgl_sys::lv_obj_set_style_radius(progress_highlight, 3, 0);
    lvgl_sys::lv_obj_set_style_border_width(progress_highlight, 0, 0);

    // Left column - NFC/Weight scan area (expanded)
    let scan_card = create_card(scr, 16, content_y + 138, left_card_width, 125);

    // === LEFT SIDE: NFC Icon (64x64) ===
    NFC_IMG_DSC.header._bitfield_1 = lvgl_sys::lv_img_header_t::new_bitfield_1(
        lvgl_sys::LV_IMG_CF_TRUE_COLOR_ALPHA as u32,
        0, 0,
        NFC_WIDTH,
        NFC_HEIGHT,
    );
    NFC_IMG_DSC.data_size = (NFC_WIDTH * NFC_HEIGHT * 3) as u32;
    NFC_IMG_DSC.data = NFC_DATA.as_ptr();

    let nfc_img = lvgl_sys::lv_img_create(scan_card);
    lvgl_sys::lv_img_set_src(nfc_img, &raw const NFC_IMG_DSC as *const _);
    lvgl_sys::lv_obj_set_pos(nfc_img, 16, 10);  // Adjusted for smaller card
    // Apply gray tint to match scale icon (darker to compensate for lighter source)
    lvgl_sys::lv_obj_set_style_img_recolor(nfc_img, lv_color_hex(0x999999), 0);
    lvgl_sys::lv_obj_set_style_img_recolor_opa(nfc_img, 220, 0);

    // "Ready" text below NFC icon (with spacing)
    let nfc_status = lvgl_sys::lv_label_create(scan_card);
    let nfc_status_text = CString::new("Ready").unwrap();
    lvgl_sys::lv_label_set_text(nfc_status, nfc_status_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(nfc_status, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_pos(nfc_status, 32, 90);  // Adjusted for smaller card

    // === CENTER: Instruction text ===
    let scan_hint = lvgl_sys::lv_label_create(scan_card);
    let hint_text = CString::new("Place spool on scale\nto scan & weigh").unwrap();
    lvgl_sys::lv_label_set_text(scan_hint, hint_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(scan_hint, lv_color_hex(COLOR_GRAY), 0);
    lvgl_sys::lv_obj_set_style_text_align(scan_hint, lvgl_sys::LV_TEXT_ALIGN_CENTER as u8, 0);
    lvgl_sys::lv_obj_align(scan_hint, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

    // === RIGHT SIDE: Weight display (icon + value + fill bar) ===
    // Current weight value
    let current_weight: f32 = 0.85;  // kg
    let max_weight: f32 = 1.0;  // kg (full spool)
    let fill_percent = ((current_weight / max_weight) * 100.0).min(100.0) as i16;
    let fill_color = if fill_percent > 50 {
        COLOR_ACCENT  // Green - good
    } else if fill_percent > 20 {
        0xFFA500  // Orange - warning
    } else {
        0xFF4444  // Red - low
    };

    // Weight icon (64x64, white - no tint)
    WEIGHT_IMG_DSC.header._bitfield_1 = lvgl_sys::lv_img_header_t::new_bitfield_1(
        lvgl_sys::LV_IMG_CF_TRUE_COLOR_ALPHA as u32,
        0, 0,
        WEIGHT_WIDTH,
        WEIGHT_HEIGHT,
    );
    WEIGHT_IMG_DSC.data_size = (WEIGHT_WIDTH * WEIGHT_HEIGHT * 3) as u32;
    WEIGHT_IMG_DSC.data = WEIGHT_DATA.as_ptr();

    let weight_img = lvgl_sys::lv_img_create(scan_card);
    lvgl_sys::lv_img_set_src(weight_img, &raw const WEIGHT_IMG_DSC as *const _);
    lvgl_sys::lv_obj_set_pos(weight_img, left_card_width - 84, 8);  // Adjusted for smaller card
    // Apply gray-white tint to match NFC icon
    lvgl_sys::lv_obj_set_style_img_recolor(weight_img, lv_color_hex(0xBBBBBB), 0);
    lvgl_sys::lv_obj_set_style_img_recolor_opa(weight_img, 200, 0);

    // Weight value below icon (green)
    let weight_value = lvgl_sys::lv_label_create(scan_card);
    let weight_str = CString::new("0.85 kg").unwrap();
    lvgl_sys::lv_label_set_text(weight_value, weight_str.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(weight_value, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_pos(weight_value, left_card_width - 80, 72);  // Adjusted for smaller card

    // Horizontal fill bar below value - enhanced with gradient
    let bar_width: i16 = 70;
    let bar_height: i16 = 14;
    let bar_x = left_card_width - 87;
    let bar_y: i16 = 93;  // Adjusted for smaller card
    let scale_fill_width = ((bar_width as f32) * (fill_percent as f32 / 100.0)) as i16;

    // Bar background (dark with border)
    let bar_bg = lvgl_sys::lv_obj_create(scan_card);
    lvgl_sys::lv_obj_set_size(bar_bg, bar_width, bar_height);
    lvgl_sys::lv_obj_set_pos(bar_bg, bar_x, bar_y);
    lvgl_sys::lv_obj_set_style_bg_color(bar_bg, lv_color_hex(0x1A1A1A), 0);
    lvgl_sys::lv_obj_set_style_radius(bar_bg, 7, 0);
    lvgl_sys::lv_obj_set_style_border_color(bar_bg, lv_color_hex(COLOR_BORDER), 0);
    lvgl_sys::lv_obj_set_style_border_width(bar_bg, 1, 0);
    set_style_pad_all(bar_bg, 0);

    // Bar fill base (darker green)
    let bar_fill = lvgl_sys::lv_obj_create(bar_bg);
    lvgl_sys::lv_obj_set_size(bar_fill, scale_fill_width, bar_height);
    lvgl_sys::lv_obj_set_pos(bar_fill, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_color(bar_fill, lv_color_hex(0x00AA00), 0);
    lvgl_sys::lv_obj_set_style_radius(bar_fill, 7, 0);
    lvgl_sys::lv_obj_set_style_border_width(bar_fill, 0, 0);
    set_style_pad_all(bar_fill, 0);

    // Highlight strip (lighter green, gradient effect)
    if scale_fill_width > 4 {
        let bar_highlight = lvgl_sys::lv_obj_create(bar_bg);
        lvgl_sys::lv_obj_set_size(bar_highlight, scale_fill_width - 4, 5);
        lvgl_sys::lv_obj_set_pos(bar_highlight, 2, 2);
        lvgl_sys::lv_obj_set_style_bg_color(bar_highlight, lv_color_hex(0x44FF44), 0);
        lvgl_sys::lv_obj_set_style_bg_opa(bar_highlight, 180, 0);
        lvgl_sys::lv_obj_set_style_radius(bar_highlight, 3, 0);
        lvgl_sys::lv_obj_set_style_border_width(bar_highlight, 0, 0);
    }

    // Action buttons (right side) - individual cards, aligned with left side cards
    // Top row aligns with printer card (height 130), bottom row aligns with scan card (height 125)
    let top_btn_height: i16 = 130;   // Match printer card height
    let bottom_btn_height: i16 = 125; // Match scan card height

    create_action_button(scr, btn_start_x, content_y, btn_width, top_btn_height, "AMS Setup", "", "ams");
    create_action_button(scr, btn_start_x, content_y + 138, btn_width, bottom_btn_height, "Catalog", "", "catalog");
    create_action_button(scr, btn_start_x + btn_width + btn_gap, content_y, btn_width, top_btn_height, "Encode Tag", "", "encode");
    create_action_button(scr, btn_start_x + btn_width + btn_gap, content_y + 138, btn_width, bottom_btn_height, "Settings", "", "settings");

    // === AMS STRIP ===
    let ams_y = content_y + 263 + card_gap;  // 52 + 263 + 8 = 323

    // Left Nozzle card
    let left_nozzle = create_card(scr, 16, ams_y, 380, 110);

    // "L" badge (green circle)
    let l_badge = lvgl_sys::lv_obj_create(left_nozzle);
    lvgl_sys::lv_obj_set_size(l_badge, 22, 22);
    lvgl_sys::lv_obj_set_pos(l_badge, 12, 10);
    lvgl_sys::lv_obj_set_style_bg_color(l_badge, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_radius(l_badge, 11, 0);
    lvgl_sys::lv_obj_set_style_border_width(l_badge, 0, 0);
    set_style_pad_all(l_badge, 0);
    let l_letter = lvgl_sys::lv_label_create(l_badge);
    let l_text = CString::new("L").unwrap();
    lvgl_sys::lv_label_set_text(l_letter, l_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(l_letter, lv_color_hex(COLOR_BG), 0);
    lvgl_sys::lv_obj_align(l_letter, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

    let left_label = lvgl_sys::lv_label_create(left_nozzle);
    let left_text = CString::new("Left Nozzle").unwrap();
    lvgl_sys::lv_label_set_text(left_label, left_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(left_label, lv_color_hex(COLOR_GRAY), 0);
    lvgl_sys::lv_obj_set_pos(left_label, 40, 13);

    // AMS slots for left nozzle - row 1 (A, B, D with 4 color squares each)
    // Slot A colors: red, yellow, green, salmon
    create_ams_slot_4color(left_nozzle, 12, 38, "A", true, &[0xFF6B6B, 0xFFD93D, 0x6BCB77, 0xFFB5A7]);
    // Slot B colors: blue, dark, light blue, empty (striped)
    create_ams_slot_4color(left_nozzle, 92, 38, "B", false, &[0x4D96FF, 0x404040, 0x9ED5FF, 0]);
    // Slot D colors: magenta, purple, light purple, empty
    create_ams_slot_4color(left_nozzle, 172, 38, "D", false, &[0xFF6BD6, 0xC77DFF, 0xE5B8F4, 0]);

    // AMS slots for left nozzle - row 2 (EXT and HT)
    create_ams_slot_single(left_nozzle, 12, 82, "EXT-1", 0xFF6B6B);
    create_ams_slot_single(left_nozzle, 92, 82, "HT-A", 0x9ED5FF);

    // Right Nozzle card
    let right_nozzle = create_card(scr, 404, ams_y, 380, 110);

    // "R" badge (green circle)
    let r_badge = lvgl_sys::lv_obj_create(right_nozzle);
    lvgl_sys::lv_obj_set_size(r_badge, 22, 22);
    lvgl_sys::lv_obj_set_pos(r_badge, 12, 10);
    lvgl_sys::lv_obj_set_style_bg_color(r_badge, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_radius(r_badge, 11, 0);
    lvgl_sys::lv_obj_set_style_border_width(r_badge, 0, 0);
    set_style_pad_all(r_badge, 0);
    let r_letter = lvgl_sys::lv_label_create(r_badge);
    let r_text = CString::new("R").unwrap();
    lvgl_sys::lv_label_set_text(r_letter, r_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(r_letter, lv_color_hex(COLOR_BG), 0);
    lvgl_sys::lv_obj_align(r_letter, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

    let right_label = lvgl_sys::lv_label_create(right_nozzle);
    let right_text = CString::new("Right Nozzle").unwrap();
    lvgl_sys::lv_label_set_text(right_label, right_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(right_label, lv_color_hex(COLOR_GRAY), 0);
    lvgl_sys::lv_obj_set_pos(right_label, 40, 13);

    // AMS slots for right nozzle - row 1
    // Slot C colors: yellow, green, cyan, teal
    create_ams_slot_4color(right_nozzle, 12, 38, "C", false, &[0xFFD93D, 0x6BCB77, 0x4ECDC4, 0x45B7AA]);

    // AMS slots for right nozzle - row 2 (HT and EXT)
    create_ams_slot_single(right_nozzle, 12, 82, "HT-B", 0xFFA500);
    create_ams_slot_single(right_nozzle, 92, 82, "EXT-2", 0);  // Empty (striped)

    // === NOTIFICATION BAR ===
    let notif_bar = create_card(scr, 16, ams_y + 110 + card_gap, 768, 30);  // Below AMS cards

    // Warning dot
    let dot = lvgl_sys::lv_obj_create(notif_bar);
    lvgl_sys::lv_obj_set_size(dot, 10, 10);
    lvgl_sys::lv_obj_set_pos(dot, 12, 10);
    lvgl_sys::lv_obj_set_style_bg_color(dot, lv_color_hex(0xFFA500), 0); // Orange
    lvgl_sys::lv_obj_set_style_radius(dot, 5, 0);
    lvgl_sys::lv_obj_set_style_border_width(dot, 0, 0);

    let notif_text = lvgl_sys::lv_label_create(notif_bar);
    let notif_str = CString::new("Low filament: PLA Black (A2) - 15% remaining").unwrap();
    lvgl_sys::lv_label_set_text(notif_text, notif_str.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(notif_text, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_pos(notif_text, 30, 8);

    let view_log = lvgl_sys::lv_label_create(notif_bar);
    let view_log_text = CString::new("View Log >").unwrap();
    lvgl_sys::lv_label_set_text(view_log, view_log_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(view_log, lv_color_hex(COLOR_GRAY), 0);
    lvgl_sys::lv_obj_set_pos(view_log, 680, 8);
}

/// Create a card with standard styling
unsafe fn create_card(parent: *mut lvgl_sys::lv_obj_t, x: i16, y: i16, w: i16, h: i16) -> *mut lvgl_sys::lv_obj_t {
    let card = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(card, w, h);
    lvgl_sys::lv_obj_set_pos(card, x, y);
    lvgl_sys::lv_obj_set_style_bg_color(card, lv_color_hex(COLOR_CARD), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(card, 255, 0);
    lvgl_sys::lv_obj_set_style_border_color(card, lv_color_hex(COLOR_BORDER), 0);
    lvgl_sys::lv_obj_set_style_border_width(card, 1, 0);
    lvgl_sys::lv_obj_set_style_radius(card, 12, 0);
    set_style_pad_all(card, 0);
    card
}

/// Create an action button with specific icon type (standalone with card border)
unsafe fn create_action_button(parent: *mut lvgl_sys::lv_obj_t, x: i16, y: i16, w: i16, h: i16, title: &str, subtitle: &str, icon_type: &str) {
    let btn = create_card(parent, x, y, w, h);
    create_action_button_content(btn, title, subtitle, icon_type);
}

/// Create an action button inside a container (no individual border)
unsafe fn create_action_button_inner(parent: *mut lvgl_sys::lv_obj_t, x: i16, y: i16, w: i16, h: i16, title: &str, subtitle: &str, icon_type: &str) {
    let btn = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(btn, w, h);
    lvgl_sys::lv_obj_set_pos(btn, x, y);
    lvgl_sys::lv_obj_set_style_bg_opa(btn, 0, 0);  // Transparent background
    lvgl_sys::lv_obj_set_style_border_width(btn, 0, 0);
    lvgl_sys::lv_obj_set_style_radius(btn, 0, 0);
    set_style_pad_all(btn, 0);
    create_action_button_content(btn, title, subtitle, icon_type);
}

/// Common content for action buttons
unsafe fn create_action_button_content(btn: *mut lvgl_sys::lv_obj_t, title: &str, subtitle: &str, icon_type: &str) {
    // Icon container (transparent, for positioning) - centered vertically with offset for title
    let icon_container = lvgl_sys::lv_obj_create(btn);
    lvgl_sys::lv_obj_set_size(icon_container, 50, 50);
    lvgl_sys::lv_obj_align(icon_container, lvgl_sys::LV_ALIGN_CENTER as u8, 0, -15);  // Center with offset up for title
    lvgl_sys::lv_obj_set_style_bg_opa(icon_container, 0, 0);
    lvgl_sys::lv_obj_set_style_border_width(icon_container, 0, 0);
    set_style_pad_all(icon_container, 0);

    match icon_type {
        "ams" => draw_ams_icon(icon_container),
        "encode" => draw_encode_icon(icon_container),
        "catalog" => draw_catalog_icon(icon_container),
        "settings" => draw_settings_icon(icon_container),
        _ => {}
    }

    // Title - positioned below center
    let title_label = lvgl_sys::lv_label_create(btn);
    let title_cstr = CString::new(title).unwrap();
    lvgl_sys::lv_label_set_text(title_label, title_cstr.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(title_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_align(title_label, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 35);  // Below icon

    // Subtitle
    if !subtitle.is_empty() {
        let sub_label = lvgl_sys::lv_label_create(btn);
        let sub_cstr = CString::new(subtitle).unwrap();
        lvgl_sys::lv_label_set_text(sub_label, sub_cstr.as_ptr());
        lvgl_sys::lv_obj_set_style_text_color(sub_label, lv_color_hex(COLOR_GRAY), 0);
        lvgl_sys::lv_obj_align(sub_label, lvgl_sys::LV_ALIGN_TOP_MID as u8, 0, 95);
    }
}

/// Draw AMS Setup icon (table/grid with rows, black background)
unsafe fn draw_ams_icon(parent: *mut lvgl_sys::lv_obj_t) {
    // Black rounded background
    let bg = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(bg, 50, 50);
    lvgl_sys::lv_obj_align(bg, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_color(bg, lv_color_hex(0x000000), 0);
    lvgl_sys::lv_obj_set_style_radius(bg, 10, 0);
    lvgl_sys::lv_obj_set_style_border_width(bg, 0, 0);
    set_style_pad_all(bg, 0);

    // Outer frame
    let frame = lvgl_sys::lv_obj_create(bg);
    lvgl_sys::lv_obj_set_size(frame, 36, 36);
    lvgl_sys::lv_obj_align(frame, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_opa(frame, 0, 0);
    lvgl_sys::lv_obj_set_style_border_color(frame, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_border_width(frame, 2, 0);
    lvgl_sys::lv_obj_set_style_radius(frame, 4, 0);
    set_style_pad_all(frame, 0);

    // Horizontal lines (3 rows)
    for i in 0..3 {
        let line = lvgl_sys::lv_obj_create(frame);
        lvgl_sys::lv_obj_set_size(line, 24, 2);
        lvgl_sys::lv_obj_set_pos(line, 4, 6 + i * 9);
        lvgl_sys::lv_obj_set_style_bg_color(line, lv_color_hex(COLOR_ACCENT), 0);
        lvgl_sys::lv_obj_set_style_border_width(line, 0, 0);
        lvgl_sys::lv_obj_set_style_radius(line, 1, 0);
    }
}

/// Draw Encode Tag icon (PNG with black background)
unsafe fn draw_encode_icon(parent: *mut lvgl_sys::lv_obj_t) {
    // Black rounded background
    let bg = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(bg, 50, 50);
    lvgl_sys::lv_obj_align(bg, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_color(bg, lv_color_hex(0x000000), 0);
    lvgl_sys::lv_obj_set_style_radius(bg, 10, 0);
    lvgl_sys::lv_obj_set_style_border_width(bg, 0, 0);
    set_style_pad_all(bg, 0);

    // Initialize encode image descriptor
    ENCODE_IMG_DSC.header._bitfield_1 = lvgl_sys::lv_img_header_t::new_bitfield_1(
        lvgl_sys::LV_IMG_CF_TRUE_COLOR_ALPHA as u32,
        0, 0,
        ENCODE_WIDTH,
        ENCODE_HEIGHT,
    );
    ENCODE_IMG_DSC.data_size = (ENCODE_WIDTH * ENCODE_HEIGHT * 3) as u32;
    ENCODE_IMG_DSC.data = ENCODE_DATA.as_ptr();

    // PNG icon
    let icon = lvgl_sys::lv_img_create(bg);
    lvgl_sys::lv_img_set_src(icon, &raw const ENCODE_IMG_DSC as *const _);
    lvgl_sys::lv_obj_align(icon, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    // Green tint
    lvgl_sys::lv_obj_set_style_img_recolor(icon, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_img_recolor_opa(icon, 255, 0);
}

/// Draw Catalog icon (grid of squares, black background)
unsafe fn draw_catalog_icon(parent: *mut lvgl_sys::lv_obj_t) {
    // Black rounded background
    let bg = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(bg, 50, 50);
    lvgl_sys::lv_obj_align(bg, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_color(bg, lv_color_hex(0x000000), 0);
    lvgl_sys::lv_obj_set_style_radius(bg, 10, 0);
    lvgl_sys::lv_obj_set_style_border_width(bg, 0, 0);
    set_style_pad_all(bg, 0);

    // 3x3 grid of small squares
    let size: i16 = 10;
    let gap: i16 = 3;
    let start_x: i16 = 7;
    let start_y: i16 = 7;

    for row in 0..3 {
        for col in 0..3 {
            let square = lvgl_sys::lv_obj_create(bg);
            lvgl_sys::lv_obj_set_size(square, size, size);
            lvgl_sys::lv_obj_set_pos(square, start_x + col * (size + gap), start_y + row * (size + gap));
            lvgl_sys::lv_obj_set_style_bg_color(square, lv_color_hex(COLOR_ACCENT), 0);
            lvgl_sys::lv_obj_set_style_border_width(square, 0, 0);
            lvgl_sys::lv_obj_set_style_radius(square, 2, 0);
        }
    }
}

/// Draw Settings icon (PNG with black background)
unsafe fn draw_settings_icon(parent: *mut lvgl_sys::lv_obj_t) {
    // Black rounded background
    let bg = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(bg, 50, 50);
    lvgl_sys::lv_obj_align(bg, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_color(bg, lv_color_hex(0x000000), 0);
    lvgl_sys::lv_obj_set_style_radius(bg, 10, 0);
    lvgl_sys::lv_obj_set_style_border_width(bg, 0, 0);
    set_style_pad_all(bg, 0);

    // Initialize setting image descriptor
    SETTING_IMG_DSC.header._bitfield_1 = lvgl_sys::lv_img_header_t::new_bitfield_1(
        lvgl_sys::LV_IMG_CF_TRUE_COLOR_ALPHA as u32,
        0, 0,
        SETTING_WIDTH,
        SETTING_HEIGHT,
    );
    SETTING_IMG_DSC.data_size = (SETTING_WIDTH * SETTING_HEIGHT * 3) as u32;
    SETTING_IMG_DSC.data = SETTING_DATA.as_ptr();

    // PNG icon
    let icon = lvgl_sys::lv_img_create(bg);
    lvgl_sys::lv_img_set_src(icon, &raw const SETTING_IMG_DSC as *const _);
    lvgl_sys::lv_obj_align(icon, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    // Green tint
    lvgl_sys::lv_obj_set_style_img_recolor(icon, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_img_recolor_opa(icon, 255, 0);
}

/// Create an AMS slot with 4 color squares (for regular AMS units A, B, C, D)
unsafe fn create_ams_slot_4color(parent: *mut lvgl_sys::lv_obj_t, x: i16, y: i16, label: &str, selected: bool, colors: &[u32; 4]) {
    let slot = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(slot, 72, 42);
    lvgl_sys::lv_obj_set_pos(slot, x, y);
    lvgl_sys::lv_obj_set_style_bg_color(slot, lv_color_hex(COLOR_BORDER), 0);
    lvgl_sys::lv_obj_set_style_radius(slot, 8, 0);
    lvgl_sys::lv_obj_set_style_border_color(slot, lv_color_hex(if selected { COLOR_ACCENT } else { COLOR_BORDER }), 0);
    lvgl_sys::lv_obj_set_style_border_width(slot, if selected { 2 } else { 1 }, 0);
    set_style_pad_all(slot, 0);

    // Slot label (A, B, C, D) - small, at top
    let slot_label = lvgl_sys::lv_label_create(slot);
    let slot_text = CString::new(label).unwrap();
    lvgl_sys::lv_label_set_text(slot_label, slot_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(slot_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(slot_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_align(slot_label, lvgl_sys::LV_ALIGN_TOP_MID as u8, 0, 2);

    // 4 color squares in a row
    let square_size: i16 = 14;
    let square_gap: i16 = 2;
    let total_width = square_size * 4 + square_gap * 3;
    let start_x = (72 - total_width) / 2;

    for (i, &color) in colors.iter().enumerate() {
        let sq = lvgl_sys::lv_obj_create(slot);
        lvgl_sys::lv_obj_set_size(sq, square_size, square_size);
        lvgl_sys::lv_obj_set_pos(sq, start_x + (i as i16) * (square_size + square_gap), 22);
        lvgl_sys::lv_obj_set_style_radius(sq, 2, 0);
        lvgl_sys::lv_obj_set_style_border_width(sq, 0, 0);
        set_style_pad_all(sq, 0);

        if color == 0 {
            // Empty slot - gray background for empty
            lvgl_sys::lv_obj_set_style_bg_color(sq, lv_color_hex(0x505050), 0);
        } else {
            lvgl_sys::lv_obj_set_style_bg_color(sq, lv_color_hex(color), 0);
        }
    }
}

/// Create a single-color AMS slot for EXT and HT slots
unsafe fn create_ams_slot_single(parent: *mut lvgl_sys::lv_obj_t, x: i16, y: i16, label: &str, color: u32) {
    let slot = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(slot, 72, 22);
    lvgl_sys::lv_obj_set_pos(slot, x, y);
    lvgl_sys::lv_obj_set_style_bg_color(slot, lv_color_hex(COLOR_BORDER), 0);
    lvgl_sys::lv_obj_set_style_radius(slot, 6, 0);
    lvgl_sys::lv_obj_set_style_border_width(slot, 0, 0);
    set_style_pad_all(slot, 0);

    // Slot label
    let slot_label = lvgl_sys::lv_label_create(slot);
    let slot_text = CString::new(label).unwrap();
    lvgl_sys::lv_label_set_text(slot_label, slot_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(slot_label, lv_color_hex(COLOR_GRAY), 0);
    lvgl_sys::lv_obj_align(slot_label, lvgl_sys::LV_ALIGN_LEFT_MID as u8, 8, 0);

    // Color indicator (small square)
    let color_sq = lvgl_sys::lv_obj_create(slot);
    lvgl_sys::lv_obj_set_size(color_sq, 14, 14);
    lvgl_sys::lv_obj_align(color_sq, lvgl_sys::LV_ALIGN_RIGHT_MID as u8, -6, 0);
    lvgl_sys::lv_obj_set_style_radius(color_sq, 2, 0);
    lvgl_sys::lv_obj_set_style_border_width(color_sq, 0, 0);
    set_style_pad_all(color_sq, 0);

    if color == 0 {
        // Empty slot - diagonal stripe
        lvgl_sys::lv_obj_set_style_bg_color(color_sq, lv_color_hex(0x505050), 0);
        let stripe = lvgl_sys::lv_obj_create(color_sq);
        lvgl_sys::lv_obj_set_size(stripe, 18, 2);
        lvgl_sys::lv_obj_align(stripe, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
        lvgl_sys::lv_obj_set_style_bg_color(stripe, lv_color_hex(0x707070), 0);
        lvgl_sys::lv_obj_set_style_border_width(stripe, 0, 0);
        lvgl_sys::lv_obj_set_style_transform_angle(stripe, 450, 0);
    } else {
        lvgl_sys::lv_obj_set_style_bg_color(color_sq, lv_color_hex(color), 0);
    }
}

fn save_framebuffer_as_raw(filename: &str) {
    use std::io::Write;
    let mut file = std::fs::File::create(filename).unwrap();
    unsafe {
        for pixel in FRAMEBUFFER.iter() {
            // RGB565 to RGB24
            let r = ((*pixel >> 11) & 0x1F) as u8;
            let g = ((*pixel >> 5) & 0x3F) as u8;
            let b = (*pixel & 0x1F) as u8;
            let r8 = ((r as u32 * 255) / 31) as u8;
            let g8 = ((g as u32 * 255) / 63) as u8;
            let b8 = ((b as u32 * 255) / 31) as u8;
            file.write_all(&[r8, g8, b8]).unwrap();
        }
    }
}

/// Helper to set all padding at once
unsafe fn set_style_pad_all(obj: *mut lvgl_sys::lv_obj_t, pad: i16) {
    lvgl_sys::lv_obj_set_style_pad_top(obj, pad, 0);
    lvgl_sys::lv_obj_set_style_pad_bottom(obj, pad, 0);
    lvgl_sys::lv_obj_set_style_pad_left(obj, pad, 0);
    lvgl_sys::lv_obj_set_style_pad_right(obj, pad, 0);
}

/// Helper to create color - RGB888 to RGB565
fn lv_color_hex(hex: u32) -> lvgl_sys::lv_color_t {
    let r = ((hex >> 16) & 0xFF) as u8;
    let g = ((hex >> 8) & 0xFF) as u8;
    let b = (hex & 0xFF) as u8;

    // RGB565: RRRRRGGGGGGBBBBB
    let r5 = (r >> 3) as u16;
    let g6 = (g >> 2) as u16;
    let b5 = (b >> 3) as u16;
    lvgl_sys::lv_color_t {
        full: (r5 << 11) | (g6 << 5) | b5,
    }
}
