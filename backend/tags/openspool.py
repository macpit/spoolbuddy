"""OpenSpool decoder.

OpenSpool is an open standard for NFC-based filament identification.
It uses NDEF records with JSON payload in the simplest possible format.

NDEF record type: "application/json"
Protocol identifier: "protocol": "openspool"

JSON structure:
{
    "protocol": "openspool",
    "version": "1.0",
    "type": "PLA",
    "color_hex": "FFAABB",
    "brand": "Generic",
    "min_temp": "220",
    "max_temp": "240"
}

Supported tag types: NTAG215, NTAG216 (>500 bytes capacity)

Reference: https://github.com/spuder/OpenSpool
"""

import base64
import json
import logging
from typing import Optional

from .models import OpenSpoolTagData, SpoolFromTag, TagType

logger = logging.getLogger(__name__)

# Slicer filament code mapping (same as OpenPrintTag)
MATERIAL_TO_SLICER = {
    "PLA": "GFL00",
    "PETG": "GFL01",
    "ABS": "GFL02",
    "ASA": "GFL03",
    "PC": "GFL04",
    "TPU": "GFL05",
    "PVA": "GFL06",
    "PA": "GFL07",
    "PAHT-CF": "GFL08",
    "PET-CF": "GFL09",
    "PA-CF": "GFL10",
    "PLA-CF": "GFL11",
}


