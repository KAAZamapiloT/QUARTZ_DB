---
layout: page
---

<HeroForge />

<div style="max-width: 1180px; margin: 0 auto; padding: 0 1.5rem 3rem;">
  <div style="border-top: 1px solid var(--vp-c-border); padding-top: 2rem; margin-top: 2rem;">
    <h2 style="font-size: 2rem; margin-bottom: 0.5rem;">Interactive Terminal & SQL Playground</h2>
    <p style="color: var(--vp-c-text-2); margin-bottom: 1.5rem;">
      Execute SQL live in your browser. This interactive engine executes 100% client-side in browser RAM with local storage persistence — zero backend server, zero data transmission, running completely on your device.
    </p>
    <div class="custom-block tip" style="margin-bottom: 1rem;">
      <p class="custom-block-title">⚡ CLIENT-SIDE EMBEDDED EXECUTION (ZERO BACKEND)</p>
      <p>Queries execute in-memory inside your browser using client-side RAM and a virtual 4KB slotted-page pager with B+ tree primary key indexing. Because this site runs without a remote backend on static Vercel edge distribution, your queries and schema modifications remain strictly private to your browser.</p>
    </div>
    <SqlPlayground />
  </div>
</div>
