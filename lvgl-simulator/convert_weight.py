#!/usr/bin/env python3
"""Convert weight/scale icon to LVGL format"""

from PIL import Image
import struct

# Load the scale icon (cleaner version)
img = Image.open('../screenshots/scale.png')
print(f"Original size: {img.size}, mode: {img.mode}")

# Resize to 64x64 for the scan card (larger for meter display)
img = img.resize((64, 64), Image.Resampling.LANCZOS)
print(f"Resized to: {img.size}")

# Ensure RGBA
if img.mode != 'RGBA':
    img = img.convert('RGBA')

# Invert colors (black -> white) while preserving alpha
pixels = list(img.getdata())
new_pixels = []
for r, g, b, a in pixels:
    # Invert RGB (black becomes white)
    new_pixels.append((255 - r, 255 - g, 255 - b, a))
img.putdata(new_pixels)

# Save preview
img.save('assets/weight_preview.png')
print("Saved preview to assets/weight_preview.png")

# Convert to LVGL TRUE_COLOR_ALPHA format (RGB565 + Alpha)
width, height = img.size
pixels = list(img.getdata())

with open('assets/weight.bin', 'wb') as f:
    for r, g, b, a in pixels:
        r5 = (r >> 3) & 0x1F
        g6 = (g >> 2) & 0x3F
        b5 = (b >> 3) & 0x1F
        rgb565 = (r5 << 11) | (g6 << 5) | b5
        f.write(struct.pack('<HB', rgb565, a))

print(f"Wrote {width * height * 3} bytes to assets/weight.bin")
print(f"Dimensions: {width}x{height}")
