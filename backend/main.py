import asyncio
import csv
import json
import socket
import logging
import time
from contextlib import asynccontextmanager
from pathlib import Path
from typing import Set, Optional, Dict, Tuple

from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from zeroconf.asyncio import AsyncZeroconf
from zeroconf import ServiceInfo
from fastapi.staticfiles import StaticFiles
from fastapi.middleware.cors import CORSMiddleware

from config import settings
from db import get_db
from mqtt import PrinterManager
from api import spools_router, printers_router, updates_router, firmware_router, tags_router, device_router, serial_router, discovery_router, catalog_router
from api.printers import set_printer_manager
from api.cloud import router as cloud_router
from models import PrinterState
from tags import TagDecoder, SpoolEaseEncoder
from usage_tracker import UsageTracker, estimate_weight_from_percent

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s"
)
logger = logging.getLogger(__name__)

# === Bambu Color Name Lookup ===
# Maps (material_id, color_rgba_hex) -> color_name from bambu-color-names.csv
_bambu_color_map: Dict[Tuple[str, str], str] = {}


def _load_bambu_color_map():
    """Load Bambu color name mappings from CSV file."""
    global _bambu_color_map
    csv_path = Path(__file__).parent.parent / "spoolease_sources" / "core" / "data" / "bambu-color-names.csv"
    if not csv_path.exists():
        logger.warning(f"Bambu color names CSV not found at {csv_path}")
        return
    try:
        with open(csv_path, "r") as f:
            reader = csv.reader(f)
            for row in reader:
                if len(row) >= 3:
                    material_id = row[0].strip()
                    color_rgba = row[1].strip().upper()
                    color_name = row[2].strip()
                    # Handle dual-color entries (e.g., "FFFFFFFF/9CDBD9FF")
                    for rgba in color_rgba.split("/"):
                        _bambu_color_map[(material_id, rgba)] = color_name
        logger.info(f"Loaded {len(_bambu_color_map)} Bambu color mappings")
    except Exception as e:
        logger.warning(f"Failed to load Bambu color names: {e}")


def lookup_bambu_color_name(material_id: str, color_rgba: int) -> Optional[str]:
    """Look up Bambu color name from material_id and RGBA color value.

    Args:
        material_id: Material ID like "GFA00", or slicer filament name like "Bambu PLA Basic"
        color_rgba: RGBA color value as integer (e.g., 0xA6A9AAFF)

    Returns:
        Color name like "Silver" if found, None otherwise
    """
    if not material_id or color_rgba == 0:
        return None

    # Convert RGBA integer to hex string (uppercase, 8 chars)
    rgba_hex = f"{color_rgba:08X}"

    # Try direct lookup
    result = _bambu_color_map.get((material_id, rgba_hex))
    if result:
        return result

    # material_id might be a full name like "Bambu PLA Basic" - need to check all GFAxx entries
    # This is a fallback for when we don't have the original material_id code
    if material_id.startswith("Bambu "):
        # Try all entries that match this color
        for (mat_id, rgba), name in _bambu_color_map.items():
            if rgba == rgba_hex:
                return name

    return None


# Load color map at module import
_load_bambu_color_map()

# Global state
printer_manager = PrinterManager()
websocket_clients: Set[WebSocket] = set()
usage_tracker = UsageTracker()
# Track previous printer states for comparison
_previous_states: Dict[str, PrinterState] = {}
# mDNS service for device discovery
_zeroconf: Optional[AsyncZeroconf] = None
_mdns_service: Optional[ServiceInfo] = None
# Track ESP32 display connection (last seen timestamp)
_display_last_seen: float = 0
_display_connected: bool = False
DISPLAY_TIMEOUT_SEC = 10  # Consider disconnected after 10s of no requests
# Pending commands for display (checked on heartbeat)
_display_pending_command: Optional[str] = None
# Device firmware version (reported by device in heartbeat)
_display_firmware_version: Optional[str] = None
# Device reports update is available
_device_update_available: bool = False
# Device state (weight, tag) - updated by WebSocket messages from device
_device_last_weight: Optional[float] = None
_device_weight_stable: bool = False

# === Tag Staging System ===
# When a tag is detected, it goes to "staging" for 30 seconds.
# This allows the user to interact with the tag even if NFC reads are flaky.
# Staging only clears on: timeout, different tag detected, or manual clear.
STAGING_TIMEOUT = 30  # seconds
_staged_tag_id: Optional[str] = None
_staged_tag_data: Optional[Dict] = None
_staged_tag_timestamp: float = 0  # time.time() when staged

# Cache of decoded tag data (persists even after staging is cleared)
# This allows re-staging a tag even if ESP32 only sends tag_id without decoded data
_tag_data_cache: Dict[str, Dict] = {}

