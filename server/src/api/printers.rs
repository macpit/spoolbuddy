use std::sync::Arc;

use axum::{
    extract::{Path, State},
    http::StatusCode,
    routing::get,
    Json, Router,
};
use serde::{Deserialize, Serialize};

use crate::{db::Printer, mqtt::PrinterCommand, AppState};

pub fn router() -> Router<Arc<AppState>> {
    Router::new()
        .route("/", get(list_printers).post(create_printer))
        .route(
            "/{serial}",
            get(get_printer).put(update_printer).delete(delete_printer),
        )
        .route("/{serial}/connect", axum::routing::post(connect_printer))
        .route("/{serial}/disconnect", axum::routing::post(disconnect_printer))
        .route("/{serial}/auto-connect", axum::routing::post(toggle_auto_connect))
        .route("/{serial}/set-slot", axum::routing::post(set_slot_filament))
}

/// Input for creating/updating a printer
#[derive(Debug, Deserialize)]
pub struct PrinterInput {
    pub serial: String,
    pub name: Option<String>,
    pub model: Option<String>,
    pub ip_address: Option<String>,
    pub access_code: Option<String>,
    pub auto_connect: Option<bool>,
}

/// Printer with connection status
#[derive(Debug, Serialize)]
pub struct PrinterWithStatus {
    #[serde(flatten)]
    pub printer: Printer,
    pub connected: bool,
}

/// GET /api/printers - List all printers
async fn list_printers(
    State(state): State<Arc<AppState>>,
) -> Result<Json<Vec<PrinterWithStatus>>, (StatusCode, String)> {
    let printers = sqlx::query_as::<_, Printer>("SELECT * FROM printers ORDER BY name")
        .fetch_all(&state.db)
        .await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    // Get connection statuses from printer manager
    let connection_statuses = state.printer_manager.get_connection_statuses().await;

    // Add connection status
    let printers_with_status: Vec<PrinterWithStatus> = printers
        .into_iter()
        .map(|printer| {
            let connected = connection_statuses
                .get(&printer.serial)
                .copied()
                .unwrap_or(false);
            PrinterWithStatus { printer, connected }
        })
        .collect();

    Ok(Json(printers_with_status))
}

/// GET /api/printers/:serial - Get a single printer
async fn get_printer(
    State(state): State<Arc<AppState>>,
    Path(serial): Path<String>,
) -> Result<Json<PrinterWithStatus>, (StatusCode, String)> {
    let printer = sqlx::query_as::<_, Printer>("SELECT * FROM printers WHERE serial = ?")
        .bind(&serial)
        .fetch_optional(&state.db)
        .await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    match printer {
        Some(p) => {
            let connected = state.printer_manager.is_connected(&serial).await;
            Ok(Json(PrinterWithStatus {
                printer: p,
                connected,
            }))
        }
        None => Err((StatusCode::NOT_FOUND, format!("Printer {} not found", serial))),
    }
}

/// POST /api/printers - Create a new printer
async fn create_printer(
    State(state): State<Arc<AppState>>,
    Json(input): Json<PrinterInput>,
) -> Result<(StatusCode, Json<Printer>), (StatusCode, String)> {
    let now = chrono::Utc::now().timestamp();

    sqlx::query(
        r#"
        INSERT INTO printers (serial, name, model, ip_address, access_code, last_seen)
        VALUES (?, ?, ?, ?, ?, ?)
        "#,
    )
    .bind(&input.serial)
    .bind(&input.name)
    .bind(&input.model)
    .bind(&input.ip_address)
    .bind(&input.access_code)
    .bind(now)
    .execute(&state.db)
    .await
    .map_err(|e| {
        if e.to_string().contains("UNIQUE constraint failed") {
            (
                StatusCode::CONFLICT,
                format!("Printer {} already exists", input.serial),
            )
        } else {
            (StatusCode::INTERNAL_SERVER_ERROR, e.to_string())
        }
    })?;

    // Fetch the created printer
    let printer = sqlx::query_as::<_, Printer>("SELECT * FROM printers WHERE serial = ?")
        .bind(&input.serial)
        .fetch_one(&state.db)
        .await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    // Broadcast to UI
    let _ = state.ui_broadcast.send(
        serde_json::json!({
            "type": "printer_added",
            "printer": printer
        })
        .to_string(),
    );

    Ok((StatusCode::CREATED, Json(printer)))
}

/// PUT /api/printers/:serial - Update a printer
async fn update_printer(
    State(state): State<Arc<AppState>>,
    Path(serial): Path<String>,
    Json(input): Json<PrinterInput>,
) -> Result<Json<Printer>, (StatusCode, String)> {
    let result = sqlx::query(
        r#"
        UPDATE printers SET
            name = COALESCE(?, name),
            model = COALESCE(?, model),
            ip_address = COALESCE(?, ip_address),
            access_code = COALESCE(?, access_code),
            auto_connect = COALESCE(?, auto_connect)
        WHERE serial = ?
        "#,
    )
    .bind(&input.name)
    .bind(&input.model)
    .bind(&input.ip_address)
    .bind(&input.access_code)
    .bind(input.auto_connect.map(|b| if b { 1 } else { 0 }))
    .bind(&serial)
    .execute(&state.db)
    .await
    .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    if result.rows_affected() == 0 {
        return Err((StatusCode::NOT_FOUND, format!("Printer {} not found", serial)));
    }

    // Fetch updated printer
    let printer = sqlx::query_as::<_, Printer>("SELECT * FROM printers WHERE serial = ?")
        .bind(&serial)
        .fetch_one(&state.db)
        .await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    // Broadcast to UI
    let _ = state.ui_broadcast.send(
        serde_json::json!({
            "type": "printer_updated",
            "printer": printer
        })
        .to_string(),
    );

    Ok(Json(printer))
}

