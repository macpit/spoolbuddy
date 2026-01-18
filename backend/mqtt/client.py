import json
import ssl
import time
import logging
import asyncio
from typing import Optional, Callable, Any
from dataclasses import dataclass, field

import paho.mqtt.client as mqtt

from models import PrinterState, AmsUnit, AmsTray

logger = logging.getLogger(__name__)


# Stage name mapping from BambuStudio DeviceManager.cpp
STAGE_NAMES = {
    0: "Printing",
    1: "Auto bed leveling",
    2: "Heatbed preheating",
    3: "Vibration compensation",
    4: "Changing filament",
    5: "M400 pause",
    6: "Paused (filament ran out)",
    7: "Heating nozzle",
    8: "Calibrating dynamic flow",
    9: "Scanning bed surface",
    10: "Inspecting first layer",
    11: "Identifying build plate type",
    12: "Calibrating Micro Lidar",
    13: "Homing toolhead",
    14: "Cleaning nozzle tip",
    15: "Checking extruder temperature",
    16: "Paused by the user",
    17: "Pause (front cover fall off)",
    18: "Calibrating the micro lidar",
    19: "Calibrating flow ratio",
    20: "Pause (nozzle temperature malfunction)",
    21: "Pause (heatbed temperature malfunction)",
    22: "Filament unloading",
    23: "Pause (step loss)",
    24: "Filament loading",
    25: "Motor noise cancellation",
    26: "Pause (AMS offline)",
    27: "Pause (low speed of the heatbreak fan)",
    28: "Pause (chamber temperature control problem)",
    29: "Cooling chamber",
    30: "Paused (toolhead not returning)",
    31: "Pause (first layer problem)",
    32: "Inspecting Z seam",
    33: "Pause (spaghetti detected)",
    34: "Pause (auto recover failed)",
    35: "Nozzle offset calibration",
    36: "Nozzle cleaning",
    37: "Pause (pressure advance)",
    38: "Nozzle offset preheating",
    39: "Pause (nozzle clog detected)",
    40: "High temperature auto bed leveling",
    41: "Auto Check: Quick Release Lever",
    42: "Auto Check: Door and Upper Cover",
    43: "Laser Calibration",
    44: "Auto Check: Platform",
    45: "Confirming BirdsEye Camera location",
    46: "Calibrating BirdsEye Camera",
    47: "Auto bed leveling - phase 1",
    48: "Auto bed leveling - phase 2",
    49: "Heating chamber",
    50: "Cooling heatbed",
    51: "Printing calibration lines",
    52: "Auto Check: Material",
    53: "Live View Camera Calibration",
    54: "Waiting for heatbed temperature",
    55: "Auto Check: Material Position",
    56: "Cutting Module Offset Calibration",
    57: "Measuring Surface",
    58: "Thermal Preconditioning",
}


def get_stage_name(stg_cur: int) -> str:
    """Get human-readable name for a stage number."""
    if stg_cur < 0 or stg_cur >= 255:
        return None  # -1 (X1 idle) or 255 (A1/P1 idle)
    return STAGE_NAMES.get(stg_cur, f"Unknown stage ({stg_cur})")


@dataclass
class Calibration:
    """Calibration profile for a filament."""
    cali_idx: int
    filament_id: str
    k_value: float
    name: str
    extruder_id: Optional[int] = None
    nozzle_diameter: Optional[str] = None


# Grace period before reporting printer as disconnected (handles brief MQTT interruptions)
DISCONNECT_GRACE_PERIOD_SEC = 5.0


@dataclass
class PendingAssignment:
    """Pending spool assignment waiting for tray insertion."""
    spool_id: str
    tray_info_idx: str
    setting_id: str
    tray_type: str
    tray_color: str
    nozzle_temp_min: int
    nozzle_temp_max: int
    cali_idx: int = -1  # K-profile calibration index
    nozzle_diameter: str = "0.4"  # For extrusion_cali_sel
    created_at: float = field(default_factory=time.time)


