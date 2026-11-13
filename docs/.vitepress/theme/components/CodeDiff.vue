<template>
  <div class="diff-container">
    <div class="diff-header">
      <div class="diff-title-wrap">
        <span class="diff-badge">INTERACTIVE ARCHITECTURAL DIFF</span>
        <span class="diff-title">{{ title || 'Code Evolution' }}</span>
      </div>
      <div class="diff-toggle-btns">
        <button 
          :class="['toggle-btn', { active: currentMode === 'before' }]" 
          @click="currentMode = 'before'"
        >
          <span class="btn-dot red"></span> {{ beforeLabel || 'Naive Flat File' }}
        </button>
        <button 
          :class="['toggle-btn', { active: currentMode === 'after' }]" 
          @click="currentMode = 'after'"
        >
          <span class="btn-dot green"></span> {{ afterLabel || 'Slotted-Page Architecture' }}
        </button>
      </div>
    </div>

    <div class="diff-body">
      <div v-show="currentMode === 'before'" class="diff-slot-content before">
        <div class="mode-indicator before">
          <span>− BEFORE (Limitations: Fragmentation & Index Invalidation)</span>
        </div>
        <div class="slot-inner">
          <slot name="before"></slot>
        </div>
      </div>

      <div v-show="currentMode === 'after'" class="diff-slot-content after">
        <div class="mode-indicator after">
          <span>+ AFTER (Production QuartzDB: Slotted 4KB Page with Compaction)</span>
        </div>
        <div class="slot-inner">
          <slot name="after"></slot>
        </div>
      </div>
    </div>

    <div class="diff-footer" v-if="explanation">
      <p class="diff-explanation"><strong>Key Architectural Difference:</strong> {{ explanation }}</p>
    </div>
  </div>
</template>

<script setup>
import { ref } from 'vue'

defineProps({
  title: String,
  beforeLabel: String,
  afterLabel: String,
  explanation: String
})

const currentMode = ref('after')
</script>

<style scoped>
.diff-container {
  border: 1px solid var(--vp-c-border);
  border-radius: 8px;
  background: var(--vp-c-bg-elv);
  margin: 2rem 0;
  overflow: hidden;
  box-shadow: var(--vp-shadow-1);
}

.diff-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 12px;
  padding: 12px 16px;
  background: var(--vp-c-bg-soft);
  border-bottom: 1px solid var(--vp-c-border);
}

.diff-title-wrap {
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.diff-badge {
  font-family: var(--vp-font-family-mono);
  font-size: 0.65rem;
  font-weight: 700;
  color: var(--vp-c-brand-1);
  letter-spacing: 0.08em;
}

.diff-title {
  font-family: var(--vp-font-family-sans);
  font-weight: 600;
  font-size: 0.95rem;
  color: var(--vp-c-text-1);
}

.diff-toggle-btns {
  display: flex;
  background: var(--vp-c-bg);
  padding: 3px;
  border-radius: 6px;
  border: 1px solid var(--vp-c-border);
  gap: 4px;
}

.toggle-btn {
  background: transparent;
  border: none;
  font-family: var(--vp-font-family-sans);
  font-size: 0.75rem;
  font-weight: 600;
  padding: 5px 12px;
  border-radius: 4px;
  cursor: pointer;
  display: flex;
  align-items: center;
  gap: 6px;
  color: var(--vp-c-text-2);
  transition: all 0.2s ease;
}

.toggle-btn.active {
  background: var(--vp-c-bg-elv);
  color: var(--vp-c-text-1);
  box-shadow: var(--vp-shadow-1);
}

.btn-dot {
  width: 7px;
  height: 7px;
  border-radius: 50%;
}
.btn-dot.red { background: #ef4444; }
.btn-dot.green { background: #10b981; }

.mode-indicator {
  padding: 6px 16px;
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  font-weight: 700;
  letter-spacing: 0.05em;
}

.mode-indicator.before {
  background: rgba(239, 68, 68, 0.08);
  color: #dc2626;
  border-bottom: 1px solid rgba(239, 68, 68, 0.2);
}

.mode-indicator.after {
  background: rgba(16, 185, 129, 0.08);
  color: #059669;
  border-bottom: 1px solid rgba(16, 185, 129, 0.2);
}

.slot-inner :deep(div[class*='language-']) {
  margin: 0 !important;
  border-radius: 0 !important;
}

.diff-footer {
  border-top: 1px solid var(--vp-c-border);
  padding: 10px 16px;
  background: var(--vp-c-bg-soft);
}

.diff-explanation {
  margin: 0;
  font-size: 0.85rem;
  color: var(--vp-c-text-2);
}
</style>
