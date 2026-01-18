import { createContext, ComponentChildren } from "preact";
import { useContext, useEffect, useRef, useCallback, useState } from "preact/hooks";

export interface AmsTray {
  ams_id: number;
  tray_id: number;
  tray_type: string | null;
  tray_color: string | null;
  tray_info_idx: string | null;
  k_value: number | null;
  nozzle_temp_min: number | null;
  nozzle_temp_max: number | null;
  remain: number | null; // Remaining filament percentage (0-100)
}

export interface AmsUnit {
  id: number;
  humidity: number | null;  // Percentage (0-100) from humidity_raw, or index (1-5) fallback
  temperature: number | null;  // Temperature in Celsius
  extruder: number | null; // 0 = right nozzle, 1 = left nozzle for H2C/H2D
  trays: AmsTray[];
}

export interface PrinterState {
  gcode_state: string | null;
  print_progress: number | null;
  layer_num: number | null;
  total_layer_num: number | null;
  subtask_name: string | null;
  mc_remaining_time: number | null; // Remaining time in minutes
  gcode_file: string | null; // Current gcode file path
  ams_units: AmsUnit[];
  vt_tray: AmsTray | null;
  tray_now: number | null; // Currently active tray (0-15 for AMS, 254/255 for external) - legacy single-nozzle
  // Dual-nozzle support (H2C/H2D)
  tray_now_left: number | null; // Active tray for left nozzle (extruder 1)
  tray_now_right: number | null; // Active tray for right nozzle (extruder 0)
  active_extruder: number | null; // Currently active extruder (0=right, 1=left)
  // Tray reading state (RFID scanning)
  tray_reading_bits: number | null; // Bitmask of trays currently being read
  // Nozzle count (auto-detected from MQTT)
  nozzle_count: number; // 1 = single nozzle, 2 = dual nozzle (H2C/H2D)
}

interface WebSocketState {
  deviceConnected: boolean;
  deviceUpdateAvailable: boolean;
  currentWeight: number | null;
  weightStable: boolean;
  currentTagId: string | null;
  printerStatuses: Map<string, boolean>;
  printerStates: Map<string, PrinterState>;
}

interface WebSocketContextValue extends WebSocketState {
  subscribe: (handler: (message: WebSocketMessage) => void) => () => void;
}

interface WebSocketMessage {
  type: string;
  [key: string]: unknown;
}

const WebSocketContext = createContext<WebSocketContextValue | null>(null);

