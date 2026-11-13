# Disk-Backed B+ Tree Index 🌲

> [!TIP] Canonical Indexing Concept
> Explore the live interactive B+ Tree simulator and node-splitting canvas at [Disk-Backed B+ Tree Indexing](/concepts/indexing).

QuartzDB implements an on-disk, self-balancing **B+ Tree Index** (`include/index/bplus_tree.hpp`, `src/index/bplus_tree.cpp`).

Every node in the tree is persisted directly into a dedicated **4096-byte database page**.

---

## Complete B+ Tree Topology

The tree separates metadata tracking, internal routing nodes, and data-pointing leaf pages:

```mermaid
%%{init: {'theme': 'base', 'themeVariables': {'edgeLabelBackground': '#1e1e2e', 'tertiaryTextColor': '#cdd6f4'}}}%%
flowchart TD
    classDef entry fill:#1e2030,stroke:#89b4fa,stroke-width:2px,color:#cdd6f4;
    classDef guard fill:#2e2619,stroke:#f9e2af,stroke-width:2px,color:#f9e2af;
    classDef success fill:#182c25,stroke:#a6e3a1,stroke-width:2px,color:#a6e3a1;
    classDef error fill:#331c24,stroke:#f38ba8,stroke-width:2px,color:#f38ba8;
    classDef disk fill:#241e38,stroke:#cba6f7,stroke-width:2px,color:#cdd6f4;
    classDef shared fill:#162a35,stroke:#89dceb,stroke-width:2px,color:#cdd6f4;

    Meta["Metadata Page 2<br/>Magic: BPM1 · Root: Page 3"]:::shared
    Meta -.->|Root Pointer| Root

    subgraph Hierarchy ["On-Disk Multi-Level Tree Hierarchy (4096-Byte Pages)"]
        Root["Root Page 3 (Internal)<br/>Separator: [ 100 ]<br/>Fan-out: &le; 128 keys"]:::entry

        IntL["Internal Page 5<br/>Key Range: &lt; 100<br/>Separator: [ 50 ]"]:::disk
        IntR["Internal Page 6<br/>Key Range: &ge; 100<br/>Separator: [ 150 ]"]:::disk

        Root -->|Child 0: key &lt; 100| IntL
        Root -->|Child 1: key &ge; 100| IntR

        L1["Leaf Page 10<br/>Keys: [ 10 .. 49 ]<br/>Payload: RecordID {P, S}"]:::success
        L2["Leaf Page 11<br/>Keys: [ 50 .. 99 ]<br/>Payload: RecordID {P, S}"]:::success
        L3["Leaf Page 12<br/>Keys: [ 100 .. 149 ]<br/>Payload: RecordID {P, S}"]:::success
        L4["Leaf Page 13<br/>Keys: [ 150 .. 200 ]<br/>Payload: RecordID {P, S}"]:::success

        IntL -->|key &lt; 50| L1
        IntL -->|key &ge; 50| L2
        IntR -->|key &lt; 150| L3
        IntR -->|key &ge; 150| L4
    end

    L1 ==>|next_leaf: 11| L2
    L2 ==>|next_leaf: 12| L3
    L3 ==>|next_leaf: 13| L4
    L4 -.->|next_leaf: NO_PAGE| Term["End of Index (0xFF..)"]:::guard
```

- **Horizontal Leaf Chaining (`next_leaf`)**: All leaf pages form a singly-linked list, allowing fast range scans (`BETWEEN X AND Y`) without backtracking to internal nodes.
- **Deep Branching Factor**: With `MAX_KEYS = 128`, a 3-level tree indexes up to **$128^3 \approx 2,097,152$ records** with at most 3 disk page reads!

---

## Interactive B+ Tree Visualizer & Simulator

Type integer keys below (e.g. `10`, `25`, `50`, `120`) to watch the B+ Tree visually insert keys, balance, split at node capacity, promote separator keys to the parent, and chain leaves horizontally:

<BTreeVisualizer />

---

## Leaf Node Spatial Memory Map

Leaf nodes store primary key values paired directly with their physical `RecordID`:

<MemoryMap
  title="B+ Tree Leaf Node (4096-Byte Page)"
  subheader="PageID: 10 · Node Type: LEAF (1) · Fixed 4KB Block"
  :totalBytes="4096"
  :showGrowthSidebar="false"
  footerNote="18-byte leaf entries are packed contiguously in sorted order. Leaves link horizontally via next_leaf."
  :segments="[
    { id: 'hdr', kind: 'header', bytes: 24, label: 'Node Header (24B)', sublabel: 'BPT1 (0x31545042) · version (1) · type: LEAF · key_count · next_leaf PageID' },
    { id: 'entries', kind: 'slot', bytes: 2304, label: 'Leaf Entries Array (128 × 18B = 2,304B)', sublabel: 'Sorted tuples: { int64_t key (8B), PageID page_id (8B), uint16_t slot_index (2B) }' },
    { id: 'free', kind: 'free', bytes: 1768, label: 'Unused Page Capacity (1,768B)', sublabel: 'Available expansion space up to 4096-byte block boundary' }
  ]"
