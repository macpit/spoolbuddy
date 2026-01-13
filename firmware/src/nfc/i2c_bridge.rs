//! NFC I2C Bridge Driver
//!
//! Communicates with the Pico NFC bridge over I2C.
//! The Pico handles PN5180 SPI communication and exposes a simple I2C interface.
//!
//! I2C Protocol:
//! - Address: 0x55
//! - Commands:
//!   - 0x00: Get status (returns 2 bytes: status, tag_present)
//!   - 0x01: Get version (returns 3 bytes: status, major, minor)
//!   - 0x10: Scan tag (returns: status, uid_len, uid[0..uid_len])
//!   - 0x20: Read tag data (returns: status, tag_type, uid_len, uid, block_data...)

use esp_idf_hal::i2c::I2cDriver;
use log::{info, warn, debug};
use std::sync::atomic::{AtomicU8, Ordering};

/// I2C address of the Pico NFC bridge
pub const PICO_NFC_ADDR: u8 = 0x55;

/// Sequence counter for log correlation
static CMD_SEQ: AtomicU8 = AtomicU8::new(0);

fn next_seq() -> u8 {
    CMD_SEQ.fetch_add(1, Ordering::Relaxed)
}

/// Commands
#[allow(dead_code)]
const CMD_GET_STATUS: u8 = 0x00;
const CMD_GET_VERSION: u8 = 0x01;
const CMD_SCAN_TAG: u8 = 0x10;
const CMD_READ_TAG_DATA: u8 = 0x20;

/// Tag types (matches Pico definitions)
pub const TAG_TYPE_UNKNOWN: u8 = 0;
pub const TAG_TYPE_NTAG: u8 = 1;
pub const TAG_TYPE_MIFARE_1K: u8 = 2;
pub const TAG_TYPE_MIFARE_4K: u8 = 3;

/// Decoded tag data from Bambu/NTAG tags
#[derive(Debug, Clone, Default)]
pub struct DecodedTagInfo {
    pub vendor: String,
    pub material: String,
    pub material_subtype: String,
    pub color_name: String,
    pub color_rgba: u32,
    pub spool_weight: i32,
    pub tag_type_name: String,
}

/// NFC Bridge state
#[derive(Debug, Clone)]
pub struct NfcBridgeState {
    pub initialized: bool,
    pub firmware_version: (u8, u8),  // major, minor
    pub tag_present: bool,
    pub tag_uid: [u8; 10],
    pub tag_uid_len: u8,
    pub tag_type: u8,
    pub decoded_info: Option<DecodedTagInfo>,
}

impl NfcBridgeState {
    pub fn new() -> Self {
        Self {
            initialized: false,
            firmware_version: (0, 0),
            tag_present: false,
            tag_uid: [0; 10],
            tag_uid_len: 0,
            tag_type: TAG_TYPE_UNKNOWN,
            decoded_info: None,
        }
    }
}

