"""Unified tag decoder that handles all supported tag types."""

import base64
import logging
from typing import Optional, Dict, List

from .models import (
    TagType,
    NfcTagType,
    TagReadResult,
    SpoolFromTag,
)
from .spoolease_format import SpoolEaseDecoder
from .bambulab import BambuLabDecoder
from .openprinttag import OpenPrintTagDecoder
from .openspool import OpenSpoolDecoder
from .opentag3d import OpenTag3DDecoder

logger = logging.getLogger(__name__)


class TagDecoder:
    """Unified decoder for all supported NFC tag types."""

    @staticmethod
    def decode_ndef_url(uid_hex: str, url: str) -> Optional[TagReadResult]:
        """Decode an NDEF URL record (typically from NTAG).

        Args:
            uid_hex: Hex-encoded tag UID
            url: URL string from NDEF record

        Returns:
            TagReadResult with parsed data
        """
        # Convert UID to base64
        uid_bytes = bytes.fromhex(uid_hex)
        uid_base64 = base64.urlsafe_b64encode(uid_bytes).decode("ascii").rstrip("=")

        result = TagReadResult(
            uid=uid_hex.upper(),
            uid_base64=uid_base64,
            nfc_type=NfcTagType.NTAG,
            tag_type=TagType.UNKNOWN,
        )

        # Try SpoolEase decoder
        if SpoolEaseDecoder.can_decode(url):
            spoolease_data = SpoolEaseDecoder.decode(url, uid_hex)
            if spoolease_data:
                result.tag_type = (
                    TagType.SPOOLEASE_V2 if spoolease_data.version == 2
                    else TagType.SPOOLEASE_V1
                )
                result.spoolease_data = spoolease_data
                return result

        # Unknown NDEF URL
        return result

    @staticmethod
    def decode_ndef_records(uid_hex: str, ndef_records: List[dict]) -> Optional[TagReadResult]:
        """Decode NDEF records (may contain URL or OpenPrintTag).

        Args:
            uid_hex: Hex-encoded tag UID
            ndef_records: List of NDEF record dicts with 'type' and 'payload'

        Returns:
            TagReadResult with parsed data
        """
        uid_bytes = bytes.fromhex(uid_hex)
        uid_base64 = base64.urlsafe_b64encode(uid_bytes).decode("ascii").rstrip("=")

        result = TagReadResult(
            uid=uid_hex.upper(),
            uid_base64=uid_base64,
            nfc_type=NfcTagType.NTAG,
            tag_type=TagType.UNKNOWN,
        )

        for record in ndef_records:
            record_type = record.get("type", b"")
            payload = record.get("payload", b"")

            if isinstance(record_type, bytes):
                record_type = record_type.decode("utf-8", errors="ignore")

            # Check for OpenPrintTag
            if record_type == OpenPrintTagDecoder.RECORD_TYPE:
                openprinttag_data = OpenPrintTagDecoder.decode(uid_hex, payload)
                if openprinttag_data:
                    result.tag_type = TagType.OPENPRINTTAG
                    result.openprinttag_data = openprinttag_data
                    return result

            # Check for OpenSpool (application/json with protocol: openspool)
            if record_type == OpenSpoolDecoder.RECORD_TYPE:
                if OpenSpoolDecoder.can_decode_payload(payload):
                    openspool_data = OpenSpoolDecoder.decode(uid_hex, payload)
                    if openspool_data:
                        result.tag_type = TagType.OPENSPOOL
                        result.openspool_data = openspool_data
                        return result

            # Check for OpenTag3D (application/opentag3d binary format)
            if record_type == OpenTag3DDecoder.RECORD_TYPE:
                opentag3d_data = OpenTag3DDecoder.decode(uid_hex, payload)
                if opentag3d_data:
                    result.tag_type = TagType.OPENTAG3D
                    result.opentag3d_data = opentag3d_data.__dict__
                    return result

            # Check for URL record (SpoolEase)
            if record_type == "U" or record_type.startswith("urn:nfc:wkt:U"):
                # URL record - payload starts with prefix byte
                if payload:
                    url = TagDecoder._decode_ndef_url_payload(payload)
                    if url and SpoolEaseDecoder.can_decode(url):
                        spoolease_data = SpoolEaseDecoder.decode(url, uid_hex)
                        if spoolease_data:
                            result.tag_type = (
                                TagType.SPOOLEASE_V2 if spoolease_data.version == 2
                                else TagType.SPOOLEASE_V1
                            )
                            result.spoolease_data = spoolease_data
                            return result

        return result

    @staticmethod
    def decode_mifare_blocks(uid_hex: str, blocks: Dict[int, bytes]) -> Optional[TagReadResult]:
        """Decode Mifare Classic blocks (Bambu Lab tags).

        Args:
            uid_hex: Hex-encoded tag UID
            blocks: Dict mapping block number to 16-byte block data

        Returns:
            TagReadResult with parsed data
        """
        uid_bytes = bytes.fromhex(uid_hex)
        uid_base64 = base64.urlsafe_b64encode(uid_bytes).decode("ascii").rstrip("=")

        result = TagReadResult(
            uid=uid_hex.upper(),
            uid_base64=uid_base64,
            nfc_type=NfcTagType.MIFARE_CLASSIC_1K,
            tag_type=TagType.UNKNOWN,
            mifare_blocks=blocks,
        )

        # Try Bambu Lab decoder
        bambulab_data = BambuLabDecoder.decode(uid_hex, blocks)
        if bambulab_data and bambulab_data.material_id:
            result.tag_type = TagType.BAMBULAB
            result.bambulab_data = bambulab_data
            return result

        return result

    @staticmethod
    def to_spool(result: TagReadResult) -> Optional[SpoolFromTag]:
        """Convert TagReadResult to normalized SpoolFromTag.

        Args:
            result: Decoded tag result

        Returns:
            Normalized spool data, or None if tag type unknown
        """
        if result.tag_type == TagType.SPOOLEASE_V1 or result.tag_type == TagType.SPOOLEASE_V2:
            if result.spoolease_data:
                return SpoolEaseDecoder.to_spool(result.spoolease_data)

        elif result.tag_type == TagType.BAMBULAB:
            if result.bambulab_data:
                return BambuLabDecoder.to_spool(result.bambulab_data)

        elif result.tag_type == TagType.OPENPRINTTAG:
            if result.openprinttag_data:
                return OpenPrintTagDecoder.to_spool(result.openprinttag_data)

        elif result.tag_type == TagType.OPENSPOOL:
            if result.openspool_data:
                return OpenSpoolDecoder.to_spool(result.openspool_data)

        elif result.tag_type == TagType.OPENTAG3D:
            if result.opentag3d_data:
                from .opentag3d import OpenTag3DTagData
                # Reconstruct the data object from dict
                tag_data = OpenTag3DTagData(**result.opentag3d_data)
                return OpenTag3DDecoder.to_spool(tag_data)

        return None

    @staticmethod
    def _decode_ndef_url_payload(payload: bytes) -> Optional[str]:
        """Decode NDEF URL payload to string.

        NDEF URL records have a prefix byte indicating the URL scheme:
        0x00: No prefix
        0x01: http://www.
        0x02: https://www.
        0x03: http://
        0x04: https://
        ... etc
        """
        URL_PREFIXES = {
            0x00: "",
            0x01: "http://www.",
            0x02: "https://www.",
            0x03: "http://",
            0x04: "https://",
            0x05: "tel:",
            0x06: "mailto:",
            0x07: "ftp://anonymous:anonymous@",
            0x08: "ftp://ftp.",
            0x09: "ftps://",
            0x0A: "sftp://",
            0x0B: "smb://",
            0x0C: "nfs://",
            0x0D: "ftp://",
            0x0E: "dav://",
            0x0F: "news:",
            0x10: "telnet://",
            0x11: "imap:",
            0x12: "rtsp://",
            0x13: "urn:",
            0x14: "pop:",
            0x15: "sip:",
            0x16: "sips:",
            0x17: "tftp:",
            0x18: "btspp://",
            0x19: "btl2cap://",
            0x1A: "btgoep://",
            0x1B: "tcpobex://",
            0x1C: "irdaobex://",
            0x1D: "file://",
            0x1E: "urn:epc:id:",
            0x1F: "urn:epc:tag:",
            0x20: "urn:epc:pat:",
            0x21: "urn:epc:raw:",
            0x22: "urn:epc:",
            0x23: "urn:nfc:",
        }

        if not payload:
            return None

        prefix_byte = payload[0]
        url_part = payload[1:].decode("utf-8", errors="ignore")

        prefix = URL_PREFIXES.get(prefix_byte, "")
        return prefix + url_part
