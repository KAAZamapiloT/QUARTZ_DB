<template>
  <div class="hex-inspector-card diagram-full">
    <!-- Component Header -->
    <div class="inspector-header">
      <div class="header-titles">
        <span class="inspector-tag">INTERACTIVE MEMORY INSPECTOR</span>
        <h3 class="inspector-title">4KB Slotted-Page Raw Hex & Structure Inspector</h3>
        <p class="inspector-sub">
          Inspect byte-by-byte alignment of page headers, slot directories, unallocated space, and variable-length record payloads.
        </p>
      </div>

      <!-- Quick Jump & View Mode Controls -->
      <div class="view-controls">
        <span class="ctrl-label">Jump to Region:</span>
        <button 
          v-for="r in jumpRegions" 
          :key="r.id"
          :class="['region-chip', { active: activeHoverRegion === r.id }]"
          @mouseenter="hoverRegion(r.id)"
          @mouseleave="clearHover"
          @click="scrollToRegion(r.offset)"
        >
          {{ r.label }}
        </button>
      </div>
    </div>

    <!-- Interactive Simulation Banner -->
    <div class="sim-actions-bar">
      <div class="sim-left">
        <span class="sim-label">Live Page Operations:</span>
        <button class="sim-btn insert" :disabled="hasSlot2" @click="simulateInsert">
          + Simulate INSERT (Slot 2)
        </button>
        <button class="sim-btn delete" :disabled="!slot1Active" @click="simulateDelete">
          ✕ Simulate DELETE (Slot 1)
        </button>
        <button class="sim-btn compact" :disabled="fragmentedBytes === 0" @click="simulateCompact">
          ⚡ Simulate COMPACT PAGE
        </button>
        <button class="sim-btn reset" @click="resetPage">
          ↺ Reset
        </button>
      </div>
      <div class="sim-right">
        <span class="status-indicator">
          <span class="status-dot"></span>
          {{ statusMsg }}
        </span>
      </div>
    </div>

    <!-- Dual-Pane Inspector Body -->
    <div class="inspector-panes">
      <!-- LEFT PANE: Hex Dump Viewer -->
      <div class="pane hex-dump-pane">
        <div class="pane-header">
          <div class="pane-title">
            <span class="pane-icon">💾</span>
            Physical Page Memory (4,096 Bytes · Offset 0x0000 - 0x0FFF)
          </div>
          <div class="view-toggle">
            <button 
              :class="['toggle-tab', { active: !fullDumpMode }]" 
              @click="fullDumpMode = false"
            >
              Key Regions
            </button>
            <button 
              :class="['toggle-tab', { active: fullDumpMode }]" 
              @click="fullDumpMode = true"
            >
              Full 4KB
            </button>
          </div>
        </div>

        <!-- Hex Column Rulers -->
        <div class="hex-ruler-row">
          <span class="ruler-offset">OFFSET</span>
          <span class="ruler-hex">00 01 02 03  04 05 06 07  08 09 0A 0B  0C 0D 0E 0F</span>
          <span class="ruler-ascii">ASCII DECODE</span>
        </div>

        <!-- Hex Dump Scrollable Viewport -->
        <div class="hex-viewport" ref="hexScrollContainer">
          <div 
            v-for="(row, rIdx) in renderedHexRows" 
            :key="row.offset"
            :id="'hex-row-' + row.offset"
            :class="['hex-row', { 'gap-break-row': row.isGapBreak }]"
          >
            <!-- Gap Break representation -->
            <template v-if="row.isGapBreak">
              <div class="gap-break-banner">
                <span class="gap-icon">⋯</span>
                <span class="gap-text">
                  [ 0x{{ row.startHex }} .. 0x{{ row.endHex }} ] — {{ row.gapBytes.toLocaleString() }} unallocated contiguous free bytes (0x00)
                </span>
                <button class="gap-expand-btn" @click="fullDumpMode = true">Expand View</button>
              </div>
            </template>

            <!-- Standard 16-Byte Hex Line -->
            <template v-else>
              <span class="row-offset">{{ formatHex(row.offset, 4) }}:</span>
              
              <!-- 16 Byte Cells -->
              <div class="row-bytes">
                <span 
                  v-for="(b, bIdx) in row.bytes" 
                  :key="bIdx"
                  :class="[
                    'hex-byte', 
                    getByteHighlightClass(row.offset + bIdx),
                    { 'quad-space': bIdx === 3 || bIdx === 7 || bIdx === 11 }
                  ]"
                  @mouseenter="onByteHover(row.offset + bIdx)"
                  @mouseleave="clearHover"
                  :title="getByteTooltip(row.offset + bIdx)"
                >
                  {{ formatHex(b, 2) }}
                </span>
              </div>

              <!-- ASCII Representation -->
              <span class="row-ascii">
                <span 
                  v-for="(b, bIdx) in row.bytes" 
                  :key="bIdx"
                  :class="['ascii-char', getByteHighlightClass(row.offset + bIdx)]"
                  @mouseenter="onByteHover(row.offset + bIdx)"
                  @mouseleave="clearHover"
                >
                  {{ formatAscii(b) }}
                </span>
              </span>
            </template>
          </div>
        </div>

        <!-- Hex Legend Bar -->
        <div class="hex-legend">
          <span class="legend-item"><span class="legend-box hdr"></span> Header (24B)</span>
          <span class="legend-item"><span class="legend-box slot0"></span> Slot 0 (4B)</span>
          <span class="legend-item"><span class="legend-box slot1"></span> Slot 1 (4B)</span>
          <span class="legend-item" v-if="hasSlot2"><span class="legend-box slot2"></span> Slot 2 (4B)</span>
          <span class="legend-item"><span class="legend-box free"></span> Free Space</span>
          <span class="legend-item"><span class="legend-box rec0"></span> Record 0</span>
          <span class="legend-item"><span class="legend-box rec1"></span> Record 1</span>
        </div>
      </div>

      <!-- RIGHT PANE: Decoded Slotted Page Inspector -->
      <div class="pane decoded-pane">
        <div class="pane-header">
          <div class="pane-title">
            <span class="pane-icon">🔍</span>
            Decoded Slotted-Page Structure
          </div>
          <div class="page-meta-badge">PageID: 1 · Fixed 4KB</div>
        </div>

        <div class="decoded-body">
          <!-- 1. Page Header Card -->
          <div 
            class="struct-card header-card"
            :class="{ active: activeHoverRegion === 'header' }"
            @mouseenter="hoverRegion('header')"
            @mouseleave="clearHover"
          >
            <div class="card-title-bar">
              <span class="title-text">
                <span class="color-pill hdr"></span>
                Page Header
              </span>
              <span class="byte-badge">Offset 0x0000 - 0x0017 (24 Bytes)</span>
            </div>

            <div class="fields-grid">
              <div class="field-item" @mouseenter.stop="hoverSubRegion('magic')" @mouseleave.stop="hoverRegion('header')">
                <span class="f-name">magic_number</span>
                <span class="f-val">0x31475051 ("QPG1")</span>
                <span class="f-sub">Offset 0x0000..0x0003 (4B)</span>
              </div>
              <div class="field-item" @mouseenter.stop="hoverSubRegion('slot_count')" @mouseleave.stop="hoverRegion('header')">
                <span class="f-name">slot_count</span>
                <span class="f-val highlight">{{ slotCount }}</span>
                <span class="f-sub">Offset 0x0004..0x0005 (2B)</span>
              </div>
              <div class="field-item" @mouseenter.stop="hoverSubRegion('free_start')" @mouseleave.stop="hoverRegion('header')">
                <span class="f-name">free_start</span>
                <span class="f-val">{{ freeStart }} (0x{{ formatHex(freeStart, 4) }})</span>
                <span class="f-sub">Grows Downward &darr;</span>
              </div>
              <div class="field-item" @mouseenter.stop="hoverSubRegion('free_end')" @mouseleave.stop="hoverRegion('header')">
                <span class="f-name">free_end</span>
                <span class="f-val">{{ freeEnd }} (0x{{ formatHex(freeEnd, 4) }})</span>
                <span class="f-sub">Grows Upward &uarr;</span>
              </div>
              <div class="field-item" @mouseenter.stop="hoverSubRegion('page_version')" @mouseleave.stop="hoverRegion('header')">
                <span class="f-name">page_version</span>
                <span class="f-val">1 (v1.0 Layout)</span>
                <span class="f-sub">Offset 0x000A..0x000B (2B)</span>
              </div>
              <div class="field-item" @mouseenter.stop="hoverSubRegion('next_page')" @mouseleave.stop="hoverRegion('header')">
                <span class="f-name">next_page</span>
                <span class="f-val">NO_PAGE (0xFFFFFFFFFFFFFFFF)</span>
                <span class="f-sub">Offset 0x000C..0x0013 (8B)</span>
              </div>
              <div class="field-item" @mouseenter.stop="hoverSubRegion('fragmented')" @mouseleave.stop="hoverRegion('header')">
                <span class="f-name">fragmented_bytes</span>
                <span class="f-val" :class="{ 'warning-txt': fragmentedBytes > 0 }">{{ fragmentedBytes }} Bytes</span>
                <span class="f-sub">Offset 0x0014..0x0015 (2B)</span>
              </div>
              <div class="field-item">
                <span class="f-name">reserved</span>
                <span class="f-val">0x0000 (Padding)</span>
                <span class="f-sub">Offset 0x0016..0x0017 (2B)</span>
              </div>
            </div>
          </div>

          <!-- 2. Slot Directory Card -->
          <div class="struct-card slots-card">
            <div class="card-title-bar">
              <span class="title-text">
                <span class="color-pill slots"></span>
                Slot Directory (Slot Array)
              </span>
              <span class="byte-badge">Offset 0x0018 &rarr; free_start</span>
            </div>

            <div class="slots-list">
              <!-- Slot 0 -->
              <div 
                class="slot-entry-item slot-0"
                :class="{ active: activeHoverRegion === 'slot0' || activeHoverRegion === 'rec0' }"
                @mouseenter="hoverRegion('slot0')"
                @mouseleave="clearHover"
              >
                <div class="slot-head">
                  <span class="slot-badge s0">Slot 0</span>
                  <span class="slot-offsets">0x0018..0x001B (4 Bytes: <code>b4 0f 4c 00</code>)</span>
                  <span class="slot-status active">ACTIVE</span>
                </div>
                <div class="slot-details">
                  <div class="slot-prop">
                    <span class="p-lbl">record_offset:</span>
                    <span class="p-val">4020 (0x0FB4)</span>
                  </div>
                  <div class="slot-prop">
                    <span class="p-lbl">record_length:</span>
                    <span class="p-val">76 Bytes</span>
                  </div>
                  <div class="slot-prop pointer-target">
                    <span class="p-lbl">Points To &rarr;</span>
                    <span class="p-val-target">Record 0 Payload [4020..4095]</span>
                  </div>
                </div>
              </div>

              <!-- Slot 1 -->
              <div 
                class="slot-entry-item slot-1"
                :class="{ active: activeHoverRegion === 'slot1' || activeHoverRegion === 'rec1', tombstone: !slot1Active }"
                @mouseenter="hoverRegion('slot1')"
                @mouseleave="clearHover"
              >
                <div class="slot-head">
                  <span class="slot-badge s1">Slot 1</span>
                  <span class="slot-offsets">0x001C..0x001F (4 Bytes)</span>
                  <span class="slot-status" :class="slot1Active ? 'active' : 'tomb'">
                    {{ slot1Active ? 'ACTIVE' : 'TOMBSTONE (LEN=0)' }}
                  </span>
                </div>
                <div class="slot-details">
                  <div class="slot-prop">
                    <span class="p-lbl">record_offset:</span>
                    <span class="p-val">{{ slot1Active ? slot1Offset + ' (0x' + formatHex(slot1Offset, 4) + ')' : '0 (Tombstone)' }}</span>
                  </div>
                  <div class="slot-prop">
                    <span class="p-lbl">record_length:</span>
                    <span class="p-val">{{ slot1Active ? '70 Bytes' : '0 Bytes (Reclaimable)' }}</span>
                  </div>
                  <div class="slot-prop pointer-target">
                    <span class="p-lbl">Points To &rarr;</span>
                    <span class="p-val-target">{{ slot1Active ? 'Record 1 Payload [' + slot1Offset + '..' + (slot1Offset + 69) + ']' : 'None (Freed)' }}</span>
                  </div>
                </div>
              </div>

              <!-- Slot 2 (Simulated) -->
              <div 
                v-if="hasSlot2"
                class="slot-entry-item slot-2"
                :class="{ active: activeHoverRegion === 'slot2' || activeHoverRegion === 'rec2' }"
                @mouseenter="hoverRegion('slot2')"
                @mouseleave="clearHover"
              >
                <div class="slot-head">
                  <span class="slot-badge s2">Slot 2</span>
                  <span class="slot-offsets">0x0020..0x0023 (4 Bytes)</span>
                  <span class="slot-status active">ACTIVE (NEW)</span>
                </div>
                <div class="slot-details">
                  <div class="slot-prop">
                    <span class="p-lbl">record_offset:</span>
                    <span class="p-val">{{ slot2Offset }} (0x{{ formatHex(slot2Offset, 4) }})</span>
                  </div>
                  <div class="slot-prop">
                    <span class="p-lbl">record_length:</span>
                    <span class="p-val">{{ slot2Length }} Bytes</span>
                  </div>
                  <div class="slot-prop pointer-target">
                    <span class="p-lbl">Points To &rarr;</span>
                    <span class="p-val-target">Record 2 Payload [{{ slot2Offset }}..{{ slot2Offset + slot2Length - 1 }}]</span>
                  </div>
                </div>
              </div>
            </div>
          </div>

          <!-- 3. Free Space Card -->
          <div 
            class="struct-card free-space-card"
            :class="{ active: activeHoverRegion === 'free' }"
            @mouseenter="hoverRegion('free')"
            @mouseleave="clearHover"
          >
            <div class="card-title-bar">
              <span class="title-text">
                <span class="color-pill free"></span>
                Contiguous Free Space
              </span>
              <span class="byte-badge">{{ freeBytes.toLocaleString() }} Bytes ({{ ((freeBytes / 4096) * 100).toFixed(1) }}% Available)</span>
            </div>
            <p class="free-desc">
              Unallocated 0x00 bytes spanning from <code>free_start</code> (0x{{ formatHex(freeStart, 4) }}) to <code>free_end</code> (0x{{ formatHex(freeEnd, 4) }}). Slots allocate downward; records allocate upward.
            </p>
          </div>

          <!-- 4. Record Payloads Card -->
          <div class="struct-card records-card">
            <div class="card-title-bar">
              <span class="title-text">
                <span class="color-pill rec"></span>
                Record Payloads (Binary Encoded)
              </span>
              <span class="byte-badge">free_end &rarr; 0x0FFF (Grows Upward &uarr;)</span>
            </div>

            <div class="records-list">
              <!-- Record 2 (If simulated) -->
              <div 
                v-if="hasSlot2"
                class="record-entry rec-2"
                :class="{ active: activeHoverRegion === 'rec2' || activeHoverRegion === 'slot2' }"
                @mouseenter="hoverRegion('rec2')"
                @mouseleave="clearHover"
              >
                <div class="rec-head">
                  <span class="rec-badge r2">Record 2 Payload</span>
                  <span class="rec-span">Offset {{ slot2Offset }}..{{ slot2Offset + slot2Length - 1 }} ({{ slot2Length }}B)</span>
                  <span class="rec-rid">RecordID {1, 2}</span>
                </div>
                <div class="rec-tuple">
                  <span class="tuple-key">id:</span> <span class="tuple-val int">3</span>,
                  <span class="tuple-key">name:</span> <span class="tuple-val str">'Claude Shannon'</span>,
                  <span class="tuple-key">role:</span> <span class="tuple-val str">'Fellow'</span>
                </div>
              </div>

              <!-- Record 1 -->
              <div 
                v-if="slot1Active"
                class="record-entry rec-1"
                :class="{ active: activeHoverRegion === 'rec1' || activeHoverRegion === 'slot1' }"
                @mouseenter="hoverRegion('rec1')"
                @mouseleave="clearHover"
              >
                <div class="rec-head">
                  <span class="rec-badge r1">Record 1 Payload</span>
                  <span class="rec-span">Offset {{ slot1Offset }}..{{ slot1Offset + 69 }} (70B)</span>
                  <span class="rec-rid">RecordID {1, 1}</span>
                </div>
                <div class="rec-tuple">
                  <span class="tuple-key">id:</span> <span class="tuple-val int">2</span>,
                  <span class="tuple-key">name:</span> <span class="tuple-val str">'Grace Hopper'</span>,
                  <span class="tuple-key">role:</span> <span class="tuple-val str">'Staff Eng'</span>
                </div>
              </div>
              <div v-else class="record-entry tombstone-entry">
                <span class="tomb-tag">DELETED / GAP</span>
                <span class="tomb-info">Offset 3950..4019 (70 Bytes reclaimable gap)</span>
              </div>

              <!-- Record 0 -->
              <div 
                class="record-entry rec-0"
                :class="{ active: activeHoverRegion === 'rec0' || activeHoverRegion === 'slot0' }"
                @mouseenter="hoverRegion('rec0')"
                @mouseleave="clearHover"
              >
                <div class="rec-head">
                  <span class="rec-badge r0">Record 0 Payload</span>
                  <span class="rec-span">Offset {{ slot0Offset }}..{{ slot0Offset + 75 }} (76B)</span>
                  <span class="rec-rid">RecordID {1, 0}</span>
                </div>
                <div class="rec-tuple">
                  <span class="tuple-key">id:</span> <span class="tuple-val int">1</span>,
                  <span class="tuple-key">name:</span> <span class="tuple-val str">'Ada Lovelace'</span>,
                  <span class="tuple-key">role:</span> <span class="tuple-val str">'Principal Arch'</span>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- Interactive Active Byte Inspector Footer Bar -->
    <div class="inspector-footer">
      <div class="footer-left">
        <span class="f-stat-tag">Active Selection:</span>
        <span class="f-stat-val">{{ activeHoverDesc }}</span>
      </div>
      <div class="footer-right">
        <span class="engine-invariant-note">
          Invariant: <code>free_start ({{ freeStart }}) &le; free_end ({{ freeEnd }})</code> &bull; Logical indirection via <code>RecordID</code> preserves index integrity.
        </span>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed } from 'vue'

