# C API Reference 🔌

QuartzDB exposes a stable C API in addition to the C++ embedding interface, enabling integration from any language that supports C FFI — including Python, Go, Rust, and Java via JNI.

---

> [!NOTE] C API vs C++ API
> The C API (`quartz/quartz.h`) provides a handle-based, opaque-pointer interface. The C++ API (`quartz/database.hpp`) offers richer typed access. For new C++ projects, prefer the C++ API. For cross-language bindings or ABI stability, use the C API.

---

## Header & Linkage

### Including the Header

```c
#include "quartz/quartz.h"
```

### Linking the Static Library (CMake)

```cmake
target_link_libraries(my_app PRIVATE quartz_db)
```

### Linking via GCC / Clang

```bash
# Link against the static library directly
gcc -o my_app my_app.c -I./include -L./build -lquartz_db -lstdc++ -lm
```

> [!IMPORTANT] C++ Runtime Required
> Although the API surface is C, the implementation is C++23. When linking from a C project, you must link against `lstdc++` (GCC) or `lc++` (Clang).

---

## Handle Lifecycle

### `qdb_open()` — Open a Database

```c
qdb_handle_t* qdb_open(const char* path);
```

Opens (or creates) a QuartzDB database file at `path`. Returns an opaque handle on success, or `NULL` on failure.

**Parameters:**
- `path` — filesystem path to the `.db` file (e.g. `"./production.db"`)

**Returns:** `qdb_handle_t*` on success; `NULL` if the file cannot be opened or the catalog is corrupt.

**Example:**
```c
qdb_handle_t* db = qdb_open("production.db");
if (!db) {
    fprintf(stderr, "Failed to open database\n");
    return 1;
}
```

---

### `qdb_close()` — Close a Database

```c
void qdb_close(qdb_handle_t* handle);
```

Closes the database, flushes any pending writes, and frees all resources associated with `handle`. After calling `qdb_close()`, the handle must not be used again.

**Parameters:**
- `handle` — a valid handle returned by `qdb_open()`

**Example:**
```c
qdb_close(db);
db = NULL; // defensive null
```

---

## Query Execution

### `qdb_execute()` — Execute a SQL Statement

```c
int qdb_execute(qdb_handle_t* handle, const char* sql,
                qdb_row_callback_t callback, void* user_data);
```

Executes a SQL statement string against the open database. For `SELECT` queries, `callback` is invoked once per result row. For DDL/DML (`CREATE TABLE`, `INSERT`, `UPDATE`, `DELETE`, `DROP TABLE`), `callback` may be `NULL`.

**Parameters:**
- `handle` — open database handle
- `sql` — null-terminated SQL string
- `callback` — called once per row in result set (can be `NULL` for non-SELECT)
- `user_data` — arbitrary pointer passed through to `callback`

**Returns:** `0` on success; non-zero error code on failure (inspect via `qdb_errmsg()`).

**Callback signature:**
```c
typedef void (*qdb_row_callback_t)(qdb_row_t* row, void* user_data);
```

---

## Error Handling

### `qdb_errmsg()` — Get Last Error Message

```c
const char* qdb_errmsg(qdb_handle_t* handle);
```

Returns a human-readable string describing the most recent error on `handle`. The returned string is valid until the next call on the same handle.

**Example:**
```c
int rc = qdb_execute(db, "SELECT * FROM nonexistent;", NULL, NULL);
if (rc != 0) {
    fprintf(stderr, "Error: %s\n", qdb_errmsg(db));
}
```

---

## Typed Result Inspection

The `qdb_row_t` struct provides typed access to individual column values in a result row:

```c
// Column count in this row
int qdb_row_column_count(qdb_row_t* row);

// Column name by index
const char* qdb_row_column_name(qdb_row_t* row, int col);

// Column type (QDB_TYPE_INT, QDB_TYPE_TEXT, QDB_TYPE_NULL)
qdb_type_t qdb_row_column_type(qdb_row_t* row, int col);

// Extract integer value (valid when type == QDB_TYPE_INT)
int64_t qdb_row_get_int(qdb_row_t* row, int col);

// Extract text value (valid when type == QDB_TYPE_TEXT)
// Returned pointer is owned by the row; do not free.
const char* qdb_row_get_text(qdb_row_t* row, int col);
```

