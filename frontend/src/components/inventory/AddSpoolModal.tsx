import { useState, useEffect, useMemo, useRef } from 'preact/hooks'
import { Modal } from './Modal'
import { Spool, SpoolInput, Printer, CalibrationProfile, SlicerPreset, SpoolKProfile, CatalogEntry, api } from '../../lib/api'
import { X, ChevronDown, ChevronRight, Cloud, CloudOff, Trash2, Unlink, Archive, ArchiveRestore } from 'lucide-preact'
import { getFilamentOptions, COLOR_PRESETS } from './utils'
import { useToast } from '../../lib/toast'

// Type for printer with its calibrations
export interface PrinterWithCalibrations {
  printer: Printer;
  calibrations: CalibrationProfile[];
}

interface AddSpoolModalProps {
  isOpen: boolean
  onClose: () => void
  onSave: (input: SpoolInput) => Promise<Spool>  // Returns the saved spool
  editSpool?: Spool | null  // If provided, we're editing
  onDelete?: (spool: Spool) => void  // Called when delete button is clicked
  onArchive?: (spool: Spool) => void  // Called when archive button is clicked
  onRestore?: (spool: Spool) => void  // Called when restore button is clicked
  onTagRemoved?: () => void  // Called after tag is successfully removed
  printersWithCalibrations?: PrinterWithCalibrations[]
  initialTagId?: string | null  // Tag ID to pre-fill when adding new spool
  initialWeight?: number | null  // Current weight from scale to pre-fill
}

interface SpoolFormData {
  material: string
  subtype: string
  brand: string
  color_name: string
  rgba: string
  label_weight: number
  core_weight: number
  slicer_filament: string
  location: string
  note: string
}

const defaultFormData: SpoolFormData = {
  material: '',
  subtype: '',
  brand: '',
  color_name: '',
  rgba: '#808080',
  label_weight: 1000,
  core_weight: 250,
  slicer_filament: '',
  location: '',
  note: '',
}

const MATERIALS = ['PLA', 'PETG', 'ABS', 'TPU', 'ASA', 'PC', 'PA', 'PVA', 'HIPS', 'PA-CF', 'PETG-CF', 'PLA-CF']
const WEIGHTS = [250, 500, 750, 1000, 2000, 3000]
const DEFAULT_BRANDS = ['Bambu', 'PolyLite', 'PolyTerra', 'eSUN', 'Overture', 'Fiberon', 'SUNLU', 'Inland', 'Hatchbox', 'Generic']

// Known filament variants/subtypes - only these will be auto-filled
const KNOWN_VARIANTS = [
  'Basic', 'Matte', 'Silk', 'Tough', 'HF', 'High Flow', 'Engineering',
  'Galaxy', 'Glow', 'Marble', 'Metal', 'Rainbow', 'Sparkle', 'Wood',
  'Translucent', 'Transparent', 'Clear', 'Lite', 'Pro', 'Plus', 'Max',
  'Super', 'Ultra', 'Flex', 'Soft', 'Hard', 'Strong', 'Impact',
  'Heat Resistant', 'UV Resistant', 'ESD', 'Conductive', 'Magnetic',
  'Gradient', 'Dual Color', 'Tri Color', 'Multicolor',
]

// Parse a slicer preset name to extract brand, material, and variant
function parsePresetName(name: string): { brand: string; material: string; variant: string } {
  // Remove @printer suffix (e.g., "@Bambu Lab H2D 0.4 nozzle")
  let cleanName = name.replace(/@.*$/, '').trim()
  // Remove (Custom) tag
  cleanName = cleanName.replace(/\(Custom\)/i, '').trim()

  // Materials list - order matters (longer/more specific first)
  const materials = [
    'PLA-CF', 'PETG-CF', 'ABS-GF', 'ASA-CF', 'PA-CF', 'PAHT-CF', 'PA6-CF', 'PA6-GF',
    'PPA-CF', 'PPA-GF', 'PET-CF', 'PPS-CF', 'PC-CF', 'PC-ABS', 'ABS-GF',
    'PETG', 'PLA', 'ABS', 'ASA', 'PC', 'PA', 'TPU', 'PVA', 'HIPS', 'BVOH', 'PPS', 'PCTG', 'PEEK', 'PEI'
  ]

  // Find material in the name
  let material = ''
  let materialIdx = -1
  for (const m of materials) {
    const idx = cleanName.toUpperCase().indexOf(m.toUpperCase())
    if (idx !== -1) {
      material = m
      materialIdx = idx
      break
    }
  }

  // Brand is everything before the material
  let brand = ''
  if (materialIdx > 0) {
    brand = cleanName.substring(0, materialIdx).trim()
    // Remove trailing spaces/dashes
    brand = brand.replace(/[-_\s]+$/, '')
  }

  // Everything after material is potential variant
  let afterMaterial = ''
  if (materialIdx !== -1 && material) {
    afterMaterial = cleanName.substring(materialIdx + material.length).trim()
    // Remove leading spaces/dashes
    afterMaterial = afterMaterial.replace(/^[-_\s]+/, '')
  }

  // Check for known variant - could be before OR after material
  let variant = ''

  // First check after material (most common)
  for (const v of KNOWN_VARIANTS) {
    if (afterMaterial.toLowerCase().includes(v.toLowerCase())) {
      variant = v
      break
    }
  }

  // If no variant found after material, check if brand contains a known variant
  // and strip it from brand (e.g., "Overture Matte" -> brand: "Overture", variant: "Matte")
  if (!variant && brand) {
    for (const v of KNOWN_VARIANTS) {
      // Check if brand ends with the variant (with space before it)
      const variantPattern = new RegExp(`\\s+${v}$`, 'i')
      if (variantPattern.test(brand)) {
        variant = v
        brand = brand.replace(variantPattern, '').trim()
        break
      }
    }
  }

  return { brand, material, variant }
}

