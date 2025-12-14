//! Bambu Lab MQTT API message structures
//!
//! Ported from SpoolEase bambu_api.rs
//! Reference: https://github.com/markhaehnel/bambulab/blob/main/src/message.rs

use serde::{Deserialize, Deserializer, Serialize, Serializer};

// ==========================================================================
// Main Message Types
// ==========================================================================

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
#[serde(untagged)]
#[allow(clippy::large_enum_variant)]
pub enum Message {
    Print(Print),
    Info(Info),
}

#[derive(Default, Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct Print {
    pub print: PrintData,
}

#[derive(Default, Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct PrintData {
    // Print state
    pub gcode_state: Option<GcodeState>,
    #[serde(
        default,
        serialize_with = "option_u32_as_str_se",
        deserialize_with = "option_u32_as_str_de"
    )]
    pub gcode_file_prepare_percent: Option<u32>,
    pub project_id: Option<String>,
    pub subtask_name: Option<String>,
    pub layer_num: Option<i32>,
    pub total_layer_num: Option<i32>,

    // AMS mapping
    pub ams_mapping: Option<Vec<i32>>,
    pub ams_mapping2: Option<Vec<AmsMapping2Entry>>,

    // AMS data
    pub ams: Option<PrintAms>,
    pub vt_tray: Option<PrintTray>,
    pub vir_slot: Option<Vec<PrintTray>>,

    // Command/response fields
    pub command: Option<String>,
    pub param: Option<String>,
    pub url: Option<String>,
    pub use_ams: Option<bool>,
    pub sequence_id: Option<String>,

    // Filament change announcement fields
    pub nozzle_temp_max: Option<u32>,
    pub nozzle_temp_min: Option<u32>,
    pub tray_color: Option<String>,
    pub tray_id: Option<i32>,
    pub slot_id: Option<i32>,
    pub ams_id: Option<i32>,
    pub cali_idx: Option<i32>,
    pub tray_info_idx: Option<String>,
    pub tray_type: Option<String>,
    pub reason: Option<String>,
    pub result: Option<String>,

    // Calibration fields
    pub nozzle_diameter: Option<String>,
    pub filament_id: Option<String>,
    pub filaments: Option<Vec<Filament>>,
    pub fun: Option<String>,
    pub device: Option<PrintDevice>,
}

// ==========================================================================
// Filament / Calibration
// ==========================================================================

#[derive(Default, Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct Filament {
    pub filament_id: String,
    pub name: String,
    pub k_value: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub setting_id: Option<String>,
    pub cali_idx: i32,
    pub nozzle_id: Option<String>,
    pub extruder_id: Option<i32>,
}

// ==========================================================================
// Device Info (Nozzle, Extruder)
// ==========================================================================

#[derive(Default, Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct PrintDevice {
    pub extruder: Option<PrintDeviceExtruder>,
    #[serde(default, deserialize_with = "ignore_errors")]
    pub nozzle: Option<PrintDeviceNozzle>,
}

#[derive(Default, Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct PrintDeviceExtruder {
    pub info: Vec<PrintDeviceExtruderInfo>,
    pub state: Option<i32>,
}

#[derive(Default, Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct PrintDeviceExtruderInfo {
    pub id: i32,
    pub snow: i32,
    pub spre: i32,
    pub star: i32,
}

#[derive(Default, Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct PrintDeviceNozzle {
    pub info: Vec<PrintDeviceNozzleInfo>,
    pub exist: Option<i32>,
    pub state: Option<i32>,
}

#[derive(Default, Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct PrintDeviceNozzleInfo {
    pub id: i32,
    pub diameter: f32,
    #[serde(rename = "type")]
    pub nozzle_type: String,
}

// ==========================================================================
// AMS (Automatic Material System)
// ==========================================================================

#[derive(Default, Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct PrintAms {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub ams: Option<Vec<PrintAmsData>>,
    pub ams_exist_bits: Option<String>,
    pub tray_exist_bits: Option<String>,
    pub tray_is_bbl_bits: Option<String>,
    #[serde(
        default,
        serialize_with = "option_as_str_se",
        deserialize_with = "option_as_str_de"
    )]
    pub tray_tar: Option<i32>,
    #[serde(
        default,
        serialize_with = "option_as_str_se",
        deserialize_with = "option_as_str_de"
    )]
    pub tray_now: Option<i32>,
    #[serde(
        default,
        serialize_with = "option_as_str_se",
        deserialize_with = "option_as_str_de"
    )]
    pub tray_pre: Option<i32>,
    pub tray_read_done_bits: Option<String>,
    pub tray_reading_bits: Option<String>,
}

