<template>
  <div class="erd-card">
    <div class="erd-toolbar">
      <div class="erd-title-group">
        <span class="erd-badge">INTERACTIVE RELATIONAL SCHEMA</span>
        <h3 class="erd-heading">QuartzDB Entity-Relationship Diagram (ERD)</h3>
      </div>
      <div class="erd-actions">
        <button 
          v-for="t in tables" 
          :key="t.id"
          :class="['filter-pill', { active: activeTable === t.id }]"
          @click="selectTable(t.id)"
        >
          {{ t.id }}
        </button>
        <button v-if="activeTable" class="filter-pill reset" @click="activeTable = null">
          ✕ Clear Focus
        </button>
      </div>
    </div>

    <!-- ERD Canvas -->
    <div class="erd-canvas">
      <div class="tables-grid">
        <div 
          v-for="t in tables" 
          :key="t.id" 
          :class="['erd-table-box', { highlighted: isHighlighted(t.id), dim: isDimmed(t.id) }]"
          @mouseenter="hoverTable = t.id"
          @mouseleave="hoverTable = null"
        >
          <div class="table-box-header">
            <span class="table-icon">🗄️</span>
            <span class="table-box-title">{{ t.id }}</span>
            <span class="table-box-rows">{{ t.rowCount }}</span>
          </div>
          <div class="table-box-body">
            <div 
              v-for="col in t.columns" 
              :key="col.name"
              :class="['col-row', { pk: col.pk, fk: col.fk, active: isColActive(t.id, col.name) }]"
            >
              <div class="col-left">
                <span v-if="col.pk" class="col-indicator pk" title="Primary Key">🔑</span>
                <span v-else-if="col.fk" class="col-indicator fk" :title="'FK to ' + col.fkTarget">🔗</span>
                <span v-else class="col-indicator bullet">•</span>
                <span class="col-name">{{ col.name }}</span>
              </div>
              <span class="col-type">{{ col.type }}</span>
            </div>
          </div>
        </div>
      </div>

      <!-- Relationships Inspector -->
      <div class="relationships-footer">
        <span class="rel-label">Active Relationships:</span>
        <div class="rel-list">
          <div 
            v-for="r in relationships" 
            :key="r.from + '->' + r.to"
            :class="['rel-chip', { active: isRelActive(r) }]"
          >
            <span><code>{{ r.from }}</code></span>
            <span class="arrow-symbol">⟵ 1:N ⟶</span>
            <span><code>{{ r.to }}</code></span>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref } from 'vue'

const activeTable = ref(null)
const hoverTable = ref(null)

const tables = [
  {
    id: 'users',
    rowCount: '120k rows',
    columns: [
      { name: 'id', type: 'INT64', pk: true },
      { name: 'email', type: 'TEXT', pii: true },
      { name: 'full_name', type: 'TEXT', pii: true },
      { name: 'created_at', type: 'INT64' },
      { name: 'role', type: 'TEXT' }
    ]
  },
  {
    id: 'orders',
    rowCount: '450k rows',
    columns: [
      { name: 'id', type: 'INT64', pk: true },
      { name: 'user_id', type: 'INT64', fk: true, fkTarget: 'users.id' },
      { name: 'total_cents', type: 'INT64' },
      { name: 'status', type: 'TEXT' },
      { name: 'placed_at', type: 'INT64' }
    ]
  },
  {
    id: 'products',
    rowCount: '8.5k rows',
    columns: [
      { name: 'id', type: 'INT64', pk: true },
      { name: 'title', type: 'TEXT' },
      { name: 'price_cents', type: 'INT64' },
      { name: 'stock_quantity', type: 'INT64' },
      { name: 'description', type: 'TEXT' }
    ]
  },
  {
    id: 'line_items',
    rowCount: '1.2M rows',
    columns: [
      { name: 'id', type: 'INT64', pk: true },
      { name: 'order_id', type: 'INT64', fk: true, fkTarget: 'orders.id' },
      { name: 'product_id', type: 'INT64', fk: true, fkTarget: 'products.id' },
      { name: 'quantity', type: 'INT64' }
    ]
  }
]

const relationships = [
  { from: 'users.id', to: 'orders.user_id', tables: ['users', 'orders'] },
  { from: 'orders.id', to: 'line_items.order_id', tables: ['orders', 'line_items'] },
  { from: 'products.id', to: 'line_items.product_id', tables: ['products', 'line_items'] }
]

const selectTable = (id) => {
  if (activeTable.value === id) {
    activeTable.value = null
  } else {
    activeTable.value = id
  }
}

const currentFocused = () => hoverTable.value || activeTable.value

const isHighlighted = (id) => {
  const focused = currentFocused()
  if (!focused) return false
  if (focused === id) return true
  return relationships.some(r => r.tables.includes(focused) && r.tables.includes(id))
}

const isDimmed = (id) => {
  const focused = currentFocused()
  if (!focused) return false
  return !isHighlighted(id)
}

