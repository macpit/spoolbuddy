import { useState, useEffect, useCallback } from "preact/hooks";
import { useWebSocket } from "../lib/websocket";
import { api, CloudAuthStatus, VersionInfo, UpdateCheck, UpdateStatus, FirmwareCheck, AMSThresholds } from "../lib/api";
import { Cloud, CloudOff, LogOut, Loader2, Mail, Lock, Key, Download, RefreshCw, CheckCircle, AlertCircle, GitBranch, ExternalLink, Wifi, WifiOff, Cpu, Usb, RotateCcw, Upload, HardDrive, Palette, Sun, Moon, LayoutDashboard, Settings2, Package, Monitor, Scale, X, ChevronRight, Droplets, Thermometer } from "lucide-preact";
import { useToast } from "../lib/toast";
import { SerialTerminal } from "../components/SerialTerminal";
import { SpoolCatalogSettings } from "../components/SpoolCatalogSettings";
import { useTheme, type ThemeStyle, type DarkBackground, type LightBackground, type ThemeAccent } from "../lib/theme";

// Storage keys for dashboard settings
const SPOOL_DISPLAY_DURATION_KEY = 'spoolbuddy-spool-display-duration';
const DEFAULT_CORE_WEIGHT_KEY = 'spoolbuddy-default-core-weight';

