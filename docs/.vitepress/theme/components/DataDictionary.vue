<template>
  <div class="dictionary-wrapper">
    <!-- Filter Controls Bar -->
    <div class="filter-bar">
      <div class="search-box">
        <span class="search-icon">🔍</span>
        <input 
          v-model="searchTerm" 
          type="text" 
          placeholder="Search columns, descriptions, or data types..."
          class="search-input"
        />
      </div>

      <div class="filter-selects">
        <select v-model="selectedTable" class="filter-dropdown">
          <option value="ALL">All Tables ({{ tablesList.length }})</option>
          <option v-for="t in tablesList" :key="t" :value="t">{{ t }}</option>
        </select>

        <select v-model="selectedConstraint" class="filter-dropdown">
          <option value="ALL">All Constraints</option>
          <option value="PK">Primary Keys 🔑</option>
          <option value="FK">Foreign Keys 🔗</option>
          <option value="PII">PII Attributes ⚠️</option>
          <option value="NULL">Nullable</option>
        </select>
      </div>
    </div>

    <!-- Stats Summary -->
    <div class="dict-stats">
      <span>Showing <strong>{{ filteredColumns.length }}</strong> of {{ allColumns.length }} total columns</span>
      <span class="copy-hint">Tip: Click on any column name to copy it to clipboard</span>
    </div>

    <!-- Data Dictionary Table -->
    <div class="table-container">
      <table class="dictionary-table">
        <thead>
          <tr>
            <th style="width: 140px;">Table</th>
            <th style="width: 180px;">Column Name</th>
            <th style="width: 120px;">Data Type</th>
            <th style="width: 150px;">Constraints</th>
            <th>Description & Business Semantics</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="col in filteredColumns" :key="col.table + '.' + col.name">
            <td>
              <a :href="'/tables/' + col.table" class="table-link">
                <code>{{ col.table }}</code>
              </a>
            </td>
            <td>
              <span 
                class="col-copy-btn" 
                @click="copyText(col.name)" 
                :title="'Click to copy ' + col.name"
              >
                <code>{{ col.name }}</code>
                <span class="copy-icon" :class="{ copied: copiedCol === col.name }">
                  {{ copiedCol === col.name ? '✓ Copied' : '📋' }}
                </span>
              </span>
            </td>
            <td>
              <span class="type-tag">{{ col.type }}</span>
            </td>
            <td>
              <div class="badge-cluster">
                <span v-if="col.is_pk" class="badge-pk">PK 🔑</span>
                <span v-if="col.is_fk" class="badge-fk" :title="'References ' + col.fk_ref">FK 🔗</span>
                <span v-if="col.is_pii" class="badge-pii">PII ⚠️</span>
                <span v-if="col.nullable" class="badge-null">NULL</span>
                <span v-else-if="!col.is_pk" class="badge-not-null">NOT NULL</span>
              </div>
            </td>
            <td class="desc-cell">
              {{ col.description }}
              <div v-if="col.fk_ref" class="fk-ref-hint">
                ↳ References <code>{{ col.fk_ref }}</code>
              </div>
            </td>
          </tr>
          <tr v-if="filteredColumns.length === 0">
            <td colspan="5" class="empty-state">
              No columns match your current search and filter criteria.
            </td>
          </tr>
        </tbody>
      </table>
    </div>
  </div>
</template>

<script setup>
import { ref, computed } from 'vue'

const searchTerm = ref('')
const selectedTable = ref('ALL')
const selectedConstraint = ref('ALL')
const copiedCol = ref(null)

