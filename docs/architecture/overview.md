# Architecture Overview 🏗️

QuartzDB is an embedded relational database engine architected around **strict layering**, **page-oriented disk storage**, and **zero external runtime dependencies**.

Every subsystem in QuartzDB corresponds directly to concrete C++23 classes and namespaces in the codebase.

---

## Complete End-to-End System Architecture

The following diagram maps the entire pipeline from raw SQL query text down to physical 4096-byte blocks on disk:

```mermaid
graph TB
    subgraph Client ["Client Interface Layer"]
        CLI["CLI REPL Shell"]
        CPP["C++ Embedding API"]
    end

    subgraph Compiler ["1. Query Compiler Engine"]
        SQL["SQL Query String"]
        LEX["Lexer Scanner"]
        TOK["Token Stream"]
        PARSE["AST Parser"]
        AST["Statement AST Node"]
        
        SQL --> LEX
        LEX --> TOK
        TOK --> PARSE
        PARSE --> AST
    end

    subgraph Runtime ["2. Execution & Catalog"]
        EXEC["Query Executor"]
        CATALOG["Catalog Page 0"]
        TABLE["Table Manager"]
        INDEX["Primary Index"]
        
        AST --> EXEC
        EXEC --> CATALOG
        CATALOG --> TABLE
        CATALOG --> INDEX
        TABLE <--> INDEX
    end

    subgraph Storage ["3. Storage & Index Subsystem"]
        SLOT["Slotted Data Pages"]
        BPT["Disk B+ Tree"]
        PAGER["Block Pager (4KB)"]
        
        TABLE --> SLOT
        INDEX --> BPT
        SLOT --> PAGER
        BPT --> PAGER
    end

    subgraph DiskLayer ["4. File Layer"]
        DISK[("Disk File (*.db)")]
        PAGER <--> DISK
    end

    CLI --> SQL
    CPP --> CATALOG
```

---

## Dual Query Execution Paths

QuartzDB dynamically selects between an **$O(\log N)$ Index Fast Path** and a **Slotted-Page Table Scan** depending on the AST expression structure:

```mermaid
sequenceDiagram
    autonumber
    actor User as Client
    participant Ex as Executor
    participant Tab as Table
    participant Idx as PrimaryIndex
    participant Bpt as B+ Tree
    participant Pager as Pager
    participant Disk as Disk File

    Note over User, Disk: Path A: Point Query on Primary Key (WHERE id = 42)
    User->>Ex: execute("SELECT ... WHERE id = 42;")
    Ex->>Ex: Inspect AST filter
    Ex->>Tab: find_by_primary_key(42)
    Tab->>Idx: find_by_primary_key(42)
    Idx->>Bpt: find(42)
    Bpt->>Pager: read_node(root_page_)
    Pager->>Disk: read 4KB block
    Bpt-->>Idx: RecordID {page, slot}
    Idx-->>Tab: RecordID
    Tab->>Pager: read_page(page_id)
    Pager->>Disk: read 4KB page
    Tab-->>Ex: Record tuple
    Ex-->>User: Tabular result (~100 μs)

    Note over User, Disk: Path B: Filter on Non-Indexed Attribute (WHERE name = 'Bob')
    User->>Ex: execute("SELECT ... WHERE name = 'Bob';")
    Ex->>Tab: scan()
    Tab->>Pager: read_page(first_page_)
    loop For each 4KB data page
        Pager->>Disk: read 4KB block
        Tab->>Tab: Iterate slot array
        Tab->>Tab: RecordCodec::deserialize()
    end
    Ex->>Ex: Evaluate predicate filter
    Ex-->>User: Tabular filtered result
```

---

## Subsystem Responsibilities

| Subsystem | Core Files | Responsibility |
| :--- | :--- | :--- |
| **Lexer & Parser** | `sql/lexer.hpp`, `sql/parser.hpp`, `sql/ast.hpp` | Tokenizes SQL text, handles identifiers, literals, escape characters, and constructs strongly typed AST nodes. |
| **Executor** | `execution/executor.hpp` | Validates column types, determines index acceleration paths, evaluates boolean filter trees (`AND`, `OR`, `<`, `=`, `!=`), and executes relational `JOIN`s. |
| **Database & Catalog** | `quartz/database.hpp` | Maintains the in-memory registry of active `Table` instances, loads/persists metadata to **Page 0**, and manages database lifecycle. |
| **Table Manager** | `storage/table.hpp` | Manages page allocation for records, handles record insertion into slotted pages, delegates primary keys to `PrimaryIndex`, and coordinates page compaction. |
| **Primary Index** | `index/primary_index.hpp` | Coordinates with `BplusTree` to map 64-bit integer keys to `RecordID` structs and enforces unique primary key constraints on insert. |
| **B+ Tree Engine** | `index/bplus_tree.hpp` | Maintains a disk-resident, balanced multi-way tree with node splits, merges, leaf chaining, and $O(\log N)$ point lookups. |
| **Slotted Storage** | `storage/table.cpp`, `storage/record.cpp` | Encodes variable-length values via `RecordCodec` and manages slot arrays with two-way page growth. |
| **Pager** | `storage/pager.hpp` | Direct file I/O abstraction managing 4096-byte boundaries, dynamic page allocation, and page deallocation free lists. |
