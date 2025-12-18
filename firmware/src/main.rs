//! SpoolBuddy Firmware - Home Screen UI
//! ESP32-S3 with Waveshare ESP32-S3-Touch-LCD-4.3 (800x480 RGB565)

#![no_std]
#![no_main]

extern crate alloc;

use alloc::boxed::Box;
use alloc::vec::Vec;
use embedded_graphics::pixelcolor::Rgb565;
use embedded_graphics::prelude::*;
use embedded_graphics::primitives::{PrimitiveStyle, Rectangle, RoundedRectangle};
use esp_alloc as _;
use esp_backtrace as _;
use esp_hal::clock::CpuClock;
use esp_hal::dma::DmaDescriptor;
use esp_hal::dma_loop_buffer;
use esp_hal::gpio::Level;
use esp_hal::i2c::master::{Config as I2cConfig, I2c};
use esp_hal::lcd_cam::LcdCam;
use esp_hal::lcd_cam::lcd::dpi::{Config as DpiConfig, Dpi, Format, FrameTiming};
use esp_hal::lcd_cam::lcd::{ClockMode, Phase, Polarity};
use esp_hal::main;
use esp_hal::time::{Duration, Instant, Rate};

esp_bootloader_esp_idf::esp_app_desc!();

// ESP32-S3 PSRAM address mapping
// Cached data access: 0x3D000000 - 0x3DFFFFFF
// Uncached access:    0x3C000000 - 0x3CFFFFFF
// DMA needs uncached access to see current data
const PSRAM_CACHED_BASE: usize = 0x3D000000;
const PSRAM_UNCACHED_BASE: usize = 0x3C000000;

// Display dimensions
const WIDTH: usize = 800;
const HEIGHT: usize = 480;
const FB_SIZE: usize = WIDTH * HEIGHT * 2; // RGB565 = 2 bytes per pixel

// Number of DMA descriptors needed (4095 bytes per descriptor max)
const DMA_DESC_COUNT: usize = (FB_SIZE + 4094) / 4095; // ~188 descriptors

// UI Colors (from mockup CSS)
const COLOR_BG: Rgb565 = Rgb565::new(0x03, 0x06, 0x03);           // #1A1A1A
const COLOR_STATUS_BAR: Rgb565 = Rgb565::new(0x02, 0x04, 0x02);   // #101010
const COLOR_CARD: Rgb565 = Rgb565::new(0x05, 0x0B, 0x05);         // #2D2D2D
const COLOR_ACCENT: Rgb565 = Rgb565::new(0x00, 0x3F, 0x00);       // #00FF00
const COLOR_BORDER: Rgb565 = Rgb565::new(0x07, 0x0F, 0x07);       // #3D3D3D

// CH422G I2C IO Expander
const CH422G_REG_MODE: u8 = 0x24;
const CH422G_REG_OUT_IO: u8 = 0x38;
const CH422G_MODE_OUTPUT: u8 = 0x01;
const CH422G_PIN_TP_RST: u8 = 1;
const CH422G_PIN_LCD_BL: u8 = 2;
const CH422G_PIN_LCD_RST: u8 = 3;

fn delay_ms(ms: u64) {
    let start = Instant::now();
    while start.elapsed() < Duration::from_millis(ms) {}
}

/// Simple framebuffer that implements DrawTarget
/// Uses UNCACHED PSRAM access so DMA can see writes immediately
struct Framebuffer {
    data: &'static mut [u8],
    cached_addr: usize,
}

impl Framebuffer {
    fn new() -> Self {
        // Allocate in PSRAM (returns cached address 0x3D...)
        let mut data: Vec<u8> = Vec::with_capacity(FB_SIZE);
        data.resize(FB_SIZE, 0);
        let data = Box::leak(data.into_boxed_slice());
        let cached_addr = data.as_ptr() as usize;

        // Convert to uncached address (0x3C...) for all access
        // This ensures DMA sees our writes without cache flush
        let uncached_ptr = if cached_addr >= PSRAM_CACHED_BASE && cached_addr < (PSRAM_CACHED_BASE + 0x01000000) {
            cached_addr - PSRAM_CACHED_BASE + PSRAM_UNCACHED_BASE
        } else {
            cached_addr
        };

        let data = unsafe {
            core::slice::from_raw_parts_mut(uncached_ptr as *mut u8, FB_SIZE)
        };

        Self { data, cached_addr }
    }

