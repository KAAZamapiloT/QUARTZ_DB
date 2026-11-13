# Getting Started with QuartzDB 🚀

QuartzDB is a lightweight embedded relational database engine written in C++23. It requires zero external dependencies, running in-process with direct 4KB block disk I/O.

---

## 30-Second Quickstart

### 1. Compile from Source

```bash
# Clone the repository
git clone https://github.com/KAAZamapiloT/QUARTZ_DB.git
cd QUARTZ_DB

# Configure and compile in Release mode
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### 2. Launch the Interactive REPL

```bash
# Windows
.\build\src\cli\Release\quartz.exe

# Linux / macOS
./build/src/cli/quartz
```

### 3. Run Your First SQL Queries

```sql
-- Create your first table with an indexed primary key
CREATE TABLE users (id INT PRIMARY KEY, name TEXT NOT NULL, email TEXT);

-- Insert records
INSERT INTO users VALUES (1, 'Alice Smith', 'alice@example.com');
INSERT INTO users VALUES (2, 'Bob Jones', 'bob@example.com');

-- Fast-path O(log N) point lookup via B+ Tree
SELECT * FROM users WHERE id = 1;

-- Inspect active catalog schema
.schema users
```

---

## Compiler Toolchain Requirements

QuartzDB utilizes modern C++23 features (`std::variant`, `std::bit_cast`, `std::copy_n`, concepts):

| Platform | Compiler Toolchain | Minimum Version |
| :--- | :--- | :--- |
| **Windows** | MSVC (Visual Studio 2022) | 19.36+ (v143) |
| **Linux** | GCC or Clang | GCC 13+ / Clang 17+ |
| **macOS** | Apple Clang / Homebrew LLVM | Clang 17+ |
| **Build System** | CMake | 3.25 or newer |

---

## Embedding in C++ Applications

QuartzDB can be linked statically or directly included into your CMake target:

```cmake
# Add QuartzDB to your CMakeLists.txt
add_subdirectory(path/to/QUARTZ_DB)
target_link_libraries(my_app PRIVATE quartz_db)
```

Direct API execution:
```cpp
#include <quartz/database.hpp>
#include <execution/executor.hpp>

int main() {
    qdb::Database db("app.db");
    qdb::Executor executor(db);

    executor.execute("CREATE TABLE accounts (id INT PRIMARY KEY, balance INT);");
    executor.execute("INSERT INTO accounts VALUES (101, 5000);");
    
    auto res = executor.execute("SELECT * FROM accounts WHERE id = 101;");
    // res.rows contains parsed records
    return 0;
}
```