// Interactive State
const fullDumpMode = ref(false)
const hasSlot2 = ref(false)
const slot1Active = ref(true)
const fragmentedBytes = ref(0)
const statusMsg = ref('Baseline Page: 2 active records. Hover over Slot 0 to see indirection.')
const activeHoverRegion = ref(null)
const activeSubRegion = ref(null)
const hoveredByteOffset = ref(null)

const hexScrollContainer = ref(null)

// Page geometry calculations
const slot0Offset = ref(4020) // 76B (4020..4095)
const slot1Offset = ref(3950) // 70B (3950..4019)
const slot2Offset = ref(3882) // 68B (3882..3949)
const slot2Length = ref(68)

const slotCount = computed(() => {
  let c = 2
  if (hasSlot2.value) c = 3
  return c
})

const freeStart = computed(() => {
  return 24 + slotCount.value * 4
})

const freeEnd = computed(() => {
  if (hasSlot2.value) {
    return slot2Offset.value
  }
  if (!slot1Active.value && fragmentedBytes.value === 0) {
    // After compaction of deleted slot 1:
    return slot0Offset.value
  }
  return slot1Offset.value
})

const freeBytes = computed(() => {
  return freeEnd.value - freeStart.value
})

const jumpRegions = [
  { id: 'header', label: 'Header (0x0000)', offset: 0 },
  { id: 'slot0', label: 'Slot 0 (0x0018)', offset: 24 },
  { id: 'free', label: 'Free Space (0x0020)', offset: 32 },
  { id: 'rec1', label: 'Record 1 (0x0F6E)', offset: 3950 },
  { id: 'rec0', label: 'Record 0 (0x0FB4)', offset: 4020 }
]

