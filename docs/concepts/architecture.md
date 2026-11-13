# Engine Architecture & Query Pipeline 🏛️

QuartzDB decouples query compilation, catalog management, and physical disk storage into four explicit architectural layers. Every subsystem corresponds directly to concrete C++23 classes and namespaces in the codebase.

---

## Complete Multi-Layer Architecture Diagram

The system architecture cleanly separates query processing, catalog coordination, and on-disk block serialization:

```mermaid
flowchart TB
    %% Class Definitions for High-Contrast Colorful Theme
    classDef client fill:#0284c7,stroke:#38bdf8,stroke-width:2px,color:#ffffff;
    classDef lexer fill:#d97706,stroke:#fbbf24,stroke-width:2px,color:#ffffff;
    classDef parser fill:#6366f1,stroke:#a5b4fc,stroke-width:2px,color:#ffffff;
    classDef ast fill:#7c3aed,stroke:#c084fc,stroke-width:2px,color:#ffffff;
    classDef executor fill:#059669,stroke:#34d399,stroke-width:2px,color:#ffffff;
    classDef catalog fill:#0d9488,stroke:#2dd4bf,stroke-width:2px,color:#ffffff;
    classDef table fill:#e11d48,stroke:#fb7185,stroke-width:2px,color:#ffffff;
    classDef index fill:#ea580c,stroke:#fb923c,stroke-width:2px,color:#ffffff;
    classDef pager fill:#2563eb,stroke:#60a5fa,stroke-width:2px,color:#ffffff;
    classDef disk fill:#1e1e24,stroke:#e05a47,stroke-width:3px,color:#ffffff;

    subgraph Layer0 ["0. Client Interface Layer"]
        CLI["Interactive CLI REPL<br>(quartz.exe)"]:::client
        CPP["C++ Native Embedding API<br>(qdb::Database, qdb::Executor)"]:::client
    end

    subgraph Layer1 ["1. Query Compilation Engine (qdb::sql)"]
        SQL["Raw SQL String"]:::client
        LEX["Lexical Scanner<br>(lexer.cpp)"]:::lexer
        TOK["Token Stream<br>(qdb::Token)"]:::lexer
        PARSE["Recursive-Descent Parser<br>(parser.cpp)"]:::parser
        AST["Statement AST Variant<br>(Select, Insert, Update, Delete...)"]:::ast

        SQL --> LEX
        LEX --> TOK
        TOK --> PARSE
        PARSE --> AST
    end

    subgraph Layer2 ["2. Execution & Catalog Engine (qdb::execution & qdb)"]
        EXEC["Query Executor<br>(executor.cpp)"]:::executor
        CATALOG["Database Catalog<br>(Page 0 · database.cpp)"]:::catalog
        TABLE["Table Engine<br>(table.cpp)"]:::table
        INDEX["Primary Key Index<br>(primary_index.cpp)"]:::index

        AST --> EXEC
        EXEC <--> CATALOG
        EXEC --> TABLE
        EXEC --> INDEX
        TABLE <--> INDEX
    end

    subgraph Layer3 ["3. Physical Storage Subsystem (qdb::storage & qdb::index)"]
        SLOT["Slotted-Page Storage<br>(4KB Page Frames · table.cpp)"]:::table
        BPT["Disk-Backed B+ Tree<br>(Nodes on 4KB Pages · bplus_tree.cpp)"]:::index
        PAGER["Block Pager Engine<br>(4096-Byte I/O · pager.cpp)"]:::pager
        DISK[("Database File (*.db)<br>Single-File Persistent Store")]:::disk

        TABLE --> SLOT
        INDEX --> BPT
        SLOT --> PAGER
        BPT --> PAGER
        PAGER <--> DISK
    end

    CLI --> SQL
    CPP --> SQL
```

---

## End-to-End Query Execution Pipeline

The lifecycle of an SQL query through QuartzDB follows an explicit 6-phase pipeline:

