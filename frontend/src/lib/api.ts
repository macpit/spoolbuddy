const API_BASE = "/api";

export interface Spool {
  id: string;
  spool_number: number | null;
  tag_id: string | null;
  material: string;
  subtype: string | null;
  color_name: string | null;
  rgba: string | null;
  brand: string | null;
  label_weight: number;
  core_weight: number;
  weight_new: number | null;
  weight_current: number | null;
  slicer_filament: string | null;
  slicer_filament_name: string | null;
  location: string | null;
  note: string | null;
  added_time: string | null;  // Unix timestamp as string for compat
  encode_time: string | null; // Unix timestamp as string for compat
  added_full: boolean | null;
  consumed_since_add: number;
  consumed_since_weight: number;
  weight_used: number;        // For tracking manual used weight
  data_origin: string | null;
  tag_type: string | null;
  ext_has_k: boolean;         // Whether has pressure advance K calibration
  archived_at: number | null;  // Unix timestamp when archived, null = active
  created_at: number | null;
  updated_at: number | null;
  last_used_time: number | null;  // Unix timestamp of last usage
}

// Map for tracking which spools are in which printers
export type SpoolsInPrinters = Record<string, string>;

export interface SpoolInput {
  tag_id?: string | null;
  material: string;
  subtype?: string | null;
  color_name?: string | null;
  rgba?: string | null;
  brand?: string | null;
  label_weight?: number | null;
  core_weight?: number | null;
  weight_new?: number | null;
  weight_current?: number | null;
  slicer_filament?: string | null;
  slicer_filament_name?: string | null;
  location?: string | null;
  note?: string | null;
  data_origin?: string | null;
  tag_type?: string | null;
  ext_has_k?: boolean;  // Whether has pressure advance K calibration
}

export interface Printer {
  serial: string;
  name: string | null;
  model: string | null;
  ip_address: string | null;
  access_code: string | null;
  last_seen: number | null;
  config: string | null;
  auto_connect: boolean | null;
  connected?: boolean;
}

export interface PrinterInput {
  serial: string;
  name?: string | null;
  model?: string | null;
  ip_address?: string | null;
  access_code?: string | null;
}

export interface SetSlotRequest {
  ams_id: number;
  tray_id: number;
  tray_info_idx: string;
  tray_type: string;
  tray_sub_brands: string;  // Preset name for slicer display (e.g., "Bambu PLA Basic")
  tray_color: string;
  nozzle_temp_min: number;
  nozzle_temp_max: number;
  setting_id: string;  // Full setting ID with version (e.g., "GFSL05_07")
}

export interface SetCalibrationRequest {
  cali_idx: number;  // -1 for default (0.02), or calibration profile index
  filament_id?: string;  // K profile's filament_id (optional)
  nozzle_diameter?: string;  // Nozzle diameter (default "0.4")
  setting_id?: string;  // K profile's setting_id for slicer compatibility (optional)
  k_value?: number;  // Direct K value to set (0.0 = skip direct setting)
  nozzle_temp_max?: number;  // Max nozzle temp for extrusion_cali_set
}

export interface AssignSpoolRequest {
  spool_id: string;
}

export interface AssignSpoolResponse {
  status: "configured" | "staged";
  message: string;
  needs_replacement: boolean;
}

export interface CalibrationProfile {
  cali_idx: number;
  filament_id: string;
  k_value: number;
  name: string;
  nozzle_diameter: string | null;
  extruder_id?: number;  // 0=right, 1=left (for dual nozzle printers)
  setting_id?: string | null;  // K profile's setting_id for slicer compatibility
}

// K-profile stored with spool
export interface SpoolKProfile {
  id?: number;
  spool_id?: string;
  printer_serial: string;
  extruder: number | null;
  nozzle_diameter: string | null;
  nozzle_type: string | null;
  k_value: string;
  name: string | null;
  cali_idx: number | null;
  setting_id: string | null;
  created_at?: number;
}

export interface DeviceStatus {
  connected: boolean;
  last_weight: number | null;
  weight_stable: boolean;
  current_tag_id: string | null;
}

// Cloud API types
export interface CloudAuthStatus {
  is_authenticated: boolean;
  email: string | null;
}

export interface CloudLoginResponse {
  success: boolean;
  needs_verification: boolean;
  message: string;
}

export interface SlicerPreset {
  setting_id: string;
  name: string;
  type: string;
  version: string | null;
  user_id: string | null;
  is_custom: boolean;
}