# Blocked tags - after manual clear, block the tag for a few seconds
# to prevent immediate re-staging from ESP32's continuous detection
_blocked_tag_id: Optional[str] = None
_blocked_until: float = 0
BLOCK_DURATION = 5  # seconds to block after manual clear

# Legacy: keep for backwards compat with existing code
_device_current_tag_id: Optional[str] = None  # Last tag ID from device (may be None if NFC flaky)
_device_tag_data: Optional[Dict] = None  # Points to staged data for backwards compat

# Simulation mode - prevents device updates from clearing simulated tag
_simulating_tag: bool = False


def get_staged_tag() -> Optional[Dict]:
    """Get staged tag if still valid (not timed out). Returns None if expired."""
    global _staged_tag_id, _staged_tag_data, _staged_tag_timestamp
    if _staged_tag_data is None:
        return None

    elapsed = time.time() - _staged_tag_timestamp
    if elapsed >= STAGING_TIMEOUT:
        # Staging expired - clear it
        logger.info(f"Staging expired for tag {_staged_tag_id}")
        _staged_tag_id = None
        _staged_tag_data = None
        _staged_tag_timestamp = 0
        return None

    return _staged_tag_data


def get_staging_remaining() -> float:
    """Get seconds remaining in staging, or 0 if no staged tag."""
    if _staged_tag_data is None:
        return 0
    remaining = STAGING_TIMEOUT - (time.time() - _staged_tag_timestamp)
    return max(0, remaining)


def stage_tag(tag_id: str, tag_data: Dict) -> bool:
    """
    Add tag to staging. Returns True if this is a new/different tag.
    Same tag does NOT reset timer - countdown continues while tag is on reader.
    Only placing a NEW tag resets the timer.
    Returns False without staging if tag is blocked.
    """
    global _staged_tag_id, _staged_tag_data, _staged_tag_timestamp, _tag_data_cache
    global _blocked_tag_id, _blocked_until

    # Check if this tag is blocked (recently cleared)
    if tag_id == _blocked_tag_id and time.time() < _blocked_until:
        # Tag is blocked, don't stage
        return False

    # Clear block if expired
    if _blocked_tag_id and time.time() >= _blocked_until:
        logger.info(f"Tag {_blocked_tag_id} block expired")
        _blocked_tag_id = None
        _blocked_until = 0

    is_new_tag = _staged_tag_id != tag_id

    _staged_tag_id = tag_id
    _staged_tag_data = tag_data

    # Only reset timer for NEW tags, not for same tag re-detection
    # This allows the countdown to actually progress while tag is on reader
    if is_new_tag:
        _staged_tag_timestamp = time.time()

    # Cache the decoded data for future re-staging
    if tag_data and tag_data.get('vendor'):
        _tag_data_cache[tag_id] = tag_data

    if is_new_tag:
        logger.info(f"Staged new tag: {tag_id} ({tag_data.get('vendor')} {tag_data.get('material')})")

    return is_new_tag


def clear_staging() -> bool:
    """Manually clear staging. Returns True if there was a staged tag."""
    global _staged_tag_id, _staged_tag_data, _staged_tag_timestamp
    global _blocked_tag_id, _blocked_until

    had_tag = _staged_tag_data is not None
    if had_tag:
        logger.info(f"Staging cleared manually for tag {_staged_tag_id}")
        # Block this tag for a few seconds to prevent immediate re-staging
        _blocked_tag_id = _staged_tag_id
        _blocked_until = time.time() + BLOCK_DURATION
        logger.info(f"Tag {_staged_tag_id} blocked for {BLOCK_DURATION}s")

    _staged_tag_id = None
    _staged_tag_data = None
    _staged_tag_timestamp = 0

    return had_tag


def _get_local_ip() -> str:
    """Get the local IP address of this machine."""
    try:
        # Create a socket to determine the local IP
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception:
        return "127.0.0.1"


def update_display_heartbeat():
    """Update display last seen time and broadcast if connection state changed."""
    global _display_last_seen, _display_connected
    import time

    now = time.time()
    was_connected = _display_connected
    _display_last_seen = now
    _display_connected = True

    # Broadcast connection change
    if not was_connected:
        logger.info("ESP32 display connected")
        try:
            loop = asyncio.get_running_loop()
            loop.create_task(broadcast_message({"type": "device_connected"}))
        except RuntimeError:
            pass


def is_display_connected() -> bool:
    """Check if display is connected (seen within timeout)."""
    import time
    if _display_last_seen == 0:
        return False
    return (time.time() - _display_last_seen) < DISPLAY_TIMEOUT_SEC


