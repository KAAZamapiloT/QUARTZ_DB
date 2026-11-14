# Engine Invariants & Constraints ⚖️

This document details the relational invariants, schema constraints, and storage guarantees enforced by the QuartzDB C++23 engine.

---

## 1. Schema & Table Invariants

### 1.1 Single-Column Integer Primary Key
- Every table created via SQL or C++ API must designate **exactly one integer primary key**:
  ```sql
  CREATE TABLE orders (id INT PRIMARY KEY, ...); -- VALID
  CREATE TABLE bad_table (name TEXT PRIMARY KEY);  -- INVALID: Only INT supported for Primary Key
  ```
- The primary key is automatically indexed into a disk-backed B+ Tree upon table creation.

### 1.2 Uniqueness Enforcement
- Inserting a record with a duplicate primary key immediately aborts the insertion:
  ```text
  insert(k, v) requires k ∉ BPlusTree(T)
  ```
- When a duplicate key is detected, the executor returns a `ConstraintViolation` error and no pages are modified.

### 1.3 Nullability Constraints
- Columns declared with `NOT NULL` reject `NULL` (`std::monostate`) values during `INSERT` and `UPDATE`.
- Default columns without `NOT NULL` permit `NULL`.

---

## 2. Storage & Page Constraints

### 2.1 Maximum Record Payload Size
- In the slotted-page architecture, every record must fit inside a single 4096-byte page alongside the page header and slot:
  ```text
  Max Record Size = PAGE_SIZE - HEADER_SIZE - SLOT_SIZE = 4096 - 24 - 4 = 4068 Bytes
  ```
- Tuples exceeding 4068 bytes are rejected with `std::length_error`.

### 2.2 Page Compaction Trigger
- When deletions or updates create fragmented gap bytes (`fragmented_bytes > 0`) and a new record cannot fit in contiguous free space (`free_end - free_start`), the engine automatically triggers `compact_page()` before requesting a new page allocation.

---

## 3. B+ Tree Invariants

- **Occupancy Bounds**: Every non-root node maintains between `MIN_KEYS = 64` and `MAX_KEYS = 128` keys.
- **Tree Uniformity**: All leaf nodes reside at the exact same depth from the root.
- **Sorted Ordering**: Keys within every node must be strictly monotonically increasing:
  ```text
  k₀ < k₁ < k₂ < ... < kₘ₋₁
  ```
- **Horizontal Chain**: Leaves must maintain valid `next_leaf` singly-linked list pointers terminating in `NO_PAGE` (`0xFFFFFFFFFFFFFFFF`).
