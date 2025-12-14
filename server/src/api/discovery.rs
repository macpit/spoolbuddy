use std::sync::Arc;

use axum::{
    extract::State,
    http::StatusCode,
    routing::{get, post},
    Json, Router,
};
use serde::Serialize;

use crate::AppState;

pub fn router() -> Router<Arc<AppState>> {
    Router::new()
        .route("/status", get(discovery_status))
        .route("/start", post(start_discovery))
        .route("/stop", post(stop_discovery))
        .route("/printers", get(get_discovered_printers))
}

/// Status of the discovery service
#[derive(Serialize)]
pub struct DiscoveryStatus {
    running: bool,
}

/// A discovered printer
#[derive(Serialize, Clone)]
pub struct DiscoveredPrinterResponse {
    serial: String,
    name: Option<String>,
    ip_address: String,
    model: Option<String>,
}

/// GET /api/discovery/status - Get discovery status
async fn discovery_status(
    State(state): State<Arc<AppState>>,
) -> Json<DiscoveryStatus> {
    let running = state.ssdp_discovery.is_running();
    Json(DiscoveryStatus { running })
}

/// POST /api/discovery/start - Start SSDP discovery
async fn start_discovery(
    State(state): State<Arc<AppState>>,
) -> Result<Json<DiscoveryStatus>, (StatusCode, String)> {
    // Clear previous discoveries
    {
        let mut discovered = state.discovered_printers.lock().await;
        discovered.clear();
    }

    // Start discovery
    state
        .ssdp_discovery
        .start()
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    Ok(Json(DiscoveryStatus { running: true }))
}

/// POST /api/discovery/stop - Stop SSDP discovery
async fn stop_discovery(
    State(state): State<Arc<AppState>>,
) -> Json<DiscoveryStatus> {
    state.ssdp_discovery.stop();
    Json(DiscoveryStatus { running: false })
}

/// GET /api/discovery/printers - Get discovered printers
async fn get_discovered_printers(
    State(state): State<Arc<AppState>>,
) -> Json<Vec<DiscoveredPrinterResponse>> {
    let discovered = state.discovered_printers.lock().await;
    let printers: Vec<DiscoveredPrinterResponse> = discovered
        .values()
        .map(|p| DiscoveredPrinterResponse {
            serial: p.serial.clone(),
            name: p.name.clone(),
            ip_address: p.ip_address.to_string(),
            model: p.model.clone(),
        })
        .collect();
    Json(printers)
}
