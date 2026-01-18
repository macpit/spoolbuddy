import { useState, useEffect, useRef, useCallback } from "preact/hooks";
import { X, CheckCircle, XCircle, Loader2 } from "lucide-preact";
import { api, Spool, Printer, AssignSpoolResponse } from "../lib/api";
import { useWebSocket, AmsUnit, AmsTray, PrinterState } from "../lib/websocket";

interface AssignAmsModalProps {
  isOpen: boolean;
  onClose: () => void;
  spool: Spool;
}

// Get AMS display name from ID
function getAmsName(amsId: number): string {
  if (amsId <= 3) {
    return `AMS ${String.fromCharCode(65 + amsId)}`; // A, B, C, D
  } else if (amsId >= 128 && amsId <= 135) {
    return `HT-${String.fromCharCode(65 + amsId - 128)}`; // HT-A, HT-B, ...
  } else if (amsId === 254) {
    return "External Left";
  } else if (amsId === 255) {
    return "External";
  }
  return `AMS ${amsId}`;
}

// Check if printer is dual-nozzle from state
// Detection priority:
// 1. nozzle_count field (set by backend when extruder_info has 2+ entries)
// 2. tray_now_left set (only dual-nozzle printers have left nozzle)
// 3. Multiple AMS units with extruder assignments
function isDualNozzle(state: PrinterState | undefined): boolean {
  if (!state) return false;

  // Primary check: nozzle_count from backend (most reliable)
  if (state.nozzle_count === 2) {
    console.log('[isDualNozzle] Detected via nozzle_count=2');
    return true;
  }

  // Fallback: check if tray_now_left is set (only dual-nozzle has left nozzle)
  if (typeof state.tray_now_left === 'number') {
    console.log('[isDualNozzle] Detected via tray_now_left:', state.tray_now_left);
    return true;
  }

  // Fallback: check if multiple AMS units have extruder assignments
  const unitsWithExtruder = state.ams_units?.filter(u => typeof u.extruder === 'number') || [];
  if (unitsWithExtruder.length >= 2) {
    console.log('[isDualNozzle] Detected via unitsWithExtruder:', unitsWithExtruder.length);
    return true;
  }

  console.log('[isDualNozzle] Not dual-nozzle. nozzle_count:', state.nozzle_count, 'tray_now_left:', state.tray_now_left, 'unitsWithExtruder:', unitsWithExtruder.length);
  return false;
}

// Build list of all AMS units including external slots
function buildAmsUnitsWithExternal(
  state: PrinterState | undefined
): AmsUnit[] {
  const units: AmsUnit[] = [...(state?.ams_units || [])];
  const dualNozzle = isDualNozzle(state);

  // Check if external slots already exist in ams_units
  const hasExternal = units.some(u => u.id === 255);
  const hasExternalLeft = units.some(u => u.id === 254);

  if (dualNozzle) {
    // Dual extruder: add Ext-L (254) and Ext-R (255) if not present
    if (!hasExternalLeft) {
      units.push({
        id: 254,
        humidity: null,
        temperature: null,
        extruder: 1, // Left extruder
        trays: state?.vt_tray && state.vt_tray.ams_id === 254
          ? [state.vt_tray]
          : [{ ams_id: 254, tray_id: 0, tray_type: null, tray_color: null, tray_info_idx: null, k_value: null, nozzle_temp_min: null, nozzle_temp_max: null, remain: null }],
      });
    }
    if (!hasExternal) {
      units.push({
        id: 255,
        humidity: null,
        temperature: null,
        extruder: 0, // Right extruder
        trays: state?.vt_tray && state.vt_tray.ams_id === 255
          ? [state.vt_tray]
          : [{ ams_id: 255, tray_id: 0, tray_type: null, tray_color: null, tray_info_idx: null, k_value: null, nozzle_temp_min: null, nozzle_temp_max: null, remain: null }],
      });
    }
  } else {
    // Single extruder: add External (255) if not present
    if (!hasExternal) {
      units.push({
        id: 255,
        humidity: null,
        temperature: null,
        extruder: null,
        trays: state?.vt_tray
          ? [state.vt_tray]
          : [{ ams_id: 255, tray_id: 0, tray_type: null, tray_color: null, tray_info_idx: null, k_value: null, nozzle_temp_min: null, nozzle_temp_max: null, remain: null }],
      });
    }
  }

  // Sort: AMS A-D first, then HT units, then external
  return units.sort((a, b) => {
    // Regular AMS (0-3) first
    if (a.id <= 3 && b.id <= 3) return a.id - b.id;
    if (a.id <= 3) return -1;
    if (b.id <= 3) return 1;
    // HT units (128-135) next
    if (a.id >= 128 && a.id <= 135 && b.id >= 128 && b.id <= 135) return a.id - b.id;
    if (a.id >= 128 && a.id <= 135) return -1;
    if (b.id >= 128 && b.id <= 135) return 1;
    // External slots last (254 before 255)
    return a.id - b.id;
  });
}