/// Initialize the NFC I2C bridge
pub fn init_bridge(i2c: &mut I2cDriver<'_>, state: &mut NfcBridgeState) -> Result<(), &'static str> {
    info!("=== NFC I2C BRIDGE INIT ===");
    info!("  Pico address: 0x{:02X}", PICO_NFC_ADDR);

    // Check if Pico is present
    let mut buf = [0u8; 1];
    if i2c.read(PICO_NFC_ADDR, &mut buf, 100).is_err() {
        warn!("  Pico NFC bridge not found at 0x{:02X}", PICO_NFC_ADDR);
        return Err("Pico not found");
    }
    info!("  Pico NFC bridge detected");

    // Get version
    match get_version(i2c) {
        Ok((major, minor)) => {
            info!("  Pico firmware: {}.{}", major, minor);
            state.firmware_version = (major, minor);
        }
        Err(e) => {
            warn!("  Failed to get version: {}", e);
        }
    }

    state.initialized = true;
    info!("=== NFC I2C BRIDGE READY ===");
    Ok(())
}

/// Get Pico firmware version
pub fn get_version(i2c: &mut I2cDriver<'_>) -> Result<(u8, u8), &'static str> {
    // Send command
    let cmd = [CMD_GET_VERSION];
    if i2c.write(PICO_NFC_ADDR, &cmd, 100).is_err() {
        return Err("I2C write failed");
    }

    // Small delay for Pico to process
    std::thread::sleep(std::time::Duration::from_millis(10));

    // Read response: [status, major, minor]
    let mut resp = [0u8; 3];
    if i2c.read(PICO_NFC_ADDR, &mut resp, 100).is_err() {
        return Err("I2C read failed");
    }

    if resp[0] != 0 {
        return Err("Command failed");
    }

    Ok((resp[1], resp[2]))
}

/// Scan for a tag
pub fn scan_tag(i2c: &mut I2cDriver<'_>, state: &mut NfcBridgeState) -> Result<bool, &'static str> {
    let seq = next_seq();

    // Send scan command with sequence number
    info!("[#{}] TX: SCAN_TAG", seq);
    let cmd = [CMD_SCAN_TAG, seq];
    if i2c.write(PICO_NFC_ADDR, &cmd, 100).is_err() {
        warn!("[#{}] I2C write failed", seq);
        return Err("I2C write failed");
    }

    // Wait for scan to complete (Pico needs time to do RF communication)
    // Hard reset can take 300-500ms, so wait longer
    std::thread::sleep(std::time::Duration::from_millis(500));
    info!("[#{}] RX: reading response", seq);

    // Read response: [status, uid_len, uid...]
    let mut resp = [0u8; 12];  // Max: status + len + 10 UID bytes
    if i2c.read(PICO_NFC_ADDR, &mut resp, 100).is_err() {
        warn!("[#{}] I2C read failed", seq);
        return Err("I2C read failed");
    }

    if resp[0] != 0 {
        // No tag or error
        info!("[#{}] No tag (status={})", seq, resp[0]);
        state.tag_present = false;
        state.tag_uid_len = 0;
        state.decoded_info = None;
        return Ok(false);
    }

    let uid_len = resp[1];
    if uid_len > 0 && uid_len <= 10 {
        state.tag_present = true;
        state.tag_uid_len = uid_len;
        state.tag_uid[..uid_len as usize].copy_from_slice(&resp[2..2 + uid_len as usize]);

        info!("[#{}] Tag found: {:02X?}", seq, &state.tag_uid[..uid_len as usize]);
        Ok(true)
    } else {
        info!("[#{}] Invalid UID len: {}", seq, uid_len);
        state.tag_present = false;
        state.tag_uid_len = 0;
        state.decoded_info = None;
        Ok(false)
    }
}

/// Read and decode tag data
pub fn read_tag_data(i2c: &mut I2cDriver<'_>, state: &mut NfcBridgeState) -> Result<bool, &'static str> {
    if !state.tag_present {
        return Ok(false);
    }

    let seq = next_seq();

    // Send read tag data command with sequence number
    info!("[#{}] TX: READ_TAG_DATA", seq);
    let cmd = [CMD_READ_TAG_DATA, seq];
    if i2c.write(PICO_NFC_ADDR, &cmd, 100).is_err() {
        warn!("[#{}] I2C write failed", seq);
        return Err("I2C write failed");
    }

    // Wait for Pico to read tag data (authentication + block reads take time)
    info!("[#{}] waiting 1000ms for auth+read", seq);
    std::thread::sleep(std::time::Duration::from_millis(1000));
    info!("[#{}] RX: reading response", seq);

    // Read response - up to 200 bytes for tag data
    // Response format:
    // [0] = status (0 = success, 1 = no tag, 2 = read error, 3 = unknown type)
    // [1] = tag_type
    // [2] = uid_len
    // [3..3+uid_len] = uid
    // For MIFARE: blocks 1, 2, 4, 5 (64 bytes)
    // For NTAG: pages 4-20 (68 bytes)
    let mut resp = [0u8; 100];
    if i2c.read(PICO_NFC_ADDR, &mut resp, 100).is_err() {
        warn!("[#{}] I2C read failed", seq);
        return Err("I2C read failed");
    }

    let status = resp[0];
    if status != 0 {
        warn!("[#{}] Read failed, status: {}", seq, status);
        return Ok(false);
    }

    let tag_type = resp[1];
    let uid_len = resp[2] as usize;
    state.tag_type = tag_type;

    info!("[#{}] Success! type={}, uid_len={}", seq, tag_type, uid_len);

    // Decode based on tag type
    let data_offset = 3 + uid_len;

    if tag_type == TAG_TYPE_MIFARE_1K || tag_type == TAG_TYPE_MIFARE_4K {
        // Bambu Lab tag - decode blocks 1, 2, 4, 5
        let decoded = decode_bambu_tag(&resp[data_offset..]);
        state.decoded_info = Some(decoded);
        Ok(true)
    } else if tag_type == TAG_TYPE_NTAG {
        // NTAG - could be SpoolEase or OpenPrintTag
        // For now just mark as NTAG, full NDEF decoding would be more complex
        state.decoded_info = Some(DecodedTagInfo {
            tag_type_name: "NTAG".to_string(),
            ..Default::default()
        });
        Ok(true)
    } else {
        state.decoded_info = None;
        Ok(false)
    }
}

/// Decode Bambu Lab tag data from raw blocks
fn decode_bambu_tag(block_data: &[u8]) -> DecodedTagInfo {
    // Block layout (each 16 bytes):
    // Block 1: Material variant ID (0-7), Material ID (8-15)
    // Block 2: Filament type (e.g., "PLA")
    // Block 4: Detailed type (e.g., "PLA Basic")
    // Block 5: Color RGBA (0-3), Spool weight (4-5 little-endian)

    if block_data.len() < 64 {
        warn!("Insufficient block data: {} bytes", block_data.len());
        return DecodedTagInfo {
            tag_type_name: "Bambu Lab".to_string(),
            ..Default::default()
        };
    }

    let block1 = &block_data[0..16];
    let block2 = &block_data[16..32];
    let block4 = &block_data[32..48];
    let block5 = &block_data[48..64];

    // Extract material ID (block 1, bytes 8-15)
    let material_id = extract_cstring(&block1[8..16]);

    // Extract filament type (block 2)
    let filament_type = extract_cstring(block2);

    // Extract detailed type (block 4)
    let detailed_type = extract_cstring(block4);

    // Extract color RGBA (block 5, bytes 0-3)
    let color_rgba = u32::from_be_bytes([block5[0], block5[1], block5[2], block5[3]]);

    // Extract spool weight (block 5, bytes 4-5, little-endian)
    let spool_weight = i16::from_le_bytes([block5[4], block5[5]]) as i32;

    // Derive subtype from detailed_type
    let material_subtype = if detailed_type.starts_with(&format!("Bambu {} ", filament_type)) {
        detailed_type.strip_prefix(&format!("Bambu {} ", filament_type))
            .unwrap_or("")
            .to_string()
    } else if detailed_type.starts_with(&filament_type) {
        detailed_type.strip_prefix(&filament_type)
            .map(|s| s.trim())
            .unwrap_or("")
            .to_string()
    } else {
        detailed_type.clone()
    };

    info!("Decoded Bambu tag: material_id={}, type={}, detailed={}, color=0x{:08X}, weight={}g",
          material_id, filament_type, detailed_type, color_rgba, spool_weight);

    DecodedTagInfo {
        vendor: "Bambu".to_string(),
        material: filament_type,
        material_subtype,
        color_name: format_color_name(color_rgba),
        color_rgba,
        spool_weight,
        tag_type_name: "Bambu Lab".to_string(),
    }
}

/// Extract null-terminated string from bytes
fn extract_cstring(data: &[u8]) -> String {
    let end = data.iter().position(|&b| b == 0).unwrap_or(data.len());
    String::from_utf8_lossy(&data[..end]).to_string()
}

/// Format color RGBA as a name (fallback to hex if no name found)
fn format_color_name(rgba: u32) -> String {
    // Simple color name lookup for common colors
    // Format is 0xRRGGBBAA
    let r = (rgba >> 24) & 0xFF;
    let g = (rgba >> 16) & 0xFF;
    let b = (rgba >> 8) & 0xFF;

    // Very basic color detection
    if r > 200 && g < 100 && b < 100 {
        "Red".to_string()
    } else if r < 100 && g > 200 && b < 100 {
        "Green".to_string()
    } else if r < 100 && g < 100 && b > 200 {
        "Blue".to_string()
    } else if r > 200 && g > 200 && b < 100 {
        "Yellow".to_string()
    } else if r > 200 && g > 200 && b > 200 {
        "White".to_string()
    } else if r < 50 && g < 50 && b < 50 {
        "Black".to_string()
    } else if r > 200 && g > 100 && b < 100 {
        "Orange".to_string()
    } else {
        // Return hex color
        format!("#{:02X}{:02X}{:02X}", r, g, b)
    }
}

/// Get UID as hex string
#[allow(dead_code)]
pub fn get_uid_hex(state: &NfcBridgeState) -> Option<String> {
    if !state.tag_present || state.tag_uid_len == 0 {
        return None;
    }

    let hex: String = state.tag_uid[..state.tag_uid_len as usize]
        .iter()
        .map(|b| format!("{:02X}", b))
        .collect::<Vec<_>>()
        .join(":");

    Some(hex)
}
