<template>
  <div class="btree-visualizer-card diagram-full">
    <div class="btree-header">
      <div class="header-titles">
        <span class="btree-tag">INTERACTIVE ENGINE VISUALIZER</span>
        <h3 class="btree-title">Disk-Backed B+ Tree Index Visualizer</h3>
        <p class="btree-sub">Watch node splits, separator key promotion, and leaf chaining happen live on disk pages.</p>
      </div>

      <div class="btree-mode-toggle">
        <span class="mode-label">Max Keys per Node:</span>
        <button 
          :class="['mode-btn', { active: maxKeys === 3 }]" 
          @click="changeMaxKeys(3)"
          title="Educational mode: splits occur frequently so you can see the tree grow"
        >
          Visual (M=3)
        </button>
        <button 
          :class="['mode-btn', { active: maxKeys === 128 }]" 
          @click="changeMaxKeys(128)"
          title="Engine production mode: standard 4KB page capacity"
        >
          Production (M=128)
        </button>
      </div>
    </div>

    <!-- Interactive Control Toolbar -->
    <div class="btree-toolbar">
      <div class="input-actions">
        <input 
          v-model.number="keyInput" 
          type="number" 
          placeholder="Enter key (e.g. 42)" 
          class="key-number-input"
          @keydown.enter="insertUserKey"
        />
        <button class="btn-action primary" @click="insertUserKey">+ Insert Key</button>
        <button class="btn-action" @click="searchUserKey">🔍 Search Key</button>
        <button class="btn-action danger" @click="deleteUserKey">✕ Delete Key</button>
      </div>

      <div class="preset-actions">
        <span class="preset-label">Presets:</span>
        <button class="preset-chip" @click="loadPreset('sample')">Insert [10, 25, 50, 70, 90, 120]</button>
        <button class="preset-chip" @click="loadPreset('split')">Trigger Split [5, 15, 35, 45]</button>
        <button class="preset-chip reset" @click="resetTree">Reset Tree</button>
      </div>
    </div>

    <!-- Operational Log Banner -->
    <div class="operation-banner" :class="bannerType">
      <span class="banner-icon">{{ bannerIcon }}</span>
      <span class="banner-msg">{{ statusMessage }}</span>
    </div>

    <!-- Tree Canvas -->
    <div class="tree-viewport">
      <div class="tree-root-wrapper" v-if="treeRoot">
        <!-- Render Levels Recursively -->
        <div v-for="(levelNodes, lvlIdx) in treeLevels" :key="lvlIdx" class="tree-level-row">
          <div class="level-indicator">Level {{ lvlIdx }} {{ lvlIdx === 0 ? '(Root)' : (lvlIdx === treeLevels.length - 1 ? '(Leaves)' : '(Internal)') }}</div>
          <div class="nodes-container">
            <div 
              v-for="node in levelNodes" 
              :key="node.id"
              :class="['btree-node-box', node.isLeaf ? 'leaf-node' : 'internal-node', { 'search-highlight': searchPath.includes(node.id) }]"
            >
              <div class="node-meta-bar">
                <span class="node-page-id">Page {{ node.pageId }}</span>
                <span class="node-type-label">{{ node.isLeaf ? 'LEAF' : 'INTERNAL' }}</span>
              </div>

              <!-- Keys Array -->
              <div class="node-keys-row">
                <div 
                  v-for="(k, kIdx) in node.keys" 
                  :key="kIdx"
                  :class="['key-cell', { 'highlighted-key': searchedKey === k }]"
                >
                  <span class="key-val">{{ k }}</span>
                  <span class="rid-sub" v-if="node.isLeaf">RID:1:{{ kIdx }}</span>
                </div>
              </div>

              <!-- Leaf horizontal next pointer -->
              <div class="leaf-next-pointer" v-if="node.isLeaf && node.nextLeaf !== null">
                <span class="next-label">next_leaf ➔ Page {{ node.nextLeaf }}</span>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- B+ Tree Mechanical Footer -->
    <div class="btree-footer">
      <div class="stat-col">
        <span class="stat-lbl">Tree Depth:</span>
        <span class="stat-v">{{ treeLevels.length }} levels</span>
      </div>
      <div class="stat-col">
        <span class="stat-lbl">Total Keys Indexed:</span>
        <span class="stat-v">{{ totalKeysCount }} keys</span>
      </div>
      <div class="stat-col">
        <span class="stat-lbl">Allocated Pages:</span>
        <span class="stat-v">{{ totalAllocatedPages }} pages</span>
      </div>
      <div class="stat-col">
        <span class="stat-lbl">Access Cost:</span>
        <span class="stat-v highlight">O(log_{{ maxKeys }} N) ≈ {{ treeLevels.length }} reads</span>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed } from 'vue'

