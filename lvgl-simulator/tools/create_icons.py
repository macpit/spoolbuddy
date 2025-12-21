#!/usr/bin/env python3
"""Create humidity and temperature icons for LVGL."""

from PIL import Image, ImageDraw
import struct

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


def create_humidity_icon(size=12):
    """Create a water droplet icon for humidity."""
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Water droplet shape - a teardrop
    # Using polygon to approximate a droplet
    cx = size // 2

    # Draw a droplet using an ellipse for the bottom and a triangle for the top
    # Bottom half - rounded ellipse
    bottom_y = size - 2
    ellipse_h = int(size * 0.6)
    ellipse_w = int(size * 0.7)
    ellipse_top = bottom_y - ellipse_h

    # Draw the droplet as a polygon
    points = [
        (cx, 1),  # Top point
        (cx - ellipse_w//2, ellipse_top + ellipse_h//3),  # Left curve start
        (cx - ellipse_w//2, bottom_y - ellipse_h//4),  # Left bottom curve
        (cx, bottom_y),  # Bottom center
        (cx + ellipse_w//2, bottom_y - ellipse_h//4),  # Right bottom curve
        (cx + ellipse_w//2, ellipse_top + ellipse_h//3),  # Right curve start
    ]

    # Use white color so it can be recolored
    draw.polygon(points, fill=(255, 255, 255, 255))

    return img


def create_temperature_icon(size=12):
    """Create a thermometer icon for temperature."""
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Thermometer shape
    cx = size // 2

    # Thermometer tube (top part)
    tube_w = 4
    tube_h = size - 5
    tube_x = cx - tube_w // 2
    draw.rectangle([tube_x, 1, tube_x + tube_w, tube_h], fill=(255, 255, 255, 255))

    # Thermometer bulb (bottom circle)
    bulb_r = 3
    bulb_cy = size - bulb_r - 1
    draw.ellipse([cx - bulb_r, bulb_cy - bulb_r, cx + bulb_r, bulb_cy + bulb_r],
                 fill=(255, 255, 255, 255))

    # Lines on the side to indicate scale
    for y in range(3, tube_h - 1, 2):
        draw.line([(tube_x - 1, y), (tube_x, y)], fill=(255, 255, 255, 255))

    return img


def main():
    size = 12  # Small icons for inline use

    print("Creating humidity icon...")
    humidity_img = create_humidity_icon(size)
    humidity_img.save('../assets/humidity.png')
    save_rgb565_alpha(humidity_img, '../assets/humidity.bin')

    print("Creating temperature icon...")
    temp_img = create_temperature_icon(size)
    temp_img.save('../assets/temperature.png')
    save_rgb565_alpha(temp_img, '../assets/temperature.bin')

    print("Done!")


if __name__ == '__main__':
    main()
