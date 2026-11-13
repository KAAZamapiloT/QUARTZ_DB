# What is QuartzDB? 💎

**QuartzDB** is a lightweight, embedded relational database engine written from scratch in modern **C++23**. 

It is designed with two core objectives:
1. **Architectural Clarity & Educational Rigor**: Clean, self-contained implementations of fundamental database concepts: slotted-page file layouts, disk-based B+ tree indexes, and a recursive-descent SQL compiler without external monolithic libraries.
2. **Practical Performance & Safety**: Strict schema enforcement, sub-millisecond point queries, and consistent throughput sustained over 120,000+ records.

---

## High-Level System Architecture

QuartzDB is structured into clear abstraction layers separating query evaluation from physical disk serialization:

```mermaid
graph TB
    subgraph Frontend ["1. Query Processing Layer"]
        SQL["SQL Query String"] --> Lexer["Lexer Scanner<br>(qdb::Lexer)"]
        Lexer --> Parser["Recursive-Descent Parser<br>(qdb::Parser)"]
        Parser --> AST["Abstract Syntax Tree<br>(qdb::Statement)"]
        AST --> Executor["Execution Engine<br>(qdb::Executor)"]
    end
    
    subgraph CatalogLayer ["2. Catalog & Table Manager"]
        Executor --> Database["Database & Catalog<br>(qdb::Database)<br>Persisted on Page 0"]
        Database --> Table["Table Manager<br>(qdb::Table)"]
        Database --> PrimaryIndex["Primary Key Index<br>(qdb::PrimaryIndex)"]
        Table <--> PrimaryIndex
    end

    subgraph StorageSubsystem ["3. Physical Storage & Indexing Engine"]
        PrimaryIndex <--> BPlusTree["Disk-Backed B+ Tree<br>(qdb::BplusTree)"]
        Table <--> Record["Slotted Data Pages<br>(Slot Arrays & Payloads)"]
        BPlusTree <--> Pager["Block Pager (qdb::pager)<br>4096-Byte Block I/O"]
        Record <--> Pager
    end

    Pager <--> Disk[("Single Database File (*.db)<br>Aligned 4KB Page Segments")]
```

---

## Key Highlights

- **Embedded Engine**: Runs directly within your process. No background daemons, socket overhead, or network latency.
- **Single File Storage**: All catalog definitions, table data, and B+ tree index nodes persist in a unified `.db` file.
- **Slotted-Page Architecture**: Fixed 4KB pages manage variable-length string payloads and avoid external fragmentation.
- **Disk-Backed B+ Tree Indexing**: Maintains an on-disk balanced tree that maps 64-bit integer keys to physical `RecordID` tuples (`{PageID, slot_index}`) for $O(\log N)$ point lookups.
- **Native SQL Engine**: Supports `CREATE TABLE`, `DROP TABLE`, `INSERT`, `SELECT` (projections, complex `WHERE` boolean clauses, relational `JOIN`), `UPDATE`, and `DELETE`.
- **Interactive REPL (`quartz.exe`)**: A terminal shell equipped with tabular ASCII output, query accumulation, catalog exploration (`.schema`, `.tables`), and multi-database switching (`USE <db>`, `CREATE DATABASE <db>`).
