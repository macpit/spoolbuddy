from .catalog import router as catalog_router
from .colors import router as colors_router
from .device import router as device_router
from .discovery import router as discovery_router
from .firmware import router as firmware_router
from .printers import router as printers_router
from .serial import router as serial_router
from .spools import router as spools_router
from .support import router as support_router
from .tags import router as tags_router
from .updates import router as updates_router

__all__ = [
    "spools_router",
    "printers_router",
    "updates_router",
    "firmware_router",
    "tags_router",
    "device_router",
    "serial_router",
    "discovery_router",
    "catalog_router",
    "colors_router",
    "support_router",
]
