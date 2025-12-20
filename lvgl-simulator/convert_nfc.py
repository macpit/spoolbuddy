#!/usr/bin/env python3
"""Convert NFC icon to LVGL format"""

from PIL import Image
import struct

# Load the NFC icon
img = Image.open('../screenshots/nfc.png')
print(f"Original size: {img.size}, mode: {img.mode}")

# Resize to 72x72 for larger display
img = img.resize((72, 72), Image.Resampling.LANCZOS)
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
img.save('assets/nfc_preview.png')
print("Saved preview to assets/nfc_preview.png")

# Convert to LVGL TRUE_COLOR_ALPHA format (RGB565 + Alpha)
width, height = img.size
pixels = list(img.getdata())

with open('assets/nfc.bin', 'wb') as f:
    for r, g, b, a in pixels:
        r5 = (r >> 3) & 0x1F
        g6 = (g >> 2) & 0x3F
        b5 = (b >> 3) & 0x1F
        rgb565 = (r5 << 11) | (g6 << 5) | b5
        f.write(struct.pack('<HB', rgb565, a))

print(f"Wrote {width * height * 3} bytes to assets/nfc.bin")
print(f"Dimensions: {width}x{height}")
