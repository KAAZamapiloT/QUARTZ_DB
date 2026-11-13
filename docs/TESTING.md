# QuartzDB Test Suite Documentation

QuartzDB maintains a comprehensive, production-grade test suite covering every layer of the database engine: from low-level 4KB page persistence, slotted-page record layout, and on-disk B+ Tree indexing with structural rebalancing, up to recursive-descent SQL parsing, relational joins, and 120,000-row scale benchmarks.

The test suite is built on **GoogleTest (v1.15.2)** and integrates with CMake CTest.

```
Total Test Suites : 13
Total Tests       : 95
Pass Rate         : 100% (95/95 passing)
```

---

## Test Suite Summary

| Suite Name | Source File | Test Count | Target Subsystem | Primary Verification Goal |
| :--- | :--- | :---: | :--- | :--- |
| **`LegacyTests`** | `tests/table_tests.cpp`<br>`tests/persistence_allocation_tests.cpp` | 2 | Table & Slotted Pages | Multi-page record insertion, page linking, and survival across 2+ database restarts. |
| **`DatabaseTest`** | `tests/database_tests.cpp` | 12 | Database Catalog & Page 0 | Table catalog serialization, schema recovery, catalog isolation, and corrupted file rejection. |
| **`SchemaTest`** | `tests/schema_tests.cpp` | 9 | Schema Engine | Column type constraints, nullability, primary key constraints, and schema persistence. |
| **`BPlusTreeTest`** | `tests/bplus_tree_tests.cpp` | 3 | B+ Tree Core | Leaf splitting, internal node splitting, root promotion, and on-disk key lookups. |
| **`PrimaryIndexTest`** | `tests/primary_index_tests.cpp` | 8 | Primary Index | Key-to-RecordID binding, duplicate key rejection, boundary values, and leaf split stability. |
| **`BplusTreeDeletionTest`** | `tests/bplus_tree_deletion_tests.cpp` | 12 | B+ Tree Deletion & Compaction | Underflow handling, sibling borrowing, node merging, root collapse, and page deallocation. |
| **`BplusTreeHighFrequencyTest`** | `tests/bplus_tree_deletion_tests.cpp` | 3 | B+ Tree Stress & Freelist | Sliding-window churn, sawtooth expansion/collapse, and 60,000-op adversarial blitz. |
| **`BplusDBTest`** | `tests/bplus_tree_deletion_tests.cpp` | 2 | Full-Stack Storage Integration | 10,000-operation mixed OLTP churn, crash recovery, and multi-session catalog integrity. |
| **`LexerTest`** | `tests/sql_tests.cpp` | 9 | SQL Lexer | Tokenization of DDL/DML, case insensitivity, string escapes, comments, and operator tokens. |
| **`ParserTest`** | `tests/sql_tests.cpp` | 11 | SQL AST Parser | Recursive-descent AST construction, precedence parsing (AND > OR), JOIN, ORDER BY / LIMIT, and aggregate function parsing. |
| **`SQLExecutorTest`** | `tests/sql_tests.cpp` | 10 | SQL Query Engine | End-to-end CRUD, B+ tree fast path, compound WHERE filters, DROP TABLE, ORDER BY / LIMIT / OFFSET, JOIN, and aggregate functions (COUNT, SUM, AVG, MIN, MAX). |
| **`SQLScaleTest`** | `tests/sql_scale_tests.cpp` | 7 | Limits & Scale Benchmarks | 100 to 120,000 row progressive benchmarks measuring QPS, read latency, and disk footprint. |
| **`CAPITest`** | `tests/c_api_tests.cpp` | 7 | Embedded C API (`libquartz`) | Handle lifecycle, typed result inspection, error propagation, callback iteration, memory safety, and cross-language FFI compatibility. |

---

## Detailed Test Suite Breakdown

### 1. `LegacyTests` (2 Tests)
Validates baseline record storage, slotted page layout, and page allocation persistence.
- **`TableTests`**: Inserts variable-length records across multiple 4KB slotted pages, performs full table scans, closes the pager, re-opens the file, and verifies 100% record integrity.
- **`PersistenceAllocationTests`**: Verifies dynamic page allocation under heavy append workloads and validates that all 240 records survive multiple consecutive close and reopen cycles without record leakage or corrupted slot arrays.