const isRelActive = (r) => {
  const focused = currentFocused()
  if (!focused) return false
  return r.tables.includes(focused)
}

const isColActive = (tableId, colName) => {
  const focused = currentFocused()
  if (!focused) return false
  const full = `${tableId}.${colName}`
  return relationships.some(r => r.tables.includes(focused) && (r.from === full || r.to === full))
}
</script>

<style scoped>
.erd-card {
  border: 1px solid var(--vp-c-border);
  background: var(--vp-c-bg-elv);
  border-radius: 8px;
  margin: 2rem 0;
  overflow: hidden;
  box-shadow: var(--vp-shadow-1);
}

.erd-toolbar {
  padding: 14px 18px;
  background: var(--vp-c-bg-soft);
  border-bottom: 1px solid var(--vp-c-border);
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 12px;
}

.erd-badge {
  font-family: var(--vp-font-family-mono);
  font-size: 0.65rem;
  font-weight: 700;
  color: var(--vp-c-brand-1);
  letter-spacing: 0.08em;
}

.erd-heading {
  margin: 2px 0 0 0;
  font-size: 1.1rem;
  font-family: var(--vp-font-family-sans);
  color: var(--vp-c-text-1);
}

.erd-actions {
  display: flex;
  gap: 6px;
  flex-wrap: wrap;
}

.filter-pill {
  border: 1px solid var(--vp-c-border);
  background: var(--vp-c-bg);
  padding: 4px 10px;
  border-radius: 4px;
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  cursor: pointer;
  color: var(--vp-c-text-1);
  transition: all 0.15s ease;
}

.filter-pill:hover,
.filter-pill.active {
  border-color: var(--vp-c-brand-1);
  background: var(--vp-c-brand-1);
  color: #fff;
}

.filter-pill.reset {
  background: transparent;
  color: var(--vp-c-text-3);
  border-style: dashed;
}

.erd-canvas {
  padding: 1.5rem;
  background: var(--vp-c-bg);
}

.tables-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(230px, 1fr));
  gap: 1.25rem;
  margin-bottom: 1.5rem;
}

.erd-table-box {
  background: var(--vp-c-bg-elv);
  border: 1px solid var(--vp-c-border);
  border-radius: 6px;
  overflow: hidden;
  box-shadow: var(--vp-shadow-1);
  transition: all 0.25s ease;
}

.erd-table-box.highlighted {
  border-color: var(--vp-c-brand-1);
  box-shadow: 0 0 0 2px var(--vp-c-brand-1), 0 4px 14px rgba(0,0,0,0.12);
  transform: translateY(-2px);
}

.erd-table-box.dim {
  opacity: 0.45;
  filter: grayscale(0.6);
}

.table-box-header {
  padding: 8px 12px;
  background: var(--vp-c-bg-soft);
  border-bottom: 1px solid var(--vp-c-border);
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.table-box-title {
  font-family: var(--vp-font-family-mono);
  font-weight: 700;
  font-size: 0.9rem;
  color: var(--vp-c-text-1);
  flex: 1;
  margin-left: 6px;
}

.table-box-rows {
  font-family: var(--vp-font-family-mono);
  font-size: 0.7rem;
  color: var(--vp-c-text-3);
}

.table-box-body {
  padding: 6px 0;
}

.col-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 5px 12px;
  font-size: 0.8rem;
  font-family: var(--vp-font-family-mono);
  color: var(--vp-c-text-2);
  transition: background 0.15s ease;
}

.col-row:hover {
  background: rgba(0, 0, 0, 0.03);
}

.col-row.active {
  background: var(--vp-c-brand-soft);
  color: var(--vp-c-brand-1);
  font-weight: 700;
}

.col-left {
  display: flex;
  align-items: center;
  gap: 6px;
}

.col-indicator {
  font-size: 0.75rem;
}

.col-type {
  font-size: 0.7rem;
  color: var(--vp-c-text-3);
}

.relationships-footer {
  border-top: 1px solid var(--vp-c-border);
  padding-top: 1rem;
  display: flex;
  align-items: center;
  flex-wrap: wrap;
  gap: 10px;
}

.rel-label {
  font-family: var(--vp-font-family-sans);
  font-size: 0.8rem;
  font-weight: 600;
  color: var(--vp-c-text-3);
}

.rel-list {
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
}

.rel-chip {
  background: var(--vp-c-bg-elv);
  border: 1px solid var(--vp-c-border);
  padding: 4px 10px;
  border-radius: 4px;
  font-size: 0.75rem;
  font-family: var(--vp-font-family-mono);
  display: flex;
  align-items: center;
  gap: 6px;
  transition: all 0.2s ease;
}

.rel-chip.active {
  border-color: var(--vp-c-brand-1);
  background: var(--vp-c-brand-soft);
  color: var(--vp-c-brand-1);
  font-weight: 600;
}

.arrow-symbol {
  color: var(--vp-c-text-3);
  font-size: 0.7rem;
}
</style>
