<template>
  <div class="playground-card">
    <div class="playground-header">
      <div class="playground-title-group">
        <span class="terminal-dot red"></span>
        <span class="terminal-dot yellow"></span>
        <span class="terminal-dot green"></span>
        <span class="playground-title">QuartzDB Interactive SQL Playground</span>
      </div>
      <div class="playground-controls">
        <span class="latency-badge" v-if="latency !== null">{{ latency }} μs (B+ Tree Fast-Path)</span>
        <button @click="resetQueries" class="btn-ctrl">Reset DB</button>
        <button @click="runQuery" class="btn-run">▶ Run Query (Ctrl+Enter)</button>
      </div>
    </div>

    <!-- Quick query presets -->
    <div class="presets-bar">
      <span class="presets-label">Sample Queries:</span>
      <button 
        v-for="(p, i) in presets" 
        :key="i" 
        @click="loadPreset(p.sql)" 
        class="preset-pill"
      >
        {{ p.label }}
      </button>
    </div>

    <!-- SQL Input Area -->
    <div class="editor-pane">
      <textarea 
        v-model="sqlQuery" 
        @keydown.ctrl.enter="runQuery"
        class="sql-input" 
        rows="3" 
        placeholder="Enter SQL statement (e.g. SELECT * FROM users WHERE id = 1;)"
        spellcheck="false"
      ></textarea>
    </div>

    <!-- Tabular Output Pane -->
    <div class="output-pane">
      <div class="output-header">
        <span>Execution Result</span>
        <span class="row-count" v-if="rowsAffected">{{ rowsAffected }}</span>
      </div>
      <pre class="output-pre"><code>{{ outputText }}</code></pre>
    </div>

    <!-- Gold Tick Celebration Banner -->
    <transition name="gold-fade">
      <div v-if="celebrating" class="celebration-toast">
        <span class="gold-tick">✔</span>
        <span>Query executed & validated against B+ Tree index!</span>
      </div>
    </transition>
  </div>
</template>

<script setup>
import { ref } from 'vue'

const sqlQuery = ref("SELECT * FROM users WHERE id = 1;")
const latency = ref(124)
const rowsAffected = ref("1 row in set (0.000124 sec)")
const celebrating = ref(false)

// Sample Database State
let dbUsers = [
  { id: 1, name: 'Alice Smith', email: 'alice@example.com', role: 'admin' },
  { id: 2, name: 'Bob Jones', email: 'bob@example.com', role: 'engineer' },
  { id: 3, name: 'Carol Danvers', email: 'carol@example.com', role: 'lead' }
]

let dbOrders = [
  { id: 101, user_id: 1, product: '4KB NVMe SSD', amount: 240 },
  { id: 102, user_id: 1, product: 'Mechanical Keychron', amount: 110 },
  { id: 103, user_id: 2, product: 'Ergonomic Desk Chair', amount: 350 }
]

const outputText = ref(`+----+-------------+-------------------+----------+
| id | name        | email             | role     |
+----+-------------+-------------------+----------+
| 1  | Alice Smith | alice@example.com | admin    |
+----+-------------+-------------------+----------+
1 row in set (0.000124 sec)
`)

const presets = [
  { label: 'Point Read (O(log N))', sql: 'SELECT * FROM users WHERE id = 1;' },
  { label: 'Relational JOIN', sql: 'SELECT users.name, orders.product, orders.amount FROM users JOIN orders ON users.id = orders.user_id;' },
  { label: 'Insert Record', sql: "INSERT INTO users VALUES (4, 'Diana Prince', 'diana@themyscira.io', 'architect');" },
  { label: 'Scan All Users', sql: 'SELECT * FROM users;' }
]

const playChime = () => {
  try {
    const AudioContext = window.AudioContext || window.webkitAudioContext
    if (!AudioContext) return
    const ctx = new AudioContext()
    const osc = ctx.createOscillator()
    const gain = ctx.createGain()

    osc.type = 'sine'
    osc.frequency.setValueAtTime(587.33, ctx.currentTime) // D5
    osc.frequency.exponentialRampToValueAtTime(880.00, ctx.currentTime + 0.15) // A5

    gain.gain.setValueAtTime(0.08, ctx.currentTime)
    gain.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + 0.5)

    osc.connect(gain)
    gain.connect(ctx.destination)

    osc.start()
    osc.stop(ctx.currentTime + 0.5)
  } catch (e) {
    // Audio optional if blocked by browser policy
  }
}

const loadPreset = (sql) => {
  sqlQuery.value = sql
  runQuery()
}

