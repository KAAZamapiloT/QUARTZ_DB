<template>
  <div class="config-explorer">
    <div class="config-controls">
      <div class="search-input-wrap">
        <span class="search-icon">🔍</span>
        <input 
          v-model="searchTerm" 
          type="text" 
          placeholder="Filter settings by name, category, or description..."
          class="config-search"
        />
      </div>
      <div class="filter-group">
        <select v-model="selectedCategory" class="config-select">
          <option value="ALL">All Categories</option>
          <option value="Storage">Storage & Pager</option>
          <option value="Index">B+ Tree Index</option>
          <option value="Execution">Query Execution</option>
          <option value="Memory">Memory & Cache</option>
        </select>
        <button 
          :class="['filter-btn', { active: onlyRestartRequired }]" 
          @click="onlyRestartRequired = !onlyRestartRequired"
        >
          {{ onlyRestartRequired ? '✓ Restart Required Only' : 'Show All' }}
        </button>
      </div>
    </div>

    <div class="config-meta-bar">
      <span>Showing <strong>{{ filteredSettings.length }}</strong> of {{ settings.length }} configuration parameters</span>
      <span v-if="searchTerm" class="clear-link" @click="searchTerm = ''">Clear search</span>
    </div>

    <div class="config-table-wrap">
      <table class="config-table">
        <thead>
          <tr>
            <th style="width: 220px;">Setting</th>
            <th style="width: 100px;">Type</th>
            <th style="width: 120px;">Default</th>
            <th style="width: 140px;">Range</th>
            <th style="width: 110px;">Reload</th>
            <th>Description & Engine Impact</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="s in filteredSettings" :key="s.name">
            <td class="setting-name-cell">
              <code>{{ s.name }}</code>
              <span class="category-pill">{{ s.category }}</span>
            </td>
            <td><span class="type-badge">{{ s.type }}</span></td>
            <td><code>{{ s.default }}</code></td>
            <td class="range-cell">{{ s.range }}</td>
            <td>
              <span :class="['reload-badge', s.dynamic ? 'dynamic' : 'restart']">
                {{ s.dynamic ? '⚡ Dynamic' : '🔄 Restart' }}
              </span>
            </td>
            <td class="desc-cell">{{ s.description }}</td>
          </tr>
          <tr v-if="filteredSettings.length === 0">
            <td colspan="6" class="empty-cell">
              No configuration settings match your search criteria.
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
const selectedCategory = ref('ALL')
const onlyRestartRequired = ref(false)

const settings = [
  {
    name: 'page_size',
    category: 'Storage',
    type: 'integer',
    default: '4096',
    range: '4096 (Fixed)',
    dynamic: false,
    description: 'Physical block allocation boundary in bytes. Aligns directly with operating system virtual memory pages and file system block boundaries.'
  },
  {
    name: 'bplus_tree.max_keys',
    category: 'Index',
    type: 'integer',
    default: '128',
    range: '32 .. 512',
    dynamic: false,
    description: 'Maximum key capacity per B+ Tree node before an automatic leaf or internal split is triggered. Determines node fan-out factor.'
  },
  {
    name: 'bplus_tree.min_keys',
    category: 'Index',
    type: 'integer',
    default: '64',
    range: '16 .. 256',
    dynamic: false,
    description: 'Minimum key occupancy threshold before triggering sibling key borrowing or node merging during record deletion.'
  },
  {
    name: 'storage.compaction_threshold',
    category: 'Storage',
    type: 'float',
    default: '0.20',
    range: '0.05 .. 0.50',
    dynamic: true,
    description: 'Fraction of fragmented gap bytes within a slotted page that triggers automatic in-place record compaction upon insert.'
  },
  {
    name: 'pager.cache_size_pages',
    category: 'Memory',
    type: 'integer',
    default: '1024',
    range: '64 .. 1048576',
    dynamic: true,
    description: 'Maximum number of 4KB pages held concurrently in memory cache before LRU buffer eviction occurs (default ~4MB).'
  },
  {
    name: 'pager.sync_mode',
    category: 'Storage',
    type: 'enum',
    default: 'NORMAL',
    range: 'OFF, NORMAL, FULL',
    dynamic: true,
    description: 'Controls fsync frequency after writing page blocks. FULL forces synchronous flush to physical disk on every commit.'
  },
  {
    name: 'sql.max_join_depth',
    category: 'Execution',
    type: 'integer',
    default: '8',
    range: '2 .. 32',
    dynamic: true,
    description: 'Maximum allowed nested relational JOIN clauses per query AST to prevent combinatorial planner exhaustion.'
  },
  {
    name: 'sql.point_lookup_optimization',
    category: 'Execution',
    type: 'boolean',
    default: 'true',
    range: 'true, false',
    dynamic: true,
    description: 'Enables executor O(log N) fast-path routing to B+ Tree primary key index when WHERE predicates target the primary key.'
  },
  {
    name: 'catalog.max_tables',
    category: 'Storage',
    type: 'integer',
    default: '256',
    range: '1 .. 1024',
    dynamic: false,
    description: 'Maximum number of table descriptors that can be serialized into Catalog Page 0 before requiring catalog page chaining.'
  }
]

