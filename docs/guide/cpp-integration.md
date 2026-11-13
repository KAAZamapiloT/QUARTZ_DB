# C++ Embedding API 🧩

QuartzDB can be embedded directly into any C++23 application as a statically linked or header-included engine.

---

## Linking QuartzDB in CMake

Include `QUARTZ_DB` in your project's `CMakeLists.txt`:

```cmake
add_subdirectory(path/to/QUARTZ_DB)

target_link_libraries(my_app PRIVATE quartz_db)
target_include_directories(my_app PRIVATE path/to/QUARTZ_DB/include)
```

---

## Direct C++ API Usage

You can interact with QuartzDB programmatically using the `qdb::Database`, `qdb::Table`, and `qdb::Schema` classes without parsing raw SQL:

```cpp
#include <quartz/database.hpp>
#include <storage/schema.hpp>
#include <storage/record.hpp>
#include <iostream>

int main() {
    // 1. Open or create database file
    qdb::Database db("app_data.db");

    // 2. Define schema with primary key constraint
    qdb::Schema schema;
    schema.add_column("id", qdb::DataType::INT64, false, true); // (name, type, nullable, is_primary)
    schema.add_column("username", qdb::DataType::STRING, false, false);
    schema.add_column("score", qdb::DataType::INT64, true, false);

    // 3. Create or retrieve table
    qdb::Table& table = db.create_table("players", schema);

    // 4. Insert typed records
    qdb::Record r1;
    r1.values = { std::int64_t(1), std::string("Gandalf"), std::int64_t(9900) };
    table.insert_record(r1);

    // 5. Query table records
    for (const auto& record : table.records()) {
        std::cout << "Player ID: " << std::get<std::int64_t>(record.values[0])
                  << ", Name: " << std::get<std::string>(record.values[1]) << "\n";
    }

    return 0;
}
```

---

## Executing SQL via Executor

If you prefer programmatic SQL execution inside C++:

```cpp
#include <execution/executor.hpp>
#include <quartz/database.hpp>

int main() {
    qdb::Database db("inventory.db");
    qdb::Executor executor(db);

    // Run DDL
    executor.execute("CREATE TABLE inventory (sku INT PRIMARY KEY, qty INT);");

    // Run DML
    executor.execute("INSERT INTO inventory VALUES (9001, 45);");

    // Query with filter
    auto result = executor.execute("SELECT * FROM inventory WHERE sku = 9001;");
    if (result.success) {
        for (const auto& row : result.rows) {
            // Process row values
        }
    }

    return 0;
}
```
