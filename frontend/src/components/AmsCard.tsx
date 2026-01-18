import { useState } from "preact/hooks";
import { AmsUnit, AmsTray } from "../lib/websocket";
import { api, CalibrationProfile, AMSThresholds } from "../lib/api";
import { Droplets, Thermometer } from "lucide-preact";

interface AmsCardProps {
  unit: AmsUnit;
  printerModel?: string;
  numExtruders?: number;
  printerSerial?: string;
  calibrations?: CalibrationProfile[];
  trayNow?: number | null; // Legacy single-nozzle: global tray index
  trayNowLeft?: number | null; // Dual-nozzle: loaded tray for left nozzle (extruder 1)
  trayNowRight?: number | null; // Dual-nozzle: loaded tray for right nozzle (extruder 0)
  activeExtruder?: number | null; // Currently printing extruder (0=right, 1=left)
  compact?: boolean; // Smaller size for secondary row
  amsThresholds?: AMSThresholds; // Thresholds for humidity/temperature coloring
  onHistoryClick?: (amsId: number, amsLabel: string, mode: 'humidity' | 'temperature') => void;
}

// Get active tray index within an AMS unit
// For dual-nozzle: only shows active if this unit's extruder is the currently printing one
// For single-nozzle: uses global tray_now index
function getActiveTrayInUnit(
  unit: AmsUnit,
  trayNow: number | null,
  trayNowLeft: number | null,
  trayNowRight: number | null,
  activeExtruder: number | null
): number | null {
  const amsId = unit.id;
  const unitExtruder = unit.extruder;

  // For dual-nozzle printers (when per-extruder values are available)
  // tray_now_left/right are "loaded" trays, activeExtruder tells us which is printing
  if (trayNowLeft !== null || trayNowRight !== null) {
    // If activeExtruder is unknown (-1 or null), don't show any tray as active
    if (activeExtruder === null || activeExtruder === -1) {
      return null;
    }

    // Only show active indicator if this unit's extruder matches the active one
    if (unitExtruder !== activeExtruder) {
      return null;
    }

    // Get the loaded tray for this extruder
    const loadedTray = unitExtruder === 0 ? trayNowRight : trayNowLeft;

    if (loadedTray === null || loadedTray === undefined || loadedTray === 255 || loadedTray >= 254) {
      return null;
    }

    // Check if this AMS unit contains the active tray (using global index)
    if (amsId <= 3) {
      // Regular AMS: global tray 0-3 = AMS 0, 4-7 = AMS 1, etc.
      const activeAmsId = Math.floor(loadedTray / 4);
      if (activeAmsId === amsId) {
        return loadedTray % 4;
      }
    } else if (amsId >= 128 && amsId <= 135) {
      // AMS-HT: global tray 16-23 maps to AMS-HT 128-135
      const htIndex = amsId - 128;
      if (loadedTray === 16 + htIndex) {
        return 0; // HT only has one slot
      }
    }
    return null;
  }

  // Legacy single-nozzle: use global tray_now
  if (trayNow === null || trayNow === undefined || trayNow === 255) return null;

  if (amsId <= 3) {
    // Regular AMS: tray_now 0-3 = AMS 0, 4-7 = AMS 1, etc.
    const activeAmsId = Math.floor(trayNow / 4);
    if (activeAmsId === amsId) {
      return trayNow % 4;
    }
  } else if (amsId >= 128 && amsId <= 135) {
    // AMS-HT: tray_now 16-23 maps to AMS-HT 128-135
    const htIndex = amsId - 128;
    if (trayNow === 16 + htIndex) {
      return 0; // HT only has one slot
    }
  }

  return null;
}

// Get AMS display name from ID
function getAmsName(amsId: number): string {
  if (amsId <= 3) {
    return `AMS ${String.fromCharCode(65 + amsId)}`; // A, B, C, D
  } else if (amsId >= 128 && amsId <= 135) {
    return `AMS HT ${String.fromCharCode(65 + amsId - 128)}`; // HT-A, HT-B, ...
  } else if (amsId === 255) {
    return "External";
  } else if (amsId === 254) {
    return "External L";
  }
  return `AMS ${amsId}`;
}

