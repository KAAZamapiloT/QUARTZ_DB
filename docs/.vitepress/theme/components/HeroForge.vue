<template>
  <div class="forge-container">
    <!-- Top Serial Header Bar -->
    <header class="forge-masthead">
      <div class="forge-masthead-top">
        <div class="masthead-brand">
          <span class="brand-initial">Q.</span>
          <span class="brand-name">QuartzDB</span>
          <span class="brand-sub">a serial publication for database & systems programmers</span>
        </div>
        <div class="masthead-nav">
          <a href="/overview" class="nav-item">Overview</a>
          <a href="/tables" class="nav-item">Tables</a>
          <a href="/diagrams" class="nav-item">ERD</a>
          <a href="/data-dictionary" class="nav-item">Dictionary</a>
          <button @click="toggleDim" class="dim-btn" :title="isDim ? 'Switch to Paper Mode' : 'Switch to Dim Mode'">
            {{ isDim ? 'PAPER' : 'DIM' }}
          </button>
        </div>
      </div>
      <div class="forge-masthead-meta">
        <span>Vol. I § Issue 01 § Published Autumn 2026</span>
        <span>Edited by the QuartzDB Engineering Team</span>
      </div>
    </header>

    <!-- Split Hero Section -->
    <section class="forge-hero">
      <!-- Left Column: Featured Serial -->
      <div class="hero-left">
        <div class="hero-pretitle">
          <span class="pretitle-tag">Currently Serializing</span>
          <span class="pretitle-issue">№ 5 of 9 · published weekly</span>
        </div>

        <h1 class="hero-title">
          Systems Programming<br>
          <span class="title-italic">in C++23.</span>
        </h1>

        <p class="hero-deck">
          A NINE-CHAPTER DEEP DIVE INTO THE MACHINE LAYER — Slotted pages, disk-backed B+ tree indexing, fixed 4KB page pagers, AST parsing, and zero-dependency relational execution. Written for engineers who want to understand databases from the metal up.
        </p>

        <div class="hero-actions">
          <a href="/overview" class="btn-primary">Read Overview</a>
          <a href="/tables" class="btn-secondary">Browse Tables (3)</a>
          <a href="/diagrams" class="btn-secondary">Interactive ERD</a>
        </div>
      </div>

      <!-- Right Column: This Week Schedule -->
      <div class="hero-right">
        <div class="schedule-card">
          <div class="schedule-header">
            <span class="schedule-title">This Week</span>
            <span class="schedule-sub">five chapters · core storage</span>
          </div>

          <ul class="schedule-list">
            <li class="schedule-item">
              <span class="item-date">Ch. 1</span>
              <span class="item-desc">4KB Block Pager (I/O)</span>
              <span class="badge-status out">OUT</span>
            </li>
            <li class="schedule-item">
              <span class="item-date">Ch. 2</span>
              <span class="item-desc">Slotted-Page Storage</span>
              <span class="badge-status out">OUT</span>
            </li>
            <li class="schedule-item">
              <span class="item-date">Ch. 3</span>
              <span class="item-desc">Disk B+ Tree Indexing</span>
              <span class="badge-status out">OUT</span>
            </li>
            <li class="schedule-item">
              <span class="item-date">Ch. 4</span>
              <span class="item-desc">Recursive-Descent Parser</span>
              <span class="badge-status scheduled">READY</span>
            </li>
            <li class="schedule-item">
              <span class="item-date">Ch. 5</span>
              <span class="item-desc">Execution Engine & CLI</span>
              <span class="badge-status scheduled">LIVE</span>
            </li>
          </ul>

          <div class="schedule-footer">
            <span>Next Release: Ch. 6 · Write-Ahead Logging</span>
          </div>
        </div>
      </div>
    </section>

    <!-- Series Library Grid -->
    <section class="series-library">
      <div class="section-heading">
        <h2>Series Library & Internal Modules</h2>
        <p>Track coverage and progress across the QuartzDB architectural curriculum.</p>
      </div>

      <div class="series-grid">
        <div v-for="s in series" :key="s.title" class="series-card">
          <div class="series-header">
            <span class="series-vol">{{ s.tag }}</span>
            <span class="series-pct">{{ s.progress }}% Complete</span>
          </div>
          <h3 class="series-card-title">{{ s.title }}</h3>
          <p class="series-card-desc">{{ s.description }}</p>
          <div class="progress-bar-bg">
            <div class="progress-bar-fill" :style="{ width: s.progress + '%' }"></div>
          </div>
          <div class="series-card-footer">
            <span>{{ s.chapters }} chapters</span>
            <a :href="s.link" class="series-link">View Module →</a>
          </div>
        </div>
      </div>
    </section>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'

const isDim = ref(false)

const toggleDim = () => {
  const html = document.documentElement
  isDim.value = !isDim.value
  if (isDim.value) {
    html.classList.add('dark')
    localStorage.setItem('vitepress-theme-appearance', 'dark')
  } else {
    html.classList.remove('dark')
    localStorage.setItem('vitepress-theme-appearance', 'light')
  }
}