// Get slot display name
function getSlotName(amsId: number, trayId: number): string {
  if (amsId <= 3) {
    return `${String.fromCharCode(65 + amsId)}${trayId + 1}`; // A1, A2, B3, etc.
  } else if (amsId >= 128 && amsId <= 135) {
    return `HT-${String.fromCharCode(65 + amsId - 128)}`;
  } else if (amsId === 254) {
    return "Ext-L";
  } else if (amsId === 255) {
    return "Ext-R";
  }
  return `Slot ${trayId + 1}`;
}

// Convert hex color from printer (e.g., "FF0000FF") to CSS color
function trayColorToCSS(color: string | null): string {
  if (!color) return "#808080";
  const hex = color.slice(0, 6);
  return `#${hex}`;
}

// Check if a tray is empty
function isTrayEmpty(tray: AmsTray): boolean {
  return !tray.tray_type || tray.tray_type === "";
}

// Slot button component
function SlotButton({
  tray,
  amsId,
  trayId,
  isSelected,
  onClick,
  disabled,
  hideSlotName,
}: {
  tray: AmsTray | null;
  amsId: number;
  trayId: number;
  isSelected: boolean;
  onClick: () => void;
  disabled?: boolean;
  hideSlotName?: boolean;
}) {
  const isEmpty = !tray || isTrayEmpty(tray);
  const color = tray ? trayColorToCSS(tray.tray_color) : "#808080";
  const slotName = getSlotName(amsId, trayId);

  return (
    <button
      onClick={onClick}
      disabled={disabled}
      class={`relative flex flex-col items-center p-2 rounded-lg border-2 transition-all ${
        isSelected
          ? "border-[var(--accent-color)] bg-[var(--accent-color)]/10"
          : disabled
            ? "border-[var(--border-color)] bg-[var(--bg-tertiary)] opacity-50 cursor-not-allowed"
            : "border-[var(--border-color)] hover:border-[var(--accent-color)]/50 hover:bg-[var(--bg-tertiary)] bg-[var(--bg-secondary)]"
      }`}
    >
      {/* Spool icon */}
      <div
        class={`w-10 h-10 rounded-full flex items-center justify-center shadow-sm ${
          isEmpty ? "border-2 border-dashed border-[var(--text-muted)]/50" : ""
        }`}
        style={!isEmpty ? { backgroundColor: color } : {}}
      >
        {isEmpty && <div class="w-2 h-2 rounded-full bg-[var(--text-muted)]/50" />}
      </div>
      {/* Slot name */}
      {!hideSlotName && (
        <span class={`text-xs font-medium mt-1 ${isSelected ? 'text-[var(--accent-color)]' : 'text-[var(--text-primary)]'}`}>{slotName}</span>
      )}
      {/* Material type */}
      <span class="text-[10px] text-[var(--text-muted)] truncate max-w-[50px]">
        {isEmpty ? "Empty" : tray?.tray_type || ""}
      </span>
      {/* Selection indicator */}
      {isSelected && (
        <div class="absolute -top-1 -right-1 w-4 h-4 bg-[var(--accent-color)] rounded-full flex items-center justify-center shadow-sm">
          <svg class="w-3 h-3 text-white" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={3} d="M5 13l4 4L19 7" />
          </svg>
        </div>
      )}
    </button>
  );
}