const formatHex = (num, padding = 2) => {
  if (num === undefined || num === null) return '00'
  return num.toString(16).padStart(padding, '0').toUpperCase()
}

const formatAscii = (byte) => {
  if (byte >= 32 && byte <= 126) {
    return String.fromCharCode(byte)
  }
  return '.'
}

const hoverRegion = (r) => {
  activeHoverRegion.value = r
  activeSubRegion.value = null
}

const hoverSubRegion = (sub) => {
  activeSubRegion.value = sub
}

const clearHover = () => {
  activeHoverRegion.value = null
  activeSubRegion.value = null
  hoveredByteOffset.value = null
}

const onByteHover = (offset) => {
  hoveredByteOffset.value = offset
  // Map byte to region
  if (offset < 24) {
    activeHoverRegion.value = 'header'
  } else if (offset >= 24 && offset < 28) {
    activeHoverRegion.value = 'slot0'
  } else if (offset >= 28 && offset < 32) {
    activeHoverRegion.value = 'slot1'
  } else if (hasSlot2.value && offset >= 32 && offset < 36) {
    activeHoverRegion.value = 'slot2'
  } else if (offset >= freeStart.value && offset < freeEnd.value) {
    activeHoverRegion.value = 'free'
  } else if (hasSlot2.value && offset >= slot2Offset.value && offset < slot2Offset.value + slot2Length.value) {
    activeHoverRegion.value = 'rec2'
  } else if (slot1Active.value && offset >= slot1Offset.value && offset < slot0Offset.value) {
    activeHoverRegion.value = 'rec1'
  } else if (offset >= slot0Offset.value && offset < 4096) {
    activeHoverRegion.value = 'rec0'
  }
}

