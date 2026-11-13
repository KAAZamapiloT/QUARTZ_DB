# QuartzDB Demonstrations 🚀

This directory contains standalone scripts and example programs demonstrating QuartzDB's SQL engine capabilities and embedded C API.

---

## Directory Overview

| File | Type | Description |
| :--- | :--- | :--- |
| **`quickstart.sql`** | SQL Script | End-to-end SQL script demonstrating DDL, DML, `JOIN`, `ORDER BY`, `LIMIT/OFFSET`, `WHERE` expressions, and aggregate functions (`COUNT`, `SUM`, `AVG`, `MIN`, `MAX`). |
| **`c_api_demo.c` / `.cpp`** | Embedded C Program | Self-contained program demonstrating embedding `libquartz` via the C API (`quartz/quartz.h`), including typed column accessors and streaming callbacks. |
| **`python_demo.py`** | Python Script | Demonstrates querying QuartzDB from Python using standard library `ctypes` FFI. |
| **`run_demo.ps1`** | PowerShell Script | One-click runner script for Windows PowerShell to execute all demos. |
| **`run_demo.sh`** | Shell Script | One-click runner script for Linux / macOS to execute all demos. |

---

## 1. Running the SQL Quickstart (`quickstart.sql`)

### Option A: Interactive CLI Execution
Launch the interactive CLI and paste commands from `quickstart.sql`:

```powershell
# Windows
.\out\build\x64-Debug\quartz.exe my_demo.db

# Linux / macOS
./build/quartz my_demo.db
```

### Option B: Pipe Script Directly
Feed the SQL file into `quartz.exe` for non-interactive batch execution:

```powershell
# Windows PowerShell
Get-Content demo\quickstart.sql | .\out\build\x64-Debug\quartz.exe demo\demo_quickstart.db

# Linux / macOS
./build/quartz demo/demo_quickstart.db < demo/quickstart.sql
```

---

## 2. Running the Embedded C API Demo (`c_api_demo.c`)

The C API demo is built as an executable target `quartz_c_demo` via CMake.

### Build and Run:

```powershell
# Windows Developer PowerShell
cd F:\QUARTZ_DB\out\build\x64-Debug
cmake --build . --target quartz_c_demo
.\quartz_c_demo.exe

# Linux / macOS
cmake -B build -S .
cmake --build build --target quartz_c_demo
./build/quartz_c_demo
```

---

## 3. One-Click Automated Runner

To run all demos in sequence:

```powershell
# Windows PowerShell
.\demo\run_demo.ps1

# Linux / macOS
chmod +x demo/run_demo.sh
./demo/run_demo.sh
```
