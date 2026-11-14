# C API & Foreign Function Interface (FFI) Guide 🔌

QuartzDB provides an embedded C ABI in `include/quartz/quartz.h` (`libquartz.a` / `quartz.dll`), allowing applications written in pure C99, Python, Rust, Go, Zig, or Java (via JNI) to embed the database engine without compiling or linking against C++ standard library types.

---

> [!NOTE] C API Architecture
> The C API wraps the native C++ engine (`qdb::Database` and `qdb::SQLExecutor`) using opaque pointers (`qdb_database_t*`, `qdb_result_t*`) and C-linkage functions (`extern "C"`). All C++ exceptions are trapped at the API boundary and converted into error status codes.

---

## 1. Header & Linkage

### Header Inclusion

Include the public C header from your application source:

```c
#include <quartz/quartz.h>
```

### Linking with CMake

If building your project with CMake:

```cmake
find_package(quartz REQUIRED) # Or add_subdirectory(quartz_db)

add_executable(my_app main.c)
target_include_directories(my_app PRIVATE ${QUARTZ_INCLUDE_DIRS})
target_link_libraries(my_app PRIVATE quartz)
```

### Linking via GCC or Clang

Because the underlying engine is compiled in C++23, link against the C++ standard library runtime (`-lstdc++` for GCC or `-lc++` for Clang):

```bash
# Static linking with GCC
gcc -std=c99 -O2 -o my_app main.c -I./include -L./build -lquartz -lstdc++ -lm

# Dynamic linking on Linux/macOS
gcc -std=c99 -o my_app main.c -I./include -L./build -lquartz -Wl,-rpath,.
```

### Linking with MSVC (Windows)

```cmd
cl.exe /std:c11 /I include main.c /link /LIBPATH:build quartz.lib
```

---

## 2. Status Codes & Types

### Return Status Codes (`qdb_status_t`)

All execution and lifecycle functions return standard integer status codes:

```c
typedef enum qdb_status {
    QDB_OK           = 0,  /* Operation succeeded */
    QDB_ERROR        = 1,  /* SQL syntax, schema, or runtime error */
    QDB_MISUSE       = 2,  /* Invalid API call sequence */
    QDB_NULL_POINTER = 3   /* Required pointer argument was NULL */
} qdb_status_t;
```

### Cell Data Types (`qdb_type_t`)

Columns in result rows evaluate to one of three storage types:

```c
typedef enum qdb_type {
    QDB_TYPE_NULL = 0,  /* SQL NULL value */
    QDB_TYPE_INT  = 1,  /* Signed 64-bit integer (int64_t) */
    QDB_TYPE_TEXT = 2   /* UTF-8 string */
} qdb_type_t;
```

---

## 3. Safe Database Handle Lifecycle

### Opening a Database (`qdb_open`)

```c
int qdb_open(const char* filepath, qdb_database_t** out_db);
```

Opens an existing QuartzDB database file or initializes a new database with Catalog Page 0 if the file does not exist.

```c
qdb_database_t* db = NULL;
int rc = qdb_open("production.db", &db);
if (rc != QDB_OK) {
    fprintf(stderr, "Failed to open database (code: %d)\n", rc);
    return 1;
}
```

### Closing a Database (`qdb_close`)

```c
void qdb_close(qdb_database_t* db);
```

Flushes dirty pages, closes file descriptors, and frees all memory allocated by the database handle. Passing `NULL` is a safe no-op.

```c
qdb_close(db);
db = NULL; /* Defensive null assignment */
```

### Inspecting Errors (`qdb_errmsg`)

```c
const char* qdb_errmsg(const qdb_database_t* db);
```

Returns the human-readable description of the most recent error on the handle. The returned string is owned by the `qdb_database_t` handle and remains valid until the next operation.

```c
if (rc != QDB_OK) {
    fprintf(stderr, "QuartzDB error: %s\n", qdb_errmsg(db));
}
```

---

## 4. Query Execution: Object API

The Object API allocates a `qdb_result_t` structure containing column headers, row counts, and decoded cells.

### Executing Statements (`qdb_execute`)