function DashboardSettings() {
  const { showToast } = useToast();
  const [spoolDisplayDuration, setSpoolDisplayDuration] = useState<number>(() => {
    const stored = localStorage.getItem(SPOOL_DISPLAY_DURATION_KEY);
    if (stored) {
      const val = parseInt(stored, 10);
      if (val >= 0 && val <= 300) return val;
    }
    return 10; // Default 10 seconds
  });

  const [defaultCoreWeight, setDefaultCoreWeight] = useState<number>(() => {
    const stored = localStorage.getItem(DEFAULT_CORE_WEIGHT_KEY);
    if (stored) {
      const val = parseInt(stored, 10);
      if (val >= 0 && val <= 500) return val;
    }
    return 250; // Default 250g (typical Bambu spool core)
  });

  const handleDurationChange = (value: number) => {
    setSpoolDisplayDuration(value);
    localStorage.setItem(SPOOL_DISPLAY_DURATION_KEY, String(value));
    showToast('success', `Spool display duration set to ${value}s`);
  };

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
            <p class="text-sm font-medium text-[var(--text-primary)]">Spool Display Duration</p>
            <p class="text-xs text-[var(--text-muted)]">
              How long to show spool info after removing from scale
            </p>
          </div>
          <div class="flex items-center gap-2">
            <select
              value={spoolDisplayDuration}
              onChange={(e) => handleDurationChange(parseInt((e.target as HTMLSelectElement).value, 10))}
              class="px-3 py-1.5 text-sm bg-[var(--bg-secondary)] border border-[var(--border-color)] rounded-lg text-[var(--text-primary)] focus:border-[var(--accent)] focus:outline-none"
            >
              <option value="0">Immediately hide</option>
              <option value="5">5 seconds</option>
              <option value="10">10 seconds</option>
              <option value="15">15 seconds</option>
              <option value="30">30 seconds</option>
              <option value="60">1 minute</option>
              <option value="120">2 minutes</option>
              <option value="300">5 minutes</option>
            </select>
          </div>
        </div>

        <div class="flex items-center justify-between pt-4 border-t border-[var(--border-color)]">
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

type SettingsTab = 'general' | 'filament' | 'system';

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
  const [showTerminal, setShowTerminal] = useState(false);

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

  // Handle hash navigation and switch to correct tab
  useEffect(() => {
    if (window.location.hash) {
      const id = window.location.hash.slice(1);
      // Map section IDs to tabs
      const sectionToTab: Record<string, SettingsTab> = {
        'updates': 'system',
        'firmware': 'system',
        'device': 'system',
        'appearance': 'general',
        'cloud': 'general',
        'about': 'general',
        'dashboard': 'filament',
        'catalog': 'filament',
      };
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

  const cancelCalibration = () => {
    setCalibrationStep('idle');
    setCalibrating(false);
  };

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
  ];

  return (
    <div class="space-y-6">
      {/* Header */}
      <div>
        <h1 class="text-3xl font-bold text-[var(--text-primary)]">Settings</h1>
        <p class="text-[var(--text-secondary)]">Configure SpoolBuddy</p>
      </div>

      {/* Tab Navigation */}
      <div class="border-b border-[var(--border-color)]">
        <nav class="flex gap-1" aria-label="Tabs">
          {tabs.map((tab) => {
            const Icon = tab.icon;
            const isActive = activeTab === tab.id;
            return (
              <button
                key={tab.id}
                onClick={() => setActiveTab(tab.id)}
                class={`flex items-center gap-2 px-4 py-2.5 text-sm font-medium border-b-2 transition-colors ${
                  isActive
                    ? 'border-[var(--accent)] text-[var(--accent)]'
                    : 'border-transparent text-[var(--text-muted)] hover:text-[var(--text-primary)] hover:border-[var(--border-color)]'
                }`}
              >
                <Icon class="w-4 h-4" />
                {tab.label}
              </button>
            );
          })}
        </nav>
      </div>

      {/* Tab Content */}
      <div class="space-y-6">

        {/* ============ GENERAL TAB ============ */}
        {activeTab === 'general' && (
          <div class="space-y-6">
            {/* Bambu Cloud settings */}
            <div id="cloud" class="card scroll-mt-20">
              <div class="px-6 py-4 border-b border-[var(--border-color)]">
                <div class="flex items-center gap-2">
                  <Cloud class="w-5 h-5 text-[var(--text-muted)]" />
                  <h2 class="text-lg font-medium text-[var(--text-primary)]">Bambu Cloud</h2>
                </div>
              </div>
              <div class="p-6 space-y-4">
                {loadingCloud ? (
                  <div class="flex items-center gap-2 text-[var(--text-muted)]">
                    <Loader2 class="w-4 h-4 animate-spin" />
                    <span>Checking cloud status...</span>
                  </div>
                ) : cloudStatus?.is_authenticated ? (
                  <div class="space-y-4">
                    <div class="flex items-center justify-between">
                      <div class="flex items-center gap-3">
                        <div class="w-10 h-10 rounded-full bg-green-500/20 flex items-center justify-center">
                          <Cloud class="w-5 h-5 text-green-500" />
                        </div>
                        <div>
                          <p class="text-sm font-medium text-[var(--text-primary)]">Connected</p>
                          <p class="text-sm text-[var(--text-secondary)]">{cloudStatus.email}</p>
                        </div>
                      </div>
                      <button onClick={handleLogout} class="btn flex items-center gap-2">
                        <LogOut class="w-4 h-4" />
                        Logout
                      </button>
                    </div>
                    <p class="text-sm text-[var(--text-muted)]">
                      Your custom filament presets will be available when adding spools.
                    </p>
                  </div>
                ) : loginStep === 'idle' ? (
                  <div class="space-y-4">
                    <div class="flex items-center gap-3">
                      <div class="w-10 h-10 rounded-full bg-[var(--text-muted)]/20 flex items-center justify-center">
                        <CloudOff class="w-5 h-5 text-[var(--text-muted)]" />
                      </div>
                      <div>
                        <p class="text-sm font-medium text-[var(--text-primary)]">Not Connected</p>
                        <p class="text-sm text-[var(--text-secondary)]">Login to access custom filament presets</p>
                      </div>
                    </div>
                    <button
                      onClick={() => setLoginStep('credentials')}
                      class="btn btn-primary flex items-center gap-2"
                    >
                      <Cloud class="w-4 h-4" />
                      Login to Bambu Cloud
                    </button>
                  </div>
                ) : loginStep === 'credentials' ? (
                  <div class="space-y-4">
                    <p class="text-sm text-[var(--text-secondary)]">
                      Enter your Bambu Lab account credentials. A verification code will be sent to your email.
                    </p>
                    {loginError && (
                      <div class="p-3 bg-red-500/10 border border-red-500/30 rounded-lg text-red-500 text-sm">
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
                    <div class="flex gap-3">
                      <button onClick={cancelLogin} class="btn" disabled={loginLoading}>
                        Cancel
                      </button>
                      <button onClick={handleLogin} class="btn btn-primary flex items-center gap-2" disabled={loginLoading}>
                        {loginLoading ? <Loader2 class="w-4 h-4 animate-spin" /> : null}
                        {loginLoading ? 'Logging in...' : 'Login'}
                      </button>
                    </div>
                  </div>
                ) : (
                  <div class="space-y-4">
                    <p class="text-sm text-[var(--text-secondary)]">
                      A verification code has been sent to <strong>{email}</strong>. Enter it below.
                    </p>
                    {loginError && (
                      <div class="p-3 bg-red-500/10 border border-red-500/30 rounded-lg text-red-500 text-sm">
                        {loginError}
                      </div>
                    )}
                    <div>
                      <label class="label">Verification Code</label>
                      <div class="relative">
                        <Key class="absolute left-3 top-1/2 -translate-y-1/2 w-4 h-4 text-[var(--text-muted)]" />
                        <input
                          type="text"
                          class="input input-with-icon"
                          placeholder="Enter 6-digit code"
                          value={verifyCode}
                          onInput={(e) => setVerifyCode((e.target as HTMLInputElement).value)}
                          disabled={loginLoading}
                          onKeyDown={(e) => e.key === 'Enter' && handleVerify()}
                        />
                      </div>
                    </div>
                    <div class="flex gap-3">
                      <button onClick={cancelLogin} class="btn" disabled={loginLoading}>
                        Cancel
                      </button>
                      <button onClick={handleVerify} class="btn btn-primary flex items-center gap-2" disabled={loginLoading}>
                        {loginLoading ? <Loader2 class="w-4 h-4 animate-spin" /> : null}
                        {loginLoading ? 'Verifying...' : 'Verify'}
                      </button>
                    </div>
                  </div>
                )}
              </div>
            </div>

            {/* Appearance Settings */}
            <div id="appearance" class="card scroll-mt-20">
              <div class="px-6 py-4 border-b border-[var(--border-color)]">
                <div class="flex items-center gap-2">
                  <Palette class="w-5 h-5 text-[var(--text-muted)]" />
                  <h2 class="text-lg font-medium text-[var(--text-primary)]">Appearance</h2>
                </div>
              </div>
              <div class="p-6 space-y-6">
                {/* Mode Toggle */}
                <div class="flex items-center justify-between">
                  <div>
                    <p class="text-sm font-medium text-[var(--text-primary)]">Theme Mode</p>
                    <p class="text-sm text-[var(--text-secondary)]">Switch between light and dark mode</p>
                  </div>
                  <button
                    onClick={toggleMode}
                    class="flex items-center gap-2 px-4 py-2 rounded-lg bg-[var(--bg-tertiary)] hover:bg-[var(--border-color)] transition-colors"
                  >
                    {mode === "dark" ? (
                      <>
                        <Moon class="w-4 h-4 text-[var(--accent)]" />
                        <span class="text-sm text-[var(--text-primary)]">Dark</span>
                      </>
                    ) : (
                      <>
                        <Sun class="w-4 h-4 text-[var(--accent)]" />
                        <span class="text-sm text-[var(--text-primary)]">Light</span>
                      </>
                    )}
                  </button>
                </div>

                {/* Dark Mode Settings */}
                <div class="border-t border-[var(--border-color)] pt-6">
                  <h3 class="text-sm font-medium text-[var(--text-primary)] mb-4 flex items-center gap-2">
                    <Moon class="w-4 h-4" />
                    Dark Mode Settings
                  </h3>
                  <div class="grid grid-cols-3 gap-4">
                    <div>
                      <label class="block text-xs text-[var(--text-muted)] mb-1">Background</label>
                      <select
                        value={darkBackground}
                        onChange={(e) => { setDarkBackground((e.target as HTMLSelectElement).value as DarkBackground); showToast('success', 'Theme updated'); }}
                        class="w-full px-2 py-1.5 text-sm bg-[var(--bg-secondary)] border border-[var(--border-color)] rounded-lg text-[var(--text-primary)] focus:border-[var(--accent)] focus:outline-none"
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
                      <label class="block text-xs text-[var(--text-muted)] mb-1">Accent</label>
                      <select
                        value={darkAccent}
                        onChange={(e) => { setDarkAccent((e.target as HTMLSelectElement).value as ThemeAccent); showToast('success', 'Theme updated'); }}
                        class="w-full px-2 py-1.5 text-sm bg-[var(--bg-secondary)] border border-[var(--border-color)] rounded-lg text-[var(--text-primary)] focus:border-[var(--accent)] focus:outline-none"
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
                      <label class="block text-xs text-[var(--text-muted)] mb-1">Style</label>
                      <select
                        value={darkStyle}
                        onChange={(e) => { setDarkStyle((e.target as HTMLSelectElement).value as ThemeStyle); showToast('success', 'Theme updated'); }}
                        class="w-full px-2 py-1.5 text-sm bg-[var(--bg-secondary)] border border-[var(--border-color)] rounded-lg text-[var(--text-primary)] focus:border-[var(--accent)] focus:outline-none"
                      >
                        <option value="classic">Classic</option>
                        <option value="glow">Glow</option>
                        <option value="vibrant">Vibrant</option>
                      </select>
                    </div>
                  </div>
                </div>

                {/* Light Mode Settings */}
                <div class="border-t border-[var(--border-color)] pt-6">
                  <h3 class="text-sm font-medium text-[var(--text-primary)] mb-4 flex items-center gap-2">
                    <Sun class="w-4 h-4" />
                    Light Mode Settings
                  </h3>
                  <div class="grid grid-cols-3 gap-4">
                    <div>
                      <label class="block text-xs text-[var(--text-muted)] mb-1">Background</label>
                      <select
                        value={lightBackground}
                        onChange={(e) => { setLightBackground((e.target as HTMLSelectElement).value as LightBackground); showToast('success', 'Theme updated'); }}
                        class="w-full px-2 py-1.5 text-sm bg-[var(--bg-secondary)] border border-[var(--border-color)] rounded-lg text-[var(--text-primary)] focus:border-[var(--accent)] focus:outline-none"
                      >
                        <option value="neutral">Neutral</option>
                        <option value="warm">Warm</option>
                        <option value="cool">Cool</option>
                      </select>
                    </div>
                    <div>
                      <label class="block text-xs text-[var(--text-muted)] mb-1">Accent</label>
                      <select
                        value={lightAccent}
                        onChange={(e) => { setLightAccent((e.target as HTMLSelectElement).value as ThemeAccent); showToast('success', 'Theme updated'); }}
                        class="w-full px-2 py-1.5 text-sm bg-[var(--bg-secondary)] border border-[var(--border-color)] rounded-lg text-[var(--text-primary)] focus:border-[var(--accent)] focus:outline-none"
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
                      <label class="block text-xs text-[var(--text-muted)] mb-1">Style</label>
                      <select
                        value={lightStyle}
                        onChange={(e) => { setLightStyle((e.target as HTMLSelectElement).value as ThemeStyle); showToast('success', 'Theme updated'); }}
                        class="w-full px-2 py-1.5 text-sm bg-[var(--bg-secondary)] border border-[var(--border-color)] rounded-lg text-[var(--text-primary)] focus:border-[var(--accent)] focus:outline-none"
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

            {/* About */}
            <div id="about" class="card scroll-mt-20">
              <div class="px-6 py-4 border-b border-[var(--border-color)]">
                <h2 class="text-lg font-medium text-[var(--text-primary)]">About</h2>
              </div>
              <div class="p-6">
                <p class="text-sm text-[var(--text-secondary)]">
                  SpoolBuddy is a filament management system for Bambu Lab 3D printers.
                </p>
                <p class="mt-2 text-sm text-[var(--text-secondary)]">
                  Features include NFC tag reading, weight scale integration, and automatic AMS configuration.
                </p>
                <div class="mt-4 flex space-x-4">
                  <a
                    href="https://github.com/maziggy/spoolbuddy"
                    target="_blank"
                    rel="noopener noreferrer"
                    class="text-sm text-[var(--accent-color)] hover:text-[var(--accent-hover)]"
                  >
                    GitHub
                  </a>
                  <a
                    href="https://github.com/maziggy/spoolbuddy/issues"
                    target="_blank"
                    rel="noopener noreferrer"
                    class="text-sm text-[var(--accent-color)] hover:text-[var(--accent-hover)]"
                  >
                    Report Issue
                  </a>
                </div>
              </div>
            </div>
          </div>
        )}

        {/* ============ FILAMENT TAB ============ */}
        {activeTab === 'filament' && (
          <div class="grid grid-cols-1 lg:grid-cols-2 gap-6">
            {/* Left Column */}
            <div class="space-y-6">
              {/* Dashboard settings */}
              <div id="dashboard" class="scroll-mt-20">
                <DashboardSettings />
              </div>

              {/* AMS Settings */}
              <div id="ams" class="scroll-mt-20">
                <AMSSettings />
              </div>
            </div>

            {/* Right Column - Spool Catalog */}
            <div id="catalog" class="scroll-mt-20">
              <SpoolCatalogSettings />
            </div>
          </div>
        )}

        {/* ============ SYSTEM TAB ============ */}
        {activeTab === 'system' && (
          <div class="space-y-6">
            {/* ESP32 Device Connection */}
            <div id="device" class="card scroll-mt-20">
              <div class="px-6 py-4 border-b border-[var(--border-color)]">
                <div class="flex items-center gap-2">
                  <Cpu class="w-5 h-5 text-[var(--text-muted)]" />
                  <h2 class="text-lg font-medium text-[var(--text-primary)]">ESP32 Device</h2>
                </div>
              </div>
              <div class="p-6 space-y-6">
                {/* Connection Status */}
                <div class="flex items-center justify-between">
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
                      <p class="text-sm font-medium text-[var(--text-primary)]">
                        {deviceUpdating ? 'Updating...' : deviceConnected ? 'Connected' : 'Disconnected'}
                      </p>
                      <p class="text-sm text-[var(--text-secondary)]">
                        {deviceUpdating ? 'Device is rebooting and installing firmware' :
                         deviceConnected ? 'Display is sending heartbeats' : 'No heartbeat from display'}
                      </p>
                    </div>
                  </div>
                </div>

                {/* Scale reading */}
                <div class="border-t border-[var(--border-color)] pt-6">
                  <h3 class="text-sm font-medium text-[var(--text-primary)]">Scale</h3>
                  <div class="mt-4 flex items-center justify-between">
                    <div>
                      <p class="text-sm text-[var(--text-secondary)]">Current reading</p>
                      <p class="text-2xl font-mono text-[var(--text-primary)]">
                        {currentWeight !== null ? `${Math.round(currentWeight)}g` : "--"}
                      </p>
                    </div>
                    <div class="space-x-3">
                      <button onClick={handleTare} disabled={!deviceConnected} class="btn">
                        Tare
                      </button>
                      <button onClick={startCalibration} disabled={!deviceConnected} class="btn">
                        Calibrate
                      </button>
                    </div>
                  </div>
                  <div class="mt-4 p-3 bg-yellow-500/10 border border-yellow-500/30 rounded-lg">
                    {!showResetConfirm ? (
                      <div class="flex items-center justify-between">
                        <p class="text-xs text-yellow-600 dark:text-yellow-400">
                          Only reset if calibration produces completely wrong values.
                        </p>
                        <button
                          onClick={() => setShowResetConfirm(true)}
                          disabled={!deviceConnected}
                          class="px-3 py-1 text-xs bg-yellow-500 hover:bg-yellow-600 text-white rounded disabled:opacity-50"
                        >
                          Reset
                        </button>
                      </div>
                    ) : (
                      <div class="space-y-2">
                        <p class="text-xs text-yellow-600 dark:text-yellow-400 font-medium">
                          Are you sure? You will need to recalibrate the scale.
                        </p>
                        <div class="flex gap-2">
                          <button
                            onClick={() => setShowResetConfirm(false)}
                            class="px-3 py-1 text-xs bg-[var(--bg-tertiary)] hover:bg-[var(--border-color)] text-[var(--text-primary)] rounded"
                          >
                            Cancel
                          </button>
                          <button
                            onClick={() => {
                              handleResetCalibration();
                              setShowResetConfirm(false);
                            }}
                            class="px-3 py-1 text-xs bg-red-500 hover:bg-red-600 text-white rounded"
                          >
                            Yes, Reset
                          </button>
                        </div>
                      </div>
                    )}
                  </div>
                </div>

                {deviceConnected && (
                  <div class="border-t border-[var(--border-color)] pt-6 flex gap-3">
                    <button onClick={handleESP32Reboot} class="btn flex items-center gap-2">
                      <RotateCcw class="w-4 h-4" />
                      Reboot Device
                    </button>
                  </div>
                )}

                {/* USB Serial Terminal */}
                <div class="border-t border-[var(--border-color)] pt-6">
                  <button
                    onClick={() => setShowTerminal(!showTerminal)}
                    class="flex items-center gap-2 text-sm text-[var(--text-secondary)] hover:text-[var(--text-primary)]"
                  >
                    <Usb class="w-4 h-4" />
                    <span>USB Serial Terminal</span>
                    <span class="text-xs">{showTerminal ? '▲' : '▼'}</span>
                  </button>
                  {showTerminal && (
                    <div class="mt-4">
                      <SerialTerminal />
                    </div>
                  )}
                </div>
              </div>
            </div>

            {/* Software Updates */}
            <div id="updates" class="card scroll-mt-20">
              <div class="px-6 py-4 border-b border-[var(--border-color)]">
                <div class="flex items-center gap-2">
                  <Download class="w-5 h-5 text-[var(--text-muted)]" />
                  <h2 class="text-lg font-medium text-[var(--text-primary)]">Software Updates</h2>
                </div>
              </div>
              <div class="p-6 space-y-6">
                {/* Current Version */}
                <div class="flex items-center justify-between">
                  <div>
                    <h3 class="text-sm font-medium text-[var(--text-primary)]">Current Version</h3>
                    <div class="flex items-center gap-3 mt-1">
                      <span class="text-lg font-mono text-[var(--accent-color)]">
                        v{versionInfo?.version || '0.1.0'}
                      </span>
                      {versionInfo?.git_branch && (
                        <span class="inline-flex items-center gap-1 text-xs text-[var(--text-muted)] bg-[var(--card-bg)] px-2 py-1 rounded">
                          <GitBranch class="w-3 h-3" />
                          {versionInfo.git_branch}
                          {versionInfo.git_commit && ` (${versionInfo.git_commit})`}
                        </span>
                      )}
                    </div>
                  </div>
                  <button
                    onClick={() => handleCheckUpdates(true)}
                    disabled={checkingUpdate || applyingUpdate}
                    class="btn flex items-center gap-2"
                  >
                    {checkingUpdate ? (
                      <Loader2 class="w-4 h-4 animate-spin" />
                    ) : (
                      <RefreshCw class="w-4 h-4" />
                    )}
                    {checkingUpdate ? 'Checking...' : 'Check for Updates'}
                  </button>
                </div>

                {/* Update Available */}
                {updateCheck && updateCheck.update_available && (
                  <div class="border-t border-[var(--border-color)] pt-4">
                    <div class="p-4 bg-[var(--accent-color)]/10 border border-[var(--accent-color)]/30 rounded-lg">
                      <div class="flex items-start justify-between">
                        <div>
                          <div class="flex items-center gap-2">
                            <CheckCircle class="w-5 h-5 text-[var(--accent-color)]" />
                            <h3 class="text-sm font-medium text-[var(--text-primary)]">
                              Update Available: v{updateCheck.latest_version}
                            </h3>
                          </div>
                          {updateCheck.published_at && (
                            <p class="text-xs text-[var(--text-muted)] mt-1">
                              Released: {new Date(updateCheck.published_at).toLocaleDateString()}
                            </p>
                          )}
                          {updateCheck.release_notes && (
                            <p class="text-sm text-[var(--text-secondary)] mt-2 whitespace-pre-wrap">
                              {updateCheck.release_notes.length > 200
                                ? updateCheck.release_notes.slice(0, 200) + '...'
                                : updateCheck.release_notes}
                            </p>
                          )}
                        </div>
                        <div class="flex gap-2">
                          {updateCheck.release_url && (
                            <a
                              href={updateCheck.release_url}
                              target="_blank"
                              rel="noopener noreferrer"
                              class="btn flex items-center gap-2"
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
                            {applyingUpdate ? 'Updating...' : 'Update Now'}
                          </button>
                        </div>
                      </div>
                    </div>
                  </div>
                )}

                {/* Update Status */}
                {updateStatus && updateStatus.status !== 'idle' && (
                  <div class="border-t border-[var(--border-color)] pt-4">
                    <div class={`p-4 rounded-lg ${
                      updateStatus.status === 'error'
                        ? 'bg-red-500/10 border border-red-500/30'
                        : updateStatus.status === 'restarting'
                        ? 'bg-green-500/10 border border-green-500/30'
                        : 'bg-[var(--card-bg)] border border-[var(--border-color)]'
                    }`}>
                      <div class="flex items-center gap-2">
                        {updateStatus.status === 'error' ? (
                          <AlertCircle class="w-5 h-5 text-red-500" />
                        ) : updateStatus.status === 'restarting' ? (
                          <CheckCircle class="w-5 h-5 text-green-500" />
                        ) : (
                          <Loader2 class="w-5 h-5 animate-spin text-[var(--accent-color)]" />
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
                        <p class="text-sm text-red-500 mt-2">{updateStatus.error}</p>
                      )}
                    </div>
                  </div>
                )}

                {/* No Updates */}
                {updateCheck && !updateCheck.update_available && !updateCheck.error && (
                  <div class="border-t border-[var(--border-color)] pt-4">
                    <div class="flex items-center gap-2 text-green-500">
                      <CheckCircle class="w-5 h-5" />
                      <span class="text-sm">You are running the latest version</span>
                    </div>
                  </div>
                )}

                {/* Device Firmware Section */}
                <div class="border-t border-[var(--border-color)] pt-6">
                  <div class="flex items-center gap-2 mb-4">
                    <HardDrive class="w-5 h-5 text-[var(--text-muted)]" />
                    <h3 class="text-sm font-medium text-[var(--text-primary)]">Device Firmware</h3>
                  </div>

                  {/* Device Status */}
                  <div class="p-4 bg-[var(--card-bg)] border border-[var(--border-color)] rounded-lg mb-4">
                    <div class="flex items-center justify-between">
                      <div class="flex items-center gap-3">
                        <div class={`w-10 h-10 rounded-full flex items-center justify-center ${
                          deviceUpdating ? 'bg-yellow-500/20' :
                          deviceConnected ? 'bg-green-500/20' : 'bg-gray-500/20'
                        }`}>
                          {deviceUpdating ? (
                            <Loader2 class="w-5 h-5 text-yellow-500 animate-spin" />
                          ) : (
                            <Cpu class={`w-5 h-5 ${deviceConnected ? 'text-green-500' : 'text-gray-400'}`} />
                          )}
                        </div>
                        <div>
                          <p class="text-sm font-medium text-[var(--text-primary)]">
                            {deviceUpdating ? 'Updating Device...' :
                             deviceConnected ? 'SpoolBuddy Display' : 'No Device Connected'}
                          </p>
                          <p class="text-sm text-[var(--text-muted)] font-mono">
                            {deviceUpdating ? 'Rebooting and installing firmware...' :
                             deviceConnected ? (
                              deviceFirmwareVersion || firmwareCheck?.current_version
                                ? `v${deviceFirmwareVersion || firmwareCheck?.current_version}`
                                : 'Version unknown - update to enable reporting'
                             ) : 'Connect device via WiFi'}
                          </p>
                        </div>
                      </div>
                      {deviceConnected && !deviceUpdating && (
                        <button
                          onClick={handleCheckFirmware}
                          disabled={checkingFirmware}
                          class="btn flex items-center gap-2"
                        >
                          {checkingFirmware ? (
                            <>
                              <Loader2 class="w-4 h-4 animate-spin" />
                              Checking...
                            </>
                          ) : (
                            <>
                              <RefreshCw class="w-4 h-4" />
                              Check for Updates
                            </>
                          )}
                        </button>
                      )}
                    </div>
                  </div>

                  {/* Firmware Update Available */}
                  {firmwareCheck?.update_available && !deviceUpdating && (
                    <div class="p-4 bg-[var(--accent-color)]/10 border border-[var(--accent-color)]/30 rounded-lg mb-4">
                      <div class="flex items-start gap-3">
                        <div class="w-10 h-10 rounded-full bg-[var(--accent-color)]/20 flex items-center justify-center flex-shrink-0">
                          <Download class="w-5 h-5 text-[var(--accent-color)]" />
                        </div>
                        <div class="flex-1">
                          <p class="text-sm font-medium text-[var(--text-primary)]">
                            Update Available: v{firmwareCheck.latest_version}
                          </p>
                          <p class="text-sm text-[var(--text-secondary)] mt-1">
                            {deviceFirmwareVersion || firmwareCheck?.current_version
                              ? `Your device is running v${deviceFirmwareVersion || firmwareCheck.current_version}.`
                              : 'Your device is running an older version without version reporting.'}
                            {' '}Click update to install the new firmware.
                          </p>
                          <div class="mt-3 p-3 bg-[var(--bg-secondary)] rounded text-xs text-[var(--text-muted)]">
                            <p class="font-medium mb-1">What will happen:</p>
                            <ol class="list-decimal list-inside space-y-1">
                              <li>Device will reboot</li>
                              <li>Download new firmware (~4.5MB)</li>
                              <li>Install and reboot again</li>
                            </ol>
                            <p class="mt-2">This takes about 1-2 minutes. Do not power off the device.</p>
                          </div>
                          <button
                            onClick={handleTriggerOTA}
                            disabled={!deviceConnected}
                            class="btn btn-primary flex items-center gap-2 mt-3"
                          >
                            <Download class="w-4 h-4" />
                            Update to v{firmwareCheck.latest_version}
                          </button>
                        </div>
                      </div>
                    </div>
                  )}

                  {/* Firmware Up to Date */}
                  {firmwareCheck && !firmwareCheck.update_available && (deviceFirmwareVersion || firmwareCheck.current_version) && !deviceUpdating && (
                    <div class="p-4 bg-green-500/10 border border-green-500/30 rounded-lg mb-4">
                      <div class="flex items-center gap-3">
                        <CheckCircle class="w-5 h-5 text-green-500" />
                        <p class="text-sm text-green-600">
                          Your device is up to date (v{deviceFirmwareVersion || firmwareCheck.current_version})
                        </p>
                      </div>
                    </div>
                  )}

                  {/* Upload Firmware (for developers) */}
                  <details class="group">
                    <summary class="cursor-pointer text-xs text-[var(--text-muted)] hover:text-[var(--text-secondary)]">
                      Developer: Upload custom firmware
                    </summary>
                    <div class="mt-2 p-4 bg-[var(--card-bg)] border border-[var(--border-color)] rounded-lg">
                      <div class="flex items-center justify-between">
                        <div>
                          <p class="text-sm font-medium text-[var(--text-primary)]">Upload Firmware Binary</p>
                          <p class="text-xs text-[var(--text-muted)]">
                            Upload a .bin file to make it available for OTA
                          </p>
                        </div>
                        <label class="btn flex items-center gap-2 cursor-pointer">
                          {uploadingFirmware ? (
                            <Loader2 class="w-4 h-4 animate-spin" />
                          ) : (
                            <Upload class="w-4 h-4" />
                          )}
                          {uploadingFirmware ? 'Uploading...' : 'Choose File'}
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
            </div>
          </div>
        )}

      </div>

      {/* Calibration Modal */}
      {calibrationStep !== 'idle' && (
        <div class="fixed inset-0 z-50 flex items-center justify-center bg-black/50">
          <div class="bg-[var(--bg-primary)] rounded-xl shadow-xl w-full max-w-md mx-4 overflow-hidden border border-[var(--border-color)]">
            {/* Header */}
            <div class="flex items-center justify-between px-6 py-4 border-b border-[var(--border-color)]">
              <div class="flex items-center gap-3">
                <Scale class="w-5 h-5 text-[var(--accent)]" />
                <h2 class="text-lg font-medium text-[var(--text-primary)]">
                  {calibrationStep === 'complete' ? 'Calibration Complete' : `Scale Calibration (${calibrationStep === 'empty' ? '1' : '2'}/2)`}
                </h2>
              </div>
              <button onClick={cancelCalibration} class="text-[var(--text-muted)] hover:text-[var(--text-primary)]">
                <X class="w-5 h-5" />
              </button>
            </div>

            {/* Content */}
            <div class="p-6">
              {calibrationStep === 'empty' && (
                <div class="space-y-6">
                  <div class="flex items-start gap-4">
                    <div class="w-12 h-12 rounded-full bg-[var(--accent)]/10 flex items-center justify-center flex-shrink-0">
                      <Scale class="w-6 h-6 text-[var(--accent)]" />
                    </div>
                    <div>
                      <h3 class="text-sm font-medium text-[var(--text-primary)]">Remove everything from the scale</h3>
                      <p class="mt-1 text-sm text-[var(--text-secondary)]">
                        Make sure the scale is empty and stable before continuing.
                      </p>
                    </div>
                  </div>

                  <div class="p-4 bg-[var(--bg-secondary)] rounded-lg">
                    <p class="text-xs text-[var(--text-muted)] mb-1">Current reading</p>
                    <p class="text-3xl font-mono text-[var(--text-primary)]">
                      {currentWeight !== null ? `${Math.round(currentWeight)}g` : "--"}
                    </p>
                  </div>
                </div>
              )}

              {calibrationStep === 'weight' && (
                <div class="space-y-6">
                  <div class="flex items-start gap-4">
                    <div class="w-12 h-12 rounded-full bg-[var(--accent)]/10 flex items-center justify-center flex-shrink-0">
                      <Scale class="w-6 h-6 text-[var(--accent)]" />
                    </div>
                    <div>
                      <h3 class="text-sm font-medium text-[var(--text-primary)]">Place calibration weight on scale</h3>
                      <p class="mt-1 text-sm text-[var(--text-secondary)]">
                        Place a known weight on the scale and enter its exact value below.
                      </p>
                    </div>
                  </div>

                  <div class="space-y-4">
                    <div>
                      <label class="block text-xs text-[var(--text-muted)] mb-1">Known weight (grams)</label>
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
                        class="w-full px-3 py-2 bg-[var(--bg-secondary)] border border-[var(--border-color)] rounded-lg text-[var(--text-primary)] text-lg font-mono focus:border-[var(--accent)] focus:outline-none"
                      />
                    </div>

                    <div class="p-4 bg-[var(--bg-secondary)] rounded-lg">
                      <div class="flex justify-between mb-2">
                        <span class="text-xs text-[var(--text-muted)]">Current reading</span>
                        <span class="text-xs text-[var(--text-muted)]">Target</span>
                      </div>
                      <div class="flex justify-between items-baseline">
                        <span class="text-2xl font-mono text-[var(--text-primary)]">
                          {currentWeight !== null ? `${Math.round(currentWeight)}g` : "--"}
                        </span>
                        <span class="text-2xl font-mono text-[var(--accent)]">
                          {calibrationWeight}g
                        </span>
                      </div>
                      {currentWeight !== null && (
                        <p class={`text-xs mt-2 ${Math.abs(currentWeight - calibrationWeight) < 10 ? 'text-green-500' : 'text-yellow-500'}`}>
                          Difference: {Math.round(currentWeight - calibrationWeight)}g
                        </p>
                      )}
                      <div class={`mt-3 flex items-center gap-2 text-xs ${weightStable ? 'text-green-500' : 'text-yellow-500'}`}>
                        <div class={`w-2 h-2 rounded-full ${weightStable ? 'bg-green-500' : 'bg-yellow-500 animate-pulse'}`} />
                        {weightStable ? 'Stabilized' : 'Waiting for weight to stabilize...'}
                      </div>
                    </div>
                  </div>
                </div>
              )}

              {calibrationStep === 'complete' && (
                <div class="text-center space-y-4">
                  <div class="w-16 h-16 rounded-full bg-green-500/10 flex items-center justify-center mx-auto">
                    <CheckCircle class="w-8 h-8 text-green-500" />
                  </div>
                  <div>
                    <h3 class="text-lg font-medium text-[var(--text-primary)]">Calibration complete!</h3>
                    <p class="mt-1 text-sm text-[var(--text-secondary)]">
                      Your scale is now calibrated and ready to use.
                    </p>
                  </div>
                </div>
              )}
            </div>

            {/* Footer */}
            <div class="flex justify-end gap-3 px-6 py-4 border-t border-[var(--border-color)]">
              {calibrationStep !== 'complete' && (
                <button onClick={cancelCalibration} class="btn">
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
                    Tare & Next
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
