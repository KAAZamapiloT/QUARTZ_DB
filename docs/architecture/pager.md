# Page & Pager Engine 📄

The **Pager** (`include/storage/pager.hpp`, `src/storage/pager.cpp`) manages physical file layout, uniform block partitioning, and low-level block I/O operations for QuartzDB.

Every database in QuartzDB persists within a single binary file strictly divided into contiguous, uniform **4096-byte (4KB)** pages.

---

## Physical File Partitioning

The database file consists of sequential 4KB blocks addressed by a 64-bit `PageID` (0, 1, 2, ...):

<ExcalidrawDiagram
  name="pager-physical-partitioning"
  title="Physical File Partitioning (Fixed 4,096-Byte Blocks)"
  alt="Physical 4096-byte database file block partitioning showing Page 0 System Catalog, Table Data Roots, B+ Tree Meta & Nodes, and Freelist Recycling Pages"
/>

Every page read and write calculates the physical file seek offset directly:
```text
File Seek Offset = PageID × 4096
```

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

The pager maintains an in-memory `free_pages` list (`std::vector<PageID>`) to recycle blocks deallocated by `DROP TABLE` or B+ tree underflow merges within a session. **This freelist is not persisted to disk.** When a `.db` file is re-opened, the freelist begins empty; previously freed page slots are not automatically reclaimed until they are overwritten by subsequent allocations or the database is rebuilt via compaction:

> [!WARNING] Session-Scoped Freelist
> The `free_pages` recycling pool exists **only for the lifetime of the current pager instance**. When you close and re-open a `.db` file, the freelist is NOT restored from disk. This means: freed pages remain on disk but are inaccessible until a manual compaction pass or new pages overwrite the same slots. QuartzDB does not currently implement a persistent freelist or write-ahead log.

<ExcalidrawDiagram
  name="pager-allocation-recycling"
  title="Page Allocation & Recycling Workflow (allocate_page & deallocate_page)"
  alt="Dual workflow diagram showing O(1) freelist recycling via free_pages stack versus file extension on disk, and deallocate_page recycling"
/>

---

## Memory Safety & Block Alignment

- **Static Boundary Enforcement**: Compile-time assertion `static_assert(sizeof(Page) == PAGE_SIZE, "Page size is not 4096 bytes");` guarantees strict 4KB alignment.
- **Direct Sector I/O**: Reads and writes align directly with OS virtual memory pages and hardware physical disk sectors.
- **Zero In-Memory Buffering Spills**: Fixed-size `std::array<std::byte, 4096>` memory buffers prevent memory corruption, buffer overflows, and segmentation faults.