const allColumns = [
  // users
  { table: 'users', name: 'id', type: 'INT64', is_pk: true, is_fk: false, is_pii: false, nullable: false, description: 'Unique user identifier and B+ Tree index primary key.' },
  { table: 'users', name: 'email', type: 'TEXT', is_pk: false, is_fk: false, is_pii: true, nullable: false, description: 'User login email address. Subject to strict GDPR/CCPA PII policies.' },
  { table: 'users', name: 'full_name', type: 'TEXT', is_pk: false, is_fk: false, is_pii: true, nullable: false, description: 'Customer legal name for shipping and invoicing.' },
  { table: 'users', name: 'created_at', type: 'INT64', is_pk: false, is_fk: false, is_pii: false, nullable: false, description: 'Epoch millisecond timestamp of account registration.' },
  { table: 'users', name: 'role', type: 'TEXT', is_pk: false, is_fk: false, is_pii: false, nullable: true, description: 'RBAC authorization level: "admin", "customer", "auditor".' },

  // orders
  { table: 'orders', name: 'id', type: 'INT64', is_pk: true, is_fk: false, is_pii: false, nullable: false, description: 'Unique sequential order transaction identifier.' },
  { table: 'orders', name: 'user_id', type: 'INT64', is_pk: false, is_fk: true, fk_ref: 'users.id', is_pii: false, nullable: false, description: 'Foreign key referencing ordering customer.' },
  { table: 'orders', name: 'total_cents', type: 'INT64', is_pk: false, is_fk: false, is_pii: false, nullable: false, description: 'Gross order transaction value in integer cents (USD).' },
  { table: 'orders', name: 'status', type: 'TEXT', is_pk: false, is_fk: false, is_pii: false, nullable: false, description: 'Order lifecycle state: "PENDING", "PAID", "SHIPPED", "CANCELLED".' },
  { table: 'orders', name: 'placed_at', type: 'INT64', is_pk: false, is_fk: false, is_pii: false, nullable: false, description: 'Timestamp when customer finalized checkout.' },

  // products
  { table: 'products', name: 'id', type: 'INT64', is_pk: true, is_fk: false, is_pii: false, nullable: false, description: 'Unique item SKU identifier.' },
  { table: 'products', name: 'title', type: 'TEXT', is_pk: false, is_fk: false, is_pii: false, nullable: false, description: 'Customer-facing product catalog title.' },
  { table: 'products', name: 'price_cents', type: 'INT64', is_pk: false, is_fk: false, is_pii: false, nullable: false, description: 'Unit price in integer cents.' },
  { table: 'products', name: 'stock_quantity', type: 'INT64', is_pk: false, is_fk: false, is_pii: false, nullable: false, description: 'Available warehouse inventory count.' },
  { table: 'products', name: 'description', type: 'TEXT', is_pk: false, is_fk: false, is_pii: false, nullable: true, description: 'Extended Markdown documentation of product attributes.' },

  // line_items
  { table: 'line_items', name: 'id', type: 'INT64', is_pk: true, is_fk: false, is_pii: false, nullable: false, description: 'Unique item ledger line identifier.' },
  { table: 'line_items', name: 'order_id', type: 'INT64', is_pk: false, is_fk: true, fk_ref: 'orders.id', is_pii: false, nullable: false, description: 'Foreign key to parent purchase order.' },
  { table: 'line_items', name: 'product_id', type: 'INT64', is_pk: false, is_fk: true, fk_ref: 'products.id', is_pii: false, nullable: false, description: 'Foreign key to purchased product catalog item.' },
  { table: 'line_items', name: 'quantity', type: 'INT64', is_pk: false, is_fk: false, is_pii: false, nullable: false, description: 'Number of units purchased in this item line.' }
]

const tablesList = ['users', 'orders', 'products', 'line_items']

const filteredColumns = computed(() => {
  return allColumns.filter(col => {
    // Table filter
    if (selectedTable.value !== 'ALL' && col.table !== selectedTable.value) return false

    // Constraint filter
    if (selectedConstraint.value === 'PK' && !col.is_pk) return false
    if (selectedConstraint.value === 'FK' && !col.is_fk) return false
    if (selectedConstraint.value === 'PII' && !col.is_pii) return false
    if (selectedConstraint.value === 'NULL' && !col.nullable) return false

    // Search filter
    if (searchTerm.value.trim() !== '') {
      const q = searchTerm.value.toLowerCase()
      const match = col.name.toLowerCase().includes(q) ||
                    col.table.toLowerCase().includes(q) ||
                    col.type.toLowerCase().includes(q) ||
                    col.description.toLowerCase().includes(q)
      if (!match) return false
    }

    return true
  })
})

