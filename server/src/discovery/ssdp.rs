use std::collections::HashMap;
use std::net::{Ipv4Addr, SocketAddrV4, UdpSocket};
use std::sync::Arc;
use std::time::Duration;

use tokio::sync::broadcast;
use tracing::{debug, error, info, warn};

/// SSDP multicast address for Bambu Lab printers
const SSDP_MULTICAST_ADDR: Ipv4Addr = Ipv4Addr::new(239, 255, 255, 250);

/// Bambu Lab uses ports 1990 and 2021 for SSDP
const BAMBU_SSDP_PORTS: [u16; 2] = [1990, 2021];

/// Notification type for Bambu Lab 3D printers
const BAMBU_NT: &str = "urn:bambulab-com:device:3dprinter";

/// Information about a discovered printer
#[derive(Debug, Clone)]
pub struct DiscoveredPrinter {
    pub serial: String,
    pub name: Option<String>,
    pub ip_address: Ipv4Addr,
    pub model: Option<String>,
    pub model_code: Option<String>,
}

/// Raw SSDP info parsed from UDP packet
#[derive(Debug, Default)]
struct SsdpInfo {
    nt: String,
    usn: String,
    location: String,
    custom: HashMap<String, String>,
}

impl SsdpInfo {
    fn is_valid(&self) -> bool {
        !self.nt.is_empty() && !self.location.is_empty()
    }

    fn is_bambu_printer(&self) -> bool {
        self.nt.contains(BAMBU_NT)
    }
}

/// Parse model code to human-readable model name
fn parse_model(model_code: &str) -> &'static str {
    match model_code {
        // X1 series
        "3DPrinter-X1" => "X1",
        "3DPrinter-X1-Carbon" | "BL-P001" => "X1-Carbon",
        "C13" => "X1E",
        // P1 series
        "C11" => "P1P",
        "C12" => "P1S",
        // P2 series
        "N7" => "P2S",
        // A1 series
        "N1" => "A1-Mini",
        "N2" => "A1",
        // H2 series
        "O1D" => "H2D",
        "H2S" => "H2S",
        "H2C" => "H2C",
        _ => "Unknown",
    }
}

/// SSDP discovery service
pub struct SsdpDiscovery {
    /// Channel to broadcast discovered printers
    tx: broadcast::Sender<DiscoveredPrinter>,
    /// Flag to stop the discovery task
    running: Arc<std::sync::atomic::AtomicBool>,
}

impl SsdpDiscovery {
    pub fn new() -> (Self, broadcast::Receiver<DiscoveredPrinter>) {
        let (tx, rx) = broadcast::channel(16);
        let running = Arc::new(std::sync::atomic::AtomicBool::new(false));
        (Self { tx, running }, rx)
    }

    /// Subscribe to discovered printers
    pub fn subscribe(&self) -> broadcast::Receiver<DiscoveredPrinter> {
        self.tx.subscribe()
    }

    /// Check if discovery is running
    pub fn is_running(&self) -> bool {
        self.running.load(std::sync::atomic::Ordering::Relaxed)
    }

    /// Stop discovery
    pub fn stop(&self) {
        self.running.store(false, std::sync::atomic::Ordering::Relaxed);
    }

    /// Start SSDP discovery in a background task
    pub fn start(&self) -> Result<(), std::io::Error> {
        if self.is_running() {
            return Ok(());
        }

        self.running.store(true, std::sync::atomic::Ordering::Relaxed);

        let tx = self.tx.clone();
        let running = self.running.clone();

        // Create sockets for both Bambu SSDP ports
        let sockets: Vec<_> = BAMBU_SSDP_PORTS
            .iter()
            .filter_map(|port| {
                match create_multicast_socket(*port) {
                    Ok(socket) => {
                        info!("SSDP socket bound to port {}", port);
                        Some(socket)
                    }
                    Err(e) => {
                        warn!("Failed to bind SSDP socket on port {}: {}", port, e);
                        None
                    }
                }
            })
            .collect();

        if sockets.is_empty() {
            return Err(std::io::Error::new(
                std::io::ErrorKind::AddrNotAvailable,
                "Could not bind to any SSDP port",
            ));
        }

        // Spawn blocking task for UDP recv (tokio doesn't directly support multicast well)
        tokio::task::spawn_blocking(move || {
            let mut buf = [0u8; 1024];

            while running.load(std::sync::atomic::Ordering::Relaxed) {
                for socket in &sockets {
                    // Non-blocking receive with timeout
                    match socket.recv_from(&mut buf) {
                        Ok((len, _addr)) => {
                            if let Some(printer) = parse_ssdp_packet(&buf[..len]) {
                                debug!("Discovered printer: {:?}", printer);
                                let _ = tx.send(printer);
                            }
                        }
                        Err(e) if e.kind() == std::io::ErrorKind::WouldBlock => {
                            // Timeout, continue
                        }
                        Err(e) => {
                            error!("SSDP recv error: {}", e);
                        }
                    }
                }
                // Small sleep to prevent busy loop
                std::thread::sleep(Duration::from_millis(100));
            }

            info!("SSDP discovery stopped");
        });

        info!("SSDP discovery started");
        Ok(())
    }
}

