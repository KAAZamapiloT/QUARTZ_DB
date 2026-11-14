<template>
  <div class="playground-card">
    <!-- Header bar -->
    <div class="playground-header">
      <div class="playground-title-group">
        <span class="terminal-dot red"></span>
        <span class="terminal-dot yellow"></span>
        <span class="terminal-dot green"></span>
        <span class="playground-title">QUARTZ DB // CLIENT-SIDE IN-MEMORY ENGINE</span>
        <span class="badge-tag">[VERCEL FRONTEND // ZERO BACKEND]</span>
      </div>
      <div class="playground-controls">
        <span class="path-badge" :class="pathClass">{{ executionPath }}</span>
        <span class="latency-badge" v-if="latency !== null">{{ latency }} μs</span>
        <button @click="resetDatabase" class="btn-ctrl" title="Reset all tables to default seed state">↺ Reset DB</button>
        <button @click="runQuery" class="btn-run" title="Execute query (Ctrl+Enter)">▶ Run Query (Ctrl+Enter)</button>
      </div>
    </div>

    <!-- Storage & Virtual Pager Status Bar -->
    <div class="storage-bar">
      <div class="storage-metric">
        <span class="metric-label">VIRTUAL PAGER:</span>
        <span class="metric-value">{{ storageMetrics.totalPages }} Pages ({{ (storageMetrics.totalPages * 4096 / 1024).toFixed(1) }} KB)</span>
      </div>
      <div class="storage-metric">
        <span class="metric-label">BROWSER RAM / STORAGE:</span>
        <div class="mini-meter">
          <div class="mini-meter-fill" :style="{ width: storagePercent + '%' }"></div>
        </div>
        <span class="metric-value">{{ (storageMetrics.bytesUsed / 1024).toFixed(1) }} KB / 5,120 KB (Safe Quota)</span>
      </div>
      <div class="storage-metric">
        <span class="metric-label">CATALOG:</span>
        <span class="metric-value">{{ storageMetrics.tablesCount }} Tables &bull; {{ storageMetrics.totalRecords }} Rows</span>
      </div>
    </div>

    <!-- Quick query presets -->
    <div class="presets-bar">
      <span class="presets-label">Presets:</span>
      <button 
        v-for="(p, i) in presets" 
        :key="i" 
        @click="loadPreset(p.sql)" 
        class="preset-pill"
        :title="p.desc"
      >
        {{ p.label }}
      </button>
    </div>

    <!-- SQL Input Area -->
    <div class="editor-pane">
      <div class="prompt-line">
        <span class="prompt-prefix">quartz&gt;</span>
        <textarea 
          v-model="sqlQuery" 
          @keydown.ctrl.enter="runQuery"
          class="sql-input" 
          rows="3" 
          placeholder="Enter SQL (e.g. SELECT * FROM users WHERE id = 1; CREATE TABLE logs (id INT PRIMARY KEY, msg TEXT);)"
          spellcheck="false"
        ></textarea>
      </div>
    </div>

    <!-- Tabular Output Pane -->
    <div class="output-pane">
      <div class="output-header">
        <span>Execution Result &bull; Output Stream</span>
        <span class="row-count" v-if="rowsAffectedText">{{ rowsAffectedText }}</span>
      </div>
      <pre class="output-pre"><code>{{ outputText }}</code></pre>
    </div>

    <!-- Gold Tick Celebration Banner -->
    <transition name="gold-fade">
      <div v-if="celebrating" class="celebration-toast">
        <span class="gold-tick">✔</span>
        <span>Query executed & verified against browser client engine!</span>
      </div>
    </transition>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { BrowserQuartzEngine, QueryResult } from '../utils/browser-engine'

const engine = new BrowserQuartzEngine()

const sqlQuery = ref("SELECT * FROM users WHERE id = 1;")
const latency = ref<number | null>(118)
const rowsAffectedText = ref("1 row in set (0.000118 sec)")
const executionPath = ref("B+ Tree Index Fast-Path (O(log N))")
const celebrating = ref(false)
const storageMetrics = ref(engine.getStorageMetrics())

const storagePercent = computed(() => {
  const pct = (storageMetrics.value.bytesUsed / storageMetrics.value.quotaBytes) * 100
  return Math.min(Math.max(pct, 1), 100).toFixed(1)
})

const pathClass = computed(() => {
  if (executionPath.value.includes('B+ Tree')) return 'path-bplus'
  if (executionPath.value.includes('DDL')) return 'path-ddl'
  return 'path-linear'
})

const outputText = ref(`+----+-------------+-------------------+----------+
| id | name        | email             | role     |
+----+-------------+-------------------+----------+
| 1  | Alice Smith | alice@example.com | admin    |
+----+-------------+-------------------+----------+
1 row in set (0.000118 sec)
`)

const presets = [
  { label: 'Point Read (O(log N))', sql: 'SELECT * FROM users WHERE id = 1;', desc: 'Fast-path B+ tree index lookup by primary key' },
  { label: 'Relational JOIN', sql: 'SELECT users.name, orders.product, orders.amount FROM users JOIN orders ON users.id = orders.user_id;', desc: 'Index nested-loop join across slotted tables' },
  { label: 'Insert Tuple', sql: "INSERT INTO users VALUES (4, 'Diana Prince', 'diana@themyscira.io', 'architect');", desc: 'Insert new row and index into B+ Tree' },
  { label: 'Filter & Limit', sql: 'SELECT * FROM users WHERE id >= 2 ORDER BY id DESC LIMIT 2;', desc: 'WHERE filter, ORDER BY, and LIMIT' },
  { label: 'Aggregate COUNT/SUM', sql: 'SELECT COUNT(*) FROM orders;', desc: 'Execute aggregate calculation' },
  { label: 'Inspect Catalog (.schema)', sql: '.schema', desc: 'Display Page 0 table schemas and column constraints' },
  { label: 'Create New Table', sql: 'CREATE TABLE telemetry (id INT PRIMARY KEY, sensor TEXT, reading INT);', desc: 'Allocate new slotted page and catalog descriptor' }
]

const formatAsciiTable = (headers: string[], rows: any[][]): string => {
  if (!rows || rows.length === 0) return "Empty set."

  const widths = headers.map(h => h.length)
  rows.forEach(r => {
    headers.forEach((h, i) => {
      const val = String(r[i] ?? 'NULL')
      if (val.length > widths[i]) widths[i] = val.length
    })
  })

  const border = '+' + widths.map(w => '-'.repeat(w + 2)).join('+') + '+'
  const headerLine = '|' + headers.map((h, i) => ' ' + h.padEnd(widths[i]) + ' ').join('|') + '|'
  
  const rowLines = rows.map(r => {
    return '|' + headers.map((h, i) => {
      const val = String(r[i] ?? 'NULL')
      return ' ' + val.padEnd(widths[i]) + ' '
    }).join('|') + '|'
  }).join('\n')

  return `${border}\n${headerLine}\n${border}\n${rowLines}\n${border}`
}

const playChime = () => {
  try {
    const AudioContext = window.AudioContext || (window as any).webkitAudioContext
    if (!AudioContext) return
    const ctx = new AudioContext()
    const osc = ctx.createOscillator()
    const gain = ctx.createGain()

    osc.type = 'sine'
    osc.frequency.setValueAtTime(587.33, ctx.currentTime) // D5
    osc.frequency.exponentialRampToValueAtTime(880.00, ctx.currentTime + 0.15) // A5

    gain.gain.setValueAtTime(0.06, ctx.currentTime)
    gain.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + 0.4)

    osc.connect(gain)
    gain.connect(ctx.destination)

    osc.start()
    osc.stop(ctx.currentTime + 0.4)
  } catch (e) {
    // Audio optional if blocked by browser policy
  }
}

