# What is QuartzDB? 💎

> [!TIP] Architecture & Overview
> For an interactive architectural pipeline walkthrough, see [Architecture & Query Pipeline](/concepts/architecture) and [Database Overview](/overview).

**QuartzDB** is a lightweight, embedded relational database engine written from scratch in modern **C++23**. 

It is designed with two core objectives:
1. **Architectural Clarity & Educational Rigor**: Clean, self-contained implementations of fundamental database concepts: slotted-page file layouts, disk-based B+ tree indexes, and a recursive-descent SQL compiler without external monolithic libraries.
2. **Practical Performance & Safety**: Strict schema enforcement, sub-millisecond point queries, and consistent throughput sustained over 120,000+ records.

---

## High-Level System Architecture

QuartzDB is structured into clear abstraction layers separating query evaluation from physical disk serialization:

```mermaid
%%{init: {'theme': 'base', 'themeVariables': {'edgeLabelBackground': '#1e1e2e', 'tertiaryTextColor': '#cdd6f4'}}}%%
flowchart TB
    classDef entry fill:#1e2030,stroke:#89b4fa,stroke-width:2px,color:#cdd6f4;
    classDef guard fill:#2e2619,stroke:#f9e2af,stroke-width:2px,color:#f9e2af;
    classDef success fill:#182c25,stroke:#a6e3a1,stroke-width:2px,color:#a6e3a1;
    classDef error fill:#331c24,stroke:#f38ba8,stroke-width:2px,color:#f38ba8;
    classDef disk fill:#241e38,stroke:#cba6f7,stroke-width:2px,color:#cdd6f4;
    classDef shared fill:#162a35,stroke:#89dceb,stroke-width:2px,color:#cdd6f4;

    subgraph Frontend ["1. Query Processing Layer"]
        SQL["SQL Query String"]:::entry --> Lexer["Lexer Scanner"]:::entry
        Lexer --> Parser["AST Parser"]:::entry
        Parser --> AST["Statement AST Node"]:::entry
        AST --> Executor["Query Executor"]:::guard
    end
    
    subgraph CatalogLayer ["2. Catalog & Table Manager"]
        Executor --> Database["Database Catalog (Page 0)"]:::shared
        Database --> Table["Table Manager"]:::guard
        Database --> PrimaryIndex["Primary Key Index"]:::guard
        Table <--> PrimaryIndex
    end

    subgraph StorageSubsystem ["3. Physical Storage Engine"]
        PrimaryIndex <--> BPlusTree["Disk B+ Tree Index"]:::shared
        Table <--> Record["Slotted Data Pages"]:::disk
        BPlusTree <--> Pager["Block Pager (4KB)"]:::disk
        Record <--> Pager
    end

    Pager <--> Disk[("Database File (*.db)<br/>Aligned 4KB Blocks")]:::disk
```

---

## Key Highlights

- **Embedded Engine**: Runs directly within your process. No background daemons, socket overhead, or network latency.
- **Single File Storage**: All catalog definitions, table data, and B+ tree index nodes persist in a unified `.db` file.
- **Slotted-Page Architecture**: Fixed 4KB pages manage variable-length string payloads and avoid external fragmentation.
- **Disk-Backed B+ Tree Indexing**: Maintains an on-disk balanced tree that maps 64-bit integer keys to physical `RecordID` tuples (`{PageID, slot_index}`) for $O(\log N)$ point lookups.
- **Native SQL Engine**: Supports `CREATE TABLE`, `DROP TABLE`, `INSERT`, `SELECT` (projections, complex `WHERE` boolean clauses, relational `JOIN`), `UPDATE`, and `DELETE`.
- **Interactive REPL (`quartz.exe`)**: A terminal shell equipped with tabular ASCII output, query accumulation, catalog exploration (`.schema`, `.tables`), and multi-database switching (`USE <db>`, `CREATE DATABASE <db>`).
