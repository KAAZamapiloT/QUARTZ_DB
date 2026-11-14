<template>
  <div class="operator-callout">
    <div class="oc-avatar-frame" v-if="avatar">
      <img :src="avatar" :alt="title" class="oc-avatar" />
    </div>
    <div class="oc-avatar-frame oc-avatar-placeholder" v-else>
      <span class="oc-avatar-icon">⚙</span>
    </div>
    <div class="oc-body">
      <div class="oc-header">
        <span class="oc-code">{{ code }}</span>
        <span class="oc-title">{{ title }}</span>
      </div>
      <div class="oc-content">
        <slot />
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
withDefaults(defineProps<{
  title?: string
  code?: string
  avatar?: string
}>(), {
  title: 'STORAGE ENGINE INVARIANT NOTE',
  code: 'MON-01',
  avatar: ''
})
</script>

<style scoped>
.operator-callout {
  display: grid;
  grid-template-columns: 130px 1fr;
  gap: 0;
  border: 2px solid var(--d1-teal, #00D2B4);
  border-radius: 4px;
  margin: 1.5rem 0;
  overflow: hidden;
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.2);
}

.oc-avatar-frame {
  width: 130px;
  background: var(--d1-navy, #141824);
  display: flex;
  align-items: center;
  justify-content: center;
  border-right: 2px solid var(--d1-teal, #00D2B4);
  padding: 16px;
}

.oc-avatar-placeholder {
  font-size: 48px;
  color: var(--d1-teal, #00D2B4);
  opacity: 0.6;
}

.oc-avatar {
  width: 100%;
  height: auto;
  image-rendering: pixelated;
}

.oc-body {
  background: var(--d3-parchment, #EFE8D8);
  padding: 0;
}

.oc-header {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 10px 16px;
  background: rgba(29, 41, 57, 0.08);
  border-bottom: 1px solid rgba(29, 41, 57, 0.15);
}

.oc-code {
  font-family: var(--vp-font-family-mono);
  font-size: 11px;
  font-weight: 700;
  color: var(--d3-parchment, #EFE8D8);
  background: var(--d3-vermilion, #D1493E);
  padding: 2px 8px;
  border-radius: 3px;
  letter-spacing: 0.08em;
  text-transform: uppercase;
}

.oc-title {
  font-family: var(--vp-font-family-mono);
  font-size: 12px;
  font-weight: 700;
  color: var(--d3-navy, #1D2939);
  text-transform: uppercase;
  letter-spacing: 0.06em;
}

.oc-content {
  padding: 14px 16px;
  font-family: var(--vp-font-family-base);
  font-size: 15px;
  color: var(--d3-navy, #1D2939);
  line-height: 1.65;
}

/* Dark mode overrides */
.dark .oc-body {
  background: var(--vp-c-bg-soft);
}

.dark .oc-header {
  background: rgba(255, 255, 255, 0.04);
  border-bottom-color: var(--vp-c-divider);
}

.dark .oc-title {
  color: var(--vp-c-text-1);
}

.dark .oc-content {
  color: var(--vp-c-text-1);
}
</style>
