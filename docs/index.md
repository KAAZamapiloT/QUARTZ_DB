---
layout: page
---

<HeroForge />

<div style="max-width: 1180px; margin: 0 auto; padding: 0 1.5rem 3rem;">
  <div style="border-top: 1px solid var(--vp-c-border); padding-top: 2rem; margin-top: 2rem;">
    <h2 style="font-size: 2rem; margin-bottom: 0.5rem;">Interactive Terminal & SQL Playground</h2>
    <p style="color: var(--vp-c-text-2); margin-bottom: 1.5rem;">
      Test queries live in your browser. QuartzDB's client-side simulator tests expressions, primary key uniqueness constraints, and tabular ASCII rendering.
    </p>
    <div class="custom-block warning" style="margin-bottom: 1rem;">
      <p class="custom-block-title">⚠️ SIMULATED ENVIRONMENT</p>
      <p>Queries execute in a browser-side JavaScript simulator, <strong>not</strong> the actual C++ QuartzDB engine. Results match the engine's behavior for supported statements, but edge cases and error messages may differ from the compiled binary.</p>
    </div>
    <SqlPlayground />
  </div>
</div>
