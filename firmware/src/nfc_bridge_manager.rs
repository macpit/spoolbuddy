//! NFC Bridge Manager with C-callable interface
//!
//! Provides FFI functions for the C UI code to access NFC tag data.
//! Uses the Pico NFC bridge over I2C.

use log::{info, warn};
use std::sync::Mutex;

use crate::nfc::i2c_bridge::{self, NfcBridgeState};
use crate::shared_i2c;

/// Global NFC state protected by mutex
static NFC_STATE: Mutex<Option<NfcBridgeState>> = Mutex::new(None);

/// NFC status for C code
#[repr(C)]
pub struct NfcStatus {
    pub initialized: bool,
    pub tag_present: bool,
    pub uid_len: u8,
    pub uid: [u8; 10],
}

/// Initialize the NFC bridge manager
pub fn init_nfc_manager() -> bool {
    // Use shared I2C to initialize
    let result = shared_i2c::with_i2c(|i2c| {
        let mut state = NfcBridgeState::new();
        match i2c_bridge::init_bridge(i2c, &mut state) {
            Ok(()) => {
                info!("NFC bridge manager initialized");
                Some(state)
            }
            Err(e) => {
                warn!("NFC bridge init failed: {}", e);
                None
            }
        }
    });

    if let Some(Some(state)) = result {
        let mut guard = NFC_STATE.lock().unwrap();
        *guard = Some(state);
        true
    } else {
        false
    }
}

/// Poll the NFC bridge (call from main loop)
pub fn poll_nfc() {
    static mut LAST_TAG_PRESENT: bool = false;
    static mut TAG_DATA_READ: bool = false;

    // Collect data from I2C, then release locks before HTTP calls
    let mut tag_just_appeared = false;
    let mut tag_just_removed = false;
    let mut tag_data_decoded = false;
    let mut uid_hex = String::new();
    #[allow(unused_variables)]
    let mut decoded_info: Option<i2c_bridge::DecodedTagInfo> = None;

    {
        let mut guard = NFC_STATE.lock().unwrap();
        if let Some(ref mut state) = *guard {
            if state.initialized {
                let _ = shared_i2c::with_i2c(|i2c| {
                    match i2c_bridge::scan_tag(i2c, state) {
                        Ok(found) => {
                            unsafe {
                                if found && !LAST_TAG_PRESENT {
                                    // Tag just appeared
                                    uid_hex = get_uid_hex_string(state);
                                    info!("NFC TAG DETECTED: {}", uid_hex);
                                    TAG_DATA_READ = false;
                                    tag_just_appeared = true;
                                }

                                // Read tag data if we haven't yet (for local decoding)
                                if found && !TAG_DATA_READ {
                                    match i2c_bridge::read_tag_data(i2c, state) {
                                        Ok(true) => {
                                            TAG_DATA_READ = true;
                                            tag_data_decoded = true;
                                            decoded_info = state.decoded_info.clone();

                                            // Copy decoded data to FFI storage
                                            if let Some(ref info) = state.decoded_info {
                                                set_decoded_tag_data(
                                                    &info.vendor,
                                                    &info.material,
                                                    &info.material_subtype,
                                                    &info.color_name,
                                                    info.color_rgba,
                                                    info.spool_weight,
                                                    &info.tag_type_name,
                                                );
                                                info!("Tag decoded: {} {} {} ({}g)",
                                                    info.vendor, info.material, info.color_name, info.spool_weight);
                                            }

                                            if uid_hex.is_empty() {
                                                uid_hex = get_uid_hex_string(state);
                                            }
                                        }
                                        Ok(false) => {
                                            // No data yet, will retry
                                        }
                                        Err(e) => {
                                            warn!("Tag data read error: {}", e);
                                            TAG_DATA_READ = true; // Don't keep retrying on error
                                        }
                                    }
                                }

                                if !found && LAST_TAG_PRESENT {
                                    // Tag just removed
                                    info!("NFC TAG REMOVED");
                                    clear_decoded_tag_data();
                                    TAG_DATA_READ = false;
                                    tag_just_removed = true;
                                }
                                LAST_TAG_PRESENT = found;
                            }
                        }
                        Err(e) => {
                            warn!("NFC scan error: {}", e);
                        }
                    }
                });
            }
        }
    } // Release NFC_STATE lock and I2C lock here

    // Now make HTTP calls outside the locks
    if tag_just_appeared || tag_data_decoded {
        let weight = crate::scale_manager::scale_get_weight();
        let stable = crate::scale_manager::scale_is_stable();
        crate::backend_client::send_device_state(Some(&uid_hex), weight, stable);
    }

    if tag_just_removed {
        let weight = crate::scale_manager::scale_get_weight();
        let stable = crate::scale_manager::scale_is_stable();
        crate::backend_client::send_device_state(None, weight, stable);
    }
}