```c
int qdb_execute(qdb_database_t* db, const char* sql, qdb_result_t** out_result);
```

Executes a SQL statement. When `out_result` is not `NULL`, an allocated `qdb_result_t` handle is returned and must be freed with `qdb_result_free()`.

```c
qdb_result_t* res = NULL;
int rc = qdb_execute(db, "SELECT id, name FROM users WHERE id = 42;", &res);
```

### Freeing Results (`qdb_result_free`)

```c
void qdb_result_free(qdb_result_t* res);
```

Frees all memory associated with the query result, including row buffers and cached text conversions. Passing `NULL` is a safe no-op.

### Inspecting Metadata & Dimensions

```c
/* Returns 1 if execution succeeded, 0 on parse or constraint failure */
int qdb_result_is_success(const qdb_result_t* res);

/* Returns execution status message */
const char* qdb_result_message(const qdb_result_t* res);

/* Number of rows modified by INSERT, UPDATE, or DELETE */
size_t qdb_result_rows_affected(const qdb_result_t* res);

/* Number of rows returned in SELECT result set */
size_t qdb_result_row_count(const qdb_result_t* res);

/* Number of columns projected */
size_t qdb_result_column_count(const qdb_result_t* res);

/* Name of the column at zero-based index col_idx */
const char* qdb_result_column_name(const qdb_result_t* res, size_t col_idx);
```

### Accessing Cell Values

```c
/* Returns QDB_TYPE_NULL, QDB_TYPE_INT, or QDB_TYPE_TEXT */
qdb_type_t qdb_column_type(const qdb_result_t* res, size_t row_idx, size_t col_idx);

/* Returns 1 if cell is SQL NULL, 0 otherwise */
int qdb_column_is_null(const qdb_result_t* res, size_t row_idx, size_t col_idx);

/* Extract signed 64-bit integer */
int64_t qdb_column_int(const qdb_result_t* res, size_t row_idx, size_t col_idx);

/* Extract null-terminated UTF-8 text string (owned by res) */
const char* qdb_column_text(const qdb_result_t* res, size_t row_idx, size_t col_idx);
```

---

## 5. Callback Execution: SQLite-Compatible API

For simple migrations from SQLite or streaming iteration over large result sets, `qdb_exec()` provides a callback interface:

```c
typedef int (*qdb_callback_t)(
    void* user_data,
    int col_count,
    char** col_values,
    char** col_names
);

int qdb_exec(
    qdb_database_t* db,
    const char* sql,
    qdb_callback_t callback,
    void* user_data,
    char** out_errmsg
);
```

- If `callback` returns non-zero, iteration aborts immediately.
- If `out_errmsg` is non-NULL and an error occurs, an allocated error string is returned. Free it using `qdb_free(err)`.

```c
static int print_row_cb(void* user, int cols, char** vals, char** names) {
    for (int i = 0; i < cols; ++i) {
        printf("%s = %s%s", names[i], vals[i] ? vals[i] : "NULL", i < cols - 1 ? ", " : "\n");
    }
    return 0; /* Continue iteration */
}

char* err = NULL;
int rc = qdb_exec(db, "SELECT * FROM users;", print_row_cb, NULL, &err);
if (rc != QDB_OK) {
    fprintf(stderr, "Query failed: %s\n", err);
    qdb_free(err);
}
```

---

## 6. Complete Pure C99 Example

