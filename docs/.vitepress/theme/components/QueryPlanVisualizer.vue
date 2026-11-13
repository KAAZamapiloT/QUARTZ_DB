<template>
  <div class="query-plan-card diagram-full">
    <div class="card-header">
      <div class="header-titles">
        <span class="plan-tag">INTERACTIVE EXPLAIN PLANNER</span>
        <h3 class="plan-heading">Execution Routing & Query Plan Visualizer</h3>
      </div>
      <div class="query-selector-tabs">
        <button 
          v-for="(q, idx) in queries" 
          :key="idx"
          :class="['query-tab', { active: selectedIndex === idx }]"
          @click="selectedIndex = idx"
        >
          <span :class="['mode-dot', q.type]"></span>
          {{ q.shortLabel }}
        </button>
      </div>
    </div>

    <!-- Active Query SQL Bar -->
    <div class="active-sql-bar">
      <span class="sql-prompt">quartz&gt; EXPLAIN</span>
      <code class="sql-code">{{ currentQuery.sql }}</code>
      <span :class="['plan-badge', currentQuery.type]">
        {{ currentQuery.type === 'index' ? '⚡ B+ Tree Index Fast-Path (O(log N))' : '🔄 Slotted-Page Table Scan (O(N))' }}
      </span>
    </div>

    <!-- Interactive Execution Tree -->
    <div class="plan-body">
      <!-- Left: Visual Tree -->
      <div class="tree-canvas">
        <div class="tree-node root-node">
          <div class="node-badge">{{ currentQuery.rootNode.badge }}</div>
          <div class="node-title">{{ currentQuery.rootNode.name }}</div>
          <div class="node-sub">{{ currentQuery.rootNode.detail }}</div>
          <div class="node-stats">
            <span>Cost: {{ currentQuery.rootNode.cost }}</span>
            <span>Rows: {{ currentQuery.rootNode.rows }}</span>
          </div>
        </div>

        <div class="tree-connector-line">
          <div class="line-stem"></div>
          <div class="line-arrow">↓</div>
        </div>

        <div class="tree-children-row">
          <div 
            v-for="(child, cIdx) in currentQuery.childNodes" 
            :key="cIdx"
            :class="['tree-node', 'child-node', child.kind]"
          >
            <div class="node-badge">{{ child.badge }}</div>
            <div class="node-title">{{ child.name }}</div>
            <div class="node-sub">{{ child.detail }}</div>
            <div class="node-stats">
              <span>Latency: {{ child.latency }}</span>
              <span>Pages: {{ child.pages }}</span>
            </div>
            <div class="node-mechanic-box" v-if="child.mechanic">
              <code>{{ child.mechanic }}</code>
            </div>
          </div>
        </div>
      </div>

      <!-- Right: Execution Metrics & Mechanical Explanation -->
      <div class="metrics-sidebar">
        <h4 class="sidebar-title">Execution Mechanics</h4>
        
        <div class="metric-row">
          <span class="metric-label">Access Method:</span>
          <span class="metric-val mono">{{ currentQuery.accessMethod }}</span>
        </div>

        <div class="metric-row">
          <span class="metric-label">Complexity:</span>
          <span class="metric-val highlight">{{ currentQuery.complexity }}</span>
        </div>

        <div class="metric-row">
          <span class="metric-label">Est. Latency:</span>
          <span class="metric-val text-green">{{ currentQuery.latency }}</span>
        </div>

        <div class="metric-row">
          <span class="metric-label">Buffer I/O:</span>
          <span class="metric-val">{{ currentQuery.pagesRead }} pages read</span>
        </div>

        <div class="narrative-box">
          <p class="narrative-text">{{ currentQuery.explanation }}</p>
        </div>

        <!-- CLI Output Preview -->
        <div class="cli-explain-output">
          <span class="cli-label">EXPLAIN TEXT PLAN:</span>
          <pre class="cli-pre"><code>{{ currentQuery.explainText }}</code></pre>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed } from 'vue'

const selectedIndex = ref(0)

