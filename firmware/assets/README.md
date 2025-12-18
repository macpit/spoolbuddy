# SpoolBuddy Firmware Assets

This directory contains graphic assets for the ESP32 display UI.

## Directory Structure

```
assets/
├── icons/          # SVG source icons
│   ├── spool.svg
│   ├── nfc.svg
│   ├── scale.svg
│   ├── wifi.svg
│   ├── settings.svg
│   ├── ams_slot.svg
│   └── ams_unit.svg
├── fonts/          # Font files (TTF → LVGL format)
└── images/         # Converted images for embedding
```

## Converting SVG to Embedded Format

### Option 1: Using ImageMagick + xxd (Simple)

```bash
# Convert SVG to PNG
convert -background transparent icons/spool.svg -resize 64x64 images/spool_64.png

# Convert PNG to C header
xxd -i images/spool_64.png > ../src/ui/assets/spool_64.h
```

### Option 2: Using LVGL Image Converter (Recommended)

1. Go to https://lvgl.io/tools/imageconverter
2. Upload PNG file
3. Select "C array" output
4. Select "RGB565" color format (matches our display)
5. Download and place in `src/ui/assets/`

### Option 3: Using tinybmp crate

For BMP support with embedded-graphics:

```rust
use tinybmp::Bmp;
use embedded_graphics::image::Image;

// Include BMP data
const SPOOL_BMP: &[u8] = include_bytes!("../../assets/images/spool.bmp");

// Load and draw
let bmp = Bmp::from_slice(SPOOL_BMP).unwrap();
Image::new(&bmp, Point::new(10, 10)).draw(&mut display)?;
```

## Color Placeholders

Some SVGs contain placeholder colors that should be replaced dynamically:
- `#FILAMENT_COLOR` - Replace with spool's RGBA color
- `#SLOT1_COLOR` through `#SLOT4_COLOR` - AMS slot filament colors

## Icon Sizes

| Use Case | Size | Notes |
|----------|------|-------|
| Status bar | 16x16, 24x24 | WiFi, server status |
| Buttons | 24x24, 32x32 | Action icons |
| Main display | 48x48, 64x64 | Spool, scale icons |
| Full spool view | 128x128 | Detailed spool graphic |
| AMS visualization | 220x80 | Full 4-slot unit |

## Color Palette

Match Bambu Lab dark UI:

```
Background:     #1A1A1A
Card:           #2D2D2D
Elevated:       #3D3D3D
Primary/Accent: #00ADB5 (cyan/teal)
Text Primary:   #FFFFFF
Text Secondary: #B0B0B0
Text Muted:     #707070
Success:        #4CAF50
Warning:        #FFC107
Error:          #F44336
```

## Fonts

### Converting TTF to embedded format

Using `fontbm` or similar tool:

```bash
# Install fontbm
pip install fontbm

# Convert Roboto to BMFont format
fontbm --font-file Roboto-Regular.ttf --output roboto_16 --font-size 16
```

Or use LVGL's font converter: https://lvgl.io/tools/fontconverter

## Batch Conversion Script

```bash
#!/bin/bash
# convert_assets.sh

SIZES="16 24 32 48 64"

for svg in icons/*.svg; do
    name=$(basename "$svg" .svg)
    for size in $SIZES; do
        convert -background transparent "$svg" -resize ${size}x${size} \
            "images/${name}_${size}.png"
    done
done

echo "Conversion complete!"
```

## Adding New Icons

1. Create SVG in `icons/` directory
2. Use the color palette above
3. Keep viewBox consistent (e.g., 24x24 for small icons)
4. Convert to needed sizes
5. Add to `src/ui/assets/` as C header or BMP