const scrollToRegion = (offset) => {
  const rowOffset = Math.floor(offset / 16) * 16
  const el = document.getElementById('hex-row-' + rowOffset)
  if (el) {
    el.scrollIntoView({ behavior: 'smooth', block: 'nearest' })
  }
}

// Byte classification for coloring
const getByteHighlightClass = (byteOffset) => {
  // Check exact hovered byte
  if (hoveredByteOffset.value === byteOffset) {
    return 'byte-active-cursor'
  }

  // Header (0..23)
  if (byteOffset < 24) {
    if (activeHoverRegion.value === 'header') return 'hl-header'
    if (activeSubRegion.value === 'magic' && byteOffset < 4) return 'hl-sub-focus'
    if (activeSubRegion.value === 'slot_count' && byteOffset >= 4 && byteOffset < 6) return 'hl-sub-focus'
    if (activeSubRegion.value === 'free_start' && byteOffset >= 6 && byteOffset < 8) return 'hl-sub-focus'
    if (activeSubRegion.value === 'free_end' && byteOffset >= 8 && byteOffset < 10) return 'hl-sub-focus'
    if (activeSubRegion.value === 'page_version' && byteOffset >= 10 && byteOffset < 12) return 'hl-sub-focus'
    if (activeSubRegion.value === 'next_page' && byteOffset >= 12 && byteOffset < 20) return 'hl-sub-focus'
    if (activeSubRegion.value === 'fragmented' && byteOffset >= 20 && byteOffset < 22) return 'hl-sub-focus'
    return 'c-hdr'
  }

  // Slot 0 (24..27)
  if (byteOffset >= 24 && byteOffset < 28) {
    if (activeHoverRegion.value === 'slot0' || activeHoverRegion.value === 'rec0') return 'hl-slot0-focus'
    return 'c-slot0'
  }

  // Slot 1 (28..31)
  if (byteOffset >= 28 && byteOffset < 32) {
    if (activeHoverRegion.value === 'slot1' || activeHoverRegion.value === 'rec1') return 'hl-slot1-focus'
    return 'c-slot1'
  }

  // Slot 2 (32..35) if simulated
  if (hasSlot2.value && byteOffset >= 32 && byteOffset < 36) {
    if (activeHoverRegion.value === 'slot2' || activeHoverRegion.value === 'rec2') return 'hl-slot2-focus'
    return 'c-slot2'
  }

  // Free Space
  if (byteOffset >= freeStart.value && byteOffset < freeEnd.value) {
    if (activeHoverRegion.value === 'free') return 'hl-free-focus'
    return 'c-free'
  }

  // Record 2 (3882..3949) if present
  if (hasSlot2.value && byteOffset >= slot2Offset.value && byteOffset < slot2Offset.value + slot2Length.value) {
    if (activeHoverRegion.value === 'rec2' || activeHoverRegion.value === 'slot2') return 'hl-rec2-focus'
    return 'c-rec2'
  }

  // Record 1 (3950..4019)
  if (byteOffset >= slot1Offset.value && byteOffset < slot0Offset.value) {
    if (!slot1Active.value) {
      if (fragmentedBytes.value > 0) return 'c-fragmented'
      return 'c-free'
    }
    if (activeHoverRegion.value === 'rec1' || activeHoverRegion.value === 'slot1') return 'hl-rec1-focus'
    return 'c-rec1'
  }

  // Record 0 (4020..4095)
  if (byteOffset >= slot0Offset.value && byteOffset < 4096) {
    if (activeHoverRegion.value === 'rec0' || activeHoverRegion.value === 'slot0') return 'hl-rec0-focus'
    return 'c-rec0'
  }

  return ''
}

const getByteTooltip = (offset) => {
  if (offset < 4) return `Byte 0x${formatHex(offset, 4)}: Magic Byte ('QPG1')`
  if (offset < 6) return `Byte 0x${formatHex(offset, 4)}: Header slot_count (${slotCount.value})`
  if (offset < 8) return `Byte 0x${formatHex(offset, 4)}: Header free_start (${freeStart.value})`
  if (offset < 10) return `Byte 0x${formatHex(offset, 4)}: Header free_end (${freeEnd.value})`
  if (offset >= 24 && offset < 28) return `Byte 0x${formatHex(offset, 4)}: Slot 0 Directory (Target offset: 4020, Len: 76)`
  if (offset >= 28 && offset < 32) return `Byte 0x${formatHex(offset, 4)}: Slot 1 Directory (Target offset: ${slot1Offset.value}, Len: ${slot1Active.value ? 70 : 0})`
  if (offset >= 4020) return `Byte 0x${formatHex(offset, 4)}: Record 0 Payload (ID: 1, Ada Lovelace)`
  if (offset >= 3950 && offset < 4020) return slot1Active.value ? `Byte 0x${formatHex(offset, 4)}: Record 1 Payload (ID: 2, Grace Hopper)` : `Byte 0x${formatHex(offset, 4)}: Fragmented Gap`
  return `Byte 0x${formatHex(offset, 4)}`
}

