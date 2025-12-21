#!/usr/bin/env python3
"""Create humidity and temperature icons from mockup SVG paths."""

import struct
from io import BytesIO

try:
    import cairosvg
    from PIL import Image
    HAS_CAIRO = True
except ImportError:
    HAS_CAIRO = False
    from PIL import Image, ImageDraw

def save_rgb565_alpha(img, output_path):
    """Save image as RGB565 + Alpha format for LVGL."""
    width, height = img.size
    pixels = list(img.convert('RGBA').getdata())

    output_data = bytearray()

    for r, g, b, a in pixels:
        # Convert to RGB565
        r5 = (r >> 3) & 0x1F
        g6 = (g >> 2) & 0x3F
        b5 = (b >> 3) & 0x1F
        rgb565 = (r5 << 11) | (g6 << 5) | b5

        # Write as little-endian RGB565 + alpha
        output_data.append(rgb565 & 0xFF)
        output_data.append((rgb565 >> 8) & 0xFF)
        output_data.append(a)

    with open(output_path, 'wb') as f:
        f.write(output_data)

    print(f"Saved: {output_path} ({width}x{height}, {len(output_data)} bytes)")
    return width, height


def create_svg_icon(svg_content, size, output_name):
    """Create icon from SVG content."""
    # Create full SVG with viewBox
    full_svg = f'''<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="{size}" height="{size}">
{svg_content}
</svg>'''

    if HAS_CAIRO:
        # Convert SVG to PNG using cairosvg
        png_data = cairosvg.svg2png(bytestring=full_svg.encode(), output_width=size, output_height=size)
        img = Image.open(BytesIO(png_data))
    else:
        # Fallback - create simple placeholder
        img = Image.new('RGBA', (size, size), (255, 255, 255, 255))

    # Save PNG preview
    img.save(f'../assets/{output_name}.png')
    print(f"Saved preview: ../assets/{output_name}.png")

    # Save LVGL binary
    save_rgb565_alpha(img, f'../assets/{output_name}.bin')

    return img


def main():
    size = 10  # Small icons for inline use

    # Humidity icon (water droplet) - WHITE so it can be recolored
    humidity_svg = '''<path fill="#FFFFFF" d="M12 2c-5.33 4.55-8 8.48-8 11.8 0 4.98 3.8 8.2 8 8.2s8-3.22 8-8.2c0-3.32-2.67-7.25-8-11.8z"/>'''

    print("Creating humidity icon...")
    create_svg_icon(humidity_svg, size, 'humidity_mockup')

    # Temperature icon (thermometer) - WHITE so it can be recolored
    temp_svg = '''<path fill="#FFFFFF" d="M15 13V5c0-1.66-1.34-3-3-3S9 3.34 9 5v8c-1.21.91-2 2.37-2 4 0 2.76 2.24 5 5 5s5-2.24 5-5c0-1.63-.79-3.09-2-4zm-4-8c0-.55.45-1 1-1s1 .45 1 1v3h-2V5z"/>'''

    print("Creating temperature icon...")
    create_svg_icon(temp_svg, size, 'temp_mockup')

    print("Done!")


if __name__ == '__main__':
    main()