def queue_display_command(command: str):
    """Queue a command for the display to execute on next heartbeat."""
    global _display_pending_command
    _display_pending_command = command
    logger.info(f"Queued display command: {command}")


def pop_display_command() -> Optional[str]:
    """Get and clear the pending display command."""
    global _display_pending_command
    cmd = _display_pending_command
    _display_pending_command = None
    return cmd


async def udp_log_listener():
    """Listen for UDP log messages from ESP32 firmware."""
    UDP_LOG_PORT = 5555

    # Create UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("0.0.0.0", UDP_LOG_PORT))
    sock.setblocking(False)

    logger.info(f"UDP log listener started on port {UDP_LOG_PORT}")

    loop = asyncio.get_event_loop()
    while True:
        try:
            data, addr = await loop.run_in_executor(None, lambda: sock.recvfrom(4096))
            message = data.decode('utf-8', errors='replace').strip()
            if message:
                # Print with ESP32 prefix for clarity
                print(f"[ESP32] {message}")
        except BlockingIOError:
            await asyncio.sleep(0.01)
        except Exception as e:
            logger.error(f"UDP listener error: {e}")
            await asyncio.sleep(1)


async def check_display_timeout():
    """Background task to check for display timeout and broadcast disconnect."""
    global _display_connected
    import time

    while True:
        await asyncio.sleep(2)  # Check every 2 seconds

        if _display_connected and not is_display_connected():
            _display_connected = False
            logger.info("ESP32 display disconnected (timeout)")
            await broadcast_message({"type": "device_disconnected"})


async def broadcast_message(message: dict):
    """Broadcast message to all connected WebSocket clients."""
    if not websocket_clients:
        return

    text = json.dumps(message)
    disconnected = set()

    for ws in websocket_clients:
        try:
            await ws.send_text(text)
        except Exception:
            disconnected.add(ws)

    # Clean up disconnected clients
    websocket_clients.difference_update(disconnected)


async def on_usage_logged(serial: str, print_name: str, tray_usage: dict):
    """Handle filament usage detection from print completion.

    Args:
        serial: Printer serial number
        print_name: Name of the completed print
        tray_usage: Dict of (ams_id, tray_id) -> percent_used
    """
    db = await get_db()

    for (ams_id, tray_id), percent_used in tray_usage.items():
        # Look up assigned spool for this slot
        spool_id = await db.get_spool_for_slot(serial, ams_id, tray_id)

        if not spool_id:
            logger.debug(
                f"No spool assigned to slot ({ams_id}, {tray_id}) on {serial}, "
                f"skipping usage logging"
            )
            continue

        # Get spool to calculate weight from percentage
        spool = await db.get_spool(spool_id)
        if not spool:
            continue

        # Estimate grams used
        label_weight = spool.label_weight or 1000
        weight_used = estimate_weight_from_percent(percent_used, label_weight)

        # Log usage history
        await db.log_usage(spool_id, serial, print_name, weight_used)

        # Update spool consumption
        await db.update_spool_consumption(spool_id, weight_used)

        logger.info(
            f"Logged usage for spool {spool_id}: {weight_used:.1f}g "
            f"({percent_used}% of {label_weight}g spool) from '{print_name}'"
        )

    # Broadcast usage update to UI
    await broadcast_message({
        "type": "usage_logged",
        "serial": serial,
        "print_name": print_name,
        "tray_usage": {f"{k[0]}_{k[1]}": v for k, v in tray_usage.items()},
    })


def on_printer_state_update(serial: str, state: PrinterState):
    """Handle printer state update from MQTT."""
    global _previous_states

    # Get previous state for comparison
    prev_state = _previous_states.get(serial)

    # Update usage tracker (detects print start/end)
    usage_tracker.on_state_update(serial, state, prev_state)

    # Store current state as previous for next update
    _previous_states[serial] = state.model_copy()

    # Convert to dict for JSON serialization
    message = {
        "type": "printer_state",
        "serial": serial,
        "state": state.model_dump(),
    }

    # Schedule broadcast in event loop
    try:
        loop = asyncio.get_running_loop()
        loop.create_task(broadcast_message(message))
    except RuntimeError:
        pass  # No running loop


def on_printer_connect(serial: str):
    """Handle printer connection from MQTT."""
    logger.info(f"Printer {serial} connected - notifying clients")

    # Broadcast connection
    message = {
        "type": "printer_connected",
        "serial": serial,
    }

    # Schedule broadcast in event loop
    try:
        loop = asyncio.get_running_loop()
        loop.create_task(broadcast_message(message))
    except RuntimeError:
        pass  # No running loop