onMounted(() => {
  isDim.value = document.documentElement.classList.contains('dark')
})

const series = [
  {
    tag: 'MODULE I',
    title: 'Block Storage & Pager Architecture',
    description: 'Fixed 4096-byte page boundaries, block seek mechanics, dynamic page allocations, and Page 0 Catalog persistence.',
    progress: 100,
    chapters: 3,
    link: '/architecture/pager'
  },
  {
    tag: 'MODULE II',
    title: 'Slotted-Page Storage & Records',
    description: 'Slot array addressing, dynamic payload growth, RecordCodec serialization, RecordID pointers, and page compaction.',
    progress: 100,
    chapters: 4,
    link: '/architecture/slotted-pages'
  },
  {
    tag: 'MODULE III',
    title: 'Disk-Backed B+ Tree Indexing',
    description: 'Persistent multi-way tree balancing, leaf and internal node splits, O(log N) point lookup, and underflow rebalancing.',
    progress: 100,
    chapters: 3,
    link: '/architecture/bplus-tree'
  },
  {
    tag: 'MODULE IV',
    title: 'Relational Schema & SQL Engine',
    description: 'Token scanner, recursive AST parsing, boolean predicate filtering, relational JOIN execution, and the interactive REPL.',
    progress: 100,
    chapters: 5,
    link: '/sql/reference'
  }
]
</script>

<style scoped>
.forge-container {
  max-width: 1180px;
  margin: 0 auto;
  padding: 1rem 1.5rem 4rem;
}

/* Masthead */
.forge-masthead {
  border-bottom: 2px solid var(--vp-c-text-1);
  padding-bottom: 0.75rem;
  margin-bottom: 3rem;
}

.forge-masthead-top {
  display: flex;
  justify-content: space-between;
  align-items: baseline;
  flex-wrap: wrap;
  gap: 1rem;
}

.masthead-brand {
  display: flex;
  align-items: baseline;
  gap: 0.5rem;
}

.brand-initial {
  font-family: var(--vp-font-family-base);
  font-size: 2rem;
  font-weight: 700;
  color: var(--vp-c-brand-1);
}

.brand-name {
  font-family: var(--vp-font-family-base);
  font-size: 1.6rem;
  font-weight: 700;
  letter-spacing: -0.02em;
}

.brand-sub {
  font-family: var(--vp-font-family-base);
  font-style: italic;
  font-size: 0.95rem;
  color: var(--vp-c-text-2);
  margin-left: 0.5rem;
  border-left: 1px solid var(--vp-c-border);
  padding-left: 0.75rem;
}

.masthead-nav {
  display: flex;
  align-items: center;
  gap: 1.25rem;
  font-family: var(--vp-font-family-sans);
  font-size: 0.85rem;
  font-weight: 500;
  letter-spacing: 0.05em;
  text-transform: uppercase;
}

.nav-item {
  color: var(--vp-c-text-2);
  text-decoration: none;
  transition: color 0.15s ease;
}

.nav-item:hover {
  color: var(--vp-c-brand-1);
}

.dim-btn {
  border: 1px solid var(--vp-c-border);
  background: var(--vp-c-bg-elv);
  padding: 4px 10px;
  font-size: 0.75rem;
  font-weight: 600;
  letter-spacing: 0.1em;
  cursor: pointer;
  border-radius: 3px;
  color: var(--vp-c-text-1);
  transition: all 0.2s ease;
}

.dim-btn:hover {
  border-color: var(--vp-c-text-1);
}

.forge-masthead-meta {
  display: flex;
  justify-content: space-between;
  margin-top: 0.75rem;
  padding-top: 0.5rem;
  border-top: 1px solid var(--vp-c-border);
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  color: var(--vp-c-text-3);
  letter-spacing: 0.05em;
  text-transform: uppercase;
}

/* Split Hero */
.forge-hero {
  display: grid;
  grid-template-columns: 1.35fr 1fr;
  gap: 3.5rem;
  align-items: start;
  margin-bottom: 4rem;
}

.hero-pretitle {
  display: flex;
  gap: 1rem;
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  text-transform: uppercase;
  letter-spacing: 0.08em;
  margin-bottom: 1.25rem;
}

.pretitle-tag {
  color: var(--vp-c-brand-1);
  font-weight: 700;
}

.pretitle-issue {
  color: var(--vp-c-text-3);
}

.hero-title {
  font-family: var(--vp-font-family-base);
  font-size: 3.8rem;
  line-height: 1.05;
  font-weight: 700;
  margin: 0 0 1.5rem 0;
  letter-spacing: -0.03em;
  color: var(--vp-c-text-1);
}

.title-italic {
  font-style: italic;
  font-weight: 400;
  color: var(--vp-c-brand-1);
}

.hero-deck {
  font-family: var(--vp-font-family-base);
  font-size: 1.15rem;
  line-height: 1.6;
  color: var(--vp-c-text-2);
  margin-bottom: 2rem;
  max-width: 580px;
}

.hero-actions {
  display: flex;
  gap: 0.85rem;
  flex-wrap: wrap;
}

