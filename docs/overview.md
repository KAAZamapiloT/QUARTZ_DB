# Database Overview & Purpose 🏛️

QuartzDB is an embedded relational database engine architected in modern C++23. It provides low-level slotted-page storage, persistent B+ tree indexing, and an integrated SQL compilation pipeline without runtime dependencies.

---

## Metadata & Ownership

| Metadata Attribute | Production Value |
| :--- | :--- |
| **Database System** | QuartzDB (Embedded Relational C++23) |
| **Engine Version** | `v1.0.0-RELEASE` (Format Version: `1`) |
| **Primary Owners** | Systems & Database Engineering Core Team |
| **Repository** | `https://github.com/KAAZamapiloT/QUARTZ_DB` |
| **Target Workload** | High-throughput embedded analytics, local catalogs, edge storage |
| **Default File Path** | `./quartz.db` (Single binary file per database) |

---

## Environments & Connection Strings

Unlike client-server engines (PostgreSQL, MySQL), QuartzDB operates in-process. Connection strings define target file locations and access modes:

```text
# Local Development File Connection
file:./dev_catalog.db?mode=rw&page_size=4096

# Staging Environment
file:/var/data/quartz/staging_ecommerce.db?mode=rw

# Production Embedded Instance
file:/var/data/quartz/prod_orders.db?mode=rw&compaction=auto
```

### CLI Terminal Connection
```bash
# Connect to specific database
./quartz /var/data/quartz/prod_orders.db

# Or switch databases inside the REPL
quartz> USE prod_orders;
Database 'prod_orders.db' is now active.
```

---

## Architectural Progression & Prerequisites

The curriculum below visualizes the dependencies required to master and extend QuartzDB:

<PrerequisiteGraph />

---

## Core Storage Evolution: Flat File vs Slotted Pages

Click below to see the architectural leap between naive flat files and QuartzDB's production slotted-page storage:

<CodeDiff 
  title="Storage Implementation Comparison"
  beforeLabel="Naive In-Memory Flat File"
  afterLabel="QuartzDB Slotted-Page Engine"
  explanation="Slotted pages introduce a 24-byte header and a slot array. Records are addressed by RecordID {page_id, slot_index}, allowing records to be relocated or defragmented without invalidating index pointers."
>
<template #before>

```cpp
// Naive flat file: records placed sequentially
struct FlatDatabase {
    std::vector<Record> records;
    
    // Deletions create gaping holes or require O(N) memory copies
    void delete_record(size_t index) {
        records.erase(records.begin() + index); // Invalidation!
    }
};
```

</template>
<template #after>

```cpp
// QuartzDB Production: 4KB Slotted-Page Engine
struct PageHeader {
    uint32_t magic;      // 0x31475051 (QPG1)
    uint16_t slot_count; // Number of slots
    uint16_t free_start; // Grows forward
    uint16_t free_end;   // Grows backward from 4096
    PageID   next_page;  // Singly-linked page chain
};

// Records referenced by stable RecordID
struct RecordID {
    PageID   page_id;
    uint16_t slot_index;
};
```

</template>
</CodeDiff>
