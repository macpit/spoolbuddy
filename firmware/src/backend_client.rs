//! Backend Client with C-callable interface
//!
//! Provides HTTP polling to the SpoolBuddy backend server for printer status.
//! Uses mDNS to discover the server automatically.

use esp_idf_svc::http::client::{Configuration as HttpConfig, EspHttpConnection};
use log::{info, warn};
use serde::Deserialize;
use std::ffi::{c_char, c_int};
use std::sync::Mutex;
use embedded_svc::http::client::Client as HttpClient;

/// Maximum number of printers to cache (reduced for memory)
const MAX_PRINTERS: usize = 4;

/// Maximum number of AMS units per printer
const MAX_AMS_UNITS: usize = 4;

/// HTTP timeout in milliseconds
const HTTP_TIMEOUT_MS: u64 = 5000;

/// Backend connection state
#[derive(Debug, Clone, PartialEq)]
pub enum BackendState {
    Disconnected,
    Discovering,
    Connected { ip: [u8; 4], port: u16 },
    #[allow(dead_code)]
    Error(String),
}

/// AMS tray from backend API
#[derive(Debug, Clone, Deserialize, Default)]
struct ApiAmsTray {
    #[serde(rename = "ams_id")]
    _ams_id: i32,
    #[serde(rename = "tray_id")]
    _tray_id: i32,
    tray_type: Option<String>,
    tray_color: Option<String>,  // RGBA hex (e.g., "FF0000FF")
    remain: Option<i32>,         // 0-100 percentage, or negative if unknown
}

/// AMS unit from backend API
#[derive(Debug, Clone, Deserialize, Default)]
struct ApiAmsUnit {
    id: i32,
    humidity: Option<i32>,
    temperature: Option<f32>,
    extruder: Option<i32>,  // 0=right, 1=left
    trays: Vec<ApiAmsTray>,
}

/// Printer status from backend API
#[derive(Debug, Clone, Deserialize)]
struct ApiPrinter {
    serial: String,
    name: Option<String>,
    ip_address: Option<String>,
    access_code: Option<String>,
    connected: bool,
    gcode_state: Option<String>,
    print_progress: Option<u8>,
    subtask_name: Option<String>,
    mc_remaining_time: Option<u16>,
    cover_url: Option<String>,
    stg_cur: Option<i8>,           // Current stage number (-1 = idle)
    stg_cur_name: Option<String>,  // Human-readable stage name
    #[serde(default)]
    ams_units: Vec<ApiAmsUnit>,
    tray_now: Option<i32>,
    tray_now_left: Option<i32>,
    tray_now_right: Option<i32>,
    active_extruder: Option<i32>,  // 0=right, 1=left, None=unknown
}

/// Time response from backend API
#[derive(Debug, Clone, Deserialize)]
struct ApiTime {
    hour: u8,
    minute: u8,
}

/// Cached AMS tray info
#[derive(Debug, Clone, Copy, Default)]
struct CachedAmsTray {
    tray_type: [u8; 16],    // Material type
    tray_color: u32,        // RGBA packed (0xRRGGBBAA)
    remain: u8,             // 0-100 percentage
}

/// Cached AMS unit info
#[derive(Debug, Clone, Copy)]
struct CachedAmsUnit {
    id: i32,
    humidity: i32,          // -1 if not available
    temperature: i16,       // Celsius * 10, -1 if not available
    extruder: i8,           // -1 if not available, 0=right, 1=left
    tray_count: u8,
    trays: [CachedAmsTray; 4],
}

impl Default for CachedAmsUnit {
    fn default() -> Self {
        Self {
            id: 0,
            humidity: -1,
            temperature: -1,
            extruder: -1,
            tray_count: 0,
            trays: [CachedAmsTray::default(); 4],
        }
    }
}

/// Cached printer info (internal)
#[derive(Debug, Clone)]
struct CachedPrinter {
    name: [u8; 32],
    serial: [u8; 20],
    ip_address: [u8; 20],
    access_code: [u8; 16],
    connected: bool,
    gcode_state: [u8; 16],
    print_progress: u8,
    subtask_name: [u8; 64],
    remaining_time_min: u16,
    stg_cur: i8,            // Current stage number (-1 = idle)
    stg_cur_name: [u8; 48], // Human-readable stage name
    // AMS data
    ams_unit_count: u8,
    ams_units: [CachedAmsUnit; MAX_AMS_UNITS],
    tray_now: i32,          // -1 if not available
    tray_now_left: i32,     // -1 if not available
    tray_now_right: i32,    // -1 if not available
    active_extruder: i32,   // -1 if not available, 0=right, 1=left
}

impl Default for CachedPrinter {
    fn default() -> Self {
        Self {
            name: [0; 32],
            serial: [0; 20],
            ip_address: [0; 20],
            access_code: [0; 16],
            connected: false,
            gcode_state: [0; 16],
            print_progress: 0,
            subtask_name: [0; 64],
            remaining_time_min: 0,
            stg_cur: -1,
            stg_cur_name: [0; 48],
            ams_unit_count: 0,
            ams_units: [CachedAmsUnit::default(); MAX_AMS_UNITS],
            tray_now: -1,
            tray_now_left: -1,
            tray_now_right: -1,
            active_extruder: -1,
        }
    }
}

