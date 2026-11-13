# Documentation Changelog 📜

All significant revisions and documentation releases for the QuartzDB project are tracked here.

---

## [v1.2.0] — 2026-11-13 (Current Release)

### Added
- **Interactive 4KB Page Hex Inspector (`<HexInspector />`)**: Dual-pane real-time inspector with 4096-byte hex dump and decoded slotted-page memory view with cross-highlighting and live insertion/deletion/compaction simulations.
- **Interactive B+ Tree Index Visualizer (`<BTreeVisualizer />`)**: Live canvas demonstrating integer key insertions, node splits, separator key promotions, leaf chaining, and $O(\log N)$ search paths.
- **Interactive Query Plan & EXPLAIN Tree Visualizer (`<QueryPlanVisualizer />`)**: Interactive execution tree widget comparing $O(\log N)$ B+ Tree fast-path routing vs $O(N)$ linear slotted-page table scans.
- **Physical Memory Map Component (`<MemoryMap />`)**: Spatial memory visualizer replacing control-flow charts for 4KB slotted pages and B+ Tree node blocks with monospace byte rulers and two-way growth arrows.
- **Editorial Publication Portal**: Integrated warm paper aesthetic, serif prose typography, and instant DIM mode toggle.
- **Interactive SQL Playground**: Client-side query simulator with instant execution, tabular ASCII grids, Web Audio synth chime, and gold celebration checkmark.
- **Interactive Relational ERD**: Hover-highlighting entity-relationship diagram with foreign key relationships.
- **Searchable Data Dictionary**: Global filterable index of all columns, types, nullability, and PII attributes with 1-click clipboard copy.
- **Architectural Prerequisite Map**: Interactive SVG node graph showing dependencies from Pager to Slotted Pages to B+ Trees and Query Executor.
- **Animated Code Diff Component**: Interactive side-by-side transition comparing naive flat files vs production slotted-page layouts.
- **Complete Information Architecture**: Added `/overview`, `/tables`, `/tables/orders`, `/tables/users`, `/tables/products`, `/diagrams`, `/data-dictionary`, `/rules`, `/security`, and `/operations`.

---

## [v1.1.0] — 2026-11-10

### Added
- Multi-database lifecycle support (`CREATE DATABASE`, `DROP DATABASE`, `USE`, `SHOW DATABASES`).
- Scale & Limits benchmark report testing 100 to 120,000+ records.
- Enhanced disk B+ tree splitting documentation with recursive parent promotion.

---

## [v1.0.0] — 2026-10-26

### Initial Release
- Core engine C++23 implementation: 4KB block pager, slotted-page storage, B+ Tree primary index, recursive-descent SQL compiler, and interactive REPL CLI.
