# SpoolBuddy

**Smart filament management system for Bambu Lab 3D printers with NFC tagging and weight tracking.**

No cloud dependency. Complete privacy. Full control.

[![GitHub](https://img.shields.io/github/stars/maziggy/spoolbuddy?style=flat-square&label=GitHub)](https://github.com/maziggy/spoolbuddy)
[![License](https://img.shields.io/github/license/maziggy/spoolbuddy?style=flat-square)](https://github.com/maziggy/spoolbuddy/blob/main/LICENSE)
[![Discord](https://img.shields.io/discord/1464629928199847946?style=flat-square&logo=discord&logoColor=white&label=Discord&color=5865F2)](https://discord.gg/3XFdHBkF)

## Quick Start

```bash
mkdir spoolbuddy && cd spoolbuddy
curl -O https://raw.githubusercontent.com/maziggy/spoolbuddy/main/docker-compose.yml
docker compose up -d
```

Open **http://localhost:3000** and connect your SpoolBuddy device.

> **Requirements:** SpoolBuddy hardware device, Bambu Lab printer with Developer Mode enabled, on the same local network.

## Supported Architectures

| Architecture | Tag |
|---|---|
| x86-64 (Intel/AMD) | `amd64` |
| arm64 (Raspberry Pi 4/5) | `arm64` |

## Features

- **NFC Spool Identification** — Read NFC tags on spools instantly (OpenSpool, OpenTag3D, SpoolEase, Bambu Lab RFID)
- **Precision Weight Tracking** — Integrated 5kg scale with 0.1g accuracy shows exactly how much filament remains
- **AMS Integration** — Configure AMS slots directly from the 7" touchscreen or web UI
- **Printer Control** — Real-time AMS status, dual-nozzle (H2D) support, K-profile selection
- **Inventory Management** — Web-based spool catalog with filtering, search, and usage tracking
- **Bambu Cloud Sync** — Import filament presets from Bambu Cloud
- **REST API & WebSocket** — Real-time updates and external tool integration
- **NFC Tag Writing** — Write spool data to NTAG tags for easy identification

## Configuration

| Variable | Default | Description |
|---|---|---|
| `TZ` | `UTC` | Timezone (e.g. `America/New_York`, `Europe/Berlin`) |

## Volumes

| Path | Purpose |
|---|---|
| `/app/data` | Database |

## Docker Compose

```yaml
services:
  spoolbuddy:
    image: ghcr.io/maziggy/spoolbuddy:latest
    container_name: spoolbuddy
    network_mode: host
    environment:
      - TZ=America/New_York
    volumes:
      - spoolbuddy_data:/app/data
    devices:
      - /dev:/dev
    privileged: true
    restart: unless-stopped

volumes:
  spoolbuddy_data:
```

> **macOS/Windows:** Docker Desktop doesn't support `network_mode: host`. Replace it with `ports: ["3000:3000"]` and add printers manually by IP. Device passthrough for serial/NFC may require additional configuration.

## Updating

```bash
docker compose pull && docker compose up -d
```

## Supported Printers

| Series | Models | Status |
|---|---|---|
| H2 | H2D, H2S | Tested |
| X1 | X1, X1 Carbon | Compatible |
| P1 | P1P, P1S | Compatible |
| P2 | P2S | Compatible |
| A1 | A1, A1 Mini | Compatible |

All printers require **Developer Mode** enabled for LAN access.

## Links

- **Website:** [spoolbuddy.cool](https://spoolbuddy.cool)
- **Documentation:** [wiki.spoolbuddy.cool](https://wiki.spoolbuddy.cool)
- **GitHub:** [github.com/maziggy/spoolbuddy](https://github.com/maziggy/spoolbuddy)
- **Discord:** [discord.gg/3XFdHBkF](https://discord.gg/3XFdHBkF)
- **Issues:** [GitHub Issues](https://github.com/maziggy/spoolbuddy/issues)

## License

MIT License - see [LICENSE](https://github.com/maziggy/spoolbuddy/blob/main/LICENSE) for details.