/// Backend manager state
struct BackendManager {
    state: BackendState,
    server_url: String,
    printers: [CachedPrinter; MAX_PRINTERS],
    printer_count: usize,
}

const EMPTY_AMS_TRAY: CachedAmsTray = CachedAmsTray {
    tray_type: [0; 16],
    tray_color: 0,
    remain: 0,
};

const EMPTY_AMS_UNIT: CachedAmsUnit = CachedAmsUnit {
    id: 0,
    humidity: -1,
    temperature: -1,
    extruder: -1,
    tray_count: 0,
    trays: [EMPTY_AMS_TRAY; 4],
};

const EMPTY_PRINTER: CachedPrinter = CachedPrinter {
    name: [0; 32],
    serial: [0; 20],
    ip_address: [0; 20],
    access_code: [0; 16],
    connected: false,
    gcode_state: [0; 16],
    print_progress: 0,
    subtask_name: [0; 64],
    stg_cur_name: [0; 48],
    remaining_time_min: 0,
    stg_cur: -1,
    ams_unit_count: 0,
    ams_units: [EMPTY_AMS_UNIT; MAX_AMS_UNITS],
    tray_now: -1,
    tray_now_left: -1,
    tray_now_right: -1,
    active_extruder: -1,
};

impl BackendManager {
    const fn new() -> Self {
        Self {
            state: BackendState::Disconnected,
            server_url: String::new(),
            printers: [EMPTY_PRINTER; MAX_PRINTERS],
            printer_count: 0,
        }
    }
}

// Global backend manager
static BACKEND_MANAGER: Mutex<BackendManager> = Mutex::new(BackendManager::new());

// Cover image storage (max 64KB for thumbnail)
const MAX_COVER_SIZE: usize = 65536;
static COVER_DATA: Mutex<Vec<u8>> = Mutex::new(Vec::new());
static COVER_VALID: std::sync::atomic::AtomicBool = std::sync::atomic::AtomicBool::new(false);
static LAST_COVER_URL: Mutex<String> = Mutex::new(String::new());

/// Initialize the backend client
pub fn init() {
    info!("Backend client initialized");
}

/// Set the backend server URL manually
pub fn set_server_url(url: &str) {
    let mut manager = BACKEND_MANAGER.lock().unwrap();
    manager.server_url = url.to_string();

    // Parse IP from URL for status
    if let Some(ip_str) = url.strip_prefix("http://") {
        if let Some(ip_port) = ip_str.split('/').next() {
            if let Some(ip_only) = ip_port.split(':').next() {
                if let Ok(ip) = ip_only.parse::<std::net::Ipv4Addr>() {
                    let octets = ip.octets();
                    let port = ip_port.split(':').nth(1)
                        .and_then(|p| p.parse().ok())
                        .unwrap_or(3000);
                    manager.state = BackendState::Connected { ip: octets, port };
                    info!("Backend server set to: {}", url);
                    return;
                }
            }
        }
    }

    manager.state = BackendState::Disconnected;
    warn!("Failed to parse server URL: {}", url);
}

/// Poll the backend server for printer status and time
/// Called from main loop every ~2 seconds
pub fn poll_backend() {
    let manager = BACKEND_MANAGER.lock().unwrap();

    // Check if we have a server URL
    if manager.server_url.is_empty() {
        return;
    }

    let base_url = manager.server_url.clone();
    drop(manager); // Release lock before HTTP calls

    // Send heartbeat to indicate display is connected
    send_heartbeat(&base_url);

    // Fetch printers
    let printers_url = format!("{}/api/printers", base_url);
    let mut cover_url_to_fetch: Option<String> = None;

    match fetch_printers(&printers_url) {
        Ok(printers) => {
            // Check if cover URL changed before updating cache
            cover_url_to_fetch = check_cover_url_changed(&printers, &base_url);

            let mut manager = BACKEND_MANAGER.lock().unwrap();
            update_printer_cache(&mut manager, &printers);
        }
        Err(e) => {
            warn!("Failed to fetch printers: {}", e);
        }
    }

    // Fetch cover image if URL changed (outside of lock)
    if let Some(url) = cover_url_to_fetch {
        fetch_cover_image(&url);
    }

    // Fetch time from backend
    fetch_and_set_time(&base_url);
}