### 2. `DatabaseTest` (12 Tests)
Validates the multi-table database catalog stored on Page 0.
- **`NewDatabaseHasEmptyValidCatalog`**: Asserts a newly initialized database file writes the correct catalog header magic (`0x31424451`) and begins with zero tables.
- **`TablesHaveDistinctFirstPages`**: Verifies that creating multiple tables allocates isolated, non-overlapping root pages.
- **`RecordsDoNotLeakBetweenTables`**: Confirms that records inserted into Table A are never accessible or visible when scanning Table B.
- **`FindTableReturnsCorrectTable`** & **`FindTableWorksThroughConstReference`**: Validates pointer lookups by table name in mutable and const contexts.
- **`FindNonexistentTableReturnsNullptr`**: Asserts graceful handling when querying non-existent table identifiers.
- **`DuplicateTableNameIsRejectedWithoutModifyingCatalog`**: Enforces table name uniqueness constraints.
- **`CloseAndReopenRecoversTablesAndRecords`**: Reopens a database with multiple tables and verifies that schemas and rows are completely reconstructed.
- **`CreateTableAfterReopenKeepsExistingData`**: Tests schema evolution by creating new tables after a restart without altering prior tables.
- **`GarbageFileIsRejected`**, **`TruncatedFileIsRejected`**, **`CorruptedCatalogHeaderIsRejected`**: Verifies resilient error detection against malformed, partial, or corrupted database files.

### 3. `SchemaTest` (9 Tests)
Ensures strict type-system enforcement and schema validation prior to writing records to disk.
- **`AcceptsRecordMatchingSchema`**: Confirms valid records pass schema validation.
- **`RejectsWrongFieldCountWithoutWritingRecord`**: Rejects rows with too few or too many fields without allocating storage space.
- **`RejectsTypeMismatchWithoutWritingRecord`**: Rejects attempts to insert a string into an integer column or vice versa.
- **`RejectsNullInNonNullableColumn`** & **`AcceptsNullInNullableColumn`**: Enforces column nullability constraints (`std::monostate`).
- **`RejectsEmptySchema`** & **`RejectsDuplicateColumnNames`**: Validates table definition integrity at creation time.
- **`RejectsNullablePrimaryKey`**: Enforces relational database primary key invariants (primary keys cannot be NULL).
- **`SchemaAndValidationSurviveDatabaseReopen`**: Confirms serialized schema definitions in Page 0 remain enforced after restarts.

### 4. `BPlusTreeTest` (3 Tests)
Validates the on-disk, page-backed B+ Tree implementation (`include/index/bplus_tree.hpp`).
- **`FindsKeysAndRejectsDuplicates`**: Basic key lookup and unique key enforcement.
- **`FindsAllKeysAfterLeafAndInternalSplits`**: Inserts keys beyond the 128-key node capacity, triggering leaf splits, internal separator node splits, and root page reallocation.
- **`RootAndEntriesSurviveReopen`**: Reopens the database from raw metadata pages and verifies complete tree traversal and key-search validity.

### 5. `PrimaryIndexTest` (8 Tests)
Tests `PrimaryIndex`, which bridges 64-bit integer primary keys to physical slotted-page `RecordID` (`{page_id, slot_id}`).
- **`CreateAndInsertThenFind`**: Tests index creation, key insertion, and $\mathcal{O}(\log n)$ lookup.
- **`FindMissingKeyReturnsEmptyOptional`**: Validates non-existent keys return `std::nullopt` instead of crashing.
- **`DuplicateKeyReturnsDuplicatePrimaryKeyStatus`**: Enforces primary key uniqueness, returning `StatusCode::DuplicatePrimaryKey`.
- **`SupportsNegativeZeroAndIntegerBoundaries`**: Verifies ordering and lookups for boundary values (`INT64_MIN`, `-1`, `0`, `INT64_MAX`).
- **`ReopenPreservesEntries`** & **`InsertAfterReopeningIndex`**: Confirms crash recovery and continued write capability after reload.
- **`LeafSplitsPreserveAllMappings`** & **`ReopenAfterLeafSplitsPreservesAllMappings`**: Verifies all `RecordID` pointers remain accurately mapped across leaf page splitting events.

### 6. `BplusTreeDeletionTest` (12 Tests)
Exhaustively tests the B+ Tree deletion and rebalancing algorithm (`BplusTree::erase`).
- **`EraseFromEmptyTreeAndSingleKeyLifecycle`**: Validates deletion on an empty tree and complete single-key insertion/deletion lifecycles.
- **`AllDeletionPermutationsSmallTree`**: Tests all permutation orders of deleting keys from small trees.
- **`SingleLeafMaxCapacityAllDeletionPatterns`**: Tests deleting keys from full leaves (boundary, middle, ascending, descending).
- **`BorrowFromRightSiblingAndUpdateSeparator`** & **`BorrowFromLeftSiblingAndUpdateSeparator`**: Validates redistribution of keys when a node underflows, updating the parent separator key.
- **`MergeWithRightSiblingAndCollapseRoot`** & **`MergeWithLeftSiblingAndCollapseRoot`**: Validates merging underflowing sibling nodes and shrinking the tree height when the root node becomes empty.
- **`SiblingBorrowsAndMergesWithRootCollapse`**: Tests interleaved borrows and merges across multiple subtrees.
- **`ThreeLevelTreeInternalBorrowMergeAndMinPropagation`**: Validates 3-level tree rebalancing with recursive minimum-key propagation to ancestors.
- **`MergedAndCollapsedPagesAreDeallocatedAndReused`**: Verifies that collapsed B+ tree pages are deallocated and recycled through the pager's freelist.
- **`PersistenceAcrossMultipleReopens`**: Tests durability of deletions across multiple reopen cycles.
- **`Int64ExtremesAndRandomizedDifferentialStress`**: Randomized fuzz testing comparing B+ tree state against `std::map`.

