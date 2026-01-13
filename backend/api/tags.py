"""Tag encoding/decoding API endpoints.

Provides endpoints for encoding spool data into various NFC tag formats
and decoding tag data for testing without hardware.
"""

import base64
import logging
import time
from enum import Enum
from typing import Optional, List

from fastapi import APIRouter, HTTPException, Query
from pydantic import BaseModel

from db import get_db
from tags import (
    TagType,
    TagDecoder,
    SpoolEaseEncoder,
    SpoolEaseTagData,
    OpenSpoolTagData,
)
from tags.openspool import OpenSpoolDecoder
from tags.opentag3d import OpenTag3DDecoder, OpenTag3DTagData

logger = logging.getLogger(__name__)
router = APIRouter(prefix="/tags", tags=["tags"])


class TagFormat(str, Enum):
    """Available tag encoding formats."""
    SPOOLEASE_V2 = "SpoolEaseV2"
    OPENSPOOL = "OpenSpool"
    OPENTAG3D = "OpenTag3D"


class TagFormatInfo(BaseModel):
    """Information about a tag format."""
    id: str
    name: str
    description: str
    nfc_type: str
    min_capacity: int  # Minimum tag capacity in bytes
    writable: bool


class EncodeRequest(BaseModel):
    """Request to encode spool data for NFC tag."""
    spool_id: str
    format: TagFormat
    tag_uid: Optional[str] = None  # Hex-encoded tag UID (optional)
    extended: bool = False  # For OpenTag3D: use extended format


class EncodeResponse(BaseModel):
    """Response with encoded tag data."""
    format: str
    spool_id: str
    tag_uid: Optional[str] = None

    # For URL-based formats (SpoolEase)
    url: Optional[str] = None

    # For binary formats (OpenTag3D)
    payload_base64: Optional[str] = None
    payload_hex: Optional[str] = None

    # For JSON formats (OpenSpool)
    json_payload: Optional[str] = None

    # NDEF record info
    ndef_type: Optional[str] = None
    payload_size: int = 0


class DecodeRequest(BaseModel):
    """Request to decode tag data (for testing)."""
    tag_uid: str  # Hex-encoded UID

    # One of these should be provided
    url: Optional[str] = None  # For SpoolEase (NDEF URL)
    json_payload: Optional[str] = None  # For OpenSpool
    payload_base64: Optional[str] = None  # For OpenTag3D binary


class DecodeResponse(BaseModel):
    """Response with decoded tag data."""
    tag_type: str
    tag_uid: str
    uid_base64: str

    # Normalized spool data
    material: Optional[str] = None
    subtype: Optional[str] = None
    color_name: Optional[str] = None
    rgba: Optional[str] = None
    brand: Optional[str] = None
    label_weight: Optional[int] = None
    core_weight: Optional[int] = None
    slicer_filament: Optional[str] = None
    note: Optional[str] = None

    # Raw data based on format
    raw_data: Optional[dict] = None


@router.get("/formats", response_model=List[TagFormatInfo])
async def list_tag_formats():
    """List available tag formats for encoding."""
    return [
        TagFormatInfo(
            id=TagFormat.SPOOLEASE_V2.value,
            name="SpoolEase V2",
            description="URL-based format compatible with SpoolEase app. Stores data in NDEF URL record.",
            nfc_type="NTAG213/215/216",
            min_capacity=137,  # NTAG213
            writable=True,
        ),
        TagFormatInfo(
            id=TagFormat.OPENSPOOL.value,
            name="OpenSpool",
            description="JSON-based open format. Simple, human-readable. Stores data in NDEF MIME record.",
            nfc_type="NTAG215/216",
            min_capacity=504,  # NTAG215
            writable=True,
        ),
        TagFormatInfo(
            id=TagFormat.OPENTAG3D.value,
            name="OpenTag3D",
            description="Binary format with rich metadata. Supports extended fields on larger tags.",
            nfc_type="NTAG213/215/216",
            min_capacity=137,  # NTAG213 for core, 504 for extended
            writable=True,
        ),
    ]