/// Send heartbeat to backend to indicate display is connected
/// Also checks for pending commands (e.g., reboot)
fn send_heartbeat(base_url: &str) {
    use esp_idf_sys::esp_restart;

    let version = env!("CARGO_PKG_VERSION");
    let update_available = crate::ota_manager::is_update_available();
    let url = format!(
        "{}/api/display/heartbeat?version={}&update_available={}",
        base_url, version, update_available
    );

    let config = HttpConfig {
        timeout: Some(std::time::Duration::from_millis(2000)),
        ..Default::default()
    };

    let connection = match EspHttpConnection::new(&config) {
        Ok(c) => c,
        Err(_) => return,
    };

    let mut client = HttpClient::wrap(connection);

    let request = match client.get(&url) {
        Ok(r) => r,
        Err(_) => return,
    };

    let mut response = match request.submit() {
        Ok(r) => r,
        Err(_) => return,
    };

    // Read response to check for commands
    let mut buf = [0u8; 256];
    if let Ok(n) = response.read(&mut buf) {
        if n > 0 {
            let body = String::from_utf8_lossy(&buf[..n]);
            // Check for update command (triggers OTA)
            if body.contains("\"command\":\"update\"") || body.contains("\"command\": \"update\"") {
                log::info!("Received update command from backend - starting OTA");
                if let Err(e) = crate::ota_manager::perform_update(base_url) {
                    log::error!("OTA update failed: {}", e);
                }
                // perform_update reboots on success, so we only get here on failure
            }
            // Check for reboot command
            else if body.contains("\"command\":\"reboot\"") || body.contains("\"command\": \"reboot\"") {
                log::info!("Received reboot command from backend");
                std::thread::sleep(std::time::Duration::from_millis(500));
                unsafe { esp_restart(); }
            }
        }
    }
}

/// Send device state to backend (weight, tag) and receive decoded tag data
/// Returns true if tag data was received and set
pub fn send_device_state(tag_uid_hex: Option<&str>, weight: f32, stable: bool) -> bool {
    let manager = BACKEND_MANAGER.lock().unwrap();
    if manager.server_url.is_empty() {
        return false;
    }
    let base_url = manager.server_url.clone();
    drop(manager);

    // Build URL with query params
    let url = if let Some(tag_id) = tag_uid_hex {
        format!(
            "{}/api/display/state?weight={:.1}&stable={}&tag_id={}",
            base_url, weight, stable, tag_id
        )
    } else {
        format!(
            "{}/api/display/state?weight={:.1}&stable={}",
            base_url, weight, stable
        )
    };

    let config = HttpConfig {
        timeout: Some(std::time::Duration::from_millis(3000)),
        ..Default::default()
    };

    let connection = match EspHttpConnection::new(&config) {
        Ok(c) => c,
        Err(_) => return false,
    };

    let mut client = HttpClient::wrap(connection);

    // POST request
    let request = match client.post(&url, &[]) {
        Ok(r) => r,
        Err(_) => return false,
    };

    let response = match request.submit() {
        Ok(r) => r,
        Err(_) => return false,
    };

    if response.status() != 200 {
        return false;
    }

    // If we have a tag, fetch decoded data from display/status
    if tag_uid_hex.is_some() {
        fetch_decoded_tag_data(&base_url);
        return true;
    }

    false
}

/// Fetch decoded tag data from backend
fn fetch_decoded_tag_data(base_url: &str) {
    let url = format!("{}/api/display/status", base_url);

    let config = HttpConfig {
        timeout: Some(std::time::Duration::from_millis(3000)),
        ..Default::default()
    };

    let connection = match EspHttpConnection::new(&config) {
        Ok(c) => c,
        Err(_) => return,
    };

    let mut client = HttpClient::wrap(connection);

    let request = match client.get(&url) {
        Ok(r) => r,
        Err(_) => return,
    };

    let mut response = match request.submit() {
        Ok(r) => r,
        Err(_) => return,
    };

    if response.status() != 200 {
        return;
    }

    // Read response
    let mut body = Vec::new();
    let mut buf = [0u8; 512];
    loop {
        match response.read(&mut buf) {
            Ok(0) => break,
            Ok(n) => body.extend_from_slice(&buf[..n]),
            Err(_) => return,
        }
    }

    // Parse JSON and extract tag_data
    #[derive(Deserialize)]
    struct DisplayStatus {
        tag_data: Option<TagData>,
    }

    #[derive(Deserialize)]
    struct TagData {
        vendor: Option<String>,
        material: Option<String>,
        subtype: Option<String>,
        color_name: Option<String>,
        color_rgba: Option<u32>,
        spool_weight: Option<i32>,
        tag_type: Option<String>,
    }

    if let Ok(status) = serde_json::from_slice::<DisplayStatus>(&body) {
        if let Some(tag_data) = status.tag_data {
            crate::nfc_bridge_manager::set_decoded_tag_data(
                tag_data.vendor.as_deref().unwrap_or(""),
                tag_data.material.as_deref().unwrap_or(""),
                tag_data.subtype.as_deref().unwrap_or(""),
                tag_data.color_name.as_deref().unwrap_or(""),
                tag_data.color_rgba.unwrap_or(0),
                tag_data.spool_weight.unwrap_or(0),
                tag_data.tag_type.as_deref().unwrap_or(""),
            );
            info!("Received decoded tag data from backend");
        }
    }
}

/// Fetch time from backend and update time manager
/// Can be called independently for quick time sync
pub fn fetch_and_set_time(base_url: &str) {
    let time_url = format!("{}/api/time", base_url);
    match fetch_time(&time_url) {
        Ok(time) => {
            crate::time_manager::set_backend_time(time.hour, time.minute);
        }
        Err(_) => {
            // Silently ignore time fetch errors
        }
    }
}

/// Quick time sync - call after setting server URL
pub fn sync_time() {
    let manager = BACKEND_MANAGER.lock().unwrap();
    if manager.server_url.is_empty() {
        return;
    }
    let base_url = manager.server_url.clone();
    drop(manager);
    fetch_and_set_time(&base_url);
}

