import { useState, useEffect, useCallback } from "preact/hooks";
import { useWebSocket } from "../lib/websocket";
import { api, CloudAuthStatus, VersionInfo, UpdateCheck, UpdateStatus, FirmwareCheck } from "../lib/api";
import { Cloud, CloudOff, LogOut, Loader2, Mail, Lock, Key, Download, RefreshCw, CheckCircle, AlertCircle, GitBranch, ExternalLink, Wifi, WifiOff, Cpu, Usb, RotateCcw, Upload, HardDrive } from "lucide-preact";
import { useToast } from "../lib/toast";
import { SerialTerminal } from "../components/SerialTerminal";
import { SpoolCatalogSettings } from "../components/SpoolCatalogSettings";

export function Settings() {
  const { deviceConnected, currentWeight } = useWebSocket();
  const { showToast } = useToast();

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

  // Trigger device OTA update (reboot device to apply)
  const handleTriggerOTA = useCallback(async () => {
    try {
      setDeviceUpdating(true);
      showToast('info', 'Sending reboot command to device...');
      await api.rebootESP32();
      showToast('success', 'Device is rebooting to install update. This may take a minute.');
      // Wait for device to disconnect and reconnect
      setTimeout(() => {
        setDeviceUpdating(false);
      }, 60000); // Reset after 60s
    } catch (e) {
      setDeviceUpdating(false);
      showToast('error', 'Failed to send reboot command');
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
    } catch (e) {
      console.error("Failed to tare:", e);
      alert("Failed to tare scale");
    }
  };

  return (
    <div class="space-y-6">
      {/* Header */}
      <div>
        <h1 class="text-3xl font-bold text-[var(--text-primary)]">Settings</h1>
        <p class="text-[var(--text-secondary)]">Configure SpoolBuddy</p>
      </div>

      {/* Two-column layout */}
      <div class="grid grid-cols-1 lg:grid-cols-2 gap-6">
        {/* Left Column */}
        <div class="space-y-6">

      {/* Bambu Cloud settings */}
      <div class="card">
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
            /* Logged in state */
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
            /* Not logged in - show login button */
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
            /* Login form */
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
            /* Verification code step */
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

      {/* ESP32 Device Connection */}
      <div class="card">
        <div class="px-6 py-4 border-b border-[var(--border-color)]">
          <div class="flex items-center gap-2">
            <Cpu class="w-5 h-5 text-[var(--text-muted)]" />
            <h2 class="text-lg font-medium text-[var(--text-primary)]">ESP32 Device</h2>
          </div>
        </div>
        <div class="p-6 space-y-6">
          {/* Connection Status - uses WebSocket deviceConnected */}
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
                  {currentWeight !== null ? `${currentWeight.toFixed(1)}g` : "--"}
                </p>
              </div>
              <div class="space-x-3">
                <button onClick={handleTare} disabled={!deviceConnected} class="btn">
                  Tare Scale
                </button>
                <button disabled={!deviceConnected} class="btn">
                  Calibrate
                </button>
              </div>
            </div>
          </div>

          {deviceConnected ? (
            /* Connected - show device actions */
            <div class="border-t border-[var(--border-color)] pt-6 flex gap-3">
              <button onClick={handleESP32Reboot} class="btn flex items-center gap-2">
                <RotateCcw class="w-4 h-4" />
                Reboot Device
              </button>
            </div>
          ) : null}

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
      <div class="card">
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

            {/* Step 1: Device Status */}
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

            {/* Step 2: Update Available (only shown after check finds update) */}
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

            {/* Step 2 alt: Up to date (only shown after check confirms up to date) */}
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

      {/* About */}
      <div class="card">
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
        {/* End Left Column */}

        {/* Right Column */}
        <div class="space-y-6">
          {/* Spool Catalog */}
          <SpoolCatalogSettings />
        </div>
        {/* End Right Column */}
      </div>
      {/* End Two-column layout */}
    </div>
  );
}