// Extract unique brands from cloud presets
function extractBrandsFromPresets(presets: SlicerPreset[]): string[] {
  const brandSet = new Set<string>(DEFAULT_BRANDS)

  for (const preset of presets) {
    const { brand } = parsePresetName(preset.name)
    if (brand && brand.length > 1) {
      brandSet.add(brand)
    }
  }

  return Array.from(brandSet).sort((a, b) => a.localeCompare(b))
}

// Quick color swatches - most common colors
const QUICK_COLORS = [
  { name: 'Black', hex: '000000' },
  { name: 'White', hex: 'FFFFFF' },
  { name: 'Gray', hex: '808080' },
  { name: 'Red', hex: 'FF0000' },
  { name: 'Orange', hex: 'FFA500' },
  { name: 'Yellow', hex: 'FFFF00' },
  { name: 'Green', hex: '00AE42' },
  { name: 'Blue', hex: '0066FF' },
  { name: 'Purple', hex: '8B00FF' },
  { name: 'Pink', hex: 'FF69B4' },
  { name: 'Brown', hex: '8B4513' },
  { name: 'Silver', hex: 'C0C0C0' },
]

// Searchable dropdown for spool weight selection
function SpoolWeightPicker({ catalog, value, onChange }: {
  catalog: CatalogEntry[]
  value: number
  onChange: (weight: number) => void
}) {
  const [search, setSearch] = useState('')
  const [isOpen, setIsOpen] = useState(false)
  const [selectedId, setSelectedId] = useState<number | null>(null) // Track selected entry by ID
  const inputRef = useRef<HTMLInputElement>(null)
  const dropdownRef = useRef<HTMLDivElement>(null)

  // Filter catalog based on search
  const filtered = useMemo(() => {
    if (!search) return catalog
    const lower = search.toLowerCase()
    return catalog.filter(e => e.name.toLowerCase().includes(lower))
  }, [catalog, search])

  // Get display value - prefer selected entry by ID, fallback to weight match
  const selectedEntry = selectedId
    ? catalog.find(c => c.id === selectedId)
    : catalog.find(c => c.weight === value)
  const displayValue = isOpen ? search : (selectedEntry?.name || '')

  // Handle click outside to close dropdown
  useEffect(() => {
    const handleClickOutside = (e: MouseEvent) => {
      if (dropdownRef.current && !dropdownRef.current.contains(e.target as Node)) {
        setIsOpen(false)
        setSearch('')
      }
    }
    if (isOpen) {
      document.addEventListener('mousedown', handleClickOutside)
      return () => document.removeEventListener('mousedown', handleClickOutside)
    }
  }, [isOpen])

  return (
    <div>
      <label class="label">Empty Spool Weight</label>
      <div class="flex gap-2 items-center">
        {/* Searchable dropdown */}
        <div class="flex-1 min-w-0 relative" ref={dropdownRef}>
          <input
            ref={inputRef}
            type="text"
            class="input w-full"
            placeholder="Search brand (e.g., Bambu Lab, eSUN)..."
            value={displayValue}
            onFocus={() => {
              setIsOpen(true)
              setSearch('')
            }}
            onInput={(e) => {
              setSearch((e.target as HTMLInputElement).value)
              setIsOpen(true)
            }}
          />
          {isOpen && (
            <div class="absolute z-50 w-full mt-1 bg-[var(--bg-secondary)] border border-[var(--border-color)] rounded-lg shadow-lg max-h-64 overflow-y-auto">
              {filtered.length === 0 ? (
                <div class="px-3 py-2 text-sm text-[var(--text-muted)]">No matches found</div>
              ) : (
                filtered.map(entry => (
                  <button
                    key={entry.id}
                    type="button"
                    class={`w-full px-3 py-2 text-left text-sm hover:bg-[var(--bg-tertiary)] flex justify-between items-center ${
                      (selectedId ? entry.id === selectedId : entry.weight === value) ? 'bg-[var(--accent-color)]/10 text-[var(--accent-color)]' : 'text-[var(--text-primary)]'
                    }`}
                    onClick={() => {
                      setSelectedId(entry.id)
                      onChange(entry.weight)
                      setIsOpen(false)
                      setSearch('')
                    }}
                  >
                    <span class="truncate">{entry.name}</span>
                    <span class="font-mono text-xs text-[var(--text-muted)] ml-2 shrink-0">{entry.weight}g</span>
                  </button>
                ))
              )}
            </div>
          )}
        </div>
        {/* Direct weight input */}
        <div class="flex items-center gap-1 shrink-0">
          <input
            type="number"
            class="input w-16 text-center"
            value={value}
            min={0}
            max={2000}
            onInput={(e) => {
              const val = parseInt((e.target as HTMLInputElement).value)
              if (!isNaN(val) && val >= 0) onChange(val)
            }}
          />
          <span class="text-[var(--text-muted)]">g</span>
        </div>
      </div>
    </div>
  )
}