/// Fetch printers from backend API
fn fetch_printers(url: &str) -> Result<Vec<ApiPrinter>, String> {
    // Create HTTP client
    let config = HttpConfig {
        timeout: Some(std::time::Duration::from_millis(HTTP_TIMEOUT_MS)),
        ..Default::default()
    };

    let connection = EspHttpConnection::new(&config)
        .map_err(|e| format!("HTTP connection failed: {:?}", e))?;

    let mut client = HttpClient::wrap(connection);

    // Make GET request
    let request = client.get(url)
        .map_err(|e| format!("GET request failed: {:?}", e))?;

    let mut response = request.submit()
        .map_err(|e| format!("Request submit failed: {:?}", e))?;

    // Check status
    let status = response.status();
    if status != 200 {
        return Err(format!("HTTP error: {}", status));
    }

    // Read response body
    let mut body = Vec::new();
    let mut buf = [0u8; 512];
    loop {
        match response.read(&mut buf) {
            Ok(0) => break,
            Ok(n) => body.extend_from_slice(&buf[..n]),
            Err(e) => return Err(format!("Read error: {:?}", e)),
        }
    }

    // Parse JSON
    let printers: Vec<ApiPrinter> = serde_json::from_slice(&body)
        .map_err(|e| format!("JSON parse error: {:?}", e))?;

    Ok(printers)
}

/// Fetch time from backend API
fn fetch_time(url: &str) -> Result<ApiTime, String> {
    let config = HttpConfig {
        timeout: Some(std::time::Duration::from_millis(2000)), // Short timeout for time
        ..Default::default()
    };

    let connection = EspHttpConnection::new(&config)
        .map_err(|e| format!("HTTP connection failed: {:?}", e))?;

    let mut client = HttpClient::wrap(connection);

    let request = client.get(url)
        .map_err(|e| format!("GET request failed: {:?}", e))?;

    let mut response = request.submit()
        .map_err(|e| format!("Request submit failed: {:?}", e))?;

    let status = response.status();
    if status != 200 {
        return Err(format!("HTTP error: {}", status));
    }

    let mut body = Vec::new();
    let mut buf = [0u8; 128];
    loop {
        match response.read(&mut buf) {
            Ok(0) => break,
            Ok(n) => body.extend_from_slice(&buf[..n]),
            Err(e) => return Err(format!("Read error: {:?}", e)),
        }
    }

    let time: ApiTime = serde_json::from_slice(&body)
        .map_err(|e| format!("JSON parse error: {:?}", e))?;

    Ok(time)
}

/// Update the cached printer data
/// Parse RGBA hex string to u32 (e.g., "FF0000FF" -> 0xFF0000FF)
fn parse_rgba_color(color: &str) -> u32 {
    u32::from_str_radix(color, 16).unwrap_or(0)
}