@router.post("/encode", response_model=EncodeResponse)
async def encode_tag(request: EncodeRequest):
    """Encode spool data for writing to NFC tag.

    Fetches the spool from the database and encodes it in the requested format.
    Returns the encoded data ready to write to a tag.

    Args:
        request: Encoding parameters including spool_id and format

    Returns:
        Encoded tag data in the appropriate format
    """
    db = await get_db()
    spool = await db.get_spool(request.spool_id)
    if not spool:
        raise HTTPException(status_code=404, detail="Spool not found")

    # Convert spool to dict for encoding
    spool_dict = spool.model_dump() if hasattr(spool, 'model_dump') else dict(spool)

    # Generate or use provided tag UID
    if request.tag_uid:
        tag_uid_hex = request.tag_uid.replace(":", "").replace(" ", "").upper()
    else:
        # Generate a placeholder UID (7 bytes for NTAG)
        tag_uid_hex = "00000000000000"

    # Convert UID to base64
    uid_bytes = bytes.fromhex(tag_uid_hex)
    uid_base64 = base64.urlsafe_b64encode(uid_bytes).decode("ascii").rstrip("=")

    response = EncodeResponse(
        format=request.format.value,
        spool_id=request.spool_id,
        tag_uid=tag_uid_hex,
    )

    if request.format == TagFormat.SPOOLEASE_V2:
        # Encode as SpoolEase V2 URL
        url = SpoolEaseEncoder.encode(
            tag_id=uid_base64,
            spool_id=request.spool_id,
            material=spool_dict.get("material"),
            material_subtype=spool_dict.get("subtype"),
            color_code=spool_dict.get("rgba"),
            color_name=spool_dict.get("color_name"),
            brand=spool_dict.get("brand"),
            weight_label=spool_dict.get("label_weight"),
            weight_core=spool_dict.get("core_weight"),
            weight_new=spool_dict.get("weight_new"),
            slicer_filament_code=spool_dict.get("slicer_filament"),
            note=spool_dict.get("note"),
            encode_time=int(time.time()),
        )
        response.url = url
        response.ndef_type = "U"  # NDEF URL record
        response.payload_size = len(url)

    elif request.format == TagFormat.OPENSPOOL:
        # Encode as OpenSpool JSON
        # Convert RGBA to RGB for OpenSpool
        color_hex = None
        if spool_dict.get("rgba"):
            rgba = spool_dict["rgba"]
            if len(rgba) >= 6:
                color_hex = rgba[:6]  # Strip alpha

        openspool_data = OpenSpoolTagData(
            tag_id=uid_base64,
            version="1.0",
            material_type=spool_dict.get("material"),
            color_hex=color_hex,
            brand=spool_dict.get("brand"),
            min_temp=None,  # Would need to get from material database
            max_temp=None,
        )

        payload = OpenSpoolDecoder.encode(openspool_data)
        response.json_payload = payload.decode("utf-8")
        response.payload_base64 = base64.b64encode(payload).decode("ascii")
        response.ndef_type = "application/json"
        response.payload_size = len(payload)

    elif request.format == TagFormat.OPENTAG3D:
        # Encode as OpenTag3D binary
        # Parse RGBA color
        primary_color = spool_dict.get("rgba")
        if primary_color and len(primary_color) == 6:
            primary_color = primary_color + "FF"  # Add alpha if missing

        opentag3d_data = OpenTag3DTagData(
            tag_id=uid_base64,
            version=0x0014,  # v0.020
            material_name=spool_dict.get("material"),
            modifiers=spool_dict.get("subtype"),
            manufacturer=spool_dict.get("brand"),
            color_name=spool_dict.get("color_name"),
            primary_color=primary_color,
            weight_g=spool_dict.get("label_weight"),
            diameter_um=1750,  # Default 1.75mm
        )

        payload = OpenTag3DDecoder.encode(opentag3d_data, extended=request.extended)
        response.payload_base64 = base64.b64encode(payload).decode("ascii")
        response.payload_hex = payload.hex().upper()
        response.ndef_type = "application/opentag3d"
        response.payload_size = len(payload)

    return response


@router.post("/decode", response_model=DecodeResponse)
async def decode_tag(request: DecodeRequest):
    """Decode tag data for testing without hardware.

    Accepts tag data in various formats and returns the decoded spool information.
    Useful for testing tag encoding/decoding without physical NFC hardware.

    Args:
        request: Tag data to decode (provide url, json_payload, or payload_base64)

    Returns:
        Decoded tag information
    """
    tag_uid_hex = request.tag_uid.replace(":", "").replace(" ", "").upper()
    uid_bytes = bytes.fromhex(tag_uid_hex)
    uid_base64 = base64.urlsafe_b64encode(uid_bytes).decode("ascii").rstrip("=")

    response = DecodeResponse(
        tag_type=TagType.UNKNOWN.value,
        tag_uid=tag_uid_hex,
        uid_base64=uid_base64,
    )

    if request.url:
        # Decode SpoolEase URL
        result = TagDecoder.decode_ndef_url(tag_uid_hex, request.url)
        if result and result.spoolease_data:
            response.tag_type = result.tag_type.value
            data = result.spoolease_data
            response.material = data.material
            response.subtype = data.material_subtype
            response.color_name = data.color_name
            response.rgba = data.color_code
            response.brand = data.brand
            response.label_weight = data.weight_label
            response.core_weight = data.weight_core
            response.slicer_filament = data.slicer_filament_code
            response.note = data.note
            response.raw_data = data.model_dump()

    elif request.json_payload:
        # Decode OpenSpool JSON
        payload = request.json_payload.encode("utf-8")
        ndef_records = [{"type": "application/json", "payload": payload}]
        result = TagDecoder.decode_ndef_records(tag_uid_hex, ndef_records)
        if result and result.openspool_data:
            response.tag_type = result.tag_type.value
            data = result.openspool_data
            response.material = data.material_type
            response.brand = data.brand
            # Convert RGB to RGBA
            if data.color_hex:
                response.rgba = data.color_hex + "FF" if len(data.color_hex) == 6 else data.color_hex
            response.raw_data = data.model_dump()

    elif request.payload_base64:
        # Decode OpenTag3D binary
        payload = base64.b64decode(request.payload_base64)
        ndef_records = [{"type": "application/opentag3d", "payload": payload}]
        result = TagDecoder.decode_ndef_records(tag_uid_hex, ndef_records)
        if result and result.opentag3d_data:
            response.tag_type = result.tag_type.value
            data = result.opentag3d_data
            response.material = data.get("material_name")
            response.subtype = data.get("modifiers")
            response.color_name = data.get("color_name")
            response.rgba = data.get("primary_color")
            response.brand = data.get("manufacturer")
            response.label_weight = data.get("weight_g")
            response.raw_data = data
    else:
        raise HTTPException(
            status_code=400,
            detail="Must provide one of: url, json_payload, or payload_base64"
        )

    return response


