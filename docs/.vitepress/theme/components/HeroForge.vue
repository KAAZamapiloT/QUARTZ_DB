<template>
  <div class="forge-container">
    <!-- Systems Monospace Watermark Texture Overlay -->
    <div class="hero-watermark-texture" aria-hidden="true">
      <div class="hex-watermark-stream">
        00000000: 3142 4451 0001 0004 0000 1000 0000 0000  1BDQ............<br>
        00000010: 0005 0000 0000 0000 0001 7573 6572 7300  ..........users.<br>
        00000020: 3147 5051 0002 0018 0f6e 0001 0000 0000  1GPQ.....n......<br>
        00000030: 3154 5042 0001 0200 0001 0000 0000 0000  1TPB............<br>
        00000040: 0000 0000 0000 0005 0000 0000 0000 0064  ...............d<br>
        00000050: 0000 0000 0000 0006 314d 5042 0001 0000  ........1MPB....
      </div>
    </div>

    <!-- Top Serial Header Bar -->
    <header class="forge-masthead">
      <div class="forge-masthead-top">
        <div class="masthead-brand">
          <span class="brand-initial">Q.</span>
          <span class="brand-name">QuartzDB</span>
          <span class="brand-sub">a serial publication for database & systems programmers</span>
        </div>
        <div class="masthead-nav">
          <a href="/getting-started/" class="nav-item">Quickstart</a>
          <a href="/concepts/architecture" class="nav-item">Concepts</a>
          <a href="/sql/" class="nav-item">SQL</a>
          <a href="/configuration/" class="nav-item">Config</a>
          <a href="/errors/" class="nav-item">Errors</a>
          <button @click="toggleDim" class="dim-btn" :title="isDim ? 'Switch to Paper Mode' : 'Switch to Dim Mode'">
            {{ isDim ? 'PAPER' : 'DIM' }}
          </button>
        </div>
      </div>
      <div class="forge-masthead-meta">
        <span>Vol. I § Issue 01 § Published Autumn 2026</span>
        <span>Edited by the QuartzDB Engineering Core</span>
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

        <!-- Hero Actions: Only "Read Overview" is solid button; others are quiet text links with arrows -->
        <div class="hero-actions-row">
          <a href="/overview" class="btn-solid-primary">Read Overview</a>
          <div class="quiet-links-group">
            <a href="/tables" class="quiet-arrow-link">Browse tables · 3 so far →</a>
            <a href="/diagrams" class="quiet-arrow-link">Interactive ERD →</a>
          </div>
        </div>
      </div>

      <!-- Right Column: This Week Chapter Tracker -->
      <div class="hero-right">
        <div class="schedule-card">
          <div class="schedule-header">
            <div class="header-titles">
              <span class="schedule-title">This Week</span>
              <span class="schedule-sub">five chapters · core storage</span>
            </div>
            <div class="tracker-mini-stats">
              <span class="tracker-pct">5 of 9 serialized</span>
            </div>
          </div>

          <!-- Thin Serialization Progress Bar -->
          <div class="thin-tracker-bar">
            <div class="thin-tracker-fill" style="width: 55.5%"></div>
          </div>

          <div class="schedule-list">
            <!-- Ch 1 (Published - Full row link) -->
            <a href="/architecture/pager" class="schedule-row-link">
              <span class="item-date">Ch. 1</span>
              <span class="item-desc">4KB Block Pager (I/O)</span>
              <span class="badge-status read">READ →</span>
            </a>

            <!-- Ch 2 (Published - Full row link) -->
            <a href="/concepts/storage" class="schedule-row-link">
              <span class="item-date">Ch. 2</span>
              <span class="item-desc">Slotted-Page Storage</span>
              <span class="badge-status read">READ →</span>
            </a>

            <!-- Ch 3 (Published - Full row link) -->
            <a href="/concepts/indexing" class="schedule-row-link">
              <span class="item-date">Ch. 3</span>
              <span class="item-desc">Disk B+ Tree Indexing</span>
              <span class="badge-status read">READ →</span>
            </a>

            <!-- Ch 4 (Ready - Full row link) -->
            <a href="/concepts/architecture" class="schedule-row-link">
              <span class="item-date">Ch. 4</span>
              <span class="item-desc">Recursive-Descent Parser</span>
              <span class="badge-status ready">READY</span>
            </a>

            <!-- Ch 5 (Live - Full row link) -->
            <a href="/tools/cli" class="schedule-row-link">
              <span class="item-date">Ch. 5</span>
              <span class="item-desc">Execution Engine & CLI</span>
              <span class="badge-status live">LIVE →</span>
            </a>
          </div>

          <!-- Schedule Footer -->
          <div class="schedule-footer">
            <span class="footer-rss">📡 Subscribe via RSS</span>
            <span class="footer-next">Next: Ch. 6 · Write-Ahead Logging (19 Nov)</span>
          </div>
        </div>
      </div>
    </section>

    <!-- Series Library Grid -->
    <section class="series-library">
      <div class="section-heading">
        <h2>Curriculum & Modules</h2>
        <p>Systematic coverage of QuartzDB's internal C++23 architectural subsystems.</p>
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
    description: 'Fixed 4096-byte boundaries, block seek mechanics, dynamic allocations, and Page 0 Catalog persistence.',
    progress: 100,
    chapters: 3,
    link: '/concepts/storage'
  },
  {
    tag: 'MODULE II',
    title: 'Slotted-Page Storage & Records',
    description: 'Slot array addressing, dynamic payload growth, RecordCodec serialization, RecordID pointers, and compaction.',
    progress: 100,
    chapters: 4,
    link: '/concepts/storage'
  },
  {
    tag: 'MODULE III',
    title: 'Disk-Backed B+ Tree Indexing',
    description: 'Balanced multi-way tree balancing, leaf and internal node splits, O(log N) point lookup, and underflow rebalancing.',
    progress: 100,
    chapters: 3,
    link: '/concepts/indexing'
  },
  {
    tag: 'MODULE IV',
    title: 'Relational Schema & SQL Engine',
    description: 'Token scanner, recursive AST parsing, boolean predicate filtering, relational JOIN execution, and the interactive REPL.',
    progress: 100,
    chapters: 5,
    link: '/sql/'
  }
]
</script>