**Type enum:**
```c
typedef enum {
    QDB_TYPE_NULL = 0,
    QDB_TYPE_INT  = 1,
    QDB_TYPE_TEXT = 2
} qdb_type_t;
```

---

## Complete C Example

```c
#include <stdio.h>
#include "quartz/quartz.h"

// Callback: prints each result row
static void print_row(qdb_row_t* row, void* user_data) {
    int ncols = qdb_row_column_count(row);
    for (int i = 0; i < ncols; i++) {
        printf("%s=", qdb_row_column_name(row, i));
        switch (qdb_row_column_type(row, i)) {
            case QDB_TYPE_INT:
                printf("%lld", (long long)qdb_row_get_int(row, i));
                break;
            case QDB_TYPE_TEXT:
                printf("\"%s\"", qdb_row_get_text(row, i));
                break;
            case QDB_TYPE_NULL:
                printf("NULL");
                break;
        }
        if (i < ncols - 1) printf(", ");
    }
    printf("\n");
}

int main(void) {
    // 1. Open database (creates production.db if it doesn't exist)
    qdb_handle_t* db = qdb_open("production.db");
    if (!db) {
        fprintf(stderr, "qdb_open failed\n");
        return 1;
    }

    // 2. DDL: Create a table
    if (qdb_execute(db, "CREATE TABLE users (id INT PRIMARY KEY, name TEXT NOT NULL);", NULL, NULL) != 0) {
        fprintf(stderr, "CREATE TABLE failed: %s\n", qdb_errmsg(db));
        qdb_close(db);
        return 1;
    }

    // 3. DML: Insert records
    qdb_execute(db, "INSERT INTO users VALUES (1, 'Alice');", NULL, NULL);
    qdb_execute(db, "INSERT INTO users VALUES (2, 'Bob');",   NULL, NULL);

    // 4. Query: SELECT with callback
    printf("All users:\n");
    if (qdb_execute(db, "SELECT * FROM users;", print_row, NULL) != 0) {
        fprintf(stderr, "SELECT failed: %s\n", qdb_errmsg(db));
    }

    // 5. Close
    qdb_close(db);
    return 0;
}
```

---

## Python ctypes Binding Example

```python
import ctypes
import ctypes.util

# Load the shared library (build as shared lib with -DBUILD_SHARED_LIBS=ON)
lib = ctypes.CDLL("./libquartz_db.so")  # or .dll on Windows

# Define opaque handle type
class QdbHandle(ctypes.Structure):
    pass
QdbHandlePtr = ctypes.POINTER(QdbHandle)

# Bind C functions
lib.qdb_open.argtypes  = [ctypes.c_char_p]
lib.qdb_open.restype   = QdbHandlePtr

lib.qdb_close.argtypes  = [QdbHandlePtr]
lib.qdb_close.restype   = None

lib.qdb_execute.argtypes = [QdbHandlePtr, ctypes.c_char_p, ctypes.c_void_p, ctypes.c_void_p]
lib.qdb_execute.restype  = ctypes.c_int

lib.qdb_errmsg.argtypes  = [QdbHandlePtr]
lib.qdb_errmsg.restype   = ctypes.c_char_p

# Usage
db = lib.qdb_open(b"test.db")
rc = lib.qdb_execute(db, b"CREATE TABLE t (id INT PRIMARY KEY, v TEXT);", None, None)
if rc != 0:
    print("Error:", lib.qdb_errmsg(db).decode())
lib.qdb_execute(db, b"INSERT INTO t VALUES (1, 'hello');", None, None)
lib.qdb_close(db)
```

---

## ABI Compatibility

> [!CAUTION] ABI Stability
> The C API header (`quartz.h`) is currently **not ABI-stable across versions**. The opaque handle layout may change between releases. Always recompile your bindings against the same version of QuartzDB that is linked at runtime.

| Guarantee | Status |
| :--- | :--- |
| Source-level API compatibility (same version) | ✅ Stable |
| Binary ABI stability across versions | ❌ Not guaranteed |
| Thread safety | ❌ Single-threaded only |
| Re-entrant handle usage | ❌ One active query per handle |
