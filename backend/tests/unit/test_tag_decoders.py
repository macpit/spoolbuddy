"""Unit tests for NFC tag decoders."""

import json
import struct
import pytest
from tags import (
    TagType,
    TagDecoder,
    SpoolEaseDecoder,
    OpenSpoolDecoder,
    OpenPrintTagDecoder,
    OpenTag3DDecoder,
    OpenTag3DTagData,
)
from tags.bambulab import BambuLabDecoder


class TestSpoolEaseDecoder:
    """Tests for SpoolEase V1/V2 decoder."""

    def test_can_decode_v2_url(self):
        """Should recognize SpoolEase V2 URLs."""
        url = "https://info.filament3d.org/V2/?TG=abc&M=PLA"
        assert SpoolEaseDecoder.can_decode(url) is True

    def test_can_decode_v1_url(self):
        """Should recognize SpoolEase V1 URLs."""
        url = "https://info.filament3d.org/V1?ID=123"
        assert SpoolEaseDecoder.can_decode(url) is True

    def test_cannot_decode_random_url(self):
        """Should reject non-SpoolEase URLs."""
        url = "https://example.com/test"
        assert SpoolEaseDecoder.can_decode(url) is False

    def test_decode_v2_full(self):
        """Should decode complete V2 URL."""
        uid_hex = "04AABBCCDD1122"
        url = (
            "https://info.filament3d.org/V2/?"
            "TG=BKq7zN0RIg"
            "&ID=spool123"
            "&M=PLA"
            "&MS=Silk"
            "&CC=FF0000FF"
            "&CN=Red"
            "&B=Polymaker"
            "&WL=1000"
            "&WE=200"
            "&WF=1200"
            "&SC=GFL99"
            "&SN=Generic%20PLA"
        )
        result = SpoolEaseDecoder.decode(url, uid_hex)

        assert result is not None
        assert result.version == 2
        assert result.material == "PLA"
        assert result.material_subtype == "Silk"
        assert result.color_code == "FF0000FF"
        assert result.color_name == "Red"
        assert result.brand == "Polymaker"
        assert result.weight_label == 1000
        assert result.weight_core == 200
        assert result.weight_new == 1200
        assert result.slicer_filament_code == "GFL99"
        assert result.slicer_filament_name == "Generic PLA"

    def test_decode_v2_minimal(self):
        """Should decode V2 URL with minimal fields."""
        uid_hex = "04AABBCCDD1122"
        url = "https://info.filament3d.org/V2/?M=PETG"
        result = SpoolEaseDecoder.decode(url, uid_hex)

        assert result is not None
        assert result.version == 2
        assert result.material == "PETG"
        assert result.brand is None

    def test_to_spool_conversion(self):
        """Should convert SpoolEase data to normalized spool."""
        uid_hex = "04AABBCCDD1122"
        url = "https://info.filament3d.org/V2/?M=ABS&CC=0000FFFF&B=eSUN&WL=1000"
        data = SpoolEaseDecoder.decode(url, uid_hex)
        spool = SpoolEaseDecoder.to_spool(data)

        assert spool.material == "ABS"
        assert spool.rgba == "0000FFFF"
        assert spool.brand == "eSUN"
        assert spool.label_weight == 1000


