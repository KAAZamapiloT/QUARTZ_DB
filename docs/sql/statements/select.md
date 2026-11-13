# SELECT

The `SELECT` statement retrieves records from one or more tables, evaluates column projections, executes relational joins, and applies conditional filtering predicates.

---

## Syntax Diagram

<SyntaxRailroad title="SELECT Statement Railroad Grammar" viewBox="0 0 740 100">
  <!-- Tracks -->
  <path d="M 20 50 L 90 50" class="track" />
  
  <!-- SELECT Token -->
  <a href="#syntax">
    <rect x="90" y="32" width="80" height="36" rx="4" class="token-box keyword" />
    <text x="130" y="50" class="token-text keyword">SELECT</text>
  </a>
  <path d="M 170 50 L 210 50" class="track" />

  <!-- Projections Branch -->
  <path d="M 210 50 L 210 25 L 230 25" class="track" />
  <path d="M 210 50 L 210 75 L 230 75" class="track" />
  
  <!-- * Option -->
  <a href="#projections">
    <rect x="230" y="10" width="35" height="30" rx="4" class="token-box" />
    <text x="247" y="25" class="token-text">*</text>
  </a>
  
  <!-- Column List Option -->
  <a href="#projections">
    <rect x="230" y="60" width="105" height="30" rx="4" class="token-box" />
    <text x="282" y="75" class="token-text">column_list</text>
  </a>

  <path d="M 265 25 L 355 25 L 355 50" class="track" />
  <path d="M 335 75 L 355 75 L 355 50" class="track" />
  <path d="M 355 50 L 375 50" class="track" />

  <!-- FROM Token & Table -->
  <a href="#from-clause">
    <rect x="375" y="32" width="60" height="36" rx="4" class="token-box keyword" />
    <text x="405" y="50" class="token-text keyword">FROM</text>
  </a>
  <path d="M 435 50 L 450 50" class="track" />
  
  <a href="#from-clause">
    <rect x="450" y="32" width="65" height="36" rx="4" class="token-box" />
    <text x="482" y="50" class="token-text">table</text>
  </a>
  <path d="M 515 50 L 535 50" class="track" />

  <!-- Optional JOIN -->
  <path d="M 535 50 Q 550 50 550 20 Q 550 15 570 15" class="track" />
  <a href="#join-clause">
    <rect x="570" y="2" width="70" height="26" rx="4" class="token-box" />
    <text x="605" y="15" class="token-text">JOIN ...</text>
  </a>
  <path d="M 640 15 Q 660 15 660 50" class="track" />

  <!-- Optional WHERE -->
  <path d="M 535 50 L 555 50" class="track" />
  <a href="#where-clause">
    <rect x="555" y="36" width="80" height="28" rx="4" class="token-box" />
    <text x="595" y="50" class="token-text">WHERE ...</text>
  </a>
  <path d="M 635 50 L 675 50" class="track" />

  <!-- Terminal Semicolon -->
  <rect x="675" y="37" width="25" height="26" rx="4" class="token-box keyword" />
  <text x="687" y="50" class="token-text keyword">;</text>
  <path d="M 700 50 L 730 50" class="track" />
</SyntaxRailroad>

---

## Clauses & Grammar

### 1. Projections (`#projections`) {#projections}
Defines the output attributes returned in the result set:
- **Wildcard (`*`)**: Expands to all columns registered in the table catalog.
- **Explicit List**: Comma-separated column identifiers (e.g. `name, email`).

### 2. FROM Clause (`#from-clause`) {#from-clause}
Specifies the target table. The table must exist on Catalog Page 0, otherwise `QERR_TABLE_NOT_FOUND` (1003) is raised.

### 3. JOIN Clause (`#join-clause`) {#join-clause}
Executes an inner join across two tables matching an equality condition:
```sql
SELECT users.name, orders.amount 
FROM users 
JOIN orders ON users.id = orders.user_id;
```

### 4. WHERE Clause (`#where-clause`) {#where-clause}
Filters records using boolean expression predicates. Supports `=`, `!=`, `<`, `<=`, `>`, `>=`, along with `AND`, `OR`, and parentheses.

```sql
SELECT * FROM users 
WHERE id > 100 AND (role = 'admin' OR email != '');
```

---

## Execution Routing: Fast-Path vs Table Scan {#fast-path-index}

When the query planner evaluates `qdb::SelectStatement`, it inspects filter clauses to route primary-key point queries to the $O(\log N)$ B+ Tree fast-path or execute a linear slotted-page scan:

<QueryPlanVisualizer />

---

## Examples

### Point Lookup on Primary Key
```sql
SELECT * FROM users WHERE id = 1;
```

### Column Projection with Filter
```sql
SELECT full_name, email FROM users WHERE id >= 50;
```

### Relational Join with Filter
```sql
SELECT orders.id, users.full_name, orders.total_cents
FROM orders
JOIN users ON orders.user_id = users.id
WHERE orders.status = 'PAID';
```