const queries = [
  {
    shortLabel: 'Point Lookup (WHERE id = 42)',
    type: 'index',
    sql: 'SELECT * FROM users WHERE id = 42;',
    accessMethod: 'PrimaryIndex::find_by_primary_key()',
    complexity: 'O(log N)',
    latency: '~108 μs',
    pagesRead: '3',
    explanation: 'The query planner inspects the WHERE clause and identifies an equality filter on the single-column primary key. It completely bypasses linear table scans, navigating directly down the 3-level disk B+ Tree to extract the exact RecordID {page_id: 1, slot_index: 2}.',
    rootNode: {
      badge: 'PLAN ROOT',
      name: 'IndexPointFetch (Users)',
      detail: 'Projection: [id, name, email]',
      cost: '3.12',
      rows: '1'
    },
    childNodes: [
      {
        kind: 'index-scan',
        badge: 'INDEX PATH',
        name: 'B+ Tree Descent',
        detail: 'Binary search Page 3 (Root) → Page 5 (Internal) → Page 10 (Leaf)',
        latency: '74 μs',
        pages: '3 pages',
        mechanic: 'BplusTree::find(42) -> RecordID'
      },
      {
        kind: 'slot-fetch',
        badge: 'SLOT ACCESS',
        name: 'Slotted Page Fetch',
        detail: 'Direct memory slot read at page 1, slot 2. Deserializes single tuple.',
        latency: '34 μs',
        pages: '1 page',
        mechanic: 'Table::get_record(rid)'
      }
    ],
    explainText: `-> IndexPointFetch on users (PrimaryIndex: idx_users_pk)
   Filter: (users.id = 42)
   B+ Tree Height: 3 levels (Root: 3 -> Int: 5 -> Leaf: 10)
   Target RecordID: { page_id: 1, slot_index: 2 }
   Execution Time: 0.000108 sec (3 pages read)`
  },
  {
    shortLabel: 'Table Scan (WHERE role = "admin")',
    type: 'scan',
    sql: "SELECT * FROM users WHERE role = 'admin';",
    accessMethod: 'Table::scan() with Filter Evaluation',
    complexity: 'O(N)',
    latency: '~1,850 μs (1.8 ms)',
    pagesRead: '28',
    explanation: 'The filter targets an unindexed attribute ("role"). The planner must initiate a linear scan, starting at first_page_ (Page 1) and traversing the next_page linked list across all allocated data blocks. Every slot is decoded and tested against the predicate.',
    rootNode: {
      badge: 'PLAN ROOT',
      name: 'Filter & Project',
      detail: "Predicate: (users.role = 'admin')",
      cost: '142.80',
      rows: '14'
    },
    childNodes: [
      {
        kind: 'seq-scan',
        badge: 'PAGE CHAIN',
        name: 'Slotted Page Scan',
        detail: 'Walks next_page linked list (Pages 1 → 4 → 9 → 15 ...)',
        latency: '1,240 μs',
        pages: '28 pages',
        mechanic: 'while (next != NO_PAGE)'
      },
      {
        kind: 'filter-eval',
        badge: 'CPU FILTER',
        name: 'RecordCodec Unpack',
        detail: 'Unpacks slot arrays, converts Value variant, evaluates string equality.',
        latency: '610 μs',
        pages: '—',
        mechanic: 'eval_predicate(tuple)'
      }
    ],
    explainText: `-> Filter: (users.role = 'admin')
   -> TableScan on users (Slotted Page Linked Chain)
      Start Page: 1 (Catalog root_data_page)
      Pages Traversed: 28 data blocks
      Tuples Examined: 1,200 | Tuples Filtered: 14
      Execution Time: 0.001850 sec (28 pages read)`
  },
  {
    shortLabel: 'Relational Join (users JOIN orders)',
    type: 'join',
    sql: 'SELECT u.name, o.product FROM users u JOIN orders o ON u.id = o.user_id;',
    accessMethod: 'Hash / Index Nested-Loop Join',
    complexity: 'O(M · log N)',
    latency: '~3,420 μs (3.4 ms)',
    pagesRead: '42',
    explanation: 'Scans the orders table as outer driving stream. For each order, it leverages users primary key B+ Tree index to execute an O(log N) indexed point fetch matching orders.user_id, minimizing tuple comparisons.',
    rootNode: {
      badge: 'PLAN ROOT',
      name: 'IndexNestedLoopJoin',
      detail: 'Join Predicate: (u.id = o.user_id)',
      cost: '310.40',
      rows: '450'
    },
    childNodes: [
      {
        kind: 'seq-scan',
        badge: 'OUTER STREAM',
        name: 'Scan Orders Table',
        detail: 'Reads outer order records sequentially across slotted pages.',
        latency: '2,100 μs',
        pages: '35 pages',
        mechanic: 'Table::scan(orders)'
      },
      {
        kind: 'index-scan',
        badge: 'INNER PROBE',
        name: 'Probe Users Index',
        detail: 'O(log N) B+ Tree point lookup per order item using u.id.',
        latency: '1,320 μs',
        pages: '7 pages (cached)',
        mechanic: 'users_idx.find(o.user_id)'
      }
    ],
    explainText: `-> IndexNestedLoopJoin
   Condition: (u.id = o.user_id)
   -> TableScan on orders (Outer driving table, 450 rows)
   -> IndexLookup on users via PrimaryIndex (Inner probed table)
   Execution Time: 0.003420 sec (42 pages read)`
  }
]

