<template>
  <div class="graph-card">
    <div class="graph-header">
      <div>
        <span class="graph-badge">PREREQUISITE MAP</span>
        <h3 class="graph-title">Architectural Mastery Dependency Graph</h3>
      </div>
      <div class="graph-legend">
        <span class="legend-item"><span class="dot done"></span> Core Built (100%)</span>
        <span class="legend-item"><span class="dot active"></span> Selected Node</span>
      </div>
    </div>

    <!-- Interactive SVG Node Canvas -->
    <div class="svg-wrapper">
      <svg viewBox="0 0 800 360" class="graph-svg">
        <defs>
          <marker id="arrow" viewBox="0 0 10 10" refX="8" refY="5" markerWidth="6" markerHeight="6" orient="auto-start-reverse">
            <path d="M 0 1 L 8 5 L 0 9 z" fill="var(--vp-c-border)" />
          </marker>
          <marker id="arrow-active" viewBox="0 0 10 10" refX="8" refY="5" markerWidth="6" markerHeight="6" orient="auto-start-reverse">
            <path d="M 0 1 L 8 5 L 0 9 z" fill="var(--vp-c-brand-1)" />
          </marker>
        </defs>

        <!-- Connecting Lines -->
        <!-- Pager -> Slotted Pages -->
        <path d="M 170 80 L 260 80" class="edge" marker-end="url(#arrow)" />
        <!-- Slotted Pages -> B+ Tree -->
        <path d="M 400 80 L 490 80" class="edge" marker-end="url(#arrow)" />
        <!-- Pager -> Page 0 Catalog -->
        <path d="M 115 110 L 115 180" class="edge" marker-end="url(#arrow)" />
        
        <!-- Lexer -> Parser -->
        <path d="M 170 280 L 260 280" class="edge" marker-end="url(#arrow)" />
        <!-- Parser -> Executor -->
        <path d="M 400 280 L 490 280" class="edge" marker-end="url(#arrow)" />
        
        <!-- B+ Tree -> Executor -->
        <path d="M 555 110 L 555 250" class="edge" marker-end="url(#arrow)" />
        <!-- Catalog -> Executor -->
        <path d="M 170 210 L 490 280" class="edge" marker-end="url(#arrow)" />
        <!-- Executor -> CLI Shell -->
        <path d="M 620 280 L 680 280" class="edge" marker-end="url(#arrow)" />

        <!-- Nodes -->
        <!-- Node 1: Pager -->
        <g class="node-group" :class="{ selected: selectedNode.id === 'pager' }" @click="select('pager')">
          <rect x="60" y="55" width="110" height="50" rx="6" class="node-box" />
          <text x="115" y="80" text-anchor="middle" class="node-text title">Pager (4KB)</text>
          <text x="115" y="96" text-anchor="middle" class="node-text sub">Block I/O</text>
        </g>

        <!-- Node 2: Slotted Pages -->
        <g class="node-group" :class="{ selected: selectedNode.id === 'slotted' }" @click="select('slotted')">
          <rect x="260" y="55" width="140" height="50" rx="6" class="node-box" />
          <text x="330" y="80" text-anchor="middle" class="node-text title">Slotted Pages</text>
          <text x="330" y="96" text-anchor="middle" class="node-text sub">Records & Slots</text>
        </g>

        <!-- Node 3: B+ Tree -->
        <g class="node-group" :class="{ selected: selectedNode.id === 'bplus' }" @click="select('bplus')">
          <rect x="490" y="55" width="130" height="50" rx="6" class="node-box" />
          <text x="555" y="80" text-anchor="middle" class="node-text title">Disk B+ Tree</text>
          <text x="555" y="96" text-anchor="middle" class="node-text sub">O(log N) Index</text>
        </g>

        <!-- Node 4: Catalog Page 0 -->
        <g class="node-group" :class="{ selected: selectedNode.id === 'catalog' }" @click="select('catalog')">
          <rect x="60" y="180" width="110" height="50" rx="6" class="node-box" />
          <text x="115" y="205" text-anchor="middle" class="node-text title">Page 0 Catalog</text>
          <text x="115" y="221" text-anchor="middle" class="node-text sub">Schema Metadata</text>
        </g>

        <!-- Node 5: Lexer -->
        <g class="node-group" :class="{ selected: selectedNode.id === 'lexer' }" @click="select('lexer')">
          <rect x="60" y="255" width="110" height="50" rx="6" class="node-box" />
          <text x="115" y="280" text-anchor="middle" class="node-text title">Lexer</text>
          <text x="115" y="296" text-anchor="middle" class="node-text sub">Token Stream</text>
        </g>

        <!-- Node 6: Parser -->
        <g class="node-group" :class="{ selected: selectedNode.id === 'parser' }" @click="select('parser')">
          <rect x="260" y="255" width="140" height="50" rx="6" class="node-box" />
          <text x="330" y="280" text-anchor="middle" class="node-text title">AST Parser</text>
          <text x="330" y="296" text-anchor="middle" class="node-text sub">Recursive Descent</text>
        </g>

        <!-- Node 7: Executor -->
        <g class="node-group" :class="{ selected: selectedNode.id === 'executor' }" @click="select('executor')">
          <rect x="490" y="255" width="130" height="50" rx="6" class="node-box highlight" />
          <text x="555" y="280" text-anchor="middle" class="node-text title">Query Executor</text>
          <text x="555" y="296" text-anchor="middle" class="node-text sub">Filter & Join</text>
        </g>

        <!-- Node 8: CLI -->
        <g class="node-group" :class="{ selected: selectedNode.id === 'cli' }" @click="select('cli')">
          <rect x="680" y="255" width="90" height="50" rx="6" class="node-box" />
          <text x="725" y="280" text-anchor="middle" class="node-text title">REPL CLI</text>
          <text x="725" y="296" text-anchor="middle" class="node-text sub">Interactive</text>
        </g>
      </svg>
    </div>

    <!-- Active Node Info Box -->
    <div class="node-details">
      <div class="detail-header">
        <span class="detail-title">{{ selectedNode.title }}</span>
        <span class="detail-files"><code>{{ selectedNode.files }}</code></span>
      </div>
      <p class="detail-desc">{{ selectedNode.desc }}</p>
      <div class="detail-meta">
        <span><strong>Prerequisites:</strong> {{ selectedNode.prereqs }}</span>
        <a :href="selectedNode.link" class="detail-link">Read Chapter Specification →</a>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref } from 'vue'

