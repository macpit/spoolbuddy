"""OpenTag3D decoder.

OpenTag3D is an open standard for NFC-based filament identification.
It uses NDEF records with binary-encoded payload.

NDEF record type: "application/opentag3d"

Supported tags: NTAG213 (144 bytes), NTAG215 (504 bytes), NTAG216 (888 bytes), SLIX2

Binary structure (core region, offsets in hex):
  0x00-0x01: Tag Version (uint16 big-endian, e.g., 0x0014 = v0.020)
  0x02-0x06: Material Name (5 bytes UTF-8, e.g., "PLA", "PETG")
  0x07-0x0B: Modifiers (5 bytes UTF-8, e.g., "CF", "GF", "HT")
  0x0C-0x1A: Reserved (15 bytes)
  0x1B-0x2A: Manufacturer (16 bytes UTF-8)
  0x2B-0x4A: Color Name (32 bytes UTF-8)
  0x4B-0x4E: Primary Color RGBA (4 bytes sRGB)
  0x4F-0x52: Secondary Color 1 RGBA (4 bytes)
  0x53-0x56: Secondary Color 2 RGBA (4 bytes)
  0x57-0x5A: Secondary Color 3 RGBA (4 bytes)
  0x5B: Reserved (1 byte)
  0x5C-0x5D: Target Diameter (uint16 big-endian, micrometers)
  0x5E-0x5F: Target Weight (uint16 big-endian, grams)
  0x60: Print Temperature (uint8, Celsius / 5)
  0x61: Bed Temperature (uint8, Celsius / 5)
  0x62-0x63: Density (uint16 big-endian, g/cm³ * 1000)
  0x64-0x65: Transmission Distance (uint16 big-endian, mm * 10)

Extended region (0x70+, NTAG215/216/SLIX2 only):
  0x70-0x8F: Online Data URL (32 bytes ASCII, without https://)
  0x90-0x9F: Serial/Batch ID (16 bytes UTF-8)
  0xA0-0xA3: Manufacture Date (YYYY, MM, DD, reserved)
  ... additional fields

Reference: https://opentag3d.info/spec
"""

import base64
import logging
import struct
from typing import Optional, List

from .models import SpoolFromTag, TagType

logger = logging.getLogger(__name__)


class OpenTag3DTagData:
    """Parsed data from an OpenTag3D tag."""

    def __init__(
        self,
        tag_id: str,
        version: int = 0,
        material_name: Optional[str] = None,
        modifiers: Optional[str] = None,
        manufacturer: Optional[str] = None,
        color_name: Optional[str] = None,
        primary_color: Optional[str] = None,
        secondary_colors: Optional[List[str]] = None,
        diameter_um: Optional[int] = None,
        weight_g: Optional[int] = None,
        print_temp_c: Optional[int] = None,
        bed_temp_c: Optional[int] = None,
        density: Optional[float] = None,
        # Extended fields
        url: Optional[str] = None,
        serial: Optional[str] = None,
        manufacture_date: Optional[str] = None,
    ):
        self.tag_id = tag_id
        self.version = version
        self.material_name = material_name
        self.modifiers = modifiers
        self.manufacturer = manufacturer
        self.color_name = color_name
        self.primary_color = primary_color
        self.secondary_colors = secondary_colors
        self.diameter_um = diameter_um
        self.weight_g = weight_g
        self.print_temp_c = print_temp_c
        self.bed_temp_c = bed_temp_c
        self.density = density
        self.url = url
        self.serial = serial
        self.manufacture_date = manufacture_date


# Slicer filament code mapping
MATERIAL_TO_SLICER = {
    "PLA": "GFL00",
    "PETG": "GFL01",
    "ABS": "GFL02",
    "ASA": "GFL03",
    "PC": "GFL04",
    "TPU": "GFL05",
    "PVA": "GFL06",
    "PA": "GFL07",
    "HIPS": "GFL14",
}


