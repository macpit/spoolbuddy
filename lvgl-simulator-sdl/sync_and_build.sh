#!/bin/bash
# Sync from Debian server, apply EEZ fixes, build and run simulator
#
# Usage:
#   ./sync_and_build.sh                    # Sync, build, run (offline mode)
#   ./sync_and_build.sh --backend          # Enable backend client (requires curl)
#   ./sync_and_build.sh http://host:3000   # Run with custom backend URL
#   ./sync_and_build.sh --no-sync          # Skip rsync (use existing files)

set -e

cd "$(dirname "$0")"

# Parse arguments
ENABLE_BACKEND="OFF"
BACKEND_URL=""
DO_SYNC="yes"

for arg in "$@"; do
    case $arg in
        --backend)
            ENABLE_BACKEND="ON"
            ;;
        --no-sync)
            DO_SYNC="no"
            ;;
        http://*)
            ENABLE_BACKEND="ON"
            BACKEND_URL="$arg"
            ;;
    esac
done

if [ "$DO_SYNC" = "yes" ]; then
    echo "=== Syncing from Debian server ==="
    cd ../../
    # Use -L to follow symlinks (dereference them during transfer)
    # This ensures actual file contents are copied, not broken symlinks
    rsync -avrL --progress --delete \
        --exclude='lvgl-simulator-sdl/build' \
        --exclude='.git' \
        --exclude='firmware/target' \
        --exclude='firmware/.embuild' \
        --exclude='node_modules' \
        --exclude='__pycache__' \
        --exclude='*.pyc' \
        root@claude:/opt/claude/projects/SpoolStation .
    cd SpoolStation/lvgl-simulator-sdl
else
    echo "=== Skipping rsync (--no-sync) ==="
fi

echo "=== Setting up UI files ==="

# Copy EEZ-generated files from eez/src/ui/
echo "Copying EEZ-generated files..."
cp -fv ../eez/src/ui/screens.c ui/
cp -fv ../eez/src/ui/screens.h ui/
cp -fv ../eez/src/ui/images.c ui/
cp -fv ../eez/src/ui/images.h ui/
cp -fv ../eez/src/ui/styles.c ui/
cp -fv ../eez/src/ui/styles.h ui/
cp -fv ../eez/src/ui/structs.h ui/
cp -fv ../eez/src/ui/fonts.h ui/
cp -fv ../eez/src/ui/vars.h ui/
cp -fv ../eez/src/ui/actions.h ui/
cp -fv ../eez/src/ui/ui.h ui/
cp -f ../eez/src/ui/ui_image_*.c ui/ 2>/dev/null || true
echo "  Copied $(ls ui/ui_image_*.c 2>/dev/null | wc -l | tr -d ' ') image files"

# NOTE: Custom UI files (ui_backend.c, ui_nfc_card.c, etc.) are developed
# directly in simulator/ui/ and synced via rsync. They are NOT copied from
# firmware anymore. See WORKFLOW.md for details.
# When simulator code is working, manually copy to firmware/components/eez_ui/

echo "=== Applying LVGL 9.x fixes ==="

# Cross-platform sed -i (macOS uses -i '', Linux uses -i)
sedi() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        sed -i '' "$@"
    else
        sed -i "$@"
    fi
}

# Fix LVGL 9.x compatibility - images.h uses lv_img_dsc_t, convert to lv_image_dsc_t
sedi 's/lv_img_dsc_t/lv_image_dsc_t/g' ui/images.h
echo "  - Fixed lv_img_dsc_t -> lv_image_dsc_t in images.h"

# Fix EEZ-generated code bugs (empty parameters)
sedi 's/lv_image_set_pivot(obj, , );//g' ui/screens.c
sedi 's/lv_image_set_rotation(obj, );//g' ui/screens.c
echo "  - Removed empty lv_image_set_pivot/rotation calls"

# Fix undefined label long mode
sedi 's/LV_LABEL_LONG_undefined/LV_LABEL_LONG_WRAP/g' ui/screens.c
echo "  - Fixed LV_LABEL_LONG_undefined -> LV_LABEL_LONG_WRAP"

# Fix duplicate 'settings' identifier (button vs screen conflict)
# Use perl for complex patterns (more portable than sed)
perl -i -pe 's/lv_obj_t \*settings;/lv_obj_t *settings_main;/ if /encode_tag/ .. /catalog/' ui/screens.h
perl -i -pe 's/objects\.settings = obj;/objects.settings_main = obj;/ if /objects\.encode_tag = obj/ .. /objects\.catalog = obj/' ui/screens.c
echo "  - Fixed duplicate 'settings' identifier"

echo "=== Building simulator ==="
echo "Backend client: $ENABLE_BACKEND"
rm -rf build
mkdir build
cd build
cmake .. -DENABLE_BACKEND_CLIENT=$ENABLE_BACKEND
make -j10

echo ""
echo "=== Build complete ==="
echo ""

echo "=== Running simulator ==="
if [ -n "$BACKEND_URL" ]; then
    echo "Connecting to backend: $BACKEND_URL"
    ./simulator "$BACKEND_URL"
else
    ./simulator
fi
