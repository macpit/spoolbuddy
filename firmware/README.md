# SpoolBuddy Firmware

ESP32-S3 firmware for the SpoolBuddy device.

## Hardware

- **Board**: Waveshare ESP32-S3-Touch-LCD-4.3
- **Display**: 4.3" 800x480 IPS RGB parallel interface (DPI)
- **Touch**: GT911 capacitive touch controller (I2C)
- **IO Expander**: CH422G (I2C) - controls backlight, LCD reset, touch reset
- **NFC Reader**: PN5180 (SPI)
- **Scale**: HX711 + Load Cell (GPIO)

### USB Ports

The board has **two USB-C ports**:
- **Bottom port**: USB-UART/JTAG - Use this for flashing and serial monitor
- **Top port**: USB-OTG - For USB device mode (not used for development)

## Testing on Standalone ESP32-S3

You can test the core firmware on any ESP32-S3 board (without the display) while waiting for the Waveshare LCD. The firmware will:
- Connect to WiFi
- Log status to serial console
- Read NFC tags (if PN5180 connected)
- Read scale values (if HX711 connected)

The display UI won't render, but all other functionality works.

## Prerequisites

1. Install Rust (if not already installed):
   ```bash
   curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
   source $HOME/.cargo/env
   ```

2. Install the ESP Rust toolchain via espup:
   ```bash
   # Install espup (ESP Rust toolchain installer)
   cargo install espup --locked

   # Install the ESP toolchain (this takes several minutes)
   espup install

   # Source the environment (add this line to your shell profile)
   . $HOME/export-esp.sh
   ```

3. Install espflash for flashing:
   ```bash
   cargo install espflash
   ```

4. (Linux only) Add udev rules for USB access:
   ```bash
   sudo usermod -a -G dialout $USER
   # Log out and back in for group change to take effect
   ```

## Building

```bash
cd firmware

# Ensure ESP environment is sourced
. $HOME/export-esp.sh

# Build release version
cargo build --release

# Build and flash (with serial monitor)
cargo run --release
```

## Project Structure

```
firmware/
├── Cargo.toml          # Dependencies and project config
├── build.rs            # Build script
├── rust-toolchain.toml # Toolchain specification
├── .cargo/
│   └── config.toml     # Cargo config (target, runner)
└── src/
    ├── main.rs         # Entry point, initialization
    ├── wifi.rs         # WiFi connection management
    ├── nfc/
    │   ├── mod.rs      # NFC reader abstraction
    │   └── pn5180.rs   # PN5180 driver
    ├── scale/
    │   ├── mod.rs      # Scale abstraction
    │   └── hx711.rs    # HX711 driver
    └── ui/
        └── mod.rs      # Display and touch UI
```

## Pin Configuration

| Function | GPIO | Notes |
|----------|------|-------|
| **PN5180 (SPI)** | | |
| MOSI | GPIO11 | SPI data out |
| MISO | GPIO13 | SPI data in |
| SCLK | GPIO12 | SPI clock |
| NSS | GPIO10 | Chip select |
| BUSY | GPIO14 | Busy indicator |
| RST | GPIO21 | Reset |
| **HX711 (Scale)** | | |
| DOUT | GPIO1 | Data out |
| SCK | GPIO2 | Clock |

*Note: Pin assignments are preliminary - verify against actual Waveshare pinout.*

## Features

- [x] Project structure
- [ ] WiFi connection
- [ ] WebSocket client
- [ ] PN5180 NFC reading
- [ ] HX711 scale reading
- [ ] Display UI (LVGL)
- [ ] Touch input
- [ ] NFC tag writing

## Development

The firmware uses:
- `esp-hal` for hardware abstraction
- `embassy` for async runtime
- `embedded-graphics` for display rendering

For debugging, connect via USB and use:
```bash
espflash monitor
```

## Troubleshooting

### "rust-src component not found" or "Cargo.lock does not exist"
This means the ESP toolchain isn't properly installed or sourced:
```bash
# Re-run espup install
espup install

# Make sure to source the environment
. $HOME/export-esp.sh
```

### "Permission denied" when flashing (Linux)
```bash
sudo usermod -a -G dialout $USER
# Log out and back in
```

### Build takes a long time
The first build compiles the entire esp-hal stack. Subsequent builds are much faster. Using release mode (`--release`) is recommended.

### "Error: Unable to find USB device"
Make sure:
1. The ESP32-S3 is connected via USB
2. You have the correct permissions (see Linux note above)
3. Try holding BOOT button while pressing RESET to enter bootloader mode
