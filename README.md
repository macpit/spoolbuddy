<p align="center">
  <img src="docs/assets/spoolbuddy_logo_dark.png" alt="SpoolBuddy Logo" width="300">
</p>

<h1 align="center">SpoolBuddy</h1>

<p align="center">
  <strong>Smart filament management system for Bambu Lab 3D printers with NFC tagging and weight tracking</strong>
</p>

<p align="center">
  <a href="https://github.com/maziggy/SpoolStation/releases"><img src="https://img.shields.io/github/v/release/maziggy/SpoolStation?style=flat-square&color=blue" alt="Release"></a>
  <a href="https://github.com/maziggy/SpoolStation/blob/main/LICENSE"><img src="https://img.shields.io/github/license/maziggy/SpoolStation?style=flat-square" alt="License"></a>
  <a href="https://github.com/maziggy/SpoolStation/stargazers"><img src="https://img.shields.io/github/stars/maziggy/SpoolStation?style=flat-square" alt="Stars"></a>
  <a href="https://github.com/maziggy/SpoolStation/issues"><img src="https://img.shields.io/github/issues/maziggy/SpoolStation?style=flat-square" alt="Issues"></a>
  <a href="https://discord.gg/aFS3ZfScHM"><img src="https://img.shields.io/discord/1461241694715645994?style=flat-square&logo=discord&logoColor=white&label=Discord&color=5865F2" alt="Discord"></a>
  <a href="https://ko-fi.com/maziggy"><img src="https://img.shields.io/badge/Ko--fi-Support-ff5e5b?style=flat-square&logo=ko-fi&logoColor=white" alt="Ko-fi"></a>
</p>

<p align="center">
  <a href="#-features">Features</a> •
  <a href="#-hardware">Hardware</a> •
  <a href="#-quick-start">Quick Start</a> •
  <a href="#-contributing">Contributing</a> •
  <a href="https://discord.gg/aFS3ZfScHM">Discord</a>
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
- ESP32-S3 based touchscreen display
- Integrated precision scale (0.1g accuracy)
- NFC reader for spool identification
- WiFi connectivity to backend server
- Compact form factor sits under your spool

### 🏷️ NFC Tag Support
- Read/write NFC tags on spools
- Multiple tag formats supported:
  - OpenSpool
  - OpenTag3D
  - SpoolEase
  - Bambu Lab RFID
- Auto-detect tag format
- Write spool data to blank tags

### ⚖️ Weight Tracking
- Real-time weight display
- Automatic weight updates when spool placed
- Core weight calibration per spool type
- Remaining filament calculation
- History of weight changes

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

SpoolBuddy consists of:

| Component | Description |
|-----------|-------------|
| **ESP32-S3 Display** | 4" touchscreen with WiFi |
| **Load Cell** | HX711-based precision scale |
| **NFC Reader** | PN532 for tag read/write |
| **Enclosure** | 3D printed housing |

### Bill of Materials

*Coming soon — Hardware documentation in progress*

---

## 🚀 Quick Start

### Requirements

- Python 3.10+ (3.11/3.12 recommended)
- Node.js 18+ (for frontend development)
- Bambu Lab printer with **Developer Mode** enabled
- SpoolBuddy hardware device

### Backend Installation

```bash
# Clone repository
git clone https://github.com/maziggy/SpoolStation.git
cd SpoolStation

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

See the `firmware/` directory for ESP32 firmware source code and flashing instructions.

---

## 🛠️ Tech Stack

| Component | Technology |
|-----------|------------|
| Backend | Python, FastAPI, SQLite |
| Frontend | Preact, TypeScript, Tailwind CSS |
| Firmware | Rust, ESP-IDF, LVGL |
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

## 🤝 Contributing

Contributions welcome! Here's how to help:

1. **Test** — Report issues with your printer model
2. **Hardware** — Improve enclosure designs
3. **Code** — Submit PRs for bugs or features
4. **Document** — Improve guides and documentation

```bash
# Development setup
git clone https://github.com/maziggy/SpoolStation.git
cd SpoolStation

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

---

<p align="center">
  Made with ❤️ for the 3D printing community
  <br><br>
  <a href="https://discord.gg/aFS3ZfScHM">Join our Discord</a> •
  <a href="https://github.com/maziggy/SpoolStation/issues">Report Bug</a> •
  <a href="https://github.com/maziggy/SpoolStation/issues">Request Feature</a>
</p>
