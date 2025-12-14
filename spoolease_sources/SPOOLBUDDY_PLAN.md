# SpoolBuddy - Project Plan

> A smart filament management system for Bambu Lab 3D printers.
> Based on [SpoolEase](https://github.com/yanshay/SpoolEase) by yanshay.

---

## Table of Contents

1. [Project Overview](#project-overview)
2. [Architecture](#architecture)
3. [Hardware](#hardware)
4. [Software Components](#software-components)
5. [Development Phases](#development-phases)
6. [Technical Details](#technical-details)
7. [Upstream Sync Strategy](#upstream-sync-strategy)

---

## Project Overview

### What is SpoolBuddy?

SpoolBuddy is a reimagined filament management system that combines:
- **NFC-based spool identification** - Read/write tags on filament spools
- **Weight tracking** - Integrated scale for precise filament measurement
- **Inventory management** - Track all your spools, usage, and K-profiles
- **Automatic printer configuration** - Auto-configure AMS slots via MQTT

### Key Differences from SpoolEase

| Aspect | SpoolEase | SpoolBuddy |
|--------|-----------|--------------|
| Architecture | Embedded (ESP32) | Server + Device |
| Display | 3.5" embedded | 5" via web UI |
| Console + Scale | Separate devices | Combined unit |
| UI Framework | Slint (embedded) | Web (Preact) |
| Updates | Firmware flash | Server update |
| Database | CSV on SD card | SQLite |

### Goals

1. **Modern UI** - Professional web-based interface
2. **Easy updates** - Change server, not firmware
3. **Multi-device** - Same UI on device, tablet, browser
4. **Maintainable** - Standard web stack, easier development
5. **Feature parity** - All SpoolEase features, then extend

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         SERVER (Docker)                          │
│                                                                  │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌───────────┐  │
│  │ Rust Backend│ │  Web UI     │ │  Database   │ │ NFS Export│  │
│  │   (Axum)    │ │  (Preact)   │ │  (SQLite)   │ │           │  │
│  │             │ │             │ │             │ │ Device    │  │
│  │ • MQTT      │ │ • Inventory │ │ • Spools    │ │ rootfs    │  │
│  │ • FTP/Gcode │ │ • Printers  │ │ • Printers  │ │           │  │
│  │ • REST API  │ │ • Dashboard │ │ • K-Values  │ │           │  │
│  │ • WebSocket │ │ • Settings  │ │ • History   │ │           │  │
│  └──────┬──────┘ └──────┬──────┘ └─────────────┘ └─────┬─────┘  │
│         │               │                              │        │
│         └───────────────┼──────────────────────────────┘        │
│                         │                                        │
└─────────────────────────┼────────────────────────────────────────┘
                          │
           ┌──────────────┼──────────────┐
           │ HTTP/WS      │ WebSocket    │ NFS
           ▼              ▼              ▼
    ┌───────────┐  ┌───────────┐  ┌─────────────────────────────┐
    │  Browser  │  │  Tablet   │  │     SpoolBuddy Device     │
    │           │  │           │  │                             │
    │  Web UI   │  │  Web UI   │  │  ┌─────────────────────┐    │
    │           │  │           │  │  │  Raspberry Pi Zero  │    │
    └───────────┘  └───────────┘  │  │       2 W           │    │
                                  │  │                     │    │
                                  │  │  Boot: SD (r/o)     │    │
                                  │  │  Root: NFS mount    │    │
                                  │  │  UI: Chromium kiosk │    │
                                  │  │                     │    │
                                  │  │  GPIO:              │    │
                                  │  │  ├── PN532 (SPI)    │    │
                                  │  │  └── HX711 (GPIO)   │    │
                                  │  └─────────────────────┘    │
                                  │                             │
                                  │  ┌───────┐ ┌───────┐ ┌───┐  │
                                  │  │Display│ │ NFC   │ │Scale│ │
                                  │  │ 5"    │ │Reader │ │     │ │
                                  │  └───────┘ └───────┘ └───┘  │
                                  └─────────────────────────────┘
```

### Communication Flow

```
Device                          Server
  │                               │
  │◄──── NFS mount (rootfs) ─────►│
  │                               │
  │◄──── WebSocket ──────────────►│
  │      • Tag detected           │
  │      • Weight changed         │
  │      • Commands (write tag)   │
  │                               │
  │◄──── HTTP (Chromium) ────────►│
  │      • Web UI                 │
  │                               │
```

---

## Hardware

### Device Components

| Component | Choice | Interface | Notes |
|-----------|--------|-----------|-------|
| **SBC** | Raspberry Pi Zero 2 W | - | WiFi, GPIO, low power |
| **Display** | Waveshare 5" HDMI (Model B) | Mini-HDMI | 800x480, capacitive touch |
| **NFC Reader** | PN532 module | SPI | Under scale platform |
| **Scale** | HX711 + Load Cell | GPIO | Standard load cell setup |
| **Power** | USB-C 5V/2A | - | Single power input |

### GPIO Pin Allocation

```
Raspberry Pi Zero 2 W GPIO:

PN532 (SPI):
  - MOSI: GPIO 10 (Pin 19)
  - MISO: GPIO 9 (Pin 21)
  - SCLK: GPIO 11 (Pin 23)
  - CS:   GPIO 8 (Pin 24)
  - IRQ:  GPIO 25 (Pin 22) [optional]

HX711 (Scale):
  - DT:   GPIO 5 (Pin 29)
  - SCK:  GPIO 6 (Pin 31)

Display:
  - HDMI (no GPIO needed)
  - Touch via USB
```

### Physical Design

- Combined Console + Scale in single case
- NFC antenna positioned under scale platform center
- Spool sits on platform, center hole aligns with NFC reader
- 5" display angled for visibility
- Single USB-C power input

---

## Software Components

### 1. Server Backend (Rust)

**Framework:** Axum

**Responsibilities:**
- REST API for web UI
- WebSocket for device communication
- MQTT client for Bambu Lab printers
- FTPS client for G-code file access
- G-code analysis for filament usage
- Database operations (SQLite)
- NFS export for device rootfs

**Portable code from SpoolEase:**
- `bambu_api.rs` - MQTT message structures (direct copy)
- `gcode_analysis.rs` - G-code parsing (direct copy)
- `threemf_extractor.rs` - 3MF handling (direct copy)
- `ndef.rs` - NDEF message handling (adapt)
- Business logic from `bambu.rs` (reimplement)
- Store logic from `store.rs` (reimplement for SQLite)

**New implementations needed:**
- Axum web server and routes
- SQLite database layer
- MQTT client (using `rumqttc`)
- FTP client (using `suppaftp`)
- WebSocket handler for devices
- NFS server configuration

### 2. Web UI (Preact + TypeScript)

**Framework:** Preact + Vite + TailwindCSS

**Pages:**
- **Dashboard** - Overview, printer status, current print
- **Inventory** - Spool list, search, filter (already started!)
- **Printers** - Printer configuration, status
- **Spool Detail** - Edit spool, K-profiles, history
- **Settings** - Server config, WiFi, display settings
- **Scale Calibration** - Tare, calibrate scale

**Shared with device:**
- Same codebase serves all clients
- Responsive design (desktop, tablet, device)
- Device-specific views (e.g., simplified for 5" screen)

### 3. Device Service (Python)

**Responsibilities:**
- Read NFC tags (PN532 via SPI)
- Read scale weight (HX711)
- Send data to server via WebSocket
- Receive commands (write NFC, tare scale)
- Local caching if server offline

**Libraries:**
- `py532lib` or `pn532pi` - PN532 NFC
- `hx711` - HX711 scale ADC
- `websockets` - WebSocket client
- `RPi.GPIO` or `gpiozero` - GPIO access

**Structure:**
```
device-service/
├── main.py           # Entry point, main loop
├── nfc_reader.py     # PN532 interface
├── scale.py          # HX711 interface
├── websocket.py      # Server communication
├── config.py         # Configuration
└── requirements.txt
```

### 4. Device System Image

**Base:** Raspberry Pi OS Lite (64-bit)

**Boot Configuration:**
- Minimal SD card (~50MB, read-only)
- Kernel + initramfs + boot config only
- Root filesystem via NFS

**Runtime:**
- Chromium in kiosk mode (full screen, no UI chrome)
- Device service (systemd)
- Auto-connect to server WiFi
- Watchdog for reliability

**NFS Root Setup:**
```
Server exports:
  /srv/spoolbuddy/rootfs  →  Device mounts as /

Device /etc/fstab (in initramfs):
  server:/srv/spoolbuddy/rootfs / nfs defaults 0 0
```

---

## Development Phases

### Phase 1: Foundation (MVP)

**Goal:** Basic working system, prove architecture

**Server:**
- [ ] Project setup (Cargo workspace)
- [ ] Basic Axum server with REST API
- [ ] SQLite database schema and migrations
- [ ] Spool CRUD operations
- [ ] WebSocket endpoint for devices
- [ ] Static file serving for web UI

**Web UI:**
- [ ] Extend existing inventory UI
- [ ] Add spool detail/edit page
- [ ] Add basic dashboard
- [ ] WebSocket integration for live updates

**Device:**
- [ ] RPi image with Chromium kiosk
- [ ] Basic Python service (NFC read, scale read)
- [ ] WebSocket connection to server
- [ ] NFS root setup

**Deliverable:** Can view/edit spools, read NFC tags, read weight

### Phase 2: Printer Integration

**Goal:** Connect to Bambu Lab printers

**Server:**
- [ ] Port `bambu_api.rs` structures
- [ ] MQTT client for printer communication
- [ ] Printer discovery (SSDP)
- [ ] Printer state tracking
- [ ] AMS slot configuration
- [ ] Tag information encoding/decoding

**Web UI:**
- [ ] Printer management page
- [ ] Printer status display
- [ ] AMS slot visualization

**Deliverable:** Auto-detect printers, show status, configure slots

### Phase 3: Filament Tracking

**Goal:** Track filament usage during prints

**Server:**
- [ ] Port `gcode_analysis.rs`
- [ ] Port `threemf_extractor.rs`
- [ ] FTP client for printer file access
- [ ] Real-time usage tracking during print
- [ ] Consumption history per spool

**Web UI:**
- [ ] Print progress display
- [ ] Usage history graphs
- [ ] Low stock warnings

**Deliverable:** Accurate filament tracking, usage history

### Phase 4: K-Profile Management

**Goal:** Pressure advance calibration management

**Server:**
- [ ] K-profile storage per spool/printer/nozzle
- [ ] Auto-restore K values when loading spool
- [ ] Import K values from printer

**Web UI:**
- [ ] K-profile editor
- [ ] Per-printer/nozzle configuration

**Deliverable:** Full pressure advance management

### Phase 5: NFC Writing & Advanced Features

**Goal:** Complete feature parity + extras

**Server:**
- [ ] NFC tag writing commands
- [ ] Tag format support (SpoolEase, Bambu, OpenPrint)
- [ ] Backup/restore functionality
- [ ] Multi-user support (optional)

**Device:**
- [ ] NFC write implementation
- [ ] Scale calibration UI
- [ ] Offline mode with sync

**Web UI:**
- [ ] Tag encoding page
- [ ] Backup/restore UI
- [ ] Settings page

**Deliverable:** Full SpoolEase feature parity

### Phase 6: Polish & Documentation

**Goal:** Production ready

- [ ] Error handling and edge cases
- [ ] Performance optimization
- [ ] User documentation
- [ ] Installation guide
- [ ] Docker compose setup
- [ ] Device image builder

---

## Technical Details

### Database Schema (SQLite)

```sql
-- Spools table
CREATE TABLE spools (
    id TEXT PRIMARY KEY,
    tag_id TEXT UNIQUE,
    material TEXT NOT NULL,
    subtype TEXT,
    color_name TEXT,
    rgba TEXT,
    brand TEXT,
    label_weight INTEGER DEFAULT 1000,
    core_weight INTEGER DEFAULT 250,
    weight_new INTEGER,
    weight_current INTEGER,
    slicer_filament TEXT,
    note TEXT,
    added_time INTEGER,
    encode_time INTEGER,
    added_full BOOLEAN DEFAULT FALSE,
    consumed_since_add REAL DEFAULT 0,
    consumed_since_weight REAL DEFAULT 0,
    data_origin TEXT,
    tag_type TEXT,
    created_at INTEGER DEFAULT (strftime('%s', 'now')),
    updated_at INTEGER DEFAULT (strftime('%s', 'now'))
);

-- Printers table
CREATE TABLE printers (
    serial TEXT PRIMARY KEY,
    name TEXT,
    model TEXT,
    ip_address TEXT,
    access_code TEXT,
    last_seen INTEGER,
    config JSON
);

-- K-Profiles table
CREATE TABLE k_profiles (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    spool_id TEXT REFERENCES spools(id),
    printer_serial TEXT REFERENCES printers(serial),
    extruder INTEGER,
    nozzle_diameter TEXT,
    nozzle_type TEXT,
    k_value TEXT,
    name TEXT,
    cali_idx INTEGER,
    setting_id TEXT,
    created_at INTEGER DEFAULT (strftime('%s', 'now'))
);

-- Usage history table
CREATE TABLE usage_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    spool_id TEXT REFERENCES spools(id),
    printer_serial TEXT,
    print_name TEXT,
    weight_used REAL,
    timestamp INTEGER DEFAULT (strftime('%s', 'now'))
);
```

### WebSocket Protocol

**Device → Server:**

```json
// Tag detected
{
    "type": "tag_detected",
    "tag_id": "04:AB:CD:EF:12:34:56",
    "tag_type": "ntag215",
    "data": { /* parsed tag data */ }
}

// Tag removed
{
    "type": "tag_removed"
}

// Weight update
{
    "type": "weight",
    "grams": 1234.5,
    "stable": true
}

// Heartbeat
{
    "type": "heartbeat",
    "uptime": 12345
}
```

**Server → Device:**

```json
// Write tag command
{
    "type": "write_tag",
    "request_id": "abc123",
    "data": { /* tag data to write */ }
}

// Tare scale
{
    "type": "tare_scale"
}

// Calibrate scale
{
    "type": "calibrate_scale",
    "known_weight": 500
}

// Show notification on device
{
    "type": "notification",
    "message": "Spool loaded: PLA Red",
    "duration": 3000
}
```

### REST API Endpoints

```
GET    /api/spools              - List all spools
POST   /api/spools              - Create spool
GET    /api/spools/:id          - Get spool
PUT    /api/spools/:id          - Update spool
DELETE /api/spools/:id          - Delete spool

GET    /api/printers            - List printers
POST   /api/printers            - Add printer
GET    /api/printers/:serial    - Get printer
PUT    /api/printers/:serial    - Update printer
DELETE /api/printers/:serial    - Remove printer

GET    /api/k-profiles/:spool   - Get K-profiles for spool
POST   /api/k-profiles          - Save K-profile
DELETE /api/k-profiles/:id      - Delete K-profile

GET    /api/device/status       - Device connection status
POST   /api/device/tare         - Tare scale
POST   /api/device/write-tag    - Write NFC tag

WS     /ws/device               - Device WebSocket
WS     /ws/ui                   - UI WebSocket (live updates)
```

### Project Structure

```
spoolbuddy/
├── server/
│   ├── Cargo.toml
│   ├── src/
│   │   ├── main.rs
│   │   ├── api/
│   │   │   ├── mod.rs
│   │   │   ├── spools.rs
│   │   │   ├── printers.rs
│   │   │   └── device.rs
│   │   ├── db/
│   │   │   ├── mod.rs
│   │   │   ├── schema.rs
│   │   │   └── queries.rs
│   │   ├── mqtt/
│   │   │   ├── mod.rs
│   │   │   ├── client.rs
│   │   │   └── bambu_api.rs  # Ported from SpoolEase
│   │   ├── gcode/
│   │   │   ├── mod.rs
│   │   │   ├── analysis.rs    # Ported from SpoolEase
│   │   │   └── threemf.rs     # Ported from SpoolEase
│   │   ├── websocket/
│   │   │   ├── mod.rs
│   │   │   ├── device.rs
│   │   │   └── ui.rs
│   │   └── config.rs
│   └── migrations/
│       └── 001_initial.sql
│
├── web/
│   ├── package.json
│   ├── vite.config.ts
│   ├── src/
│   │   ├── main.tsx
│   │   ├── App.tsx
│   │   ├── components/
│   │   ├── pages/
│   │   ├── lib/
│   │   └── hooks/
│   └── public/
│
├── device/
│   ├── service/
│   │   ├── main.py
│   │   ├── nfc_reader.py
│   │   ├── scale.py
│   │   ├── websocket.py
│   │   └── requirements.txt
│   ├── image/
│   │   ├── build.sh
│   │   ├── config.txt
│   │   └── kiosk.service
│   └── README.md
│
├── docker/
│   ├── Dockerfile.server
│   ├── docker-compose.yml
│   └── nfs-exports
│
├── docs/
│   ├── setup.md
│   ├── hardware.md
│   └── api.md
│
├── SPOOLBUDDY_PLAN.md  # This file
├── LICENSE               # MIT (same as SpoolEase)
└── README.md
```

---

## Upstream Sync Strategy

### Relationship to SpoolEase

SpoolBuddy is **based on SpoolEase** but is a separate project with different architecture. We will:

1. **Credit SpoolEase** prominently in README and About page
2. **Use MIT license** (same as SpoolEase)
3. **Watch upstream** for relevant changes
4. **Reimplement** changes that apply to our architecture

### What to Watch

| SpoolEase File | SpoolBuddy Impact | Action |
|----------------|---------------------|--------|
| `bambu_api.rs` | Direct impact | Copy/adapt changes |
| `gcode_analysis.rs` | Direct impact | Copy/adapt changes |
| `bambu.rs` (state machine) | Logic changes | Reimplement |
| `store.rs` | Data model changes | Adapt for SQLite |
| `spool_tag.rs` | Tag format changes | Reimplement |
| UI files | No impact | Ignore |
| Hardware drivers | No impact | Ignore |

### Process

1. **Weekly check** of SpoolEase commits
2. **Evaluate** relevance to SpoolBuddy
3. **Document** in changelog what was synced
4. **Test** thoroughly after any sync

---

## Next Steps

1. **Create repository** - `github.com/<user>/spoolbuddy`
2. **Set up project structure** - As defined above
3. **Start Phase 1** - Foundation/MVP
4. **Order hardware** - RPi Zero 2 W, display, components

---

*Document created: December 2024*
*Based on SpoolEase by yanshay*