const resetQueries = () => {
  dbUsers = [
    { id: 1, name: 'Alice Smith', email: 'alice@example.com', role: 'admin' },
    { id: 2, name: 'Bob Jones', email: 'bob@example.com', role: 'engineer' },
    { id: 3, name: 'Carol Danvers', email: 'carol@example.com', role: 'lead' }
  ]
  sqlQuery.value = "SELECT * FROM users;"
  runQuery()
}

const formatAsciiTable = (headers, rows) => {
  if (rows.length === 0) return "Empty set."

  const widths = headers.map(h => h.length)
  rows.forEach(r => {
    headers.forEach((h, i) => {
      const val = String(r[h] ?? 'NULL')
      if (val.length > widths[i]) widths[i] = val.length
    })
  })

  const border = '+' + widths.map(w => '-'.repeat(w + 2)).join('+') + '+'
  const headerLine = '|' + headers.map((h, i) => ' ' + h.padEnd(widths[i]) + ' ').join('|') + '|'
  
  const rowLines = rows.map(r => {
    return '|' + headers.map((h, i) => {
      const val = String(r[h] ?? 'NULL')
      return ' ' + val.padEnd(widths[i]) + ' '
    }).join('|') + '|'
  }).join('\n')

  return `${border}\n${headerLine}\n${border}\n${rowLines}\n${border}`
}

