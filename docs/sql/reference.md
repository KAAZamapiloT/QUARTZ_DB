# SQL Syntax Reference 📜

> [!TIP] Canonical SQL Reference
> Explore the full statement index and interactive syntax railroad diagrams at [SQL Reference](/sql/).

QuartzDB implements an intuitive, relational SQL subset covering DDL (schema management) and DML (data manipulation).

---

## Data Types

| SQL Type | C++ Representation | Constraints Supported |
| :--- | :--- | :--- |
| `INT` / `INTEGER` | `std::int64_t` (64-bit signed) | `PRIMARY KEY`, `NOT NULL` |
| `TEXT` / `VARCHAR` | `std::string` (variable length) | `NOT NULL` |
| `NULL` | `std::monostate` | Default for unconstrained columns |

---

## Data Definition Language (DDL)

### `CREATE TABLE`

Defines a new persistent table with column names, types, and constraints:

```sql
CREATE TABLE users (
    id INT PRIMARY KEY,
    name TEXT NOT NULL,
    email TEXT
);
```

- Each table must designate exactly one integer `PRIMARY KEY` to initialize the underlying B+ Tree index.
- Columns marked `NOT NULL` reject `NULL` insertions during execution.

### `DROP TABLE`

Drops an existing table, freeing its metadata from Catalog Page 0 and releasing page allocations:

```sql
DROP TABLE users;
```

---

## Data Manipulation Language (DML)

### `INSERT INTO`

Inserts a new tuple into the target table:

```sql
INSERT INTO users VALUES (1, 'Alice Smith', 'alice@example.com');
INSERT INTO users VALUES (2, 'Bob Jones', NULL);
```

If the primary key already exists, the insertion aborts with a primary key uniqueness constraint violation.

### `SELECT`

Queries records with optional projections, conditional filters, and table joins.

#### All Columns
```sql
SELECT * FROM users;
```

#### Column Projection
```sql
SELECT name, email FROM users;
```

#### Point Lookup ($O(\log N)$ Index Path)
```sql
SELECT * FROM users WHERE id = 1;
```

#### Complex Predicates & Boolean Logic
Supports `=`, `!=`, `<`, `<=`, `>`, `>=`, along with `AND`, `OR`, and parentheses:
```sql
SELECT * FROM users 
WHERE id > 100 AND (name = 'Alice' OR email != '');
```

#### Inner Join
```sql
SELECT users.name, orders.amount 
FROM users 
JOIN orders ON users.id = orders.user_id;
```

### `UPDATE`

Modifies column values for records satisfying a filter:

```sql
UPDATE users 
SET email = 'alice.new@example.com' 
WHERE id = 1;
```

### `DELETE`

Removes matching records from the table and primary key index:

```sql
DELETE FROM users WHERE id = 2 OR name = 'Bob Jones';
```