const copyText = (txt) => {
  navigator.clipboard.writeText(txt)
  copiedCol.value = txt
  setTimeout(() => {
    if (copiedCol.value === txt) copiedCol.value = null
  }, 2000)
}
</script>

<style scoped>
.dictionary-wrapper {
  margin: 1.5rem 0;
}

.filter-bar {
  display: flex;
  justify-content: space-between;
  gap: 12px;
  flex-wrap: wrap;
  margin-bottom: 1rem;
}

.search-box {
  flex: 1;
  min-width: 260px;
  display: flex;
  align-items: center;
  background: var(--vp-c-bg-elv);
  border: 1px solid var(--vp-c-border);
  border-radius: 6px;
  padding: 8px 12px;
  gap: 8px;
}

.search-icon {
  font-size: 0.9rem;
  color: var(--vp-c-text-3);
}

.search-input {
  width: 100%;
  border: none;
  outline: none;
  background: transparent;
  font-family: var(--vp-font-family-sans);
  font-size: 0.85rem;
  color: var(--vp-c-text-1);
}

.filter-selects {
  display: flex;
  gap: 8px;
}

.filter-dropdown {
  background: var(--vp-c-bg-elv);
  border: 1px solid var(--vp-c-border);
  border-radius: 6px;
  padding: 8px 12px;
  font-family: var(--vp-font-family-sans);
  font-size: 0.85rem;
  color: var(--vp-c-text-1);
  outline: none;
  cursor: pointer;
}

.dict-stats {
  display: flex;
  justify-content: space-between;
  font-family: var(--vp-font-family-sans);
  font-size: 0.8rem;
  color: var(--vp-c-text-3);
  margin-bottom: 0.75rem;
}

.copy-hint {
  font-style: italic;
}

.table-container {
  overflow-x: auto;
  border: 1px solid var(--vp-c-border);
  border-radius: 8px;
  background: var(--vp-c-bg-elv);
  box-shadow: var(--vp-shadow-1);
}

.dictionary-table {
  margin: 0;
  width: 100%;
}

.table-link {
  text-decoration: none;
  font-weight: 600;
  color: var(--vp-c-brand-1);
}
.table-link:hover {
  text-decoration: underline;
}

.col-copy-btn {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  cursor: pointer;
  padding: 2px 6px;
  border-radius: 4px;
  transition: all 0.15s ease;
  background: var(--vp-c-bg-soft);
}

.col-copy-btn:hover {
  background: var(--vp-c-brand-soft);
  color: var(--vp-c-brand-1);
}

.copy-icon {
  font-size: 0.75rem;
  color: var(--vp-c-text-3);
  opacity: 0.6;
}

.copy-icon.copied {
  color: #10b981;
  font-weight: 700;
  opacity: 1;
}

.type-tag {
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  background: var(--vp-c-bg-soft);
  padding: 2px 6px;
  border-radius: 3px;
  border: 1px solid var(--vp-c-divider);
}

.badge-cluster {
  display: flex;
  gap: 4px;
  flex-wrap: wrap;
}

.badge-not-null {
  background: var(--vp-c-bg-soft);
  color: var(--vp-c-text-2);
  padding: 2px 6px;
  border-radius: 4px;
  font-family: var(--vp-font-family-mono);
  font-size: 11px;
  font-weight: 500;
  border: 1px solid var(--vp-c-divider);
}

.desc-cell {
  font-size: 0.85rem;
  color: var(--vp-c-text-2);
}

.fk-ref-hint {
  font-size: 0.75rem;
  color: #0369a1;
  margin-top: 4px;
  font-family: var(--vp-font-family-sans);
}

.empty-state {
  text-align: center;
  padding: 2.5rem;
  color: var(--vp-c-text-3);
  font-style: italic;
}
</style>