#[derive(Default, Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct PrintAmsData {
    #[serde(serialize_with = "u32_as_str_se", deserialize_with = "u32_as_str_de")]
    pub id: u32,
    pub humidity: String,
    pub tray: Vec<PrintTray>,
    #[serde(
        default,
        serialize_with = "option_u32_as_str_hex_se",
        deserialize_with = "option_u32_as_str_hex_de"
    )]
    pub info: Option<u32>,
}

#[derive(Default, Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct PrintTray {
    #[serde(
        default,
        serialize_with = "option_u32_as_str_se",
        deserialize_with = "option_u32_as_str_de"
    )]
    pub id: Option<u32>,
    #[serde(skip_serializing)]
    pub k: Option<f32>,
    #[serde(skip_serializing)]
    pub cali_idx: Option<i32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub tray_info_idx: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub tray_type: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub tray_color: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    #[serde(
        default,
        serialize_with = "option_u32_as_str_se",
        deserialize_with = "option_u32_as_str_de"
    )]
    pub nozzle_temp_max: Option<u32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    #[serde(
        default,
        serialize_with = "option_u32_as_str_se",
        deserialize_with = "option_u32_as_str_de"
    )]
    pub nozzle_temp_min: Option<u32>,
}

#[derive(Deserialize, Serialize, Debug, PartialEq, Clone)]
pub struct AmsMapping2Entry {
    pub ams_id: i32,
    pub slot_id: i32,
}

// ==========================================================================
// Gcode State
// ==========================================================================

#[derive(Deserialize, Serialize, Debug, PartialEq, Clone, Copy, Default)]
#[allow(clippy::upper_case_acronyms)]
pub enum GcodeState {
    #[default]
    Unknown,
    IDLE,
    SLICING,
    PREPARE,
    RUNNING,
    FINISH,
    FAILED,
    PAUSE,
    #[serde(other)]
    Unsupported,
}

// ==========================================================================
// Commands
// ==========================================================================

/// Push all state command
#[derive(Default, Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct PushAllCommand {
    pub pushing: PushAll,
}

#[derive(Default, Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct PushAll {
    pub command: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub sequence_id: Option<String>,
}

impl PushAllCommand {
    pub fn new() -> Self {
        Self {
            pushing: PushAll {
                command: String::from("pushall"),
                sequence_id: Some(String::from("1")),
            },
        }
    }
}

/// Get version command
#[derive(Default, Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct GetVersionCommand {
    pub info: GetVersion,
}

#[derive(Default, Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct GetVersion {
    pub command: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub sequence_id: Option<String>,
}

impl GetVersionCommand {
    pub fn new() -> Self {
        Self {
            info: GetVersion {
                command: String::from("get_version"),
                sequence_id: Some(String::from("1")),
            },
        }
    }
}

/// AMS filament setting command
#[derive(Default, Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct AmsFilamentSettingCommand {
    pub print: AmsFilamentSetting,
}

#[derive(Default, Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct AmsFilamentSetting {
    pub command: String,
    pub ams_id: i32,
    pub tray_id: i32,
    pub slot_id: i32,
    pub tray_info_idx: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub setting_id: Option<String>,
    pub tray_color: String,
    pub nozzle_temp_min: u32,
    pub nozzle_temp_max: u32,
    pub tray_type: String,
    pub sequence_id: String,
}

impl AmsFilamentSettingCommand {
    #[allow(clippy::too_many_arguments)]
    pub fn new(
        ams_id: i32,
        tray_id: i32,
        slot_id: i32,
        tray_info_idx: &str,
        setting_id: Option<&str>,
        tray_type: &str,
        tray_color: &str,
        nozzle_temp_min: u32,
        nozzle_temp_max: u32,
    ) -> Self {
        Self {
            print: AmsFilamentSetting {
                command: String::from("ams_filament_setting"),
                ams_id,
                tray_id,
                slot_id,
                tray_info_idx: String::from(tray_info_idx),
                setting_id: setting_id.map(String::from),
                tray_color: String::from(tray_color),
                nozzle_temp_min,
                nozzle_temp_max,
                tray_type: String::from(tray_type),
                sequence_id: String::from("1"),
            },
        }
    }
}

