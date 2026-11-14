<template>
  <div class="excalidraw-diagram-container" :class="{ 'is-modal-open': isModalOpen }">
    <!-- Window Header Chrome -->
    <div class="diagram-window-header">
      <div class="header-left">
        <span class="retro-icon">📐</span>
        <span class="badge-katakana">[EXCALIDRAW // 設計図]</span>
        <span class="diagram-title">{{ title }}</span>
      </div>
      <div class="header-actions">
        <button 
          type="button" 
          class="action-btn zoom-btn" 
          @click="openModal" 
          title="Zoom & Fullscreen Inspect"
        >
          <span>⤢</span> Zoom
        </button>
        <a 
          :href="withBase(`/diagrams/${name}.svg`)" 
          target="_blank" 
          class="action-btn download-btn" 
          title="Open raw SVG vector in new tab"
        >
          <span>⎘</span> SVG
        </a>
        <a 
          :href="`https://excalidraw.com`" 
          target="_blank" 
          class="action-btn excalidraw-link" 
          title="Open Excalidraw editor"
        >
          <span>✎</span> Edit
        </a>
      </div>
    </div>

    <!-- Diagram Display Frame -->
    <div class="diagram-body" @click="openModal" title="Click to expand / inspect full resolution">
      <div class="svg-wrapper">
        <img 
          :src="withBase(`/diagrams/${name}.svg`)" 
          :alt="alt || title" 
          class="rendered-diagram-img" 
          loading="lazy"
        />
      </div>
      <div class="inspection-hint">
        <span>Click diagram or [⤢ Zoom] to inspect full vector resolution</span>
      </div>
    </div>

    <!-- Fullscreen / Zoom Lightbox Modal -->
    <Teleport to="body">
      <Transition name="fade">
        <div v-if="isModalOpen" class="excalidraw-lightbox-backdrop" @click="closeModal" @keydown.esc="closeModal">
          <div class="lightbox-dialog" @click.stop>
            <div class="lightbox-header">
              <div class="lightbox-title-group">
                <span class="retro-icon">📐</span>
                <span class="badge-katakana">[EXCALIDRAW // INSPECT]</span>
                <span class="lightbox-title">{{ title }}</span>
              </div>
              <div class="lightbox-controls">
                <button type="button" class="zoom-ctrl-btn" @click="zoomOut" title="Zoom Out">−</button>
                <span class="zoom-level">{{ Math.round(zoomScale * 100) }}%</span>
                <button type="button" class="zoom-ctrl-btn" @click="zoomIn" title="Zoom In">+</button>
                <button type="button" class="zoom-ctrl-btn" @click="resetZoom" title="Reset">1:1</button>
                <button type="button" class="close-btn" @click="closeModal" title="Close (Esc)">[ ✕ ]</button>
              </div>
            </div>

            <div 
              class="lightbox-viewport"
              ref="viewportRef"
              @touchstart="onTouchStart"
              @touchmove="onTouchMove"
              @touchend="onTouchEnd"
              @mousedown="onMouseDown"
              @mousemove="onMouseMove"
              @mouseup="onMouseUp"
              @mouseleave="onMouseUp"
              @wheel.prevent="onWheel"
              @dblclick="toggleZoom"
            >
              <div 
                class="lightbox-canvas" 
                :class="{ 'is-dragging': isPanning, 'can-pan': zoomScale > 1.0 }"
                :style="{ transform: `translate3d(${panX}px, ${panY}px, 0px) scale(${zoomScale})` }"
              >
                <img 
                  :src="withBase(`/diagrams/${name}.svg`)" 
                  :alt="alt || title" 
                  class="lightbox-img"
                  draggable="false"
                />
              </div>
            </div>

            <div class="lightbox-footer">
              <span class="footer-hint">Touch: Pinch to zoom &bull; Drag to pan &bull; Double-tap 2&times; // Desktop: Scroll wheel &bull; Drag to pan (zoomed)</span>
              <a :href="withBase(`/diagrams/${name}.svg`)" download class="footer-download-btn">Download Vector SVG</a>
            </div>
          </div>
        </div>
      </Transition>
    </Teleport>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue'
import { withBase } from 'vitepress'

const props = defineProps<{
  name: string
  title: string
  alt?: string
}>()

const isModalOpen = ref(false)
const zoomScale = ref(1.0)
const panX = ref(0)
const panY = ref(0)
const isPanning = ref(false)
const viewportRef = ref<HTMLElement | null>(null)

// Touch tracking
let initialPinchDist = 0
let initialZoom = 1.0
let touchStartX = 0
let touchStartY = 0
let startPanX = 0
let startPanY = 0
let lastTapTime = 0

// Mouse tracking
let mouseStartX = 0
let mouseStartY = 0

function openModal() {
  isModalOpen.value = true
  zoomScale.value = 1.0
  panX.value = 0
  panY.value = 0
  document.body.style.overflow = 'hidden'
}

function closeModal() {
  isModalOpen.value = false
  zoomScale.value = 1.0
  panX.value = 0
  panY.value = 0
  document.body.style.overflow = ''
}

function zoomIn() {
  zoomScale.value = Math.min(zoomScale.value + 0.25, 4.0)
}

function zoomOut() {
  zoomScale.value = Math.max(zoomScale.value - 0.25, 0.5)
  if (zoomScale.value <= 1.0) {
    panX.value = 0
    panY.value = 0
  }
}

function resetZoom() {
  zoomScale.value = 1.0
  panX.value = 0
  panY.value = 0
}

function toggleZoom() {
  if (zoomScale.value > 1.0) {
    resetZoom()
  } else {
    zoomScale.value = 2.0
  }
}

function onTouchStart(e: TouchEvent) {
  if (e.touches.length === 2) {
    // Pinch-to-zoom
    initialPinchDist = Math.hypot(
      e.touches[0].clientX - e.touches[1].clientX,
      e.touches[0].clientY - e.touches[1].clientY
    )
    initialZoom = zoomScale.value
  } else if (e.touches.length === 1) {
    const now = Date.now()
    if (now - lastTapTime < 300) {
      toggleZoom()
      lastTapTime = 0
      return
    }
    lastTapTime = now

    if (zoomScale.value > 1.0) {
      touchStartX = e.touches[0].clientX
      touchStartY = e.touches[0].clientY
      startPanX = panX.value
      startPanY = panY.value
      isPanning.value = true
    }
  }
}

function onTouchMove(e: TouchEvent) {
  if (e.touches.length === 2 && initialPinchDist > 0) {
    e.preventDefault()
    const dist = Math.hypot(
      e.touches[0].clientX - e.touches[1].clientX,
      e.touches[0].clientY - e.touches[1].clientY
    )
    const factor = dist / initialPinchDist
    zoomScale.value = Math.min(Math.max(initialZoom * factor, 0.5), 4.0)
  } else if (e.touches.length === 1 && isPanning.value && zoomScale.value > 1.0) {
    e.preventDefault()
    const dx = e.touches[0].clientX - touchStartX
    const dy = e.touches[0].clientY - touchStartY
    panX.value = startPanX + dx
    panY.value = startPanY + dy
  }
}

function onTouchEnd() {
  initialPinchDist = 0
  isPanning.value = false
  if (zoomScale.value <= 1.0) {
    panX.value = 0
    panY.value = 0
  }
}

function onMouseDown(e: MouseEvent) {
  if (e.button !== 0) return
  if (zoomScale.value <= 1.0) return // Only pan when zoomed
  isPanning.value = true
  mouseStartX = e.clientX
  mouseStartY = e.clientY
  startPanX = panX.value
  startPanY = panY.value
}

function onMouseMove(e: MouseEvent) {
  if (!isPanning.value || zoomScale.value <= 1.0) return
  const dx = e.clientX - mouseStartX
  const dy = e.clientY - mouseStartY
  panX.value = startPanX + dx
  panY.value = startPanY + dy
}

function onMouseUp() {
  isPanning.value = false
  if (zoomScale.value <= 1.0) {
    panX.value = 0
    panY.value = 0
  }
}

function onWheel(e: WheelEvent) {
  const delta = e.deltaY < 0 ? 0.2 : -0.2
  zoomScale.value = Math.min(Math.max(zoomScale.value + delta, 0.5), 4.0)
  if (zoomScale.value <= 1.0) {
    panX.value = 0
    panY.value = 0
  }
}

function handleKeyDown(e: KeyboardEvent) {
  if (e.key === 'Escape' && isModalOpen.value) {
    closeModal()
  }
}

onMounted(() => {
  window.addEventListener('keydown', handleKeyDown)
})

onUnmounted(() => {
  window.removeEventListener('keydown', handleKeyDown)
  document.body.style.overflow = ''
})
</script>

<style scoped>
.excalidraw-diagram-container {
  margin: 2rem 0;
  border: 2px solid var(--q-ink);
  border-radius: 8px;
  background: var(--q-bg-panel);
  box-shadow: 4px 4px 0px rgba(29, 41, 57, 0.18);
  font-family: var(--vp-font-family-mono, monospace);
  overflow: hidden;
  transition: border-color 0.2s ease, box-shadow 0.2s ease;
}

.dark .excalidraw-diagram-container {
  box-shadow: 4px 4px 0px #0a0d14;
  border-color: var(--q-accent-primary);
}

/* Header bar */
.diagram-window-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0.5rem 0.85rem;
  background: var(--q-bg-elevated);
  border-bottom: 2px solid var(--q-ink);
  border-top-left-radius: 6px;
  border-top-right-radius: 6px;
  gap: 0.75rem;
  flex-wrap: wrap;
}

.dark .diagram-window-header {
  border-bottom-color: var(--q-accent-primary);
}

.header-left {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  flex: 1 1 auto;
  min-width: 200px;
}

.retro-icon {
  font-size: 1.1rem;
}

.badge-katakana {
  font-size: 0.75rem;
  font-weight: 700;
  color: var(--q-accent-secondary);
  letter-spacing: 0.05em;
}

.diagram-title {
  font-size: 0.85rem;
  font-weight: 600;
  color: var(--q-ink);
}

.header-actions {
  display: flex;
  align-items: center;
  gap: 0.4rem;
}

.action-btn {
  display: inline-flex;
  align-items: center;
  gap: 0.25rem;
  padding: 0.2rem 0.5rem;
  font-size: 0.75rem;
  font-weight: 600;
  text-decoration: none !important;
  color: var(--q-ink) !important;
  background: var(--q-bg-panel);
  border: 1px solid var(--q-ink-muted);
  cursor: pointer;
  border-radius: 2px;
  transition: all 0.15s ease;
}

.action-btn:hover {
  background: var(--q-accent-primary);
  color: #ffffff !important;
  border-color: var(--q-accent-primary);
}

.dark .action-btn:hover {
  color: #141824 !important;
}

/* Body */
.diagram-body {
  padding: 1.25rem;
  cursor: zoom-in;
  position: relative;
  background: var(--q-bg-panel);
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
}

.svg-wrapper {
  width: 100%;
  max-width: 100%;
  display: flex;
  justify-content: center;
  align-items: center;
}

.rendered-diagram-img {
  width: 100%;
  max-width: 100%;
  height: auto;
  display: block;
  object-fit: contain;
  transition: transform 0.2s ease;
}

.diagram-body:hover .rendered-diagram-img {
  transform: scale(1.005);
}

.inspection-hint {
  margin-top: 0.75rem;
  font-size: 0.72rem;
  color: var(--q-ink-muted);
  text-align: center;
  opacity: 0.8;
  letter-spacing: 0.02em;
}

/* Lightbox Modal */
.excalidraw-lightbox-backdrop {
  position: fixed;
  inset: 0;
  z-index: 99999;
  background: rgba(10, 13, 20, 0.82);
  backdrop-filter: blur(4px);
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 1.5rem;
}

.lightbox-dialog {
  width: 95vw;
  max-width: 1300px;
  height: 90vh;
  background: var(--q-bg-panel);
  border: 2px solid var(--q-accent-primary);
  border-radius: 8px;
  box-shadow: 0 10px 30px rgba(0, 0, 0, 0.5);
  display: flex;
  flex-direction: column;
  overflow: hidden;
  font-family: var(--vp-font-family-mono, monospace);
}

.lightbox-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0.65rem 1.25rem;
  background: var(--q-bg-elevated);
  border-bottom: 2px solid var(--q-ink);
  gap: 1rem;
}

