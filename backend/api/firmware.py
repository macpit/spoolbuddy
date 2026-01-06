"""
ESP32 Firmware OTA Update API Routes

Handles firmware version checking and OTA binary serving for the SpoolBuddy device.
"""

import hashlib
import logging
import re
import struct
from pathlib import Path
from typing import Optional
from datetime import datetime, timedelta

import httpx
from fastapi import APIRouter, HTTPException, UploadFile, File, Form
from fastapi.responses import FileResponse, StreamingResponse
from pydantic import BaseModel

from config import GITHUB_REPO, settings

logger = logging.getLogger(__name__)
router = APIRouter(prefix="/firmware", tags=["firmware"])

# Firmware releases directory
FIRMWARE_DIR = settings.project_root / "firmware" / "releases"

# Cache for GitHub firmware checks
_firmware_cache: Optional[dict] = None
_firmware_cache_time: Optional[datetime] = None
CACHE_DURATION = timedelta(minutes=5)


class FirmwareVersion(BaseModel):
    version: str
    filename: str
    size: Optional[int] = None
    checksum: Optional[str] = None
    url: Optional[str] = None


class FirmwareCheck(BaseModel):
    current_version: Optional[str] = None
    latest_version: Optional[str] = None
    update_available: bool = False
    download_url: Optional[str] = None
    release_notes: Optional[str] = None
    error: Optional[str] = None


def _get_local_firmware() -> list[FirmwareVersion]:
    """Get list of locally available firmware files."""
    if not FIRMWARE_DIR.exists():
        return []

    firmware_files = []
    for f in FIRMWARE_DIR.glob("*.bin"):
        # Extract version from filename (e.g., spoolbuddy-1.0.0.bin -> 1.0.0)
        name = f.stem
        version = name.replace("spoolbuddy-", "").replace("firmware-", "")

        firmware_files.append(FirmwareVersion(
            version=version,
            filename=f.name,
            size=f.stat().st_size,
        ))

    # Sort by version descending (using parsed version for proper ordering)
    firmware_files.sort(key=lambda x: _parse_version(x.version) or (0, 0, 0, 0, 0), reverse=True)
    return firmware_files


def _parse_version(v: str):
    """Parse version string supporting semver and PEP 440 pre-releases.

    Supports: 0.1.0, 0.1.0a1, 0.1.0b2, 0.1.0rc1, 0.1.0-beta.2
    """
    import re
    # Normalize: 0.1.0-beta.2 -> 0.1.0b2
    v = re.sub(r'-?alpha\.?', 'a', v)
    v = re.sub(r'-?beta\.?', 'b', v)
    v = re.sub(r'-?rc\.?', 'rc', v)

    # Match: major.minor.patch[prerelease][prerelease_num]
    match = re.match(r'^(\d+)\.(\d+)\.(\d+)(a|b|rc)?(\d+)?$', v)
    if not match:
        return None

    major, minor, patch = int(match.group(1)), int(match.group(2)), int(match.group(3))
    pre_type = match.group(4)  # a, b, rc, or None
    pre_num = int(match.group(5)) if match.group(5) else 0

    # Pre-release ordering: a < b < rc < release
    pre_order = {'a': 0, 'b': 1, 'rc': 2, None: 3}

    return (major, minor, patch, pre_order.get(pre_type, 3), pre_num)


def _compare_versions(current: str, latest: str) -> bool:
    """Compare version strings. Returns True if latest > current.

    Supports: 0.1.0, 0.1.0a1, 0.1.0b2, 0.1.0rc1
    """
    try:
        current_parsed = _parse_version(current)
        latest_parsed = _parse_version(latest)

        if current_parsed and latest_parsed:
            return latest_parsed > current_parsed

        # Fallback to string comparison
        return latest > current
    except Exception:
        return latest > current


@router.get("/version", response_model=list[FirmwareVersion])
async def list_firmware_versions():
    """List available firmware versions (local files)."""
    return _get_local_firmware()


@router.get("/latest", response_model=FirmwareVersion)
async def get_latest_firmware():
    """Get the latest available firmware version."""
    firmware_list = _get_local_firmware()
    if not firmware_list:
        raise HTTPException(status_code=404, detail="No firmware available")
    return firmware_list[0]