```mermaid
flowchart LR
    %% Pipeline Node Styling
    classDef pInput fill:#0284c7,stroke:#38bdf8,stroke-width:2px,color:#ffffff;
    classDef pLex fill:#d97706,stroke:#fbbf24,stroke-width:2px,color:#ffffff;
    classDef pParse fill:#6366f1,stroke:#a5b4fc,stroke-width:2px,color:#ffffff;
    classDef pPlan fill:#059669,stroke:#34d399,stroke-width:2px,color:#ffffff;
    classDef pExec fill:#e11d48,stroke:#fb7185,stroke-width:2px,color:#ffffff;
    classDef pOutput fill:#0d9488,stroke:#2dd4bf,stroke-width:2px,color:#ffffff;

    Q1["1. Raw SQL Query<br><code>SELECT * FROM users...</code>"]:::pInput
    Q2["2. Lexical Scan<br>Keyword, Ident, Literal Tokens"]:::pLex
    Q3["3. AST Construction<br>Expression Trees & Clauses"]:::pParse
    Q4["4. Planning & Routing<br>PK Fast-Path vs Full Scan"]:::pPlan
    Q5["5. Physical Execution<br>INLJ, B+ Tree, Slotted Page"]:::pExec
    Q6["6. QueryResult<br>Tabular Rows & Affected Counts"]:::pOutput

    Q1 --> Q2 --> Q3 --> Q4 --> Q5 --> Q6
```

### Detailed Pipeline Stages

```mermaid
flowchart TD
    classDef stepBox fill:#18181b,stroke:#e05a47,stroke-width:2px,color:#f4f4f5;
    classDef branchFast fill:#064e3b,stroke:#34d399,stroke-width:2px,color:#ecfdf5;
    classDef branchScan fill:#451a03,stroke:#f59e0b,stroke-width:2px,color:#fffbeb;
    classDef aggBox fill:#3b0764,stroke:#c084fc,stroke-width:2px,color:#faf5ff;

    InSQL["SQL Statement Input"]:::stepBox
    LexerPass["qdb::sql::Lexer: Tokenizes keywords, string escapes, numbers, operators"]:::stepBox
    ParserPass["qdb::sql::Parser: Recursive-descent AST tree construction"]:::stepBox
    CheckStmt{"Statement Type?"}:::stepBox

    %% Routing
    CheckStmt -- "SELECT" --> CheckFilter{"WHERE id = :val<br>on Primary Key?"}:::stepBox
    CheckFilter -- "Yes (Fast-Path)" --> BTreeFind["B+ Tree find_by_primary_key()<br>O(log N) root-to-leaf descent"]:::branchFast
    CheckFilter -- "No (Scan)" --> TableScan["Linear Slotted-Page Scan<br>O(N) traversal across linked pages"]:::branchScan

    BTreeFind --> PostFilter["Evaluate remaining AND/OR expression predicates"]:::stepBox
    TableScan --> PostFilter

    PostFilter --> CheckJoin{"Has JOIN?"}:::stepBox
    CheckJoin -- "Yes" --> ExecJoin["Index Nested-Loop Join (O(N log M))<br>or Nested Scan"]:::stepBox
    CheckJoin -- "No" --> CheckAgg{"Has Aggregates?"}:::stepBox
    ExecJoin --> CheckAgg

    CheckAgg -- "Yes (COUNT, SUM, AVG...)" --> RunAgg["evaluate_aggregates()<br>Single-pass accumulator"]:::aggBox
    CheckAgg -- "No (Raw Columns)" --> OrderLimit["Apply ORDER BY (stable sort)<br>and LIMIT / OFFSET windowing"]:::stepBox
    RunAgg --> OrderLimit

    OrderLimit --> ReturnRes["Return qdb::QueryResult<br>Formatted tabular rows"]:::stepBox

    %% Other statements
    CheckStmt -- "INSERT" --> DoInsert["Serialize tuple -> Allocate slot -> Insert B+ Tree key"]:::stepBox
    CheckStmt -- "UPDATE" --> DoUpdate["In-place / re-slot rewrite -> Update B+ Tree index"]:::stepBox
    CheckStmt -- "DELETE" --> DoDelete["Zero slot length (tombstone) -> Erase B+ Tree key"]:::stepBox
    CheckStmt -- "CREATE TABLE" --> DoCreate["Validate schema -> Allocate Page 1 & Meta -> Register in Page 0"]:::stepBox
    CheckStmt -- "DROP TABLE" --> DoDrop["Unregister from Page 0 -> Recycle pages into freelist"]:::stepBox

    InSQL --> LexerPass --> ParserPass --> CheckStmt
```