@dataclass
class PrinterConnection:
    """Manages MQTT connection to a single Bambu printer."""

    serial: str
    ip_address: str
    access_code: str
    name: Optional[str] = None

    _client: Optional[mqtt.Client] = field(default=None, repr=False)
    _connected: bool = field(default=False, repr=False)
    _disconnect_time: Optional[float] = field(default=None, repr=False)  # Timestamp of disconnect
    _state: PrinterState = field(default_factory=PrinterState, repr=False)
    _on_state_update: Optional[Callable[[str, PrinterState], None]] = field(default=None, repr=False)
    _loop: Optional[asyncio.AbstractEventLoop] = field(default=None, repr=False)
    _calibrations: dict = field(default_factory=dict, repr=False)  # cali_idx -> Calibration
    _kprofiles: list = field(default_factory=list, repr=False)  # List of calibration profiles (updated on broadcast)
    _pending_kprofile_response: Optional[asyncio.Event] = field(default=None, repr=False)
    _expected_kprofile_nozzle: Optional[str] = field(default=None, repr=False)
    _kprofile_lock: Optional[asyncio.Lock] = field(default=None, repr=False)  # Lock to prevent concurrent requests
    _kprofile_cache: dict = field(default_factory=dict, repr=False)  # nozzle_diameter -> (profiles, timestamp)
    _kprofile_cache_ttl: float = field(default=30.0, repr=False)  # Cache TTL in seconds
    _pending_assignments: dict = field(default_factory=dict, repr=False)  # (ams_id, tray_id) -> PendingAssignment
    _on_assignment_complete: Optional[Callable[[str, int, int, str, bool], None]] = field(default=None, repr=False)  # (serial, ams_id, tray_id, spool_id, success)
    _on_tray_reading_change: Optional[Callable[[str, Optional[int], int], None]] = field(default=None, repr=False)  # (serial, old_bits, new_bits)
    _on_nozzle_count_update: Optional[Callable[[str, int], None]] = field(default=None, repr=False)  # (serial, nozzle_count)
    _nozzle_diameters: dict = field(default_factory=dict, repr=False)  # extruder_id -> nozzle_diameter string
    _nozzle_count_detected: bool = field(default=False, repr=False)  # Track if we've already detected nozzle count

    @property
    def connected(self) -> bool:
        """Check if connected, with grace period for brief disconnects."""
        if self._connected:
            return True
        # If recently disconnected, still report as connected during grace period
        if self._disconnect_time is not None:
            elapsed = time.time() - self._disconnect_time
            if elapsed < DISCONNECT_GRACE_PERIOD_SEC:
                return True
        return False

    @property
    def state(self) -> PrinterState:
        return self._state

    def get_nozzle_diameter(self, extruder_id: int = 0) -> str:
        """Get nozzle diameter for an extruder. Returns '0.4' as fallback."""
        return self._nozzle_diameters.get(extruder_id, "0.4")

    def connect(self, on_state_update: Callable[[str, PrinterState], None], on_disconnect: Optional[Callable[[str], None]] = None, on_connect: Optional[Callable[[str], None]] = None):
        """Connect to printer via MQTT."""
        self._on_state_update = on_state_update
        self._on_disconnect_callback = on_disconnect
        self._on_connect_callback = on_connect
        self._loop = asyncio.get_event_loop()

        # Create MQTT client
        self._client = mqtt.Client(
            client_id=f"spoolbuddy_{self.serial}",
            protocol=mqtt.MQTTv311,
            callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
        )

        # Set credentials
        self._client.username_pw_set("bblp", self.access_code)

        # Configure TLS (Bambu printers use self-signed certs)
        ssl_context = ssl.create_default_context()
        ssl_context.check_hostname = False
        ssl_context.verify_mode = ssl.CERT_NONE
        self._client.tls_set_context(ssl_context)

        # Set callbacks
        self._client.on_connect = self._on_connect
        self._client.on_disconnect = self._on_disconnect
        self._client.on_message = self._on_message

        # Connect
        try:
            self._client.connect(self.ip_address, 8883, keepalive=60)
            self._client.loop_start()
            logger.info(f"Connecting to printer {self.serial} at {self.ip_address}")
        except Exception as e:
            logger.error(f"Failed to connect to {self.serial}: {e}")
            raise

    def disconnect(self):
        """Disconnect from printer."""
        if self._client:
            self._client.loop_stop()
            self._client.disconnect()
            self._connected = False
            logger.info(f"Disconnected from printer {self.serial}")

    def reset_slot(self, ams_id: int, tray_id: int) -> bool:
        """Trigger RFID re-read on an AMS slot.

        Sends the ams_get_rfid command to force the printer to re-scan the RFID tag.

        Args:
            ams_id: AMS unit ID (0-3 for regular AMS, 128-135 for AMS-HT, 254/255 for external)
            tray_id: Tray ID within AMS (0-3 for regular AMS, 0 for HT/external)

        Returns:
            True if command was sent successfully
        """
        if not self._client or not self._connected:
            logger.error(f"Cannot reset slot: not connected to {self.serial}")
            return False

        # Calculate slot_id based on AMS type
        if ams_id <= 3:
            slot_id = tray_id
        elif ams_id >= 128 and ams_id <= 135:
            slot_id = 0
        else:
            slot_id = 0

        topic = f"device/{self.serial}/request"

        # Send ams_get_rfid command to trigger RFID re-read
        rfid_command = {
            "print": {
                "command": "ams_get_rfid",
                "ams_id": ams_id,
                "slot_id": slot_id,
                "sequence_id": "1",
            }
        }

        try:
            result = self._client.publish(topic, json.dumps(rfid_command))
            if result.rc != mqtt.MQTT_ERR_SUCCESS:
                logger.error(f"Failed to send ams_get_rfid command: {result.rc}")
                return False

            logger.info(f"Triggered RFID re-read on {self.serial}: AMS {ams_id}, slot {slot_id}")

            return True

        except Exception as e:
            logger.error(f"Error triggering RFID re-read on {self.serial}: {e}")
            return False

    def set_calibration(
        self,
        ams_id: int,
        tray_id: int,
        cali_idx: int = -1,
        filament_id: str = "",
        nozzle_diameter: str = "0.4",
    ) -> bool:
        """Set calibration profile (k-value) for an AMS slot.

        Args:
            ams_id: AMS unit ID
            tray_id: Tray ID within AMS
            cali_idx: Calibration index (-1 for default/no profile)
            filament_id: Filament preset ID
            nozzle_diameter: Nozzle diameter

        Returns:
            True if command was sent successfully
        """
        if not self._client or not self._connected:
            logger.error(f"Cannot set calibration: not connected to {self.serial}")
            return False

        # Calculate slot_id and original_tray_id based on AMS type
        if ams_id <= 3:
            slot_id = tray_id
            original_tray_id = ams_id * 4 + tray_id
        elif ams_id >= 128 and ams_id <= 135:
            slot_id = 0
            original_tray_id = (ams_id - 128) * 4 + tray_id
        else:
            slot_id = 0
            original_tray_id = ams_id

        command = {
            "print": {
                "command": "extrusion_cali_sel",
                "cali_idx": cali_idx,
                "filament_id": filament_id,
                "nozzle_diameter": nozzle_diameter,
                "ams_id": ams_id,
                "tray_id": original_tray_id,
                "slot_id": slot_id,
                "sequence_id": "1",
            }
        }

        topic = f"device/{self.serial}/request"
        payload_json = json.dumps(command)
        logger.info(f"[{self.serial}] MQTT extrusion_cali_sel: {payload_json}")
        try:
            result = self._client.publish(topic, payload_json)
            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                logger.info(
                    f"Set calibration on {self.serial}: AMS {ams_id}, tray {tray_id}, "
                    f"original_tray_id={original_tray_id}, cali_idx={cali_idx}"
                )
                return True
            else:
                logger.error(f"Failed to publish calibration setting: {result.rc}")
                return False
        except Exception as e:
            logger.error(f"Error setting calibration on {self.serial}: {e}")
            return False

    def get_calibrations(self) -> list[dict]:
        """Get list of available calibration profiles.

        Returns:
            List of calibration profiles with cali_idx, name, k_value, filament_id
        """
        return [
            {
                "cali_idx": cal.cali_idx,
                "filament_id": cal.filament_id,
                "k_value": cal.k_value,
                "name": cal.name,
                "nozzle_diameter": cal.nozzle_diameter,
            }
            for cal in self._calibrations.values()
        ]

    async def get_kprofiles(self, nozzle_diameter: str = "0.4", timeout: float = 5.0, max_retries: int = 3) -> list[dict]:
        """Request K-profiles from printer with retry logic.

        Bambu printers sometimes ignore the first request, so we retry.
        Uses a lock to prevent concurrent requests from interfering.
        Results are cached for 30 seconds to avoid hammering the printer.

        Args:
            nozzle_diameter: Filter by nozzle diameter (e.g., "0.4")
            timeout: Timeout in seconds for each attempt
            max_retries: Maximum number of retry attempts

        Returns:
            List of K-profile dicts
        """
        if not self._client or not self._connected:
            logger.warning(f"[{self.serial}] Cannot get K-profiles: not connected")
            return []

        # Check cache first
        cached = self._kprofile_cache.get(nozzle_diameter)
        if cached:
            profiles, timestamp = cached
            if time.time() - timestamp < self._kprofile_cache_ttl:
                logger.debug(f"[{self.serial}] Returning cached K-profiles for nozzle {nozzle_diameter}")
                return profiles

        # Initialize lock lazily (must be done in async context)
        if self._kprofile_lock is None:
            self._kprofile_lock = asyncio.Lock()

        # Use lock to prevent concurrent requests
        async with self._kprofile_lock:
            # Check cache again (another request may have populated it while we waited)
            cached = self._kprofile_cache.get(nozzle_diameter)
            if cached:
                profiles, timestamp = cached
                if time.time() - timestamp < self._kprofile_cache_ttl:
                    logger.debug(f"[{self.serial}] Returning cached K-profiles (post-lock)")
                    return profiles

            for attempt in range(max_retries):
                try:
                    # Create event for waiting
                    self._pending_kprofile_response = asyncio.Event()
                    self._expected_kprofile_nozzle = nozzle_diameter
                    self._kprofiles = []  # Clear previous profiles

                    # Send the request
                    self._fetch_calibrations(nozzle_diameter)

                    # Wait for response with timeout
                    try:
                        await asyncio.wait_for(
                            self._pending_kprofile_response.wait(),
                            timeout=timeout
                        )
                        logger.info(f"[{self.serial}] Got K-profiles response on attempt {attempt + 1}")
                        # Cache the result
                        self._kprofile_cache[nozzle_diameter] = (self._kprofiles, time.time())
                        return self._kprofiles
                    except asyncio.TimeoutError:
                        logger.warning(f"[{self.serial}] K-profile request timeout on attempt {attempt + 1}/{max_retries}")
                        continue

                finally:
                    self._pending_kprofile_response = None
                    self._expected_kprofile_nozzle = None

            # If all retries failed, cache empty result to avoid hammering printer
            logger.warning(f"[{self.serial}] All K-profile retries failed, caching empty result")
            self._kprofile_cache[nozzle_diameter] = ([], time.time())
            return []

    def set_filament(
        self,
        ams_id: int,
        tray_id: int,
        tray_info_idx: str = "",
        setting_id: str = "",
        tray_type: str = "",
        tray_color: str = "FFFFFFFF",
        nozzle_temp_min: int = 190,
        nozzle_temp_max: int = 230,
    ) -> bool:
        """Set filament information for an AMS slot.

        Args:
            ams_id: AMS unit ID (0-3 for regular AMS, 128-135 for AMS-HT, 254/255 for external)
            tray_id: Tray ID within AMS (0-3 for regular AMS, 0 for HT/external)
            tray_info_idx: Filament ID (e.g., "GFL05") - short format
            setting_id: Setting ID (e.g., "GFSL05_07") - full format with version
            tray_type: Material type (e.g., "PLA", "PETG")
            tray_color: RGBA hex color (e.g., "FF0000FF" for red)
            nozzle_temp_min: Minimum nozzle temperature
            nozzle_temp_max: Maximum nozzle temperature

        Returns:
            True if command was sent successfully
        """
        if not self._client or not self._connected:
            logger.error(f"Cannot set filament: not connected to {self.serial}")
            return False

        # Calculate slot_id based on AMS type
        if ams_id <= 3:
            # Regular AMS: slot_id = tray_id
            slot_id = tray_id
        else:
            # AMS-HT or external: slot_id = 0
            slot_id = 0

        command = {
            "print": {
                "command": "ams_filament_setting",
                "ams_id": ams_id,
                "tray_id": tray_id,
                "slot_id": slot_id,
                "tray_info_idx": tray_info_idx,
                "tray_type": tray_type,
                "tray_color": tray_color,
                "nozzle_temp_min": nozzle_temp_min,
                "nozzle_temp_max": nozzle_temp_max,
                "sequence_id": "1",
            }
        }
        # Only include setting_id if provided (it's optional)
        if setting_id:
            command["print"]["setting_id"] = setting_id

        topic = f"device/{self.serial}/request"
        payload_json = json.dumps(command)
        logger.info(f"[{self.serial}] MQTT ams_filament_setting: {payload_json}")
        try:
            result = self._client.publish(topic, payload_json)
            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                logger.info(
                    f"Set filament on {self.serial}: AMS {ams_id}, tray {tray_id}, "
                    f"type={tray_type}, color={tray_color}, tray_info_idx={tray_info_idx}"
                )
                return True
            else:
                logger.error(f"Failed to publish filament setting: {result.rc}")
                return False
        except Exception as e:
            logger.error(f"Error setting filament on {self.serial}: {e}")
            return False

    def stage_assignment(
        self,
        ams_id: int,
        tray_id: int,
        spool_id: str,
        tray_info_idx: str = "",
        setting_id: str = "",
        tray_type: str = "",
        tray_color: str = "FFFFFFFF",
        nozzle_temp_min: int = 190,
        nozzle_temp_max: int = 230,
        cali_idx: int = -1,
        nozzle_diameter: str = "0.4",
    ) -> bool:
        """Stage a pending assignment for an AMS slot.

        The assignment will be executed when a spool is inserted into the slot.

        Args:
            ams_id: AMS unit ID
            tray_id: Tray ID within AMS
            spool_id: Spool ID from database
            tray_info_idx: Filament ID (short format)
            setting_id: Setting ID (full format)
            tray_type: Material type
            tray_color: RGBA hex color
            nozzle_temp_min: Minimum nozzle temperature
            nozzle_temp_max: Maximum nozzle temperature
            cali_idx: K-profile calibration index (-1 for no profile)
            nozzle_diameter: Nozzle diameter for calibration

        Returns:
            True if staged successfully
        """
        key = (ams_id, tray_id)
        self._pending_assignments[key] = PendingAssignment(
            spool_id=spool_id,
            tray_info_idx=tray_info_idx,
            setting_id=setting_id,
            tray_type=tray_type,
            tray_color=tray_color,
            nozzle_temp_min=nozzle_temp_min,
            nozzle_temp_max=nozzle_temp_max,
            cali_idx=cali_idx,
            nozzle_diameter=nozzle_diameter,
        )
        return True

    def cancel_assignment(self, ams_id: int, tray_id: int) -> bool:
        """Cancel a pending assignment for an AMS slot.

        Returns:
            True if an assignment was cancelled, False if none existed
        """
        key = (ams_id, tray_id)
        if key in self._pending_assignments:
            del self._pending_assignments[key]
            return True
        return False

    def get_pending_assignment(self, ams_id: int, tray_id: int) -> Optional[PendingAssignment]:
        """Get pending assignment for an AMS slot."""
        return self._pending_assignments.get((ams_id, tray_id))

    def get_all_pending_assignments(self) -> dict:
        """Get all pending assignments."""
        return dict(self._pending_assignments)

    def _execute_pending_assignment(self, ams_id: int, tray_id: int):
        """Execute a pending assignment for an AMS slot.

        Called when a spool is detected in a slot that has a pending assignment.
        """
        key = (ams_id, tray_id)
        assignment = self._pending_assignments.get(key)
        if not assignment:
            return

        # Send the filament setting command
        success = self.set_filament(
            ams_id=ams_id,
            tray_id=tray_id,
            tray_info_idx=assignment.tray_info_idx,
            setting_id=assignment.setting_id,
            tray_type=assignment.tray_type,
            tray_color=assignment.tray_color,
            nozzle_temp_min=assignment.nozzle_temp_min,
            nozzle_temp_max=assignment.nozzle_temp_max,
        )

        # Also send extrusion_cali_sel to set K-profile (like SpoolEase does)
        self.set_calibration(
            ams_id=ams_id,
            tray_id=tray_id,
            cali_idx=assignment.cali_idx,
            filament_id=assignment.tray_info_idx,
            nozzle_diameter=assignment.nozzle_diameter,
        )

        # Remove from pending regardless of success (user can retry)
        del self._pending_assignments[key]

        # Notify callback
        if self._on_assignment_complete and self._loop:
            spool_id = assignment.spool_id
            self._loop.call_soon_threadsafe(
                lambda: self._on_assignment_complete(self.serial, ams_id, tray_id, spool_id, success)
            )

    def _on_connect(self, client, userdata, flags, reason_code, properties):
        """MQTT connect callback."""
        logger.info(f"MQTT _on_connect for {self.serial}: reason_code={reason_code}")
        if reason_code == 0:
            self._connected = True
            self._disconnect_time = None  # Clear disconnect timestamp on reconnect
            logger.info(f"Connected to printer {self.serial} - _connected is now True")

            # Subscribe to report topic
            topic = f"device/{self.serial}/report"
            client.subscribe(topic)
            logger.info(f"Subscribed to {topic}")

            # Fetch calibrations for all nozzle sizes
            for nozzle_diameter in ["0.2", "0.4", "0.6", "0.8"]:
                self._fetch_calibrations(nozzle_diameter)

            # Request full state
            self._send_pushall()

            # Notify connect callback
            if hasattr(self, '_on_connect_callback') and self._on_connect_callback:
                if self._loop and self._loop.is_running():
                    self._loop.call_soon_threadsafe(
                        lambda: self._on_connect_callback(self.serial)
                    )
        else:
            logger.error(f"Connection to {self.serial} failed: {reason_code}")

    def _on_disconnect(self, client, userdata, flags, reason_code, properties):
        """MQTT disconnect callback."""
        self._connected = False
        self._disconnect_time = time.time()  # Track when disconnect happened
        logger.info(f"Disconnected from printer {self.serial}: {reason_code} (grace period: {DISCONNECT_GRACE_PERIOD_SEC}s)")

        # Notify disconnect callback
        if hasattr(self, '_on_disconnect_callback') and self._on_disconnect_callback:
            if self._loop and self._loop.is_running():
                self._loop.call_soon_threadsafe(
                    lambda: self._on_disconnect_callback(self.serial)
                )

    def _on_message(self, client, userdata, msg):
        """MQTT message callback."""
        try:
            payload = json.loads(msg.payload.decode())
            # Debug: log messages that might contain calibration data or command responses
            if "print" in payload:
                print_data = payload["print"]
                cmd = print_data.get("command", "")
                if "filaments" in print_data or cmd in ["extrusion_cali_get", "extrusion_cali_sel", "ams_filament_setting"]:
                    logger.info(f"[{self.serial}] CMD RESPONSE [{cmd}]: {json.dumps(print_data)[:500]}")
            self._handle_message(payload)
        except json.JSONDecodeError as e:
            logger.debug(f"Failed to parse message: {e}")
        except Exception as e:
            logger.error(f"Error handling message from {self.serial}: {e}")

    def _send_pushall(self):
        """Request full printer state."""
        if self._client and self._connected:
            topic = f"device/{self.serial}/request"
            payload = json.dumps({"pushing": {"command": "pushall", "sequence_id": "1"}})
            self._client.publish(topic, payload)
            logger.debug(f"[{self.serial}] Sent pushall request")

    def refresh_state(self):
        """Request full printer state refresh (public method)."""
        self._send_pushall()

    def _fetch_calibrations(self, nozzle_diameter: str = "0.4"):
        """Request calibration profiles for a nozzle diameter."""
        if self._client and self._connected:
            topic = f"device/{self.serial}/request"
            payload = json.dumps({
                "print": {
                    "command": "extrusion_cali_get",
                    "filament_id": "",
                    "nozzle_diameter": nozzle_diameter,
                    "sequence_id": "1"
                }
            })
            self._client.publish(topic, payload)
            logger.info(f"[{self.serial}] Requested calibrations for nozzle {nozzle_diameter}")

    def _handle_message(self, payload: dict):
        """Process incoming MQTT message."""
        if "print" not in payload:
            return

        print_data = payload["print"]

        # Debug: log command if present
        command = print_data.get("command")
        if command:
            logger.debug(f"[{self.serial}] Received command: {command}")

        # Debug: check if there are filaments in the response (calibration data)
        if "filaments" in print_data:
            logger.info(f"[{self.serial}] Found 'filaments' in message with command={command}, count={len(print_data.get('filaments', []))}")
            self._handle_calibration_response(print_data)
            return

        # Handle extrusion_cali_get response (calibration profiles)
        if command == "extrusion_cali_get":
            self._handle_calibration_response(print_data)
            return

        # Extract nozzle diameter (single-nozzle printers)
        if "nozzle_diameter" in print_data:
            self._nozzle_diameters[0] = str(print_data["nozzle_diameter"])

        # Extract gcode state
        if "gcode_state" in print_data:
            self._state.gcode_state = print_data["gcode_state"]

        # Extract current stage (stg_cur) - detailed printer status
        if "stg_cur" in print_data:
            new_stg = print_data["stg_cur"]
            self._state.stg_cur = new_stg
            self._state.stg_cur_name = get_stage_name(new_stg)

        # Extract progress (mc_percent is actual print progress)
        if "mc_percent" in print_data:
            self._state.print_progress = print_data["mc_percent"]

        # Extract subtask name
        if "subtask_name" in print_data:
            self._state.subtask_name = print_data["subtask_name"]

        # Extract remaining time (in minutes)
        if "mc_remaining_time" in print_data:
            self._state.mc_remaining_time = self._safe_int(print_data["mc_remaining_time"])

        # Extract gcode file path
        if "gcode_file" in print_data:
            self._state.gcode_file = print_data["gcode_file"]

        # Extract layer info from nested "3D" object or direct fields
        data_3d = print_data.get("3D", {})
        if "layer_num" in data_3d:
            self._state.layer_num = data_3d["layer_num"]
        elif "layer_num" in print_data:
            self._state.layer_num = print_data["layer_num"]

        if "total_layer_num" in data_3d:
            self._state.total_layer_num = data_3d["total_layer_num"]
        elif "total_layer_num" in print_data:
            self._state.total_layer_num = print_data["total_layer_num"]

        # Extract AMS data
        if "ams" in print_data:
            self._parse_ams_data(print_data["ams"])

        # Extract virtual tray (external spool)
        if "vt_tray" in print_data:
            self._state.vt_tray = self._parse_tray(print_data["vt_tray"], 255, 0)

        # Extract tray_now (currently active tray) from AMS data
        ams_data = print_data.get("ams", {})
        if "tray_now" in ams_data:
            tray_now_raw = ams_data["tray_now"]
            self._state.tray_now = self._safe_int(tray_now_raw)

        # Dual-nozzle support: parse device.extruder for H2C/H2D printers
        device_data = print_data.get("device", {})
        extruder_data = device_data.get("extruder", {})
        extruder_info = extruder_data.get("info", [])
        extruder_state = extruder_data.get("state")

        if extruder_info:
            # Detect dual-nozzle: must have 2+ extruder entries (single-nozzle may have 1)
            if len(extruder_info) >= 2:
                # Set nozzle_count on state so frontend can detect dual-nozzle
                self._state.nozzle_count = 2
                if not self._nozzle_count_detected:
                    self._nozzle_count_detected = True
                    logger.info(f"[{self.serial}] Detected dual-nozzle printer (extruder_info has {len(extruder_info)} entries)")
                    if self._on_nozzle_count_update and self._loop:
                        serial = self.serial  # Capture for lambda
                        self._loop.call_soon_threadsafe(
                            lambda s=serial: self._on_nozzle_count_update(s, 2)
                        )

            # Parse per-extruder tray_now values from 'snow' field
            # snow is encoded as: (ams_id << 8) | slot_id
            # We decode it to a global tray index: ams_id * 4 + slot_id
            for ext_info in extruder_info:
                ext_id = ext_info.get("id")  # 0=right, 1=left

                # Parse nozzle diameter (dia field)
                nozzle_dia = ext_info.get("dia")
                if nozzle_dia is not None and ext_id is not None:
                    self._nozzle_diameters[ext_id] = str(nozzle_dia)

                snow = ext_info.get("snow")  # encoded tray_now for this extruder
                if snow is not None:
                    snow_int = self._safe_int(snow)
                    if snow_int is not None and snow_int != 65535 and snow_int != 65279:  # 0xFFFF and 0xFEFF are "no tray"
                        # Decode: ams_id = high byte, slot_id = low byte
                        ams_id = (snow_int >> 8) & 0xFF
                        slot_id = snow_int & 0xFF
                        # Convert to global tray index (consistent with legacy tray_now)
                        if ams_id <= 3:
                            global_tray = ams_id * 4 + slot_id
                        elif ams_id >= 128:
                            global_tray = 16 + (ams_id - 128)  # HT uses indices 16+
                        else:
                            global_tray = snow_int  # External or unknown
                        if ext_id == 0:
                            self._state.tray_now_right = global_tray
                        elif ext_id == 1:
                            self._state.tray_now_left = global_tray

        if extruder_state is not None:
            # Active extruder is in bits 4-7: (state >> 4) & 0xF
            state_val = self._safe_int(extruder_state)
            if state_val is not None:
                self._state.active_extruder = (state_val >> 4) & 0xF

        # Notify listener
        if self._on_state_update:
            # Schedule callback in event loop if running from MQTT thread
            if self._loop:
                self._loop.call_soon_threadsafe(
                    lambda: self._on_state_update(self.serial, self._state)
                )

    def _handle_calibration_response(self, print_data: dict):
        """Process calibration profiles from extrusion_cali_get response."""
        filaments = print_data.get("filaments", [])
        response_nozzle = print_data.get("nozzle_diameter")
        has_pending_request = self._pending_kprofile_response is not None
        expected_nozzle = self._expected_kprofile_nozzle

        logger.info(f"[{self.serial}] K-profile response: nozzle={response_nozzle}, {len(filaments)} profiles, pending={has_pending_request}, expected={expected_nozzle}")

        # If waiting for specific nozzle and this doesn't match, ignore (it's a broadcast)
        if has_pending_request and expected_nozzle and response_nozzle != expected_nozzle:
            logger.debug(f"[{self.serial}] Ignoring broadcast: got nozzle={response_nozzle}, waiting for {expected_nozzle}")
            return

        # Parse all profiles
        profiles = []
        for filament in filaments:
            cali_idx = filament.get("cali_idx")
            if cali_idx is None:
                continue

            k_value_str = filament.get("k_value", "0")
            try:
                k_value = float(k_value_str)
            except (ValueError, TypeError):
                k_value = 0.0

            calibration = Calibration(
                cali_idx=cali_idx,
                filament_id=filament.get("filament_id", ""),
                k_value=k_value,
                name=filament.get("name", ""),
                extruder_id=filament.get("extruder_id"),
                nozzle_diameter=response_nozzle,
            )
            self._calibrations[cali_idx] = calibration
            profiles.append({
                "cali_idx": cali_idx,
                "filament_id": calibration.filament_id,
                "k_value": calibration.k_value,
                "name": calibration.name,
                "nozzle_diameter": response_nozzle,
                "extruder_id": calibration.extruder_id,
            })

        # Update kprofiles list
        self._kprofiles = profiles
        logger.info(f"[{self.serial}] Stored {len(profiles)} K-profiles for nozzle {response_nozzle}")

        # Signal pending request if any
        if self._pending_kprofile_response:
            logger.info(f"[{self.serial}] Signaling pending K-profile request")
            if self._loop and self._loop.is_running():
                self._loop.call_soon_threadsafe(self._pending_kprofile_response.set)
            else:
                self._pending_kprofile_response.set()

    def _parse_ams_data(self, ams_data: dict):
        """Parse AMS units and trays from MQTT data."""
        if "ams" not in ams_data:
            return

        # Build/update AMS extruder map from info field
        # This map persists across updates even when info field is missing
        if not hasattr(self, "_ams_extruder_map"):
            self._ams_extruder_map = {}

        # Track previous tray states for detecting spool insertion
        if not hasattr(self, "_prev_tray_states"):
            self._prev_tray_states = {}  # (ams_id, tray_id) -> tray_type

        # Parse tray_reading_bits (bitmask of trays currently being read)
        tray_reading_bits_raw = ams_data.get("tray_reading_bits")
        if tray_reading_bits_raw is not None:
            try:
                new_tray_reading = int(tray_reading_bits_raw, 16) if isinstance(tray_reading_bits_raw, str) else int(tray_reading_bits_raw)
                if new_tray_reading != self._state.tray_reading_bits:
                    self._state.tray_reading_bits = new_tray_reading
            except (ValueError, TypeError):
                pass

        # Track if we have info field in this update (helps debug)
        has_info_field = any(unit.get("info") is not None for unit in ams_data["ams"])

        # Only parse extruder assignments for dual-nozzle printers
        # Single-nozzle printers also have info field but don't have dual extruders
        if self._nozzle_count_detected:
            for ams_unit in ams_data["ams"]:
                unit_id = self._safe_int(ams_unit.get("id"), 0)
                info = ams_unit.get("info")
                if info is not None:
                    try:
                        info_val = int(info) if isinstance(info, str) else info
                        # Extract bit 8 for extruder assignment
                        # Bit 8 = 0 means LEFT extruder (id 1), bit 8 = 1 means RIGHT extruder (id 0)
                        # So we invert: extruder_id = 1 - bit8
                        bit8 = (info_val >> 8) & 0x1
                        extruder_id = 1 - bit8  # 0=right, 1=left
                        self._ams_extruder_map[unit_id] = extruder_id
                    except (ValueError, TypeError):
                        pass

        # Collect trays to check for pending assignments
        trays_to_check = []

        units = []
        for ams_unit in ams_data["ams"]:
            unit_id = self._safe_int(ams_unit.get("id"), 0)

            # Prefer humidity_raw (actual percentage) over humidity (index 1-5)
            humidity_raw = ams_unit.get("humidity_raw")
            humidity_idx = ams_unit.get("humidity")
            humidity = None
            if humidity_raw is not None:
                humidity = self._safe_int(humidity_raw)
            if humidity is None and humidity_idx is not None:
                humidity = self._safe_int(humidity_idx)

            # Temperature from temp field
            temp = self._safe_float(ams_unit.get("temp"))

            # Get extruder from our persisted map (only for dual-nozzle printers)
            extruder = self._ams_extruder_map.get(unit_id) if self._nozzle_count_detected else None

            # Parse trays
            trays = []
            for tray_data in ams_unit.get("tray", []):
                tray_id = self._safe_int(tray_data.get("id"), 0)
                tray = self._parse_tray(tray_data, unit_id, tray_id)
                if tray:
                    trays.append(tray)

                    # Check for spool insertion (tray_type was empty, now has value)
                    key = (unit_id, tray_id)
                    prev_tray_type = self._prev_tray_states.get(key)
                    curr_tray_type = tray.tray_type

                    # Update state tracking
                    self._prev_tray_states[key] = curr_tray_type

                    # Detect insertion: was empty (None or ""), now has type
                    was_empty = not prev_tray_type
                    is_occupied = bool(curr_tray_type)

                    if was_empty and is_occupied and key in self._pending_assignments:
                        trays_to_check.append((unit_id, tray_id))

            units.append(AmsUnit(
                id=unit_id,
                humidity=humidity,
                temperature=temp,
                extruder=extruder,
                trays=trays,
            ))

        self._state.ams_units = units

        # Execute any pending assignments (after state is updated)
        for ams_id, tray_id in trays_to_check:
            self._execute_pending_assignment(ams_id, tray_id)

    def _parse_tray(self, tray_data: dict, ams_id: int, tray_id: int) -> Optional[AmsTray]:
        """Parse single tray data."""
        if not tray_data:
            return None

        # Resolve k_value:
        # 1. Try direct "k" field in tray data
        # 2. If not present, look up from calibrations using cali_idx
        k_value = None

        # Direct k value from tray
        k_raw = tray_data.get("k")
        if k_raw is not None:
            try:
                k_value = float(k_raw)
            except (ValueError, TypeError):
                pass

        # If no direct k value, try to look up from calibrations
        if k_value is None:
            cali_idx = tray_data.get("cali_idx")
            if cali_idx is not None and cali_idx > 0:
                calibration = self._calibrations.get(cali_idx)
                if calibration:
                    k_value = calibration.k_value
                    logger.debug(f"Resolved k_value={k_value} from cali_idx={cali_idx}")
                # If no calibration found, k_value remains None

        tray = AmsTray(
            ams_id=ams_id,
            tray_id=tray_id,
            tray_type=tray_data.get("tray_type"),
            tray_color=tray_data.get("tray_color"),
            tray_info_idx=tray_data.get("tray_info_idx"),
            k_value=k_value,
            nozzle_temp_min=self._safe_int(tray_data.get("nozzle_temp_min")),
            nozzle_temp_max=self._safe_int(tray_data.get("nozzle_temp_max")),
            remain=self._safe_int(tray_data.get("remain")),
        )

        return tray

    @staticmethod
    def _safe_int(value: Any, default: Optional[int] = None) -> Optional[int]:
        """Safely convert value to int."""
        if value is None:
            return default
        try:
            return int(value)
        except (ValueError, TypeError):
            return default

    @staticmethod
    def _safe_float(value: Any, default: Optional[float] = None) -> Optional[float]:
        """Safely convert value to float."""
        if value is None:
            return default
        try:
            return float(value)
        except (ValueError, TypeError):
            return default


