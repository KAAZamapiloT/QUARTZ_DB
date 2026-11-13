<template>
  <div class="error-explorer">
    <!-- Search-First Input -->
    <div class="search-hero">
      <div class="search-bar-wrap">
        <span class="search-icon">🔍</span>
        <input 
          v-model="searchQuery" 
          type="text" 
          placeholder="Paste your error message or code (e.g. 'duplicate key', 'QERR_1001', 'corrupt magic')..."
          class="error-search-input"
          autofocus
        />
        <button v-if="searchQuery" class="clear-btn" @click="searchQuery = ''">✕</button>
      </div>
    </div>

    <!-- Subsystem Filter Pills -->
    <div class="filter-pills">
      <button 
        v-for="sub in subsystems" 
        :key="sub"
        :class="['sub-pill', { active: activeSubsystem === sub }]"
        @click="activeSubsystem = sub"
      >
        {{ sub }}
      </button>
    </div>

    <!-- Results Count -->
    <div class="results-meta">
      <span>Found <strong>{{ filteredErrors.length }}</strong> error definitions</span>
    </div>

    <!-- Error Cards List -->
    <div class="error-cards-list">
      <div 
        v-for="err in filteredErrors" 
        :key="err.code" 
        :id="err.code.toLowerCase()"
        class="error-card"
      >
        <div class="error-card-header">
          <div class="code-group">
            <span class="error-code-badge">{{ err.code }}</span>
            <span class="error-num">Code #{{ err.num }}</span>
          </div>
          <div class="error-tags">
            <span class="subsystem-tag">{{ err.subsystem }}</span>
            <span :class="['severity-tag', err.severity.toLowerCase()]">{{ err.severity }}</span>
          </div>
        </div>

        <h3 class="error-card-title">{{ err.title }}</h3>
        
        <div class="error-card-body">
          <div class="section-block">
            <span class="block-label">Engine Cause</span>
            <p class="cause-text">{{ err.cause }}</p>
          </div>

          <div class="fix-tip-box">
            <div class="tip-header">
              <span class="tip-icon">💡</span>
              <span class="tip-title">How to Fix</span>
            </div>
            <p class="tip-text">{{ err.fix }}</p>
          </div>

          <div class="code-examples" v-if="err.brokenExample && err.fixedExample">
            <div class="example-column">
              <span class="ex-label broken">✕ Broken Query</span>
              <pre class="ex-pre broken"><code>{{ err.brokenExample }}</code></pre>
            </div>
            <div class="example-column">
              <span class="ex-label fixed">✓ Corrected Query</span>
              <pre class="ex-pre fixed"><code>{{ err.fixedExample }}</code></pre>
            </div>
          </div>
        </div>
      </div>

      <div v-if="filteredErrors.length === 0" class="no-results">
        <p>No error codes matched "<strong>{{ searchQuery }}</strong>".</p>
        <p class="no-results-hint">Try searching by keyword like <code>duplicate</code>, <code>null</code>, <code>magic</code>, or <code>table</code>.</p>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed } from 'vue'

const searchQuery = ref('')
const activeSubsystem = ref('ALL')

const subsystems = ['ALL', 'Primary Index', 'Slotted Storage', 'Database Catalog', 'SQL Parser', 'Execution Engine']