def on_printer_disconnect(serial: str):
    """Handle printer disconnection from MQTT."""
    logger.info(f"Printer {serial} disconnected - notifying clients")

    # Clear previous state
    _previous_states.pop(serial, None)

    # Broadcast disconnection
    message = {
        "type": "printer_disconnected",
        "serial": serial,
    }

    # Schedule broadcast in event loop
    try:
        loop = asyncio.get_running_loop()
        loop.create_task(broadcast_message(message))
    except RuntimeError:
        pass  # No running loop


async def auto_connect_printers():
    """Connect to printers with auto_connect enabled."""
    await asyncio.sleep(0.5)  # Wait for startup

    db = await get_db()
    printers = await db.get_auto_connect_printers()

    for printer in printers:
        if printer.ip_address and printer.access_code:
            logger.info(f"Auto-connecting to printer {printer.serial}")
            try:
                await printer_manager.connect(
                    serial=printer.serial,
                    ip_address=printer.ip_address,
                    access_code=printer.access_code,
                    name=printer.name,
                )
            except Exception as e:
                logger.error(f"Failed to auto-connect to {printer.serial}: {e}")


@asynccontextmanager
async def lifespan(app: FastAPI):
    """Application lifespan handler."""
    global _zeroconf, _mdns_service

    # Startup
    logger.info("Starting SpoolBuddy server...")

    # Initialize database
    await get_db()
    logger.info("Database initialized")

    # Set up usage tracker
    usage_tracker.set_usage_callback(on_usage_logged)
    usage_tracker.set_event_loop(asyncio.get_running_loop())

    # Set up printer manager
    set_printer_manager(printer_manager)
    printer_manager.set_state_callback(on_printer_state_update)
    printer_manager.set_connect_callback(on_printer_connect)
    printer_manager.set_disconnect_callback(on_printer_disconnect)

    # Register mDNS service for device discovery
    # Service type must be <= 15 chars, using "_spbuddy-srv" (12 chars)
    try:
        local_ip = _get_local_ip()
        _zeroconf = AsyncZeroconf()
        _mdns_service = ServiceInfo(
            "_spbuddy-srv._tcp.local.",
            "SpoolBuddy._spbuddy-srv._tcp.local.",
            addresses=[socket.inet_aton(local_ip)],
            port=settings.port,
            properties={"version": "0.1.0", "api": "/api"},
        )
        await _zeroconf.async_register_service(_mdns_service)
        logger.info(f"mDNS service registered: {local_ip}:{settings.port} (_spbuddy-srv._tcp)")
    except Exception as e:
        logger.warning(f"Failed to register mDNS service: {e}")

    # Auto-connect printers
    asyncio.create_task(auto_connect_printers())

    # Start display timeout checker
    asyncio.create_task(check_display_timeout())

    # Start UDP log listener for ESP32 logs
    asyncio.create_task(udp_log_listener())

    yield

    # Shutdown
    logger.info("Shutting down...")

    # Unregister mDNS service
    if _zeroconf and _mdns_service:
        try:
            await _zeroconf.async_unregister_service(_mdns_service)
            await _zeroconf.async_close()
            logger.info("mDNS service unregistered")
        except Exception as e:
            logger.warning(f"Failed to unregister mDNS service: {e}")

    await printer_manager.disconnect_all()


# Create FastAPI app
app = FastAPI(
    title="SpoolBuddy",
    description="Filament management for Bambu Lab printers",
    version="0.1.0",
    lifespan=lifespan,
)

# CORS middleware
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# API routes
app.include_router(spools_router, prefix="/api")
app.include_router(printers_router, prefix="/api")
app.include_router(cloud_router, prefix="/api")
app.include_router(updates_router, prefix="/api")
app.include_router(firmware_router, prefix="/api")
app.include_router(tags_router, prefix="/api")
app.include_router(device_router, prefix="/api")
app.include_router(serial_router, prefix="/api")
app.include_router(discovery_router, prefix="/api")
app.include_router(catalog_router, prefix="/api")


@app.get("/api/time")
async def get_server_time():
    """Get server time for ESP32 clock sync."""
    import datetime
    now = datetime.datetime.now()
    return {
        "hour": now.hour,
        "minute": now.minute,
        "second": now.second,
        "timestamp": int(now.timestamp())
    }


