//! MQTT client for Bambu Lab printers
//!
//! Handles TLS connection, subscription, and message handling

use std::sync::Arc;
use std::time::Duration;

use rumqttc::{AsyncClient, Event, MqttOptions, Packet, QoS, TlsConfiguration, Transport};
use tokio::sync::{broadcast, mpsc};
use tracing::{debug, error, info, warn};

use super::bambu_api::{
    AmsFilamentSettingCommand, GetVersionCommand, Message, PrintData, PushAllCommand,
};

/// Bambu Lab printer connection configuration
#[derive(Debug, Clone)]
pub struct PrinterConfig {
    pub serial: String,
    pub ip_address: String,
    pub access_code: String,
    pub name: Option<String>,
}

/// Events from the printer
#[derive(Debug, Clone)]
pub enum PrinterEvent {
    Connected { serial: String },
    Disconnected { serial: String },
    StateUpdate { serial: String, state: PrinterState },
    Error { serial: String, message: String },
}

/// Printer state derived from MQTT messages
#[derive(Debug, Clone, Default)]
pub struct PrinterState {
    pub gcode_state: Option<String>,
    pub print_progress: Option<u32>,
    pub layer_num: Option<i32>,
    pub total_layer_num: Option<i32>,
    pub subtask_name: Option<String>,
    pub ams_trays: Vec<AmsTrayState>,
    pub vt_tray: Option<AmsTrayState>,
}

/// AMS tray state
#[derive(Debug, Clone, Default)]
pub struct AmsTrayState {
    pub ams_id: u32,
    pub tray_id: u32,
    pub tray_type: Option<String>,
    pub tray_color: Option<String>,
    pub tray_info_idx: Option<String>,
    pub k_value: Option<f32>,
}

/// Commands to send to the printer
#[derive(Debug, Clone)]
pub enum PrinterCommand {
    /// Request full state push
    PushAll,
    /// Get version information
    GetVersion,
    /// Set filament in AMS slot
    SetFilament {
        ams_id: i32,
        tray_id: i32,
        slot_id: i32,
        tray_info_idx: String,
        tray_type: String,
        tray_color: String,
        nozzle_temp_min: u32,
        nozzle_temp_max: u32,
    },
}

/// MQTT client for a single Bambu Lab printer
pub struct BambuMqttClient {
    config: PrinterConfig,
    client: Option<AsyncClient>,
    event_tx: broadcast::Sender<PrinterEvent>,
    command_rx: mpsc::Receiver<PrinterCommand>,
}

impl BambuMqttClient {
    /// Create a new MQTT client for a printer
    pub fn new(
        config: PrinterConfig,
        event_tx: broadcast::Sender<PrinterEvent>,
        command_rx: mpsc::Receiver<PrinterCommand>,
    ) -> Self {
        Self {
            config,
            client: None,
            event_tx,
            command_rx,
        }
    }

    /// Run the MQTT client (blocking)
    pub async fn run(mut self) {
        loop {
            match self.connect_and_run().await {
                Ok(()) => {
                    info!("MQTT client for {} exited normally", self.config.serial);
                }
                Err(e) => {
                    error!(
                        "MQTT client for {} error: {:?}, reconnecting...",
                        self.config.serial, e
                    );
                    let _ = self.event_tx.send(PrinterEvent::Disconnected {
                        serial: self.config.serial.clone(),
                    });
                }
            }
            tokio::time::sleep(Duration::from_secs(5)).await;
        }
    }