    fn clear(&mut self, color: Rgb565) {
        let raw = color.into_storage();
        for chunk in self.data.chunks_mut(2) {
            chunk[0] = (raw & 0xFF) as u8;
            chunk[1] = (raw >> 8) as u8;
        }
    }

    fn as_dma_slice(&self) -> &'static mut [u8] {
        // Already using uncached address, safe for DMA
        unsafe { core::slice::from_raw_parts_mut(self.data.as_ptr() as *mut u8, self.data.len()) }
    }

    fn cached_address(&self) -> usize {
        self.cached_addr
    }

    fn uncached_address(&self) -> usize {
        self.data.as_ptr() as usize
    }
}

impl DrawTarget for Framebuffer {
    type Color = Rgb565;
    type Error = core::convert::Infallible;

    fn draw_iter<I>(&mut self, pixels: I) -> Result<(), Self::Error>
    where
        I: IntoIterator<Item = Pixel<Self::Color>>,
    {
        for Pixel(point, color) in pixels {
            if point.x >= 0 && point.x < WIDTH as i32 && point.y >= 0 && point.y < HEIGHT as i32 {
                let idx = ((point.y as usize) * WIDTH + (point.x as usize)) * 2;
                let raw = color.into_storage();
                self.data[idx] = (raw & 0xFF) as u8;
                self.data[idx + 1] = (raw >> 8) as u8;
            }
        }
        Ok(())
    }
}

impl OriginDimensions for Framebuffer {
    fn size(&self) -> Size {
        Size::new(WIDTH as u32, HEIGHT as u32)
    }
}

/// Draw the home screen UI
fn draw_home_screen(fb: &mut Framebuffer) {
    // Clear to background color
    fb.clear(COLOR_BG);

    // Status bar (top 44px)
    Rectangle::new(Point::new(0, 0), Size::new(800, 44))
        .into_styled(PrimitiveStyle::with_fill(COLOR_STATUS_BAR))
        .draw(fb)
        .unwrap();

    // Bottom bar (bottom 44px)
    Rectangle::new(Point::new(0, 436), Size::new(800, 44))
        .into_styled(PrimitiveStyle::with_fill(COLOR_STATUS_BAR))
        .draw(fb)
        .unwrap();

    // Main content area
    let content_y = 44 + 12;
    let content_h = 480 - 44 - 44 - 24; // ~380px

    // AMS Panel card (left side)
    RoundedRectangle::with_equal_corners(
        Rectangle::new(Point::new(12, content_y), Size::new(596, content_h as u32)),
        Size::new(12, 12),
    )
    .into_styled(PrimitiveStyle::with_fill(COLOR_CARD))
    .draw(fb)
    .unwrap();

    // Right side: Action buttons in 2x2 grid
    let btn_x = 620;
    let btn_size = 82;
    let btn_gap = 8;

    for row in 0..2 {
        for col in 0..2 {
            let x = btn_x + col * (btn_size + btn_gap);
            let y = content_y + row * (btn_size + btn_gap);

            RoundedRectangle::with_equal_corners(
                Rectangle::new(Point::new(x, y), Size::new(btn_size as u32, btn_size as u32)),
                Size::new(10, 10),
            )
            .into_styled(PrimitiveStyle::with_fill(COLOR_CARD))
            .draw(fb)
            .unwrap();
        }
    }

    // Draw AMS units inside the AMS panel
    let ams_y = content_y + 30;
    let ams_card_w = 185;
    let ams_card_h = 150;
    let ams_gap = 8;

    for i in 0..3 {
        let x = 20 + i * (ams_card_w + ams_gap);

        // AMS unit card border
        RoundedRectangle::with_equal_corners(
            Rectangle::new(Point::new(x, ams_y), Size::new(ams_card_w as u32, ams_card_h as u32)),
            Size::new(8, 8),
        )
        .into_styled(PrimitiveStyle::with_stroke(COLOR_BORDER, 2))
        .draw(fb)
        .unwrap();

        // Draw 4 spool slots inside each AMS
        for slot in 0..4 {
            let slot_x = x + 10 + slot * 42;
            let slot_y = ams_y + 40;

            // Spool colors
            let spool_colors = [
                Rgb565::new(31, 0, 0),   // Red
                Rgb565::new(0, 63, 0),   // Green
                Rgb565::new(0, 0, 31),   // Blue
                Rgb565::new(31, 63, 0),  // Yellow
            ];

            Rectangle::new(
                Point::new(slot_x, slot_y),
                Size::new(36, 80),
            )
            .into_styled(PrimitiveStyle::with_fill(spool_colors[(i as usize + slot as usize) % 4]))
            .draw(fb)
            .unwrap();
        }
    }

    // Green accent line (status indicator)
    Rectangle::new(Point::new(12, content_y), Size::new(4, content_h as u32))
        .into_styled(PrimitiveStyle::with_fill(COLOR_ACCENT))
        .draw(fb)
        .unwrap();
}

