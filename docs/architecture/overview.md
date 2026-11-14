# Architecture Overview 🏗️

> [!TIP] Canonical Architecture Reference
> For the interactive pipeline diagram and subsystem mapping, see [Architecture & Query Pipeline](/concepts/architecture).

QuartzDB is an embedded relational database engine architected around **strict layering**, **page-oriented disk storage**, and **zero external runtime dependencies**.

Every subsystem in QuartzDB corresponds directly to concrete C++23 classes and namespaces in the codebase.

---

## Complete End-to-End System Architecture

The following diagram maps the entire pipeline from raw SQL query text down to physical 4096-byte blocks on disk:

<ExcalidrawDiagram
  name="overview-system-architecture"
  title="Complete End-to-End System Architecture"
  alt="End-to-end multi-tier architecture of QuartzDB: Client Layer, Query Compiler Engine, Relational Execution Engine, Paged Storage Subsystem, and OS File System"
/>

---

## Dual Query Execution Paths

QuartzDB dynamically selects between an **O(log N) Index Fast Path** and a **Slotted-Page Table Scan** depending on the AST expression structure:

<ExcalidrawDiagram
  name="overview-dual-query-paths"
  title="Dual Query Execution Paths: Index Fast-Path vs Table Scan"
  alt="Sequence diagram comparing Path A (B+ Tree O(log N) point lookup) versus Path B (O(N) slotted-page table scan)"
/>

---

## Subsystem Responsibilities

| Subsystem | Core Files | Responsibility |
| :--- | :--- | :--- |
| **Lexer & Parser** | `sql/lexer.hpp`, `sql/parser.hpp`, `sql/ast.hpp` | Tokenizes SQL text, handles identifiers, literals, escape characters, and constructs strongly typed AST nodes. |
| **Executor** | `execution/executor.hpp` | Validates column types, determines index acceleration paths, evaluates boolean filter trees (`AND`, `OR`, `<`, `=`, `!=`), and executes relational `JOIN`s. |
| **Database & Catalog** | `quartz/database.hpp` | Maintains the in-memory registry of active `Table` instances, loads/persists metadata to **Page 0**, and manages database lifecycle. |
| **Table Manager** | `storage/table.hpp` | Manages page allocation for records, handles record insertion into slotted pages, delegates primary keys to `PrimaryIndex`, and coordinates page compaction. |
| **Primary Index** | `index/primary_index.hpp` | Coordinates with `BplusTree` to map 64-bit integer keys to `RecordID` structs and enforces unique primary key constraints on insert. |
| **B+ Tree Engine** | `index/bplus_tree.hpp` | Maintains a disk-resident, balanced multi-way tree with node splits, merges, leaf chaining, and O(log N) point lookups. |
| **Slotted Storage** | `storage/table.cpp`, `storage/record.cpp` | Encodes variable-length values via `RecordCodec` and manages slot arrays with two-way page growth. |
| **Pager** | `storage/pager.hpp` | Direct file I/O abstraction managing 4096-byte boundaries, dynamic page allocation, and page deallocation free lists. |
