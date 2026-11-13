# Interactive REPL Shell (`quartz.exe`) 💻

QuartzDB includes an interactive production command-line terminal shell (`quartz.exe`) designed for query accumulation, catalog inspection, and database administration.

---

## Launching the Shell

```bash
# Launch with default quartz.db
./quartz

# Open or create a specific database file
./quartz my_database.db
```

---

## Multi-Database Switching Commands

| SQL Command | Action |
| :--- | :--- |
| `CREATE DATABASE <name>;` | Initializes a new `.db` file and switches the active session to it. |
| `DROP DATABASE [IF EXISTS] <name>;` | Deletes a `.db` file from disk (resets to `quartz.db` if currently active). |
| `USE <name>;` | Switches active database context. |
| `SHOW DATABASES;` | Lists all `.db` database files found in the current working directory. |
| `SHOW TABLES;` | Lists all registered tables in the active database catalog. |
| `SELECT DATABASE();` | Prints the file path of the currently connected database. |

---

## Dot Administrative Commands

```text
Dot Commands:
  .open <path>       Open or create a database file (e.g. .open mydb.db)
  .drop <name>       Drop/delete a database file from disk
  .db                Show current database file path and allocation stats
  .tables            List all tables in the current database
  .schema [table]    Show column definitions for a table (or all tables)
  .help              Show the interactive help menu
  .exit / .quit      Exit QuartzDB terminal
```
