//! Theme definitions for SpoolBuddy UI.
//!
//! Supports both light and dark themes with teal accent colors.

use embedded_graphics::pixelcolor::Rgb565;
use embedded_graphics::prelude::{IntoStorage, RgbColor};

/// Theme mode
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum ThemeMode {
    Light,
    #[default]
    Dark,
}

/// Color palette for a theme
#[derive(Clone, Copy)]
pub struct ThemeColors {
    /// Main background color
    pub bg: Rgb565,
    /// Card/surface background color
    pub card_bg: Rgb565,
    /// Primary accent color
    pub primary: Rgb565,
    /// Primary text color
    pub text_primary: Rgb565,
    /// Secondary/muted text color
    pub text_secondary: Rgb565,
    /// Success color
    pub success: Rgb565,
    /// Warning color
    pub warning: Rgb565,
    /// Error color
    pub error: Rgb565,
    /// Disabled/inactive color
    pub disabled: Rgb565,
    /// Status bar background
    pub status_bar_bg: Rgb565,
    /// Button background
    pub button_bg: Rgb565,
    /// Button pressed state
    pub button_pressed: Rgb565,
    /// Progress bar background
    pub progress_bg: Rgb565,
    /// Border/divider color
    pub border: Rgb565,
}

/// Dark theme colors - Bambu Lab inspired
pub const DARK_THEME: ThemeColors = ThemeColors {
    bg: Rgb565::new(0x03, 0x03, 0x03),              // #1A1A1A (near black)
    card_bg: Rgb565::new(0x05, 0x0B, 0x0B),         // #2D2D2D (dark gray)
    primary: Rgb565::new(0x00, 0x2B, 0x16),         // #00ADB5 (cyan/teal - Bambu style)
    text_primary: Rgb565::WHITE,                    // #FFFFFF
    text_secondary: Rgb565::new(0x16, 0x2C, 0x16),  // #B0B0B0 (gray)
    success: Rgb565::new(0x09, 0x2B, 0x0A),         // #4CAF50 (green)
    warning: Rgb565::new(0x1F, 0x30, 0x00),         // #FFC107 (amber)
    error: Rgb565::new(0x1E, 0x08, 0x06),           // #F44336 (red)
    disabled: Rgb565::new(0x0E, 0x1C, 0x0E),        // #707070 (muted)
    status_bar_bg: Rgb565::new(0x02, 0x04, 0x02),   // #101010 (darker)
    button_bg: Rgb565::new(0x05, 0x0B, 0x0B),       // #2D2D2D (card_bg)
    button_pressed: Rgb565::new(0x00, 0x2B, 0x16),  // primary
    progress_bg: Rgb565::new(0x07, 0x0F, 0x0F),     // #3D3D3D (elevated)
    border: Rgb565::new(0x07, 0x0F, 0x0F),          // #3D3D3D
};

/// Light theme colors
pub const LIGHT_THEME: ThemeColors = ThemeColors {
    bg: Rgb565::new(0x1f, 0x3f, 0x1f),              // #f8f9fa (off-white)
    card_bg: Rgb565::WHITE,                         // #ffffff
    primary: Rgb565::new(0x00, 0x2a, 0x12),         // #00a884 (darker teal for contrast)
    text_primary: Rgb565::new(0x02, 0x04, 0x02),    // #111827 (dark gray)
    text_secondary: Rgb565::new(0x0c, 0x18, 0x0c),  // #6b7280 (medium gray)
    success: Rgb565::new(0x04, 0x28, 0x08),         // #22c55e
    warning: Rgb565::new(0x1f, 0x25, 0x00),         // #f59e0b
    error: Rgb565::new(0x1e, 0x08, 0x08),           // #ef4444
    disabled: Rgb565::new(0x17, 0x2e, 0x17),        // #d1d5db
    status_bar_bg: Rgb565::new(0x1d, 0x3b, 0x1d),   // #e5e7eb
    button_bg: Rgb565::new(0x1d, 0x3b, 0x1d),       // #e5e7eb
    button_pressed: Rgb565::new(0x00, 0x2a, 0x12),  // primary
    progress_bg: Rgb565::new(0x1d, 0x3b, 0x1d),     // #e5e7eb
    border: Rgb565::new(0x17, 0x2e, 0x17),          // #d1d5db
};