export function WebSocketProvider({ children }: { children: ComponentChildren }) {
  // Use useState for reactive state that triggers re-renders
  const [deviceConnected, setDeviceConnected] = useState(false);
  const [deviceUpdateAvailable, setDeviceUpdateAvailable] = useState(false);
  const [currentWeight, setCurrentWeight] = useState<number | null>(null);
  const [weightStable, setWeightStable] = useState(false);
  const [currentTagId, setCurrentTagId] = useState<string | null>(null);
  const [printerStatuses, setPrinterStatuses] = useState<Map<string, boolean>>(new Map());
  const [printerStates, setPrinterStates] = useState<Map<string, PrinterState>>(new Map());

  const wsRef = useRef<WebSocket | null>(null);
  const handlersRef = useRef<Set<(message: WebSocketMessage) => void>>(new Set());
  const reconnectTimeoutRef = useRef<number | null>(null);

  // Handle incoming WebSocket messages
  const handleMessage = useCallback((message: WebSocketMessage) => {
    switch (message.type) {
      case "initial_state":
        if (message.device && typeof message.device === "object") {
          const device = message.device as {
            connected?: boolean;
            last_weight?: number;
            weight_stable?: boolean;
            current_tag_id?: string;
            update_available?: boolean;
          };
          setDeviceConnected(device.connected ?? false);
          setDeviceUpdateAvailable(device.update_available ?? false);
          setCurrentWeight(device.last_weight ?? null);
          setWeightStable(device.weight_stable ?? false);
          setCurrentTagId(device.current_tag_id ?? null);
        }
        // Parse initial printer statuses
        if (message.printers && typeof message.printers === "object") {
          const printers = message.printers as Record<string, boolean>;
          setPrinterStatuses(new Map(Object.entries(printers)));
        }
        break;

      case "device_update_available":
        setDeviceUpdateAvailable(message.update_available as boolean);
        break;

      case "device_connected":
        setDeviceConnected(true);
        break;

      case "device_disconnected":
        setDeviceConnected(false);
        setCurrentWeight(null);
        setCurrentTagId(null);
        break;

      case "weight":
        setCurrentWeight(message.grams as number);
        setWeightStable(message.stable as boolean);
        break;

      case "device_state":
        // Backend sends device_state with weight updates
        if (message.weight !== undefined) {
          setCurrentWeight(message.weight as number);
        }
        if (message.stable !== undefined) {
          setWeightStable(message.stable as boolean);
        }
        // Update tag_id if explicitly included in message (handles both setting and clearing)
        if ("tag_id" in message) {
          setCurrentTagId(message.tag_id as string | null);
        }
        break;

      case "tag_detected":
        setCurrentTagId(message.tag_id as string);
        break;

      case "tag_staged":
        // Backend broadcasts tag_staged when a tag is detected and staged
        setCurrentTagId(message.tag_id as string);
        break;

      case "tag_removed":
      case "staging_cleared":
        setCurrentTagId(null);
        break;

      case "printer_connected": {
        const serial = message.serial as string;
        setPrinterStatuses(prev => {
          const newMap = new Map(prev);
          newMap.set(serial, true);
          return newMap;
        });
        break;
      }

      case "printer_disconnected": {
        const serial = message.serial as string;
        setPrinterStatuses(prev => {
          const newMap = new Map(prev);
          newMap.set(serial, false);
          return newMap;
        });
        break;
      }

      case "printer_state": {
        const serial = message.serial as string;
        const state = message.state as PrinterState;
        setPrinterStates(prev => {
          const newMap = new Map(prev);
          newMap.set(serial, state);
          return newMap;
        });
        break;
      }

      case "printer_added":
      case "printer_updated":
      case "printer_removed":
        // These are handled by subscribers (e.g., Printers page)
        break;
    }
  }, []);

  const connect = useCallback(() => {
    // Determine WebSocket URL
    const protocol = window.location.protocol === "https:" ? "wss:" : "ws:";
    const wsUrl = `${protocol}//${window.location.host}/ws/ui`;

    console.log("Connecting to WebSocket:", wsUrl);
    const ws = new WebSocket(wsUrl);
    wsRef.current = ws;

    ws.onopen = () => {
      console.log("WebSocket connected");
      if (reconnectTimeoutRef.current) {
        clearTimeout(reconnectTimeoutRef.current);
        reconnectTimeoutRef.current = null;
      }
    };

    ws.onclose = () => {
      console.log("WebSocket disconnected, reconnecting in 3s...");
      setDeviceConnected(false);
      reconnectTimeoutRef.current = window.setTimeout(connect, 3000);
    };

    ws.onerror = (error) => {
      console.error("WebSocket error:", error);
    };

    ws.onmessage = (event) => {
      try {
        const message: WebSocketMessage = JSON.parse(event.data);
        handleMessage(message);

        // Notify subscribers
        handlersRef.current.forEach((handler) => handler(message));
      } catch (e) {
        console.error("Failed to parse WebSocket message:", e);
      }
    };
  }, [handleMessage]);

  useEffect(() => {
    connect();
    return () => {
      if (reconnectTimeoutRef.current) {
        clearTimeout(reconnectTimeoutRef.current);
      }
      wsRef.current?.close();
    };
  }, [connect]);

  const subscribe = useCallback((handler: (message: WebSocketMessage) => void) => {
    handlersRef.current.add(handler);
    return () => {
      handlersRef.current.delete(handler);
    };
  }, []);

  const value: WebSocketContextValue = {
    deviceConnected,
    deviceUpdateAvailable,
    currentWeight,
    weightStable,
    currentTagId,
    printerStatuses,
    printerStates,
    subscribe,
  };

  return (
    <WebSocketContext.Provider value={value}>{children}</WebSocketContext.Provider>
  );
}

export function useWebSocket(): WebSocketContextValue {
  const context = useContext(WebSocketContext);
  if (!context) {
    throw new Error("useWebSocket must be used within a WebSocketProvider");
  }
  return context;
}