const filteredSettings = computed(() => {
  return settings.filter(s => {
    if (selectedCategory.value !== 'ALL' && s.category !== selectedCategory.value) return false
    if (onlyRestartRequired.value && s.dynamic) return false

    if (searchTerm.value.trim() !== '') {
      const q = searchTerm.value.toLowerCase()
      const match = s.name.toLowerCase().includes(q) ||
                    s.category.toLowerCase().includes(q) ||
                    s.description.toLowerCase().includes(q)
      if (!match) return false
    }

    return true
  })
})
</script>

<style scoped>
.config-explorer {
  margin: 1.5rem 0;
}

.config-controls {
  display: flex;
  gap: 12px;
  flex-wrap: wrap;
  margin-bottom: 1rem;
}

.search-input-wrap {
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

.config-search {
  width: 100%;
  border: none;
  outline: none;
  background: transparent;
  font-family: var(--vp-font-family-sans);
  font-size: 0.85rem;
  color: var(--vp-c-text-1);
}

.filter-group {
  display: flex;
  gap: 8px;
}

.config-select {
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

.filter-btn {
  background: var(--vp-c-bg-elv);
  border: 1px solid var(--vp-c-border);
  border-radius: 6px;
  padding: 8px 14px;
  font-family: var(--vp-font-family-sans);
  font-size: 0.85rem;
  color: var(--vp-c-text-2);
  cursor: pointer;
  transition: all 0.15s ease;
}

.filter-btn.active {
  background: var(--vp-c-brand-1);
  border-color: var(--vp-c-brand-1);
  color: #fff;
}

.config-meta-bar {
  display: flex;
  justify-content: space-between;
  font-family: var(--vp-font-family-sans);
  font-size: 0.8rem;
  color: var(--vp-c-text-3);
  margin-bottom: 0.75rem;
}

.clear-link {
  color: var(--vp-c-brand-1);
  cursor: pointer;
  text-decoration: underline;
}

.config-table-wrap {
  overflow-x: auto;
  border: 1px solid var(--vp-c-border);
  border-radius: 8px;
  background: var(--vp-c-bg-elv);
  box-shadow: var(--vp-shadow-1);
}

.config-table {
  width: 100%;
  margin: 0;
  border-collapse: collapse;
}

.setting-name-cell {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.category-pill {
  font-family: var(--vp-font-family-sans);
  font-size: 0.65rem;
  color: var(--vp-c-text-3);
  text-transform: uppercase;
  letter-spacing: 0.05em;
}

.type-badge {
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  background: var(--vp-c-bg-soft);
  padding: 2px 6px;
  border-radius: 3px;
  border: 1px solid var(--vp-c-divider);
}

.range-cell {
  font-family: var(--vp-font-family-mono);
  font-size: 0.8rem;
  color: var(--vp-c-text-2);
}

.reload-badge {
  display: inline-block;
  font-family: var(--vp-font-family-sans);
  font-size: 0.7rem;
  font-weight: 600;
  padding: 2px 8px;
  border-radius: 4px;
}

.reload-badge.dynamic {
  background: rgba(16, 185, 129, 0.1);
  color: #059669;
  border: 1px solid rgba(16, 185, 129, 0.2);
}

.reload-badge.restart {
  background: rgba(239, 68, 68, 0.1);
  color: #dc2626;
  border: 1px solid rgba(239, 68, 68, 0.2);
}

.desc-cell {
  font-size: 0.85rem;
  color: var(--vp-c-text-2);
  line-height: 1.5;
}

.empty-cell {
  text-align: center;
  padding: 2rem;
  color: var(--vp-c-text-3);
  font-style: italic;
}
</style>