export interface SlicerSettingsResponse {
  filament: SlicerPreset[];
  printer: SlicerPreset[];
  process: SlicerPreset[];
}

export interface DiscoveryStatus {
  running: boolean;
}

export interface DiscoveredPrinter {
  serial: string;
  name: string | null;
  ip_address: string;
  model: string | null;
}

// Update API types
export interface VersionInfo {
  version: string;
  git_commit: string | null;
  git_branch: string | null;
}

export interface UpdateCheck {
  current_version: string;
  latest_version: string | null;
  update_available: boolean;
  release_notes: string | null;
  release_url: string | null;
  published_at: string | null;
  error: string | null;
}

export interface UpdateStatus {
  status: "idle" | "checking" | "downloading" | "applying" | "restarting" | "error";
  message: string | null;
  progress: number | null;
  error: string | null;
}

export interface FirmwareVersion {
  version: string;
  filename: string;
  size: number | null;
  checksum: string | null;
  url: string | null;
}

export interface FirmwareCheck {
  current_version: string | null;
  latest_version: string | null;
  update_available: boolean;
  download_url: string | null;
  release_notes: string | null;
  error: string | null;
}

// ESP32 Device Connection types
export interface ESP32DeviceInfo {
  ip: string;
  hostname: string | null;
  mac_address: string | null;
  firmware_version: string | null;
  nfc_status: boolean | null;
  scale_status: boolean | null;
  uptime: number | null;
  last_seen: string | null;
}

export interface ESP32DeviceConfig {
  ip: string;
  port: number;
  name: string | null;
}

export interface ESP32ConnectionStatus {
  connected: boolean;
  device: ESP32DeviceInfo | null;
  last_error: string | null;
  reconnect_attempts: number;
}

export interface ESP32DiscoveryResult {
  devices: ESP32DeviceInfo[];
  scan_duration_ms: number;
}

export interface ESP32RecoveryInfo {
  steps: string[];
  serial_commands: Record<string, string>;
  firmware_url: string | null;
}

// Spool catalog types
export interface CatalogEntry {
  id: number;
  name: string;
  weight: number;
  is_default: boolean;
  created_at: number | null;
}

export interface CatalogEntryInput {
  name: string;
  weight: number;
}

// Color catalog types
export interface ColorEntry {
  id: number;
  manufacturer: string;
  color_name: string;
  hex_color: string;
  material: string | null;
  is_default: boolean;
  created_at: number | null;
}

export interface ColorEntryInput {
  manufacturer: string;
  color_name: string;
  hex_color: string;
  material?: string | null;
}

export interface ColorLookupResult {
  found: boolean;
  hex_color: string | null;
  material: string | null;
}

// AMS History types
export interface AMSHistoryPoint {
  recorded_at: number;
  humidity: number | null;
  humidity_raw: number | null;
  temperature: number | null;
}

export interface AMSHistoryResponse {
  printer_serial: string;
  ams_id: number;
  data: AMSHistoryPoint[];
  min_humidity: number | null;
  max_humidity: number | null;
  avg_humidity: number | null;
  min_temperature: number | null;
  max_temperature: number | null;
  avg_temperature: number | null;
}

// AMS Thresholds
export interface AMSThresholds {
  humidity_good: number;
  humidity_fair: number;
  temp_good: number;
  temp_fair: number;
  history_retention_days: number;
}

// Support API types
export interface DebugLoggingState {
  enabled: boolean;
  enabled_at: string | null;
  duration_seconds: number | null;
}

export interface LogEntry {
  timestamp: string;
  level: string;
  logger_name: string;
  message: string;
}

export interface LogsResponse {
  entries: LogEntry[];
  total_count: number;
  filtered_count: number;
}

export interface SystemInfo {
  // Application
  version: string;
  uptime: string;
  uptime_seconds: number;
  hostname: string;
  // Database stats
  spool_count: number;
  printer_count: number;
  connected_printers: number;
  // Storage
  database_size: string;
  disk_total: string;
  disk_used: string;
  disk_free: string;
  disk_percent: number;
  // System
  platform: string;
  platform_release: string;
  python_version: string;
  boot_time: string;
  // Memory
  memory_total: string;
  memory_used: string;
  memory_available: string;
  memory_percent: number;
  // CPU
  cpu_count: number;
  cpu_percent: number;
}

