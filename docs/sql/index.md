# SQL Reference 📜

QuartzDB implements an intuitive, relational SQL dialect covering DDL (table schema management) and DML (data queries, insertions, updates, deletions, and joins).

---

## SQL Statements

Each statement features detailed syntax diagrams, clause anchors, and execution rules:

| Statement | Category | Syntax Summary | Documentation |
| :--- | :--- | :--- | :--- |
| **`SELECT`** | DML | `SELECT * FROM table [JOIN ...] [WHERE ...];` | [View `SELECT` Reference](/sql/statements/select) |
| **`INSERT`** | DML | `INSERT INTO table VALUES (...);` | [View `INSERT` Reference](/sql/statements/insert) |
| **`UPDATE`** | DML | `UPDATE table SET col = val WHERE ...;` | [View `UPDATE` Reference](/sql/statements/update) |
| **`DELETE`** | DML | `DELETE FROM table WHERE ...;` | [View `DELETE` Reference](/sql/statements/delete) |
| **`CREATE TABLE`** | DDL | `CREATE TABLE table (col type [PK], ...);` | [View `CREATE TABLE` Reference](/sql/statements/create-table) |
| **`DROP TABLE`** | DDL | `DROP TABLE table;` | [View `DROP TABLE` Reference](/sql/statements/drop-table) |

---

## Data Types & Operators

- [**Data Types**](/sql/data-types): `INT64`, `TEXT`, `NULL` specifications and binary serialization.
- [**Operators**](/sql/operators): Binary comparison (`=`, `!=`, `<`, `<=`, `>`, `>=`) and boolean logic (`AND`, `OR`, `NOT`).
