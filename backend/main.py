import asyncio
import json
import logging
from contextlib import asynccontextmanager
from typing import Set, Optional, Dict

from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.staticfiles import StaticFiles
from fastapi.middleware.cors import CORSMiddleware

from config import settings
from db import get_db
from mqtt import PrinterManager
from api import spools_router, printers_router, updates_router, firmware_router, tags_router
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

# Global state
printer_manager = PrinterManager()
websocket_clients: Set[WebSocket] = set()
usage_tracker = UsageTracker()
# Track previous printer states for comparison
_previous_states: Dict[str, PrinterState] = {}


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

    # Auto-connect printers
    asyncio.create_task(auto_connect_printers())

    yield

    # Shutdown
    logger.info("Shutting down...")
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


async def handle_tag_detected(websocket: WebSocket, message: dict):
    """Handle tag_detected message from device."""
    uid_hex = message.get("uid", "")
    tag_type = message.get("tag_type", "")  # "NTAG", "MifareClassic1K", etc.

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


@app.websocket("/ws/ui")
async def websocket_endpoint(websocket: WebSocket):
    """WebSocket endpoint for real-time UI updates."""
    await websocket.accept()
    websocket_clients.add(websocket)
    logger.info("WebSocket client connected")

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
                    await broadcast_message({"type": "tag_removed"})
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
