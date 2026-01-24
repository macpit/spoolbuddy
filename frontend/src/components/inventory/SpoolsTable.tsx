import { useMemo, useState, useEffect } from 'preact/hooks'
import {
  createColumnHelper,
  flexRender,
  getCoreRowModel,
  getSortedRowModel,
  getFilteredRowModel,
  getPaginationRowModel,
  useReactTable,
  SortingState,
  ColumnFiltersState,
  ColumnDef,
} from '@tanstack/react-table'
import { Spool, SpoolsInPrinters } from '../../lib/api'
import { SpoolCard } from './SpoolCard'
import { WeightProgress } from './ProgressBar'
import { PrinterBadge, KBadge, OriginBadge } from './Badge'
import { EmptyState } from './EmptyState'
import { getNetWeight, getGrossWeight, compareWeights, formatWeight, getFilamentName } from './utils'
import { ChevronUp, ChevronDown, Search, Check, AlertTriangle, Columns, RefreshCw, X, Archive, Clock, Package } from 'lucide-preact'
import { ColumnConfig } from './ColumnConfigModal'

const columnHelper = createColumnHelper<Spool>()
const PAGE_SIZE_KEY = 'spoolbuddy-page-size'
const SORTING_KEY = 'spoolbuddy-sorting'
const USAGE_FILTER_KEY = 'spoolbuddy-usage-filter'
const ARCHIVE_FILTER_KEY = 'spoolbuddy-archive-filter'
const COLUMN_FILTERS_KEY = 'spoolbuddy-column-filters'

type UsageFilter = 'all' | 'used' | 'unused'
type ArchiveFilter = 'active' | 'archived' | 'all'

function getStoredUsageFilter(): UsageFilter {
  try {
    const stored = localStorage.getItem(USAGE_FILTER_KEY)
    if (stored && ['all', 'used', 'unused'].includes(stored)) {
      return stored as UsageFilter
    }
  } catch {
    // Ignore errors
  }
  return 'all'
}

function getStoredArchiveFilter(): ArchiveFilter {
  try {
    const stored = localStorage.getItem(ARCHIVE_FILTER_KEY)
    if (stored && ['active', 'archived', 'all'].includes(stored)) {
      return stored as ArchiveFilter
    }
  } catch {
    // Ignore errors
  }
  return 'active'  // Default to showing only active (non-archived) spools
}

function getStoredColumnFilters(): ColumnFiltersState {
  try {
    const stored = localStorage.getItem(COLUMN_FILTERS_KEY)
    if (stored) {
      return JSON.parse(stored)
    }
  } catch {
    // Ignore errors
  }
  return []
}

function getStoredPageSize(): number {
  try {
    const stored = localStorage.getItem(PAGE_SIZE_KEY)
    if (stored) {
      const size = parseInt(stored, 10)
      if ([15, 30, 50, 100].includes(size)) return size
    }
  } catch {
    // Ignore errors
  }
  return 15
}

function getStoredSorting(): SortingState {
  try {
    const stored = localStorage.getItem(SORTING_KEY)
    if (stored) {
      return JSON.parse(stored)
    }
  } catch {
    // Ignore errors
  }
  return [{ id: 'id', desc: false }]
}

interface SpoolsTableProps {
  spools: Spool[]
  spoolsInPrinters?: SpoolsInPrinters
  onEditSpool?: (spool: Spool) => void
  onSyncWeight?: (spool: Spool) => void
  columnConfig?: ColumnConfig[]
  onOpenColumns?: () => void
  onAddSpool?: () => void
}