const currentQuery = computed(() => queries[selectedIndex.value])
</script>

<style scoped>
.query-plan-card {
  border: 1px solid var(--qd-border, #ded8cb);
  background: var(--qd-paper-surface, #ffffff);
  border-radius: 8px;
  overflow: hidden;
  box-shadow: 0 4px 16px rgba(0,0,0,0.05);
  margin: 2rem auto;
}

.card-header {
  padding: 14px 18px;
  background: var(--qd-paper-alt, #eee9dc);
  border-bottom: 1px solid var(--qd-border, #ded8cb);
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 12px;
}

.plan-tag {
  font-family: var(--vp-font-family-mono);
  font-size: 0.65rem;
  font-weight: 700;
  color: var(--qd-accent, #b33927);
  letter-spacing: 0.08em;
  display: block;
}

.plan-heading {
  margin: 2px 0 0 0;
  font-size: 1.15rem;
  font-family: var(--vp-font-family-base);
  color: var(--qd-ink, #181716);
}

.query-selector-tabs {
  display: flex;
  gap: 6px;
  flex-wrap: wrap;
}

.query-tab {
  background: var(--qd-paper, #f7f3e9);
  border: 1px solid var(--qd-border, #ded8cb);
  border-radius: 4px;
  padding: 5px 12px;
  font-family: var(--vp-font-family-sans);
  font-size: 0.75rem;
  font-weight: 600;
  color: var(--qd-ink-muted, #5e5a54);
  cursor: pointer;
  display: flex;
  align-items: center;
  gap: 6px;
  transition: all 0.15s ease;
}

.query-tab.active {
  background: var(--qd-ink, #181716);
  color: var(--qd-paper, #f7f3e9);
  border-color: var(--qd-ink, #181716);
}

.mode-dot {
  width: 7px;
  height: 7px;
  border-radius: 50%;
}
.mode-dot.index { background: #10b981; }
.mode-dot.scan { background: #f59e0b; }
.mode-dot.join { background: #0284c7; }

/* Active SQL Bar */
.active-sql-bar {
  padding: 10px 18px;
  background: var(--qd-paper, #f7f3e9);
  border-bottom: 1px solid var(--qd-border, #ded8cb);
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-wrap: wrap;
  gap: 10px;
}

.sql-prompt {
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  font-weight: 700;
  color: var(--qd-accent, #b33927);
}

.sql-code {
  font-family: var(--vp-font-family-mono);
  font-size: 0.85rem;
  color: var(--qd-ink, #181716);
  background: transparent;
  padding: 0;
}

.plan-badge {
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  font-weight: 700;
  padding: 3px 8px;
  border-radius: 4px;
}

.plan-badge.index {
  background: rgba(16, 185, 129, 0.1);
  color: #059669;
  border: 1px solid rgba(16, 185, 129, 0.25);
}

.plan-badge.scan {
  background: rgba(245, 158, 11, 0.1);
  color: #d97706;
  border: 1px solid rgba(245, 158, 11, 0.25);
}

.plan-badge.join {
  background: rgba(2, 132, 199, 0.1);
  color: #0284c7;
  border: 1px solid rgba(2, 132, 199, 0.25);
}

/* Plan Body Grid */
.plan-body {
  display: grid;
  grid-template-columns: 1.25fr 1fr;
  gap: 20px;
  padding: 20px 18px;
  background: var(--qd-paper-surface, #ffffff);
}

@media (max-width: 820px) {
  .plan-body {
    grid-template-columns: 1fr;
  }
}

/* Tree Canvas */
.tree-canvas {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 10px 0;
}

.tree-node {
  background: var(--qd-paper, #f7f3e9);
  border: 1px solid var(--qd-border, #ded8cb);
  border-radius: 6px;
  padding: 10px 14px;
  box-shadow: 0 1px 4px rgba(0,0,0,0.04);
  width: 100%;
  max-width: 320px;
}

.tree-node.root-node {
  border-top: 3px solid var(--qd-accent, #b33927);
  text-align: center;
}

.node-badge {
  font-family: var(--vp-font-family-mono);
  font-size: 0.65rem;
  font-weight: 700;
  color: var(--qd-accent, #b33927);
  letter-spacing: 0.05em;
  margin-bottom: 2px;
}

.node-title {
  font-family: var(--vp-font-family-sans);
  font-weight: 700;
  font-size: 0.95rem;
  color: var(--qd-ink, #181716);
}

.node-sub {
  font-family: var(--vp-font-family-sans);
  font-size: 0.75rem;
  color: var(--qd-ink-muted, #5e5a54);
  margin-top: 2px;
}

.node-stats {
  display: flex;
  justify-content: center;
  gap: 12px;
  margin-top: 6px;
  font-family: var(--vp-font-family-mono);
  font-size: 0.7rem;
  color: var(--qd-ink-faint, #8e8a82);
}

.tree-connector-line {
  display: flex;
  flex-direction: column;
  align-items: center;
  margin: 6px 0;
}

.line-stem {
  width: 2px;
  height: 18px;
  background: var(--qd-border, #ded8cb);
}

.line-arrow {
  font-size: 0.8rem;
  color: var(--qd-border, #ded8cb);
  line-height: 1;
}

.tree-children-row {
  display: flex;
  gap: 12px;
  width: 100%;
}

.tree-node.child-node {
  flex: 1;
  max-width: none;
}

.tree-node.child-node.index-scan {
  border-top: 3px solid #10b981;
}

.tree-node.child-node.slot-fetch {
  border-top: 3px solid #0284c7;
}

.tree-node.child-node.seq-scan {
  border-top: 3px solid #f59e0b;
}

.tree-node.child-node.filter-eval {
  border-top: 3px solid #b33927;
}

.node-mechanic-box {
  margin-top: 8px;
  padding: 4px 6px;
  background: var(--qd-paper-alt, #eee9dc);
  border-radius: 4px;
  font-family: var(--vp-font-family-mono);
  font-size: 0.7rem;
  color: var(--qd-ink, #181716);
  text-align: center;
}

/* Metrics Sidebar */
.metrics-sidebar {
  border-left: 1px solid var(--qd-border, #ded8cb);
  padding-left: 18px;
  display: flex;
  flex-direction: column;
  gap: 8px;
}

@media (max-width: 820px) {
  .metrics-sidebar {
    border-left: none;
    border-top: 1px solid var(--qd-border, #ded8cb);
    padding-left: 0;
    padding-top: 16px;
  }
}

.sidebar-title {
  margin: 0 0 4px 0;
  font-family: var(--vp-font-family-sans);
  font-size: 0.9rem;
  font-weight: 700;
  color: var(--qd-ink, #181716);
  text-transform: uppercase;
  letter-spacing: 0.05em;
}

.metric-row {
  display: flex;
  justify-content: space-between;
  align-items: baseline;
  font-size: 0.8rem;
  border-bottom: 1px solid var(--qd-border-subtle, #eae4d8);
  padding: 4px 0;
}

.metric-label {
  color: var(--qd-ink-muted, #5e5a54);
}

.metric-val {
  font-weight: 600;
  color: var(--qd-ink, #181716);
}

.metric-val.mono {
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
}

.metric-val.highlight {
  color: var(--qd-accent, #b33927);
  font-weight: 700;
  font-family: var(--vp-font-family-mono);
}

.metric-val.text-green {
  color: #059669;
  font-weight: 700;
  font-family: var(--vp-font-family-mono);
}

.narrative-box {
  background: var(--qd-paper-alt, #eee9dc);
  border-radius: 4px;
  padding: 10px 12px;
  margin-top: 4px;
}

.narrative-text {
  margin: 0;
  font-size: 0.8rem;
  line-height: 1.5;
  color: var(--qd-ink-muted, #5e5a54);
}

.cli-explain-output {
  margin-top: 8px;
}

.cli-label {
  font-family: var(--vp-font-family-mono);
  font-size: 0.65rem;
  font-weight: 700;
  color: var(--qd-ink-faint, #8e8a82);
  display: block;
  margin-bottom: 3px;
}

.cli-pre {
  margin: 0;
  padding: 8px 10px;
  border-radius: 4px;
  background: var(--qd-paper, #f7f3e9);
  border: 1px solid var(--qd-border, #ded8cb);
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  line-height: 1.4;
  color: var(--qd-ink, #181716);
  overflow-x: auto;
}
</style>
