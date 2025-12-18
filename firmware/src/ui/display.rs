//! RGB Parallel Display Driver for Waveshare ESP32-S3-Touch-LCD-4.3
//!
//! The display uses a 16-bit RGB565 parallel interface with:
//! - 800x480 resolution
//! - RGB565 color format (16-bit per pixel)
//! - DE (Data Enable) mode timing
//!
//! GPIO Mapping (Waveshare ESP32-S3-Touch-LCD-4.3):
//! - LCD_DE:    GPIO40
//! - LCD_VSYNC: GPIO41
//! - LCD_HSYNC: GPIO39
//! - LCD_PCLK:  GPIO42
//! - LCD_R0-R4: GPIO45, 48, 47, 21, 14  (directly active bits)
//! - LCD_G0-G5: GPIO9, 46, 3, 8, 18, 17
//! - LCD_B0-B4: GPIO10, 11, 12, 13, 14
//! - LCD_BL:    GPIO2 (Backlight PWM)

#![allow(dead_code)]

use super::{DisplayError, DISPLAY_HEIGHT, DISPLAY_WIDTH};
use embedded_graphics::pixelcolor::Rgb565;
use embedded_graphics::prelude::*;
use log::info;

/// Display timing configuration for 800x480 LCD
#[derive(Clone)]
pub struct DisplayTiming {
    pub h_res: u16,
    pub v_res: u16,
    pub h_sync_width: u16,
    pub h_back_porch: u16,
    pub h_front_porch: u16,
    pub v_sync_width: u16,
    pub v_back_porch: u16,
    pub v_front_porch: u16,
    pub pclk_hz: u32,
}

impl Default for DisplayTiming {
    fn default() -> Self {
        // Timing for Waveshare 4.3" 800x480 LCD
        Self {
            h_res: 800,
            v_res: 480,
            h_sync_width: 4,
            h_back_porch: 8,
            h_front_porch: 8,
            v_sync_width: 4,
            v_back_porch: 8,
            v_front_porch: 8,
            pclk_hz: 16_000_000, // 16MHz pixel clock (conservative)
        }
    }
}

/// Framebuffer size in bytes (RGB565 = 2 bytes per pixel)
pub const FRAMEBUFFER_SIZE: usize = (DISPLAY_WIDTH as usize) * (DISPLAY_HEIGHT as usize) * 2;

/// Framebuffer for the display - allocated in PSRAM
/// Must be aligned to 4 bytes for DMA
#[repr(C, align(4))]
pub struct FrameBuffer {
    pub data: [u8; FRAMEBUFFER_SIZE],
}

impl FrameBuffer {
    pub const fn new() -> Self {
        Self {
            data: [0u8; FRAMEBUFFER_SIZE],
        }
    }
}

/// Display driver wrapper for embedded-graphics compatibility
pub struct Display {
    /// Pointer to framebuffer (in PSRAM)
    framebuffer: &'static mut [u8],
    /// Current backlight brightness (0-100)
    brightness: u8,
    /// Whether display is initialized
    initialized: bool,
}

impl Display {
    /// Create a new display driver
    ///
    /// # Safety
    /// The framebuffer must be allocated in PSRAM and persist for the lifetime of the display.
    pub fn new(framebuffer: &'static mut [u8]) -> Result<Self, DisplayError> {
        if framebuffer.len() < FRAMEBUFFER_SIZE {
            return Err(DisplayError::BufferOverflow);
        }

        Ok(Self {
            framebuffer,
            brightness: 80,
            initialized: false,
        })
    }

    /// Mark display as initialized (called after hardware setup)
    pub fn set_initialized(&mut self) {
        self.initialized = true;
        info!("Display marked as initialized");
    }

    /// Set backlight brightness (0-100)
    pub fn set_backlight(&mut self, brightness: u8) -> Result<(), DisplayError> {
        self.brightness = brightness.min(100);
        // Note: Actual PWM control is done in main.rs with the LEDC peripheral
        info!("Backlight set to {}%", self.brightness);
        Ok(())
    }

    /// Get current backlight brightness
    pub fn backlight(&self) -> u8 {
        self.brightness
    }

    /// Clear the framebuffer to black
    pub fn clear_buffer(&mut self) {
        self.framebuffer[..FRAMEBUFFER_SIZE].fill(0);
    }

