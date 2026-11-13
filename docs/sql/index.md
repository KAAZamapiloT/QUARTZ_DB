# SQL Syntax Reference 📜

QuartzDB implements an intuitive, relational SQL dialect covering DDL (table schema management), DML (queries, insertions, updates, deletions), relational joins, aggregate functions, and result pagination.

---

## SQL Statements (DDL & DML)

| Statement | Category | Syntax Summary | Documentation |
| :--- | :---: | :--- | :--- |
| **`SELECT`** | DML | `SELECT * FROM table [JOIN ...] [WHERE ...] [ORDER BY ...] [LIMIT ...];` | [View `SELECT` Reference](/sql/statements/select) |
| **`INSERT`** | DML | `INSERT INTO table [(cols...)] VALUES (...);` | [View `INSERT` Reference](/sql/statements/insert) |
| **`UPDATE`** | DML | `UPDATE table SET col = val [WHERE ...];` | [View `UPDATE` Reference](/sql/statements/update) |
| **`DELETE`** | DML | `DELETE FROM table [WHERE ...];` | [View `DELETE` Reference](/sql/statements/delete) |
| **`CREATE TABLE`** | DDL | `CREATE TABLE table (col type [PK], ...);` | [View `CREATE TABLE` Reference](/sql/statements/create-table) |
| **`DROP TABLE`** | DDL | `DROP TABLE table;` | [View `DROP TABLE` Reference](/sql/statements/drop-table) |

---

## Query Clauses & Relational Operations

| Operation / Clause | Category | Syntax Summary | Documentation |
| :--- | :---: | :--- | :--- |
| **`JOIN` / `LEFT JOIN`** | Relational | `FROM a [LEFT] JOIN b ON a.k = b.k` | [View Relational `JOIN` Reference](/sql/joins) |
| **Aggregations** | Metrics | `SELECT COUNT(*), SUM(cents), AVG(cents), MIN, MAX...` | [View Aggregations Reference](/sql/aggregations) |
| **`ORDER BY` & `LIMIT`** | Windowing | `ORDER BY col [ASC\|DESC] LIMIT n OFFSET m` | [View Sorting & Pagination Reference](/sql/order-by-limit) |

---

## Data Types & Expressions

- [**Data Types & Wire Encodings**](/sql/data-types): `INT64`, `TEXT`, `NULL` specifications, `RecordCodec` byte tags, and schema constraints.
- [**Operators & Expression Trees**](/sql/operators): Relational comparisons (`=`, `!=`, `<`, `<=`, `>`, `>=`), boolean logic (`AND`, `OR`, `NOT`), and operator precedence.