// Static storage for DMA descriptors (must be in internal RAM)
static mut DMA_DESCRIPTORS: [DmaDescriptor; DMA_DESC_COUNT] = [DmaDescriptor::EMPTY; DMA_DESC_COUNT];

#[main]
fn main() -> ! {
    // Initialize ESP-HAL first
    let config = esp_hal::Config::default().with_cpu_clock(CpuClock::max());
    let peripherals = esp_hal::init(config);

    // Initialize PSRAM (8MB octal) and add to heap
    esp_alloc::psram_allocator!(&peripherals.PSRAM, esp_hal::psram);

    // Small delay for USB
    for _ in 0..1_000_000 {
        core::hint::spin_loop();
    }

    esp_println::println!("SpoolBuddy Firmware v0.1 - Home Screen");
    esp_println::println!("Initializing...");

    // Initialize I2C for CH422G
    let i2c_config = I2cConfig::default().with_frequency(Rate::from_khz(100));
    let mut i2c = I2c::new(peripherals.I2C0, i2c_config)
        .unwrap()
        .with_sda(peripherals.GPIO8)
        .with_scl(peripherals.GPIO9);

    // Configure CH422G and reset LCD
    let _ = i2c.write(CH422G_REG_MODE, &[CH422G_MODE_OUTPUT]);
    let reset_low = (1 << CH422G_PIN_TP_RST) | (0 << CH422G_PIN_LCD_RST);
    let _ = i2c.write(CH422G_REG_OUT_IO, &[reset_low]);
    delay_ms(20);
    let reset_high = (1 << CH422G_PIN_TP_RST) | (1 << CH422G_PIN_LCD_RST);
    let _ = i2c.write(CH422G_REG_OUT_IO, &[reset_high]);
    delay_ms(50);
    let backlight_on = reset_high | (1 << CH422G_PIN_LCD_BL);
    let _ = i2c.write(CH422G_REG_OUT_IO, &[backlight_on]);
    esp_println::println!("  LCD initialized, backlight ON");

    // Create framebuffer in PSRAM (using uncached access for DMA compatibility)
    esp_println::println!("Creating framebuffer ({} bytes in PSRAM)...", FB_SIZE);
    let mut fb = Framebuffer::new();
    esp_println::println!("  Cached addr:   0x{:08X}", fb.cached_address());
    esp_println::println!("  Uncached addr: 0x{:08X} (used for all access)", fb.uncached_address());

    // Draw home screen (writes go directly to PSRAM, bypassing cache)
    esp_println::println!("Drawing home screen...");
    draw_home_screen(&mut fb);
    esp_println::println!("  Home screen rendered");

    // Debug: print first few pixels to verify rendering
    let data = fb.as_dma_slice();
    esp_println::println!("  First 8 bytes: {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X}",
        data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);

    // Setup DPI
    let lcd_cam = LcdCam::new(peripherals.LCD_CAM);

    let dpi_config = DpiConfig::default()
        .with_frequency(Rate::from_mhz(16))
        .with_clock_mode(ClockMode {
            polarity: Polarity::IdleLow,
            phase: Phase::ShiftLow,
        })
        .with_format(Format {
            enable_2byte_mode: true,
            ..Default::default()
        })
        .with_timing(FrameTiming {
            horizontal_active_width: 800,
            horizontal_total_width: 928,
            horizontal_blank_front_porch: 40,
            vertical_active_height: 480,
            vertical_total_height: 528,
            vertical_blank_front_porch: 13,
            hsync_width: 48,
            vsync_width: 3,
            hsync_position: 0,
        })
        .with_vsync_idle_level(Level::High)
        .with_hsync_idle_level(Level::High)
        .with_de_idle_level(Level::Low)
        .with_disable_black_region(false);

    let dpi_result = Dpi::new(lcd_cam.lcd, peripherals.DMA_CH0, dpi_config)
        .map(|dpi| dpi
            .with_vsync(peripherals.GPIO3)
            .with_hsync(peripherals.GPIO46)
            .with_de(peripherals.GPIO5)
            .with_pclk(peripherals.GPIO7)
            .with_data0(peripherals.GPIO14)
            .with_data1(peripherals.GPIO38)
            .with_data2(peripherals.GPIO18)
            .with_data3(peripherals.GPIO17)
            .with_data4(peripherals.GPIO10)
            .with_data5(peripherals.GPIO39)
            .with_data6(peripherals.GPIO0)
            .with_data7(peripherals.GPIO45)
            .with_data8(peripherals.GPIO48)
            .with_data9(peripherals.GPIO47)
            .with_data10(peripherals.GPIO21)
            .with_data11(peripherals.GPIO1)
            .with_data12(peripherals.GPIO2)
            .with_data13(peripherals.GPIO42)
            .with_data14(peripherals.GPIO41)
            .with_data15(peripherals.GPIO40)
        );

    match dpi_result {
        Ok(dpi) => {
            esp_println::println!("  DPI ready");

            // Use dma_loop_buffer which is proven to work
            // Copy 2 lines from our PSRAM framebuffer into the loop buffer
            // This will show the top of the UI repeated across the screen
            esp_println::println!("Using dma_loop_buffer (proven working)...");

            // 2 lines = 3200 bytes (fits in 4095 limit)
            const LOOP_LINES: usize = 2;
            const LOOP_SIZE: usize = WIDTH * LOOP_LINES * 2; // 3200 bytes

            let mut dma_buf = dma_loop_buffer!(LOOP_SIZE);
            esp_println::println!("  Loop buffer created ({} bytes)", LOOP_SIZE);

            // Copy lines from the middle of the AMS panel area (where spools are)
            // Line 126 is where the spool colors start (ams_y + 40 = 56 + 30 + 40 = 126)
            let start_line = 126;
            let src_offset = start_line * WIDTH * 2;
            let psram_slice = fb.as_dma_slice();

            // Copy from PSRAM to loop buffer
            for i in 0..LOOP_SIZE {
                dma_buf[i] = psram_slice[src_offset + i];
            }
            esp_println::println!("  Copied lines {}-{} from PSRAM framebuffer", start_line, start_line + LOOP_LINES - 1);
            esp_println::println!("  First bytes: {:02X} {:02X} {:02X} {:02X}",
                dma_buf[0], dma_buf[1], dma_buf[2], dma_buf[3]);

            esp_println::println!("Sending to display (continuous loop)...");
            let _transfer = match dpi.send(true, dma_buf) {
                Ok(t) => {
                    esp_println::println!("  DMA transfer started!");
                    t
                }
                Err((e, _, _)) => {
                    esp_println::println!("  Send error: {:?}", e);
                    loop { delay_ms(1000); }
                }
            };

            esp_println::println!("");
            esp_println::println!("=== SHOWING 2 LINES FROM FRAMEBUFFER ===");
            esp_println::println!("Should see colored spool bars repeated");
            esp_println::println!("(Line {} where the spools are drawn)", start_line);

            loop {
                delay_ms(5000);
            }
        }
        Err(e) => {
            esp_println::println!("DPI error: {:?}", e);
            loop { delay_ms(1000); }
        }
    }
}
