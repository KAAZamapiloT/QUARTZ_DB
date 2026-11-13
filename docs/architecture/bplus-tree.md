# Disk-Backed B+ Tree Index 🌲

QuartzDB implements an on-disk, self-balancing **B+ Tree Index** (`include/index/bplus_tree.hpp`, `src/index/bplus_tree.cpp`).

Every node in the tree is persisted directly into a dedicated **4096-byte database page**.

---

## Complete B+ Tree Topology

The tree separates metadata tracking, internal routing nodes, and data-pointing leaf pages:

```mermaid
graph TD
    subgraph MetaBlock ["B+ Tree Metadata"]
        META["<b>Metadata Page (Page 2)</b><br>Magic: BPM1 | Version: 1<br>Points to Root Page ID: 3"]
    end

    subgraph InternalNodes ["Routing Layer (Internal Nodes - Type 2)"]
        ROOT["<b>Root Page 3 (Internal)</b><br>Keys: [100]<br>Children: [Page 5, Page 6]"]
        INT1["<b>Page 5 (Internal)</b><br>Keys: [50]<br>Children: [Page 10, Page 11]"]
        INT2["<b>Page 6 (Internal)</b><br>Keys: [150]<br>Children: [Page 12, Page 13]"]
    end

    subgraph LeafNodes ["Storage Layer (Leaf Nodes - Type 1)"]
        L1["<b>Leaf Page 10</b><br>Keys: 10 .. 49<br>Values: RecordID[]"]
        L2["<b>Leaf Page 11</b><br>Keys: 50 .. 99<br>Values: RecordID[]"]
        L3["<b>Leaf Page 12</b><br>Keys: 100 .. 149<br>Values: RecordID[]"]
        L4["<b>Leaf Page 13</b><br>Keys: 150 .. 200<br>Values: RecordID[]"]
    end

    META -.->|Persists Root Pointer| ROOT
    ROOT -->|key &lt; 100| INT1
    ROOT -->|key &gt;= 100| INT2

    INT1 -->|key &lt; 50| L1
    INT1 -->|key &gt;= 50| L2

    INT2 -->|key &lt; 150| L3
    INT2 -->|key &gt;= 150| L4

    L1 ==>|next_leaf: 11| L2
    L2 ==>|next_leaf: 12| L3
    L3 ==>|next_leaf: 13| L4
    L4 ==>|next_leaf: NO_PAGE| END[End of Chain]
```

- **Horizontal Leaf Chaining (`next_leaf`)**: All leaf pages form a singly-linked list, allowing fast range scans (`BETWEEN X AND Y`) without backtracking to internal nodes.
- **Deep Branching Factor**: With `MAX_KEYS = 128`, a 3-level tree indexes up to **$128^3 \approx 2,097,152$ records** with at most 3 disk page reads!

---

## On-Disk Node Binary Layouts

Every B+ tree node is serialized into a single 4096-byte page with a 24-byte header:

```mermaid
classDiagram
    class NodeHeader {
        +00..03: uint32 NODE_MAGIC = 0x31545042 ("BPT1")
        +04..05: uint16 FORMAT_VERSION = 1
        +06: uint8 node_type (1: LEAF, 2: INTERNAL)
        +07: uint8 reserved_padding
        +08..09: uint16 key_count
        +10..15: uint8 reserved[6]
        +16..23: uint64 next_leaf (Leaf: PageID, Internal: NO_PAGE)
    }

    class LeafNodePayload {
        +offset 24..4095: LeafEntry entries[]
    }

    class LeafEntry {
        +00..07: int64 key (8 Bytes)
        +08..15: uint64 page_id (8 Bytes)
        +16..17: uint16 slot_index (2 Bytes)
        Note: Total 18 Bytes per entry
    }

    class InternalNodePayload {
        +00..07: uint64 first_child_page_id (8 Bytes)
        +offset 32..4095: InternalPair pairs[]
    }

    class InternalPair {
        +00..07: int64 separator_key (8 Bytes)
        +08..15: uint64 child_page_id (8 Bytes)
        Note: Total 16 Bytes per pair
    }

    NodeHeader <|-- LeafNodePayload
    NodeHeader <|-- InternalNodePayload
    LeafNodePayload *-- LeafEntry
    InternalNodePayload *-- InternalPair
```

---

## $O(\log N)$ Point Lookup Traversal (`find`)

When searching for a primary key (e.g. `WHERE id = 75`):

```mermaid
flowchart TD
    Start["Call BplusTree::find(key)"] --> ReadRoot["Read root_page_ via pager"]
    
    CheckType{"Is current node a Leaf?"}
    
    CheckType -- "No (Internal Node)" --> BinSearchInt["Binary search (std::upper_bound)<br>Find child_index for key"]
    BinSearchInt --> NextPage["current_page = node.children[child_index]<br>Read child node from pager"]
    NextPage --> CheckType
    
    CheckType -- "Yes (Leaf Node)" --> BinSearchLeaf["Binary search (std::lower_bound)<br>Search for exact key"]
    
    BinSearchLeaf --> Found{"Key exists in leaf?"}
    Found -- Yes --> ReturnRID["Return node.values[index]<br>(RecordID {page_id, slot_index})"]
    Found -- No --> ReturnNull["Return std::nullopt (Not Found)"]
```

---

## Node Splitting & Root Promotion (`InsertResult`)

When inserting a key into a node that exceeds `MAX_KEYS = 128`:

```mermaid
sequenceDiagram
    participant Leaf as Left Leaf Node (Page A)
    participant Right as New Right Leaf (Page B)
    participant Parent as Parent Internal Node
    participant Pager as qdb::pager

    Note over Leaf: Leaf has 129 keys (Exceeds MAX_KEYS: 128)
    Leaf->>Pager: allocate_page(Page B)
    Pager-->>Right: Returns Page B
    Note over Leaf, Right: Split at middle (index 64):<br>Right receives keys 64 .. 128 and values 64 .. 128
    Leaf->>Right: Right.next_leaf = Leaf.next_leaf
    Leaf->>Leaf: Leaf.next_leaf = Page B
    Leaf->>Parent: Return Split { separator: Right.keys.front(), rightPage: Page B }
    Note over Parent: Insert separator key and child pointer into Parent
    alt Parent also exceeds MAX_KEYS
        Note over Parent: Recursively split Parent Internal Node
    else Parent is Root and splits
        Note over Parent: Allocate New Root Page with 2 child pointers<br>Update Metadata Page
    end
```

---

## Underflow Rebalancing & Merging (`EraseResult`)

When a key is deleted via `BplusTree::erase(key)` and occupancy drops below `MIN_KEYS = 64`:
1. **Borrow from Left/Right Sibling**: If a neighboring node has more than `MIN_KEYS` entries, borrow one entry and adjust the parent separator key.
2. **Node Merge**: If neighboring siblings are also at `MIN_KEYS`, merge the node into its sibling and free the empty page through `pager::deallocate_page()`.
3. **Root Collapse**: When the root internal node is reduced to 0 keys with a single child, the child becomes the new root, decreasing tree height by 1.