class OpenSpoolDecoder:
    """Decoder for OpenSpool NDEF JSON records."""

    RECORD_TYPE = "application/json"
    PROTOCOL_ID = "openspool"

    @staticmethod
    def can_decode_json(data: dict) -> bool:
        """Check if JSON data is an OpenSpool record.

        Args:
            data: Parsed JSON dict

        Returns:
            True if this is an OpenSpool record
        """
        return data.get("protocol") == OpenSpoolDecoder.PROTOCOL_ID

    @staticmethod
    def can_decode_payload(payload: bytes) -> bool:
        """Check if NDEF payload is an OpenSpool JSON record.

        Args:
            payload: Raw NDEF payload bytes

        Returns:
            True if this appears to be an OpenSpool record
        """
        try:
            text = payload.decode("utf-8")
            data = json.loads(text)
            return OpenSpoolDecoder.can_decode_json(data)
        except (UnicodeDecodeError, json.JSONDecodeError):
            return False

    @staticmethod
    def decode(uid_hex: str, payload: bytes) -> Optional[OpenSpoolTagData]:
        """Decode OpenSpool JSON payload.

        Args:
            uid_hex: Hex-encoded tag UID
            payload: Raw NDEF record payload (JSON data)

        Returns:
            Parsed tag data, or None if decoding fails
        """
        try:
            # Convert UID to base64
            uid_bytes = bytes.fromhex(uid_hex)
            uid_base64 = base64.urlsafe_b64encode(uid_bytes).decode("ascii").rstrip("=")

            # Decode JSON
            text = payload.decode("utf-8")
            data = json.loads(text)

            # Verify protocol
            if data.get("protocol") != OpenSpoolDecoder.PROTOCOL_ID:
                logger.debug("Not an OpenSpool record: missing protocol field")
                return None

            # Parse temperature fields (stored as strings in OpenSpool)
            min_temp = None
            max_temp = None
            if "min_temp" in data:
                try:
                    min_temp = int(data["min_temp"])
                except (ValueError, TypeError):
                    pass
            if "max_temp" in data:
                try:
                    max_temp = int(data["max_temp"])
                except (ValueError, TypeError):
                    pass

            return OpenSpoolTagData(
                tag_id=uid_base64,
                version=data.get("version", "1.0"),
                material_type=data.get("type"),
                color_hex=data.get("color_hex"),
                brand=data.get("brand"),
                min_temp=min_temp,
                max_temp=max_temp,
            )

        except (UnicodeDecodeError, json.JSONDecodeError) as e:
            logger.error(f"Failed to decode OpenSpool JSON: {e}")
            return None
        except Exception as e:
            logger.error(f"Failed to decode OpenSpool: {e}")
            return None

    @staticmethod
    def decode_json(uid_hex: str, data: dict) -> Optional[OpenSpoolTagData]:
        """Decode OpenSpool from pre-parsed JSON dict.

        Args:
            uid_hex: Hex-encoded tag UID
            data: Parsed JSON dict

        Returns:
            Parsed tag data, or None if decoding fails
        """
        try:
            # Convert UID to base64
            uid_bytes = bytes.fromhex(uid_hex)
            uid_base64 = base64.urlsafe_b64encode(uid_bytes).decode("ascii").rstrip("=")

            # Verify protocol
            if data.get("protocol") != OpenSpoolDecoder.PROTOCOL_ID:
                return None

            # Parse temperature fields
            min_temp = None
            max_temp = None
            if "min_temp" in data:
                try:
                    min_temp = int(data["min_temp"])
                except (ValueError, TypeError):
                    pass
            if "max_temp" in data:
                try:
                    max_temp = int(data["max_temp"])
                except (ValueError, TypeError):
                    pass

            return OpenSpoolTagData(
                tag_id=uid_base64,
                version=data.get("version", "1.0"),
                material_type=data.get("type"),
                color_hex=data.get("color_hex"),
                brand=data.get("brand"),
                min_temp=min_temp,
                max_temp=max_temp,
            )

        except Exception as e:
            logger.error(f"Failed to decode OpenSpool from dict: {e}")
            return None

    @staticmethod
    def to_spool(data: OpenSpoolTagData) -> SpoolFromTag:
        """Convert OpenSpool data to normalized spool data."""
        # Convert color_hex (RGB) to RGBA
        rgba = None
        if data.color_hex:
            # OpenSpool uses RGB without alpha
            color = data.color_hex.upper().lstrip("#")
            if len(color) == 6:
                rgba = color + "FF"  # Add full opacity
            elif len(color) == 8:
                rgba = color  # Already has alpha

        # Get slicer filament code
        material_upper = (data.material_type or "").upper()
        slicer_code = MATERIAL_TO_SLICER.get(material_upper, "")

        # Build note with temperature info and missing fields
        notes = []
        if data.min_temp and data.max_temp:
            notes.append(f"Temp: {data.min_temp}-{data.max_temp}C")
        elif data.min_temp:
            notes.append(f"Min temp: {data.min_temp}C")
        elif data.max_temp:
            notes.append(f"Max temp: {data.max_temp}C")

        missing = []
        if not data.material_type:
            missing.append("Material")
        if not slicer_code:
            missing.append("Slicer Filament")
        if not rgba:
            missing.append("Color")
        if not data.brand:
            missing.append("Brand")

        if missing:
            notes.append(f"Missing: {', '.join(missing)}")

        note = "; ".join(notes) if notes else None

        return SpoolFromTag(
            tag_id=data.tag_id,
            tag_type=TagType.OPENSPOOL.value,
            material=data.material_type,
            subtype=None,
            color_name=None,  # OpenSpool doesn't have color names
            rgba=rgba,
            brand=data.brand,
            label_weight=None,  # OpenSpool doesn't store weight
            core_weight=None,
            weight_new=None,
            slicer_filament=slicer_code if slicer_code else None,
            note=note,
            data_origin=TagType.OPENSPOOL.value,
        )

    @staticmethod
    def encode(data: OpenSpoolTagData) -> bytes:
        """Encode OpenSpool data to JSON bytes for writing to tag.

        Args:
            data: Tag data to encode

        Returns:
            JSON bytes ready to write as NDEF payload
        """
        obj = {
            "protocol": OpenSpoolDecoder.PROTOCOL_ID,
            "version": data.version,
        }

        if data.material_type:
            obj["type"] = data.material_type
        if data.color_hex:
            obj["color_hex"] = data.color_hex
        if data.brand:
            obj["brand"] = data.brand
        if data.min_temp is not None:
            obj["min_temp"] = str(data.min_temp)
        if data.max_temp is not None:
            obj["max_temp"] = str(data.max_temp)

        return json.dumps(obj, separators=(",", ":")).encode("utf-8")
