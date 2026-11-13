# Disk-Backed B+ Tree Indexing 🌲

QuartzDB implements an on-disk, self-balancing **B+ Tree Index** (`include/index/bplus_tree.hpp`). Every node occupies an independent 4096-byte database page.

---

## Topology & Node Fan-Out

```mermaid
graph TD
    Meta["Metadata Page (BPM1)<br>Root Page Pointer"] -.-> Root
    
    subgraph TreeNodes ["On-Disk Tree Hierarchy"]
        Root["Root Node (Page 3)<br>Keys: [100] | Type: Internal"]
        Root -->|key < 100| LeftInt["Internal Node (Page 5)"]
        Root -->|key >= 100| RightInt["Internal Node (Page 6)"]

        LeftInt --> L1["Leaf Page 10 [10 .. 49]"]
        LeftInt --> L2["Leaf Page 11 [50 .. 99]"]
        RightInt --> L3["Leaf Page 12 [100 .. 149]"]
        RightInt --> L4["Leaf Page 13 [150 .. 200]"]

        L1 ==>|next_leaf| L2
        L2 ==>|next_leaf| L3
        L3 ==>|next_leaf| L4
    end
```

- **Fan-Out (`MAX_KEYS = 128`)**: A 3-level tree indexes over 2 million records with only 3 disk page reads.
- **Horizontal Leaf Chaining (`next_leaf`)**: Singly-linked leaf nodes enable sequential range scans without navigating ancestor internal pages.
- **Point Lookups ($O(\log N)$)**: Point queries on primary keys execute in sub-millisecond latencies (~100–350 μs) across 120,000+ records.