class OpenTag3DDecoder:
    """Decoder for OpenTag3D NDEF binary records."""

    RECORD_TYPE = "application/opentag3d"

    # Core region offsets
    OFF_VERSION = 0x00
    OFF_MATERIAL = 0x02
    OFF_MODIFIERS = 0x07
    OFF_MANUFACTURER = 0x1B
    OFF_COLOR_NAME = 0x2B
    OFF_COLOR_PRIMARY = 0x4B
    OFF_COLOR_2 = 0x4F
    OFF_COLOR_3 = 0x53
    OFF_COLOR_4 = 0x57
    OFF_DIAMETER = 0x5C
    OFF_WEIGHT = 0x5E
    OFF_PRINT_TEMP = 0x60
    OFF_BED_TEMP = 0x61
    OFF_DENSITY = 0x62
    OFF_TRANSMISSION = 0x64

    # Extended region offsets
    OFF_URL = 0x70
    OFF_SERIAL = 0x90
    OFF_MFG_DATE = 0xA0

    # Field sizes
    SIZE_MATERIAL = 5
    SIZE_MODIFIERS = 5
    SIZE_MANUFACTURER = 16
    SIZE_COLOR_NAME = 32
    SIZE_COLOR = 4
    SIZE_URL = 32
    SIZE_SERIAL = 16

    @staticmethod
    def can_decode(ndef_records: list) -> bool:
        """Check if NDEF contains an OpenTag3D record."""
        for record in ndef_records:
            record_type = record.get("type", b"")
            if isinstance(record_type, bytes):
                record_type = record_type.decode("utf-8", errors="ignore")
            if record_type == OpenTag3DDecoder.RECORD_TYPE:
                return True
        return False

    @staticmethod
    def _read_string(data: bytes, offset: int, length: int) -> Optional[str]:
        """Read a null-terminated or fixed-length UTF-8 string."""
        if offset + length > len(data):
            return None
        raw = data[offset:offset + length]
        # Find null terminator
        null_idx = raw.find(b'\x00')
        if null_idx >= 0:
            raw = raw[:null_idx]
        try:
            s = raw.decode("utf-8").strip()
            return s if s else None
        except UnicodeDecodeError:
            return None

    @staticmethod
    def _read_uint16_be(data: bytes, offset: int) -> Optional[int]:
        """Read a big-endian uint16."""
        if offset + 2 > len(data):
            return None
        value = struct.unpack(">H", data[offset:offset + 2])[0]
        return value if value > 0 else None

    @staticmethod
    def _read_uint8(data: bytes, offset: int) -> Optional[int]:
        """Read a uint8."""
        if offset >= len(data):
            return None
        value = data[offset]
        return value if value > 0 else None

    @staticmethod
    def _read_color(data: bytes, offset: int) -> Optional[str]:
        """Read a 4-byte RGBA color as hex string."""
        if offset + 4 > len(data):
            return None
        rgba = data[offset:offset + 4]
        # Check if all zeros (no color)
        if rgba == b'\x00\x00\x00\x00':
            return None
        return rgba.hex().upper()

    @staticmethod
    def decode(uid_hex: str, payload: bytes) -> Optional[OpenTag3DTagData]:
        """Decode OpenTag3D binary payload.

        Args:
            uid_hex: Hex-encoded tag UID
            payload: Raw NDEF record payload (binary data)

        Returns:
            Parsed tag data, or None if decoding fails
        """
        try:
            # Convert UID to base64
            uid_bytes = bytes.fromhex(uid_hex)
            uid_base64 = base64.urlsafe_b64encode(uid_bytes).decode("ascii").rstrip("=")

            # Need at least core region
            if len(payload) < 0x66:
                logger.warning(f"OpenTag3D payload too short: {len(payload)} bytes")
                return None

            # Parse version
            version = OpenTag3DDecoder._read_uint16_be(payload, OpenTag3DDecoder.OFF_VERSION) or 0

            # Parse strings
            material_name = OpenTag3DDecoder._read_string(
                payload, OpenTag3DDecoder.OFF_MATERIAL, OpenTag3DDecoder.SIZE_MATERIAL
            )
            modifiers = OpenTag3DDecoder._read_string(
                payload, OpenTag3DDecoder.OFF_MODIFIERS, OpenTag3DDecoder.SIZE_MODIFIERS
            )
            manufacturer = OpenTag3DDecoder._read_string(
                payload, OpenTag3DDecoder.OFF_MANUFACTURER, OpenTag3DDecoder.SIZE_MANUFACTURER
            )
            color_name = OpenTag3DDecoder._read_string(
                payload, OpenTag3DDecoder.OFF_COLOR_NAME, OpenTag3DDecoder.SIZE_COLOR_NAME
            )

            # Parse colors
            primary_color = OpenTag3DDecoder._read_color(payload, OpenTag3DDecoder.OFF_COLOR_PRIMARY)
            secondary_colors = []
            for off in [OpenTag3DDecoder.OFF_COLOR_2, OpenTag3DDecoder.OFF_COLOR_3, OpenTag3DDecoder.OFF_COLOR_4]:
                color = OpenTag3DDecoder._read_color(payload, off)
                if color:
                    secondary_colors.append(color)

            # Parse numeric fields
            diameter_um = OpenTag3DDecoder._read_uint16_be(payload, OpenTag3DDecoder.OFF_DIAMETER)
            weight_g = OpenTag3DDecoder._read_uint16_be(payload, OpenTag3DDecoder.OFF_WEIGHT)

            # Temperature is stored as Celsius / 5
            print_temp_raw = OpenTag3DDecoder._read_uint8(payload, OpenTag3DDecoder.OFF_PRINT_TEMP)
            print_temp_c = print_temp_raw * 5 if print_temp_raw else None

            bed_temp_raw = OpenTag3DDecoder._read_uint8(payload, OpenTag3DDecoder.OFF_BED_TEMP)
            bed_temp_c = bed_temp_raw * 5 if bed_temp_raw else None

            # Density is stored as g/cm³ * 1000
            density_raw = OpenTag3DDecoder._read_uint16_be(payload, OpenTag3DDecoder.OFF_DENSITY)
            density = density_raw / 1000.0 if density_raw else None

            # Extended region (if present)
            url = None
            serial = None
            manufacture_date = None

            if len(payload) >= OpenTag3DDecoder.OFF_URL + OpenTag3DDecoder.SIZE_URL:
                url = OpenTag3DDecoder._read_string(
                    payload, OpenTag3DDecoder.OFF_URL, OpenTag3DDecoder.SIZE_URL
                )
                if url:
                    url = "https://" + url

            if len(payload) >= OpenTag3DDecoder.OFF_SERIAL + OpenTag3DDecoder.SIZE_SERIAL:
                serial = OpenTag3DDecoder._read_string(
                    payload, OpenTag3DDecoder.OFF_SERIAL, OpenTag3DDecoder.SIZE_SERIAL
                )

            if len(payload) >= OpenTag3DDecoder.OFF_MFG_DATE + 4:
                year = OpenTag3DDecoder._read_uint16_be(payload, OpenTag3DDecoder.OFF_MFG_DATE)
                month = OpenTag3DDecoder._read_uint8(payload, OpenTag3DDecoder.OFF_MFG_DATE + 2)
                day = OpenTag3DDecoder._read_uint8(payload, OpenTag3DDecoder.OFF_MFG_DATE + 3)
                if year and month and day:
                    manufacture_date = f"{year:04d}-{month:02d}-{day:02d}"

            return OpenTag3DTagData(
                tag_id=uid_base64,
                version=version,
                material_name=material_name,
                modifiers=modifiers,
                manufacturer=manufacturer,
                color_name=color_name,
                primary_color=primary_color,
                secondary_colors=secondary_colors if secondary_colors else None,
                diameter_um=diameter_um,
                weight_g=weight_g,
                print_temp_c=print_temp_c,
                bed_temp_c=bed_temp_c,
                density=density,
                url=url,
                serial=serial,
                manufacture_date=manufacture_date,
            )

        except Exception as e:
            logger.error(f"Failed to decode OpenTag3D: {e}")
            return None

    @staticmethod
    def to_spool(data: OpenTag3DTagData) -> SpoolFromTag:
        """Convert OpenTag3D data to normalized spool data."""
        # Combine material name with modifiers
        material = data.material_name
        subtype = data.modifiers

        # Get slicer filament code
        material_upper = (data.material_name or "").upper()
        slicer_code = MATERIAL_TO_SLICER.get(material_upper, "")

        # Build note with extra info
        notes = []

        if data.print_temp_c:
            if data.bed_temp_c:
                notes.append(f"Print: {data.print_temp_c}C, Bed: {data.bed_temp_c}C")
            else:
                notes.append(f"Print temp: {data.print_temp_c}C")

        if data.density:
            notes.append(f"Density: {data.density:.2f} g/cm³")

        if data.diameter_um:
            diameter_mm = data.diameter_um / 1000.0
            notes.append(f"Diameter: {diameter_mm:.2f}mm")

        if data.serial:
            notes.append(f"S/N: {data.serial}")

        if data.manufacture_date:
            notes.append(f"Mfg: {data.manufacture_date}")

        if data.url:
            notes.append(f"URL: {data.url}")

        # Track missing fields
        missing = []
        if not material:
            missing.append("Material")
        if not slicer_code:
            missing.append("Slicer Filament")
        if not data.color_name and not data.primary_color:
            missing.append("Color")
        if not data.manufacturer:
            missing.append("Brand")

        if missing:
            notes.append(f"Missing: {', '.join(missing)}")

        note = "; ".join(notes) if notes else None

        return SpoolFromTag(
            tag_id=data.tag_id,
            tag_type="OpenTag3D",
            material=material,
            subtype=subtype,
            color_name=data.color_name,
            rgba=data.primary_color,
            brand=data.manufacturer,
            label_weight=data.weight_g,
            core_weight=None,  # OpenTag3D doesn't store this
            weight_new=None,
            slicer_filament=slicer_code if slicer_code else None,
            note=note,
            data_origin="OpenTag3D",
        )

    @staticmethod
    def encode(data: OpenTag3DTagData, extended: bool = False) -> bytes:
        """Encode OpenTag3D data to binary bytes for writing to tag.

        Args:
            data: Tag data to encode
            extended: If True, include extended region (requires NTAG215+)

        Returns:
            Binary bytes ready to write as NDEF payload
        """
        # Core region size
        core_size = 0x66  # 102 bytes
        ext_size = 0xA4 if extended else 0  # 164 bytes for extended

        payload = bytearray(core_size + ext_size)

        # Version (default to 0x0014 = v0.020)
        struct.pack_into(">H", payload, OpenTag3DDecoder.OFF_VERSION, data.version or 0x0014)

        # Material name
        if data.material_name:
            mat_bytes = data.material_name.encode("utf-8")[:OpenTag3DDecoder.SIZE_MATERIAL]
            payload[OpenTag3DDecoder.OFF_MATERIAL:OpenTag3DDecoder.OFF_MATERIAL + len(mat_bytes)] = mat_bytes

        # Modifiers
        if data.modifiers:
            mod_bytes = data.modifiers.encode("utf-8")[:OpenTag3DDecoder.SIZE_MODIFIERS]
            payload[OpenTag3DDecoder.OFF_MODIFIERS:OpenTag3DDecoder.OFF_MODIFIERS + len(mod_bytes)] = mod_bytes

        # Manufacturer
        if data.manufacturer:
            mfg_bytes = data.manufacturer.encode("utf-8")[:OpenTag3DDecoder.SIZE_MANUFACTURER]
            payload[OpenTag3DDecoder.OFF_MANUFACTURER:OpenTag3DDecoder.OFF_MANUFACTURER + len(mfg_bytes)] = mfg_bytes

        # Color name
        if data.color_name:
            color_bytes = data.color_name.encode("utf-8")[:OpenTag3DDecoder.SIZE_COLOR_NAME]
            payload[OpenTag3DDecoder.OFF_COLOR_NAME:OpenTag3DDecoder.OFF_COLOR_NAME + len(color_bytes)] = color_bytes

        # Primary color (RGBA hex to bytes)
        if data.primary_color:
            try:
                color_bytes = bytes.fromhex(data.primary_color)
                payload[OpenTag3DDecoder.OFF_COLOR_PRIMARY:OpenTag3DDecoder.OFF_COLOR_PRIMARY + 4] = color_bytes[:4]
            except ValueError:
                pass

        # Secondary colors
        if data.secondary_colors:
            offsets = [OpenTag3DDecoder.OFF_COLOR_2, OpenTag3DDecoder.OFF_COLOR_3, OpenTag3DDecoder.OFF_COLOR_4]
            for i, color in enumerate(data.secondary_colors[:3]):
                try:
                    color_bytes = bytes.fromhex(color)
                    payload[offsets[i]:offsets[i] + 4] = color_bytes[:4]
                except ValueError:
                    pass

        # Diameter
        if data.diameter_um:
            struct.pack_into(">H", payload, OpenTag3DDecoder.OFF_DIAMETER, data.diameter_um)

        # Weight
        if data.weight_g:
            struct.pack_into(">H", payload, OpenTag3DDecoder.OFF_WEIGHT, data.weight_g)

        # Print temperature (Celsius / 5)
        if data.print_temp_c:
            payload[OpenTag3DDecoder.OFF_PRINT_TEMP] = data.print_temp_c // 5

        # Bed temperature (Celsius / 5)
        if data.bed_temp_c:
            payload[OpenTag3DDecoder.OFF_BED_TEMP] = data.bed_temp_c // 5

        # Density (g/cm³ * 1000)
        if data.density:
            struct.pack_into(">H", payload, OpenTag3DDecoder.OFF_DENSITY, int(data.density * 1000))

        # Extended region
        if extended:
            if data.url:
                # Remove https:// prefix if present
                url = data.url
                if url.startswith("https://"):
                    url = url[8:]
                elif url.startswith("http://"):
                    url = url[7:]
                url_bytes = url.encode("ascii")[:OpenTag3DDecoder.SIZE_URL]
                payload[OpenTag3DDecoder.OFF_URL:OpenTag3DDecoder.OFF_URL + len(url_bytes)] = url_bytes

            if data.serial:
                serial_bytes = data.serial.encode("utf-8")[:OpenTag3DDecoder.SIZE_SERIAL]
                payload[OpenTag3DDecoder.OFF_SERIAL:OpenTag3DDecoder.OFF_SERIAL + len(serial_bytes)] = serial_bytes

            if data.manufacture_date:
                try:
                    parts = data.manufacture_date.split("-")
                    year = int(parts[0])
                    month = int(parts[1])
                    day = int(parts[2])
                    struct.pack_into(">H", payload, OpenTag3DDecoder.OFF_MFG_DATE, year)
                    payload[OpenTag3DDecoder.OFF_MFG_DATE + 2] = month
                    payload[OpenTag3DDecoder.OFF_MFG_DATE + 3] = day
                except (ValueError, IndexError):
                    pass

        return bytes(payload)