@app.get("/api/display/heartbeat")
async def display_heartbeat(version: Optional[str] = None, update_available: Optional[bool] = None):
    """Heartbeat endpoint for ESP32 display to indicate it's connected."""
    global _display_firmware_version, _device_update_available
    update_display_heartbeat()
    if version:
        _display_firmware_version = version
    if update_available is not None:
        old_status = _device_update_available
        _device_update_available = update_available
        # Broadcast if update availability changed
        if old_status != update_available:
            try:
                loop = asyncio.get_running_loop()
                loop.create_task(broadcast_message({
                    "type": "device_update_available",
                    "update_available": update_available,
                }))
            except RuntimeError:
                pass
    cmd = pop_display_command()
    if cmd:
        logger.info(f"Sending command to display: {cmd}")
        return {"ok": True, "command": cmd}
    return {"ok": True}


def get_display_firmware_version() -> Optional[str]:
    """Get the last reported firmware version from the display."""
    return _display_firmware_version


@app.get("/api/display/status")
async def display_status():
    """Get display connection status including staged tag info."""
    staged = get_staged_tag()  # Returns None if expired
    remaining = get_staging_remaining()

    return {
        "connected": is_display_connected(),
        "last_seen": _display_last_seen if _display_last_seen > 0 else None,
        "firmware_version": _display_firmware_version,
        "update_available": _device_update_available,
        "weight": _device_last_weight,
        "weight_stable": _device_weight_stable,
        # Staging info (new)
        "staged_tag_id": _staged_tag_id if staged else None,
        "staged_tag_data": staged,
        "staging_remaining": round(remaining, 1) if staged else 0,
        # Legacy (backwards compat) - points to staged data
        "tag_id": _staged_tag_id if staged else None,
        "tag_data": staged,
    }


@app.post("/api/display/state")
async def update_device_state(
    weight: Optional[float] = None,
    stable: Optional[bool] = None,
    tag_id: Optional[str] = None,
    tag_vendor: Optional[str] = None,
    tag_material: Optional[str] = None,
    tag_subtype: Optional[str] = None,
    tag_color: Optional[str] = None,
    tag_color_rgba: Optional[int] = None,
    tag_weight: Optional[int] = None,
    tag_type: Optional[str] = None,
):
    """HTTP endpoint for device to update state (alternative to WebSocket)."""
    update_display_heartbeat()

    # Build tag_data if decoded data provided
    tag_data = None
    if tag_id and tag_vendor:
        tag_data = {
            "uid": tag_id,
            "tag_type": tag_type or "bambulab",
            "vendor": tag_vendor or "",
            "material": tag_material or "",
            "subtype": tag_subtype or "",
            "color_name": tag_color or "",
            "color_rgba": tag_color_rgba or 0,
            "spool_weight": tag_weight or 0,
        }
        logger.info(f"Received decoded tag data from device: {tag_vendor} {tag_material}")

    await handle_device_state({
        "weight": weight,
        "stable": stable if stable is not None else False,
        "tag_id": tag_id,
        "tag_data": tag_data,
    })
    return {"ok": True}


@app.post("/api/test/simulate-tag")
async def simulate_tag(present: bool = True):
    """Test endpoint to simulate NFC tag for UI development."""
    global _device_tag_data, _device_current_tag_id, _simulating_tag

    if present:
        _simulating_tag = True
        _device_current_tag_id = "A7:B2:65:00"
        _device_tag_data = {
            "uid": "A7:B2:65:00",
            "tag_type": "bambulab",
            "vendor": "Bambu",
            "material": "PLA",
            "subtype": "Basic",
            "color_name": "Gray",
            "color_rgba": 0xA6A9AAFF,
            "spool_weight": 1000,
        }
        logger.info("Simulated tag PRESENT (simulation mode ON)")
    else:
        _simulating_tag = False
        _device_current_tag_id = None
        _device_tag_data = None
        logger.info("Simulated tag REMOVED (simulation mode OFF)")

    return {"ok": True, "tag_present": present}


@app.post("/api/staging/clear")
async def api_clear_staging():
    """Manually clear the staged tag."""
    had_tag = clear_staging()
    # Also broadcast to WebSocket clients
    if had_tag:
        await broadcast_message({"type": "staging_cleared"})
    return {"ok": True, "had_tag": had_tag}


@app.get("/api/staging")
async def api_get_staging():
    """Get current staging status."""
    staged = get_staged_tag()
    remaining = get_staging_remaining()
    return {
        "tag_id": _staged_tag_id if staged else None,
        "tag_data": staged,
        "remaining": round(remaining, 1) if staged else 0,
    }


