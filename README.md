<p align="center">
  <img src="docs/assets/spoolbuddy_logo_dark.png" alt="SpoolBuddy Logo" width="300">
</p>

<h1 align="center">SpoolBuddy</h1>

<p align="center">
  <strong>Smart filament management system for Bambu Lab 3D printers with NFC tagging and weight tracking</strong>
</p>

<p align="center">
  <a href="#-features">Features</a> •
  <a href="#-hardware">Hardware</a> •
  <a href="#-quick-start">Quick Start</a> •
  <a href="https://wiki.spoolbuddy.cool">Documentation</a> •
  <a href="#-contributing">Contributing</a> •
</p>

---

## Why SpoolBuddy?

- **Know your filament** — NFC tags identify spools instantly, no more guessing
- **Track remaining weight** — Precision scale shows exactly how much is left
- **Seamless AMS integration** — Configure AMS slots directly from the display
- **Works offline** — Uses Developer Mode for direct printer control via local network

---

## ✨ Features

<table>
<tr>
<td width="50%" valign="top">

### 📱 Hardware Device
- 7" IPS touchscreen display (800x480)
- Integrated precision scale (5kg capacity, 0.1g accuracy)
- NFC reader for spool identification
- WiFi connectivity to backend server
- Compact form factor sits under your spool

### 🏷️ NFC Tag Support
- Read NFC tags on spools
- Multiple tag formats supported:
  - OpenSpool
  - OpenTag3D
  - SpoolEase
  - Bambu Lab RFID (ISO 15693)
- Auto-detect tag format
- Write spool data to NTAG tags

### ⚖️ Weight Tracking
- Real-time weight display
- Automatic weight updates when spool placed
- Core weight calibration per spool type
- Remaining filament calculation

</td>
<td width="50%" valign="top">

### 🖨️ Printer Integration
- MQTT connection to Bambu Lab printers
- Real-time AMS status visualization
- Configure AMS slots from display or web UI
- Support for regular AMS and AMS HT
- Dual-nozzle (H2D) support
- K-profile (pressure advance) selection

### 📊 Inventory Management
- Web-based spool catalog
- Filter by material, brand, color
- Track spool usage and remaining weight
- Link spools to AMS slots
- Import presets from Bambu Cloud

### 🔧 Integration Ready
- REST API for external tools
- WebSocket for real-time updates
- Works with Bambuddy for full print management
- Bambu Cloud profile sync

</td>
</tr>
</table>

---

## 🔧 Hardware

SpoolBuddy requires these components (~$100-150 total):

| Component | Description |
|-----------|-------------|
| **Elecrow CrowPanel 7.0" Advance** | ESP32-S3 display with 800x480 IPS touchscreen |
| **Raspberry Pi Pico** | NFC bridge controller |
| **PN5180 NFC Reader** | ISO 15693 support for Bambu Lab tags (3.3V only!) |
| **SparkFun Qwiic NAU7802** | 24-bit scale ADC |
| **5kg Load Cell** | Weight sensor |
| **22AWG Silicone Wire** | Wiring connections |
| **M4x25 Screws** | Mounting hardware |

> **Warning:** The PN5180 is 3.3V only — 5V will damage it! RC522 readers are not compatible.

### Bill of Materials

See the [Hardware](https://wiki.spoolbuddy.cool/getting-started/hardware/) page for the complete BOM with purchase links.

---

## 🚀 Quick Start

### Requirements

- Python 3.10+ (3.11/3.12 recommended)
- Node.js 18+ (for frontend development)
- Bambu Lab printer with **Developer Mode** enabled
- SpoolBuddy hardware device

### Docker (Recommended)

```bash
mkdir spoolbuddy && cd spoolbuddy
curl -O https://raw.githubusercontent.com/maziggy/spoolbuddy/main/docker-compose.yml
docker compose up -d
```

Open **http://localhost:3000** in your browser.

> **Multi-architecture support:** Pre-built images are available for `linux/amd64` and `linux/arm64` (Raspberry Pi 4/5).

### Manual Installation

```bash
# Clone repository
git clone https://github.com/maziggy/spoolbuddy.git
cd spoolbuddy

# Backend setup
cd backend
python3 -m venv venv
source venv/bin/activate  # Windows: venv\Scripts\activate
pip install -r requirements.txt

# Run server
python main.py
```

Open **http://localhost:3000** in your browser.

### Frontend Development

```bash
cd frontend
npm install
npm run dev
```

### Firmware

SpoolBuddy uses two firmware components:

| Device | Format | Description |
|--------|--------|-------------|
| **Raspberry Pi Pico** | `.uf2` (Arduino) | NFC bridge controller |
| **CrowPanel 7.0"** | `.bin` (Rust/ESP-IDF) | Display, scale, WiFi |

**Quick flash (pre-built):**
```bash
# Pico: Hold BOOTSEL, plug USB, drag pico-nfc-bridge.uf2 to RPI-RP2 drive

# CrowPanel:
cargo install espflash
espflash flash --monitor spoolbuddy-firmware.bin
```

See the [Firmware Guide](https://wiki.spoolbuddy.cool/getting-started/firmware/) for detailed instructions.

---

## 🛠️ Tech Stack

| Component | Technology |
|-----------|------------|
| Backend | Python, FastAPI, SQLite |
| Frontend | Preact, TypeScript, Tailwind CSS |
| Display Firmware | Rust, ESP-IDF, LVGL |
| NFC Bridge | Arduino (Pico) |
| Communication | MQTT (TLS), WebSocket, REST |

---

## 🖨️ Supported Printers

| Series | Models |
|--------|--------|
| H2 | H2D, H2S |
| X1 | X1, X1 Carbon |
| P1 | P1P, P1S, P2S |
| A1 | A1, A1 Mini |

---

## 📚 Documentation

Full documentation available at **[wiki.spoolbuddy.cool](https://wiki.spoolbuddy.cool)**:

- [Getting Started](https://wiki.spoolbuddy.cool/getting-started/) — Hardware setup and installation
- [Hardware Required](https://wiki.spoolbuddy.cool/getting-started/hardware-required/) — Bill of materials
- [Firmware](https://wiki.spoolbuddy.cool/getting-started/firmware/) — Flashing instructions
- [Software](https://wiki.spoolbuddy.cool/getting-started/software/) — Backend setup

---

## 🤝 Contributing

Contributions welcome! Here's how to help:

1. **Test** — Report issues with your printer model
2. **Hardware** — Improve enclosure designs
3. **Code** — Submit PRs for bugs or features
4. **Document** — Improve guides and documentation

```bash
# Development setup
git clone https://github.com/maziggy/spoolbuddy.git
cd spoolbuddy

# Backend
cd backend
python3 -m venv venv && source venv/bin/activate
pip install -r requirements.txt
python main.py

# Frontend (separate terminal)
cd frontend && npm install && npm run dev
```

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

---

## 📄 License

MIT License — see [LICENSE](LICENSE) for details.

---

## 🙏 Acknowledgments

- [SpoolEase](https://github.com/yanshay/SpoolEase) by yanshay — Original embedded system inspiration
- [Bambu Lab](https://bambulab.com/) for amazing printers
- [OpenSpool](https://github.com/spuder/OpenSpool) for NFC tag format inspiration
- [Bambuddy](https://github.com/maziggy/bambuddy) for printer integration patterns
- The reverse engineering community for protocol documentation