// AMS unit display component
function AmsUnitDisplay({
  unit,
  printerSerial,
  selectedSlot,
  onSelectSlot,
  readingSlot,
  isWaiting,
  dualNozzle,
}: {
  unit: AmsUnit;
  printerSerial: string;
  selectedSlot: { serial: string; amsId: number; trayId: number } | null;
  onSelectSlot: (serial: string, amsId: number, trayId: number) => void;
  readingSlot: { serial: string; amsId: number; trayId: number } | null;
  isWaiting: boolean;
  dualNozzle: boolean;
}) {
  const amsName = getAmsName(unit.id);
  const isHt = unit.id >= 128 && unit.id <= 135;
  const isExternal = unit.id === 254 || unit.id === 255;
  const isSingleSlot = isHt || isExternal;

  // Build slots array
  const slotCount = isSingleSlot ? 1 : 4;
  const slots: (AmsTray | null)[] = Array(slotCount).fill(null);
  unit.trays.forEach((tray) => {
    if (tray.tray_id >= 0 && tray.tray_id < slotCount) {
      slots[tray.tray_id] = tray;
    }
  });

  // For single-slot units (HT-*, External), show in same container but 1/4 width
  if (isSingleSlot) {
    const tray = slots[0];
    const isSelected = selectedSlot?.serial === printerSerial &&
      selectedSlot?.amsId === unit.id &&
      selectedSlot?.trayId === 0;
    const isThisSlotReading = readingSlot?.serial === printerSerial &&
      readingSlot?.amsId === unit.id &&
      readingSlot?.trayId === 0;

    // For external slots: show "Ext" for single-nozzle, "Ext-L"/"Ext-R" for dual-nozzle
    const slotName = isExternal && !dualNozzle ? "Ext" : getSlotName(unit.id, 0);

    return (
      <div class="bg-[var(--bg-secondary)] border border-[var(--border-color)] rounded-lg p-3 w-fit">
        <div class="text-sm font-medium text-[var(--text-primary)] mb-2 whitespace-nowrap">{slotName}</div>
        <div class="relative">
          <SlotButton
            tray={tray}
            amsId={unit.id}
            trayId={0}
            isSelected={isSelected}
            onClick={() => !isWaiting && onSelectSlot(printerSerial, unit.id, 0)}
            disabled={isWaiting}
            hideSlotName
          />
          {isThisSlotReading && (
            <div class="absolute inset-0 bg-black/50 rounded-lg flex items-center justify-center">
              <div class="w-5 h-5 border-2 border-white border-t-transparent rounded-full animate-spin" />
            </div>
          )}
        </div>
      </div>
    );
  }

  return (
    <div class="bg-[var(--bg-secondary)] border border-[var(--border-color)] rounded-lg p-3">
      <div class="text-sm font-medium text-[var(--text-primary)] mb-2">{amsName}</div>
      <div class="grid gap-2 grid-cols-4">
        {slots.map((tray, idx) => {
          const isSelected = selectedSlot?.serial === printerSerial &&
            selectedSlot?.amsId === unit.id &&
            selectedSlot?.trayId === idx;
          const isThisSlotReading = readingSlot?.serial === printerSerial &&
            readingSlot?.amsId === unit.id &&
            readingSlot?.trayId === idx;

          return (
            <div key={idx} class="relative">
              <SlotButton
                tray={tray}
                amsId={unit.id}
                trayId={idx}
                isSelected={isSelected}
                onClick={() => !isWaiting && onSelectSlot(printerSerial, unit.id, idx)}
                disabled={isWaiting}
              />
              {isThisSlotReading && (
                <div class="absolute inset-0 bg-black/50 rounded-lg flex items-center justify-center">
                  <div class="w-5 h-5 border-2 border-white border-t-transparent rounded-full animate-spin" />
                </div>
              )}
            </div>
          );
        })}
      </div>
    </div>
  );
}

// Status message types
type StatusType = "info" | "success" | "error" | null;

// Calculate bit position for a tray in tray_reading_bits
function getTrayBit(amsId: number, trayId: number): number {
  if (amsId <= 3) {
    // Regular AMS: bits 0-15 (AMS A = 0-3, AMS B = 4-7, etc.)
    return 1 << (amsId * 4 + trayId);
  }
  // HT and external slots - bit position unclear, return 0 for now
  return 0;
}

