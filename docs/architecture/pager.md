# Page & Pager Engine 📄

The **Pager** (`include/storage/pager.hpp`, `src/storage/pager.cpp`) manages the physical file layout and low-level block I/O operations for QuartzDB.

Every database in QuartzDB is stored in a single binary file divided into contiguous, uniform **4096-byte (4KB)** pages.

---

## Physical File Partitioning

The database file consists of sequential 4KB blocks addressed by a 64-bit `PageID` ($0, 1, 2, \dots$):

```mermaid
graph LR
    subgraph SingleFile ["Physical File (*.db) - Aligned 4096-Byte Blocks"]
        direction LR
        P0["Page 0<br><b>Database Catalog</b><br>Magic: QDB1<br>Schema & Tables"]
        P1["Page 1<br><b>Table Data Page</b><br>Magic: QPG1<br>Slotted Records"]
        P2["Page 2<br><b>B+ Tree Metadata</b><br>Magic: BPM1<br>Root Pointer"]
        P3["Page 3<br><b>B+ Tree Root Node</b><br>Magic: BPT1<br>Internal/Leaf"]
        P4["Page 4<br><b>Next Data Page</b><br>Linked from Page 1"]
        PN["Page N<br><b>Recycled Page</b><br>On Free List"]
        
        P0 --> P1 --> P2 --> P3 --> P4 --> PN
    end
```

Every page read and write computes the exact file seek offset:
$$\text{File Offset} = \text{PageID} \times 4096$$

---

## Page 0: The Catalog Architecture

Page 0 is permanently reserved for the global database catalog. It tracks all tables, schema definitions, and root pages across database sessions:

```mermaid
classDiagram
    class Page0_Catalog {
        +00..03: uint32 DB_MAGIC = 0x31424451 ("QDB1")
        +04..05: uint16 DB_VERSION = 1
        +06..07: uint16 table_count
        +08..11: uint32 page_size = 4096
        +12..4095: Table_Catalog_Entries[]
    }

    class Table_Catalog_Entry {
        +uint16 name_length
        +uint64 first_page_id
        +bytes table_name (UTF-8)
        +uint16 column_count
        +Column_Spec columns[]
        +uint8 has_primary_index (0 or 1)
        +uint64 index_metadata_page_id (optional)
    }

    class Column_Spec {
        +uint16 column_name_length
        +bytes column_name (UTF-8)
        +uint8 data_type (1: INT64, 2: Text)
        +uint8 is_nullable (0 or 1)
        +uint8 is_primary_key (0 or 1)
    }

    Page0_Catalog *-- Table_Catalog_Entry : contains 0..N
    Table_Catalog_Entry *-- Column_Spec : contains 1..N
```

### Binary Layout Details

```text
+-----------------------------------------------------------------------+
| BYTES 0..3   | DB_MAGIC: 0x31424451 ("QDB1" in Little-Endian)         |
| BYTES 4..5   | DB_VERSION: 1                                          |
| BYTES 6..7   | TABLE COUNT: Number of registered tables in catalog    |
| BYTES 8..11  | PAGE_SIZE: 4096 bytes                                  |
+-----------------------------------------------------------------------+
| BYTES 12..N  | Table Entry 0:                                         |
|              |   - name_length (uint16) + first_page (uint64)         |
|              |   - name bytes                                         |
|              |   - col_count (uint16) + column specs                  |
|              |   - has_index (uint8) + index_meta_page (uint64)       |
+-----------------------------------------------------------------------+
|              | Table Entry 1 ...                                      |
+-----------------------------------------------------------------------+
|              | [ UNALLOCATED FREE SPACE IN PAGE 0 ]                   |
+-----------------------------------------------------------------------+
```

---

## Page Allocation & Recycling Workflow

The pager maintains an in-memory `free_pages` list to recycle blocks freed by `DROP TABLE` or B+ tree rebalancing:

```mermaid
flowchart TD
    Start["Call pager::allocate_page(out_id)"] --> CheckFree{"Is free_pages<br>vector non-empty?"}
    
    CheckFree -- "Yes (Page Recycled)" --> PopFree["Pop PageID from free_pages<br>Reuse existing file block"]
    CheckFree -- "No (File Expansion)" --> CheckBounds["Get current page_count<br>num_pages"]
    
    CheckBounds --> NewID["out_id = num_pages<br>num_pages++"]
    PopFree --> InitZero["Write blank 4096-byte page<br>to out_id offset"]
    NewID --> InitZero
    
    InitZero --> Success["Return true with valid PageID"]

    Dealloc["Call pager::deallocate_page(id)"] --> Validate{"id &gt; 0 and<br>id &lt; num_pages?"}
    Validate -- Yes --> PushFree["Push id into free_pages vector<br>Available for next allocation"]
    Validate -- No --> Fail["Return false"]
```

---

## Memory Safety & Block Alignment

- **Static Size Enforcement**: `static_assert(sizeof(Page) == PAGE_SIZE, "Page size is not 4096 bytes");` guarantees strict boundary alignment at compile time.
- **Zero In-Memory Buffering Spills**: Full 4KB `std::array<std::byte, 4096>` structs prevent buffer overruns and align with standard CPU cache line and OS memory page architectures.
