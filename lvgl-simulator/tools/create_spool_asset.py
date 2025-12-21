#!/usr/bin/env python3
"""Create a 3D spool bitmap asset for LVGL using PIL."""

from PIL import Image, ImageDraw
import struct

def create_spool_frame(width=32, height=42):
    """Create spool frame (flanges only) - not recolored."""
    img = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    sx = width / 53.0
    sy = height / 76.0

    # Left flange (ellipse)
    flange_cx = int(9.67 * sx)
    flange_rx = int(7.85 * sx)
    flange_ry = int(36.36 * sy)
    flange_cy = int(38.16 * sy)

    draw.ellipse([
        flange_cx - flange_rx, flange_cy - flange_ry,
        flange_cx + flange_rx, flange_cy + flange_ry
    ], fill=(220, 220, 220, 255), outline=(160, 160, 160, 255), width=1)

    # Right flange
    rflange_cx = int(43.5 * sx)
    draw.ellipse([
        rflange_cx - flange_rx, flange_cy - flange_ry,
        rflange_cx + flange_rx, flange_cy + flange_ry
    ], fill=(220, 220, 220, 255), outline=(160, 160, 160, 255), width=1)

    # Center hole on left flange
    hole_rx = int(1.5 * sx)
    hole_ry = int(6 * sy)
    draw.ellipse([
        flange_cx - hole_rx, flange_cy - hole_ry,
        flange_cx + hole_rx, flange_cy + hole_ry
    ], fill=(100, 100, 100, 255))

    return img

def create_spool_fill(width=32, height=42):
    """Create spool fill area (filament body) - this gets recolored."""
    img = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    sx = width / 53.0
    sy = height / 76.0

    # Filament body area (between flanges)
    body_left = int(10 * sx)
    body_right = int(43 * sx)
    body_top = int(5 * sy)
    body_bottom = int(71 * sy)

    # Draw body - white so it can be fully recolored
    draw.rounded_rectangle([
        body_left, body_top, body_right, body_bottom
    ], radius=2, fill=(255, 255, 255, 255))

    return img

def create_spool_image(width=32, height=42):
    """Create a grayscale spool image that can be colorized."""
    img = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Spool dimensions (based on spool_clean.svg proportions scaled to 32x42)
    # The SVG is 53x76, we're scaling to 32x42

    # Scale factors
    sx = width / 53.0
    sy = height / 76.0

    # Left flange (ellipse) - light gray with border
    flange_cx = int(9.67 * sx)
    flange_rx = int(7.85 * sx)
    flange_ry = int(36.36 * sy)
    flange_cy = int(38.16 * sy)

    # Draw left flange outline
    draw.ellipse([
        flange_cx - flange_rx, flange_cy - flange_ry,
        flange_cx + flange_rx, flange_cy + flange_ry
    ], fill=(248, 248, 248, 255), outline=(172, 172, 172, 255), width=1)

    # Draw the main spool body (the wound filament area)
    # This is the colored area - from x ~15 to x ~44 in original SVG
    body_left = int(15 * sx)
    body_right = int(44 * sx)
    body_top = int(9 * sy)
    body_bottom = int(67 * sy)

    # Draw body with slight rounding
    draw.rounded_rectangle([
        body_left, body_top, body_right, body_bottom
    ], radius=2, fill=(248, 248, 248, 255), outline=(172, 172, 172, 255), width=1)

    # Right flange (partial ellipse visible)
    rflange_cx = int(43.5 * sx)
    rflange_rx = int(7.85 * sx)
    rflange_ry = int(36.36 * sy)
    rflange_cy = int(38.16 * sy)

    # Draw right flange
    draw.ellipse([
        rflange_cx - rflange_rx, rflange_cy - rflange_ry,
        rflange_cx + rflange_rx, rflange_cy + rflange_ry
    ], fill=(248, 248, 248, 255), outline=(172, 172, 172, 255), width=1)

    # Center hole on left flange
    hole_rx = int(1.2 * sx)
    hole_ry = int(5.45 * sy)
    draw.ellipse([
        flange_cx - hole_rx, flange_cy - hole_ry,
        flange_cx + hole_rx, flange_cy + hole_ry
    ], fill=(172, 172, 172, 255))

    return img

def create_spool_mask(width=32, height=42):
    """Create a mask showing where the filament color should be applied."""
    mask = Image.new('L', (width, height), 0)
    draw = ImageDraw.Draw(mask)

    sx = width / 53.0
    sy = height / 76.0

    # The colorable area is the main body (filament wound area)
    body_left = int(17 * sx)
    body_right = int(42 * sx)
    body_top = int(11 * sy)
    body_bottom = int(65 * sy)

    draw.rounded_rectangle([
        body_left, body_top, body_right, body_bottom
    ], radius=2, fill=255)

    return mask

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

def main():
    # Create spool at 32x42 (same as mockup)
    width, height = 32, 42

    print(f"Creating spool assets {width}x{height}...")

    # Create spool fill (colored filament area) - white, gets recolored
    fill_img = create_spool_fill(width, height)
    fill_img.save('../assets/spool_fill.png')
    save_rgb565_alpha(fill_img, '../assets/spool_fill.bin')
    print("Saved spool_fill.bin")

    # Create spool frame (gray flanges) - not recolored
    frame_img = create_spool_frame(width, height)
    frame_img.save('../assets/spool_frame.png')
    save_rgb565_alpha(frame_img, '../assets/spool_frame.bin')
    print("Saved spool_frame.bin")

    # Also create combined spool for backward compatibility
    spool_img = create_spool_image(width, height)
    spool_img.save('../assets/spool_base.png')
    save_rgb565_alpha(spool_img, '../assets/spool.bin')
    print("Saved spool.bin (combined)")

    # Create and save mask
    mask = create_spool_mask(width, height)
    mask.save('../assets/spool_mask.png')
    print("Saved spool_mask.png")

    print("Done!")

if __name__ == '__main__':
    main()