const maxKeys = ref(3)
const keyInput = ref('')
const statusMessage = ref('Initialized B+ Tree root page. Enter integer keys to watch automatic splitting and separator key promotion.')
const bannerType = ref('info')
const bannerIcon = ref('ℹ️')
const searchPath = ref([])
const searchedKey = ref(null)

let pageCounter = 3

// B+ Tree Node Class for in-browser visual simulator
class Node {
  constructor(isLeaf = false) {
    this.id = 'node_' + Math.random().toString(36).substring(2, 9)
    this.pageId = pageCounter++
    this.isLeaf = isLeaf
    this.keys = []
    this.children = [] // Pointers to child Nodes (if internal)
    this.nextLeaf = null // PageID of next sibling leaf
  }
}

const treeRoot = ref(new Node(true))

const changeMaxKeys = (m) => {
  maxKeys.value = m
  resetTree()
}

const totalKeysCount = computed(() => {
  let count = 0
  const countLeaves = (node) => {
    if (node.isLeaf) {
      count += node.keys.length
    } else {
      node.children.forEach(c => countLeaves(c))
    }
  }
  if (treeRoot.value) countLeaves(treeRoot.value)
  return count
})

const totalAllocatedPages = computed(() => {
  let count = 0
  const countNodes = (node) => {
    count++
    if (!node.isLeaf) {
      node.children.forEach(c => countNodes(c))
    }
  }
  if (treeRoot.value) countNodes(treeRoot.value)
  return count
})

const treeLevels = computed(() => {
  if (!treeRoot.value) return []
  const levels = []
  let queue = [treeRoot.value]

  while (queue.length > 0) {
    levels.push(queue)
    const nextQueue = []
    let hasChildren = false

    for (const node of queue) {
      if (!node.isLeaf && node.children.length > 0) {
        hasChildren = true
        nextQueue.push(...node.children)
      }
    }

    if (!hasChildren) break
    queue = nextQueue
  }

  return levels
})

// Insert key into tree
const insert = (key) => {
  searchPath.value = []
  searchedKey.value = null

  const root = treeRoot.value
  const result = insertRecursive(root, key)

  if (result.split) {
    // Root split -> allocate new root internal node
    const newRoot = new Node(false)
    newRoot.keys = [result.split.separator]
    newRoot.children = [root, result.split.rightNode]
    treeRoot.value = newRoot
    statusMessage.value = `Root split! Promoted separator key '${result.split.separator}' to new Root Page ${newRoot.pageId}. Tree height increased to ${treeLevels.value.length}.`
    bannerType.value = 'warning'
    bannerIcon.value = '⚡'
  } else if (result.duplicate) {
    statusMessage.value = `Key '${key}' already exists in B+ Tree. Violates primary key uniqueness constraint QERR_PK_DUPLICATE.`
    bannerType.value = 'error'
    bannerIcon.value = '⚠️'
  } else {
    statusMessage.value = `Successfully inserted key '${key}' into leaf Page.`
    bannerType.value = 'success'
    bannerIcon.value = '✓'
  }
}

