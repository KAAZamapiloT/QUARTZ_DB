# Data Types 🔢

QuartzDB provides strong static typing with three foundational relational data types:

| SQL Type Name | Aliases | C++ Representation | Binary Storage Encoding |
| :--- | :--- | :--- | :--- |
| **`INT`** | `INTEGER`, `INT64`, `BIGINT` | `std::int64_t` (8 Bytes signed) | Tag `1` + 8 bytes little-endian |
| **`TEXT`** | `VARCHAR`, `STRING` | `std::string` | Tag `2` + 4 bytes length + raw UTF-8 bytes |
| **`NULL`** | — | `std::monostate` | Tag `0` (1 single byte) |

---

## Constraints

- **`PRIMARY KEY`**: Supported exclusively on `INT` columns. Automatically creates an on-disk B+ Tree index.
- **`NOT NULL`**: Supported on both `INT` and `TEXT` columns. Enforces that `NULL` (`std::monostate`) is rejected at insert time.