const activeHoverDesc = computed(() => {
  if (hoveredByteOffset.value !== null) {
    return getByteTooltip(hoveredByteOffset.value)
  }
  if (activeHoverRegion.value === 'slot0' || activeHoverRegion.value === 'rec0') {
    return 'Slot 0 [0x0018..0x001B] points to Record 0 Payload at byte 4020 (0x0FB4, length 76B)'
  }
  if (activeHoverRegion.value === 'slot1' || activeHoverRegion.value === 'rec1') {
    return slot1Active.value 
      ? `Slot 1 [0x001C..0x001F] points to Record 1 Payload at byte ${slot1Offset.value} (length 70B)`
      : 'Slot 1 is marked as TOMBSTONE (length = 0). 70 bytes fragmented space pending compaction.'
  }
  if (activeHoverRegion.value === 'slot2' || activeHoverRegion.value === 'rec2') {
    return `Slot 2 [0x0020..0x0023] points to Record 2 Payload at byte ${slot2Offset.value} (length 68B)`
  }
  if (activeHoverRegion.value === 'header') {
    return 'Page Header (24B): Magic QPG1, slot_count, free_start/free_end pointers, next_page link'
  }
  if (activeHoverRegion.value === 'free') {
    return `Unallocated Contiguous Space: ${freeBytes.value} bytes available between slot array and payloads`
  }
  return 'Hover any byte, slot, or record to inspect physical memory layout and pointer references.'
})

// Build the byte representation
const generateMemoryBuffer = () => {
  const buf = new Uint8Array(4096)

  // Header 0..23
  // Magic 0x31475051 in Little-Endian: 51 50 47 31 ('Q', 'P', 'G', '1')
  buf[0] = 0x51; buf[1] = 0x50; buf[2] = 0x47; buf[3] = 0x31;
  // slot_count
  buf[4] = slotCount.value & 0xFF; buf[5] = (slotCount.value >> 8) & 0xFF;
  // free_start
  buf[6] = freeStart.value & 0xFF; buf[7] = (freeStart.value >> 8) & 0xFF;
  // free_end
  buf[8] = freeEnd.value & 0xFF; buf[9] = (freeEnd.value >> 8) & 0xFF;
  // page_version = 1
  buf[10] = 0x01; buf[11] = 0x00;
  // next_page = NO_PAGE (0xFF * 8)
  for (let i = 12; i < 20; i++) buf[i] = 0xFF;
  // fragmented_bytes
  buf[20] = fragmentedBytes.value & 0xFF; buf[21] = (fragmentedBytes.value >> 8) & 0xFF;
  // reserved
  buf[22] = 0x00; buf[23] = 0x00;

  // Slot 0 (24..27): offset 4020 (0x0FB4), len 76 (0x004C)
  buf[24] = 0xB4; buf[25] = 0x0F; buf[26] = 0x4C; buf[27] = 0x00;

  // Slot 1 (28..31): offset 3950 (0x0F6E), len 70 (0x0046) or 0 if deleted
  if (slot1Active.value) {
    buf[28] = 0x6E; buf[29] = 0x0F; buf[30] = 0x46; buf[31] = 0x00;
  } else {
    buf[28] = 0x00; buf[29] = 0x00; buf[30] = 0x00; buf[31] = 0x00;
  }

  // Slot 2 (32..35) if active: offset 3882 (0x0F2A), len 68 (0x0044)
  if (hasSlot2.value) {
    buf[32] = 0x2A; buf[33] = 0x0F; buf[34] = 0x44; buf[35] = 0x00;
  }

  // Record 0 Payload at 4020..4095 (76 Bytes)
  // Encodes: ID: 1, Name: "Ada Lovelace", Role: "Principal Arch"
  const rec0Str = "\x01\x01\x00\x00\x00\x00\x00\x00\x00\x02\x0C\x00\x00\x00Ada Lovelace\x02\x0E\x00\x00\x00Principal Arch"
  for (let i = 0; i < rec0Str.length; i++) {
    buf[slot0Offset.value + i] = rec0Str.charCodeAt(i)
  }
  for (let i = slot0Offset.value + rec0Str.length; i < 4096; i++) {
    buf[i] = 0x7E ^ (i & 0x0F) // Fill byte signature
  }

  // Record 1 Payload at 3950..4019 (70 Bytes) if active or remaining fragmented
  if (slot1Active.value || fragmentedBytes.value > 0) {
    const rec1Str = "\x01\x02\x00\x00\x00\x00\x00\x00\x00\x02\x0C\x00\x00\x00Grace Hopper\x02\x09\x00\x00\x00Staff Eng"
    for (let i = 0; i < rec1Str.length; i++) {
      buf[slot1Offset.value + i] = rec1Str.charCodeAt(i)
    }
  }

  // Record 2 Payload at 3882..3949 (68 Bytes)
  if (hasSlot2.value) {
    const rec2Str = "\x01\x03\x00\x00\x00\x00\x00\x00\x00\x02\x0E\x00\x00\x00Claude Shannon\x02\x06\x00\x00\x00Fellow"
    for (let i = 0; i < rec2Str.length; i++) {
      buf[slot2Offset.value + i] = rec2Str.charCodeAt(i)
    }
  }

  return buf
}

const renderedHexRows = computed(() => {
  const buf = generateMemoryBuffer()
  const rows = []

  if (!fullDumpMode.value) {
    // Mode 1: Key Regions (Header + Slots + Gap Break + Record Payloads)
    // 1. Header & Slots (offsets 0, 16, 32)
    for (let offset = 0; offset <= 32; offset += 16) {
      const slice = Array.from(buf.slice(offset, offset + 16))
      rows.push({ offset, bytes: slice, isGapBreak: false })
    }

    // 2. Synthesized Gap Break for Unallocated Free Space
    const gapStart = hasSlot2.value ? 48 : 32
    const gapEnd = freeEnd.value
    rows.push({
      offset: gapStart,
      isGapBreak: true,
      startHex: formatHex(gapStart, 4),
      endHex: formatHex(gapEnd - 1, 4),
      gapBytes: gapEnd - gapStart
    })

    // 3. Record Payloads (rounded down to 16-byte boundary)
    const recStartRow = Math.floor(freeEnd.value / 16) * 16
    for (let offset = recStartRow; offset < 4096; offset += 16) {
      const slice = Array.from(buf.slice(offset, offset + 16))
      rows.push({ offset, bytes: slice, isGapBreak: false })
    }
  } else {
    // Mode 2: Full 4096 Continuous buffer (256 rows)
    for (let offset = 0; offset < 4096; offset += 16) {
      const slice = Array.from(buf.slice(offset, offset + 16))
      rows.push({ offset, bytes: slice, isGapBreak: false })
    }
  }

  return rows
})

// Simulation actions
const simulateInsert = () => {
  hasSlot2.value = true
  statusMsg.value = 'Inserted Slot 2: free_start moved down to 36; Record 2 payload placed at byte 3882 (free_end moved up).'
}

