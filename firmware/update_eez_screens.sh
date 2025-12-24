#!/bin/bash
# Update screens from EEZ Studio export
# Run this after exporting from EEZ Studio to ../eez/src/ui/

set -e

EEZ_DIR="../eez/src/ui"
TARGET_DIR="components/eez_ui"

echo "Updating EEZ UI screens..."

# Check if EEZ export exists
if [ ! -d "$EEZ_DIR" ]; then
    echo "ERROR: EEZ export not found at $EEZ_DIR"
    echo "Export from EEZ Studio first!"
    exit 1
fi

# Copy all files EXCEPT ui.c (contains custom navigation code)
cp "$EEZ_DIR/screens.c" "$TARGET_DIR/"
cp "$EEZ_DIR/screens.h" "$TARGET_DIR/"
cp "$EEZ_DIR/images.h" "$TARGET_DIR/"
cp "$EEZ_DIR/images.c" "$TARGET_DIR/"
cp "$EEZ_DIR/vars.h" "$TARGET_DIR/"
cp "$EEZ_DIR/actions.h" "$TARGET_DIR/"
cp "$EEZ_DIR/styles.c" "$TARGET_DIR/"
cp "$EEZ_DIR/styles.h" "$TARGET_DIR/"
cp "$EEZ_DIR/structs.h" "$TARGET_DIR/"
cp "$EEZ_DIR/fonts.h" "$TARGET_DIR/"
cp "$EEZ_DIR"/ui_image_*.c "$TARGET_DIR/"

echo "  - Copied screens, images, and headers"

# Fix LVGL 9.x compatibility
sed -i 's/lv_img_dsc_t/lv_image_dsc_t/g' "$TARGET_DIR/images.h"
echo "  - Applied LVGL 9.x fixes (lv_img_dsc_t -> lv_image_dsc_t)"

echo ""
echo "Done! Do NOT copy ui.c - it contains custom navigation code."
echo ""
echo "Rebuild with: cargo clean && cargo build --release --jobs 8"