/// Current theme instance (thread-safe via critical section)
static mut CURRENT_THEME: ThemeMode = ThemeMode::Dark;

/// Get the current theme colors
pub fn theme() -> &'static ThemeColors {
    unsafe {
        match CURRENT_THEME {
            ThemeMode::Dark => &DARK_THEME,
            ThemeMode::Light => &LIGHT_THEME,
        }
    }
}

/// Get the current theme mode
pub fn theme_mode() -> ThemeMode {
    unsafe { CURRENT_THEME }
}

/// Set the current theme mode
pub fn set_theme_mode(mode: ThemeMode) {
    unsafe {
        CURRENT_THEME = mode;
    }
}

/// Toggle between light and dark themes
pub fn toggle_theme() -> ThemeMode {
    unsafe {
        CURRENT_THEME = match CURRENT_THEME {
            ThemeMode::Dark => ThemeMode::Light,
            ThemeMode::Light => ThemeMode::Dark,
        };
        CURRENT_THEME
    }
}

/// Convert RGBA u32 to Rgb565
pub fn rgba_to_rgb565(rgba: u32) -> Rgb565 {
    let r = ((rgba >> 24) & 0xFF) as u8;
    let g = ((rgba >> 16) & 0xFF) as u8;
    let b = ((rgba >> 8) & 0xFF) as u8;
    // Alpha is ignored for display

    // RGB565: 5 bits red, 6 bits green, 5 bits blue
    Rgb565::new(r >> 3, g >> 2, b >> 3)
}

/// Blend two colors (for hover effects, transparency, etc.)
pub fn blend_colors(fg: Rgb565, bg: Rgb565, alpha: u8) -> Rgb565 {
    let fg_raw = fg.into_storage();
    let bg_raw = bg.into_storage();

    // Extract RGB565 components
    let fg_r = ((fg_raw >> 11) & 0x1F) as u16;
    let fg_g = ((fg_raw >> 5) & 0x3F) as u16;
    let fg_b = (fg_raw & 0x1F) as u16;

    let bg_r = ((bg_raw >> 11) & 0x1F) as u16;
    let bg_g = ((bg_raw >> 5) & 0x3F) as u16;
    let bg_b = (bg_raw & 0x1F) as u16;

    let a = alpha as u16;
    let inv_a = 255 - a;

    // Blend
    let r = ((fg_r * a + bg_r * inv_a) / 255) as u16;
    let g = ((fg_g * a + bg_g * inv_a) / 255) as u16;
    let b = ((fg_b * a + bg_b * inv_a) / 255) as u16;

    Rgb565::new((r & 0x1F) as u8, (g & 0x3F) as u8, (b & 0x1F) as u8)
}

/// Darken a color by a percentage (0-100)
pub fn darken(color: Rgb565, percent: u8) -> Rgb565 {
    let raw = color.into_storage();
    let r = ((raw >> 11) & 0x1F) as u16;
    let g = ((raw >> 5) & 0x3F) as u16;
    let b = (raw & 0x1F) as u16;

    let factor = (100 - percent.min(100)) as u16;

    let r = ((r * factor) / 100) as u8;
    let g = ((g * factor) / 100) as u8;
    let b = ((b * factor) / 100) as u8;

    Rgb565::new(r, g, b)
}

/// Lighten a color by a percentage (0-100)
pub fn lighten(color: Rgb565, percent: u8) -> Rgb565 {
    let raw = color.into_storage();
    let r = ((raw >> 11) & 0x1F) as u16;
    let g = ((raw >> 5) & 0x3F) as u16;
    let b = (raw & 0x1F) as u16;

    let factor = percent.min(100) as u16;

    let r = (r + ((0x1F - r) * factor) / 100) as u8;
    let g = (g + ((0x3F - g) * factor) / 100) as u8;
    let b = (b + ((0x1F - b) * factor) / 100) as u8;

    Rgb565::new(r, g, b)
}