@router.get("/check", response_model=FirmwareCheck)
async def check_firmware_update(current_version: Optional[str] = None):
    """
    Check for firmware updates.

    Checks both local releases directory and GitHub releases, returns the newest.

    Args:
        current_version: The device's current firmware version (if not provided, uses last known device version)
    """
    global _firmware_cache, _firmware_cache_time

    # Use device's reported version if not explicitly provided
    if not current_version:
        from main import get_display_firmware_version
        current_version = get_display_firmware_version()

    result = FirmwareCheck(current_version=current_version)

    # Track best available firmware from all sources
    best_version: Optional[str] = None
    best_url: Optional[str] = None
    best_notes: Optional[str] = None

    # Check local firmware
    local_firmware = _get_local_firmware()
    if local_firmware:
        latest_local = local_firmware[0]
        best_version = latest_local.version
        best_url = f"/api/firmware/download/{latest_local.filename}"
        logger.debug(f"Local firmware available: {latest_local.version}")

    # Check GitHub releases (always check, compare with local)
    if not _firmware_cache or not _firmware_cache_time or \
            datetime.now() - _firmware_cache_time > CACHE_DURATION:
        try:
            async with httpx.AsyncClient(timeout=10.0) as client:
                response = await client.get(
                    f"https://api.github.com/repos/{GITHUB_REPO}/releases",
                    headers={"Accept": "application/vnd.github.v3+json"},
                )

                if response.status_code == 200:
                    releases = response.json()

                    # Find release with firmware asset
                    for release in releases:
                        for asset in release.get("assets", []):
                            if asset["name"].endswith(".bin"):
                                _firmware_cache = {
                                    "version": release["tag_name"].lstrip("v"),
                                    "filename": asset["name"],
                                    "url": asset["browser_download_url"],
                                    "notes": release.get("body"),
                                }
                                _firmware_cache_time = datetime.now()
                                logger.debug(f"GitHub firmware available: {_firmware_cache['version']}")
                                break
                        if _firmware_cache:
                            break

        except Exception as e:
            logger.warning(f"Could not check GitHub for firmware: {e}")
            # Don't set error if we have local firmware

    # Compare local vs GitHub and use the newest
    if _firmware_cache:
        github_version = _firmware_cache["version"]

        if best_version is None:
            # No local firmware, use GitHub
            best_version = github_version
            best_url = _firmware_cache["url"]
            best_notes = _firmware_cache.get("notes")
        elif _compare_versions(best_version, github_version):
            # GitHub is newer than local
            best_version = github_version
            best_url = _firmware_cache["url"]
            best_notes = _firmware_cache.get("notes")
            logger.info(f"GitHub firmware ({github_version}) is newer than local ({local_firmware[0].version if local_firmware else 'none'})")

    # Set result
    if best_version:
        result.latest_version = best_version
        result.download_url = best_url
        result.release_notes = best_notes

        if current_version:
            result.update_available = _compare_versions(current_version, best_version)
        else:
            # Device version unknown - offer update if firmware is available
            # This handles devices running old firmware without version reporting
            result.update_available = True
            logger.info(f"Device version unknown, offering update to {best_version}")
    else:
        result.error = "No firmware available (local or GitHub)"

    return result


@router.get("/download/{filename}")
async def download_firmware(filename: str):
    """
    Download a firmware binary file.

    For ESP32 OTA updates, the device will request this endpoint.
    """
    # Security: only allow .bin files and prevent directory traversal
    if not filename.endswith(".bin") or "/" in filename or "\\" in filename:
        raise HTTPException(status_code=400, detail="Invalid filename")

    filepath = FIRMWARE_DIR / filename
    if not filepath.exists():
        raise HTTPException(status_code=404, detail="Firmware not found")

    return FileResponse(
        filepath,
        media_type="application/octet-stream",
        filename=filename,
        headers={
            "Content-Length": str(filepath.stat().st_size),
        }
    )