/// Create a UDP socket bound to the multicast group
fn create_multicast_socket(port: u16) -> Result<UdpSocket, std::io::Error> {
    use socket2::{Domain, Protocol, Socket, Type};

    let socket = Socket::new(Domain::IPV4, Type::DGRAM, Some(Protocol::UDP))?;

    // Allow address reuse
    socket.set_reuse_address(true)?;
    #[cfg(all(unix, not(target_os = "linux")))]
    socket.set_reuse_port(true)?;

    // Bind to the port
    let addr = SocketAddrV4::new(Ipv4Addr::UNSPECIFIED, port);
    socket.bind(&addr.into())?;

    // Join multicast group
    socket.join_multicast_v4(&SSDP_MULTICAST_ADDR, &Ipv4Addr::UNSPECIFIED)?;

    // Set read timeout for non-blocking behavior in loop
    socket.set_read_timeout(Some(Duration::from_millis(500)))?;

    Ok(socket.into())
}

/// Parse an SSDP packet and extract Bambu printer info
fn parse_ssdp_packet(data: &[u8]) -> Option<DiscoveredPrinter> {
    let text = std::str::from_utf8(data).ok()?;

    let mut info = SsdpInfo::default();

    for line in text.lines() {
        if let Some((key, value)) = line.split_once(' ') {
            let value = value.trim();
            match key {
                "NT:" => info.nt = value.to_string(),
                "Location:" => info.location = value.to_string(),
                "USN:" => info.usn = value.to_string(),
                "NOTIFY" | "HOST:" | "Server:" => {}
                _ => {
                    // Custom headers like "DevName.bambu.com:"
                    info.custom.insert(key.to_string(), value.to_string());
                }
            }
        }
    }

    if !info.is_valid() || !info.is_bambu_printer() {
        return None;
    }

    // Parse IP address from location
    let ip_address = info.location.parse::<Ipv4Addr>().ok()?;

    // Extract model info (try with and without colon in key)
    let model_code = info.custom.get("DevModel.bambu.com:")
        .or_else(|| info.custom.get("DevModel.bambu.com"))
        .cloned();
    let model = model_code.as_ref().map(|c| parse_model(c).to_string());

    tracing::debug!("SSDP parsed - serial: {}, model_code: {:?}, model: {:?}", info.usn, model_code, model);

    // Extract name (try with and without colon)
    let name = info.custom.get("DevName.bambu.com:")
        .or_else(|| info.custom.get("DevName.bambu.com"))
        .cloned();

    Some(DiscoveredPrinter {
        serial: info.usn,
        name,
        ip_address,
        model,
        model_code,
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_parse_model() {
        assert_eq!(parse_model("3DPrinter-X1-Carbon"), "X1 Carbon");
        assert_eq!(parse_model("C11"), "P1P");
        assert_eq!(parse_model("C12"), "P1S");
        assert_eq!(parse_model("N1"), "A1 Mini");
        assert_eq!(parse_model("N2"), "A1");
        assert_eq!(parse_model("unknown"), "Unknown");
    }

    #[test]
    fn test_parse_ssdp_packet() {
        let packet = b"NOTIFY * HTTP/1.1\r\n\
            HOST: 239.255.255.250:1990\r\n\
            NT: urn:bambulab-com:device:3dprinter:1\r\n\
            USN: 00M09A123456789\r\n\
            Location: 192.168.1.100\r\n\
            DevName.bambu.com: My Printer\r\n\
            DevModel.bambu.com: 3DPrinter-X1-Carbon\r\n";

        let printer = parse_ssdp_packet(packet).unwrap();
        assert_eq!(printer.serial, "00M09A123456789");
        assert_eq!(printer.name, Some("My Printer".to_string()));
        assert_eq!(printer.ip_address, Ipv4Addr::new(192, 168, 1, 100));
        assert_eq!(printer.model, Some("X1 Carbon".to_string()));
    }
}