export function AssignAmsModal({ isOpen, onClose, spool }: AssignAmsModalProps) {
  const [printers, setPrinters] = useState<Printer[]>([]);
  const [loading, setLoading] = useState(true);
  const [selectedSlot, setSelectedSlot] = useState<{
    serial: string;
    amsId: number;
    trayId: number;
  } | null>(null);
  const [statusMessage, setStatusMessage] = useState<string | null>(null);
  const [statusType, setStatusType] = useState<StatusType>(null);
  // Track which slot is being read (for spinner)
  const [readingSlot, setReadingSlot] = useState<{
    serial: string;
    amsId: number;
    trayId: number;
  } | null>(null);
  const { printerStatuses, printerStates, subscribe } = useWebSocket();

  // Track which slot we're waiting for configuration on
  const waitingForSlotRef = useRef<{
    serial: string;
    amsId: number;
    trayId: number;
    originalTrayType: string | null;
    originalTrayColor: string | null;
    readingComplete: boolean; // Track if RFID reading has completed
    startTime: number; // Track when we started waiting
  } | null>(null);

  // Load printers when modal opens
  useEffect(() => {
    if (isOpen) {
      loadPrinters();
      setSelectedSlot(null);
      setStatusMessage(null);
      setStatusType(null);
      setReadingSlot(null);
      waitingForSlotRef.current = null;
    }
  }, [isOpen]);

  // Subscribe to WebSocket messages to detect slot reading and configuration
  useEffect(() => {
    if (!isOpen) return;

    const unsubscribe = subscribe((message) => {
      const waiting = waitingForSlotRef.current;

      // Handle tray_reading messages (RFID scanning start/stop)
      if (message.type === "tray_reading" && waiting) {
        const serial = message.serial as string;
        const oldBits = message.old_bits as number | null;
        const newBits = message.new_bits as number;

        // Check if we're waiting for this printer
        if (waiting.serial !== serial) return;

        const trayBit = getTrayBit(waiting.amsId, waiting.trayId);

        if (trayBit > 0) {
          const wasReading = oldBits !== null && (oldBits & trayBit) !== 0;
          const isReading = (newBits & trayBit) !== 0;

          if (!wasReading && isReading) {
            // Reading started for our slot - show spinner
            setReadingSlot({ serial, amsId: waiting.amsId, trayId: waiting.trayId });
            setStatusMessage("Reading slot...");
          } else if (wasReading && !isReading) {
            // Reading stopped for our slot - mark it so we can detect config change
            setReadingSlot(null);
            waitingForSlotRef.current = { ...waiting, readingComplete: true };
          }
        }
      }

      // Handle assignment_complete messages from backend (for empty slot -> spool inserted)
      if (message.type === "assignment_complete" && waiting) {
        const serial = message.serial as string;
        const amsId = message.ams_id as number;
        const trayId = message.tray_id as number;
        const success = message.success as boolean;

        // Check if this matches our waiting assignment
        if (serial === waiting.serial && amsId === waiting.amsId && trayId === waiting.trayId) {
          waitingForSlotRef.current = null;
          setReadingSlot(null);

          if (success) {
            setStatusType("success");
            setStatusMessage("Slot configured successfully!");
            // Auto-close modal after showing success
            setTimeout(() => {
              handleClose();
            }, 1500);
          } else {
            setStatusType("error");
            setStatusMessage("Failed to configure slot");
          }
        }
      }

      // Handle printer_state updates (check reading state and detect configuration)
      if (message.type === "printer_state" && waiting) {
        if (message.serial !== waiting.serial) return;

        const state = message.state as PrinterState;

        // Find the tray in the state
        const unit = state.ams_units?.find((u: AmsUnit) => u.id === waiting.amsId);
        const tray = unit?.trays?.find((t: AmsTray) => t.tray_id === waiting.trayId);

        // For external slots, check vt_tray
        const isExternal = waiting.amsId === 254 || waiting.amsId === 255;
        const vtTray = isExternal ? state.vt_tray : null;
        const relevantTray = isExternal ? vtTray : tray;

        // Check if currently reading this slot (from printer_state, like ESP firmware does)
        const trayBit = getTrayBit(waiting.amsId, waiting.trayId);
        const isReading = trayBit > 0 &&
          state.tray_reading_bits !== null &&
          (state.tray_reading_bits & trayBit) !== 0;

        // Update spinner state based on reading bits (poll-style like ESP firmware)
        if (isReading) {
          // Slot is being read - show spinner
          if (!readingSlot || readingSlot.amsId !== waiting.amsId || readingSlot.trayId !== waiting.trayId) {
            setReadingSlot({ serial: waiting.serial, amsId: waiting.amsId, trayId: waiting.trayId });
            setStatusMessage("Reading slot...");
          }
        } else if (readingSlot && readingSlot.amsId === waiting.amsId && readingSlot.trayId === waiting.trayId) {
          // Was reading, now stopped - mark reading complete
          setReadingSlot(null);
          waitingForSlotRef.current = { ...waiting, readingComplete: true };
        }

        if (!relevantTray) {
          // Tray not found in state - check timeout
          const elapsed = Date.now() - waiting.startTime;
          if (elapsed > 30000) {
            waitingForSlotRef.current = null;
            setReadingSlot(null);
            setStatusType("error");
            setStatusMessage("Timeout waiting for slot configuration");
          }
          return;
        }

        // Check if tray_type or tray_color changed from original
        const currentType = relevantTray.tray_type || null;
        const currentColor = relevantTray.tray_color || null;
        const typeChanged = currentType !== waiting.originalTrayType;
        const colorChanged = currentColor !== waiting.originalTrayColor;

        // Detection conditions:
        // 1. Type or color changed AND not currently reading AND has a type
        // 2. OR: Reading completed AND slot now has a type (even if type didn't "change" from null)
        const hasConfig = !!currentType;
        const configDetected = (typeChanged || colorChanged) && !isReading && hasConfig;
        const postReadConfig = waiting.readingComplete && hasConfig && !isReading;

        if (configDetected || postReadConfig) {
          waitingForSlotRef.current = null;
          setReadingSlot(null);
          setStatusType("success");
          setStatusMessage(`Slot configured with ${currentType}`);
          // Auto-close modal after showing success
          setTimeout(() => {
            handleClose();
          }, 1500);
        }
      }
    });

    return unsubscribe;
  }, [isOpen, subscribe]);

  const loadPrinters = async () => {
    setLoading(true);
    try {
      const data = await api.listPrinters();
      setPrinters(data);
    } catch (e) {
      console.error("Failed to load printers:", e);
    } finally {
      setLoading(false);
    }
  };

  const handleAssign = async () => {
    if (!selectedSlot) return;

    const slotName = getSlotName(selectedSlot.amsId, selectedSlot.trayId);
    const spoolName = `${spool.brand || ""} ${spool.material} - ${spool.color_name || ""}`.trim();

    // Get current tray state so we can detect when it changes
    const state = printerStates.get(selectedSlot.serial);
    const unit = state?.ams_units?.find(u => u.id === selectedSlot.amsId);
    const tray = unit?.trays?.find(t => t.tray_id === selectedSlot.trayId);
    const isExternal = selectedSlot.amsId === 254 || selectedSlot.amsId === 255;
    const vtTray = isExternal ? state?.vt_tray : null;
    const relevantTray = isExternal ? vtTray : tray;

    const originalTrayType = relevantTray?.tray_type || null;
    const originalTrayColor = relevantTray?.tray_color || null;

    setStatusType("info");
    setStatusMessage(`Please insert spool "${spoolName}" into slot ${slotName}...`);
    waitingForSlotRef.current = {
      ...selectedSlot,
      originalTrayType,
      originalTrayColor,
      readingComplete: false,
      startTime: Date.now(),
    };

    try {
      const result: AssignSpoolResponse = await api.assignSpoolToSlot(
        selectedSlot.serial,
        selectedSlot.amsId,
        selectedSlot.trayId,
        spool.id
      );

      if (result.status === "configured") {
        // Already configured, show success immediately
        waitingForSlotRef.current = null;
        setReadingSlot(null);
        setStatusType("success");
        setStatusMessage(result.message);
        // Auto-close modal after showing success
        setTimeout(() => {
          handleClose();
        }, 1500);
      }
    } catch (e) {
      console.error("Failed to assign spool:", e);
      waitingForSlotRef.current = null;
      setReadingSlot(null);
      setStatusType("error");
      setStatusMessage(e instanceof Error ? e.message : "Failed to assign spool");
    }
  };

  const handleClose = useCallback(() => {
    waitingForSlotRef.current = null;
    setReadingSlot(null);
    onClose();
  }, [onClose]);

  if (!isOpen) return null;

  // Filter to connected printers only
  const connectedPrinters = printers.filter((p) => {
    const isConnected = printerStatuses.get(p.serial) ?? p.connected ?? false;
    return isConnected;
  });

  return (
    <div class="fixed inset-0 z-50 flex items-center justify-center">
      {/* Backdrop */}
      <div class="absolute inset-0 bg-black/50" onClick={handleClose} />

      {/* Modal */}
      <div class="relative bg-[var(--bg-primary)] rounded-lg shadow-xl max-w-2xl w-full mx-4 max-h-[90vh] overflow-hidden flex flex-col">
        {/* Header */}
        <div class="flex items-center justify-between px-6 py-4 border-b border-[var(--border-color)]">
          <div>
            <h2 class="text-lg font-semibold text-[var(--text-primary)]">Assign to AMS Slot</h2>
            <p class="text-sm text-[var(--text-secondary)]">
              {spool.brand} {spool.material} - {spool.color_name}
            </p>
          </div>
          <button
            onClick={handleClose}
            class="p-2 rounded-lg transition-colors hover:bg-[var(--bg-tertiary)]"
          >
            <X class="w-5 h-5 text-[var(--text-muted)]" />
          </button>
        </div>

        {/* Status message area */}
        {statusMessage && (
          <div
            class={`mx-6 mt-4 p-4 rounded-lg flex items-center gap-3 ${
              statusType === "info"
                ? "bg-[var(--accent-color)]/20 border-2 border-[var(--accent-color)]"
                : statusType === "success"
                  ? "bg-green-500/20 border-2 border-green-500"
                  : "bg-red-500/20 border-2 border-red-500"
            }`}
          >
            {statusType === "info" && (
              <Loader2 class="w-5 h-5 text-[var(--accent-color)] animate-spin flex-shrink-0" />
            )}
            {statusType === "success" && (
              <CheckCircle class="w-5 h-5 text-green-400 flex-shrink-0" />
            )}
            {statusType === "error" && (
              <XCircle class="w-5 h-5 text-red-400 flex-shrink-0" />
            )}
            <span
              class={`text-sm font-medium ${
                statusType === "info"
                  ? "text-[var(--accent-color)]"
                  : statusType === "success"
                    ? "text-green-300"
                    : "text-red-300"
              }`}
            >
              {statusMessage}
            </span>
          </div>
        )}

        {/* Content */}
        <div class="flex-1 overflow-y-auto p-6">
          {loading ? (
            <div class="text-center text-[var(--text-muted)] py-8">Loading printers...</div>
          ) : connectedPrinters.length === 0 ? (
            <div class="text-center text-[var(--text-muted)] py-8">
              No printers connected. Connect a printer first.
            </div>
          ) : (
            <div class="space-y-6">
              {connectedPrinters.map((printer) => {
                const state = printerStates.get(printer.serial);
                const dualNozzle = isDualNozzle(state);
                const amsUnits = buildAmsUnitsWithExternal(state);

                // Separate regular AMS units from single-slot units
                const regularUnits = amsUnits.filter(u => u.id <= 3);
                const singleSlotUnits = amsUnits.filter(u => u.id > 3);

                return (
                  <div key={printer.serial} class="space-y-3">
                    <div class="text-sm font-medium text-[var(--text-primary)]">
                      {printer.name || printer.serial}
                      {printer.model && <span class="ml-2 text-xs text-[var(--text-muted)]">({printer.model})</span>}
                    </div>
                    {/* Regular AMS units in grid */}
                    {regularUnits.length > 0 && (
                      <div class="grid grid-cols-1 sm:grid-cols-2 gap-3">
                        {regularUnits.map((unit) => (
                          <AmsUnitDisplay
                            key={`${printer.serial}-${unit.id}`}
                            unit={unit}
                            printerSerial={printer.serial}
                            selectedSlot={selectedSlot}
                            onSelectSlot={(serial, amsId, trayId) =>
                              setSelectedSlot({ serial, amsId, trayId })
                            }
                            readingSlot={readingSlot}
                            isWaiting={waitingForSlotRef.current !== null}
                            dualNozzle={dualNozzle}
                          />
                        ))}
                      </div>
                    )}
                    {/* Single-slot units (HT-*, External) in flex row */}
                    {singleSlotUnits.length > 0 && (
                      <div class="flex flex-wrap gap-3">
                        {singleSlotUnits.map((unit) => (
                          <AmsUnitDisplay
                            key={`${printer.serial}-${unit.id}`}
                            unit={unit}
                            printerSerial={printer.serial}
                            selectedSlot={selectedSlot}
                            onSelectSlot={(serial, amsId, trayId) =>
                              setSelectedSlot({ serial, amsId, trayId })
                            }
                            readingSlot={readingSlot}
                            isWaiting={waitingForSlotRef.current !== null}
                            dualNozzle={dualNozzle}
                          />
                        ))}
                      </div>
                    )}
                  </div>
                );
              })}
            </div>
          )}
        </div>

        {/* Footer */}
        <div class="flex justify-end gap-3 px-6 py-4 border-t border-[var(--border-color)]">
          {statusType ? (
            // Show close button when any status is active (waiting, success, or error)
            <button onClick={handleClose} class="btn btn-primary">
              Close
            </button>
          ) : (
            <>
              <button onClick={handleClose} class="btn">
                Cancel
              </button>
              <button
                onClick={handleAssign}
                disabled={!selectedSlot}
                class="btn btn-primary"
              >
                Assign
              </button>
            </>
          )}
        </div>
      </div>
    </div>
  );
}