    /// Fill the framebuffer with a solid color
    pub fn fill(&mut self, color: Rgb565) {
        let color_bytes = color.into_storage().to_le_bytes();
        for chunk in self.framebuffer[..FRAMEBUFFER_SIZE].chunks_exact_mut(2) {
            chunk[0] = color_bytes[0];
            chunk[1] = color_bytes[1];
        }
    }

    /// Get a mutable reference to the framebuffer
    pub fn framebuffer_mut(&mut self) -> &mut [u8] {
        &mut self.framebuffer[..FRAMEBUFFER_SIZE]
    }

    /// Get an immutable reference to the framebuffer
    pub fn framebuffer(&self) -> &[u8] {
        &self.framebuffer[..FRAMEBUFFER_SIZE]
    }

    /// Set a pixel in the framebuffer
    #[inline]
    pub fn set_pixel(&mut self, x: u32, y: u32, color: Rgb565) {
        if x < DISPLAY_WIDTH && y < DISPLAY_HEIGHT {
            let offset = ((y * DISPLAY_WIDTH + x) * 2) as usize;
            let color_bytes = color.into_storage().to_le_bytes();
            self.framebuffer[offset] = color_bytes[0];
            self.framebuffer[offset + 1] = color_bytes[1];
        }
    }

    /// Check if display is initialized
    pub fn is_initialized(&self) -> bool {
        self.initialized
    }
}

/// Implement embedded-graphics DrawTarget for the display
impl DrawTarget for Display {
    type Color = Rgb565;
    type Error = DisplayError;

    fn draw_iter<I>(&mut self, pixels: I) -> Result<(), Self::Error>
    where
        I: IntoIterator<Item = Pixel<Self::Color>>,
    {
        for Pixel(coord, color) in pixels {
            if coord.x >= 0
                && coord.y >= 0
                && (coord.x as u32) < DISPLAY_WIDTH
                && (coord.y as u32) < DISPLAY_HEIGHT
            {
                self.set_pixel(coord.x as u32, coord.y as u32, color);
            }
        }
        Ok(())
    }
}

impl OriginDimensions for Display {
    fn size(&self) -> Size {
        Size::new(DISPLAY_WIDTH, DISPLAY_HEIGHT)
    }
}

/// Display configuration for initialization
#[derive(Clone)]
pub struct DisplayConfig {
    /// Initial backlight brightness (0-100)
    pub brightness: u8,
    /// Pixel clock frequency in Hz
    pub pclk_hz: u32,
}

impl Default for DisplayConfig {
    fn default() -> Self {
        Self {
            brightness: 80,
            pclk_hz: 16_000_000,
        }
    }
}

/// GPIO pin assignments for the Waveshare ESP32-S3-Touch-LCD-4.3
/// Source: https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3
pub mod pins {
    // Control signals
    /// Data Enable pin
    pub const DE: u8 = 5;
    /// Vertical Sync pin
    pub const VSYNC: u8 = 3;
    /// Horizontal Sync pin
    pub const HSYNC: u8 = 46;
    /// Pixel Clock pin
    pub const PCLK: u8 = 7;

    // RGB565: 5 bits red, 6 bits green, 5 bits blue = 16 bits total
    // The Waveshare board uses 16-bit parallel data

    /// Red data pins (R3-R7)
    pub const R3: u8 = 1;
    pub const R4: u8 = 2;
    pub const R5: u8 = 42;
    pub const R6: u8 = 41;
    pub const R7: u8 = 40;

    /// Green data pins (G2-G7)
    pub const G2: u8 = 39;
    pub const G3: u8 = 0;
    pub const G4: u8 = 45;
    pub const G5: u8 = 48;
    pub const G6: u8 = 47;
    pub const G7: u8 = 21;

    /// Blue data pins (B3-B7)
    pub const B3: u8 = 14;
    pub const B4: u8 = 38;
    pub const B5: u8 = 18;
    pub const B6: u8 = 17;
    pub const B7: u8 = 10;

    // Touch interface
    pub const TOUCH_IRQ: u8 = 4;
    pub const TOUCH_SDA: u8 = 8;
    pub const TOUCH_SCL: u8 = 9;

    // CH422G IO Expander (I2C address 0x24)
    // Controls: TP_RST (EXIO1), LCD_BL (EXIO2), LCD_RST (EXIO3), SD_CS (EXIO4)
    pub const IO_EXPANDER_ADDR: u8 = 0x24;
    pub const EXIO_TP_RST: u8 = 1;
    pub const EXIO_LCD_BL: u8 = 2;
    pub const EXIO_LCD_RST: u8 = 3;
    pub const EXIO_SD_CS: u8 = 4;
}
