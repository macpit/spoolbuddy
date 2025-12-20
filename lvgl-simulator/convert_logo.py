#!/usr/bin/env python3
"""Convert SpoolBuddy logo to LVGL image format (RGB565 + Alpha)"""

from PIL import Image
import struct

# Load the logo - use full logo as-is
img = Image.open('../screenshots/spoolbuddy_logo_transparent.png')
print(f"Original size: {img.size}, mode: {img.mode}")

# Ensure RGBA mode
if img.mode != 'RGBA':
    img = img.convert('RGBA')

# Convert to LVGL TRUE_COLOR_ALPHA format for RGB565 display
# Format: 2 bytes RGB565 + 1 byte Alpha = 3 bytes per pixel
width, height = img.size
pixels = list(img.getdata())

# Write as raw binary (RGB565 + Alpha)
with open('assets/logo.bin', 'wb') as f:
    for r, g, b, a in pixels:
        # Convert RGB888 to RGB565
        r5 = (r >> 3) & 0x1F
        g6 = (g >> 2) & 0x3F
        b5 = (b >> 3) & 0x1F
        rgb565 = (r5 << 11) | (g6 << 5) | b5
        # Write RGB565 (little-endian) + Alpha
        f.write(struct.pack('<HB', rgb565, a))

print(f"Wrote {width * height * 3} bytes to assets/logo.bin")
print(f"Logo dimensions: {width}x{height}")

# Generate Rust code for the image descriptor
print(f"""
// Rust code to include:
const LOGO_WIDTH: u32 = {width};
const LOGO_HEIGHT: u32 = {height};
static LOGO_DATA: &[u8] = include_bytes!("../assets/logo.bin");
// Data size: {width * height * 3} bytes (RGB565 + Alpha, 3 bytes per pixel)
""")
