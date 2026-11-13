# Page & Pager Engine 📄

The **Pager** (`include/storage/pager.hpp`, `src/storage/pager.cpp`) manages physical file layout, uniform block partitioning, and low-level block I/O operations for QuartzDB.

Every database in QuartzDB persists within a single binary file strictly divided into contiguous, uniform **4096-byte (4KB)** pages.

---

## Physical File Partitioning

The database file consists of sequential 4KB blocks addressed by a 64-bit `PageID` ($0, 1, 2, \dots$):

```mermaid
flowchart LR
    %% Distinctive Page Type Color Definitions
    classDef catPage fill:#78350f,stroke:#f59e0b,stroke-width:2px,color:#ffffff;
    classDef dataPage fill:#064e3b,stroke:#34d399,stroke-width:2px,color:#ffffff;
    classDef metaPage fill:#581c87,stroke:#c084fc,stroke-width:2px,color:#ffffff;
    classDef bptRoot fill:#1e1b4b,stroke:#818cf8,stroke-width:2px,color:#ffffff;
    classDef bptLeaf fill:#831843,stroke:#f472b6,stroke-width:2px,color:#ffffff;
    classDef freePage fill:#27272a,stroke:#71717a,stroke-width:2px,stroke-dasharray: 4 4,color:#a1a1aa;

    subgraph FileStore ["Physical Database File Layout (Fixed 4,096-Byte Blocks)"]
        direction TB

        subgraph BlockRow1 ["Base Allocations (Pages 0 - 2)"]
            direction LR
            P0["<b>Page 0: System Catalog</b><br>Offset: 0x0000 - 0x0FFF (4KB)<br>Magic: 0x31424451 (QDB1)<br>Tables registry &amp; Schema root"]:::catPage
            P1["<b>Page 1: Table Data Root</b><br>Offset: 0x1000 - 0x1FFF (4KB)<br>Magic: 0x31475051 (QPG1)<br>First Slotted-Page of Table A"]:::dataPage
            P2["<b>Page 2: B+ Tree Meta</b><br>Offset: 0x2000 - 0x2FFF (4KB)<br>Magic: 0x314D5042 (BPM1)<br>Index root pointer &amp; fan-out"]:::metaPage
        end

        subgraph BlockRow2 ["Index & Data Expansion (Pages 3 - 5)"]
            direction LR
            P3["<b>Page 3: B+ Tree Root Node</b><br>Offset: 0x3000 - 0x3FFF (4KB)<br>Magic: 0x31545042 (BPT1)<br>Routing keys &amp; child pointers"]:::bptRoot
            P4["<b>Page 4: Next Table Page</b><br>Offset: 0x4000 - 0x4FFF (4KB)<br>Linked via next_page from Page 1<br>Slotted records &amp; compaction"]:::dataPage
            P5["<b>Page 5: B+ Tree Leaf Node</b><br>Offset: 0x5000 - 0x5FFF (4KB)<br>Sorted {key, RecordID} pairs<br>Chained via next_leaf"]:::bptLeaf
        end

        subgraph BlockRow3 ["Freelist Recycling Pool (Pages 6+)"]
            direction LR
            P6["<b>Page 6: Recycled Page</b><br>Offset: 0x6000 - 0x6FFF (4KB)<br>Status: IN FREELIST<br>Ready for instant zero-cost reuse"]:::freePage
            PN["<b>Page N: File Extension</b><br>Offset: N * 4096 Bytes<br>Dynamically appended on demand"]:::dataPage
        end

        P0 --> P1 --> P2
        P2 --> P3 --> P4 --> P5
        P5 -.-> P6 -.-> PN
    end
```

Every page read and write calculates the physical file seek offset directly:
$$\text{File Seek Offset} = \text{PageID} \times 4096$$

---

## Page 0: The Catalog Spatial Map

Page 0 is permanently reserved for the global database catalog. It tracks all registered tables, column schema definitions, and root pages across database sessions:

<MemoryMap
  title="Page 0 Database Catalog (4096-Byte Page)"
  subheader="PageID: 0 · Permanent Metadata Root"
  :totalBytes="4096"
  :showGrowthSidebar="false"
  footerNote="Page 0 mounts on startup. If DB_MAGIC equals 0x31424451 (QDB1), the engine loads table schemas without full scans."
  :segments="[
    { id: 'hdr', kind: 'header', bytes: 12, label: 'Catalog Header (12B)', sublabel: 'QDB1 (0x31424451) · DB_VERSION (1) · table_count · page_size (4096)' },
    { id: 'entries', kind: 'slot', bytes: 512, label: 'Table Catalog Entries (~512B)', sublabel: 'Serialized table descriptors: name, first_page, column schemas, index_metadata_page' },
    { id: 'free', kind: 'free', bytes: 3572, label: 'Unallocated Free Space (3,572B)', sublabel: 'Available for additional table schema registrations' }
  ]"
/>

---

## Page Allocation & Recycling Workflow

The pager maintains an in-memory `free_pages` list (`std::vector<PageID>`) to recycle blocks deallocated by `DROP TABLE` or B+ tree underflow merges, eliminating disk fragmentation and avoiding unnecessary file size inflation:

```mermaid
flowchart TD
    %% Colorful Process Box Styling
    classDef startNode fill:#0284c7,stroke:#38bdf8,stroke-width:2px,color:#ffffff;
    classDef decisionNode fill:#78350f,stroke:#f59e0b,stroke-width:2px,color:#ffffff;
    classDef recyclePath fill:#064e3b,stroke:#34d399,stroke-width:2px,color:#ffffff;
    classDef expandPath fill:#1e1b4b,stroke:#818cf8,stroke-width:2px,color:#ffffff;
    classDef zeroNode fill:#0f766e,stroke:#2dd4bf,stroke-width:2px,color:#ffffff;
    classDef deallocNode fill:#831843,stroke:#f472b6,stroke-width:2px,color:#ffffff;

    subgraph AllocEngine ["Page Allocation Flow: allocate_page()"]
        AllocStart["1. Client Requests Page:<br><code>pager::allocate_page()</code>"]:::startNode
        CheckFree{"2. Is free_pages<br>pool empty?"}:::decisionNode

        PopRecycle["<b>3a. Recycle Pool Hit (O(1))</b><br>Pop PageID from free_pages stack<br>Zero fragmentation overhead"]:::recyclePath
        AppendExpand["<b>3b. File Append Expansion</b><br>PageID = current_file_pages++<br>Seek to file end (PageID * 4096)"]:::expandPath

        ZeroFrame["4. Initialize Clean 4KB Buffer<br><code>std::fill(data.begin(), data.end(), 0)</code>"]:::zeroNode
        FlushNew["5. Write 4KB Frame to Disk<br><code>pager::write_page(id, page)</code>"]:::zeroNode
        ReturnID["6. Return Allocated PageID"]:::startNode

        AllocStart --> CheckFree
        CheckFree -- "No (Recycle)" --> PopRecycle
        CheckFree -- "Yes (Expand)" --> AppendExpand
        PopRecycle --> ZeroFrame
        AppendExpand --> ZeroFrame
        ZeroFrame --> FlushNew --> ReturnID
    end

    subgraph DeallocEngine ["Page Recycling Flow: deallocate_page()"]
        DeallocStart["1. Deallocation Trigger:<br><code>DROP TABLE</code> or <code>B+ Tree Merge</code>"]:::deallocNode
        ValidateID{"2. Validate PageID<br>&lt; total_pages?"}:::decisionNode
        PushFree["3. Append to free_pages Vector<br><code>free_pages.push_back(id)</code>"]:::recyclePath
        RejectFail["4. Ignore / Return False<br>Invalid PageID Guard"]:::deallocNode

        DeallocStart --> ValidateID
        ValidateID -- "Valid" --> PushFree
        ValidateID -- "Invalid" --> RejectFail
    end

    PushFree -.->|Reclaimed Blocks Available| PopRecycle
```

---

## Memory Safety & Block Alignment

- **Static Boundary Enforcement**: Compile-time assertion `static_assert(sizeof(Page) == PAGE_SIZE, "Page size is not 4096 bytes");` guarantees strict 4KB alignment.
- **Direct Sector I/O**: Reads and writes align directly with OS virtual memory pages and hardware physical disk sectors.
- **Zero In-Memory Buffering Spills**: Fixed-size `std::array<std::byte, 4096>` memory buffers prevent memory corruption, buffer overflows, and segmentation faults.