export function AddSpoolModal({ isOpen, onClose, onSave, editSpool, onDelete, onArchive, onRestore, onTagRemoved, printersWithCalibrations = [], initialTagId, initialWeight }: AddSpoolModalProps) {
  const [formData, setFormData] = useState<SpoolFormData>(defaultFormData)
  const [isSubmitting, setIsSubmitting] = useState(false)
  const [error, setError] = useState<string | null>(null)
  const [showAllColors, setShowAllColors] = useState(false)
  const [activeTab, setActiveTab] = useState<'filament' | 'pa_profile'>('filament')
  const [expandedPrinters, setExpandedPrinters] = useState<Set<string>>(new Set())
  const [selectedProfiles, setSelectedProfiles] = useState<Set<string>>(new Set()) // "serial:cali_idx:extruder_id"
  const { showToast, updateToast } = useToast()

  // Cloud presets state
  const [cloudPresets, setCloudPresets] = useState<SlicerPreset[]>([])
  const [cloudAuthenticated, setCloudAuthenticated] = useState(false)
  const [loadingCloudPresets, setLoadingCloudPresets] = useState(false)

  // Spool catalog state (for empty spool weight lookup)
  const [spoolCatalog, setSpoolCatalog] = useState<CatalogEntry[]>([])

  // Separate state for preset input display (shows name, not code)
  const [presetInputValue, setPresetInputValue] = useState('')

  // State for remove tag confirmation modal
  const [showRemoveTagConfirm, setShowRemoveTagConfirm] = useState(false)
  const [isRemovingTag, setIsRemovingTag] = useState(false)

  const isEditing = !!editSpool

  // Fetch cloud presets and spool catalog when modal opens
  useEffect(() => {
    if (isOpen) {
      const fetchCloudPresets = async () => {
        setLoadingCloudPresets(true)
        try {
          const status = await api.getCloudStatus()
          setCloudAuthenticated(status.is_authenticated)

          if (status.is_authenticated) {
            const presets = await api.getFilamentPresets()
            setCloudPresets(presets)
          }
        } catch (e) {
          console.error('Failed to fetch cloud presets:', e)
          setCloudAuthenticated(false)
        } finally {
          setLoadingCloudPresets(false)
        }
      }
      fetchCloudPresets()

      // Fetch spool catalog for weight lookup
      api.getSpoolCatalog().then(setSpoolCatalog).catch(console.error)
    }
  }, [isOpen])

  // Reset form when modal opens/closes or editSpool changes
  useEffect(() => {
    if (isOpen) {
      if (editSpool) {
        setFormData({
          material: editSpool.material || '',
          subtype: editSpool.subtype || '',
          brand: editSpool.brand || '',
          color_name: editSpool.color_name || '',
          rgba: editSpool.rgba ? (editSpool.rgba.startsWith('#') ? editSpool.rgba : `#${editSpool.rgba}`) : '#808080',
          label_weight: editSpool.label_weight || 1000,
          core_weight: editSpool.core_weight || 250,
          slicer_filament: editSpool.slicer_filament || '',
          location: editSpool.location || '',
          note: editSpool.note || '',
        })
        // presetInputValue will be synced by the effect below

        // Load K-profiles for this spool
        api.getSpoolKProfiles(editSpool.id).then(profiles => {
          const profileKeys = new Set<string>()
          for (const p of profiles) {
            // Find matching calibration in printersWithCalibrations to get cali_idx
            const printerCals = printersWithCalibrations.find(pc => pc.printer.serial === p.printer_serial)
            if (printerCals && p.cali_idx !== null) {
              // Use consistent key format - null/undefined both become 'null'
              profileKeys.add(`${p.printer_serial}:${p.cali_idx}:${p.extruder ?? 'null'}`)
            }
          }
          setSelectedProfiles(profileKeys)
        }).catch(() => {
          // Ignore errors loading K-profiles
        })
      } else {
        setFormData(defaultFormData)
        setPresetInputValue('')
        setSelectedProfiles(new Set())
      }
      setError(null)
      setShowAllColors(false)
      setActiveTab('filament')
      setShowRemoveTagConfirm(false)
      // Expand all printers by default
      setExpandedPrinters(new Set(printersWithCalibrations.map(p => p.printer.serial)))
    }
  }, [isOpen, editSpool, printersWithCalibrations])

  const updateField = <K extends keyof SpoolFormData>(key: K, value: SpoolFormData[K]) => {
    setFormData(prev => ({ ...prev, [key]: value }))
  }

  const handleSubmit = async () => {
    if (!formData.slicer_filament) {
      setError('Slicer preset is required')
      setActiveTab('filament')
      return
    }

    setIsSubmitting(true)
    setError(null)

    // Show loading toast
    const toastId = showToast('loading', 'Saving spool...')

    try {
      // Save the spool
      const input: SpoolInput = {
        material: formData.material,
        subtype: formData.subtype || null,
        brand: formData.brand || null,
        color_name: formData.color_name || null,
        rgba: formData.rgba.replace('#', '') || null,
        label_weight: formData.label_weight,
        core_weight: formData.core_weight,
        slicer_filament: formData.slicer_filament || null,
        slicer_filament_name: selectedPresetOption?.displayName || presetInputValue || null,
        location: formData.location || null,
        note: formData.note || null,
        ext_has_k: selectedProfiles.size > 0,
        // Set data_origin for new spools (preserve existing for edits)
        data_origin: isEditing ? undefined : 'web',
        // Set tag_id: preserve for edits, use initialTagId for new spools
        tag_id: isEditing ? editSpool?.tag_id : initialTagId || null,
        // Set weight_current from scale if provided (for new spools)
        weight_current: isEditing ? editSpool?.weight_current : initialWeight || null,
      }

      const savedSpool = await onSave(input)

      // Save K-profiles if any selected
      if (selectedProfiles.size > 0) {
        updateToast(toastId, 'loading', 'Saving K-profiles...')
        const profiles: Omit<SpoolKProfile, 'id' | 'spool_id' | 'created_at'>[] = []

        selectedProfiles.forEach(key => {
          const parts = key.split(':')
          const serial = parts[0]
          const caliIdx = parseInt(parts[1], 10)
          // 'null' means single-nozzle or unspecified extruder
          const extruderStr = parts[2]
          const extruderId = extruderStr === 'null' ? null : parseInt(extruderStr, 10)

          // Find the calibration profile to get k_value and name
          const printerData = printersWithCalibrations.find(p => p.printer.serial === serial)
          const calProfile = printerData?.calibrations.find(c =>
            c.cali_idx === caliIdx && (c.extruder_id ?? null) === extruderId
          )

          if (calProfile) {
            profiles.push({
              printer_serial: serial,
              extruder: extruderId,
              nozzle_diameter: calProfile.nozzle_diameter,
              nozzle_type: null,
              k_value: calProfile.k_value.toString(),
              name: calProfile.name || calProfile.filament_id,
              cali_idx: caliIdx,
              setting_id: null,
            })
          }
        })

        if (profiles.length > 0) {
          await api.saveSpoolKProfiles(savedSpool.id, profiles)
        }
      } else {
        // Clear K-profiles if none selected
        await api.saveSpoolKProfiles(savedSpool.id, [])
      }

      updateToast(toastId, 'success', 'Spool saved successfully')
      onClose()
    } catch (e) {
      const errorMsg = e instanceof Error ? e.message : 'Failed to save spool'
      setError(errorMsg)
      updateToast(toastId, 'error', errorMsg)
    } finally {
      setIsSubmitting(false)
    }
  }

  const handleRemoveTag = async () => {
    if (!editSpool) return

    setIsRemovingTag(true)
    const toastId = showToast('loading', 'Removing tag...')

    try {
      await api.updateSpool(editSpool.id, {
        ...editSpool,
        tag_id: null,
        tag_type: null,
      } as SpoolInput)

      updateToast(toastId, 'success', 'Tag removed from spool')
      setShowRemoveTagConfirm(false)
      onTagRemoved?.()  // Notify parent to refresh
      onClose()
    } catch (e) {
      const errorMsg = e instanceof Error ? e.message : 'Failed to remove tag'
      updateToast(toastId, 'error', errorMsg)
    } finally {
      setIsRemovingTag(false)
    }
  }

  const selectColor = (hex: string, name: string) => {
    updateField('rgba', `#${hex}`)
    updateField('color_name', name)
  }

  const togglePrinterExpanded = (serial: string) => {
    setExpandedPrinters(prev => {
      const next = new Set(prev)
      if (next.has(serial)) next.delete(serial)
      else next.add(serial)
      return next
    })
  }

  const toggleProfileSelected = (serial: string, caliIdx: number, extruderId?: number | null) => {
    // Use consistent key format - null/undefined both become 'null'
    const key = `${serial}:${caliIdx}:${extruderId ?? 'null'}`
    // Printer/nozzle combination key (only one profile allowed per printer/nozzle)
    const printerNozzleKey = `${serial}:${extruderId ?? 'null'}`

    setSelectedProfiles(prev => {
      const next = new Set(prev)
      if (next.has(key)) {
        // Deselect if already selected
        next.delete(key)
      } else {
        // Remove any existing profile for this printer/nozzle combination first
        for (const existingKey of next) {
          const parts = existingKey.split(':')
          const existingPrinterNozzle = `${parts[0]}:${parts[2]}`
          if (existingPrinterNozzle === printerNozzleKey) {
            next.delete(existingKey)
          }
        }
        // Then add the new selection
        next.add(key)
      }
      return next
    })
  }

  // Get configured printer models for filtering
  const configuredPrinterModels = useMemo(() => {
    const models = new Set<string>()
    printersWithCalibrations.forEach(({ printer }) => {
      if (printer.model) {
        // Normalize model names (e.g., "X1 Carbon" -> "X1C", "P1S" -> "P1S")
        const model = printer.model.toUpperCase()
        models.add(model)
        // Also add common variants
        if (model.includes('X1')) models.add('X1C')
        if (model.includes('P1S')) models.add('P1S')
        if (model.includes('P1P')) models.add('P1P')
        if (model.includes('A1')) models.add('A1')
        if (model.includes('A1 MINI')) models.add('A1 MINI')
      }
    })
    return models
  }, [printersWithCalibrations])

  // Get filament options for dropdown - use cloud presets if available, else defaults
  const filamentOptions = useMemo(() => {
    // If we have cloud presets, filter and deduplicate them
    if (cloudPresets.length > 0) {
      const customPresets: { code: string; name: string; displayName: string; isCustom: boolean; allCodes: string[] }[] = []
      const defaultPresetsMap = new Map<string, { code: string; name: string; displayName: string; isCustom: boolean; allCodes: string[] }>()

      for (const preset of cloudPresets) {
        if (preset.is_custom) {
          // Custom presets: include if matches configured printers or no printer filter
          const presetNameUpper = preset.name.toUpperCase()
          const matchesPrinter = configuredPrinterModels.size === 0 ||
            Array.from(configuredPrinterModels).some(model => presetNameUpper.includes(model)) ||
            !presetNameUpper.includes('@')  // Include presets without printer suffix

          if (matchesPrinter) {
            customPresets.push({
              code: preset.setting_id,
              name: preset.name,
              displayName: `${preset.name} (Custom)`,
              isCustom: true,
              allCodes: [preset.setting_id],
            })
          }
        } else {
          // Default presets: deduplicate by base name (without @printer suffix)
          // Extract base name: "Bambu PLA Basic @BBL X1C" -> "Bambu PLA Basic"
          const baseName = preset.name.replace(/@.*$/, '').trim()

          // Collect all setting_ids for this base name (for K-profile matching)
          const existing = defaultPresetsMap.get(baseName)
          if (existing) {
            existing.allCodes.push(preset.setting_id)
          } else {
            defaultPresetsMap.set(baseName, {
              code: preset.setting_id,
              name: baseName,
              displayName: baseName,
              isCustom: false,
              allCodes: [preset.setting_id],
            })
          }
        }
      }

      // Combine and sort all presets alphabetically (A-Z)
      const result = [
        ...customPresets,
        ...Array.from(defaultPresetsMap.values()),
      ].sort((a, b) => a.displayName.localeCompare(b.displayName))

      return result
    }

    // Fallback to hardcoded defaults
    return getFilamentOptions().map(o => ({ ...o, displayName: o.name, isCustom: false, allCodes: [o.code] }))
  }, [cloudPresets, configuredPrinterModels])

  // Dynamic brands list - extract from cloud presets
  const availableBrands = useMemo(() => {
    return extractBrandsFromPresets(cloudPresets)
  }, [cloudPresets])

  // Find the selected preset option (for display and K-profile matching)
  // Check both primary code and allCodes array for matches
  const selectedPresetOption = useMemo(() => {
    if (!formData.slicer_filament) return undefined
    // First try exact match on primary code
    let option = filamentOptions.find(o => o.code === formData.slicer_filament)
    if (!option) {
      // Try matching against any code in allCodes
      option = filamentOptions.find(o => o.allCodes.includes(formData.slicer_filament))
    }
    if (!option) {
      // Try case-insensitive match
      const slicerLower = formData.slicer_filament.toLowerCase()
      option = filamentOptions.find(o =>
        o.code.toLowerCase() === slicerLower ||
        o.allCodes.some(c => c.toLowerCase() === slicerLower)
      )
    }
    return option
  }, [filamentOptions, formData.slicer_filament])

  // Sync presetInputValue with selected preset's display name
  useEffect(() => {
    if (selectedPresetOption) {
      setPresetInputValue(selectedPresetOption.displayName)
    } else if (formData.slicer_filament) {
      // If no matching option found, show the raw code
      setPresetInputValue(formData.slicer_filament)
    }
  }, [selectedPresetOption, formData.slicer_filament])

  // Color list based on toggle
  const colorList = showAllColors ? COLOR_PRESETS : QUICK_COLORS

  // Check if a calibration matches based on brand, material, and variant
  const isMatchingCalibration = (cal: CalibrationProfile) => {
    // Need at least material to match
    if (!formData.material) return false

    // Parse the calibration profile name to extract brand/material/variant
    // K-profile names are like "High Flow_Devil Design PLA Basic" or "HF Devil Design PLA Basic"
    const profileName = cal.name || ''

    // Remove flow type prefixes
    let cleanName = profileName
      .replace(/^High Flow[_\s]+/i, '')
      .replace(/^Standard[_\s]+/i, '')
      .replace(/^HF[_\s]+/i, '')
      .replace(/^S[_\s]+/i, '')
      .trim()

    const parsed = parsePresetName(cleanName)

    // Match material (required)
    const materialMatch = parsed.material.toUpperCase() === formData.material.toUpperCase()
    if (!materialMatch) return false

    // Match brand if specified in form
    if (formData.brand) {
      const brandMatch = parsed.brand.toLowerCase().includes(formData.brand.toLowerCase()) ||
                         formData.brand.toLowerCase().includes(parsed.brand.toLowerCase())
      if (!brandMatch) return false
    }

    // Match variant/subtype if specified in form
    if (formData.subtype) {
      const variantMatch = parsed.variant.toLowerCase().includes(formData.subtype.toLowerCase()) ||
                           formData.subtype.toLowerCase().includes(parsed.variant.toLowerCase()) ||
                           cleanName.toLowerCase().includes(formData.subtype.toLowerCase())
      if (!variantMatch) return false
    }

    return true
  }

  return (
    <Modal
      isOpen={isOpen}
      onClose={onClose}
      title={isEditing ? 'Edit Spool' : 'Add New Spool'}
      size="lg"
      footer={
        <div class="flex items-center justify-between w-full gap-3">
          <div class="flex items-center gap-2">
            {isEditing && onDelete && editSpool && (
              <button
                class="btn btn-danger-outline"
                onClick={() => {
                  onDelete(editSpool)
                  onClose()
                }}
                disabled={isSubmitting}
              >
                <Trash2 class="w-4 h-4" />
                Delete
              </button>
            )}
            {isEditing && editSpool?.tag_id && (
              <button
                class="btn btn-secondary"
                onClick={() => setShowRemoveTagConfirm(true)}
                disabled={isSubmitting}
                title="Remove NFC tag from this spool"
              >
                <Unlink class="w-4 h-4" />
                Remove Tag
              </button>
            )}
            {isEditing && editSpool && !editSpool.archived_at && onArchive && (
              <button
                class="btn btn-secondary"
                onClick={() => {
                  onArchive(editSpool)
                  onClose()
                }}
                disabled={isSubmitting}
                title="Archive this spool"
              >
                <Archive class="w-4 h-4" />
                Archive
              </button>
            )}
            {isEditing && editSpool && editSpool.archived_at && onRestore && (
              <button
                class="btn btn-secondary"
                onClick={() => {
                  onRestore(editSpool)
                  onClose()
                }}
                disabled={isSubmitting}
                title="Restore this spool"
              >
                <ArchiveRestore class="w-4 h-4" />
                Restore
              </button>
            )}
          </div>
          <div class="flex items-center gap-2">
            <button class="btn btn-ghost" onClick={onClose} disabled={isSubmitting}>
              Cancel
            </button>
            <button class="btn btn-primary" onClick={handleSubmit} disabled={isSubmitting}>
              {isSubmitting ? 'Saving...' : isEditing ? 'Save' : 'Add Spool'}
            </button>
          </div>
        </div>
      }
    >
      {error && (
        <div class="mb-4 p-3 bg-[var(--error-color)]/10 border border-[var(--error-color)]/30 rounded-lg text-[var(--error-color)] text-sm flex items-center justify-between">
          <span>{error}</span>
          <button onClick={() => setError(null)}><X class="w-4 h-4" /></button>
        </div>
      )}

      {/* Tabs */}
      <div class="tabs mb-4">
        <button
          class={`tab ${activeTab === 'filament' ? 'active' : ''}`}
          onClick={() => setActiveTab('filament')}
        >
          Filament Info
        </button>
        <button
          class={`tab ${activeTab === 'pa_profile' ? 'active' : ''}`}
          onClick={() => setActiveTab('pa_profile')}
        >
          PA Profile (K)
        </button>
      </div>

      {/* Tab Content */}
      {activeTab === 'filament' ? (
        <div class="space-y-6">
          {/* === FILAMENT SECTION === */}
          <div class="space-y-4">
            <h3 class="section-header">Filament</h3>

            {/* Slicer Preset - REQUIRED */}
            <div>
              <div class="flex items-center justify-between mb-1">
                <label class="label mb-0">Slicer Preset *</label>
                <span class={`flex items-center gap-1 text-xs ${cloudAuthenticated ? 'text-green-500' : 'text-[var(--text-muted)]'}`}>
                  {loadingCloudPresets ? (
                    <>Loading...</>
                  ) : cloudAuthenticated ? (
                    <><Cloud class="w-3 h-3" /> Cloud</>
                  ) : (
                    <><CloudOff class="w-3 h-3" /> Local</>
                  )}
                </span>
              </div>
              <input
                type="text"
                list="slicer-presets"
                class="input"
                placeholder={loadingCloudPresets ? "Loading presets..." : "Type to search presets..."}
                value={presetInputValue}
                disabled={loadingCloudPresets}
                onInput={(e) => {
                  const inputValue = (e.target as HTMLInputElement).value
                  setPresetInputValue(inputValue)

                  // Look up option by displayName first, then by code
                  let option = filamentOptions.find(o => o.displayName === inputValue)
                  if (!option) {
                    // Try matching by code (in case datalist returned the code)
                    option = filamentOptions.find(o => o.code === inputValue)
                  }
                  if (!option) {
                    // Try case-insensitive displayName match
                    const inputLower = inputValue.toLowerCase()
                    option = filamentOptions.find(o => o.displayName.toLowerCase() === inputLower)
                  }

                  if (option) {
                    updateField('slicer_filament', option.code)
                    // Auto-fill material/brand/variant from preset using parser
                    const parsed = parsePresetName(option.name)
                    if (parsed.brand) updateField('brand', parsed.brand)
                    if (parsed.material) updateField('material', parsed.material)
                    // Only set variant if a known one was detected (otherwise leave empty)
                    if (parsed.variant) {
                      updateField('subtype', parsed.variant)
                    } else {
                      // Clear subtype if no known variant found
                      updateField('subtype', '')
                    }
                  } else {
                    // User is typing - store the raw input as the code for now
                    updateField('slicer_filament', inputValue)
                  }
                }}
              />
              <datalist id="slicer-presets">
                {filamentOptions.map(({ code, displayName }) => (
                  <option key={code} value={displayName}>{displayName}</option>
                ))}
              </datalist>
              {selectedPresetOption && (
                <p class="text-xs text-[var(--accent-color)] mt-1">
                  Selected: {selectedPresetOption.displayName}
                </p>
              )}
              {!cloudAuthenticated && !loadingCloudPresets && !selectedPresetOption && (
                <p class="text-xs text-[var(--text-muted)] mt-1">
                  Login to Bambu Cloud in Settings for custom presets
                </p>
              )}
            </div>

            {/* Material + Subtype */}
            <div class="grid grid-cols-2 gap-3">
              <div>
                <label class="label">Material</label>
                <select
                  class="select"
                  value={formData.material}
                  onChange={(e) => updateField('material', (e.target as HTMLSelectElement).value)}
                >
                  <option value="">Select...</option>
                  {MATERIALS.map(m => (
                    <option key={m} value={m}>{m}</option>
                  ))}
                </select>
              </div>
              <div>
                <label class="label">Variant</label>
                <input
                  type="text"
                  class="input"
                  placeholder="e.g., Silk, Matte, HF"
                  value={formData.subtype}
                  onInput={(e) => updateField('subtype', (e.target as HTMLInputElement).value)}
                />
              </div>
            </div>

            {/* Brand + Weight */}
            <div class="grid grid-cols-2 gap-3">
              <div>
                <label class="label">Brand</label>
                <select
                  class="select"
                  value={formData.brand}
                  onChange={(e) => updateField('brand', (e.target as HTMLSelectElement).value)}
                >
                  <option value="">Select...</option>
                  {availableBrands.map(brand => (
                    <option key={brand} value={brand}>{brand}</option>
                  ))}
                </select>
              </div>
              <div>
                <label class="label">Spool Weight</label>
                <select
                  class="select"
                  value={formData.label_weight}
                  onChange={(e) => updateField('label_weight', parseInt((e.target as HTMLSelectElement).value))}
                >
                  {WEIGHTS.map(w => (
                    <option key={w} value={w}>{w >= 1000 ? `${w/1000}kg` : `${w}g`}</option>
                  ))}
                </select>
              </div>
            </div>
          </div>

          {/* === COLOR SECTION === */}
          <div class="space-y-4">
            <h3 class="section-header">Color</h3>

            {/* Color Preview Banner */}
            <div
              class="h-12 rounded-lg flex items-center justify-between px-4 border border-[var(--border-color)]"
              style={{ backgroundColor: formData.rgba }}
            >
              <span
                class="text-sm font-medium px-2 py-0.5 rounded"
                style={{ backgroundColor: 'rgba(255,255,255,0.9)', color: '#333' }}
              >
                {formData.color_name || 'Select a color'}
              </span>
              <span
                class="font-mono text-xs px-2 py-0.5 rounded"
                style={{ backgroundColor: 'rgba(0,0,0,0.5)', color: '#fff' }}
              >
                #{formData.rgba.replace('#', '').toUpperCase()}
              </span>
            </div>

            {/* Color Swatches */}
            <div class="flex flex-wrap gap-2">
              {colorList.map(color => (
                <button
                  key={color.name}
                  type="button"
                  onClick={() => selectColor(color.hex, color.name)}
                  class={`w-7 h-7 rounded-md border-2 transition-transform hover:scale-110 ${
                    formData.rgba.replace('#', '').toUpperCase() === color.hex.toUpperCase()
                      ? 'border-[var(--accent-color)] ring-2 ring-[var(--accent-color)]/30'
                      : 'border-[var(--border-color)]'
                  }`}
                  style={{ backgroundColor: `#${color.hex}` }}
                  title={color.name}
                />
              ))}
              <button
                type="button"
                onClick={() => setShowAllColors(!showAllColors)}
                class="w-7 h-7 rounded-md border-2 border-dashed border-[var(--border-color)] text-[var(--text-muted)] text-xs flex items-center justify-center hover:border-[var(--text-secondary)]"
                title={showAllColors ? 'Show less' : 'Show more colors'}
              >
                {showAllColors ? '−' : '+'}
              </button>
            </div>

            {/* Color Name + Hex */}
            <div class="grid grid-cols-2 gap-3">
              <div>
                <label class="label">Color Name</label>
                <input
                  type="text"
                  list="color-name-presets"
                  class="input"
                  placeholder="Type or select..."
                  value={formData.color_name}
                  onInput={(e) => {
                    const name = (e.target as HTMLInputElement).value
                    updateField('color_name', name)
                    const preset = COLOR_PRESETS.find(c => c.name === name)
                    if (preset) updateField('rgba', `#${preset.hex}`)
                  }}
                />
                <datalist id="color-name-presets">
                  {COLOR_PRESETS.map(color => (
                    <option key={color.name} value={color.name} />
                  ))}
                </datalist>
              </div>
              <div>
                <label class="label">Hex Color</label>
                <div class="flex gap-2">
                  <input
                    type="text"
                    class="input flex-1 font-mono"
                    placeholder="RRGGBB"
                    value={formData.rgba.replace('#', '')}
                    onInput={(e) => {
                      let val = (e.target as HTMLInputElement).value.replace('#', '').replace(/[^0-9A-Fa-f]/g, '')
                      if (val.length <= 8) updateField('rgba', `#${val}`)
                    }}
                  />
                  <input
                    type="color"
                    class="w-10 h-[38px] rounded cursor-pointer border border-[var(--border-color)] shrink-0"
                    value={formData.rgba.substring(0, 7)}
                    onInput={(e) => updateField('rgba', (e.target as HTMLInputElement).value)}
                  />
                </div>
              </div>
            </div>
          </div>

          {/* === ADDITIONAL SECTION === */}
          <div class="space-y-4">
            <h3 class="section-header">Additional</h3>

            {/* Location */}
            <div>
              <label class="label">Storage Location</label>
              <input
                type="text"
                class="input"
                placeholder="e.g., Shelf A, Drawer 1"
                value={formData.location}
                onInput={(e) => updateField('location', (e.target as HTMLInputElement).value)}
              />
            </div>

            {/* Empty Spool Weight - searchable from catalog */}
            <SpoolWeightPicker
              catalog={spoolCatalog}
              value={formData.core_weight}
              onChange={(weight) => updateField('core_weight', weight)}
            />

            {/* Note */}
            <div>
              <label class="label">Notes</label>
              <textarea
                class="input min-h-[60px] resize-none"
                placeholder="Any additional notes about this spool..."
                value={formData.note}
                onInput={(e) => updateField('note', (e.target as HTMLTextAreaElement).value)}
              />
            </div>
          </div>
        </div>
      ) : (
        /* PA Profile Tab */
        <div class="space-y-4">
          {!formData.material ? (
            <div class="p-4 bg-[var(--bg-tertiary)] rounded-lg text-center text-[var(--text-secondary)]">
              <p>Please select a material first in the Filament Info tab.</p>
            </div>
          ) : printersWithCalibrations.length === 0 ? (
            <div class="p-4 bg-[var(--bg-tertiary)] rounded-lg text-center text-[var(--text-secondary)]">
              <p>No printers configured. Add printers in the Printers page.</p>
            </div>
          ) : (
            <div class="space-y-3">
              {/* Show hint about matching criteria */}
              <p class="text-xs text-[var(--text-muted)]">
                Showing K-profiles matching: {formData.brand || 'Any brand'} / {formData.material} / {formData.subtype || 'Any variant'}
              </p>

              {printersWithCalibrations.map(({ printer, calibrations }) => {
                const isExpanded = expandedPrinters.has(printer.serial)
                // Filter to only matching calibrations
                const matchingCals = calibrations.filter(cal => isMatchingCalibration(cal))
                const matchingCount = matchingCals.length

                // Check if multi-nozzle printer (from matching profiles)
                const isMultiNozzle = matchingCals.some(cal => cal.extruder_id !== undefined && cal.extruder_id !== null && cal.extruder_id > 0)

                // Group matching profiles by extruder for display
                const leftNozzleCals = matchingCals.filter(cal => cal.extruder_id === 1)
                const rightNozzleCals = matchingCals.filter(cal => cal.extruder_id === 0 || cal.extruder_id === undefined || cal.extruder_id === null)

                const renderProfile = (cal: CalibrationProfile) => {
                  const key = `${printer.serial}:${cal.cali_idx}:${cal.extruder_id}`
                  const isSelected = selectedProfiles.has(key)
                  return (
                    <label
                      key={`${cal.cali_idx}-${cal.extruder_id}`}
                      class="flex items-center gap-3 p-2 rounded cursor-pointer transition-colors bg-[var(--accent-color)]/10 hover:bg-[var(--accent-color)]/20"
                    >
                      <input
                        type="checkbox"
                        checked={isSelected}
                        onChange={() => toggleProfileSelected(printer.serial, cal.cali_idx, cal.extruder_id)}
                        class="w-4 h-4 rounded border-[var(--border-color)] text-[var(--accent-color)]"
                      />
                      <span class="flex-1 text-sm font-medium text-[var(--accent-color)]">
                        {cal.name || cal.filament_id}
                      </span>
                      <span class="text-xs font-mono text-[var(--text-muted)]">
                        K={cal.k_value.toFixed(3)}
                      </span>
                    </label>
                  )
                }

                return (
                  <div key={printer.serial} class="border border-[var(--border-color)] rounded-lg overflow-hidden">
                    {/* Printer Header */}
                    <button
                      type="button"
                      onClick={() => togglePrinterExpanded(printer.serial)}
                      class="w-full px-4 py-3 flex items-center justify-between bg-[var(--bg-tertiary)] hover:bg-[var(--bg-secondary)] transition-colors"
                    >
                      <div class="flex items-center gap-3">
                        {isExpanded ? (
                          <ChevronDown class="w-4 h-4 text-[var(--text-muted)]" />
                        ) : (
                          <ChevronRight class="w-4 h-4 text-[var(--text-muted)]" />
                        )}
                        <span class="font-medium text-[var(--text-primary)]">
                          {printer.name || printer.serial}
                        </span>
                        {matchingCount > 0 ? (
                          <span class="text-xs px-2 py-0.5 rounded bg-[var(--accent-color)]/20 text-[var(--accent-color)]">
                            {matchingCount} profile{matchingCount !== 1 ? 's' : ''}
                          </span>
                        ) : (
                          <span class="text-xs px-2 py-0.5 rounded bg-[var(--bg-secondary)] text-[var(--text-muted)]">
                            No matches
                          </span>
                        )}
                      </div>
                      <span class={`text-xs px-2 py-1 rounded ${
                        printer.connected
                          ? 'bg-green-500/20 text-green-500'
                          : 'bg-[var(--text-muted)]/20 text-[var(--text-muted)]'
                      }`}>
                        {printer.connected ? 'Connected' : 'Offline'}
                      </span>
                    </button>

                    {/* Calibration Profiles */}
                    {isExpanded && (
                      <div class="px-4 py-3 space-y-3 bg-[var(--bg-primary)]">
                        {!printer.connected ? (
                          <p class="text-sm text-[var(--text-muted)] italic">
                            Printer is offline. Connect to view calibration profiles.
                          </p>
                        ) : matchingCount === 0 ? (
                          <p class="text-sm text-[var(--text-muted)] italic">
                            No K-profiles match {formData.brand ? `${formData.brand} ` : ''}{formData.material}{formData.subtype ? ` ${formData.subtype}` : ''}
                          </p>
                        ) : isMultiNozzle ? (
                          /* Multi-nozzle: group by extruder */
                          <>
                            {leftNozzleCals.length > 0 && (
                              <div class="space-y-2">
                                <p class="text-xs font-medium text-[var(--text-secondary)]">
                                  Left Nozzle ({leftNozzleCals.length})
                                </p>
                                {leftNozzleCals.map(renderProfile)}
                              </div>
                            )}
                            {rightNozzleCals.length > 0 && (
                              <div class="space-y-2">
                                <p class="text-xs font-medium text-[var(--text-secondary)]">
                                  Right Nozzle ({rightNozzleCals.length})
                                </p>
                                {rightNozzleCals.map(renderProfile)}
                              </div>
                            )}
                          </>
                        ) : (
                          /* Single nozzle: show matching profiles */
                          <div class="space-y-2">
                            {matchingCals.map(renderProfile)}
                          </div>
                        )}
                      </div>
                    )}
                  </div>
                )
              })}

              {/* Summary */}
              {selectedProfiles.size > 0 && (
                <div class="p-3 bg-[var(--accent-color)]/10 border border-[var(--accent-color)]/30 rounded-lg">
                  <p class="text-sm text-[var(--text-primary)]">
                    <span class="font-medium">{selectedProfiles.size}</span> calibration profile(s) selected
                  </p>
                </div>
              )}
            </div>
          )}
        </div>
      )}

      {/* Remove Tag Confirmation Modal */}
      {showRemoveTagConfirm && (
        <div class="fixed inset-0 z-[60] flex items-center justify-center">
          <div class="absolute inset-0 bg-black/50" onClick={() => setShowRemoveTagConfirm(false)} />
          <div class="relative bg-[var(--bg-primary)] rounded-lg border border-[var(--border-color)] p-6 max-w-md shadow-xl">
            <h3 class="text-lg font-semibold text-[var(--text-primary)] mb-2">
              Remove NFC Tag?
            </h3>
            <p class="text-sm text-[var(--text-secondary)] mb-4">
              This will unlink the NFC tag from this spool. The tag can then be assigned to a different spool.
            </p>
            {editSpool?.tag_id && (
              <p class="text-xs text-[var(--text-muted)] font-mono mb-4 p-2 bg-[var(--bg-tertiary)] rounded">
                Tag ID: {editSpool.tag_id}
              </p>
            )}
            <div class="flex justify-end gap-2">
              <button
                class="btn"
                onClick={() => setShowRemoveTagConfirm(false)}
                disabled={isRemovingTag}
              >
                Cancel
              </button>
              <button
                class="btn btn-danger"
                onClick={handleRemoveTag}
                disabled={isRemovingTag}
              >
                {isRemovingTag ? 'Removing...' : 'Remove Tag'}
              </button>
            </div>
          </div>
        </div>
      )}
    </Modal>
  )
}
