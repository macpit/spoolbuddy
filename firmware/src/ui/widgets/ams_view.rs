//! AMS (Automatic Material System) visualization widget.
//!
//! Displays a Bambu Lab-style AMS unit with 4 filament slots,
//! showing colors and active slot indicator.

use crate::ui::theme::{self, radius, spacing};
use micromath::F32Ext;
use embedded_graphics::{
    mono_font::{ascii::FONT_6X10, MonoTextStyle},
    pixelcolor::Rgb565,
    prelude::*,
    primitives::{PrimitiveStyle, Rectangle, RoundedRectangle},
    text::{Alignment, Text},
};

/// AMS slot data
#[derive(Clone, Copy, Default)]
pub struct AmsSlot {
    /// Filament color (RGB565)
    pub color: Option<Rgb565>,
    /// Material type (e.g., "PLA", "PETG")
    pub material: Option<&'static str>,
    /// Whether this slot is currently active
    pub active: bool,
    /// Whether this slot is empty
    pub empty: bool,
}

/// AMS unit visualization widget
pub struct AmsView {
    /// Top-left position
    position: Point,
    /// Size of the widget
    size: Size,
    /// Slots data (4 slots)
    slots: [AmsSlot; 4],
    /// AMS unit label (e.g., "A", "B")
    label: char,
}

impl AmsView {
    /// Slot dimensions
    const SLOT_WIDTH: u32 = 44;
    const SLOT_HEIGHT: u32 = 56;
    const SLOT_SPACING: u32 = 6;
    const SLOT_PADDING: u32 = 8;

    /// Create a new AMS view
    pub fn new(position: Point, label: char) -> Self {
        // Calculate size based on 4 slots
        let width = Self::SLOT_PADDING * 2 + Self::SLOT_WIDTH * 4 + Self::SLOT_SPACING * 3;
        let height = Self::SLOT_PADDING * 2 + Self::SLOT_HEIGHT + 16; // Extra for label

        Self {
            position,
            size: Size::new(width, height),
            slots: [AmsSlot::default(); 4],
            label,
        }
    }

    /// Set slot data
    pub fn set_slot(&mut self, index: usize, slot: AmsSlot) {
        if index < 4 {
            self.slots[index] = slot;
        }
    }

    /// Set all slots at once
    pub fn set_slots(&mut self, slots: [AmsSlot; 4]) {
        self.slots = slots;
    }

    /// Get widget size
    pub fn size(&self) -> Size {
        self.size
    }

    /// Draw the AMS unit
    pub fn draw<D>(&self, display: &mut D) -> Result<(), D::Error>
    where
        D: DrawTarget<Color = Rgb565>,
    {
        let theme = theme::theme();

        // Main housing
        let housing = RoundedRectangle::with_equal_corners(
            Rectangle::new(self.position, self.size),
            Size::new(radius::MD, radius::MD),
        );
        housing
            .into_styled(PrimitiveStyle::with_fill(theme.card_bg))
            .draw(display)?;

        // Draw border
        housing
            .into_styled(PrimitiveStyle::with_stroke(theme.border, 1))
            .draw(display)?;

        // Draw each slot
        for (i, slot) in self.slots.iter().enumerate() {
            self.draw_slot(display, i, slot)?;
        }

        // AMS label at bottom
        let label_y = self.position.y + self.size.height as i32 - 12;
        let label_x = self.position.x + self.size.width as i32 / 2;

        let label_text: heapless::String<8> = {
            let mut s = heapless::String::new();
            let _ = core::fmt::write(&mut s, format_args!("AMS {}", self.label));
            s
        };

        Text::with_alignment(
            &label_text,
            Point::new(label_x, label_y),
            MonoTextStyle::new(&FONT_6X10, theme.primary),
            Alignment::Center,
        )
        .draw(display)?;

        Ok(())
    }

    /// Draw a single slot
    fn draw_slot<D>(&self, display: &mut D, index: usize, slot: &AmsSlot) -> Result<(), D::Error>
    where
        D: DrawTarget<Color = Rgb565>,
    {
        let theme = theme::theme();

        // Calculate slot position
        let slot_x = self.position.x
            + Self::SLOT_PADDING as i32
            + (index as i32) * (Self::SLOT_WIDTH as i32 + Self::SLOT_SPACING as i32);
        let slot_y = self.position.y + Self::SLOT_PADDING as i32;

        // Slot background
        let slot_rect = RoundedRectangle::with_equal_corners(
            Rectangle::new(
                Point::new(slot_x, slot_y),
                Size::new(Self::SLOT_WIDTH, Self::SLOT_HEIGHT),
            ),
            Size::new(radius::SM, radius::SM),
        );

        // Background color (darker for empty slots)
        let bg_color = if slot.empty {
            theme.bg
        } else {
            Rgb565::new(0x02, 0x04, 0x02) // Very dark
        };
        slot_rect
            .into_styled(PrimitiveStyle::with_fill(bg_color))
            .draw(display)?;

        // Active slot indicator (border glow)
        if slot.active {
            slot_rect
                .into_styled(PrimitiveStyle::with_stroke(theme.primary, 2))
                .draw(display)?;
        }

        // Filament color area
        if let Some(color) = slot.color {
            let color_rect = RoundedRectangle::with_equal_corners(
                Rectangle::new(
                    Point::new(slot_x + 4, slot_y + 4),
                    Size::new(Self::SLOT_WIDTH - 8, Self::SLOT_HEIGHT - 20),
                ),
                Size::new(2, 2),
            );
            color_rect
                .into_styled(PrimitiveStyle::with_fill(color))
                .draw(display)?;

            // Spool center hole
            let hole_x = slot_x + Self::SLOT_WIDTH as i32 / 2;
            let hole_y = slot_y + (Self::SLOT_HEIGHT as i32 - 12) / 2;

            // Draw ellipse approximation (vertical oval)
            for dy in -10..=10i32 {
                let width = ((100 - dy * dy) as f32).sqrt() as i32 / 3;
                if width > 0 {
                    Rectangle::new(
                        Point::new(hole_x - width, hole_y + dy),
                        Size::new((width * 2) as u32, 1),
                    )
                    .into_styled(PrimitiveStyle::with_fill(theme.bg))
                    .draw(display)?;
                }
            }
        }

        // Slot number
        let num_y = slot_y + Self::SLOT_HEIGHT as i32 - 8;
        let num_x = slot_x + Self::SLOT_WIDTH as i32 / 2;

        let num_text: heapless::String<2> = {
            let mut s = heapless::String::new();
            let _ = core::fmt::write(&mut s, format_args!("{}", index + 1));
            s
        };

        let num_color = if slot.active {
            theme.primary
        } else {
            theme.text_secondary
        };

        Text::with_alignment(
            &num_text,
            Point::new(num_x, num_y),
            MonoTextStyle::new(&FONT_6X10, num_color),
            Alignment::Center,
        )
        .draw(display)?;

        Ok(())
    }
}

/// Convert RGBA u32 to Rgb565 for slot colors
pub fn rgba_to_slot_color(rgba: u32) -> Rgb565 {
    theme::rgba_to_rgb565(rgba)
}
