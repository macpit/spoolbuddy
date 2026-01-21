import { useState, useEffect, useCallback } from 'preact/hooks'
import { api, ColorEntry } from '../lib/api'
import { useToast } from '../lib/toast'
import { Palette, Plus, Trash2, RotateCcw, Loader2, Edit2, Check, X, Search, Download, Upload, Cloud } from 'lucide-preact'
import { useRef } from 'preact/hooks'

export function ColorCatalogSettings() {
  const { showToast } = useToast()
  const [catalog, setCatalog] = useState<ColorEntry[]>([])
  const [loading, setLoading] = useState(true)
  const [search, setSearch] = useState('')
  const [filterManufacturer, setFilterManufacturer] = useState<string>('Bambu Lab')
  const fileInputRef = useRef<HTMLInputElement>(null)

  // Add/Edit form state
  const [showAddForm, setShowAddForm] = useState(false)
  const [editingId, setEditingId] = useState<number | null>(null)
  const [formManufacturer, setFormManufacturer] = useState('')
  const [formColorName, setFormColorName] = useState('')
  const [formHexColor, setFormHexColor] = useState('#FFFFFF')
  const [formMaterial, setFormMaterial] = useState('')
  const [saving, setSaving] = useState(false)

  // Sync state
  const [syncing, setSyncing] = useState(false)
  const [syncProgress, setSyncProgress] = useState<{ fetched: number; total: number } | null>(null)

  // Delete confirmation state
  const [deleteId, setDeleteId] = useState<number | null>(null)
  const [deleteName, setDeleteName] = useState('')
  const [showResetConfirm, setShowResetConfirm] = useState(false)

  // Load catalog
  const loadCatalog = useCallback(async () => {
    try {
      const entries = await api.getColorCatalog()
      setCatalog(entries)
    } catch (e) {
      showToast('error', 'Failed to load color catalog')
    } finally {
      setLoading(false)
    }
  }, [showToast])

  useEffect(() => {
    loadCatalog()
  }, [loadCatalog])

  // Get unique manufacturers for filter dropdown
  const manufacturers = [...new Set(catalog.map(e => e.manufacturer))].sort()

  // Filter catalog based on search and manufacturer
  const filteredCatalog = catalog.filter(entry => {
    const matchesSearch = search === '' ||
      entry.manufacturer.toLowerCase().includes(search.toLowerCase()) ||
      entry.color_name.toLowerCase().includes(search.toLowerCase()) ||
      (entry.material?.toLowerCase().includes(search.toLowerCase()) ?? false)
    const matchesManufacturer = filterManufacturer === '' || entry.manufacturer === filterManufacturer
    return matchesSearch && matchesManufacturer
  })

  // Reset form
  const resetForm = () => {
    setFormManufacturer('')
    setFormColorName('')
    setFormHexColor('#FFFFFF')
    setFormMaterial('')
  }

  // Handle add entry
  const handleAdd = async () => {
    if (!formManufacturer.trim() || !formColorName.trim() || !formHexColor) {
      showToast('error', 'Manufacturer, color name, and hex color are required')
      return
    }
    setSaving(true)
    try {
      const entry = await api.addColorEntry({
        manufacturer: formManufacturer.trim(),
        color_name: formColorName.trim(),
        hex_color: formHexColor,
        material: formMaterial.trim() || null
      })
      setCatalog(prev => [...prev, entry].sort((a, b) =>
        a.manufacturer.localeCompare(b.manufacturer) ||
        (a.material || '').localeCompare(b.material || '') ||
        a.color_name.localeCompare(b.color_name)
      ))
      setShowAddForm(false)
      resetForm()
      showToast('success', 'Color added')
    } catch (e) {
      showToast('error', 'Failed to add color')
    } finally {
      setSaving(false)
    }
  }

  // Handle edit entry
  const startEdit = (entry: ColorEntry) => {
    setEditingId(entry.id)
    setFormManufacturer(entry.manufacturer)
    setFormColorName(entry.color_name)
    setFormHexColor(entry.hex_color)
    setFormMaterial(entry.material || '')
  }

  const cancelEdit = () => {
    setEditingId(null)
    resetForm()
  }

  const handleUpdate = async (id: number) => {
    if (!formManufacturer.trim() || !formColorName.trim() || !formHexColor) {
      showToast('error', 'Manufacturer, color name, and hex color are required')
      return
    }
    setSaving(true)
    try {
      const updated = await api.updateColorEntry(id, {
        manufacturer: formManufacturer.trim(),
        color_name: formColorName.trim(),
        hex_color: formHexColor,
        material: formMaterial.trim() || null
      })
      setCatalog(prev =>
        prev.map(e => e.id === id ? updated : e).sort((a, b) =>
          a.manufacturer.localeCompare(b.manufacturer) ||
          (a.material || '').localeCompare(b.material || '') ||
          a.color_name.localeCompare(b.color_name)
        )
      )
      setEditingId(null)
      resetForm()
      showToast('success', 'Color updated')
    } catch (e) {
      showToast('error', 'Failed to update color')
    } finally {
      setSaving(false)
    }
  }

  // Handle delete entry
  const confirmDelete = (entry: ColorEntry) => {
    setDeleteId(entry.id)
    setDeleteName(`${entry.manufacturer} - ${entry.color_name}`)
  }

  const handleDelete = async () => {
    if (!deleteId) return
    try {
      await api.deleteColorEntry(deleteId)
      setCatalog(prev => prev.filter(e => e.id !== deleteId))
      showToast('success', 'Color deleted')
    } catch (e) {
      showToast('error', 'Failed to delete color')
    } finally {
      setDeleteId(null)
      setDeleteName('')
    }
  }

  // Handle reset to defaults
  const handleReset = async () => {
    setShowResetConfirm(false)
    setLoading(true)
    try {
      await api.resetColorCatalog()
      await loadCatalog()
      showToast('success', 'Color catalog reset to defaults')
    } catch (e) {
      showToast('error', 'Failed to reset catalog')
      setLoading(false)
    }
  }

  // Handle sync from FilamentColors.xyz
  const handleSync = async () => {
    setSyncing(true)
    setSyncProgress(null)

    try {
      const response = await fetch('/api/colors/sync', { method: 'POST' })

      if (!response.ok) {
        throw new Error('Failed to start sync')
      }

      const reader = response.body?.getReader()
      if (!reader) {
        throw new Error('No response body')
      }

      const decoder = new TextDecoder()
      let buffer = ''

      while (true) {
        const { done, value } = await reader.read()
        if (done) break

        buffer += decoder.decode(value, { stream: true })
        const lines = buffer.split('\n')
        buffer = lines.pop() || ''

        for (const line of lines) {
          if (line.startsWith('data: ')) {
            try {
              const data = JSON.parse(line.slice(6))
              if (data.type === 'progress') {
                setSyncProgress({ fetched: data.total_fetched, total: data.total_available })
              } else if (data.type === 'complete') {
                if (data.added === 0) {
                  showToast('success', `Already up to date (${data.total_fetched} colors checked)`)
                } else {
                  showToast('success', `Added ${data.added} new colors (${data.skipped} already existed)`)
                }
              } else if (data.type === 'error') {
                showToast('error', `Sync error: ${data.error}`)
              }
            } catch {
              // Ignore parse errors
            }
          }
        }
      }

      await loadCatalog()
    } catch (e) {
      showToast('error', 'Failed to sync from FilamentColors.xyz')
    } finally {
      setSyncing(false)
      setSyncProgress(null)
    }
  }

  // Export catalog to JSON file
  const handleExport = () => {
    const exportData = catalog.map(({ manufacturer, color_name, hex_color, material }) => ({
      manufacturer, color_name, hex_color, material
    }))
    const blob = new Blob([JSON.stringify(exportData, null, 2)], { type: 'application/json' })
    const url = URL.createObjectURL(blob)
    const a = document.createElement('a')
    a.href = url
    a.download = 'color-catalog.json'
    document.body.appendChild(a)
    a.click()
    document.body.removeChild(a)
    URL.revokeObjectURL(url)
    showToast('success', `Exported ${catalog.length} colors`)
  }

  // Import catalog from JSON file
  const handleImport = async (e: Event) => {
    const file = (e.target as HTMLInputElement).files?.[0]
    if (!file) return

    try {
      const text = await file.text()
      const data = JSON.parse(text) as Array<{ manufacturer: string; color_name: string; hex_color: string; material?: string | null }>

      if (!Array.isArray(data)) {
        throw new Error('Invalid format: expected array')
      }

      let added = 0
      let skipped = 0

      for (const item of data) {
        if (!item.manufacturer || !item.color_name || !item.hex_color) {
          skipped++
          continue
        }
        // Check if entry already exists
        const exists = catalog.some(c =>
          c.manufacturer.toLowerCase() === item.manufacturer.toLowerCase() &&
          c.color_name.toLowerCase() === item.color_name.toLowerCase() &&
          (c.material || '').toLowerCase() === (item.material || '').toLowerCase()
        )
        if (exists) {
          skipped++
          continue
        }
        try {
          const entry = await api.addColorEntry({
            manufacturer: item.manufacturer,
            color_name: item.color_name,
            hex_color: item.hex_color,
            material: item.material || null
          })
          setCatalog(prev => [...prev, entry].sort((a, b) =>
            a.manufacturer.localeCompare(b.manufacturer) ||
            (a.material || '').localeCompare(b.material || '') ||
            a.color_name.localeCompare(b.color_name)
          ))
          added++
        } catch {
          skipped++
        }
      }

      showToast('success', `Imported ${added} colors (${skipped} skipped)`)
    } catch (e) {
      showToast('error', 'Failed to import: invalid JSON format')
    }

    // Reset file input
    if (fileInputRef.current) {
      fileInputRef.current.value = ''
    }
  }

  return (
    <div class="card">
      <div class="px-6 py-4 border-b border-[var(--border-color)]">
        <div class="flex items-center gap-2 mb-3">
          <Palette class="w-5 h-5 text-[var(--text-muted)]" />
          <h2 class="text-lg font-medium text-[var(--text-primary)]">Color Catalog</h2>
          <span class="text-sm text-[var(--text-muted)]">({catalog.length})</span>
        </div>
        <div class="flex items-center gap-2 flex-wrap">
          <button
            onClick={handleExport}
            class="btn flex items-center gap-1.5"
            title="Export catalog to JSON"
          >
            <Download class="w-4 h-4" />
            <span class="hidden sm:inline">Export</span>
          </button>
          <button
            onClick={() => fileInputRef.current?.click()}
            class="btn flex items-center gap-1.5"
            title="Import catalog from JSON"
          >
            <Upload class="w-4 h-4" />
            <span class="hidden sm:inline">Import</span>
          </button>
          <input
            ref={fileInputRef}
            type="file"
            accept=".json"
            class="hidden"
            onChange={handleImport}
          />
          <button
            onClick={handleSync}
            disabled={syncing}
            class="btn flex items-center gap-1.5"
            title="Sync from FilamentColors.xyz (2000+ colors, may take a minute)"
          >
            {syncing ? <Loader2 class="w-4 h-4 animate-spin" /> : <Cloud class="w-4 h-4" />}
            <span class="hidden sm:inline">
              {syncing
                ? syncProgress
                  ? `${Math.min(syncProgress.fetched, syncProgress.total)} / ${syncProgress.total}`
                  : 'Starting...'
                : 'Sync'}
            </span>
          </button>
          <button
            onClick={() => setShowResetConfirm(true)}
            class="btn flex items-center gap-1.5"
            title="Reset to defaults"
          >
            <RotateCcw class="w-4 h-4" />
            <span class="hidden sm:inline">Reset</span>
          </button>
          <button
            onClick={() => setShowAddForm(true)}
            class="btn btn-primary flex items-center gap-1.5"
          >
            <Plus class="w-4 h-4" />
            <span class="hidden sm:inline">Add</span>
          </button>
        </div>
      </div>

      <div class="p-6 space-y-4">
        <p class="text-sm text-[var(--text-secondary)]">
          Filament colors by manufacturer/material. Used for automatic color lookup when adding spools.
        </p>

        {/* Search and filter */}
        <div class="flex gap-2 flex-wrap">
          <div class="relative flex-1 min-w-[200px]">
            <Search class="absolute left-3 top-1/2 -translate-y-1/2 w-4 h-4 text-[var(--text-muted)]" />
            <input
              type="text"
              class="input input-with-icon w-full"
              placeholder="Search colors..."
              value={search}
              onInput={(e) => setSearch((e.target as HTMLInputElement).value)}
            />
          </div>
          <select
            class="input"
            value={filterManufacturer}
            onInput={(e) => setFilterManufacturer((e.target as HTMLSelectElement).value)}
          >
            <option value="">All manufacturers</option>
            {manufacturers.map(m => (
              <option key={m} value={m}>{m}</option>
            ))}
          </select>
        </div>

        {/* Add form */}
        {showAddForm && (
          <div class="p-4 bg-[var(--bg-tertiary)] rounded-lg border border-[var(--border-color)]">
            <h3 class="text-sm font-medium text-[var(--text-primary)] mb-3">Add New Color</h3>
            <div class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-5 gap-2 items-end">
              <input
                type="text"
                class="input"
                placeholder="Manufacturer"
                value={formManufacturer}
                onInput={(e) => setFormManufacturer((e.target as HTMLInputElement).value)}
              />
              <input
                type="text"
                class="input"
                placeholder="Color Name"
                value={formColorName}
                onInput={(e) => setFormColorName((e.target as HTMLInputElement).value)}
              />
              <div class="flex items-center gap-2">
                <input
                  type="color"
                  class="w-10 h-10 rounded cursor-pointer border border-[var(--border-color)]"
                  value={formHexColor}
                  onInput={(e) => setFormHexColor((e.target as HTMLInputElement).value)}
                />
                <input
                  type="text"
                  class="input flex-1"
                  placeholder="#FFFFFF"
                  value={formHexColor}
                  onInput={(e) => setFormHexColor((e.target as HTMLInputElement).value)}
                />
              </div>
              <input
                type="text"
                class="input"
                placeholder="Material (optional)"
                value={formMaterial}
                onInput={(e) => setFormMaterial((e.target as HTMLInputElement).value)}
              />
              <div class="flex gap-2">
                <button
                  onClick={handleAdd}
                  disabled={saving}
                  class="btn btn-primary flex items-center gap-1 flex-1"
                >
                  {saving ? <Loader2 class="w-4 h-4 animate-spin" /> : <Check class="w-4 h-4" />}
                  Add
                </button>
                <button
                  onClick={() => { setShowAddForm(false); resetForm() }}
                  class="btn"
                >
                  <X class="w-4 h-4" />
                </button>
              </div>
            </div>
          </div>
        )}

        {/* Debug: show filter state */}
        {(search || filterManufacturer) && (
          <div class="text-xs text-[var(--text-muted)]">
            Showing {filteredCatalog.length} of {catalog.length} colors
            {search && ` (search: "${search}")`}
            {filterManufacturer && ` (manufacturer: ${filterManufacturer})`}
          </div>
        )}

        {/* Catalog list */}
        {loading ? (
          <div class="flex items-center justify-center py-8 text-[var(--text-muted)]">
            <Loader2 class="w-5 h-5 animate-spin mr-2" />
            Loading catalog...
          </div>
        ) : (
          <div class="max-h-[400px] overflow-auto border border-[var(--border-color)] rounded-lg">
            <table class="w-full text-sm">
              <thead class="bg-[var(--bg-tertiary)] sticky top-0">
                <tr>
                  <th class="px-3 py-2 text-left text-[var(--text-secondary)] font-medium w-12"></th>
                  <th class="px-3 py-2 text-left text-[var(--text-secondary)] font-medium">Manufacturer</th>
                  <th class="px-3 py-2 text-left text-[var(--text-secondary)] font-medium">Color Name</th>
                  <th class="px-3 py-2 text-left text-[var(--text-secondary)] font-medium w-24">Hex</th>
                  <th class="px-3 py-2 text-left text-[var(--text-secondary)] font-medium">Material</th>
                  <th class="px-3 py-2 w-16"></th>
                </tr>
              </thead>
              <tbody>
                {filteredCatalog.length === 0 ? (
                  <tr>
                    <td colSpan={6} class="px-3 py-8 text-center text-[var(--text-muted)]">
                      {search || filterManufacturer ? 'No colors match your search' : 'No colors in catalog'}
                    </td>
                  </tr>
                ) : (
                  filteredCatalog.map(entry => (
                    <tr
                      key={entry.id}
                      class="border-t border-[var(--border-color)] hover:bg-[var(--bg-secondary)]"
                    >
                      {editingId === entry.id ? (
                        <>
                          <td class="px-3 py-2">
                            <input
                              type="color"
                              class="w-8 h-8 rounded cursor-pointer border border-[var(--border-color)]"
                              value={formHexColor}
                              onInput={(e) => setFormHexColor((e.target as HTMLInputElement).value)}
                              title={formHexColor}
                            />
                          </td>
                          <td class="px-3 py-2">
                            <input
                              type="text"
                              class="input w-full text-sm"
                              value={formManufacturer}
                              onInput={(e) => setFormManufacturer((e.target as HTMLInputElement).value)}
                            />
                          </td>
                          <td class="px-3 py-2">
                            <input
                              type="text"
                              class="input w-full text-sm"
                              value={formColorName}
                              onInput={(e) => setFormColorName((e.target as HTMLInputElement).value)}
                            />
                          </td>
                          <td class="px-3 py-2">
                            <input
                              type="text"
                              class="input w-full text-sm"
                              value={formHexColor}
                              onInput={(e) => setFormHexColor((e.target as HTMLInputElement).value)}
                            />
                          </td>
                          <td class="px-3 py-2">
                            <input
                              type="text"
                              class="input w-full text-sm"
                              value={formMaterial}
                              onInput={(e) => setFormMaterial((e.target as HTMLInputElement).value)}
                            />
                          </td>
                          <td class="px-3 py-2">
                            <div class="flex justify-end gap-1">
                              <button
                                onClick={() => handleUpdate(entry.id)}
                                disabled={saving}
                                class="p-1.5 rounded hover:bg-green-500/20 text-green-500"
                                title="Save"
                              >
                                {saving ? <Loader2 class="w-4 h-4 animate-spin" /> : <Check class="w-4 h-4" />}
                              </button>
                              <button
                                onClick={cancelEdit}
                                class="p-1.5 rounded hover:bg-[var(--bg-tertiary)] text-[var(--text-muted)]"
                                title="Cancel"
                              >
                                <X class="w-4 h-4" />
                              </button>
                            </div>
                          </td>
                        </>
                      ) : (
                        <>
                          <td class="px-3 py-2">
                            <div
                              class="w-8 h-8 rounded border border-[var(--border-color)]"
                              style={{ backgroundColor: entry.hex_color }}
                              title={entry.hex_color}
                            />
                          </td>
                          <td class="px-3 py-2 text-[var(--text-primary)]">{entry.manufacturer}</td>
                          <td class="px-3 py-2 text-[var(--text-primary)]">{entry.color_name}</td>
                          <td class="px-3 py-2 font-mono text-xs text-[var(--text-muted)]">{entry.hex_color}</td>
                          <td class="px-3 py-2 text-[var(--text-secondary)]">{entry.material || '-'}</td>
                          <td class="px-3 py-2">
                            <div class="flex justify-end gap-1">
                              <button
                                onClick={() => startEdit(entry)}
                                class="p-1.5 rounded hover:bg-[var(--bg-tertiary)] text-[var(--text-muted)] hover:text-[var(--text-primary)]"
                                title="Edit"
                              >
                                <Edit2 class="w-4 h-4" />
                              </button>
                              <button
                                onClick={() => confirmDelete(entry)}
                                class="p-1.5 rounded bg-red-500/10 hover:bg-red-500/20 text-red-500"
                                title="Delete"
                              >
                                <Trash2 class="w-4 h-4" />
                              </button>
                            </div>
                          </td>
                        </>
                      )}
                    </tr>
                  ))
                )}
              </tbody>
            </table>
          </div>
        )}
      </div>

      {/* Delete confirmation modal */}
      {deleteId && (
        <div class="fixed inset-0 z-50 flex items-center justify-center">
          <div class="absolute inset-0 bg-black/50" onClick={() => setDeleteId(null)} />
          <div class="relative bg-[var(--bg-primary)] rounded-lg shadow-xl p-6 max-w-sm mx-4">
            <h3 class="text-lg font-medium text-[var(--text-primary)] mb-2">Delete Color</h3>
            <p class="text-sm text-[var(--text-secondary)] mb-4">
              Are you sure you want to delete "<span class="font-medium">{deleteName}</span>"?
            </p>
            <div class="flex justify-end gap-2">
              <button
                onClick={() => setDeleteId(null)}
                class="btn"
              >
                Cancel
              </button>
              <button
                onClick={handleDelete}
                class="px-4 py-2 rounded-lg font-medium bg-red-500 hover:bg-red-600 text-white transition-colors"
              >
                Delete
              </button>
            </div>
          </div>
        </div>
      )}

      {/* Reset confirmation modal */}
      {showResetConfirm && (
        <div class="fixed inset-0 z-50 flex items-center justify-center">
          <div class="absolute inset-0 bg-black/50" onClick={() => setShowResetConfirm(false)} />
          <div class="relative bg-[var(--bg-primary)] rounded-lg shadow-xl p-6 max-w-sm mx-4">
            <h3 class="text-lg font-medium text-[var(--text-primary)] mb-2">Reset Color Catalog</h3>
            <p class="text-sm text-[var(--text-secondary)] mb-4">
              Reset catalog to defaults? This will remove all custom colors.
            </p>
            <div class="flex justify-end gap-2">
              <button
                onClick={() => setShowResetConfirm(false)}
                class="btn"
              >
                Cancel
              </button>
              <button
                onClick={handleReset}
                class="px-4 py-2 rounded-lg font-medium bg-red-500 hover:bg-red-600 text-white transition-colors"
              >
                Reset
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  )
}
