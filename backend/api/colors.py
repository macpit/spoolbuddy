"""Color catalog API endpoints."""

import json
import logging

import httpx
from db.database import get_db
from fastapi import APIRouter, HTTPException
from fastapi.responses import StreamingResponse
from pydantic import BaseModel

logger = logging.getLogger(__name__)
router = APIRouter(prefix="/colors", tags=["colors"])

# FilamentColors.xyz API
FILAMENT_COLORS_API = "https://filamentcolors.xyz/api"


class ColorEntry(BaseModel):
    """Color catalog entry."""

    id: int
    manufacturer: str
    color_name: str
    hex_color: str
    material: str | None
    is_default: bool
    created_at: int | None = None


class ColorEntryCreate(BaseModel):
    """Create a color catalog entry."""

    manufacturer: str
    color_name: str
    hex_color: str
    material: str | None = None


class ColorEntryUpdate(BaseModel):
    """Update a color catalog entry."""

    manufacturer: str
    color_name: str
    hex_color: str
    material: str | None = None


class ColorLookupResult(BaseModel):
    """Result of color lookup."""

    found: bool
    hex_color: str | None = None
    material: str | None = None


@router.get("")
async def get_color_catalog() -> list[ColorEntry]:
    """Get all color catalog entries."""
    db = await get_db()
    entries = await db.get_color_catalog()
    return [ColorEntry(**e) for e in entries]


@router.post("")
async def add_color_entry(entry: ColorEntryCreate) -> ColorEntry:
    """Add a new color catalog entry."""
    db = await get_db()
    result = await db.add_color_catalog_entry(entry.manufacturer, entry.color_name, entry.hex_color, entry.material)
    if not result:
        raise HTTPException(status_code=500, detail="Failed to add entry")
    return ColorEntry(**result)


@router.put("/{entry_id}")
async def update_color_entry(entry_id: int, entry: ColorEntryUpdate) -> ColorEntry:
    """Update a color catalog entry."""
    db = await get_db()
    result = await db.update_color_catalog_entry(
        entry_id, entry.manufacturer, entry.color_name, entry.hex_color, entry.material
    )
    if not result:
        raise HTTPException(status_code=404, detail="Entry not found")
    return ColorEntry(**result)


@router.delete("/{entry_id}")
async def delete_color_entry(entry_id: int) -> dict:
    """Delete a color catalog entry."""
    db = await get_db()
    success = await db.delete_color_catalog_entry(entry_id)
    if not success:
        raise HTTPException(status_code=404, detail="Entry not found")
    return {"status": "deleted"}


@router.post("/reset")
async def reset_color_catalog() -> dict:
    """Reset color catalog to defaults."""
    db = await get_db()
    await db.reset_color_catalog()
    return {"status": "reset"}


@router.get("/lookup")
async def lookup_color(manufacturer: str, color_name: str, material: str | None = None) -> ColorLookupResult:
    """Look up a color by manufacturer and color name."""
    db = await get_db()
    result = await db.lookup_color(manufacturer, color_name, material)
    if result:
        return ColorLookupResult(found=True, hex_color=result["hex_color"], material=result["material"])
    return ColorLookupResult(found=False)


class SyncResult(BaseModel):
    """Result of sync operation."""

    added: int
    skipped: int
    total_fetched: int
    total_available: int
    error: str | None = None


@router.post("/sync")
async def sync_from_filamentcolors():
    """Sync colors from FilamentColors.xyz API with progress streaming."""

    async def generate():
        db = await get_db()
        added = 0
        skipped = 0
        total_fetched = 0
        total_available = 0

        try:
            async with httpx.AsyncClient(timeout=120.0) as client:
                # Fetch all swatches (paginated using page parameter)
                page = 1

                while True:
                    response = await client.get(
                        f"{FILAMENT_COLORS_API}/swatch/",
                        params={"page": page},
                    )
                    response.raise_for_status()
                    data = response.json()

                    # Update total count
                    total_available = data.get("count", total_available)

                    results = data.get("results", [])
                    if not results:
                        break

                    for swatch in results:
                        total_fetched += 1

                        # Extract manufacturer name from nested object
                        manufacturer_data = swatch.get("manufacturer")
                        if isinstance(manufacturer_data, dict):
                            manufacturer_name = manufacturer_data.get("name", "")
                        else:
                            manufacturer_name = ""

                        # Extract filament type name from nested object
                        filament_type_data = swatch.get("filament_type")
                        if isinstance(filament_type_data, dict):
                            material = filament_type_data.get("name", "")
                        else:
                            material = None

                        color_name = swatch.get("color_name", "")
                        hex_color = swatch.get("hex_color", "")

                        if not manufacturer_name or not color_name or not hex_color:
                            skipped += 1
                            continue

                        # Ensure hex color has # prefix
                        if not hex_color.startswith("#"):
                            hex_color = f"#{hex_color}"

                        # Try to add using INSERT OR IGNORE to handle duplicates
                        try:
                            cursor = await db.conn.execute(
                                "INSERT OR IGNORE INTO color_catalog (manufacturer, color_name, hex_color, material, is_default) VALUES (?, ?, ?, ?, 0)",
                                (manufacturer_name, color_name, hex_color.upper(), material),
                            )
                            if cursor.rowcount > 0:
                                added += 1
                            else:
                                skipped += 1
                        except Exception as e:
                            logger.warning(f"Failed to insert color {manufacturer_name} - {color_name}: {e}")
                            skipped += 1

                    # Commit after each page
                    await db.conn.commit()

                    # Send progress update after each page
                    progress = {
                        "type": "progress",
                        "added": added,
                        "skipped": skipped,
                        "total_fetched": total_fetched,
                        "total_available": total_available,
                    }
                    yield f"data: {json.dumps(progress)}\n\n"

                    # Check if there are more pages
                    if not data.get("next") or total_fetched >= total_available:
                        break
                    page += 1

            # Send final result
            result = {
                "type": "complete",
                "added": added,
                "skipped": skipped,
                "total_fetched": total_fetched,
                "total_available": total_available,
            }
            yield f"data: {json.dumps(result)}\n\n"

        except httpx.HTTPError as e:
            logger.error(f"HTTP error syncing from FilamentColors.xyz: {e}")
            error_result = {
                "type": "error",
                "added": added,
                "skipped": skipped,
                "total_fetched": total_fetched,
                "total_available": total_available,
                "error": f"HTTP error: {str(e)}",
            }
            yield f"data: {json.dumps(error_result)}\n\n"
        except Exception as e:
            logger.error(f"Error syncing from FilamentColors.xyz: {e}")
            error_result = {
                "type": "error",
                "added": added,
                "skipped": skipped,
                "total_fetched": total_fetched,
                "total_available": total_available,
                "error": str(e),
            }
            yield f"data: {json.dumps(error_result)}\n\n"

    return StreamingResponse(generate(), media_type="text/event-stream")