export function SpoolsTable({
  spools,
  spoolsInPrinters = {},
  onEditSpool,
  onSyncWeight,
  columnConfig,
  onOpenColumns,
  onAddSpool,
}: SpoolsTableProps) {
  const [sorting, setSorting] = useState<SortingState>(getStoredSorting)
  const [globalFilter, setGlobalFilter] = useState('')
  const [viewMode, setViewMode] = useState<'table' | 'cards'>('table')
  const [usageFilter, setUsageFilter] = useState<UsageFilter>(getStoredUsageFilter)
  const [archiveFilter, setArchiveFilter] = useState<ArchiveFilter>(getStoredArchiveFilter)
  const [columnFilters, setColumnFilters] = useState<ColumnFiltersState>(getStoredColumnFilters)
  const [pagination, setPagination] = useState({
    pageIndex: 0,
    pageSize: getStoredPageSize(),
  })

  // Save page size to localStorage when it changes
  useEffect(() => {
    localStorage.setItem(PAGE_SIZE_KEY, String(pagination.pageSize))
  }, [pagination.pageSize])

  // Save sorting to localStorage when it changes
  useEffect(() => {
    localStorage.setItem(SORTING_KEY, JSON.stringify(sorting))
  }, [sorting])

  // Save usage filter to localStorage when it changes
  useEffect(() => {
    localStorage.setItem(USAGE_FILTER_KEY, usageFilter)
  }, [usageFilter])

  // Save archive filter to localStorage when it changes
  useEffect(() => {
    localStorage.setItem(ARCHIVE_FILTER_KEY, archiveFilter)
  }, [archiveFilter])

  // Save column filters to localStorage when they change
  useEffect(() => {
    localStorage.setItem(COLUMN_FILTERS_KEY, JSON.stringify(columnFilters))
  }, [columnFilters])

  // All available column definitions
  const allColumnDefs = useMemo(
    () => [
      // ID (spool number)
      columnHelper.accessor('spool_number', {
        id: 'id',
        header: '#',
        cell: (info) => <span class="font-medium">{info.getValue() ?? '-'}</span>,
        size: 50,
      }),
      // Added (use created_at as fallback)
      columnHelper.accessor((row) => row.added_time || row.created_at, {
        id: 'added_time',
        header: 'Added',
        cell: (info) => {
          const value = info.getValue()
          if (!value) return <span class="text-[var(--text-muted)]">-</span>
          const timestamp = typeof value === 'string' ? parseInt(value) : value
          const date = new Date(timestamp * 1000)
          return date.toLocaleDateString('en-GB', { day: '2-digit', month: '2-digit', year: '2-digit' })
        },
        size: 90,
      }),
      // Encoded
      columnHelper.accessor('encode_time', {
        id: 'encode_time',
        header: 'Encoded',
        cell: (info) => {
          const value = info.getValue()
          if (!value) return <span class="text-[var(--text-muted)]">-</span>
          const date = new Date(parseInt(value) * 1000)
          return date.toLocaleDateString('en-GB', { day: '2-digit', month: '2-digit', year: '2-digit' })
        },
        size: 90,
      }),
      // Last Used
      columnHelper.accessor('last_used_time', {
        id: 'last_used_time',
        header: 'Last Used',
        cell: (info) => {
          const value = info.getValue()
          if (!value) return <span class="text-[var(--text-muted)]">Never</span>
          const date = new Date(value * 1000)
          return date.toLocaleDateString('en-GB', { day: '2-digit', month: '2-digit', year: '2-digit' })
        },
        size: 90,
      }),
      // RGBA (color swatch)
      columnHelper.accessor('rgba', {
        header: 'Color',
        cell: (info) => {
          const rgba = info.getValue()
          const colorStyle = rgba ? (rgba.startsWith('#') ? rgba : `#${rgba}`) : '#ccc'
          return (
            <div
              class="color-swatch"
              style={{ backgroundColor: colorStyle }}
              title={info.row.original.color_name || rgba || undefined}
            />
          )
        },
        size: 60,
      }),
      // Material
      columnHelper.accessor('material', {
        header: 'Material',
        cell: (info) => info.getValue() || '-',
        size: 80,
      }),
      // Subtype
      columnHelper.accessor('subtype', {
        header: 'Subtype',
        cell: (info) => info.getValue() || <span class="text-[var(--text-muted)]">-</span>,
        size: 80,
      }),
      // Color (name)
      columnHelper.accessor('color_name', {
        id: 'color_name',
        header: 'Color Name',
        cell: (info) => info.getValue() || <span class="text-[var(--text-muted)]">-</span>,
        size: 120,
      }),
      // Brand
      columnHelper.accessor('brand', {
        header: 'Brand',
        cell: (info) => info.getValue() || '-',
        size: 100,
      }),
      // Slicer Filament
      columnHelper.accessor('slicer_filament', {
        id: 'slicer_filament',
        header: 'Slicer Filament',
        cell: (info) => {
          const code = info.getValue()
          const spool = info.row.original
          if (!code) return <span class="text-[var(--text-muted)]">-</span>
          // Prefer stored name, fallback to lookup, then code
          const name = spool.slicer_filament_name || getFilamentName(code)
          return <span title={code}>{name}</span>
        },
        size: 150,
      }),
      // Location (printer/AMS if loaded, otherwise storage location)
      columnHelper.accessor((row) => spoolsInPrinters[row.id] || row.location || '', {
        id: 'location',
        header: 'Location',
        cell: (info) => {
          const spool = info.row.original
          const printerLocation = spoolsInPrinters[spool.id]
          if (printerLocation) {
            return <PrinterBadge location={printerLocation} />
          }
          return spool.location ? <span>{spool.location}</span> : <span class="text-[var(--text-muted)]">-</span>
        },
        size: 120,
      }),
      // Label (label_weight)
      columnHelper.accessor('label_weight', {
        id: 'label_weight',
        header: 'Label',
        cell: (info) => formatWeight(info.getValue() || 0),
        size: 70,
      }),
      // Net (calculated remaining weight)
      columnHelper.accessor((row) => getNetWeight(row), {
        id: 'net',
        header: 'Net',
        cell: (info) => formatWeight(info.getValue()),
        size: 70,
      }),
      // Gross (net + core weight)
      columnHelper.accessor((row) => getGrossWeight(row), {
        id: 'gross',
        header: 'Gross',
        cell: (info) => formatWeight(info.getValue()),
        size: 70,
      }),
      // Full (was spool full when added)
      columnHelper.accessor('added_full', {
        id: 'added_full',
        header: 'Full',
        cell: (info) => {
          const value = info.getValue()
          if (value === undefined || value === null) return <span class="text-[var(--text-muted)]">-</span>
          return value ? 'Yes' : 'No'
        },
        size: 50,
      }),
      // Used (weight_used + consumed_since_weight)
      columnHelper.accessor((row) => (row.weight_used || 0) + (row.consumed_since_weight || 0), {
        id: 'used',
        header: 'Used',
        cell: (info) => {
          const value = info.getValue()
          if (!value) return <span class="text-[var(--text-muted)]">-</span>
          return formatWeight(value, false, true)
        },
        size: 70,
      }),
      // Printed Total (same as consumed_since_add)
      columnHelper.accessor('consumed_since_add', {
        id: 'printed_total',
        header: 'Printed Total',
        cell: (info) => formatWeight(info.getValue() || 0, false, true),
        size: 100,
      }),
      // Printed Since Weight
      columnHelper.accessor('consumed_since_weight', {
        id: 'printed_since_weight',
        header: 'Printed Since Weight',
        cell: (info) => formatWeight(info.getValue() || 0, false, true),
        size: 130,
      }),
      // Note
      columnHelper.accessor('note', {
        id: 'note',
        header: 'Note',
        cell: (info) => {
          const note = info.getValue()
          return note ? (
            <span class="truncate max-w-[150px] block" title={note}>{note}</span>
          ) : (
            <span class="text-[var(--text-muted)]">-</span>
          )
        },
        size: 150,
      }),
      // PA(K) - has pressure advance calibration
      columnHelper.accessor('ext_has_k', {
        id: 'pa_k',
        header: 'PA(K)',
        cell: (info) => info.getValue() ? <KBadge /> : <span class="text-[var(--text-muted)]">-</span>,
        size: 60,
      }),
      // Tag ID
      columnHelper.accessor('tag_id', {
        id: 'tag_id',
        header: 'Tag ID',
        cell: (info) => (
          <span class="font-mono text-xs">{info.getValue() || '-'}</span>
        ),
        size: 100,
      }),
      // Data Origin
      columnHelper.accessor('data_origin', {
        header: 'Data Origin',
        cell: (info) => {
          const origin = info.getValue()
          return origin ? <OriginBadge origin={origin} /> : <span class="text-[var(--text-muted)]">-</span>
        },
        size: 100,
      }),
      // Linked Tag Type
      columnHelper.accessor('tag_type', {
        id: 'tag_type',
        header: 'Linked Tag Type',
        cell: (info) => info.getValue() || <span class="text-[var(--text-muted)]">-</span>,
        size: 120,
      }),
      // Remaining (progress bar) - extra visual column
      columnHelper.accessor((row) => getNetWeight(row), {
        id: 'remaining',
        header: 'Remaining',
        cell: (info) => {
          const spool = info.row.original
          const netWeight = getNetWeight(spool)
          return (
            <WeightProgress
              remaining={netWeight}
              total={spool.label_weight || 0}
              size="md"
            />
          )
        },
        size: 150,
      }),
      // Scale weight with comparison
      columnHelper.accessor('weight_current', {
        id: 'scale',
        header: 'Scale',
        cell: (info) => {
          const spool = info.row.original
          const { scaleWeight, calculatedWeight, difference, isMatch } = compareWeights(spool)

          if (scaleWeight === null) {
            return <span class="text-[var(--text-muted)]" title="No scale measurement">-</span>
          }

          const diffStr = difference !== null ? (difference > 0 ? `+${Math.round(difference)}` : Math.round(difference)) : '?'
          const tooltip = isMatch
            ? `Scale: ${Math.round(scaleWeight)}g\nCalculated: ${Math.round(calculatedWeight)}g\nDifference: ${diffStr}g (within tolerance)`
            : `Scale: ${Math.round(scaleWeight)}g\nCalculated: ${Math.round(calculatedWeight)}g\nDifference: ${diffStr}g (mismatch!)`

          return (
            <div
              class={`flex items-center gap-1 ${isMatch ? 'weight-match' : 'weight-mismatch'}`}
              title={tooltip}
            >
              <span>{Math.round(scaleWeight)}g</span>
              {isMatch ? (
                <Check class="w-3 h-3" />
              ) : (
                <>
                  <AlertTriangle class="w-3 h-3" />
                  <button
                    type="button"
                    onClick={(e) => {
                      e.stopPropagation()
                      e.preventDefault()
                      onSyncWeight?.(spool)
                    }}
                    class="p-1 hover:bg-[var(--accent-color)]/20 rounded transition-colors text-[var(--accent-color)]"
                    title="Sync: trust scale weight and reset tracking"
                  >
                    <RefreshCw class="w-3.5 h-3.5" />
                  </button>
                </>
              )}
            </div>
          )
        },
        size: 100,
      }),
    ],
    [spoolsInPrinters, onSyncWeight]
  )

  // Apply column configuration (visibility and order)
  const columns = useMemo(() => {
    if (!columnConfig) return allColumnDefs

    // Create a map for quick lookup - use id or accessorKey
    const columnDefsMap = new Map(
      allColumnDefs.map(col => {
        const colId = col.id || (col as { accessorKey?: string }).accessorKey
        return [colId, col]
      })
    )

    // Filter and order based on config
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    const result: ColumnDef<Spool, any>[] = []
    for (const cfg of columnConfig) {
      if (cfg.visible) {
        const col = columnDefsMap.get(cfg.id)
        if (col) result.push(col)
      }
    }
    return result
  }, [allColumnDefs, columnConfig])

  // Filter spools by archive status and usage
  const filteredSpools = useMemo(() => {
    let result = spools

    // Filter by archive status first
    if (archiveFilter === 'active') {
      result = result.filter(s => s.archived_at === null)
    } else if (archiveFilter === 'archived') {
      result = result.filter(s => s.archived_at !== null)
    }

    // Then filter by usage
    if (usageFilter === 'used') {
      result = result.filter(s => s.last_used_time !== null)
    } else if (usageFilter === 'unused') {
      result = result.filter(s => s.last_used_time === null)
    }

    return result
  }, [spools, archiveFilter, usageFilter])

  // Extract unique values for column filters
  const uniqueMaterials = useMemo(() =>
    [...new Set(spools.map(s => s.material).filter(Boolean))].sort(),
    [spools]
  )
  const uniqueBrands = useMemo(() =>
    [...new Set(spools.map(s => s.brand).filter(Boolean))].sort() as string[],
    [spools]
  )
  const uniqueLocations = useMemo(() =>
    [...new Set(spools.map(s => s.location).filter(Boolean))].sort() as string[],
    [spools]
  )

  // Helper to get current filter value
  const getFilterValue = (columnId: string): string => {
    const filter = columnFilters.find(f => f.id === columnId)
    return (filter?.value as string) || ''
  }

  // Helper to set filter value
  const setFilterValue = (columnId: string, value: string) => {
    setColumnFilters(prev => {
      const others = prev.filter(f => f.id !== columnId)
      if (value) {
        return [...others, { id: columnId, value }]
      }
      return others
    })
  }

  const table = useReactTable({
    data: filteredSpools,
    columns,
    state: {
      sorting,
      globalFilter,
      columnFilters,
      pagination,
    },
    onSortingChange: setSorting,
    onGlobalFilterChange: setGlobalFilter,
    onColumnFiltersChange: setColumnFilters,
    onPaginationChange: setPagination,
    getCoreRowModel: getCoreRowModel(),
    getSortedRowModel: getSortedRowModel(),
    getFilteredRowModel: getFilteredRowModel(),
    getPaginationRowModel: getPaginationRowModel(),
  })

  // Check if any filters are active
  const hasActiveFilters = archiveFilter !== 'active' ||
    usageFilter !== 'all' ||
    getFilterValue('material') ||
    getFilterValue('brand') ||
    getFilterValue('location') ||
    globalFilter

  const clearAllFilters = () => {
    setArchiveFilter('active')
    setUsageFilter('all')
    setColumnFilters([])
    setGlobalFilter('')
  }

  return (
    <div class="space-y-4">
      {/* Toolbar */}
      <div class="flex flex-col gap-3">
        {/* Top row: Search, View toggle, Columns */}
        <div class="flex flex-col sm:flex-row gap-3 items-start sm:items-center justify-between">
          <div class="relative flex-1 max-w-md">
            <Search class="absolute left-3 top-1/2 -translate-y-1/2 w-4 h-4 text-[var(--text-muted)]" />
            <input
              type="text"
              value={globalFilter}
              onInput={(e) => setGlobalFilter((e.target as HTMLInputElement).value)}
              placeholder="Search spools..."
              class="input input-with-icon w-full"
            />
            {globalFilter && (
              <button
                onClick={() => setGlobalFilter('')}
                class="absolute right-3 top-1/2 -translate-y-1/2 text-[var(--text-muted)] hover:text-[var(--text-primary)]"
              >
                <X class="w-4 h-4" />
              </button>
            )}
          </div>

          <div class="flex items-center gap-2">
            <button
              onClick={onOpenColumns}
              class="btn"
              title="Configure Columns"
            >
              <Columns class="w-4 h-4" />
              <span class="hidden sm:inline">Columns</span>
            </button>
            <div class="flex bg-[var(--bg-primary)] border border-[var(--border-color)] rounded-lg overflow-hidden">
              <button
                onClick={() => setViewMode('table')}
                class={`px-3 py-1.5 text-sm font-medium transition-colors ${
                  viewMode === 'table'
                    ? 'bg-[var(--accent-color)] text-white'
                    : 'text-[var(--text-secondary)] hover:bg-[var(--bg-tertiary)]'
                }`}
              >
                Table
              </button>
              <button
                onClick={() => setViewMode('cards')}
                class={`px-3 py-1.5 text-sm font-medium transition-colors ${
                  viewMode === 'cards'
                    ? 'bg-[var(--accent-color)] text-white'
                    : 'text-[var(--text-secondary)] hover:bg-[var(--bg-tertiary)]'
                }`}
              >
                Cards
              </button>
            </div>
          </div>
        </div>

        {/* Filter chips row */}
        <div class="flex flex-wrap items-center gap-2">
          {/* Status chips */}
          <div class="flex items-center rounded-lg border border-[var(--border-color)] overflow-hidden">
            <button
              onClick={() => setArchiveFilter('active')}
              class={`filter-chip ${archiveFilter === 'active' ? 'filter-chip-active' : ''}`}
            >
              <Package class="w-3.5 h-3.5" />
              Active
            </button>
            <button
              onClick={() => setArchiveFilter('archived')}
              class={`filter-chip ${archiveFilter === 'archived' ? 'filter-chip-active' : ''}`}
            >
              <Archive class="w-3.5 h-3.5" />
              Archived
            </button>
          </div>

          <div class="w-px h-5 bg-[var(--border-color)]" />

          {/* Usage chips */}
          <div class="flex items-center rounded-lg border border-[var(--border-color)] overflow-hidden">
            <button
              onClick={() => setUsageFilter('all')}
              class={`filter-chip ${usageFilter === 'all' ? 'filter-chip-active' : ''}`}
            >
              All
            </button>
            <button
              onClick={() => setUsageFilter('used')}
              class={`filter-chip ${usageFilter === 'used' ? 'filter-chip-active' : ''}`}
            >
              <Clock class="w-3.5 h-3.5" />
              Used
            </button>
            <button
              onClick={() => setUsageFilter('unused')}
              class={`filter-chip ${usageFilter === 'unused' ? 'filter-chip-active' : ''}`}
            >
              New
            </button>
          </div>

          <div class="w-px h-5 bg-[var(--border-color)]" />

          {/* Material dropdown as chip */}
          <select
            value={getFilterValue('material')}
            onChange={(e) => setFilterValue('material', (e.target as HTMLSelectElement).value)}
            class={`filter-chip-select ${getFilterValue('material') ? 'filter-chip-select-active' : ''}`}
          >
            <option value="">Material</option>
            {uniqueMaterials.map(m => (
              <option key={m} value={m}>{m}</option>
            ))}
          </select>

          {/* Brand dropdown as chip */}
          <select
            value={getFilterValue('brand')}
            onChange={(e) => setFilterValue('brand', (e.target as HTMLSelectElement).value)}
            class={`filter-chip-select ${getFilterValue('brand') ? 'filter-chip-select-active' : ''}`}
          >
            <option value="">Brand</option>
            {uniqueBrands.map(b => (
              <option key={b} value={b}>{b}</option>
            ))}
          </select>

          {/* Location dropdown as chip */}
          {uniqueLocations.length > 0 && (
            <select
              value={getFilterValue('location')}
              onChange={(e) => setFilterValue('location', (e.target as HTMLSelectElement).value)}
              class={`filter-chip-select ${getFilterValue('location') ? 'filter-chip-select-active' : ''}`}
            >
              <option value="">Location</option>
              {uniqueLocations.map(l => (
                <option key={l} value={l}>{l}</option>
              ))}
            </select>
          )}

          {/* Clear all filters button */}
          {hasActiveFilters && (
            <>
              <div class="w-px h-5 bg-[var(--border-color)]" />
              <button
                onClick={clearAllFilters}
                class="text-xs text-[var(--text-muted)] hover:text-[var(--accent-color)] transition-colors flex items-center gap-1"
              >
                <X class="w-3.5 h-3.5" />
                Clear filters
              </button>
            </>
          )}

          {/* Results count */}
          <span class="ml-auto text-xs text-[var(--text-muted)]">
            {table.getFilteredRowModel().rows.length} spool{table.getFilteredRowModel().rows.length !== 1 ? 's' : ''}
          </span>
        </div>
      </div>

      {/* Content */}
      {viewMode === 'cards' ? (
        <div class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-4">
          {table.getRowModel().rows.map((row, index) => (
            <SpoolCard
              key={row.original.id || index}
              spool={row.original}
              isInPrinter={!!spoolsInPrinters[row.original.id]}
              printerLocation={spoolsInPrinters[row.original.id]}
              onClick={() => onEditSpool?.(row.original)}
            />
          ))}
          {table.getRowModel().rows.length === 0 && (
            <div class="col-span-full">
              <EmptyState
                type={
                  globalFilter ? 'no-search-results' :
                  archiveFilter === 'archived' ? 'no-archived' :
                  usageFilter === 'used' ? 'no-used' :
                  usageFilter === 'unused' ? 'no-unused' :
                  'no-spools'
                }
                onAddSpool={onAddSpool}
              />
            </div>
          )}
        </div>
      ) : (
        <>
          <div class="card overflow-hidden">
            <div class="overflow-x-auto">
              <table class="table">
                <thead>
                  {table.getHeaderGroups().map((headerGroup) => (
                    <tr key={headerGroup.id}>
                      {headerGroup.headers.map((header) => (
                        <th
                          key={header.id}
                          style={{ width: header.getSize() }}
                          onClick={header.column.getToggleSortingHandler()}
                        >
                          <div class="flex items-center gap-1">
                            {flexRender(header.column.columnDef.header, header.getContext())}
                            {header.column.getIsSorted() === 'asc' && <ChevronUp class="w-3 h-3" />}
                            {header.column.getIsSorted() === 'desc' && <ChevronDown class="w-3 h-3" />}
                          </div>
                        </th>
                      ))}
                    </tr>
                  ))}
                </thead>
                <tbody>
                  {table.getRowModel().rows.map((row) => (
                    <tr
                      key={row.id}
                      onClick={() => onEditSpool?.(row.original)}
                    >
                      {row.getVisibleCells().map((cell) => (
                        <td key={cell.id}>
                          {flexRender(cell.column.columnDef.cell, cell.getContext())}
                        </td>
                      ))}
                    </tr>
                  ))}
                  {table.getRowModel().rows.length === 0 && (
                    <tr>
                      <td colSpan={columns.length}>
                        <EmptyState
                          type={
                            globalFilter ? 'no-search-results' :
                            archiveFilter === 'archived' ? 'no-archived' :
                            usageFilter === 'used' ? 'no-used' :
                            usageFilter === 'unused' ? 'no-unused' :
                            'no-spools'
                          }
                          onAddSpool={onAddSpool}
                        />
                      </td>
                    </tr>
                  )}
                </tbody>
              </table>
            </div>

            {/* Pagination */}
            <div class="flex items-center justify-between px-4 py-3 bg-[var(--bg-tertiary)] border-t border-[var(--border-color)] text-sm">
              <span class="text-[var(--text-secondary)]">
                Showing {table.getState().pagination.pageIndex * table.getState().pagination.pageSize + 1} to{' '}
                {Math.min(
                  (table.getState().pagination.pageIndex + 1) * table.getState().pagination.pageSize,
                  table.getFilteredRowModel().rows.length
                )}{' '}
                of {table.getFilteredRowModel().rows.length} spools
              </span>

              <div class="flex items-center gap-2">
                <span class="text-[var(--text-secondary)]">Show</span>
                <select
                  class="select w-auto"
                  value={table.getState().pagination.pageSize}
                  onChange={(e) => table.setPageSize(Number((e.target as HTMLSelectElement).value))}
                >
                  {[15, 30, 50, 100].map((size) => (
                    <option key={size} value={size}>{size}</option>
                  ))}
                </select>

                <button
                  onClick={() => table.setPageIndex(0)}
                  disabled={!table.getCanPreviousPage()}
                  class="btn btn-icon"
                >
                  ««
                </button>
                <button
                  onClick={() => table.previousPage()}
                  disabled={!table.getCanPreviousPage()}
                  class="btn btn-icon"
                >
                  ‹
                </button>
                <span class="px-2 text-[var(--text-secondary)] whitespace-nowrap">
                  Page {table.getState().pagination.pageIndex + 1} of {table.getPageCount()}
                </span>
                <button
                  onClick={() => table.nextPage()}
                  disabled={!table.getCanNextPage()}
                  class="btn btn-icon"
                >
                  ›
                </button>
                <button
                  onClick={() => table.setPageIndex(table.getPageCount() - 1)}
                  disabled={!table.getCanNextPage()}
                  class="btn btn-icon"
                >
                  »»
                </button>
              </div>
            </div>
          </div>
        </>
      )}
    </div>
  )
}
