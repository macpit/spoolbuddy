from pydantic import BaseModel
from typing import Optional
from datetime import datetime


# ============ Spool Models ============

class SpoolBase(BaseModel):
    tag_id: Optional[str] = None
    material: str
    subtype: Optional[str] = None
    color_name: Optional[str] = None
    rgba: Optional[str] = None
    brand: Optional[str] = None
    label_weight: Optional[int] = 1000
    core_weight: Optional[int] = 250
    weight_new: Optional[int] = None
    weight_current: Optional[int] = None
    slicer_filament: Optional[str] = None
    slicer_filament_name: Optional[str] = None
    location: Optional[str] = None
    note: Optional[str] = None
    data_origin: Optional[str] = None
    tag_type: Optional[str] = None
    ext_has_k: Optional[bool] = False


class SpoolCreate(SpoolBase):
    pass


class SpoolUpdate(SpoolBase):
    material: Optional[str] = None


class Spool(SpoolBase):
    id: str
    spool_number: Optional[int] = None
    added_time: Optional[int] = None
    encode_time: Optional[int] = None
    added_full: Optional[int] = 0
    consumed_since_add: Optional[float] = 0
    consumed_since_weight: Optional[float] = 0
    weight_used: Optional[float] = 0
    archived_at: Optional[int] = None  # Timestamp when archived, null = active
    created_at: Optional[int] = None
    updated_at: Optional[int] = None
    last_used_time: Optional[int] = None  # From usage_history table

    class Config:
        from_attributes = True


# ============ Printer Models ============

class PrinterBase(BaseModel):
    serial: str
    name: Optional[str] = None
    model: Optional[str] = None
    ip_address: Optional[str] = None
    access_code: Optional[str] = None
    auto_connect: bool = False


class PrinterCreate(PrinterBase):
    pass


class PrinterUpdate(BaseModel):
    name: Optional[str] = None
    model: Optional[str] = None
    ip_address: Optional[str] = None
    access_code: Optional[str] = None
    auto_connect: Optional[bool] = None


class Printer(PrinterBase):
    last_seen: Optional[int] = None
    config: Optional[str] = None
    nozzle_count: int = 1  # 1 or 2, auto-detected from MQTT

    class Config:
        from_attributes = True


# ============ AMS Models ============
# NOTE: AMS models defined before PrinterWithStatus to avoid forward references

class AmsTray(BaseModel):
    """Single AMS tray/slot."""
    ams_id: int
    tray_id: int
    tray_type: Optional[str] = None
    tray_color: Optional[str] = None
    tray_info_idx: Optional[str] = None
    k_value: Optional[float] = None
    nozzle_temp_min: Optional[int] = None
    nozzle_temp_max: Optional[int] = None
    remain: Optional[int] = None  # Remaining filament percentage (0-100)


class AmsUnit(BaseModel):
    """AMS unit with humidity and trays."""
    id: int
    humidity: Optional[int] = None  # Percentage (0-100) from humidity_raw, or index (1-5) fallback
    temperature: Optional[float] = None  # Temperature in Celsius
    extruder: Optional[int] = None  # 0 = right nozzle, 1 = left nozzle
    trays: list[AmsTray] = []


class PrinterWithStatus(BaseModel):
    """Printer with connection status and live state."""
    serial: str
    name: Optional[str] = None
    model: Optional[str] = None
    ip_address: Optional[str] = None
    access_code: Optional[str] = None
    last_seen: Optional[int] = None
    config: Optional[str] = None
    auto_connect: bool = False
    nozzle_count: int = 1  # 1 or 2, auto-detected from MQTT
    connected: bool = False
    # Live state from MQTT
    gcode_state: Optional[str] = None
    print_progress: Optional[int] = None
    subtask_name: Optional[str] = None  # Current print job name
    mc_remaining_time: Optional[int] = None  # Remaining time in minutes
    cover_url: Optional[str] = None  # URL to cover image if printing
    # Detailed status tracking
    stg_cur: int = -1  # Current stage number (-1 = idle/unknown)
    stg_cur_name: Optional[str] = None  # Human-readable stage name (e.g., "Auto bed leveling")
    # AMS state
    ams_units: list[AmsUnit] = []
    tray_now: Optional[int] = None  # Active tray (single nozzle)
    tray_now_left: Optional[int] = None  # Active tray left nozzle (dual)
    tray_now_right: Optional[int] = None  # Active tray right nozzle (dual)
    active_extruder: Optional[int] = None  # Currently active extruder (0=right, 1=left)
    # Tray reading state (RFID scanning)
    tray_reading_bits: Optional[int] = None  # Bitmask of trays currently being read