@router.get("/ota")
async def get_ota_firmware(version: Optional[str] = None):
    """
    ESP32 OTA endpoint.

    This endpoint is designed for ESP32 HTTP OTA updates.
    It returns the latest firmware binary with appropriate headers.

    Args:
        version: Optional specific version to download
    """
    firmware_list = _get_local_firmware()
    if not firmware_list:
        raise HTTPException(status_code=404, detail="No firmware available")

    # Find requested version or use latest
    firmware = None
    if version:
        for fw in firmware_list:
            if fw.version == version:
                firmware = fw
                break
        if not firmware:
            raise HTTPException(status_code=404, detail=f"Version {version} not found")
    else:
        firmware = firmware_list[0]

    filepath = FIRMWARE_DIR / firmware.filename
    if not filepath.exists():
        raise HTTPException(status_code=404, detail="Firmware file not found")

    # Return binary with ESP32 OTA-compatible headers
    return FileResponse(
        filepath,
        media_type="application/octet-stream",
        filename=firmware.filename,
        headers={
            "Content-Length": str(filepath.stat().st_size),
            "X-Firmware-Version": firmware.version,
        }
    )


# ESP32 firmware magic bytes and structure
ESP32_IMAGE_MAGIC = 0xE9
ESP32_APP_DESC_MAGIC = 0xABCD5432
ESP32_APP_DESC_OFFSET = 0x20  # App descriptor offset in first segment


class FirmwareValidationError(Exception):
    """Raised when firmware validation fails."""
    pass


def _validate_esp32_firmware(data: bytes) -> dict:
    """
    Validate ESP32 firmware binary and extract metadata.

    Args:
        data: Raw firmware binary data

    Returns:
        Dict with version, project_name, idf_version, etc.

    Raises:
        FirmwareValidationError: If validation fails
    """
    if len(data) < 256:
        raise FirmwareValidationError("File too small to be valid firmware")

    # Check ESP32 image magic byte
    if data[0] != ESP32_IMAGE_MAGIC:
        raise FirmwareValidationError(
            f"Invalid ESP32 magic byte: expected 0x{ESP32_IMAGE_MAGIC:02X}, "
            f"got 0x{data[0]:02X}"
        )

    # ESP32 image header structure (simplified):
    # 0x00: magic (1 byte) = 0xE9
    # 0x01: segment count (1 byte)
    # 0x02: SPI mode (1 byte)
    # 0x03: SPI speed/size (1 byte)
    # 0x04-0x07: entry point (4 bytes)
    # 0x08-0x17: segment info
    # 0x18: hash appended (1 byte)
    # ...

    # The app descriptor is located at a fixed offset in the .rodata section
    # For most ESP-IDF apps, it's at offset 0x20 in the first segment
    # Look for the app descriptor magic

    app_desc_offset = None
    # Search for app descriptor magic in first 64KB
    search_range = min(len(data), 65536)
    for offset in range(0, search_range - 256, 4):
        if len(data) >= offset + 4:
            magic = struct.unpack_from("<I", data, offset)[0]
            if magic == ESP32_APP_DESC_MAGIC:
                app_desc_offset = offset
                break

    if app_desc_offset is None:
        # Firmware is valid but doesn't have standard app descriptor
        # This can happen with custom builds
        logger.warning("No ESP32 app descriptor found, using filename for version")
        return {
            "valid": True,
            "has_descriptor": False,
        }

    # Parse app descriptor (esp_app_desc_t structure):
    # 0x00: magic (4 bytes) = 0xABCD5432
    # 0x04: secure_version (4 bytes)
    # 0x08: reserv1 (8 bytes)
    # 0x10: version (32 bytes, null-terminated string)
    # 0x30: project_name (32 bytes, null-terminated string)
    # 0x50: time (16 bytes, null-terminated string)
    # 0x60: date (16 bytes, null-terminated string)
    # 0x70: idf_ver (32 bytes, null-terminated string)
    # 0x90: app_elf_sha256 (32 bytes)

    try:
        def read_str(offset: int, length: int) -> str:
            raw = data[offset:offset + length]
            null_idx = raw.find(b'\x00')
            if null_idx >= 0:
                raw = raw[:null_idx]
            return raw.decode('utf-8', errors='replace').strip()

        version = read_str(app_desc_offset + 0x10, 32)
        project_name = read_str(app_desc_offset + 0x30, 32)
        compile_time = read_str(app_desc_offset + 0x50, 16)
        compile_date = read_str(app_desc_offset + 0x60, 16)
        idf_version = read_str(app_desc_offset + 0x70, 32)

        # Validate version format (should be semver-like)
        if not version or not re.match(r'^[\d\w\.\-]+$', version):
            raise FirmwareValidationError(f"Invalid version string: {version!r}")

        return {
            "valid": True,
            "has_descriptor": True,
            "version": version,
            "project_name": project_name,
            "compile_time": compile_time,
            "compile_date": compile_date,
            "idf_version": idf_version,
        }

    except struct.error as e:
        raise FirmwareValidationError(f"Failed to parse app descriptor: {e}")


