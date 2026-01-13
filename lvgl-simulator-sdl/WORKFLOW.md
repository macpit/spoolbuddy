# LVGL Simulator Development Workflow

## Overview

The LVGL simulator allows rapid UI development without flashing the ESP32 firmware for every change. The simulator connects to the real Python backend, so all functionality works exactly as on the real device.

## Development Flow

```
1. START backend (python backend/main.py)
         ↓
2. DEVELOP in simulator (lvgl-simulator-sdl/ui/)
         ↓
3. TEST in simulator (./build/simulator --backend)
         ↓
4. ITERATE until working
         ↓
5. COPY working code to firmware (firmware/components/eez_ui/)
         ↓
6. FLASH and test on hardware
```

## Directory Structure

```
SpoolStation/
├── lvgl-simulator-sdl/          # Simulator (development here first)
│   ├── ui/                      # UI code being developed
│   │   ├── ui_backend.c         # Backend status display
│   │   ├── ui_nfc_card.c        # NFC popup on main screen
│   │   ├── ui_scan_result.c     # Scan result screen logic
│   │   ├── screens.c/h          # EEZ-generated (don't edit)
│   │   └── ...
│   ├── backend_client.c/h       # Backend API client
│   ├── sim_control.h            # Simulator keyboard controls
│   ├── main.c                   # Simulator main loop
│   └── sync_and_build.sh        # Sync script (for local Mac)
│
├── firmware/components/eez_ui/  # Firmware UI code (copy here when done)
│   ├── ui_backend.c
│   ├── ui_nfc_card.c
│   └── ...
│
├── backend/                     # Python FastAPI backend
│   ├── api/spools.py            # Spool inventory API
│   ├── api/printers.py          # Printer API
│   ├── db/                      # SQLite database
│   └── ...
│
└── eez/src/ui/                  # EEZ Studio generated files (source of truth)
    ├── screens.c/h
    ├── images.c/h
    └── ...
```

## What Goes Where

### Same Code (simulator AND firmware)
All UI logic is identical - no #ifdefs, no platform-specific code:
- `ui_backend.c` - Backend/printer status display
- `ui_nfc_card.c` - NFC card popup
- `ui_scan_result.c` - Scan result screen
- `ui_wifi.c` - WiFi settings
- `ui_settings.c` - Settings screens
- `ui_printer.c` - Printer management
- `ui_scale.c` - Scale display
- `ui.c` - Main UI tick and navigation
- `ui_internal.h` - Shared types and declarations

### Data from Backend
The backend provides ALL data via API:
- Spool inventory (GET/POST /api/spools)
- NFC tag data
- Scale weight readings
- WiFi status
- Printer/AMS status
- OTA update status

### Simulator-Specific Files
Files that only exist in simulator:
- `backend_client.c/h` - HTTP client for backend API
- `sim_control.h` - Keyboard control functions (for testing)
- `main.c` - SDL window, LVGL init, main loop

## Running the Simulator

```bash
# 1. Start the backend first
cd backend
python main.py

# 2. Run simulator with backend connection
cd lvgl-simulator-sdl/build
./simulator --backend http://localhost:3000
```

## Sync Script Usage (on local Mac)

```bash
# Full sync from server, build, and run with backend
./sync_and_build.sh --backend

# Connect to specific backend URL
./sync_and_build.sh http://192.168.1.10:3000
```

## Keyboard Controls (Simulator)

For testing purposes, keyboard shortcuts can simulate hardware events:

| Key | Action |
|-----|--------|
| N | Toggle NFC tag present |
| +/= | Increase scale weight by 50g |
| - | Decrease scale weight by 50g |
| H | Show help |
| ESC | Exit simulator |

## Important Rules

1. **Full functionality** - Simulator has complete functionality via backend
2. **Same code everywhere** - UI code is identical on simulator and firmware
3. **No mocks for application logic** - Spool inventory, settings, etc. use real backend
4. **Test thoroughly in simulator** - Before copying to firmware
5. **EEZ files are read-only** - Don't edit `screens.c/h` directly, use EEZ Studio

## Debugging Crashes

If the simulator crashes:
1. Check the log output for which LVGL function failed
2. Verify parent objects exist before creating children
3. Check if the crash happens on a specific screen
4. Compare with firmware behavior if possible
5. Fix the actual bug - don't disable the feature

## Moving Code to Firmware

When UI code is working in simulator:

```bash
# Copy specific file to firmware
cp lvgl-simulator-sdl/ui/ui_nfc_card.c firmware/components/eez_ui/

# Or copy multiple files
cp lvgl-simulator-sdl/ui/ui_*.c firmware/components/eez_ui/
```

Then flash and test on hardware:
```bash
cd firmware
cargo build --release
# Flash to ESP32
```
