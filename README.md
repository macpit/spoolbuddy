# SpoolBuddy

A smart filament management system for Bambu Lab 3D printers.

Based on [SpoolEase](https://github.com/yanshay/SpoolEase) by yanshay.

## Features

- **NFC-based spool identification** - Read NTAG, Mifare Classic, and Bambu Lab RFID tags
- **Weight tracking** - Integrated scale for precise filament measurement
- **Inventory management** - Track all your spools, colors, and remaining filament
- **Automatic printer configuration** - Auto-configure AMS slots via MQTT
- **K-profile management** - Store and restore pressure advance calibration

## Architecture

SpoolBuddy uses a server + device architecture:

- **Server** - Rust backend (Axum) with SQLite database, serving web UI and handling MQTT
- **Web UI** - Preact + TailwindCSS, works on desktop, tablet, and device display
- **Device** - Raspberry Pi Zero 2 W with NFC reader (PN5180) and scale (HX711)

See [SPOOLBUDDY_PLAN.md](SPOOLBUDDY_PLAN.md) for detailed architecture and roadmap.

## Quick Start

### Prerequisites

- Rust (stable)
- Node.js 20+
- SQLite

### Server

```bash
cd server
cargo run
```

Server runs on `http://localhost:3000`

### Web UI (Development)

```bash
cd web
npm install
npm run dev
```

Development server runs on `http://localhost:5173` with API proxy to server.

### Web UI (Production Build)

```bash
cd web
npm run build
```

Built files go to `web/dist/`, served by the Rust server.

## Project Structure

```
spoolbuddy/
├── server/           # Rust backend (Axum)
│   ├── src/
│   │   ├── main.rs
│   │   ├── api/      # REST endpoints
│   │   ├── db/       # SQLite models
│   │   └── websocket/# WebSocket handlers
│   └── Cargo.toml
├── web/              # Preact frontend
│   ├── src/
│   │   ├── pages/    # Route components
│   │   ├── components/
│   │   └── lib/      # API client, WebSocket
│   └── package.json
├── device/           # Pi Zero device service
│   ├── service/      # Python NFC/scale service
│   └── image/        # Device image config
└── docker/           # Docker deployment
```

## API Endpoints

```
GET    /api/spools          - List all spools
POST   /api/spools          - Create spool
GET    /api/spools/:id      - Get spool
PUT    /api/spools/:id      - Update spool
DELETE /api/spools/:id      - Delete spool

GET    /api/printers        - List printers
GET    /api/device/status   - Device connection status
POST   /api/device/tare     - Tare scale

WS     /ws/device           - Device WebSocket
WS     /ws/ui               - UI WebSocket (live updates)
```

## Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `BIND_ADDRESS` | `0.0.0.0:3000` | Server bind address |
| `DATABASE_URL` | `sqlite:spoolbuddy.db?mode=rwc` | SQLite database path |
| `STATIC_DIR` | `../web/dist` | Static files directory |

## License

MIT - Same license as SpoolEase.

## Credits

- [SpoolEase](https://github.com/yanshay/SpoolEase) by yanshay - Original embedded system
- Bambu Lab for their excellent printers and MQTT API documentation