/// Get UID as hex string (internal helper)
fn get_uid_hex_string(state: &NfcBridgeState) -> String {
    if state.tag_present && state.tag_uid_len > 0 {
        state.tag_uid[..state.tag_uid_len as usize]
            .iter()
            .map(|b| format!("{:02X}", b))
            .collect::<Vec<_>>()
            .join(":")
    } else {
        String::new()
    }
}

// =============================================================================
// C-callable FFI functions
// =============================================================================

/// Get current NFC status
#[no_mangle]
pub extern "C" fn nfc_get_status(status: *mut NfcStatus) {
    if status.is_null() {
        return;
    }

    let guard = NFC_STATE.lock().unwrap();
    let status = unsafe { &mut *status };

    if let Some(ref state) = *guard {
        status.initialized = state.initialized;
        status.tag_present = state.tag_present;
        status.uid_len = state.tag_uid_len;
        status.uid = state.tag_uid;
    } else {
        status.initialized = false;
        status.tag_present = false;
        status.uid_len = 0;
        status.uid = [0; 10];
    }
}

/// Check if NFC is initialized
#[no_mangle]
pub extern "C" fn nfc_is_initialized() -> bool {
    let guard = NFC_STATE.lock().unwrap();
    if let Some(ref state) = *guard {
        state.initialized
    } else {
        false
    }
}

/// Check if a tag is present
#[no_mangle]
pub extern "C" fn nfc_tag_present() -> bool {
    let guard = NFC_STATE.lock().unwrap();
    if let Some(ref state) = *guard {
        state.tag_present
    } else {
        false
    }
}

/// Get tag UID length (0 if no tag)
#[no_mangle]
pub extern "C" fn nfc_get_uid_len() -> u8 {
    let guard = NFC_STATE.lock().unwrap();
    if let Some(ref state) = *guard {
        if state.tag_present {
            state.tag_uid_len
        } else {
            0
        }
    } else {
        0
    }
}

/// Copy tag UID to buffer (returns actual length copied)
#[no_mangle]
pub extern "C" fn nfc_get_uid(buf: *mut u8, buf_len: u8) -> u8 {
    if buf.is_null() || buf_len == 0 {
        return 0;
    }

    let guard = NFC_STATE.lock().unwrap();
    if let Some(ref state) = *guard {
        if state.tag_present && state.tag_uid_len > 0 {
            let copy_len = std::cmp::min(state.tag_uid_len, buf_len) as usize;
            unsafe {
                std::ptr::copy_nonoverlapping(state.tag_uid.as_ptr(), buf, copy_len);
            }
            return copy_len as u8;
        }
    }
    0
}

/// Get UID as hex string (for display)
/// Writes to buf, returns length written (not including null terminator)
#[no_mangle]
pub extern "C" fn nfc_get_uid_hex(buf: *mut u8, buf_len: u8) -> u8 {
    if buf.is_null() || buf_len < 3 {
        return 0;
    }

    let guard = NFC_STATE.lock().unwrap();
    if let Some(ref state) = *guard {
        if state.tag_present && state.tag_uid_len > 0 {
            // Format: "XX:XX:XX:XX" - each byte is 2 chars + separator
            let max_bytes = ((buf_len as usize) + 1) / 3;  // Account for : separators
            let uid_len = std::cmp::min(state.tag_uid_len as usize, max_bytes);

            let mut pos = 0usize;
            for i in 0..uid_len {
                if pos + 2 > buf_len as usize {
                    break;
                }
                let hex_chars: [u8; 16] = *b"0123456789ABCDEF";
                let byte = state.tag_uid[i];
                unsafe {
                    *buf.add(pos) = hex_chars[(byte >> 4) as usize];
                    *buf.add(pos + 1) = hex_chars[(byte & 0x0F) as usize];
                }
                pos += 2;

                // Add separator if not last byte
                if i < uid_len - 1 && pos < buf_len as usize {
                    unsafe {
                        *buf.add(pos) = b':';
                    }
                    pos += 1;
                }
            }

            return pos as u8;
        }
    }
    0
}

// =============================================================================
// Decoded Tag Data Storage
// =============================================================================

/// Decoded tag data (populated by backend or local decoding)
struct DecodedTagData {
    vendor: [u8; 32],
    material: [u8; 32],
    material_subtype: [u8; 32],
    color_name: [u8; 32],
    color_rgba: u32,
    spool_weight: i32,
    tag_type: [u8; 32],
}