### 7. `BplusTreeHighFrequencyTest` (3 Tests)
Heavy stress tests validating high-frequency churn and freelist page recycling.
- **`SlidingWindowHighFrequencyChurnAndDiskBound`**: Continuously inserts new keys and deletes old keys in a sliding window to prove that disk space remains bounded via page recycling.
- **`MultiWaveSawtoothExpansionCollapseAndFileReuse`**: Alternates between allocating thousands of keys and deleting them, validating sawtooth expansion and collapse.
- **`SixtyThousandOpAdversarialReadWriteDeleteBlitz`**: Executes 60,000 random operations (insert, find, erase) against an oracle, executing deep tree invariant validation at each stage.

### 8. `BplusDBTest` (2 Tests)
Integration testing connecting all physical storage layers together (`Table` + `Record` + `PrimaryIndex` + `BplusTree`).
- **`FullStackHighFrequencyReadWriteUpdateDeleteAndLazyCompaction`**:
  - Session 1: 1,200 initial inserts followed by 10,000 randomized OLTP operations (`SELECT`, `UPDATE`, `DELETE`, `INSERT`).
  - Session 2: Database restart, full table scan, and 100% deletion of all records, verifying final compaction and structural integrity.
- **`DatabaseCatalogMultiSessionTableAndBplusTreeIntegrity`**: Multi-table catalog with 600 records, 420 deletions, 180 updates, and crash recovery verification.

### 9. `LexerTest` (9 Tests)
Tests the hand-written SQL lexer (`src/sql/lexer.cpp`).
- **`TokenizeCreateTable`**, **`TokenizeInsertInto`**, **`TokenizeSelectWhereAndOperators`**, **`TokenizeUpdateSet`**, **`TokenizeDeleteFrom`**: Verifies precise token sequence generation for all supported SQL statements.
- **`CaseInsensitiveKeywords`**: Confirms SQL keywords are case-insensitive (`select`, `SELECT`, `SeLeCt`).
- **`StringEscapingAndQuotes`**: Validates single-quoted string literals with embedded escape characters.
- **`CommentsSkipped`**: Ensures single-line SQL comments (`-- comment`) are skipped without polluting the token stream.
- **`SyntaxErrorOnUnterminatedString`**: Tests robust error reporting on unclosed string literals.

### 10. `ParserTest` (11 Tests)
Tests the recursive-descent AST parser (`src/sql/parser.cpp`).
- **`ParseCreateTable`**, **`ParseDropTable`**, **`ParseInsertWithAndWithoutColumns`**, **`ParseSelectStarAndColumns`**, **`ParseUpdateStatement`**, **`ParseDeleteStatement`**: AST generation for all statements.
- **`SyntaxErrorHandling`**: Verifies informative error messages when encountering invalid SQL syntax.
- **`ParseCompoundWhereWithAndOr`**: Verifies expression tree construction with operator precedence (`AND` binds tighter than `OR`) and parenthesized expressions (`(a OR b) AND c`).
- **`ParseJoinStatements`**: Verifies parsing of `INNER JOIN` and `LEFT JOIN` with `ON` predicates and table-qualified columns (`users.id = orders.user_id`).
- **`ParseOrderByAndLimit`**: Verifies parsing of `ORDER BY <col> [ASC|DESC]`, `LIMIT <n>`, and `OFFSET <m>`.
- **`ParseAggregateFunctions`**: Verifies parsing of `COUNT(*)`, `COUNT(col)`, `SUM(col)`, `AVG(col)`, `MIN(col)`, `MAX(col)`, and enforces syntax constraints (rejecting `*` in non-COUNT aggregates, rejecting mixing plain columns with aggregates without `GROUP BY`).

