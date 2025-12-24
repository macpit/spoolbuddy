//! SpoolBuddy LVGL PC Simulator
//!
//! Runs the EEZ Studio designed UI on desktop with SDL2.
//!
//! # Usage
//! ```bash
//! # Interactive mode (requires display)
//! DEP_LV_CONFIG_PATH=$(pwd)/include cargo run --release
//!
//! # Headless mode - renders to BMP file
//! DEP_LV_CONFIG_PATH=$(pwd)/include cargo run --release -- --headless
//! ```

use log::info;
use std::io::Write;
use std::time::Instant;

// Display dimensions (same as firmware)
const WIDTH: i16 = 800;
const HEIGHT: i16 = 480;

// Global framebuffer for headless mode
static mut FRAMEBUFFER: [u16; (800 * 480) as usize] = [0u16; (800 * 480) as usize];

// External C functions from EEZ generated code
extern "C" {
    fn ui_init();
    fn ui_tick();
}

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
    info!("SpoolBuddy LVGL Simulator - HEADLESS MODE (EEZ UI)");
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

        // Initialize EEZ UI
        ui_init();
        info!("EEZ UI initialized");

        // Run a few timer cycles to render
        for _ in 0..10 {
            lvgl_sys::lv_tick_inc(16);
            lvgl_sys::lv_timer_handler();
            ui_tick();
        }

        // Save screenshot
        save_screenshot("home.bmp");
        info!("Screenshot saved to home.bmp");
    }
}

/// Headless flush callback - copies to framebuffer
unsafe extern "C" fn headless_flush_cb(
    disp_drv: *mut lvgl_sys::lv_disp_drv_t,
    area: *const lvgl_sys::lv_area_t,
    color_p: *mut lvgl_sys::lv_color_t,
) {
    let area = &*area;
    let mut src = color_p;

    for y in area.y1..=area.y2 {
        for x in area.x1..=area.x2 {
            let idx = (y as usize) * (WIDTH as usize) + (x as usize);
            if idx < FRAMEBUFFER.len() {
                FRAMEBUFFER[idx] = (*src).full;
            }
            src = src.add(1);
        }
    }

    lvgl_sys::lv_disp_flush_ready(disp_drv);
}

/// Save framebuffer as BMP
fn save_screenshot(filename: &str) {
    let width = WIDTH as u32;
    let height = HEIGHT as u32;

    // BMP header
    let row_size = ((24 * width + 31) / 32) * 4;
    let pixel_data_size = row_size * height;
    let file_size = 54 + pixel_data_size;

    let mut file = std::fs::File::create(filename).expect("Failed to create BMP file");

    // BMP Header (14 bytes)
    file.write_all(b"BM").unwrap();
    file.write_all(&(file_size as u32).to_le_bytes()).unwrap();
    file.write_all(&[0u8; 4]).unwrap(); // Reserved
    file.write_all(&54u32.to_le_bytes()).unwrap(); // Pixel data offset

    // DIB Header (40 bytes)
    file.write_all(&40u32.to_le_bytes()).unwrap(); // Header size
    file.write_all(&(width as i32).to_le_bytes()).unwrap();
    file.write_all(&(-(height as i32)).to_le_bytes()).unwrap(); // Negative = top-down
    file.write_all(&1u16.to_le_bytes()).unwrap(); // Planes
    file.write_all(&24u16.to_le_bytes()).unwrap(); // Bits per pixel
    file.write_all(&0u32.to_le_bytes()).unwrap(); // No compression
    file.write_all(&pixel_data_size.to_le_bytes()).unwrap();
    file.write_all(&2835u32.to_le_bytes()).unwrap(); // X pixels per meter
    file.write_all(&2835u32.to_le_bytes()).unwrap(); // Y pixels per meter
    file.write_all(&0u32.to_le_bytes()).unwrap(); // Colors in palette
    file.write_all(&0u32.to_le_bytes()).unwrap(); // Important colors

    // Pixel data (RGB565 -> BGR24)
    unsafe {
        for y in 0..height {
            for x in 0..width {
                let idx = (y as usize) * (width as usize) + (x as usize);
                let rgb565 = FRAMEBUFFER[idx];

                // Extract RGB565 components
                let r5 = ((rgb565 >> 11) & 0x1F) as u8;
                let g6 = ((rgb565 >> 5) & 0x3F) as u8;
                let b5 = (rgb565 & 0x1F) as u8;

                // Convert to 8-bit
                let r = (r5 << 3) | (r5 >> 2);
                let g = (g6 << 2) | (g6 >> 4);
                let b = (b5 << 3) | (b5 >> 2);

                // BMP uses BGR order
                file.write_all(&[b, g, r]).unwrap();
            }
            // Row padding to 4-byte boundary
            let padding = (4 - ((width * 3) % 4)) % 4;
            for _ in 0..padding {
                file.write_all(&[0u8]).unwrap();
            }
        }
    }

    info!("Saved {}x{} BMP screenshot", width, height);
}

// Statics for SDL interactive mode
static mut SDL_DISP_BUF: lvgl_sys::lv_disp_draw_buf_t = unsafe { std::mem::zeroed() };
static mut SDL_DISP_DRV: lvgl_sys::lv_disp_drv_t = unsafe { std::mem::zeroed() };
static mut SDL_INDEV_DRV: lvgl_sys::lv_indev_drv_t = unsafe { std::mem::zeroed() };

fn run_interactive() {
    info!("SpoolBuddy LVGL Simulator - INTERACTIVE MODE (EEZ UI)");
    info!("Display: {}x{}", WIDTH, HEIGHT);

    unsafe {
        // Initialize LVGL
        lvgl_sys::lv_init();
        info!("LVGL initialized");

        // Initialize SDL display driver
        lvgl_sys::sdl_init();
        info!("SDL initialized");

        // Setup display buffer and driver
        lvgl_sys::lv_disp_draw_buf_init(
            &raw mut SDL_DISP_BUF,
            lvgl_sys::sdl_get_buf1(),
            lvgl_sys::sdl_get_buf2(),
            (WIDTH as u32) * (HEIGHT as u32),
        );

        lvgl_sys::lv_disp_drv_init(&raw mut SDL_DISP_DRV);
        SDL_DISP_DRV.hor_res = WIDTH;
        SDL_DISP_DRV.ver_res = HEIGHT;
        SDL_DISP_DRV.flush_cb = Some(lvgl_sys::sdl_display_flush);
        SDL_DISP_DRV.draw_buf = &raw mut SDL_DISP_BUF;
        let _disp = lvgl_sys::lv_disp_drv_register(&raw mut SDL_DISP_DRV);
        info!("Display driver registered");

        // Setup mouse input
        lvgl_sys::lv_indev_drv_init(&raw mut SDL_INDEV_DRV);
        SDL_INDEV_DRV.type_ = lvgl_sys::lv_indev_type_t_LV_INDEV_TYPE_POINTER;
        SDL_INDEV_DRV.read_cb = Some(lvgl_sys::sdl_mouse_read);
        let _indev = lvgl_sys::lv_indev_drv_register(&raw mut SDL_INDEV_DRV);
        info!("Mouse input registered");

        // Initialize EEZ UI
        ui_init();
        info!("EEZ UI initialized");

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
            ui_tick();
            std::thread::sleep(std::time::Duration::from_millis(5));
        }
    }
}
