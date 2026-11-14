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
  border: var(--q-border-style);
  border-radius: 0;
  margin: 1.75rem 0;
  overflow: hidden;
  box-shadow: var(--q-shadow);
}

.oc-avatar-frame {
  width: 130px;
  background: var(--q-bg-canvas);
  display: flex;
  align-items: center;
  justify-content: center;
  border-right: var(--q-border-style);
  padding: 16px;
}

.oc-avatar-placeholder {
  font-size: 48px;
  color: var(--q-accent-primary);
  opacity: 0.85;
}

.oc-avatar {
  width: 100%;
  height: auto;
  image-rendering: pixelated;
}

.oc-body {
  background: var(--q-bg-panel);
  padding: 0;
}

.oc-header {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 10px 16px;
  background: var(--q-bg-elevated);
  border-bottom: 1.5px solid var(--q-ink-muted);
}

.oc-code {
  font-family: var(--vp-font-family-mono);
  font-size: 11px;
  font-weight: 700;
  color: var(--q-bg-panel);
  background: var(--q-accent-secondary);
  padding: 2px 8px;
  border-radius: 0;
  letter-spacing: 0.08em;
  text-transform: uppercase;
}

.oc-title {
  font-family: var(--vp-font-family-mono);
  font-size: 12px;
  font-weight: 700;
  color: var(--q-ink);
  text-transform: uppercase;
  letter-spacing: 0.06em;
}

.oc-content {
  padding: 14px 16px;
  font-family: var(--vp-font-family-base);
  font-size: 15px;
  color: var(--q-ink);
  line-height: 1.65;
}
</style>
