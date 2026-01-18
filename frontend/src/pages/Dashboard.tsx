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

  // Format remaining time
  const formatRemainingTime = (minutes: number | null | undefined): string | null => {
    if (minutes === null || minutes === undefined || minutes <= 0) return null;
    if (minutes < 60) return `${minutes}m`;
    const hours = Math.floor(minutes / 60);
    const mins = minutes % 60;
    return mins > 0 ? `${hours}h ${mins}m` : `${hours}h`;
  };

  // Calculate stats
  const totalSpools = spools.length;
  const materials = new Set(spools.map((s) => s.material)).size;
  const brands = new Set(spools.filter((s) => s.brand).map((s) => s.brand)).size;

  return (
    <div class="space-y-6">
      {/* Page header */}
      <div class="flex items-start justify-between">
        <div>
          <h1 class="text-3xl font-bold text-[var(--text-primary)]">Dashboard</h1>
          <p class="text-[var(--text-secondary)]">Overview of your filament inventory</p>
        </div>
        {/* Cloud status indicator */}
        {cloudStatus && (
          <Link
            href="/settings"
            class={`flex items-center gap-2 px-3 py-1.5 rounded-full text-sm font-medium transition-colors ${
              cloudStatus.is_authenticated
                ? "bg-green-500/20 text-green-500 hover:bg-green-500/30"
                : "bg-[var(--bg-tertiary)] text-[var(--text-muted)] hover:bg-[var(--border-color)]"
            }`}
            title={cloudStatus.is_authenticated ? `Cloud: ${cloudStatus.email}` : "Cloud: Not connected"}
          >
            {cloudStatus.is_authenticated ? (
              <Cloud class="w-4 h-4" />
            ) : (
              <CloudOff class="w-4 h-4" />
            )}
            <span class="hidden sm:inline">
              {cloudStatus.is_authenticated ? "Cloud Connected" : "Cloud Offline"}
            </span>
          </Link>
        )}
      </div>

      {/* Cloud status banner */}
      {cloudStatus && !cloudStatus.is_authenticated && !cloudBannerDismissed && (
        <div class="flex items-center justify-between gap-4 p-4 bg-[var(--accent-color)]/10 border border-[var(--accent-color)]/30 rounded-lg">
          <div class="flex items-center gap-3">
            <CloudOff class="w-5 h-5 text-[var(--accent-color)]" />
            <div>
              <p class="text-sm font-medium text-[var(--text-primary)]">
                Connect to Bambu Cloud for custom filament presets
              </p>
              <p class="text-xs text-[var(--text-secondary)]">
                Login in Settings to access your custom slicer presets when adding spools
              </p>
            </div>
          </div>
          <div class="flex items-center gap-2">
            <Link href="/settings" class="btn btn-primary text-sm px-3 py-1.5">
              <Cloud class="w-4 h-4" />
              Connect
            </Link>
            <button
              onClick={dismissCloudBanner}
              class="p-1.5 text-[var(--text-muted)] hover:text-[var(--text-primary)] transition-colors"
              title="Dismiss"
            >
              <X class="w-4 h-4" />
            </button>
          </div>
        </div>
      )}

      {/* Stats cards */}
      <div class="grid grid-cols-1 md:grid-cols-3 gap-6">
        <div class="card p-6">
          <div class="flex items-center">
            <div class="p-3 bg-blue-500/20 rounded-full">
              <svg class="w-8 h-8 text-blue-500" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M19 11H5m14 0a2 2 0 012 2v6a2 2 0 01-2 2H5a2 2 0 01-2-2v-6a2 2 0 012-2m14 0V9a2 2 0 00-2-2M5 11V9a2 2 0 012-2m0 0V5a2 2 0 012-2h6a2 2 0 012 2v2M7 7h10" />
              </svg>
            </div>
            <div class="ml-4">
              <p class="text-sm font-medium text-[var(--text-muted)]">Total Spools</p>
              <p class="text-2xl font-semibold text-[var(--text-primary)]">
                {loading ? "-" : totalSpools}
              </p>
            </div>
          </div>
        </div>

        <div class="card p-6">
          <div class="flex items-center">
            <div class="p-3 bg-green-500/20 rounded-full">
              <svg class="w-8 h-8 text-green-500" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M7 21a4 4 0 01-4-4V5a2 2 0 012-2h4a2 2 0 012 2v12a4 4 0 01-4 4zm0 0h12a2 2 0 002-2v-4a2 2 0 00-2-2h-2.343M11 7.343l1.657-1.657a2 2 0 012.828 0l2.829 2.829a2 2 0 010 2.828l-8.486 8.485M7 17h.01" />
              </svg>
            </div>
            <div class="ml-4">
              <p class="text-sm font-medium text-[var(--text-muted)]">Materials</p>
              <p class="text-2xl font-semibold text-[var(--text-primary)]">
                {loading ? "-" : materials}
              </p>
            </div>
          </div>
        </div>

        <div class="card p-6">
          <div class="flex items-center">
            <div class="p-3 bg-purple-500/20 rounded-full">
              <svg class="w-8 h-8 text-purple-500" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M19 21V5a2 2 0 00-2-2H7a2 2 0 00-2 2v16m14 0h2m-2 0h-5m-9 0H3m2 0h5M9 7h1m-1 4h1m4-4h1m-1 4h1m-5 10v-5a1 1 0 011-1h2a1 1 0 011 1v5m-4 0h4" />
              </svg>
            </div>
            <div class="ml-4">
              <p class="text-sm font-medium text-[var(--text-muted)]">Brands</p>
              <p class="text-2xl font-semibold text-[var(--text-primary)]">
                {loading ? "-" : brands}
              </p>
            </div>
          </div>
        </div>
      </div>

      {/* Printer status */}
      {printers.length > 0 && (
        <div class="card p-6">
          <h2 class="text-lg font-semibold text-[var(--text-primary)] mb-4">Printers</h2>
          <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
            {printers.map((printer) => {
              const stateInfo = getPrinterStateInfo(printer);
              const hasProgress = stateInfo.progress !== undefined && stateInfo.progress !== null;
              const remainingTimeStr = formatRemainingTime(stateInfo.remainingTime);

              return (
                <Link
                  key={printer.serial}
                  href="/printers"
                  class="block p-3 bg-[var(--bg-tertiary)] rounded-lg hover:bg-[var(--border-color)] transition-colors"
                >
                  {/* Header row: name/model and status badge */}
                  <div class="flex items-center justify-between gap-2">
                    <div class="min-w-0 flex-1">
                      <p class="font-medium text-[var(--text-primary)] truncate">{printer.name || printer.serial}</p>
                      <p class="text-sm text-[var(--text-secondary)]">{printer.model}</p>
                    </div>
                    <span
                      class={`shrink-0 px-2.5 py-0.5 rounded-full text-xs font-medium ${stateInfo.bgColor} ${stateInfo.color}`}
                    >
                      {stateInfo.status}
                    </span>
                  </div>

                  {/* Print job info when printing or paused */}
                  {stateInfo.jobName && (
                    <p class="mt-2 text-xs text-[var(--text-muted)] truncate" title={stateInfo.jobName}>
                      {stateInfo.jobName}
                    </p>
                  )}

                  {/* Progress bar when printing or paused */}
                  {hasProgress && (
                    <div class="mt-2">
                      <div class="flex justify-between text-xs text-[var(--text-muted)] mb-1">
                        <span>{stateInfo.progress}%</span>
                        {remainingTimeStr && <span>{remainingTimeStr} left</span>}
                      </div>
                      <div class="h-1.5 bg-[var(--bg-secondary)] rounded-full overflow-hidden">
                        <div
                          class={`h-full rounded-full transition-all ${
                            stateInfo.status === "Paused" ? "bg-yellow-500" : "bg-blue-500"
                          }`}
                          style={{ width: `${stateInfo.progress}%` }}
                        />
                      </div>
                    </div>
                  )}
                </Link>
              );
            })}
          </div>
        </div>
      )}

      {/* Device status & current spool */}
      <div class="grid grid-cols-1 lg:grid-cols-2 gap-6">
        {/* Device status */}
        <div class="card p-6">
          <h2 class="text-lg font-semibold text-[var(--text-primary)] mb-4">SpoolBuddy Device</h2>
          <div class="space-y-4">
            <div class="flex items-center justify-between">
              <span class="text-[var(--text-secondary)]">Connection</span>
              <span
                class={`px-3 py-1 rounded-full text-sm font-medium ${
                  deviceConnected
                    ? "bg-green-500/20 text-green-500"
                    : "bg-red-500/20 text-red-500"
                }`}
              >
                {deviceConnected ? "Connected" : "Disconnected"}
              </span>
            </div>
            <div class="flex items-center justify-between">
              <span class="text-[var(--text-secondary)]">Scale Weight</span>
              <span class="text-xl font-mono text-[var(--text-primary)]">
                {currentWeight !== null ? (
                  <>
                    {Math.round(Math.max(0, currentWeight))}g
                    {weightStable && (
                      <span class="ml-2 text-green-500 text-sm">stable</span>
                    )}
                  </>
                ) : (
                  <span class="text-[var(--text-muted)]">--</span>
                )}
              </span>
            </div>
            <div class="flex items-center justify-between">
              <span class="text-[var(--text-secondary)]">NFC Tag</span>
              <span class={`font-mono text-sm ${currentTagId ? 'text-green-500' : 'text-[var(--text-muted)]'}`}>
                {currentTagId ? `Detected: ${currentTagId}` : 'No tag'}
              </span>
            </div>
            {deviceUpdateAvailable && (
              <div class="flex items-center justify-between pt-2 border-t border-[var(--border-color)]">
                <span class="text-[var(--text-secondary)]">Firmware</span>
                <Link
                  href="/settings#updates"
                  class="flex items-center gap-2 px-3 py-1 rounded-full text-sm font-medium bg-blue-500/20 text-blue-400 hover:bg-blue-500/30 transition-colors"
                >
                  <Download class="w-4 h-4" />
                  Update Available
                </Link>
              </div>
            )}
          </div>
        </div>

        {/* Current spool */}
        <div class="card p-6">
          <div class="flex items-center justify-between mb-4">
            <h2 class="text-lg font-semibold text-[var(--text-primary)]">Current Spool</h2>
            {isShowingLastKnown && displayCountdown !== null && displayCountdown > 0 && (
              <span class="text-xs text-[var(--text-muted)] bg-[var(--bg-tertiary)] px-2 py-1 rounded-full">
                Hiding in {displayCountdown}s
              </span>
            )}
          </div>
          {displayedSpool ? (
            (() => {
              // When spool is on scale, always use live scale weight (never fall back to old values)
              // When showing last known (spool removed), use the saved last known weight
              const grossWeight = isShowingLastKnown
                ? (lastKnownWeight ?? lastKnownWeightRef.current ?? null)
                : (currentWeight !== null ? Math.round(Math.max(0, currentWeight)) : null);
              // Use spool's core_weight from inventory for remaining calculation
              const coreWeight = displayedSpool.core_weight && displayedSpool.core_weight > 0
                ? displayedSpool.core_weight
                : getDefaultCoreWeight();
              // Calculate remaining filament (gross weight - core weight)
              const remaining = grossWeight !== null
                ? Math.round(Math.max(0, grossWeight - coreWeight))
                : null;
              const labelWeight = Math.round(displayedSpool.label_weight || 1000);
              const fillPercent = remaining !== null ? Math.min(100, Math.round((remaining / labelWeight) * 100)) : null;

              return (
                <div class="flex flex-col items-center text-center space-y-4">
                  {/* Spool header with color */}
                  <div class="flex flex-col items-center space-y-2">
                    <SpoolIcon
                      color={displayedSpool.rgba ? `#${displayedSpool.rgba.slice(0, 6)}` : '#808080'}
                      isEmpty={false}
                      size={64}
                    />
                    <div>
                      <p class="font-medium text-[var(--text-primary)]">
                        {displayedSpool.color_name || "Unknown color"}
                      </p>
                      <p class="text-sm text-[var(--text-secondary)]">
                        {displayedSpool.brand} {displayedSpool.material}
                        {displayedSpool.subtype && ` ${displayedSpool.subtype}`}
                      </p>
                    </div>
                  </div>

                  {/* Scale weight display */}
                  {grossWeight !== null && (
                    <div class="w-full max-w-xs">
                      <div class="text-center mb-2">
                        <span class="text-2xl font-mono text-[var(--text-primary)]">{grossWeight}g</span>
                        <span class="text-sm text-[var(--text-muted)] ml-2">scale weight</span>
                      </div>
                      {/* Fill level bar */}
                      {fillPercent !== null && (
                        <>
                          <div class="flex justify-between text-xs text-[var(--text-muted)] mb-1">
                            <span>Fill Level</span>
                            <span>{fillPercent}%</span>
                          </div>
                          <div class="h-3 bg-[var(--bg-tertiary)] rounded-full overflow-hidden">
                            <div
                              class={`h-full rounded-full transition-all ${
                                fillPercent > 50 ? 'bg-green-500' : fillPercent > 20 ? 'bg-yellow-500' : 'bg-red-500'
                              }`}
                              style={{ width: `${fillPercent}%` }}
                            />
                          </div>
                          <div class="flex justify-between text-xs text-[var(--text-muted)] mt-1">
                            <span>{remaining}g filament</span>
                            <span>of {labelWeight}g</span>
                          </div>
                        </>
                      )}
                    </div>
                  )}

                  {displayedSpool.location && (
                    <p class="text-sm text-[var(--text-secondary)]">
                      <span class="text-[var(--text-muted)]">Location:</span> {displayedSpool.location}
                    </p>
                  )}

                  {/* Action buttons for spool in inventory */}
                  <div class="flex flex-col sm:flex-row gap-2 w-full max-w-xs">
                    <button
                      onClick={() => {
                        setAssignModalSpool(displayedSpool);
                        setShowAssignModal(true);
                      }}
                      class="btn btn-primary flex-1 justify-center"
                    >
                      <svg class="w-4 h-4" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                        <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M4 6h16M4 12h16m-7 6h7" />
                      </svg>
                      Assign to AMS
                    </button>
                    <Link
                      href={`/inventory?edit=${displayedSpool.id}`}
                      class="btn flex-1 justify-center"
                    >
                      Edit Details
                    </Link>
                  </div>
                </div>
              );
            })()
          ) : currentTagId ? (
            // Tag detected but spool NOT in inventory (only show when actual tag present)
            (() => {
              // For unknown spools, estimate remaining using default core weight
              const defaultCoreWeight = getDefaultCoreWeight();
              const grossWeight = currentWeight !== null ? Math.round(Math.max(0, currentWeight)) : null;
              const estimatedRemaining = grossWeight !== null
                ? Math.round(Math.max(0, grossWeight - defaultCoreWeight))
                : null;

              return (
                <div class="flex flex-col items-center justify-center py-6 text-center space-y-4">
                  <div class="w-16 h-16 rounded-full bg-[var(--bg-tertiary)] flex items-center justify-center">
                    <svg class="w-8 h-8 text-[var(--text-muted)]" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                      <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M7 7h.01M7 3h5c.512 0 1.024.195 1.414.586l7 7a2 2 0 010 2.828l-7 7a2 2 0 01-2.828 0l-7-7A2 2 0 013 12V7a4 4 0 014-4z" />
                    </svg>
                  </div>
                  <div>
                    <p class="font-medium text-[var(--text-primary)]">New Spool Detected</p>
                    <p class="text-sm text-[var(--text-muted)]">Tag ID: {currentTagId}</p>
                  </div>
                  {grossWeight !== null && (
                    <div class="text-sm text-[var(--text-secondary)]">
                      <p>Scale: {grossWeight}g</p>
                      {estimatedRemaining !== null && estimatedRemaining > 0 && (
                        <p class="text-[var(--text-muted)]">~{estimatedRemaining}g filament (estimated)</p>
                      )}
                    </div>
                  )}
                  <p class="text-sm text-[var(--text-secondary)]">
                    This spool is not in your inventory yet.
                  </p>
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
            // No tag detected
            <div class="flex flex-col items-center justify-center py-8 text-center">
              <div class="mb-4">
                <SpoolIcon color="#808080" isEmpty={true} size={64} />
              </div>
              <p class="text-[var(--text-muted)]">
                Place a spool on the scale to identify it
              </p>
            </div>
          )}
        </div>
      </div>

      {/* Quick actions */}
      <div class="card p-6">
        <h2 class="text-lg font-semibold text-[var(--text-primary)] mb-4">Quick Actions</h2>
        <div class="flex flex-wrap gap-4">
          <Link
            href="/inventory"
            class="btn btn-primary"
          >
            <svg class="w-5 h-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 6v6m0 0v6m0-6h6m-6 0H6" />
            </svg>
            Add Spool
          </Link>
          <Link
            href="/printers"
            class="btn"
          >
            <svg class="w-5 h-5 text-[var(--text-muted)]" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M17 17h2a2 2 0 002-2v-4a2 2 0 00-2-2H5a2 2 0 00-2 2v4a2 2 0 002 2h2m2 4h6a2 2 0 002-2v-4a2 2 0 00-2-2H9a2 2 0 00-2 2v4a2 2 0 002 2zm8-12V5a2 2 0 00-2-2H9a2 2 0 00-2 2v4h10z" />
            </svg>
            Manage Printers
          </Link>
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