impl Default for DecodedTagData {
    fn default() -> Self {
        Self {
            vendor: [0; 32],
            material: [0; 32],
            material_subtype: [0; 32],
            color_name: [0; 32],
            color_rgba: 0,
            spool_weight: 0,
            tag_type: [0; 32],
        }
    }
}

static DECODED_TAG: Mutex<DecodedTagData> = Mutex::new(DecodedTagData {
    vendor: [0; 32],
    material: [0; 32],
    material_subtype: [0; 32],
    color_name: [0; 32],
    color_rgba: 0,
    spool_weight: 0,
    tag_type: [0; 32],
});

/// Helper to copy string to fixed buffer
fn copy_str_to_buf(src: &str, dst: &mut [u8]) {
    let bytes = src.as_bytes();
    let len = bytes.len().min(dst.len() - 1);
    dst[..len].copy_from_slice(&bytes[..len]);
    dst[len] = 0;
}

/// Set decoded tag data (called from backend response parsing)
pub fn set_decoded_tag_data(
    vendor: &str,
    material: &str,
    subtype: &str,
    color_name: &str,
    color_rgba: u32,
    spool_weight: i32,
    tag_type: &str,
) {
    let mut data = DECODED_TAG.lock().unwrap();
    copy_str_to_buf(vendor, &mut data.vendor);
    copy_str_to_buf(material, &mut data.material);
    copy_str_to_buf(subtype, &mut data.material_subtype);
    copy_str_to_buf(color_name, &mut data.color_name);
    data.color_rgba = color_rgba;
    data.spool_weight = spool_weight;
    copy_str_to_buf(tag_type, &mut data.tag_type);
    info!("Decoded tag data set: {} {} {}", vendor, material, color_name);
}

/// Clear decoded tag data (when tag removed)
pub fn clear_decoded_tag_data() {
    let mut data = DECODED_TAG.lock().unwrap();
    *data = DecodedTagData::default();
}

// =============================================================================
// Decoded Tag Data FFI Functions
// =============================================================================

/// Get tag vendor (returns pointer to static string, valid until next call)
#[no_mangle]
pub extern "C" fn nfc_get_tag_vendor() -> *const std::ffi::c_char {
    static mut VENDOR_BUF: [u8; 32] = [0; 32];
    let data = DECODED_TAG.lock().unwrap();
    unsafe {
        VENDOR_BUF.copy_from_slice(&data.vendor);
        VENDOR_BUF.as_ptr() as *const std::ffi::c_char
    }
}

/// Get tag material type
#[no_mangle]
pub extern "C" fn nfc_get_tag_material() -> *const std::ffi::c_char {
    static mut MATERIAL_BUF: [u8; 32] = [0; 32];
    let data = DECODED_TAG.lock().unwrap();
    unsafe {
        MATERIAL_BUF.copy_from_slice(&data.material);
        MATERIAL_BUF.as_ptr() as *const std::ffi::c_char
    }
}

/// Get tag material subtype
#[no_mangle]
pub extern "C" fn nfc_get_tag_material_subtype() -> *const std::ffi::c_char {
    static mut SUBTYPE_BUF: [u8; 32] = [0; 32];
    let data = DECODED_TAG.lock().unwrap();
    unsafe {
        SUBTYPE_BUF.copy_from_slice(&data.material_subtype);
        SUBTYPE_BUF.as_ptr() as *const std::ffi::c_char
    }
}

/// Get tag color name
#[no_mangle]
pub extern "C" fn nfc_get_tag_color_name() -> *const std::ffi::c_char {
    static mut COLOR_BUF: [u8; 32] = [0; 32];
    let data = DECODED_TAG.lock().unwrap();
    unsafe {
        COLOR_BUF.copy_from_slice(&data.color_name);
        COLOR_BUF.as_ptr() as *const std::ffi::c_char
    }
}

/// Get tag color as RGBA (0xRRGGBBAA)
#[no_mangle]
pub extern "C" fn nfc_get_tag_color_rgba() -> u32 {
    let data = DECODED_TAG.lock().unwrap();
    data.color_rgba
}

/// Get spool weight from tag (grams)
#[no_mangle]
pub extern "C" fn nfc_get_tag_spool_weight() -> i32 {
    let data = DECODED_TAG.lock().unwrap();
    data.spool_weight
}

/// Get tag type (e.g., "bambu", "spoolease", "generic")
#[no_mangle]
pub extern "C" fn nfc_get_tag_type() -> *const std::ffi::c_char {
    static mut TYPE_BUF: [u8; 32] = [0; 32];
    let data = DECODED_TAG.lock().unwrap();
    unsafe {
        TYPE_BUF.copy_from_slice(&data.tag_type);
        TYPE_BUF.as_ptr() as *const std::ffi::c_char
    }
}