async def handle_tag_detected(websocket: WebSocket, message: dict):
    """Handle tag_detected message from device."""
    global _device_tag_data, _device_current_tag_id
    uid_hex = message.get("uid", "")
    tag_type = message.get("tag_type", "")  # "NTAG", "MifareClassic1K", etc.
    _device_current_tag_id = uid_hex

    # Data depends on tag type
    ndef_url = message.get("ndef_url")  # For NTAG with URL
    ndef_records = message.get("ndef_records")  # For NTAG with raw records
    mifare_blocks = message.get("blocks")  # For Mifare Classic

    logger.info(f"Tag detected: UID={uid_hex}, type={tag_type}")

    result = None

    # Decode based on what data we have
    if ndef_url:
        result = TagDecoder.decode_ndef_url(uid_hex, ndef_url)
    elif ndef_records:
        result = TagDecoder.decode_ndef_records(uid_hex, ndef_records)
    elif mifare_blocks:
        # Convert hex strings to bytes if needed
        blocks = {}
        for block_num, data in mifare_blocks.items():
            if isinstance(data, str):
                blocks[int(block_num)] = bytes.fromhex(data)
            else:
                blocks[int(block_num)] = bytes(data)
        result = TagDecoder.decode_mifare_blocks(uid_hex, blocks)

    if result:
        # Try to find matching spool in database
        db = await get_db()
        spool = await db.get_spool_by_tag(result.uid_base64)

        if spool:
            result.matched_spool_id = spool.id
            logger.info(f"Tag matched to spool: {spool.id}")
        else:
            # Convert to spool data for potential creation
            spool_data = TagDecoder.to_spool(result)
            if spool_data:
                logger.info(f"New tag detected: {spool_data.material} {spool_data.color_name}")

        # Store decoded tag data for HTTP polling
        _device_tag_data = {
            "uid": result.uid,
            "tag_type": result.tag_type.value,
        }
        # Extract normalized spool data from decoded result
        if result.spoolease_data:
            d = result.spoolease_data
            _device_tag_data["vendor"] = d.brand or ""
            _device_tag_data["material"] = d.material or ""
            _device_tag_data["subtype"] = d.material_subtype or ""
            _device_tag_data["color_name"] = d.color_name or ""
            _device_tag_data["color_rgba"] = int(d.color_code + "FF", 16) if d.color_code and len(d.color_code) == 6 else 0
            _device_tag_data["spool_weight"] = d.weight_label or 0
            _device_tag_data["slicer_filament"] = d.slicer_filament_code or ""
        elif result.bambulab_data:
            d = result.bambulab_data
            _device_tag_data["vendor"] = "Bambu"
            _device_tag_data["material"] = d.tray_type or ""
            _device_tag_data["subtype"] = d.tray_sub_brands or ""
            color_rgba = d.tray_color if d.tray_color else 0
            _device_tag_data["color_rgba"] = color_rgba
            _device_tag_data["spool_weight"] = d.spool_weight or 0
            # Map material_id to human-readable slicer profile name
            from tags.bambulab import BAMBU_MATERIALS
            material_id = d.material_id or ""
            if material_id in BAMBU_MATERIALS:
                slicer_name, _ = BAMBU_MATERIALS[material_id]
            else:
                slicer_name = material_id  # Fallback to code if not found
            _device_tag_data["slicer_filament"] = slicer_name
            # Look up color name from Bambu color database
            color_name = lookup_bambu_color_name(material_id, color_rgba)
            _device_tag_data["color_name"] = color_name or ""
        elif result.openprinttag_data:
            d = result.openprinttag_data
            _device_tag_data["vendor"] = d.brand or ""
            _device_tag_data["material"] = d.material_type or ""
            _device_tag_data["subtype"] = ""
            _device_tag_data["color_name"] = ""
            color_hex = d.color_hex or ""
            _device_tag_data["color_rgba"] = int(color_hex + "FF", 16) if len(color_hex) == 6 else 0
            _device_tag_data["spool_weight"] = 0
            _device_tag_data["slicer_filament"] = ""  # OpenPrintTag doesn't have slicer info

        # Stage the decoded tag data immediately (ensures slicer_filament is included)
        stage_tag(uid_hex, _device_tag_data)

        # Send result back to all clients
        response = {
            "type": "tag_result",
            "uid": result.uid,
            "uid_base64": result.uid_base64,
            "tag_type": result.tag_type.value,
            "matched_spool_id": result.matched_spool_id,
        }

        # Include parsed data
        if result.spoolease_data:
            response["spoolease_data"] = result.spoolease_data.model_dump()
        if result.bambulab_data:
            response["bambulab_data"] = result.bambulab_data.model_dump(exclude={"blocks"})
        if result.openprinttag_data:
            response["openprinttag_data"] = result.openprinttag_data.model_dump()

        await broadcast_message(response)
    else:
        # No decoded data, just store UID
        _device_tag_data = {"uid": uid_hex, "tag_type": tag_type}