class PrinterManager:
    """Manages multiple printer connections."""

    def __init__(self):
        self._connections: dict[str, PrinterConnection] = {}
        self._on_state_update: Optional[Callable[[str, PrinterState], None]] = None
        self._on_disconnect: Optional[Callable[[str], None]] = None
        self._on_connect: Optional[Callable[[str], None]] = None
        self._on_assignment_complete: Optional[Callable[[str, int, int, str, bool], None]] = None
        self._on_tray_reading_change: Optional[Callable[[str, Optional[int], int], None]] = None
        self._on_nozzle_count_update: Optional[Callable[[str, int], None]] = None

    def set_state_callback(self, callback: Callable[[str, PrinterState], None]):
        """Set callback for printer state updates."""
        self._on_state_update = callback

    def set_disconnect_callback(self, callback: Callable[[str], None]):
        """Set callback for printer disconnection."""
        self._on_disconnect = callback

    def set_connect_callback(self, callback: Callable[[str], None]):
        """Set callback for printer connection."""
        self._on_connect = callback

    def set_assignment_complete_callback(self, callback: Callable[[str, int, int, str, bool], None]):
        """Set callback for when a staged assignment completes.

        Callback receives: (serial, ams_id, tray_id, spool_id, success)
        """
        self._on_assignment_complete = callback
        # Also set on existing connections
        for conn in self._connections.values():
            conn._on_assignment_complete = callback

    def set_tray_reading_callback(self, callback: Callable[[str, Optional[int], int], None]):
        """Set callback for when tray reading state changes.

        Callback receives: (serial, old_bits, new_bits)
        The bits indicate which trays are currently being read (RFID scanning).
        """
        self._on_tray_reading_change = callback
        # Also set on existing connections
        for conn in self._connections.values():
            conn._on_tray_reading_change = callback

    def set_nozzle_count_callback(self, callback: Callable[[str, int], None]):
        """Set callback for when nozzle count is detected from MQTT.

        Callback receives: (serial, nozzle_count)
        Used to auto-detect dual-nozzle printers (H2C/H2D).
        """
        self._on_nozzle_count_update = callback
        # Also set on existing connections
        for conn in self._connections.values():
            conn._on_nozzle_count_update = callback

    async def connect(self, serial: str, ip_address: str, access_code: str, name: Optional[str] = None):
        """Connect to a printer."""
        if serial in self._connections:
            logger.warning(f"Printer {serial} already connected")
            return

        conn = PrinterConnection(
            serial=serial,
            ip_address=ip_address,
            access_code=access_code,
            name=name,
        )

        # Set assignment callback if configured
        if self._on_assignment_complete:
            conn._on_assignment_complete = self._on_assignment_complete

        # Set tray reading callback if configured
        if self._on_tray_reading_change:
            conn._on_tray_reading_change = self._on_tray_reading_change

        # Set nozzle count callback if configured
        if self._on_nozzle_count_update:
            conn._on_nozzle_count_update = self._on_nozzle_count_update

        try:
            conn.connect(self._handle_state_update, self._handle_disconnect, self._handle_connect)
            self._connections[serial] = conn
        except Exception as e:
            logger.error(f"Failed to connect to {serial}: {e}")
            raise

    def _handle_connect(self, serial: str):
        """Handle printer connection."""
        logger.info(f"Printer {serial} connected successfully")
        # Notify external callback
        if self._on_connect:
            self._on_connect(serial)

    def _handle_disconnect(self, serial: str):
        """Handle printer disconnection."""
        logger.info(f"PrinterManager: Printer {serial} disconnected")
        # Don't remove from _connections - allow reconnection to work
        # The PrinterConnection.connected property handles status correctly
        # and paho-mqtt's loop_start() will attempt automatic reconnection
        if serial in self._connections:
            conn = self._connections[serial]
            logger.info(f"PrinterManager: {serial} status: _connected={conn._connected}, disconnect_time={conn._disconnect_time}")
        # Notify external callback
        if self._on_disconnect:
            self._on_disconnect(serial)

    async def disconnect(self, serial: str):
        """Disconnect from a printer."""
        if serial not in self._connections:
            return

        conn = self._connections.pop(serial)
        conn.disconnect()

    async def disconnect_all(self):
        """Disconnect all printers."""
        for serial in list(self._connections.keys()):
            await self.disconnect(serial)

    def refresh_all(self):
        """Request full state refresh from all connected printers.

        This sends a pushall command to each printer, which makes them send
        their complete current state including AMS extruder assignments.
        """
        refreshed = 0
        for serial, conn in self._connections.items():
            if conn.connected:
                conn.refresh_state()
                refreshed += 1
        if refreshed > 0:
            logger.info(f"Requested state refresh from {refreshed} printer(s)")

    def is_connected(self, serial: str) -> bool:
        """Check if printer is connected."""
        conn = self._connections.get(serial)
        return conn.connected if conn else False

    def get_state(self, serial: str) -> Optional[PrinterState]:
        """Get printer state."""
        conn = self._connections.get(serial)
        return conn.state if conn else None

    def get_connection_statuses(self) -> dict[str, bool]:
        """Get connection status for all managed printers."""
        statuses = {serial: conn.connected for serial, conn in self._connections.items()}
        # Log detailed status for debugging
        for serial, conn in self._connections.items():
            logger.info(f"Connection status for {serial}: _connected={conn._connected}, disconnect_time={conn._disconnect_time}, connected_prop={conn.connected}")
        return statuses

    def set_filament(
        self,
        serial: str,
        ams_id: int,
        tray_id: int,
        tray_info_idx: str = "",
        setting_id: str = "",
        tray_type: str = "",
        tray_color: str = "FFFFFFFF",
        nozzle_temp_min: int = 190,
        nozzle_temp_max: int = 230,
    ) -> bool:
        """Set filament for an AMS slot on a printer."""
        conn = self._connections.get(serial)
        if not conn:
            logger.error(f"Printer {serial} not connected")
            return False

        return conn.set_filament(
            ams_id=ams_id,
            tray_id=tray_id,
            tray_info_idx=tray_info_idx,
            setting_id=setting_id,
            tray_type=tray_type,
            tray_color=tray_color,
            nozzle_temp_min=nozzle_temp_min,
            nozzle_temp_max=nozzle_temp_max,
        )

    def reset_slot(self, serial: str, ams_id: int, tray_id: int) -> bool:
        """Reset/clear an AMS slot on a printer."""
        conn = self._connections.get(serial)
        if not conn:
            logger.error(f"Printer {serial} not connected")
            return False

        return conn.reset_slot(ams_id=ams_id, tray_id=tray_id)

    def set_calibration(
        self,
        serial: str,
        ams_id: int,
        tray_id: int,
        cali_idx: int = -1,
        filament_id: str = "",
        nozzle_diameter: str = "0.4",
    ) -> bool:
        """Set calibration profile for an AMS slot on a printer."""
        conn = self._connections.get(serial)
        if not conn:
            logger.error(f"Printer {serial} not connected")
            return False

        return conn.set_calibration(
            ams_id=ams_id,
            tray_id=tray_id,
            cali_idx=cali_idx,
            filament_id=filament_id,
            nozzle_diameter=nozzle_diameter,
        )

    def get_calibrations(self, serial: str) -> list[dict]:
        """Get calibration profiles for a printer (sync, returns cached)."""
        conn = self._connections.get(serial)
        if not conn:
            return []

        return conn.get_calibrations()

    async def get_kprofiles(self, serial: str, nozzle_diameter: str = "0.4") -> list[dict]:
        """Get K-profiles from printer (async with retry)."""
        conn = self._connections.get(serial)
        if not conn:
            logger.warning(f"[{serial}] No connection found for get_kprofiles")
            return []

        return await conn.get_kprofiles(nozzle_diameter)

    def get_nozzle_diameter(self, serial: str, extruder_id: int = 0) -> str:
        """Get nozzle diameter for a printer's extruder."""
        conn = self._connections.get(serial)
        if not conn:
            return "0.4"  # Fallback
        return conn.get_nozzle_diameter(extruder_id)

    def stage_assignment(
        self,
        serial: str,
        ams_id: int,
        tray_id: int,
        spool_id: str,
        tray_info_idx: str = "",
        setting_id: str = "",
        tray_type: str = "",
        tray_color: str = "FFFFFFFF",
        nozzle_temp_min: int = 190,
        nozzle_temp_max: int = 230,
        cali_idx: int = -1,
        nozzle_diameter: str = "0.4",
    ) -> bool:
        """Stage a pending assignment for an AMS slot.

        The assignment will be executed when a spool is inserted into the slot.
        """
        conn = self._connections.get(serial)
        if not conn:
            logger.error(f"Printer {serial} not connected")
            return False

        return conn.stage_assignment(
            ams_id=ams_id,
            tray_id=tray_id,
            spool_id=spool_id,
            tray_info_idx=tray_info_idx,
            setting_id=setting_id,
            tray_type=tray_type,
            tray_color=tray_color,
            nozzle_temp_min=nozzle_temp_min,
            nozzle_temp_max=nozzle_temp_max,
            cali_idx=cali_idx,
            nozzle_diameter=nozzle_diameter,
        )

    def cancel_assignment(self, serial: str, ams_id: int, tray_id: int) -> bool:
        """Cancel a pending assignment for an AMS slot."""
        conn = self._connections.get(serial)
        if not conn:
            return False

        return conn.cancel_assignment(ams_id, tray_id)

    def get_pending_assignment(self, serial: str, ams_id: int, tray_id: int) -> Optional[PendingAssignment]:
        """Get pending assignment for an AMS slot."""
        conn = self._connections.get(serial)
        if not conn:
            return None

        return conn.get_pending_assignment(ams_id, tray_id)

    def get_all_pending_assignments(self, serial: str) -> dict:
        """Get all pending assignments for a printer."""
        conn = self._connections.get(serial)
        if not conn:
            return {}

        return conn.get_all_pending_assignments()

    def _handle_state_update(self, serial: str, state: PrinterState):
        """Handle state update from printer."""
        if self._on_state_update:
            self._on_state_update(serial, state)
