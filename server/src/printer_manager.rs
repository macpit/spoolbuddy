//! Printer connection manager
//!
//! Manages MQTT connections to multiple Bambu Lab printers

use std::collections::HashMap;

use tokio::sync::{broadcast, mpsc, Mutex, RwLock};
use tracing::{debug, info, warn};

use crate::mqtt::{BambuMqttClient, PrinterCommand, PrinterConfig, PrinterEvent, PrinterState};

/// Handle for sending commands to a connected printer
#[derive(Clone)]
pub struct PrinterHandle {
    pub serial: String,
    pub command_tx: mpsc::Sender<PrinterCommand>,
}

/// Connection info for a printer
#[derive(Debug, Clone)]
pub struct PrinterConnection {
    pub serial: String,
    pub connected: bool,
    pub state: Option<PrinterState>,
}

/// Manages connections to multiple printers
pub struct PrinterManager {
    /// Active printer connections (serial -> handle)
    connections: RwLock<HashMap<String, PrinterHandle>>,
    /// Printer states (serial -> state)
    states: RwLock<HashMap<String, PrinterState>>,
    /// Connection status (serial -> connected)
    connected: RwLock<HashMap<String, bool>>,
    /// Event broadcaster for UI updates
    event_tx: broadcast::Sender<PrinterEvent>,
    /// Shutdown signals for printer tasks
    shutdown_txs: Mutex<HashMap<String, mpsc::Sender<()>>>,
}

impl PrinterManager {
    /// Create a new printer manager
    pub fn new() -> (Self, broadcast::Receiver<PrinterEvent>) {
        let (event_tx, event_rx) = broadcast::channel(100);

        let manager = Self {
            connections: RwLock::new(HashMap::new()),
            states: RwLock::new(HashMap::new()),
            connected: RwLock::new(HashMap::new()),
            event_tx,
            shutdown_txs: Mutex::new(HashMap::new()),
        };

        (manager, event_rx)
    }

    /// Subscribe to printer events
    pub fn subscribe(&self) -> broadcast::Receiver<PrinterEvent> {
        self.event_tx.subscribe()
    }

    /// Check if a printer is connected
    pub async fn is_connected(&self, serial: &str) -> bool {
        self.connected.read().await.get(serial).copied().unwrap_or(false)
    }

    /// Get all connection statuses
    pub async fn get_connection_statuses(&self) -> HashMap<String, bool> {
        self.connected.read().await.clone()
    }

    /// Get printer state
    pub async fn get_state(&self, serial: &str) -> Option<PrinterState> {
        self.states.read().await.get(serial).cloned()
    }

    /// Connect to a printer
    pub async fn connect(
        &self,
        serial: String,
        ip_address: String,
        access_code: String,
        name: Option<String>,
    ) -> Result<(), String> {
        // Check if already connected
        if self.is_connected(&serial).await {
            return Err(format!("Printer {} is already connected", serial));
        }

        info!("Connecting to printer {} at {}", serial, ip_address);

        let config = PrinterConfig {
            serial: serial.clone(),
            ip_address,
            access_code,
            name,
        };

        // Create channels for this printer
        let (command_tx, command_rx) = mpsc::channel(32);
        let (shutdown_tx, mut shutdown_rx) = mpsc::channel(1);

        // Store handle
        {
            let mut connections = self.connections.write().await;
            connections.insert(
                serial.clone(),
                PrinterHandle {
                    serial: serial.clone(),
                    command_tx,
                },
            );
        }

        // Store shutdown sender
        {
            let mut shutdown_txs = self.shutdown_txs.lock().await;
            shutdown_txs.insert(serial.clone(), shutdown_tx);
        }

        // Create MQTT client
        let client = BambuMqttClient::new(config, self.event_tx.clone(), command_rx);

        // Spawn the client task
        let serial_clone = serial.clone();
        let event_tx = self.event_tx.clone();

        tokio::spawn(async move {
            tokio::select! {
                _ = client.run() => {
                    debug!("MQTT client for {} finished", serial_clone);
                }
                _ = shutdown_rx.recv() => {
                    info!("Shutting down MQTT client for {}", serial_clone);
                }
            }
            // Send disconnect event
            let _ = event_tx.send(PrinterEvent::Disconnected {
                serial: serial_clone,
            });
        });

        Ok(())
    }

    /// Disconnect from a printer
    pub async fn disconnect(&self, serial: &str) -> Result<(), String> {
        info!("Disconnecting from printer {}", serial);

        // Send shutdown signal
        {
            let mut shutdown_txs = self.shutdown_txs.lock().await;
            if let Some(tx) = shutdown_txs.remove(serial) {
                let _ = tx.send(()).await;
            }
        }

        // Remove connection
        {
            let mut connections = self.connections.write().await;
            connections.remove(serial);
        }

        // Update status
        {
            let mut connected = self.connected.write().await;
            connected.remove(serial);
        }

        // Clear state
        {
            let mut states = self.states.write().await;
            states.remove(serial);
        }

        Ok(())
    }

    /// Send a command to a printer
    pub async fn send_command(
        &self,
        serial: &str,
        command: PrinterCommand,
    ) -> Result<(), String> {
        let connections = self.connections.read().await;

        if let Some(handle) = connections.get(serial) {
            handle
                .command_tx
                .send(command)
                .await
                .map_err(|e| format!("Failed to send command: {}", e))
        } else {
            Err(format!("Printer {} is not connected", serial))
        }
    }

    /// Handle printer events (call this in a background task)
    pub async fn handle_event(&self, event: PrinterEvent) {
        match &event {
            PrinterEvent::Connected { serial } => {
                info!("Printer {} connected", serial);
                let mut connected = self.connected.write().await;
                connected.insert(serial.clone(), true);
            }
            PrinterEvent::Disconnected { serial } => {
                info!("Printer {} disconnected", serial);
                let mut connected = self.connected.write().await;
                connected.insert(serial.clone(), false);
            }
            PrinterEvent::StateUpdate { serial, state } => {
                debug!("Printer {} state update", serial);
                let mut states = self.states.write().await;
                states.insert(serial.clone(), state.clone());
            }
            PrinterEvent::Error { serial, message } => {
                warn!("Printer {} error: {}", serial, message);
            }
        }
    }
}

impl Default for PrinterManager {
    fn default() -> Self {
        Self::new().0
    }
}