class TestOpenSpoolDecoder:
    """Tests for OpenSpool JSON decoder."""

    def test_can_decode_valid_payload(self):
        """Should recognize OpenSpool JSON."""
        payload = json.dumps({
            "protocol": "openspool",
            "version": "1.0",
            "type": "PLA"
        }).encode("utf-8")
        assert OpenSpoolDecoder.can_decode_payload(payload) is True

    def test_cannot_decode_other_json(self):
        """Should reject JSON without openspool protocol."""
        payload = json.dumps({"name": "test"}).encode("utf-8")
        assert OpenSpoolDecoder.can_decode_payload(payload) is False

    def test_cannot_decode_invalid_json(self):
        """Should reject invalid JSON."""
        payload = b"not json"
        assert OpenSpoolDecoder.can_decode_payload(payload) is False

    def test_decode_full(self):
        """Should decode complete OpenSpool JSON."""
        uid_hex = "04AABBCCDD1122"
        payload = json.dumps({
            "protocol": "openspool",
            "version": "1.0",
            "type": "PETG",
            "color_hex": "FF5733",
            "brand": "Generic",
            "min_temp": "230",
            "max_temp": "250"
        }).encode("utf-8")

        result = OpenSpoolDecoder.decode(uid_hex, payload)

        assert result is not None
        assert result.version == "1.0"
        assert result.material_type == "PETG"
        assert result.color_hex == "FF5733"
        assert result.brand == "Generic"
        assert result.min_temp == 230
        assert result.max_temp == 250

    def test_decode_minimal(self):
        """Should decode minimal OpenSpool JSON."""
        uid_hex = "04AABBCCDD1122"
        payload = json.dumps({
            "protocol": "openspool",
            "type": "TPU"
        }).encode("utf-8")

        result = OpenSpoolDecoder.decode(uid_hex, payload)

        assert result is not None
        assert result.material_type == "TPU"
        assert result.brand is None

    def test_to_spool_conversion(self):
        """Should convert OpenSpool data to normalized spool."""
        uid_hex = "04AABBCCDD1122"
        payload = json.dumps({
            "protocol": "openspool",
            "type": "PLA",
            "color_hex": "00FF00",
            "brand": "eSUN",
            "min_temp": "200",
            "max_temp": "220"
        }).encode("utf-8")

        data = OpenSpoolDecoder.decode(uid_hex, payload)
        spool = OpenSpoolDecoder.to_spool(data)

        assert spool.material == "PLA"
        assert spool.rgba == "00FF00FF"  # Alpha added
        assert spool.brand == "eSUN"
        assert "200-220C" in spool.note

    def test_encode_roundtrip(self):
        """Should encode and decode back to same data."""
        from tags.models import OpenSpoolTagData

        original = OpenSpoolTagData(
            tag_id="test",
            version="1.0",
            material_type="ABS",
            color_hex="AABBCC",
            brand="Test Brand",
            min_temp=240,
            max_temp=260
        )

        encoded = OpenSpoolDecoder.encode(original)
        decoded = OpenSpoolDecoder.decode("04AABBCCDD1122", encoded)

        assert decoded.material_type == original.material_type
        assert decoded.color_hex == original.color_hex
        assert decoded.brand == original.brand
        assert decoded.min_temp == original.min_temp
        assert decoded.max_temp == original.max_temp


class TestOpenTag3DDecoder:
    """Tests for OpenTag3D binary decoder."""

    def _create_payload(
        self,
        material: str = "PLA",
        modifiers: str = "",
        manufacturer: str = "",
        color_name: str = "",
        primary_color: str = "",
        weight: int = 0,
        print_temp: int = 0,
        bed_temp: int = 0,
    ) -> bytes:
        """Helper to create OpenTag3D binary payload."""
        payload = bytearray(102)

        # Version
        struct.pack_into(">H", payload, 0x00, 0x0014)

        # Material
        mat_bytes = material.encode("utf-8")[:5]
        payload[0x02:0x02 + len(mat_bytes)] = mat_bytes

        # Modifiers
        if modifiers:
            mod_bytes = modifiers.encode("utf-8")[:5]
            payload[0x07:0x07 + len(mod_bytes)] = mod_bytes

        # Manufacturer
        if manufacturer:
            mfg_bytes = manufacturer.encode("utf-8")[:16]
            payload[0x1B:0x1B + len(mfg_bytes)] = mfg_bytes

        # Color name
        if color_name:
            color_bytes = color_name.encode("utf-8")[:32]
            payload[0x2B:0x2B + len(color_bytes)] = color_bytes

        # Primary color
        if primary_color:
            payload[0x4B:0x4F] = bytes.fromhex(primary_color)

        # Diameter (1.75mm = 1750um)
        struct.pack_into(">H", payload, 0x5C, 1750)

        # Weight
        if weight:
            struct.pack_into(">H", payload, 0x5E, weight)

        # Print temp (stored as Celsius / 5)
        if print_temp:
            payload[0x60] = print_temp // 5

        # Bed temp
        if bed_temp:
            payload[0x61] = bed_temp // 5

        return bytes(payload)

    def test_decode_minimal(self):
        """Should decode minimal OpenTag3D payload."""
        payload = self._create_payload(material="PLA")
        result = OpenTag3DDecoder.decode("04AABBCCDD1122", payload)

        assert result is not None
        assert result.material_name == "PLA"

    def test_decode_full(self):
        """Should decode complete OpenTag3D payload."""
        payload = self._create_payload(
            material="PETG",
            modifiers="CF",
            manufacturer="Polymaker",
            color_name="Black",
            primary_color="1A1A1AFF",
            weight=1000,
            print_temp=250,
            bed_temp=80
        )
        result = OpenTag3DDecoder.decode("04AABBCCDD1122", payload)

        assert result is not None
        assert result.material_name == "PETG"
        assert result.modifiers == "CF"
        assert result.manufacturer == "Polymaker"
        assert result.color_name == "Black"
        assert result.primary_color == "1A1A1AFF"
        assert result.weight_g == 1000
        assert result.print_temp_c == 250
        assert result.bed_temp_c == 80

    def test_decode_too_short(self):
        """Should reject payload that's too short."""
        payload = bytes(50)  # Need at least 102 bytes
        result = OpenTag3DDecoder.decode("04AABBCCDD1122", payload)
        assert result is None

    def test_to_spool_conversion(self):
        """Should convert OpenTag3D data to normalized spool."""
        payload = self._create_payload(
            material="ASA",
            modifiers="GF",
            manufacturer="eSUN",
            color_name="Orange",
            primary_color="FFA500FF",
            weight=750,
            print_temp=260,
            bed_temp=100
        )
        data = OpenTag3DDecoder.decode("04AABBCCDD1122", payload)
        spool = OpenTag3DDecoder.to_spool(data)

        assert spool.material == "ASA"
        assert spool.subtype == "GF"
        assert spool.brand == "eSUN"
        assert spool.color_name == "Orange"
        assert spool.rgba == "FFA500FF"
        assert spool.label_weight == 750
        assert "260C" in spool.note
        assert "100C" in spool.note

    def test_encode_roundtrip(self):
        """Should encode and decode back to same data."""
        original = OpenTag3DTagData(
            tag_id="test",
            version=0x0014,
            material_name="PC",
            modifiers="CF",
            manufacturer="Prusament",
            color_name="Jet Black",
            primary_color="000000FF",
            weight_g=800,
            print_temp_c=270,
            bed_temp_c=110,
            density=1.2
        )

        encoded = OpenTag3DDecoder.encode(original)
        decoded = OpenTag3DDecoder.decode("04AABBCCDD1122", encoded)

        assert decoded.material_name == original.material_name
        assert decoded.modifiers == original.modifiers
        assert decoded.manufacturer == original.manufacturer
        assert decoded.color_name == original.color_name
        assert decoded.primary_color == original.primary_color
        assert decoded.weight_g == original.weight_g
        assert decoded.print_temp_c == original.print_temp_c
        assert decoded.bed_temp_c == original.bed_temp_c