/// Extrusion calibration get command
#[derive(Default, Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct ExtrusionCaliGetCommand {
    pub print: ExtrusionCaliGet,
}

#[derive(Default, Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct ExtrusionCaliGet {
    pub command: String,
    pub filament_id: String,
    pub nozzle_diameter: String,
    pub sequence_id: String,
}

impl ExtrusionCaliGetCommand {
    pub fn new(nozzle_diameter: &str) -> Self {
        Self {
            print: ExtrusionCaliGet {
                command: String::from("extrusion_cali_get"),
                filament_id: String::new(),
                nozzle_diameter: String::from(nozzle_diameter),
                sequence_id: String::from("1"),
            },
        }
    }
}

/// Extrusion calibration select command
#[derive(Default, Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct ExtrusionCaliSelCommand {
    pub print: ExtrusionCaliSel,
}

#[derive(Default, Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct ExtrusionCaliSel {
    pub command: String,
    pub cali_idx: i32,
    pub filament_id: String,
    pub nozzle_diameter: String,
    pub ams_id: i32,
    pub tray_id: i32,
    pub slot_id: i32,
    pub sequence_id: String,
}

impl ExtrusionCaliSelCommand {
    pub fn new(
        nozzle_diameter: &str,
        ams_id: i32,
        tray_id: i32,
        slot_id: i32,
        filament_id: &str,
        cali_idx: Option<i32>,
    ) -> Self {
        Self {
            print: ExtrusionCaliSel {
                command: String::from("extrusion_cali_sel"),
                cali_idx: cali_idx.unwrap_or(-1),
                filament_id: String::from(filament_id),
                nozzle_diameter: String::from(nozzle_diameter),
                ams_id,
                tray_id,
                slot_id,
                sequence_id: String::from("1"),
            },
        }
    }
}

/// Extrusion calibration set command
#[derive(Default, Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct ExtrusionCaliSetCommand {
    pub print: ExtrusionCaliSet,
}

#[derive(Default, Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct ExtrusionCaliSet {
    pub command: String,
    pub filaments: Vec<ExtrusionCaliSetFilament>,
    pub nozzle_diameter: String,
    pub sequence_id: String,
}

#[derive(Default, Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct ExtrusionCaliSetFilament {
    pub ams_id: i32,
    pub extruder_id: i32,
    pub filament_id: String,
    pub k_value: String,
    pub n_coef: String,
    pub name: String,
    pub nozzle_diameter: String,
    pub nozzle_id: String,
    pub setting_id: String,
    pub slot_id: i32,
    pub tray_id: i32,
}

impl ExtrusionCaliSetCommand {
    #[allow(clippy::too_many_arguments)]
    pub fn new(
        extruder_id: i32,
        nozzle_diameter: &str,
        nozzle_id: &str,
        filament_id: &str,
        setting_id: &str,
        k_value: &str,
        name: &str,
    ) -> Self {
        let filaments = vec![ExtrusionCaliSetFilament {
            ams_id: 0,
            extruder_id,
            filament_id: filament_id.to_string(),
            k_value: k_value.to_string(),
            n_coef: "0.000000".to_string(),
            name: name.to_string(),
            nozzle_diameter: nozzle_diameter.to_string(),
            nozzle_id: nozzle_id.to_string(),
            setting_id: setting_id.to_string(),
            slot_id: 0,
            tray_id: -1,
        }];
        Self {
            print: ExtrusionCaliSet {
                command: String::from("extrusion_cali_set"),
                filaments,
                nozzle_diameter: nozzle_diameter.to_string(),
                sequence_id: "1".to_string(),
            },
        }
    }
}

// ==========================================================================
// Info Response
// ==========================================================================

#[derive(Default, Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct Info {
    pub info: InfoData,
}

#[derive(Default, Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct InfoData {
    pub command: String,
    pub sequence_id: String,
    pub module: Vec<InfoModule>,
    pub result: Option<String>,
    pub reason: Option<String>,
}