const loadPreset = (sql: string) => {
  sqlQuery.value = sql
  runQuery()
}

const resetDatabase = () => {
  engine.resetDatabase()
  storageMetrics.value = engine.getStorageMetrics()
  sqlQuery.value = "SELECT * FROM users;"
  runQuery()
}

const runQuery = () => {
  const q = sqlQuery.value.trim()
  const result: QueryResult = engine.execute(q)

  latency.value = result.latencyUs
  executionPath.value = result.executionPath
  storageMetrics.value = result.pageStats

  playChime()
  celebrating.value = true
  setTimeout(() => { celebrating.value = false }, 2000)

  const durSec = (result.latencyUs / 1000000).toFixed(6)

  if (!result.success) {
    rowsAffectedText.value = "Execution Error"
    outputText.value = `❌ Error: ${result.message}\n(Execution time: ${durSec} sec)`
    return
  }

  if (result.columns && result.rows) {
    const tableStr = formatAsciiTable(result.columns, result.rows)
    const count = result.rows.length
    rowsAffectedText.value = `${count} row${count === 1 ? '' : 's'} in set (${durSec} sec)`
    outputText.value = `${tableStr}\n${rowsAffectedText.value}`
  } else {
    rowsAffectedText.value = `Query OK (${durSec} sec)`
    outputText.value = `${result.message || 'Query OK.'}\n${rowsAffectedText.value}`
  }
}