class FirmwareUploadResponse(BaseModel):
    success: bool
    message: str
    version: Optional[str] = None
    filename: Optional[str] = None
    size: Optional[int] = None
    checksum: Optional[str] = None


@router.post("/upload", response_model=FirmwareUploadResponse)
async def upload_firmware(
    file: UploadFile = File(...),
    version: Optional[str] = Form(None),
):
    """
    Upload a new firmware binary.

    Validates the firmware binary and saves it to the releases directory.

    Args:
        file: The firmware binary file (.bin)
        version: Optional version override (extracted from binary if not provided)

    Returns:
        Upload result with version and filename
    """
    # Validate file extension
    if not file.filename or not file.filename.endswith(".bin"):
        raise HTTPException(
            status_code=400,
            detail="Invalid file type. Must be a .bin file"
        )

    # Read file content
    try:
        content = await file.read()
    except Exception as e:
        raise HTTPException(
            status_code=400,
            detail=f"Failed to read file: {e}"
        )

    # Validate firmware
    try:
        metadata = _validate_esp32_firmware(content)
    except FirmwareValidationError as e:
        raise HTTPException(
            status_code=400,
            detail=f"Invalid firmware: {e}"
        )

    # Determine version
    if version:
        firmware_version = version
    elif metadata.get("version"):
        firmware_version = metadata["version"]
    else:
        # Try to extract from filename
        match = re.search(r'(\d+\.\d+\.\d+)', file.filename)
        if match:
            firmware_version = match.group(1)
        else:
            raise HTTPException(
                status_code=400,
                detail="Could not determine firmware version. Please provide version parameter."
            )

    # Clean version string
    firmware_version = firmware_version.lstrip("v")

    # Ensure releases directory exists
    FIRMWARE_DIR.mkdir(parents=True, exist_ok=True)

    # Generate filename and checksum
    checksum = hashlib.sha256(content).hexdigest()[:16]
    filename = f"spoolbuddy-{firmware_version}.bin"
    filepath = FIRMWARE_DIR / filename

    # Check for existing file with same version
    if filepath.exists():
        existing_checksum = hashlib.sha256(filepath.read_bytes()).hexdigest()[:16]
        if existing_checksum == checksum:
            return FirmwareUploadResponse(
                success=True,
                message=f"Firmware {firmware_version} already exists (identical)",
                version=firmware_version,
                filename=filename,
                size=len(content),
                checksum=checksum,
            )
        else:
            # Different file with same version - rename old one
            backup_name = f"spoolbuddy-{firmware_version}.{existing_checksum}.bin.bak"
            filepath.rename(FIRMWARE_DIR / backup_name)
            logger.info(f"Backed up existing firmware to {backup_name}")

    # Save firmware
    try:
        filepath.write_bytes(content)
    except Exception as e:
        raise HTTPException(
            status_code=500,
            detail=f"Failed to save firmware: {e}"
        )

    logger.info(f"Uploaded firmware {firmware_version}: {filename} ({len(content)} bytes)")

    return FirmwareUploadResponse(
        success=True,
        message=f"Firmware {firmware_version} uploaded successfully",
        version=firmware_version,
        filename=filename,
        size=len(content),
        checksum=checksum,
    )


@router.delete("/{version}")
async def delete_firmware(version: str):
    """
    Delete a firmware version.

    Args:
        version: Version to delete (e.g., "1.0.0")
    """
    version = version.lstrip("v")
    filename = f"spoolbuddy-{version}.bin"
    filepath = FIRMWARE_DIR / filename

    if not filepath.exists():
        raise HTTPException(status_code=404, detail=f"Version {version} not found")

    try:
        filepath.unlink()
        logger.info(f"Deleted firmware {version}")
        return {"success": True, "message": f"Firmware {version} deleted"}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to delete: {e}")