    async fn connect_and_run(&mut self) -> Result<(), Box<dyn std::error::Error + Send + Sync>> {
        let serial = &self.config.serial;
        let ip = &self.config.ip_address;
        let access_code = &self.config.access_code;

        info!("Connecting to printer {} at {}:8883", serial, ip);

        // Create MQTT options
        let mut mqttoptions = MqttOptions::new(
            format!("spoolbuddy-{}", serial),
            ip.clone(),
            8883,
        );
        mqttoptions.set_keep_alive(Duration::from_secs(30));
        mqttoptions.set_credentials("bblp", access_code);
        // Bambu Lab printers send large status messages (can be 15KB+)
        mqttoptions.set_max_packet_size(64 * 1024, 64 * 1024); // 64KB incoming, 64KB outgoing

        // Configure TLS - Bambu Lab uses self-signed certificates
        debug!("Creating TLS configuration...");
        let tls_config = Self::create_tls_config()?;
        debug!("TLS configuration created successfully");
        mqttoptions.set_transport(Transport::tls_with_config(tls_config));

        debug!("Creating MQTT client...");
        let (client, mut eventloop) = AsyncClient::new(mqttoptions, 100);
        self.client = Some(client.clone());
        debug!("MQTT client created, starting event loop...");

        // First, poll the event loop to establish connection
        debug!("Polling event loop to establish connection...");

        // Poll until we get ConnAck or error
        let mut connected = false;
        for attempt in 0..30 {
            // 30 attempts * 1 second timeout = 30 seconds max
            match tokio::time::timeout(Duration::from_secs(1), eventloop.poll()).await {
                Ok(Ok(Event::Incoming(Packet::ConnAck(ack)))) => {
                    info!("MQTT connected to printer {} (connack: {:?})", serial, ack);
                    connected = true;
                    break;
                }
                Ok(Ok(event)) => {
                    debug!("Pre-connect event: {:?}", event);
                }
                Ok(Err(e)) => {
                    error!("MQTT connection error for {}: {:?}", serial, e);
                    return Err(Box::new(e));
                }
                Err(_) => {
                    debug!("Connection attempt {} - waiting...", attempt + 1);
                }
            }
        }

        if !connected {
            return Err("Connection timeout - no ConnAck received".into());
        }

        // Subscribe to printer report topic
        let report_topic = format!("device/{}/report", serial);
        debug!("Subscribing to {}", report_topic);
        client
            .subscribe(&report_topic, QoS::AtLeastOnce)
            .await?;

        info!("Subscribed to {}", report_topic);

        // Send initial pushall to get current state
        debug!("Sending pushall command...");
        self.send_command(PrinterCommand::PushAll).await?;

        // Notify that we're connected
        info!("Printer {} connected and ready", serial);
        let _ = self.event_tx.send(PrinterEvent::Connected {
            serial: serial.clone(),
        });

        // Main event loop
        loop {
            tokio::select! {
                // Handle incoming MQTT events
                event = eventloop.poll() => {
                    match event {
                        Ok(Event::Incoming(Packet::Publish(publish))) => {
                            self.handle_message(&publish.payload).await;
                        }
                        Ok(Event::Incoming(Packet::ConnAck(_))) => {
                            debug!("Duplicate ConnAck from {}", serial);
                        }
                        Ok(Event::Incoming(Packet::PingResp)) => {
                            debug!("Ping response from {}", serial);
                        }
                        Ok(event) => {
                            debug!("MQTT event: {:?}", event);
                        }
                        Err(e) => {
                            error!("MQTT error for {}: {:?}", serial, e);
                            return Err(Box::new(e));
                        }
                    }
                }
                // Handle outgoing commands
                cmd = self.command_rx.recv() => {
                    if let Some(cmd) = cmd {
                        if let Err(e) = self.send_command(cmd).await {
                            error!("Failed to send command: {:?}", e);
                        }
                    }
                }
            }
        }
    }

    fn create_tls_config() -> Result<TlsConfiguration, Box<dyn std::error::Error + Send + Sync>> {
        // Create a TLS configuration that accepts Bambu Lab's self-signed certificates
        // Bambu Lab printers use self-signed certificates, so we skip verification
        use rumqttc::TlsConfiguration;

        // ClientConfig::builder() uses ring provider with safe defaults
        let config = rustls::ClientConfig::builder()
            .dangerous()
            .with_custom_certificate_verifier(Arc::new(NoVerifier))
            .with_no_client_auth();

        Ok(TlsConfiguration::Rustls(Arc::new(config)))
    }

    async fn handle_message(&self, payload: &[u8]) {
        let payload_str = match std::str::from_utf8(payload) {
            Ok(s) => s,
            Err(_) => {
                warn!("Non-UTF8 payload from printer");
                return;
            }
        };

        debug!("Received message: {}", &payload_str[..payload_str.len().min(200)]);

        // Parse the message
        match serde_json::from_str::<Message>(payload_str) {
            Ok(Message::Print(print)) => {
                self.handle_print_message(&print.print).await;
            }
            Ok(Message::Info(info)) => {
                debug!("Info message: {:?}", info);
            }
            Err(e) => {
                // Not all messages match our structure, that's ok
                debug!("Failed to parse message: {}", e);
            }
        }
    }

