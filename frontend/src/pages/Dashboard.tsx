import { useEffect, useState, useRef } from "preact/hooks";
import { Link } from "wouter-preact";
import { api, Spool, Printer, CloudAuthStatus } from "../lib/api";
import { useWebSocket } from "../lib/websocket";
import { Cloud, CloudOff, X, Download } from "lucide-preact";
import { SpoolIcon } from "../components/AmsCard";
import { AssignAmsModal } from "../components/AssignAmsModal";

// Storage keys for dashboard settings
const SPOOL_DISPLAY_DURATION_KEY = 'spoolbuddy-spool-display-duration';
const DEFAULT_CORE_WEIGHT_KEY = 'spoolbuddy-default-core-weight';

function getSpoolDisplayDuration(): number {
  try {
    const stored = localStorage.getItem(SPOOL_DISPLAY_DURATION_KEY);
    if (stored) {
      const seconds = parseInt(stored, 10);
      if (seconds >= 0 && seconds <= 300) return seconds;
    }
  } catch {
    // Ignore errors
  }
  return 10; // Default 10 seconds
}

function getDefaultCoreWeight(): number {
  try {
    const stored = localStorage.getItem(DEFAULT_CORE_WEIGHT_KEY);
    if (stored) {
      const weight = parseInt(stored, 10);
      if (weight >= 0 && weight <= 500) return weight;
    }
  } catch {
    // Ignore errors
  }
  return 250; // Default 250g (typical Bambu spool core)
}

