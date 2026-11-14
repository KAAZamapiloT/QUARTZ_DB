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
          :href="`/diagrams/${name}.svg`" 
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
          :src="`/diagrams/${name}.svg`" 
          :alt="alt || title" 
          class="rendered-diagram-img" 
          loading="lazy"
        />
      </div>
      <div class="inspection-hint">
        <span>Click diagram to inspect full vector resolution (⤢)</span>
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

            <div class="lightbox-viewport">
              <div class="lightbox-canvas" :style="{ transform: `scale(${zoomScale})` }">
                <img 
                  :src="`/diagrams/${name}.svg`" 
                  :alt="alt || title" 
                  class="lightbox-img"
                />
              </div>
            </div>

            <div class="lightbox-footer">
              <span class="footer-hint">Source: <code>docs/diagrams/editable/{{ name }}.excalidraw</code> // Hand-drawn vector wireframe</span>
              <a :href="`/diagrams/${name}.svg`" download class="footer-download-btn">Download Vector SVG</a>
            </div>
          </div>
        </div>
      </Transition>
    </Teleport>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue'

const props = defineProps<{
  name: string
  title: string
  alt?: string
}>()

const isModalOpen = ref(false)
const zoomScale = ref(1.0)

function openModal() {
  isModalOpen.value = true
  zoomScale.value = 1.0
  document.body.style.overflow = 'hidden'
}

function closeModal() {
  isModalOpen.value = false
  document.body.style.overflow = ''
}

function zoomIn() {
  zoomScale.value = Math.min(zoomScale.value + 0.25, 3.0)
}

function zoomOut() {
  zoomScale.value = Math.max(zoomScale.value - 0.25, 0.5)
}

function resetZoom() {
  zoomScale.value = 1.0
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
  overflow: auto;
  padding: 2rem;
  background: var(--q-bg-canvas);
  display: flex;
  align-items: center;
  justify-content: center;
}

.lightbox-canvas {
  transform-origin: center center;
  transition: transform 0.15s ease-out;
  display: flex;
  align-items: center;
  justify-content: center;
  max-width: 100%;
}

.lightbox-img {
  max-width: 100%;
  max-height: 80vh;
  object-fit: contain;
  box-shadow: 0 4px 15px rgba(0, 0, 0, 0.15);
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
