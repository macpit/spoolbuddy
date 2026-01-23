import { useState, useEffect, useCallback } from "preact/hooks";
import * as preact from "preact";
import { useWebSocket } from "../lib/websocket";
import { api, CloudAuthStatus, VersionInfo, UpdateCheck, UpdateStatus, FirmwareCheck, AMSThresholds, DebugLoggingState, LogEntry, SystemInfo, APIKey, APIKeyCreate } from "../lib/api";
import { Cloud, CloudOff, LogOut, Loader2, Mail, Lock, Key, Download, RefreshCw, CheckCircle, AlertCircle, GitBranch, ExternalLink, Wifi, WifiOff, Cpu, Usb, RotateCcw, Upload, HardDrive, Palette, Sun, Moon, LayoutDashboard, Settings2, Package, Monitor, Scale, X, ChevronRight, Droplets, Thermometer, LifeBuoy, Bug, Trash2, FileText, Server, Database, Activity, HelpCircle, Play, Square, Copy, Globe, Plus } from "lucide-preact";
import { useToast } from "../lib/toast";
import { SerialTerminal } from "../components/SerialTerminal";
import { SpoolCatalogSettings } from "../components/SpoolCatalogSettings";
import { ColorCatalogSettings } from "../components/ColorCatalogSettings";
import { APIBrowser } from "../components/APIBrowser";
import { useTheme, type ThemeStyle, type DarkBackground, type LightBackground, type ThemeAccent } from "../lib/theme";

// Storage keys for dashboard settings
const DEFAULT_CORE_WEIGHT_KEY = 'spoolbuddy-default-core-weight';

function DashboardSettings() {
  const { showToast } = useToast();

  const [defaultCoreWeight, setDefaultCoreWeight] = useState<number>(() => {
    const stored = localStorage.getItem(DEFAULT_CORE_WEIGHT_KEY);
    if (stored) {
      const val = parseInt(stored, 10);
      if (val >= 0 && val <= 500) return val;
    }
    return 250; // Default 250g (typical Bambu spool core)
  });

  const handleCoreWeightChange = (value: number) => {
    setDefaultCoreWeight(value);
    localStorage.setItem(DEFAULT_CORE_WEIGHT_KEY, String(value));
    showToast('success', `Default core weight set to ${value}g`);
  };

  return (
    <div class="card">
      <div class="px-6 py-4 border-b border-[var(--border-color)]">
        <div class="flex items-center gap-2">
          <LayoutDashboard class="w-5 h-5 text-[var(--text-muted)]" />
          <h2 class="text-lg font-medium text-[var(--text-primary)]">Dashboard</h2>
        </div>
      </div>
      <div class="p-6 space-y-4">
        <div class="flex items-center justify-between">
          <div>
            <p class="text-sm font-medium text-[var(--text-primary)]">Default Core Weight</p>
            <p class="text-xs text-[var(--text-muted)]">
              Empty spool weight for unknown spools (used to calculate remaining filament)
            </p>
          </div>
          <div class="flex items-center gap-2">
            <input
              type="number"
              min="0"
              max="500"
              value={defaultCoreWeight}
              onChange={(e) => {
                const val = parseInt((e.target as HTMLInputElement).value, 10);
                if (!isNaN(val) && val >= 0 && val <= 500) {
                  handleCoreWeightChange(val);
                }
              }}
              class="w-20 px-3 py-1.5 text-sm bg-[var(--bg-secondary)] border border-[var(--border-color)] rounded-lg text-[var(--text-primary)] focus:border-[var(--accent)] focus:outline-none text-right"
            />
            <span class="text-sm text-[var(--text-muted)]">g</span>
          </div>
        </div>
      </div>
    </div>
  );
}

function AMSSettings() {
  const { showToast } = useToast();
  const [thresholds, setThresholds] = useState<AMSThresholds | null>(null);
  const [loading, setLoading] = useState(true);
  const [saving, setSaving] = useState(false);
  const [pendingChanges, setPendingChanges] = useState<Partial<AMSThresholds>>({});

  useEffect(() => {
    api.getAMSThresholds()
      .then(setThresholds)
      .catch(err => {
        console.error("Failed to load AMS thresholds:", err);
        // Use defaults on error
        setThresholds({
          humidity_good: 40,
          humidity_fair: 60,
          temp_good: 28,
          temp_fair: 35,
          history_retention_days: 30,
        });
      })
      .finally(() => setLoading(false));
  }, []);

  // Debounced save - only save after 500ms of no changes
  useEffect(() => {
    if (Object.keys(pendingChanges).length === 0 || !thresholds) return;

    const timeout = setTimeout(async () => {
      const updated = { ...thresholds, ...pendingChanges };
      setSaving(true);
      try {
        await api.setAMSThresholds(updated);
        setThresholds(updated);
        setPendingChanges({});
        showToast('success', 'AMS thresholds saved');
      } catch (err) {
        showToast('error', 'Failed to save thresholds');
      } finally {
        setSaving(false);
      }
    }, 500);

    return () => clearTimeout(timeout);
  }, [pendingChanges, thresholds, showToast]);

  const updateThreshold = (key: keyof AMSThresholds, value: number) => {
    if (!thresholds) return;
    setPendingChanges(prev => ({ ...prev, [key]: value }));
    // Update local display immediately
    setThresholds(prev => prev ? { ...prev, [key]: value } : prev);
  };

  if (loading) {
    return (
      <div class="card">
        <div class="px-6 py-4 border-b border-[var(--border-color)]">
          <div class="flex items-center gap-2">
            <Droplets class="w-5 h-5 text-[var(--text-muted)]" />
            <h2 class="text-lg font-medium text-[var(--text-primary)]">AMS Sensors</h2>
          </div>
        </div>
        <div class="p-6 flex items-center justify-center">
          <Loader2 class="w-5 h-5 animate-spin text-[var(--text-muted)]" />
        </div>
      </div>
    );
  }

  return (
    <div class="card">
      <div class="px-6 py-4 border-b border-[var(--border-color)]">
        <div class="flex items-center justify-between">
          <div class="flex items-center gap-2">
            <Droplets class="w-5 h-5 text-[var(--text-muted)]" />
            <h2 class="text-lg font-medium text-[var(--text-primary)]">AMS Sensors</h2>
          </div>
          {saving && (
            <span class="text-xs text-[var(--text-muted)] flex items-center gap-1.5">
              <Loader2 class="w-3 h-3 animate-spin" />
              Saving...
            </span>
          )}
        </div>
      </div>
      <div class="p-6 space-y-4">
        <p class="text-sm text-[var(--text-secondary)]">
          Click humidity/temperature on AMS cards to view graphs. Values are colored based on thresholds below.
        </p>

        {/* Thresholds Table */}
        <div class="overflow-hidden rounded-lg border border-[var(--border-color)]">
          <table class="w-full text-sm">
            <thead>
              <tr class="bg-[var(--bg-tertiary)]">
                <th class="px-4 py-2 text-left text-xs font-medium text-[var(--text-muted)]">Metric</th>
                <th class="px-4 py-2 text-center text-xs font-medium text-[#22c55e]">Good</th>
                <th class="px-4 py-2 text-center text-xs font-medium text-[#eab308]">Fair</th>
                <th class="px-4 py-2 text-center text-xs font-medium text-[#ef4444]">High</th>
              </tr>
            </thead>
            <tbody class="divide-y divide-[var(--border-color)]">
              <tr>
                <td class="px-4 py-3">
                  <div class="flex items-center gap-2">
                    <Droplets class="w-4 h-4 text-blue-500" />
                    <span class="text-[var(--text-primary)]">Humidity</span>
                  </div>
                </td>
                <td class="px-4 py-3 text-center">
                  <div class="flex items-center justify-center gap-1">
                    <span class="text-[var(--text-muted)]">&le;</span>
                    <input
                      type="number"
                      min="10"
                      max="80"
                      value={thresholds?.humidity_good ?? 40}
                      onInput={(e) => {
                        const val = parseInt((e.target as HTMLInputElement).value);
                        if (!isNaN(val) && val >= 10 && val <= 80) updateThreshold('humidity_good', val);
                      }}
                      class="w-14 px-2 py-1 text-sm bg-[var(--bg-secondary)] border border-[var(--border-color)] rounded text-center text-[var(--text-primary)] focus:border-[var(--accent)] focus:outline-none"
                      disabled={saving}
                    />
                    <span class="text-[var(--text-muted)]">%</span>
                  </div>
                </td>
                <td class="px-4 py-3 text-center">
                  <div class="flex items-center justify-center gap-1">
                    <span class="text-[var(--text-muted)]">&le;</span>
                    <input
                      type="number"
                      min="20"
                      max="90"
                      value={thresholds?.humidity_fair ?? 60}
                      onInput={(e) => {
                        const val = parseInt((e.target as HTMLInputElement).value);
                        if (!isNaN(val) && val >= 20 && val <= 90) updateThreshold('humidity_fair', val);
                      }}
                      class="w-14 px-2 py-1 text-sm bg-[var(--bg-secondary)] border border-[var(--border-color)] rounded text-center text-[var(--text-primary)] focus:border-[var(--accent)] focus:outline-none"
                      disabled={saving}
                    />
                    <span class="text-[var(--text-muted)]">%</span>
                  </div>
                </td>
                <td class="px-4 py-3 text-center text-[var(--text-muted)]">
                  &gt; {thresholds?.humidity_fair ?? 60}%
                </td>
              </tr>
              <tr>
                <td class="px-4 py-3">
                  <div class="flex items-center gap-2">
                    <Thermometer class="w-4 h-4 text-orange-500" />
                    <span class="text-[var(--text-primary)]">Temperature</span>
                  </div>
                </td>
                <td class="px-4 py-3 text-center">
                  <div class="flex items-center justify-center gap-1">
                    <span class="text-[var(--text-muted)]">&le;</span>
                    <input
                      type="number"
                      min="15"
                      max="40"
                      step="0.5"
                      value={thresholds?.temp_good ?? 28}
                      onInput={(e) => {
                        const val = parseFloat((e.target as HTMLInputElement).value);
                        if (!isNaN(val) && val >= 15 && val <= 40) updateThreshold('temp_good', val);
                      }}
                      class="w-14 px-2 py-1 text-sm bg-[var(--bg-secondary)] border border-[var(--border-color)] rounded text-center text-[var(--text-primary)] focus:border-[var(--accent)] focus:outline-none"
                      disabled={saving}
                    />
                    <span class="text-[var(--text-muted)]">°C</span>
                  </div>
                </td>
                <td class="px-4 py-3 text-center">
                  <div class="flex items-center justify-center gap-1">
                    <span class="text-[var(--text-muted)]">&le;</span>
                    <input
                      type="number"
                      min="20"
                      max="50"
                      step="0.5"
                      value={thresholds?.temp_fair ?? 35}
                      onInput={(e) => {
                        const val = parseFloat((e.target as HTMLInputElement).value);
                        if (!isNaN(val) && val >= 20 && val <= 50) updateThreshold('temp_fair', val);
                      }}
                      class="w-14 px-2 py-1 text-sm bg-[var(--bg-secondary)] border border-[var(--border-color)] rounded text-center text-[var(--text-primary)] focus:border-[var(--accent)] focus:outline-none"
                      disabled={saving}
                    />
                    <span class="text-[var(--text-muted)]">°C</span>
                  </div>
                </td>
                <td class="px-4 py-3 text-center text-[var(--text-muted)]">
                  &gt; {thresholds?.temp_fair ?? 35}°C
                </td>
              </tr>
            </tbody>
          </table>
        </div>

        {/* History Retention */}
        <div class="flex items-center justify-between pt-2">
          <div>
            <p class="text-sm font-medium text-[var(--text-primary)]">History Retention</p>
            <p class="text-xs text-[var(--text-muted)]">How long to keep sensor history</p>
          </div>
          <select
            value={thresholds?.history_retention_days ?? 30}
            onChange={(e) => updateThreshold('history_retention_days', parseInt((e.target as HTMLSelectElement).value))}
            class="px-3 py-1.5 text-sm bg-[var(--bg-secondary)] border border-[var(--border-color)] rounded-lg text-[var(--text-primary)] focus:border-[var(--accent)] focus:outline-none"
            disabled={saving}
          >
            <option value="7">7 days</option>
            <option value="14">14 days</option>
            <option value="30">30 days</option>
            <option value="60">60 days</option>
            <option value="90">90 days</option>
          </select>
        </div>
      </div>
    </div>
  );
}

type SettingsTab = 'general' | 'filament' | 'system' | 'api' | 'support';

