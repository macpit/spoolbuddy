import { useEffect, useState } from "preact/hooks";
import { api, Printer, DiscoveredPrinter, CalibrationProfile, AMSThresholds } from "../lib/api";
import { useWebSocket } from "../lib/websocket";
import { AmsCard, ExternalSpool } from "../components/AmsCard";
import { AMSHistoryModal } from "../components/AMSHistoryModal";
import { useToast } from "../lib/toast";
import { Modal } from "../components/inventory/Modal";
import {
  Plus,
  Search,
  Trash2,
  ChevronRight,
  Printer as PrinterIcon,
  Wifi,
  WifiOff,
  Loader2,
  Info,
  Image,
  RefreshCw,
  Frown,
} from "lucide-preact";

const EXPANDED_PRINTERS_KEY = "spoolbuddy-expanded-printers";

// Format remaining time in minutes to human readable
function formatRemainingTime(minutes: number | null): string {
  if (minutes === null || minutes === undefined || minutes < 0) return "";
  if (minutes < 60) return `${minutes}m`;
  const hours = Math.floor(minutes / 60);
  const mins = minutes % 60;
  if (hours < 24) return mins > 0 ? `${hours}h ${mins}m` : `${hours}h`;
  const days = Math.floor(hours / 24);
  const remainingHours = hours % 24;
  return remainingHours > 0 ? `${days}d ${remainingHours}h` : `${days}d`;
}

// Calculate ETA from remaining minutes
function calculateETA(minutes: number | null): string {
  if (minutes === null || minutes === undefined || minutes < 0) return "";
  const eta = new Date(Date.now() + minutes * 60 * 1000);
  const now = new Date();
  const isToday = eta.toDateString() === now.toDateString();
  const isTomorrow = eta.toDateString() === new Date(Date.now() + 86400000).toDateString();

  const timeStr = eta.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
  if (isToday) return timeStr;
  if (isTomorrow) return `Tomorrow ${timeStr}`;
  return eta.toLocaleDateString([], { weekday: 'short', hour: '2-digit', minute: '2-digit' });
}

// Load expanded state from localStorage
function loadExpandedPrinters(): Set<string> {
  try {
    const stored = localStorage.getItem(EXPANDED_PRINTERS_KEY);
    if (stored) {
      return new Set(JSON.parse(stored));
    }
  } catch {
    // Ignore errors
  }
  return new Set();
}

// Save expanded state to localStorage
function saveExpandedPrinters(expanded: Set<string>) {
  try {
    localStorage.setItem(EXPANDED_PRINTERS_KEY, JSON.stringify([...expanded]));
  } catch {
    // Ignore errors
  }
}