class TagLookupResponse(BaseModel):
    """Response with tag/spool data lookup."""
    found: bool = False
    tag_uid: str = ""

    # Decoded/looked up data
    vendor: Optional[str] = None
    material: Optional[str] = None
    subtype: Optional[str] = None
    color_name: Optional[str] = None
    color_rgba: Optional[int] = None  # RGBA as integer (0xRRGGBBAA)
    spool_weight: Optional[int] = None
    tag_type: Optional[str] = None


@router.get("/decode", response_model=TagLookupResponse)
async def lookup_tag_by_uid(uid: str = Query(..., description="Tag UID in hex (e.g., 87:0D:51:00 or 870D5100)")):
    """Look up spool data by NFC tag UID.

    Searches the spool database for a spool with the given tag_id.
    Used by the simulator to fetch decoded tag data.

    Args:
        uid: Tag UID in hex format (with or without colons)

    Returns:
        Tag/spool data if found, otherwise found=False
    """
    # Normalize UID (remove colons, uppercase)
    tag_uid_hex = uid.replace(":", "").replace(" ", "").upper()

    # Also create colon-separated format for searching
    tag_uid_colon = ":".join([tag_uid_hex[i:i+2] for i in range(0, len(tag_uid_hex), 2)])

    response = TagLookupResponse(tag_uid=tag_uid_hex)

    # Search for spool with this tag_id
    db = await get_db()
    spools = await db.list_spools()

    for spool in spools:
        spool_tag = spool.tag_id if hasattr(spool, 'tag_id') else spool.get('tag_id', '')
        if not spool_tag:
            continue

        # Normalize spool's tag_id for comparison
        spool_tag_normalized = spool_tag.replace(":", "").replace(" ", "").upper()

        if spool_tag_normalized == tag_uid_hex:
            response.found = True

            # Extract spool data
            if hasattr(spool, 'model_dump'):
                spool_dict = spool.model_dump()
            else:
                spool_dict = dict(spool)

            response.vendor = spool_dict.get('brand', '')
            response.material = spool_dict.get('material', '')
            response.subtype = spool_dict.get('subtype', '')
            response.color_name = spool_dict.get('color_name', '')
            response.spool_weight = spool_dict.get('label_weight', 0)
            response.tag_type = spool_dict.get('tag_type', 'database')

            # Convert RGBA hex string to integer
            rgba_str = spool_dict.get('rgba', '')
            if rgba_str and len(rgba_str) >= 6:
                try:
                    # Ensure 8 chars (add FF alpha if missing)
                    if len(rgba_str) == 6:
                        rgba_str = rgba_str + "FF"
                    response.color_rgba = int(rgba_str, 16)
                except ValueError:
                    response.color_rgba = 0

            break

    return response


@router.post("/encode-from-spool/{spool_id}")
async def encode_from_spool(
    spool_id: str,
    format: TagFormat = Query(TagFormat.SPOOLEASE_V2, description="Tag format to use"),
    tag_uid: Optional[str] = Query(None, description="Tag UID in hex (optional)"),
    extended: bool = Query(False, description="Use extended format for OpenTag3D"),
):
    """Shorthand endpoint to encode a spool by ID.

    Convenience endpoint that takes spool_id as path parameter.
    """
    request = EncodeRequest(
        spool_id=spool_id,
        format=format,
        tag_uid=tag_uid,
        extended=extended,
    )
    return await encode_tag(request)