// Check if AMS is HT type (single slot per unit)
function isHtAms(amsId: number): boolean {
  return amsId >= 128 && amsId <= 135;
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

// Spool icon SVG - colored spool shape like OrcaSlicer (simple circle for AMS cards)
export function SpoolIcon({ color, isEmpty, size = 32 }: { color: string; isEmpty: boolean; size?: number }) {
  if (isEmpty) {
    return (
      <div
        class="rounded-full border-2 border-dashed border-[var(--text-muted)] flex items-center justify-center"
        style={{ width: size, height: size }}
      >
        <div class="w-2 h-2 rounded-full bg-[var(--text-muted)]" />
      </div>
    );
  }

  return (
    <svg width={size} height={size} viewBox="0 0 32 32">
      {/* Outer ring with white stroke for visibility */}
      <circle cx="16" cy="16" r="14" fill={color} stroke="white" strokeWidth="1.5" strokeOpacity="0.7" />
      {/* Inner shadow/depth */}
      <circle cx="16" cy="16" r="11" fill={color} style={{ filter: "brightness(0.85)" }} />
    </svg>
  );
}

// Detailed 3D spool icon for dashboard/detail views
export function DetailedSpoolIcon({ color, size = 64 }: { color: string; size?: number }) {
  return (
    <svg width={size} height={size} viewBox="0 0 64 64">
      {/* Left flange (ellipse) */}
      <ellipse
        cx="16"
        cy="32"
        rx="8"
        ry="26"
        fill="#e8e8e8"
        stroke="#888"
        strokeWidth="1"
      />
      {/* Left flange inner hole */}
      <ellipse
        cx="16"
        cy="32"
        rx="2"
        ry="4"
        fill="#666"
      />

      {/* Filament body (center) */}
      <rect
        x="16"
        y="8"
        width="32"
        height="48"
        fill={color}
        rx="2"
      />

      {/* Grid lines on filament */}
      <g stroke="#fff" strokeWidth="0.5" strokeOpacity="0.3">
        {/* Horizontal lines */}
        <line x1="16" y1="16" x2="48" y2="16" />
        <line x1="16" y1="24" x2="48" y2="24" />
        <line x1="16" y1="32" x2="48" y2="32" />
        <line x1="16" y1="40" x2="48" y2="40" />
        <line x1="16" y1="48" x2="48" y2="48" />
        {/* Vertical lines */}
        <line x1="24" y1="8" x2="24" y2="56" />
        <line x1="32" y1="8" x2="32" y2="56" />
        <line x1="40" y1="8" x2="40" y2="56" />
      </g>

      {/* Right flange (ellipse) - partial, showing 3D effect */}
      <ellipse
        cx="48"
        cy="32"
        rx="8"
        ry="26"
        fill="none"
        stroke="#888"
        strokeWidth="1"
      />
      {/* Right flange visible portion */}
      <path
        d="M 48 6 A 8 26 0 0 1 48 58"
        fill="#e8e8e8"
        stroke="#888"
        strokeWidth="1"
      />
      {/* Right flange inner edge */}
      <ellipse
        cx="48"
        cy="32"
        rx="2"
        ry="4"
        fill="#666"
      />

      {/* Highlight on filament */}
      <rect
        x="16"
        y="8"
        width="32"
        height="12"
        fill="url(#spoolHighlight)"
        rx="2"
      />

      {/* Gradient definitions */}
      <defs>
        <linearGradient id="spoolHighlight" x1="0%" y1="0%" x2="0%" y2="100%">
          <stop offset="0%" stopColor="white" stopOpacity="0.3" />
          <stop offset="100%" stopColor="white" stopOpacity="0" />
        </linearGradient>
      </defs>
    </svg>
  );
}

// Fill level indicator bar - always shown, dimmed if no data
function FillLevelBar({ remain }: { remain: number | null }) {
  const hasData = remain !== null && remain !== undefined && remain >= 0;
  const fillPercent = hasData ? remain : 0;
  const fillColor = hasData
    ? (remain! > 50 ? "#22c55e" : remain! > 20 ? "#f59e0b" : "#ef4444")
    : "transparent";

  return (
    <div class={`w-full h-1 rounded-full overflow-hidden mt-1 ${hasData ? 'bg-[var(--bg-tertiary)]' : 'bg-[var(--bg-tertiary)] opacity-30'}`}
         style={!hasData ? { backgroundImage: 'repeating-linear-gradient(45deg, transparent, transparent 2px, var(--text-muted) 2px, var(--text-muted) 4px)' } : {}}>
      <div
        class="h-full rounded-full transition-all"
        style={{ width: `${fillPercent}%`, backgroundColor: fillColor }}
      />
    </div>
  );
}

// Format humidity value - could be percentage (0-100) or index (1-5)
function formatHumidity(value: number | null): string {
  if (value === null || value === undefined) return "-";

  // If value > 5, it's likely a percentage from humidity_raw
  if (value > 5) {
    return `${value}%`;
  }

  // Otherwise it's an index (1-5), show approximate range
  const percentRanges: Record<number, string> = {
    1: "<20%",
    2: "20-40%",
    3: "40-60%",
    4: "60-80%",
    5: ">80%",
  };
  return percentRanges[value] || "-";
}

// Format temperature value
function formatTemperature(value: number | null): string {
  if (value === null || value === undefined) return "";
  return `${value.toFixed(1)}°C`;
}

// Get humidity color based on thresholds
function getHumidityColor(value: number | null, thresholds?: AMSThresholds): string {
  if (value === null || value === undefined) return "var(--text-muted)";
  const good = thresholds?.humidity_good ?? 40;
  const fair = thresholds?.humidity_fair ?? 60;
  // If value <= 5, it's an index - convert to approximate percentage
  const pct = value > 5 ? value : value * 20;
  if (pct <= good) return "#22c55e"; // green
  if (pct <= fair) return "#eab308"; // yellow
  return "#ef4444"; // red
}

// Get temperature color based on thresholds
function getTempColor(value: number | null, thresholds?: AMSThresholds): string {
  if (value === null || value === undefined) return "var(--text-muted)";
  const good = thresholds?.temp_good ?? 28;
  const fair = thresholds?.temp_fair ?? 35;
  if (value <= good) return "#22c55e"; // green
  if (value <= fair) return "#eab308"; // yellow
  return "#ef4444"; // red
}

// Humidity/Temperature indicator component
interface SensorIndicatorProps {
  humidity: number | null;
  temperature: number | null;
  thresholds?: AMSThresholds;
  amsId: number;
  amsLabel: string;
  onHistoryClick?: (amsId: number, amsLabel: string, mode: 'humidity' | 'temperature') => void;
}

function SensorIndicator({ humidity, temperature, thresholds, amsId, amsLabel, onHistoryClick }: SensorIndicatorProps) {
  const humidityStr = formatHumidity(humidity);
  const temperatureStr = formatTemperature(temperature);
  const humidityColor = getHumidityColor(humidity, thresholds);
  const tempColor = getTempColor(temperature, thresholds);
  const isClickable = !!onHistoryClick;

  return (
    <div class="flex items-center gap-2">
      {/* Humidity */}
      <button
        type="button"
        onClick={() => onHistoryClick?.(amsId, amsLabel, 'humidity')}
        class={`flex items-center gap-1 text-xs ${isClickable ? 'cursor-pointer hover:opacity-80 transition-opacity' : 'cursor-default'}`}
        title={`Humidity: ${humidityStr}${isClickable ? ' (click for history)' : ''}`}
        disabled={!isClickable}
      >
        <Droplets class="w-3.5 h-3.5" style={{ color: humidityColor }} />
        <span style={{ color: humidityColor }}>{humidityStr}</span>
      </button>

      {/* Temperature */}
      {temperatureStr && (
        <button
          type="button"
          onClick={() => onHistoryClick?.(amsId, amsLabel, 'temperature')}
          class={`flex items-center gap-1 text-xs ${isClickable ? 'cursor-pointer hover:opacity-80 transition-opacity' : 'cursor-default'}`}
          title={`Temperature: ${temperatureStr}${isClickable ? ' (click for history)' : ''}`}
          disabled={!isClickable}
        >
          <Thermometer class="w-3.5 h-3.5" style={{ color: tempColor }} />
          <span style={{ color: tempColor }}>{temperatureStr}</span>
        </button>
      )}
    </div>
  );
}

// Slot action menu component
interface SlotMenuProps {
  printerSerial: string;
  amsId: number;
  trayId: number;
  calibrations: CalibrationProfile[];
  currentKValue: number | null;
}

function SlotMenu({ printerSerial, amsId, trayId, calibrations, currentKValue }: SlotMenuProps) {
  const [isOpen, setIsOpen] = useState(false);
  const [loading, setLoading] = useState(false);

  const handleReset = async (e: Event) => {
    e.stopPropagation();
    setLoading(true);
    try {
      await api.resetSlot(printerSerial, amsId, trayId);
    } catch (err) {
      console.error("Failed to reset slot:", err);
    } finally {
      setLoading(false);
      setIsOpen(false);
    }
  };

  const handleSetCalibration = async (caliIdx: number, filamentId: string = "") => {
    setLoading(true);
    try {
      await api.setCalibration(printerSerial, amsId, trayId, {
        cali_idx: caliIdx,
        filament_id: filamentId,
      });
    } catch (err) {
      console.error("Failed to set calibration:", err);
    } finally {
      setLoading(false);
      setIsOpen(false);
    }
  };

  return (
    <div class="relative inline-block">
      <button
        onClick={(e) => {
          e.stopPropagation();
          setIsOpen(!isOpen);
        }}
        class="p-1 rounded hover:bg-[var(--bg-tertiary)] text-[var(--text-muted)] hover:text-[var(--text-primary)] transition-colors"
        title="Slot options"
        disabled={loading}
      >
        {loading ? (
          <svg class="w-4 h-4 animate-spin" fill="none" viewBox="0 0 24 24">
            <circle class="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" stroke-width="4" />
            <path class="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4z" />
          </svg>
        ) : (
          <svg class="w-4 h-4" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 5v.01M12 12v.01M12 19v.01M12 6a1 1 0 110-2 1 1 0 010 2zm0 7a1 1 0 110-2 1 1 0 010 2zm0 7a1 1 0 110-2 1 1 0 010 2z" />
          </svg>
        )}
      </button>

      {isOpen && (
        <>
          {/* Backdrop to close menu */}
          <div
            class="fixed inset-0 z-40 bg-black/50"
            onClick={() => setIsOpen(false)}
          />
          {/* Menu - fixed position in center of screen for visibility */}
          <div class="fixed left-1/2 top-1/2 transform -translate-x-1/2 -translate-y-1/2 z-50 bg-[var(--bg-secondary)] rounded-lg shadow-xl border border-[var(--border-color)] py-2 min-w-[240px] max-h-[80vh] overflow-y-auto">
            <div class="px-4 py-2 border-b border-[var(--border-color)] font-medium text-[var(--text-primary)]">
              Slot {trayId + 1} Options
            </div>

            <button
              onClick={handleReset}
              class="w-full px-4 py-3 text-left text-sm text-[var(--text-secondary)] hover:bg-[var(--bg-tertiary)] flex items-center gap-3"
            >
              <svg class="w-5 h-5 text-[var(--text-muted)]" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15" />
              </svg>
              Re-read RFID Tag
            </button>

            <div class="border-t border-[var(--border-color)] my-2" />

            <div class="px-4 py-2 text-xs text-[var(--text-muted)] font-medium uppercase">K-Profile Selection</div>

            <button
              onClick={() => handleSetCalibration(-1)}
              class={`w-full px-4 py-3 text-left text-sm hover:bg-[var(--bg-tertiary)] flex items-center justify-between ${
                currentKValue === null || currentKValue === 0.02 ? "text-[var(--accent-color)] bg-[var(--accent-color)]/10" : "text-[var(--text-secondary)]"
              }`}
            >
              <span>Default (K = 0.020)</span>
              {(currentKValue === null || currentKValue === 0.02) && (
                <svg class="w-5 h-5" fill="currentColor" viewBox="0 0 20 20">
                  <path fill-rule="evenodd" d="M16.707 5.293a1 1 0 010 1.414l-8 8a1 1 0 01-1.414 0l-4-4a1 1 0 011.414-1.414L8 12.586l7.293-7.293a1 1 0 011.414 0z" clip-rule="evenodd" />
                </svg>
              )}
            </button>

            {calibrations.length > 0 && (
              <>
                {calibrations.map((cal) => (
                  <button
                    key={cal.cali_idx}
                    onClick={() => handleSetCalibration(cal.cali_idx, cal.filament_id)}
                    class={`w-full px-4 py-3 text-left text-sm hover:bg-[var(--bg-tertiary)] flex items-center justify-between ${
                      currentKValue !== null && Math.abs(currentKValue - cal.k_value) < 0.001 ? "text-[var(--accent-color)] bg-[var(--accent-color)]/10" : "text-[var(--text-secondary)]"
                    }`}
                    title={cal.name || cal.filament_id}
                  >
                    <span class="truncate mr-2">{cal.name || cal.filament_id || `Profile ${cal.cali_idx}`}</span>
                    <span class="text-[var(--text-muted)] flex-shrink-0 font-mono text-xs">K = {cal.k_value.toFixed(3)}</span>
                  </button>
                ))}
              </>
            )}

            <div class="border-t border-[var(--border-color)] mt-2 pt-2">
              <button
                onClick={() => setIsOpen(false)}
                class="w-full px-4 py-2 text-sm text-[var(--text-muted)] hover:bg-[var(--bg-tertiary)]"
              >
                Cancel
              </button>
            </div>
          </div>
        </>
      )}
    </div>
  );
}

// Regular AMS card (4 slots)
function RegularAmsCard({ unit, numExtruders = 1, printerSerial, calibrations = [], trayNow, trayNowLeft, trayNowRight, activeExtruder, amsThresholds, onHistoryClick }: AmsCardProps) {
  const amsName = getAmsName(unit.id);

  // Get active tray for this AMS unit (handles both single and dual-nozzle)
  const activeTrayIdx = getActiveTrayInUnit(unit, trayNow ?? null, trayNowLeft ?? null, trayNowRight ?? null, activeExtruder ?? null);

  // Get nozzle label for multi-nozzle printers
  // extruder 0 = Right nozzle, extruder 1 = Left nozzle (per SpoolEase/Bambu convention)
  const isDualNozzle = (numExtruders ?? 1) >= 2;
  const nozzleLabel = isDualNozzle && unit.extruder !== undefined && unit.extruder !== null
    ? (unit.extruder === 0 ? "R" : "L")
    : null;

  // Build slots array (4 slots for regular AMS)
  const slots: (AmsTray | undefined)[] = [undefined, undefined, undefined, undefined];
  const sortedTrays = [...unit.trays].sort((a, b) => a.tray_id - b.tray_id);
  sortedTrays.forEach(tray => {
    if (tray.tray_id >= 0 && tray.tray_id < 4) {
      slots[tray.tray_id] = tray;
    }
  });

  const hasControls = !!printerSerial;

  return (
    <div class="relative bg-[var(--bg-secondary)] border border-[var(--border-color)] rounded-lg overflow-hidden" style={{ width: 280 }}>
      {/* Header */}
      <div class="flex items-center justify-between px-3 py-2 bg-[var(--bg-tertiary)]">
        <div class="flex items-center gap-2">
          <span class="text-sm font-medium text-[var(--text-primary)]">{amsName}</span>
          {nozzleLabel && (
            <span class={`px-1.5 py-0.5 text-xs rounded ${
              nozzleLabel === "L" ? "bg-blue-600 text-white" : "bg-purple-600 text-white"
            }`}>
              {nozzleLabel}
            </span>
          )}
        </div>
        <SensorIndicator
          humidity={unit.humidity}
          temperature={unit.temperature}
          thresholds={amsThresholds}
          amsId={unit.id}
          amsLabel={amsName}
          onHistoryClick={onHistoryClick}
        />
      </div>

      {/* AMS unit image with spools overlaid */}
      <div class="relative ams-image-theme">
        <img
          src="/images/ams/ams.png"
          alt="AMS"
          class="w-full"
        />
        {/* Spool icons overlaid on top of AMS slots - positioned at exact slot locations */}
        {slots.map((tray, idx) => {
          const isEmpty = !tray || isTrayEmpty(tray);
          const color = tray ? trayColorToCSS(tray.tray_color) : "#808080";
          const isActive = activeTrayIdx === idx;
          // Slot positions as percentage from left (measured from AMS image slot centers)
          const slotPositions = [21, 40, 60, 79];
          return (
            <div
              key={idx}
              class="absolute flex flex-col items-center"
              style={{ left: `${slotPositions[idx]}%`, top: "6%", transform: "translateX(-50%)" }}
            >
              <div
                class={`rounded-full ${isActive ? "ring-2 ring-[var(--accent-color)] ring-offset-1 ring-offset-transparent" : ""}`}
                style={{ filter: "drop-shadow(0 0 3px rgba(255,255,255,0.8))" }}
              >
                <SpoolIcon color={color} isEmpty={isEmpty} size={36} />
              </div>
              {isActive && (
                <div class="w-1.5 h-1.5 rounded-full bg-[var(--accent-color)] mt-0.5" title="Active" />
              )}
            </div>
          );
        })}
      </div>

      {/* Material labels with K value and fill level */}
      <div class="flex justify-around px-2 py-2 bg-[var(--bg-secondary)]">
        {slots.map((tray, idx) => {
          const isEmpty = !tray || isTrayEmpty(tray);
          const material = tray?.tray_type || "";
          const kValue = tray?.k_value;
          const remain = tray?.remain;
          return (
            <div key={idx} class="flex flex-col items-center" style={{ width: 56 }}>
              {/* Slot menu button - centered */}
              {hasControls && (
                <div class="mb-0.5">
                  <SlotMenu
                    printerSerial={printerSerial!}
                    amsId={unit.id}
                    trayId={idx}
                    calibrations={calibrations}
                    currentKValue={kValue ?? null}
                  />
                </div>
              )}
              <span
                class={`text-xs font-medium truncate text-center ${isEmpty ? "text-[var(--text-muted)]" : "text-[var(--text-primary)]"}`}
                style={{ maxWidth: 56 }}
                title={material}
              >
                {isEmpty ? "-" : material}
              </span>
              <span class="text-[10px] text-[var(--text-muted)]" title="K value (pressure advance)">
                {!isEmpty ? `K ${(kValue ?? 0.020).toFixed(3)}` : "\u00A0"}
              </span>
              <FillLevelBar remain={isEmpty ? null : remain ?? null} />
              {!isEmpty && remain !== null && remain !== undefined && remain >= 0 && (
                <span class="text-[10px] text-[var(--text-muted)]">{remain}%</span>
              )}
            </div>
          );
        })}
      </div>
    </div>
  );
}

// HT AMS card (single slot) - 50% width of regular AMS
function HtAmsCard({ unit, numExtruders = 1, printerSerial, calibrations = [], trayNow, trayNowLeft, trayNowRight, activeExtruder, amsThresholds, onHistoryClick }: AmsCardProps) {
  const amsName = getAmsName(unit.id);
  const tray = unit.trays[0];
  const isEmpty = !tray || isTrayEmpty(tray);
  const color = tray ? trayColorToCSS(tray.tray_color) : "#808080";
  const material = tray?.tray_type || "";
  const kValue = tray?.k_value;
  const remain = tray?.remain;

  // Get nozzle label for multi-nozzle printers
  const isDualNozzle = (numExtruders ?? 1) >= 2;
  const nozzleLabel = isDualNozzle && unit.extruder !== undefined && unit.extruder !== null
    ? (unit.extruder === 0 ? "R" : "L")
    : null;
  const hasControls = !!printerSerial;
  const isActive = getActiveTrayInUnit(unit, trayNow ?? null, trayNowLeft ?? null, trayNowRight ?? null, activeExtruder ?? null) === 0;

  return (
    <div class="relative bg-[var(--bg-secondary)] border border-[var(--border-color)] rounded-lg overflow-hidden flex-1 min-w-[180px] max-w-[280px]">
      {/* Header */}
      <div class="flex items-center justify-between px-3 py-2 bg-[var(--bg-tertiary)]">
        <div class="flex items-center gap-1.5">
          <span class="text-sm font-medium text-[var(--text-primary)]">{amsName.replace("AMS ", "")}</span>
          {nozzleLabel && (
            <span class={`px-1.5 py-0.5 text-[10px] rounded ${
              nozzleLabel === "L" ? "bg-blue-600 text-white" : "bg-purple-600 text-white"
            }`}>
              {nozzleLabel}
            </span>
          )}
        </div>
        <SensorIndicator
          humidity={unit.humidity}
          temperature={unit.temperature}
          thresholds={amsThresholds}
          amsId={unit.id}
          amsLabel={amsName}
          onHistoryClick={onHistoryClick}
        />
      </div>

      {/* Spool icon and info */}
      <div class="flex gap-3 p-3 bg-[var(--bg-primary)]">
        {/* Left: Spool icon */}
        <div class="flex flex-col items-center">
          <div class={`rounded-full p-0.5 ${isActive ? "ring-2 ring-[var(--accent-color)] ring-offset-1 ring-offset-[var(--bg-primary)]" : ""}`}>
            <SpoolIcon color={color} isEmpty={isEmpty} size={48} />
          </div>
          {isActive && (
            <div class="w-1.5 h-1.5 rounded-full bg-[var(--accent-color)] mt-1" title="Active" />
          )}
        </div>

        {/* Right: Material info */}
        <div class="flex-1 flex flex-col justify-center min-w-0">
          <div class="flex items-center justify-between gap-2">
            <span
              class={`text-sm font-medium truncate ${isEmpty ? "text-[var(--text-muted)]" : "text-[var(--text-primary)]"}`}
              title={material}
            >
              {isEmpty ? "Empty" : material}
            </span>
            {hasControls && (
              <SlotMenu
                printerSerial={printerSerial!}
                amsId={unit.id}
                trayId={0}
                calibrations={calibrations}
                currentKValue={kValue ?? null}
              />
            )}
          </div>
          {!isEmpty && kValue !== null && kValue !== undefined && (
            <span class="text-xs text-[var(--text-muted)]" title="K value (pressure advance)">
              K {kValue.toFixed(3)}
            </span>
          )}
          <div class="mt-1.5">
            <FillLevelBar remain={isEmpty ? null : remain ?? null} />
            {!isEmpty && remain !== null && remain !== undefined && remain >= 0 && (
              <span class="text-[10px] text-[var(--text-muted)]">{remain}%</span>
            )}
          </div>
        </div>
      </div>
    </div>
  );
}

export function AmsCard({ unit, printerModel, numExtruders = 1, printerSerial, calibrations = [], trayNow, trayNowLeft, trayNowRight, activeExtruder, amsThresholds, onHistoryClick }: AmsCardProps) {
  const isHt = isHtAms(unit.id);

  if (isHt) {
    return <HtAmsCard unit={unit} printerModel={printerModel} numExtruders={numExtruders} printerSerial={printerSerial} calibrations={calibrations} trayNow={trayNow} trayNowLeft={trayNowLeft} trayNowRight={trayNowRight} activeExtruder={activeExtruder} amsThresholds={amsThresholds} onHistoryClick={onHistoryClick} />;
  }

  return <RegularAmsCard unit={unit} printerModel={printerModel} numExtruders={numExtruders} printerSerial={printerSerial} calibrations={calibrations} trayNow={trayNow} trayNowLeft={trayNowLeft} trayNowRight={trayNowRight} activeExtruder={activeExtruder} amsThresholds={amsThresholds} onHistoryClick={onHistoryClick} />;
}

// External spool holder (Virtual Tray) - 50% width of regular AMS
interface ExternalSpoolProps {
  tray: AmsTray | null;
  position?: "left" | "right";  // left = Ext-1/L nozzle, right = Ext-2/R nozzle
  numExtruders?: number;
  printerSerial?: string;
  calibrations?: CalibrationProfile[];
}

export function ExternalSpool({ tray, position = "left", numExtruders = 1, printerSerial, calibrations = [] }: ExternalSpoolProps) {
  const isEmpty = !tray || isTrayEmpty(tray);
  const color = tray ? trayColorToCSS(tray.tray_color) : "#808080";
  const material = tray?.tray_type || "";

  // For single extruder: just "Ext"
  // For multi-nozzle: "Ext-L" for left nozzle, "Ext-R" for right nozzle
  const isDualNozzle = numExtruders >= 2;
  const label = isDualNozzle
    ? (position === "left" ? "Ext-L" : "Ext-R")
    : "Ext";
  const nozzleLabel = isDualNozzle ? (position === "left" ? "L" : "R") : null;

  const kValue = tray?.k_value;
  const remain = tray?.remain;
  const hasControls = !!printerSerial;
  // External tray uses ams_id 255 (or 254 for left on dual nozzle)
  const amsId = numExtruders === 1 ? 255 : (position === "left" ? 254 : 255);

  return (
    <div class="bg-[var(--bg-secondary)] border border-[var(--border-color)] rounded-lg overflow-hidden flex-1 min-w-[180px] max-w-[280px]">
      {/* Header */}
      <div class="flex items-center justify-between px-3 py-2 bg-[var(--bg-tertiary)]">
        <div class="flex items-center gap-1.5">
          <span class="text-sm font-medium text-[var(--text-primary)]">{label}</span>
          {nozzleLabel && (
            <span class={`px-1.5 py-0.5 text-[10px] rounded ${
              nozzleLabel === "L" ? "bg-blue-600 text-white" : "bg-purple-600 text-white"
            }`}>
              {nozzleLabel}
            </span>
          )}
        </div>
      </div>

      {/* Spool icon and info */}
      <div class="flex gap-3 p-3 bg-[var(--bg-primary)]">
        {/* Left: Spool icon */}
        <div class="flex flex-col items-center">
          <SpoolIcon color={color} isEmpty={isEmpty} size={48} />
        </div>

        {/* Right: Material info */}
        <div class="flex-1 flex flex-col justify-center min-w-0">
          <div class="flex items-center justify-between gap-2">
            <span
              class={`text-sm font-medium truncate ${isEmpty ? "text-[var(--text-muted)]" : "text-[var(--text-primary)]"}`}
              title={material}
            >
              {isEmpty ? "Empty" : material}
            </span>
            {hasControls && (
              <SlotMenu
                printerSerial={printerSerial!}
                amsId={amsId}
                trayId={0}
                calibrations={calibrations}
                currentKValue={kValue ?? null}
              />
            )}
          </div>
          {!isEmpty && kValue !== null && kValue !== undefined && (
            <span class="text-xs text-[var(--text-muted)]" title="K value (pressure advance)">
              K {kValue.toFixed(3)}
            </span>
          )}
          <div class="mt-1.5">
            <FillLevelBar remain={isEmpty ? null : remain ?? null} />
            {!isEmpty && remain !== null && remain !== undefined && remain >= 0 && (
              <span class="text-[10px] text-[var(--text-muted)]">{remain}%</span>
            )}
          </div>
        </div>
      </div>
    </div>
  );
}