async def handle_device_state(message: dict):
    """Handle device_state message from device (weight, tag info).

    Uses the staging system: when a tag is detected, it's staged for 30s.
    Flaky NFC reads (tag_id=None) don't clear staging - only timeout or new tag does.
    """
    global _device_last_weight, _device_weight_stable, _device_current_tag_id, _device_tag_data

    weight = message.get("weight")
    stable = message.get("stable", False)
    tag_id = message.get("tag_id")
    provided_tag_data = message.get("tag_data")

    state_changed = False

    # Update weight
    if weight is not None and weight != _device_last_weight:
        _device_last_weight = weight
        state_changed = True

    if stable != _device_weight_stable:
        _device_weight_stable = stable
        state_changed = True

    # Don't update tag state if we're in simulation mode
    if _simulating_tag:
        return  # Ignore all tag updates in simulation mode

    # Track what device reports (for debugging)
    _device_current_tag_id = tag_id

    # === Staging Logic ===
    # Only stage when we have BOTH tag_id AND decoded data
    # Flaky reads (no tag_id) are ignored - staging handles persistence

    if tag_id and provided_tag_data and provided_tag_data.get("vendor"):
        # Tag with decoded data - enrich with slicer filament name if missing
        if "slicer_filament" not in provided_tag_data or not provided_tag_data.get("slicer_filament"):
            vendor = provided_tag_data.get("vendor", "")
            material = provided_tag_data.get("material", "")
            subtype = provided_tag_data.get("subtype", "")

            # For Bambu tags, combine vendor + material + subtype
            if vendor == "Bambu" and material:
                if subtype:
                    provided_tag_data["slicer_filament"] = f"Bambu {material} {subtype}"
                else:
                    provided_tag_data["slicer_filament"] = f"Bambu {material}"
            elif material:
                # Generic filament - use material type
                provided_tag_data["slicer_filament"] = f"Generic {material}"

        # Look up color name from Bambu color database
        color_name = provided_tag_data.get("color_name", "")
        if not color_name or color_name.startswith("#"):
            # No color name or it's a hex code - try to look up from CSV
            material_id = provided_tag_data.get("slicer_filament", "")  # May be code like "GFA00" or name
            color_rgba = provided_tag_data.get("color_rgba", 0)
            looked_up_name = lookup_bambu_color_name(material_id, color_rgba)
            if looked_up_name:
                provided_tag_data["color_name"] = looked_up_name
                logger.debug(f"Looked up color name: {looked_up_name} for {material_id}/{color_rgba:08X}")
            else:
                provided_tag_data["color_name"] = ""
        # Stage the enriched data
        is_new = stage_tag(tag_id, provided_tag_data)
        if is_new:
            state_changed = True
            # Broadcast that a new tag was staged
            await broadcast_message({
                "type": "tag_staged",
                "tag_id": tag_id,
                "tag_data": provided_tag_data,
                "timeout": STAGING_TIMEOUT,
            })
    elif tag_id and provided_tag_data and not provided_tag_data.get("vendor"):
        # Unknown tag type - has tag_data but no decoded vendor/material
        # Still stage it so user can see it and potentially configure manually
        tag_type = provided_tag_data.get("tag_type", "Unknown")
        provided_tag_data["vendor"] = "Unknown"
        provided_tag_data["material"] = tag_type
        logger.info(f"Staging unknown tag: {tag_id} (type: {tag_type})")
        is_new = stage_tag(tag_id, provided_tag_data)
        if is_new:
            state_changed = True
            await broadcast_message({
                "type": "tag_staged",
                "tag_id": tag_id,
                "tag_data": provided_tag_data,
                "timeout": STAGING_TIMEOUT,
            })
    elif tag_id and not provided_tag_data:
        # Tag ID but no decoded data yet - check if it's already staged
        if _staged_tag_id == tag_id:
            # Same tag, reset timer
            stage_tag(tag_id, _staged_tag_data)
        elif tag_id in _tag_data_cache:
            # We have cached decoded data for this tag - use it
            cached_data = _tag_data_cache[tag_id]
            is_new = stage_tag(tag_id, cached_data)
            logger.info(f"Re-staged tag from cache: {tag_id}")
            if is_new:
                state_changed = True
                await broadcast_message({
                    "type": "tag_staged",
                    "tag_id": tag_id,
                    "tag_data": cached_data,
                    "timeout": STAGING_TIMEOUT,
                })
        else:
            # New tag without decoded data - try database lookup
            tag_data = await _lookup_tag_in_database(tag_id)
            if tag_data:
                is_new = stage_tag(tag_id, tag_data)
                if is_new:
                    state_changed = True
                    await broadcast_message({
                        "type": "tag_staged",
                        "tag_id": tag_id,
                        "tag_data": tag_data,
                        "timeout": STAGING_TIMEOUT,
                    })
            else:
                # Unknown tag not in database - stage as "Unknown" so user can see it
                logger.info(f"Staging unknown tag (not in database): {tag_id}")
                tag_data = {
                    "uid": tag_id,
                    "tag_type": "Unknown",
                    "vendor": "Unknown",
                    "material": "Unknown",
                    "subtype": "",
                    "color_name": "",
                    "color_rgba": 0x888888FF,
                    "spool_weight": 0,
                }
                is_new = stage_tag(tag_id, tag_data)
                if is_new:
                    state_changed = True
                    await broadcast_message({
                        "type": "tag_staged",
                        "tag_id": tag_id,
                        "tag_data": tag_data,
                        "timeout": STAGING_TIMEOUT,
                    })
    # else: no tag_id - ignore, let staging timeout naturally

    # Keep legacy _device_tag_data in sync with staging for backwards compat
    _device_tag_data = get_staged_tag()

    # Broadcast weight updates
    if state_changed:
        await broadcast_message({
            "type": "device_state",
            "weight": _device_last_weight,
            "stable": _device_weight_stable,
        })