### 11. `SQLExecutorTest` (10 Tests)
Validates query planning, execution, and relational algebra (`src/execution/executor.cpp`).
- **`EndToEndCreateTableInsertSelect`**: Complete DDL and DML query pipeline.
- **`EndToEndUpdateAndDelete`**: Modifies and deletes rows using filtered WHERE clauses.
- **`OrderByAndLimitPagination`**: Validates `ORDER BY` (ASC/DESC, integers, strings, NULL placement, unprojected columns) and pagination with `LIMIT` and `OFFSET`.
- **`AggregateFunctions`**: Validates execution of aggregate functions `COUNT(*)` vs `COUNT(col)` skipping NULLs, `SUM` and `AVG` arithmetic with NULL exclusion, `MIN` and `MAX` across integer and text domains, post-aggregation WHERE filtering, empty-set aggregation semantics (`COUNT` = 0, `SUM`/`AVG`/`MIN`/`MAX` = NULL), non-integer type checking, `LIMIT 0`/`1` behavior, and aggregations across relational JOINs.
- **`NonPrimaryKeyUpdateAndDelete`**: Validates multi-row updates and deletes on tables without primary keys, larger payload resizing, unconditional delete, and persistence across database restart.
- **`DuplicatePrimaryKeyIsRejected`**: Ensures SQL inserts fail gracefully when primary keys collide.
- **`DropTableLifecycleAndPersistence`**: Validates dropping tables, verifying in-memory and on-disk catalog deletion, page deallocation, subsequent query/drop rejections, and clean table recreation across restarts.
- **`MultiConditionWhereAndOr`**: Validates execution of compound conditions with logical `AND` / `OR` and nested parentheses.
- **`PersistenceAcrossReopen`**: Queries data across database restart sessions via SQL commands.
- **`JoinTwoTables`**: Validates relational joins:
  - `INNER JOIN` with all columns (`SELECT *`)
  - `INNER JOIN` with explicit column projections
  - `INNER JOIN` filtered by post-join `WHERE` predicates
  - Reversible `ON` conditions (`users.id = orders.user_id` and `orders.user_id = users.id`)
  - `LEFT JOIN` emitting outer rows with `NULL` padding for unmatched records
  - Wildcard projection with table qualifier (`SELECT users.*, orders.product ...`)
  - Single-table queries using table-qualified columns (`SELECT users.name FROM users WHERE users.id = 2`)

### 12. `SQLScaleTest` (7 Tests)
End-to-end performance and scalability suite scaling up to 120,000 records.
- **`Scale_100`** to **`Scale_120000`**: Measures write throughput (QPS), read throughput (QPS), read latency ($\mu\text{s}$), and file footprint at scale milestones (100, 1K, 10K, 50K, 100K, 120K).
- **`FullProgressionAndLimitsMatrix`**: Executes the comprehensive scale matrix and verifies 100% data integrity across all 120,000 records.

### 13. `CAPITest` (7 Tests)
Validates the embedded C API (`include/quartz/quartz.h`, `include/quartz.h`, `src/capi/quartz_c.cpp`).
- **`LifecycleOpenClose`**: Tests database handle creation (`qdb_open`), shutdown (`qdb_close`), and null pointer safety guards.
- **`TableCreationAndInsertion`**: Validates executing DDL and DML statements and inspecting rows affected (`qdb_result_rows_affected`).
- **`SelectAndTypedColumnAccess`**: Verifies typed cell access (`qdb_column_type`, `qdb_column_int`, `qdb_column_text`, `qdb_column_is_null`), column metadata, and out-of-bounds guards.
- **`AggregateQueries`**: Validates aggregate functions evaluated through the C API (`COUNT(*)`, `COUNT(col)`, `SUM`, `AVG`, `MIN`, `MAX`).
- **`ErrorHandling`**: Verifies error code propagation (`QDB_ERROR`), error messages via `qdb_errmsg()` and `qdb_result_message()`.
- **`CallbackExecAPI`**: Tests SQLite-compatible callback execution (`qdb_exec`), user data passing, row iteration, and early abort logic.
- **`VersionInfo`**: Validates version reporting via `qdb_version()`.

---

## Running the Tests

### Run All Tests via CTest
```bash
ctest --test-dir build --output-on-failure
```

### Run All Tests Directly via Executable
```bash
# Windows
.\out\build\x64-Debug\quartz_tests.exe

# Linux / macOS
./build/quartz_tests
```

### Run Specific Test Suites
Use the `--gtest_filter` flag to run individual suites or patterns:

```bash
# Run only SQL executor and join tests
.\quartz_tests.exe --gtest_filter=SQLExecutorTest.*

# Run only SQL parser tests
.\quartz_tests.exe --gtest_filter=ParserTest.*

# Run B+ tree deletion and rebalancing tests
.\quartz_tests.exe --gtest_filter=BplusTreeDeletionTest.*

# Run all SQL tests (lexer, parser, executor)
.\quartz_tests.exe --gtest_filter=*SQL*:*Parser*:*Lexer*

# Run the 120,000-row scale benchmark
.\quartz_tests.exe --gtest_filter=SQLScaleTest.FullProgressionAndLimitsMatrix
```
