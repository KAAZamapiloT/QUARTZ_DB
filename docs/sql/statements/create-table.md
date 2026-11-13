# CREATE TABLE

The `CREATE TABLE` statement defines a new relational table, registers its schema into Catalog Page 0, initializes the first 4KB slotted data page, and allocates a new disk B+ Tree for the primary key index.

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

## Engine Allocation Flow

```mermaid
sequenceDiagram
    participant SQL as CREATE TABLE query
    participant Cat as Catalog (Page 0)
    participant Pager as qdb::pager
    participant Bpt as qdb::BplusTree

    SQL->>Pager: allocate_page() -> Returns first_page_ (e.g. Page 1)
    SQL->>Pager: allocate_page() -> Returns index_metadata_page (e.g. Page 2)
    SQL->>Bpt: BplusTree::Create() -> Allocates Root Node (Page 3)
    SQL->>Cat: Serialize table name, schema, first_page, and index_meta_page
    Cat->>Pager: write_page(0, catalog_buffer)
```

---

## Examples

```sql
CREATE TABLE users (
    id INT PRIMARY KEY,
    name TEXT NOT NULL,
    email TEXT
);

CREATE TABLE orders (
    id INT PRIMARY KEY,
    user_id INT NOT NULL,
    total_cents INT NOT NULL,
    status TEXT NOT NULL
);
```