export function Dashboard() {
  const [spools, setSpools] = useState<Spool[]>([]);
  const [printers, setPrinters] = useState<Printer[]>([]);
  const [loading, setLoading] = useState(true);
  const { deviceConnected, deviceUpdateAvailable, currentWeight, weightStable, currentTagId, printerStatuses, printerStates } = useWebSocket();
  const [cloudStatus, setCloudStatus] = useState<CloudAuthStatus | null>(null);
  const [cloudBannerDismissed, setCloudBannerDismissed] = useState(() => {
    return localStorage.getItem('spoolbuddy-cloud-banner-dismissed') === 'true';
  });
  const [showAssignModal, setShowAssignModal] = useState(false);
  const [assignModalSpool, setAssignModalSpool] = useState<Spool | null>(null);

  // Spool display timer state
  const [lastKnownSpool, setLastKnownSpool] = useState<Spool | null>(null);
  const [lastKnownWeight, setLastKnownWeight] = useState<number | null>(null);
  const [displayCountdown, setDisplayCountdown] = useState<number | null>(null);
  const timerRef = useRef<number | null>(null);
  // Refs to track spool/weight synchronously (avoid React state batching delays)
  const lastKnownSpoolRef = useRef<Spool | null>(null);
  const lastKnownWeightRef = useRef<number | null>(null);
  // Track if we've ever seen a tag in this session (prevents timer on initial load)
  const hadTagInSessionRef = useRef<boolean>(false);
  // Track the current tag ID to detect when switching to a different spool
  const currentTagIdRef = useRef<string | null>(null);
  // Debounce ref for tag removal (prevents timer bounce from flaky NFC reads)
  const tagRemovalDebounceRef = useRef<number | null>(null);
  const TAG_REMOVAL_DEBOUNCE_MS = 1500; // Wait 1.5s before considering tag truly removed

  // Find spool by tag_id in the loaded spools list
  const findSpoolByTagId = (tagId: string | null, spoolList: Spool[]): Spool | null => {
    if (!tagId) return null;
    return spoolList.find(s => s.tag_id === tagId) || null;
  };

  useEffect(() => {
    loadSpools();
    loadPrinters();
    loadCloudStatus();
  }, []);

  // Compute currentSpool directly from currentTagId and spools
  const computedSpool = currentTagId ? findSpoolByTagId(currentTagId, spools) : null;

  // Track weight while spool is on scale - only update when stable
  useEffect(() => {
    if (currentTagId && currentWeight !== null && weightStable) {
      // Spool is on scale with stable reading - save this weight
      lastKnownWeightRef.current = Math.round(Math.max(0, currentWeight));
    }
  }, [currentTagId, currentWeight, weightStable]);

  // Handle spool display timer when tag is removed (with debounce for flaky NFC)
  useEffect(() => {
    const previousTagId = currentTagIdRef.current;
    const isNewTag = currentTagId && currentTagId !== previousTagId;

    if (currentTagId) {
      // Update the current tag ref
      currentTagIdRef.current = currentTagId;

      // Tag detected - mark that we've had a tag in this session
      hadTagInSessionRef.current = true;

      // Cancel any pending tag removal debounce
      if (tagRemovalDebounceRef.current) {
        clearTimeout(tagRemovalDebounceRef.current);
        tagRemovalDebounceRef.current = null;
      }

      // Clear any running countdown timer
      if (timerRef.current) {
        clearInterval(timerRef.current);
        timerRef.current = null;
      }
      setDisplayCountdown(null);

      // If switching to a different tag, clear all old state
      if (isNewTag) {
        setLastKnownSpool(null);
        setLastKnownWeight(null);
        lastKnownSpoolRef.current = null;
        lastKnownWeightRef.current = null;
      }

      if (computedSpool) {
        // Save to both state and ref (ref is synchronous for next effect run)
        setLastKnownSpool(computedSpool);
        lastKnownSpoolRef.current = computedSpool;
      }
    } else if (hadTagInSessionRef.current && previousTagId && !tagRemovalDebounceRef.current) {
      // Tag removed - start debounce before starting timer
      const spoolFromRef = lastKnownSpoolRef.current;
      const weightFromRef = lastKnownWeightRef.current;
      const removedTagId = previousTagId;

      tagRemovalDebounceRef.current = window.setTimeout(() => {
        tagRemovalDebounceRef.current = null;

        // Only start timer if spoolFromRef matches the tag that was removed
        if (spoolFromRef && spoolFromRef.tag_id === removedTagId) {
          // Ensure state matches ref
          setLastKnownSpool(spoolFromRef);
          if (weightFromRef !== null) {
            setLastKnownWeight(weightFromRef);
          }
          // Start countdown
          const duration = getSpoolDisplayDuration();
          if (duration > 0) {
            setDisplayCountdown(duration);
            timerRef.current = window.setInterval(() => {
              setDisplayCountdown(prev => {
                if (prev === null || prev <= 1) {
                  // Timer expired - clear interval (state cleanup handled by effect below)
                  if (timerRef.current) {
                    clearInterval(timerRef.current);
                    timerRef.current = null;
                  }
                  return 0; // Set to 0 to trigger cleanup effect
                }
                return prev - 1;
              });
            }, 1000);
          } else {
            // Duration is 0 - clear immediately
            setLastKnownSpool(null);
            setLastKnownWeight(null);
            lastKnownSpoolRef.current = null;
            lastKnownWeightRef.current = null;
          }
        }
        // Clear the tag ref after processing removal
        currentTagIdRef.current = null;
      }, TAG_REMOVAL_DEBOUNCE_MS);
    }

    return () => {
      if (timerRef.current) {
        clearInterval(timerRef.current);
      }
      if (tagRemovalDebounceRef.current) {
        clearTimeout(tagRemovalDebounceRef.current);
      }
    };
  }, [currentTagId, computedSpool?.id]);

  // Clean up when countdown reaches 0
  useEffect(() => {
    if (displayCountdown === 0) {
      setLastKnownSpool(null);
      setLastKnownWeight(null);
      setDisplayCountdown(null);
      lastKnownSpoolRef.current = null;
      lastKnownWeightRef.current = null;
    }
  }, [displayCountdown]);

  // The displayed spool: current if tag present, or last known (shown immediately, cleared when timer expires)
  const displayedSpool = computedSpool || lastKnownSpool;
  const isShowingLastKnown = !currentTagId && lastKnownSpool !== null;

  // Track if we've updated weight for current spool to avoid duplicate updates
  const [weightUpdatedForSpool, setWeightUpdatedForSpool] = useState<string | null>(null);

  // Reset weight tracking when spool changes
  useEffect(() => {
    if (computedSpool?.id !== weightUpdatedForSpool) {
      setWeightUpdatedForSpool(null);
    }
  }, [computedSpool?.id]);

  // Update spool weight in backend when known spool is detected on scale
  useEffect(() => {
    // Only update weight once per spool detection, when weight is stable
    if (computedSpool && currentWeight !== null && weightStable && weightUpdatedForSpool !== computedSpool.id) {
      const newWeight = Math.round(Math.max(0, currentWeight));
      // Only update if weight is different (any change counts)
      if (computedSpool.weight_current === null || computedSpool.weight_current !== newWeight) {
        // Use setSpoolWeight to properly sync and reset consumed_since_weight
        api.setSpoolWeight(computedSpool.id, newWeight)
          .then(() => {
            // Mark as updated to prevent duplicate updates
            setWeightUpdatedForSpool(computedSpool.id);
            // Refresh spools list to keep it in sync
            loadSpools();
          })
          .catch(err => console.error('Failed to update spool weight:', err));
      } else {
        // Weight is same, just mark as processed
        setWeightUpdatedForSpool(computedSpool.id);
      }
    }
  }, [computedSpool?.id, currentWeight, weightStable, weightUpdatedForSpool]);

  const loadCloudStatus = async () => {
    try {
      const status = await api.getCloudStatus();
      setCloudStatus(status);
    } catch (e) {
      console.error("Failed to load cloud status:", e);
    }
  };

  const dismissCloudBanner = () => {
    setCloudBannerDismissed(true);
    localStorage.setItem('spoolbuddy-cloud-banner-dismissed', 'true');
  };

  const loadSpools = async () => {
    try {
      const data = await api.listSpools();
      setSpools(data);
    } catch (e) {
      console.error("Failed to load spools:", e);
    } finally {
      setLoading(false);
    }
  };

  const loadPrinters = async () => {
    try {
      const data = await api.listPrinters();
      setPrinters(data);
    } catch (e) {
      console.error("Failed to load printers:", e);
    }
  };

  // Get effective connection status for a printer
  const isPrinterConnected = (printer: Printer) => {
    return printerStatuses.get(printer.serial) ?? printer.connected ?? false;
  };

  // Get printer state info for display
  const getPrinterStateInfo = (printer: Printer) => {
    const connected = isPrinterConnected(printer);
    if (!connected) {
      return { status: "Offline", color: "text-[var(--text-muted)]", bgColor: "bg-[var(--bg-secondary)]" };
    }

    const state = printerStates.get(printer.serial);
    if (!state || !state.gcode_state) {
      return { status: "Connected", color: "text-green-500", bgColor: "bg-green-500/20" };
    }

    const gcodeState = state.gcode_state.toUpperCase();
    switch (gcodeState) {
      case "RUNNING":
        return {
          status: "Printing",
          color: "text-blue-400",
          bgColor: "bg-blue-500/20",
          progress: state.print_progress,
          jobName: state.subtask_name,
          remainingTime: state.mc_remaining_time
        };
      case "PAUSE":
        return {
          status: "Paused",
          color: "text-yellow-500",
          bgColor: "bg-yellow-500/20",
          progress: state.print_progress,
          jobName: state.subtask_name
        };
      case "FINISH":
        return { status: "Finished", color: "text-green-500", bgColor: "bg-green-500/20" };
      case "FAILED":
        return { status: "Failed", color: "text-red-500", bgColor: "bg-red-500/20" };
      case "PREPARE":
        return { status: "Preparing", color: "text-cyan-400", bgColor: "bg-cyan-500/20" };
      case "SLICING":
        return { status: "Slicing", color: "text-purple-400", bgColor: "bg-purple-500/20" };
      case "IDLE":
      default:
        return { status: "Idle", color: "text-green-500", bgColor: "bg-green-500/20" };
    }
  };

  // Calculate stats
  const totalSpools = spools.length;
  const materials = new Set(spools.map((s) => s.material)).size;
  const brands = new Set(spools.filter((s) => s.brand).map((s) => s.brand)).size;

  return (
    <div class="space-y-8">
      {/* Page header with stats bar */}
      <div class="flex flex-col gap-4">
        <div class="flex items-center justify-between">
          <h1 class="text-2xl font-bold text-[var(--text-primary)]">Dashboard</h1>
          <div class="flex items-center gap-3">
            {/* Cloud status indicator */}
            {cloudStatus && (
              <Link
                href="/settings"
                class={`flex items-center gap-2 px-3 py-1.5 rounded-full text-xs font-medium transition-all hover:scale-105 ${
                  cloudStatus.is_authenticated
                    ? "bg-green-500/15 text-green-500 hover:bg-green-500/25"
                    : "bg-[var(--bg-tertiary)] text-[var(--text-muted)] hover:bg-[var(--border-color)]"
                }`}
                title={cloudStatus.is_authenticated ? `Cloud: ${cloudStatus.email}` : "Cloud: Not connected"}
              >
                {cloudStatus.is_authenticated ? (
                  <Cloud class="w-3.5 h-3.5" />
                ) : (
                  <CloudOff class="w-3.5 h-3.5" />
                )}
                <span class="hidden sm:inline">
                  {cloudStatus.is_authenticated ? "Cloud" : "Offline"}
                </span>
              </Link>
            )}
            {/* Add spool button */}
            <Link href="/inventory?add=true" class="btn btn-primary btn-sm">
              <svg class="w-4 h-4" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 6v6m0 0v6m0-6h6m-6 0H6" />
              </svg>
              <span class="hidden sm:inline">Add Spool</span>
            </Link>
          </div>
        </div>

        {/* Compact stats bar */}
        <div class="flex items-center gap-6 px-4 py-3 bg-[var(--bg-secondary)] rounded-xl border border-[var(--border-color)]">
          <div class="flex items-center gap-2">
            <span class="text-2xl font-bold text-[var(--text-primary)]">{loading ? "—" : totalSpools}</span>
            <span class="text-sm text-[var(--text-muted)]">Spools</span>
          </div>
          <div class="w-px h-6 bg-[var(--border-color)]" />
          <div class="flex items-center gap-2">
            <span class="text-2xl font-bold text-[var(--text-primary)]">{loading ? "—" : materials}</span>
            <span class="text-sm text-[var(--text-muted)]">Materials</span>
          </div>
          <div class="w-px h-6 bg-[var(--border-color)]" />
          <div class="flex items-center gap-2">
            <span class="text-2xl font-bold text-[var(--text-primary)]">{loading ? "—" : brands}</span>
            <span class="text-sm text-[var(--text-muted)]">Brands</span>
          </div>
        </div>
      </div>

      {/* Cloud status banner */}
      {cloudStatus && !cloudStatus.is_authenticated && !cloudBannerDismissed && (
        <div class="flex items-center justify-between gap-4 p-4 bg-[var(--accent-color)]/10 border border-[var(--accent-color)]/20 rounded-xl">
          <div class="flex items-center gap-3">
            <div class="p-2 bg-[var(--accent-color)]/20 rounded-lg">
              <CloudOff class="w-5 h-5 text-[var(--accent-color)]" />
            </div>
            <div>
              <p class="text-sm font-medium text-[var(--text-primary)]">
                Connect to Bambu Cloud
              </p>
              <p class="text-xs text-[var(--text-secondary)]">
                Access your custom slicer presets
              </p>
            </div>
          </div>
          <div class="flex items-center gap-2">
            <Link href="/settings" class="btn btn-primary btn-sm">
              Connect
            </Link>
            <button
              onClick={dismissCloudBanner}
              class="p-1.5 text-[var(--text-muted)] hover:text-[var(--text-primary)] transition-colors rounded-lg hover:bg-[var(--bg-tertiary)]"
              title="Dismiss"
            >
              <X class="w-4 h-4" />
            </button>
          </div>
        </div>
      )}

      {/* Main content grid - Current Spool as hero + side panels */}
      <div class="grid grid-cols-1 lg:grid-cols-3 gap-6">

        {/* Left column: Device Status + Printers */}
        <div class="space-y-6 lg:col-span-1">

          {/* Device status - compact visual */}
          <div class="card p-5">
            <div class="flex items-center justify-between mb-4">
              <h2 class="text-sm font-semibold text-[var(--text-primary)] uppercase tracking-wide">Device</h2>
              {deviceUpdateAvailable && (
                <Link
                  href="/settings#updates"
                  class="flex items-center gap-1.5 px-2 py-1 rounded-md text-xs font-medium bg-blue-500/20 text-blue-400 hover:bg-blue-500/30 transition-colors"
                >
                  <Download class="w-3 h-3" />
                  Update
                </Link>
              )}
            </div>

            <div class="space-y-3">
              {/* Connection status */}
              <div class="flex items-center gap-3">
                <div class={`w-2.5 h-2.5 rounded-full ${deviceConnected ? 'bg-green-500 animate-pulse' : 'bg-red-500'}`} />
                <span class="text-sm text-[var(--text-secondary)]">
                  {deviceConnected ? "Connected" : "Disconnected"}
                </span>
              </div>

              {/* Scale weight */}
              <div class="flex items-center justify-between p-3 bg-[var(--bg-tertiary)] rounded-lg">
                <div class="flex items-center gap-2">
                  <svg class="w-4 h-4 text-[var(--text-muted)]" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                    <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M3 6l3 1m0 0l-3 9a5.002 5.002 0 006.001 0M6 7l3 9M6 7l6-2m6 2l3-1m-3 1l-3 9a5.002 5.002 0 006.001 0M18 7l3 9m-3-9l-6-2m0-2v2m0 16V5m0 16H9m3 0h3" />
                  </svg>
                  <span class="text-xs text-[var(--text-muted)]">Scale</span>
                </div>
                <div class="flex items-center gap-2">
                  <span class="text-lg font-mono font-semibold text-[var(--text-primary)]">
                    {currentWeight !== null ? `${Math.round(Math.max(0, currentWeight))}g` : '—'}
                  </span>
                  {weightStable && currentWeight !== null && (
                    <span class="w-2 h-2 rounded-full bg-green-500" title="Stable" />
                  )}
                </div>
              </div>

              {/* NFC status */}
              <div class="flex items-center justify-between p-3 bg-[var(--bg-tertiary)] rounded-lg">
                <div class="flex items-center gap-2">
                  <svg class="w-4 h-4 text-[var(--text-muted)]" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                    <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M7 7h.01M7 3h5c.512 0 1.024.195 1.414.586l7 7a2 2 0 010 2.828l-7 7a2 2 0 01-2.828 0l-7-7A2 2 0 013 12V7a4 4 0 014-4z" />
                  </svg>
                  <span class="text-xs text-[var(--text-muted)]">NFC</span>
                </div>
                <span class={`text-sm font-medium ${currentTagId ? 'text-green-500' : 'text-[var(--text-muted)]'}`}>
                  {currentTagId ? 'Tag detected' : 'No tag'}
                </span>
              </div>
            </div>
          </div>

          {/* Printers - compact list */}
          {printers.length > 0 && (
            <div class="card p-5">
              <div class="flex items-center justify-between mb-4">
                <h2 class="text-sm font-semibold text-[var(--text-primary)] uppercase tracking-wide">Printers</h2>
                <Link href="/printers" class="text-xs text-[var(--accent-color)] hover:underline">
                  View all
                </Link>
              </div>
              <div class="space-y-2">
                {printers.map((printer) => {
                  const stateInfo = getPrinterStateInfo(printer);
                  const hasProgress = stateInfo.progress !== undefined && stateInfo.progress !== null;

                  // Status color for left border
                  const borderColor = stateInfo.status === "Printing" ? "border-l-blue-500"
                    : stateInfo.status === "Paused" ? "border-l-yellow-500"
                    : stateInfo.status === "Idle" ? "border-l-green-500"
                    : stateInfo.status === "Offline" ? "border-l-gray-500"
                    : "border-l-green-500";

                  return (
                    <Link
                      key={printer.serial}
                      href="/printers"
                      class={`block p-3 bg-[var(--bg-tertiary)] rounded-lg border-l-3 ${borderColor} hover:bg-[var(--border-color)] transition-all hover:translate-x-0.5`}
                    >
                      <div class="flex items-center justify-between gap-2">
                        <div class="min-w-0 flex-1">
                          <p class="text-sm font-medium text-[var(--text-primary)] truncate">{printer.name || printer.serial}</p>
                          {hasProgress ? (
                            <div class="flex items-center gap-2 mt-1">
                              <div class="flex-1 h-1 bg-[var(--bg-secondary)] rounded-full overflow-hidden">
                                <div
                                  class={`h-full rounded-full ${stateInfo.status === "Paused" ? "bg-yellow-500" : "bg-blue-500"}`}
                                  style={{ width: `${stateInfo.progress}%` }}
                                />
                              </div>
                              <span class="text-xs text-[var(--text-muted)]">{stateInfo.progress}%</span>
                            </div>
                          ) : (
                            <p class="text-xs text-[var(--text-muted)]">{stateInfo.status}</p>
                          )}
                        </div>
                      </div>
                    </Link>
                  );
                })}
              </div>
            </div>
          )}
        </div>

        {/* Right column: Current Spool - HERO */}
        <div class="lg:col-span-2">
          <div class="card p-6 h-full">
            <div class="flex items-center justify-between mb-6">
              <h2 class="text-sm font-semibold text-[var(--text-primary)] uppercase tracking-wide">Current Spool</h2>
              {isShowingLastKnown && displayCountdown !== null && displayCountdown > 0 && (
                <span class="text-xs text-[var(--text-muted)] bg-[var(--bg-tertiary)] px-2.5 py-1 rounded-full">
                  {displayCountdown}s
                </span>
              )}
            </div>

            {displayedSpool ? (
              (() => {
                const grossWeight = isShowingLastKnown
                  ? (lastKnownWeight ?? lastKnownWeightRef.current ?? null)
                  : (currentWeight !== null ? Math.round(Math.max(0, currentWeight)) : null);
                const coreWeight = displayedSpool.core_weight && displayedSpool.core_weight > 0
                  ? displayedSpool.core_weight
                  : getDefaultCoreWeight();
                const remaining = grossWeight !== null
                  ? Math.round(Math.max(0, grossWeight - coreWeight))
                  : null;
                const labelWeight = Math.round(displayedSpool.label_weight || 1000);
                const fillPercent = remaining !== null ? Math.min(100, Math.round((remaining / labelWeight) * 100)) : null;
                const fillColor = fillPercent !== null
                  ? fillPercent > 50 ? '#22c55e' : fillPercent > 20 ? '#eab308' : '#ef4444'
                  : '#808080';

                return (
                  <div class="flex flex-col lg:flex-row items-center gap-8">
                    {/* Left: Large spool visualization */}
                    <div class="flex flex-col items-center">
                      <div class="relative">
                        <SpoolIcon
                          color={displayedSpool.rgba ? `#${displayedSpool.rgba.slice(0, 6)}` : '#808080'}
                          isEmpty={false}
                          size={120}
                        />
                        {/* Fill percentage badge */}
                        {fillPercent !== null && (
                          <div
                            class="absolute -bottom-2 -right-2 px-2.5 py-1 rounded-full text-xs font-bold text-white shadow-lg"
                            style={{ backgroundColor: fillColor }}
                          >
                            {fillPercent}%
                          </div>
                        )}
                      </div>
                    </div>

                    {/* Right: Spool details */}
                    <div class="flex-1 text-center lg:text-left space-y-4">
                      {/* Name and type */}
                      <div>
                        <h3 class="text-xl font-semibold text-[var(--text-primary)]">
                          {displayedSpool.color_name || "Unknown color"}
                        </h3>
                        <p class="text-sm text-[var(--text-secondary)]">
                          {displayedSpool.brand} • {displayedSpool.material}
                          {displayedSpool.subtype && ` ${displayedSpool.subtype}`}
                        </p>
                      </div>

                      {/* Weight info */}
                      {grossWeight !== null && (
                        <div class="space-y-3">
                          <div class="flex items-baseline gap-2 justify-center lg:justify-start">
                            <span class="text-4xl font-bold font-mono text-[var(--text-primary)]">{remaining}g</span>
                            <span class="text-sm text-[var(--text-muted)]">of {labelWeight}g</span>
                          </div>

                          {/* Fill bar */}
                          <div class="max-w-sm mx-auto lg:mx-0">
                            <div class="h-2.5 bg-[var(--bg-tertiary)] rounded-full overflow-hidden">
                              <div
                                class="h-full rounded-full transition-all duration-500"
                                style={{ width: `${fillPercent}%`, backgroundColor: fillColor }}
                              />
                            </div>
                          </div>
                        </div>
                      )}

                      {displayedSpool.location && (
                        <p class="text-sm text-[var(--text-muted)]">
                          📍 {displayedSpool.location}
                        </p>
                      )}

                      {/* Action buttons */}
                      <div class="flex flex-wrap gap-2 justify-center lg:justify-start pt-2">
                        <button
                          onClick={() => {
                            setAssignModalSpool(displayedSpool);
                            setShowAssignModal(true);
                          }}
                          class="btn btn-primary"
                        >
                          Assign to AMS
                        </button>
                        <Link
                          href={`/inventory?edit=${displayedSpool.id}`}
                          class="btn btn-secondary"
                        >
                          Edit
                        </Link>
                      </div>
                    </div>
                  </div>
                );
              })()
            ) : currentTagId ? (
              // New spool detected
              (() => {
                const defaultCoreWeight = getDefaultCoreWeight();
                const grossWeight = currentWeight !== null ? Math.round(Math.max(0, currentWeight)) : null;
                const estimatedRemaining = grossWeight !== null
                  ? Math.round(Math.max(0, grossWeight - defaultCoreWeight))
                  : null;

                return (
                  <div class="flex flex-col items-center justify-center py-8 text-center space-y-5">
                    <div class="w-20 h-20 rounded-2xl bg-[var(--accent-color)]/15 flex items-center justify-center">
                      <svg class="w-10 h-10 text-[var(--accent-color)]" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                        <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M7 7h.01M7 3h5c.512 0 1.024.195 1.414.586l7 7a2 2 0 010 2.828l-7 7a2 2 0 01-2.828 0l-7-7A2 2 0 013 12V7a4 4 0 014-4z" />
                      </svg>
                    </div>
                    <div>
                      <h3 class="text-lg font-semibold text-[var(--text-primary)]">New Spool Detected</h3>
                      <p class="text-sm text-[var(--text-muted)] font-mono mt-1">{currentTagId}</p>
                    </div>
                    {grossWeight !== null && (
                      <div class="text-sm text-[var(--text-secondary)]">
                        <span class="font-mono font-semibold">{grossWeight}g</span> on scale
                        {estimatedRemaining !== null && estimatedRemaining > 0 && (
                          <span class="text-[var(--text-muted)]"> • ~{estimatedRemaining}g filament</span>
                        )}
                      </div>
                    )}
                    <Link
                      href={`/inventory?add=true&tagId=${encodeURIComponent(currentTagId)}${currentWeight !== null ? `&weight=${Math.round(Math.max(0, currentWeight))}` : ''}`}
                      class="btn btn-primary"
                    >
                      <svg class="w-4 h-4" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                        <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 6v6m0 0v6m0-6h6m-6 0H6" />
                      </svg>
                      Add to Inventory
                    </Link>
                  </div>
                );
              })()
            ) : (
              // No tag - empty state
              <div class="flex flex-col items-center justify-center py-12 text-center">
                <div class="mb-6 opacity-40">
                  <SpoolIcon color="#808080" isEmpty={true} size={100} />
                </div>
                <p class="text-[var(--text-muted)] text-sm">
                  Place a spool on the scale to identify it
                </p>
              </div>
            )}
          </div>
        </div>
      </div>

      {/* Assign to AMS Modal */}
      {assignModalSpool && (
        <AssignAmsModal
          isOpen={showAssignModal}
          onClose={() => {
            setShowAssignModal(false);
            setAssignModalSpool(null);
          }}
          spool={assignModalSpool}
        />
      )}
    </div>
  );
}
