<template>
  <div class="railroad-container">
    <div class="railroad-header">
      <span class="railroad-tag">SYNTAX DIAGRAM</span>
      <span class="railroad-title">{{ title }}</span>
    </div>
    <div class="railroad-body">
      <svg :viewBox="viewBox" class="railroad-svg">
        <slot />
      </svg>
    </div>
    <div class="railroad-hint" v-if="hint">
      <span>{{ hint }}</span>
    </div>
  </div>
</template>

<script setup>
defineProps({
  title: String,
  viewBox: {
    type: String,
    default: '0 0 780 120'
  },
  hint: {
    type: String,
    default: 'Click on any clause element to jump directly to its specification anchor.'
  }
})
</script>

<style scoped>
.railroad-container {
  border: 1px solid var(--vp-c-border);
  background: var(--vp-c-bg-elv);
  border-radius: 8px;
  margin: 1.5rem 0 2rem;
  overflow: hidden;
  box-shadow: var(--vp-shadow-1);
}

.railroad-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 8px 14px;
  background: var(--vp-c-bg-soft);
  border-bottom: 1px solid var(--vp-c-border);
}

.railroad-tag {
  font-family: var(--vp-font-family-mono);
  font-size: 0.65rem;
  font-weight: 700;
  color: var(--vp-c-brand-1);
  letter-spacing: 0.08em;
}

.railroad-title {
  font-family: var(--vp-font-family-sans);
  font-weight: 600;
  font-size: 0.85rem;
  color: var(--vp-c-text-1);
}

.railroad-body {
  padding: 1.25rem 1rem;
  background: var(--vp-c-bg);
  overflow-x: auto;
  display: flex;
  justify-content: center;
}

.railroad-svg {
  width: 100%;
  max-width: 780px;
  height: auto;
}

.railroad-hint {
  padding: 6px 14px;
  border-top: 1px solid var(--vp-c-border);
  background: var(--vp-c-bg-soft);
  font-family: var(--vp-font-family-sans);
  font-size: 0.75rem;
  color: var(--vp-c-text-3);
  font-style: italic;
}

:deep(.track) {
  stroke: var(--vp-c-border);
  stroke-width: 2;
  fill: none;
}

:deep(.token-box) {
  fill: var(--vp-c-bg-elv);
  stroke: var(--vp-c-border);
  stroke-width: 1.5;
  transition: all 0.2s ease;
  cursor: pointer;
}

:deep(.token-box.keyword) {
  fill: var(--vp-c-brand-soft);
  stroke: var(--vp-c-brand-1);
}

:deep(a:hover .token-box) {
  stroke: var(--vp-c-brand-1);
  filter: drop-shadow(0 2px 4px rgba(0,0,0,0.1));
}

:deep(.token-text) {
  font-family: var(--vp-font-family-mono);
  font-size: 11px;
  font-weight: 600;
  fill: var(--vp-c-text-1);
  text-anchor: middle;
  dominant-baseline: central;
}

:deep(.token-text.keyword) {
  fill: var(--vp-c-brand-1);
  font-weight: 700;
}
</style>