class TestBambuLabDecoder:
    """Tests for Bambu Lab MIFARE decoder."""

    def _create_blocks(
        self,
        material_variant: str = "A00-G1",
        material_id: str = "GFA00",
        filament_type: str = "PLA",
        detailed_type: str = "PLA Basic",
        color_rgba: str = "FF0000FF",
        spool_weight: int = 250
    ) -> dict:
        """Helper to create Bambu Lab MIFARE blocks."""
        blocks = {}

        # Block 1: material variant + material ID
        block1 = bytearray(16)
        variant_bytes = material_variant.encode("ascii")[:8]
        block1[0:len(variant_bytes)] = variant_bytes
        id_bytes = material_id.encode("ascii")[:8]
        block1[8:8 + len(id_bytes)] = id_bytes
        blocks[1] = bytes(block1)

        # Block 2: filament type
        block2 = bytearray(16)
        type_bytes = filament_type.encode("ascii")[:16]
        block2[0:len(type_bytes)] = type_bytes
        blocks[2] = bytes(block2)

        # Block 4: detailed filament type
        block4 = bytearray(16)
        detailed_bytes = detailed_type.encode("ascii")[:16]
        block4[0:len(detailed_bytes)] = detailed_bytes
        blocks[4] = bytes(block4)

        # Block 5: color + spool weight
        block5 = bytearray(16)
        block5[0:4] = bytes.fromhex(color_rgba)
        struct.pack_into("<H", block5, 4, spool_weight)
        blocks[5] = bytes(block5)

        return blocks

    def test_decode_basic(self):
        """Should decode basic Bambu Lab tag."""
        uid_hex = "04AABBCCDD1122"
        blocks = self._create_blocks()
        result = BambuLabDecoder.decode(uid_hex, blocks)

        assert result is not None
        assert result.material_variant_id == "A00-G1"
        assert result.material_id == "GFA00"
        assert result.filament_type == "PLA"
        assert result.detailed_filament_type == "PLA Basic"
        assert result.color_rgba == "FF0000FF"
        assert result.spool_weight == 250

    def test_decode_different_materials(self):
        """Should decode various Bambu Lab materials."""
        uid_hex = "04AABBCCDD1122"

        # Test PETG
        blocks = self._create_blocks(
            material_id="GFG00",
            filament_type="PETG",
            detailed_type="PETG Basic"
        )
        result = BambuLabDecoder.decode(uid_hex, blocks)
        assert result.filament_type == "PETG"

        # Test ABS
        blocks = self._create_blocks(
            material_id="GFB00",
            filament_type="ABS",
            detailed_type="ABS"
        )
        result = BambuLabDecoder.decode(uid_hex, blocks)
        assert result.filament_type == "ABS"

    def test_to_spool_conversion(self):
        """Should convert Bambu Lab data to normalized spool."""
        uid_hex = "04AABBCCDD1122"
        blocks = self._create_blocks(
            material_id="GFA01",
            filament_type="PLA",
            detailed_type="PLA Matte",
            color_rgba="00FF00FF"
        )
        data = BambuLabDecoder.decode(uid_hex, blocks)
        spool = BambuLabDecoder.to_spool(data)

        assert spool.material == "PLA"
        assert spool.rgba == "00FF00FF"
        assert spool.brand == "Bambu"
        assert spool.core_weight == 250