fn update_printer_cache(manager: &mut BackendManager, printers: &[ApiPrinter]) {
    manager.printer_count = printers.len().min(MAX_PRINTERS);

    info!("Updating printer cache with {} printers", printers.len());

    for (i, printer) in printers.iter().take(MAX_PRINTERS).enumerate() {
        let cached = &mut manager.printers[i];

        info!("Printer {}: serial={}, name={:?}, connected={}",
              i, printer.serial, printer.name, printer.connected);

        // Copy name
        cached.name = [0; 32];
        if let Some(ref name) = printer.name {
            let bytes = name.as_bytes();
            let len = bytes.len().min(31);
            cached.name[..len].copy_from_slice(&bytes[..len]);
        }

        // Copy serial
        cached.serial = [0; 20];
        let serial_bytes = printer.serial.as_bytes();
        let serial_len = serial_bytes.len().min(19);
        cached.serial[..serial_len].copy_from_slice(&serial_bytes[..serial_len]);

        // Copy IP address
        cached.ip_address = [0; 20];
        if let Some(ref ip) = printer.ip_address {
            let bytes = ip.as_bytes();
            let len = bytes.len().min(19);
            cached.ip_address[..len].copy_from_slice(&bytes[..len]);
        }

        // Copy access code
        cached.access_code = [0; 16];
        if let Some(ref code) = printer.access_code {
            let bytes = code.as_bytes();
            let len = bytes.len().min(15);
            cached.access_code[..len].copy_from_slice(&bytes[..len]);
        }

        // Copy state
        cached.connected = printer.connected;
        cached.gcode_state = [0; 16];
        cached.print_progress = 0;
        cached.subtask_name = [0; 64];
        cached.remaining_time_min = 0;

        if let Some(ref gcode) = printer.gcode_state {
            let bytes = gcode.as_bytes();
            let len = bytes.len().min(15);
            cached.gcode_state[..len].copy_from_slice(&bytes[..len]);
        }
        if let Some(progress) = printer.print_progress {
            cached.print_progress = progress;
        }
        if let Some(ref subtask) = printer.subtask_name {
            let bytes = subtask.as_bytes();
            let len = bytes.len().min(63);
            cached.subtask_name[..len].copy_from_slice(&bytes[..len]);
        }
        if let Some(time) = printer.mc_remaining_time {
            cached.remaining_time_min = time;
        }

        // Copy stage info
        cached.stg_cur = printer.stg_cur.unwrap_or(-1);
        cached.stg_cur_name = [0; 48];
        if let Some(ref stg_name) = printer.stg_cur_name {
            let bytes = stg_name.as_bytes();
            let len = bytes.len().min(47);
            cached.stg_cur_name[..len].copy_from_slice(&bytes[..len]);
        }

        // Copy active tray info
        cached.tray_now = printer.tray_now.unwrap_or(-1);
        cached.tray_now_left = printer.tray_now_left.unwrap_or(-1);
        cached.tray_now_right = printer.tray_now_right.unwrap_or(-1);
        cached.active_extruder = printer.active_extruder.unwrap_or(-1);

        // Copy AMS units
        cached.ams_unit_count = printer.ams_units.len().min(MAX_AMS_UNITS) as u8;
        cached.ams_units = [EMPTY_AMS_UNIT; MAX_AMS_UNITS];

        info!("Printer {} has {} AMS units, tray_now={}, active_extruder={}",
              i, printer.ams_units.len(), cached.tray_now, cached.active_extruder);

        for (j, ams) in printer.ams_units.iter().take(MAX_AMS_UNITS).enumerate() {
            let cached_ams = &mut cached.ams_units[j];
            cached_ams.id = ams.id;
            cached_ams.humidity = ams.humidity.unwrap_or(-1);
            cached_ams.temperature = ams.temperature.map(|t| (t * 10.0) as i16).unwrap_or(-1);
            cached_ams.extruder = ams.extruder.map(|e| e as i8).unwrap_or(-1);
            cached_ams.tray_count = ams.trays.len().min(4) as u8;

            info!("  AMS[{}] id={} extruder={:?} trays={}", j, ams.id, ams.extruder, ams.trays.len());

            for (k, tray) in ams.trays.iter().take(4).enumerate() {
                let cached_tray = &mut cached_ams.trays[k];

                // Copy tray type
                cached_tray.tray_type = [0; 16];
                if let Some(ref tray_type) = tray.tray_type {
                    let bytes = tray_type.as_bytes();
                    let len = bytes.len().min(15);
                    cached_tray.tray_type[..len].copy_from_slice(&bytes[..len]);
                }

                // Parse color
                cached_tray.tray_color = tray.tray_color
                    .as_ref()
                    .map(|c| parse_rgba_color(c))
                    .unwrap_or(0);

                // Remaining percentage (clamp negative to 0)
                cached_tray.remain = tray.remain.unwrap_or(0).max(0) as u8;
            }
        }
    }

}

/// Check if cover URL changed and return the new URL if so
fn check_cover_url_changed(printers: &[ApiPrinter], base_url: &str) -> Option<String> {
    if let Some(printer) = printers.first() {
        if let Some(ref cover_url) = printer.cover_url {
            let mut last_url = LAST_COVER_URL.lock().unwrap();
            if *last_url != *cover_url {
                // Cover URL changed
                *last_url = cover_url.clone();
                return Some(format!("{}{}", base_url, cover_url));
            }
        } else {
            // No cover URL, invalidate cover
            COVER_VALID.store(false, std::sync::atomic::Ordering::Relaxed);
            let mut last_url = LAST_COVER_URL.lock().unwrap();
            last_url.clear();
        }
    }
    None
}

/// Fetch cover image from URL
fn fetch_cover_image(url: &str) {
    info!("Fetching cover image from: {}", url);

    let config = HttpConfig {
        timeout: Some(std::time::Duration::from_millis(10000)), // 10s timeout for image
        ..Default::default()
    };

    let connection = match EspHttpConnection::new(&config) {
        Ok(c) => c,
        Err(e) => {
            warn!("Cover fetch connection failed: {:?}", e);
            COVER_VALID.store(false, std::sync::atomic::Ordering::Relaxed);
            return;
        }
    };

    let mut client = HttpClient::wrap(connection);

    let request = match client.get(url) {
        Ok(r) => r,
        Err(e) => {
            warn!("Cover fetch request failed: {:?}", e);
            COVER_VALID.store(false, std::sync::atomic::Ordering::Relaxed);
            return;
        }
    };

    let mut response = match request.submit() {
        Ok(r) => r,
        Err(e) => {
            warn!("Cover fetch submit failed: {:?}", e);
            COVER_VALID.store(false, std::sync::atomic::Ordering::Relaxed);
            return;
        }
    };

    if response.status() != 200 {
        warn!("Cover fetch HTTP error: {}", response.status());
        COVER_VALID.store(false, std::sync::atomic::Ordering::Relaxed);
        return;
    }

    // Read image data
    let mut data = Vec::new();
    let mut buf = [0u8; 1024];
    loop {
        match response.read(&mut buf) {
            Ok(0) => break,
            Ok(n) => {
                if data.len() + n > MAX_COVER_SIZE {
                    warn!("Cover image too large, truncating");
                    break;
                }
                data.extend_from_slice(&buf[..n]);
            }
            Err(e) => {
                warn!("Cover read error: {:?}", e);
                COVER_VALID.store(false, std::sync::atomic::Ordering::Relaxed);
                return;
            }
        }
    }

    info!("Downloaded cover image: {} bytes", data.len());

    // Store cover data
    let mut cover = COVER_DATA.lock().unwrap();
    *cover = data;
    COVER_VALID.store(true, std::sync::atomic::Ordering::Relaxed);
}

