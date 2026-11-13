# Interactive REPL CLI 💻

QuartzDB includes an interactive terminal shell (`quartz.exe`) for database creation, query testing, schema inspection, and data visualization.

---

## Launching the Terminal

```bash
# Launch with default database file (quartz.db)
./quartz

# Launch or create a specific database file
./quartz my_database.db
```

---

## Multi-Database Management

Switch and manage database files seamlessly without exiting the shell:

| Command | Description |
| :--- | :--- |
| `CREATE DATABASE <name>;` | Initializes a new `.db` file and switches active connection to it. |
| `USE <name>;` | Switches current active database context. |
| `SHOW DATABASES;` | Lists all `.db` files found in the current working directory. |
| `SHOW TABLES;` | Lists all registered tables in the currently active database. |
| `SELECT DATABASE();` | Prints the file path of the active database. |

---

## Dot Commands

Dot commands provide administrative utilities outside standard SQL:

```text
Dot Commands:
  .open <path>       Open or create a database file (e.g. .open mydb.db)
  .db                Show current database file and location
  .tables            List all tables in the current database
  .schema [table]    Show column definitions for a table (or all tables)
  .help              Show the command help menu
  .exit / .quit      Exit QuartzDB terminal
```

### Schema Inspection Example

```sql
quartz> .schema users
+-------------+---------+----------+-------------+
| Column      | Type    | Nullable | Primary Key |
+-------------+---------+----------+-------------+
| id          | INT64   | NO       | YES         |
| name        | STRING  | NO       | NO          |
| email       | STRING  | YES      | NO          |
+-------------+---------+----------+-------------+
```

---

## Multi-Line Query Accumulation

Queries do not execute until you type a semicolon (`;`). You can write formatted multi-line queries across lines:

```sql
quartz> SELECT u.name, o.product
   ...> FROM users u
   ...> JOIN orders o ON u.id = o.user_id
   ...> WHERE u.id > 10;
+-------+--------------------+
| name  | product            |
+-------+--------------------+
| Carol | 4K Monitor         |
| Dave  | Ergo Chair         |
+-------+--------------------+
2 rows in set (0.000318 sec)
```