class ApiClient {
  private async request<T>(
    path: string,
    options: RequestInit = {}
  ): Promise<T> {
    const response = await fetch(`${API_BASE}${path}`, {
      ...options,
      headers: {
        "Content-Type": "application/json",
        ...options.headers,
      },
    });

    if (!response.ok) {
      const error = await response.text();
      throw new Error(error || `HTTP ${response.status}`);
    }

    // Handle 204 No Content or empty body
    if (response.status === 204) {
      return undefined as T;
    }

    // Check if response has content before parsing JSON
    const text = await response.text();
    if (!text) {
      return undefined as T;
    }

    return JSON.parse(text);
  }

  // Spools
  async listSpools(params?: {
    material?: string;
    brand?: string;
    search?: string;
  }): Promise<Spool[]> {
    const searchParams = new URLSearchParams();
    if (params?.material) searchParams.set("material", params.material);
    if (params?.brand) searchParams.set("brand", params.brand);
    if (params?.search) searchParams.set("search", params.search);

    const query = searchParams.toString();
    return this.request<Spool[]>(`/spools${query ? `?${query}` : ""}`);
  }

  async getSpool(id: string): Promise<Spool> {
    return this.request<Spool>(`/spools/${id}`);
  }

  async createSpool(input: SpoolInput): Promise<Spool> {
    return this.request<Spool>("/spools", {
      method: "POST",
      body: JSON.stringify(input),
    });
  }

  async updateSpool(id: string, input: SpoolInput): Promise<Spool> {
    return this.request<Spool>(`/spools/${id}`, {
      method: "PUT",
      body: JSON.stringify(input),
    });
  }

  async deleteSpool(id: string): Promise<void> {
    return this.request<void>(`/spools/${id}`, {
      method: "DELETE",
    });
  }

  async archiveSpool(id: string): Promise<Spool> {
    return this.request<Spool>(`/spools/${id}/archive`, {
      method: "POST",
    });
  }

  async restoreSpool(id: string): Promise<Spool> {
    return this.request<Spool>(`/spools/${id}/restore`, {
      method: "POST",
    });
  }

  /**
   * Set spool weight from scale measurement.
   * This updates weight_current and resets consumed_since_weight to 0,
   * effectively syncing the tracking to match the scale.
   * @param id Spool ID
   * @param weight Total weight in grams (including spool core)
   */
  async setSpoolWeight(id: string, weight: number): Promise<Spool> {
    return this.request<Spool>(`/spools/${id}/weight`, {
      method: "POST",
      body: JSON.stringify({ weight }),
    });
  }

  // Spool K-Profiles
  async getSpoolKProfiles(spoolId: string): Promise<SpoolKProfile[]> {
    return this.request<SpoolKProfile[]>(`/spools/${spoolId}/k-profiles`);
  }

  async saveSpoolKProfiles(spoolId: string, profiles: Omit<SpoolKProfile, 'id' | 'spool_id' | 'created_at'>[]): Promise<{ status: string; count: number }> {
    return this.request<{ status: string; count: number }>(`/spools/${spoolId}/k-profiles`, {
      method: "PUT",
      body: JSON.stringify({ profiles }),
    });
  }

  // Printers
  async listPrinters(): Promise<Printer[]> {
    return this.request<Printer[]>("/printers");
  }

  async getPrinter(serial: string): Promise<Printer> {
    return this.request<Printer>(`/printers/${serial}`);
  }

  async createPrinter(input: PrinterInput): Promise<Printer> {
    return this.request<Printer>("/printers", {
      method: "POST",
      body: JSON.stringify(input),
    });
  }

  async updatePrinter(serial: string, input: Partial<PrinterInput>): Promise<Printer> {
    return this.request<Printer>(`/printers/${serial}`, {
      method: "PUT",
      body: JSON.stringify(input),
    });
  }

  async deletePrinter(serial: string): Promise<void> {
    return this.request<void>(`/printers/${serial}`, {
      method: "DELETE",
    });
  }

  async connectPrinter(serial: string): Promise<void> {
    return this.request<void>(`/printers/${serial}/connect`, {
      method: "POST",
    });
  }

  async disconnectPrinter(serial: string): Promise<void> {
    return this.request<void>(`/printers/${serial}/disconnect`, {
      method: "POST",
    });
  }

  async setSlotFilament(serial: string, request: SetSlotRequest): Promise<void> {
    const { ams_id, tray_id, ...filamentData } = request
    return this.request<void>(`/printers/${serial}/ams/${ams_id}/tray/${tray_id}/filament`, {
      method: "POST",
      body: JSON.stringify(filamentData),
    });
  }

