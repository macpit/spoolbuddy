import { ComponentChildren } from "preact";
import { useState, useEffect } from "preact/hooks";
import { Link, useLocation } from "wouter-preact";
import { useWebSocket } from "../lib/websocket";
import { useTheme } from "../lib/theme";
import { Sun, Moon, Github, Bug } from "lucide-preact";
import { api, DebugLoggingState } from "../lib/api";

interface LayoutProps {
  children: ComponentChildren;
}

const navItems = [
  { path: "/", label: "Dashboard", icon: "M3 12l2-2m0 0l7-7 7 7M5 10v10a1 1 0 001 1h3m10-11l2 2m-2-2v10a1 1 0 01-1 1h-3m-6 0a1 1 0 001-1v-4a1 1 0 011-1h2a1 1 0 011 1v4a1 1 0 001 1m-6 0h6" },
  { path: "/inventory", label: "Inventory", icon: "M19 11H5m14 0a2 2 0 012 2v6a2 2 0 01-2 2H5a2 2 0 01-2-2v-6a2 2 0 012-2m14 0V9a2 2 0 00-2-2M5 11V9a2 2 0 012-2m0 0V5a2 2 0 012-2h6a2 2 0 012 2v2M7 7h10" },
  { path: "/printers", label: "Printers", icon: "M17 17h2a2 2 0 002-2v-4a2 2 0 00-2-2H5a2 2 0 00-2 2v4a2 2 0 002 2h2m2 4h6a2 2 0 002-2v-4a2 2 0 00-2-2H9a2 2 0 00-2 2v4a2 2 0 002 2zm8-12V5a2 2 0 00-2-2H9a2 2 0 00-2 2v4h10z" },
  { path: "/settings", label: "Settings", icon: "M10.325 4.317c.426-1.756 2.924-1.756 3.35 0a1.724 1.724 0 002.573 1.066c1.543-.94 3.31.826 2.37 2.37a1.724 1.724 0 001.065 2.572c1.756.426 1.756 2.924 0 3.35a1.724 1.724 0 00-1.066 2.573c.94 1.543-.826 3.31-2.37 2.37a1.724 1.724 0 00-2.572 1.065c-.426 1.756-2.924 1.756-3.35 0a1.724 1.724 0 00-2.573-1.066c-1.543.94-3.31-.826-2.37-2.37a1.724 1.724 0 00-1.065-2.572c-1.756-.426-1.756-2.924 0-3.35a1.724 1.724 0 001.066-2.573c-.94-1.543.826-3.31 2.37-2.37.996.608 2.296.07 2.572-1.065z M15 12a3 3 0 11-6 0 3 3 0 016 0z" },
];