class PrinterState(BaseModel):
    """Real-time printer state from MQTT."""
    gcode_state: Optional[str] = None
    print_progress: Optional[int] = None
    layer_num: Optional[int] = None
    total_layer_num: Optional[int] = None
    subtask_name: Optional[str] = None
    mc_remaining_time: Optional[int] = None  # Remaining time in minutes
    gcode_file: Optional[str] = None  # Current gcode file path
    ams_units: list[AmsUnit] = []
    vt_tray: Optional[AmsTray] = None
    tray_now: Optional[int] = None  # Currently active tray (0-15 for AMS, 254/255 for external) - legacy single-nozzle
    # Dual-nozzle support (H2C/H2D)
    tray_now_left: Optional[int] = None  # Active tray for left nozzle (extruder 1)
    tray_now_right: Optional[int] = None  # Active tray for right nozzle (extruder 0)
    active_extruder: Optional[int] = None  # Currently active extruder (0=right, 1=left)
    # Detailed status tracking (from stg_cur field)
    stg_cur: int = -1  # Current stage number (-1 = idle/unknown, 255 = idle on A1/P1)
    stg_cur_name: Optional[str] = None  # Human-readable stage name
    # Tray reading state (for tracking RFID scanning)
    tray_reading_bits: Optional[int] = None  # Bitmask of trays currently being read
    # Nozzle count (auto-detected from MQTT device.extruder.info)
    nozzle_count: int = 1  # 1 = single nozzle, 2 = dual nozzle (H2C/H2D)


# ============ AMS Filament Setting ============

class AmsFilamentSettingRequest(BaseModel):
    """Request to set filament in an AMS slot."""
    tray_info_idx: str = ""  # Filament preset ID (e.g., "GFL99")
    tray_type: str = ""  # Material type (e.g., "PLA")
    tray_color: str = "FFFFFFFF"  # RGBA hex (e.g., "FF0000FF")
    nozzle_temp_min: int = 190
    nozzle_temp_max: int = 230


class AssignSpoolRequest(BaseModel):
    """Request to assign a spool to an AMS slot."""
    spool_id: str
    # Note: ams_id and tray_id come from path parameters, not body


class SetCalibrationRequest(BaseModel):
    """Request to set calibration profile for an AMS slot."""
    cali_idx: int = -1  # -1 for default (0.02), or calibration profile index
    filament_id: str = ""  # Filament preset ID (optional)
    nozzle_diameter: str = "0.4"  # Nozzle diameter


# ============ WebSocket Messages ============

class WSMessage(BaseModel):
    """WebSocket message wrapper."""
    type: str
    data: dict = {}


# ============ Bambu Cloud Models ============

class CloudLoginRequest(BaseModel):
    """Request to login to Bambu Cloud."""
    email: str
    password: str


class CloudVerifyRequest(BaseModel):
    """Request to verify login with code."""
    email: str
    code: str


class CloudTokenRequest(BaseModel):
    """Request to set token directly."""
    access_token: str


class CloudLoginResponse(BaseModel):
    """Response from login attempt."""
    success: bool
    needs_verification: bool = False
    message: str


class CloudAuthStatus(BaseModel):
    """Cloud authentication status."""
    is_authenticated: bool
    email: Optional[str] = None


class SlicerPreset(BaseModel):
    """A slicer preset (filament, printer, or process)."""
    setting_id: str
    name: str
    type: str  # filament, printer, process
    version: Optional[str] = None
    user_id: Optional[str] = None
    is_custom: bool = False  # True if user's private preset


class SlicerSettingsResponse(BaseModel):
    """Response containing all slicer presets."""
    filament: list[SlicerPreset] = []
    printer: list[SlicerPreset] = []
    process: list[SlicerPreset] = []
