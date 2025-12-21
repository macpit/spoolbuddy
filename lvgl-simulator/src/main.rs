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

// NFC icon (32x32)
const NFC_WIDTH: u32 = 32;
const NFC_HEIGHT: u32 = 32;
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

// Spool assets (32x42) - 3D spool graphic
const SPOOL_WIDTH: u32 = 32;
const SPOOL_HEIGHT: u32 = 42;
static SPOOL_DATA: &[u8] = include_bytes!("../assets/spool.bin");
static SPOOL_FILL_DATA: &[u8] = include_bytes!("../assets/spool_fill.bin");
static SPOOL_FRAME_DATA: &[u8] = include_bytes!("../assets/spool_frame.bin");
static SPOOL_CLEAN_DATA: &[u8] = include_bytes!("../assets/spool_clean.bin");

// Humidity icon (10x10) - from mockup SVG
const HUMIDITY_WIDTH: u32 = 10;
const HUMIDITY_HEIGHT: u32 = 10;
static HUMIDITY_DATA: &[u8] = include_bytes!("../assets/humidity_mockup.bin");

// Temperature icon (10x10) - from mockup SVG
const TEMP_WIDTH: u32 = 10;
const TEMP_HEIGHT: u32 = 10;
static TEMP_DATA: &[u8] = include_bytes!("../assets/temp_mockup.bin");

// Image descriptors (static, initialized at runtime)
static mut LOGO_IMG_DSC: lvgl_sys::lv_img_dsc_t = unsafe { std::mem::zeroed() };
static mut BELL_IMG_DSC: lvgl_sys::lv_img_dsc_t = unsafe { std::mem::zeroed() };
static mut NFC_IMG_DSC: lvgl_sys::lv_img_dsc_t = unsafe { std::mem::zeroed() };
static mut WEIGHT_IMG_DSC: lvgl_sys::lv_img_dsc_t = unsafe { std::mem::zeroed() };
static mut POWER_IMG_DSC: lvgl_sys::lv_img_dsc_t = unsafe { std::mem::zeroed() };
static mut SETTING_IMG_DSC: lvgl_sys::lv_img_dsc_t = unsafe { std::mem::zeroed() };
static mut ENCODE_IMG_DSC: lvgl_sys::lv_img_dsc_t = unsafe { std::mem::zeroed() };
static mut SPOOL_IMG_DSC: lvgl_sys::lv_img_dsc_t = unsafe { std::mem::zeroed() };
static mut SPOOL_FILL_IMG_DSC: lvgl_sys::lv_img_dsc_t = unsafe { std::mem::zeroed() };
static mut SPOOL_FRAME_IMG_DSC: lvgl_sys::lv_img_dsc_t = unsafe { std::mem::zeroed() };
static mut SPOOL_CLEAN_IMG_DSC: lvgl_sys::lv_img_dsc_t = unsafe { std::mem::zeroed() };
static mut HUMIDITY_IMG_DSC: lvgl_sys::lv_img_dsc_t = unsafe { std::mem::zeroed() };
static mut TEMP_IMG_DSC: lvgl_sys::lv_img_dsc_t = unsafe { std::mem::zeroed() };

fn main() {
    env_logger::Builder::from_env(env_logger::Env::default().default_filter_or("info")).init();

    let args: Vec<String> = std::env::args().collect();
    let headless = args.iter().any(|a| a == "--headless" || a == "-h");

    // Parse --screen argument (default: "home")
    let screen = args.iter()
        .position(|a| a == "--screen")
        .and_then(|i| args.get(i + 1))
        .map(|s| s.as_str())
        .unwrap_or("home");

    if headless {
        run_headless(screen);
    } else {
        run_interactive();
    }
}

// Statics for headless mode
static mut HEADLESS_DISP_BUF: lvgl_sys::lv_disp_draw_buf_t = unsafe { std::mem::zeroed() };
static mut HEADLESS_BUF1: [lvgl_sys::lv_color_t; (800 * 480 / 10) as usize] =
    [lvgl_sys::lv_color_t { full: 0 }; (800 * 480 / 10) as usize];
static mut HEADLESS_DISP_DRV: lvgl_sys::lv_disp_drv_t = unsafe { std::mem::zeroed() };