// Reusable section card component for consistent styling
function SettingsCard({
  id,
  icon: Icon,
  title,
  description,
  children,
  headerRight
}: {
  id?: string;
  icon?: typeof Settings2;
  title: string;
  description?: string;
  children: preact.ComponentChildren;
  headerRight?: preact.ComponentChildren;
}) {
  return (
    <div id={id} class="card overflow-hidden scroll-mt-20">
      <div class="px-5 py-4 bg-[var(--bg-tertiary)]/50 border-b border-[var(--border-color)]">
        <div class="flex items-center justify-between">
          <div class="flex items-center gap-3">
            {Icon && (
              <div class="w-9 h-9 rounded-lg bg-[var(--accent)]/10 flex items-center justify-center flex-shrink-0">
                <Icon class="w-4.5 h-4.5 text-[var(--accent)]" />
              </div>
            )}
            <div>
              <h2 class="text-base font-semibold text-[var(--text-primary)]">{title}</h2>
              {description && (
                <p class="text-xs text-[var(--text-muted)] mt-0.5">{description}</p>
              )}
            </div>
          </div>
          {headerRight}
        </div>
      </div>
      <div class="p-5">
        {children}
      </div>
    </div>
  );
}

// Reusable setting row component
function SettingRow({
  label,
  description,
  children
}: {
  label: string;
  description?: string;
  children: preact.ComponentChildren;
}) {
  return (
    <div class="flex items-center justify-between gap-4 py-3">
      <div class="min-w-0 flex-1">
        <p class="text-sm font-medium text-[var(--text-primary)]">{label}</p>
        {description && (
          <p class="text-xs text-[var(--text-muted)] mt-0.5">{description}</p>
        )}
      </div>
      <div class="flex-shrink-0">
        {children}
      </div>
    </div>
  );
}