// ============================================================================
// C-callable interface
// ============================================================================

/// Backend status for C interface
#[repr(C)]
pub struct BackendStatus {
    /// 0=Disconnected, 1=Discovering, 2=Connected, 3=Error
    pub state: c_int,
    /// Server IP address (valid when state=2)
    pub server_ip: [u8; 4],
    /// Server port (valid when state=2)
    pub server_port: u16,
    /// Number of printers cached
    pub printer_count: u8,
}

/// Printer info for C interface
#[repr(C)]
pub struct PrinterInfo {
    pub name: [c_char; 32],           // 32 bytes
    pub serial: [c_char; 20],         // 20 bytes
    pub ip_address: [c_char; 20],     // 20 bytes - for settings sync
    pub access_code: [c_char; 16],    // 16 bytes - for settings sync
    pub gcode_state: [c_char; 16],    // 16 bytes
    pub subtask_name: [c_char; 64],   // 64 bytes
    pub stg_cur_name: [c_char; 48],   // 48 bytes - detailed stage name
    pub remaining_time_min: u16,      // 2 bytes
    pub print_progress: u8,           // 1 byte
    pub stg_cur: i8,                  // 1 byte - stage number (-1 = idle)
    pub connected: bool,              // 1 byte
    pub _pad: [u8; 3],                // 3 bytes padding for alignment
}

/// Get backend connection status
#[no_mangle]
pub extern "C" fn backend_get_status(status: *mut BackendStatus) {
    if status.is_null() {
        return;
    }

    let manager = BACKEND_MANAGER.lock().unwrap();

    unsafe {
        match &manager.state {
            BackendState::Disconnected => {
                (*status).state = 0;
                (*status).server_ip = [0; 4];
                (*status).server_port = 0;
            }
            BackendState::Discovering => {
                (*status).state = 1;
                (*status).server_ip = [0; 4];
                (*status).server_port = 0;
            }
            BackendState::Connected { ip, port } => {
                (*status).state = 2;
                (*status).server_ip = *ip;
                (*status).server_port = *port;
            }
            BackendState::Error(_) => {
                (*status).state = 3;
                (*status).server_ip = [0; 4];
                (*status).server_port = 0;
            }
        }
        (*status).printer_count = manager.printer_count as u8;
    }
}

/// Get printer info by index
/// Returns 0 on success, -1 if index out of range
#[no_mangle]
pub extern "C" fn backend_get_printer(index: c_int, info: *mut PrinterInfo) -> c_int {
    if info.is_null() || index < 0 {
        return -1;
    }

    let manager = BACKEND_MANAGER.lock().unwrap();
    let idx = index as usize;

    if idx >= manager.printer_count {
        return -1;
    }

    let cached = &manager.printers[idx];

    unsafe {
        // Copy name (already null-terminated due to zero init)
        for (i, &b) in cached.name.iter().enumerate() {
            (*info).name[i] = b as c_char;
        }

        // Copy serial
        for (i, &b) in cached.serial.iter().enumerate() {
            (*info).serial[i] = b as c_char;
        }

        // Copy IP address
        for (i, &b) in cached.ip_address.iter().enumerate() {
            (*info).ip_address[i] = b as c_char;
        }

        // Copy access code
        for (i, &b) in cached.access_code.iter().enumerate() {
            (*info).access_code[i] = b as c_char;
        }

        // Copy state
        for (i, &b) in cached.gcode_state.iter().enumerate() {
            (*info).gcode_state[i] = b as c_char;
        }

        // Copy subtask name
        for (i, &b) in cached.subtask_name.iter().enumerate() {
            (*info).subtask_name[i] = b as c_char;
        }

        (*info).connected = cached.connected;
        (*info).print_progress = cached.print_progress;
        (*info).remaining_time_min = cached.remaining_time_min;

        // Copy stage info
        (*info).stg_cur = cached.stg_cur;
        for (i, &b) in cached.stg_cur_name.iter().enumerate() {
            (*info).stg_cur_name[i] = b as c_char;
        }

        // Initialize padding
        (*info)._pad = [0; 3];
    }

    0
}

/// Set backend server URL from C
/// Returns 0 on success, -1 on error
#[no_mangle]
pub extern "C" fn backend_set_url(url: *const c_char) -> c_int {
    if url.is_null() {
        return -1;
    }

    let url_str = unsafe {
        match std::ffi::CStr::from_ptr(url).to_str() {
            Ok(s) => s,
            Err(_) => return -1,
        }
    };

    set_server_url(url_str);
    0
}

/// Trigger mDNS discovery for backend server
/// Returns 0 if discovery started, -1 on error
#[no_mangle]
pub extern "C" fn backend_discover_server() -> c_int {
    // TODO: Implement mDNS browsing for _spoolbuddy-server._tcp
    // For now, this is a placeholder
    info!("Backend server discovery requested (not yet implemented)");

    let mut manager = BACKEND_MANAGER.lock().unwrap();
    manager.state = BackendState::Discovering;

    0
}

