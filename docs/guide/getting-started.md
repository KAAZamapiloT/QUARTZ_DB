# Getting Started & Building 🚀

This guide walks you through compiling QuartzDB from source and launching the interactive CLI terminal.

---

## System Requirements

QuartzDB is written in modern **C++23** and requires a compatible compiler toolchain:

| Platform | Recommended Compiler | Minimum Version |
| :--- | :--- | :--- |
| **Windows** | MSVC (Visual Studio 2022) | 19.36+ (v143) |
| **Linux** | GCC or Clang | GCC 13+ / Clang 17+ |
| **macOS** | Apple Clang / LLVM | Clang 17+ |
| **Build Tool** | CMake | 3.25 or newer |

---

## Cloning the Repository

```bash
git clone https://github.com/KAAZamapiloT/QUARTZ_DB.git
cd QUARTZ_DB
```

---

## Compiling with CMake

### 1. Configure the Build Directory

Generate your build configuration targeting Release or Debug mode:

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
```

### 2. Compile the Binaries

Build the database library, the test suite, and the CLI binary:

```bash
cmake --build build --config Release
```

On completion, the executables will be placed in the build output directory (e.g. `build/bin/Release/quartz.exe` or `build/bin/quartz`).

---

## Running the Unit Test Suite

QuartzDB features a comprehensive test suite covering page allocations, slotted records, B+ tree rebalancing, schema validation, and SQL scale tests:

```bash
ctest --test-dir build --output-on-failure
```

Or invoke the test executable directly:

```bash
# Windows
.\build\tests\Debug\quartz_tests.exe

# Linux / macOS
./build/tests/quartz_tests
```

---

## Launching the Interactive CLI

Run the REPL shell without arguments to work with the default `quartz.db` file, or provide a specific database path:

```bash
# Open default quartz.db
.\build\src\cli\Release\quartz.exe

# Open or create a specific database
.\build\src\cli\Release\quartz.exe mydb.db
```

Try your first query:

```sql
quartz> CREATE TABLE items (id INT PRIMARY KEY, name TEXT NOT NULL, price INT);
Query OK.

quartz> INSERT INTO items VALUES (101, 'Mechanical Keyboard', 120);
Query OK, 1 row affected.

quartz> SELECT * FROM items WHERE id = 101;
+-----+---------------------+-------+
| id  | name                | price |
+-----+---------------------+-------+
| 101 | Mechanical Keyboard | 120   |
+-----+---------------------+-------+
1 row in set.
```