    async fn handle_print_message(&self, data: &PrintData) {
        let mut state = PrinterState::default();

        // Extract gcode state
        if let Some(gcode_state) = &data.gcode_state {
            state.gcode_state = Some(format!("{:?}", gcode_state));
        }

        // Extract progress
        state.print_progress = data.gcode_file_prepare_percent;
        state.layer_num = data.layer_num;
        state.total_layer_num = data.total_layer_num;
        state.subtask_name = data.subtask_name.clone();

        // Extract AMS tray information
        if let Some(ams) = &data.ams {
            if let Some(ams_units) = &ams.ams {
                for ams_unit in ams_units {
                    for tray in &ams_unit.tray {
                        if let Some(tray_id) = tray.id {
                            state.ams_trays.push(AmsTrayState {
                                ams_id: ams_unit.id,
                                tray_id,
                                tray_type: tray.tray_type.clone(),
                                tray_color: tray.tray_color.clone(),
                                tray_info_idx: tray.tray_info_idx.clone(),
                                k_value: tray.k,
                            });
                        }
                    }
                }
            }
        }

        // Extract virtual tray
        if let Some(vt) = &data.vt_tray {
            state.vt_tray = Some(AmsTrayState {
                ams_id: 255,
                tray_id: vt.id.unwrap_or(254),
                tray_type: vt.tray_type.clone(),
                tray_color: vt.tray_color.clone(),
                tray_info_idx: vt.tray_info_idx.clone(),
                k_value: vt.k,
            });
        }

        // Broadcast state update
        let _ = self.event_tx.send(PrinterEvent::StateUpdate {
            serial: self.config.serial.clone(),
            state,
        });
    }

    async fn send_command(
        &self,
        cmd: PrinterCommand,
    ) -> Result<(), Box<dyn std::error::Error + Send + Sync>> {
        let client = self.client.as_ref().ok_or("Not connected")?;
        let topic = format!("device/{}/request", self.config.serial);

        let payload = match cmd {
            PrinterCommand::PushAll => {
                serde_json::to_string(&PushAllCommand::new())?
            }
            PrinterCommand::GetVersion => {
                serde_json::to_string(&GetVersionCommand::new())?
            }
            PrinterCommand::SetFilament {
                ams_id,
                tray_id,
                slot_id,
                tray_info_idx,
                tray_type,
                tray_color,
                nozzle_temp_min,
                nozzle_temp_max,
            } => {
                let cmd = AmsFilamentSettingCommand::new(
                    ams_id,
                    tray_id,
                    slot_id,
                    &tray_info_idx,
                    None,
                    &tray_type,
                    &tray_color,
                    nozzle_temp_min,
                    nozzle_temp_max,
                );
                serde_json::to_string(&cmd)?
            }
        };

        debug!("Sending to {}: {}", topic, payload);
        client
            .publish(&topic, QoS::AtLeastOnce, false, payload)
            .await?;

        Ok(())
    }
}

/// Certificate verifier that accepts any certificate
/// WARNING: This is insecure and should only be used for Bambu Lab printers
/// which use self-signed certificates
#[derive(Debug)]
struct NoVerifier;

impl rustls::client::danger::ServerCertVerifier for NoVerifier {
    fn verify_server_cert(
        &self,
        _end_entity: &rustls::pki_types::CertificateDer<'_>,
        _intermediates: &[rustls::pki_types::CertificateDer<'_>],
        _server_name: &rustls::pki_types::ServerName<'_>,
        _ocsp_response: &[u8],
        _now: rustls::pki_types::UnixTime,
    ) -> Result<rustls::client::danger::ServerCertVerified, rustls::Error> {
        Ok(rustls::client::danger::ServerCertVerified::assertion())
    }

    fn verify_tls12_signature(
        &self,
        _message: &[u8],
        _cert: &rustls::pki_types::CertificateDer<'_>,
        _dss: &rustls::DigitallySignedStruct,
    ) -> Result<rustls::client::danger::HandshakeSignatureValid, rustls::Error> {
        Ok(rustls::client::danger::HandshakeSignatureValid::assertion())
    }

    fn verify_tls13_signature(
        &self,
        _message: &[u8],
        _cert: &rustls::pki_types::CertificateDer<'_>,
        _dss: &rustls::DigitallySignedStruct,
    ) -> Result<rustls::client::danger::HandshakeSignatureValid, rustls::Error> {
        Ok(rustls::client::danger::HandshakeSignatureValid::assertion())
    }

    fn supported_verify_schemes(&self) -> Vec<rustls::SignatureScheme> {
        vec![
            rustls::SignatureScheme::RSA_PKCS1_SHA256,
            rustls::SignatureScheme::RSA_PKCS1_SHA384,
            rustls::SignatureScheme::RSA_PKCS1_SHA512,
            rustls::SignatureScheme::ECDSA_NISTP256_SHA256,
            rustls::SignatureScheme::ECDSA_NISTP384_SHA384,
            rustls::SignatureScheme::RSA_PSS_SHA256,
            rustls::SignatureScheme::RSA_PSS_SHA384,
            rustls::SignatureScheme::RSA_PSS_SHA512,
            rustls::SignatureScheme::ED25519,
        ]
    }
}
