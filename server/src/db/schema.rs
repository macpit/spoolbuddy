use serde::{Deserialize, Serialize};
use sqlx::FromRow;

/// Spool record from database
#[derive(Debug, Clone, Serialize, Deserialize, FromRow)]
pub struct Spool {
    pub id: String,
    pub tag_id: Option<String>,
    pub material: String,
    pub subtype: Option<String>,
    pub color_name: Option<String>,
    pub rgba: Option<String>,
    pub brand: Option<String>,
    pub label_weight: Option<i32>,
    pub core_weight: Option<i32>,
    pub weight_new: Option<i32>,
    pub weight_current: Option<i32>,
    pub slicer_filament: Option<String>,
    pub note: Option<String>,
    pub added_time: Option<i64>,
    pub encode_time: Option<i64>,
    pub added_full: Option<i32>,
    pub consumed_since_add: Option<f64>,
    pub consumed_since_weight: Option<f64>,
    pub data_origin: Option<String>,
    pub tag_type: Option<String>,
    pub created_at: Option<i64>,
    pub updated_at: Option<i64>,
}

/// Create/update spool request
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SpoolInput {
    pub tag_id: Option<String>,
    pub material: String,
    pub subtype: Option<String>,
    pub color_name: Option<String>,
    pub rgba: Option<String>,
    pub brand: Option<String>,
    pub label_weight: Option<i32>,
    pub core_weight: Option<i32>,
    pub weight_new: Option<i32>,
    pub weight_current: Option<i32>,
    pub slicer_filament: Option<String>,
    pub note: Option<String>,
    pub data_origin: Option<String>,
    pub tag_type: Option<String>,
}

/// Printer record from database
#[derive(Debug, Clone, Serialize, Deserialize, FromRow)]
pub struct Printer {
    pub serial: String,
    pub name: Option<String>,
    pub model: Option<String>,
    pub ip_address: Option<String>,
    pub access_code: Option<String>,
    pub last_seen: Option<i64>,
    pub config: Option<String>,
    pub auto_connect: Option<bool>,
}

/// K-Profile record from database
#[derive(Debug, Clone, Serialize, Deserialize, FromRow)]
pub struct KProfile {
    pub id: i64,
    pub spool_id: Option<String>,
    pub printer_serial: Option<String>,
    pub extruder: Option<i32>,
    pub nozzle_diameter: Option<String>,
    pub nozzle_type: Option<String>,
    pub k_value: Option<String>,
    pub name: Option<String>,
    pub cali_idx: Option<i32>,
    pub setting_id: Option<String>,
    pub created_at: Option<i64>,
}

/// Usage history record from database
#[derive(Debug, Clone, Serialize, Deserialize, FromRow)]
pub struct UsageHistory {
    pub id: i64,
    pub spool_id: Option<String>,
    pub printer_serial: Option<String>,
    pub print_name: Option<String>,
    pub weight_used: Option<f64>,
    pub timestamp: Option<i64>,
}