  async setAutoConnect(serial: string, autoConnect: boolean): Promise<Printer> {
    return this.request<Printer>(`/printers/${serial}/auto-connect`, {
      method: "POST",
      body: JSON.stringify({ auto_connect: autoConnect }),
    });
  }

  // AMS slot operations

  /** Trigger RFID re-read on an AMS slot (sends ams_get_rfid command) */
  async rereadSlot(serial: string, amsId: number, trayId: number): Promise<void> {
    return this.request<void>(`/printers/${serial}/ams/${amsId}/tray/${trayId}/reset`, {
      method: "POST",
    });
  }

  /** Clear/reset an AMS slot to empty state by setting empty filament info */
  async clearSlot(serial: string, amsId: number, trayId: number): Promise<void> {
    return this.request<void>(`/printers/${serial}/ams/${amsId}/tray/${trayId}/filament`, {
      method: "POST",
      body: JSON.stringify({
        tray_info_idx: "",
        tray_type: "",
        tray_color: "FFFFFFFF",
        nozzle_temp_min: 0,
        nozzle_temp_max: 0,
      }),
    });
  }

  /** @deprecated Use rereadSlot instead */
  async resetSlot(serial: string, amsId: number, trayId: number): Promise<void> {
    return this.rereadSlot(serial, amsId, trayId);
  }

  async setCalibration(serial: string, amsId: number, trayId: number, request: SetCalibrationRequest): Promise<void> {
    return this.request<void>(`/printers/${serial}/ams/${amsId}/tray/${trayId}/calibration`, {
      method: "POST",
      body: JSON.stringify(request),
    });
  }

  async getCalibrations(serial: string): Promise<CalibrationProfile[]> {
    return this.request<CalibrationProfile[]>(`/printers/${serial}/calibrations`);
  }

  async assignSpoolToSlot(
    serial: string,
    amsId: number,
    trayId: number,
    spoolId: string
  ): Promise<AssignSpoolResponse> {
    return this.request<AssignSpoolResponse>(
      `/printers/${serial}/ams/${amsId}/tray/${trayId}/assign`,
      {
        method: "POST",
        body: JSON.stringify({ spool_id: spoolId }),
      }
    );
  }

  // Device
  async getDeviceStatus(): Promise<DeviceStatus> {
    return this.request<DeviceStatus>("/device/status");
  }

  async tareScale(): Promise<void> {
    return this.request<void>("/device/scale/tare", { method: "POST" });
  }

  async calibrateScale(knownWeight: number): Promise<void> {
    return this.request<void>(`/device/scale/calibrate?known_weight=${knownWeight}`, { method: "POST" });
  }

  async resetScaleCalibration(): Promise<void> {
    return this.request<void>("/device/scale/reset", { method: "POST" });
  }

  async writeTag(spoolId: string): Promise<void> {
    return this.request<void>("/device/write-tag", {
      method: "POST",
      body: JSON.stringify({ spool_id: spoolId }),
    });
  }

  // Discovery
  async getDiscoveryStatus(): Promise<DiscoveryStatus> {
    return this.request<DiscoveryStatus>("/discovery/status");
  }

  async startDiscovery(): Promise<DiscoveryStatus> {
    return this.request<DiscoveryStatus>("/discovery/start", {
      method: "POST",
    });
  }

  async stopDiscovery(): Promise<DiscoveryStatus> {
    return this.request<DiscoveryStatus>("/discovery/stop", {
      method: "POST",
    });
  }

  async getDiscoveredPrinters(): Promise<DiscoveredPrinter[]> {
    return this.request<DiscoveredPrinter[]>("/discovery/printers");
  }

  // Cloud API
  async getCloudStatus(): Promise<CloudAuthStatus> {
    return this.request<CloudAuthStatus>("/cloud/status");
  }

  async cloudLogin(email: string, password: string): Promise<CloudLoginResponse> {
    return this.request<CloudLoginResponse>("/cloud/login", {
      method: "POST",
      body: JSON.stringify({ email, password }),
    });
  }

  async cloudVerify(email: string, code: string): Promise<CloudLoginResponse> {
    return this.request<CloudLoginResponse>("/cloud/verify", {
      method: "POST",
      body: JSON.stringify({ email, code }),
    });
  }

  async cloudSetToken(access_token: string): Promise<CloudAuthStatus> {
    return this.request<CloudAuthStatus>("/cloud/token", {
      method: "POST",
      body: JSON.stringify({ access_token }),
    });
  }

