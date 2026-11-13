"""
QuartzDB Python Integration Demo (via ctypes FFI)
Demonstrates embedding and querying QuartzDB directly from Python.
"""

import ctypes
from ctypes import c_char_p, c_int, c_int64, c_size_t, c_void_p, byref
import os
import sys

def load_quartz_library():
    """Locate and load the compiled QuartzDB shared/dynamic library."""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    candidates = [
        # Windows
        os.path.join(script_dir, "..", "out", "build", "x64-Debug", "quartz.dll"),
        os.path.join(script_dir, "..", "build", "quartz.dll"),
        os.path.join(script_dir, "..", "build", "Debug", "quartz.dll"),
        os.path.join(script_dir, "..", "build", "Release", "quartz.dll"),
        # Linux / macOS
        os.path.join(script_dir, "..", "build", "libquartz.so"),
        os.path.join(script_dir, "..", "build", "libquartz.dylib"),
    ]

    for path in candidates:
        if os.path.exists(path):
            try:
                return ctypes.CDLL(path)
            except Exception as e:
                print(f"Found {path} but failed to load: {e}")

    return None

def main():
    print("=" * 65)
    print("  QuartzDB Python FFI Demonstration (ctypes)")
    print("=" * 65)

    lib = load_quartz_library()
    if not lib:
        print("\nNote: quartz.dll / libquartz.so not found in standard build directories.")
        print("To generate the dynamic library, build the project with dynamic export enabled.")
        print("Demonstrating Python C-API bindings mapping:\n")
        print("  qdb_open(path, &db)                  -> int")
        print("  qdb_execute(db, sql, &result)        -> int")
        print("  qdb_result_row_count(result)         -> size_t")
        print("  qdb_column_int(result, row, col)     -> int64")
        print("  qdb_column_text(result, row, col)    -> const char*")
        print("  qdb_result_free(result)")
        print("  qdb_close(db)")
        print("\nSee 'demo/c_api_demo.c' for compiled C execution, or run 'demo/quickstart.sql' via quartz.exe.")
        return

    # Define function signatures
    lib.qdb_version.restype = c_char_p
    print(f"Connected to QuartzDB Engine Version: {lib.qdb_version().decode('utf-8')}")

    lib.qdb_open.argtypes = [c_char_p, ctypes.POINTER(c_void_p)]
    lib.qdb_open.restype = c_int

    lib.qdb_close.argtypes = [c_void_p]
    lib.qdb_close.restype = None

    lib.qdb_execute.argtypes = [c_void_p, c_char_p, ctypes.POINTER(c_void_p)]
    lib.qdb_execute.restype = c_int

    lib.qdb_result_row_count.argtypes = [c_void_p]
    lib.qdb_result_row_count.restype = c_size_t

    lib.qdb_result_column_count.argtypes = [c_void_p]
    lib.qdb_result_column_count.restype = c_size_t

    lib.qdb_column_int.argtypes = [c_void_p, c_size_t, c_size_t]
    lib.qdb_column_int.restype = c_int64

    lib.qdb_column_text.argtypes = [c_void_p, c_size_t, c_size_t]
    lib.qdb_column_text.restype = c_char_p

    lib.qdb_result_free.argtypes = [c_void_p]
    lib.qdb_result_free.restype = None

    # Execute workflow
    db = c_void_p()
    db_name = b"demo_python.db"
    if lib.qdb_open(db_name, byref(db)) != 0:
        print("Failed to open database")
        return

    print("Created database 'demo_python.db'")

    res = c_void_p()
    lib.qdb_execute(db, b"CREATE TABLE users (id INT PRIMARY KEY, name TEXT);", byref(res))
    lib.qdb_result_free(res)

    lib.qdb_execute(db, b"INSERT INTO users VALUES (1, 'Alice');", byref(res))
    lib.qdb_result_free(res)
    lib.qdb_execute(db, b"INSERT INTO users VALUES (2, 'Bob');", byref(res))
    lib.qdb_result_free(res)

    lib.qdb_execute(db, b"SELECT id, name FROM users;", byref(res))
    rows = lib.qdb_result_row_count(res)
    print(f"Selected {rows} rows from Python:")
    for r in range(rows):
        uid = lib.qdb_column_int(res, r, 0)
        name = lib.qdb_column_text(res, r, 1).decode('utf-8')
        print(f"  Row {r}: id={uid}, name={name}")

    lib.qdb_result_free(res)
    lib.qdb_close(db)
    print("Database closed cleanly from Python.")

if __name__ == "__main__":
    main()