export function Layout({ children }: LayoutProps) {
  const [location] = useLocation();
  const { deviceConnected } = useWebSocket();
  const { theme, toggleTheme } = useTheme();
  const [debugLogging, setDebugLogging] = useState<DebugLoggingState | null>(null);

  // Fetch debug logging state and poll for updates
  useEffect(() => {
    const fetchDebugState = async () => {
      try {
        const state = await api.getDebugLogging();
        setDebugLogging(state);
      } catch (e) {
        // Silently fail - debug banner just won't show
      }
    };

    fetchDebugState();
    const interval = setInterval(fetchDebugState, 10000); // Poll every 10 seconds
    return () => clearInterval(interval);
  }, []);

  return (
    <div class="min-h-screen flex flex-col bg-[var(--bg-secondary)]">
      {/* Debug Logging Banner - shown on all pages when enabled */}
      {debugLogging?.enabled && (
        <div class="bg-amber-500/90 text-black px-4 py-2">
          <div class="max-w-7xl mx-auto flex items-center justify-between">
            <div class="flex items-center gap-3">
              <Bug class="w-5 h-5" />
              <span class="font-medium">Debug Logging Active</span>
              {debugLogging.duration_seconds !== null && (
                <span class="text-amber-900">
                  ({Math.floor(debugLogging.duration_seconds / 60)}m {debugLogging.duration_seconds % 60}s)
                </span>
              )}
            </div>
            <a
              href="/settings#debug"
              class="text-sm font-medium hover:underline"
              onClick={(e) => {
                // If already on settings page, need to force hash change
                if (location === '/settings' || location.startsWith('/settings#')) {
                  e.preventDefault();
                  window.location.hash = 'debug';
                  window.dispatchEvent(new HashChangeEvent('hashchange'));
                }
              }}
            >
              Manage →
            </a>
          </div>
        </div>
      )}

      {/* Header */}
      <header class="bg-[var(--bg-header)] text-[var(--header-text)] shadow-md border-b border-[var(--border-color)]">
        <div class="w-full px-4 sm:px-6 lg:px-8">
          <div class="flex items-center justify-between h-16">
            {/* Logo */}
            <div class="flex items-center">
              <Link href="/" class="flex items-center">
                <img
                  src={theme === "dark" ? "/spoolbuddy_logo_dark.png" : "/spoolbuddy_logo_light.png"}
                  alt="SpoolBuddy"
                  class="h-10"
                />
              </Link>
            </div>

            {/* Navigation */}
            <nav class="hidden md:flex space-x-4">
              {navItems.map((item) => (
                <Link
                  key={item.path}
                  href={item.path}
                  class={`px-3 py-2 rounded-md text-sm font-medium transition-colors ${
                    location === item.path
                      ? "bg-[var(--accent-color)] text-white"
                      : "text-[var(--header-text-muted)] hover:bg-[var(--bg-header-hover)] hover:text-[var(--header-text)]"
                  }`}
                >
                  {item.label}
                </Link>
              ))}
            </nav>

            {/* Status indicators */}
            <div class="flex items-center space-x-4">
              {/* Theme toggle */}
              <button
                onClick={toggleTheme}
                class="p-2 rounded-md hover:bg-[var(--bg-header-hover)] transition-colors"
                title={theme === "dark" ? "Switch to light mode" : "Switch to dark mode"}
              >
                {theme === "dark" ? (
                  <Sun class="w-5 h-5 text-yellow-300" />
                ) : (
                  <Moon class="w-5 h-5 text-[var(--header-text-muted)]" />
                )}
              </button>

              {/* GitHub link */}
              <a
                href="https://github.com/maziggy/spoolbuddy"
                target="_blank"
                rel="noopener noreferrer"
                class="p-2 rounded-md hover:bg-[var(--bg-header-hover)] transition-colors"
                title="View on GitHub"
              >
                <Github class="w-5 h-5 text-[var(--header-text-muted)]" />
              </a>

              {/* Device status */}
              <div class="flex items-center space-x-2">
                <div
                  class={`w-3 h-3 rounded-full ${
                    deviceConnected ? "bg-green-500" : "bg-red-500"
                  }`}
                  title={deviceConnected ? "Device connected" : "Device disconnected"}
                />
                <span class="text-sm text-[var(--header-text-muted)]">
                  {deviceConnected ? "Connected" : "Offline"}
                </span>
              </div>
            </div>
          </div>
        </div>
      </header>

      {/* Mobile navigation */}
      <nav class="md:hidden bg-[var(--bg-header)] border-t border-[var(--border-color)]">
        <div class="flex justify-around">
          {navItems.map((item) => (
            <Link
              key={item.path}
              href={item.path}
              class={`flex flex-col items-center py-2 px-3 text-xs ${
                location === item.path
                  ? "text-[var(--accent-color)]"
                  : "text-[var(--header-text-muted)]"
              }`}
            >
              <svg class="w-6 h-6" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d={item.icon} />
              </svg>
              <span class="mt-1">{item.label}</span>
            </Link>
          ))}
        </div>
      </nav>

      {/* Main content */}
      <main class="flex-1">
        <div class={`py-6 px-4 sm:px-6 lg:px-8 mx-auto ${
          location === "/inventory" ? "w-full" : "max-w-7xl w-full"
        }`}>
          {children}
        </div>
      </main>
    </div>
  );
}