  async cloudLogout(): Promise<void> {
    return this.request<void>("/cloud/logout", {
      method: "POST",
    });
  }

  async getSlicerSettings(): Promise<SlicerSettingsResponse> {
    return this.request<SlicerSettingsResponse>("/cloud/settings");
  }

  async getFilamentPresets(): Promise<SlicerPreset[]> {
    return this.request<SlicerPreset[]>("/cloud/filaments");
  }

  async getSettingDetail(settingId: string): Promise<{ setting_id: string; filament_id?: string; base_id?: string; name?: string }> {
    return this.request<{ setting_id: string; filament_id?: string; base_id?: string; name?: string }>(`/cloud/settings/${encodeURIComponent(settingId)}`);
  }

  // Updates API
  async getVersion(): Promise<VersionInfo> {
    return this.request<VersionInfo>("/updates/version");
  }

  async checkForUpdates(force: boolean = false): Promise<UpdateCheck> {
    const query = force ? "?force=true" : "";
    return this.request<UpdateCheck>(`/updates/check${query}`);
  }

  async applyUpdate(version?: string): Promise<UpdateStatus> {
    return this.request<UpdateStatus>("/updates/apply", {
      method: "POST",
      body: JSON.stringify({ version }),
    });
  }

  async getUpdateStatus(): Promise<UpdateStatus> {
    return this.request<UpdateStatus>("/updates/status");
  }

  async resetUpdateStatus(): Promise<UpdateStatus> {
    return this.request<UpdateStatus>("/updates/reset-status", {
      method: "POST",
    });
  }

  // Firmware API
  async getFirmwareVersions(): Promise<FirmwareVersion[]> {
    return this.request<FirmwareVersion[]>("/firmware/version");
  }

  async getLatestFirmware(): Promise<FirmwareVersion> {
    return this.request<FirmwareVersion>("/firmware/latest");
  }

  async checkFirmwareUpdate(currentVersion?: string): Promise<FirmwareCheck> {
    const query = currentVersion ? `?current_version=${encodeURIComponent(currentVersion)}` : "";
    return this.request<FirmwareCheck>(`/firmware/check${query}`);
  }

  // ESP32 Device Connection API
  async getESP32Status(): Promise<ESP32ConnectionStatus> {
    return this.request<ESP32ConnectionStatus>("/device/status");
  }

  async getESP32Config(): Promise<ESP32DeviceConfig | null> {
    return this.request<ESP32DeviceConfig | null>("/device/config");
  }

  async saveESP32Config(config: ESP32DeviceConfig): Promise<ESP32DeviceConfig> {
    return this.request<ESP32DeviceConfig>("/device/config", {
      method: "POST",
      body: JSON.stringify(config),
    });
  }

  async connectESP32(config?: ESP32DeviceConfig): Promise<ESP32ConnectionStatus> {
    return this.request<ESP32ConnectionStatus>("/device/connect", {
      method: "POST",
      body: config ? JSON.stringify(config) : undefined,
    });
  }

  async disconnectESP32(): Promise<void> {
    return this.request<void>("/device/disconnect", {
      method: "POST",
    });
  }

  async discoverESP32Devices(timeoutMs: number = 3000): Promise<ESP32DiscoveryResult> {
    return this.request<ESP32DiscoveryResult>(`/device/discover?timeout_ms=${timeoutMs}`, {
      method: "POST",
    });
  }

  async pingESP32(ip: string, port: number = 80): Promise<{ reachable: boolean; device: ESP32DeviceInfo | null }> {
    return this.request<{ reachable: boolean; device: ESP32DeviceInfo | null }>(`/device/ping?ip=${encodeURIComponent(ip)}&port=${port}`, {
      method: "POST",
    });
  }

  async rebootESP32(): Promise<{ success: boolean; message: string }> {
    return this.request<{ success: boolean; message: string }>("/device/reboot", {
      method: "POST",
    });
  }

  async triggerOTA(): Promise<{ success: boolean; message: string }> {
    return this.request<{ success: boolean; message: string }>("/device/update", {
      method: "POST",
    });
  }

  async getDisplayStatus(): Promise<{ connected: boolean; last_seen: number | null; firmware_version: string | null }> {
    return this.request<{ connected: boolean; last_seen: number | null; firmware_version: string | null }>("/display/status");
  }

  async factoryResetESP32(): Promise<{ success: boolean; message: string }> {
    return this.request<{ success: boolean; message: string }>("/device/factory-reset", {
      method: "POST",
    });
  }