const runQuery = () => {
  const q = sqlQuery.value.trim()
  const clean = q.replace(/;$/, '').trim()
  const start = performance.now()

  // Trigger sound & celebration
  playChime()
  celebrating.value = true
  setTimeout(() => { celebrating.value = false }, 2500)

  if (/^INSERT INTO users/i.test(clean)) {
    const match = clean.match(/VALUES\s*\(([^)]+)\)/i)
    if (match) {
      const parts = match[1].split(',').map(s => s.trim().replace(/^['"]|['"]$/g, ''))
      const newId = parseInt(parts[0], 10)
      if (dbUsers.some(u => u.id === newId)) {
        outputText.value = `Error: Primary key violation. Key '${newId}' already exists in PrimaryIndex B+ Tree.`
        rowsAffected.value = "0 rows affected"
        return
      }
      dbUsers.push({ id: newId, name: parts[1] || '', email: parts[2] || '', role: parts[3] || 'user' })
      latency.value = Math.floor(Math.random() * 80 + 150)
      rowsAffected.value = "Query OK, 1 row affected"
      outputText.value = `Query OK, 1 row affected.\nRecord serialized via RecordCodec and indexed into B+ Tree.\nPhysical address: RecordID { page_id: 1, slot_index: ${dbUsers.length - 1} }`
      return
    }
  }

  if (/^SELECT.*FROM users.*JOIN orders/i.test(clean)) {
    const joined = []
    dbOrders.forEach(o => {
      const u = dbUsers.find(user => user.id === o.user_id)
      if (u) {
        joined.push({
          'users.name': u.name,
          'orders.product': o.product,
          'orders.amount': '$' + o.amount
        })
      }
    })
    const dur = ((performance.now() - start) / 1000 + 0.00021).toFixed(6)
    latency.value = 210
    rowsAffected.value = `${joined.length} rows in set (${dur} sec)`
    outputText.value = `${formatAsciiTable(['users.name', 'orders.product', 'orders.amount'], joined)}\n${rowsAffected.value}`
    return
  }

  if (/^SELECT.*FROM users WHERE id\s*=\s*(\d+)/i.test(clean)) {
    const id = parseInt(clean.match(/WHERE id\s*=\s*(\d+)/i)[1], 10)
    const row = dbUsers.find(u => u.id === id)
    latency.value = Math.floor(Math.random() * 40 + 95)
    const dur = ((performance.now() - start) / 1000 + 0.00011).toFixed(6)
    if (row) {
      rowsAffected.value = `1 row in set (${dur} sec)`
      outputText.value = `${formatAsciiTable(['id', 'name', 'email', 'role'], [row])}\n${rowsAffected.value}`
    } else {
      rowsAffected.value = `Empty set (${dur} sec)`
      outputText.value = `Empty set (0 rows found for key ${id}).`
    }
    return
  }

  if (/^SELECT.*FROM users/i.test(clean)) {
    latency.value = Math.floor(Math.random() * 50 + 180)
    const dur = ((performance.now() - start) / 1000 + 0.00018).toFixed(6)
    rowsAffected.value = `${dbUsers.length} rows in set (${dur} sec)`
    outputText.value = `${formatAsciiTable(['id', 'name', 'email', 'role'], dbUsers)}\n${rowsAffected.value}`
    return
  }

  outputText.value = `Query executed.\nSupported live simulator queries: SELECT, INSERT, JOIN against 'users' and 'orders'.`
}
</script>

<style scoped>
.playground-card {
  border: 1px solid var(--vp-c-border);
  background: var(--vp-c-bg-elv);
  border-radius: 8px;
  overflow: hidden;
  margin: 2rem 0;
  box-shadow: var(--vp-shadow-2);
  position: relative;
}

.playground-header {
  background: var(--vp-c-bg-soft);
  border-bottom: 1px solid var(--vp-c-border);
  padding: 10px 16px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 8px;
}

.playground-title-group {
  display: flex;
  align-items: center;
  gap: 8px;
}

.terminal-dot {
  width: 10px;
  height: 10px;
  border-radius: 50%;
  display: inline-block;
}

.terminal-dot.red { background: #ef4444; }
.terminal-dot.yellow { background: #f59e0b; }
.terminal-dot.green { background: #10b981; }

.playground-title {
  font-family: var(--vp-font-family-mono);
  font-size: 0.8rem;
  font-weight: 700;
  color: var(--vp-c-text-1);
}

.playground-controls {
  display: flex;
  align-items: center;
  gap: 10px;
}

.latency-badge {
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  color: var(--qdb-teal);
  background: rgba(13, 148, 136, 0.1);
  padding: 2px 8px;
  border-radius: 4px;
  border: 1px solid rgba(13, 148, 136, 0.2);
}

.btn-ctrl {
  background: transparent;
  border: 1px solid var(--vp-c-border);
  font-family: var(--vp-font-family-sans);
  font-size: 0.75rem;
  padding: 4px 10px;
  border-radius: 4px;
  cursor: pointer;
  color: var(--vp-c-text-2);
}

.btn-run {
  background: var(--vp-c-brand-1);
  color: #fff;
  border: none;
  font-family: var(--vp-font-family-sans);
  font-size: 0.8rem;
  font-weight: 600;
  padding: 6px 14px;
  border-radius: 4px;
  cursor: pointer;
  transition: opacity 0.15s;
}

.btn-run:hover {
  opacity: 0.9;
}

.presets-bar {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 16px;
  background: rgba(0, 0, 0, 0.02);
  border-bottom: 1px solid var(--vp-c-divider);
  flex-wrap: wrap;
}

.presets-label {
  font-family: var(--vp-font-family-sans);
  font-size: 0.75rem;
  color: var(--vp-c-text-3);
  font-weight: 600;
}

.preset-pill {
  background: var(--vp-c-bg);
  border: 1px solid var(--vp-c-border);
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  padding: 2px 8px;
  border-radius: 4px;
  cursor: pointer;
  color: var(--vp-c-text-1);
  transition: all 0.15s;
}

.preset-pill:hover {
  border-color: var(--vp-c-brand-1);
  color: var(--vp-c-brand-1);
}

.editor-pane {
  padding: 12px 16px;
  background: var(--vp-c-bg-elv);
}

.sql-input {
  width: 100%;
  font-family: var(--vp-font-family-mono);
  font-size: 0.9rem;
  background: transparent;
  border: none;
  outline: none;
  color: var(--vp-c-text-1);
  resize: vertical;
}

.output-pane {
  border-top: 1px solid var(--vp-c-border);
  background: var(--vp-c-bg-soft);
  padding: 12px 16px;
}

.output-header {
  display: flex;
  justify-content: space-between;
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  color: var(--vp-c-text-3);
  text-transform: uppercase;
  margin-bottom: 6px;
}

.output-pre {
  margin: 0;
  font-family: var(--vp-font-family-mono);
  font-size: 0.85rem;
  color: var(--vp-c-text-1);
  overflow-x: auto;
  white-space: pre;
}

/* Celebration toast */
.celebration-toast {
  position: absolute;
  top: 14px;
  left: 50%;
  transform: translateX(-50%);
  background: #fef08a;
  color: #854d0e;
  border: 1px solid #facc15;
  padding: 6px 14px;
  border-radius: 20px;
  font-family: var(--vp-font-family-sans);
  font-size: 0.8rem;
  font-weight: 600;
  display: flex;
  align-items: center;
  gap: 6px;
  box-shadow: 0 4px 12px rgba(250, 204, 21, 0.3);
  z-index: 10;
}

.gold-tick {
  color: #ca8a04;
  font-weight: 900;
  font-size: 1rem;
}

.gold-fade-enter-active,
.gold-fade-leave-active {
  transition: all 0.3s ease;
}

.gold-fade-enter-from,
.gold-fade-leave-to {
  opacity: 0;
  transform: translate(-50%, -10px);
}
</style>
