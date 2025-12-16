"""Tag encoding/decoding module for NFC spool tags.

Supports:
- SpoolEase V2 tags (NTAG with NDEF URL)
- Bambu Lab tags (Mifare Classic 1K)
- OpenPrintTag tags (NTAG with NDEF CBOR)
- OpenSpool tags (NTAG with NDEF JSON)
"""

from .models import (
    TagType,
    TagReadResult,
    SpoolEaseTagData,
    BambuLabTagData,
    OpenPrintTagData,
    OpenSpoolTagData,
)
from .spoolease_format import SpoolEaseDecoder, SpoolEaseEncoder
from .bambulab import BambuLabDecoder
from .openprinttag import OpenPrintTagDecoder
from .openspool import OpenSpoolDecoder
from .decoder import TagDecoder

__all__ = [
    "TagType",
    "TagReadResult",
    "SpoolEaseTagData",
    "BambuLabTagData",
    "OpenPrintTagData",
    "OpenSpoolTagData",
    "SpoolEaseDecoder",
    "SpoolEaseEncoder",
    "BambuLabDecoder",
    "OpenPrintTagDecoder",
    "OpenSpoolDecoder",
    "TagDecoder",
]