/// Check if backend is connected
/// Returns 1 if connected, 0 otherwise
#[no_mangle]
pub extern "C" fn backend_is_connected() -> c_int {
    let manager = BACKEND_MANAGER.lock().unwrap();
    match manager.state {
        BackendState::Connected { .. } => 1,
        _ => 0,
    }
}

/// Get number of cached printers
#[no_mangle]
pub extern "C" fn backend_get_printer_count() -> c_int {
    let manager = BACKEND_MANAGER.lock().unwrap();
    manager.printer_count as c_int
}

/// Check if cover image is available
/// Returns 1 if valid cover exists, 0 otherwise
#[no_mangle]
pub extern "C" fn backend_has_cover() -> c_int {
    if COVER_VALID.load(std::sync::atomic::Ordering::Relaxed) {
        1
    } else {
        0
    }
}

/// Get cover image data
/// Returns pointer to PNG data, or null if not available
/// Size is written to size_out if provided
#[no_mangle]
pub extern "C" fn backend_get_cover_data(size_out: *mut u32) -> *const u8 {
    if !COVER_VALID.load(std::sync::atomic::Ordering::Relaxed) {
        if !size_out.is_null() {
            unsafe { *size_out = 0; }
        }
        return std::ptr::null();
    }

    let cover = COVER_DATA.lock().unwrap();
    if cover.is_empty() {
        if !size_out.is_null() {
            unsafe { *size_out = 0; }
        }
        return std::ptr::null();
    }

    if !size_out.is_null() {
        unsafe { *size_out = cover.len() as u32; }
    }

    cover.as_ptr()
}

// ============================================================================
// AMS FFI functions
// ============================================================================

/// AMS tray info for C interface
#[repr(C)]
pub struct AmsTrayCInfo {
    pub tray_type: [c_char; 16],  // Material type
    pub tray_color: u32,          // RGBA packed
    pub remain: u8,               // 0-100 percentage
}

/// AMS unit info for C interface
#[repr(C)]
pub struct AmsUnitCInfo {
    pub id: c_int,
    pub humidity: c_int,          // -1 if not available
    pub temperature: i16,         // Celsius * 10, -1 if not available
    pub extruder: i8,             // -1 if not available, 0=right, 1=left
    pub tray_count: u8,
    pub trays: [AmsTrayCInfo; 4],
}

/// Get number of AMS units for a printer
#[no_mangle]
pub extern "C" fn backend_get_ams_count(printer_index: c_int) -> c_int {
    let manager = BACKEND_MANAGER.lock().unwrap();
    if printer_index < 0 || printer_index as usize >= manager.printer_count {
        return 0;
    }
    let count = manager.printers[printer_index as usize].ams_unit_count as c_int;
    info!("backend_get_ams_count({}) = {}", printer_index, count);
    count
}

/// Get AMS unit info
/// Returns 0 on success, -1 on error
#[no_mangle]
pub extern "C" fn backend_get_ams_unit(
    printer_index: c_int,
    ams_index: c_int,
    info: *mut AmsUnitCInfo,
) -> c_int {
    if info.is_null() {
        return -1;
    }

    let manager = BACKEND_MANAGER.lock().unwrap();
    if printer_index < 0 || printer_index as usize >= manager.printer_count {
        return -1;
    }

    let printer = &manager.printers[printer_index as usize];
    if ams_index < 0 || ams_index as usize >= printer.ams_unit_count as usize {
        return -1;
    }

    let ams = &printer.ams_units[ams_index as usize];

    unsafe {
        let out = &mut *info;
        out.id = ams.id;
        out.humidity = ams.humidity;
        out.temperature = ams.temperature;
        out.extruder = ams.extruder;
        out.tray_count = ams.tray_count;

        for (i, tray) in ams.trays.iter().enumerate() {
            out.trays[i].tray_type = [0; 16];
            for (j, &byte) in tray.tray_type.iter().enumerate() {
                out.trays[i].tray_type[j] = byte as c_char;
            }
            out.trays[i].tray_color = tray.tray_color;
            out.trays[i].remain = tray.remain;
        }
    }

    0
}

/// Get active tray for single-nozzle printer
/// Returns -1 if not available
#[no_mangle]
pub extern "C" fn backend_get_tray_now(printer_index: c_int) -> c_int {
    let manager = BACKEND_MANAGER.lock().unwrap();
    if printer_index < 0 || printer_index as usize >= manager.printer_count {
        return -1;
    }
    manager.printers[printer_index as usize].tray_now
}

/// Get active tray for left nozzle (dual-nozzle printer)
/// Returns -1 if not available
#[no_mangle]
pub extern "C" fn backend_get_tray_now_left(printer_index: c_int) -> c_int {
    let manager = BACKEND_MANAGER.lock().unwrap();
    if printer_index < 0 || printer_index as usize >= manager.printer_count {
        return -1;
    }
    manager.printers[printer_index as usize].tray_now_left
}

/// Get active tray for right nozzle (dual-nozzle printer)
/// Returns -1 if not available
#[no_mangle]
pub extern "C" fn backend_get_tray_now_right(printer_index: c_int) -> c_int {
    let manager = BACKEND_MANAGER.lock().unwrap();
    if printer_index < 0 || printer_index as usize >= manager.printer_count {
        return -1;
    }
    manager.printers[printer_index as usize].tray_now_right
}

