//! Button widget for touch interactions.

use crate::ui::theme::{self, spacing};
use embedded_graphics::{
    mono_font::{ascii::FONT_6X10, ascii::FONT_10X20, MonoTextStyle},
    pixelcolor::Rgb565,
    prelude::*,
    primitives::{PrimitiveStyle, Rectangle, RoundedRectangle},
    text::{Alignment, Text},
};

/// Button style variants
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ButtonStyle {
    /// Primary action button (filled with primary color)
    Primary,
    /// Secondary button (outlined)
    Secondary,
    /// Danger/destructive action (red)
    Danger,
    /// Ghost button (no background)
    Ghost,
}

/// Button widget
pub struct Button<'a> {
    /// Position (top-left corner)
    pub position: Point,
    /// Size of the button
    pub size: Size,
    /// Button label text
    pub label: &'a str,
    /// Button style
    pub style: ButtonStyle,
    /// Whether button is pressed
    pub pressed: bool,
    /// Whether button is disabled
    pub disabled: bool,
    /// Use large font
    pub large: bool,
}

impl<'a> Button<'a> {
    /// Create a new button
    pub fn new(position: Point, size: Size, label: &'a str) -> Self {
        Self {
            position,
            size,
            label,
            style: ButtonStyle::Primary,
            pressed: false,
            disabled: false,
            large: false,
        }
    }

    /// Set button style
    pub fn with_style(mut self, style: ButtonStyle) -> Self {
        self.style = style;
        self
    }

    /// Set pressed state
    pub fn set_pressed(&mut self, pressed: bool) {
        self.pressed = pressed;
    }

    /// Set disabled state
    pub fn set_disabled(&mut self, disabled: bool) {
        self.disabled = disabled;
    }

    /// Set large font
    pub fn with_large_font(mut self) -> Self {
        self.large = true;
        self
    }

    /// Check if a point is within the button bounds
    pub fn contains(&self, point: Point) -> bool {
        point.x >= self.position.x
            && point.x < self.position.x + self.size.width as i32
            && point.y >= self.position.y
            && point.y < self.position.y + self.size.height as i32
    }

    /// Draw the button
    pub fn draw<D>(&self, display: &mut D) -> Result<(), D::Error>
    where
        D: DrawTarget<Color = Rgb565>,
    {
        let theme = theme::theme();

        // Determine colors based on style and state
        let (bg_color, text_color, border_color) = if self.disabled {
            (theme.disabled, theme.text_secondary, theme.disabled)
        } else if self.pressed {
            match self.style {
                ButtonStyle::Primary => (theme::darken(theme.primary, 20), theme.bg, theme.primary),
                ButtonStyle::Secondary => (theme.primary, theme.bg, theme.primary),
                ButtonStyle::Danger => (theme::darken(theme.error, 20), theme.bg, theme.error),
                ButtonStyle::Ghost => (theme.card_bg, theme.text_primary, theme.card_bg),
            }
        } else {
            match self.style {
                ButtonStyle::Primary => (theme.primary, theme.bg, theme.primary),
                ButtonStyle::Secondary => (theme.card_bg, theme.text_primary, theme.border),
                ButtonStyle::Danger => (theme.error, theme.bg, theme.error),
                ButtonStyle::Ghost => (theme.bg, theme.text_primary, theme.bg),
            }
        };

        // Button background
        let button = RoundedRectangle::with_equal_corners(
            Rectangle::new(self.position, self.size),
            Size::new(theme::radius::SM, theme::radius::SM),
        );

        // Fill
        button
            .into_styled(PrimitiveStyle::with_fill(bg_color))
            .draw(display)?;

        // Border for secondary style
        if matches!(self.style, ButtonStyle::Secondary) && !self.pressed {
            let border = RoundedRectangle::with_equal_corners(
                Rectangle::new(self.position, self.size),
                Size::new(theme::radius::SM, theme::radius::SM),
            );
            border
                .into_styled(PrimitiveStyle::with_stroke(border_color, 2))
                .draw(display)?;
        }

        // Label text
        let text_style = if self.large {
            MonoTextStyle::new(&FONT_10X20, text_color)
        } else {
            MonoTextStyle::new(&FONT_6X10, text_color)
        };

        let text_pos = Point::new(
            self.position.x + (self.size.width as i32) / 2,
            self.position.y + (self.size.height as i32) / 2 + if self.large { 8 } else { 4 },
        );

        Text::with_alignment(self.label, text_pos, text_style, Alignment::Center)
            .draw(display)?;

        Ok(())
    }
}