const simulateDelete = () => {
  slot1Active.value = false
  fragmentedBytes.value = 70
  statusMsg.value = 'Deleted Record 1: Slot 1 length set to 0 (tombstone). 70 bytes recorded as fragmented_bytes.'
}

const simulateCompact = () => {
  slot1Active.value = false
  fragmentedBytes.value = 0
  statusMsg.value = 'Page compacted: Table engine slide-compacted active records contiguous to byte 4096. Fragmented bytes reset to 0.'
}

const resetPage = () => {
  hasSlot2.value = false
  slot1Active.value = true
  fragmentedBytes.value = 0
  statusMsg.value = 'Page reset to baseline: 2 active records, 0 fragmented bytes.'
  activeHoverRegion.value = null
  activeSubRegion.value = null
  hoveredByteOffset.value = null
}
</script>

<style scoped>
.hex-inspector-card {
  margin: 2.5rem 0;
  border-radius: 12px;
  border: 1px solid var(--vp-c-divider);
  background: var(--vp-c-bg-soft);
  box-shadow: 0 8px 30px rgba(0, 0, 0, 0.08);
  overflow: hidden;
  font-family: var(--vp-font-family-base);
}

.inspector-header {
  padding: 1.5rem 1.75rem 1.25rem;
  border-bottom: 1px solid var(--vp-c-divider);
  background: var(--vp-c-bg);
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  flex-wrap: wrap;
  gap: 1rem;
}

.header-titles {
  max-width: 650px;
}

.inspector-tag {
  display: inline-block;
  font-family: var(--vp-font-family-mono);
  font-size: 0.72rem;
  font-weight: 700;
  letter-spacing: 0.08em;
  color: #b83a24;
  background: rgba(184, 58, 36, 0.08);
  padding: 3px 8px;
  border-radius: 4px;
  margin-bottom: 0.4rem;
}

.dark .inspector-tag {
  color: #f87171;
  background: rgba(248, 113, 113, 0.12);
}

.inspector-title {
  margin: 0 0 0.4rem;
  font-family: 'Newsreader', Georgia, serif;
  font-size: 1.45rem;
  font-weight: 700;
  color: var(--vp-c-text-1);
}

.inspector-sub {
  margin: 0;
  font-size: 0.9rem;
  color: var(--vp-c-text-2);
  line-height: 1.45;
}

.view-controls {
  display: flex;
  align-items: center;
  gap: 0.4rem;
  flex-wrap: wrap;
}

.ctrl-label {
  font-size: 0.78rem;
  font-family: var(--vp-font-family-mono);
  color: var(--vp-c-text-2);
  margin-right: 0.2rem;
}

.region-chip {
  padding: 4px 9px;
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  border: 1px solid var(--vp-c-divider);
  border-radius: 6px;
  background: var(--vp-c-bg-soft);
  color: var(--vp-c-text-1);
  cursor: pointer;
  transition: all 0.15s ease;
}

.region-chip:hover,
.region-chip.active {
  border-color: #b83a24;
  color: #b83a24;
  background: rgba(184, 58, 36, 0.08);
}

.dark .region-chip:hover,
.dark .region-chip.active {
  border-color: #f87171;
  color: #f87171;
  background: rgba(248, 113, 113, 0.15);
}

/* Simulation Action Bar */
.sim-actions-bar {
  padding: 0.75rem 1.75rem;
  background: var(--vp-c-bg-mute);
  border-bottom: 1px solid var(--vp-c-divider);
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 0.75rem;
}

.sim-left {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  flex-wrap: wrap;
}

.sim-label {
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  font-weight: 600;
  color: var(--vp-c-text-2);
}

.sim-btn {
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  font-weight: 600;
  padding: 4px 10px;
  border-radius: 6px;
  border: 1px solid var(--vp-c-divider);
  background: var(--vp-c-bg);
  color: var(--vp-c-text-1);
  cursor: pointer;
  transition: all 0.15s ease;
}

.sim-btn:hover:not(:disabled) {
  transform: translateY(-1px);
}

.sim-btn.insert:hover:not(:disabled) {
  border-color: #10b981;
  color: #10b981;
}

.sim-btn.delete:hover:not(:disabled) {
  border-color: #ef4444;
  color: #ef4444;
}

.sim-btn.compact:hover:not(:disabled) {
  border-color: #f59e0b;
  color: #f59e0b;
}

.sim-btn:disabled {
  opacity: 0.4;
  cursor: not-allowed;
}

.status-indicator {
  display: inline-flex;
  align-items: center;
  gap: 0.5rem;
  font-size: 0.8rem;
  color: var(--vp-c-text-2);
}

.status-dot {
  width: 7px;
  height: 7px;
  border-radius: 50%;
  background: #10b981;
  box-shadow: 0 0 6px rgba(16, 185, 129, 0.6);
}

/* Dual-Pane Layout */
.inspector-panes {
  display: grid;
  grid-template-columns: 1fr 1fr;
  min-height: 520px;
}

@media (max-width: 960px) {
  .inspector-panes {
    grid-template-columns: 1fr;
  }
}

.pane {
  display: flex;
  flex-direction: column;
}

.hex-dump-pane {
  border-right: 1px solid var(--vp-c-divider);
  background: #121214;
  color: #f4f4f5;
}

.decoded-pane {
  background: var(--vp-c-bg);
}

.pane-header {
  padding: 0.85rem 1.25rem;
  border-bottom: 1px solid var(--vp-c-divider);
  display: flex;
  justify-content: space-between;
  align-items: center;
  background: rgba(0, 0, 0, 0.03);
}

.hex-dump-pane .pane-header {
  background: #1a1a1e;
  border-bottom-color: #27272a;
}

.pane-title {
  font-family: var(--vp-font-family-mono);
  font-size: 0.82rem;
  font-weight: 600;
  display: flex;
  align-items: center;
  gap: 0.5rem;
}

.page-meta-badge {
  font-family: var(--vp-font-family-mono);
  font-size: 0.72rem;
  padding: 2px 7px;
  border-radius: 4px;
  background: var(--vp-c-bg-mute);
  color: var(--vp-c-text-2);
  border: 1px solid var(--vp-c-divider);
}

.view-toggle {
  display: flex;
  border: 1px solid #3f3f46;
  border-radius: 5px;
  overflow: hidden;
}

.toggle-tab {
  padding: 2px 8px;
  font-size: 0.72rem;
  font-family: var(--vp-font-family-mono);
  background: #18181b;
  color: #a1a1aa;
  border: none;
  cursor: pointer;
}

.toggle-tab.active {
  background: #27272a;
  color: #fafafa;
  font-weight: 600;
}