  async getESP32RecoveryInfo(): Promise<ESP32RecoveryInfo> {
    return this.request<ESP32RecoveryInfo>("/device/recovery-info");
  }

  // Spool Catalog API
  async getSpoolCatalog(): Promise<CatalogEntry[]> {
    return this.request<CatalogEntry[]>("/catalog");
  }

  async addCatalogEntry(entry: CatalogEntryInput): Promise<CatalogEntry> {
    return this.request<CatalogEntry>("/catalog", {
      method: "POST",
      body: JSON.stringify(entry),
    });
  }

  async updateCatalogEntry(id: number, entry: CatalogEntryInput): Promise<CatalogEntry> {
    return this.request<CatalogEntry>(`/catalog/${id}`, {
      method: "PUT",
      body: JSON.stringify(entry),
    });
  }

  async deleteCatalogEntry(id: number): Promise<void> {
    return this.request<void>(`/catalog/${id}`, {
      method: "DELETE",
    });
  }

  async resetSpoolCatalog(): Promise<void> {
    return this.request<void>("/catalog/reset", {
      method: "POST",
    });
  }

  // Color Catalog API
  async getColorCatalog(): Promise<ColorEntry[]> {
    return this.request<ColorEntry[]>("/colors");
  }

  async addColorEntry(entry: ColorEntryInput): Promise<ColorEntry> {
    return this.request<ColorEntry>("/colors", {
      method: "POST",
      body: JSON.stringify(entry),
    });
  }

  async updateColorEntry(id: number, entry: ColorEntryInput): Promise<ColorEntry> {
    return this.request<ColorEntry>(`/colors/${id}`, {
      method: "PUT",
      body: JSON.stringify(entry),
    });
  }

  async deleteColorEntry(id: number): Promise<void> {
    return this.request<void>(`/colors/${id}`, {
      method: "DELETE",
    });
  }

  async resetColorCatalog(): Promise<void> {
    return this.request<void>("/colors/reset", {
      method: "POST",
    });
  }

  // Note: syncColorCatalog uses SSE streaming, called directly via fetch in ColorCatalogSettings

  async lookupColor(manufacturer: string, colorName: string, material?: string): Promise<ColorLookupResult> {
    const params = new URLSearchParams({ manufacturer, color_name: colorName });
    if (material) params.append("material", material);
    return this.request<ColorLookupResult>(`/colors/lookup?${params.toString()}`);
  }

  // AMS History API
  async getAMSHistory(printerSerial: string, amsId: number, hours: number = 24): Promise<AMSHistoryResponse> {
    return this.request<AMSHistoryResponse>(`/printers/${printerSerial}/ams/${amsId}/history?hours=${hours}`);
  }

  // AMS Thresholds API
  async getAMSThresholds(): Promise<AMSThresholds> {
    return this.request<AMSThresholds>("/settings/ams/thresholds");
  }

  async setAMSThresholds(thresholds: AMSThresholds): Promise<AMSThresholds> {
    return this.request<AMSThresholds>("/settings/ams/thresholds", {
      method: "PUT",
      body: JSON.stringify(thresholds),
    });
  }

  // Support API
  async getDebugLogging(): Promise<DebugLoggingState> {
    return this.request<DebugLoggingState>("/support/debug-logging");
  }

  async setDebugLogging(enabled: boolean): Promise<DebugLoggingState> {
    return this.request<DebugLoggingState>("/support/debug-logging", {
      method: "POST",
      body: JSON.stringify({ enabled }),
    });
  }

  async getLogs(params?: { limit?: number; level?: string; search?: string }): Promise<LogsResponse> {
    const searchParams = new URLSearchParams();
    if (params?.limit) searchParams.set("limit", String(params.limit));
    if (params?.level) searchParams.set("level", params.level);
    if (params?.search) searchParams.set("search", params.search);
    const query = searchParams.toString();
    return this.request<LogsResponse>(`/support/logs${query ? `?${query}` : ""}`);
  }

  async clearLogs(): Promise<{ message: string }> {
    return this.request<{ message: string }>("/support/logs", {
      method: "DELETE",
    });
  }

  async getSystemInfo(): Promise<SystemInfo> {
    return this.request<SystemInfo>("/support/system-info");
  }

  async downloadSupportBundle(): Promise<Blob> {
    const response = await fetch(`${API_BASE}/support/bundle`);
    if (!response.ok) {
      const error = await response.text();
      throw new Error(error || `HTTP ${response.status}`);
    }
    return response.blob();
  }
}

export const api = new ApiClient();