/// Icon button (square button with icon)
pub struct IconButton {
    /// Position (top-left corner)
    pub position: Point,
    /// Size (width = height)
    pub size: u32,
    /// Button style
    pub style: ButtonStyle,
    /// Whether button is pressed
    pub pressed: bool,
    /// Whether button is disabled
    pub disabled: bool,
}

impl IconButton {
    /// Create a new icon button
    pub fn new(position: Point, size: u32) -> Self {
        Self {
            position,
            size,
            style: ButtonStyle::Secondary,
            pressed: false,
            disabled: false,
        }
    }

    /// Set button style
    pub fn with_style(mut self, style: ButtonStyle) -> Self {
        self.style = style;
        self
    }

    /// Check if a point is within the button bounds
    pub fn contains(&self, point: Point) -> bool {
        point.x >= self.position.x
            && point.x < self.position.x + self.size as i32
            && point.y >= self.position.y
            && point.y < self.position.y + self.size as i32
    }

    /// Draw the button background (icon should be drawn separately)
    pub fn draw_background<D>(&self, display: &mut D) -> Result<Rgb565, D::Error>
    where
        D: DrawTarget<Color = Rgb565>,
    {
        let theme = theme::theme();

        let (bg_color, icon_color) = if self.disabled {
            (theme.disabled, theme.text_secondary)
        } else if self.pressed {
            (theme.button_pressed, theme.bg)
        } else {
            match self.style {
                ButtonStyle::Primary => (theme.primary, theme.bg),
                ButtonStyle::Secondary => (theme.button_bg, theme.text_primary),
                ButtonStyle::Danger => (theme.error, theme.bg),
                ButtonStyle::Ghost => (theme.bg, theme.text_primary),
            }
        };

        // Button background
        let button = RoundedRectangle::with_equal_corners(
            Rectangle::new(self.position, Size::new(self.size, self.size)),
            Size::new(theme::radius::SM, theme::radius::SM),
        );
        button
            .into_styled(PrimitiveStyle::with_fill(bg_color))
            .draw(display)?;

        Ok(icon_color)
    }
}

/// Button bar for action buttons at bottom of screen
pub struct ButtonBar<'a> {
    /// Y position of the button bar
    pub y: i32,
    /// Height of the button bar
    pub height: u32,
    /// Buttons with their labels
    pub buttons: &'a [&'a str],
}

impl<'a> ButtonBar<'a> {
    /// Create a new button bar
    pub fn new(y: i32, height: u32, buttons: &'a [&'a str]) -> Self {
        Self { y, height, buttons }
    }

    /// Draw the button bar
    pub fn draw<D>(&self, display: &mut D, screen_width: u32) -> Result<(), D::Error>
    where
        D: DrawTarget<Color = Rgb565>,
    {
        if self.buttons.is_empty() {
            return Ok(());
        }

        let theme = theme::theme();
        let num_buttons = self.buttons.len() as u32;
        let button_width = (screen_width - spacing::MD as u32 * (num_buttons + 1)) / num_buttons;

        for (i, label) in self.buttons.iter().enumerate() {
            let x = spacing::MD as u32 + (button_width + spacing::MD as u32) * (i as u32);
            let button = Button::new(
                Point::new(x as i32, self.y),
                Size::new(button_width, self.height),
                label,
            )
            .with_style(if i == 0 {
                ButtonStyle::Primary
            } else {
                ButtonStyle::Secondary
            });
            button.draw(display)?;
        }

        Ok(())
    }

    /// Get which button index was pressed at a given point, if any
    pub fn button_at(&self, point: Point, screen_width: u32) -> Option<usize> {
        if point.y < self.y || point.y >= self.y + self.height as i32 {
            return None;
        }

        let num_buttons = self.buttons.len() as u32;
        let button_width = (screen_width - spacing::MD as u32 * (num_buttons + 1)) / num_buttons;

        for i in 0..self.buttons.len() {
            let x = spacing::MD as i32 + (button_width as i32 + spacing::MD) * (i as i32);
            if point.x >= x && point.x < x + button_width as i32 {
                return Some(i);
            }
        }

        None
    }
}