export function Settings() {
  const { deviceConnected, currentWeight, weightStable } = useWebSocket();
  const { showToast } = useToast();
  const {
    mode,
    darkStyle, darkBackground, darkAccent,
    lightStyle, lightBackground, lightAccent,
    toggleMode,
    setDarkStyle, setDarkBackground, setDarkAccent,
    setLightStyle, setLightBackground, setLightAccent,
  } = useTheme();

  // Tab state
  const [activeTab, setActiveTab] = useState<SettingsTab>('general');

  // Cloud auth state
  const [cloudStatus, setCloudStatus] = useState<CloudAuthStatus | null>(null);
  const [loadingCloud, setLoadingCloud] = useState(true);
  const [loginStep, setLoginStep] = useState<'idle' | 'credentials' | 'verify'>('idle');
  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [verifyCode, setVerifyCode] = useState('');
  const [loginLoading, setLoginLoading] = useState(false);
  const [loginError, setLoginError] = useState<string | null>(null);

  // Update state
  const [versionInfo, setVersionInfo] = useState<VersionInfo | null>(null);
  const [updateCheck, setUpdateCheck] = useState<UpdateCheck | null>(null);
  const [updateStatus, setUpdateStatus] = useState<UpdateStatus | null>(null);
  const [checkingUpdate, setCheckingUpdate] = useState(false);
  const [applyingUpdate, setApplyingUpdate] = useState(false);

  // ESP32 Device state

  // Firmware update state
  const [firmwareCheck, setFirmwareCheck] = useState<FirmwareCheck | null>(null);
  const [checkingFirmware, setCheckingFirmware] = useState(false);
  const [uploadingFirmware, setUploadingFirmware] = useState(false);
  const [deviceFirmwareVersion, setDeviceFirmwareVersion] = useState<string | null>(null);

  // Scale calibration state
  type CalibrationStep = 'idle' | 'empty' | 'weight' | 'complete';
  const [calibrationStep, setCalibrationStep] = useState<CalibrationStep>('idle');
  const [calibrationWeight, setCalibrationWeight] = useState<number>(500);
  const [calibrating, setCalibrating] = useState(false);
  const [showResetConfirm, setShowResetConfirm] = useState(false);

  // Support tab state
  const [systemInfo, setSystemInfo] = useState<SystemInfo | null>(null);
  const [loadingSystemInfo, setLoadingSystemInfo] = useState(false);
  const [debugLogging, setDebugLogging] = useState<DebugLoggingState | null>(null);
  const [loadingDebug, setLoadingDebug] = useState(false);
  const [logs, setLogs] = useState<LogEntry[]>([]);
  const [loadingLogs, setLoadingLogs] = useState(false);
  const [logLevel, setLogLevel] = useState<string>('');
  const [logSearch, setLogSearch] = useState<string>('');
  const [logsTotal, setLogsTotal] = useState(0);
  const [logsFiltered, setLogsFiltered] = useState(0);
  const [downloadingBundle, setDownloadingBundle] = useState(false);
  const [logStreaming, setLogStreaming] = useState(false);

  // API Keys tab state
  const [apiKeys, setApiKeys] = useState<APIKey[]>([]);
  const [loadingApiKeys, setLoadingApiKeys] = useState(false);
  const [showCreateApiKey, setShowCreateApiKey] = useState(false);
  const [creatingApiKey, setCreatingApiKey] = useState(false);
  const [createdApiKey, setCreatedApiKey] = useState<string | null>(null);
  const [newApiKeyName, setNewApiKeyName] = useState('');
  const [newApiKeyPermissions, setNewApiKeyPermissions] = useState({ can_read: true, can_write: false, can_control: false });
  const [testApiKey, setTestApiKey] = useState('');
  const [deleteKeyId, setDeleteKeyId] = useState<number | null>(null);

  // Handle hash navigation and switch to correct tab
  useEffect(() => {
    const sectionToTab: Record<string, SettingsTab> = {
      'updates': 'system',
      'firmware': 'system',
      'device': 'system',
      'appearance': 'general',
      'cloud': 'general',
      'about': 'general',
      'dashboard': 'filament',
      'catalog': 'filament',
      'colors': 'filament',
      'system-info': 'support',
      'logs': 'support',
      'debug': 'support',
      'api-keys': 'api',
    };

    const handleHashChange = () => {
      if (window.location.hash) {
        const id = window.location.hash.slice(1);
        if (sectionToTab[id]) {
          setActiveTab(sectionToTab[id]);
        }
        setTimeout(() => {
          const el = document.getElementById(id);
          if (el) {
            el.scrollIntoView({ behavior: 'smooth', block: 'start' });
          }
        }, 100);
      }
    };

    // Handle initial hash on mount
    handleHashChange();

    // Listen for hash changes
    window.addEventListener('hashchange', handleHashChange);
    return () => window.removeEventListener('hashchange', handleHashChange);
  }, []);

  // Fetch cloud status on mount
  useEffect(() => {
    const fetchStatus = async () => {
      try {
        const status = await api.getCloudStatus();
        setCloudStatus(status);
      } catch (e) {
        console.error('Failed to fetch cloud status:', e);
      } finally {
        setLoadingCloud(false);
      }
    };
    fetchStatus();
  }, []);

  // Fetch version info on mount
  useEffect(() => {
    const fetchVersion = async () => {
      try {
        const info = await api.getVersion();
        setVersionInfo(info);
      } catch (e) {
        console.error('Failed to fetch version info:', e);
      }
    };
    fetchVersion();
  }, []);

  // Fetch device firmware version on mount and when device connects
  useEffect(() => {
    const fetchDisplayStatus = async () => {
      try {
        const status = await api.getDisplayStatus();
        if (status.firmware_version) {
          setDeviceFirmwareVersion(status.firmware_version);
        }
      } catch (e) {
        console.error('Failed to fetch display status:', e);
      }
    };
    fetchDisplayStatus();
  }, [deviceConnected]);

  // ESP32 reboot handler
  const handleESP32Reboot = useCallback(async () => {
    try {
      await api.rebootESP32();
      showToast('success', 'Reboot command sent');
    } catch (e) {
      showToast('error', 'Failed to send reboot command');
    }
  }, [showToast]);

  // Check for firmware updates
  const handleCheckFirmware = useCallback(async () => {
    setCheckingFirmware(true);
    try {
      // Device version is reported by firmware during OTA check, not from frontend
      const check = await api.checkFirmwareUpdate(undefined);
      setFirmwareCheck(check);
      // Update device version state if returned
      if (check.current_version) {
        setDeviceFirmwareVersion(check.current_version);
      }
      if (check.error) {
        showToast('error', check.error);
      } else if (check.update_available) {
        showToast('info', `Firmware update available: v${check.latest_version}`);
      } else if (check.current_version) {
        showToast('success', 'Firmware is up to date');
      } else {
        showToast('info', 'No firmware version reported by device');
      }
    } catch (e) {
      showToast('error', e instanceof Error ? e.message : 'Failed to check firmware');
    } finally {
      setCheckingFirmware(false);
    }
  }, [showToast]);

  // Upload firmware file
  const handleFirmwareUpload = useCallback(async (e: Event) => {
    const input = e.target as HTMLInputElement;
    const file = input.files?.[0];
    if (!file) return;

    if (!file.name.endsWith('.bin')) {
      showToast('error', 'Please select a .bin firmware file');
      return;
    }

    setUploadingFirmware(true);
    try {
      const formData = new FormData();
      formData.append('file', file);

      const response = await fetch('/api/firmware/upload', {
        method: 'POST',
        body: formData,
      });

      if (!response.ok) {
        const error = await response.json();
        throw new Error(error.detail || 'Upload failed');
      }

      const result = await response.json();
      showToast('success', `Firmware v${result.version} uploaded successfully`);
      handleCheckFirmware();
    } catch (e) {
      showToast('error', e instanceof Error ? e.message : 'Failed to upload firmware');
    } finally {
      setUploadingFirmware(false);
      input.value = ''; // Reset file input
    }
  }, [showToast, handleCheckFirmware]);

  // Device update state
  const [deviceUpdating, setDeviceUpdating] = useState(false);

  // Trigger device OTA update
  const handleTriggerOTA = useCallback(async () => {
    try {
      setDeviceUpdating(true);
      showToast('info', 'Sending update command to device...');
      await api.triggerOTA();
      showToast('success', 'Device is downloading and installing update. This may take a minute.');
      // Wait for device to disconnect and reconnect
      setTimeout(() => {
        setDeviceUpdating(false);
      }, 120000); // Reset after 2 min (OTA takes longer)
    } catch (e) {
      setDeviceUpdating(false);
      showToast('error', 'Failed to send update command');
    }
  }, [showToast]);

  // Reset updating state when device reconnects
  useEffect(() => {
    if (deviceConnected && deviceUpdating) {
      setDeviceUpdating(false);
      showToast('success', 'Device reconnected - update complete!');
    }
  }, [deviceConnected, deviceUpdating, showToast]);

  // Check for updates
  const handleCheckUpdates = useCallback(async (force: boolean = false) => {
    setCheckingUpdate(true);
    try {
      const check = await api.checkForUpdates(force);
      setUpdateCheck(check);
      if (check.error) {
        showToast('error', check.error);
      } else if (check.update_available) {
        showToast('info', `Update available: v${check.latest_version}`);
      } else {
        showToast('success', 'You are running the latest version');
      }
    } catch (e) {
      showToast('error', e instanceof Error ? e.message : 'Failed to check for updates');
    } finally {
      setCheckingUpdate(false);
    }
  }, [showToast]);

  // Apply update
  const handleApplyUpdate = useCallback(async () => {
    setApplyingUpdate(true);
    try {
      const status = await api.applyUpdate();
      setUpdateStatus(status);

      // Poll for status updates
      const pollStatus = async () => {
        const s = await api.getUpdateStatus();
        setUpdateStatus(s);
        if (s.status === 'restarting') {
          showToast('success', 'Update applied! Please restart the application.');
          setApplyingUpdate(false);
        } else if (s.status === 'error') {
          showToast('error', s.error || 'Update failed');
          setApplyingUpdate(false);
        } else if (s.status !== 'idle') {
          setTimeout(pollStatus, 1000);
        } else {
          setApplyingUpdate(false);
        }
      };

      setTimeout(pollStatus, 1000);
    } catch (e) {
      showToast('error', e instanceof Error ? e.message : 'Failed to apply update');
      setApplyingUpdate(false);
    }
  }, [showToast]);

  const handleLogin = async () => {
    if (!email || !password) {
      setLoginError('Email and password are required');
      return;
    }

    setLoginLoading(true);
    setLoginError(null);

    try {
      const result = await api.cloudLogin(email, password);

      if (result.success) {
        // Direct login success (rare)
        const status = await api.getCloudStatus();
        setCloudStatus(status);
        setLoginStep('idle');
        setEmail('');
        setPassword('');
        showToast('success', 'Logged in to Bambu Cloud');
      } else if (result.needs_verification) {
        // Need verification code
        setLoginStep('verify');
        showToast('info', 'Check your email for verification code');
      } else {
        setLoginError(result.message);
      }
    } catch (e) {
      setLoginError(e instanceof Error ? e.message : 'Login failed');
    } finally {
      setLoginLoading(false);
    }
  };

  const handleVerify = async () => {
    if (!verifyCode) {
      setLoginError('Verification code is required');
      return;
    }

    setLoginLoading(true);
    setLoginError(null);

    try {
      const result = await api.cloudVerify(email, verifyCode);

      if (result.success) {
        const status = await api.getCloudStatus();
        setCloudStatus(status);
        setLoginStep('idle');
        setEmail('');
        setPassword('');
        setVerifyCode('');
        showToast('success', 'Logged in to Bambu Cloud');
      } else {
        setLoginError(result.message);
      }
    } catch (e) {
      setLoginError(e instanceof Error ? e.message : 'Verification failed');
    } finally {
      setLoginLoading(false);
    }
  };

  const handleLogout = async () => {
    try {
      await api.cloudLogout();
      setCloudStatus({ is_authenticated: false, email: null });
      showToast('success', 'Logged out of Bambu Cloud');
    } catch (e) {
      showToast('error', 'Failed to logout');
    }
  };

  const cancelLogin = () => {
    setLoginStep('idle');
    setEmail('');
    setPassword('');
    setVerifyCode('');
    setLoginError(null);
  };

  const handleTare = async () => {
    try {
      await api.tareScale();
      showToast('success', 'Scale zeroed successfully');
    } catch (e) {
      console.error("Failed to tare:", e);
      showToast('error', 'Failed to zero scale');
    }
  };

  const handleResetCalibration = async () => {
    try {
      await api.resetScaleCalibration();
      showToast('success', 'Scale calibration reset to defaults');
    } catch (e) {
      console.error("Failed to reset calibration:", e);
      showToast('error', 'Failed to reset calibration');
    }
  };

  const startCalibration = () => {
    setCalibrationStep('empty');
    setCalibrationWeight(500);
  };

  // Load API keys when tab becomes active
  useEffect(() => {
    if (activeTab !== 'api') return;

    const loadApiKeys = async () => {
      setLoadingApiKeys(true);
      try {
        const keys = await api.getAPIKeys();
        setApiKeys(keys);
      } catch (e) {
        console.error('Failed to load API keys:', e);
        showToast('error', 'Failed to load API keys');
      } finally {
        setLoadingApiKeys(false);
      }
    };

    loadApiKeys();
  }, [activeTab, showToast]);

  const handleCreateApiKey = async () => {
    if (!newApiKeyName.trim()) {
      showToast('error', 'Key name is required');
      return;
    }

    setCreatingApiKey(true);
    try {
      const data: APIKeyCreate = {
        name: newApiKeyName.trim(),
        ...newApiKeyPermissions,
      };
      const result = await api.createAPIKey(data);
      setCreatedApiKey(result.key);
      setApiKeys(prev => [result, ...prev]);
      setShowCreateApiKey(false);
      setNewApiKeyName('');
      setNewApiKeyPermissions({ can_read: true, can_write: false, can_control: false });
    } catch (e) {
      showToast('error', e instanceof Error ? e.message : 'Failed to create API key');
    } finally {
      setCreatingApiKey(false);
    }
  };

  const handleDeleteApiKey = async (keyId: number) => {
    try {
      await api.deleteAPIKey(keyId);
      setApiKeys(prev => prev.filter(k => k.id !== keyId));
      setDeleteKeyId(null);
      showToast('success', 'API key deleted');
    } catch (e) {
      showToast('error', e instanceof Error ? e.message : 'Failed to delete API key');
    }
  };

  const copyToClipboard = async (text: string) => {
    try {
      if (navigator.clipboard && navigator.clipboard.writeText) {
        await navigator.clipboard.writeText(text);
      } else {
        const textArea = document.createElement('textarea');
        textArea.value = text;
        textArea.style.position = 'fixed';
        textArea.style.left = '-999999px';
        document.body.appendChild(textArea);
        textArea.select();
        document.execCommand('copy');
        document.body.removeChild(textArea);
      }
      showToast('success', 'Copied to clipboard');
    } catch {
      showToast('error', 'Failed to copy');
    }
  };

  const formatTimestamp = (ts: number | null) => {
    if (!ts) return 'Never';
    return new Date(ts * 1000).toLocaleDateString();
  };

  // Load support data when tab becomes active
  useEffect(() => {
    if (activeTab !== 'support') return;

    const loadSupportData = async () => {
      // Load system info
      setLoadingSystemInfo(true);
      try {
        const info = await api.getSystemInfo();
        setSystemInfo(info);
      } catch (e) {
        console.error('Failed to load system info:', e);
      } finally {
        setLoadingSystemInfo(false);
      }

      // Load debug logging state
      setLoadingDebug(true);
      try {
        const state = await api.getDebugLogging();
        setDebugLogging(state);
      } catch (e) {
        console.error('Failed to load debug logging state:', e);
      } finally {
        setLoadingDebug(false);
      }
    };

    loadSupportData();

    // Poll debug logging state every 10 seconds to update timer
    const interval = setInterval(async () => {
      try {
        const state = await api.getDebugLogging();
        setDebugLogging(state);
      } catch (e) {
        // Silently ignore polling errors
      }
    }, 10000);

    return () => clearInterval(interval);
  }, [activeTab]);

  const refreshLogs = async () => {
    setLoadingLogs(true);
    try {
      const response = await api.getLogs({
        limit: 200,
        level: logLevel || undefined,
        search: logSearch || undefined,
      });
      setLogs(response.entries);
      setLogsTotal(response.total_count);
      setLogsFiltered(response.filtered_count);
    } catch (e) {
      console.error('Failed to load logs:', e);
      showToast('error', 'Failed to load logs');
    } finally {
      setLoadingLogs(false);
    }
  };

  const handleToggleDebugLogging = async () => {
    if (!debugLogging) return;
    setLoadingDebug(true);
    try {
      const newState = await api.setDebugLogging(!debugLogging.enabled);
      setDebugLogging(newState);
      showToast('success', newState.enabled ? 'Debug logging enabled' : 'Debug logging disabled');
    } catch (e) {
      showToast('error', 'Failed to toggle debug logging');
    } finally {
      setLoadingDebug(false);
    }
  };

  const handleClearLogs = async () => {
    try {
      await api.clearLogs();
      setLogs([]);
      setLogsTotal(0);
      setLogsFiltered(0);
      showToast('success', 'Logs cleared');
    } catch (e) {
      showToast('error', 'Failed to clear logs');
    }
  };

  const handleDownloadBundle = async () => {
    if (!debugLogging?.enabled) {
      showToast('error', 'Enable debug logging first to generate support bundle');
      return;
    }
    setDownloadingBundle(true);
    try {
      const blob = await api.downloadSupportBundle();
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = `spoolbuddy-support-${new Date().toISOString().slice(0, 19).replace(/[:-]/g, '')}.zip`;
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
      URL.revokeObjectURL(url);
      showToast('success', 'Support bundle downloaded');
    } catch (e) {
      showToast('error', e instanceof Error ? e.message : 'Failed to download support bundle');
    } finally {
      setDownloadingBundle(false);
    }
  };

  // Refresh logs when filter changes (only if streaming or has logs already)
  useEffect(() => {
    if (activeTab === 'support' && (logStreaming || logs.length > 0)) {
      const timeout = setTimeout(() => {
        refreshLogs();
      }, 300);
      return () => clearTimeout(timeout);
    }
  }, [logLevel, logSearch]);

  // Auto-refresh logs when streaming is enabled
  useEffect(() => {
    if (!logStreaming || activeTab !== 'support') return;
    const interval = setInterval(() => {
      refreshLogs();
    }, 2000);
    return () => clearInterval(interval);
  }, [logStreaming, activeTab]);

  // Stop streaming when leaving support tab
  useEffect(() => {
    if (activeTab !== 'support') {
      setLogStreaming(false);
    }
  }, [activeTab]);

  const cancelCalibration = useCallback(() => {
    setCalibrationStep('idle');
    setCalibrating(false);
  }, []);

  // Handle escape key and body scroll for calibration modal
  useEffect(() => {
    if (calibrationStep === 'idle') return;

    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key === 'Escape') cancelCalibration();
    };

    document.addEventListener('keydown', handleKeyDown);
    document.body.style.overflow = 'hidden';

    return () => {
      document.removeEventListener('keydown', handleKeyDown);
      document.body.style.overflow = '';
    };
  }, [calibrationStep, cancelCalibration]);

  const handleCalibrationNext = async () => {
    if (calibrationStep === 'empty') {
      // Tare the scale (set zero point while empty)
      setCalibrating(true);
      try {
        await api.tareScale();
        setCalibrationStep('weight');
      } catch (e) {
        showToast('error', 'Failed to set zero point');
      } finally {
        setCalibrating(false);
      }
    } else if (calibrationStep === 'weight') {
      // Perform calibration with known weight
      setCalibrating(true);
      try {
        await api.calibrateScale(calibrationWeight);
        // Wait a moment for the scale to update
        await new Promise(resolve => setTimeout(resolve, 1500));
        // Check if calibration actually worked by comparing current weight to target
        const weightDiff = Math.abs((currentWeight ?? 0) - calibrationWeight);
        if (weightDiff > 50) {
          // Weight is more than 50g off - calibration likely failed
          showToast('error', `Calibration failed - scale shows ${Math.round(currentWeight ?? 0)}g instead of ${calibrationWeight}g. Check load cell connection.`);
          setCalibrationStep('idle');
        } else {
          setCalibrationStep('complete');
          showToast('success', 'Scale calibrated successfully');
        }
      } catch (e) {
        showToast('error', 'Calibration failed');
      } finally {
        setCalibrating(false);
      }
    } else if (calibrationStep === 'complete') {
      setCalibrationStep('idle');
    }
  };

  const tabs: { id: SettingsTab; label: string; icon: typeof Settings2 }[] = [
    { id: 'general', label: 'General', icon: Settings2 },
    { id: 'filament', label: 'Filament', icon: Package },
    { id: 'system', label: 'System', icon: Monitor },
    { id: 'api', label: 'API', icon: Globe },
    { id: 'support', label: 'Support', icon: LifeBuoy },
  ];

  return (
    <div class="space-y-6">
      {/* Header */}
      <div class="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-4">
        <div>
          <h1 class="text-2xl font-bold text-[var(--text-primary)]">Settings</h1>
          <p class="text-sm text-[var(--text-muted)] mt-1">Configure SpoolBuddy preferences and system options</p>
        </div>
        {versionInfo && (
          <div class="flex items-center gap-2 px-3 py-1.5 bg-[var(--bg-secondary)] rounded-lg border border-[var(--border-color)]">
            <span class="text-xs text-[var(--text-muted)]">Version</span>
            <span class="text-sm font-mono font-medium text-[var(--accent)]">v{versionInfo.version}</span>
          </div>
        )}
      </div>

      {/* Tab Navigation - pill style */}
      <div class="flex items-center gap-1 p-1 bg-[var(--bg-secondary)] rounded-xl border border-[var(--border-color)] w-fit">
        {tabs.map((tab) => {
          const Icon = tab.icon;
          const isActive = activeTab === tab.id;
          return (
            <button
              key={tab.id}
              onClick={() => setActiveTab(tab.id)}
              class={`flex items-center gap-2 px-4 py-2 text-sm font-medium rounded-lg transition-all ${
                isActive
                  ? 'bg-[var(--accent)] text-white shadow-md'
                  : 'text-[var(--text-muted)] hover:text-[var(--text-primary)] hover:bg-[var(--bg-tertiary)]'
              }`}
            >
              <Icon class="w-4 h-4" />
              <span class="hidden sm:inline">{tab.label}</span>
            </button>
          );
        })}
      </div>

      {/* Tab Content */}
      <div class="space-y-6">

        {/* ============ GENERAL TAB ============ */}
        {activeTab === 'general' && (
          <div class="space-y-6">
            {/* Bambu Cloud settings */}
            <SettingsCard
              id="cloud"
              icon={Cloud}
              title="Bambu Cloud"
              description="Connect to access your custom filament presets"
            >
              {loadingCloud ? (
                <div class="flex items-center justify-center py-8">
                  <Loader2 class="w-6 h-6 animate-spin text-[var(--accent)]" />
                </div>
              ) : cloudStatus?.is_authenticated ? (
                <div class="space-y-4">
                  <div class="flex items-center justify-between p-4 bg-green-500/10 rounded-xl border border-green-500/20">
                    <div class="flex items-center gap-3">
                      <div class="w-10 h-10 rounded-full bg-green-500/20 flex items-center justify-center">
                        <Cloud class="w-5 h-5 text-green-500" />
                      </div>
                      <div>
                        <p class="text-sm font-semibold text-green-500">Connected</p>
                        <p class="text-sm text-[var(--text-secondary)]">{cloudStatus.email}</p>
                      </div>
                    </div>
                    <button onClick={handleLogout} class="btn btn-ghost flex items-center gap-2">
                      <LogOut class="w-4 h-4" />
                      Logout
                    </button>
                  </div>
                  <p class="text-sm text-[var(--text-muted)] pl-1">
                    Your custom filament presets are now available when adding spools.
                  </p>
                </div>
              ) : loginStep === 'idle' ? (
                <div class="flex flex-col sm:flex-row items-start sm:items-center justify-between gap-4 p-4 bg-[var(--bg-tertiary)]/50 rounded-xl">
                  <div class="flex items-center gap-3">
                    <div class="w-10 h-10 rounded-full bg-[var(--text-muted)]/10 flex items-center justify-center">
                      <CloudOff class="w-5 h-5 text-[var(--text-muted)]" />
                    </div>
                    <div>
                      <p class="text-sm font-medium text-[var(--text-primary)]">Not Connected</p>
                      <p class="text-xs text-[var(--text-muted)]">Login to access custom presets</p>
                    </div>
                  </div>
                  <button
                    onClick={() => setLoginStep('credentials')}
                    class="btn btn-primary flex items-center gap-2"
                  >
                    <Cloud class="w-4 h-4" />
                    Connect
                  </button>
                </div>
              ) : loginStep === 'credentials' ? (
                <div class="space-y-4 max-w-md">
                  <p class="text-sm text-[var(--text-secondary)]">
                    Enter your Bambu Lab credentials. A verification code will be sent to your email.
                  </p>
                  {loginError && (
                    <div class="flex items-center gap-2 p-3 bg-red-500/10 border border-red-500/30 rounded-lg text-red-500 text-sm">
                      <AlertCircle class="w-4 h-4 flex-shrink-0" />
                      {loginError}
                    </div>
                  )}
                  <div class="space-y-3">
                    <div>
                      <label class="label">Email</label>
                      <div class="relative">
                        <Mail class="absolute left-3 top-1/2 -translate-y-1/2 w-4 h-4 text-[var(--text-muted)]" />
                        <input
                          type="email"
                          class="input input-with-icon"
                          placeholder="your@email.com"
                          value={email}
                          onInput={(e) => setEmail((e.target as HTMLInputElement).value)}
                          disabled={loginLoading}
                        />
                      </div>
                    </div>
                    <div>
                      <label class="label">Password</label>
                      <div class="relative">
                        <Lock class="absolute left-3 top-1/2 -translate-y-1/2 w-4 h-4 text-[var(--text-muted)]" />
                        <input
                          type="password"
                          class="input input-with-icon"
                          placeholder="Password"
                          value={password}
                          onInput={(e) => setPassword((e.target as HTMLInputElement).value)}
                          disabled={loginLoading}
                          onKeyDown={(e) => e.key === 'Enter' && handleLogin()}
                        />
                      </div>
                    </div>
                  </div>
                  <div class="flex gap-3 pt-2">
                    <button onClick={cancelLogin} class="btn btn-ghost" disabled={loginLoading}>
                      Cancel
                    </button>
                    <button onClick={handleLogin} class="btn btn-primary flex items-center gap-2" disabled={loginLoading}>
                      {loginLoading ? <Loader2 class="w-4 h-4 animate-spin" /> : null}
                      {loginLoading ? 'Logging in...' : 'Continue'}
                    </button>
                  </div>
                </div>
              ) : (
                <div class="space-y-4 max-w-md">
                  <p class="text-sm text-[var(--text-secondary)]">
                    A verification code has been sent to <strong class="text-[var(--text-primary)]">{email}</strong>
                  </p>
                  {loginError && (
                    <div class="flex items-center gap-2 p-3 bg-red-500/10 border border-red-500/30 rounded-lg text-red-500 text-sm">
                      <AlertCircle class="w-4 h-4 flex-shrink-0" />
                      {loginError}
                    </div>
                  )}
                  <div>
                    <label class="label">Verification Code</label>
                    <div class="relative">
                      <Key class="absolute left-3 top-1/2 -translate-y-1/2 w-4 h-4 text-[var(--text-muted)]" />
                      <input
                        type="text"
                        class="input input-with-icon font-mono tracking-wider"
                        placeholder="000000"
                        value={verifyCode}
                        onInput={(e) => setVerifyCode((e.target as HTMLInputElement).value)}
                        disabled={loginLoading}
                        onKeyDown={(e) => e.key === 'Enter' && handleVerify()}
                      />
                    </div>
                  </div>
                  <div class="flex gap-3 pt-2">
                    <button onClick={cancelLogin} class="btn btn-ghost" disabled={loginLoading}>
                      Cancel
                    </button>
                    <button onClick={handleVerify} class="btn btn-primary flex items-center gap-2" disabled={loginLoading}>
                      {loginLoading ? <Loader2 class="w-4 h-4 animate-spin" /> : null}
                      {loginLoading ? 'Verifying...' : 'Verify'}
                    </button>
                  </div>
                </div>
              )}
            </SettingsCard>

            {/* Appearance Settings */}
            <SettingsCard
              id="appearance"
              icon={Palette}
              title="Appearance"
              description="Customize the look and feel"
            >
              <div class="space-y-6">
                {/* Mode Toggle */}
                <SettingRow
                  label="Theme Mode"
                  description="Switch between light and dark mode"
                >
                  <button
                    onClick={toggleMode}
                    class="flex items-center gap-2 px-4 py-2.5 rounded-xl bg-[var(--bg-tertiary)] hover:bg-[var(--border-color)] transition-all border border-[var(--border-color)]"
                  >
                    {mode === "dark" ? (
                      <>
                        <Moon class="w-4 h-4 text-[var(--accent)]" />
                        <span class="text-sm font-medium text-[var(--text-primary)]">Dark</span>
                      </>
                    ) : (
                      <>
                        <Sun class="w-4 h-4 text-[var(--accent)]" />
                        <span class="text-sm font-medium text-[var(--text-primary)]">Light</span>
                      </>
                    )}
                  </button>
                </SettingRow>

                {/* Theme customization panels */}
                <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
                  {/* Dark Mode Settings */}
                  <div class="p-4 rounded-xl bg-[var(--bg-tertiary)]/50 border border-[var(--border-color)]">
                    <h3 class="text-sm font-semibold text-[var(--text-primary)] mb-4 flex items-center gap-2">
                      <Moon class="w-4 h-4 text-[var(--accent)]" />
                      Dark Mode
                    </h3>
                    <div class="space-y-3">
                      <div>
                        <label class="block text-xs text-[var(--text-muted)] mb-1.5">Background</label>
                        <select
                          value={darkBackground}
                          onChange={(e) => { setDarkBackground((e.target as HTMLSelectElement).value as DarkBackground); showToast('success', 'Theme updated'); }}
                          class="w-full px-3 py-2 text-sm bg-[var(--bg-secondary)] border border-[var(--border-color)] rounded-lg text-[var(--text-primary)] focus:border-[var(--accent)] focus:outline-none"
                        >
                          <option value="neutral">Neutral</option>
                          <option value="warm">Warm</option>
                          <option value="cool">Cool</option>
                          <option value="oled">OLED Black</option>
                          <option value="slate">Slate</option>
                          <option value="forest">Forest</option>
                        </select>
                      </div>
                      <div>
                        <label class="block text-xs text-[var(--text-muted)] mb-1.5">Accent Color</label>
                        <select
                          value={darkAccent}
                          onChange={(e) => { setDarkAccent((e.target as HTMLSelectElement).value as ThemeAccent); showToast('success', 'Theme updated'); }}
                          class="w-full px-3 py-2 text-sm bg-[var(--bg-secondary)] border border-[var(--border-color)] rounded-lg text-[var(--text-primary)] focus:border-[var(--accent)] focus:outline-none"
                        >
                          <option value="green">Green</option>
                          <option value="teal">Teal</option>
                          <option value="blue">Blue</option>
                          <option value="orange">Orange</option>
                          <option value="purple">Purple</option>
                          <option value="red">Red</option>
                        </select>
                      </div>
                      <div>
                        <label class="block text-xs text-[var(--text-muted)] mb-1.5">Style</label>
                        <select
                          value={darkStyle}
                          onChange={(e) => { setDarkStyle((e.target as HTMLSelectElement).value as ThemeStyle); showToast('success', 'Theme updated'); }}
                          class="w-full px-3 py-2 text-sm bg-[var(--bg-secondary)] border border-[var(--border-color)] rounded-lg text-[var(--text-primary)] focus:border-[var(--accent)] focus:outline-none"
                        >
                          <option value="classic">Classic</option>
                          <option value="glow">Glow</option>
                          <option value="vibrant">Vibrant</option>
                        </select>
                      </div>
                    </div>
                  </div>

                  {/* Light Mode Settings */}
                  <div class="p-4 rounded-xl bg-[var(--bg-tertiary)]/50 border border-[var(--border-color)]">
                    <h3 class="text-sm font-semibold text-[var(--text-primary)] mb-4 flex items-center gap-2">
                      <Sun class="w-4 h-4 text-[var(--accent)]" />
                      Light Mode
                    </h3>
                    <div class="space-y-3">
                      <div>
                        <label class="block text-xs text-[var(--text-muted)] mb-1.5">Background</label>
                        <select
                          value={lightBackground}
                          onChange={(e) => { setLightBackground((e.target as HTMLSelectElement).value as LightBackground); showToast('success', 'Theme updated'); }}
                          class="w-full px-3 py-2 text-sm bg-[var(--bg-secondary)] border border-[var(--border-color)] rounded-lg text-[var(--text-primary)] focus:border-[var(--accent)] focus:outline-none"
                        >
                          <option value="neutral">Neutral</option>
                          <option value="warm">Warm</option>
                          <option value="cool">Cool</option>
                        </select>
                      </div>
                      <div>
                        <label class="block text-xs text-[var(--text-muted)] mb-1.5">Accent Color</label>
                        <select
                          value={lightAccent}
                          onChange={(e) => { setLightAccent((e.target as HTMLSelectElement).value as ThemeAccent); showToast('success', 'Theme updated'); }}
                          class="w-full px-3 py-2 text-sm bg-[var(--bg-secondary)] border border-[var(--border-color)] rounded-lg text-[var(--text-primary)] focus:border-[var(--accent)] focus:outline-none"
                        >
                          <option value="green">Green</option>
                          <option value="teal">Teal</option>
                          <option value="blue">Blue</option>
                          <option value="orange">Orange</option>
                          <option value="purple">Purple</option>
                          <option value="red">Red</option>
                        </select>
                      </div>
                      <div>
                        <label class="block text-xs text-[var(--text-muted)] mb-1.5">Style</label>
                        <select
                          value={lightStyle}
                          onChange={(e) => { setLightStyle((e.target as HTMLSelectElement).value as ThemeStyle); showToast('success', 'Theme updated'); }}
                          class="w-full px-3 py-2 text-sm bg-[var(--bg-secondary)] border border-[var(--border-color)] rounded-lg text-[var(--text-primary)] focus:border-[var(--accent)] focus:outline-none"
                        >
                          <option value="classic">Classic</option>
                          <option value="glow">Glow</option>
                          <option value="vibrant">Vibrant</option>
                        </select>
                      </div>
                    </div>
                  </div>
                </div>
              </div>
            </SettingsCard>

            {/* About */}
            <SettingsCard
              id="about"
              icon={ExternalLink}
              title="About SpoolBuddy"
              description="Filament management for Bambu Lab printers"
            >
              <div class="flex items-center gap-4">
                <a
                  href="https://github.com/maziggy/spoolbuddy"
                  target="_blank"
                  rel="noopener noreferrer"
                  class="flex items-center gap-2 text-sm text-[var(--accent)] hover:underline"
                >
                  <GitBranch class="w-4 h-4" />
                  GitHub
                </a>
                <a
                  href="https://github.com/maziggy/spoolbuddy/issues"
                  target="_blank"
                  rel="noopener noreferrer"
                  class="flex items-center gap-2 text-sm text-[var(--accent)] hover:underline"
                >
                  <AlertCircle class="w-4 h-4" />
                  Report Issue
                </a>
              </div>
            </SettingsCard>
          </div>
        )}

        {/* ============ FILAMENT TAB ============ */}
        {activeTab === 'filament' && (
          <div class="space-y-6">
            {/* Top row - Dashboard and AMS settings */}
            <div class="grid grid-cols-1 lg:grid-cols-2 gap-6">
              <div id="dashboard" class="scroll-mt-20">
                <DashboardSettings />
              </div>
              <div id="ams" class="scroll-mt-20">
                <AMSSettings />
              </div>
            </div>

            {/* Full width - Spool Catalog */}
            <div id="catalog" class="scroll-mt-20">
              <SpoolCatalogSettings />
            </div>

            {/* Full width - Color Catalog */}
            <div id="colors" class="scroll-mt-20">
              <ColorCatalogSettings />
            </div>
          </div>
        )}

        {/* ============ API TAB ============ */}
        {activeTab === 'api' && (
          <div class="grid grid-cols-1 xl:grid-cols-2 gap-8">
            {/* Left Column - API Keys Management */}
            <div class="space-y-6">
              <div class="flex items-start justify-between gap-4">
                <div class="flex-1">
                  <h2 class="text-lg font-semibold text-[var(--text-primary)] flex items-center gap-2">
                    <Key class="w-5 h-5 text-[var(--accent)]" />
                    API Keys
                  </h2>
                  <p class="text-sm text-[var(--text-muted)] mt-1">
                    Create API keys for external integrations and webhooks.
                  </p>
                </div>
                <button onClick={() => setShowCreateApiKey(true)} class="btn btn-primary flex items-center gap-2">
                  <Plus class="w-4 h-4" />
                  Create Key
                </button>
              </div>

              {/* Created Key Display */}
              {createdApiKey && (
                <div class="card border-[var(--accent)]">
                  <div class="p-4">
                    <div class="flex items-start gap-3">
                      <CheckCircle class="w-5 h-5 text-green-500 flex-shrink-0 mt-0.5" />
                      <div class="flex-1 min-w-0">
                        <p class="text-[var(--text-primary)] font-medium mb-1">API Key Created Successfully</p>
                        <p class="text-sm text-[var(--text-muted)] mb-2">
                          Copy this key now - it won't be shown again!
                        </p>
                        <div class="flex items-center gap-2 bg-[var(--bg-secondary)] rounded-lg p-2">
                          <code class="flex-1 text-sm text-[var(--accent)] font-mono break-all">
                            {createdApiKey}
                          </code>
                          <button
                            class="btn btn-ghost p-1.5"
                            onClick={() => copyToClipboard(createdApiKey)}
                          >
                            <Copy class="w-4 h-4" />
                          </button>
                        </div>
                        <div class="flex gap-2 mt-3">
                          <button
                            class="btn btn-ghost text-sm"
                            onClick={() => {
                              setTestApiKey(createdApiKey);
                              showToast('success', 'Key added to API Browser');
                            }}
                          >
                            Use in API Browser
                          </button>
                          <button
                            class="btn btn-ghost text-sm"
                            onClick={() => setCreatedApiKey(null)}
                          >
                            Dismiss
                          </button>
                        </div>
                      </div>
                    </div>
                  </div>
                </div>
              )}

              {/* Create Key Form */}
              {showCreateApiKey && (
                <div class="card">
                  <div class="px-5 py-4 bg-[var(--bg-tertiary)]/50 border-b border-[var(--border-color)]">
                    <h3 class="text-base font-semibold text-[var(--text-primary)]">Create New API Key</h3>
                  </div>
                  <div class="p-5 space-y-4">
                    <div>
                      <label class="block text-sm text-[var(--text-muted)] mb-1">Key Name</label>
                      <input
                        type="text"
                        value={newApiKeyName}
                        onInput={(e) => setNewApiKeyName((e.target as HTMLInputElement).value)}
                        placeholder="e.g., Home Assistant, OctoPrint"
                        class="input w-full"
                      />
                    </div>
                    <div>
                      <label class="block text-sm text-[var(--text-muted)] mb-2">Permissions</label>
                      <div class="space-y-2">
                        <label class="flex items-center gap-3 cursor-pointer">
                          <input
                            type="checkbox"
                            checked={newApiKeyPermissions.can_read}
                            onChange={(e) => setNewApiKeyPermissions(prev => ({ ...prev, can_read: (e.target as HTMLInputElement).checked }))}
                            class="w-4 h-4 rounded border-[var(--border-color)] text-[var(--accent)] focus:ring-[var(--accent)]"
                          />
                          <div>
                            <span class="text-[var(--text-primary)]">Read</span>
                            <p class="text-xs text-[var(--text-muted)]">View spools, printers, and status</p>
                          </div>
                        </label>
                        <label class="flex items-center gap-3 cursor-pointer">
                          <input
                            type="checkbox"
                            checked={newApiKeyPermissions.can_write}
                            onChange={(e) => setNewApiKeyPermissions(prev => ({ ...prev, can_write: (e.target as HTMLInputElement).checked }))}
                            class="w-4 h-4 rounded border-[var(--border-color)] text-[var(--accent)] focus:ring-[var(--accent)]"
                          />
                          <div>
                            <span class="text-[var(--text-primary)]">Write</span>
                            <p class="text-xs text-[var(--text-muted)]">Create and modify spools and settings</p>
                          </div>
                        </label>
                        <label class="flex items-center gap-3 cursor-pointer">
                          <input
                            type="checkbox"
                            checked={newApiKeyPermissions.can_control}
                            onChange={(e) => setNewApiKeyPermissions(prev => ({ ...prev, can_control: (e.target as HTMLInputElement).checked }))}
                            class="w-4 h-4 rounded border-[var(--border-color)] text-[var(--accent)] focus:ring-[var(--accent)]"
                          />
                          <div>
                            <span class="text-[var(--text-primary)]">Control</span>
                            <p class="text-xs text-[var(--text-muted)]">Control printers and AMS slots</p>
                          </div>
                        </label>
                      </div>
                    </div>
                    <div class="flex items-center gap-2 pt-2">
                      <button
                        onClick={handleCreateApiKey}
                        disabled={creatingApiKey}
                        class="btn btn-primary flex items-center gap-2"
                      >
                        {creatingApiKey ? (
                          <Loader2 class="w-4 h-4 animate-spin" />
                        ) : (
                          <Plus class="w-4 h-4" />
                        )}
                        Create Key
                      </button>
                      <button class="btn btn-ghost" onClick={() => setShowCreateApiKey(false)}>
                        Cancel
                      </button>
                    </div>
                  </div>
                </div>
              )}

              {/* Existing Keys List */}
              {loadingApiKeys ? (
                <div class="flex justify-center py-12">
                  <Loader2 class="w-8 h-8 text-[var(--accent)] animate-spin" />
                </div>
              ) : apiKeys.length > 0 ? (
                <div class="space-y-3">
                  {apiKeys.map((key) => (
                    <div key={key.id} class="card">
                      <div class="p-4">
                        <div class="flex items-center justify-between">
                          <div class="flex items-center gap-3">
                            <Key class={`w-5 h-5 ${key.enabled ? 'text-[var(--accent)]' : 'text-[var(--text-muted)]'}`} />
                            <div>
                              <p class="text-[var(--text-primary)] font-medium">{key.name}</p>
                              <p class="text-xs text-[var(--text-muted)]">
                                {key.key_prefix}........
                                {key.last_used && ` · Last used: ${formatTimestamp(key.last_used)}`}
                              </p>
                            </div>
                          </div>
                          <div class="flex items-center gap-2">
                            <div class="flex gap-1 text-xs">
                              {key.can_read && (
                                <span class="px-1.5 py-0.5 bg-blue-500/20 text-blue-400 rounded">Read</span>
                              )}
                              {key.can_write && (
                                <span class="px-1.5 py-0.5 bg-green-500/20 text-green-400 rounded">Write</span>
                              )}
                              {key.can_control && (
                                <span class="px-1.5 py-0.5 bg-orange-500/20 text-orange-400 rounded">Control</span>
                              )}
                            </div>
                            <button
                              class="btn btn-ghost p-1.5"
                              onClick={() => setDeleteKeyId(key.id)}
                            >
                              <Trash2 class="w-4 h-4 text-red-400" />
                            </button>
                          </div>
                        </div>
                      </div>
                    </div>
                  ))}
                </div>
              ) : (
                <div class="card">
                  <div class="p-12 text-center">
                    <Key class="w-16 h-16 mx-auto mb-4 text-[var(--text-muted)] opacity-30" />
                    <p class="text-lg font-medium text-[var(--text-primary)] mb-2">No API keys</p>
                    <p class="text-sm text-[var(--text-muted)] mb-4">Create an API key to integrate with external services.</p>
                    <button onClick={() => setShowCreateApiKey(true)} class="btn btn-primary flex items-center gap-2 mx-auto">
                      <Plus class="w-4 h-4" />
                      Create Your First Key
                    </button>
                  </div>
                </div>
              )}

              {/* API Usage Documentation */}
              <div class="card">
                <div class="px-5 py-4 bg-[var(--bg-tertiary)]/50 border-b border-[var(--border-color)]">
                  <h3 class="text-base font-semibold text-[var(--text-primary)]">Using API Keys</h3>
                </div>
                <div class="p-5 space-y-3 text-sm">
                  <p class="text-[var(--text-muted)]">
                    Use your API key in the <code class="text-[var(--accent)] bg-[var(--bg-tertiary)] px-1 rounded">X-API-Key</code> header or as <code class="text-[var(--accent)] bg-[var(--bg-tertiary)] px-1 rounded">Authorization: Bearer &lt;key&gt;</code>.
                  </p>
                  <div class="space-y-2 font-mono text-xs">
                    <div class="p-2 bg-[var(--bg-secondary)] rounded">
                      <span class="text-blue-400">GET</span>{' '}
                      <span class="text-[var(--text-primary)]">/api/spools</span>
                      <span class="text-[var(--text-muted)]"> - List all spools</span>
                    </div>
                    <div class="p-2 bg-[var(--bg-secondary)] rounded">
                      <span class="text-blue-400">GET</span>{' '}
                      <span class="text-[var(--text-primary)]">/api/printers</span>
                      <span class="text-[var(--text-muted)]"> - List all printers</span>
                    </div>
                    <div class="p-2 bg-[var(--bg-secondary)] rounded">
                      <span class="text-green-400">POST</span>{' '}
                      <span class="text-[var(--text-primary)]">/api/spools</span>
                      <span class="text-[var(--text-muted)]"> - Create a spool</span>
                    </div>
                  </div>
                </div>
              </div>
            </div>

            {/* Right Column - API Browser */}
            <div class="space-y-6">
              <div>
                <h2 class="text-lg font-semibold text-[var(--text-primary)] flex items-center gap-2">
                  <Globe class="w-5 h-5 text-[var(--accent)]" />
                  API Browser
                </h2>
                <p class="text-sm text-[var(--text-muted)] mt-1">
                  Explore and test all available API endpoints.
                </p>
              </div>

              {/* API Key Input for Testing */}
              <div class="card">
                <div class="p-4">
                  <label class="block text-sm text-[var(--text-muted)] mb-2">API Key for Testing</label>
                  <input
                    type="text"
                    value={testApiKey}
                    onInput={(e) => setTestApiKey((e.target as HTMLInputElement).value)}
                    placeholder="Paste your API key here to test authenticated endpoints..."
                    class="input w-full font-mono text-sm"
                  />
                  <p class="text-xs text-[var(--text-muted)] mt-2">
                    This key will be sent as <code class="text-[var(--accent)]">X-API-Key</code> header with requests.
                  </p>
                </div>
              </div>

              <APIBrowser apiKey={testApiKey} />
            </div>
          </div>
        )}

        {/* Delete API Key Confirmation Modal */}
        {deleteKeyId !== null && (
          <div class="fixed inset-0 bg-black/50 flex items-center justify-center z-50 p-4">
            <div class="card max-w-md w-full">
              <div class="p-5">
                <h3 class="text-lg font-semibold text-[var(--text-primary)] mb-2">Delete API Key</h3>
                <p class="text-sm text-[var(--text-muted)] mb-4">
                  Are you sure you want to delete this API key? Any integrations using this key will stop working.
                </p>
                <div class="flex justify-end gap-2">
                  <button class="btn btn-ghost" onClick={() => setDeleteKeyId(null)}>
                    Cancel
                  </button>
                  <button class="btn bg-red-500 hover:bg-red-600 text-white" onClick={() => handleDeleteApiKey(deleteKeyId)}>
                    Delete Key
                  </button>
                </div>
              </div>
            </div>
          </div>
        )}

        {/* ============ SUPPORT TAB ============ */}
        {activeTab === 'support' && (
          <div class="space-y-6">
            {/* System Information */}
            <SettingsCard
              id="system-info"
              icon={Server}
              title="System Information"
              description="Server status and resource usage"
              headerRight={
                <button
                  onClick={async () => {
                    setLoadingSystemInfo(true);
                    try {
                      const info = await api.getSystemInfo();
                      setSystemInfo(info);
                    } catch (e) {
                      showToast('error', 'Failed to refresh system info');
                    } finally {
                      setLoadingSystemInfo(false);
                    }
                  }}
                  disabled={loadingSystemInfo}
                  class="btn btn-ghost flex items-center gap-2"
                >
                  {loadingSystemInfo ? (
                    <Loader2 class="w-4 h-4 animate-spin" />
                  ) : (
                    <RefreshCw class="w-4 h-4" />
                  )}
                </button>
              }
            >
              {loadingSystemInfo && !systemInfo ? (
                <div class="flex items-center justify-center py-8">
                  <Loader2 class="w-6 h-6 animate-spin text-[var(--accent)]" />
                </div>
              ) : systemInfo ? (
                <div class="grid grid-cols-1 md:grid-cols-2 gap-6">
                  {/* Application Info */}
                  <div class="space-y-3">
                    <h4 class="text-xs font-semibold text-[var(--text-muted)] uppercase tracking-wider">Application</h4>
                    <div class="space-y-2">
                      <div class="flex justify-between">
                        <span class="text-sm text-[var(--text-muted)]">Version</span>
                        <span class="text-sm font-mono text-[var(--accent)]">v{systemInfo.version}</span>
                      </div>
                      <div class="flex justify-between">
                        <span class="text-sm text-[var(--text-muted)]">Uptime</span>
                        <span class="text-sm text-[var(--text-primary)]">{systemInfo.uptime}</span>
                      </div>
                      <div class="flex justify-between">
                        <span class="text-sm text-[var(--text-muted)]">Hostname</span>
                        <span class="text-sm text-[var(--text-primary)]">{systemInfo.hostname}</span>
                      </div>
                    </div>
                  </div>

                  {/* Database Info */}
                  <div class="space-y-3">
                    <h4 class="text-xs font-semibold text-[var(--text-muted)] uppercase tracking-wider flex items-center gap-2">
                      <Database class="w-3.5 h-3.5" />
                      Database
                    </h4>
                    <div class="space-y-2">
                      <div class="flex justify-between">
                        <span class="text-sm text-[var(--text-muted)]">Spools</span>
                        <span class="text-sm text-[var(--text-primary)]">{systemInfo.spool_count}</span>
                      </div>
                      <div class="flex justify-between">
                        <span class="text-sm text-[var(--text-muted)]">Printers</span>
                        <span class="text-sm text-[var(--text-primary)]">{systemInfo.printer_count} ({systemInfo.connected_printers} connected)</span>
                      </div>
                      <div class="flex justify-between">
                        <span class="text-sm text-[var(--text-muted)]">Size</span>
                        <span class="text-sm text-[var(--text-primary)]">{systemInfo.database_size}</span>
                      </div>
                    </div>
                  </div>

                  {/* System Info */}
                  <div class="space-y-3">
                    <h4 class="text-xs font-semibold text-[var(--text-muted)] uppercase tracking-wider flex items-center gap-2">
                      <Monitor class="w-3.5 h-3.5" />
                      System
                    </h4>
                    <div class="space-y-2">
                      <div class="flex justify-between">
                        <span class="text-sm text-[var(--text-muted)]">Platform</span>
                        <span class="text-sm text-[var(--text-primary)]">{systemInfo.platform} {systemInfo.platform_release}</span>
                      </div>
                      <div class="flex justify-between">
                        <span class="text-sm text-[var(--text-muted)]">Python</span>
                        <span class="text-sm font-mono text-[var(--text-primary)]">{systemInfo.python_version}</span>
                      </div>
                      <div class="flex justify-between">
                        <span class="text-sm text-[var(--text-muted)]">Boot Time</span>
                        <span class="text-sm text-[var(--text-primary)]">{systemInfo.boot_time}</span>
                      </div>
                    </div>
                  </div>

                  {/* Resources */}
                  <div class="space-y-3">
                    <h4 class="text-xs font-semibold text-[var(--text-muted)] uppercase tracking-wider flex items-center gap-2">
                      <Activity class="w-3.5 h-3.5" />
                      Resources
                    </h4>
                    <div class="space-y-2">
                      <div class="space-y-1">
                        <div class="flex justify-between text-sm">
                          <span class="text-[var(--text-muted)]">CPU ({systemInfo.cpu_count} cores)</span>
                          <span class="text-[var(--text-primary)]">{systemInfo.cpu_percent}%</span>
                        </div>
                        <div class="h-1.5 bg-[var(--bg-tertiary)] rounded-full overflow-hidden">
                          <div class="h-full bg-[var(--accent)] rounded-full" style={{ width: `${systemInfo.cpu_percent}%` }} />
                        </div>
                      </div>
                      <div class="space-y-1">
                        <div class="flex justify-between text-sm">
                          <span class="text-[var(--text-muted)]">Memory</span>
                          <span class="text-[var(--text-primary)]">{systemInfo.memory_used} / {systemInfo.memory_total} ({systemInfo.memory_percent}%)</span>
                        </div>
                        <div class="h-1.5 bg-[var(--bg-tertiary)] rounded-full overflow-hidden">
                          <div class="h-full bg-[var(--accent)] rounded-full" style={{ width: `${systemInfo.memory_percent}%` }} />
                        </div>
                      </div>
                      <div class="space-y-1">
                        <div class="flex justify-between text-sm">
                          <span class="text-[var(--text-muted)]">Disk</span>
                          <span class="text-[var(--text-primary)]">{systemInfo.disk_used} / {systemInfo.disk_total} ({systemInfo.disk_percent}%)</span>
                        </div>
                        <div class="h-1.5 bg-[var(--bg-tertiary)] rounded-full overflow-hidden">
                          <div class="h-full bg-[var(--accent)] rounded-full" style={{ width: `${systemInfo.disk_percent}%` }} />
                        </div>
                      </div>
                    </div>
                  </div>
                </div>
              ) : (
                <p class="text-sm text-[var(--text-muted)]">Failed to load system information</p>
              )}
            </SettingsCard>

            {/* Debug Logging & Support Bundle */}
            <SettingsCard
              id="debug"
              icon={Bug}
              title="Debug Logging"
              description="Enable verbose logging for troubleshooting"
            >
              <div class="space-y-5">
                {/* Debug Toggle */}
                <div class={`flex items-center justify-between p-4 rounded-xl border ${
                  debugLogging?.enabled
                    ? 'bg-amber-500/10 border-amber-500/30'
                    : 'bg-[var(--bg-tertiary)]/50 border-[var(--border-color)]'
                }`}>
                  <div class="flex items-center gap-3">
                    <div class={`w-10 h-10 rounded-full flex items-center justify-center ${
                      debugLogging?.enabled ? 'bg-amber-500/20' : 'bg-[var(--text-muted)]/10'
                    }`}>
                      <Bug class={`w-5 h-5 ${debugLogging?.enabled ? 'text-amber-500' : 'text-[var(--text-muted)]'}`} />
                    </div>
                    <div>
                      <p class="text-sm font-medium text-[var(--text-primary)]">Debug Logging</p>
                      <p class="text-xs text-[var(--text-muted)]">
                        {debugLogging?.enabled
                          ? 'Capturing detailed logs'
                          : 'Normal logging level'}
                        {debugLogging?.enabled && debugLogging.duration_seconds !== null && (
                          <span class="text-amber-400 ml-2">
                            ({Math.floor(debugLogging.duration_seconds / 60)}m {debugLogging.duration_seconds % 60}s)
                          </span>
                        )}
                      </p>
                    </div>
                  </div>
                  <button
                    onClick={handleToggleDebugLogging}
                    disabled={loadingDebug}
                    class={`px-4 py-2 rounded-lg font-medium transition-colors flex items-center gap-2 disabled:opacity-50 ${
                      debugLogging?.enabled
                        ? 'bg-amber-500/20 text-amber-400 hover:bg-amber-500/30'
                        : 'bg-green-500 text-white hover:bg-green-600'
                    }`}
                  >
                    {loadingDebug ? (
                      <Loader2 class="w-4 h-4 animate-spin" />
                    ) : debugLogging?.enabled ? (
                      <>Disable</>
                    ) : (
                      <>Enable</>
                    )}
                  </button>
                </div>

                {/* Support Bundle */}
                <div class="p-4 rounded-xl border border-[var(--border-color)] space-y-4">
                  <div class="flex items-center justify-between">
                    <div class="flex items-center gap-3">
                      <FileText class="w-5 h-5 text-[var(--accent)]" />
                      <div>
                        <p class="text-sm font-medium text-[var(--text-primary)]">Support Bundle</p>
                        <p class="text-xs text-[var(--text-muted)]">
                          Download logs and system info for troubleshooting
                        </p>
                      </div>
                    </div>
                    <button
                      onClick={handleDownloadBundle}
                      disabled={downloadingBundle || !debugLogging?.enabled}
                      class="btn btn-primary flex items-center gap-2"
                      title={!debugLogging?.enabled ? 'Enable debug logging first' : undefined}
                    >
                      {downloadingBundle ? (
                        <Loader2 class="w-4 h-4 animate-spin" />
                      ) : (
                        <Download class="w-4 h-4" />
                      )}
                      Download
                    </button>
                  </div>

                  {/* Privacy Info */}
                  <div class="p-4 bg-[var(--bg-tertiary)] rounded-lg space-y-3">
                    <p class="text-sm font-medium text-[var(--text-primary)]">What's in the support bundle?</p>
                    <div class="grid grid-cols-1 md:grid-cols-2 gap-4 text-sm">
                      <div>
                        <p class="text-green-500 font-medium mb-1">Collected:</p>
                        <ul class="text-[var(--text-muted)] space-y-0.5">
                          <li>• App version and uptime</li>
                          <li>• OS, platform, Python version</li>
                          <li>• Database statistics (counts only)</li>
                          <li>• Printer models (no names/serials)</li>
                          <li>• Resource usage (CPU, memory, disk)</li>
                          <li>• Debug logs (sanitized)</li>
                        </ul>
                      </div>
                      <div>
                        <p class="text-red-400 font-medium mb-1">NOT collected:</p>
                        <ul class="text-[var(--text-muted)] space-y-0.5">
                          <li>• Printer names, IPs, serial numbers</li>
                          <li>• Access codes and passwords</li>
                          <li>• Email addresses</li>
                          <li>• API keys and tokens</li>
                          <li>• Your hostname or username</li>
                          <li>• Personal file paths</li>
                        </ul>
                      </div>
                    </div>
                    <p class="text-xs text-[var(--text-muted)]/70">
                      IP addresses in logs are replaced with [IP], email addresses with [EMAIL], and usernames in paths with [user].
                    </p>
                  </div>

                  {/* Instructions when debug logging is disabled */}
                  {!debugLogging?.enabled && (
                    <div class="p-4 bg-amber-500/10 border border-amber-500/20 rounded-lg">
                      <p class="text-sm text-[var(--text-secondary)]">
                        <span class="text-amber-400 font-medium">To report an issue:</span>
                        <br />
                        1. Enable debug logging above
                        <br />
                        2. Reproduce the issue
                        <br />
                        3. Download the support bundle
                        <br />
                        4. Attach the ZIP file to your issue report
                      </p>
                    </div>
                  )}
                </div>
              </div>
            </SettingsCard>

            {/* Log Viewer */}
            <SettingsCard
              id="logs"
              icon={FileText}
              title="Application Logs"
              description="View and filter application log entries"
              headerRight={
                <div class="flex items-center gap-2">
                  {/* Live indicator */}
                  {logStreaming && (
                    <span class="flex items-center gap-1.5 px-2 py-1 bg-green-500/20 rounded text-green-500 text-xs">
                      <span class="w-1.5 h-1.5 bg-green-500 rounded-full animate-pulse" />
                      Live
                    </span>
                  )}
                  {/* Start/Stop button */}
                  {logStreaming ? (
                    <button
                      onClick={() => setLogStreaming(false)}
                      class="flex items-center gap-1.5 px-3 py-1.5 text-sm bg-red-500/20 text-red-400 hover:bg-red-500/30 rounded transition-colors"
                    >
                      <Square class="w-4 h-4" />
                      Stop
                    </button>
                  ) : (
                    <button
                      onClick={() => {
                        setLogStreaming(true);
                        refreshLogs();
                      }}
                      class="flex items-center gap-1.5 px-3 py-1.5 text-sm bg-green-500/20 text-green-500 hover:bg-green-500/30 rounded transition-colors"
                    >
                      <Play class="w-4 h-4" />
                      Start
                    </button>
                  )}
                  {/* Clear button */}
                  <button
                    onClick={handleClearLogs}
                    disabled={logs.length === 0}
                    class="flex items-center gap-1.5 px-3 py-1.5 text-sm bg-[var(--bg-tertiary)] text-[var(--text-muted)] hover:text-[var(--text-primary)] hover:bg-[var(--border-color)] rounded transition-colors disabled:opacity-50"
                  >
                    <Trash2 class="w-4 h-4" />
                    Clear
                  </button>
                  {/* Refresh button */}
                  <button
                    onClick={refreshLogs}
                    disabled={loadingLogs}
                    class="flex items-center gap-1.5 px-3 py-1.5 text-sm bg-[var(--bg-tertiary)] text-[var(--text-muted)] hover:text-[var(--text-primary)] hover:bg-[var(--border-color)] rounded transition-colors disabled:opacity-50"
                  >
                    <RefreshCw class={`w-4 h-4 ${loadingLogs ? 'animate-spin' : ''}`} />
                  </button>
                </div>
              }
            >
              <div class="space-y-4">
                {/* Filters */}
                <div class="flex flex-wrap gap-3">
                  <select
                    value={logLevel}
                    onChange={(e) => setLogLevel((e.target as HTMLSelectElement).value)}
                    class="px-3 py-2 text-sm bg-[var(--bg-secondary)] border border-[var(--border-color)] rounded-lg text-[var(--text-primary)] focus:border-[var(--accent)] focus:outline-none"
                  >
                    <option value="">All Levels</option>
                    <option value="DEBUG">DEBUG</option>
                    <option value="INFO">INFO</option>
                    <option value="WARNING">WARNING</option>
                    <option value="ERROR">ERROR</option>
                  </select>
                  <input
                    type="text"
                    placeholder="Search logs..."
                    value={logSearch}
                    onInput={(e) => setLogSearch((e.target as HTMLInputElement).value)}
                    class="flex-1 min-w-[200px] px-3 py-2 text-sm bg-[var(--bg-secondary)] border border-[var(--border-color)] rounded-lg text-[var(--text-primary)] focus:border-[var(--accent)] focus:outline-none"
                  />
                  <span class="text-xs text-[var(--text-muted)] self-center">
                    {logsFiltered} of {logsTotal} entries
                  </span>
                </div>

                {/* Log Entries */}
                <div class="max-h-[500px] overflow-auto rounded-lg border border-[var(--border-color)] bg-[var(--bg-secondary)]">
                  {loadingLogs && logs.length === 0 ? (
                    <div class="flex items-center justify-center py-8">
                      <Loader2 class="w-5 h-5 animate-spin text-[var(--text-muted)]" />
                    </div>
                  ) : logs.length === 0 ? (
                    <div class="flex flex-col items-center justify-center py-8 text-[var(--text-muted)]">
                      <FileText class="w-8 h-8 mb-2 opacity-50" />
                      <p class="text-sm">No log entries</p>
                      <p class="text-xs mt-1">Click Start to enable live log streaming</p>
                    </div>
                  ) : (
                    <div class="divide-y divide-[var(--border-color)]">
                      {logs.map((entry, i) => (
                        <div key={i} class="px-3 py-2 hover:bg-[var(--bg-tertiary)]/50 transition-colors">
                          <div class="flex items-start gap-2">
                            <span class={`text-xs font-mono px-1.5 py-0.5 rounded flex-shrink-0 ${
                              entry.level === 'ERROR' ? 'bg-red-500/20 text-red-500' :
                              entry.level === 'WARNING' ? 'bg-yellow-500/20 text-yellow-500' :
                              entry.level === 'DEBUG' ? 'bg-purple-500/20 text-purple-500' :
                              'bg-blue-500/20 text-blue-500'
                            }`}>
                              {entry.level}
                            </span>
                            <span class="text-xs font-mono text-[var(--text-muted)] flex-shrink-0">
                              {entry.timestamp.split(' ')[1]?.split(',')[0] || entry.timestamp}
                            </span>
                            <span class="text-xs text-[var(--accent)] flex-shrink-0">
                              [{entry.logger_name}]
                            </span>
                          </div>
                          <p class="text-sm text-[var(--text-primary)] mt-1 ml-0 font-mono whitespace-pre-wrap break-all">
                            {entry.message}
                          </p>
                        </div>
                      ))}
                    </div>
                  )}
                </div>

                {/* Footer status */}
                <div class="flex items-center justify-between text-sm text-[var(--text-muted)]">
                  {logStreaming ? (
                    <span class="flex items-center gap-2">
                      <span class="w-2 h-2 bg-green-500 rounded-full animate-pulse" />
                      Auto-refreshing every 2 seconds
                    </span>
                  ) : (
                    <span>Click Start to enable live log streaming</span>
                  )}
                </div>
              </div>
            </SettingsCard>

            {/* Help Section */}
            <SettingsCard
              id="help"
              icon={HelpCircle}
              title="Get Help"
              description="Resources for troubleshooting issues"
            >
              <div class="flex flex-wrap gap-3">
                <a
                  href="https://github.com/maziggy/spoolbuddy/issues"
                  target="_blank"
                  rel="noopener noreferrer"
                  class="btn btn-ghost flex items-center gap-2"
                >
                  <AlertCircle class="w-4 h-4" />
                  Report Issue
                </a>
                <a
                  href="https://github.com/maziggy/spoolbuddy/wiki"
                  target="_blank"
                  rel="noopener noreferrer"
                  class="btn btn-ghost flex items-center gap-2"
                >
                  <ExternalLink class="w-4 h-4" />
                  Documentation
                </a>
              </div>
            </SettingsCard>
          </div>
        )}

        {/* ============ SYSTEM TAB ============ */}
        {activeTab === 'system' && (
          <div class="space-y-6">
            {/* ESP32 Device Connection */}
            <SettingsCard
              id="device"
              icon={Cpu}
              title="ESP32 Device"
              description="SpoolBuddy hardware status and controls"
            >
              <div class="space-y-5">
                {/* Connection Status */}
                <div class={`flex items-center justify-between p-4 rounded-xl border ${
                  deviceUpdating ? 'bg-yellow-500/10 border-yellow-500/20' :
                  deviceConnected ? 'bg-green-500/10 border-green-500/20' : 'bg-red-500/10 border-red-500/20'
                }`}>
                  <div class="flex items-center gap-3">
                    <div class={`w-10 h-10 rounded-full ${
                      deviceUpdating ? 'bg-yellow-500/20' :
                      deviceConnected ? 'bg-green-500/20' : 'bg-red-500/20'
                    } flex items-center justify-center`}>
                      {deviceUpdating ? (
                        <Loader2 class="w-5 h-5 text-yellow-500 animate-spin" />
                      ) : deviceConnected ? (
                        <Wifi class="w-5 h-5 text-green-500" />
                      ) : (
                        <WifiOff class="w-5 h-5 text-red-500" />
                      )}
                    </div>
                    <div>
                      <p class={`text-sm font-semibold ${
                        deviceUpdating ? 'text-yellow-500' :
                        deviceConnected ? 'text-green-500' : 'text-red-500'
                      }`}>
                        {deviceUpdating ? 'Updating...' : deviceConnected ? 'Connected' : 'Disconnected'}
                      </p>
                      <p class="text-xs text-[var(--text-muted)]">
                        {deviceUpdating ? 'Rebooting and installing firmware' :
                         deviceConnected ? 'Display sending heartbeats' : 'No heartbeat from display'}
                      </p>
                    </div>
                  </div>
                  {deviceConnected && !deviceUpdating && (
                    <button onClick={handleESP32Reboot} class="btn btn-ghost flex items-center gap-2">
                      <RotateCcw class="w-4 h-4" />
                      <span class="hidden sm:inline">Reboot</span>
                    </button>
                  )}
                </div>

                {/* Scale Section */}
                <div class="p-4 rounded-xl bg-[var(--bg-tertiary)]/50 border border-[var(--border-color)]">
                  <div class="flex items-center gap-2 mb-4">
                    <Scale class="w-4 h-4 text-[var(--accent)]" />
                    <h3 class="text-sm font-semibold text-[var(--text-primary)]">Scale</h3>
                  </div>
                  <div class="flex items-center justify-between">
                    <div class="flex items-center gap-4">
                      <div class="text-3xl font-mono font-bold text-[var(--text-primary)]">
                        {currentWeight !== null ? `${Math.abs(currentWeight) <= 20 ? 0 : Math.round(currentWeight)}g` : "—"}
                      </div>
                      {weightStable && currentWeight !== null && (
                        <span class="flex items-center gap-1.5 text-xs text-green-500 bg-green-500/10 px-2 py-1 rounded-full">
                          <span class="w-1.5 h-1.5 rounded-full bg-green-500"></span>
                          Stable
                        </span>
                      )}
                    </div>
                    <div class="flex gap-2">
                      <button onClick={handleTare} disabled={!deviceConnected} class="btn btn-ghost">
                        Tare
                      </button>
                      <button onClick={startCalibration} disabled={!deviceConnected} class="btn btn-primary">
                        Calibrate
                      </button>
                    </div>
                  </div>

                  {/* Reset calibration warning */}
                  <div class="mt-4 pt-4 border-t border-[var(--border-color)]">
                    {!showResetConfirm ? (
                      <div class="flex items-center justify-between">
                        <p class="text-xs text-[var(--text-muted)]">
                          Only reset if calibration produces completely wrong values
                        </p>
                        <button
                          onClick={() => setShowResetConfirm(true)}
                          disabled={!deviceConnected}
                          class="text-xs text-yellow-500 hover:text-yellow-400 font-medium disabled:opacity-50"
                        >
                          Reset Calibration
                        </button>
                      </div>
                    ) : (
                      <div class="flex items-center justify-between p-3 bg-red-500/10 border border-red-500/20 rounded-lg">
                        <p class="text-xs text-red-400 font-medium">
                          Reset calibration? You'll need to recalibrate.
                        </p>
                        <div class="flex gap-2">
                          <button
                            onClick={() => setShowResetConfirm(false)}
                            class="btn btn-ghost btn-sm"
                          >
                            Cancel
                          </button>
                          <button
                            onClick={() => {
                              handleResetCalibration();
                              setShowResetConfirm(false);
                            }}
                            class="btn btn-danger btn-sm"
                          >
                            Reset
                          </button>
                        </div>
                      </div>
                    )}
                  </div>
                </div>

                {/* USB Serial Terminal */}
                <details class="group">
                  <summary class="flex items-center gap-2 text-sm text-[var(--text-muted)] hover:text-[var(--text-primary)] cursor-pointer list-none">
                    <Usb class="w-4 h-4" />
                    <span>USB Serial Terminal</span>
                    <ChevronRight class="w-4 h-4 transition-transform group-open:rotate-90" />
                  </summary>
                  <div class="mt-4">
                    <SerialTerminal />
                  </div>
                </details>
              </div>
            </SettingsCard>

            {/* Software Updates */}
            <SettingsCard
              id="updates"
              icon={Download}
              title="Software Updates"
              description="Keep SpoolBuddy up to date"
              headerRight={
                <button
                  onClick={() => handleCheckUpdates(true)}
                  disabled={checkingUpdate || applyingUpdate}
                  class="btn btn-ghost flex items-center gap-2"
                >
                  {checkingUpdate ? (
                    <Loader2 class="w-4 h-4 animate-spin" />
                  ) : (
                    <RefreshCw class="w-4 h-4" />
                  )}
                  <span class="hidden sm:inline">{checkingUpdate ? 'Checking...' : 'Check'}</span>
                </button>
              }
            >
              <div class="space-y-5">
                {/* Current Version */}
                <div class="flex items-center justify-between p-4 rounded-xl bg-[var(--bg-tertiary)]/50 border border-[var(--border-color)]">
                  <div>
                    <p class="text-xs text-[var(--text-muted)] mb-1">Server Version</p>
                    <div class="flex items-center gap-3">
                      <span class="text-xl font-mono font-bold text-[var(--accent)]">
                        v{versionInfo?.version || '0.1.0'}
                      </span>
                      {versionInfo?.git_branch && (
                        <span class="inline-flex items-center gap-1 text-xs text-[var(--text-muted)] bg-[var(--bg-secondary)] px-2 py-1 rounded-full">
                          <GitBranch class="w-3 h-3" />
                          {versionInfo.git_branch}
                        </span>
                      )}
                    </div>
                  </div>
                  {updateCheck && !updateCheck.update_available && !updateCheck.error && (
                    <div class="flex items-center gap-2 text-green-500 bg-green-500/10 px-3 py-1.5 rounded-full">
                      <CheckCircle class="w-4 h-4" />
                      <span class="text-xs font-medium">Up to date</span>
                    </div>
                  )}
                </div>

                {/* Update Available */}
                {updateCheck && updateCheck.update_available && (
                  <div class="p-4 bg-[var(--accent)]/10 border border-[var(--accent)]/20 rounded-xl">
                    <div class="flex flex-col sm:flex-row sm:items-start justify-between gap-4">
                      <div>
                        <div class="flex items-center gap-2">
                          <Download class="w-5 h-5 text-[var(--accent)]" />
                          <h3 class="text-sm font-semibold text-[var(--text-primary)]">
                            Update Available: v{updateCheck.latest_version}
                          </h3>
                        </div>
                        {updateCheck.published_at && (
                          <p class="text-xs text-[var(--text-muted)] mt-1 ml-7">
                            Released {new Date(updateCheck.published_at).toLocaleDateString()}
                          </p>
                        )}
                        {updateCheck.release_notes && (
                          <p class="text-sm text-[var(--text-secondary)] mt-3 ml-7">
                            {updateCheck.release_notes.length > 200
                              ? updateCheck.release_notes.slice(0, 200) + '...'
                              : updateCheck.release_notes}
                          </p>
                        )}
                      </div>
                      <div class="flex gap-2 sm:flex-shrink-0">
                        {updateCheck.release_url && (
                          <a
                            href={updateCheck.release_url}
                            target="_blank"
                            rel="noopener noreferrer"
                            class="btn btn-ghost flex items-center gap-2"
                          >
                            <ExternalLink class="w-4 h-4" />
                            View
                          </a>
                        )}
                        <button
                          onClick={handleApplyUpdate}
                          disabled={applyingUpdate}
                          class="btn btn-primary flex items-center gap-2"
                        >
                          {applyingUpdate ? (
                            <Loader2 class="w-4 h-4 animate-spin" />
                          ) : (
                            <Download class="w-4 h-4" />
                          )}
                          {applyingUpdate ? 'Updating...' : 'Update'}
                        </button>
                      </div>
                    </div>
                  </div>
                )}

                {/* Update Status */}
                {updateStatus && updateStatus.status !== 'idle' && (
                  <div class={`p-4 rounded-xl ${
                    updateStatus.status === 'error'
                      ? 'bg-red-500/10 border border-red-500/20'
                      : updateStatus.status === 'restarting'
                      ? 'bg-green-500/10 border border-green-500/20'
                      : 'bg-[var(--bg-tertiary)]/50 border border-[var(--border-color)]'
                  }`}>
                    <div class="flex items-center gap-2">
                      {updateStatus.status === 'error' ? (
                        <AlertCircle class="w-5 h-5 text-red-500" />
                      ) : updateStatus.status === 'restarting' ? (
                        <CheckCircle class="w-5 h-5 text-green-500" />
                      ) : (
                        <Loader2 class="w-5 h-5 animate-spin text-[var(--accent)]" />
                      )}
                      <span class={`text-sm font-medium ${
                        updateStatus.status === 'error'
                          ? 'text-red-500'
                          : updateStatus.status === 'restarting'
                          ? 'text-green-500'
                          : 'text-[var(--text-primary)]'
                      }`}>
                        {updateStatus.message || updateStatus.status}
                      </span>
                    </div>
                    {updateStatus.error && (
                      <p class="text-sm text-red-400 mt-2 ml-7">{updateStatus.error}</p>
                    )}
                  </div>
                )}

                {/* Device Firmware Section */}
                <div class="pt-4 border-t border-[var(--border-color)]">
                  <div class="flex items-center gap-2 mb-4">
                    <HardDrive class="w-4 h-4 text-[var(--accent)]" />
                    <h3 class="text-sm font-semibold text-[var(--text-primary)]">Device Firmware</h3>
                  </div>

                  {/* Device Status */}
                  <div class={`p-4 rounded-xl border ${
                    deviceUpdating ? 'bg-yellow-500/10 border-yellow-500/20' :
                    deviceConnected ? 'bg-[var(--bg-tertiary)]/50 border-[var(--border-color)]' : 'bg-[var(--bg-tertiary)]/30 border-[var(--border-color)]'
                  }`}>
                    <div class="flex items-center justify-between">
                      <div class="flex items-center gap-3">
                        <div class={`w-10 h-10 rounded-full flex items-center justify-center ${
                          deviceUpdating ? 'bg-yellow-500/20' :
                          deviceConnected ? 'bg-green-500/20' : 'bg-[var(--text-muted)]/10'
                        }`}>
                          {deviceUpdating ? (
                            <Loader2 class="w-5 h-5 text-yellow-500 animate-spin" />
                          ) : (
                            <Cpu class={`w-5 h-5 ${deviceConnected ? 'text-green-500' : 'text-[var(--text-muted)]'}`} />
                          )}
                        </div>
                        <div>
                          <p class="text-sm font-medium text-[var(--text-primary)]">
                            {deviceUpdating ? 'Updating Device...' :
                             deviceConnected ? 'SpoolBuddy Display' : 'No Device Connected'}
                          </p>
                          <p class="text-xs text-[var(--text-muted)] font-mono">
                            {deviceUpdating ? 'Installing firmware...' :
                             deviceConnected ? (
                              deviceFirmwareVersion || firmwareCheck?.current_version
                                ? `v${deviceFirmwareVersion || firmwareCheck?.current_version}`
                                : 'Version unknown'
                             ) : 'Connect via WiFi'}
                          </p>
                        </div>
                      </div>
                      {deviceConnected && !deviceUpdating && (
                        <button
                          onClick={handleCheckFirmware}
                          disabled={checkingFirmware}
                          class="btn btn-ghost flex items-center gap-2"
                        >
                          {checkingFirmware ? (
                            <Loader2 class="w-4 h-4 animate-spin" />
                          ) : (
                            <RefreshCw class="w-4 h-4" />
                          )}
                          <span class="hidden sm:inline">{checkingFirmware ? 'Checking...' : 'Check'}</span>
                        </button>
                      )}
                    </div>
                  </div>

                  {/* Firmware Update Available */}
                  {firmwareCheck?.update_available && !deviceUpdating && (
                    <div class="mt-4 p-4 bg-[var(--accent)]/10 border border-[var(--accent)]/20 rounded-xl">
                      <div class="flex flex-col sm:flex-row sm:items-start justify-between gap-4">
                        <div>
                          <div class="flex items-center gap-2">
                            <Download class="w-5 h-5 text-[var(--accent)]" />
                            <h4 class="text-sm font-semibold text-[var(--text-primary)]">
                              Firmware v{firmwareCheck.latest_version} Available
                            </h4>
                          </div>
                          <p class="text-xs text-[var(--text-muted)] mt-2 ml-7">
                            {deviceFirmwareVersion || firmwareCheck?.current_version
                              ? `Current: v${deviceFirmwareVersion || firmwareCheck.current_version}`
                              : 'Current version unknown'}
                          </p>
                        </div>
                        <button
                          onClick={handleTriggerOTA}
                          disabled={!deviceConnected}
                          class="btn btn-primary flex items-center gap-2"
                        >
                          <Download class="w-4 h-4" />
                          Update
                        </button>
                      </div>
                    </div>
                  )}

                  {/* Firmware Up to Date */}
                  {firmwareCheck && !firmwareCheck.update_available && (deviceFirmwareVersion || firmwareCheck.current_version) && !deviceUpdating && (
                    <div class="mt-4 flex items-center gap-2 text-green-500 bg-green-500/10 px-3 py-2 rounded-lg w-fit">
                      <CheckCircle class="w-4 h-4" />
                      <span class="text-xs font-medium">Firmware up to date</span>
                    </div>
                  )}

                  {/* Upload Firmware (for developers) */}
                  <details class="group mt-4">
                    <summary class="flex items-center gap-2 text-xs text-[var(--text-muted)] hover:text-[var(--text-primary)] cursor-pointer list-none">
                      <Upload class="w-3.5 h-3.5" />
                      <span>Upload custom firmware</span>
                      <ChevronRight class="w-3.5 h-3.5 transition-transform group-open:rotate-90" />
                    </summary>
                    <div class="mt-3 p-4 bg-[var(--bg-tertiary)]/50 border border-[var(--border-color)] rounded-xl">
                      <div class="flex items-center justify-between">
                        <div>
                          <p class="text-sm font-medium text-[var(--text-primary)]">Upload .bin File</p>
                          <p class="text-xs text-[var(--text-muted)]">
                            Make firmware available for OTA
                          </p>
                        </div>
                        <label class="btn btn-ghost flex items-center gap-2 cursor-pointer">
                          {uploadingFirmware ? (
                            <Loader2 class="w-4 h-4 animate-spin" />
                          ) : (
                            <Upload class="w-4 h-4" />
                          )}
                          {uploadingFirmware ? 'Uploading...' : 'Choose'}
                          <input
                            type="file"
                            accept=".bin"
                            onChange={handleFirmwareUpload}
                            disabled={uploadingFirmware}
                            class="hidden"
                          />
                        </label>
                      </div>
                    </div>
                  </details>
                </div>
              </div>
            </SettingsCard>
          </div>
        )}

      </div>

      {/* Calibration Modal */}
      {calibrationStep !== 'idle' && (
        <div class="modal-overlay" onClick={cancelCalibration}>
          <div class="modal max-w-md" onClick={(e) => e.stopPropagation()}>
            {/* Header */}
            <div class="modal-header">
              <div class="flex items-center gap-3">
                <div class="w-9 h-9 rounded-lg bg-[var(--accent)]/10 flex items-center justify-center">
                  <Scale class="w-5 h-5 text-[var(--accent)]" />
                </div>
                <div>
                  <h2 class="modal-title">
                    {calibrationStep === 'complete' ? 'Calibration Complete' : 'Scale Calibration'}
                  </h2>
                  {calibrationStep !== 'complete' && (
                    <p class="text-xs text-[var(--text-muted)]">Step {calibrationStep === 'empty' ? '1' : '2'} of 2</p>
                  )}
                </div>
              </div>
              <button onClick={cancelCalibration} class="p-2 rounded-lg text-[var(--text-muted)] hover:text-[var(--text-primary)] hover:bg-[var(--bg-tertiary)] transition-colors">
                <X class="w-5 h-5" />
              </button>
            </div>

            {/* Content */}
            <div class="modal-body">
              {calibrationStep === 'empty' && (
                <div class="space-y-5">
                  <div class="p-4 bg-[var(--accent)]/5 border border-[var(--accent)]/20 rounded-xl">
                    <h3 class="text-sm font-semibold text-[var(--text-primary)] mb-1">Remove everything from the scale</h3>
                    <p class="text-xs text-[var(--text-muted)]">
                      Ensure scale is empty and stable before continuing
                    </p>
                  </div>

                  <div class="p-5 bg-[var(--bg-tertiary)]/50 rounded-xl border border-[var(--border-color)] text-center">
                    <p class="text-xs text-[var(--text-muted)] mb-2">Current reading</p>
                    <p class="text-4xl font-mono font-bold text-[var(--text-primary)]">
                      {currentWeight !== null ? `${Math.round(currentWeight)}g` : "—"}
                    </p>
                  </div>
                </div>
              )}

              {calibrationStep === 'weight' && (
                <div class="space-y-5">
                  <div class="p-4 bg-[var(--accent)]/5 border border-[var(--accent)]/20 rounded-xl">
                    <h3 class="text-sm font-semibold text-[var(--text-primary)] mb-1">Place calibration weight</h3>
                    <p class="text-xs text-[var(--text-muted)]">
                      Add a known weight and enter the exact value below
                    </p>
                  </div>

                  <div>
                    <label class="label">Known Weight (grams)</label>
                    <input
                      type="number"
                      min="10"
                      max="5000"
                      step="1"
                      value={calibrationWeight}
                      onInput={(e) => {
                        const val = parseInt((e.target as HTMLInputElement).value);
                        if (!isNaN(val) && val >= 1 && val <= 9999) {
                          setCalibrationWeight(val);
                        }
                      }}
                      class="input text-lg font-mono text-center"
                    />
                  </div>

                  <div class="p-4 bg-[var(--bg-tertiary)]/50 rounded-xl border border-[var(--border-color)]">
                    <div class="flex justify-between items-center mb-4">
                      <div class="text-center">
                        <p class="text-xs text-[var(--text-muted)] mb-1">Current</p>
                        <span class="text-2xl font-mono font-bold text-[var(--text-primary)]">
                          {currentWeight !== null ? `${Math.round(currentWeight)}g` : "—"}
                        </span>
                      </div>
                      <div class="text-[var(--text-muted)]">→</div>
                      <div class="text-center">
                        <p class="text-xs text-[var(--text-muted)] mb-1">Target</p>
                        <span class="text-2xl font-mono font-bold text-[var(--accent)]">
                          {calibrationWeight}g
                        </span>
                      </div>
                    </div>
                    {currentWeight !== null && (
                      <div class={`text-center py-2 rounded-lg ${
                        Math.abs(currentWeight - calibrationWeight) < 10
                          ? 'bg-green-500/10 text-green-500'
                          : 'bg-yellow-500/10 text-yellow-500'
                      }`}>
                        <span class="text-sm font-medium">
                          {Math.abs(currentWeight - calibrationWeight) < 10
                            ? 'Readings close - ready to calibrate'
                            : `Difference: ${Math.round(currentWeight - calibrationWeight)}g`}
                        </span>
                      </div>
                    )}
                    <div class={`mt-3 flex items-center justify-center gap-2 text-xs ${weightStable ? 'text-green-500' : 'text-yellow-500'}`}>
                      <div class={`w-2 h-2 rounded-full ${weightStable ? 'bg-green-500' : 'bg-yellow-500 animate-pulse'}`} />
                      {weightStable ? 'Weight stabilized' : 'Waiting for stability...'}
                    </div>
                  </div>
                </div>
              )}

              {calibrationStep === 'complete' && (
                <div class="text-center py-6">
                  <div class="w-20 h-20 rounded-full bg-green-500/10 flex items-center justify-center mx-auto mb-4">
                    <CheckCircle class="w-10 h-10 text-green-500" />
                  </div>
                  <h3 class="text-lg font-semibold text-[var(--text-primary)]">Calibration Complete!</h3>
                  <p class="mt-2 text-sm text-[var(--text-muted)]">
                    Your scale is now calibrated and ready to use.
                  </p>
                </div>
              )}
            </div>

            {/* Footer */}
            <div class="modal-footer">
              {calibrationStep !== 'complete' && (
                <button onClick={cancelCalibration} class="btn btn-ghost">
                  Cancel
                </button>
              )}
              <button
                onClick={handleCalibrationNext}
                disabled={calibrating}
                class="btn btn-primary flex items-center gap-2"
              >
                {calibrating && calibrationStep === 'empty' ? (
                  <>
                    <Loader2 class="w-4 h-4 animate-spin" />
                    Zeroing...
                  </>
                ) : calibrating && calibrationStep === 'weight' ? (
                  <>
                    <Loader2 class="w-4 h-4 animate-spin" />
                    Calibrating...
                  </>
                ) : calibrationStep === 'empty' ? (
                  <>
                    Tare & Continue
                    <ChevronRight class="w-4 h-4" />
                  </>
                ) : calibrationStep === 'weight' ? (
                  'Calibrate'
                ) : (
                  'Done'
                )}
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}