fn run_headless(screen: &str) {
    info!("SpoolBuddy LVGL Simulator - HEADLESS MODE");
    info!("Display: {}x{}", WIDTH, HEIGHT);
    info!("Screen: {}", screen);

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

        // Create UI based on screen selection
        let screenshot_name = match screen {
            "ams" | "ams-overview" => {
                create_ams_overview_screen();
                "ams_overview"
            }
            _ => {
                create_home_screen();
                "home"
            }
        };
        info!("UI created");

        // Run a few frames to let LVGL render
        for _ in 0..10 {
            lvgl_sys::lv_tick_inc(10);
            lvgl_sys::lv_timer_handler();
            std::thread::sleep(std::time::Duration::from_millis(10));
        }

        // Save screenshot
        std::fs::create_dir_all("screenshots").unwrap();
        let bmp_path = format!("screenshots/{}.bmp", screenshot_name);
        save_framebuffer_as_bmp(&bmp_path);
        info!("Saved: {}", bmp_path);

        // Also save as raw RGB for debugging
        let raw_path = format!("screenshots/{}.raw", screenshot_name);
        save_framebuffer_as_raw(&raw_path);
        info!("Saved: {}", raw_path);

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

    // === STATUS BAR (44px) - Premium styling with subtle bottom shadow ===
    let status_bar = lvgl_sys::lv_obj_create(scr);
    lvgl_sys::lv_obj_set_size(status_bar, 800, 44);
    lvgl_sys::lv_obj_set_pos(status_bar, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_color(status_bar, lv_color_hex(COLOR_STATUS_BAR), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(status_bar, 255, 0);
    lvgl_sys::lv_obj_set_style_border_width(status_bar, 0, 0);
    lvgl_sys::lv_obj_set_style_radius(status_bar, 0, 0);
    lvgl_sys::lv_obj_set_style_pad_left(status_bar, 16, 0);
    lvgl_sys::lv_obj_set_style_pad_right(status_bar, 16, 0);
    // Visible bottom shadow for depth separation
    lvgl_sys::lv_obj_set_style_shadow_color(status_bar, lv_color_hex(0x000000), 0);
    lvgl_sys::lv_obj_set_style_shadow_width(status_bar, 25, 0);
    lvgl_sys::lv_obj_set_style_shadow_ofs_y(status_bar, 8, 0);
    lvgl_sys::lv_obj_set_style_shadow_spread(status_bar, 0, 0);
    lvgl_sys::lv_obj_set_style_shadow_opa(status_bar, 200, 0);

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

    // Printer selector (center) - with dropdown indicator
    let printer_btn = lvgl_sys::lv_btn_create(status_bar);
    lvgl_sys::lv_obj_set_size(printer_btn, 200, 32);  // Wider to fit dropdown arrow
    lvgl_sys::lv_obj_align(printer_btn, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_color(printer_btn, lv_color_hex(0x242424), 0);
    lvgl_sys::lv_obj_set_style_radius(printer_btn, 16, 0);
    lvgl_sys::lv_obj_set_style_border_color(printer_btn, lv_color_hex(0x3D3D3D), 0);
    lvgl_sys::lv_obj_set_style_border_width(printer_btn, 1, 0);

    // Left status dot (green = connected) with subtle glow
    let left_dot = lvgl_sys::lv_obj_create(printer_btn);
    lvgl_sys::lv_obj_set_size(left_dot, 8, 8);
    lvgl_sys::lv_obj_align(left_dot, lvgl_sys::LV_ALIGN_LEFT_MID as u8, 12, 0);
    lvgl_sys::lv_obj_set_style_bg_color(left_dot, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_radius(left_dot, 4, 0);
    lvgl_sys::lv_obj_set_style_border_width(left_dot, 0, 0);
    lvgl_sys::lv_obj_set_style_shadow_color(left_dot, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_shadow_width(left_dot, 6, 0);
    lvgl_sys::lv_obj_set_style_shadow_spread(left_dot, 2, 0);
    lvgl_sys::lv_obj_set_style_shadow_opa(left_dot, 150, 0);

    let printer_label = lvgl_sys::lv_label_create(printer_btn);
    let printer_text = CString::new("X1C-Studio").unwrap();
    lvgl_sys::lv_label_set_text(printer_label, printer_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(printer_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_align(printer_label, lvgl_sys::LV_ALIGN_LEFT_MID as u8, 28, 0);

    // Power icon (orange = printing)
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
    lvgl_sys::lv_obj_align(power_img, lvgl_sys::LV_ALIGN_RIGHT_MID as u8, -24, 0);
    lvgl_sys::lv_obj_set_style_img_recolor(power_img, lv_color_hex(0xFFA500), 0);
    lvgl_sys::lv_obj_set_style_img_recolor_opa(power_img, 255, 0);

    // Dropdown arrow - text-based "▼" symbol for visibility
    let arrow_label = lvgl_sys::lv_label_create(printer_btn);
    let arrow_text = CString::new("v").unwrap();
    lvgl_sys::lv_label_set_text(arrow_label, arrow_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(arrow_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_align(arrow_label, lvgl_sys::LV_ALIGN_RIGHT_MID as u8, -8, 2);

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

    // Print thumbnail frame - polished with inner shadow and 3D cube icon
    let cover_size = 70;
    let cover_img = lvgl_sys::lv_obj_create(printer_card);
    lvgl_sys::lv_obj_set_size(cover_img, cover_size, cover_size);
    lvgl_sys::lv_obj_set_pos(cover_img, 12, 12);
    lvgl_sys::lv_obj_set_style_bg_color(cover_img, lv_color_hex(0x1A1A1A), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(cover_img, 255, 0);
    lvgl_sys::lv_obj_set_style_radius(cover_img, 10, 0);
    lvgl_sys::lv_obj_set_style_border_color(cover_img, lv_color_hex(0x3A3A3A), 0);
    lvgl_sys::lv_obj_set_style_border_width(cover_img, 1, 0);
    set_style_pad_all(cover_img, 0);

    // 3D cube icon - front face
    let cube_front = lvgl_sys::lv_obj_create(cover_img);
    lvgl_sys::lv_obj_set_size(cube_front, 24, 24);
    lvgl_sys::lv_obj_set_pos(cube_front, 18, 26);
    lvgl_sys::lv_obj_set_style_bg_opa(cube_front, 0, 0);
    lvgl_sys::lv_obj_set_style_border_color(cube_front, lv_color_hex(0x505050), 0);
    lvgl_sys::lv_obj_set_style_border_width(cube_front, 2, 0);
    lvgl_sys::lv_obj_set_style_radius(cube_front, 2, 0);
    set_style_pad_all(cube_front, 0);

    // 3D cube icon - top face (parallelogram effect with offset rectangle)
    let cube_top = lvgl_sys::lv_obj_create(cover_img);
    lvgl_sys::lv_obj_set_size(cube_top, 24, 10);
    lvgl_sys::lv_obj_set_pos(cube_top, 26, 18);
    lvgl_sys::lv_obj_set_style_bg_opa(cube_top, 0, 0);
    lvgl_sys::lv_obj_set_style_border_color(cube_top, lv_color_hex(0x505050), 0);
    lvgl_sys::lv_obj_set_style_border_width(cube_top, 2, 0);
    lvgl_sys::lv_obj_set_style_radius(cube_top, 2, 0);
    set_style_pad_all(cube_top, 0);

    // 3D cube icon - side face
    let cube_side = lvgl_sys::lv_obj_create(cover_img);
    lvgl_sys::lv_obj_set_size(cube_side, 10, 24);
    lvgl_sys::lv_obj_set_pos(cube_side, 40, 26);
    lvgl_sys::lv_obj_set_style_bg_opa(cube_side, 0, 0);
    lvgl_sys::lv_obj_set_style_border_color(cube_side, lv_color_hex(0x505050), 0);
    lvgl_sys::lv_obj_set_style_border_width(cube_side, 2, 0);
    lvgl_sys::lv_obj_set_style_radius(cube_side, 2, 0);
    set_style_pad_all(cube_side, 0);

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

    // Progress bar (full width at bottom) - vibrant gradient with glow
    let progress_width = left_card_width - 24;
    let progress_percent: f32 = 0.6;  // 60%
    let fill_width = (progress_width as f32 * progress_percent) as i16;

    // Background track with inner shadow effect
    let progress_bg = lvgl_sys::lv_obj_create(printer_card);
    lvgl_sys::lv_obj_set_size(progress_bg, progress_width, 16);
    lvgl_sys::lv_obj_set_pos(progress_bg, 12, 104);
    lvgl_sys::lv_obj_set_style_bg_color(progress_bg, lv_color_hex(0x0A0A0A), 0);
    lvgl_sys::lv_obj_set_style_radius(progress_bg, 8, 0);
    lvgl_sys::lv_obj_set_style_border_color(progress_bg, lv_color_hex(0x2A2A2A), 0);
    lvgl_sys::lv_obj_set_style_border_width(progress_bg, 1, 0);
    set_style_pad_all(progress_bg, 0);

    // Solid fill with subtle glow (no gradient to avoid banding)
    let progress_fill = lvgl_sys::lv_obj_create(progress_bg);
    lvgl_sys::lv_obj_set_size(progress_fill, fill_width, 14);
    lvgl_sys::lv_obj_set_pos(progress_fill, 1, 1);
    lvgl_sys::lv_obj_set_style_bg_color(progress_fill, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_radius(progress_fill, 7, 0);
    lvgl_sys::lv_obj_set_style_border_width(progress_fill, 0, 0);
    // Subtle glow - just enough to make it pop
    lvgl_sys::lv_obj_set_style_shadow_color(progress_fill, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_shadow_width(progress_fill, 8, 0);
    lvgl_sys::lv_obj_set_style_shadow_spread(progress_fill, 0, 0);
    lvgl_sys::lv_obj_set_style_shadow_opa(progress_fill, 80, 0);
    set_style_pad_all(progress_fill, 0);

    // Left column - NFC/Weight scan area (expanded)
    // Layout: NFC icon left | Center text (spool info area) | Weight right
    let scan_card = create_card(scr, 16, content_y + 138, left_card_width, 125);

    // NFC Icon (left side)
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
    lvgl_sys::lv_obj_set_pos(nfc_img, 16, 16);
    lvgl_sys::lv_obj_set_style_img_recolor(nfc_img, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_img_recolor_opa(nfc_img, 255, 0);

    // "Ready" status under NFC icon
    let nfc_status = lvgl_sys::lv_label_create(scan_card);
    let nfc_status_text = CString::new("Ready").unwrap();
    lvgl_sys::lv_label_set_text(nfc_status, nfc_status_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(nfc_status, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_pos(nfc_status, 32, 92);

    // Center text area - instruction or spool info (centered in card)
    let center_text = lvgl_sys::lv_label_create(scan_card);
    let center_str = CString::new("Place spool on scale\nto scan & weigh").unwrap();
    lvgl_sys::lv_label_set_text(center_text, center_str.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(center_text, lv_color_hex(COLOR_GRAY), 0);
    lvgl_sys::lv_obj_set_style_text_align(center_text, lvgl_sys::LV_TEXT_ALIGN_CENTER as u8, 0);
    lvgl_sys::lv_obj_align(center_text, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

    // Weight section (right side)
    let current_weight: f32 = 0.85;
    let max_weight: f32 = 1.0;
    let fill_percent = ((current_weight / max_weight) * 100.0).min(100.0) as i16;

    // Weight icon
    WEIGHT_IMG_DSC.header._bitfield_1 = lvgl_sys::lv_img_header_t::new_bitfield_1(
        lvgl_sys::LV_IMG_CF_TRUE_COLOR_ALPHA as u32,
        0, 0,
        WEIGHT_WIDTH,
        WEIGHT_HEIGHT,
    );
    WEIGHT_IMG_DSC.data_size = (WEIGHT_WIDTH * WEIGHT_HEIGHT * 3) as u32;
    WEIGHT_IMG_DSC.data = WEIGHT_DATA.as_ptr();

    // Scale icon - far right of card (card is ~492px wide)
    let weight_img = lvgl_sys::lv_img_create(scan_card);
    lvgl_sys::lv_img_set_src(weight_img, &raw const WEIGHT_IMG_DSC as *const _);
    lvgl_sys::lv_obj_set_pos(weight_img, 412, 8);  // Far right: 492 - 64 - 16
    lvgl_sys::lv_obj_set_style_img_recolor(weight_img, lv_color_hex(0xBBBBBB), 0);
    lvgl_sys::lv_obj_set_style_img_recolor_opa(weight_img, 200, 0);

    // Weight value - under scale icon
    let weight_value = lvgl_sys::lv_label_create(scan_card);
    let weight_str = CString::new("0.85 kg").unwrap();
    lvgl_sys::lv_label_set_text(weight_value, weight_str.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(weight_value, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_pos(weight_value, 412, 74);

    // Scale fill bar - under weight value
    let bar_width: i16 = 80;
    let bar_height: i16 = 12;
    let bar_x: i16 = 404;  // Centered under scale icon
    let bar_y: i16 = 98;
    let scale_fill_width = ((bar_width as f32) * (fill_percent as f32 / 100.0)) as i16;

    let bar_bg = lvgl_sys::lv_obj_create(scan_card);
    lvgl_sys::lv_obj_set_size(bar_bg, bar_width, bar_height);
    lvgl_sys::lv_obj_set_pos(bar_bg, bar_x, bar_y);
    lvgl_sys::lv_obj_set_style_bg_color(bar_bg, lv_color_hex(0x0A0A0A), 0);
    lvgl_sys::lv_obj_set_style_radius(bar_bg, 6, 0);
    lvgl_sys::lv_obj_set_style_border_color(bar_bg, lv_color_hex(0x2A2A2A), 0);
    lvgl_sys::lv_obj_set_style_border_width(bar_bg, 1, 0);
    set_style_pad_all(bar_bg, 0);

    let bar_fill = lvgl_sys::lv_obj_create(bar_bg);
    lvgl_sys::lv_obj_set_size(bar_fill, scale_fill_width, bar_height - 2);
    lvgl_sys::lv_obj_set_pos(bar_fill, 1, 1);
    lvgl_sys::lv_obj_set_style_bg_color(bar_fill, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_radius(bar_fill, 5, 0);
    lvgl_sys::lv_obj_set_style_border_width(bar_fill, 0, 0);
    set_style_pad_all(bar_fill, 0);

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

    // Left Nozzle card - taller for more spacing
    let left_nozzle = create_card(scr, 16, ams_y, 380, 118);

    // "L" badge (green circle) - smaller, moved down
    let l_badge = lvgl_sys::lv_obj_create(left_nozzle);
    lvgl_sys::lv_obj_set_size(l_badge, 18, 18);
    lvgl_sys::lv_obj_set_pos(l_badge, 12, 10);
    lvgl_sys::lv_obj_set_style_bg_color(l_badge, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_radius(l_badge, 9, 0);
    lvgl_sys::lv_obj_set_style_border_width(l_badge, 0, 0);
    set_style_pad_all(l_badge, 0);
    let l_letter = lvgl_sys::lv_label_create(l_badge);
    let l_text = CString::new("L").unwrap();
    lvgl_sys::lv_label_set_text(l_letter, l_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(l_letter, lv_color_hex(COLOR_BG), 0);
    lvgl_sys::lv_obj_set_style_text_font(l_letter, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_align(l_letter, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

    let left_label = lvgl_sys::lv_label_create(left_nozzle);
    let left_text = CString::new("Left Nozzle").unwrap();
    lvgl_sys::lv_label_set_text(left_label, left_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(left_label, lv_color_hex(COLOR_GRAY), 0);
    lvgl_sys::lv_obj_set_style_text_font(left_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(left_label, 34, 12);

    // AMS slots for left nozzle - row 1 (A, B, D with 4 color squares each) - moved down
    // Slot A colors: red, yellow, green, salmon - slot 0 (red) is active
    create_ams_slot_4color(left_nozzle, 12, 36, "A", 0, &[0xFF6B6B, 0xFFD93D, 0x6BCB77, 0xFFB5A7]);
    // Slot B colors: blue, dark, light blue, empty - no active slot
    create_ams_slot_4color(left_nozzle, 92, 36, "B", -1, &[0x4D96FF, 0x404040, 0x9ED5FF, 0]);
    // Slot D colors: magenta, purple, light purple, empty - no active slot
    create_ams_slot_4color(left_nozzle, 172, 36, "D", -1, &[0xFF6BD6, 0xC77DFF, 0xE5B8F4, 0]);

    // AMS slots for left nozzle - row 2 (HT then EXT) - swapped order, moved down
    create_ams_slot_single(left_nozzle, 12, 92, "HT-A", 0x9ED5FF, false);
    create_ams_slot_single(left_nozzle, 92, 92, "EXT-1", 0xFF6B6B, false);

    // Right Nozzle card - taller for more spacing
    let right_nozzle = create_card(scr, 404, ams_y, 380, 118);

    // "R" badge (green circle) - smaller, moved down
    let r_badge = lvgl_sys::lv_obj_create(right_nozzle);
    lvgl_sys::lv_obj_set_size(r_badge, 18, 18);
    lvgl_sys::lv_obj_set_pos(r_badge, 12, 10);
    lvgl_sys::lv_obj_set_style_bg_color(r_badge, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_radius(r_badge, 9, 0);
    lvgl_sys::lv_obj_set_style_border_width(r_badge, 0, 0);
    set_style_pad_all(r_badge, 0);
    let r_letter = lvgl_sys::lv_label_create(r_badge);
    let r_text = CString::new("R").unwrap();
    lvgl_sys::lv_label_set_text(r_letter, r_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(r_letter, lv_color_hex(COLOR_BG), 0);
    lvgl_sys::lv_obj_set_style_text_font(r_letter, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_align(r_letter, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

    let right_label = lvgl_sys::lv_label_create(right_nozzle);
    let right_text = CString::new("Right Nozzle").unwrap();
    lvgl_sys::lv_label_set_text(right_label, right_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(right_label, lv_color_hex(COLOR_GRAY), 0);
    lvgl_sys::lv_obj_set_style_text_font(right_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(right_label, 34, 12);

    // AMS slots for right nozzle - row 1 - moved down
    // Slot C colors: yellow, green, cyan, teal - no active slot
    create_ams_slot_4color(right_nozzle, 12, 36, "C", -1, &[0xFFD93D, 0x6BCB77, 0x4ECDC4, 0x45B7AA]);

    // AMS slots for right nozzle - row 2 (HT then EXT) - swapped order, moved down
    create_ams_slot_single(right_nozzle, 12, 92, "HT-B", 0xFFA500, false);
    create_ams_slot_single(right_nozzle, 92, 92, "EXT-2", 0, false);  // Empty (striped)

    // === NOTIFICATION BAR - Premium warning style ===
    let notif_bar = lvgl_sys::lv_obj_create(scr);
    lvgl_sys::lv_obj_set_size(notif_bar, 768, 30);
    lvgl_sys::lv_obj_set_pos(notif_bar, 16, ams_y + 118 + card_gap);
    lvgl_sys::lv_obj_set_style_bg_color(notif_bar, lv_color_hex(0x262626), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(notif_bar, 255, 0);
    lvgl_sys::lv_obj_set_style_radius(notif_bar, 8, 0);  // Smaller radius for compact bar
    lvgl_sys::lv_obj_set_style_border_color(notif_bar, lv_color_hex(0x3D3D3D), 0);
    lvgl_sys::lv_obj_set_style_border_width(notif_bar, 1, 0);
    set_style_pad_all(notif_bar, 0);

    // Subtle orange top accent line
    let top_accent = lvgl_sys::lv_obj_create(notif_bar);
    lvgl_sys::lv_obj_set_size(top_accent, 760, 2);
    lvgl_sys::lv_obj_set_pos(top_accent, 4, 0);
    lvgl_sys::lv_obj_set_style_bg_color(top_accent, lv_color_hex(0xFFA500), 0);  // Orange
    lvgl_sys::lv_obj_set_style_bg_opa(top_accent, 180, 0);
    lvgl_sys::lv_obj_set_style_radius(top_accent, 1, 0);
    lvgl_sys::lv_obj_set_style_border_width(top_accent, 0, 0);
    set_style_pad_all(top_accent, 0);

    // Warning dot with glow
    let dot = lvgl_sys::lv_obj_create(notif_bar);
    lvgl_sys::lv_obj_set_size(dot, 10, 10);
    lvgl_sys::lv_obj_set_pos(dot, 12, 10);
    lvgl_sys::lv_obj_set_style_bg_color(dot, lv_color_hex(0xFFA500), 0); // Orange
    lvgl_sys::lv_obj_set_style_radius(dot, 5, 0);
    lvgl_sys::lv_obj_set_style_border_width(dot, 0, 0);
    // Subtle orange glow for warning emphasis
    lvgl_sys::lv_obj_set_style_shadow_color(dot, lv_color_hex(0xFFA500), 0);
    lvgl_sys::lv_obj_set_style_shadow_width(dot, 8, 0);
    lvgl_sys::lv_obj_set_style_shadow_spread(dot, 2, 0);
    lvgl_sys::lv_obj_set_style_shadow_opa(dot, 150, 0);

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

/// Create AMS Overview screen
unsafe fn create_ams_overview_screen() {
    let disp = lvgl_sys::lv_disp_get_default();
    let scr = lvgl_sys::lv_disp_get_scr_act(disp);

    // Background
    lvgl_sys::lv_obj_set_style_bg_color(scr, lv_color_hex(COLOR_BG), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(scr, 255, 0);
    set_style_pad_all(scr, 0);

    // === STATUS BAR ===
    create_status_bar(scr);

    // === MAIN CONTENT AREA ===
    let content_y: i16 = 48;
    let panel_x: i16 = 8;
    let sidebar_x: i16 = 616;
    let panel_w: i16 = sidebar_x - panel_x - 8;
    let panel_h: i16 = 388;  // Expanded to fill space down to bottom bar

    // === AMS PANEL - ONE container card for all units ===
    let ams_panel = lvgl_sys::lv_obj_create(scr);
    lvgl_sys::lv_obj_set_size(ams_panel, panel_w, panel_h);
    lvgl_sys::lv_obj_set_pos(ams_panel, panel_x, content_y);
    lvgl_sys::lv_obj_set_style_bg_color(ams_panel, lv_color_hex(0x2D2D2D), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(ams_panel, 255, 0);
    lvgl_sys::lv_obj_set_style_radius(ams_panel, 12, 0);
    lvgl_sys::lv_obj_set_style_border_width(ams_panel, 0, 0);
    set_style_pad_all(ams_panel, 10);

    // "AMS Units" title INSIDE the panel
    let title = lvgl_sys::lv_label_create(ams_panel);
    let title_text = CString::new("AMS Units").unwrap();
    lvgl_sys::lv_label_set_text(title, title_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(title, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(title, &lvgl_sys::lv_font_montserrat_14, 0);
    lvgl_sys::lv_obj_set_pos(title, 0, 0);

    // Grid layout inside panel
    let unit_gap: i16 = 4;
    let row1_y: i16 = 22;
    let row1_h: i16 = 170;  // Taller rows to fill expanded panel
    let row2_y: i16 = row1_y + row1_h + unit_gap;
    let row2_h: i16 = 170;

    // Row 1: AMS A, AMS B, AMS C (3 equal width units)
    let inner_w: i16 = panel_w - 20;
    let unit_w_4slot: i16 = (inner_w - 2 * unit_gap) / 3;

    create_ams_unit_compact(ams_panel, 0, row1_y, unit_w_4slot, row1_h,
        "AMS A", "L", "19%", "25°C", true, &[
            ("PLA", 0xF5C518, "A1", "85%", true),
            ("PETG", 0x333333, "A2", "62%", false),
            ("PETG", 0xFF9800, "A3", "45%", false),
            ("PLA", 0x9E9E9E, "A4", "90%", false),
        ]);

    create_ams_unit_compact(ams_panel, unit_w_4slot + unit_gap, row1_y, unit_w_4slot, row1_h,
        "AMS B", "L", "24%", "24°C", false, &[
            ("PLA", 0xE91E63, "B1", "72%", false),
            ("PLA", 0x2196F3, "B2", "55%", false),
            ("PETG", 0x4CAF50, "B3", "33%", false),
            ("", 0, "B4", "", false),
        ]);

    create_ams_unit_compact(ams_panel, 2 * (unit_w_4slot + unit_gap), row1_y, unit_w_4slot, row1_h,
        "AMS C", "R", "31%", "23°C", false, &[
            ("ASA", 0xFFFFFF, "C1", "95%", false),
            ("ASA", 0x212121, "C2", "88%", false),
            ("", 0, "C3", "", false),
            ("", 0, "C4", "", false),
        ]);

    // Row 2: AMS D (4 slots), HT-A, HT-B, Ext 1, Ext 2
    let ams_d_w: i16 = unit_w_4slot;
    let single_w: i16 = (inner_w - ams_d_w - 4 * unit_gap) / 4;

    create_ams_unit_compact(ams_panel, 0, row2_y, ams_d_w, row2_h,
        "AMS D", "R", "28%", "22°C", false, &[
            ("PLA", 0x00BCD4, "D1", "100%", false),
            ("PLA", 0xFF5722, "D2", "67%", false),
            ("", 0, "D3", "", false),
            ("", 0, "D4", "", false),
        ]);

    let sx = ams_d_w + unit_gap;
    create_single_unit_compact(ams_panel, sx, row2_y, single_w, row2_h,
        "HT-A", "L", "42%", "65°C", "ABS", 0x673AB7, "78%");
    create_single_unit_compact(ams_panel, sx + single_w + unit_gap, row2_y, single_w, row2_h,
        "HT-B", "R", "38%", "58°C", "PC", 0xECEFF1, "52%");
    create_ext_unit_compact(ams_panel, sx + 2 * (single_w + unit_gap), row2_y, single_w, row2_h,
        "Ext 1", "L", "TPU", 0x607D8B);
    create_ext_unit_compact(ams_panel, sx + 3 * (single_w + unit_gap), row2_y, single_w, row2_h,
        "Ext 2", "R", "PVA", 0x8BC34A);

    // === RIGHT SIDEBAR - Action buttons (2x2 grid) ===
    let btn_x: i16 = 620;
    let btn_y: i16 = content_y;
    let btn_w: i16 = 82;
    let btn_h: i16 = 82;
    let btn_gap: i16 = 8;

    create_action_button(scr, btn_x, btn_y, btn_w, btn_h, "Scan", "", "nfc");
    create_action_button(scr, btn_x + btn_w + btn_gap, btn_y, btn_w, btn_h, "Catalog", "", "catalog");
    create_action_button(scr, btn_x, btn_y + btn_h + btn_gap, btn_w, btn_h, "Calibrate", "", "calibrate");
    create_action_button(scr, btn_x + btn_w + btn_gap, btn_y + btn_h + btn_gap, btn_w, btn_h, "Settings", "", "settings");

    // === BOTTOM STATUS BAR ===
    create_bottom_status_bar(scr);
}

/// Create status bar (reusable for all screens)
unsafe fn create_status_bar(scr: *mut lvgl_sys::lv_obj_t) {
    let status_bar = lvgl_sys::lv_obj_create(scr);
    lvgl_sys::lv_obj_set_size(status_bar, 800, 44);
    lvgl_sys::lv_obj_set_pos(status_bar, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_color(status_bar, lv_color_hex(COLOR_STATUS_BAR), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(status_bar, 255, 0);
    lvgl_sys::lv_obj_set_style_border_width(status_bar, 0, 0);
    lvgl_sys::lv_obj_set_style_radius(status_bar, 0, 0);
    lvgl_sys::lv_obj_set_style_pad_left(status_bar, 16, 0);
    lvgl_sys::lv_obj_set_style_pad_right(status_bar, 16, 0);
    // Visible bottom shadow for depth separation
    lvgl_sys::lv_obj_set_style_shadow_color(status_bar, lv_color_hex(0x000000), 0);
    lvgl_sys::lv_obj_set_style_shadow_width(status_bar, 25, 0);
    lvgl_sys::lv_obj_set_style_shadow_ofs_y(status_bar, 8, 0);
    lvgl_sys::lv_obj_set_style_shadow_spread(status_bar, 0, 0);
    lvgl_sys::lv_obj_set_style_shadow_opa(status_bar, 200, 0);

    // SpoolBuddy logo
    LOGO_IMG_DSC.header._bitfield_1 = lvgl_sys::lv_img_header_t::new_bitfield_1(
        lvgl_sys::LV_IMG_CF_TRUE_COLOR_ALPHA as u32,
        0, 0,
        LOGO_WIDTH,
        LOGO_HEIGHT,
    );
    LOGO_IMG_DSC.data_size = (LOGO_WIDTH * LOGO_HEIGHT * 3) as u32;
    LOGO_IMG_DSC.data = LOGO_DATA.as_ptr();

    let logo_img = lvgl_sys::lv_img_create(status_bar);
    lvgl_sys::lv_img_set_src(logo_img, &raw const LOGO_IMG_DSC as *const _);
    lvgl_sys::lv_obj_align(logo_img, lvgl_sys::LV_ALIGN_LEFT_MID as u8, 0, 0);

    // Printer selector (center)
    let printer_btn = lvgl_sys::lv_btn_create(status_bar);
    lvgl_sys::lv_obj_set_size(printer_btn, 200, 32);
    lvgl_sys::lv_obj_align(printer_btn, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_color(printer_btn, lv_color_hex(0x242424), 0);
    lvgl_sys::lv_obj_set_style_radius(printer_btn, 16, 0);
    lvgl_sys::lv_obj_set_style_border_color(printer_btn, lv_color_hex(0x3D3D3D), 0);
    lvgl_sys::lv_obj_set_style_border_width(printer_btn, 1, 0);

    let left_dot = lvgl_sys::lv_obj_create(printer_btn);
    lvgl_sys::lv_obj_set_size(left_dot, 8, 8);
    lvgl_sys::lv_obj_align(left_dot, lvgl_sys::LV_ALIGN_LEFT_MID as u8, 12, 0);
    lvgl_sys::lv_obj_set_style_bg_color(left_dot, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_radius(left_dot, 4, 0);
    lvgl_sys::lv_obj_set_style_border_width(left_dot, 0, 0);
    lvgl_sys::lv_obj_set_style_shadow_color(left_dot, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_shadow_width(left_dot, 6, 0);
    lvgl_sys::lv_obj_set_style_shadow_spread(left_dot, 2, 0);
    lvgl_sys::lv_obj_set_style_shadow_opa(left_dot, 150, 0);

    let printer_label = lvgl_sys::lv_label_create(printer_btn);
    let printer_text = CString::new("X1C-Studio").unwrap();
    lvgl_sys::lv_label_set_text(printer_label, printer_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(printer_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_align(printer_label, lvgl_sys::LV_ALIGN_LEFT_MID as u8, 28, 0);

    // Power icon (orange = printing)
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
    lvgl_sys::lv_obj_align(power_img, lvgl_sys::LV_ALIGN_RIGHT_MID as u8, -24, 0);
    lvgl_sys::lv_obj_set_style_img_recolor(power_img, lv_color_hex(0xFFA500), 0);
    lvgl_sys::lv_obj_set_style_img_recolor_opa(power_img, 255, 0);

    let arrow_label = lvgl_sys::lv_label_create(printer_btn);
    let arrow_text = CString::new("v").unwrap();
    lvgl_sys::lv_label_set_text(arrow_label, arrow_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(arrow_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_align(arrow_label, lvgl_sys::LV_ALIGN_RIGHT_MID as u8, -8, 2);

    // Time (rightmost)
    let time_label = lvgl_sys::lv_label_create(status_bar);
    let time_text = CString::new("14:23").unwrap();
    lvgl_sys::lv_label_set_text(time_label, time_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(time_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_align(time_label, lvgl_sys::LV_ALIGN_RIGHT_MID as u8, 0, 0);

    // WiFi bars
    let wifi_x = -50;
    let wifi_bottom = 8;
    let wifi_bar3 = lvgl_sys::lv_obj_create(status_bar);
    lvgl_sys::lv_obj_set_size(wifi_bar3, 4, 16);
    lvgl_sys::lv_obj_align(wifi_bar3, lvgl_sys::LV_ALIGN_RIGHT_MID as u8, wifi_x, wifi_bottom - 8);
    lvgl_sys::lv_obj_set_style_bg_color(wifi_bar3, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(wifi_bar3, 255, 0);
    lvgl_sys::lv_obj_set_style_radius(wifi_bar3, 1, 0);
    lvgl_sys::lv_obj_set_style_border_width(wifi_bar3, 0, 0);

    let wifi_bar2 = lvgl_sys::lv_obj_create(status_bar);
    lvgl_sys::lv_obj_set_size(wifi_bar2, 4, 12);
    lvgl_sys::lv_obj_align(wifi_bar2, lvgl_sys::LV_ALIGN_RIGHT_MID as u8, wifi_x - 6, wifi_bottom - 6);
    lvgl_sys::lv_obj_set_style_bg_color(wifi_bar2, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(wifi_bar2, 255, 0);
    lvgl_sys::lv_obj_set_style_radius(wifi_bar2, 1, 0);
    lvgl_sys::lv_obj_set_style_border_width(wifi_bar2, 0, 0);

    let wifi_bar1 = lvgl_sys::lv_obj_create(status_bar);
    lvgl_sys::lv_obj_set_size(wifi_bar1, 4, 8);
    lvgl_sys::lv_obj_align(wifi_bar1, lvgl_sys::LV_ALIGN_RIGHT_MID as u8, wifi_x - 12, wifi_bottom - 4);
    lvgl_sys::lv_obj_set_style_bg_color(wifi_bar1, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(wifi_bar1, 255, 0);
    lvgl_sys::lv_obj_set_style_radius(wifi_bar1, 1, 0);
    lvgl_sys::lv_obj_set_style_border_width(wifi_bar1, 0, 0);

    // Bell icon
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

    // Notification badge
    let badge = lvgl_sys::lv_obj_create(status_bar);
    lvgl_sys::lv_obj_set_size(badge, 14, 14);
    lvgl_sys::lv_obj_align(badge, lvgl_sys::LV_ALIGN_RIGHT_MID as u8, -70, -8);
    lvgl_sys::lv_obj_set_style_bg_color(badge, lv_color_hex(0xFF4444), 0);
    lvgl_sys::lv_obj_set_style_radius(badge, 7, 0);
    lvgl_sys::lv_obj_set_style_border_width(badge, 0, 0);
    set_style_pad_all(badge, 0);

    let badge_num = lvgl_sys::lv_label_create(badge);
    let badge_text = CString::new("3").unwrap();
    lvgl_sys::lv_label_set_text(badge_num, badge_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(badge_num, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(badge_num, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_align(badge_num, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
}

/// Create AMS unit card v2 - 4 slots with spool icons
unsafe fn create_ams_unit_card_v2(
    parent: *mut lvgl_sys::lv_obj_t,
    x: i16, y: i16, w: i16, h: i16,
    name: &str, humidity: &str, temp: &str, active: bool,
    slots: &[(&str, u32, &str, &str, bool); 4],
) {
    let card = if active {
        create_card_glow(parent, x, y, w, h)
    } else {
        create_card(parent, x, y, w, h)
    };

    // Header row: compact name badge + green dot + stats on right
    let badge = lvgl_sys::lv_obj_create(card);
    lvgl_sys::lv_obj_set_size(badge, 48, 16);
    lvgl_sys::lv_obj_set_pos(badge, 6, 6);
    lvgl_sys::lv_obj_set_style_bg_color(badge, lv_color_hex(0x404040), 0);
    lvgl_sys::lv_obj_set_style_radius(badge, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(badge, 0, 0);
    set_style_pad_all(badge, 0);

    let name_label = lvgl_sys::lv_label_create(badge);
    let name_text = CString::new(name).unwrap();
    lvgl_sys::lv_label_set_text(name_label, name_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(name_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(name_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_align(name_label, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

    // Green active dot next to badge
    if active {
        let dot = lvgl_sys::lv_obj_create(card);
        lvgl_sys::lv_obj_set_size(dot, 6, 6);
        lvgl_sys::lv_obj_set_pos(dot, 58, 11);
        lvgl_sys::lv_obj_set_style_bg_color(dot, lv_color_hex(COLOR_ACCENT), 0);
        lvgl_sys::lv_obj_set_style_radius(dot, 3, 0);
        lvgl_sys::lv_obj_set_style_border_width(dot, 0, 0);
        set_style_pad_all(dot, 0);
    }

    // Humidity + temp on right (smaller font)
    let stats_label = lvgl_sys::lv_label_create(card);
    let stats_str = format!("{} {}", humidity, temp);
    let stats_text = CString::new(stats_str).unwrap();
    lvgl_sys::lv_label_set_text(stats_label, stats_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(stats_label, lv_color_hex(0x707070), 0);
    lvgl_sys::lv_obj_set_style_text_font(stats_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(stats_label, w - 68, 8);

    // Material labels row (above spools)
    let mat_row_y: i16 = 24;
    let slot_spacing: i16 = 46;  // Space between slot centers
    let first_slot_x: i16 = 8;

    for (i, (material, color, _, _, _)) in slots.iter().enumerate() {
        if *color != 0 {
            let mat_label = lvgl_sys::lv_label_create(card);
            let mat_text = CString::new(*material).unwrap();
            lvgl_sys::lv_label_set_text(mat_label, mat_text.as_ptr());
            lvgl_sys::lv_obj_set_style_text_color(mat_label, lv_color_hex(0x808080), 0);
            lvgl_sys::lv_obj_set_style_text_font(mat_label, &lvgl_sys::lv_font_montserrat_12, 0);
            lvgl_sys::lv_obj_set_pos(mat_label, first_slot_x + (i as i16) * slot_spacing, mat_row_y);
        }
    }

    // Spool area - draw colored rectangles as simple spool representation for now
    let spool_y: i16 = 38;
    let spool_h: i16 = 50;

    for (i, (_, color, _, _, _)) in slots.iter().enumerate() {
        if *color != 0 {
            let spool_x = first_slot_x + (i as i16) * slot_spacing;

            // Simple colored rectangle with rounded corners as spool placeholder
            let spool = lvgl_sys::lv_obj_create(card);
            lvgl_sys::lv_obj_set_size(spool, 36, spool_h);
            lvgl_sys::lv_obj_set_pos(spool, spool_x, spool_y);
            lvgl_sys::lv_obj_set_style_bg_color(spool, lv_color_hex(*color), 0);
            lvgl_sys::lv_obj_set_style_radius(spool, 4, 0);
            lvgl_sys::lv_obj_set_style_border_color(spool, lv_color_hex(0x606060), 0);
            lvgl_sys::lv_obj_set_style_border_width(spool, 1, 0);
            set_style_pad_all(spool, 0);
        }
    }

    // Slot ID badges row (dark rounded pills with text)
    let badge_y: i16 = spool_y + spool_h + 4;

    for (i, (_, color, label, _, slot_active)) in slots.iter().enumerate() {
        let badge_x = first_slot_x + (i as i16) * slot_spacing + 4;

        if *color != 0 {
            // Dark rounded badge for slot ID
            let slot_badge = lvgl_sys::lv_obj_create(card);
            lvgl_sys::lv_obj_set_size(slot_badge, 28, 16);
            lvgl_sys::lv_obj_set_pos(slot_badge, badge_x, badge_y);

            if *slot_active {
                // Active slot: green background
                lvgl_sys::lv_obj_set_style_bg_color(slot_badge, lv_color_hex(COLOR_ACCENT), 0);
            } else {
                // Normal slot: dark background
                lvgl_sys::lv_obj_set_style_bg_color(slot_badge, lv_color_hex(0x303030), 0);
            }
            lvgl_sys::lv_obj_set_style_radius(slot_badge, 8, 0);
            lvgl_sys::lv_obj_set_style_border_width(slot_badge, 0, 0);
            set_style_pad_all(slot_badge, 0);

            let slot_label = lvgl_sys::lv_label_create(slot_badge);
            let slot_text = CString::new(*label).unwrap();
            lvgl_sys::lv_label_set_text(slot_label, slot_text.as_ptr());
            let text_color = if *slot_active { 0x1A1A1A } else { COLOR_WHITE };
            lvgl_sys::lv_obj_set_style_text_color(slot_label, lv_color_hex(text_color), 0);
            lvgl_sys::lv_obj_set_style_text_font(slot_label, &lvgl_sys::lv_font_montserrat_12, 0);
            lvgl_sys::lv_obj_align(slot_label, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
        } else {
            // Empty slot: just dimmed text
            let slot_label = lvgl_sys::lv_label_create(card);
            let slot_text = CString::new(*label).unwrap();
            lvgl_sys::lv_label_set_text(slot_label, slot_text.as_ptr());
            lvgl_sys::lv_obj_set_style_text_color(slot_label, lv_color_hex(0x404040), 0);
            lvgl_sys::lv_obj_set_style_text_font(slot_label, &lvgl_sys::lv_font_montserrat_12, 0);
            lvgl_sys::lv_obj_set_pos(slot_label, badge_x + 4, badge_y + 2);
        }
    }

    // Percentage row below badges
    let pct_y: i16 = badge_y + 18;

    for (i, (_, color, _, percent, _)) in slots.iter().enumerate() {
        if *color != 0 && !percent.is_empty() {
            let pct_x = first_slot_x + (i as i16) * slot_spacing + 8;
            let pct_label = lvgl_sys::lv_label_create(card);
            let pct_text = CString::new(*percent).unwrap();
            lvgl_sys::lv_label_set_text(pct_label, pct_text.as_ptr());
            lvgl_sys::lv_obj_set_style_text_color(pct_label, lv_color_hex(0x707070), 0);
            lvgl_sys::lv_obj_set_style_text_font(pct_label, &lvgl_sys::lv_font_montserrat_12, 0);
            lvgl_sys::lv_obj_set_pos(pct_label, pct_x, pct_y);
        }
    }
}

/// Create AMS unit card with 2 slots (for AMS D which shows partial)
unsafe fn create_ams_unit_card_2slot(
    parent: *mut lvgl_sys::lv_obj_t,
    x: i16, y: i16, w: i16, h: i16,
    name: &str, humidity: &str, temp: &str,
    slots: &[(&str, u32, &str, &str); 2],
) {
    let card = create_card(parent, x, y, w, h);

    // Header row
    let badge = lvgl_sys::lv_obj_create(card);
    lvgl_sys::lv_obj_set_size(badge, 54, 18);
    lvgl_sys::lv_obj_set_pos(badge, 8, 8);
    lvgl_sys::lv_obj_set_style_bg_color(badge, lv_color_hex(0x505050), 0);
    lvgl_sys::lv_obj_set_style_radius(badge, 4, 0);
    lvgl_sys::lv_obj_set_style_border_width(badge, 0, 0);
    set_style_pad_all(badge, 0);

    let name_label = lvgl_sys::lv_label_create(badge);
    let name_text = CString::new(name).unwrap();
    lvgl_sys::lv_label_set_text(name_label, name_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(name_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(name_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_align(name_label, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

    // Humidity + temp
    let stats_label = lvgl_sys::lv_label_create(card);
    let stats_str = format!("{} {}", humidity, temp);
    let stats_text = CString::new(stats_str).unwrap();
    lvgl_sys::lv_label_set_text(stats_label, stats_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(stats_label, lv_color_hex(0x808080), 0);
    lvgl_sys::lv_obj_set_style_text_font(stats_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(stats_label, w - 70, 11);

    // 2 spool slots (using 3D spool image 32x42)
    let spool_w: i16 = SPOOL_WIDTH as i16;  // 32
    let spool_h: i16 = SPOOL_HEIGHT as i16; // 42
    let spool_gap: i16 = 12;
    let total_width = spool_w * 2 + spool_gap;
    let spool_start_x = (w - total_width) / 2;
    let spool_y: i16 = 40;
    let label_y: i16 = spool_y + spool_h + 2;

    for (i, (material, color, label, percent)) in slots.iter().enumerate() {
        let sx = spool_start_x + (i as i16) * (spool_w + spool_gap);

        if *color != 0 {
            // Material label
            let mat_label = lvgl_sys::lv_label_create(card);
            let mat_text = CString::new(*material).unwrap();
            lvgl_sys::lv_label_set_text(mat_label, mat_text.as_ptr());
            lvgl_sys::lv_obj_set_style_text_color(mat_label, lv_color_hex(0x909090), 0);
            lvgl_sys::lv_obj_set_style_text_font(mat_label, &lvgl_sys::lv_font_montserrat_12, 0);
            lvgl_sys::lv_obj_set_pos(mat_label, sx, 30);

            // 3D Spool image
            create_spool_icon_v2(card, sx, spool_y, spool_w, *color, false);

            // Slot label
            let slot_label = lvgl_sys::lv_label_create(card);
            let slot_text = CString::new(*label).unwrap();
            lvgl_sys::lv_label_set_text(slot_label, slot_text.as_ptr());
            lvgl_sys::lv_obj_set_style_text_color(slot_label, lv_color_hex(COLOR_WHITE), 0);
            lvgl_sys::lv_obj_set_style_text_font(slot_label, &lvgl_sys::lv_font_montserrat_14, 0);
            lvgl_sys::lv_obj_set_pos(slot_label, sx + 6, label_y + 8);

            // Percentage
            let pct_label = lvgl_sys::lv_label_create(card);
            let pct_text = CString::new(*percent).unwrap();
            lvgl_sys::lv_label_set_text(pct_label, pct_text.as_ptr());
            lvgl_sys::lv_obj_set_style_text_color(pct_label, lv_color_hex(0x808080), 0);
            lvgl_sys::lv_obj_set_style_text_font(pct_label, &lvgl_sys::lv_font_montserrat_12, 0);
            lvgl_sys::lv_obj_set_pos(pct_label, sx + 2, label_y + 24);
        }
    }
}

/// Create a single-slot card (for HT-A, HT-B, Ext 1, Ext 2)
unsafe fn create_single_slot_card(
    parent: *mut lvgl_sys::lv_obj_t,
    x: i16, y: i16, w: i16, h: i16,
    name: &str, humidity: &str, material: &str, color: u32, percent: &str,
) {
    let card = create_card(parent, x, y, w, h);

    // Header with name badge
    let badge = lvgl_sys::lv_obj_create(card);
    lvgl_sys::lv_obj_set_size(badge, 48, 18);
    lvgl_sys::lv_obj_set_pos(badge, 8, 8);
    lvgl_sys::lv_obj_set_style_bg_color(badge, lv_color_hex(0x505050), 0);
    lvgl_sys::lv_obj_set_style_radius(badge, 4, 0);
    lvgl_sys::lv_obj_set_style_border_width(badge, 0, 0);
    set_style_pad_all(badge, 0);

    let name_label = lvgl_sys::lv_label_create(badge);
    let name_text = CString::new(name).unwrap();
    lvgl_sys::lv_label_set_text(name_label, name_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(name_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(name_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_align(name_label, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

    // Humidity (if provided)
    if !humidity.is_empty() {
        let hum_label = lvgl_sys::lv_label_create(card);
        let hum_text = CString::new(humidity).unwrap();
        lvgl_sys::lv_label_set_text(hum_label, hum_text.as_ptr());
        lvgl_sys::lv_obj_set_style_text_color(hum_label, lv_color_hex(0x808080), 0);
        lvgl_sys::lv_obj_set_style_text_font(hum_label, &lvgl_sys::lv_font_montserrat_12, 0);
        lvgl_sys::lv_obj_set_pos(hum_label, w - 36, 11);
    }

    // Single spool icon (using 3D spool image 32x42)
    let spool_w: i16 = SPOOL_WIDTH as i16;  // 32
    let spool_h: i16 = SPOOL_HEIGHT as i16; // 42
    let spool_x = (w - spool_w) / 2;
    let spool_y: i16 = 38;
    create_spool_icon_v2(card, spool_x, spool_y, spool_w, color, false);

    // Material label below spool
    let mat_label = lvgl_sys::lv_label_create(card);
    let mat_text = CString::new(material).unwrap();
    lvgl_sys::lv_label_set_text(mat_label, mat_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(mat_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(mat_label, &lvgl_sys::lv_font_montserrat_14, 0);
    lvgl_sys::lv_obj_set_pos(mat_label, (w - 24) / 2, spool_y + spool_h + 6);

    // Percentage (if provided)
    if !percent.is_empty() {
        let pct_label = lvgl_sys::lv_label_create(card);
        let pct_text = CString::new(percent).unwrap();
        lvgl_sys::lv_label_set_text(pct_label, pct_text.as_ptr());
        lvgl_sys::lv_obj_set_style_text_color(pct_label, lv_color_hex(0x808080), 0);
        lvgl_sys::lv_obj_set_style_text_font(pct_label, &lvgl_sys::lv_font_montserrat_12, 0);
        lvgl_sys::lv_obj_set_pos(pct_label, (w - 24) / 2, spool_y + spool_h + 22);
    }
}

/// Create AMS unit inside a container (no card background)
/// nozzle: "L" or "R" for extruder side badge, or "" for none
unsafe fn create_ams_unit_inside(
    parent: *mut lvgl_sys::lv_obj_t,
    x: i16, y: i16, w: i16, h: i16,
    name: &str, nozzle: &str, humidity: &str, temp: &str, active: bool,
    slots: &[(&str, u32, &str, &str, bool); 4],
) {
    // Container for this unit (transparent, just for positioning)
    let container = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(container, w, h);
    lvgl_sys::lv_obj_set_pos(container, x, y);
    lvgl_sys::lv_obj_set_style_bg_opa(container, 0, 0);
    lvgl_sys::lv_obj_set_style_border_width(container, 0, 0);
    lvgl_sys::lv_obj_set_style_radius(container, 0, 0);
    set_style_pad_all(container, 0);

    // Header row: name badge + optional nozzle badge + stats on right
    let badge = lvgl_sys::lv_obj_create(container);
    lvgl_sys::lv_obj_set_size(badge, 48, 16);
    lvgl_sys::lv_obj_set_pos(badge, 4, 8);
    lvgl_sys::lv_obj_set_style_bg_color(badge, lv_color_hex(0x404040), 0);
    lvgl_sys::lv_obj_set_style_radius(badge, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(badge, 0, 0);
    set_style_pad_all(badge, 0);

    let name_label = lvgl_sys::lv_label_create(badge);
    let name_text = CString::new(name).unwrap();
    lvgl_sys::lv_label_set_text(name_label, name_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(name_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(name_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_align(name_label, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

    // Nozzle badge (R or L) - circular green badge
    if !nozzle.is_empty() {
        let nozzle_badge = lvgl_sys::lv_obj_create(container);
        lvgl_sys::lv_obj_set_size(nozzle_badge, 18, 18);
        lvgl_sys::lv_obj_set_pos(nozzle_badge, 56, 7);
        lvgl_sys::lv_obj_set_style_bg_color(nozzle_badge, lv_color_hex(COLOR_ACCENT), 0);
        lvgl_sys::lv_obj_set_style_radius(nozzle_badge, 9, 0);
        lvgl_sys::lv_obj_set_style_border_width(nozzle_badge, 0, 0);
        set_style_pad_all(nozzle_badge, 0);

        let nozzle_label = lvgl_sys::lv_label_create(nozzle_badge);
        let nozzle_text = CString::new(nozzle).unwrap();
        lvgl_sys::lv_label_set_text(nozzle_label, nozzle_text.as_ptr());
        lvgl_sys::lv_obj_set_style_text_color(nozzle_label, lv_color_hex(0x1A1A1A), 0);
        lvgl_sys::lv_obj_set_style_text_font(nozzle_label, &lvgl_sys::lv_font_montserrat_12, 0);
        lvgl_sys::lv_obj_align(nozzle_label, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    }

    // Humidity icon + value
    let stats_x = w - 80;
    if !humidity.is_empty() {
        // Initialize humidity icon
        HUMIDITY_IMG_DSC.header._bitfield_1 = lvgl_sys::lv_img_header_t::new_bitfield_1(
            lvgl_sys::LV_IMG_CF_TRUE_COLOR_ALPHA as u32,
            0, 0,
            HUMIDITY_WIDTH,
            HUMIDITY_HEIGHT,
        );
        HUMIDITY_IMG_DSC.data_size = (HUMIDITY_WIDTH * HUMIDITY_HEIGHT * 3) as u32;
        HUMIDITY_IMG_DSC.data = HUMIDITY_DATA.as_ptr();

        let hum_icon = lvgl_sys::lv_img_create(container);
        lvgl_sys::lv_img_set_src(hum_icon, &raw const HUMIDITY_IMG_DSC as *const _);
        lvgl_sys::lv_obj_set_pos(hum_icon, stats_x, 9);
        lvgl_sys::lv_obj_set_style_img_recolor(hum_icon, lv_color_hex(0x4FC3F7), 0);  // Light blue for humidity
        lvgl_sys::lv_obj_set_style_img_recolor_opa(hum_icon, 255, 0);

        let hum_label = lvgl_sys::lv_label_create(container);
        let hum_text = CString::new(humidity).unwrap();
        lvgl_sys::lv_label_set_text(hum_label, hum_text.as_ptr());
        lvgl_sys::lv_obj_set_style_text_color(hum_label, lv_color_hex(0x707070), 0);
        lvgl_sys::lv_obj_set_style_text_font(hum_label, &lvgl_sys::lv_font_montserrat_12, 0);
        lvgl_sys::lv_obj_set_pos(hum_label, stats_x + 14, 10);
    }

    // Temperature icon + value
    if !temp.is_empty() {
        // Initialize temperature icon
        TEMP_IMG_DSC.header._bitfield_1 = lvgl_sys::lv_img_header_t::new_bitfield_1(
            lvgl_sys::LV_IMG_CF_TRUE_COLOR_ALPHA as u32,
            0, 0,
            TEMP_WIDTH,
            TEMP_HEIGHT,
        );
        TEMP_IMG_DSC.data_size = (TEMP_WIDTH * TEMP_HEIGHT * 3) as u32;
        TEMP_IMG_DSC.data = TEMP_DATA.as_ptr();

        let temp_icon = lvgl_sys::lv_img_create(container);
        lvgl_sys::lv_img_set_src(temp_icon, &raw const TEMP_IMG_DSC as *const _);
        lvgl_sys::lv_obj_set_pos(temp_icon, stats_x + 38, 9);
        lvgl_sys::lv_obj_set_style_img_recolor(temp_icon, lv_color_hex(0xFF7043), 0);  // Orange for temperature
        lvgl_sys::lv_obj_set_style_img_recolor_opa(temp_icon, 255, 0);

        let temp_label = lvgl_sys::lv_label_create(container);
        let temp_text = CString::new(temp).unwrap();
        lvgl_sys::lv_label_set_text(temp_label, temp_text.as_ptr());
        lvgl_sys::lv_obj_set_style_text_color(temp_label, lv_color_hex(0x707070), 0);
        lvgl_sys::lv_obj_set_style_text_font(temp_label, &lvgl_sys::lv_font_montserrat_12, 0);
        lvgl_sys::lv_obj_set_pos(temp_label, stats_x + 52, 10);
    }

    // Material labels row (above spools)
    let mat_row_y: i16 = 26;
    let slot_spacing: i16 = 42;
    let first_slot_x: i16 = 6;

    for (i, (material, color, _, _, _)) in slots.iter().enumerate() {
        if *color != 0 {
            let mat_label = lvgl_sys::lv_label_create(container);
            let mat_text = CString::new(*material).unwrap();
            lvgl_sys::lv_label_set_text(mat_label, mat_text.as_ptr());
            lvgl_sys::lv_obj_set_style_text_color(mat_label, lv_color_hex(0x808080), 0);
            lvgl_sys::lv_obj_set_style_text_font(mat_label, &lvgl_sys::lv_font_montserrat_12, 0);
            lvgl_sys::lv_obj_set_pos(mat_label, first_slot_x + (i as i16) * slot_spacing, mat_row_y);
        }
    }

    // Spool area - draw colored rectangles
    let spool_y: i16 = 40;
    let spool_h: i16 = 50;

    for (i, (_, color, _, _, _)) in slots.iter().enumerate() {
        if *color != 0 {
            let spool_x = first_slot_x + (i as i16) * slot_spacing;

            let spool = lvgl_sys::lv_obj_create(container);
            lvgl_sys::lv_obj_set_size(spool, 32, spool_h);
            lvgl_sys::lv_obj_set_pos(spool, spool_x, spool_y);
            lvgl_sys::lv_obj_set_style_bg_color(spool, lv_color_hex(*color), 0);
            lvgl_sys::lv_obj_set_style_radius(spool, 4, 0);
            lvgl_sys::lv_obj_set_style_border_color(spool, lv_color_hex(0x606060), 0);
            lvgl_sys::lv_obj_set_style_border_width(spool, 1, 0);
            set_style_pad_all(spool, 0);
        }
    }

    // Slot ID badges row
    let badge_y: i16 = spool_y + spool_h + 4;

    for (i, (_, color, label, _, slot_active)) in slots.iter().enumerate() {
        let badge_x = first_slot_x + (i as i16) * slot_spacing + 2;

        if *color != 0 {
            let slot_badge = lvgl_sys::lv_obj_create(container);
            lvgl_sys::lv_obj_set_size(slot_badge, 28, 16);
            lvgl_sys::lv_obj_set_pos(slot_badge, badge_x, badge_y);

            if *slot_active {
                lvgl_sys::lv_obj_set_style_bg_color(slot_badge, lv_color_hex(COLOR_ACCENT), 0);
            } else {
                lvgl_sys::lv_obj_set_style_bg_color(slot_badge, lv_color_hex(0x303030), 0);
            }
            lvgl_sys::lv_obj_set_style_radius(slot_badge, 8, 0);
            lvgl_sys::lv_obj_set_style_border_width(slot_badge, 0, 0);
            set_style_pad_all(slot_badge, 0);

            let slot_label = lvgl_sys::lv_label_create(slot_badge);
            let slot_text = CString::new(*label).unwrap();
            lvgl_sys::lv_label_set_text(slot_label, slot_text.as_ptr());
            let text_color = if *slot_active { 0x1A1A1A } else { COLOR_WHITE };
            lvgl_sys::lv_obj_set_style_text_color(slot_label, lv_color_hex(text_color), 0);
            lvgl_sys::lv_obj_set_style_text_font(slot_label, &lvgl_sys::lv_font_montserrat_12, 0);
            lvgl_sys::lv_obj_align(slot_label, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
        } else {
            let slot_label = lvgl_sys::lv_label_create(container);
            let slot_text = CString::new(*label).unwrap();
            lvgl_sys::lv_label_set_text(slot_label, slot_text.as_ptr());
            lvgl_sys::lv_obj_set_style_text_color(slot_label, lv_color_hex(0x404040), 0);
            lvgl_sys::lv_obj_set_style_text_font(slot_label, &lvgl_sys::lv_font_montserrat_12, 0);
            lvgl_sys::lv_obj_set_pos(slot_label, badge_x + 4, badge_y + 2);
        }
    }

    // Percentage row below badges
    let pct_y: i16 = badge_y + 18;

    for (i, (_, color, _, percent, _)) in slots.iter().enumerate() {
        if *color != 0 && !percent.is_empty() {
            let pct_x = first_slot_x + (i as i16) * slot_spacing + 6;
            let pct_label = lvgl_sys::lv_label_create(container);
            let pct_text = CString::new(*percent).unwrap();
            lvgl_sys::lv_label_set_text(pct_label, pct_text.as_ptr());
            lvgl_sys::lv_obj_set_style_text_color(pct_label, lv_color_hex(0x707070), 0);
            lvgl_sys::lv_obj_set_style_text_font(pct_label, &lvgl_sys::lv_font_montserrat_12, 0);
            lvgl_sys::lv_obj_set_pos(pct_label, pct_x, pct_y);
        }
    }

    // Active indicator: subtle border around the whole unit area
    if active {
        lvgl_sys::lv_obj_set_style_border_color(container, lv_color_hex(COLOR_ACCENT), 0);
        lvgl_sys::lv_obj_set_style_border_width(container, 1, 0);
        lvgl_sys::lv_obj_set_style_border_side(container, lvgl_sys::LV_BORDER_SIDE_RIGHT as u8, 0);
    }
}

/// Create 2-slot AMS unit inside a container (no card background)
unsafe fn create_ams_unit_2slot_inside(
    parent: *mut lvgl_sys::lv_obj_t,
    x: i16, y: i16, w: i16, h: i16,
    name: &str, _nozzle: &str, humidity: &str, temp: &str,
    slots: &[(&str, u32, &str, &str); 2],
) {
    let container = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(container, w, h);
    lvgl_sys::lv_obj_set_pos(container, x, y);
    lvgl_sys::lv_obj_set_style_bg_opa(container, 0, 0);
    lvgl_sys::lv_obj_set_style_border_width(container, 0, 0);
    lvgl_sys::lv_obj_set_style_radius(container, 0, 0);
    set_style_pad_all(container, 0);

    // Header with name badge
    let badge = lvgl_sys::lv_obj_create(container);
    lvgl_sys::lv_obj_set_size(badge, 54, 16);
    lvgl_sys::lv_obj_set_pos(badge, 4, 8);
    lvgl_sys::lv_obj_set_style_bg_color(badge, lv_color_hex(0x404040), 0);
    lvgl_sys::lv_obj_set_style_radius(badge, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(badge, 0, 0);
    set_style_pad_all(badge, 0);

    let name_label = lvgl_sys::lv_label_create(badge);
    let name_text = CString::new(name).unwrap();
    lvgl_sys::lv_label_set_text(name_label, name_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(name_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(name_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_align(name_label, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

    // Stats
    if !humidity.is_empty() || !temp.is_empty() {
        let stats_label = lvgl_sys::lv_label_create(container);
        let stats_str = format!("{} {}", humidity, temp);
        let stats_text = CString::new(stats_str).unwrap();
        lvgl_sys::lv_label_set_text(stats_label, stats_text.as_ptr());
        lvgl_sys::lv_obj_set_style_text_color(stats_label, lv_color_hex(0x707070), 0);
        lvgl_sys::lv_obj_set_style_text_font(stats_label, &lvgl_sys::lv_font_montserrat_12, 0);
        lvgl_sys::lv_obj_set_pos(stats_label, 4, 26);
    }

    // 2 spool slots
    let spool_w: i16 = 32;
    let spool_h: i16 = 50;
    let spool_gap: i16 = 8;
    let total_width = spool_w * 2 + spool_gap;
    let spool_start_x = (w - total_width) / 2;
    let spool_y: i16 = 42;
    let label_y: i16 = spool_y + spool_h + 4;

    for (i, (material, color, label, percent)) in slots.iter().enumerate() {
        let sx = spool_start_x + (i as i16) * (spool_w + spool_gap);

        if *color != 0 {
            // Spool rectangle
            let spool = lvgl_sys::lv_obj_create(container);
            lvgl_sys::lv_obj_set_size(spool, spool_w, spool_h);
            lvgl_sys::lv_obj_set_pos(spool, sx, spool_y);
            lvgl_sys::lv_obj_set_style_bg_color(spool, lv_color_hex(*color), 0);
            lvgl_sys::lv_obj_set_style_radius(spool, 4, 0);
            lvgl_sys::lv_obj_set_style_border_color(spool, lv_color_hex(0x606060), 0);
            lvgl_sys::lv_obj_set_style_border_width(spool, 1, 0);
            set_style_pad_all(spool, 0);

            // Material label above
            let mat_label = lvgl_sys::lv_label_create(container);
            let mat_text = CString::new(*material).unwrap();
            lvgl_sys::lv_label_set_text(mat_label, mat_text.as_ptr());
            lvgl_sys::lv_obj_set_style_text_color(mat_label, lv_color_hex(0x808080), 0);
            lvgl_sys::lv_obj_set_style_text_font(mat_label, &lvgl_sys::lv_font_montserrat_12, 0);
            lvgl_sys::lv_obj_set_pos(mat_label, sx, spool_y - 14);

            // Slot badge
            let slot_badge = lvgl_sys::lv_obj_create(container);
            lvgl_sys::lv_obj_set_size(slot_badge, 28, 16);
            lvgl_sys::lv_obj_set_pos(slot_badge, sx + 2, label_y);
            lvgl_sys::lv_obj_set_style_bg_color(slot_badge, lv_color_hex(0x303030), 0);
            lvgl_sys::lv_obj_set_style_radius(slot_badge, 8, 0);
            lvgl_sys::lv_obj_set_style_border_width(slot_badge, 0, 0);
            set_style_pad_all(slot_badge, 0);

            let slot_label = lvgl_sys::lv_label_create(slot_badge);
            let slot_text = CString::new(*label).unwrap();
            lvgl_sys::lv_label_set_text(slot_label, slot_text.as_ptr());
            lvgl_sys::lv_obj_set_style_text_color(slot_label, lv_color_hex(COLOR_WHITE), 0);
            lvgl_sys::lv_obj_set_style_text_font(slot_label, &lvgl_sys::lv_font_montserrat_12, 0);
            lvgl_sys::lv_obj_align(slot_label, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

            // Percentage
            if !percent.is_empty() {
                let pct_label = lvgl_sys::lv_label_create(container);
                let pct_text = CString::new(*percent).unwrap();
                lvgl_sys::lv_label_set_text(pct_label, pct_text.as_ptr());
                lvgl_sys::lv_obj_set_style_text_color(pct_label, lv_color_hex(0x707070), 0);
                lvgl_sys::lv_obj_set_style_text_font(pct_label, &lvgl_sys::lv_font_montserrat_12, 0);
                lvgl_sys::lv_obj_set_pos(pct_label, sx + 6, label_y + 18);
            }
        }
    }
}

/// Create single slot unit inside a container (no card background)
unsafe fn create_single_unit_inside(
    parent: *mut lvgl_sys::lv_obj_t,
    x: i16, y: i16, w: i16, h: i16,
    name: &str, humidity: &str, material: &str, color: u32, percent: &str,
) {
    let container = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(container, w, h);
    lvgl_sys::lv_obj_set_pos(container, x, y);
    lvgl_sys::lv_obj_set_style_bg_opa(container, 0, 0);
    lvgl_sys::lv_obj_set_style_border_width(container, 0, 0);
    lvgl_sys::lv_obj_set_style_radius(container, 0, 0);
    set_style_pad_all(container, 0);

    // Header with name badge
    let badge = lvgl_sys::lv_obj_create(container);
    let badge_w = if name.len() > 4 { 42 } else { 36 };
    lvgl_sys::lv_obj_set_size(badge, badge_w, 16);
    lvgl_sys::lv_obj_set_pos(badge, (w - badge_w) / 2, 8);
    lvgl_sys::lv_obj_set_style_bg_color(badge, lv_color_hex(0x404040), 0);
    lvgl_sys::lv_obj_set_style_radius(badge, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(badge, 0, 0);
    set_style_pad_all(badge, 0);

    let name_label = lvgl_sys::lv_label_create(badge);
    let name_text = CString::new(name).unwrap();
    lvgl_sys::lv_label_set_text(name_label, name_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(name_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(name_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_align(name_label, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

    // Humidity (if provided)
    if !humidity.is_empty() {
        let hum_label = lvgl_sys::lv_label_create(container);
        let hum_text = CString::new(humidity).unwrap();
        lvgl_sys::lv_label_set_text(hum_label, hum_text.as_ptr());
        lvgl_sys::lv_obj_set_style_text_color(hum_label, lv_color_hex(0x707070), 0);
        lvgl_sys::lv_obj_set_style_text_font(hum_label, &lvgl_sys::lv_font_montserrat_12, 0);
        lvgl_sys::lv_obj_set_pos(hum_label, (w - 24) / 2, 26);
    }

    // Single spool
    let spool_w: i16 = 32;
    let spool_h: i16 = 50;
    let spool_x = (w - spool_w) / 2;
    let spool_y: i16 = 40;

    let spool = lvgl_sys::lv_obj_create(container);
    lvgl_sys::lv_obj_set_size(spool, spool_w, spool_h);
    lvgl_sys::lv_obj_set_pos(spool, spool_x, spool_y);
    lvgl_sys::lv_obj_set_style_bg_color(spool, lv_color_hex(color), 0);
    lvgl_sys::lv_obj_set_style_radius(spool, 4, 0);
    lvgl_sys::lv_obj_set_style_border_color(spool, lv_color_hex(0x606060), 0);
    lvgl_sys::lv_obj_set_style_border_width(spool, 1, 0);
    set_style_pad_all(spool, 0);

    // Material label
    let mat_label = lvgl_sys::lv_label_create(container);
    let mat_text = CString::new(material).unwrap();
    lvgl_sys::lv_label_set_text(mat_label, mat_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(mat_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(mat_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(mat_label, (w - 24) / 2, spool_y + spool_h + 6);

    // Percentage (if provided)
    if !percent.is_empty() {
        let pct_label = lvgl_sys::lv_label_create(container);
        let pct_text = CString::new(percent).unwrap();
        lvgl_sys::lv_label_set_text(pct_label, pct_text.as_ptr());
        lvgl_sys::lv_obj_set_style_text_color(pct_label, lv_color_hex(0x707070), 0);
        lvgl_sys::lv_obj_set_style_text_font(pct_label, &lvgl_sys::lv_font_montserrat_12, 0);
        lvgl_sys::lv_obj_set_pos(pct_label, (w - 24) / 2, spool_y + spool_h + 20);
    }
}

/// Create AMS unit card - EXACT mockup match
/// Card with border, header (name + nozzle + humidity/temp icons), and 4 spool slots
unsafe fn create_ams_unit_card_exact(
    parent: *mut lvgl_sys::lv_obj_t,
    x: i16, y: i16, w: i16, h: i16,
    name: &str, nozzle: &str, humidity: &str, temp: &str, active: bool,
    slots: &[(&str, u32, &str, &str, bool)],  // (material, color, slot_id, fill_pct, slot_active)
) {
    // Card with border matching mockup: 2px solid #404040, 8px radius
    let card = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(card, w, h);
    lvgl_sys::lv_obj_set_pos(card, x, y);
    // Gradient matching mockup: #3D3D3D -> #2A2A2A (less extreme to avoid banding)
    lvgl_sys::lv_obj_set_style_bg_color(card, lv_color_hex(0x3D3D3D), 0);  // Top
    lvgl_sys::lv_obj_set_style_bg_grad_color(card, lv_color_hex(0x2A2A2A), 0);  // Bottom
    lvgl_sys::lv_obj_set_style_bg_grad_dir(card, lvgl_sys::LV_GRAD_DIR_VER as u8, 0);
    lvgl_sys::lv_obj_set_style_bg_opa(card, 255, 0);
    lvgl_sys::lv_obj_set_style_radius(card, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(card, 2, 0);
    if active {
        lvgl_sys::lv_obj_set_style_border_color(card, lv_color_hex(COLOR_ACCENT), 0);
    } else {
        lvgl_sys::lv_obj_set_style_border_color(card, lv_color_hex(0x404040), 0);
    }
    set_style_pad_all(card, 6);

    // === HEADER ROW ===
    // Left side: name label + nozzle badge
    let name_label = lvgl_sys::lv_label_create(card);
    let name_text = CString::new(name).unwrap();
    lvgl_sys::lv_label_set_text(name_label, name_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(name_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(name_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(name_label, 4, 4);

    // Nozzle badge (L or R) - small green pill
    if !nozzle.is_empty() {
        let nozzle_badge = lvgl_sys::lv_obj_create(card);
        lvgl_sys::lv_obj_set_size(nozzle_badge, 14, 12);
        lvgl_sys::lv_obj_set_pos(nozzle_badge, 48, 4);
        lvgl_sys::lv_obj_set_style_bg_color(nozzle_badge, lv_color_hex(COLOR_ACCENT), 0);
        lvgl_sys::lv_obj_set_style_radius(nozzle_badge, 3, 0);
        lvgl_sys::lv_obj_set_style_border_width(nozzle_badge, 0, 0);
        set_style_pad_all(nozzle_badge, 0);

        let nozzle_label = lvgl_sys::lv_label_create(nozzle_badge);
        let nozzle_text = CString::new(nozzle).unwrap();
        lvgl_sys::lv_label_set_text(nozzle_label, nozzle_text.as_ptr());
        lvgl_sys::lv_obj_set_style_text_color(nozzle_label, lv_color_hex(0x1A1A1A), 0);
        lvgl_sys::lv_obj_set_style_text_font(nozzle_label, &lvgl_sys::lv_font_montserrat_12, 0);
        lvgl_sys::lv_obj_align(nozzle_label, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    }

    // Right side: humidity icon + value, temp icon + value
    let stats_right_x = w - 78;

    // Initialize humidity icon
    HUMIDITY_IMG_DSC.header._bitfield_1 = lvgl_sys::lv_img_header_t::new_bitfield_1(
        lvgl_sys::LV_IMG_CF_TRUE_COLOR_ALPHA as u32,
        0, 0,
        HUMIDITY_WIDTH,
        HUMIDITY_HEIGHT,
    );
    HUMIDITY_IMG_DSC.data_size = (HUMIDITY_WIDTH * HUMIDITY_HEIGHT * 3) as u32;
    HUMIDITY_IMG_DSC.data = HUMIDITY_DATA.as_ptr();

    let hum_icon = lvgl_sys::lv_img_create(card);
    lvgl_sys::lv_img_set_src(hum_icon, &raw const HUMIDITY_IMG_DSC as *const _);
    lvgl_sys::lv_obj_set_pos(hum_icon, stats_right_x, 5);
    lvgl_sys::lv_obj_set_style_img_recolor(hum_icon, lv_color_hex(0x4FC3F7), 0);
    lvgl_sys::lv_obj_set_style_img_recolor_opa(hum_icon, 255, 0);

    let hum_label = lvgl_sys::lv_label_create(card);
    let hum_text = CString::new(humidity).unwrap();
    lvgl_sys::lv_label_set_text(hum_label, hum_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(hum_label, lv_color_hex(0x808080), 0);
    lvgl_sys::lv_obj_set_style_text_font(hum_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(hum_label, stats_right_x + 14, 5);

    // Initialize temperature icon
    TEMP_IMG_DSC.header._bitfield_1 = lvgl_sys::lv_img_header_t::new_bitfield_1(
        lvgl_sys::LV_IMG_CF_TRUE_COLOR_ALPHA as u32,
        0, 0,
        TEMP_WIDTH,
        TEMP_HEIGHT,
    );
    TEMP_IMG_DSC.data_size = (TEMP_WIDTH * TEMP_HEIGHT * 3) as u32;
    TEMP_IMG_DSC.data = TEMP_DATA.as_ptr();

    let temp_icon = lvgl_sys::lv_img_create(card);
    lvgl_sys::lv_img_set_src(temp_icon, &raw const TEMP_IMG_DSC as *const _);
    lvgl_sys::lv_obj_set_pos(temp_icon, stats_right_x + 38, 5);
    lvgl_sys::lv_obj_set_style_img_recolor(temp_icon, lv_color_hex(0xFF8A65), 0);
    lvgl_sys::lv_obj_set_style_img_recolor_opa(temp_icon, 255, 0);

    let temp_label = lvgl_sys::lv_label_create(card);
    let temp_text = CString::new(temp).unwrap();
    lvgl_sys::lv_label_set_text(temp_label, temp_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(temp_label, lv_color_hex(0x808080), 0);
    lvgl_sys::lv_obj_set_style_text_font(temp_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(temp_label, stats_right_x + 52, 5);

    // === SPOOL ROW ===
    // Layout: evenly spaced across card width
    let num_slots = slots.len().min(4);
    let spool_w: i16 = 32;
    let spool_h: i16 = 42;
    let total_spool_width = (num_slots as i16) * spool_w;
    let gap = if num_slots > 1 { (w - 12 - total_spool_width) / (num_slots as i16 - 1).max(1) } else { 0 };
    let start_x: i16 = 6;
    let spool_y: i16 = 32;

    for (i, (material, color, slot_id, fill_pct, slot_active)) in slots.iter().enumerate() {
        let sx = start_x + (i as i16) * (spool_w + gap);

        // Material label above spool
        let mat_label = lvgl_sys::lv_label_create(card);
        if *color != 0 {
            let mat_text = CString::new(*material).unwrap();
            lvgl_sys::lv_label_set_text(mat_label, mat_text.as_ptr());
        } else {
            let mat_text = CString::new("--").unwrap();
            lvgl_sys::lv_label_set_text(mat_label, mat_text.as_ptr());
        }
        lvgl_sys::lv_obj_set_style_text_color(mat_label, lv_color_hex(0x808080), 0);
        lvgl_sys::lv_obj_set_style_text_font(mat_label, &lvgl_sys::lv_font_montserrat_12, 0);
        lvgl_sys::lv_obj_set_pos(mat_label, sx, spool_y - 12);

        // Spool visual
        create_spool_visual_exact(card, sx, spool_y, *color, *slot_active);

        // Slot ID badge
        let badge_y = spool_y + spool_h + 2;
        let slot_badge = lvgl_sys::lv_obj_create(card);
        lvgl_sys::lv_obj_set_size(slot_badge, 28, 14);
        lvgl_sys::lv_obj_set_pos(slot_badge, sx + 2, badge_y);
        if *slot_active {
            lvgl_sys::lv_obj_set_style_bg_color(slot_badge, lv_color_hex(COLOR_ACCENT), 0);
        } else if *color != 0 {
            lvgl_sys::lv_obj_set_style_bg_color(slot_badge, lv_color_hex(0x404040), 0);
        } else {
            lvgl_sys::lv_obj_set_style_bg_color(slot_badge, lv_color_hex(0x303030), 0);
        }
        lvgl_sys::lv_obj_set_style_radius(slot_badge, 7, 0);
        lvgl_sys::lv_obj_set_style_border_width(slot_badge, 0, 0);
        set_style_pad_all(slot_badge, 0);

        let slot_label = lvgl_sys::lv_label_create(slot_badge);
        let slot_text = CString::new(*slot_id).unwrap();
        lvgl_sys::lv_label_set_text(slot_label, slot_text.as_ptr());
        let text_color = if *slot_active { 0x1A1A1A } else { COLOR_WHITE };
        lvgl_sys::lv_obj_set_style_text_color(slot_label, lv_color_hex(text_color), 0);
        lvgl_sys::lv_obj_set_style_text_font(slot_label, &lvgl_sys::lv_font_montserrat_12, 0);
        lvgl_sys::lv_obj_align(slot_label, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

        // Fill percentage
        let pct_y = badge_y + 16;
        let pct_label = lvgl_sys::lv_label_create(card);
        if *color != 0 && !fill_pct.is_empty() {
            let pct_text = CString::new(*fill_pct).unwrap();
            lvgl_sys::lv_label_set_text(pct_label, pct_text.as_ptr());
        } else {
            let pct_text = CString::new("--").unwrap();
            lvgl_sys::lv_label_set_text(pct_label, pct_text.as_ptr());
        }
        lvgl_sys::lv_obj_set_style_text_color(pct_label, lv_color_hex(0x707070), 0);
        lvgl_sys::lv_obj_set_style_text_font(pct_label, &lvgl_sys::lv_font_montserrat_12, 0);
        lvgl_sys::lv_obj_set_pos(pct_label, sx + 6, pct_y);
    }
}

/// Create spool visual - gray frame with colored fill overlay matching mockup
unsafe fn create_spool_visual_exact(parent: *mut lvgl_sys::lv_obj_t, x: i16, y: i16, color: u32, _active: bool) {
    let spool_w: i16 = 32;
    let spool_h: i16 = 42;

    // Container for spool
    let container = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(container, spool_w, spool_h);
    lvgl_sys::lv_obj_set_pos(container, x, y);
    lvgl_sys::lv_obj_set_style_bg_opa(container, 0, 0);
    lvgl_sys::lv_obj_set_style_border_width(container, 0, 0);
    set_style_pad_all(container, 0);

    // Initialize spool frame (gray flanges)
    SPOOL_FRAME_IMG_DSC.header._bitfield_1 = lvgl_sys::lv_img_header_t::new_bitfield_1(
        lvgl_sys::LV_IMG_CF_TRUE_COLOR_ALPHA as u32,
        0, 0,
        SPOOL_WIDTH,
        SPOOL_HEIGHT,
    );
    SPOOL_FRAME_IMG_DSC.data_size = (SPOOL_WIDTH * SPOOL_HEIGHT * 3) as u32;
    SPOOL_FRAME_IMG_DSC.data = SPOOL_FRAME_DATA.as_ptr();

    // Draw the gray spool frame
    let frame_img = lvgl_sys::lv_img_create(container);
    lvgl_sys::lv_img_set_src(frame_img, &raw const SPOOL_FRAME_IMG_DSC as *const _);
    lvgl_sys::lv_obj_set_pos(frame_img, 0, 0);

    // Draw color tint overlay (positioned like CSS: top:12%, left:27%, right:22%, bottom:12%)
    if color != 0 {
        // Calculate overlay position from percentages
        let top = (spool_h * 12) / 100;      // ~5px
        let left = (spool_w * 27) / 100;     // ~9px
        let right = (spool_w * 22) / 100;    // ~7px
        let bottom = (spool_h * 12) / 100;   // ~5px
        let overlay_w = spool_w - left - right;  // ~16px
        let overlay_h = spool_h - top - bottom;  // ~32px

        let color_overlay = lvgl_sys::lv_obj_create(container);
        lvgl_sys::lv_obj_set_size(color_overlay, overlay_w, overlay_h);
        lvgl_sys::lv_obj_set_pos(color_overlay, left, top);
        lvgl_sys::lv_obj_set_style_bg_color(color_overlay, lv_color_hex(color), 0);
        lvgl_sys::lv_obj_set_style_bg_opa(color_overlay, 216, 0);  // 85% of 255
        lvgl_sys::lv_obj_set_style_radius(color_overlay, 2, 0);
        lvgl_sys::lv_obj_set_style_border_width(color_overlay, 0, 0);
        set_style_pad_all(color_overlay, 0);
    }
}

/// Create single-slot unit card (HT-A, HT-B) - EXACT mockup match
unsafe fn create_single_unit_card_exact(
    parent: *mut lvgl_sys::lv_obj_t,
    x: i16, y: i16, w: i16, h: i16,
    name: &str, nozzle: &str, humidity: &str, temp: &str,
    material: &str, color: u32, fill_pct: &str,
) {
    // Card with border matching mockup
    let card = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(card, w, h);
    lvgl_sys::lv_obj_set_pos(card, x, y);
    lvgl_sys::lv_obj_set_style_bg_color(card, lv_color_hex(0x2D2D2D), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(card, 255, 0);
    lvgl_sys::lv_obj_set_style_radius(card, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(card, 2, 0);
    lvgl_sys::lv_obj_set_style_border_color(card, lv_color_hex(0x404040), 0);
    set_style_pad_all(card, 6);

    // === HEADER ROW ===
    let name_label = lvgl_sys::lv_label_create(card);
    let name_text = CString::new(name).unwrap();
    lvgl_sys::lv_label_set_text(name_label, name_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(name_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(name_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(name_label, 4, 4);

    // Nozzle badge
    if !nozzle.is_empty() {
        let nozzle_badge = lvgl_sys::lv_obj_create(card);
        lvgl_sys::lv_obj_set_size(nozzle_badge, 14, 12);
        lvgl_sys::lv_obj_set_pos(nozzle_badge, 42, 4);
        lvgl_sys::lv_obj_set_style_bg_color(nozzle_badge, lv_color_hex(COLOR_ACCENT), 0);
        lvgl_sys::lv_obj_set_style_radius(nozzle_badge, 3, 0);
        lvgl_sys::lv_obj_set_style_border_width(nozzle_badge, 0, 0);
        set_style_pad_all(nozzle_badge, 0);

        let nozzle_label = lvgl_sys::lv_label_create(nozzle_badge);
        let nozzle_text = CString::new(nozzle).unwrap();
        lvgl_sys::lv_label_set_text(nozzle_label, nozzle_text.as_ptr());
        lvgl_sys::lv_obj_set_style_text_color(nozzle_label, lv_color_hex(0x1A1A1A), 0);
        lvgl_sys::lv_obj_set_style_text_font(nozzle_label, &lvgl_sys::lv_font_montserrat_12, 0);
        lvgl_sys::lv_obj_align(nozzle_label, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    }

    // Humidity/temp on second line for narrow cards
    let stats_y: i16 = 18;

    HUMIDITY_IMG_DSC.header._bitfield_1 = lvgl_sys::lv_img_header_t::new_bitfield_1(
        lvgl_sys::LV_IMG_CF_TRUE_COLOR_ALPHA as u32,
        0, 0,
        HUMIDITY_WIDTH,
        HUMIDITY_HEIGHT,
    );
    HUMIDITY_IMG_DSC.data_size = (HUMIDITY_WIDTH * HUMIDITY_HEIGHT * 3) as u32;
    HUMIDITY_IMG_DSC.data = HUMIDITY_DATA.as_ptr();

    let hum_icon = lvgl_sys::lv_img_create(card);
    lvgl_sys::lv_img_set_src(hum_icon, &raw const HUMIDITY_IMG_DSC as *const _);
    lvgl_sys::lv_obj_set_pos(hum_icon, 4, stats_y);
    lvgl_sys::lv_obj_set_style_img_recolor(hum_icon, lv_color_hex(0x4FC3F7), 0);
    lvgl_sys::lv_obj_set_style_img_recolor_opa(hum_icon, 255, 0);

    let hum_label = lvgl_sys::lv_label_create(card);
    let hum_text = CString::new(humidity).unwrap();
    lvgl_sys::lv_label_set_text(hum_label, hum_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(hum_label, lv_color_hex(0x808080), 0);
    lvgl_sys::lv_obj_set_style_text_font(hum_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(hum_label, 18, stats_y);

    TEMP_IMG_DSC.header._bitfield_1 = lvgl_sys::lv_img_header_t::new_bitfield_1(
        lvgl_sys::LV_IMG_CF_TRUE_COLOR_ALPHA as u32,
        0, 0,
        TEMP_WIDTH,
        TEMP_HEIGHT,
    );
    TEMP_IMG_DSC.data_size = (TEMP_WIDTH * TEMP_HEIGHT * 3) as u32;
    TEMP_IMG_DSC.data = TEMP_DATA.as_ptr();

    let temp_icon = lvgl_sys::lv_img_create(card);
    lvgl_sys::lv_img_set_src(temp_icon, &raw const TEMP_IMG_DSC as *const _);
    lvgl_sys::lv_obj_set_pos(temp_icon, 48, stats_y);
    lvgl_sys::lv_obj_set_style_img_recolor(temp_icon, lv_color_hex(0xFF8A65), 0);
    lvgl_sys::lv_obj_set_style_img_recolor_opa(temp_icon, 255, 0);

    let temp_label = lvgl_sys::lv_label_create(card);
    let temp_text = CString::new(temp).unwrap();
    lvgl_sys::lv_label_set_text(temp_label, temp_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(temp_label, lv_color_hex(0x808080), 0);
    lvgl_sys::lv_obj_set_style_text_font(temp_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(temp_label, 62, stats_y);

    // === CENTERED SPOOL ===
    let spool_y: i16 = 48;
    let spool_x = (w - 32) / 2;

    // Material label
    let mat_label = lvgl_sys::lv_label_create(card);
    let mat_text = CString::new(material).unwrap();
    lvgl_sys::lv_label_set_text(mat_label, mat_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(mat_label, lv_color_hex(0x808080), 0);
    lvgl_sys::lv_obj_set_style_text_font(mat_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(mat_label, spool_x, spool_y - 12);

    // Spool visual
    create_spool_visual_exact(card, spool_x, spool_y, color, false);

    // Slot ID badge
    let badge_y = spool_y + 44;
    let slot_badge = lvgl_sys::lv_obj_create(card);
    lvgl_sys::lv_obj_set_size(slot_badge, 34, 14);
    lvgl_sys::lv_obj_set_pos(slot_badge, (w - 34) / 2, badge_y);
    lvgl_sys::lv_obj_set_style_bg_color(slot_badge, lv_color_hex(0x404040), 0);
    lvgl_sys::lv_obj_set_style_radius(slot_badge, 7, 0);
    lvgl_sys::lv_obj_set_style_border_width(slot_badge, 0, 0);
    set_style_pad_all(slot_badge, 0);

    let slot_label = lvgl_sys::lv_label_create(slot_badge);
    let slot_text = CString::new(name).unwrap();
    lvgl_sys::lv_label_set_text(slot_label, slot_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(slot_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(slot_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_align(slot_label, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

    // Fill percentage
    let pct_label = lvgl_sys::lv_label_create(card);
    let pct_text = CString::new(fill_pct).unwrap();
    lvgl_sys::lv_label_set_text(pct_label, pct_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(pct_label, lv_color_hex(0x707070), 0);
    lvgl_sys::lv_obj_set_style_text_font(pct_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(pct_label, (w - 20) / 2, badge_y + 16);
}

/// Create external spool card (Ext 1, Ext 2) - EXACT mockup match
/// Uses front-view circle SVG instead of side-view spool
unsafe fn create_ext_unit_card_exact(
    parent: *mut lvgl_sys::lv_obj_t,
    x: i16, y: i16, w: i16, h: i16,
    name: &str, nozzle: &str, material: &str, color: u32,
) {
    // Card with border matching mockup
    let card = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(card, w, h);
    lvgl_sys::lv_obj_set_pos(card, x, y);
    lvgl_sys::lv_obj_set_style_bg_color(card, lv_color_hex(0x2D2D2D), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(card, 255, 0);
    lvgl_sys::lv_obj_set_style_radius(card, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(card, 2, 0);
    lvgl_sys::lv_obj_set_style_border_color(card, lv_color_hex(0x404040), 0);
    set_style_pad_all(card, 6);

    // === HEADER ROW ===
    let name_label = lvgl_sys::lv_label_create(card);
    let name_text = CString::new(name).unwrap();
    lvgl_sys::lv_label_set_text(name_label, name_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(name_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(name_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(name_label, 4, 4);

    // Nozzle badge
    if !nozzle.is_empty() {
        let nozzle_badge = lvgl_sys::lv_obj_create(card);
        lvgl_sys::lv_obj_set_size(nozzle_badge, 14, 12);
        lvgl_sys::lv_obj_set_pos(nozzle_badge, 42, 4);
        lvgl_sys::lv_obj_set_style_bg_color(nozzle_badge, lv_color_hex(COLOR_ACCENT), 0);
        lvgl_sys::lv_obj_set_style_radius(nozzle_badge, 3, 0);
        lvgl_sys::lv_obj_set_style_border_width(nozzle_badge, 0, 0);
        set_style_pad_all(nozzle_badge, 0);

        let nozzle_label = lvgl_sys::lv_label_create(nozzle_badge);
        let nozzle_text = CString::new(nozzle).unwrap();
        lvgl_sys::lv_label_set_text(nozzle_label, nozzle_text.as_ptr());
        lvgl_sys::lv_obj_set_style_text_color(nozzle_label, lv_color_hex(0x1A1A1A), 0);
        lvgl_sys::lv_obj_set_style_text_font(nozzle_label, &lvgl_sys::lv_font_montserrat_12, 0);
        lvgl_sys::lv_obj_align(nozzle_label, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    }

    // === CENTERED CIRCULAR SPOOL (front view) ===
    let spool_size: i16 = 40;
    let spool_x = (w - spool_size) / 2;
    let spool_y: i16 = 36;

    // Material label above
    let mat_label = lvgl_sys::lv_label_create(card);
    let mat_text = CString::new(material).unwrap();
    lvgl_sys::lv_label_set_text(mat_label, mat_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(mat_label, lv_color_hex(0x808080), 0);
    lvgl_sys::lv_obj_set_style_text_font(mat_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(mat_label, spool_x + 4, spool_y - 14);

    // Outer circle (filament color)
    let outer = lvgl_sys::lv_obj_create(card);
    lvgl_sys::lv_obj_set_size(outer, spool_size, spool_size);
    lvgl_sys::lv_obj_set_pos(outer, spool_x, spool_y);
    lvgl_sys::lv_obj_set_style_bg_color(outer, lv_color_hex(color), 0);
    lvgl_sys::lv_obj_set_style_radius(outer, spool_size / 2, 0);
    // Slightly lighter border
    let border_color = lighten_color(color, 20);
    lvgl_sys::lv_obj_set_style_border_color(outer, lv_color_hex(border_color), 0);
    lvgl_sys::lv_obj_set_style_border_width(outer, 2, 0);
    set_style_pad_all(outer, 0);

    // Inner hole (center of spool)
    let inner_size: i16 = 14;
    let inner = lvgl_sys::lv_obj_create(outer);
    lvgl_sys::lv_obj_set_size(inner, inner_size, inner_size);
    lvgl_sys::lv_obj_align(inner, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_color(inner, lv_color_hex(0x2D2D2D), 0);
    lvgl_sys::lv_obj_set_style_radius(inner, inner_size / 2, 0);
    lvgl_sys::lv_obj_set_style_border_color(inner, lv_color_hex(0x505050), 0);
    lvgl_sys::lv_obj_set_style_border_width(inner, 1, 0);
    set_style_pad_all(inner, 0);

    // Slot ID badge below
    let badge_y = spool_y + spool_size + 4;
    let slot_badge = lvgl_sys::lv_obj_create(card);
    lvgl_sys::lv_obj_set_size(slot_badge, 34, 14);
    lvgl_sys::lv_obj_set_pos(slot_badge, (w - 34) / 2, badge_y);
    lvgl_sys::lv_obj_set_style_bg_color(slot_badge, lv_color_hex(0x404040), 0);
    lvgl_sys::lv_obj_set_style_radius(slot_badge, 7, 0);
    lvgl_sys::lv_obj_set_style_border_width(slot_badge, 0, 0);
    set_style_pad_all(slot_badge, 0);

    let slot_label = lvgl_sys::lv_label_create(slot_badge);
    let slot_text = CString::new(name).unwrap();
    lvgl_sys::lv_label_set_text(slot_label, slot_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(slot_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(slot_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_align(slot_label, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

    // Fill percentage (--  for external)
    let pct_label = lvgl_sys::lv_label_create(card);
    let pct_text = CString::new("--").unwrap();
    lvgl_sys::lv_label_set_text(pct_label, pct_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(pct_label, lv_color_hex(0x707070), 0);
    lvgl_sys::lv_obj_set_style_text_font(pct_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(pct_label, (w - 16) / 2, badge_y + 16);
}

/// Lighten a color by adding to each channel
fn lighten_color(color: u32, amount: u8) -> u32 {
    let r = ((color >> 16) & 0xFF) as u8;
    let g = ((color >> 8) & 0xFF) as u8;
    let b = (color & 0xFF) as u8;
    let r = r.saturating_add(amount);
    let g = g.saturating_add(amount);
    let b = b.saturating_add(amount);
    ((r as u32) << 16) | ((g as u32) << 8) | (b as u32)
}

/// Create compact AMS unit (4-slot) - cleaner mockup style
unsafe fn create_ams_unit_compact(
    parent: *mut lvgl_sys::lv_obj_t,
    x: i16, y: i16, w: i16, h: i16,
    name: &str, nozzle: &str, humidity: &str, temp: &str, active: bool,
    slots: &[(&str, u32, &str, &str, bool)],
) {
    // Outer card: mockup --card: #2D2D2D
    let unit = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(unit, w, h);
    lvgl_sys::lv_obj_set_pos(unit, x, y);
    lvgl_sys::lv_obj_clear_flag(unit, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_color(unit, lv_color_hex(0x2D2D2D), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(unit, 255, 0);
    lvgl_sys::lv_obj_set_style_radius(unit, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(unit, 2, 0);
    if active {
        lvgl_sys::lv_obj_set_style_border_color(unit, lv_color_hex(COLOR_ACCENT), 0);
    } else {
        lvgl_sys::lv_obj_set_style_border_color(unit, lv_color_hex(0x404040), 0);
    }
    set_style_pad_all(unit, 6);

    // Header row: name + badge on left, humidity/temp on right
    // Name label (12px - LARGER)
    let name_lbl = lvgl_sys::lv_label_create(unit);
    let name_txt = CString::new(name).unwrap();
    lvgl_sys::lv_label_set_text(name_lbl, name_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(name_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(name_lbl, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(name_lbl, 4, 0);

    // Nozzle badge (L or R) - smaller badge with more gap from name
    if !nozzle.is_empty() {
        let name_width: i16 = (name.len() as i16) * 7 + 12;  // ~7px per char + more gap
        let badge_lbl = lvgl_sys::lv_label_create(unit);
        let badge_txt = CString::new(nozzle).unwrap();
        lvgl_sys::lv_label_set_text(badge_lbl, badge_txt.as_ptr());
        lvgl_sys::lv_obj_set_style_text_color(badge_lbl, lv_color_hex(0x1A1A1A), 0);
        lvgl_sys::lv_obj_set_style_text_font(badge_lbl, &lvgl_sys::lv_font_montserrat_8, 0);
        lvgl_sys::lv_obj_set_style_bg_color(badge_lbl, lv_color_hex(COLOR_ACCENT), 0);
        lvgl_sys::lv_obj_set_style_bg_opa(badge_lbl, 255, 0);
        lvgl_sys::lv_obj_set_style_pad_left(badge_lbl, 2, 0);
        lvgl_sys::lv_obj_set_style_pad_right(badge_lbl, 2, 0);
        lvgl_sys::lv_obj_set_style_pad_top(badge_lbl, 0, 0);
        lvgl_sys::lv_obj_set_style_pad_bottom(badge_lbl, 0, 0);
        lvgl_sys::lv_obj_set_style_radius(badge_lbl, 2, 0);
        lvgl_sys::lv_obj_set_pos(badge_lbl, name_width, 3);
    }

    // Humidity icon + value (12px - LARGER)
    let stats_x = w - 95;  // Moved left, away from card border
    HUMIDITY_IMG_DSC.header._bitfield_1 = lvgl_sys::lv_img_header_t::new_bitfield_1(
        lvgl_sys::LV_IMG_CF_TRUE_COLOR_ALPHA as u32, 0, 0, HUMIDITY_WIDTH, HUMIDITY_HEIGHT,
    );
    HUMIDITY_IMG_DSC.data_size = (HUMIDITY_WIDTH * HUMIDITY_HEIGHT * 3) as u32;
    HUMIDITY_IMG_DSC.data = HUMIDITY_DATA.as_ptr();

    let hum_icon = lvgl_sys::lv_img_create(unit);
    lvgl_sys::lv_img_set_src(hum_icon, &raw const HUMIDITY_IMG_DSC as *const _);
    lvgl_sys::lv_obj_set_pos(hum_icon, stats_x, 2);
    lvgl_sys::lv_obj_set_style_img_recolor(hum_icon, lv_color_hex(0x4FC3F7), 0);
    lvgl_sys::lv_obj_set_style_img_recolor_opa(hum_icon, 255, 0);

    let hum_lbl = lvgl_sys::lv_label_create(unit);
    let hum_txt = CString::new(humidity).unwrap();
    lvgl_sys::lv_label_set_text(hum_lbl, hum_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(hum_lbl, lv_color_hex(0xFFFFFF), 0);
    lvgl_sys::lv_obj_set_style_text_font(hum_lbl, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(hum_lbl, stats_x + 12, 0);

    // Temperature icon + value (12px - LARGER)
    TEMP_IMG_DSC.header._bitfield_1 = lvgl_sys::lv_img_header_t::new_bitfield_1(
        lvgl_sys::LV_IMG_CF_TRUE_COLOR_ALPHA as u32, 0, 0, TEMP_WIDTH, TEMP_HEIGHT,
    );
    TEMP_IMG_DSC.data_size = (TEMP_WIDTH * TEMP_HEIGHT * 3) as u32;
    TEMP_IMG_DSC.data = TEMP_DATA.as_ptr();

    let temp_icon = lvgl_sys::lv_img_create(unit);
    lvgl_sys::lv_img_set_src(temp_icon, &raw const TEMP_IMG_DSC as *const _);
    lvgl_sys::lv_obj_set_pos(temp_icon, stats_x + 38, 2);
    lvgl_sys::lv_obj_set_style_img_recolor(temp_icon, lv_color_hex(0xFFB74D), 0);
    lvgl_sys::lv_obj_set_style_img_recolor_opa(temp_icon, 255, 0);

    let temp_lbl = lvgl_sys::lv_label_create(unit);
    let temp_txt = CString::new(temp).unwrap();
    lvgl_sys::lv_label_set_text(temp_lbl, temp_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(temp_lbl, lv_color_hex(0xFFFFFF), 0);
    lvgl_sys::lv_obj_set_style_text_font(temp_lbl, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(temp_lbl, stats_x + 50, 0);

    // Inner housing container with gradient (mockup .ams-housing)
    // Header height ~18px, housing fills remaining space
    let housing_y: i16 = 18;
    let housing_h: i16 = h - 12 - housing_y;  // 12 = 2*6px padding
    let housing_w: i16 = w - 12;  // Account for padding

    let housing = lvgl_sys::lv_obj_create(unit);
    lvgl_sys::lv_obj_set_size(housing, housing_w, housing_h);
    lvgl_sys::lv_obj_set_pos(housing, 0, housing_y);
    lvgl_sys::lv_obj_clear_flag(housing, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    // Mockup .ams-housing: darker top (#2A2A2A) -> #1A1A1A gradient
    lvgl_sys::lv_obj_set_style_bg_color(housing, lv_color_hex(0x2A2A2A), 0);
    lvgl_sys::lv_obj_set_style_bg_grad_color(housing, lv_color_hex(0x1A1A1A), 0);
    lvgl_sys::lv_obj_set_style_bg_grad_dir(housing, lvgl_sys::LV_GRAD_DIR_VER as u8, 0);
    lvgl_sys::lv_obj_set_style_bg_opa(housing, 255, 0);
    lvgl_sys::lv_obj_set_style_radius(housing, 5, 0);
    lvgl_sys::lv_obj_set_style_border_width(housing, 0, 0);
    set_style_pad_all(housing, 4);

    // Spools row - LARGER 40x52 spools with tight spacing
    let num_slots = slots.len().min(4) as i16;
    let spool_w: i16 = 40;     // Larger spool width
    let _spool_h: i16 = 52;    // Larger spool height
    let spool_step: i16 = 42;  // Spacing between spools (slightly wider for larger spools)
    let spool_row_w = spool_w + (num_slots - 1) * spool_step;
    let start_x: i16 = (housing_w - spool_row_w) / 2 - 4;

    // Vertical layout within housing
    let mat_y: i16 = 8;        // Material labels (relative to housing)
    let spool_y: i16 = 26;     // Spools
    let badge_y: i16 = 82;     // Slot badges
    let pct_y: i16 = 98;       // Percentages

    for (i, (material, color, slot_id, fill_pct, slot_active)) in slots.iter().enumerate() {
        let sx = start_x + (i as i16) * spool_step;
        // Visual spool center is offset by -4 due to zoom expansion
        let visual_spool_x = sx - 4;

        // Material label (10px) - set width and use text alignment
        let mat_lbl = lvgl_sys::lv_label_create(housing);
        let mat_txt = if *color != 0 {
            CString::new(*material).unwrap()
        } else {
            CString::new("--").unwrap()
        };
        lvgl_sys::lv_label_set_text(mat_lbl, mat_txt.as_ptr());
        lvgl_sys::lv_obj_set_width(mat_lbl, spool_w);
        lvgl_sys::lv_obj_set_style_text_color(mat_lbl, lv_color_hex(COLOR_WHITE), 0);
        lvgl_sys::lv_obj_set_style_text_font(mat_lbl, &lvgl_sys::lv_font_montserrat_10, 0);
        lvgl_sys::lv_obj_set_style_text_align(mat_lbl, lvgl_sys::LV_TEXT_ALIGN_CENTER as u8, 0);
        lvgl_sys::lv_obj_set_pos(mat_lbl, visual_spool_x, mat_y);

        // Spool visual (larger 40x52)
        create_spool_large(housing, sx, spool_y, *color);

        // Slot badge (10px) - centered under visual spool
        let slot_badge = lvgl_sys::lv_obj_create(housing);
        lvgl_sys::lv_obj_set_size(slot_badge, 28, 14);
        lvgl_sys::lv_obj_set_pos(slot_badge, visual_spool_x + (spool_w - 28) / 2, badge_y);
        if *slot_active {
            lvgl_sys::lv_obj_set_style_bg_color(slot_badge, lv_color_hex(COLOR_ACCENT), 0);
        } else {
            lvgl_sys::lv_obj_set_style_bg_color(slot_badge, lv_color_hex(0x000000), 0);
            lvgl_sys::lv_obj_set_style_bg_opa(slot_badge, 153, 0);
        }
        lvgl_sys::lv_obj_set_style_radius(slot_badge, 7, 0);
        lvgl_sys::lv_obj_set_style_border_width(slot_badge, 0, 0);
        set_style_pad_all(slot_badge, 0);

        let slot_lbl = lvgl_sys::lv_label_create(slot_badge);
        let slot_txt = CString::new(*slot_id).unwrap();
        lvgl_sys::lv_label_set_text(slot_lbl, slot_txt.as_ptr());
        let txt_color = if *slot_active { 0x1A1A1A } else { COLOR_WHITE };
        lvgl_sys::lv_obj_set_style_text_color(slot_lbl, lv_color_hex(txt_color), 0);
        lvgl_sys::lv_obj_set_style_text_font(slot_lbl, &lvgl_sys::lv_font_montserrat_10, 0);
        lvgl_sys::lv_obj_align(slot_lbl, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

        // Fill percentage (10px) - set width and use text alignment
        let pct_lbl = lvgl_sys::lv_label_create(housing);
        let pct_str = if *color != 0 && !fill_pct.is_empty() { *fill_pct } else { "--" };
        let pct_txt = CString::new(pct_str).unwrap();
        lvgl_sys::lv_label_set_text(pct_lbl, pct_txt.as_ptr());
        lvgl_sys::lv_obj_set_width(pct_lbl, spool_w);
        lvgl_sys::lv_obj_set_style_text_color(pct_lbl, lv_color_hex(COLOR_WHITE), 0);
        lvgl_sys::lv_obj_set_style_text_font(pct_lbl, &lvgl_sys::lv_font_montserrat_10, 0);
        lvgl_sys::lv_obj_set_style_text_align(pct_lbl, lvgl_sys::LV_TEXT_ALIGN_CENTER as u8, 0);
        lvgl_sys::lv_obj_set_pos(pct_lbl, visual_spool_x, pct_y);
    }
}

/// Create compact spool visual - clean spool with color tint overlay (like mockup)
unsafe fn create_spool_compact(parent: *mut lvgl_sys::lv_obj_t, x: i16, y: i16, color: u32) {
    // Inner area dimensions (from mockup CSS: top 12%, left 27%, right 22%, bottom 12%)
    // For 32x42: top=5, left=9, width=16, height=32
    let inner_top: i16 = 5;
    let inner_left: i16 = 9;
    let inner_w: i16 = 16;
    let inner_h: i16 = 32;

    // 1. First draw the clean gray spool image
    SPOOL_CLEAN_IMG_DSC.header._bitfield_1 = lvgl_sys::lv_img_header_t::new_bitfield_1(
        lvgl_sys::LV_IMG_CF_TRUE_COLOR_ALPHA as u32,
        0, 0,
        SPOOL_WIDTH,
        SPOOL_HEIGHT,
    );
    SPOOL_CLEAN_IMG_DSC.data_size = (SPOOL_WIDTH * SPOOL_HEIGHT * 3) as u32;
    SPOOL_CLEAN_IMG_DSC.data = SPOOL_CLEAN_DATA.as_ptr();

    let spool_img = lvgl_sys::lv_img_create(parent);
    lvgl_sys::lv_img_set_src(spool_img, &raw const SPOOL_CLEAN_IMG_DSC as *const _);
    lvgl_sys::lv_obj_set_pos(spool_img, x, y);

    // For empty slots, reduce opacity (like mockup: opacity 0.3)
    if color == 0 {
        lvgl_sys::lv_obj_set_style_img_opa(spool_img, 76, 0);  // 0.3 * 255 = ~76
    }

    // 2. Then overlay a semi-transparent color tint ON TOP (like mockup)
    if color != 0 {
        let tint = lvgl_sys::lv_obj_create(parent);
        lvgl_sys::lv_obj_set_size(tint, inner_w, inner_h);
        lvgl_sys::lv_obj_set_pos(tint, x + inner_left, y + inner_top);
        lvgl_sys::lv_obj_set_style_bg_color(tint, lv_color_hex(color), 0);
        lvgl_sys::lv_obj_set_style_bg_opa(tint, 217, 0);  // 0.85 * 255 = ~217
        lvgl_sys::lv_obj_set_style_radius(tint, 2, 0);
        lvgl_sys::lv_obj_set_style_border_width(tint, 0, 0);
        set_style_pad_all(tint, 0);
    }
}

/// Create LARGER spool visual (40x52) - scaled up version
unsafe fn create_spool_large(parent: *mut lvgl_sys::lv_obj_t, x: i16, y: i16, color: u32) {
    // Zoom expands image from center, so visual top-left shifts
    // Original 32x42 -> zoomed 40x52 (1.25x)
    // Visual offset: -4 horizontal, -5 vertical from set_pos
    let zoom_offset_x: i16 = -4;
    let zoom_offset_y: i16 = -5;

    // Inner tint area relative to visual (zoomed) top-left
    // Original inner: left=9, top=5, w=16, h=32 for 32x42
    // Scaled inner: left=11, top=6, w=20, h=40 for 40x52
    let inner_left: i16 = 11;
    let inner_top: i16 = 6;
    let inner_w: i16 = 20;
    let inner_h: i16 = 40;

    // Draw scaled spool using the 32x42 image with zoom
    SPOOL_CLEAN_IMG_DSC.header._bitfield_1 = lvgl_sys::lv_img_header_t::new_bitfield_1(
        lvgl_sys::LV_IMG_CF_TRUE_COLOR_ALPHA as u32,
        0, 0,
        SPOOL_WIDTH,
        SPOOL_HEIGHT,
    );
    SPOOL_CLEAN_IMG_DSC.data_size = (SPOOL_WIDTH * SPOOL_HEIGHT * 3) as u32;
    SPOOL_CLEAN_IMG_DSC.data = SPOOL_CLEAN_DATA.as_ptr();

    let spool_img = lvgl_sys::lv_img_create(parent);
    lvgl_sys::lv_img_set_src(spool_img, &raw const SPOOL_CLEAN_IMG_DSC as *const _);
    lvgl_sys::lv_obj_set_pos(spool_img, x, y);
    // Scale to 125% (256 = 100%, 320 = 125%)
    lvgl_sys::lv_img_set_zoom(spool_img, 320);

    // For empty slots, reduce opacity and add striped background with "+" indicator
    if color == 0 {
        lvgl_sys::lv_obj_set_style_img_opa(spool_img, 50, 0);  // More faded

        // Inner area position
        let inner_x = x + zoom_offset_x + inner_left;
        let inner_y = y + zoom_offset_y + inner_top;

        // Add striped background in the inner area
        let empty_bg = lvgl_sys::lv_obj_create(parent);
        lvgl_sys::lv_obj_set_size(empty_bg, inner_w, inner_h);
        lvgl_sys::lv_obj_set_pos(empty_bg, inner_x, inner_y);
        lvgl_sys::lv_obj_set_style_bg_color(empty_bg, lv_color_hex(0x1A1A1A), 0);
        lvgl_sys::lv_obj_set_style_bg_opa(empty_bg, 255, 0);
        lvgl_sys::lv_obj_set_style_radius(empty_bg, 2, 0);
        lvgl_sys::lv_obj_set_style_border_width(empty_bg, 1, 0);
        lvgl_sys::lv_obj_set_style_border_color(empty_bg, lv_color_hex(0x3A3A3A), 0);
        set_style_pad_all(empty_bg, 0);
        lvgl_sys::lv_obj_clear_flag(empty_bg, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
        lvgl_sys::lv_obj_set_style_clip_corner(empty_bg, true, 0);

        // Add diagonal stripes using line objects
        let stripe_color = 0x383838;  // More visible stripes
        let stripe_spacing: i16 = 5;
        let num_stripes = (inner_w + inner_h) / stripe_spacing;

        for i in 0..num_stripes {
            let offset = i * stripe_spacing - inner_h;
            let line = lvgl_sys::lv_line_create(empty_bg);

            // Calculate line points (diagonal from top-left to bottom-right direction)
            let x1: i16 = offset.max(0);
            let y1: i16 = if offset < 0 { -offset } else { 0 };
            let x2: i16 = (offset + inner_h).min(inner_w);
            let y2: i16 = if offset + inner_h > inner_w { inner_h - (offset + inner_h - inner_w) } else { inner_h };

            // Store points in static array (LVGL needs persistent memory)
            static mut LINE_POINTS: [[lvgl_sys::lv_point_t; 2]; 32] = [[lvgl_sys::lv_point_t { x: 0, y: 0 }; 2]; 32];
            let idx = i as usize % 32;
            LINE_POINTS[idx][0] = lvgl_sys::lv_point_t { x: x1, y: y1 };
            LINE_POINTS[idx][1] = lvgl_sys::lv_point_t { x: x2, y: y2 };

            lvgl_sys::lv_line_set_points(line, LINE_POINTS[idx].as_ptr(), 2);
            lvgl_sys::lv_obj_set_style_line_color(line, lv_color_hex(stripe_color), 0);
            lvgl_sys::lv_obj_set_style_line_width(line, 2, 0);
        }

        // Add "+" indicator centered (on top of stripes)
        let plus_lbl = lvgl_sys::lv_label_create(empty_bg);
        let plus_txt = CString::new("+").unwrap();
        lvgl_sys::lv_label_set_text(plus_lbl, plus_txt.as_ptr());
        lvgl_sys::lv_obj_set_style_text_color(plus_lbl, lv_color_hex(0x505050), 0);
        lvgl_sys::lv_obj_set_style_text_font(plus_lbl, &lvgl_sys::lv_font_montserrat_16, 0);
        lvgl_sys::lv_obj_align(plus_lbl, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 1);
    }

    // Color tint overlay - position relative to zoomed visual position
    // Visual top-left = (x + zoom_offset_x, y + zoom_offset_y)
    // Tint position = visual top-left + inner offset
    if color != 0 {
        let tint = lvgl_sys::lv_obj_create(parent);
        lvgl_sys::lv_obj_set_size(tint, inner_w, inner_h);
        let tint_x = x + zoom_offset_x + inner_left;
        let tint_y = y + zoom_offset_y + inner_top;
        lvgl_sys::lv_obj_set_pos(tint, tint_x, tint_y);
        lvgl_sys::lv_obj_set_style_bg_color(tint, lv_color_hex(color), 0);
        lvgl_sys::lv_obj_set_style_bg_opa(tint, 217, 0);
        lvgl_sys::lv_obj_set_style_radius(tint, 3, 0);
        lvgl_sys::lv_obj_set_style_border_width(tint, 0, 0);
        set_style_pad_all(tint, 0);
    }
}

/// Create compact single-slot unit (HT-A, HT-B)
unsafe fn create_single_unit_compact(
    parent: *mut lvgl_sys::lv_obj_t,
    x: i16, y: i16, w: i16, h: i16,
    name: &str, nozzle: &str, humidity: &str, temp: &str,
    material: &str, color: u32, fill_pct: &str,
) {
    // Outer card: #2D2D2D
    let unit = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(unit, w, h);
    lvgl_sys::lv_obj_set_pos(unit, x, y);
    lvgl_sys::lv_obj_clear_flag(unit, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_color(unit, lv_color_hex(0x2D2D2D), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(unit, 255, 0);
    lvgl_sys::lv_obj_set_style_radius(unit, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(unit, 2, 0);
    lvgl_sys::lv_obj_set_style_border_color(unit, lv_color_hex(0x404040), 0);
    set_style_pad_all(unit, 6);

    // Header row: name + badge on left (12px font like AMS cards)
    let name_lbl = lvgl_sys::lv_label_create(unit);
    let name_txt = CString::new(name).unwrap();
    lvgl_sys::lv_label_set_text(name_lbl, name_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(name_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(name_lbl, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(name_lbl, 4, 0);

    // Nozzle badge after name
    if !nozzle.is_empty() {
        let name_width: i16 = (name.len() as i16) * 7 + 12;
        let badge_lbl = lvgl_sys::lv_label_create(unit);
        let badge_txt = CString::new(nozzle).unwrap();
        lvgl_sys::lv_label_set_text(badge_lbl, badge_txt.as_ptr());
        lvgl_sys::lv_obj_set_style_text_color(badge_lbl, lv_color_hex(0x1A1A1A), 0);
        lvgl_sys::lv_obj_set_style_text_font(badge_lbl, &lvgl_sys::lv_font_montserrat_8, 0);
        lvgl_sys::lv_obj_set_style_bg_color(badge_lbl, lv_color_hex(COLOR_ACCENT), 0);
        lvgl_sys::lv_obj_set_style_bg_opa(badge_lbl, 255, 0);
        lvgl_sys::lv_obj_set_style_pad_left(badge_lbl, 2, 0);
        lvgl_sys::lv_obj_set_style_pad_right(badge_lbl, 2, 0);
        lvgl_sys::lv_obj_set_style_pad_top(badge_lbl, 0, 0);
        lvgl_sys::lv_obj_set_style_pad_bottom(badge_lbl, 0, 0);
        lvgl_sys::lv_obj_set_style_radius(badge_lbl, 2, 0);
        lvgl_sys::lv_obj_set_pos(badge_lbl, name_width, 3);
    }

    // Inner housing with gradient
    let housing_y: i16 = 18;
    let housing_h: i16 = h - 12 - housing_y;
    let housing_w: i16 = w - 12;

    let housing = lvgl_sys::lv_obj_create(unit);
    lvgl_sys::lv_obj_set_size(housing, housing_w, housing_h);
    lvgl_sys::lv_obj_set_pos(housing, 0, housing_y);
    lvgl_sys::lv_obj_clear_flag(housing, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_color(housing, lv_color_hex(0x2A2A2A), 0);
    lvgl_sys::lv_obj_set_style_bg_grad_color(housing, lv_color_hex(0x1A1A1A), 0);
    lvgl_sys::lv_obj_set_style_bg_grad_dir(housing, lvgl_sys::LV_GRAD_DIR_VER as u8, 0);
    lvgl_sys::lv_obj_set_style_bg_opa(housing, 255, 0);
    lvgl_sys::lv_obj_set_style_radius(housing, 5, 0);
    lvgl_sys::lv_obj_set_style_border_width(housing, 0, 0);
    set_style_pad_all(housing, 4);

    // Humidity/temp row inside housing
    let stats_y: i16 = 2;
    HUMIDITY_IMG_DSC.header._bitfield_1 = lvgl_sys::lv_img_header_t::new_bitfield_1(
        lvgl_sys::LV_IMG_CF_TRUE_COLOR_ALPHA as u32, 0, 0, HUMIDITY_WIDTH, HUMIDITY_HEIGHT,
    );
    HUMIDITY_IMG_DSC.data_size = (HUMIDITY_WIDTH * HUMIDITY_HEIGHT * 3) as u32;
    HUMIDITY_IMG_DSC.data = HUMIDITY_DATA.as_ptr();

    let hum_icon = lvgl_sys::lv_img_create(housing);
    lvgl_sys::lv_img_set_src(hum_icon, &raw const HUMIDITY_IMG_DSC as *const _);
    lvgl_sys::lv_obj_set_pos(hum_icon, 0, stats_y);
    lvgl_sys::lv_obj_set_style_img_recolor(hum_icon, lv_color_hex(0x4FC3F7), 0);
    lvgl_sys::lv_obj_set_style_img_recolor_opa(hum_icon, 255, 0);

    let hum_lbl = lvgl_sys::lv_label_create(housing);
    let hum_txt = CString::new(humidity).unwrap();
    lvgl_sys::lv_label_set_text(hum_lbl, hum_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(hum_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(hum_lbl, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(hum_lbl, 11, stats_y - 2);

    TEMP_IMG_DSC.header._bitfield_1 = lvgl_sys::lv_img_header_t::new_bitfield_1(
        lvgl_sys::LV_IMG_CF_TRUE_COLOR_ALPHA as u32, 0, 0, TEMP_WIDTH, TEMP_HEIGHT,
    );
    TEMP_IMG_DSC.data_size = (TEMP_WIDTH * TEMP_HEIGHT * 3) as u32;
    TEMP_IMG_DSC.data = TEMP_DATA.as_ptr();

    let temp_icon = lvgl_sys::lv_img_create(housing);
    lvgl_sys::lv_img_set_src(temp_icon, &raw const TEMP_IMG_DSC as *const _);
    lvgl_sys::lv_obj_set_pos(temp_icon, 40, stats_y);
    lvgl_sys::lv_obj_set_style_img_recolor(temp_icon, lv_color_hex(0xFFB74D), 0);
    lvgl_sys::lv_obj_set_style_img_recolor_opa(temp_icon, 255, 0);

    let temp_lbl = lvgl_sys::lv_label_create(housing);
    let temp_txt = CString::new(temp).unwrap();
    lvgl_sys::lv_label_set_text(temp_lbl, temp_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(temp_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(temp_lbl, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(temp_lbl, 52, stats_y - 2);

    // Spool content inside housing
    let spool_x = (housing_w - 40) / 2;
    let mat_y: i16 = 24;
    let spool_y: i16 = 42;
    let badge_y: i16 = 98;
    let pct_y: i16 = 114;

    // Material label
    let mat_lbl = lvgl_sys::lv_label_create(housing);
    let mat_txt = CString::new(material).unwrap();
    lvgl_sys::lv_label_set_text(mat_lbl, mat_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(mat_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(mat_lbl, &lvgl_sys::lv_font_montserrat_10, 0);
    lvgl_sys::lv_obj_set_pos(mat_lbl, spool_x + 4, mat_y);

    create_spool_large(housing, spool_x, spool_y, color);

    // Slot badge
    let slot_badge = lvgl_sys::lv_obj_create(housing);
    lvgl_sys::lv_obj_set_size(slot_badge, 36, 14);
    lvgl_sys::lv_obj_set_pos(slot_badge, (housing_w - 36) / 2, badge_y);
    lvgl_sys::lv_obj_set_style_bg_color(slot_badge, lv_color_hex(0x000000), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(slot_badge, 153, 0);
    lvgl_sys::lv_obj_set_style_radius(slot_badge, 7, 0);
    lvgl_sys::lv_obj_set_style_border_width(slot_badge, 0, 0);
    set_style_pad_all(slot_badge, 0);

    let slot_lbl = lvgl_sys::lv_label_create(slot_badge);
    let slot_txt = CString::new(name).unwrap();
    lvgl_sys::lv_label_set_text(slot_lbl, slot_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(slot_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(slot_lbl, &lvgl_sys::lv_font_montserrat_10, 0);
    lvgl_sys::lv_obj_align(slot_lbl, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

    // Percentage
    let pct_lbl = lvgl_sys::lv_label_create(housing);
    let pct_txt = CString::new(fill_pct).unwrap();
    lvgl_sys::lv_label_set_text(pct_lbl, pct_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(pct_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(pct_lbl, &lvgl_sys::lv_font_montserrat_10, 0);
    lvgl_sys::lv_obj_set_pos(pct_lbl, (housing_w - 20) / 2, pct_y);
}

/// Create compact external spool unit
unsafe fn create_ext_unit_compact(
    parent: *mut lvgl_sys::lv_obj_t,
    x: i16, y: i16, w: i16, h: i16,
    name: &str, nozzle: &str, material: &str, color: u32,
) {
    // Outer card: #2D2D2D
    let unit = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(unit, w, h);
    lvgl_sys::lv_obj_set_pos(unit, x, y);
    lvgl_sys::lv_obj_clear_flag(unit, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_color(unit, lv_color_hex(0x2D2D2D), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(unit, 255, 0);
    lvgl_sys::lv_obj_set_style_radius(unit, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(unit, 2, 0);
    lvgl_sys::lv_obj_set_style_border_color(unit, lv_color_hex(0x404040), 0);
    set_style_pad_all(unit, 6);

    // Header: name + nozzle badge
    let name_lbl = lvgl_sys::lv_label_create(unit);
    let name_txt = CString::new(name).unwrap();
    lvgl_sys::lv_label_set_text(name_lbl, name_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(name_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(name_lbl, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(name_lbl, 4, 0);

    if !nozzle.is_empty() {
        let name_width: i16 = (name.len() as i16) * 6 + 4;
        let badge_gap: i16 = 4;  // Space between name and badge
        let badge_lbl = lvgl_sys::lv_label_create(unit);
        let badge_txt = CString::new(nozzle).unwrap();
        lvgl_sys::lv_label_set_text(badge_lbl, badge_txt.as_ptr());
        lvgl_sys::lv_obj_set_style_text_color(badge_lbl, lv_color_hex(0x1A1A1A), 0);
        lvgl_sys::lv_obj_set_style_text_font(badge_lbl, &lvgl_sys::lv_font_montserrat_8, 0);
        lvgl_sys::lv_obj_set_style_bg_color(badge_lbl, lv_color_hex(COLOR_ACCENT), 0);
        lvgl_sys::lv_obj_set_style_bg_opa(badge_lbl, 255, 0);
        lvgl_sys::lv_obj_set_style_pad_left(badge_lbl, 2, 0);
        lvgl_sys::lv_obj_set_style_pad_right(badge_lbl, 2, 0);
        lvgl_sys::lv_obj_set_style_pad_top(badge_lbl, 0, 0);
        lvgl_sys::lv_obj_set_style_pad_bottom(badge_lbl, 0, 0);
        lvgl_sys::lv_obj_set_style_radius(badge_lbl, 2, 0);
        lvgl_sys::lv_obj_set_pos(badge_lbl, name_width + badge_gap, 3);
    }

    // Inner housing with gradient
    let housing_y: i16 = 18;
    let housing_h: i16 = h - 12 - housing_y;
    let housing_w: i16 = w - 12;

    let housing = lvgl_sys::lv_obj_create(unit);
    lvgl_sys::lv_obj_set_size(housing, housing_w, housing_h);
    lvgl_sys::lv_obj_set_pos(housing, 0, housing_y);
    lvgl_sys::lv_obj_clear_flag(housing, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_color(housing, lv_color_hex(0x2A2A2A), 0);
    lvgl_sys::lv_obj_set_style_bg_grad_color(housing, lv_color_hex(0x1A1A1A), 0);
    lvgl_sys::lv_obj_set_style_bg_grad_dir(housing, lvgl_sys::LV_GRAD_DIR_VER as u8, 0);
    lvgl_sys::lv_obj_set_style_bg_opa(housing, 255, 0);
    lvgl_sys::lv_obj_set_style_radius(housing, 5, 0);
    lvgl_sys::lv_obj_set_style_border_width(housing, 0, 0);
    set_style_pad_all(housing, 4);

    // Content inside housing
    let mat_y: i16 = 16;
    let spool_size: i16 = 70;
    let spool_y: i16 = 34;
    let badge_y: i16 = 110;

    // Material label
    let mat_lbl = lvgl_sys::lv_label_create(housing);
    let mat_txt = CString::new(material).unwrap();
    lvgl_sys::lv_label_set_text(mat_lbl, mat_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(mat_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(mat_lbl, &lvgl_sys::lv_font_montserrat_10, 0);
    lvgl_sys::lv_obj_align(mat_lbl, lvgl_sys::LV_ALIGN_TOP_MID as u8, 0, mat_y);

    // Circular spool
    let outer = lvgl_sys::lv_obj_create(housing);
    lvgl_sys::lv_obj_set_size(outer, spool_size, spool_size);
    lvgl_sys::lv_obj_align(outer, lvgl_sys::LV_ALIGN_TOP_MID as u8, 0, spool_y);
    lvgl_sys::lv_obj_clear_flag(outer, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_color(outer, lv_color_hex(color), 0);
    lvgl_sys::lv_obj_set_style_radius(outer, spool_size / 2, 0);
    lvgl_sys::lv_obj_set_style_border_color(outer, lv_color_hex(lighten_color(color, 20)), 0);
    lvgl_sys::lv_obj_set_style_border_width(outer, 2, 0);
    set_style_pad_all(outer, 0);

    let inner_size: i16 = 20;
    let inner = lvgl_sys::lv_obj_create(outer);
    lvgl_sys::lv_obj_set_size(inner, inner_size, inner_size);
    lvgl_sys::lv_obj_align(inner, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_clear_flag(inner, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_color(inner, lv_color_hex(0x2D2D2D), 0);
    lvgl_sys::lv_obj_set_style_radius(inner, inner_size / 2, 0);
    lvgl_sys::lv_obj_set_style_border_color(inner, lv_color_hex(0x505050), 0);
    lvgl_sys::lv_obj_set_style_border_width(inner, 1, 0);
    set_style_pad_all(inner, 0);

    // Slot badge
    let badge_w: i16 = 32;
    let slot_badge = lvgl_sys::lv_obj_create(housing);
    lvgl_sys::lv_obj_set_size(slot_badge, badge_w, 16);
    lvgl_sys::lv_obj_align(slot_badge, lvgl_sys::LV_ALIGN_TOP_MID as u8, 0, badge_y);
    lvgl_sys::lv_obj_set_style_bg_color(slot_badge, lv_color_hex(0x000000), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(slot_badge, 153, 0);
    lvgl_sys::lv_obj_set_style_radius(slot_badge, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(slot_badge, 0, 0);
    set_style_pad_all(slot_badge, 0);

    let slot_lbl = lvgl_sys::lv_label_create(slot_badge);
    let slot_txt = CString::new(name).unwrap();
    lvgl_sys::lv_label_set_text(slot_lbl, slot_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(slot_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(slot_lbl, &lvgl_sys::lv_font_montserrat_10, 0);
    lvgl_sys::lv_obj_align(slot_lbl, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
}

/// Create spool icon v2 - 3D spool with two layers: colored fill + gray frame
unsafe fn create_spool_icon_v2(parent: *mut lvgl_sys::lv_obj_t, x: i16, y: i16, _size: i16, color: u32, active: bool) {
    // Initialize spool fill image descriptor (white, gets recolored)
    SPOOL_FILL_IMG_DSC.header._bitfield_1 = lvgl_sys::lv_img_header_t::new_bitfield_1(
        lvgl_sys::LV_IMG_CF_TRUE_COLOR_ALPHA as u32,
        0, 0,
        SPOOL_WIDTH,
        SPOOL_HEIGHT,
    );
    SPOOL_FILL_IMG_DSC.data_size = (SPOOL_WIDTH * SPOOL_HEIGHT * 3) as u32;
    SPOOL_FILL_IMG_DSC.data = SPOOL_FILL_DATA.as_ptr();

    // Initialize spool frame image descriptor (gray, not recolored)
    SPOOL_FRAME_IMG_DSC.header._bitfield_1 = lvgl_sys::lv_img_header_t::new_bitfield_1(
        lvgl_sys::LV_IMG_CF_TRUE_COLOR_ALPHA as u32,
        0, 0,
        SPOOL_WIDTH,
        SPOOL_HEIGHT,
    );
    SPOOL_FRAME_IMG_DSC.data_size = (SPOOL_WIDTH * SPOOL_HEIGHT * 3) as u32;
    SPOOL_FRAME_IMG_DSC.data = SPOOL_FRAME_DATA.as_ptr();

    // Container for spool with optional glow
    let container = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(container, SPOOL_WIDTH as i16, SPOOL_HEIGHT as i16);
    lvgl_sys::lv_obj_set_pos(container, x, y);
    lvgl_sys::lv_obj_set_style_bg_opa(container, 0, 0);
    lvgl_sys::lv_obj_set_style_border_width(container, 0, 0);
    set_style_pad_all(container, 0);

    // Active glow effect
    if active {
        lvgl_sys::lv_obj_set_style_shadow_color(container, lv_color_hex(COLOR_ACCENT), 0);
        lvgl_sys::lv_obj_set_style_shadow_width(container, 15, 0);
        lvgl_sys::lv_obj_set_style_shadow_spread(container, 3, 0);
        lvgl_sys::lv_obj_set_style_shadow_opa(container, 180, 0);
    }

    // Layer 1: Colored fill (filament body) - fully recolored
    let fill_img = lvgl_sys::lv_img_create(container);
    lvgl_sys::lv_img_set_src(fill_img, &raw const SPOOL_FILL_IMG_DSC as *const _);
    lvgl_sys::lv_obj_align(fill_img, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_img_recolor(fill_img, lv_color_hex(color), 0);
    lvgl_sys::lv_obj_set_style_img_recolor_opa(fill_img, 255, 0);  // Full recolor for fill

    // Layer 2: Gray frame (flanges) - not recolored, drawn on top
    let frame_img = lvgl_sys::lv_img_create(container);
    lvgl_sys::lv_img_set_src(frame_img, &raw const SPOOL_FRAME_IMG_DSC as *const _);
    lvgl_sys::lv_obj_align(frame_img, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
}

/// Create AMS unit card with 4 spool slots in horizontal row
unsafe fn create_ams_unit_card(
    parent: *mut lvgl_sys::lv_obj_t,
    x: i16, y: i16, w: i16, h: i16,
    name: &str, humidity: &str, temp: &str, active: bool,
    slots: &[(&str, u32, &str, &str, bool); 4],
) {
    let card = if active {
        create_card_glow(parent, x, y, w, h)
    } else {
        create_card(parent, x, y, w, h)
    };

    // Header row: name badge + green dot + humidity/temp
    // Name badge (darker background)
    let badge = lvgl_sys::lv_obj_create(card);
    lvgl_sys::lv_obj_set_size(badge, 50, 16);
    lvgl_sys::lv_obj_set_pos(badge, 6, 4);
    lvgl_sys::lv_obj_set_style_bg_color(badge, lv_color_hex(0x404040), 0);
    lvgl_sys::lv_obj_set_style_radius(badge, 4, 0);
    lvgl_sys::lv_obj_set_style_border_width(badge, 0, 0);
    set_style_pad_all(badge, 0);

    let name_label = lvgl_sys::lv_label_create(badge);
    let name_text = CString::new(name).unwrap();
    lvgl_sys::lv_label_set_text(name_label, name_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(name_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(name_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_align(name_label, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

    // Green dot for active unit
    if active {
        let dot = lvgl_sys::lv_obj_create(card);
        lvgl_sys::lv_obj_set_size(dot, 6, 6);
        lvgl_sys::lv_obj_set_pos(dot, 60, 9);
        lvgl_sys::lv_obj_set_style_bg_color(dot, lv_color_hex(COLOR_ACCENT), 0);
        lvgl_sys::lv_obj_set_style_radius(dot, 3, 0);
        lvgl_sys::lv_obj_set_style_border_width(dot, 0, 0);
    }

    // Humidity + temp on right side of header
    let stats_label = lvgl_sys::lv_label_create(card);
    let stats_str = format!("{} {}", humidity, temp);
    let stats_text = CString::new(stats_str).unwrap();
    lvgl_sys::lv_label_set_text(stats_label, stats_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(stats_label, lv_color_hex(COLOR_GRAY), 0);
    lvgl_sys::lv_obj_set_style_text_font(stats_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(stats_label, w - 70, 6);

    // Horizontal layout for 4 slots
    let slot_w: i16 = 40;
    let slot_gap: i16 = 4;
    let slot_start_x: i16 = 8;
    let row_y_mat: i16 = 22;      // Material labels row
    let row_y_spool: i16 = 34;    // Spool icons row
    let row_y_label: i16 = 58;    // Slot labels row
    let row_y_pct: i16 = 72;      // Percentage row

    for (i, (material, color, label, percent, slot_active)) in slots.iter().enumerate() {
        let sx = slot_start_x + (i as i16) * (slot_w + slot_gap);

        if *color != 0 {
            // Material label
            let mat_label = lvgl_sys::lv_label_create(card);
            let mat_text = CString::new(*material).unwrap();
            lvgl_sys::lv_label_set_text(mat_label, mat_text.as_ptr());
            lvgl_sys::lv_obj_set_style_text_color(mat_label, lv_color_hex(COLOR_GRAY), 0);
            lvgl_sys::lv_obj_set_style_text_font(mat_label, &lvgl_sys::lv_font_montserrat_12, 0);
            lvgl_sys::lv_obj_set_pos(mat_label, sx + 4, row_y_mat);

            // Spool icon (circle with hole)
            create_spool_icon(card, sx + 8, row_y_spool, 22, *color, *slot_active);

            // Slot label
            let slot_label = lvgl_sys::lv_label_create(card);
            let slot_text = CString::new(*label).unwrap();
            lvgl_sys::lv_label_set_text(slot_label, slot_text.as_ptr());
            lvgl_sys::lv_obj_set_style_text_color(slot_label, lv_color_hex(if *slot_active { COLOR_ACCENT } else { COLOR_WHITE }), 0);
            lvgl_sys::lv_obj_set_style_text_font(slot_label, &lvgl_sys::lv_font_montserrat_12, 0);
            lvgl_sys::lv_obj_set_pos(slot_label, sx + 10, row_y_label);

            // Percentage
            let pct_label = lvgl_sys::lv_label_create(card);
            let pct_text = CString::new(*percent).unwrap();
            lvgl_sys::lv_label_set_text(pct_label, pct_text.as_ptr());
            lvgl_sys::lv_obj_set_style_text_color(pct_label, lv_color_hex(COLOR_GRAY), 0);
            lvgl_sys::lv_obj_set_style_text_font(pct_label, &lvgl_sys::lv_font_montserrat_12, 0);
            lvgl_sys::lv_obj_set_pos(pct_label, sx + 6, row_y_pct);
        } else {
            // Empty slot - just show slot label centered
            let slot_label = lvgl_sys::lv_label_create(card);
            let slot_text = CString::new(*label).unwrap();
            lvgl_sys::lv_label_set_text(slot_label, slot_text.as_ptr());
            lvgl_sys::lv_obj_set_style_text_color(slot_label, lv_color_hex(0x505050), 0);
            lvgl_sys::lv_obj_set_style_text_font(slot_label, &lvgl_sys::lv_font_montserrat_12, 0);
            lvgl_sys::lv_obj_set_pos(slot_label, sx + 10, row_y_spool + 8);
        }
    }
}

/// Create a spool icon (colored circle with dark hole in center)
unsafe fn create_spool_icon(parent: *mut lvgl_sys::lv_obj_t, x: i16, y: i16, size: i16, color: u32, active: bool) {
    // Outer colored circle
    let spool = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(spool, size, size);
    lvgl_sys::lv_obj_set_pos(spool, x, y);
    lvgl_sys::lv_obj_set_style_bg_color(spool, lv_color_hex(color), 0);
    lvgl_sys::lv_obj_set_style_radius(spool, size / 2, 0);
    lvgl_sys::lv_obj_set_style_border_width(spool, 0, 0);
    set_style_pad_all(spool, 0);

    // Active glow
    if active {
        lvgl_sys::lv_obj_set_style_shadow_color(spool, lv_color_hex(COLOR_ACCENT), 0);
        lvgl_sys::lv_obj_set_style_shadow_width(spool, 10, 0);
        lvgl_sys::lv_obj_set_style_shadow_spread(spool, 2, 0);
        lvgl_sys::lv_obj_set_style_shadow_opa(spool, 150, 0);
    }

    // Inner dark hole
    let hole = lvgl_sys::lv_obj_create(spool);
    let hole_size = size / 3;
    lvgl_sys::lv_obj_set_size(hole, hole_size, hole_size);
    lvgl_sys::lv_obj_align(hole, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_color(hole, lv_color_hex(0x1A1A1A), 0);
    lvgl_sys::lv_obj_set_style_radius(hole, hole_size / 2, 0);
    lvgl_sys::lv_obj_set_style_border_width(hole, 0, 0);
    set_style_pad_all(hole, 0);
}

/// Create a single spool slot within an AMS card
unsafe fn create_ams_spool_slot(
    parent: *mut lvgl_sys::lv_obj_t,
    x: i16, y: i16, w: i16, h: i16,
    material: &str, color: u32, label: &str, percent: &str, active: bool,
) {
    let slot = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(slot, w, h);
    lvgl_sys::lv_obj_set_pos(slot, x, y);
    lvgl_sys::lv_obj_set_style_bg_color(slot, lv_color_hex(if active { 0x1A2A1A } else { 0x2A2A2A }), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(slot, 255, 0);
    lvgl_sys::lv_obj_set_style_radius(slot, 6, 0);
    lvgl_sys::lv_obj_set_style_border_color(slot, lv_color_hex(if active { COLOR_ACCENT } else { 0x3D3D3D }), 0);
    lvgl_sys::lv_obj_set_style_border_width(slot, if active { 2 } else { 1 }, 0);
    set_style_pad_all(slot, 0);

    if active {
        lvgl_sys::lv_obj_set_style_shadow_color(slot, lv_color_hex(COLOR_ACCENT), 0);
        lvgl_sys::lv_obj_set_style_shadow_width(slot, 8, 0);
        lvgl_sys::lv_obj_set_style_shadow_spread(slot, 2, 0);
        lvgl_sys::lv_obj_set_style_shadow_opa(slot, 100, 0);
    }

    if color != 0 {
        // Material type label at top
        let mat_label = lvgl_sys::lv_label_create(slot);
        let mat_text = CString::new(material).unwrap();
        lvgl_sys::lv_label_set_text(mat_label, mat_text.as_ptr());
        lvgl_sys::lv_obj_set_style_text_color(mat_label, lv_color_hex(COLOR_GRAY), 0);
        lvgl_sys::lv_obj_set_style_text_font(mat_label, &lvgl_sys::lv_font_montserrat_12, 0);
        lvgl_sys::lv_obj_align(mat_label, lvgl_sys::LV_ALIGN_TOP_MID as u8, 0, 2);

        // Colored spool circle
        let spool = lvgl_sys::lv_obj_create(slot);
        lvgl_sys::lv_obj_set_size(spool, 14, 14);
        lvgl_sys::lv_obj_align(spool, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
        lvgl_sys::lv_obj_set_style_bg_color(spool, lv_color_hex(color), 0);
        lvgl_sys::lv_obj_set_style_radius(spool, 7, 0);
        lvgl_sys::lv_obj_set_style_border_width(spool, 0, 0);
        set_style_pad_all(spool, 0);

        // Slot label (A1, A2, etc)
        let slot_label = lvgl_sys::lv_label_create(slot);
        let slot_text = CString::new(label).unwrap();
        lvgl_sys::lv_label_set_text(slot_label, slot_text.as_ptr());
        lvgl_sys::lv_obj_set_style_text_color(slot_label, lv_color_hex(if active { COLOR_ACCENT } else { COLOR_GRAY }), 0);
        lvgl_sys::lv_obj_set_style_text_font(slot_label, &lvgl_sys::lv_font_montserrat_12, 0);
        lvgl_sys::lv_obj_align(slot_label, lvgl_sys::LV_ALIGN_BOTTOM_LEFT as u8, 4, -2);

        // Percentage
        let pct_label = lvgl_sys::lv_label_create(slot);
        let pct_text = CString::new(percent).unwrap();
        lvgl_sys::lv_label_set_text(pct_label, pct_text.as_ptr());
        lvgl_sys::lv_obj_set_style_text_color(pct_label, lv_color_hex(COLOR_GRAY), 0);
        lvgl_sys::lv_obj_set_style_text_font(pct_label, &lvgl_sys::lv_font_montserrat_12, 0);
        lvgl_sys::lv_obj_align(pct_label, lvgl_sys::LV_ALIGN_BOTTOM_RIGHT as u8, -2, -2);
    } else {
        // Empty slot - just show slot label
        let slot_label = lvgl_sys::lv_label_create(slot);
        let slot_text = CString::new(label).unwrap();
        lvgl_sys::lv_label_set_text(slot_label, slot_text.as_ptr());
        lvgl_sys::lv_obj_set_style_text_color(slot_label, lv_color_hex(0x505050), 0);
        lvgl_sys::lv_obj_set_style_text_font(slot_label, &lvgl_sys::lv_font_montserrat_12, 0);
        lvgl_sys::lv_obj_align(slot_label, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    }
}

/// Create HT (High Temperature) card with single spool
unsafe fn create_ht_card(
    parent: *mut lvgl_sys::lv_obj_t,
    x: i16, y: i16, w: i16, h: i16,
    name: &str, humidity: &str, material: &str, color: u32, percent: &str,
) {
    let card = create_card(parent, x, y, w, h);

    // Header with name badge
    let badge = lvgl_sys::lv_obj_create(card);
    lvgl_sys::lv_obj_set_size(badge, 40, 14);
    lvgl_sys::lv_obj_set_pos(badge, 4, 4);
    lvgl_sys::lv_obj_set_style_bg_color(badge, lv_color_hex(0x404040), 0);
    lvgl_sys::lv_obj_set_style_radius(badge, 4, 0);
    lvgl_sys::lv_obj_set_style_border_width(badge, 0, 0);
    set_style_pad_all(badge, 0);

    let name_label = lvgl_sys::lv_label_create(badge);
    let name_text = CString::new(name).unwrap();
    lvgl_sys::lv_label_set_text(name_label, name_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(name_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(name_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_align(name_label, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

    // Humidity on right
    let hum_label = lvgl_sys::lv_label_create(card);
    let hum_text = CString::new(humidity).unwrap();
    lvgl_sys::lv_label_set_text(hum_label, hum_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(hum_label, lv_color_hex(COLOR_GRAY), 0);
    lvgl_sys::lv_obj_set_style_text_font(hum_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(hum_label, w - 28, 6);

    // Spool icon with hole
    create_spool_icon(card, (w - 32) / 2, 22, 32, color, false);

    // Material label
    let mat_label = lvgl_sys::lv_label_create(card);
    let mat_text = CString::new(material).unwrap();
    lvgl_sys::lv_label_set_text(mat_label, mat_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(mat_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(mat_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(mat_label, 8, 58);

    // Percentage
    let pct_label = lvgl_sys::lv_label_create(card);
    let pct_text = CString::new(percent).unwrap();
    lvgl_sys::lv_label_set_text(pct_label, pct_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(pct_label, lv_color_hex(COLOR_GRAY), 0);
    lvgl_sys::lv_obj_set_style_text_font(pct_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(pct_label, 8, 72);
}

/// Create External spool card
unsafe fn create_ext_card(
    parent: *mut lvgl_sys::lv_obj_t,
    x: i16, y: i16, w: i16, h: i16,
    name: &str, material: &str, color: u32,
) {
    let card = create_card(parent, x, y, w, h);

    // Header with name badge
    let badge = lvgl_sys::lv_obj_create(card);
    lvgl_sys::lv_obj_set_size(badge, 42, 14);
    lvgl_sys::lv_obj_set_pos(badge, 4, 4);
    lvgl_sys::lv_obj_set_style_bg_color(badge, lv_color_hex(0x404040), 0);
    lvgl_sys::lv_obj_set_style_radius(badge, 4, 0);
    lvgl_sys::lv_obj_set_style_border_width(badge, 0, 0);
    set_style_pad_all(badge, 0);

    let name_label = lvgl_sys::lv_label_create(badge);
    let name_text = CString::new(name).unwrap();
    lvgl_sys::lv_label_set_text(name_label, name_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(name_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(name_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_align(name_label, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

    // Spool icon with hole (larger)
    create_spool_icon(card, (w - 38) / 2, 24, 38, color, false);

    // Material label
    let mat_label = lvgl_sys::lv_label_create(card);
    let mat_text = CString::new(material).unwrap();
    lvgl_sys::lv_label_set_text(mat_label, mat_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(mat_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(mat_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_align(mat_label, lvgl_sys::LV_ALIGN_BOTTOM_MID as u8, 0, -8);
}

/// Create bottom status bar for AMS overview
unsafe fn create_bottom_status_bar(scr: *mut lvgl_sys::lv_obj_t) {
    let bar_y: i16 = 436;
    let bar_h: i16 = 44;

    // Horizontal separator line above status bar
    let separator = lvgl_sys::lv_obj_create(scr);
    lvgl_sys::lv_obj_set_size(separator, 800, 1);
    lvgl_sys::lv_obj_set_pos(separator, 0, bar_y);
    lvgl_sys::lv_obj_set_style_bg_color(separator, lv_color_hex(0x404040), 0);
    lvgl_sys::lv_obj_set_style_border_width(separator, 0, 0);

    // Full-width dark background bar
    let bar = lvgl_sys::lv_obj_create(scr);
    lvgl_sys::lv_obj_set_size(bar, 800, bar_h);
    lvgl_sys::lv_obj_set_pos(bar, 0, bar_y + 1);
    lvgl_sys::lv_obj_set_style_bg_color(bar, lv_color_hex(0x1A1A1A), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(bar, 255, 0);
    lvgl_sys::lv_obj_set_style_border_width(bar, 0, 0);
    lvgl_sys::lv_obj_set_style_radius(bar, 0, 0);
    set_style_pad_all(bar, 0);

    // Connection status (left side)
    let conn_dot = lvgl_sys::lv_obj_create(bar);
    lvgl_sys::lv_obj_set_size(conn_dot, 10, 10);
    lvgl_sys::lv_obj_set_pos(conn_dot, 20, 17);
    lvgl_sys::lv_obj_set_style_bg_color(conn_dot, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_radius(conn_dot, 5, 0);
    lvgl_sys::lv_obj_set_style_border_width(conn_dot, 0, 0);

    let conn_label = lvgl_sys::lv_label_create(bar);
    let conn_text = CString::new("Connected").unwrap();
    lvgl_sys::lv_label_set_text(conn_label, conn_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(conn_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(conn_label, &lvgl_sys::lv_font_montserrat_14, 0);
    lvgl_sys::lv_obj_set_pos(conn_label, 36, 12);

    // Print status (centered) - separate labels for different colors
    let status_label = lvgl_sys::lv_label_create(bar);
    let status_text = CString::new("Printing").unwrap();
    lvgl_sys::lv_label_set_text(status_label, status_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(status_label, lv_color_hex(0xFFA500), 0);  // Orange
    lvgl_sys::lv_obj_set_style_text_font(status_label, &lvgl_sys::lv_font_montserrat_14, 0);
    lvgl_sys::lv_obj_align(status_label, lvgl_sys::LV_ALIGN_CENTER as u8, -95, 0);

    let progress_label = lvgl_sys::lv_label_create(bar);
    let progress_text = CString::new("45% 2h 15m left").unwrap();
    lvgl_sys::lv_label_set_text(progress_label, progress_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(progress_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(progress_label, &lvgl_sys::lv_font_montserrat_14, 0);
    lvgl_sys::lv_obj_align(progress_label, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

    // Last sync time (far right) - combined label
    let sync_label = lvgl_sys::lv_label_create(bar);
    let sync_text = CString::new("Updated 5s ago").unwrap();
    lvgl_sys::lv_label_set_text(sync_label, sync_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(sync_label, lv_color_hex(COLOR_GRAY), 0);
    lvgl_sys::lv_obj_set_style_text_font(sync_label, &lvgl_sys::lv_font_montserrat_14, 0);
    lvgl_sys::lv_obj_align(sync_label, lvgl_sys::LV_ALIGN_RIGHT_MID as u8, -20, 0);
}

/// Create a card with glossy styling - shiny highlights and depth
unsafe fn create_card(parent: *mut lvgl_sys::lv_obj_t, x: i16, y: i16, w: i16, h: i16) -> *mut lvgl_sys::lv_obj_t {
    let card = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(card, w, h);
    lvgl_sys::lv_obj_set_pos(card, x, y);

    // Dark card background matching AMS cards
    lvgl_sys::lv_obj_set_style_bg_color(card, lv_color_hex(0x2D2D2D), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(card, 255, 0);

    // Subtle border
    lvgl_sys::lv_obj_set_style_border_color(card, lv_color_hex(0x404040), 0);
    lvgl_sys::lv_obj_set_style_border_width(card, 1, 0);
    lvgl_sys::lv_obj_set_style_radius(card, 8, 0);

    set_style_pad_all(card, 0);

    card
}

/// Create a card with solid green border (for highlighted/active cards)
unsafe fn create_card_glow(parent: *mut lvgl_sys::lv_obj_t, x: i16, y: i16, w: i16, h: i16) -> *mut lvgl_sys::lv_obj_t {
    let card = create_card(parent, x, y, w, h);

    // Solid green border (no glow/shadow, just clean border)
    lvgl_sys::lv_obj_set_style_border_color(card, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_border_width(card, 2, 0);
    lvgl_sys::lv_obj_set_style_border_opa(card, 255, 0);

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

/// Common content for action buttons - polished version
unsafe fn create_action_button_content(btn: *mut lvgl_sys::lv_obj_t, title: &str, _subtitle: &str, icon_type: &str) {
    // Icon container - centered in upper portion of button (smaller 40x40)
    let icon_container = lvgl_sys::lv_obj_create(btn);
    lvgl_sys::lv_obj_set_size(icon_container, 40, 40);
    lvgl_sys::lv_obj_align(icon_container, lvgl_sys::LV_ALIGN_TOP_MID as u8, 0, 12);
    lvgl_sys::lv_obj_set_style_bg_opa(icon_container, 0, 0);
    lvgl_sys::lv_obj_set_style_border_width(icon_container, 0, 0);
    set_style_pad_all(icon_container, 0);

    match icon_type {
        "ams" => draw_ams_icon(icon_container),
        "encode" => draw_encode_icon(icon_container),
        "catalog" => draw_catalog_icon(icon_container),
        "settings" => draw_settings_icon(icon_container),
        "nfc" => draw_nfc_icon(icon_container),
        "calibrate" => draw_calibrate_icon(icon_container),
        _ => {}
    }

    // Title - smaller font, positioned at bottom
    let title_label = lvgl_sys::lv_label_create(btn);
    let title_cstr = CString::new(title).unwrap();
    lvgl_sys::lv_label_set_text(title_label, title_cstr.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(title_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(title_label, &lvgl_sys::lv_font_montserrat_10, 0);
    lvgl_sys::lv_obj_align(title_label, lvgl_sys::LV_ALIGN_BOTTOM_MID as u8, 0, -6);
}

/// Draw AMS Setup icon (table/grid with rows, black background)
unsafe fn draw_ams_icon(parent: *mut lvgl_sys::lv_obj_t) {
    // Black rounded background
    let bg = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(bg, 40, 40);
    lvgl_sys::lv_obj_align(bg, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_color(bg, lv_color_hex(0x000000), 0);
    lvgl_sys::lv_obj_set_style_radius(bg, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(bg, 0, 0);
    set_style_pad_all(bg, 0);

    // Outer frame
    let frame = lvgl_sys::lv_obj_create(bg);
    lvgl_sys::lv_obj_set_size(frame, 28, 28);
    lvgl_sys::lv_obj_align(frame, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_opa(frame, 0, 0);
    lvgl_sys::lv_obj_set_style_border_color(frame, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_border_width(frame, 2, 0);
    lvgl_sys::lv_obj_set_style_radius(frame, 3, 0);
    set_style_pad_all(frame, 0);

    // Horizontal lines (3 rows)
    for i in 0..3 {
        let line = lvgl_sys::lv_obj_create(frame);
        lvgl_sys::lv_obj_set_size(line, 18, 2);
        lvgl_sys::lv_obj_set_pos(line, 3, 4 + i * 7);
        lvgl_sys::lv_obj_set_style_bg_color(line, lv_color_hex(COLOR_ACCENT), 0);
        lvgl_sys::lv_obj_set_style_border_width(line, 0, 0);
        lvgl_sys::lv_obj_set_style_radius(line, 1, 0);
    }
}

/// Draw Encode Tag icon (PNG with black background)
unsafe fn draw_encode_icon(parent: *mut lvgl_sys::lv_obj_t) {
    // Black rounded background
    let bg = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(bg, 40, 40);
    lvgl_sys::lv_obj_align(bg, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_color(bg, lv_color_hex(0x000000), 0);
    lvgl_sys::lv_obj_set_style_radius(bg, 8, 0);
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
    lvgl_sys::lv_obj_set_size(bg, 40, 40);
    lvgl_sys::lv_obj_align(bg, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_color(bg, lv_color_hex(0x000000), 0);
    lvgl_sys::lv_obj_set_style_radius(bg, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(bg, 0, 0);
    set_style_pad_all(bg, 0);

    // 3x3 grid of small squares
    let size: i16 = 8;
    let gap: i16 = 2;
    let start_x: i16 = 6;
    let start_y: i16 = 6;

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
    lvgl_sys::lv_obj_set_size(bg, 40, 40);
    lvgl_sys::lv_obj_align(bg, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_color(bg, lv_color_hex(0x000000), 0);
    lvgl_sys::lv_obj_set_style_radius(bg, 8, 0);
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

/// Draw NFC/Scan icon (PNG with black background)
unsafe fn draw_nfc_icon(parent: *mut lvgl_sys::lv_obj_t) {
    // Black rounded background
    let bg = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(bg, 40, 40);
    lvgl_sys::lv_obj_align(bg, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_color(bg, lv_color_hex(0x000000), 0);
    lvgl_sys::lv_obj_set_style_radius(bg, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(bg, 0, 0);
    set_style_pad_all(bg, 0);

    // Initialize NFC image descriptor
    NFC_IMG_DSC.header._bitfield_1 = lvgl_sys::lv_img_header_t::new_bitfield_1(
        lvgl_sys::LV_IMG_CF_TRUE_COLOR_ALPHA as u32,
        0, 0,
        NFC_WIDTH,
        NFC_HEIGHT,
    );
    NFC_IMG_DSC.data_size = (NFC_WIDTH * NFC_HEIGHT * 3) as u32;
    NFC_IMG_DSC.data = NFC_DATA.as_ptr();

    // PNG icon - 28x28, no scaling needed
    let icon = lvgl_sys::lv_img_create(bg);
    lvgl_sys::lv_img_set_src(icon, &raw const NFC_IMG_DSC as *const _);
    lvgl_sys::lv_obj_align(icon, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    // Green tint
    lvgl_sys::lv_obj_set_style_img_recolor(icon, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_img_recolor_opa(icon, 255, 0);
}

/// Draw Calibrate icon (scale/balance icon, drawn programmatically)
unsafe fn draw_calibrate_icon(parent: *mut lvgl_sys::lv_obj_t) {
    // Black rounded background
    let bg = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(bg, 40, 40);
    lvgl_sys::lv_obj_align(bg, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_color(bg, lv_color_hex(0x000000), 0);
    lvgl_sys::lv_obj_set_style_radius(bg, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(bg, 0, 0);
    set_style_pad_all(bg, 0);

    // Draw a simple scale/target icon
    // Center crosshair
    let h_line = lvgl_sys::lv_obj_create(bg);
    lvgl_sys::lv_obj_set_size(h_line, 24, 2);
    lvgl_sys::lv_obj_align(h_line, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_color(h_line, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_border_width(h_line, 0, 0);
    lvgl_sys::lv_obj_set_style_radius(h_line, 1, 0);

    let v_line = lvgl_sys::lv_obj_create(bg);
    lvgl_sys::lv_obj_set_size(v_line, 2, 24);
    lvgl_sys::lv_obj_align(v_line, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_color(v_line, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_border_width(v_line, 0, 0);
    lvgl_sys::lv_obj_set_style_radius(v_line, 1, 0);

    // Outer circle (target ring)
    let ring = lvgl_sys::lv_obj_create(bg);
    lvgl_sys::lv_obj_set_size(ring, 28, 28);
    lvgl_sys::lv_obj_align(ring, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_opa(ring, 0, 0);
    lvgl_sys::lv_obj_set_style_border_color(ring, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_border_width(ring, 2, 0);
    lvgl_sys::lv_obj_set_style_radius(ring, 14, 0);
    set_style_pad_all(ring, 0);

    // Center dot
    let dot = lvgl_sys::lv_obj_create(bg);
    lvgl_sys::lv_obj_set_size(dot, 4, 4);
    lvgl_sys::lv_obj_align(dot, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_color(dot, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_border_width(dot, 0, 0);
    lvgl_sys::lv_obj_set_style_radius(dot, 2, 0);
}

/// Create an AMS slot with 4 color squares (for regular AMS units A, B, C, D)
/// active_slot: -1 for none, 0-3 for which color slot is currently in use
unsafe fn create_ams_slot_4color(parent: *mut lvgl_sys::lv_obj_t, x: i16, y: i16, label: &str, active_slot: i8, colors: &[u32; 4]) {
    let has_active = active_slot >= 0 && active_slot < 4;

    let slot = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(slot, 72, 42);
    lvgl_sys::lv_obj_set_pos(slot, x, y);
    lvgl_sys::lv_obj_set_style_bg_color(slot, lv_color_hex(if has_active { 0x1A2A1A } else { 0x2A2A2A }), 0);
    lvgl_sys::lv_obj_set_style_radius(slot, 8, 0);
    lvgl_sys::lv_obj_set_style_border_color(slot, lv_color_hex(if has_active { COLOR_ACCENT } else { 0x3D3D3D }), 0);
    lvgl_sys::lv_obj_set_style_border_width(slot, if has_active { 2 } else { 1 }, 0);

    // Strong glow for active AMS unit
    if has_active {
        lvgl_sys::lv_obj_set_style_shadow_color(slot, lv_color_hex(COLOR_ACCENT), 0);
        lvgl_sys::lv_obj_set_style_shadow_width(slot, 20, 0);
        lvgl_sys::lv_obj_set_style_shadow_spread(slot, 2, 0);
        lvgl_sys::lv_obj_set_style_shadow_opa(slot, 150, 0);
    }

    set_style_pad_all(slot, 0);

    // Slot label (A, B, C, D) - prominent, at top
    let slot_label = lvgl_sys::lv_label_create(slot);
    let slot_text = CString::new(label).unwrap();
    lvgl_sys::lv_label_set_text(slot_label, slot_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(slot_label, lv_color_hex(if has_active { COLOR_ACCENT } else { COLOR_WHITE }), 0);
    lvgl_sys::lv_obj_set_style_text_font(slot_label, &lvgl_sys::lv_font_montserrat_14, 0);
    lvgl_sys::lv_obj_align(slot_label, lvgl_sys::LV_ALIGN_TOP_MID as u8, 0, 0);

    // 4 color squares in a row
    let square_size: i16 = 14;
    let square_gap: i16 = 2;
    let total_width = square_size * 4 + square_gap * 3;
    let start_x = (72 - total_width) / 2;

    for (i, &color) in colors.iter().enumerate() {
        let is_active_color = has_active && i as i8 == active_slot;

        let sq = lvgl_sys::lv_obj_create(slot);
        lvgl_sys::lv_obj_set_size(sq, square_size, square_size);
        lvgl_sys::lv_obj_set_pos(sq, start_x + (i as i16) * (square_size + square_gap), 22);
        lvgl_sys::lv_obj_set_style_radius(sq, 3, 0);
        set_style_pad_all(sq, 0);

        if color == 0 {
            // Empty slot - gray background
            lvgl_sys::lv_obj_set_style_bg_color(sq, lv_color_hex(0x404040), 0);
            lvgl_sys::lv_obj_set_style_border_width(sq, 0, 0);
        } else {
            lvgl_sys::lv_obj_set_style_bg_color(sq, lv_color_hex(color), 0);

            if is_active_color {
                // Active color slot - bright green border and strong glow
                lvgl_sys::lv_obj_set_style_border_color(sq, lv_color_hex(COLOR_ACCENT), 0);
                lvgl_sys::lv_obj_set_style_border_width(sq, 2, 0);
                lvgl_sys::lv_obj_set_style_shadow_color(sq, lv_color_hex(COLOR_ACCENT), 0);
                lvgl_sys::lv_obj_set_style_shadow_width(sq, 10, 0);
                lvgl_sys::lv_obj_set_style_shadow_spread(sq, 2, 0);
                lvgl_sys::lv_obj_set_style_shadow_opa(sq, 200, 0);
            } else {
                // Inactive color slot - subtle glow
                lvgl_sys::lv_obj_set_style_border_width(sq, 0, 0);
                lvgl_sys::lv_obj_set_style_shadow_color(sq, lv_color_hex(color), 0);
                lvgl_sys::lv_obj_set_style_shadow_width(sq, 4, 0);
                lvgl_sys::lv_obj_set_style_shadow_spread(sq, 0, 0);
                lvgl_sys::lv_obj_set_style_shadow_opa(sq, 80, 0);
            }
        }
    }
}

/// Create a single-color AMS slot for EXT and HT slots
/// active: whether this slot is currently in use
unsafe fn create_ams_slot_single(parent: *mut lvgl_sys::lv_obj_t, x: i16, y: i16, label: &str, color: u32, active: bool) {
    let slot = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(slot, 72, 22);
    lvgl_sys::lv_obj_set_pos(slot, x, y);
    // Visible background and border for all slots
    lvgl_sys::lv_obj_set_style_bg_color(slot, lv_color_hex(if active { 0x1A2A1A } else { 0x2A2A2A }), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(slot, 255, 0);
    lvgl_sys::lv_obj_set_style_radius(slot, 6, 0);
    lvgl_sys::lv_obj_set_style_border_color(slot, lv_color_hex(if active { COLOR_ACCENT } else { 0x4A4A4A }), 0);
    lvgl_sys::lv_obj_set_style_border_width(slot, if active { 2 } else { 1 }, 0);
    set_style_pad_all(slot, 0);

    // Active glow
    if active {
        lvgl_sys::lv_obj_set_style_shadow_color(slot, lv_color_hex(COLOR_ACCENT), 0);
        lvgl_sys::lv_obj_set_style_shadow_width(slot, 12, 0);
        lvgl_sys::lv_obj_set_style_shadow_spread(slot, 1, 0);
        lvgl_sys::lv_obj_set_style_shadow_opa(slot, 120, 0);
    }

    // Slot label
    let slot_label = lvgl_sys::lv_label_create(slot);
    let slot_text = CString::new(label).unwrap();
    lvgl_sys::lv_label_set_text(slot_label, slot_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(slot_label, lv_color_hex(if active { COLOR_ACCENT } else { COLOR_GRAY }), 0);
    lvgl_sys::lv_obj_align(slot_label, lvgl_sys::LV_ALIGN_LEFT_MID as u8, 8, 0);

    // Color indicator (small square)
    let color_sq = lvgl_sys::lv_obj_create(slot);
    lvgl_sys::lv_obj_set_size(color_sq, 14, 14);
    lvgl_sys::lv_obj_align(color_sq, lvgl_sys::LV_ALIGN_RIGHT_MID as u8, -6, 0);
    lvgl_sys::lv_obj_set_style_radius(color_sq, 3, 0);
    set_style_pad_all(color_sq, 0);

    if color == 0 {
        // Empty slot - diagonal stripe
        lvgl_sys::lv_obj_set_style_bg_color(color_sq, lv_color_hex(0x404040), 0);
        lvgl_sys::lv_obj_set_style_border_width(color_sq, 0, 0);
        let stripe = lvgl_sys::lv_obj_create(color_sq);
        lvgl_sys::lv_obj_set_size(stripe, 18, 2);
        lvgl_sys::lv_obj_align(stripe, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
        lvgl_sys::lv_obj_set_style_bg_color(stripe, lv_color_hex(0x606060), 0);
        lvgl_sys::lv_obj_set_style_border_width(stripe, 0, 0);
        lvgl_sys::lv_obj_set_style_transform_angle(stripe, 450, 0);
    } else {
        lvgl_sys::lv_obj_set_style_bg_color(color_sq, lv_color_hex(color), 0);
        if active {
            // Active - green border and strong glow
            lvgl_sys::lv_obj_set_style_border_color(color_sq, lv_color_hex(COLOR_ACCENT), 0);
            lvgl_sys::lv_obj_set_style_border_width(color_sq, 2, 0);
            lvgl_sys::lv_obj_set_style_shadow_color(color_sq, lv_color_hex(COLOR_ACCENT), 0);
            lvgl_sys::lv_obj_set_style_shadow_width(color_sq, 8, 0);
            lvgl_sys::lv_obj_set_style_shadow_spread(color_sq, 2, 0);
            lvgl_sys::lv_obj_set_style_shadow_opa(color_sq, 180, 0);
        } else {
            // Inactive - subtle glow
            lvgl_sys::lv_obj_set_style_border_width(color_sq, 0, 0);
            lvgl_sys::lv_obj_set_style_shadow_color(color_sq, lv_color_hex(color), 0);
            lvgl_sys::lv_obj_set_style_shadow_width(color_sq, 4, 0);
            lvgl_sys::lv_obj_set_style_shadow_spread(color_sq, 0, 0);
            lvgl_sys::lv_obj_set_style_shadow_opa(color_sq, 80, 0);
        }
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