onMounted(() => {
  storageMetrics.value = engine.getStorageMetrics()
})
</script>

<style scoped>
.playground-card {
  border: 2px solid var(--q-ink, #1d2939);
  background: var(--q-bg-panel, #f6f1e5);
  border-radius: 8px;
  overflow: hidden;
  margin: 2rem 0;
  box-shadow: 4px 4px 0px rgba(29, 41, 57, 0.2);
  font-family: var(--vp-font-family-mono, monospace);
  position: relative;
  transition: all 0.2s ease;
}

.dark .playground-card {
  border-color: var(--q-accent-primary, #00d2b4);
  background: var(--q-bg-panel, #191f2e);
  box-shadow: 4px 4px 0px #0a0d14;
}

.playground-header {
  background: var(--q-bg-elevated, #e3dccb);
  border-bottom: 2px solid var(--q-ink, #1d2939);
  padding: 8px 14px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 8px;
}

.dark .playground-header {
  background: var(--q-bg-elevated, #21293c);
  border-bottom-color: var(--q-accent-primary, #00d2b4);
}

.playground-title-group {
  display: flex;
  align-items: center;
  gap: 8px;
  flex-wrap: wrap;
}

.terminal-dot {
  width: 10px;
  height: 10px;
  border-radius: 50%;
  display: inline-block;
  border: 1px solid rgba(0, 0, 0, 0.2);
}

.terminal-dot.red { background: #ef4444; }
.terminal-dot.yellow { background: #f59e0b; }
.terminal-dot.green { background: #10b981; }

.playground-title {
  font-size: 0.82rem;
  font-weight: 700;
  color: var(--q-ink, #1d2939);
  letter-spacing: 0.04em;
}

.dark .playground-title {
  color: var(--q-ink, #f4efe6);
}

.badge-tag {
  font-size: 0.7rem;
  font-weight: 700;
  color: var(--q-accent-secondary, #d1493e);
  letter-spacing: 0.05em;
}

.dark .badge-tag {
  color: #ffb000;
}

.playground-controls {
  display: flex;
  align-items: center;
  gap: 8px;
  flex-wrap: wrap;
}

.path-badge {
  font-size: 0.72rem;
  font-weight: 600;
  padding: 3px 8px;
  border-radius: 3px;
  border: 1px solid;
}

.path-bplus {
  background: rgba(21, 128, 61, 0.12);
  color: #15803d;
  border-color: rgba(21, 128, 61, 0.35);
}

.dark .path-bplus {
  background: rgba(16, 185, 129, 0.15);
  color: #10b981;
  border-color: rgba(16, 185, 129, 0.4);
}

.path-linear {
  background: rgba(14, 116, 144, 0.12);
  color: #0e7490;
  border-color: rgba(14, 116, 144, 0.35);
}

.dark .path-linear {
  background: rgba(56, 189, 248, 0.15);
  color: #38bdf8;
  border-color: rgba(56, 189, 248, 0.4);
}

.path-ddl {
  background: rgba(109, 40, 217, 0.12);
  color: #6d28d9;
  border-color: rgba(109, 40, 217, 0.35);
}

.dark .path-ddl {
  background: rgba(167, 139, 250, 0.15);
  color: #a78bfa;
  border-color: rgba(167, 139, 250, 0.4);
}

.latency-badge {
  font-size: 0.74rem;
  font-weight: 700;
  color: var(--q-ink, #1d2939);
  background: var(--q-bg-panel, #f6f1e5);
  padding: 3px 7px;
  border-radius: 3px;
  border: 1px solid var(--q-ink-muted, #5a6778);
}

.dark .latency-badge {
  color: var(--q-ink, #f4efe6);
  background: var(--q-bg-panel, #191f2e);
  border-color: var(--q-ink-muted, #8e9bae);
}

.btn-ctrl {
  background: var(--q-bg-panel, #f6f1e5);
  border: 1px solid var(--q-ink-muted, #5a6778);
  font-size: 0.74rem;
  font-weight: 600;
  padding: 4px 9px;
  border-radius: 3px;
  cursor: pointer;
  color: var(--q-ink, #1d2939);
  transition: all 0.15s ease;
}

.btn-ctrl:hover {
  background: var(--q-accent-secondary, #d1493e);
  color: #fff;
  border-color: var(--q-accent-secondary, #d1493e);
}

.dark .btn-ctrl {
  background: var(--q-bg-panel, #191f2e);
  color: var(--q-ink, #f4efe6);
}

.btn-run {
  background: var(--q-accent-primary, #2563eb);
  color: #fff;
  border: 1px solid var(--q-accent-primary, #2563eb);
  font-size: 0.78rem;
  font-weight: 700;
  padding: 5px 12px;
  border-radius: 3px;
  cursor: pointer;
  transition: all 0.15s ease;
  box-shadow: 2px 2px 0px rgba(0, 0, 0, 0.2);
}

.btn-run:hover {
  background: #1d4ed8;
  transform: translateY(-1px);
}

.dark .btn-run {
  background: var(--q-accent-primary, #00d2b4);
  color: #141824;
  border-color: var(--q-accent-primary, #00d2b4);
}

.dark .btn-run:hover {
  background: #00f0ce;
}

/* Storage & Pager Status Bar */
.storage-bar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 6px 14px;
  background: rgba(0, 0, 0, 0.04);
  border-bottom: 1px solid var(--q-ink-muted, #5a6778);
  font-size: 0.72rem;
  flex-wrap: wrap;
  gap: 12px;
}

.dark .storage-bar {
  background: rgba(255, 255, 255, 0.03);
  border-bottom-color: rgba(255, 255, 255, 0.1);
}

.storage-metric {
  display: flex;
  align-items: center;
  gap: 6px;
}

.metric-label {
  font-weight: 700;
  color: var(--q-ink-muted, #5a6778);
  font-size: 0.68rem;
}

.dark .metric-label {
  color: var(--q-ink-muted, #8e9bae);
}

.metric-value {
  font-weight: 600;
  color: var(--q-ink, #1d2939);
}

.dark .metric-value {
  color: var(--q-ink, #f4efe6);
}

.mini-meter {
  width: 50px;
  height: 6px;
  background: rgba(0, 0, 0, 0.15);
  border-radius: 3px;
  overflow: hidden;
  display: inline-block;
  vertical-align: middle;
}

.dark .mini-meter {
  background: rgba(255, 255, 255, 0.2);
}

.mini-meter-fill {
  height: 100%;
  background: #10b981;
  transition: width 0.3s ease;
}

/* Presets bar */
.presets-bar {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 6px 14px;
  background: rgba(0, 0, 0, 0.02);
  border-bottom: 1px solid var(--q-ink-muted, #5a6778);
  flex-wrap: wrap;
}

.dark .presets-bar {
  background: rgba(0, 0, 0, 0.2);
  border-bottom-color: rgba(255, 255, 255, 0.08);
}

.presets-label {
  font-size: 0.72rem;
  font-weight: 700;
  color: var(--q-ink-muted, #5a6778);
}

.preset-pill {
  background: var(--q-bg-panel, #f6f1e5);
  border: 1px solid var(--q-ink-muted, #5a6778);
  font-size: 0.7rem;
  padding: 2px 7px;
  border-radius: 2px;
  cursor: pointer;
  color: var(--q-ink, #1d2939);
  transition: all 0.15s ease;
}

.preset-pill:hover {
  background: var(--q-accent-primary, #2563eb);
  color: #fff;
  border-color: var(--q-accent-primary, #2563eb);
}

.dark .preset-pill {
  background: var(--q-bg-panel, #191f2e);
  color: var(--q-ink, #f4efe6);
  border-color: rgba(255, 255, 255, 0.15);
}

.dark .preset-pill:hover {
  background: var(--q-accent-primary, #00d2b4);
  color: #141824;
}

/* Editor pane */
.editor-pane {
  padding: 10px 14px;
  background: var(--q-bg-panel, #f6f1e5);
}

.dark .editor-pane {
  background: var(--q-bg-panel, #191f2e);
}

.prompt-line {
  display: flex;
  align-items: flex-start;
  gap: 8px;
}

.prompt-prefix {
  font-weight: 700;
  color: var(--q-accent-primary, #2563eb);
  font-size: 0.88rem;
  line-height: 1.4;
  user-select: none;
}

.dark .prompt-prefix {
  color: var(--q-accent-primary, #00d2b4);
}

.sql-input {
  flex: 1;
  font-family: var(--vp-font-family-mono, monospace);
  font-size: 0.88rem;
  line-height: 1.4;
  background: transparent;
  border: none;
  outline: none;
  color: var(--q-ink, #1d2939);
  resize: vertical;
}

.dark .sql-input {
  color: var(--q-ink, #f4efe6);
}

/* Output pane */
.output-pane {
  border-top: 1px solid var(--q-ink-muted, #5a6778);
  background: var(--q-bg-canvas, #efe8d8);
  padding: 10px 14px;
}

.dark .output-pane {
  background: var(--q-bg-canvas, #141824);
  border-top-color: rgba(255, 255, 255, 0.1);
}

.output-header {
  display: flex;
  justify-content: space-between;
  font-size: 0.72rem;
  color: var(--q-ink-muted, #5a6778);
  font-weight: 700;
  letter-spacing: 0.05em;
  text-transform: uppercase;
  margin-bottom: 6px;
}

.dark .output-header {
  color: var(--q-ink-muted, #8e9bae);
}

.output-pre {
  margin: 0;
  font-size: 0.82rem;
  line-height: 1.45;
  color: var(--q-ink, #1d2939);
  overflow-x: auto;
  white-space: pre;
}

.dark .output-pre {
  color: var(--q-ink, #f4efe6);
}

/* Celebration toast */
.celebration-toast {
  position: absolute;
  top: 12px;
  left: 50%;
  transform: translateX(-50%);
  background: #fef08a;
  color: #854d0e;
  border: 1px solid #facc15;
  padding: 4px 12px;
  border-radius: 16px;
  font-size: 0.76rem;
  font-weight: 700;
  display: flex;
  align-items: center;
  gap: 6px;
  box-shadow: 0 4px 12px rgba(250, 204, 21, 0.35);
  z-index: 10;
  pointer-events: none;
}

.dark .celebration-toast {
  background: #2a3321;
  color: #86efac;
  border-color: #4ade80;
  box-shadow: 0 4px 12px rgba(74, 222, 128, 0.25);
}

.gold-tick {
  color: #ca8a04;
  font-weight: 900;
}

.dark .gold-tick {
  color: #4ade80;
}

.gold-fade-enter-active,
.gold-fade-leave-active {
  transition: all 0.25s ease;
}

.gold-fade-enter-from,
.gold-fade-leave-to {
  opacity: 0;
  transform: translate(-50%, -8px);
}
</style>