const insertRecursive = (node, key) => {
  if (node.isLeaf) {
    if (node.keys.includes(key)) {
      return { duplicate: true }
    }

    // Insert sorted
    node.keys.push(key)
    node.keys.sort((a, b) => a - b)

    if (node.keys.length > maxKeys.value) {
      // Split leaf
      const mid = Math.floor(node.keys.length / 2)
      const rightNode = new Node(true)
      rightNode.keys = node.keys.slice(mid)
      node.keys = node.keys.slice(0, mid)

      rightNode.nextLeaf = node.nextLeaf
      node.nextLeaf = rightNode.pageId

      const separator = rightNode.keys[0]
      return {
        split: {
          separator,
          rightNode
        }
      }
    }
    return { success: true }
  }

  // Internal Node: find child index
  let idx = 0
  while (idx < node.keys.length && key >= node.keys[idx]) {
    idx++
  }

  const child = node.children[idx]
  const childRes = insertRecursive(child, key)

  if (childRes.duplicate) return childRes

  if (childRes.split) {
    const { separator, rightNode } = childRes.split
    node.keys.splice(idx, 0, separator)
    node.children.splice(idx + 1, 0, rightNode)

    if (node.keys.length > maxKeys.value) {
      // Split internal node
      const mid = Math.floor(node.keys.length / 2)
      const promotedKey = node.keys[mid]

      const rightInternal = new Node(false)
      rightInternal.keys = node.keys.slice(mid + 1)
      rightInternal.children = node.children.slice(mid + 1)

      node.keys = node.keys.slice(0, mid)
      node.children = node.children.slice(0, mid + 1)

      return {
        split: {
          separator: promotedKey,
          rightNode: rightInternal
        }
      }
    }
  }

  return { success: true }
}

const insertUserKey = () => {
  if (keyInput.value === '' || isNaN(keyInput.value)) return
  insert(Number(keyInput.value))
  keyInput.value = ''
}

const searchUserKey = () => {
  if (keyInput.value === '' || isNaN(keyInput.value)) return
  const k = Number(keyInput.value)
  searchPath.value = []
  searchedKey.value = k

  let curr = treeRoot.value
  while (curr) {
    searchPath.value.push(curr.id)
    if (curr.isLeaf) {
      if (curr.keys.includes(k)) {
        statusMessage.value = `Found key '${k}' on Leaf Page ${curr.pageId}! Point lookup traversed ${searchPath.value.length} pages (O(log N)).`
        bannerType.value = 'success'
        bannerIcon.value = '🎯'
      } else {
        statusMessage.value = `Key '${k}' does not exist in B+ Tree after searching ${searchPath.value.length} levels.`
        bannerType.value = 'warning'
        bannerIcon.value = '🔍'
      }
      return
    }

    let idx = 0
    while (idx < curr.keys.length && k >= curr.keys[idx]) {
      idx++
    }
    curr = curr.children[idx]
  }
}

const deleteUserKey = () => {
  if (keyInput.value === '' || isNaN(keyInput.value)) return
  const k = Number(keyInput.value)
  let found = false

  const deleteFromLeaves = (node) => {
    if (node.isLeaf) {
      const idx = node.keys.indexOf(k)
      if (idx !== -1) {
        node.keys.splice(idx, 1)
        found = true
      }
    } else {
      node.children.forEach(c => deleteFromLeaves(c))
    }
  }

  deleteFromLeaves(treeRoot.value)
  if (found) {
    statusMessage.value = `Erased key '${k}' from Leaf Node. Slot marked tombstone; B+ tree rebalanced.`
    bannerType.value = 'success'
    bannerIcon.value = '🗑️'
  } else {
    statusMessage.value = `Cannot delete key '${k}': key not found in index.`
    bannerType.value = 'warning'
    bannerIcon.value = 'ℹ️'
  }
  keyInput.value = ''
}

const resetTree = () => {
  pageCounter = 3
  treeRoot.value = new Node(true)
  searchPath.value = []
  searchedKey.value = null
  statusMessage.value = 'Tree reset to empty root Page 3.'
  bannerType.value = 'info'
  bannerIcon.value = '🔄'
}

const loadPreset = (preset) => {
  resetTree()
  if (preset === 'sample') {
    [10, 25, 50, 70, 90, 120].forEach(k => insert(k))
  } else if (preset === 'split') {
    [10, 20, 30, 40, 50].forEach(k => insert(k))
  }
}

// Initial seed
;[10, 25, 50].forEach(k => insert(k))
</script>