const errors = [
  {
    code: 'QERR_PK_DUPLICATE',
    num: 1001,
    subsystem: 'Primary Index',
    severity: 'Error',
    title: 'Primary Key Uniqueness Violation',
    cause: 'An INSERT or UPDATE attempted to write an existing 64-bit integer primary key. QuartzDB enforces strict single-column primary key uniqueness via the disk-backed B+ Tree index.',
    fix: 'Verify the primary key does not already exist before inserting, or generate a monotonic ID. Use UPDATE with a WHERE clause to modify existing records.',
    brokenExample: "INSERT INTO users VALUES (1, 'Alice');\nINSERT INTO users VALUES (1, 'Bob'); -- Fails: Key 1 exists",
    fixedExample: "INSERT INTO users VALUES (1, 'Alice');\nINSERT INTO users VALUES (2, 'Bob'); -- Success: Unique key"
  },
  {
    code: 'QERR_NOT_NULL_VIOLATION',
    num: 1002,
    subsystem: 'Execution Engine',
    severity: 'Error',
    title: 'Null Value in NOT NULL Column',
    cause: 'An INSERT or UPDATE provided a NULL (std::monostate) value for a column explicitly declared with the NOT NULL constraint.',
    fix: 'Provide a valid non-null string or integer literal matching the column type, or update schema definition if the column should be nullable.',
    brokenExample: "CREATE TABLE users (id INT PRIMARY KEY, name TEXT NOT NULL);\nINSERT INTO users VALUES (1, NULL); -- Fails",
    fixedExample: "INSERT INTO users VALUES (1, 'Valid Name'); -- Success"
  },
  {
    code: 'QERR_TABLE_NOT_FOUND',
    num: 1003,
    subsystem: 'Database Catalog',
    severity: 'Error',
    title: 'Unknown Table in Catalog',
    cause: 'A query referenced a table name that is not registered in Page 0 catalog. Table names are case-sensitive identifiers.',
    fix: 'Check table name spelling with SHOW TABLES; or .tables in the CLI. If opening an existing database, ensure the correct .db file was loaded.',
    brokenExample: "SELECT * FROM usr_profile; -- Fails: Table 'usr_profile' missing",
    fixedExample: "SELECT * FROM users;       -- Success: Matches catalog table"
  },
  {
    code: 'QERR_RECORD_TOO_LARGE',
    num: 1004,
    subsystem: 'Slotted Storage',
    severity: 'Fatal',
    title: 'Record Size Exceeds 4KB Page Capacity',
    cause: 'The serialized record payload exceeds the available capacity of a single slotted page (PAGE_SIZE - HEADER_SIZE - SLOT_SIZE = 4068 bytes). QuartzDB requires tuples to fit within a single 4096-byte page.',
    fix: 'Truncate text payloads to under 4000 bytes or normalize large binary blobs/long strings into external asset storage.',
    brokenExample: "-- Inserting a 10KB string payload into a TEXT column\nINSERT INTO logs VALUES (1, '<10,000 bytes string>');",
    fixedExample: "-- Keep payload within 4068-byte single-page envelope\nINSERT INTO logs VALUES (1, 'Summary log text < 4KB');"
  },
  {
    code: 'QERR_CORRUPT_MAGIC',
    num: 1005,
    subsystem: 'Slotted Storage',
    severity: 'Fatal',
    title: 'Invalid Page Magic Number',
    cause: 'A 4KB page read from disk did not match the expected magic bytes (QDB1: 0x31424451 for database catalog, QPG1: 0x31475051 for slotted pages, or BPT1: 0x31545042 for B+ tree nodes). The file is corrupted or not a QuartzDB file.',
    fix: 'Verify the target file was created by QuartzDB. If corrupted due to power failure, restore from cold backup or inspect page 0 offset.',
    brokenExample: "./quartz corrupt_file.bin -- Fails: 'Invalid database magic'",
    fixedExample: "./quartz valid_database.db -- Success: Valid 4KB page header"
  },
  {
    code: 'QERR_SYNTAX_ERROR',
    num: 1006,
    subsystem: 'SQL Parser',
    severity: 'Error',
    title: 'Recursive-Descent Parser Syntax Error',
    cause: 'The SQL query token stream violated the grammar rules expected by qdb::Parser (e.g. unclosed parentheses, missing semicolon, unexpected operator).',
    fix: 'Verify SQL statements end with a semicolon (;). Check operator syntax (=, !=, <, <=, >, >=) and ensure strings are quoted with single or double quotes.',
    brokenExample: "SELECT FROM users WHERE; -- Fails: Missing column projection",
    fixedExample: "SELECT * FROM users;     -- Success: Well-formed SQL statement"
  },
  {
    code: 'QERR_OUT_OF_PAGES',
    num: 1007,
    subsystem: 'Slotted Storage',
    severity: 'Error',
    title: 'Physical File Block Allocation Failure',
    cause: 'The pager was unable to allocate an additional 4096-byte page block on disk due to disk quota limits or filesystem read-only permissions.',
    fix: 'Check free disk space and verify write permissions for the directory containing the .db file.',
    brokenExample: "-- File system is 100% full\nINSERT INTO users VALUES (100000, 'Test');",
    fixedExample: "-- Free disk space or allocate to unconstrained volume"
  }
]

const filteredErrors = computed(() => {
  return errors.filter(err => {
    if (activeSubsystem.value !== 'ALL' && err.subsystem !== activeSubsystem.value) return false

    if (searchQuery.value.trim() !== '') {
      const q = searchQuery.value.toLowerCase()
      const match = err.code.toLowerCase().includes(q) ||
                    String(err.num).includes(q) ||
                    err.title.toLowerCase().includes(q) ||
                    err.cause.toLowerCase().includes(q) ||
                    err.fix.toLowerCase().includes(q)
      if (!match) return false
    }

    return true
  })
})
</script>

<style scoped>
.error-explorer {
  margin: 1.5rem 0 3rem;
}

.search-hero {
  margin-bottom: 1.25rem;
}

.search-bar-wrap {
  display: flex;
  align-items: center;
  background: var(--vp-c-bg-elv);
  border: 2px solid var(--vp-c-border);
  border-radius: 8px;
  padding: 12px 16px;
  gap: 12px;
  box-shadow: var(--vp-shadow-1);
  transition: border-color 0.2s ease;
}

.search-bar-wrap:focus-within {
  border-color: var(--vp-c-brand-1);
}

.search-icon {
  font-size: 1.1rem;
  color: var(--vp-c-text-3);
}

.error-search-input {
  width: 100%;
  border: none;
  outline: none;
  background: transparent;
  font-family: var(--vp-font-family-sans);
  font-size: 1rem;
  color: var(--vp-c-text-1);
}