class TestTagDecoder:
    """Tests for unified TagDecoder."""

    def test_decode_ndef_url_spoolease(self):
        """Should decode SpoolEase via NDEF URL."""
        uid_hex = "04AABBCCDD1122"
        url = "https://info.filament3d.org/V2/?M=PLA&B=Test"

        result = TagDecoder.decode_ndef_url(uid_hex, url)

        assert result.tag_type == TagType.SPOOLEASE_V2
        assert result.spoolease_data is not None
        assert result.spoolease_data.material == "PLA"

    def test_decode_ndef_records_openspool(self):
        """Should decode OpenSpool via NDEF records."""
        uid_hex = "04AABBCCDD1122"
        payload = json.dumps({
            "protocol": "openspool",
            "type": "PETG"
        }).encode("utf-8")
        records = [{"type": "application/json", "payload": payload}]

        result = TagDecoder.decode_ndef_records(uid_hex, records)

        assert result.tag_type == TagType.OPENSPOOL
        assert result.openspool_data is not None
        assert result.openspool_data.material_type == "PETG"

    def test_decode_ndef_records_opentag3d(self):
        """Should decode OpenTag3D via NDEF records."""
        uid_hex = "04AABBCCDD1122"
        payload = bytearray(102)
        struct.pack_into(">H", payload, 0x00, 0x0014)
        payload[0x02:0x05] = b"ABS"
        records = [{"type": "application/opentag3d", "payload": bytes(payload)}]

        result = TagDecoder.decode_ndef_records(uid_hex, records)

        assert result.tag_type == TagType.OPENTAG3D
        assert result.opentag3d_data is not None
        assert result.opentag3d_data["material_name"] == "ABS"

    def test_decode_mifare_bambulab(self):
        """Should decode Bambu Lab via MIFARE blocks."""
        uid_hex = "04AABBCCDD1122"

        # Create minimal blocks
        blocks = {
            1: b"A00-G1\x00\x00GFA00\x00\x00\x00",
            2: b"PLA\x00" + bytes(12),
            4: b"PLA Basic\x00" + bytes(6),
            5: bytes.fromhex("FF0000FF") + struct.pack("<H", 250) + bytes(10),
        }

        result = TagDecoder.decode_mifare_blocks(uid_hex, blocks)

        assert result.tag_type == TagType.BAMBULAB
        assert result.bambulab_data is not None

    def test_to_spool_all_types(self):
        """Should convert all tag types to normalized spool."""
        uid_hex = "04AABBCCDD1122"

        # SpoolEase
        url = "https://info.filament3d.org/V2/?M=PLA"
        result = TagDecoder.decode_ndef_url(uid_hex, url)
        spool = TagDecoder.to_spool(result)
        assert spool is not None
        assert spool.material == "PLA"

        # OpenSpool
        payload = json.dumps({"protocol": "openspool", "type": "PETG"}).encode()
        result = TagDecoder.decode_ndef_records(uid_hex, [{"type": "application/json", "payload": payload}])
        spool = TagDecoder.to_spool(result)
        assert spool is not None
        assert spool.material == "PETG"

        # OpenTag3D
        payload = bytearray(102)
        struct.pack_into(">H", payload, 0x00, 0x0014)
        payload[0x02:0x05] = b"TPU"
        result = TagDecoder.decode_ndef_records(uid_hex, [{"type": "application/opentag3d", "payload": bytes(payload)}])
        spool = TagDecoder.to_spool(result)
        assert spool is not None
        assert spool.material == "TPU"

    def test_unknown_tag_type(self):
        """Should handle unknown NDEF records gracefully."""
        uid_hex = "04AABBCCDD1122"
        records = [{"type": "application/unknown", "payload": b"test"}]

        result = TagDecoder.decode_ndef_records(uid_hex, records)

        assert result.tag_type == TagType.UNKNOWN
        assert TagDecoder.to_spool(result) is None
