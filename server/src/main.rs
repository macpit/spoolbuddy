mod api;
mod config;
mod db;
mod discovery;
mod mqtt;
mod printer_manager;
mod websocket;

use std::collections::HashMap;
use std::sync::Arc;

use axum::Router;
use sqlx::SqlitePool;
use tokio::sync::{broadcast, Mutex};
use tower_http::{cors::CorsLayer, services::ServeDir, trace::TraceLayer};
use tracing_subscriber::{layer::SubscriberExt, util::SubscriberInitExt};

use crate::config::Config;
use crate::discovery::{DiscoveredPrinter, SsdpDiscovery};
use crate::printer_manager::PrinterManager;
use crate::websocket::DeviceState;

/// Shared application state
pub struct AppState {
    pub db: SqlitePool,
    pub config: Config,
    pub device_state: DeviceState,
    /// Broadcast channel for UI updates
    pub ui_broadcast: broadcast::Sender<String>,
    /// SSDP discovery service
    pub ssdp_discovery: SsdpDiscovery,
    /// Discovered printers (keyed by serial)
    pub discovered_printers: Mutex<HashMap<String, DiscoveredPrinter>>,
    /// Printer connection manager
    pub printer_manager: PrinterManager,
}

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    // Initialize tracing
    tracing_subscriber::registry()
        .with(
            tracing_subscriber::EnvFilter::try_from_default_env()
                .unwrap_or_else(|_| "spoolbuddy_server=debug,tower_http=debug".into()),
        )
        .with(tracing_subscriber::fmt::layer())
        .init();

    // Load configuration
    let config = Config::from_env();

    // Connect to database
    let db = db::connect(&config.database_url).await?;

    // Run migrations
    db::migrate(&db).await?;

    // Create broadcast channel for UI updates
    let (ui_broadcast, _) = broadcast::channel(100);

    // Create SSDP discovery service
    let (ssdp_discovery, mut ssdp_rx) = SsdpDiscovery::new();

    // Create printer manager
    let (printer_manager, mut printer_event_rx) = PrinterManager::new();

    // Create shared state
    let state = Arc::new(AppState {
        db,
        config: config.clone(),
        device_state: DeviceState::new(),
        ui_broadcast,
        ssdp_discovery,
        discovered_printers: Mutex::new(HashMap::new()),
        printer_manager,
    });

    // Spawn task to collect discovered printers
    {
        let state = state.clone();
        tokio::spawn(async move {
            while let Ok(printer) = ssdp_rx.recv().await {
                let mut discovered = state.discovered_printers.lock().await;
                discovered.insert(printer.serial.clone(), printer);
            }
        });
    }

    // Auto-connect printers with auto_connect enabled
    {
        let state = state.clone();
        tokio::spawn(async move {
            // Wait a moment for server to fully initialize
            tokio::time::sleep(std::time::Duration::from_millis(500)).await;

            let printers: Vec<db::Printer> =
                match sqlx::query_as("SELECT * FROM printers WHERE auto_connect = 1")
                    .fetch_all(&state.db)
                    .await
                {
                    Ok(p) => p,
                    Err(e) => {
                        tracing::error!("Failed to fetch auto-connect printers: {}", e);
                        return;
                    }
                };

            for printer in printers {
                if let (Some(ip), Some(code)) = (printer.ip_address, printer.access_code) {
                    tracing::info!("Auto-connecting to printer {}", printer.serial);
                    if let Err(e) = state
                        .printer_manager
                        .connect(printer.serial.clone(), ip, code, printer.name)
                        .await
                    {
                        tracing::error!("Failed to auto-connect to {}: {}", printer.serial, e);
                    }
                }
            }
        });
    }

    // Spawn task to handle printer events and forward to UI
    {
        let state = state.clone();
        tokio::spawn(async move {
            while let Ok(event) = printer_event_rx.recv().await {
                // Update internal state
                state.printer_manager.handle_event(event.clone()).await;

                // Forward to UI as JSON
                let ui_message = match &event {
                    mqtt::PrinterEvent::Connected { serial } => {
                        serde_json::json!({
                            "type": "printer_connected",
                            "serial": serial
                        })
                    }
                    mqtt::PrinterEvent::Disconnected { serial } => {
                        serde_json::json!({
                            "type": "printer_disconnected",
                            "serial": serial
                        })
                    }
                    mqtt::PrinterEvent::StateUpdate { serial, state } => {
                        serde_json::json!({
                            "type": "printer_state",
                            "serial": serial,
                            "state": {
                                "gcode_state": state.gcode_state,
                                "print_progress": state.print_progress,
                                "layer_num": state.layer_num,
                                "total_layer_num": state.total_layer_num,
                                "subtask_name": state.subtask_name,
                            }
                        })
                    }
                    mqtt::PrinterEvent::Error { serial, message } => {
                        serde_json::json!({
                            "type": "printer_error",
                            "serial": serial,
                            "message": message
                        })
                    }
                };

                let _ = state.ui_broadcast.send(ui_message.to_string());
            }
        });
    }

    // Build router
    let app = Router::new()
        .nest("/api", api::router())
        .nest("/ws", websocket::router())
        .fallback_service(ServeDir::new(&config.static_dir))
        .layer(CorsLayer::permissive())
        .layer(TraceLayer::new_for_http())
        .with_state(state);

    // Start server
    let listener = tokio::net::TcpListener::bind(&config.bind_address).await?;
    tracing::info!("SpoolBuddy server listening on {}", config.bind_address);

    axum::serve(listener, app).await?;

    Ok(())
}
