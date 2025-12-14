# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

SpoolBuddy is a filament management system for Bambu Lab 3D printers, based on [SpoolEase](https://github.com/yanshay/SpoolEase). The original SpoolEase source code is in `spoolease_sources/`. See `SPOOLBUDDY_PLAN.md` for the roadmap of planned modifications.

The system provides:
- NFC-based spool identification (NTAG, Mifare Classic, Bambu Lab RFID tags)
- Weight scale integration for filament tracking
- Inventory management and spool catalog
- MQTT-based automatic AMS slot configuration for X1, P1, A1, H2, P2 product lines

## Repository Structure

```
spoolease_sources/
├── core/           # ESP32-S3 embedded Rust firmware (main application)
│   ├── src/        # Rust source files
│   ├── ui/         # Slint UI definitions (.slint files)
│   ├── static/     # Web assets for embedded server
│   └── data/       # CSV catalogs (brands, materials, spool weights)
└── shared/         # Shared Rust library (NFC, gcode, FTP, etc.)
    └── src/        # Library source files
```

## Build Commands

### Core Firmware (Rust/ESP32-S3)

Requires the ESP Rust toolchain (`esp` channel). Install via [espup](https://github.com/esp-rs/espup).

```bash
cd spoolease_sources/core

# Build release
cargo build --release

# Flash and monitor (16MB flash, DIO mode, 80MHz)
cargo run --release
```

Flash configuration is in `spoolease_sources/core/.cargo/config.toml`.

### Deploy Scripts

In `spoolease_sources/core/`:
- `deploy-beta.sh` - Deploy to beta/unstable OTA channel
- `deploy-rel.sh` - Deploy to release OTA channel
- `deploy-debug.sh` - Debug deployment

These require `esp-hal-app` xtask tooling and `spoolease-bin` directory in parent directories.

## Architecture

### Core Firmware (`spoolease_sources/core/src/`)

A `no_std` embedded Rust application using:
- **esp-hal** ecosystem (esp-hal, esp-wifi, esp-mbedtls, embassy-*)
- **Slint** for touch UI (rendered via software renderer)
- **esp-hal-app-framework** - Custom framework for WiFi, display, settings

Key modules:
- `main.rs` - Entry point, hardware init, embassy task spawning
- `bambu.rs` / `bambu_api.rs` - Printer communication via MQTT
- `view_model.rs` - UI state management (largest file, ~150KB)
- `store.rs` - Persistent storage (sequential-storage on flash)
- `spool_scale.rs` - Scale communication and weight tracking
- `web_app.rs` - Embedded web server (picoserve)
- `my_mqtt.rs` - MQTT client for printer communication
- `csvdb.rs` - CSV data access for catalogs

### Shared Library (`spoolease_sources/shared/src/`)

`no_std` library for reusable components:
- `spool_tag.rs` - NFC tag data encoding/decoding (SpoolEase format)
- `pn532_ext.rs` - PN532 NFC reader async extensions
- `gcode_analysis.rs` / `gcode_analysis_task.rs` - Print file analysis
- `my_ftp.rs` - Async FTP client for printer file access
- `threemf_extractor.rs` - 3MF file parsing with miniz_oxide

### UI Layer

- `spoolease_sources/core/ui/*.slint` - Slint UI definitions
- `spoolease_sources/core/static/` - HTML/CSS served by embedded web server

## Development Notes

- **Target**: `xtensa-esp32s3-none-elf`
- **Toolchain**: `esp` channel (nightly features required)
- **Memory**: Uses PSRAM for heap, DRAM2 for bootloader-shared area
- **TLS certs**: In `core/src/certs/` (Bambu Lab, OTA server)
- **Catalog data**: CSV files in `core/data/` (brands, materials, spool weights)
- **Log level**: Set via `ESP_LOG` env var (default: `info,SpoolEase=trace`)