/* Hex Ruler */
.hex-ruler-row {
  display: flex;
  padding: 0.5rem 1rem;
  font-family: var(--vp-font-family-mono);
  font-size: 0.68rem;
  color: #71717a;
  background: #161619;
  border-bottom: 1px solid #27272a;
}

.ruler-offset {
  width: 58px;
  flex-shrink: 0;
}

.ruler-hex {
  flex: 1;
  letter-spacing: 0.05em;
}

.ruler-ascii {
  width: 130px;
  text-align: left;
}

/* Hex Viewport */
.hex-viewport {
  flex: 1;
  max-height: 440px;
  overflow-y: auto;
  padding: 0.5rem 1rem;
  font-family: var(--vp-font-family-mono);
  font-size: 0.78rem;
  line-height: 1.55;
}

.hex-row {
  display: flex;
  align-items: center;
  padding: 1px 0;
  transition: background 0.1s ease;
}

.hex-row:hover {
  background: rgba(255, 255, 255, 0.04);
}

.row-offset {
  width: 58px;
  color: #71717a;
  user-select: none;
  font-weight: 600;
  flex-shrink: 0;
}

.row-bytes {
  flex: 1;
  display: flex;
  align-items: center;
  gap: 4px;
}

.hex-byte {
  display: inline-block;
  padding: 1px 2px;
  border-radius: 3px;
  cursor: pointer;
  transition: all 0.12s ease;
  color: #d4d4d8;
}

.hex-byte.quad-space {
  margin-right: 6px;
}

.row-ascii {
  width: 130px;
  display: flex;
  color: #a1a1aa;
  letter-spacing: 0.05em;
  user-select: none;
}

.ascii-char {
  display: inline-block;
  width: 8px;
  text-align: center;
}

/* Gap break */
.gap-break-row {
  padding: 0.5rem 0;
}

.gap-break-banner {
  width: 100%;
  padding: 0.45rem 0.85rem;
  border-radius: 6px;
  border: 1px dashed #3f3f46;
  background: rgba(255, 255, 255, 0.02);
  display: flex;
  align-items: center;
  justify-content: space-between;
  font-size: 0.74rem;
  color: #a1a1aa;
}

.gap-expand-btn {
  background: transparent;
  border: 1px solid #52525b;
  color: #e4e4e7;
  padding: 2px 7px;
  border-radius: 4px;
  font-size: 0.7rem;
  cursor: pointer;
}

.gap-expand-btn:hover {
  background: #27272a;
}

/* Legend */
.hex-legend {
  padding: 0.5rem 1rem;
  background: #161619;
  border-top: 1px solid #27272a;
  display: flex;
  flex-wrap: wrap;
  gap: 0.75rem;
  font-size: 0.7rem;
  font-family: var(--vp-font-family-mono);
}

.legend-item {
  display: flex;
  align-items: center;
  gap: 0.35rem;
  color: #a1a1aa;
}

.legend-box {
  width: 10px;
  height: 10px;
  border-radius: 2px;
}