/// DELETE /api/printers/:serial - Delete a printer
async fn delete_printer(
    State(state): State<Arc<AppState>>,
    Path(serial): Path<String>,
) -> Result<StatusCode, (StatusCode, String)> {
    let result = sqlx::query("DELETE FROM printers WHERE serial = ?")
        .bind(&serial)
        .execute(&state.db)
        .await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    if result.rows_affected() == 0 {
        return Err((StatusCode::NOT_FOUND, format!("Printer {} not found", serial)));
    }

    // Broadcast to UI
    let _ = state.ui_broadcast.send(
        serde_json::json!({
            "type": "printer_removed",
            "serial": serial
        })
        .to_string(),
    );

    Ok(StatusCode::NO_CONTENT)
}

/// POST /api/printers/:serial/connect - Connect to a printer
async fn connect_printer(
    State(state): State<Arc<AppState>>,
    Path(serial): Path<String>,
) -> Result<StatusCode, (StatusCode, String)> {
    tracing::info!("Connect request for printer {}", serial);

    // Get printer from database
    let printer = sqlx::query_as::<_, Printer>("SELECT * FROM printers WHERE serial = ?")
        .bind(&serial)
        .fetch_optional(&state.db)
        .await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    let printer = match printer {
        Some(p) => p,
        None => return Err((StatusCode::NOT_FOUND, format!("Printer {} not found", serial))),
    };

    // Check required fields
    let ip_address = printer
        .ip_address
        .ok_or((StatusCode::BAD_REQUEST, "Printer has no IP address".to_string()))?;
    let access_code = printer
        .access_code
        .ok_or((StatusCode::BAD_REQUEST, "Printer has no access code".to_string()))?;

    // Connect via printer manager
    state
        .printer_manager
        .connect(serial, ip_address, access_code, printer.name)
        .await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e))?;

    Ok(StatusCode::OK)
}

/// POST /api/printers/:serial/disconnect - Disconnect from a printer
async fn disconnect_printer(
    State(state): State<Arc<AppState>>,
    Path(serial): Path<String>,
) -> Result<StatusCode, (StatusCode, String)> {
    tracing::info!("Disconnect request for printer {}", serial);

    state
        .printer_manager
        .disconnect(&serial)
        .await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e))?;

    Ok(StatusCode::OK)
}

/// Request to toggle auto-connect
#[derive(Debug, Deserialize)]
pub struct AutoConnectRequest {
    pub auto_connect: bool,
}

/// POST /api/printers/:serial/auto-connect - Toggle auto-connect for a printer
async fn toggle_auto_connect(
    State(state): State<Arc<AppState>>,
    Path(serial): Path<String>,
    Json(request): Json<AutoConnectRequest>,
) -> Result<Json<Printer>, (StatusCode, String)> {
    tracing::info!("Auto-connect {} for printer {}", request.auto_connect, serial);

    let result = sqlx::query("UPDATE printers SET auto_connect = ? WHERE serial = ?")
        .bind(if request.auto_connect { 1 } else { 0 })
        .bind(&serial)
        .execute(&state.db)
        .await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    if result.rows_affected() == 0 {
        return Err((StatusCode::NOT_FOUND, format!("Printer {} not found", serial)));
    }

    // Fetch updated printer
    let printer = sqlx::query_as::<_, Printer>("SELECT * FROM printers WHERE serial = ?")
        .bind(&serial)
        .fetch_one(&state.db)
        .await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    // Broadcast to UI
    let _ = state.ui_broadcast.send(
        serde_json::json!({
            "type": "printer_updated",
            "printer": printer
        })
        .to_string(),
    );

    Ok(Json(printer))
}

/// Request to set filament in an AMS slot
#[derive(Debug, Deserialize)]
pub struct SetSlotFilamentRequest {
    pub ams_id: i32,
    pub tray_id: i32,
    pub tray_info_idx: String,
    pub tray_type: String,
    pub tray_color: String,
    pub nozzle_temp_min: u32,
    pub nozzle_temp_max: u32,
}

/// POST /api/printers/:serial/set-slot - Set filament in an AMS slot
async fn set_slot_filament(
    State(state): State<Arc<AppState>>,
    Path(serial): Path<String>,
    Json(request): Json<SetSlotFilamentRequest>,
) -> Result<StatusCode, (StatusCode, String)> {
    tracing::info!(
        "Set slot request for printer {}: AMS {} Tray {} -> {}",
        serial,
        request.ams_id,
        request.tray_id,
        request.tray_info_idx
    );

    // Check if printer is connected
    if !state.printer_manager.is_connected(&serial).await {
        return Err((
            StatusCode::BAD_REQUEST,
            format!("Printer {} is not connected", serial),
        ));
    }

    // Calculate slot_id from ams_id and tray_id
    let slot_id = request.ams_id * 4 + request.tray_id;

    // Send command via printer manager
    let command = PrinterCommand::SetFilament {
        ams_id: request.ams_id,
        tray_id: request.tray_id,
        slot_id,
        tray_info_idx: request.tray_info_idx,
        tray_type: request.tray_type,
        tray_color: request.tray_color,
        nozzle_temp_min: request.nozzle_temp_min,
        nozzle_temp_max: request.nozzle_temp_max,
    };

    state
        .printer_manager
        .send_command(&serial, command)
        .await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e))?;

    Ok(StatusCode::OK)
}