.clear-btn {
  background: transparent;
  border: none;
  font-size: 1rem;
  color: var(--vp-c-text-3);
  cursor: pointer;
}

.filter-pills {
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
  margin-bottom: 1rem;
}

.sub-pill {
  background: var(--vp-c-bg-elv);
  border: 1px solid var(--vp-c-border);
  border-radius: 20px;
  padding: 6px 14px;
  font-family: var(--vp-font-family-sans);
  font-size: 0.75rem;
  font-weight: 500;
  color: var(--vp-c-text-2);
  cursor: pointer;
  transition: all 0.15s ease;
}

.sub-pill:hover,
.sub-pill.active {
  background: var(--vp-c-brand-1);
  border-color: var(--vp-c-brand-1);
  color: #fff;
}

.results-meta {
  font-family: var(--vp-font-family-sans);
  font-size: 0.8rem;
  color: var(--vp-c-text-3);
  margin-bottom: 1.5rem;
}

.error-cards-list {
  display: flex;
  flex-direction: column;
  gap: 1.5rem;
}

.error-card {
  border: 1px solid var(--vp-c-border);
  border-radius: 8px;
  background: var(--vp-c-bg-elv);
  padding: 1.5rem;
  box-shadow: var(--vp-shadow-1);
}

.error-card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 8px;
  margin-bottom: 0.75rem;
}

.code-group {
  display: flex;
  align-items: baseline;
  gap: 8px;
}

.error-code-badge {
  font-family: var(--vp-font-family-mono);
  font-weight: 700;
  font-size: 0.95rem;
  color: var(--vp-c-brand-1);
}

.error-num {
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  color: var(--vp-c-text-3);
}

.error-tags {
  display: flex;
  gap: 6px;
}

.subsystem-tag {
  font-family: var(--vp-font-family-sans);
  font-size: 0.7rem;
  background: var(--vp-c-bg-soft);
  padding: 2px 8px;
  border-radius: 4px;
  border: 1px solid var(--vp-c-border);
  color: var(--vp-c-text-2);
}

.severity-tag {
  font-family: var(--vp-font-family-sans);
  font-size: 0.7rem;
  font-weight: 600;
  padding: 2px 8px;
  border-radius: 4px;
}

.severity-tag.error {
  background: rgba(239, 68, 68, 0.1);
  color: #dc2626;
  border: 1px solid rgba(239, 68, 68, 0.2);
}

.severity-tag.fatal {
  background: #7f1d1d;
  color: #fee2e2;
}

.error-card-title {
  font-family: var(--vp-font-family-sans);
  font-size: 1.25rem;
  margin: 0 0 1rem 0;
  color: var(--vp-c-text-1);
}

.section-block {
  margin-bottom: 1rem;
}

.block-label {
  display: block;
  font-family: var(--vp-font-family-sans);
  font-size: 0.7rem;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 0.05em;
  color: var(--vp-c-text-3);
  margin-bottom: 4px;
}

.cause-text {
  font-size: 0.9rem;
  color: var(--vp-c-text-2);
  margin: 0;
  line-height: 1.6;
}

.fix-tip-box {
  background: rgba(16, 185, 129, 0.06);
  border: 1px solid rgba(16, 185, 129, 0.25);
  border-radius: 6px;
  padding: 12px 14px;
  margin-bottom: 1.25rem;
}

.tip-header {
  display: flex;
  align-items: center;
  gap: 6px;
  margin-bottom: 4px;
}

.tip-title {
  font-family: var(--vp-font-family-sans);
  font-size: 0.8rem;
  font-weight: 700;
  color: #059669;
  text-transform: uppercase;
  letter-spacing: 0.05em;
}

.tip-text {
  font-size: 0.9rem;
  color: var(--vp-c-text-1);
  margin: 0;
  line-height: 1.5;
}

.code-examples {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 12px;
}

.example-column {
  display: flex;
  flex-direction: column;
}

.ex-label {
  font-family: var(--vp-font-family-sans);
  font-size: 0.75rem;
  font-weight: 600;
  margin-bottom: 4px;
}

.ex-label.broken { color: #dc2626; }
.ex-label.fixed { color: #059669; }

.ex-pre {
  margin: 0;
  padding: 10px 12px;
  border-radius: 6px;
  font-family: var(--vp-font-family-mono);
  font-size: 0.8rem;
  line-height: 1.5;
  overflow-x: auto;
}

.ex-pre.broken {
  background: rgba(239, 68, 68, 0.04);
  border: 1px solid rgba(239, 68, 68, 0.2);
}

.ex-pre.fixed {
  background: rgba(16, 185, 129, 0.04);
  border: 1px solid rgba(16, 185, 129, 0.2);
}

.no-results {
  text-align: center;
  padding: 3rem 1rem;
  color: var(--vp-c-text-2);
}

.no-results-hint {
  font-size: 0.85rem;
  color: var(--vp-c-text-3);
}

@media (max-width: 768px) {
  .code-examples {
    grid-template-columns: 1fr;
  }
}
</style>