async def _lookup_tag_in_database(tag_id: str) -> Optional[Dict]:
    """Look up tag in spool database, return tag_data dict or None."""
    try:
        db = await get_db()
        # Use the dedicated method to look up by tag
        spool = await db.get_spool_by_tag(tag_id)
        if spool:
            spool_dict = spool.model_dump() if hasattr(spool, 'model_dump') else dict(spool)
            tag_data = {
                "uid": tag_id,
                "tag_type": spool_dict.get("tag_type", "database"),
                "vendor": spool_dict.get("brand", ""),
                "material": spool_dict.get("material", ""),
                "subtype": spool_dict.get("subtype", ""),
                "color_name": spool_dict.get("color_name", ""),
                "spool_weight": spool_dict.get("label_weight", 0),
            }
            # Convert RGBA hex to int
            rgba_str = spool_dict.get("rgba", "")
            if rgba_str and len(rgba_str) >= 6:
                try:
                    if len(rgba_str) == 6:
                        rgba_str = rgba_str + "FF"
                    tag_data["color_rgba"] = int(rgba_str, 16)
                except ValueError:
                    tag_data["color_rgba"] = 0
            logger.info(f"Tag {tag_id} matched to spool: {spool_dict.get('material')} {spool_dict.get('color_name')}")
            return tag_data
    except Exception as e:
        logger.warning(f"Error looking up spool for tag {tag_id}: {e}")

    return None


@app.websocket("/ws/ui")
async def websocket_endpoint(websocket: WebSocket):
    """WebSocket endpoint for real-time UI updates."""
    global _device_current_tag_id, _device_tag_data
    await websocket.accept()
    websocket_clients.add(websocket)
    logger.info("WebSocket client connected")

    # Send initial state to new client
    try:
        display_connected = is_display_connected()
        logger.info(f"Sending initial_state: device.connected={display_connected}")
        initial_state = {
            "type": "initial_state",
            "device": {
                "connected": display_connected,
                "update_available": _device_update_available,
                "last_weight": _device_last_weight,
                "weight_stable": _device_weight_stable,
                "current_tag_id": _device_current_tag_id,
            },
            "printers": {
                serial: conn.connected
                for serial, conn in printer_manager._connections.items()
            }
        }
        await websocket.send_text(json.dumps(initial_state))
    except Exception as e:
        logger.warning(f"Failed to send initial state: {e}")

    try:
        while True:
            # Keep connection alive, handle any incoming messages
            data = await websocket.receive_text()

            try:
                message = json.loads(data)
                msg_type = message.get("type", "")

                if msg_type == "tag_detected":
                    await handle_tag_detected(websocket, message)
                elif msg_type == "tag_removed":
                    _device_current_tag_id = None
                    _device_tag_data = None
                    await broadcast_message({"type": "tag_removed"})
                elif msg_type == "device_state":
                    await handle_device_state(message)
                else:
                    logger.debug(f"Received from WebSocket: {data}")

            except json.JSONDecodeError:
                logger.debug(f"Received non-JSON from WebSocket: {data}")

    except WebSocketDisconnect:
        logger.info("WebSocket client disconnected")
    except Exception as e:
        logger.error(f"WebSocket error: {e}")
    finally:
        websocket_clients.discard(websocket)


# Mount static files (frontend) - must be last
if settings.static_dir.exists():
    app.mount("/", StaticFiles(directory=settings.static_dir, html=True), name="static")


if __name__ == "__main__":
    import uvicorn
    uvicorn.run(
        "main:app",
        host=settings.host,
        port=settings.port,
        reload=True,
    )