const nodesData = {
  pager: {
    id: 'pager',
    title: 'Block Pager (4096-Byte I/O)',
    files: 'storage/pager.hpp, src/storage/pager.cpp',
    desc: 'Lowest level storage engine component. Manages physical disk seeks, fixed 4KB page alignment, free-list page recycling, and raw block reading/writing.',
    prereqs: 'None (Foundation)',
    link: '/architecture/pager'
  },
  slotted: {
    id: 'slotted',
    title: 'Slotted-Page Architecture & Records',
    files: 'storage/table.hpp, storage/record.hpp',
    desc: 'Manages variable-length tuples inside 4KB pages. Uses two-way growth (slot array growing forward, record payloads growing backward) and page compaction.',
    prereqs: 'Block Pager',
    link: '/architecture/slotted-pages'
  },
  bplus: {
    id: 'bplus',
    title: 'Disk-Backed B+ Tree Index',
    files: 'index/bplus_tree.hpp, src/index/bplus_tree.cpp',
    desc: 'Maintains balanced multi-way index pages on disk. Handles MAX_KEYS=128 splitting, root promotion, O(log N) point lookup, and horizontal leaf range scans.',
    prereqs: 'Block Pager, Slotted Pages',
    link: '/architecture/bplus-tree'
  },
  catalog: {
    id: 'catalog',
    title: 'Page 0 Database Catalog',
    files: 'quartz/database.hpp, src/quartz/database.cpp',
    desc: 'Serializes and validates table metadata, schemas, column constraints, and B+ tree root pointers onto Page 0 with magic bytes QDB1.',
    prereqs: 'Block Pager',
    link: '/architecture/pager'
  },
  lexer: {
    id: 'lexer',
    title: 'Lexical Token Scanner',
    files: 'sql/lexer.hpp, src/sql/lexer.cpp',
    desc: 'Character-by-character SQL scanner recognizing keywords, quoted strings with escapes, numbers, and operators into strongly-typed Token structs.',
    prereqs: 'None',
    link: '/sql/reference'
  },
  parser: {
    id: 'parser',
    title: 'Recursive-Descent AST Parser',
    files: 'sql/parser.hpp, src/sql/parser.cpp',
    desc: 'Consumes tokens to generate type-safe Statement ASTs: CreateTable, Select, Insert, Update, and Delete with boolean WHERE expression trees.',
    prereqs: 'Lexer',
    link: '/sql/reference'
  },
  executor: {
    id: 'executor',
    title: 'Relational Query Executor',
    files: 'execution/executor.hpp, src/execution/executor.cpp',
    desc: 'Coordinates query plans. Chooses between O(log N) B+ Tree index point lookups and slotted-page scans, filters predicates, and evaluates joins.',
    prereqs: 'Parser, Database Catalog, B+ Tree, Table',
    link: '/architecture/overview'
  },
  cli: {
    id: 'cli',
    title: 'Interactive Production REPL',
    files: 'src/cli/main.cpp',
    desc: 'Interactive terminal accumulating multi-line SQL, formatted ASCII grid tables, schema inspection (.schema, .tables), and database switching.',
    prereqs: 'Query Executor',
    link: '/sql/interactive-cli'
  }
}