#[derive(Default, Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct InfoModule {
    pub name: String,
    pub project_name: Option<String>,
    pub product_name: Option<String>,
    pub sw_ver: String,
    pub hw_ver: String,
    pub sn: String,
    pub flag: Option<i32>,
    pub loader_ver: Option<String>,
    pub ota_ver: Option<String>,
}

// ==========================================================================
// Serde Helpers
// ==========================================================================

fn u32_as_str_se<S>(x: &u32, s: S) -> Result<S::Ok, S::Error>
where
    S: Serializer,
{
    s.serialize_str(&x.to_string())
}

fn u32_as_str_de<'de, D>(deserializer: D) -> Result<u32, D::Error>
where
    D: Deserializer<'de>,
{
    let s: &str = Deserialize::deserialize(deserializer)?;
    s.parse::<u32>().map_err(serde::de::Error::custom)
}

fn option_u32_as_str_se<S>(value: &Option<u32>, serializer: S) -> Result<S::Ok, S::Error>
where
    S: Serializer,
{
    match value {
        Some(v) => u32_as_str_se(v, serializer),
        None => serializer.serialize_none(),
    }
}

fn option_u32_as_str_de<'de, D>(deserializer: D) -> Result<Option<u32>, D::Error>
where
    D: Deserializer<'de>,
{
    let option: Option<String> = Option::deserialize(deserializer)?;
    option
        .as_deref()
        .map(|s| s.parse::<u32>().map_err(serde::de::Error::custom))
        .transpose()
}

fn as_str_se<T, S>(x: &T, s: S) -> Result<S::Ok, S::Error>
where
    S: Serializer,
    T: std::fmt::Display,
{
    s.serialize_str(&x.to_string())
}

fn option_as_str_se<T, S>(value: &Option<T>, serializer: S) -> Result<S::Ok, S::Error>
where
    S: Serializer,
    T: std::fmt::Display,
{
    match value {
        Some(v) => as_str_se::<T, S>(v, serializer),
        None => serializer.serialize_none(),
    }
}

fn option_as_str_de<'de, T, D>(deserializer: D) -> Result<Option<T>, D::Error>
where
    D: Deserializer<'de>,
    T: std::str::FromStr,
    <T as std::str::FromStr>::Err: std::fmt::Display,
{
    let option: Option<String> = Option::deserialize(deserializer)?;
    option
        .as_deref()
        .map(|s| s.parse::<T>().map_err(serde::de::Error::custom))
        .transpose()
}

fn u32_as_str_hex_se<S>(x: &u32, s: S) -> Result<S::Ok, S::Error>
where
    S: Serializer,
{
    s.serialize_str(&format!("{:x}", x))
}

fn option_u32_as_str_hex_se<S>(x: &Option<u32>, s: S) -> Result<S::Ok, S::Error>
where
    S: Serializer,
{
    match x {
        Some(v) => u32_as_str_hex_se(v, s),
        None => s.serialize_none(),
    }
}

fn option_u32_as_str_hex_de<'de, D>(deserializer: D) -> Result<Option<u32>, D::Error>
where
    D: Deserializer<'de>,
{
    let opt: Option<String> = Option::deserialize(deserializer)?;
    opt.as_deref()
        .map(|s| u32::from_str_radix(s, 16).map_err(serde::de::Error::custom))
        .transpose()
}

fn ignore_errors<'de, D, T>(deserializer: D) -> Result<Option<T>, D::Error>
where
    D: serde::Deserializer<'de>,
    T: Deserialize<'de>,
{
    Ok(T::deserialize(deserializer).ok())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_push_all_command() {
        let cmd = PushAllCommand::new();
        let json = serde_json::to_string(&cmd).unwrap();
        assert!(json.contains("pushall"));
    }

    #[test]
    fn test_ams_filament_setting() {
        let cmd = AmsFilamentSettingCommand::new(
            0,
            0,
            0,
            "GFL99",
            None,
            "PLA",
            "FF0000FF",
            190,
            250,
        );
        let json = serde_json::to_string(&cmd).unwrap();
        assert!(json.contains("ams_filament_setting"));
        assert!(json.contains("GFL99"));
    }
}
