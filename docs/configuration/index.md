# Engine Configuration Reference ⚙️

QuartzDB provides configurable tuning knobs across storage block management, B+ tree node splitting, memory caching, and query execution.

Use the interactive reference table below to inspect parameter defaults, ranges, and reload behavior (dynamic vs restart-required).

---

<ConfigTable />

---

## Modifying Engine Settings

In QuartzDB, parameters can be configured through three mechanisms:

### 1. Embedded C++ API (`qdb::DatabaseConfig`)
```cpp
#include <quartz/database.hpp>

int main() {
    qdb::DatabaseConfig config;
    config.pager_cache_size_pages = 2048;      // 8MB cache
    config.bplus_tree_max_keys = 128;          // Node fan-out
    config.compaction_threshold = 0.25;        // 25% fragmentation trigger

    qdb::Database db("production.db", config);
    return 0;
}
```

### 2. Runtime SQL Pragmas / Terminal Settings
```sql
-- View current page cache size
SHOW PARAMETER pager.cache_size_pages;

-- Dynamically adjust compaction threshold
SET storage.compaction_threshold = 0.15;
```

### 3. Connection String Parameters
```text
file:./orders.db?cache_size=2048&sync_mode=FULL
```