.dark .lightbox-header {
  border-bottom-color: var(--q-accent-primary);
}

.lightbox-title-group {
  display: flex;
  align-items: center;
  gap: 0.6rem;
}

.lightbox-title {
  font-size: 0.95rem;
  font-weight: 700;
  color: var(--q-ink);
}

.lightbox-controls {
  display: flex;
  align-items: center;
  gap: 0.5rem;
}

.zoom-ctrl-btn {
  padding: 0.25rem 0.65rem;
  font-size: 0.85rem;
  font-weight: 700;
  background: var(--q-bg-panel);
  border: 1px solid var(--q-ink-muted);
  color: var(--q-ink);
  cursor: pointer;
  border-radius: 2px;
}

.zoom-ctrl-btn:hover {
  background: var(--q-accent-primary);
  color: #ffffff;
}

.zoom-level {
  font-size: 0.8rem;
  font-weight: 600;
  color: var(--q-ink);
  min-width: 48px;
  text-align: center;
}

.close-btn {
  padding: 0.25rem 0.65rem;
  font-size: 0.85rem;
  font-weight: bold;
  background: var(--q-accent-secondary);
  color: #ffffff;
  border: none;
  cursor: pointer;
  border-radius: 2px;
}

.lightbox-viewport {
  flex: 1;
  width: 100%;
  height: 100%;
  overflow: hidden;
  padding: 1.5rem;
  background: var(--q-bg-canvas);
  display: flex;
  align-items: center;
  justify-content: center;
  position: relative;
  touch-action: none;
  user-select: none;
}

