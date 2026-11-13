<template>
  <div class="memory-map-card diagram-full">
    <div class="memory-map-header" v-if="title || subheader">
      <div class="header-left">
        <span class="map-badge">SPATIAL MEMORY MAP</span>
        <h3 class="map-title" v-if="title">{{ title }}</h3>
        <p class="map-sub" v-if="subheader">{{ subheader }}</p>
      </div>
      <div class="header-right" v-if="totalBytesDisplay">
        <span class="total-bytes-badge">{{ totalBytesDisplay }}</span>
      </div>
    </div>

    <div class="memory-map-frame">
      <!-- Ruler & Column Grid -->
      <div class="map-grid">
        <!-- Vertical Ruler & Segments Column -->
        <div class="segments-column">
          <div 
            v-for="(seg, idx) in normalizedSegments" 
            :key="seg.id || idx"
            :class="['mem-segment', seg.kind, { 'has-growth-down': seg.growth === 'down', 'has-growth-up': seg.growth === 'up' }]"
            :style="{ minHeight: seg.calculatedHeight + 'px' }"
          >
            <!-- Boundary Offset Markers -->
            <div class="offset-marker top">
              <span class="offset-val">0x{{ seg.startOffset.toString(16).toUpperCase().padStart(4, '0') }}</span>
              <span class="offset-dec">[{{ seg.startOffset }}]</span>
            </div>

            <!-- Segment Content Body -->
            <div class="segment-body">
              <div class="segment-meta">
                <span class="kind-pill">{{ seg.kind.toUpperCase() }}</span>
                <span class="segment-bytes">{{ seg.bytes }} bytes</span>
              </div>
              <div class="segment-label-group">
                <div class="segment-label">{{ seg.label }}</div>
                <div class="segment-sublabel" v-if="seg.sublabel">{{ seg.sublabel }}</div>
              </div>

              <!-- Growth indicator annotation inside segment -->
              <div class="growth-annotation down" v-if="seg.growth === 'down'">
                <span class="arrow-symbol">↓</span>
                <span class="growth-text">grows downward (toward free space)</span>
              </div>

              <div class="growth-annotation up" v-if="seg.growth === 'up'">
                <span class="arrow-symbol">↑</span>
                <span class="growth-text">grows upward (from bottom byte 4096)</span>
              </div>
            </div>

            <!-- Last segment bottom marker -->
            <div class="offset-marker bottom" v-if="idx === normalizedSegments.length - 1">
              <span class="offset-val">0x{{ seg.endOffset.toString(16).toUpperCase().padStart(4, '0') }}</span>
              <span class="offset-dec">[{{ seg.endOffset }}]</span>
            </div>
          </div>
        </div>

        <!-- Right Annotation Sidebar: Two-way growth explanation -->
        <div class="growth-legend-sidebar" v-if="showGrowthSidebar">
          <div class="growth-marker slot-growth">
            <div class="marker-bracket top-bracket"></div>
            <div class="marker-content">
              <span class="marker-arrow">↓</span>
              <span class="marker-title">Slots Grow Forward</span>
              <span class="marker-desc">Header + N × 4B slots allocate sequentially downward</span>
            </div>
          </div>

          <div class="growth-marker free-boundary">
            <span class="free-divider-label">← free_start .. free_end →</span>
          </div>

          <div class="growth-marker record-growth">
            <div class="marker-bracket bottom-bracket"></div>
            <div class="marker-content">
              <span class="marker-arrow">↑</span>
              <span class="marker-title">Payloads Grow Backward</span>
              <span class="marker-desc">Variable-length records allocate from byte 4095 upward</span>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- Footer Notes -->
    <div class="memory-map-footer" v-if="footerNote">
      <span class="footer-icon">💡</span>
      <span class="footer-text">{{ footerNote }}</span>
    </div>
  </div>
</template>

<script setup>
import { computed } from 'vue'

const props = defineProps({
  title: String,
  subheader: String,
  totalBytes: {
    type: Number,
    default: 4096
  },
  showGrowthSidebar: {
    type: Boolean,
    default: true
  },
  footerNote: String,
  segments: {
    type: Array,
    required: true
  }
})