const selectedNode = ref(nodesData.bplus)

const select = (id) => {
  if (nodesData[id]) {
    selectedNode.value = nodesData[id]
  }
}
</script>

<style scoped>
.graph-card {
  border: 1px solid var(--vp-c-border);
  background: var(--vp-c-bg-elv);
  border-radius: 8px;
  margin: 2rem 0;
  overflow: hidden;
  box-shadow: var(--vp-shadow-1);
}

.graph-header {
  padding: 14px 18px;
  background: var(--vp-c-bg-soft);
  border-bottom: 1px solid var(--vp-c-border);
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 10px;
}

.graph-badge {
  font-family: var(--vp-font-family-mono);
  font-size: 0.65rem;
  font-weight: 700;
  color: var(--vp-c-brand-1);
  letter-spacing: 0.08em;
}

.graph-title {
  margin: 2px 0 0 0;
  font-size: 1.1rem;
  font-family: var(--vp-font-family-sans);
  color: var(--vp-c-text-1);
}

.graph-legend {
  display: flex;
  gap: 12px;
  font-family: var(--vp-font-family-sans);
  font-size: 0.75rem;
  color: var(--vp-c-text-2);
}

.legend-item {
  display: flex;
  align-items: center;
  gap: 5px;
}

.dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
}
.dot.done { background: var(--vp-c-brand-1); }
.dot.active { background: #0284c7; }

.svg-wrapper {
  padding: 1.5rem 1rem;
  background: var(--vp-c-bg);
  display: flex;
  justify-content: center;
  overflow-x: auto;
}

.graph-svg {
  width: 100%;
  max-width: 800px;
  height: auto;
}

.edge {
  fill: none;
  stroke: var(--vp-c-border);
  stroke-width: 2;
  stroke-dasharray: 4;
}

.node-group {
  cursor: pointer;
  transition: all 0.2s ease;
}

.node-box {
  fill: var(--vp-c-bg-elv);
  stroke: var(--vp-c-border);
  stroke-width: 1.5;
  transition: all 0.2s ease;
}

.node-box.highlight {
  stroke: var(--vp-c-brand-1);
}

.node-group:hover .node-box {
  stroke: var(--vp-c-brand-1);
  filter: drop-shadow(0 2px 6px rgba(0,0,0,0.1));
}

.node-group.selected .node-box {
  stroke: var(--vp-c-brand-1);
  stroke-width: 2.5;
  fill: var(--vp-c-brand-soft);
}

.node-text.title {
  font-family: var(--vp-font-family-sans);
  font-weight: 600;
  font-size: 13px;
  fill: var(--vp-c-text-1);
}

.node-text.sub {
  font-family: var(--vp-font-family-mono);
  font-size: 10px;
  fill: var(--vp-c-text-3);
}

.node-details {
  border-top: 1px solid var(--vp-c-border);
  background: var(--vp-c-bg-elv);
  padding: 16px 20px;
}

.detail-header {
  display: flex;
  justify-content: space-between;
  align-items: baseline;
  flex-wrap: wrap;
  gap: 8px;
  margin-bottom: 6px;
}

.detail-title {
  font-family: var(--vp-font-family-base);
  font-size: 1.15rem;
  font-weight: 700;
  color: var(--vp-c-text-1);
}

.detail-files {
  font-size: 0.8rem;
  color: var(--vp-c-text-2);
}

.detail-desc {
  margin: 0 0 10px 0;
  font-size: 0.9rem;
  color: var(--vp-c-text-2);
}

.detail-meta {
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-size: 0.8rem;
  color: var(--vp-c-text-3);
  font-family: var(--vp-font-family-sans);
}

.detail-link {
  color: var(--vp-c-brand-1);
  text-decoration: none;
  font-weight: 600;
}
.detail-link:hover {
  text-decoration: underline;
}
</style>