/// Get currently active extruder (dual-nozzle printer)
/// Returns -1 if not available, 0=right, 1=left
#[no_mangle]
pub extern "C" fn backend_get_active_extruder(printer_index: c_int) -> c_int {
    let manager = BACKEND_MANAGER.lock().unwrap();
    if printer_index < 0 || printer_index as usize >= manager.printer_count {
        return -1;
    }
    manager.printers[printer_index as usize].active_extruder
}

/// Check if firmware update is available
/// Returns 1 if update available, 0 otherwise
#[no_mangle]
pub extern "C" fn ota_is_update_available() -> c_int {
    if crate::ota_manager::is_update_available() {
        1
    } else {
        0
    }
}

/// Get current firmware version
/// Copies version string to buffer, returns length or -1 on error
#[no_mangle]
pub extern "C" fn ota_get_current_version(buf: *mut c_char, buf_len: c_int) -> c_int {
    if buf.is_null() || buf_len <= 0 {
        return -1;
    }
    let version = crate::ota_manager::get_version();
    let bytes = version.as_bytes();
    let copy_len = std::cmp::min(bytes.len(), (buf_len - 1) as usize);
    unsafe {
        std::ptr::copy_nonoverlapping(bytes.as_ptr(), buf as *mut u8, copy_len);
        *buf.add(copy_len) = 0; // Null terminate
    }
    copy_len as c_int
}

/// Get available update version
/// Copies version string to buffer, returns length or -1 on error
#[no_mangle]
pub extern "C" fn ota_get_update_version(buf: *mut c_char, buf_len: c_int) -> c_int {
    if buf.is_null() || buf_len <= 0 {
        return -1;
    }
    let version = crate::ota_manager::get_update_version();
    let bytes = version.as_bytes();
    let copy_len = std::cmp::min(bytes.len(), (buf_len - 1) as usize);
    unsafe {
        std::ptr::copy_nonoverlapping(bytes.as_ptr(), buf as *mut u8, copy_len);
        *buf.add(copy_len) = 0; // Null terminate
    }
    copy_len as c_int
}

/// Get OTA state
/// Returns: 0=Idle, 1=Checking, 2=Downloading, 3=Validating, 4=Flashing, 5=Complete, 6=Error
#[no_mangle]
pub extern "C" fn ota_get_state() -> c_int {
    match crate::ota_manager::get_state() {
        crate::ota_manager::OtaState::Idle => 0,
        crate::ota_manager::OtaState::Checking => 1,
        crate::ota_manager::OtaState::Downloading { .. } => 2,
        crate::ota_manager::OtaState::Validating => 3,
        crate::ota_manager::OtaState::Flashing { .. } => 4,
        crate::ota_manager::OtaState::Complete => 5,
        crate::ota_manager::OtaState::Error(_) => 6,
    }
}

/// Get OTA download/flash progress (0-100)
/// Returns -1 if not in a progress state
#[no_mangle]
pub extern "C" fn ota_get_progress() -> c_int {
    match crate::ota_manager::get_state() {
        crate::ota_manager::OtaState::Downloading { progress } => progress as c_int,
        crate::ota_manager::OtaState::Flashing { progress } => progress as c_int,
        _ => -1,
    }
}

/// Trigger OTA update check (non-blocking, spawns thread)
/// Returns 0 on success, -1 on error
#[no_mangle]
pub extern "C" fn ota_check_for_update() -> c_int {
    // Get backend URL
    let manager = BACKEND_MANAGER.lock().unwrap();
    let url = match &manager.state {
        BackendState::Connected { ip, port } => {
            format!("http://{}.{}.{}.{}:{}", ip[0], ip[1], ip[2], ip[3], port)
        }
        _ => return -1, // Not connected
    };
    drop(manager);

    // Spawn thread with larger stack (HTTP client needs more stack space)
    std::thread::Builder::new()
        .name("ota_check".into())
        .stack_size(8192)  // 8KB stack
        .spawn(move || {
            match crate::ota_manager::check_for_update(&url) {
                Ok(info) => {
                    crate::ota_manager::set_update_available(info.available, &info.version);
                    info!("OTA check complete: available={}, version={}", info.available, info.version);
                }
                Err(e) => {
                    warn!("OTA check failed: {}", e);
                }
            }
        })
        .ok();
    0
}

/// Trigger OTA update (non-blocking, spawns thread)
/// Returns 0 on success, -1 on error
#[no_mangle]
pub extern "C" fn ota_start_update() -> c_int {
    // Get backend URL
    let manager = BACKEND_MANAGER.lock().unwrap();
    let url = match &manager.state {
        BackendState::Connected { ip, port } => {
            format!("http://{}.{}.{}.{}:{}", ip[0], ip[1], ip[2], ip[3], port)
        }
        _ => return -1, // Not connected
    };
    drop(manager);

    // Spawn thread to perform update
    std::thread::spawn(move || {
        if let Err(e) = crate::ota_manager::perform_update(&url) {
            log::error!("OTA update failed: {}", e);
        }
        // Note: perform_update reboots on success, so we only get here on error
    });
    0
}