.legend-box.hdr { background: #3b82f6; }
.legend-box.slot0 { background: #f59e0b; }
.legend-box.slot1 { background: #10b981; }
.legend-box.slot2 { background: #a855f7; }
.legend-box.free { background: #52525b; }
.legend-box.rec0 { background: #ef4444; }
.legend-box.rec1 { background: #06b6d4; }

/* Byte Colors in Dump */
.c-hdr { color: #93c5fd; }
.c-slot0 { color: #fde68a; }
.c-slot1 { color: #a7f3d0; }
.c-slot2 { color: #e9d5ff; }
.c-free { color: #52525b; }
.c-rec0 { color: #fca5a5; }
.c-rec1 { color: #a5f3fc; }
.c-rec2 { color: #d8b4fe; }
.c-fragmented { color: #ef4444; text-decoration: line-through; }

/* Dynamic Highlight overlays */
.byte-active-cursor {
  outline: 2px solid #ffffff !important;
  background: #ffffff !important;
  color: #000000 !important;
  font-weight: 700;
}

.hl-header,
.hl-sub-focus {
  background: rgba(59, 130, 246, 0.35);
  color: #ffffff;
  outline: 1px solid #3b82f6;
}

.hl-slot0-focus {
  background: rgba(245, 158, 11, 0.45);
  color: #ffffff;
  font-weight: 700;
  outline: 1px solid #f59e0b;
}

.hl-rec0-focus {
  background: rgba(239, 68, 68, 0.4);
  color: #ffffff;
  font-weight: 700;
  outline: 1px solid #ef4444;
}

.hl-slot1-focus {
  background: rgba(16, 185, 129, 0.45);
  color: #ffffff;
  font-weight: 700;
  outline: 1px solid #10b981;
}

.hl-rec1-focus {
  background: rgba(6, 182, 212, 0.4);
  color: #ffffff;
  font-weight: 700;
  outline: 1px solid #06b6d4;
}

.hl-slot2-focus {
  background: rgba(168, 85, 247, 0.45);
  color: #ffffff;
  font-weight: 700;
  outline: 1px solid #a855f7;
}

.hl-rec2-focus {
  background: rgba(216, 180, 254, 0.4);
  color: #ffffff;
  font-weight: 700;
  outline: 1px solid #c084fc;
}

.hl-free-focus {
  background: rgba(113, 113, 122, 0.3);
  color: #e4e4e7;
}

/* RIGHT PANE: Decoded Inspector Styles */
.decoded-body {
  padding: 1rem 1.25rem;
  max-height: 480px;
  overflow-y: auto;
  display: flex;
  flex-direction: column;
  gap: 0.85rem;
}

.struct-card {
  border: 1px solid var(--vp-c-divider);
  border-radius: 8px;
  padding: 0.85rem 1rem;
  background: var(--vp-c-bg-soft);
  transition: all 0.15s ease;
}

.struct-card:hover,
.struct-card.active {
  border-color: #b83a24;
  box-shadow: 0 4px 15px rgba(0, 0, 0, 0.05);
}

.dark .struct-card:hover,
.dark .struct-card.active {
  border-color: #f87171;
}

.card-title-bar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 0.65rem;
}

.title-text {
  font-family: var(--vp-font-family-mono);
  font-size: 0.82rem;
  font-weight: 700;
  color: var(--vp-c-text-1);
  display: flex;
  align-items: center;
  gap: 0.4rem;
}

.color-pill {
  width: 9px;
  height: 9px;
  border-radius: 3px;
  display: inline-block;
}

.color-pill.hdr { background: #3b82f6; }
.color-pill.slots { background: #f59e0b; }
.color-pill.free { background: #71717a; }
.color-pill.rec { background: #ef4444; }

.byte-badge {
  font-family: var(--vp-font-family-mono);
  font-size: 0.7rem;
  color: var(--vp-c-text-2);
  background: var(--vp-c-bg-mute);
  padding: 2px 6px;
  border-radius: 4px;
}

/* Fields Grid in Header */
.fields-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(130px, 1fr));
  gap: 0.5rem;
}

.field-item {
  padding: 0.45rem 0.6rem;
  border-radius: 6px;
  background: var(--vp-c-bg);
  border: 1px solid var(--vp-c-divider);
  display: flex;
  flex-direction: column;
  transition: all 0.12s ease;
}

.field-item:hover {
  border-color: #3b82f6;
  background: rgba(59, 130, 246, 0.05);
}

.f-name {
  font-family: var(--vp-font-family-mono);
  font-size: 0.68rem;
  color: var(--vp-c-text-2);
  margin-bottom: 2px;
}

.f-val {
  font-family: var(--vp-font-family-mono);
  font-size: 0.78rem;
  font-weight: 600;
  color: var(--vp-c-text-1);
}

.f-val.highlight {
  color: #b83a24;
}

.dark .f-val.highlight {
  color: #f87171;
}

.f-sub {
  font-size: 0.65rem;
  color: var(--vp-c-text-3);
  margin-top: 2px;
}

/* Slot List */
.slots-list {
  display: flex;
  flex-direction: column;
  gap: 0.5rem;
}

.slot-entry-item {
  border: 1px solid var(--vp-c-divider);
  border-radius: 6px;
  padding: 0.6rem 0.75rem;
  background: var(--vp-c-bg);
  cursor: pointer;
  transition: all 0.15s ease;
}

.slot-entry-item.slot-0:hover,
.slot-entry-item.slot-0.active {
  border-color: #f59e0b;
  background: rgba(245, 158, 11, 0.08);
}

.slot-entry-item.slot-1:hover,
.slot-entry-item.slot-1.active {
  border-color: #10b981;
  background: rgba(16, 185, 129, 0.08);
}

.slot-entry-item.slot-2:hover,
.slot-entry-item.slot-2.active {
  border-color: #a855f7;
  background: rgba(168, 85, 247, 0.08);
}

.slot-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 0.35rem;
}

.slot-badge {
  font-family: var(--vp-font-family-mono);
  font-size: 0.72rem;
  font-weight: 700;
  padding: 2px 6px;
  border-radius: 4px;
}

.slot-badge.s0 { background: rgba(245, 158, 11, 0.15); color: #d97706; }
.dark .slot-badge.s0 { color: #fbbf24; }
.slot-badge.s1 { background: rgba(16, 185, 129, 0.15); color: #059669; }
.dark .slot-badge.s1 { color: #34d399; }
.slot-badge.s2 { background: rgba(168, 85, 247, 0.15); color: #9333ea; }
.dark .slot-badge.s2 { color: #c084fc; }

.slot-offsets {
  font-family: var(--vp-font-family-mono);
  font-size: 0.7rem;
  color: var(--vp-c-text-2);
}

.slot-status {
  font-family: var(--vp-font-family-mono);
  font-size: 0.65rem;
  font-weight: 700;
  padding: 1px 5px;
  border-radius: 3px;
}

.slot-status.active { background: rgba(16, 185, 129, 0.12); color: #10b981; }
.slot-status.tomb { background: rgba(239, 68, 68, 0.12); color: #ef4444; }

.slot-details {
  display: flex;
  gap: 1rem;
  font-size: 0.75rem;
  font-family: var(--vp-font-family-mono);
  flex-wrap: wrap;
}

.p-lbl {
  color: var(--vp-c-text-2);
  margin-right: 4px;
}

.p-val {
  font-weight: 600;
  color: var(--vp-c-text-1);
}

.p-val-target {
  font-weight: 700;
  color: #b83a24;
}

.dark .p-val-target {
  color: #f87171;
}

/* Free Space Card */
.free-desc {
  margin: 0;
  font-size: 0.78rem;
  color: var(--vp-c-text-2);
  line-height: 1.45;
}

/* Record Payloads */
.records-list {
  display: flex;
  flex-direction: column;
  gap: 0.5rem;
}

.record-entry {
  border: 1px solid var(--vp-c-divider);
  border-radius: 6px;
  padding: 0.6rem 0.75rem;
  background: var(--vp-c-bg);
  cursor: pointer;
  transition: all 0.15s ease;
}

.record-entry.rec-0:hover,
.record-entry.rec-0.active {
  border-color: #ef4444;
  background: rgba(239, 68, 68, 0.08);
}

.record-entry.rec-1:hover,
.record-entry.rec-1.active {
  border-color: #06b6d4;
  background: rgba(6, 182, 212, 0.08);
}

.record-entry.rec-2:hover,
.record-entry.rec-2.active {
  border-color: #a855f7;
  background: rgba(168, 85, 247, 0.08);
}

.rec-head {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 0.35rem;
}

.rec-badge {
  font-family: var(--vp-font-family-mono);
  font-size: 0.72rem;
  font-weight: 700;
}

.rec-badge.r0 { color: #ef4444; }
.rec-badge.r1 { color: #06b6d4; }
.rec-badge.r2 { color: #a855f7; }

.rec-span {
  font-family: var(--vp-font-family-mono);
  font-size: 0.7rem;
  color: var(--vp-c-text-2);
}

.rec-rid {
  font-family: var(--vp-font-family-mono);
  font-size: 0.7rem;
  font-weight: 600;
  padding: 1px 5px;
  border-radius: 3px;
  background: var(--vp-c-bg-mute);
  color: var(--vp-c-text-1);
}

.rec-tuple {
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  background: var(--vp-c-bg-soft);
  padding: 4px 8px;
  border-radius: 4px;
}

.tuple-key {
  color: var(--vp-c-text-2);
}

.tuple-val.int {
  color: #3b82f6;
  font-weight: 600;
}

.tuple-val.str {
  color: #10b981;
}

.tombstone-entry {
  border: 1px dashed #ef4444;
  background: rgba(239, 68, 68, 0.05);
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-family: var(--vp-font-family-mono);
  font-size: 0.72rem;
}

.tomb-tag {
  color: #ef4444;
  font-weight: 700;
}

.tomb-info {
  color: var(--vp-c-text-2);
}

/* Footer Bar */
.inspector-footer {
  padding: 0.75rem 1.75rem;
  background: var(--vp-c-bg);
  border-top: 1px solid var(--vp-c-divider);
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 0.75rem;
}

.footer-left {
  display: flex;
  align-items: center;
  gap: 0.5rem;
}

.f-stat-tag {
  font-family: var(--vp-font-family-mono);
  font-size: 0.72rem;
  font-weight: 700;
  color: #b83a24;
}

.dark .f-stat-tag {
  color: #f87171;
}

.f-stat-val {
  font-family: var(--vp-font-family-mono);
  font-size: 0.78rem;
  color: var(--vp-c-text-1);
}

.engine-invariant-note {
  font-size: 0.75rem;
  color: var(--vp-c-text-2);
}
</style>