/>

---

## Internal Routing Node Spatial Memory Map

Internal nodes store separator keys and child page pointers to direct tree searches:

<MemoryMap
  title="B+ Tree Internal Node (4096-Byte Page)"
  subheader="PageID: 5 · Node Type: INTERNAL (2) · Fixed 4KB Block"
  :totalBytes="4096"
  :showGrowthSidebar="false"
  footerNote="Internal nodes maintain fan-out with child pointers (PageID) interleaved with separator keys."
  :segments="[
    { id: 'hdr', kind: 'header', bytes: 24, label: 'Node Header (24B)', sublabel: 'BPT1 · version (1) · type: INTERNAL · key_count · next_leaf: NO_PAGE' },
    { id: 'first_child', kind: 'slot', bytes: 8, label: 'First Child Pointer (8B)', sublabel: 'PageID child_0 (leftmost subtree)' },
    { id: 'pairs', kind: 'slot', bytes: 2048, label: 'Separator Pairs (128 × 16B = 2,048B)', sublabel: 'Array of { int64_t separator_key (8B), PageID child_page_id (8B) }' },
    { id: 'free', kind: 'free', bytes: 2016, label: 'Unused Page Capacity (2,016B)', sublabel: 'Available capacity up to 4096 bytes' }
  ]"
/>

---

## Point Lookup Traversal (`find`)

When searching for a primary key (e.g. `WHERE id = 75`), traversal runs in $O(\log N)$:

```mermaid
%%{init: {'theme': 'base', 'themeVariables': {'edgeLabelBackground': '#1e1e2e', 'tertiaryTextColor': '#cdd6f4'}}}%%
flowchart TD
    classDef entry fill:#1e2030,stroke:#89b4fa,stroke-width:2px,color:#cdd6f4;
    classDef guard fill:#2e2619,stroke:#f9e2af,stroke-width:2px,color:#f9e2af;
    classDef success fill:#182c25,stroke:#a6e3a1,stroke-width:2px,color:#a6e3a1;
    classDef error fill:#331c24,stroke:#f38ba8,stroke-width:2px,color:#f38ba8;
    classDef disk fill:#241e38,stroke:#cba6f7,stroke-width:2px,color:#cdd6f4;
    classDef shared fill:#162a35,stroke:#89dceb,stroke-width:2px,color:#cdd6f4;

    Start["1. Call find(key)<br/>O(log N) Point Query"]:::entry --> ReadRoot["2. Read root_page_ from Meta<br/>Pager::read_page(root)"]:::disk
    ReadRoot --> CheckType{"Node Type == LEAF?"}:::guard
    
    CheckType -- "No (Internal Page)" --> SearchInt["3a. Binary Search Keys<br/>Find separator index: keys[i] &le; key"]:::entry
    SearchInt --> FetchChild["4a. Pager::read_page(child_id)<br/>Follow downward pointer"]:::disk
    FetchChild --> CheckType
    
    CheckType -- "Yes (Leaf Page)" --> SearchLeaf["3b. Binary Search Leaf Keys<br/>Find exact primary key match"]:::entry
    SearchLeaf --> Found{"Key exists in Leaf?"}:::guard
    Found -- "Yes (Match Found)" --> ReturnRID["4b. Return RecordID<br/>{page_id, slot_index}"]:::success
    Found -- "No (Key Missing)" --> ReturnNull["4c. Return std::nullopt<br/>Record Not Found"]:::error
```

---

## Node Splitting & Promotion (`InsertResult`)

When inserting a key into a node that exceeds `MAX_KEYS = 128`:

```mermaid
sequenceDiagram
    autonumber
    participant Leaf as Left Leaf (Page A)
    participant Right as New Right Leaf (Page B)
    participant Parent as Parent Internal Node
    participant Pager as qdb::pager

    Note over Leaf: Leaf has 129 keys (Exceeds MAX_KEYS)
    Leaf->>Pager: allocate_page(Page B)
    Pager-->>Right: Returns Page B
    Note over Leaf, Right: Split at middle (index 64)
    Leaf->>Right: Right.next_leaf = Leaf.next_leaf
    Leaf->>Leaf: Leaf.next_leaf = Page B
    Leaf->>Parent: Push Separator Key & Page B
    alt Parent exceeds MAX_KEYS
        Note over Parent: Recursively split Parent
    else Root splits
        Note over Parent: Allocate New Root Page
    end
```

---

## Underflow Rebalancing (`EraseResult`)

When a key is deleted via `BplusTree::erase(key)` and occupancy drops below `MIN_KEYS = 64`:
1. **Borrow from Sibling**: If a neighboring node has more than `MIN_KEYS` entries, borrow one entry and adjust the parent separator key.
2. **Node Merge**: If neighboring siblings are also at `MIN_KEYS`, merge the node into its sibling and free the empty page through `pager::deallocate_page()`.
3. **Root Collapse**: When the root internal node is reduced to 0 keys with a single child, the child becomes the new root, decreasing tree height by 1.
