import { useState, useEffect, useCallback, useRef, useMemo } from 'preact/hooks'
import { X, Droplets, Thermometer, TrendingUp, TrendingDown, Minus, RefreshCw } from 'lucide-preact'
import { Chart, LineController, LineElement, PointElement, LinearScale, TimeScale, CategoryScale, Tooltip, Legend, Filler } from 'chart.js'
import { api, AMSHistoryResponse, AMSThresholds } from '../lib/api'

// Register Chart.js components
Chart.register(LineController, LineElement, PointElement, LinearScale, TimeScale, CategoryScale, Tooltip, Legend, Filler)

interface AMSHistoryModalProps {
  printerSerial: string
  amsId: number
  amsLabel: string
  mode: 'humidity' | 'temperature'
  thresholds?: AMSThresholds
  onClose: () => void
}

type TimeRange = '6h' | '24h' | '48h' | '7d'

const TIME_RANGES: { value: TimeRange; label: string; hours: number }[] = [
  { value: '6h', label: '6h', hours: 6 },
  { value: '24h', label: '24h', hours: 24 },
  { value: '48h', label: '48h', hours: 48 },
  { value: '7d', label: '7d', hours: 168 },
]

export function AMSHistoryModal({
  printerSerial,
  amsId,
  amsLabel,
  mode: initialMode,
  thresholds,
  onClose,
}: AMSHistoryModalProps) {
  const [timeRange, setTimeRange] = useState<TimeRange>('24h')
  const [mode, setMode] = useState<'humidity' | 'temperature'>(initialMode)
  const [data, setData] = useState<AMSHistoryResponse | null>(null)
  const [isLoading, setIsLoading] = useState(true)
  const [error, setError] = useState<string | null>(null)
  const chartRef = useRef<HTMLCanvasElement>(null)
  const chartInstanceRef = useRef<Chart | null>(null)

  // Detect dark mode
  const isDark = document.documentElement.classList.contains('dark') ||
    window.matchMedia('(prefers-color-scheme: dark)').matches

  // Theme-aware colors
  const modalBg = isDark ? '#2d2d2d' : '#ffffff'
  const cardBg = isDark ? '#1d1d1d' : '#f3f4f6'
  const borderColor = isDark ? '#3d3d3d' : '#e5e7eb'
  const textPrimary = isDark ? '#ffffff' : '#111827'
  const textSecondary = isDark ? '#9ca3af' : '#4b5563'

  // Close on Escape key
  const handleKeyDown = useCallback((e: KeyboardEvent) => {
    if (e.key === 'Escape') onClose()
  }, [onClose])

  useEffect(() => {
    window.addEventListener('keydown', handleKeyDown)
    document.body.style.overflow = 'hidden'
    return () => {
      window.removeEventListener('keydown', handleKeyDown)
      document.body.style.overflow = ''
    }
  }, [handleKeyDown])

  const hours = TIME_RANGES.find(r => r.value === timeRange)?.hours || 24

  // Fetch data
  const fetchData = useCallback(async () => {
    setIsLoading(true)
    setError(null)
    try {
      const result = await api.getAMSHistory(printerSerial, amsId, hours)
      setData(result)
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to load data')
    } finally {
      setIsLoading(false)
    }
  }, [printerSerial, amsId, hours])

  useEffect(() => {
    fetchData()
    const interval = setInterval(fetchData, 60000)
    return () => clearInterval(interval)
  }, [fetchData])

  // Get thresholds
  const humidityGood = thresholds?.humidity_good ?? 40
  const humidityFair = thresholds?.humidity_fair ?? 60
  const tempGood = thresholds?.temp_good ?? 30
  const tempFair = thresholds?.temp_fair ?? 35

  // Format data for chart (memoized to prevent recreation)
  const chartData = useMemo(() => {
    if (!data?.data) return []
    return data.data.map(point => ({
      time: new Date(point.recorded_at * 1000),
      humidity: point.humidity,
      temperature: point.temperature,
    }))
  }, [data])

  // Current values (last data point)
  const lastPoint = chartData[chartData.length - 1]
  const currentHumidity = lastPoint?.humidity
  const currentTemp = lastPoint?.temperature

  // Trend calculation
  const getTrend = (values: (number | null | undefined)[]) => {
    const filtered = values.filter((v): v is number => v != null)
    if (filtered.length < 4) return 'stable'
    const firstQuarter = filtered.slice(0, Math.floor(filtered.length / 4))
    const lastQuarter = filtered.slice(-Math.floor(filtered.length / 4))
    const firstAvg = firstQuarter.reduce((a, b) => a + b, 0) / firstQuarter.length
    const lastAvg = lastQuarter.reduce((a, b) => a + b, 0) / lastQuarter.length
    const diff = lastAvg - firstAvg
    if (Math.abs(diff) < 2) return 'stable'
    return diff > 0 ? 'up' : 'down'
  }

  const humidityTrend = getTrend(chartData.map(d => d.humidity))
  const tempTrend = getTrend(chartData.map(d => d.temperature))

  const TrendIcon = ({ trend }: { trend: string }) => {
    if (trend === 'up') return <TrendingUp class="w-4 h-4 text-red-400" />
    if (trend === 'down') return <TrendingDown class="w-4 h-4 text-green-400" />
    return <Minus class="w-4 h-4 text-gray-400" />
  }

  // Get status color for current value
  const getHumidityColor = (value: number | undefined | null) => {
    if (value == null) return '#9ca3af'
    if (value <= humidityGood) return '#22a352'
    if (value <= humidityFair) return '#d4a017'
    return '#c62828'
  }

  const getTempColor = (value: number | undefined | null) => {
    if (value == null) return '#9ca3af'
    if (value <= tempGood) return '#22a352'
    if (value <= tempFair) return '#d4a017'
    return '#c62828'
  }

  // Create/update Chart.js chart
  useEffect(() => {
    if (!chartRef.current || chartData.length === 0) return

    const labels = chartData.map(d => {
      if (hours > 24) {
        return d.time.toLocaleDateString([], { day: 'numeric', month: 'short' })
      }
      return d.time.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })
    })

    const values = chartData.map(d => mode === 'humidity' ? d.humidity : d.temperature)
    const lineColor = mode === 'humidity' ? '#3b82f6' : '#f97316'

    // Update existing chart if it exists
    if (chartInstanceRef.current) {
      chartInstanceRef.current.data.labels = labels
      chartInstanceRef.current.data.datasets[0].data = values
      chartInstanceRef.current.data.datasets[0].label = mode === 'humidity' ? 'Humidity' : 'Temperature'
      chartInstanceRef.current.data.datasets[0].borderColor = lineColor
      chartInstanceRef.current.data.datasets[0].backgroundColor = lineColor + '20'
      chartInstanceRef.current.options.scales!.y!.min = mode === 'humidity' ? 0 : undefined
      chartInstanceRef.current.options.scales!.y!.max = mode === 'humidity' ? 100 : undefined
      chartInstanceRef.current.update() // Animate the update
      return
    }

    const ctx = chartRef.current.getContext('2d')
    if (!ctx) return

    chartInstanceRef.current = new Chart(ctx, {
      type: 'line',
      data: {
        labels,
        datasets: [{
          label: mode === 'humidity' ? 'Humidity' : 'Temperature',
          data: values,
          borderColor: lineColor,
          backgroundColor: lineColor + '20',
          borderWidth: 2,
          fill: true,
          tension: 0.3,
          pointRadius: 0,
          pointHoverRadius: 4,
        }]
      },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        animation: {
          duration: 750,
          easing: 'easeOutQuart',
        },
        interaction: {
          intersect: false,
          mode: 'index',
        },
        plugins: {
          legend: {
            display: true,
            labels: {
              color: textSecondary,
            }
          },
          tooltip: {
            backgroundColor: isDark ? '#2d2d2d' : '#ffffff',
            titleColor: textPrimary,
            bodyColor: textPrimary,
            borderColor: borderColor,
            borderWidth: 1,
            callbacks: {
              label: (context) => {
                const value = context.parsed.y
                return mode === 'humidity' ? `Humidity: ${value}%` : `Temperature: ${value}°C`
              }
            }
          },
        },
        scales: {
          x: {
            grid: {
              color: isDark ? '#3d3d3d' : '#e5e7eb',
            },
            ticks: {
              color: textSecondary,
              maxTicksLimit: 8,
            }
          },
          y: {
            min: mode === 'humidity' ? 0 : undefined,
            max: mode === 'humidity' ? 100 : undefined,
            grid: {
              color: isDark ? '#3d3d3d' : '#e5e7eb',
            },
            ticks: {
              color: textSecondary,
              callback: (value) => mode === 'humidity' ? `${value}%` : `${value}°C`
            }
          }
        }
      }
    })
  // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [data, mode, hours])

  // Cleanup on unmount
  useEffect(() => {
    return () => {
      if (chartInstanceRef.current) {
        chartInstanceRef.current.destroy()
        chartInstanceRef.current = null
      }
    }
  }, [])

  return (
    <div class="fixed inset-0 bg-black/50 flex items-center justify-center z-50" onClick={onClose}>
      <div
        class="rounded-xl w-full max-w-4xl max-h-[90vh] overflow-hidden shadow-xl mx-4"
        style={{ backgroundColor: modalBg }}
        onClick={e => e.stopPropagation()}
      >
        {/* Header */}
        <div
          class="flex items-center justify-between px-6 py-4 border-b"
          style={{ borderColor }}
        >
          <div>
            <h2 class="text-lg font-semibold" style={{ color: textPrimary }}>
              {amsLabel} History
            </h2>
          </div>
          <div class="flex items-center gap-2">
            <button
              onClick={(e) => { e.stopPropagation(); fetchData(); }}
              class="p-2 rounded-lg transition-colors hover:bg-black/10"
              style={{ color: textSecondary }}
              title="Refresh"
            >
              <RefreshCw class={`w-5 h-5 ${isLoading ? 'animate-spin' : ''}`} />
            </button>
            <button
              onClick={onClose}
              class="p-2 rounded-lg transition-colors hover:bg-black/10"
              style={{ color: textSecondary }}
            >
              <X class="w-5 h-5" />
            </button>
          </div>
        </div>

        {/* Content */}
        <div class="p-6 space-y-6 overflow-y-auto max-h-[calc(90vh-80px)]">
          {/* Time Range & Mode Selector */}
          <div class="flex items-center justify-between flex-wrap gap-3">
            <div class="flex gap-1 rounded-lg p-1" style={{ backgroundColor: cardBg }}>
              <button
                onClick={() => setMode('humidity')}
                class={`flex items-center gap-2 px-3 py-1.5 text-sm rounded-md transition-colors ${
                  mode === 'humidity' ? 'bg-blue-600 text-white' : ''
                }`}
                style={mode !== 'humidity' ? { color: textSecondary } : undefined}
              >
                <Droplets class="w-4 h-4" />
                Humidity
              </button>
              <button
                onClick={() => setMode('temperature')}
                class={`flex items-center gap-2 px-3 py-1.5 text-sm rounded-md transition-colors ${
                  mode === 'temperature' ? 'bg-orange-600 text-white' : ''
                }`}
                style={mode !== 'temperature' ? { color: textSecondary } : undefined}
              >
                <Thermometer class="w-4 h-4" />
                Temperature
              </button>
            </div>

            <div class="flex gap-1 rounded-lg p-1" style={{ backgroundColor: cardBg }}>
              {TIME_RANGES.map(range => (
                <button
                  key={range.value}
                  onClick={() => setTimeRange(range.value)}
                  class={`px-3 py-1 text-sm rounded-md transition-colors ${
                    timeRange === range.value ? 'bg-green-600 text-white' : ''
                  }`}
                  style={timeRange !== range.value ? { color: textSecondary } : undefined}
                >
                  {range.label}
                </button>
              ))}
            </div>
          </div>

          {/* Stats Cards */}
          <div class="grid grid-cols-4 gap-4">
            {mode === 'humidity' ? (
              <>
                <div class="rounded-lg p-4" style={{ backgroundColor: cardBg }}>
                  <p class="text-xs" style={{ color: textSecondary }}>Current</p>
                  <div class="flex items-center gap-2">
                    <p class="text-2xl font-bold" style={{ color: getHumidityColor(currentHumidity) }}>
                      {currentHumidity != null ? `${currentHumidity}%` : '—'}
                    </p>
                    <TrendIcon trend={humidityTrend} />
                  </div>
                </div>
                <div class="rounded-lg p-4" style={{ backgroundColor: cardBg }}>
                  <p class="text-xs" style={{ color: textSecondary }}>Average</p>
                  <p class="text-2xl font-bold" style={{ color: textPrimary }}>
                    {data?.avg_humidity != null ? `${data.avg_humidity.toFixed(1)}%` : '—'}
                  </p>
                </div>
                <div class="rounded-lg p-4" style={{ backgroundColor: cardBg }}>
                  <p class="text-xs" style={{ color: textSecondary }}>Min</p>
                  <p class="text-2xl font-bold text-green-500">
                    {data?.min_humidity != null ? `${data.min_humidity}%` : '—'}
                  </p>
                </div>
                <div class="rounded-lg p-4" style={{ backgroundColor: cardBg }}>
                  <p class="text-xs" style={{ color: textSecondary }}>Max</p>
                  <p class="text-2xl font-bold text-red-500">
                    {data?.max_humidity != null ? `${data.max_humidity}%` : '—'}
                  </p>
                </div>
              </>
            ) : (
              <>
                <div class="rounded-lg p-4" style={{ backgroundColor: cardBg }}>
                  <p class="text-xs" style={{ color: textSecondary }}>Current</p>
                  <div class="flex items-center gap-2">
                    <p class="text-2xl font-bold" style={{ color: getTempColor(currentTemp) }}>
                      {currentTemp != null ? `${currentTemp}°C` : '—'}
                    </p>
                    <TrendIcon trend={tempTrend} />
                  </div>
                </div>
                <div class="rounded-lg p-4" style={{ backgroundColor: cardBg }}>
                  <p class="text-xs" style={{ color: textSecondary }}>Average</p>
                  <p class="text-2xl font-bold" style={{ color: textPrimary }}>
                    {data?.avg_temperature != null ? `${data.avg_temperature.toFixed(1)}°C` : '—'}
                  </p>
                </div>
                <div class="rounded-lg p-4" style={{ backgroundColor: cardBg }}>
                  <p class="text-xs" style={{ color: textSecondary }}>Min</p>
                  <p class="text-2xl font-bold text-blue-500">
                    {data?.min_temperature != null ? `${data.min_temperature}°C` : '—'}
                  </p>
                </div>
                <div class="rounded-lg p-4" style={{ backgroundColor: cardBg }}>
                  <p class="text-xs" style={{ color: textSecondary }}>Max</p>
                  <p class="text-2xl font-bold text-red-500">
                    {data?.max_temperature != null ? `${data.max_temperature}°C` : '—'}
                  </p>
                </div>
              </>
            )}
          </div>

          {/* Chart */}
          <div class="rounded-lg p-4 relative" style={{ backgroundColor: cardBg }}>
            {/* Loading overlay - show on top of chart when we have data */}
            {isLoading && chartData.length > 0 && (
              <div class="absolute inset-0 flex items-center justify-center bg-black/20 rounded-lg z-10">
                <RefreshCw class="w-6 h-6 animate-spin" style={{ color: textSecondary }} />
              </div>
            )}
            {/* Initial loading state - only when no data yet */}
            {isLoading && chartData.length === 0 ? (
              <div class="h-[300px] flex items-center justify-center" style={{ color: textSecondary }}>
                Loading...
              </div>
            ) : error ? (
              <div class="h-[300px] flex items-center justify-center text-red-500">
                {error}
              </div>
            ) : chartData.length === 0 ? (
              <div class="h-[300px] flex items-center justify-center" style={{ color: textSecondary }}>
                No data available for this time range
              </div>
            ) : (
              <div class="h-[300px]">
                <canvas ref={chartRef} />
              </div>
            )}
          </div>

          {/* Legend for thresholds */}
          {!isLoading && !error && chartData.length > 0 && (
            <div class="flex items-center justify-center gap-6 text-xs" style={{ color: textSecondary }}>
              <div class="flex items-center gap-1.5">
                <div class="w-3 h-3 rounded-full bg-[#22a352]" />
                <span>Good (≤ {mode === 'humidity' ? `${humidityGood}%` : `${tempGood}°C`})</span>
              </div>
              <div class="flex items-center gap-1.5">
                <div class="w-3 h-3 rounded-full bg-[#d4a017]" />
                <span>Fair (≤ {mode === 'humidity' ? `${humidityFair}%` : `${tempFair}°C`})</span>
              </div>
              <div class="flex items-center gap-1.5">
                <div class="w-3 h-3 rounded-full bg-[#c62828]" />
                <span>High (&gt; {mode === 'humidity' ? `${humidityFair}%` : `${tempFair}°C`})</span>
              </div>
            </div>
          )}

          {/* Info */}
          <div class="text-xs text-center" style={{ color: textSecondary }}>
            Data is recorded every 5 minutes while the printer is connected
          </div>
        </div>
      </div>
    </div>
  )
}
