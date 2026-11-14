# Data Types & Storage Encodings 🔢

QuartzDB enforces strong, static column typing across table creation, tuple serialization, and query execution. Values are represented internally using `std::variant<std::monostate, std::int64_t, std::string>` (`include/storage/record.hpp`).

---

## Supported Relational Types

| SQL Type Name | Aliases | C++ In-Memory Representation | Wire / Disk Tag | Binary Serialization Format |
| :--- | :--- | :--- | :---: | :--- |
| **`INT`** | `INTEGER`, `INT64`, `BIGINT` | `std::int64_t` (8-byte signed) | `0x01` | `0x01` + 8 bytes (little-endian) |
| **`TEXT`** | `VARCHAR`, `STRING` | `std::string` (arbitrary UTF-8) | `0x02` | `0x02` + 4 bytes length (N) + N raw UTF-8 bytes |
| **`NULL`** | — | `std::monostate` | `0x00` | `0x00` (1 single tag byte) |

---

## Detailed Binary Serialization (`RecordCodec`)

In QuartzDB, records are packed into contiguous byte slices before writing into slotted page payloads:

### 1. Integer Serialization (`INT`)
- **Tag**: `0x01` (1 byte)
- **Payload**: 64-bit signed two's complement integer stored in **little-endian** byte order:
```text
Value: 42
Bytes: 01  2A 00 00 00 00 00 00 00  (9 bytes total)
```

### 2. Variable-Length String Serialization (`TEXT`)
- **Tag**: `0x02` (1 byte)
- **Length**: 32-bit unsigned integer (N) in little-endian format (4 bytes)
- **Bytes**: Raw UTF-8 string bytes:
```text
Value: 'Quartz'
Bytes: 02  06 00 00 00  51 75 61 72 74 7A  (11 bytes total)
```

### 3. Null Value Representation (`NULL`)
- **Tag**: `0x00` (1 byte)
- **Payload**: None. Represents unassigned or missing values for nullable columns.

---

## Column Constraints & Invariants

### 1. `PRIMARY KEY`
- Supported exclusively on single `INT` columns.
- Automatically initializes an on-disk B+ Tree index with `RecordID` addressing.
- Rejects duplicate values at insert time with `QERR_PK_DUPLICATE` (Code 1001).
- Cannot be assigned `NULL`.

### 2. `NOT NULL`
- Supported on both `INT` and `TEXT` columns.
- Rejects `NULL` insertions during tuple validation with `QERR_NOT_NULL_VIOLATION` (Code 1002).

---

## Literals & Escape Sequences

QuartzDB's lexer parses literals using standard ANSI SQL conventions:

| Literal Category | Valid Examples | Description |
| :--- | :--- | :--- |
| **Integer Literals** | `0`, `42`, `-1024`, `9223372036854775807` | Parsed into signed 64-bit integers. |
| **String Literals** | `'Alice'`, `'O\'Reilly'`, `'Hello\nWorld'` | Single-quoted strings. Escaped single quotes (`\'` or `''`) and standard backslash escapes supported. |
| **Null Literals** | `NULL`, `null` | Case-insensitive keyword yielding `std::monostate`. |

---

## Schema DDL Example

```sql
CREATE TABLE employees (
    id INT PRIMARY KEY,
    full_name TEXT NOT NULL,
    department TEXT NOT NULL,
    phone TEXT, -- Nullable column
    salary INT NOT NULL
);
```
