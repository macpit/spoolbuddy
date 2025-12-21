#!/usr/bin/env python3
"""Convert SVG spool image to LVGL-compatible RGB565 with alpha bitmap."""

import sys
import struct
from PIL import Image
import io

# Try to import svg rendering library
try:
    import cairosvg
    HAS_CAIROSVG = True
except ImportError:
    HAS_CAIROSVG = False

def svg_to_png_bytes(svg_path, width=None, height=None):
    """Convert SVG to PNG bytes using cairosvg."""
    if not HAS_CAIROSVG:
        raise ImportError("cairosvg is required: pip install cairosvg")

    kwargs = {}
    if width:
        kwargs['output_width'] = width
    if height:
        kwargs['output_height'] = height

    return cairosvg.svg2png(url=svg_path, **kwargs)

def png_to_rgb565_alpha(png_data, output_path):
    """Convert PNG to RGB565 with alpha format for LVGL."""
    img = Image.open(io.BytesIO(png_data)).convert('RGBA')
    width, height = img.size
    pixels = list(img.getdata())

    # LVGL LV_IMG_CF_TRUE_COLOR_ALPHA format: RGB565 (2 bytes) + Alpha (1 byte) = 3 bytes per pixel
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

    print(f"Converted: {width}x{height} -> {output_path}")
    print(f"Size: {len(output_data)} bytes ({width}*{height}*3 = {width*height*3})")
    return width, height

def main():
    if len(sys.argv) < 3:
        print("Usage: svg_to_lvgl.py <input.svg> <output.bin> [width] [height]")
        sys.exit(1)

    svg_path = sys.argv[1]
    output_path = sys.argv[2]
    width = int(sys.argv[3]) if len(sys.argv) > 3 else None
    height = int(sys.argv[4]) if len(sys.argv) > 4 else None

    print(f"Converting {svg_path} to {output_path}")
    if width and height:
        print(f"Target size: {width}x{height}")

    png_data = svg_to_png_bytes(svg_path, width, height)
    png_to_rgb565_alpha(png_data, output_path)

if __name__ == '__main__':
    main()