<style scoped>
.forge-container {
  max-width: 1180px;
  margin: 0 auto;
  padding: 1.5rem 1.5rem 4rem;
  position: relative;
}

/* Monospace Hex Watermark Texture Overlay */
.hero-watermark-texture {
  position: absolute;
  top: 60px;
  right: 10px;
  width: 480px;
  height: 220px;
  pointer-events: none;
  overflow: hidden;
  opacity: 0.038;
  user-select: none;
  z-index: 0;
}

.dark .hero-watermark-texture {
  opacity: 0.05;
}

.hex-watermark-stream {
  font-family: var(--vp-font-family-mono);
  font-size: 11px;
  line-height: 1.8;
  color: var(--qd-ink, #181716);
  white-space: pre;
}

/* Masthead */
.forge-masthead {
  border-bottom: 2px solid var(--qd-ink, #181716);
  padding-bottom: 0.75rem;
  margin-bottom: 3.5rem;
  position: relative;
  z-index: 1;
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
  font-size: 2.2rem;
  font-weight: 700;
  color: var(--qd-accent, #b33927);
  line-height: 1;
}

.brand-name {
  font-family: var(--vp-font-family-base);
  font-size: 1.7rem;
  font-weight: 700;
  letter-spacing: -0.02em;
  color: var(--qd-ink, #181716);
}

.brand-sub {
  font-family: var(--vp-font-family-base);
  font-style: italic;
  font-size: 0.95rem;
  color: var(--qd-ink-muted, #5e5a54);
  margin-left: 0.5rem;
  border-left: 1px solid var(--qd-border, #ded8cb);
  padding-left: 0.75rem;
}

.masthead-nav {
  display: flex;
  align-items: center;
  gap: 1.25rem;
  font-family: var(--vp-font-family-sans);
  font-size: 0.85rem;
  font-weight: 600;
  letter-spacing: 0.04em;
  text-transform: uppercase;
}

.nav-item {
  color: var(--qd-ink-muted, #5e5a54);
  text-decoration: none;
  transition: color 0.15s ease;
}

.nav-item:hover {
  color: var(--qd-accent, #b33927);
}

.dim-btn {
  border: 1px solid var(--qd-border, #ded8cb);
  background: var(--qd-paper-surface, #ffffff);
  padding: 4px 10px;
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  font-weight: 700;
  letter-spacing: 0.08em;
  cursor: pointer;
  border-radius: 4px;
  color: var(--qd-ink, #181716);
  transition: all 0.2s ease;
}

.dim-btn:hover {
  border-color: var(--qd-accent, #b33927);
  color: var(--qd-accent, #b33927);
}

.forge-masthead-meta {
  display: flex;
  justify-content: space-between;
  margin-top: 0.75rem;
  padding-top: 0.5rem;
  border-top: 1px solid var(--qd-border, #ded8cb);
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  color: var(--qd-ink-faint, #8e8a82);
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
  position: relative;
  z-index: 1;
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
  color: var(--qd-accent, #b33927);
  font-weight: 700;
}

.pretitle-issue {
  color: var(--qd-ink-faint, #8e8a82);
}

.hero-title {
  font-family: var(--vp-font-family-base);
  font-size: 3.8rem;
  line-height: 1.05;
  font-weight: 700;
  margin: 0 0 1.5rem 0;
  letter-spacing: -0.03em;
  color: var(--qd-ink, #181716);
}

.title-italic {
  font-style: italic;
  font-weight: 400;
  color: var(--qd-accent, #b33927);
}

.hero-deck {
  font-family: var(--vp-font-family-base);
  font-size: 1.15rem;
  line-height: 1.6;
  color: var(--qd-ink-muted, #5e5a54);
  margin-bottom: 2rem;
  max-width: 580px;
}

/* Hero Button Hierarchy: 1 Solid Button + Quiet Arrow Links */
.hero-actions-row {
  display: flex;
  align-items: center;
  gap: 1.5rem;
  flex-wrap: wrap;
}

.btn-solid-primary {
  background: var(--qd-ink, #181716);
  color: var(--qd-paper, #f7f3e9);
  padding: 11px 22px;
  font-family: var(--vp-font-family-sans);
  font-size: 0.85rem;
  font-weight: 600;
  border-radius: 4px;
  text-decoration: none;
  transition: opacity 0.2s ease, transform 0.15s ease;
  box-shadow: 0 2px 6px rgba(0,0,0,0.1);
}

.btn-solid-primary:hover {
  opacity: 0.92;
  transform: translateY(-1px);
}

.quiet-links-group {
  display: flex;
  align-items: center;
  gap: 1.25rem;
}

.quiet-arrow-link {
  font-family: var(--vp-font-family-sans);
  font-size: 0.85rem;
  font-weight: 600;
  color: var(--qd-ink-muted, #5e5a54);
  text-decoration: none;
  transition: color 0.15s ease;
}

.quiet-arrow-link:hover {
  color: var(--qd-accent, #b33927);
  text-decoration: underline;
}

/* Schedule Card ("THIS WEEK") */
.schedule-card {
  border: 1px solid var(--qd-border, #ded8cb);
  background: var(--qd-paper-surface, #ffffff);
  border-radius: 8px;
  padding: 1.5rem;
  box-shadow: 0 4px 16px rgba(0, 0, 0, 0.04);
}

.schedule-header {
  display: flex;
  justify-content: space-between;
  align-items: baseline;
  margin-bottom: 0.5rem;
}

.schedule-title {
  font-family: var(--vp-font-family-mono);
  font-size: 0.85rem;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 0.05em;
  color: var(--qd-ink, #181716);
}

.schedule-sub {
  font-family: var(--vp-font-family-base);
  font-style: italic;
  font-size: 0.8rem;
  color: var(--qd-ink-faint, #8e8a82);
  margin-left: 6px;
}

.tracker-pct {
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  font-weight: 600;
  color: var(--qd-accent, #b33927);
}

/* Thin Tracker Bar */
.thin-tracker-bar {
  height: 3px;
  background: var(--qd-paper-alt, #eee9dc);
  border-radius: 2px;
  overflow: hidden;
  margin-bottom: 1rem;
}

.thin-tracker-fill {
  height: 100%;
  background: var(--qd-accent, #b33927);
}

.schedule-list {
  display: flex;
  flex-direction: column;
}

.schedule-row-link {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0.75rem 0.5rem;
  border-bottom: 1px solid var(--qd-border-subtle, #eae4d8);
  text-decoration: none;
  border-radius: 4px;
  transition: background 0.15s ease;
}

.schedule-row-link:last-child {
  border-bottom: none;
}

.schedule-row-link:hover {
  background: var(--qd-paper-alt, #eee9dc);
}

.item-date {
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  color: var(--qd-ink-faint, #8e8a82);
  width: 48px;
}

.item-desc {
  flex: 1;
  font-family: var(--vp-font-family-base);
  font-weight: 600;
  font-size: 0.95rem;
  color: var(--qd-ink, #181716);
  padding: 0 0.5rem;
}

.badge-status {
  font-family: var(--vp-font-family-mono);
  font-size: 0.65rem;
  font-weight: 700;
  padding: 2px 7px;
  border-radius: 3px;
  letter-spacing: 0.05em;
  transition: all 0.15s ease;
}

.badge-status.read {
  background: var(--qd-accent-soft, rgba(179, 57, 39, 0.08));
  color: var(--qd-accent, #b33927);
  border: 1px solid var(--qd-accent-border, rgba(179, 57, 39, 0.25));
}

.schedule-row-link:hover .badge-status.read {
  background: var(--qd-accent, #b33927);
  color: #ffffff;
}

.badge-status.ready {
  border: 1px solid var(--qd-ink-muted, #5e5a54);
  color: var(--qd-ink-muted, #5e5a54);
}

.badge-status.live {
  border: 1px solid #0d9488;
  color: #0d9488;
}

.schedule-footer {
  margin-top: 1rem;
  padding-top: 0.75rem;
  border-top: 1px dashed var(--qd-border, #ded8cb);
  display: flex;
  justify-content: space-between;
  font-family: var(--vp-font-family-mono);
  font-size: 0.7rem;
  color: var(--qd-ink-faint, #8e8a82);
}

.footer-rss {
  cursor: pointer;
  color: var(--qd-ink-muted, #5e5a54);
}

.footer-rss:hover {
  color: var(--qd-accent, #b33927);
}

/* Series Library */
.series-library {
  margin-top: 4rem;
  padding-top: 2.5rem;
  border-top: 1px solid var(--qd-border, #ded8cb);
}

.section-heading h2 {
  font-size: 2rem;
  margin-bottom: 0.5rem;
  color: var(--qd-ink, #181716);
}

.section-heading p {
  color: var(--qd-ink-muted, #5e5a54);
  margin-bottom: 2rem;
}

.series-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(260px, 1fr));
  gap: 1.5rem;
}

.series-card {
  border: 1px solid var(--qd-border, #ded8cb);
  background: var(--qd-paper-surface, #ffffff);
  padding: 1.5rem;
  border-radius: 6px;
  display: flex;
  flex-direction: column;
  transition: transform 0.2s, box-shadow 0.2s;
}

.series-card:hover {
  transform: translateY(-2px);
  box-shadow: 0 6px 18px rgba(0,0,0,0.06);
}

.series-header {
  display: flex;
  justify-content: space-between;
  font-family: var(--vp-font-family-mono);
  font-size: 0.7rem;
  margin-bottom: 0.75rem;
}

.series-vol {
  color: var(--qd-accent, #b33927);
  font-weight: 700;
}

.series-pct {
  color: var(--qd-ink-faint, #8e8a82);
}

.series-card-title {
  font-size: 1.25rem;
  margin: 0 0 0.5rem 0;
  line-height: 1.3;
  color: var(--qd-ink, #181716);
}

.series-card-desc {
  font-size: 0.9rem;
  color: var(--qd-ink-muted, #5e5a54);
  flex: 1;
  margin-bottom: 1rem;
}

.progress-bar-bg {
  height: 4px;
  background: var(--qd-paper-alt, #eee9dc);
  border-radius: 2px;
  overflow: hidden;
  margin-bottom: 1rem;
}

.progress-bar-fill {
  height: 100%;
  background: var(--qd-accent, #b33927);
}

.series-card-footer {
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  color: var(--qd-ink-faint, #8e8a82);
}

.series-link {
  color: var(--qd-ink, #181716);
  text-decoration: none;
  font-weight: 600;
}

.series-link:hover {
  color: var(--qd-accent, #b33927);
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
