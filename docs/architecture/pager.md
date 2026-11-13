# Page & Pager Engine 📄

The **Pager** (`include/storage/pager.hpp`, `src/storage/pager.cpp`) is the lowest layer of QuartzDB's storage engine. It abstracts raw operating system file I/O into a clean, block-oriented storage interface.

---

## Fixed 4KB Page Boundaries

Every file handled by QuartzDB is partitioned into contiguous, uniform **4096-byte (4KB)** pages:

```
+---------------------------------------------------------------+
|  Page 0 (Catalog & Meta) |  Page 1 (Data)   |  Page 2 (Index) | ...
|        4096 Bytes        |    4096 Bytes    |   4096 Bytes    |
+---------------------------------------------------------------+
0                        4096               8192             12288
```

- **Alignment**: Every page begins at an exact multiple of 4096 bytes ($offset = PageID \times 4096$).
- **Atomic Read/Write**: Operations are performed on full 4KB page buffers, matching modern OS virtual memory page boundaries and file system block sizes.

---

## Dynamic Allocation & Page Recycling

When a table or index requires additional storage:
1. `pager::allocate_page()` is invoked.
2. The pager checks its free list for recycled pages freed by dropped objects or page merges.
3. If no free page exists, it expands the database file by 4KB and assigns a new monotonically increasing `PageID`.

---

## Reserved Page 0: The Catalog

**Page 0** is permanently reserved for database metadata:
- **Magic Bytes & Engine Version**: Validates file integrity and ensures format compatibility.
- **Table Registry**: Serialized list of all registered tables in the database.
- **Root Pointers**:
  - `root_data_page`: The first slotted data page for each table.
  - `root_index_page`: The root node page ID of each table's B+ Tree.
- **Free-List Head**: Offset tracking recycled, unallocated pages.

When `qdb::Database` is instantiated with a `.db` path, Page 0 is immediately read to reconstruct the catalog without scanning data blocks.