---

## Architectural Layer Responsibilities

### 1. Lexical Scanner (`include/sql/lexer.hpp`, `src/sql/lexer.cpp`)
- Scans SQL character streams into strongly-typed `qdb::Token` structures.
- Detects keywords (`SELECT`, `INSERT`, `UPDATE`, `DELETE`, `CREATE`, `DROP`, `JOIN`, `LEFT`, `ON`, `WHERE`, `ORDER`, `BY`, `LIMIT`, `OFFSET`, `COUNT`, `SUM`, `AVG`, `MIN`, `MAX`).
- Handles complex string literal escaping (`'O\'Reilly'`, `''`, `\n`, `\t`).
- Emits typed literals for signed 64-bit integers and comparison operators (`=`, `!=`, `<`, `<=`, `>`, `>=`).

### 2. AST Parser (`include/sql/parser.hpp`, `src/sql/parser.cpp`)
- Hand-written recursive-descent grammar compiler generating type-safe `qdb::sql::Statement` variants.
- Parses boolean expression trees with explicit operator precedence (`NOT` > `AND` > `OR`).
- Parses projections, wildcard expansions (`*`, `table.*`), join specifications, aggregate function calls, order directions (`ASC`, `DESC`), and pagination limits.

### 3. Execution Engine (`include/execution/executor.hpp`, `src/execution/executor.cpp`)
- Evaluates statements against the database catalog:
  - **Single Point Lookups**: When filtering on equality against the primary key, routes directly to the $O(\log N)$ B+ Tree fast-path.
  - **Index Nested-Loop Join (INLJ)**: Resolves relational joins against indexed primary keys in $O(N \log M)$ time.
  - **Aggregate Engine**: Evaluates `COUNT`, `SUM`, `AVG`, `MIN`, `MAX` with automatic type validation and `NULL` semantics.
  - **In-Memory Sort & Windowing**: Applies `std::stable_sort` on typed variants and performs zero-copy `LIMIT`/`OFFSET` slicing.

### 4. Slotted-Page Table Manager (`include/storage/table.hpp`, `src/storage/table.cpp`)
- Packs dynamic tuples into fixed 4096-byte blocks using the slotted-page format:
  - Slot directory grows **downward** from offset 24.
  - Variable-length record payloads grow **upward** from offset 4096.
- Addresses records via stable 64-bit `RecordID` (`{PageID, slot_index}`).
- In-memory `compact_page()` eliminates internal fragmentation without invalidating external B+ tree index pointers.

### 5. Disk-Backed B+ Tree Index (`include/index/bplus_tree.hpp`, `src/index/bplus_tree.cpp`)
- Fully persistent balanced tree where every root, internal routing, and leaf node occupies a dedicated 4KB database page.
- Balances automatically upon insertions: splits nodes when `keys > 128`, promotes separator keys to internal parent nodes, and chains leaf pages horizontally via `next_leaf`.
- Handles deletions via underflow detection, borrowing from adjacent siblings, or merging nodes and collapsing roots.

### 6. Block Pager & Catalog (`include/storage/pager.hpp`, `src/storage/pager.cpp`, `src/quartz/database.cpp`)
- Performs direct 4096-byte page reads and writes at byte offset $\text{PageID} \times 4096$.
- Manages dynamic file expansion and tracks recycled blocks via an in-memory `free_pages` list.
- Permanently reserves **Page 0** for the database catalog (`DB_MAGIC = 0x31424451`), enabling instant mounting of table schemas and root pointers across restarts.