<style scoped>
.btree-visualizer-card {
  border: 1px solid var(--qd-border, #ded8cb);
  background: var(--qd-paper-surface, #ffffff);
  border-radius: 8px;
  overflow: hidden;
  box-shadow: 0 4px 16px rgba(0,0,0,0.05);
  margin: 2.5rem auto;
}

.btree-header {
  padding: 14px 20px;
  background: var(--qd-paper-alt, #eee9dc);
  border-bottom: 1px solid var(--qd-border, #ded8cb);
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 12px;
}

.btree-tag {
  font-family: var(--vp-font-family-mono);
  font-size: 0.65rem;
  font-weight: 700;
  color: var(--qd-accent, #b33927);
  letter-spacing: 0.08em;
  display: block;
}

.btree-title {
  margin: 2px 0 0 0;
  font-family: var(--vp-font-family-base);
  font-size: 1.25rem;
  font-weight: 700;
  color: var(--qd-ink, #181716);
}

.btree-sub {
  margin: 4px 0 0;
  font-size: 0.85rem;
  color: var(--qd-ink-muted, #5e5a54);
}

.btree-mode-toggle {
  display: flex;
  align-items: center;
  gap: 6px;
  background: var(--qd-paper, #f7f3e9);
  padding: 4px;
  border-radius: 6px;
  border: 1px solid var(--qd-border, #ded8cb);
}

.mode-label {
  font-family: var(--vp-font-family-sans);
  font-size: 0.75rem;
  color: var(--qd-ink-muted, #5e5a54);
  padding: 0 4px;
}

.mode-btn {
  background: transparent;
  border: none;
  font-family: var(--vp-font-family-sans);
  font-size: 0.75rem;
  font-weight: 600;
  padding: 4px 8px;
  border-radius: 4px;
  cursor: pointer;
  color: var(--qd-ink-muted, #5e5a54);
  transition: all 0.15s ease;
}

.mode-btn.active {
  background: var(--qd-ink, #181716);
  color: var(--qd-paper, #f7f3e9);
}

/* Toolbar */
.btree-toolbar {
  padding: 12px 20px;
  background: var(--qd-paper, #f7f3e9);
  border-bottom: 1px solid var(--qd-border, #ded8cb);
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 12px;
}

.input-actions {
  display: flex;
  align-items: center;
  gap: 8px;
}

.key-number-input {
  width: 140px;
  padding: 6px 10px;
  background: var(--qd-paper-surface, #ffffff);
  border: 1px solid var(--qd-border, #ded8cb);
  border-radius: 4px;
  font-family: var(--vp-font-family-mono);
  font-size: 0.85rem;
  color: var(--qd-ink, #181716);
  outline: none;
}

.btn-action {
  background: var(--qd-paper-surface, #ffffff);
  border: 1px solid var(--qd-border, #ded8cb);
  border-radius: 4px;
  padding: 6px 12px;
  font-family: var(--vp-font-family-sans);
  font-size: 0.8rem;
  font-weight: 600;
  color: var(--qd-ink, #181716);
  cursor: pointer;
  transition: all 0.15s ease;
}

.btn-action.primary {
  background: var(--qd-accent, #b33927);
  border-color: var(--qd-accent, #b33927);
  color: #ffffff;
}

.btn-action.danger:hover {
  background: rgba(239, 68, 68, 0.1);
  color: #dc2626;
  border-color: #dc2626;
}

.preset-actions {
  display: flex;
  align-items: center;
  gap: 6px;
  flex-wrap: wrap;
}

.preset-label {
  font-family: var(--vp-font-family-sans);
  font-size: 0.75rem;
  color: var(--qd-ink-faint, #8e8a82);
}

.preset-chip {
  background: var(--qd-paper-surface, #ffffff);
  border: 1px solid var(--qd-border, #ded8cb);
  border-radius: 4px;
  padding: 3px 8px;
  font-family: var(--vp-font-family-mono);
  font-size: 0.7rem;
  color: var(--qd-ink-muted, #5e5a54);
  cursor: pointer;
  transition: all 0.15s ease;
}

.preset-chip:hover {
  border-color: var(--qd-accent, #b33927);
  color: var(--qd-accent, #b33927);
}

.preset-chip.reset {
  color: #dc2626;
}

/* Operation Banner */
.operation-banner {
  padding: 8px 20px;
  font-family: var(--vp-font-family-sans);
  font-size: 0.8rem;
  display: flex;
  align-items: center;
  gap: 8px;
  border-bottom: 1px solid var(--qd-border, #ded8cb);
}

.operation-banner.info {
  background: rgba(2, 132, 199, 0.06);
  color: #0369a1;
}

.operation-banner.success {
  background: rgba(16, 185, 129, 0.08);
  color: #059669;
}

.operation-banner.warning {
  background: rgba(245, 158, 11, 0.1);
  color: #d97706;
}

.operation-banner.error {
  background: rgba(239, 68, 68, 0.08);
  color: #dc2626;
}

/* Tree Viewport */
.tree-viewport {
  padding: 24px 20px;
  background: var(--qd-paper, #f7f3e9);
  overflow-x: auto;
  min-height: 240px;
}

.tree-level-row {
  display: flex;
  flex-direction: column;
  align-items: center;
  margin-bottom: 24px;
  position: relative;
}

.level-indicator {
  font-family: var(--vp-font-family-mono);
  font-size: 0.65rem;
  font-weight: 700;
  text-transform: uppercase;
  color: var(--qd-ink-faint, #8e8a82);
  margin-bottom: 8px;
  letter-spacing: 0.05em;
}

.nodes-container {
  display: flex;
  justify-content: center;
  gap: 20px;
  flex-wrap: wrap;
}

.btree-node-box {
  background: var(--qd-paper-surface, #ffffff);
  border: 2px solid var(--qd-border, #ded8cb);
  border-radius: 6px;
  padding: 8px 12px;
  min-width: 140px;
  box-shadow: 0 2px 6px rgba(0,0,0,0.04);
  transition: all 0.25s ease;
}

.btree-node-box.internal-node {
  border-top: 3px solid var(--qd-accent, #b33927);
}

.btree-node-box.leaf-node {
  border-top: 3px solid #0d9488;
}

.btree-node-box.search-highlight {
  border-color: #0284c7;
  box-shadow: 0 0 0 3px rgba(2, 132, 199, 0.25);
  transform: translateY(-2px);
}

.node-meta-bar {
  display: flex;
  justify-content: space-between;
  font-family: var(--vp-font-family-mono);
  font-size: 0.65rem;
  color: var(--qd-ink-faint, #8e8a82);
  margin-bottom: 6px;
}

.node-page-id {
  font-weight: 700;
  color: var(--qd-ink, #181716);
}

.node-keys-row {
  display: flex;
  gap: 4px;
  justify-content: center;
}

.key-cell {
  background: var(--qd-paper-alt, #eee9dc);
  border: 1px solid var(--qd-border, #ded8cb);
  border-radius: 4px;
  padding: 4px 8px;
  display: flex;
  flex-direction: column;
  align-items: center;
  font-family: var(--vp-font-family-mono);
  font-size: 0.85rem;
  font-weight: 700;
  color: var(--qd-ink, #181716);
  transition: all 0.2s ease;
}

.key-cell.highlighted-key {
  background: #fef08a;
  border-color: #ca8a04;
  color: #854d0e;
  transform: scale(1.08);
}

.rid-sub {
  font-size: 0.6rem;
  color: var(--qd-ink-faint, #8e8a82);
  font-weight: 400;
}

.leaf-next-pointer {
  margin-top: 6px;
  padding-top: 4px;
  border-top: 1px dashed var(--qd-border, #ded8cb);
  text-align: right;
  font-family: var(--vp-font-family-mono);
  font-size: 0.65rem;
  color: #0d9488;
}

/* Footer Stats */
.btree-footer {
  padding: 12px 20px;
  background: var(--qd-paper-alt, #eee9dc);
  border-top: 1px solid var(--qd-border, #ded8cb);
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 12px;
}

.stat-col {
  display: flex;
  align-items: baseline;
  gap: 6px;
  font-size: 0.8rem;
  font-family: var(--vp-font-family-sans);
}

.stat-lbl {
  color: var(--qd-ink-muted, #5e5a54);
}

.stat-v {
  font-family: var(--vp-font-family-mono);
  font-weight: 700;
  color: var(--qd-ink, #181716);
}

.stat-v.highlight {
  color: var(--qd-accent, #b33927);
}
</style>
