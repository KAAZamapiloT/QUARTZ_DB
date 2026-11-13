# Disk-Backed B+ Tree Indexing 🌲

QuartzDB implements an on-disk, self-balancing **B+ Tree Index** (`include/index/bplus_tree.hpp`). Every node occupies an independent 4096-byte database page.

---

## Interactive B+ Tree Visualizer & Simulator

Type integer keys below (e.g. `10`, `25`, `50`, `120`) to watch the B+ Tree visually insert keys, balance, split at node capacity, promote separator keys to the parent, and chain leaves horizontally:

<BTreeVisualizer />

---

## Topology & Node Fan-Out

```mermaid
flowchart TD
    %% Colorful class definitions for B+ Tree components
    classDef meta fill:#78350f,stroke:#fbbf24,stroke-width:2px,color:#fffbeb;
    classDef rootNode fill:#581c87,stroke:#c084fc,stroke-width:3px,color:#ffffff;
    classDef intNode fill:#1e1b4b,stroke:#818cf8,stroke-width:2px,color:#ffffff;
    classDef leafNode fill:#064e3b,stroke:#34d399,stroke-width:2px,color:#ffffff;

    Meta["<b>Metadata Page 2</b><br>Magic: BPM1 · Root: Page 3"]:::meta
    Meta -.->|Root Pointer| Root

    subgraph Hierarchy ["On-Disk Multi-Level Tree Hierarchy (4096-Byte Pages)"]
        Root["<b>Root Page 3 (Internal)</b><br>Separator: [ 100 ]<br>Fan-out: &le; 128 keys"]:::rootNode

        LeftInt["<b>Internal Page 5</b><br>Key Range: &lt; 100<br>Separator: [ 50 ]"]:::intNode
        RightInt["<b>Internal Page 6</b><br>Key Range: &ge; 100<br>Separator: [ 150 ]"]:::intNode

        Root -->|Child 0: key &lt; 100| LeftInt
        Root -->|Child 1: key &ge; 100| RightInt

        L1["<b>Leaf Page 10</b><br>Keys: [ 10 .. 49 ]<br>Payload: RecordID {P, S}"]:::leafNode
        L2["<b>Leaf Page 11</b><br>Keys: [ 50 .. 99 ]<br>Payload: RecordID {P, S}"]:::leafNode
        L3["<b>Leaf Page 12</b><br>Keys: [ 100 .. 149 ]<br>Payload: RecordID {P, S}"]:::leafNode
        L4["<b>Leaf Page 13</b><br>Keys: [ 150 .. 200 ]<br>Payload: RecordID {P, S}"]:::leafNode

        LeftInt -->|key &lt; 50| L1
        LeftInt -->|key &ge; 50| L2
        RightInt -->|key &lt; 150| L3
        RightInt -->|key &ge; 150| L4
    end

    %% Singly-Linked Horizontal Leaf Chain
    L1 ==>|next_leaf: 11| L2
    L2 ==>|next_leaf: 12| L3
    L3 ==>|next_leaf: 13| L4
    L4 -.->|next_leaf: NO_PAGE| Term["End of Index (0xFF..)"]:::meta
```

- **Fan-Out (`MAX_KEYS = 128`)**: A 3-level tree indexes over 2 million records with only 3 disk page reads.
- **Horizontal Leaf Chaining (`next_leaf`)**: Singly-linked leaf nodes enable sequential range scans without navigating ancestor internal pages.
- **Point Lookups ($O(\log N)$)**: Point queries on primary keys execute in sub-millisecond latencies (~100–350 μs) across 120,000+ records.

---

## Leaf Node Spatial Layout

<MemoryMap
  title="B+ Tree Leaf Node (4096-Byte Page)"
  subheader="PageID: 10 · Node Type: LEAF (1) · Fixed 4KB Block"
  :totalBytes="4096"
  :showGrowthSidebar="false"
  footerNote="Leaf entries are packed contiguously in sorted order. Leaves link horizontally via next_leaf."
  :segments="[
    { id: 'hdr', kind: 'header', bytes: 24, label: 'Node Header (24B)', sublabel: 'BPT1 · version 1 · type: LEAF · key_count · next_leaf' },
    { id: 'entries', kind: 'slot', bytes: 2304, label: 'Leaf Entries (128 × 18B = 2,304B)', sublabel: 'Sorted pairs: { key (8B), RecordID {page_id, slot} (10B) }' },
    { id: 'free', kind: 'free', bytes: 1768, label: 'Unused Capacity (1,768B)', sublabel: 'Available expansion space' }
  ]"
/>