const totalBytesDisplay = computed(() => {
  return `${props.totalBytes} Bytes (Aligned 4KB Page)`
})

const normalizedSegments = computed(() => {
  let currentOffset = 0
  const total = props.totalBytes || 4096

  return props.segments.map((seg) => {
    const start = currentOffset
    const end = start + seg.bytes
    currentOffset = end

    // Proportional height with readable minimum
    const ratio = seg.bytes / total
    let height = Math.round(ratio * 320)
    if (seg.kind === 'free') {
      height = Math.max(height, 80)
    } else if (seg.kind === 'header') {
      height = Math.max(height, 68)
    } else {
      height = Math.max(height, 74)
    }

    return {
      ...seg,
      startOffset: start,
      endOffset: end,
      calculatedHeight: height
    }
  })
})
</script>

<style scoped>
.memory-map-card {
  border: 1px solid var(--qd-border, #ded8cb);
  background: var(--qd-paper-surface, #ffffff);
  border-radius: 8px;
  overflow: hidden;
  box-shadow: 0 4px 16px rgba(0, 0, 0, 0.05);
  margin: 2.5rem auto;
}

.memory-map-header {
  padding: 14px 20px;
  background: var(--qd-paper-alt, #eee9dc);
  border-bottom: 1px solid var(--qd-border, #ded8cb);
  display: flex;
  justify-content: space-between;
  align-items: baseline;
  flex-wrap: wrap;
  gap: 12px;
}

.map-badge {
  font-family: var(--vp-font-family-mono);
  font-size: 0.65rem;
  font-weight: 700;
  color: var(--qd-accent, #b33927);
  letter-spacing: 0.1em;
  display: block;
  margin-bottom: 2px;
}

.map-title {
  margin: 0;
  font-family: var(--vp-font-family-base);
  font-size: 1.25rem;
  font-weight: 700;
  color: var(--qd-ink, #181716);
  letter-spacing: -0.02em;
}

.map-sub {
  margin: 4px 0 0;
  font-size: 0.85rem;
  color: var(--qd-ink-muted, #5e5a54);
}

.total-bytes-badge {
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  font-weight: 600;
  color: var(--qd-ink-muted, #5e5a54);
  background: var(--qd-paper, #f7f3e9);
  padding: 4px 10px;
  border-radius: 4px;
  border: 1px solid var(--qd-border, #ded8cb);
}

/* Frame & Grid */
.memory-map-frame {
  padding: 24px 20px;
  background: var(--qd-paper, #f7f3e9);
}

.map-grid {
  display: grid;
  grid-template-columns: 1fr 240px;
  gap: 24px;
  align-items: stretch;
}

@media (max-width: 820px) {
  .map-grid {
    grid-template-columns: 1fr;
  }
  .growth-legend-sidebar {
    display: none;
  }
}

/* Segments Column */
.segments-column {
  display: flex;
  flex-direction: column;
  position: relative;
  border: 2px solid var(--qd-border, #ded8cb);
  border-radius: 6px;
  background: var(--qd-paper-surface, #ffffff);
  overflow: visible;
}

.mem-segment {
  position: relative;
  display: flex;
  flex-direction: column;
  justify-content: center;
  padding: 16px 20px 16px 130px;
  border-bottom: 1px solid var(--qd-border, #ded8cb);
  transition: background 0.15s ease;
}

.mem-segment:last-child {
  border-bottom: none;
}

/* Left Offset Marker Gutter */
.offset-marker {
  position: absolute;
  left: 12px;
  display: flex;
  align-items: baseline;
  gap: 6px;
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  color: var(--qd-ink-faint, #8e8a82);
  user-select: none;
}

.offset-marker.top {
  top: 8px;
}

.offset-marker.bottom {
  bottom: 8px;
}

.offset-val {
  font-weight: 700;
  color: var(--qd-accent, #b33927);
}

.offset-dec {
  font-size: 0.7rem;
  opacity: 0.8;
}

/* Segment Kinds Styling */
.mem-segment.header {
  background: rgba(179, 57, 39, 0.04);
  border-left: 4px solid var(--qd-accent, #b33927);
}

.mem-segment.slot {
  background: var(--qd-paper-surface, #ffffff);
  border-left: 4px solid #0284c7;
}

.mem-segment.free {
  background: repeating-linear-gradient(
    -45deg,
    transparent,
    transparent 8px,
    var(--qd-hatch, rgba(142, 138, 130, 0.12)) 8px,
    var(--qd-hatch, rgba(142, 138, 130, 0.12)) 16px
  );
  border-style: dashed;
  border-color: var(--qd-border, #ded8cb);
  border-left: 4px dashed var(--qd-ink-faint, #8e8a82);
  text-align: center;
  padding-left: 120px;
}

.mem-segment.record {
  background: rgba(13, 148, 136, 0.05);
  border-left: 4px solid #0d9488;
}

/* Body & Labels */
.segment-body {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.segment-meta {
  display: flex;
  align-items: center;
  gap: 8px;
}

.kind-pill {
  font-family: var(--vp-font-family-mono);
  font-size: 0.65rem;
  font-weight: 700;
  letter-spacing: 0.06em;
  padding: 1px 6px;
  border-radius: 3px;
  background: var(--qd-paper-alt, #eee9dc);
  color: var(--qd-ink, #181716);
}

.segment-bytes {
  font-family: var(--vp-font-family-mono);
  font-size: 0.7rem;
  color: var(--qd-ink-faint, #8e8a82);
}

.segment-label {
  font-family: var(--vp-font-family-mono);
  font-size: 0.9rem;
  font-weight: 700;
  color: var(--qd-ink, #181716);
}

.segment-sublabel {
  font-family: var(--vp-font-family-sans);
  font-size: 0.8rem;
  color: var(--qd-ink-muted, #5e5a54);
  line-height: 1.4;
}

/* In-Segment Growth Annotations */
.growth-annotation {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  margin-top: 6px;
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  font-weight: 600;
}

.growth-annotation.down {
  color: #0284c7;
}

.growth-annotation.up {
  color: #0d9488;
}

.arrow-symbol {
  font-size: 0.9rem;
  font-weight: 900;
}

/* Right Sidebar Growth Markers */
.growth-legend-sidebar {
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  padding: 10px 0;
}

.growth-marker {
  display: flex;
  flex-direction: column;
  gap: 4px;
  padding: 12px;
  background: var(--qd-paper-surface, #ffffff);
  border: 1px solid var(--qd-border, #ded8cb);
  border-radius: 6px;
  box-shadow: 0 1px 3px rgba(0,0,0,0.03);
}

.growth-marker.slot-growth {
  border-left: 3px solid #0284c7;
}

.growth-marker.record-growth {
  border-left: 3px solid #0d9488;
}

.growth-marker.free-boundary {
  background: transparent;
  border: 1px dashed var(--qd-border, #ded8cb);
  text-align: center;
  padding: 6px;
}

.free-divider-label {
  font-family: var(--vp-font-family-mono);
  font-size: 0.7rem;
  color: var(--qd-ink-faint, #8e8a82);
}

.marker-arrow {
  font-size: 1.1rem;
  font-weight: 900;
  color: var(--qd-accent, #b33927);
}

.marker-title {
  font-family: var(--vp-font-family-sans);
  font-weight: 700;
  font-size: 0.85rem;
  color: var(--qd-ink, #181716);
}

.marker-desc {
  font-family: var(--vp-font-family-sans);
  font-size: 0.75rem;
  color: var(--qd-ink-muted, #5e5a54);
  line-height: 1.4;
}

/* Footer */
.memory-map-footer {
  padding: 10px 20px;
  background: var(--qd-paper-alt, #eee9dc);
  border-top: 1px solid var(--qd-border, #ded8cb);
  display: flex;
  align-items: center;
  gap: 8px;
  font-family: var(--vp-font-family-sans);
  font-size: 0.8rem;
  color: var(--qd-ink-muted, #5e5a54);
}
</style>
