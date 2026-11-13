# CREATE TABLE 🏗️

The `CREATE TABLE` statement defines a new relational table, registers its schema into Catalog Page 0, initializes the first 4KB slotted data page, and allocates a new disk B+ Tree for primary key index lookups.

---

## Syntax Diagram

<SyntaxRailroad title="CREATE TABLE Railroad Grammar" viewBox="0 0 680 80">
  <path d="M 20 40 L 50 40" class="track" />

  <!-- CREATE TABLE -->
  <rect x="50" y="24" width="135" height="32" rx="4" class="token-box keyword" />
  <text x="117" y="40" class="token-text keyword">CREATE TABLE</text>
  <path d="M 185 40 L 210 40" class="track" />

  <!-- Table Name -->
  <rect x="210" y="24" width="85" height="32" rx="4" class="token-box" />
  <text x="252" y="40" class="token-text">table</text>
  <path d="M 295 40 L 320 40" class="track" />

  <!-- Column Definitions (...) -->
  <rect x="320" y="24" width="280" height="32" rx="4" class="token-box" />
  <text x="460" y="40" class="token-text">( col_1 type [constraints], ... )</text>
  <path d="M 600 40 L 620 40" class="track" />

  <!-- Semicolon -->
  <rect x="620" y="25" width="25" height="30" rx="4" class="token-box keyword" />
  <text x="632" y="40" class="token-text keyword">;</text>
  <path d="M 645 40 L 670 40" class="track" />
</SyntaxRailroad>

---

## Column Constraints

| Constraint | Syntax | Description |
| :--- | :--- | :--- |
| **`PRIMARY KEY`** | `col_name INT PRIMARY KEY` | Designates the 64-bit integer index key. Exactly one column per table must declare this. |
| **`NOT NULL`** | `col_name TEXT NOT NULL` | Rejects `NULL` insertions during execution. |

---

## How CREATE TABLE Works Under the Hood in QuartzDB

Table creation coordinates schema validation, 4KB page allocation, B+ tree root construction, and Catalog Page 0 persistence:

```mermaid
%%{init: {'theme': 'base', 'themeVariables': {'edgeLabelBackground': '#1e1e2e', 'tertiaryTextColor': '#cdd6f4'}}}%%
flowchart TD
    classDef entry fill:#1e2030,stroke:#89b4fa,stroke-width:2px,color:#cdd6f4;
    classDef guard fill:#2e2619,stroke:#f9e2af,stroke-width:2px,color:#f9e2af;
    classDef success fill:#182c25,stroke:#a6e3a1,stroke-width:2px,color:#a6e3a1;
    classDef error fill:#331c24,stroke:#f38ba8,stroke-width:2px,color:#f38ba8;
    classDef disk fill:#241e38,stroke:#cba6f7,stroke-width:2px,color:#cdd6f4;
    classDef shared fill:#162a35,stroke:#89dceb,stroke-width:2px,color:#cdd6f4;

    Start["1. Client Issues CREATE TABLE<br/>CREATE TABLE users (id INT PRIMARY KEY, name TEXT NOT NULL);"]:::entry
    ValidateSchema["2. Schema Validation:<br/>&bull; Exactly 1 PRIMARY KEY (must be INT)<br/>&bull; Column names non-empty &amp; unique<br/>&bull; Valid types: INT, TEXT"]:::entry

    CheckCatalog{"3. Table Name already<br/>exists in Page 0?"}:::guard
    CheckCatalog -- "Yes" --> AbortDup["Abort: Table already exists"]:::error

    CheckCatalog -- "No" --> AllocDataPage["4. Allocate First Slotted Page (Page 1)<br/>&bull; Header initialized: Magic = 0x31475051 (QPG1)<br/>&bull; free_start = 24, free_end = 4096, slot_count = 0"]:::disk

    AllocDataPage --> AllocMetaPage["5. Allocate Index Metadata (Page 2)<br/>&bull; Header initialized: Magic = 0x314D5042 (BPM1)"]:::shared

    AllocMetaPage --> AllocBptRoot["6. Allocate B+ Tree Root Node (Page 3)<br/>&bull; Header initialized: Magic = 0x31545042 (BPT1)<br/>&bull; Node Type: LEAF (1), key_count = 0, next_leaf = NO_PAGE"]:::shared

    AllocBptRoot --> WriteCatalog["7. Serialize into Catalog Page 0<br/>&bull; Write table_name, first_page (1), index_meta_page (2)<br/>&bull; Serialize schema columns &amp; increment table_count<br/>&bull; pager::write_page(0, catalog_buffer)"]:::disk

    WriteCatalog --> SuccessDone["8. Table Ready for Queries!"]:::success
```

### Allocation Flow Sequence

<div class="diagram-full">

```mermaid
sequenceDiagram
    autonumber
    participant SQL as DDL Query Executor
    participant Cat as Catalog (Page 0)
    participant Pager as Pager (4KB Engine)
    participant Bpt as Disk B+ Tree Index

    SQL->>Pager: allocate_page() -> Page 1
    Pager-->>SQL: Page 1 Initialized (Table Data Root)
    SQL->>Pager: allocate_page() -> Page 2
    Pager-->>SQL: Page 2 Initialized (Index Metadata)
    SQL->>Bpt: Create(Page 2)
    Bpt->>Pager: allocate_page() -> Page 3
    Pager-->>Bpt: Page 3 Initialized (B+ Tree Root Node)
    Bpt-->>SQL: Index Hierarchy Ready
    SQL->>Cat: Serialize Table Descriptor (Name, Schema, Page 1, Page 2)
    Cat->>Pager: write_page(0, catalog_buffer)
    Pager-->>Cat: Flushed Page 0 to Disk
```

</div>

---

## Practical Examples

### 1. Basic Table with Primary Key
```sql
CREATE TABLE users (
    id INT PRIMARY KEY,
    name TEXT NOT NULL,
    email TEXT
);
```

### 2. Multi-Column Relational Table
```sql
CREATE TABLE orders (
    id INT PRIMARY KEY,
    user_id INT NOT NULL,
    total_cents INT NOT NULL,
    status TEXT NOT NULL
);
```
