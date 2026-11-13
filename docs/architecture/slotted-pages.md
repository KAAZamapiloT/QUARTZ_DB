# Slotted-Page Storage 📦

QuartzDB uses the **Slotted-Page Architecture** (`include/storage/table.hpp`, `src/storage/table.cpp`) for storing records within 4KB data pages.

---

## Why Slotted Pages?

Relational tables frequently contain variable-length attributes (e.g. `std::string` text fields) and dynamic values (`NULL`). Traditional fixed-record arrays lead to severe internal and external disk fragmentation.

The slotted-page design solves this by separating **record addressing pointers (slots)** from **actual record payload bytes**.

---

## Page Byte Layout

Inside each 4096-byte data page, memory is laid out with a **two-way growth pattern**:

```text
+-----------------------------------------------------------------------+
|  PAGE HEADER (Magic, Slot Count, Free Space Start/End, Next Page ID)   |
+-----------------------------------------------------------------------+
|  Slot 0: { Offset, Length }   --> Grows DOWNWARD                      |
|  Slot 1: { Offset, Length }                                           |
|  Slot 2: { Offset, Length }                                           |
|  ...                                                                  |
|                      [ UNALLOCATED FREE SPACE ]                       |
|  ...                                                                  |
|  Record 2 Payload                                                     |
|  Record 1 Payload                                                     |
|  Record 0 Payload             <-- Grows UPWARD from bottom            |
+-----------------------------------------------------------------------+
```

### Components

1. **Page Header**:
   - `magic_number`: Verifies valid slotted page structure.
   - `slot_count`: Current count of allocated slots in the array.
   - `free_space_start`: Byte offset where the next slot entry can be written.
   - `free_space_end`: Byte offset where the next record payload can be placed.
   - `next_page_id`: Singly linked list pointer to the subsequent data page in the table.

2. **Slot Array**:
   - Resides immediately after the header and grows **forward** towards the bottom.
   - Each slot is a compact pair: `struct Slot { uint16_t offset; uint16_t length; }`.

3. **Record Payloads**:
   - Appended starting from byte 4095 and grow **backward** towards the slot array.

4. **Collision Detection**:
   - When `free_space_start + sizeof(Slot) > free_space_end - record_size`, the page is full.
   - The pager allocates a new data page, updates `next_page_id`, and links it to the chain.

---

## Record Addressing: `RecordID`

Every record in QuartzDB has a permanent 64-bit address defined by:

```cpp
struct RecordID {
    PageID   page_id;     // 32-bit ID of the 4KB data page
    uint16_t slot_index;  // Index into the page's slot array
};
```

This indirection allows the engine to defragment or relocate record payloads within a page without invalidating index pointers, as the `RecordID` references the slot, not the raw physical byte offset.
