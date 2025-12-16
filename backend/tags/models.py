"""Tag data models."""

from enum import Enum
from typing import Optional, Dict, List
from pydantic import BaseModel


class TagType(str, Enum):
    """Type of NFC tag."""
    SPOOLEASE_V1 = "SpoolEaseV1"
    SPOOLEASE_V2 = "SpoolEaseV2"
    BAMBULAB = "Bambu Lab"
    OPENPRINTTAG = "OpenPrintTag"
    OPENSPOOL = "OpenSpool"
    OPENTAG3D = "OpenTag3D"
    UNKNOWN = "Unknown"


class NfcTagType(str, Enum):
    """Physical NFC tag type."""
    NTAG = "NTAG"  # NTAG213/215/216
    MIFARE_CLASSIC_1K = "MifareClassic1K"
    MIFARE_CLASSIC_4K = "MifareClassic4K"
    UNKNOWN = "Unknown"


class SpoolEaseTagData(BaseModel):
    """Parsed data from a SpoolEase tag (V1 or V2)."""
    version: int = 2  # 1 or 2
    tag_id: str  # Base64-encoded UID
    spool_id: Optional[str] = None
    material: Optional[str] = None
    material_subtype: Optional[str] = None
    color_code: Optional[str] = None  # RGBA hex (e.g., "FF0000FF")
    color_name: Optional[str] = None
    brand: Optional[str] = None
    weight_label: Optional[int] = None  # Advertised weight in grams
    weight_core: Optional[int] = None  # Empty spool weight
    weight_new: Optional[int] = None  # Actual weight when full
    slicer_filament_code: Optional[str] = None  # e.g., "GFL99"
    slicer_filament_name: Optional[str] = None
    note: Optional[str] = None
    encode_time: Optional[int] = None  # Unix timestamp
    added_time: Optional[int] = None  # Unix timestamp


class BambuLabTagData(BaseModel):
    """Parsed data from a Bambu Lab RFID tag."""
    tag_id: str  # Hex-encoded UID
    material_variant_id: Optional[str] = None  # e.g., "A00-G1"
    material_id: Optional[str] = None  # e.g., "GFA00"
    filament_type: Optional[str] = None  # e.g., "PLA"
    detailed_filament_type: Optional[str] = None  # e.g., "PLA Basic"
    color_rgba: Optional[str] = None  # e.g., "FF0000FF"
    color_rgba2: Optional[str] = None  # Secondary color for multi-color
    spool_weight: Optional[int] = None  # Empty spool weight in grams
    # Raw block data for reference
    blocks: Optional[Dict[int, bytes]] = None


class OpenPrintTagData(BaseModel):
    """Parsed data from an OpenPrintTag."""
    tag_id: str  # Base64-encoded UID
    material_name: Optional[str] = None
    material_type: Optional[str] = None  # e.g., "PLA", "PETG"
    brand_name: Optional[str] = None
    primary_color: Optional[str] = None  # RGBA hex
    secondary_colors: Optional[List[str]] = None
    nominal_weight: Optional[int] = None  # Advertised weight
    actual_weight: Optional[int] = None  # Real weight when full
    empty_weight: Optional[int] = None  # Empty spool weight


class OpenSpoolTagData(BaseModel):
    """Parsed data from an OpenSpool tag.

    OpenSpool uses JSON in NDEF MIME records (application/json).
    Format: {"protocol": "openspool", "version": "1.0", "type": "PLA", ...}
    """
    tag_id: str  # Base64-encoded UID
    version: str = "1.0"  # Protocol version
    material_type: Optional[str] = None  # e.g., "PLA", "PETG"
    color_hex: Optional[str] = None  # RGB hex without alpha (e.g., "FFAABB")
    brand: Optional[str] = None
    min_temp: Optional[int] = None  # Minimum print temperature
    max_temp: Optional[int] = None  # Maximum print temperature


class TagReadResult(BaseModel):
    """Result of reading an NFC tag."""
    uid: str  # Hex-encoded UID
    uid_base64: str  # Base64-encoded UID (for SpoolEase compatibility)
    nfc_type: NfcTagType
    tag_type: TagType

    # Parsed data (one of these will be set based on tag_type)
    spoolease_data: Optional[SpoolEaseTagData] = None
    bambulab_data: Optional[BambuLabTagData] = None
    openprinttag_data: Optional[OpenPrintTagData] = None
    openspool_data: Optional[OpenSpoolTagData] = None
    opentag3d_data: Optional[dict] = None  # Uses OpenTag3DTagData from opentag3d module

    # Raw data
    ndef_message: Optional[bytes] = None  # Raw NDEF for NTAG
    mifare_blocks: Optional[Dict[int, bytes]] = None  # Raw blocks for Mifare

    # For matching to existing spools
    matched_spool_id: Optional[str] = None


class SpoolFromTag(BaseModel):
    """Spool data extracted from any tag type, normalized for database."""
    tag_id: str
    tag_type: str
    material: Optional[str] = None
    subtype: Optional[str] = None
    color_name: Optional[str] = None
    rgba: Optional[str] = None
    brand: Optional[str] = None
    label_weight: Optional[int] = None
    core_weight: Optional[int] = None
    weight_new: Optional[int] = None
    slicer_filament: Optional[str] = None
    note: Optional[str] = None
    data_origin: Optional[str] = None