/// Standard spacing values
pub mod spacing {
    /// Extra small spacing (4px)
    pub const XS: i32 = 4;
    /// Small spacing (8px)
    pub const SM: i32 = 8;
    /// Medium spacing (16px)
    pub const MD: i32 = 16;
    /// Large spacing (24px)
    pub const LG: i32 = 24;
    /// Extra large spacing (32px)
    pub const XL: i32 = 32;
}

/// Standard border radius values
pub mod radius {
    /// Small radius for buttons
    pub const SM: u32 = 4;
    /// Medium radius for cards
    pub const MD: u32 = 8;
    /// Large radius for modals
    pub const LG: u32 = 12;
    /// Pill/rounded radius
    pub const PILL: u32 = 999;
}

/// Font sizes (in pixels, for embedded-graphics fonts)
pub mod font_size {
    /// Small text (status, labels)
    pub const SM: u32 = 12;
    /// Normal text
    pub const MD: u32 = 16;
    /// Large text (headers)
    pub const LG: u32 = 24;
    /// Extra large (weight display)
    pub const XL: u32 = 48;
    /// Huge (main weight)
    pub const XXL: u32 = 72;
}

/// Z-index layers for rendering order
pub mod layer {
    /// Background layer
    pub const BG: u8 = 0;
    /// Content layer
    pub const CONTENT: u8 = 10;
    /// Widget layer
    pub const WIDGET: u8 = 20;
    /// Overlay layer (modals, toasts)
    pub const OVERLAY: u8 = 30;
    /// Top layer (notifications)
    pub const TOP: u8 = 40;
}

/// Material type colors for visual identification
pub fn material_color(material: &str) -> Rgb565 {
    match material.to_uppercase().as_str() {
        "PLA" => Rgb565::new(0x00, 0x30, 0x18),   // Green-ish
        "PETG" => Rgb565::new(0x00, 0x18, 0x30),  // Blue-ish
        "ABS" => Rgb565::new(0x30, 0x18, 0x00),   // Orange-ish
        "TPU" => Rgb565::new(0x20, 0x00, 0x30),   // Purple-ish
        "ASA" => Rgb565::new(0x30, 0x30, 0x00),   // Yellow-ish
        "PA" | "NYLON" => Rgb565::new(0x18, 0x18, 0x18), // Gray
        "PC" => Rgb565::new(0x10, 0x20, 0x30),    // Light blue
        "PVA" => Rgb565::new(0x20, 0x30, 0x20),   // Light green
        "HIPS" => Rgb565::new(0x28, 0x28, 0x18),  // Tan
        _ => theme().card_bg,
    }
}

/// WiFi signal strength to icon level (0-4 bars)
pub fn wifi_signal_bars(rssi: i8) -> u8 {
    match rssi {
        -50..=0 => 4,    // Excellent
        -60..=-51 => 3,  // Good
        -70..=-61 => 2,  // Fair
        -80..=-71 => 1,  // Weak
        _ => 0,          // No signal
    }
}

/// Battery level to icon level (0-4 bars)
/// Note: ESP32-S3 Touch LCD 4.3 doesn't have battery, but included for future
pub fn battery_bars(percent: u8) -> u8 {
    match percent {
        80..=100 => 4,
        60..=79 => 3,
        40..=59 => 2,
        20..=39 => 1,
        _ => 0,
    }
}

/// Calculate remaining percentage from weight
pub fn weight_percentage(current: f32, label: f32) -> u8 {
    if label <= 0.0 {
        return 0;
    }
    ((current / label) * 100.0).clamp(0.0, 100.0) as u8
}

/// Format weight for display (e.g., "1,234.5 g")
pub fn format_weight(grams: f32) -> heapless::String<16> {
    let mut s = heapless::String::new();
    let whole = grams as u32;
    let frac = ((grams - whole as f32) * 10.0) as u8;

    // Add thousands separator
    if whole >= 1000 {
        let _ = core::fmt::write(&mut s, format_args!("{},{:03}.{} g", whole / 1000, whole % 1000, frac));
    } else {
        let _ = core::fmt::write(&mut s, format_args!("{}.{} g", whole, frac));
    }
    s
}
