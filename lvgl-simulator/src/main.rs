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

// Humidity icon (10x10) for AMS overview
const HUMIDITY_WIDTH: u32 = 10;
const HUMIDITY_HEIGHT: u32 = 10;
static HUMIDITY_DATA: &[u8] = include_bytes!("../assets/humidity_mockup.bin");

// Temperature icon (10x10) for AMS overview
const TEMP_WIDTH: u32 = 10;
const TEMP_HEIGHT: u32 = 10;
static TEMP_DATA: &[u8] = include_bytes!("../assets/temp_mockup.bin");

// Spool clean icon (32x42) for AMS overview
const SPOOL_WIDTH: u32 = 32;
const SPOOL_HEIGHT: u32 = 42;
static SPOOL_CLEAN_DATA: &[u8] = include_bytes!("../assets/spool_clean.bin");

// Image descriptors (static, initialized at runtime)
static mut LOGO_IMG_DSC: lvgl_sys::lv_img_dsc_t = unsafe { std::mem::zeroed() };
static mut BELL_IMG_DSC: lvgl_sys::lv_img_dsc_t = unsafe { std::mem::zeroed() };
static mut NFC_IMG_DSC: lvgl_sys::lv_img_dsc_t = unsafe { std::mem::zeroed() };
static mut WEIGHT_IMG_DSC: lvgl_sys::lv_img_dsc_t = unsafe { std::mem::zeroed() };
static mut POWER_IMG_DSC: lvgl_sys::lv_img_dsc_t = unsafe { std::mem::zeroed() };
static mut SETTING_IMG_DSC: lvgl_sys::lv_img_dsc_t = unsafe { std::mem::zeroed() };
static mut ENCODE_IMG_DSC: lvgl_sys::lv_img_dsc_t = unsafe { std::mem::zeroed() };
static mut HUMIDITY_IMG_DSC: lvgl_sys::lv_img_dsc_t = unsafe { std::mem::zeroed() };
static mut TEMP_IMG_DSC: lvgl_sys::lv_img_dsc_t = unsafe { std::mem::zeroed() };
static mut SPOOL_CLEAN_IMG_DSC: lvgl_sys::lv_img_dsc_t = unsafe { std::mem::zeroed() };

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
            "scan-result" => {
                create_scan_result_screen();
                "scan_result"
            }
            "spool-detail" => {
                create_spool_detail_screen();
                "spool_detail"
            }
            "catalog" => {
                create_catalog_screen();
                "catalog"
            }
            "settings" => {
                create_settings_screen();
                "settings"
            }
            _ => {
                create_home_screen();
                "home"
            }
        };
        info!("UI created: {}", screenshot_name);

        // Run a few frames to let LVGL render
        for _ in 0..10 {
            lvgl_sys::lv_tick_inc(10);
            lvgl_sys::lv_timer_handler();
            std::thread::sleep(std::time::Duration::from_millis(10));
        }

        // Save screenshot
        std::fs::create_dir_all("screenshots").unwrap();
        let bmp_path = format!("screenshots/{}.bmp", screenshot_name);
        let raw_path = format!("screenshots/{}.raw", screenshot_name);

        save_framebuffer_as_bmp(&bmp_path);
        info!("Saved: {}", bmp_path);

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
const COLOR_TEXT_MUTED: u32 = 0x707070;
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
    lvgl_sys::lv_obj_set_size(l_badge, 14, 14);
    lvgl_sys::lv_obj_set_pos(l_badge, 12, 11);
    lvgl_sys::lv_obj_set_style_bg_color(l_badge, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_radius(l_badge, 2, 0);
    lvgl_sys::lv_obj_set_style_border_width(l_badge, 0, 0);
    set_style_pad_all(l_badge, 0);
    let l_letter = lvgl_sys::lv_label_create(l_badge);
    let l_text = CString::new("L").unwrap();
    lvgl_sys::lv_label_set_text(l_letter, l_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(l_letter, lv_color_hex(COLOR_BG), 0);
    lvgl_sys::lv_obj_set_style_text_font(l_letter, &lvgl_sys::lv_font_montserrat_10, 0);
    lvgl_sys::lv_obj_align(l_letter, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

    let left_label = lvgl_sys::lv_label_create(left_nozzle);
    let left_text = CString::new("Left Nozzle").unwrap();
    lvgl_sys::lv_label_set_text(left_label, left_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(left_label, lv_color_hex(COLOR_GRAY), 0);
    lvgl_sys::lv_obj_set_style_text_font(left_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(left_label, 30, 11);

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
    lvgl_sys::lv_obj_set_size(r_badge, 14, 14);
    lvgl_sys::lv_obj_set_pos(r_badge, 12, 11);
    lvgl_sys::lv_obj_set_style_bg_color(r_badge, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_radius(r_badge, 2, 0);
    lvgl_sys::lv_obj_set_style_border_width(r_badge, 0, 0);
    set_style_pad_all(r_badge, 0);
    let r_letter = lvgl_sys::lv_label_create(r_badge);
    let r_text = CString::new("R").unwrap();
    lvgl_sys::lv_label_set_text(r_letter, r_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(r_letter, lv_color_hex(COLOR_BG), 0);
    lvgl_sys::lv_obj_set_style_text_font(r_letter, &lvgl_sys::lv_font_montserrat_10, 0);
    lvgl_sys::lv_obj_align(r_letter, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

    let right_label = lvgl_sys::lv_label_create(right_nozzle);
    let right_text = CString::new("Right Nozzle").unwrap();
    lvgl_sys::lv_label_set_text(right_label, right_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(right_label, lv_color_hex(COLOR_GRAY), 0);
    lvgl_sys::lv_obj_set_style_text_font(right_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(right_label, 30, 11);

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

/// Create a card with glossy styling - shiny highlights and depth
unsafe fn create_card(parent: *mut lvgl_sys::lv_obj_t, x: i16, y: i16, w: i16, h: i16) -> *mut lvgl_sys::lv_obj_t {
    let card = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(card, w, h);
    lvgl_sys::lv_obj_set_pos(card, x, y);

    // Dark polished background
    lvgl_sys::lv_obj_set_style_bg_color(card, lv_color_hex(0x2D2D2D), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(card, 255, 0);

    // Subtle border for glossy bevel effect
    lvgl_sys::lv_obj_set_style_border_color(card, lv_color_hex(0x505050), 0);
    lvgl_sys::lv_obj_set_style_border_width(card, 1, 0);
    lvgl_sys::lv_obj_set_style_radius(card, 14, 0);

    // Strong shadow for depth and polish
    lvgl_sys::lv_obj_set_style_shadow_color(card, lv_color_hex(0x000000), 0);
    lvgl_sys::lv_obj_set_style_shadow_width(card, 12, 0);
    lvgl_sys::lv_obj_set_style_shadow_ofs_x(card, 0, 0);
    lvgl_sys::lv_obj_set_style_shadow_ofs_y(card, 4, 0);
    lvgl_sys::lv_obj_set_style_shadow_spread(card, 2, 0);
    lvgl_sys::lv_obj_set_style_shadow_opa(card, 180, 0);

    set_style_pad_all(card, 0);

    // Bright glossy highlight at top edge
    let gloss_top = lvgl_sys::lv_obj_create(card);
    lvgl_sys::lv_obj_set_size(gloss_top, w - 28, 3);
    lvgl_sys::lv_obj_set_pos(gloss_top, 14, 1);
    lvgl_sys::lv_obj_set_style_bg_color(gloss_top, lv_color_hex(0xFFFFFF), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(gloss_top, 80, 0);
    lvgl_sys::lv_obj_set_style_radius(gloss_top, 2, 0);
    lvgl_sys::lv_obj_set_style_border_width(gloss_top, 0, 0);
    set_style_pad_all(gloss_top, 0);

    // Dark bottom edge for depth
    let dark_bottom = lvgl_sys::lv_obj_create(card);
    lvgl_sys::lv_obj_set_size(dark_bottom, w - 28, 2);
    lvgl_sys::lv_obj_set_pos(dark_bottom, 14, h - 3);
    lvgl_sys::lv_obj_set_style_bg_color(dark_bottom, lv_color_hex(0x000000), 0);
    lvgl_sys::lv_obj_set_style_bg_opa(dark_bottom, 60, 0);
    lvgl_sys::lv_obj_set_style_radius(dark_bottom, 2, 0);
    lvgl_sys::lv_obj_set_style_border_width(dark_bottom, 0, 0);
    set_style_pad_all(dark_bottom, 0);

    card
}

/// Create a card with accent glow (for highlighted/active cards)
unsafe fn create_card_glow(parent: *mut lvgl_sys::lv_obj_t, x: i16, y: i16, w: i16, h: i16) -> *mut lvgl_sys::lv_obj_t {
    let card = create_card(parent, x, y, w, h);

    // Add green accent glow
    lvgl_sys::lv_obj_set_style_shadow_color(card, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_shadow_width(card, 25, 0);
    lvgl_sys::lv_obj_set_style_shadow_spread(card, 2, 0);
    lvgl_sys::lv_obj_set_style_shadow_opa(card, 100, 0);

    // Accent border
    lvgl_sys::lv_obj_set_style_border_color(card, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_border_opa(card, 150, 0);

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

/// Common content for action buttons - compact version for sidebar
unsafe fn create_action_button_content(btn: *mut lvgl_sys::lv_obj_t, title: &str, _subtitle: &str, icon_type: &str) {
    // Icon container - centered in upper portion of button (smaller 40x40)
    let icon_container = lvgl_sys::lv_obj_create(btn);
    lvgl_sys::lv_obj_set_size(icon_container, 40, 40);
    lvgl_sys::lv_obj_align(icon_container, lvgl_sys::LV_ALIGN_TOP_MID as u8, 0, 12);
    lvgl_sys::lv_obj_set_style_bg_opa(icon_container, 0, 0);
    lvgl_sys::lv_obj_set_style_border_width(icon_container, 0, 0);
    set_style_pad_all(icon_container, 0);

    match icon_type {
        "ams" => draw_ams_icon_small(icon_container),
        "encode" => draw_encode_icon_small(icon_container),
        "catalog" => draw_catalog_icon_small(icon_container),
        "settings" => draw_settings_icon_small(icon_container),
        "nfc" => draw_nfc_icon_small(icon_container),
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

/// Draw AMS icon (small 40x40 version for sidebar)
unsafe fn draw_ams_icon_small(parent: *mut lvgl_sys::lv_obj_t) {
    let bg = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(bg, 40, 40);
    lvgl_sys::lv_obj_align(bg, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_color(bg, lv_color_hex(0x000000), 0);
    lvgl_sys::lv_obj_set_style_radius(bg, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(bg, 0, 0);
    set_style_pad_all(bg, 0);

    let frame = lvgl_sys::lv_obj_create(bg);
    lvgl_sys::lv_obj_set_size(frame, 28, 28);
    lvgl_sys::lv_obj_align(frame, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_opa(frame, 0, 0);
    lvgl_sys::lv_obj_set_style_border_color(frame, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_border_width(frame, 2, 0);
    lvgl_sys::lv_obj_set_style_radius(frame, 3, 0);
    set_style_pad_all(frame, 0);

    for i in 0..3 {
        let line = lvgl_sys::lv_obj_create(frame);
        lvgl_sys::lv_obj_set_size(line, 18, 2);
        lvgl_sys::lv_obj_set_pos(line, 3, 4 + i * 7);
        lvgl_sys::lv_obj_set_style_bg_color(line, lv_color_hex(COLOR_ACCENT), 0);
        lvgl_sys::lv_obj_set_style_border_width(line, 0, 0);
        lvgl_sys::lv_obj_set_style_radius(line, 1, 0);
    }
}

/// Draw Encode icon (small 40x40 version for sidebar)
unsafe fn draw_encode_icon_small(parent: *mut lvgl_sys::lv_obj_t) {
    let bg = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(bg, 40, 40);
    lvgl_sys::lv_obj_align(bg, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_color(bg, lv_color_hex(0x000000), 0);
    lvgl_sys::lv_obj_set_style_radius(bg, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(bg, 0, 0);
    set_style_pad_all(bg, 0);

    ENCODE_IMG_DSC.header._bitfield_1 = lvgl_sys::lv_img_header_t::new_bitfield_1(
        lvgl_sys::LV_IMG_CF_TRUE_COLOR_ALPHA as u32, 0, 0, ENCODE_WIDTH, ENCODE_HEIGHT,
    );
    ENCODE_IMG_DSC.data_size = (ENCODE_WIDTH * ENCODE_HEIGHT * 3) as u32;
    ENCODE_IMG_DSC.data = ENCODE_DATA.as_ptr();

    let icon = lvgl_sys::lv_img_create(bg);
    lvgl_sys::lv_img_set_src(icon, &raw const ENCODE_IMG_DSC as *const _);
    lvgl_sys::lv_obj_align(icon, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_img_set_zoom(icon, 179);  // Scale 40x40 to ~28x28
    lvgl_sys::lv_obj_set_style_img_recolor(icon, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_img_recolor_opa(icon, 255, 0);
}

/// Draw Catalog icon (small 40x40 version for sidebar)
unsafe fn draw_catalog_icon_small(parent: *mut lvgl_sys::lv_obj_t) {
    let bg = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(bg, 40, 40);
    lvgl_sys::lv_obj_align(bg, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_color(bg, lv_color_hex(0x000000), 0);
    lvgl_sys::lv_obj_set_style_radius(bg, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(bg, 0, 0);
    set_style_pad_all(bg, 0);

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

/// Draw Settings icon (small 40x40 version for sidebar)
unsafe fn draw_settings_icon_small(parent: *mut lvgl_sys::lv_obj_t) {
    let bg = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(bg, 40, 40);
    lvgl_sys::lv_obj_align(bg, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_color(bg, lv_color_hex(0x000000), 0);
    lvgl_sys::lv_obj_set_style_radius(bg, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(bg, 0, 0);
    set_style_pad_all(bg, 0);

    SETTING_IMG_DSC.header._bitfield_1 = lvgl_sys::lv_img_header_t::new_bitfield_1(
        lvgl_sys::LV_IMG_CF_TRUE_COLOR_ALPHA as u32, 0, 0, SETTING_WIDTH, SETTING_HEIGHT,
    );
    SETTING_IMG_DSC.data_size = (SETTING_WIDTH * SETTING_HEIGHT * 3) as u32;
    SETTING_IMG_DSC.data = SETTING_DATA.as_ptr();

    let icon = lvgl_sys::lv_img_create(bg);
    lvgl_sys::lv_img_set_src(icon, &raw const SETTING_IMG_DSC as *const _);
    lvgl_sys::lv_obj_align(icon, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_img_set_zoom(icon, 179);  // Scale 40x40 to ~28x28
    lvgl_sys::lv_obj_set_style_img_recolor(icon, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_img_recolor_opa(icon, 255, 0);
}

/// Draw NFC/Scan icon (small 40x40 version for sidebar)
unsafe fn draw_nfc_icon_small(parent: *mut lvgl_sys::lv_obj_t) {
    let bg = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(bg, 40, 40);
    lvgl_sys::lv_obj_align(bg, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_color(bg, lv_color_hex(0x000000), 0);
    lvgl_sys::lv_obj_set_style_radius(bg, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(bg, 0, 0);
    set_style_pad_all(bg, 0);

    NFC_IMG_DSC.header._bitfield_1 = lvgl_sys::lv_img_header_t::new_bitfield_1(
        lvgl_sys::LV_IMG_CF_TRUE_COLOR_ALPHA as u32, 0, 0, NFC_WIDTH, NFC_HEIGHT,
    );
    NFC_IMG_DSC.data_size = (NFC_WIDTH * NFC_HEIGHT * 3) as u32;
    NFC_IMG_DSC.data = NFC_DATA.as_ptr();

    let icon = lvgl_sys::lv_img_create(bg);
    lvgl_sys::lv_img_set_src(icon, &raw const NFC_IMG_DSC as *const _);
    lvgl_sys::lv_obj_align(icon, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_img_set_zoom(icon, 100);  // Scale 72x72 to ~28x28 (100/256 * 72 ≈ 28)
    lvgl_sys::lv_obj_set_style_img_recolor(icon, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_img_recolor_opa(icon, 255, 0);
}

/// Draw Calibrate icon (target/crosshair)
unsafe fn draw_calibrate_icon(parent: *mut lvgl_sys::lv_obj_t) {
    let bg = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(bg, 40, 40);
    lvgl_sys::lv_obj_align(bg, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_color(bg, lv_color_hex(0x000000), 0);
    lvgl_sys::lv_obj_set_style_radius(bg, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(bg, 0, 0);
    set_style_pad_all(bg, 0);

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

    let ring = lvgl_sys::lv_obj_create(bg);
    lvgl_sys::lv_obj_set_size(ring, 28, 28);
    lvgl_sys::lv_obj_align(ring, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_opa(ring, 0, 0);
    lvgl_sys::lv_obj_set_style_border_color(ring, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_border_width(ring, 2, 0);
    lvgl_sys::lv_obj_set_style_radius(ring, 14, 0);
    set_style_pad_all(ring, 0);

    let dot = lvgl_sys::lv_obj_create(bg);
    lvgl_sys::lv_obj_set_size(dot, 4, 4);
    lvgl_sys::lv_obj_align(dot, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_set_style_bg_color(dot, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_border_width(dot, 0, 0);
    lvgl_sys::lv_obj_set_style_radius(dot, 2, 0);
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
/// AMS Overview screen - shows all 4 AMS units
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
    let panel_h: i16 = 388;

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
    let row1_h: i16 = 170;
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

/// Create bottom status bar
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

    // Print status (centered)
    let status_label = lvgl_sys::lv_label_create(bar);
    let status_text = CString::new("Printing").unwrap();
    lvgl_sys::lv_label_set_text(status_label, status_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(status_label, lv_color_hex(0xFFA500), 0);
    lvgl_sys::lv_obj_set_style_text_font(status_label, &lvgl_sys::lv_font_montserrat_14, 0);
    lvgl_sys::lv_obj_align(status_label, lvgl_sys::LV_ALIGN_CENTER as u8, -95, 0);

    let progress_label = lvgl_sys::lv_label_create(bar);
    let progress_text = CString::new("45% - 2h 15m left").unwrap();
    lvgl_sys::lv_label_set_text(progress_label, progress_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(progress_label, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(progress_label, &lvgl_sys::lv_font_montserrat_14, 0);
    lvgl_sys::lv_obj_align(progress_label, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

    // Last sync time (far right)
    let sync_label = lvgl_sys::lv_label_create(bar);
    let sync_text = CString::new("Updated 5s ago").unwrap();
    lvgl_sys::lv_label_set_text(sync_label, sync_text.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(sync_label, lv_color_hex(COLOR_GRAY), 0);
    lvgl_sys::lv_obj_set_style_text_font(sync_label, &lvgl_sys::lv_font_montserrat_14, 0);
    lvgl_sys::lv_obj_align(sync_label, lvgl_sys::LV_ALIGN_RIGHT_MID as u8, -20, 0);
}

fn lighten_color(color: u32, amount: u8) -> u32 {
    let r = ((color >> 16) & 0xFF) as u8;
    let g = ((color >> 8) & 0xFF) as u8;
    let b = (color & 0xFF) as u8;
    let r = r.saturating_add(amount);
    let g = g.saturating_add(amount);
    let b = b.saturating_add(amount);
    ((r as u32) << 16) | ((g as u32) << 8) | (b as u32)
}

/// Create compact AMS unit (4-slot)
unsafe fn create_ams_unit_compact(
    parent: *mut lvgl_sys::lv_obj_t,
    x: i16, y: i16, w: i16, h: i16,
    name: &str, nozzle: &str, humidity: &str, temp: &str, active: bool,
    slots: &[(&str, u32, &str, &str, bool)],
) {
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
    let name_lbl = lvgl_sys::lv_label_create(unit);
    let name_txt = CString::new(name).unwrap();
    lvgl_sys::lv_label_set_text(name_lbl, name_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(name_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(name_lbl, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(name_lbl, 4, 0);

    // Nozzle badge (L or R)
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

    // Humidity icon + value
    let stats_x = w - 95;
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

    // Temperature icon + value
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

    // Inner housing container with gradient
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

    // Spools row
    let num_slots = slots.len().min(4) as i16;
    let spool_w: i16 = 40;
    let spool_step: i16 = 42;
    let spool_row_w = spool_w + (num_slots - 1) * spool_step;
    let start_x: i16 = (housing_w - spool_row_w) / 2 - 4;

    let mat_y: i16 = 8;
    let spool_y: i16 = 26;
    let badge_y: i16 = 82;
    let pct_y: i16 = 98;

    for (i, (material, color, slot_id, fill_pct, slot_active)) in slots.iter().enumerate() {
        let sx = start_x + (i as i16) * spool_step;
        let visual_spool_x = sx - 4;

        // Material label
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

        // Spool visual
        create_spool_large(housing, sx, spool_y, *color);

        // Slot badge
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

        // Fill percentage
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

/// Create LARGER spool visual (40x52)
unsafe fn create_spool_large(parent: *mut lvgl_sys::lv_obj_t, x: i16, y: i16, color: u32) {
    let zoom_offset_x: i16 = -4;
    let zoom_offset_y: i16 = -5;

    let inner_left: i16 = 11;
    let inner_top: i16 = 6;
    let inner_w: i16 = 20;
    let inner_h: i16 = 40;

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
    lvgl_sys::lv_img_set_zoom(spool_img, 320);

    if color == 0 {
        lvgl_sys::lv_obj_set_style_img_opa(spool_img, 50, 0);

        let inner_x = x + zoom_offset_x + inner_left;
        let inner_y = y + zoom_offset_y + inner_top;

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

        // Add diagonal stripes
        let stripe_color = 0x404040;  // Brighter stripes for visibility
        let stripe_spacing: i16 = 6;
        let num_stripes = (inner_w + inner_h) / stripe_spacing;

        // Global counter for unique stripe indices (max 256 stripes total)
        static mut STRIPE_COUNTER: usize = 0;
        static mut STRIPE_POINTS: [[lvgl_sys::lv_point_t; 2]; 256] = [[lvgl_sys::lv_point_t { x: 0, y: 0 }; 2]; 256];

        for i in 0..num_stripes {
            let offset = i * stripe_spacing - inner_h;
            let line = lvgl_sys::lv_line_create(empty_bg);

            let x1: i16 = offset.max(0);
            let y1: i16 = if offset < 0 { -offset } else { 0 };
            let x2: i16 = (offset + inner_h).min(inner_w);
            let y2: i16 = if offset + inner_h > inner_w { inner_h - (offset + inner_h - inner_w) } else { inner_h };

            let idx = STRIPE_COUNTER % 256;
            STRIPE_COUNTER += 1;
            STRIPE_POINTS[idx][0] = lvgl_sys::lv_point_t { x: x1, y: y1 };
            STRIPE_POINTS[idx][1] = lvgl_sys::lv_point_t { x: x2, y: y2 };

            lvgl_sys::lv_line_set_points(line, STRIPE_POINTS[idx].as_ptr(), 2);
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

    let name_lbl = lvgl_sys::lv_label_create(unit);
    let name_txt = CString::new(name).unwrap();
    lvgl_sys::lv_label_set_text(name_lbl, name_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(name_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(name_lbl, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(name_lbl, 4, 0);

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

    let spool_x = (housing_w - 40) / 2;
    let mat_y: i16 = 24;
    let spool_y: i16 = 42;
    let badge_y: i16 = 98;
    let pct_y: i16 = 114;

    let mat_lbl = lvgl_sys::lv_label_create(housing);
    let mat_txt = CString::new(material).unwrap();
    lvgl_sys::lv_label_set_text(mat_lbl, mat_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(mat_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(mat_lbl, &lvgl_sys::lv_font_montserrat_10, 0);
    lvgl_sys::lv_obj_set_pos(mat_lbl, spool_x + 4, mat_y);

    create_spool_large(housing, spool_x, spool_y, color);

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

    let name_lbl = lvgl_sys::lv_label_create(unit);
    let name_txt = CString::new(name).unwrap();
    lvgl_sys::lv_label_set_text(name_lbl, name_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(name_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(name_lbl, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(name_lbl, 4, 0);

    if !nozzle.is_empty() {
        let name_width: i16 = (name.len() as i16) * 6 + 4;
        let badge_gap: i16 = 4;
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

    let mat_y: i16 = 16;
    let spool_size: i16 = 70;
    let spool_y: i16 = 34;
    let badge_y: i16 = 110;

    let mat_lbl = lvgl_sys::lv_label_create(housing);
    let mat_txt = CString::new(material).unwrap();
    lvgl_sys::lv_label_set_text(mat_lbl, mat_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(mat_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(mat_lbl, &lvgl_sys::lv_font_montserrat_10, 0);
    lvgl_sys::lv_obj_align(mat_lbl, lvgl_sys::LV_ALIGN_TOP_MID as u8, 0, mat_y);

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

/// Create Scan Result screen
unsafe fn create_scan_result_screen() {
    let disp = lvgl_sys::lv_disp_get_default();
    let scr = lvgl_sys::lv_disp_get_scr_act(disp);
    lvgl_sys::lv_obj_set_style_bg_color(scr, lv_color_hex(COLOR_BG), 0);

    // Header bar
    let header = lvgl_sys::lv_obj_create(scr);
    lvgl_sys::lv_obj_set_size(header, 800, 44);
    lvgl_sys::lv_obj_set_pos(header, 0, 0);
    lvgl_sys::lv_obj_clear_flag(header, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_color(header, lv_color_hex(COLOR_BG), 0);
    lvgl_sys::lv_obj_set_style_border_width(header, 0, 0);
    lvgl_sys::lv_obj_set_style_radius(header, 0, 0);
    set_style_pad_all(header, 0);

    // Back button
    let back_lbl = lvgl_sys::lv_label_create(header);
    let back_txt = CString::new("<").unwrap();
    lvgl_sys::lv_label_set_text(back_lbl, back_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(back_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(back_lbl, &lvgl_sys::lv_font_montserrat_20, 0);
    lvgl_sys::lv_obj_set_pos(back_lbl, 16, 10);

    // Title
    let title_lbl = lvgl_sys::lv_label_create(header);
    let title_txt = CString::new("Scan Result").unwrap();
    lvgl_sys::lv_label_set_text(title_lbl, title_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(title_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(title_lbl, &lvgl_sys::lv_font_montserrat_16, 0);
    lvgl_sys::lv_obj_set_pos(title_lbl, 46, 12);

    // Time
    let time_lbl = lvgl_sys::lv_label_create(header);
    let time_txt = CString::new("14:23").unwrap();
    lvgl_sys::lv_label_set_text(time_lbl, time_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(time_lbl, lv_color_hex(COLOR_TEXT_MUTED), 0);
    lvgl_sys::lv_obj_set_style_text_font(time_lbl, &lvgl_sys::lv_font_montserrat_14, 0);
    lvgl_sys::lv_obj_align(time_lbl, lvgl_sys::LV_ALIGN_TOP_RIGHT as u8, -16, 14);

    // Success banner
    let banner = lvgl_sys::lv_obj_create(scr);
    lvgl_sys::lv_obj_set_size(banner, 768, 50);
    lvgl_sys::lv_obj_set_pos(banner, 16, 52);
    lvgl_sys::lv_obj_clear_flag(banner, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_color(banner, lv_color_hex(0x1B5E20), 0);
    lvgl_sys::lv_obj_set_style_radius(banner, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(banner, 0, 0);
    set_style_pad_all(banner, 0);

    // OK circle
    let ok_circle = lvgl_sys::lv_obj_create(banner);
    lvgl_sys::lv_obj_set_size(ok_circle, 28, 28);
    lvgl_sys::lv_obj_set_pos(ok_circle, 12, 11);
    lvgl_sys::lv_obj_clear_flag(ok_circle, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_color(ok_circle, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_radius(ok_circle, 14, 0);
    lvgl_sys::lv_obj_set_style_border_width(ok_circle, 0, 0);
    set_style_pad_all(ok_circle, 0);

    let ok_lbl = lvgl_sys::lv_label_create(ok_circle);
    let ok_txt = CString::new("OK").unwrap();
    lvgl_sys::lv_label_set_text(ok_lbl, ok_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(ok_lbl, lv_color_hex(0x000000), 0);
    lvgl_sys::lv_obj_set_style_text_font(ok_lbl, &lvgl_sys::lv_font_montserrat_10, 0);
    lvgl_sys::lv_obj_align(ok_lbl, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

    // Banner text
    let banner_title = lvgl_sys::lv_label_create(banner);
    let banner_title_txt = CString::new("Spool Detected").unwrap();
    lvgl_sys::lv_label_set_text(banner_title, banner_title_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(banner_title, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(banner_title, &lvgl_sys::lv_font_montserrat_14, 0);
    lvgl_sys::lv_obj_set_pos(banner_title, 52, 8);

    let banner_sub = lvgl_sys::lv_label_create(banner);
    let banner_sub_txt = CString::new("Bambu Lab NFC tag read successfully").unwrap();
    lvgl_sys::lv_label_set_text(banner_sub, banner_sub_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(banner_sub, lv_color_hex(0x81C784), 0);
    lvgl_sys::lv_obj_set_style_text_font(banner_sub, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(banner_sub, 52, 28);

    // Spool info card
    let card = lvgl_sys::lv_obj_create(scr);
    lvgl_sys::lv_obj_set_size(card, 768, 130);
    lvgl_sys::lv_obj_set_pos(card, 16, 110);
    lvgl_sys::lv_obj_clear_flag(card, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_color(card, lv_color_hex(0x2D2D2D), 0);
    lvgl_sys::lv_obj_set_style_radius(card, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(card, 0, 0);
    set_style_pad_all(card, 16);

    // Spool image (yellow spool)
    let spool_container = lvgl_sys::lv_obj_create(card);
    lvgl_sys::lv_obj_set_size(spool_container, 70, 90);
    lvgl_sys::lv_obj_set_pos(spool_container, 0, 4);
    lvgl_sys::lv_obj_clear_flag(spool_container, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_opa(spool_container, 0, 0);
    lvgl_sys::lv_obj_set_style_border_width(spool_container, 0, 0);
    set_style_pad_all(spool_container, 0);

    // Draw spool visual
    create_spool_large(spool_container, 15, 0, 0xF5C518);

    // Weight badge
    let weight_badge = lvgl_sys::lv_obj_create(spool_container);
    lvgl_sys::lv_obj_set_size(weight_badge, 40, 18);
    lvgl_sys::lv_obj_set_pos(weight_badge, 15, 60);
    lvgl_sys::lv_obj_clear_flag(weight_badge, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_color(weight_badge, lv_color_hex(0x424242), 0);
    lvgl_sys::lv_obj_set_style_radius(weight_badge, 9, 0);
    lvgl_sys::lv_obj_set_style_border_width(weight_badge, 0, 0);
    set_style_pad_all(weight_badge, 0);

    let weight_lbl = lvgl_sys::lv_label_create(weight_badge);
    let weight_txt = CString::new("847g").unwrap();
    lvgl_sys::lv_label_set_text(weight_lbl, weight_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(weight_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(weight_lbl, &lvgl_sys::lv_font_montserrat_10, 0);
    lvgl_sys::lv_obj_align(weight_lbl, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

    // Spool info text
    let info_x: i16 = 90;

    let name_lbl = lvgl_sys::lv_label_create(card);
    let name_txt = CString::new("PLA Basic").unwrap();
    lvgl_sys::lv_label_set_text(name_lbl, name_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(name_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(name_lbl, &lvgl_sys::lv_font_montserrat_20, 0);
    lvgl_sys::lv_obj_set_pos(name_lbl, info_x, 4);

    // Color indicator
    let color_dot = lvgl_sys::lv_obj_create(card);
    lvgl_sys::lv_obj_set_size(color_dot, 12, 12);
    lvgl_sys::lv_obj_set_pos(color_dot, info_x, 32);
    lvgl_sys::lv_obj_clear_flag(color_dot, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_color(color_dot, lv_color_hex(0xF5C518), 0);
    lvgl_sys::lv_obj_set_style_radius(color_dot, 6, 0);
    lvgl_sys::lv_obj_set_style_border_width(color_dot, 0, 0);
    set_style_pad_all(color_dot, 0);

    let color_lbl = lvgl_sys::lv_label_create(card);
    let color_txt = CString::new("Yellow").unwrap();
    lvgl_sys::lv_label_set_text(color_lbl, color_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(color_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(color_lbl, &lvgl_sys::lv_font_montserrat_14, 0);
    lvgl_sys::lv_obj_set_pos(color_lbl, info_x + 18, 29);

    let brand_lbl = lvgl_sys::lv_label_create(card);
    let brand_txt = CString::new("Bambu Lab").unwrap();
    lvgl_sys::lv_label_set_text(brand_lbl, brand_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(brand_lbl, lv_color_hex(COLOR_TEXT_MUTED), 0);
    lvgl_sys::lv_obj_set_style_text_font(brand_lbl, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(brand_lbl, info_x, 50);

    // Specs row 1
    let specs_y1: i16 = 70;
    let specs_y2: i16 = 86;

    let nozzle_label = lvgl_sys::lv_label_create(card);
    let nozzle_label_txt = CString::new("Nozzle").unwrap();
    lvgl_sys::lv_label_set_text(nozzle_label, nozzle_label_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(nozzle_label, lv_color_hex(COLOR_TEXT_MUTED), 0);
    lvgl_sys::lv_obj_set_style_text_font(nozzle_label, &lvgl_sys::lv_font_montserrat_10, 0);
    lvgl_sys::lv_obj_set_pos(nozzle_label, info_x, specs_y1);

    let nozzle_val = lvgl_sys::lv_label_create(card);
    let nozzle_val_txt = CString::new("190-220°C").unwrap();
    lvgl_sys::lv_label_set_text(nozzle_val, nozzle_val_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(nozzle_val, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(nozzle_val, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(nozzle_val, info_x, specs_y2);

    let bed_label = lvgl_sys::lv_label_create(card);
    let bed_label_txt = CString::new("Bed").unwrap();
    lvgl_sys::lv_label_set_text(bed_label, bed_label_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(bed_label, lv_color_hex(COLOR_TEXT_MUTED), 0);
    lvgl_sys::lv_obj_set_style_text_font(bed_label, &lvgl_sys::lv_font_montserrat_10, 0);
    lvgl_sys::lv_obj_set_pos(bed_label, info_x + 100, specs_y1);

    let bed_val = lvgl_sys::lv_label_create(card);
    let bed_val_txt = CString::new("45-65°C").unwrap();
    lvgl_sys::lv_label_set_text(bed_val, bed_val_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(bed_val, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(bed_val, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(bed_val, info_x + 100, specs_y2);

    // Specs row 2
    let k_label = lvgl_sys::lv_label_create(card);
    let k_label_txt = CString::new("K Factor").unwrap();
    lvgl_sys::lv_label_set_text(k_label, k_label_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(k_label, lv_color_hex(COLOR_TEXT_MUTED), 0);
    lvgl_sys::lv_obj_set_style_text_font(k_label, &lvgl_sys::lv_font_montserrat_10, 0);
    lvgl_sys::lv_obj_set_pos(k_label, info_x + 200, specs_y1);

    let k_val = lvgl_sys::lv_label_create(card);
    let k_val_txt = CString::new("0.020").unwrap();
    lvgl_sys::lv_label_set_text(k_val, k_val_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(k_val, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(k_val, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(k_val, info_x + 200, specs_y2);

    let dia_label = lvgl_sys::lv_label_create(card);
    let dia_label_txt = CString::new("Diameter").unwrap();
    lvgl_sys::lv_label_set_text(dia_label, dia_label_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(dia_label, lv_color_hex(COLOR_TEXT_MUTED), 0);
    lvgl_sys::lv_obj_set_style_text_font(dia_label, &lvgl_sys::lv_font_montserrat_10, 0);
    lvgl_sys::lv_obj_set_pos(dia_label, info_x + 300, specs_y1);

    let dia_val = lvgl_sys::lv_label_create(card);
    let dia_val_txt = CString::new("1.75mm").unwrap();
    lvgl_sys::lv_label_set_text(dia_val, dia_val_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(dia_val, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(dia_val, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(dia_val, info_x + 300, specs_y2);

    // Assign to AMS Slot section
    let assign_label = lvgl_sys::lv_label_create(scr);
    let assign_txt = CString::new("Assign to AMS Slot").unwrap();
    lvgl_sys::lv_label_set_text(assign_label, assign_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(assign_label, lv_color_hex(COLOR_TEXT_MUTED), 0);
    lvgl_sys::lv_obj_set_style_text_font(assign_label, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(assign_label, 16, 252);

    // AMS slot rows
    let slot_y: i16 = 275;
    let slot_row_h: i16 = 36;
    let slot_gap: i16 = 8;

    // Row 1: A, B, C, D
    create_ams_slot_row(scr, 16, slot_y, "A", &[
        (0xF5C518, true), (0, false), (0x4CAF50, false), (0, false)
    ]);
    create_ams_slot_row(scr, 210, slot_y, "B", &[
        (0xE91E63, false), (0x2196F3, false), (0x4CAF50, false), (0xF5C518, true)
    ]);
    create_ams_slot_row(scr, 404, slot_y, "C", &[
        (0xFFFFFF, false), (0, false), (0, false), (0, false)
    ]);
    create_ams_slot_row(scr, 598, slot_y, "D", &[
        (0x00BCD4, false), (0xFF5722, false), (0, false), (0, false)
    ]);

    // Row 2: HT-A, HT-B, Ext 1, Ext 2
    let row2_y = slot_y + slot_row_h + slot_gap;
    create_single_slot(scr, 16, row2_y, "HT-A", 0x673AB7, false);
    create_single_slot(scr, 110, row2_y, "HT-B", 0xECEFF1, false);
    create_single_slot(scr, 204, row2_y, "Ext 1", 0x607D8B, false);
    create_single_slot(scr, 298, row2_y, "Ext 2", 0xCDDC39, false);

    // Assign & Save button
    let btn = lvgl_sys::lv_btn_create(scr);
    lvgl_sys::lv_obj_set_size(btn, 768, 50);
    lvgl_sys::lv_obj_set_pos(btn, 16, 418);
    lvgl_sys::lv_obj_set_style_bg_color(btn, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_radius(btn, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(btn, 0, 0);

    let btn_lbl = lvgl_sys::lv_label_create(btn);
    let btn_txt = CString::new("Assign & Save").unwrap();
    lvgl_sys::lv_label_set_text(btn_lbl, btn_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(btn_lbl, lv_color_hex(0x000000), 0);
    lvgl_sys::lv_obj_set_style_text_font(btn_lbl, &lvgl_sys::lv_font_montserrat_16, 0);
    lvgl_sys::lv_obj_align(btn_lbl, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
}

/// Helper: Create AMS slot row (label + 4 slots)
unsafe fn create_ams_slot_row(parent: *mut lvgl_sys::lv_obj_t, x: i16, y: i16, label: &str, slots: &[(u32, bool); 4]) {
    let row = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(row, 180, 36);
    lvgl_sys::lv_obj_set_pos(row, x, y);
    lvgl_sys::lv_obj_clear_flag(row, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_opa(row, 0, 0);
    lvgl_sys::lv_obj_set_style_border_width(row, 0, 0);
    set_style_pad_all(row, 0);

    // Label
    let lbl = lvgl_sys::lv_label_create(row);
    let lbl_txt = CString::new(label).unwrap();
    lvgl_sys::lv_label_set_text(lbl, lbl_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(lbl, &lvgl_sys::lv_font_montserrat_14, 0);
    lvgl_sys::lv_obj_set_pos(lbl, 0, 8);

    // 4 slots
    for (i, (color, selected)) in slots.iter().enumerate() {
        let sx = 26 + (i as i16) * 38;
        let slot = lvgl_sys::lv_obj_create(row);
        lvgl_sys::lv_obj_set_size(slot, 32, 32);
        lvgl_sys::lv_obj_set_pos(slot, sx, 2);
        lvgl_sys::lv_obj_clear_flag(slot, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
        lvgl_sys::lv_obj_set_style_radius(slot, 16, 0);
        set_style_pad_all(slot, 0);

        if *color != 0 {
            lvgl_sys::lv_obj_set_style_bg_color(slot, lv_color_hex(*color), 0);
            lvgl_sys::lv_obj_set_style_border_color(slot, lv_color_hex(lighten_color(*color, 30)), 0);
        } else {
            lvgl_sys::lv_obj_set_style_bg_color(slot, lv_color_hex(0x3D3D3D), 0);
            lvgl_sys::lv_obj_set_style_border_color(slot, lv_color_hex(0x505050), 0);
        }

        if *selected {
            lvgl_sys::lv_obj_set_style_border_width(slot, 3, 0);
            lvgl_sys::lv_obj_set_style_border_color(slot, lv_color_hex(COLOR_ACCENT), 0);
        } else {
            lvgl_sys::lv_obj_set_style_border_width(slot, 2, 0);
        }

        // Slot number
        let num_lbl = lvgl_sys::lv_label_create(slot);
        let num_txt = CString::new(format!("{}", i + 1)).unwrap();
        lvgl_sys::lv_label_set_text(num_lbl, num_txt.as_ptr());
        let text_color = if *color != 0 && *color != 0xFFFFFF && *color != 0xECEFF1 && *color != 0xCDDC39 {
            COLOR_WHITE
        } else {
            0x000000
        };
        lvgl_sys::lv_obj_set_style_text_color(num_lbl, lv_color_hex(text_color), 0);
        lvgl_sys::lv_obj_set_style_text_font(num_lbl, &lvgl_sys::lv_font_montserrat_12, 0);
        lvgl_sys::lv_obj_align(num_lbl, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    }
}

/// Helper: Create single slot (HT-A, HT-B, Ext 1, Ext 2)
unsafe fn create_single_slot(parent: *mut lvgl_sys::lv_obj_t, x: i16, y: i16, label: &str, color: u32, selected: bool) {
    let container = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(container, 86, 36);
    lvgl_sys::lv_obj_set_pos(container, x, y);
    lvgl_sys::lv_obj_clear_flag(container, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_opa(container, 0, 0);
    lvgl_sys::lv_obj_set_style_border_width(container, 0, 0);
    set_style_pad_all(container, 0);

    // Label
    let lbl = lvgl_sys::lv_label_create(container);
    let lbl_txt = CString::new(label).unwrap();
    lvgl_sys::lv_label_set_text(lbl, lbl_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(lbl, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(lbl, 0, 8);

    // Slot circle
    let slot = lvgl_sys::lv_obj_create(container);
    lvgl_sys::lv_obj_set_size(slot, 32, 32);
    lvgl_sys::lv_obj_set_pos(slot, 50, 2);
    lvgl_sys::lv_obj_clear_flag(slot, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_radius(slot, 16, 0);
    set_style_pad_all(slot, 0);

    if color != 0 {
        lvgl_sys::lv_obj_set_style_bg_color(slot, lv_color_hex(color), 0);
        lvgl_sys::lv_obj_set_style_border_color(slot, lv_color_hex(lighten_color(color, 30)), 0);
    } else {
        lvgl_sys::lv_obj_set_style_bg_color(slot, lv_color_hex(0x3D3D3D), 0);
        lvgl_sys::lv_obj_set_style_border_color(slot, lv_color_hex(0x505050), 0);
    }

    if selected {
        lvgl_sys::lv_obj_set_style_border_width(slot, 3, 0);
        lvgl_sys::lv_obj_set_style_border_color(slot, lv_color_hex(COLOR_ACCENT), 0);
    } else {
        lvgl_sys::lv_obj_set_style_border_width(slot, 2, 0);
    }
}

/// Create Spool Detail screen
unsafe fn create_spool_detail_screen() {
    let disp = lvgl_sys::lv_disp_get_default();
    let scr = lvgl_sys::lv_disp_get_scr_act(disp);
    lvgl_sys::lv_obj_set_style_bg_color(scr, lv_color_hex(COLOR_BG), 0);

    // Header bar
    let header = lvgl_sys::lv_obj_create(scr);
    lvgl_sys::lv_obj_set_size(header, 800, 44);
    lvgl_sys::lv_obj_set_pos(header, 0, 0);
    lvgl_sys::lv_obj_clear_flag(header, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_color(header, lv_color_hex(COLOR_BG), 0);
    lvgl_sys::lv_obj_set_style_border_width(header, 0, 0);
    lvgl_sys::lv_obj_set_style_radius(header, 0, 0);
    set_style_pad_all(header, 0);

    // Back button
    let back_lbl = lvgl_sys::lv_label_create(header);
    let back_txt = CString::new("<").unwrap();
    lvgl_sys::lv_label_set_text(back_lbl, back_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(back_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(back_lbl, &lvgl_sys::lv_font_montserrat_20, 0);
    lvgl_sys::lv_obj_set_pos(back_lbl, 16, 10);

    // Title
    let title_lbl = lvgl_sys::lv_label_create(header);
    let title_txt = CString::new("Spool Details").unwrap();
    lvgl_sys::lv_label_set_text(title_lbl, title_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(title_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(title_lbl, &lvgl_sys::lv_font_montserrat_16, 0);
    lvgl_sys::lv_obj_set_pos(title_lbl, 46, 12);

    // Time
    let time_lbl = lvgl_sys::lv_label_create(header);
    let time_txt = CString::new("14:23").unwrap();
    lvgl_sys::lv_label_set_text(time_lbl, time_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(time_lbl, lv_color_hex(COLOR_TEXT_MUTED), 0);
    lvgl_sys::lv_obj_set_style_text_font(time_lbl, &lvgl_sys::lv_font_montserrat_14, 0);
    lvgl_sys::lv_obj_align(time_lbl, lvgl_sys::LV_ALIGN_TOP_RIGHT as u8, -16, 14);

    // Main spool info card
    let card = lvgl_sys::lv_obj_create(scr);
    lvgl_sys::lv_obj_set_size(card, 768, 120);
    lvgl_sys::lv_obj_set_pos(card, 16, 52);
    lvgl_sys::lv_obj_clear_flag(card, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_color(card, lv_color_hex(0x2D2D2D), 0);
    lvgl_sys::lv_obj_set_style_radius(card, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(card, 0, 0);
    set_style_pad_all(card, 16);

    // Spool image container
    let spool_container = lvgl_sys::lv_obj_create(card);
    lvgl_sys::lv_obj_set_size(spool_container, 70, 90);
    lvgl_sys::lv_obj_set_pos(spool_container, 0, 0);
    lvgl_sys::lv_obj_clear_flag(spool_container, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_opa(spool_container, 0, 0);
    lvgl_sys::lv_obj_set_style_border_width(spool_container, 0, 0);
    set_style_pad_all(spool_container, 0);

    create_spool_large(spool_container, 15, 0, 0xF5C518);

    // A1 badge
    let slot_badge = lvgl_sys::lv_obj_create(spool_container);
    lvgl_sys::lv_obj_set_size(slot_badge, 28, 18);
    lvgl_sys::lv_obj_set_pos(slot_badge, 20, 60);
    lvgl_sys::lv_obj_clear_flag(slot_badge, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_color(slot_badge, lv_color_hex(0x424242), 0);
    lvgl_sys::lv_obj_set_style_radius(slot_badge, 9, 0);
    lvgl_sys::lv_obj_set_style_border_width(slot_badge, 0, 0);
    set_style_pad_all(slot_badge, 0);

    let slot_lbl = lvgl_sys::lv_label_create(slot_badge);
    let slot_txt = CString::new("A1").unwrap();
    lvgl_sys::lv_label_set_text(slot_lbl, slot_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(slot_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(slot_lbl, &lvgl_sys::lv_font_montserrat_10, 0);
    lvgl_sys::lv_obj_align(slot_lbl, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

    // Spool info
    let info_x: i16 = 90;

    let name_lbl = lvgl_sys::lv_label_create(card);
    let name_txt = CString::new("PLA Basic").unwrap();
    lvgl_sys::lv_label_set_text(name_lbl, name_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(name_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(name_lbl, &lvgl_sys::lv_font_montserrat_20, 0);
    lvgl_sys::lv_obj_set_pos(name_lbl, info_x, 0);

    // Color indicator
    let color_dot = lvgl_sys::lv_obj_create(card);
    lvgl_sys::lv_obj_set_size(color_dot, 12, 12);
    lvgl_sys::lv_obj_set_pos(color_dot, info_x, 28);
    lvgl_sys::lv_obj_clear_flag(color_dot, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_color(color_dot, lv_color_hex(0xF5C518), 0);
    lvgl_sys::lv_obj_set_style_radius(color_dot, 6, 0);
    lvgl_sys::lv_obj_set_style_border_width(color_dot, 0, 0);
    set_style_pad_all(color_dot, 0);

    let color_lbl = lvgl_sys::lv_label_create(card);
    let color_txt = CString::new("Yellow").unwrap();
    lvgl_sys::lv_label_set_text(color_lbl, color_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(color_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(color_lbl, &lvgl_sys::lv_font_montserrat_14, 0);
    lvgl_sys::lv_obj_set_pos(color_lbl, info_x + 18, 25);

    let brand_lbl = lvgl_sys::lv_label_create(card);
    let brand_txt = CString::new("Bambu Lab - 1.75mm").unwrap();
    lvgl_sys::lv_label_set_text(brand_lbl, brand_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(brand_lbl, lv_color_hex(COLOR_TEXT_MUTED), 0);
    lvgl_sys::lv_obj_set_style_text_font(brand_lbl, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(brand_lbl, info_x, 46);

    // Weight display
    let weight_lbl = lvgl_sys::lv_label_create(card);
    let weight_txt = CString::new("847").unwrap();
    lvgl_sys::lv_label_set_text(weight_lbl, weight_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(weight_lbl, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_text_font(weight_lbl, &lvgl_sys::lv_font_montserrat_28, 0);
    lvgl_sys::lv_obj_set_pos(weight_lbl, info_x, 62);

    let weight_unit = lvgl_sys::lv_label_create(card);
    let weight_unit_txt = CString::new("g (85%)").unwrap();
    lvgl_sys::lv_label_set_text(weight_unit, weight_unit_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(weight_unit, lv_color_hex(COLOR_TEXT_MUTED), 0);
    lvgl_sys::lv_obj_set_style_text_font(weight_unit, &lvgl_sys::lv_font_montserrat_14, 0);
    lvgl_sys::lv_obj_set_pos(weight_unit, info_x + 60, 72);

    // Print Settings section
    let section1_y: i16 = 180;
    let section1_lbl = lvgl_sys::lv_label_create(scr);
    let section1_txt = CString::new("Print Settings").unwrap();
    lvgl_sys::lv_label_set_text(section1_lbl, section1_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(section1_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(section1_lbl, &lvgl_sys::lv_font_montserrat_14, 0);
    lvgl_sys::lv_obj_set_pos(section1_lbl, 16, section1_y);

    let settings_card = lvgl_sys::lv_obj_create(scr);
    lvgl_sys::lv_obj_set_size(settings_card, 768, 60);
    lvgl_sys::lv_obj_set_pos(settings_card, 16, section1_y + 24);
    lvgl_sys::lv_obj_clear_flag(settings_card, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_color(settings_card, lv_color_hex(0x2D2D2D), 0);
    lvgl_sys::lv_obj_set_style_radius(settings_card, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(settings_card, 0, 0);
    set_style_pad_all(settings_card, 12);

    // Settings grid
    create_setting_item(settings_card, 0, "Nozzle", "190-220°C");
    create_setting_item(settings_card, 170, "Bed", "45-65°C");
    create_setting_item(settings_card, 340, "K Factor", "0.020");
    create_setting_item(settings_card, 510, "Max Speed", "500mm/s");

    // Spool Information section
    let section2_y: i16 = 275;
    let section2_lbl = lvgl_sys::lv_label_create(scr);
    let section2_txt = CString::new("Spool Information").unwrap();
    lvgl_sys::lv_label_set_text(section2_lbl, section2_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(section2_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(section2_lbl, &lvgl_sys::lv_font_montserrat_14, 0);
    lvgl_sys::lv_obj_set_pos(section2_lbl, 16, section2_y);

    let info_card = lvgl_sys::lv_obj_create(scr);
    lvgl_sys::lv_obj_set_size(info_card, 768, 80);
    lvgl_sys::lv_obj_set_pos(info_card, 16, section2_y + 24);
    lvgl_sys::lv_obj_clear_flag(info_card, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_color(info_card, lv_color_hex(0x2D2D2D), 0);
    lvgl_sys::lv_obj_set_style_radius(info_card, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(info_card, 0, 0);
    set_style_pad_all(info_card, 12);

    // Info grid - row 1
    create_setting_item(info_card, 0, "Tag ID", "A4B7C912");
    create_setting_item(info_card, 170, "Initial Weight", "1000g");
    create_setting_item(info_card, 340, "Used", "153g");
    create_setting_item(info_card, 510, "Last Weighed", "2 min ago");

    // Info grid - row 2
    create_setting_item_row2(info_card, 0, "Added", "Dec 10, 2024");
    create_setting_item_row2(info_card, 170, "Uses", "12 prints");

    // Bottom buttons
    let btn_y: i16 = 410;
    let btn_h: i16 = 44;

    // Assign Slot button (green)
    let assign_btn = lvgl_sys::lv_btn_create(scr);
    lvgl_sys::lv_obj_set_size(assign_btn, 160, btn_h);
    lvgl_sys::lv_obj_set_pos(assign_btn, 180, btn_y);
    lvgl_sys::lv_obj_set_style_bg_color(assign_btn, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_radius(assign_btn, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(assign_btn, 0, 0);

    let assign_lbl = lvgl_sys::lv_label_create(assign_btn);
    let assign_txt = CString::new("Assign Slot").unwrap();
    lvgl_sys::lv_label_set_text(assign_lbl, assign_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(assign_lbl, lv_color_hex(0x000000), 0);
    lvgl_sys::lv_obj_set_style_text_font(assign_lbl, &lvgl_sys::lv_font_montserrat_14, 0);
    lvgl_sys::lv_obj_align(assign_lbl, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

    // Edit Info button (gray)
    let edit_btn = lvgl_sys::lv_btn_create(scr);
    lvgl_sys::lv_obj_set_size(edit_btn, 130, btn_h);
    lvgl_sys::lv_obj_set_pos(edit_btn, 350, btn_y);
    lvgl_sys::lv_obj_set_style_bg_color(edit_btn, lv_color_hex(0x424242), 0);
    lvgl_sys::lv_obj_set_style_radius(edit_btn, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(edit_btn, 0, 0);

    let edit_lbl = lvgl_sys::lv_label_create(edit_btn);
    let edit_txt = CString::new("Edit Info").unwrap();
    lvgl_sys::lv_label_set_text(edit_lbl, edit_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(edit_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(edit_lbl, &lvgl_sys::lv_font_montserrat_14, 0);
    lvgl_sys::lv_obj_align(edit_lbl, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);

    // Remove button (red outline)
    let remove_btn = lvgl_sys::lv_btn_create(scr);
    lvgl_sys::lv_obj_set_size(remove_btn, 130, btn_h);
    lvgl_sys::lv_obj_set_pos(remove_btn, 490, btn_y);
    lvgl_sys::lv_obj_set_style_bg_opa(remove_btn, 0, 0);
    lvgl_sys::lv_obj_set_style_radius(remove_btn, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(remove_btn, 2, 0);
    lvgl_sys::lv_obj_set_style_border_color(remove_btn, lv_color_hex(0xE57373), 0);

    let remove_lbl = lvgl_sys::lv_label_create(remove_btn);
    let remove_txt = CString::new("Remove").unwrap();
    lvgl_sys::lv_label_set_text(remove_lbl, remove_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(remove_lbl, lv_color_hex(0xE57373), 0);
    lvgl_sys::lv_obj_set_style_text_font(remove_lbl, &lvgl_sys::lv_font_montserrat_14, 0);
    lvgl_sys::lv_obj_align(remove_lbl, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
}

/// Helper: Create setting item (label + value)
unsafe fn create_setting_item(parent: *mut lvgl_sys::lv_obj_t, x: i16, label: &str, value: &str) {
    let lbl = lvgl_sys::lv_label_create(parent);
    let lbl_txt = CString::new(label).unwrap();
    lvgl_sys::lv_label_set_text(lbl, lbl_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(lbl, lv_color_hex(COLOR_TEXT_MUTED), 0);
    lvgl_sys::lv_obj_set_style_text_font(lbl, &lvgl_sys::lv_font_montserrat_10, 0);
    lvgl_sys::lv_obj_set_pos(lbl, x, 0);

    let val = lvgl_sys::lv_label_create(parent);
    let val_txt = CString::new(value).unwrap();
    lvgl_sys::lv_label_set_text(val, val_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(val, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(val, &lvgl_sys::lv_font_montserrat_14, 0);
    lvgl_sys::lv_obj_set_pos(val, x, 14);
}

/// Helper: Create setting item row 2
unsafe fn create_setting_item_row2(parent: *mut lvgl_sys::lv_obj_t, x: i16, label: &str, value: &str) {
    let lbl = lvgl_sys::lv_label_create(parent);
    let lbl_txt = CString::new(label).unwrap();
    lvgl_sys::lv_label_set_text(lbl, lbl_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(lbl, lv_color_hex(COLOR_TEXT_MUTED), 0);
    lvgl_sys::lv_obj_set_style_text_font(lbl, &lvgl_sys::lv_font_montserrat_10, 0);
    lvgl_sys::lv_obj_set_pos(lbl, x, 36);

    let val = lvgl_sys::lv_label_create(parent);
    let val_txt = CString::new(value).unwrap();
    lvgl_sys::lv_label_set_text(val, val_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(val, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(val, &lvgl_sys::lv_font_montserrat_14, 0);
    lvgl_sys::lv_obj_set_pos(val, x, 50);
}

/// Create Catalog screen
unsafe fn create_catalog_screen() {
    let disp = lvgl_sys::lv_disp_get_default();
    let scr = lvgl_sys::lv_disp_get_scr_act(disp);
    lvgl_sys::lv_obj_set_style_bg_color(scr, lv_color_hex(COLOR_BG), 0);

    // Header bar
    let header = lvgl_sys::lv_obj_create(scr);
    lvgl_sys::lv_obj_set_size(header, 800, 44);
    lvgl_sys::lv_obj_set_pos(header, 0, 0);
    lvgl_sys::lv_obj_clear_flag(header, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_color(header, lv_color_hex(COLOR_BG), 0);
    lvgl_sys::lv_obj_set_style_border_width(header, 0, 0);
    lvgl_sys::lv_obj_set_style_radius(header, 0, 0);
    set_style_pad_all(header, 0);

    // Back button
    let back_lbl = lvgl_sys::lv_label_create(header);
    let back_txt = CString::new("<").unwrap();
    lvgl_sys::lv_label_set_text(back_lbl, back_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(back_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(back_lbl, &lvgl_sys::lv_font_montserrat_20, 0);
    lvgl_sys::lv_obj_set_pos(back_lbl, 16, 10);

    // Title
    let title_lbl = lvgl_sys::lv_label_create(header);
    let title_txt = CString::new("Spool Catalog").unwrap();
    lvgl_sys::lv_label_set_text(title_lbl, title_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(title_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(title_lbl, &lvgl_sys::lv_font_montserrat_16, 0);
    lvgl_sys::lv_obj_set_pos(title_lbl, 46, 12);

    // Time
    let time_lbl = lvgl_sys::lv_label_create(header);
    let time_txt = CString::new("14:23").unwrap();
    lvgl_sys::lv_label_set_text(time_lbl, time_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(time_lbl, lv_color_hex(COLOR_TEXT_MUTED), 0);
    lvgl_sys::lv_obj_set_style_text_font(time_lbl, &lvgl_sys::lv_font_montserrat_14, 0);
    lvgl_sys::lv_obj_align(time_lbl, lvgl_sys::LV_ALIGN_TOP_RIGHT as u8, -16, 14);

    // Search bar
    let search_bar = lvgl_sys::lv_obj_create(scr);
    lvgl_sys::lv_obj_set_size(search_bar, 280, 36);
    lvgl_sys::lv_obj_set_pos(search_bar, 16, 52);
    lvgl_sys::lv_obj_clear_flag(search_bar, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_color(search_bar, lv_color_hex(0x2D2D2D), 0);
    lvgl_sys::lv_obj_set_style_radius(search_bar, 18, 0);
    lvgl_sys::lv_obj_set_style_border_width(search_bar, 0, 0);
    set_style_pad_all(search_bar, 0);

    // Search icon (Q)
    let search_icon = lvgl_sys::lv_label_create(search_bar);
    let search_icon_txt = CString::new("Q").unwrap();
    lvgl_sys::lv_label_set_text(search_icon, search_icon_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(search_icon, lv_color_hex(COLOR_TEXT_MUTED), 0);
    lvgl_sys::lv_obj_set_style_text_font(search_icon, &lvgl_sys::lv_font_montserrat_14, 0);
    lvgl_sys::lv_obj_set_pos(search_icon, 14, 9);

    let search_txt_lbl = lvgl_sys::lv_label_create(search_bar);
    let search_txt = CString::new("Search spools...").unwrap();
    lvgl_sys::lv_label_set_text(search_txt_lbl, search_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(search_txt_lbl, lv_color_hex(COLOR_TEXT_MUTED), 0);
    lvgl_sys::lv_obj_set_style_text_font(search_txt_lbl, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(search_txt_lbl, 36, 10);

    // Filter pills
    let pills_x: i16 = 310;
    create_filter_pill(scr, pills_x, 52, "All (24)", true);
    create_filter_pill(scr, pills_x + 80, 52, "In AMS (6)", false);
    create_filter_pill(scr, pills_x + 170, 52, "PLA", false);
    create_filter_pill(scr, pills_x + 220, 52, "PETG", false);

    // Spool grid
    let grid_y: i16 = 100;
    let card_w: i16 = 180;
    let card_h: i16 = 110;
    let gap: i16 = 12;

    // Spool data: (material, color_name, color_hex, weight, pct, slot)
    let spools = [
        ("PLA Basic", "Yellow", 0xF5C518, "847g", "85%", "A1"),
        ("PETG HF", "Black", 0x333333, "620g", "62%", "A2"),
        ("PETG Basic", "Orange", 0xFF9800, "450g", "45%", "A3"),
        ("PLA Matte", "Gray", 0x9E9E9E, "900g", "90%", "A4"),
        ("PLA Silk", "Pink", 0xE91E63, "720g", "72%", "B1"),
        ("PLA Basic", "Blue", 0x2196F3, "550g", "55%", "B2"),
        ("PLA Basic", "Red", 0xF44336, "1000g", "100%", ""),
        ("PETG HF", "Green", 0x4CAF50, "880g", "88%", ""),
        ("PLA Basic", "White", 0xFFFFFF, "750g", "75%", ""),
        ("ABS", "Purple", 0x673AB7, "780g", "78%", ""),
        ("TPU 95A", "Lime", 0xCDDC39, "920g", "92%", ""),
        ("PETG Basic", "Cyan", 0x00BCD4, "650g", "65%", ""),
    ];

    for (i, (material, color_name, color_hex, weight, pct, slot)) in spools.iter().enumerate() {
        let col = (i % 4) as i16;
        let row = (i / 4) as i16;
        let x = 16 + col * (card_w + gap);
        let y = grid_y + row * (card_h + gap);

        create_catalog_card(scr, x, y, card_w, card_h, material, color_name, *color_hex, weight, pct, slot);
    }
}

/// Helper: Create filter pill
unsafe fn create_filter_pill(parent: *mut lvgl_sys::lv_obj_t, x: i16, y: i16, text: &str, active: bool) {
    let pill = lvgl_sys::lv_obj_create(parent);
    let w: i16 = if text.len() > 8 { 75 } else { 50 };
    lvgl_sys::lv_obj_set_size(pill, w, 36);
    lvgl_sys::lv_obj_set_pos(pill, x, y);
    lvgl_sys::lv_obj_clear_flag(pill, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_radius(pill, 18, 0);
    set_style_pad_all(pill, 0);

    if active {
        lvgl_sys::lv_obj_set_style_bg_color(pill, lv_color_hex(COLOR_ACCENT), 0);
        lvgl_sys::lv_obj_set_style_border_width(pill, 0, 0);
    } else {
        lvgl_sys::lv_obj_set_style_bg_color(pill, lv_color_hex(0x2D2D2D), 0);
        lvgl_sys::lv_obj_set_style_border_width(pill, 1, 0);
        lvgl_sys::lv_obj_set_style_border_color(pill, lv_color_hex(0x505050), 0);
    }

    let lbl = lvgl_sys::lv_label_create(pill);
    let lbl_txt = CString::new(text).unwrap();
    lvgl_sys::lv_label_set_text(lbl, lbl_txt.as_ptr());
    let text_color = if active { 0x000000 } else { COLOR_WHITE };
    lvgl_sys::lv_obj_set_style_text_color(lbl, lv_color_hex(text_color), 0);
    lvgl_sys::lv_obj_set_style_text_font(lbl, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_align(lbl, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
}

/// Helper: Create catalog card
unsafe fn create_catalog_card(
    parent: *mut lvgl_sys::lv_obj_t,
    x: i16, y: i16, w: i16, h: i16,
    material: &str, color_name: &str, color_hex: u32, weight: &str, pct: &str, slot: &str,
) {
    let card = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(card, w, h);
    lvgl_sys::lv_obj_set_pos(card, x, y);
    lvgl_sys::lv_obj_clear_flag(card, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_color(card, lv_color_hex(0x2D2D2D), 0);
    lvgl_sys::lv_obj_set_style_radius(card, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(card, 0, 0);
    set_style_pad_all(card, 10);

    // Spool visual (simplified circle)
    let spool_size: i16 = 50;
    let spool = lvgl_sys::lv_obj_create(card);
    lvgl_sys::lv_obj_set_size(spool, spool_size, spool_size);
    lvgl_sys::lv_obj_set_pos(spool, 5, 10);
    lvgl_sys::lv_obj_clear_flag(spool, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_color(spool, lv_color_hex(color_hex), 0);
    lvgl_sys::lv_obj_set_style_radius(spool, spool_size / 2, 0);
    lvgl_sys::lv_obj_set_style_border_color(spool, lv_color_hex(lighten_color(color_hex, 30)), 0);
    lvgl_sys::lv_obj_set_style_border_width(spool, 2, 0);
    set_style_pad_all(spool, 0);

    // Inner circle (spool hole)
    let inner_size: i16 = 16;
    let inner = lvgl_sys::lv_obj_create(spool);
    lvgl_sys::lv_obj_set_size(inner, inner_size, inner_size);
    lvgl_sys::lv_obj_align(inner, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    lvgl_sys::lv_obj_clear_flag(inner, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_color(inner, lv_color_hex(0x2D2D2D), 0);
    lvgl_sys::lv_obj_set_style_radius(inner, inner_size / 2, 0);
    lvgl_sys::lv_obj_set_style_border_color(inner, lv_color_hex(0x505050), 0);
    lvgl_sys::lv_obj_set_style_border_width(inner, 1, 0);
    set_style_pad_all(inner, 0);

    // Slot badge (if assigned)
    if !slot.is_empty() {
        let badge = lvgl_sys::lv_obj_create(card);
        lvgl_sys::lv_obj_set_size(badge, 26, 18);
        lvgl_sys::lv_obj_set_pos(badge, w - 40, 5);
        lvgl_sys::lv_obj_clear_flag(badge, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
        lvgl_sys::lv_obj_set_style_bg_color(badge, lv_color_hex(COLOR_ACCENT), 0);
        lvgl_sys::lv_obj_set_style_radius(badge, 9, 0);
        lvgl_sys::lv_obj_set_style_border_width(badge, 0, 0);
        set_style_pad_all(badge, 0);

        let badge_lbl = lvgl_sys::lv_label_create(badge);
        let badge_txt = CString::new(slot).unwrap();
        lvgl_sys::lv_label_set_text(badge_lbl, badge_txt.as_ptr());
        lvgl_sys::lv_obj_set_style_text_color(badge_lbl, lv_color_hex(0x000000), 0);
        lvgl_sys::lv_obj_set_style_text_font(badge_lbl, &lvgl_sys::lv_font_montserrat_10, 0);
        lvgl_sys::lv_obj_align(badge_lbl, lvgl_sys::LV_ALIGN_CENTER as u8, 0, 0);
    }

    // Material name
    let mat_lbl = lvgl_sys::lv_label_create(card);
    let mat_txt = CString::new(material).unwrap();
    lvgl_sys::lv_label_set_text(mat_lbl, mat_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(mat_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(mat_lbl, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(mat_lbl, 65, 10);

    // Color dot + name
    let dot = lvgl_sys::lv_obj_create(card);
    lvgl_sys::lv_obj_set_size(dot, 10, 10);
    lvgl_sys::lv_obj_set_pos(dot, 65, 30);
    lvgl_sys::lv_obj_clear_flag(dot, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_color(dot, lv_color_hex(color_hex), 0);
    lvgl_sys::lv_obj_set_style_radius(dot, 5, 0);
    lvgl_sys::lv_obj_set_style_border_width(dot, 0, 0);
    set_style_pad_all(dot, 0);

    let color_lbl = lvgl_sys::lv_label_create(card);
    let color_txt = CString::new(color_name).unwrap();
    lvgl_sys::lv_label_set_text(color_lbl, color_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(color_lbl, lv_color_hex(COLOR_TEXT_MUTED), 0);
    lvgl_sys::lv_obj_set_style_text_font(color_lbl, &lvgl_sys::lv_font_montserrat_10, 0);
    lvgl_sys::lv_obj_set_pos(color_lbl, 80, 28);

    // Weight + percentage
    let weight_lbl = lvgl_sys::lv_label_create(card);
    let weight_txt = CString::new(format!("{} ({})", weight, pct)).unwrap();
    lvgl_sys::lv_label_set_text(weight_lbl, weight_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(weight_lbl, lv_color_hex(COLOR_TEXT_MUTED), 0);
    lvgl_sys::lv_obj_set_style_text_font(weight_lbl, &lvgl_sys::lv_font_montserrat_10, 0);
    lvgl_sys::lv_obj_set_pos(weight_lbl, 65, 48);
}

/// Create Settings screen
unsafe fn create_settings_screen() {
    let disp = lvgl_sys::lv_disp_get_default();
    let scr = lvgl_sys::lv_disp_get_scr_act(disp);
    lvgl_sys::lv_obj_set_style_bg_color(scr, lv_color_hex(COLOR_BG), 0);

    // Status bar (same as home screen style)
    let status_bar = lvgl_sys::lv_obj_create(scr);
    lvgl_sys::lv_obj_set_size(status_bar, 800, 44);
    lvgl_sys::lv_obj_set_pos(status_bar, 0, 0);
    lvgl_sys::lv_obj_clear_flag(status_bar, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_color(status_bar, lv_color_hex(COLOR_BG), 0);
    lvgl_sys::lv_obj_set_style_border_width(status_bar, 0, 0);
    lvgl_sys::lv_obj_set_style_radius(status_bar, 0, 0);
    set_style_pad_all(status_bar, 0);

    // SpoolBuddy logo/text
    let logo_lbl = lvgl_sys::lv_label_create(status_bar);
    let logo_txt = CString::new("SpoolBuddy").unwrap();
    lvgl_sys::lv_label_set_text(logo_lbl, logo_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(logo_lbl, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_text_font(logo_lbl, &lvgl_sys::lv_font_montserrat_14, 0);
    lvgl_sys::lv_obj_set_pos(logo_lbl, 16, 14);

    // Printer dropdown
    let printer_btn = lvgl_sys::lv_obj_create(status_bar);
    lvgl_sys::lv_obj_set_size(printer_btn, 150, 30);
    lvgl_sys::lv_obj_align(printer_btn, lvgl_sys::LV_ALIGN_TOP_MID as u8, 0, 7);
    lvgl_sys::lv_obj_clear_flag(printer_btn, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_color(printer_btn, lv_color_hex(0x2D2D2D), 0);
    lvgl_sys::lv_obj_set_style_radius(printer_btn, 15, 0);
    lvgl_sys::lv_obj_set_style_border_width(printer_btn, 0, 0);
    set_style_pad_all(printer_btn, 0);

    let printer_dot = lvgl_sys::lv_obj_create(printer_btn);
    lvgl_sys::lv_obj_set_size(printer_dot, 8, 8);
    lvgl_sys::lv_obj_set_pos(printer_dot, 12, 11);
    lvgl_sys::lv_obj_clear_flag(printer_dot, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_color(printer_dot, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_radius(printer_dot, 4, 0);
    lvgl_sys::lv_obj_set_style_border_width(printer_dot, 0, 0);
    set_style_pad_all(printer_dot, 0);

    let printer_lbl = lvgl_sys::lv_label_create(printer_btn);
    let printer_txt = CString::new("X1C-Studio").unwrap();
    lvgl_sys::lv_label_set_text(printer_lbl, printer_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(printer_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(printer_lbl, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(printer_lbl, 28, 7);

    // Status icons on right
    let time_lbl = lvgl_sys::lv_label_create(status_bar);
    let time_txt = CString::new("14:23").unwrap();
    lvgl_sys::lv_label_set_text(time_lbl, time_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(time_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(time_lbl, &lvgl_sys::lv_font_montserrat_14, 0);
    lvgl_sys::lv_obj_align(time_lbl, lvgl_sys::LV_ALIGN_TOP_RIGHT as u8, -16, 14);

    // Content area
    let content_y: i16 = 52;

    // Network section
    create_section_header(scr, 16, content_y, "Network");

    create_settings_row(scr, 16, content_y + 24, "WiFi", "SpoolBuddy-Net", "", true);
    create_settings_row(scr, 16, content_y + 68, "Backend Server", "192.168.1.100:3000", "", true);

    // Printers section
    let printers_y = content_y + 130;
    create_section_header(scr, 16, printers_y, "Printers");

    create_settings_row(scr, 16, printers_y + 24, "X1C-Studio", "* Connected", "green", true);
    create_settings_row(scr, 16, printers_y + 68, "P1S-Garage", "* Offline", "gray", true);
    create_settings_row(scr, 16, printers_y + 112, "Add Printer", "", "", true);

    // Hardware section
    let hardware_y = printers_y + 174;
    create_section_header(scr, 16, hardware_y, "Hardware");

    create_settings_row(scr, 16, hardware_y + 24, "Scale Calibration", "Last: 2d ago", "", true);
    create_settings_row(scr, 16, hardware_y + 68, "NFC Reader", "* Ready", "green", true);
    create_settings_row(scr, 16, hardware_y + 112, "Display Brightness", "80%", "", true);
}

/// Helper: Create section header
unsafe fn create_section_header(parent: *mut lvgl_sys::lv_obj_t, x: i16, y: i16, text: &str) {
    let lbl = lvgl_sys::lv_label_create(parent);
    let lbl_txt = CString::new(text).unwrap();
    lvgl_sys::lv_label_set_text(lbl, lbl_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(lbl, lv_color_hex(COLOR_TEXT_MUTED), 0);
    lvgl_sys::lv_obj_set_style_text_font(lbl, &lvgl_sys::lv_font_montserrat_12, 0);
    lvgl_sys::lv_obj_set_pos(lbl, x, y);
}

/// Helper: Create settings row
unsafe fn create_settings_row(parent: *mut lvgl_sys::lv_obj_t, x: i16, y: i16, title: &str, value: &str, status_color: &str, has_arrow: bool) {
    let row = lvgl_sys::lv_obj_create(parent);
    lvgl_sys::lv_obj_set_size(row, 768, 40);
    lvgl_sys::lv_obj_set_pos(row, x, y);
    lvgl_sys::lv_obj_clear_flag(row, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_color(row, lv_color_hex(0x2D2D2D), 0);
    lvgl_sys::lv_obj_set_style_radius(row, 8, 0);
    lvgl_sys::lv_obj_set_style_border_width(row, 0, 0);
    set_style_pad_all(row, 0);

    // Green indicator bar on left
    let indicator = lvgl_sys::lv_obj_create(row);
    lvgl_sys::lv_obj_set_size(indicator, 4, 24);
    lvgl_sys::lv_obj_set_pos(indicator, 0, 8);
    lvgl_sys::lv_obj_clear_flag(indicator, lvgl_sys::LV_OBJ_FLAG_SCROLLABLE);
    lvgl_sys::lv_obj_set_style_bg_color(indicator, lv_color_hex(COLOR_ACCENT), 0);
    lvgl_sys::lv_obj_set_style_radius(indicator, 0, 0);
    lvgl_sys::lv_obj_set_style_border_width(indicator, 0, 0);
    set_style_pad_all(indicator, 0);

    // Title
    let title_lbl = lvgl_sys::lv_label_create(row);
    let title_txt = CString::new(title).unwrap();
    lvgl_sys::lv_label_set_text(title_lbl, title_txt.as_ptr());
    lvgl_sys::lv_obj_set_style_text_color(title_lbl, lv_color_hex(COLOR_WHITE), 0);
    lvgl_sys::lv_obj_set_style_text_font(title_lbl, &lvgl_sys::lv_font_montserrat_14, 0);
    lvgl_sys::lv_obj_set_pos(title_lbl, 16, 11);

    // Value/status
    if !value.is_empty() {
        let value_lbl = lvgl_sys::lv_label_create(row);
        let value_txt = CString::new(value).unwrap();
        lvgl_sys::lv_label_set_text(value_lbl, value_txt.as_ptr());

        let color = match status_color {
            "green" => COLOR_ACCENT,
            "gray" => COLOR_TEXT_MUTED,
            _ => COLOR_TEXT_MUTED,
        };
        lvgl_sys::lv_obj_set_style_text_color(value_lbl, lv_color_hex(color), 0);
        lvgl_sys::lv_obj_set_style_text_font(value_lbl, &lvgl_sys::lv_font_montserrat_12, 0);
        lvgl_sys::lv_obj_align(value_lbl, lvgl_sys::LV_ALIGN_RIGHT_MID as u8, -30, 0);
    }

    // Arrow
    if has_arrow {
        let arrow_lbl = lvgl_sys::lv_label_create(row);
        let arrow_txt = CString::new(">").unwrap();
        lvgl_sys::lv_label_set_text(arrow_lbl, arrow_txt.as_ptr());
        lvgl_sys::lv_obj_set_style_text_color(arrow_lbl, lv_color_hex(COLOR_TEXT_MUTED), 0);
        lvgl_sys::lv_obj_set_style_text_font(arrow_lbl, &lvgl_sys::lv_font_montserrat_14, 0);
        lvgl_sys::lv_obj_align(arrow_lbl, lvgl_sys::LV_ALIGN_RIGHT_MID as u8, -10, 0);
    }
}

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