.lightbox-canvas {
  width: 100%;
  height: 100%;
  transform-origin: center center;
  transition: transform 0.08s ease-out;
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: default;
}

.lightbox-canvas.can-pan {
  cursor: grab;
}

.lightbox-canvas.is-dragging {
  cursor: grabbing;
  transition: none;
}

.lightbox-img {
  max-width: 92%;
  max-height: 76vh;
  width: auto;
  height: auto;
  object-fit: contain;
  background: var(--q-bg-panel);
  border: 1px solid var(--q-ink-muted);
  border-radius: 8px;
  padding: 1.25rem;
  box-shadow: 0 10px 30px rgba(0, 0, 0, 0.35);
  pointer-events: none;
}

.lightbox-footer {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0.5rem 1.25rem;
  background: var(--q-bg-elevated);
  border-top: 1px solid var(--q-ink-muted);
  font-size: 0.75rem;
  color: var(--q-ink-muted);
}

.footer-hint code {
  color: var(--q-accent-primary);
  background: transparent;
  padding: 0;
}

.footer-download-btn {
  font-weight: 600;
  color: var(--q-accent-primary) !important;
  text-decoration: underline;
}

/* Animations */
.fade-enter-active,
.fade-leave-active {
  transition: opacity 0.2s ease;
}

.fade-enter-from,
.fade-leave-to {
  opacity: 0;
}
</style>