```c
#include <stdio.h>
#include <stdlib.h>
#include <quartz/quartz.h>

int main(void) {
    printf("QuartzDB Engine Version: %s\n", qdb_version());

    // 1. Open Database
    qdb_database_t* db = NULL;
    if (qdb_open("app_c.db", &db) != QDB_OK) {
        fprintf(stderr, "Failed to open database\n");
        return EXIT_FAILURE;
    }

    // 2. DDL: Create Table
    qdb_result_t* res = NULL;
    const char* ddl = "CREATE TABLE users (id INT PRIMARY KEY, name TEXT NOT NULL, role TEXT);";
    if (qdb_execute(db, ddl, &res) != QDB_OK) {
        fprintf(stderr, "DDL Error: %s\n", qdb_errmsg(db));
    }
    qdb_result_free(res);

    // 3. DML: Insert Records
    qdb_execute(db, "INSERT INTO users VALUES (1, 'Alice Smith', 'ADMIN');", NULL);
    qdb_execute(db, "INSERT INTO users VALUES (2, 'Bob Jones', 'USER');", NULL);

    // 4. Query: Typed Object Inspection
    res = NULL;
    if (qdb_execute(db, "SELECT id, name, role FROM users ORDER BY id ASC;", &res) == QDB_OK) {
        size_t rows = qdb_result_row_count(res);
        size_t cols = qdb_result_column_count(res);

        printf("\nResult Grid (%zu rows, %zu columns):\n", rows, cols);
        for (size_t c = 0; c < cols; ++c) {
            printf("%-16s", qdb_result_column_name(res, c));
        }
        printf("\n------------------------------------------------\n");

        for (size_t r = 0; r < rows; ++r) {
            for (size_t c = 0; c < cols; ++c) {
                if (qdb_column_is_null(res, r, c)) {
                    printf("%-16s", "NULL");
                } else if (qdb_column_type(res, r, c) == QDB_TYPE_INT) {
                    printf("%-16lld", (long long)qdb_column_int(res, r, c));
                } else {
                    printf("%-16s", qdb_column_text(res, r, c));
                }
            }
            printf("\n");
        }
    }
    qdb_result_free(res);

    // 5. Clean Shutdown
    qdb_close(db);
    return EXIT_SUCCESS;
}
```

---

## 7. Python FFI (`ctypes`) Wrapper

Developers can interact with QuartzDB directly from Python scripts without installing C++ compilers or third-party build packages by using Python's standard library `ctypes`:

```python
"""
quartz_db.py — Pure Python ctypes wrapper for QuartzDB C API
Requires libquartz.so (Linux), libquartz.dylib (macOS), or quartz.dll (Windows).
"""
import ctypes
import os
from typing import Any, Dict, List, Optional, Tuple

# Load Engine Shared Library
_LIB_NAME = "quartz.dll" if os.name == "nt" else "libquartz.so"
_lib = ctypes.CDLL(os.path.abspath(_LIB_NAME))

# Status Codes
QDB_OK = 0
QDB_TYPE_NULL = 0
QDB_TYPE_INT = 1
QDB_TYPE_TEXT = 2

# Opaque Pointer Types
class _QdbDatabase(ctypes.Structure): pass
class _QdbResult(ctypes.Structure): pass

QdbDatabasePtr = ctypes.POINTER(_QdbDatabase)
QdbResultPtr = ctypes.POINTER(_QdbResult)

# Bind Function Signatures
_lib.qdb_open.argtypes = [ctypes.c_char_p, ctypes.POINTER(QdbDatabasePtr)]
_lib.qdb_open.restype = ctypes.c_int

_lib.qdb_close.argtypes = [QdbDatabasePtr]
_lib.qdb_close.restype = None

_lib.qdb_errmsg.argtypes = [QdbDatabasePtr]
_lib.qdb_errmsg.restype = ctypes.c_char_p

_lib.qdb_execute.argtypes = [QdbDatabasePtr, ctypes.c_char_p, ctypes.POINTER(QdbResultPtr)]
_lib.qdb_execute.restype = ctypes.c_int

_lib.qdb_result_free.argtypes = [QdbResultPtr]
_lib.qdb_result_free.restype = None

_lib.qdb_result_is_success.argtypes = [QdbResultPtr]
_lib.qdb_result_is_success.restype = ctypes.c_int

_lib.qdb_result_row_count.argtypes = [QdbResultPtr]
_lib.qdb_result_row_count.restype = ctypes.c_size_t

_lib.qdb_result_column_count.argtypes = [QdbResultPtr]
_lib.qdb_result_column_count.restype = ctypes.c_size_t

_lib.qdb_result_column_name.argtypes = [QdbResultPtr, ctypes.c_size_t]
_lib.qdb_result_column_name.restype = ctypes.c_char_p

_lib.qdb_column_type.argtypes = [QdbResultPtr, ctypes.c_size_t, ctypes.c_size_t]
_lib.qdb_column_type.restype = ctypes.c_int

_lib.qdb_column_is_null.argtypes = [QdbResultPtr, ctypes.c_size_t, ctypes.c_size_t]
_lib.qdb_column_is_null.restype = ctypes.c_int

_lib.qdb_column_int.argtypes = [QdbResultPtr, ctypes.c_size_t, ctypes.c_size_t]
_lib.qdb_column_int.restype = ctypes.c_int64

_lib.qdb_column_text.argtypes = [QdbResultPtr, ctypes.c_size_t, ctypes.c_size_t]
_lib.qdb_column_text.restype = ctypes.c_char_p

_lib.qdb_version.argtypes = []
_lib.qdb_version.restype = ctypes.c_char_p


class QuartzDB:
    def __init__(self, filepath: str):
        self._db = QdbDatabasePtr()
        rc = _lib.qdb_open(filepath.encode("utf-8"), ctypes.byref(self._db))
        if rc != QDB_OK:
            raise RuntimeError(f"Failed to open QuartzDB '{filepath}' (code {rc})")

    def execute(self, sql: str) -> List[Dict[str, Any]]:
        """Executes a SQL query and returns a list of dictionaries."""
        res = QdbResultPtr()
        rc = _lib.qdb_execute(self._db, sql.encode("utf-8"), ctypes.byref(res))
        
        try:
            if rc != QDB_OK or not _lib.qdb_result_is_success(res):
                err = _lib.qdb_errmsg(self._db).decode("utf-8")
                raise RuntimeError(f"Query Error: {err}")

            row_count = _lib.qdb_result_row_count(res)
            col_count = _lib.qdb_result_column_count(res)
            col_names = [
                _lib.qdb_result_column_name(res, c).decode("utf-8")
                for c in range(col_count)
            ]

            results = []
            for r in range(row_count):
                row_dict = {}
                for c in range(col_count):
                    col = col_names[c]
                    if _lib.qdb_column_is_null(res, r, c):
                        row_dict[col] = None
                    elif _lib.qdb_column_type(res, r, c) == QDB_TYPE_INT:
                        row_dict[col] = int(_lib.qdb_column_int(res, r, c))
                    else:
                        row_dict[col] = _lib.qdb_column_text(res, r, c).decode("utf-8")
                results.append(row_dict)
            return results
        finally:
            _lib.qdb_result_free(res)

    def close(self):
        if self._db:
            _lib.qdb_close(self._db)
            self._db = None

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()


# Example Usage:
if __name__ == "__main__":
    print(f"QuartzDB C Version: {_lib.qdb_version().decode('utf-8')}")
    
    with QuartzDB("test_py.db") as db:
        db.execute("CREATE TABLE inventory (sku INT PRIMARY KEY, name TEXT, qty INT);")
        db.execute("INSERT INTO inventory VALUES (101, 'Mechanical Keyboard', 45);")
        db.execute("INSERT INTO inventory VALUES (102, 'Optical Mouse', 120);")

        rows = db.execute("SELECT sku, name, qty FROM inventory WHERE qty > 50;")
        for row in rows:
            print(f"SKU {row['sku']}: {row['name']} (Quantity: {row['qty']})")
```

---

## 8. Memory Management & ABI Rules

| Rule | Guarantee |
| :--- | :--- |
| **Handle Lifetime** | `qdb_database_t` handles must be explicitly destroyed with `qdb_close()`. |
| **Result Lifetime** | Pointers returned from `qdb_column_text()` are owned by `qdb_result_t`. They are invalidated when `qdb_result_free()` is called. |
| **Error Strings** | `qdb_errmsg()` returns internal buffer pointers valid until the next statement on that database handle. |
| **`qdb_exec` Errors** | Strings returned through `char** out_errmsg` must be freed using `qdb_free()`. |
| **Concurrency** | Handles are **not thread-safe**. Open one `qdb_database_t` handle per thread, or serialize calls using external mutexes. |
| **Exception Safety** | C++ exceptions never escape `quartz.h` boundaries; all are caught and reported as `QDB_ERROR`. |