export function Printers() {
  const [printers, setPrinters] = useState<Printer[]>([]);
  const [loading, setLoading] = useState(true);
  const [showAddModal, setShowAddModal] = useState(false);
  const [showDiscoverModal, setShowDiscoverModal] = useState(false);
  const [selectedDiscovered, setSelectedDiscovered] = useState<DiscoveredPrinter | null>(null);
  const [deleteConfirm, setDeleteConfirm] = useState<string | null>(null); // serial to delete
  const [connecting, setConnecting] = useState<string | null>(null); // serial being connected
  const [calibrations, setCalibrations] = useState<Record<string, CalibrationProfile[]>>({}); // serial -> calibrations
  const [expandedPrinters, setExpandedPrinters] = useState<Set<string>>(loadExpandedPrinters); // expanded printers by serial
  const [amsThresholds, setAmsThresholds] = useState<AMSThresholds | undefined>(undefined);
  const [historyModal, setHistoryModal] = useState<{
    printerSerial: string;
    amsId: number;
    amsLabel: string;
    mode: 'humidity' | 'temperature';
  } | null>(null);
  const { showToast } = useToast();

  const toggleExpanded = (serial: string) => {
    setExpandedPrinters(prev => {
      const next = new Set(prev);
      if (next.has(serial)) {
        next.delete(serial);
      } else {
        next.add(serial);
      }
      saveExpandedPrinters(next);
      return next;
    });
  };

  const { printerStatuses, printerStates, subscribe } = useWebSocket();

  // Fetch calibrations for a connected printer
  const fetchCalibrations = async (serial: string) => {
    try {
      const cals = await api.getCalibrations(serial);
      setCalibrations(prev => ({ ...prev, [serial]: cals }));
    } catch (e) {
      console.error(`Failed to fetch calibrations for ${serial}:`, e);
    }
  };

  useEffect(() => {
    loadPrinters();

    // Subscribe to WebSocket messages for real-time updates
    const unsubscribe = subscribe((message) => {
      // Printer config changes - need to reload full list
      if (
        message.type === "printer_added" ||
        message.type === "printer_updated" ||
        message.type === "printer_removed"
      ) {
        loadPrinters();
      }

      // Connection status changes - handled by printerStatuses, no reload needed
      // This avoids flickering caused by API race conditions
      if (message.type === "printer_connected") {
        setConnecting(null);
        // Fetch calibrations when printer connects
        if (message.serial) {
          fetchCalibrations(message.serial as string);
        }
      } else if (message.type === "printer_disconnected") {
        setConnecting(null);
      }
    });

    return unsubscribe;
  }, [subscribe]);

  // Fetch calibrations for already connected printers on mount
  useEffect(() => {
    printers.forEach(printer => {
      const connected = printerStatuses.get(printer.serial) ?? printer.connected ?? false;
      if (connected && !calibrations[printer.serial]) {
        fetchCalibrations(printer.serial);
      }
    });
  }, [printers, printerStatuses]);

  // Load AMS thresholds
  useEffect(() => {
    api.getAMSThresholds()
      .then(setAmsThresholds)
      .catch(err => console.error("Failed to load AMS thresholds:", err));
  }, []);

  // Handle AMS history click
  const handleHistoryClick = (printerSerial: string) => (amsId: number, amsLabel: string, mode: 'humidity' | 'temperature') => {
    setHistoryModal({ printerSerial, amsId, amsLabel, mode });
  };

  const loadPrinters = async () => {
    try {
      const data = await api.listPrinters();
      setPrinters(data);
    } catch (e) {
      console.error("Failed to load printers:", e);
    } finally {
      setLoading(false);
    }
  };

  const handleDelete = (serial: string) => {
    setDeleteConfirm(serial);
  };

  const confirmDelete = async () => {
    if (!deleteConfirm) return;

    const serial = deleteConfirm;
    const printerToDelete = printers.find(p => p.serial === serial);
    setDeleteConfirm(null);

    try {
      await api.deletePrinter(serial);
      await loadPrinters();
      showToast('success', `Deleted printer "${printerToDelete?.name || serial}"`);
    } catch (e) {
      console.error("Failed to delete printer:", e);
      showToast('error', `Failed to delete printer: ${e instanceof Error ? e.message : e}`);
    }
  };

  const handleConnect = async (serial: string) => {
    const printer = printers.find(p => p.serial === serial);
    setConnecting(serial);
    try {
      await api.connectPrinter(serial);
      showToast('success', `Connected to "${printer?.name || serial}"`);
      // WebSocket will trigger loadPrinters() when printer_connected is received
      // Don't call loadPrinters() here to avoid double-reload and flickering
    } catch (e) {
      console.error("Failed to connect:", e);
      setConnecting(null);
      showToast('error', `Failed to connect: ${e instanceof Error ? e.message : e}`);
    }
  };

  const handleDisconnect = async (serial: string) => {
    const printer = printers.find(p => p.serial === serial);
    try {
      await api.disconnectPrinter(serial);
      showToast('success', `Disconnected from "${printer?.name || serial}"`);
    } catch (e) {
      console.error("Failed to disconnect:", e);
      showToast('error', `Failed to disconnect: ${e instanceof Error ? e.message : e}`);
    }
  };

  const handleAutoConnectToggle = async (serial: string, currentValue: boolean) => {
    try {
      await api.setAutoConnect(serial, !currentValue);
      await loadPrinters();
      showToast('success', `Auto-connect ${!currentValue ? 'enabled' : 'disabled'}`);
    } catch (e) {
      console.error("Failed to toggle auto-connect:", e);
      showToast('error', `Failed to toggle auto-connect: ${e instanceof Error ? e.message : e}`);
    }
  };

  // Get effective connection status (from WebSocket or API)
  const isConnected = (printer: Printer) => {
    return printerStatuses.get(printer.serial) ?? printer.connected ?? false;
  };

  return (
    <div class="space-y-6">
      {/* Header */}
      <div class="flex justify-between items-center">
        <div>
          <h1 class="text-3xl font-bold text-[var(--text-primary)]">Printers</h1>
          <p class="text-[var(--text-secondary)]">Manage your Bambu Lab printers</p>
        </div>
        <div class="flex gap-3">
          <button onClick={() => setShowDiscoverModal(true)} class="btn">
            <Search class="w-4 h-4" />
            <span>Discover</span>
          </button>
          <button onClick={() => setShowAddModal(true)} class="btn btn-primary">
            <Plus class="w-4 h-4" />
            <span>Add Printer</span>
          </button>
        </div>
      </div>

      {/* Printer list */}
      <div class="card">
        {loading ? (
          <div class="p-12 text-center text-[var(--text-muted)] flex items-center justify-center gap-2">
            <Loader2 class="w-5 h-5 animate-spin" />
            <span>Loading printers...</span>
          </div>
        ) : printers.length === 0 ? (
          <div class="flex flex-col items-center justify-center py-16 px-4">
            <div class="relative mb-6">
              <div class="absolute inset-0 -m-4 bg-[var(--accent-color)]/5 rounded-full blur-2xl" />
              <div class="relative flex items-center justify-center w-24 h-24 rounded-2xl bg-gradient-to-br from-[var(--bg-secondary)] to-[var(--bg-tertiary)] border border-[var(--border-color)] shadow-lg">
                <div class="absolute -top-1 -right-1 w-3 h-3 rounded-full bg-[var(--accent-color)]/30" />
                <div class="absolute -bottom-2 -left-2 w-2 h-2 rounded-full bg-[var(--accent-color)]/20" />
                <PrinterIcon class="w-10 h-10 text-[var(--text-muted)]" strokeWidth={1.5} />
              </div>
            </div>
            <h3 class="text-lg font-semibold text-[var(--text-primary)] mb-2 text-center">
              No printers yet
            </h3>
            <p class="text-sm text-[var(--text-muted)] text-center max-w-sm mb-6">
              Add your Bambu Lab printer to get started with automatic AMS configuration.
            </p>
            <button onClick={() => setShowAddModal(true)} class="btn btn-primary">
              <Plus class="w-4 h-4" />
              Add Your First Printer
            </button>
          </div>
        ) : (
          <ul class="divide-y divide-[var(--border-color)]">
            {printers.map((printer) => {
              const state = printerStates.get(printer.serial);
              const connected = isConnected(printer);
              // Use nozzle_count from state if available, otherwise fall back to model check
              const nozzleCount = state?.nozzle_count ?? 1;
              const numExtruders = nozzleCount >= 2 ? 2 : 1;
              const isExpanded = expandedPrinters.has(printer.serial);
              const hasDetails = connected && state && (state.ams_units?.length > 0 || state.vt_tray || (state.gcode_state && state.gcode_state !== "IDLE"));

              return (
                <li key={printer.serial} class="p-4 hover:bg-[var(--bg-tertiary)]/50 transition-colors">
                  <div
                    class="flex items-center justify-between cursor-pointer"
                    onClick={() => hasDetails && toggleExpanded(printer.serial)}
                  >
                    <div class="flex items-center gap-3">
                      {/* Expand/collapse icon */}
                      <div class="flex-shrink-0 w-5">
                        {hasDetails && (
                          <ChevronRight
                            class={`w-5 h-5 text-[var(--text-muted)] transition-transform ${isExpanded ? "rotate-90" : ""}`}
                          />
                        )}
                      </div>
                      {/* Printer icon */}
                      <div class={`flex-shrink-0 w-12 h-12 rounded-xl flex items-center justify-center ${
                        connected
                          ? "bg-[var(--success-color)]/10 text-[var(--success-color)]"
                          : "bg-[var(--bg-tertiary)] text-[var(--text-muted)]"
                      }`}>
                        <PrinterIcon class="w-6 h-6" strokeWidth={1.5} />
                      </div>
                      {/* Printer info */}
                      <div>
                        <p class="text-sm font-medium text-[var(--text-primary)]">
                          {printer.name || printer.serial}
                        </p>
                        <p class="text-sm text-[var(--text-secondary)]">
                          {printer.model || "Unknown Model"} &bull; {printer.ip_address || "No IP"}
                        </p>
                        <p class="text-xs text-[var(--text-muted)] font-mono">{printer.serial}</p>
                      </div>
                    </div>
                    <div class="flex items-center gap-3">
                      {/* Auto-connect toggle */}
                      <button
                        onClick={(e) => { e.stopPropagation(); handleAutoConnectToggle(printer.serial, printer.auto_connect ?? false); }}
                        class="flex items-center gap-2 text-sm"
                        title={printer.auto_connect ? "Disable auto-connect" : "Enable auto-connect"}
                      >
                        <span class={`relative inline-flex h-5 w-9 flex-shrink-0 cursor-pointer rounded-full transition-colors duration-200 ease-in-out ${
                          printer.auto_connect ? "bg-[var(--accent-color)]" : "bg-[var(--bg-tertiary)]"
                        }`}>
                          <span class={`pointer-events-none inline-block h-4 w-4 mt-0.5 transform rounded-full bg-white shadow transition duration-200 ease-in-out ${
                            printer.auto_connect ? "translate-x-4 ml-0.5" : "translate-x-0.5"
                          }`} />
                        </span>
                        <span class="text-xs text-[var(--text-muted)]">Auto</span>
                      </button>
                      {/* Status badge */}
                      {connecting === printer.serial ? (
                        <span class="inline-flex items-center gap-1.5 px-2.5 py-1 rounded-full text-xs font-medium bg-[var(--warning-color)]/10 text-[var(--warning-color)]">
                          <Loader2 class="w-3 h-3 animate-spin" />
                          Connecting...
                        </span>
                      ) : (
                        <span class={`inline-flex items-center gap-1.5 px-2.5 py-1 rounded-full text-xs font-medium ${
                          connected
                            ? "bg-[var(--success-color)]/10 text-[var(--success-color)]"
                            : "bg-[var(--bg-tertiary)] text-[var(--text-muted)]"
                        }`}>
                          {connected ? <Wifi class="w-3 h-3" /> : <WifiOff class="w-3 h-3" />}
                          {connected ? "Connected" : "Offline"}
                        </span>
                      )}
                      {/* Connect/Disconnect button */}
                      {!connected && connecting !== printer.serial && (
                        <button
                          onClick={(e) => { e.stopPropagation(); handleConnect(printer.serial); }}
                          class="btn btn-sm"
                        >
                          Connect
                        </button>
                      )}
                      {connected && (
                        <button
                          onClick={(e) => { e.stopPropagation(); handleDisconnect(printer.serial); }}
                          class="btn btn-sm btn-ghost"
                        >
                          Disconnect
                        </button>
                      )}
                      {/* Delete button */}
                      <button
                        onClick={(e) => { e.stopPropagation(); handleDelete(printer.serial); }}
                        class="p-2 text-[var(--text-muted)] hover:text-[var(--error-color)] hover:bg-[var(--error-color)]/10 rounded-lg transition-colors"
                        title="Delete printer"
                      >
                        <Trash2 class="w-4 h-4" />
                      </button>
                    </div>
                  </div>

                  {/* Expandable details section */}
                  {isExpanded && (
                    <>
                      {/* AMS Section - shown when connected */}
                      {connected && state && (
                        <div class="mt-4 pt-4 border-t border-[var(--border-color)]">
                          {/* Top row: Regular AMS units (id 0-3) */}
                          {state.ams_units && state.ams_units.filter(u => u.id <= 3).length > 0 && (
                            <div class="flex flex-wrap gap-3 mb-3">
                              {state.ams_units
                                .filter(unit => unit.id <= 3)
                                .sort((a, b) => a.id - b.id)
                                .map((unit) => (
                                  <AmsCard
                                    key={unit.id}
                                    unit={unit}
                                    printerModel={printer.model || undefined}
                                    numExtruders={numExtruders}
                                    printerSerial={printer.serial}
                                    calibrations={calibrations[printer.serial] || []}
                                    trayNow={state.tray_now}
                                    trayNowLeft={state.tray_now_left}
                                    trayNowRight={state.tray_now_right}
                                    activeExtruder={state.active_extruder}
                                    amsThresholds={amsThresholds}
                                    onHistoryClick={handleHistoryClick(printer.serial)}
                                  />
                                ))}
                            </div>
                          )}
                          {/* Bottom row: AMS-HT units (id 128+) and External slots */}
                          <div class="flex flex-wrap gap-3 items-start">
                            {state.ams_units && state.ams_units
                              .filter(unit => unit.id >= 128)
                              .sort((a, b) => a.id - b.id)
                              .map((unit) => (
                                <AmsCard
                                  key={unit.id}
                                  unit={unit}
                                  printerModel={printer.model || undefined}
                                  numExtruders={numExtruders}
                                  printerSerial={printer.serial}
                                  calibrations={calibrations[printer.serial] || []}
                                  trayNow={state.tray_now}
                                  trayNowLeft={state.tray_now_left}
                                  trayNowRight={state.tray_now_right}
                                  activeExtruder={state.active_extruder}
                                  amsThresholds={amsThresholds}
                                  onHistoryClick={handleHistoryClick(printer.serial)}
                                />
                              ))}
                            {/* External spool - Ext for single nozzle, Ext-L for dual nozzle */}
                            <ExternalSpool
                              tray={state.vt_tray}
                              position="left"
                              numExtruders={numExtruders}
                              printerSerial={printer.serial}
                              calibrations={calibrations[printer.serial] || []}
                            />
                            {/* Ext-R for dual-nozzle printers only */}
                            {numExtruders >= 2 && (
                              <ExternalSpool
                                tray={null}
                                position="right"
                                numExtruders={numExtruders}
                                printerSerial={printer.serial}
                                calibrations={calibrations[printer.serial] || []}
                              />
                            )}
                          </div>
                        </div>
                      )}

                      {/* Print status - shown when printing */}
                      {connected && state && state.gcode_state && state.gcode_state !== "IDLE" && (
                        <div class="mt-3 pt-3 border-t border-[var(--border-color)]">
                          <div class="flex gap-4">
                            {/* Thumbnail placeholder */}
                            <div class="flex-shrink-0 w-16 h-16 bg-[var(--bg-tertiary)] rounded-lg flex items-center justify-center">
                              <Image class="w-8 h-8 text-[var(--text-muted)]" strokeWidth={1.5} />
                            </div>

                            {/* Print info */}
                            <div class="flex-1 min-w-0">
                              <div class="flex items-center justify-between gap-2">
                                <span class="text-sm font-medium text-[var(--text-primary)] truncate">
                                  {state.subtask_name || "Printing"}
                                </span>
                                <span class={`text-xs px-2 py-0.5 rounded-full flex-shrink-0 ${
                                  state.gcode_state === "RUNNING" ? "bg-[var(--info-color)]/10 text-[var(--info-color)]" :
                                  state.gcode_state === "PAUSE" ? "bg-[var(--warning-color)]/10 text-[var(--warning-color)]" :
                                  state.gcode_state === "FINISH" ? "bg-[var(--success-color)]/10 text-[var(--success-color)]" :
                                  state.gcode_state === "FAILED" ? "bg-[var(--error-color)]/10 text-[var(--error-color)]" :
                                  "bg-[var(--bg-tertiary)] text-[var(--text-secondary)]"
                                }`}>
                                  {state.gcode_state}
                                </span>
                              </div>

                              {/* Progress bar */}
                              <div class="mt-1.5 flex items-center gap-2">
                                <div class="flex-1 bg-[var(--bg-tertiary)] rounded-full h-2">
                                  <div
                                    class="bg-[var(--accent-color)] h-2 rounded-full transition-all duration-300"
                                    style={{ width: `${state.print_progress ?? 0}%` }}
                                  />
                                </div>
                                <span class="text-sm font-medium text-[var(--text-primary)] w-10 text-right">
                                  {state.print_progress ?? 0}%
                                </span>
                              </div>

                              {/* Details row */}
                              <div class="mt-1.5 flex items-center gap-3 text-xs text-[var(--text-muted)]">
                                {state.layer_num !== null && state.total_layer_num !== null && (
                                  <span>Layer {state.layer_num}/{state.total_layer_num}</span>
                                )}
                                {state.mc_remaining_time !== null && state.mc_remaining_time > 0 && (
                                  <>
                                    <span>•</span>
                                    <span>{formatRemainingTime(state.mc_remaining_time)} left</span>
                                    <span>•</span>
                                    <span>ETA {calculateETA(state.mc_remaining_time)}</span>
                                  </>
                                )}
                              </div>
                            </div>
                          </div>
                        </div>
                      )}
                    </>
                  )}
                </li>
              );
            })}
          </ul>
        )}
      </div>

      {/* Info card */}
      <div class="bg-[var(--info-color)]/10 border border-[var(--info-color)]/30 rounded-lg p-4">
        <div class="flex gap-3">
          <Info class="w-5 h-5 text-[var(--info-color)] flex-shrink-0 mt-0.5" />
          <div>
            <h3 class="text-sm font-medium text-[var(--text-primary)]">Printer Connection</h3>
            <p class="mt-1 text-sm text-[var(--text-secondary)]">
              SpoolBuddy connects to your Bambu Lab printers via MQTT over your local network.
              You'll need the printer's serial number, IP address, and access code (found in the printer's network settings).
            </p>
          </div>
        </div>
      </div>

      {/* Add printer modal */}
      {showAddModal && (
        <AddPrinterModal
          onClose={() => {
            setShowAddModal(false);
            setSelectedDiscovered(null);
          }}
          onCreated={() => {
            setShowAddModal(false);
            setSelectedDiscovered(null);
            loadPrinters();
          }}
          prefill={selectedDiscovered}
        />
      )}

      {/* Discover printers modal */}
      {showDiscoverModal && (
        <DiscoverModal
          onClose={() => setShowDiscoverModal(false)}
          onSelect={(printer) => {
            setSelectedDiscovered(printer);
            setShowDiscoverModal(false);
            setShowAddModal(true);
          }}
          existingSerials={printers.map(p => p.serial)}
        />
      )}

      {/* Delete confirmation modal */}
      {deleteConfirm && (() => {
        const printerToDelete = printers.find(p => p.serial === deleteConfirm);
        return (
          <Modal
            isOpen={true}
            onClose={() => setDeleteConfirm(null)}
            title="Delete Printer"
            size="sm"
            footer={
              <>
                <button class="btn" onClick={() => setDeleteConfirm(null)}>
                  Cancel
                </button>
                <button class="btn btn-danger" onClick={confirmDelete}>
                  Delete
                </button>
              </>
            }
          >
            <p class="text-sm text-[var(--text-secondary)]">
              Are you sure you want to delete this printer? This action cannot be undone.
            </p>
            <div class="mt-3 p-3 bg-[var(--bg-tertiary)] rounded-lg">
              <p class="text-sm font-medium text-[var(--text-primary)]">
                {printerToDelete?.name || deleteConfirm}
              </p>
              {printerToDelete?.name && (
                <p class="text-xs font-mono text-[var(--text-muted)] mt-1">{deleteConfirm}</p>
              )}
            </div>
          </Modal>
        );
      })()}

      {/* AMS History Modal */}
      {historyModal && (
        <AMSHistoryModal
          printerSerial={historyModal.printerSerial}
          amsId={historyModal.amsId}
          amsLabel={historyModal.amsLabel}
          mode={historyModal.mode}
          thresholds={amsThresholds}
          onClose={() => setHistoryModal(null)}
        />
      )}
    </div>
  );
}

interface AddPrinterModalProps {
  onClose: () => void;
  onCreated: () => void;
  prefill?: DiscoveredPrinter | null;
}

// Try to detect model from printer name
function detectModelFromName(name: string): string | null {
  const lower = name.toLowerCase();

  // Check specific models first (longer matches before shorter)
  if (lower.includes("x1 carbon") || lower.includes("x1-carbon") || lower.includes("x1c")) return "X1-Carbon";
  if (lower.includes("x1e") || lower.includes("x1 e")) return "X1E";
  if (lower.includes("x1")) return "X1";
  if (lower.includes("a1 mini") || lower.includes("a1-mini") || lower.includes("a1mini")) return "A1-Mini";
  if (lower.includes("a1")) return "A1";
  if (lower.includes("p1s") || lower.includes("p1 s")) return "P1S";
  if (lower.includes("p1p") || lower.includes("p1 p")) return "P1P";
  if (lower.includes("p2s") || lower.includes("p2 s")) return "P2S";
  if (lower.includes("h2c") || lower.includes("h2 c")) return "H2C";
  if (lower.includes("h2d") || lower.includes("h2 d")) return "H2D";
  if (lower.includes("h2s") || lower.includes("h2 s")) return "H2S";

  return null;
}

function AddPrinterModal({ onClose, onCreated, prefill }: AddPrinterModalProps) {
  const { showToast } = useToast();

  // Try to get model from prefill, or detect from name
  const getInitialModel = () => {
    if (prefill?.model && prefill.model !== "Unknown") {
      return prefill.model;
    }
    // Fallback: detect from name
    if (prefill?.name) {
      const detected = detectModelFromName(prefill.name);
      if (detected) return detected;
    }
    return "";
  };

  const [serial, setSerial] = useState(prefill?.serial || "");
  const [name, setName] = useState(prefill?.name || "");
  const [model, setModel] = useState(getInitialModel());
  const [modelAutoDetected, setModelAutoDetected] = useState(!!getInitialModel());
  const [ipAddress, setIpAddress] = useState(prefill?.ip_address || "");
  const [accessCode, setAccessCode] = useState("");
  const [saving, setSaving] = useState(false);
  const [error, setError] = useState("");

  // Auto-detect model when name changes
  const handleNameChange = (newName: string) => {
    setName(newName);
    const detected = detectModelFromName(newName);
    if (detected && !modelAutoDetected) {
      setModel(detected);
      setModelAutoDetected(true);
    }
  };

  // Reset auto-detection flag when user manually changes model
  const handleModelChange = (newModel: string) => {
    setModel(newModel);
    setModelAutoDetected(false);
  };

  const handleSubmit = async (e: Event) => {
    e.preventDefault();
    setError("");

    if (!serial.trim()) {
      setError("Serial number is required");
      return;
    }
    if (!model) {
      setError("Please select a model");
      return;
    }
    if (!ipAddress.trim()) {
      setError("IP address is required");
      return;
    }
    if (!accessCode.trim()) {
      setError("Access code is required");
      return;
    }

    setSaving(true);

    try {
      await api.createPrinter({
        serial: serial.trim(),
        name: name.trim() || null,
        model: model || null,
        ip_address: ipAddress.trim(),
        access_code: accessCode.trim(),
      });
      showToast('success', `Added printer "${name.trim() || serial.trim()}"`);
      onCreated();
    } catch (e) {
      console.error("Failed to create printer:", e);
      const errorMsg = e instanceof Error ? e.message : "Failed to add printer";
      setError(errorMsg);
      showToast('error', errorMsg);
    } finally {
      setSaving(false);
    }
  };

  return (
    <Modal
      isOpen={true}
      onClose={onClose}
      title="Add Printer"
      size="md"
      footer={
        <>
          <button class="btn" onClick={onClose} disabled={saving}>
            Cancel
          </button>
          <button class="btn btn-primary" onClick={handleSubmit} disabled={saving}>
            {saving ? "Adding..." : "Add Printer"}
          </button>
        </>
      }
    >
      <div class="space-y-4">
        {error && (
          <div class="p-3 bg-[var(--error-color)]/10 border border-[var(--error-color)]/30 rounded-lg text-sm text-[var(--error-color)]">
            {error}
          </div>
        )}
        <div class="form-field">
          <label class="form-label">
            Serial Number <span class="text-[var(--error-color)]">*</span>
          </label>
          <input
            type="text"
            value={serial}
            onInput={(e) => setSerial((e.target as HTMLInputElement).value)}
            placeholder="e.g., 00M09A123456789"
            class="input font-mono"
          />
          <p class="mt-1 text-xs text-[var(--text-muted)]">
            Found in printer settings or on the label
          </p>
        </div>
        <div class="form-field">
          <label class="form-label">Name</label>
          <input
            type="text"
            value={name}
            onInput={(e) => handleNameChange((e.target as HTMLInputElement).value)}
            placeholder="e.g., My X1 Carbon"
            class="input"
          />
        </div>
        <div class="form-field">
          <label class="form-label">
            Model {modelAutoDetected && <span class="text-xs text-[var(--success-color)]">(auto-detected)</span>}
          </label>
          <select
            value={model}
            onChange={(e) => handleModelChange((e.target as HTMLSelectElement).value)}
            class="select"
          >
            <option value="" disabled>Select model...</option>
            <option value="A1">A1</option>
            <option value="A1-Mini">A1 Mini</option>
            <option value="H2C">H2C</option>
            <option value="H2D">H2D</option>
            <option value="H2S">H2S</option>
            <option value="P1P">P1P</option>
            <option value="P1S">P1S</option>
            <option value="P2S">P2S</option>
            <option value="X1">X1</option>
            <option value="X1-Carbon">X1 Carbon</option>
            <option value="X1E">X1E</option>
          </select>
        </div>
        <div class="form-field">
          <label class="form-label">
            IP Address <span class="text-[var(--error-color)]">*</span>
          </label>
          <input
            type="text"
            value={ipAddress}
            onInput={(e) => setIpAddress((e.target as HTMLInputElement).value)}
            placeholder="e.g., 192.168.1.100"
            class="input font-mono"
          />
          <p class="mt-1 text-xs text-[var(--text-muted)]">
            Found in printer's network settings
          </p>
        </div>
        <div class="form-field">
          <label class="form-label">
            Access Code <span class="text-[var(--error-color)]">*</span>
          </label>
          <input
            type="password"
            value={accessCode}
            onInput={(e) => setAccessCode((e.target as HTMLInputElement).value)}
            placeholder="8-digit code"
            class="input font-mono"
          />
          <p class="mt-1 text-xs text-[var(--text-muted)]">
            Found in printer's network settings (LAN Only Mode)
          </p>
        </div>
      </div>
    </Modal>
  );
}

interface DiscoverModalProps {
  onClose: () => void;
  onSelect: (printer: DiscoveredPrinter) => void;
  existingSerials: string[];
}

function DiscoverModal({ onClose, onSelect, existingSerials }: DiscoverModalProps) {
  const [discovering, setDiscovering] = useState(false);
  const [discovered, setDiscovered] = useState<DiscoveredPrinter[]>([]);
  const [error, setError] = useState("");

  // Filter out already-added printers
  const newPrinters = discovered.filter(p => !existingSerials.includes(p.serial));

  const startDiscovery = async () => {
    setError("");
    setDiscovered([]);
    setDiscovering(true);

    try {
      await api.startDiscovery();
      // Poll for discovered printers
      const pollInterval = setInterval(async () => {
        try {
          const printers = await api.getDiscoveredPrinters();
          setDiscovered(printers);
        } catch (e) {
          console.error("Failed to get discovered printers:", e);
        }
      }, 1000);

      // Stop after 10 seconds
      setTimeout(async () => {
        clearInterval(pollInterval);
        await api.stopDiscovery();
        setDiscovering(false);
        // Final fetch
        const printers = await api.getDiscoveredPrinters();
        setDiscovered(printers);
      }, 10000);
    } catch (e) {
      console.error("Failed to start discovery:", e);
      setError(e instanceof Error ? e.message : "Failed to start discovery");
      setDiscovering(false);
    }
  };

  useEffect(() => {
    startDiscovery();
    return () => {
      api.stopDiscovery();
    };
  }, []);

  return (
    <Modal
      isOpen={true}
      onClose={onClose}
      title="Discover Printers"
      size="md"
      footer={
        <button class="btn" onClick={onClose}>
          Close
        </button>
      }
    >
      {error && (
        <div class="p-3 bg-[var(--error-color)]/10 border border-[var(--error-color)]/30 rounded-lg text-sm text-[var(--error-color)] mb-4">
          {error}
        </div>
      )}

      {discovering && (
        <div class="flex items-center justify-center py-8 gap-3">
          <Loader2 class="w-8 h-8 text-[var(--accent-color)] animate-spin" />
          <span class="text-[var(--text-secondary)]">Scanning network for Bambu Lab printers...</span>
        </div>
      )}

      {!discovering && newPrinters.length === 0 && (
        <div class="text-center py-8">
          <Frown class="mx-auto w-12 h-12 text-[var(--text-muted)]" strokeWidth={1.5} />
          <p class="mt-3 text-sm text-[var(--text-secondary)]">No new printers found on your network.</p>
          <p class="text-xs text-[var(--text-muted)] mt-1">Make sure your printers are powered on and connected to the same network.</p>
          <button onClick={startDiscovery} class="btn btn-ghost mt-4">
            <RefreshCw class="w-4 h-4" />
            Scan Again
          </button>
        </div>
      )}

      {newPrinters.length > 0 && (
        <ul class="divide-y divide-[var(--border-color)]">
          {newPrinters.map((printer) => (
            <li
              key={printer.serial}
              class="py-3 flex items-center justify-between hover:bg-[var(--bg-tertiary)] cursor-pointer px-3 -mx-3 rounded-lg transition-colors"
              onClick={() => onSelect(printer)}
            >
              <div>
                <p class="text-sm font-medium text-[var(--text-primary)]">
                  {printer.name || printer.serial}
                </p>
                <p class="text-sm text-[var(--text-secondary)]">
                  {printer.model || "Unknown Model"} &bull; {printer.ip_address}
                </p>
                <p class="text-xs text-[var(--text-muted)] font-mono">{printer.serial}</p>
              </div>
              <ChevronRight class="w-5 h-5 text-[var(--text-muted)]" />
            </li>
          ))}
        </ul>
      )}

      {!discovering && newPrinters.length > 0 && (
        <div class="mt-4 text-center">
          <button onClick={startDiscovery} class="btn btn-ghost btn-sm">
            <RefreshCw class="w-4 h-4" />
            Scan Again
          </button>
        </div>
      )}
    </Modal>
  );
}