.btn-primary {
  background: var(--vp-c-text-1);
  color: var(--vp-c-bg);
  padding: 10px 20px;
  font-family: var(--vp-font-family-sans);
  font-size: 0.85rem;
  font-weight: 600;
  border-radius: 4px;
  text-decoration: none;
  transition: opacity 0.2s;
}

.btn-primary:hover {
  opacity: 0.9;
}

.btn-secondary {
  border: 1px solid var(--vp-c-border);
  background: var(--vp-c-bg-elv);
  color: var(--vp-c-text-1);
  padding: 10px 18px;
  font-family: var(--vp-font-family-sans);
  font-size: 0.85rem;
  font-weight: 600;
  border-radius: 4px;
  text-decoration: none;
  transition: all 0.2s;
}

.btn-secondary:hover {
  border-color: var(--vp-c-text-1);
}

/* Schedule Card */
.schedule-card {
  border: 1px solid var(--vp-c-border);
  background: var(--vp-c-bg-elv);
  border-radius: 6px;
  padding: 1.5rem;
  box-shadow: var(--vp-shadow-1);
}

.schedule-header {
  display: flex;
  justify-content: space-between;
  align-items: baseline;
  padding-bottom: 0.75rem;
  border-bottom: 1px solid var(--vp-c-border);
  margin-bottom: 1rem;
}

.schedule-title {
  font-family: var(--vp-font-family-mono);
  font-size: 0.85rem;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 0.05em;
  color: var(--vp-c-text-1);
}

.schedule-sub {
  font-family: var(--vp-font-family-base);
  font-style: italic;
  font-size: 0.8rem;
  color: var(--vp-c-text-3);
}

.schedule-list {
  list-style: none;
  padding: 0;
  margin: 0;
}

.schedule-item {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0.75rem 0;
  border-bottom: 1px solid var(--vp-c-divider);
  font-size: 0.9rem;
}

.schedule-item:last-child {
  border-bottom: none;
}

.item-date {
  font-family: var(--vp-font-family-mono);
  font-size: 0.8rem;
  color: var(--vp-c-text-3);
  width: 50px;
}

.item-desc {
  flex: 1;
  font-family: var(--vp-font-family-base);
  font-weight: 600;
  color: var(--vp-c-text-1);
  padding: 0 0.5rem;
}

.badge-status {
  font-family: var(--vp-font-family-mono);
  font-size: 0.65rem;
  font-weight: 700;
  padding: 2px 6px;
  border-radius: 3px;
  letter-spacing: 0.05em;
}

.badge-status.out {
  border: 1px solid var(--vp-c-text-1);
  color: var(--vp-c-text-1);
}

.badge-status.scheduled {
  border: 1px solid var(--vp-c-brand-1);
  color: var(--vp-c-brand-1);
}

.schedule-footer {
  margin-top: 1rem;
  padding-top: 0.75rem;
  border-top: 1px dashed var(--vp-c-border);
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  color: var(--vp-c-text-3);
  text-align: right;
  font-style: italic;
}

/* Series Library */
.series-library {
  margin-top: 4rem;
  padding-top: 2rem;
  border-top: 1px solid var(--vp-c-border);
}

.section-heading h2 {
  font-size: 2rem;
  margin-bottom: 0.5rem;
}

.section-heading p {
  color: var(--vp-c-text-2);
  margin-bottom: 2rem;
}

.series-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(260px, 1fr));
  gap: 1.5rem;
}

.series-card {
  border: 1px solid var(--vp-c-border);
  background: var(--vp-c-bg-elv);
  padding: 1.5rem;
  border-radius: 6px;
  display: flex;
  flex-direction: column;
  transition: transform 0.2s, box-shadow 0.2s;
}

.series-card:hover {
  transform: translateY(-2px);
  box-shadow: var(--vp-shadow-2);
}

.series-header {
  display: flex;
  justify-content: space-between;
  font-family: var(--vp-font-family-mono);
  font-size: 0.7rem;
  margin-bottom: 0.75rem;
}

.series-vol {
  color: var(--vp-c-brand-1);
  font-weight: 700;
}

.series-pct {
  color: var(--vp-c-text-3);
}

.series-card-title {
  font-size: 1.25rem;
  margin: 0 0 0.5rem 0;
  line-height: 1.3;
}

.series-card-desc {
  font-size: 0.9rem;
  color: var(--vp-c-text-2);
  flex: 1;
  margin-bottom: 1rem;
}

.progress-bar-bg {
  height: 4px;
  background: var(--vp-c-bg-soft);
  border-radius: 2px;
  overflow: hidden;
  margin-bottom: 1rem;
}

.progress-bar-fill {
  height: 100%;
  background: var(--vp-c-brand-1);
}

.series-card-footer {
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  color: var(--vp-c-text-3);
}

.series-link {
  color: var(--vp-c-text-1);
  text-decoration: none;
  font-weight: 600;
}

.series-link:hover {
  color: var(--vp-c-brand-1);
}

@media (max-width: 900px) {
  .forge-hero {
    grid-template-columns: 1fr;
    gap: 2rem;
  }
  .hero-title {
    font-size: 2.8rem;
  }
}
</style>
