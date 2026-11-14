# Slotted-Page Storage 📦

> [!TIP] Canonical Storage Concept
> Explore the interactive 4KB Hex Inspector and spatial memory maps at [Slotted-Page Storage](/concepts/storage).

QuartzDB uses the **Slotted-Page Architecture** (`include/storage/table.hpp`, `src/storage/table.cpp`) for storing records within 4096-byte data pages.

---

## Memory Layout

A slotted page is a spatial memory structure, not a sequential stream. To avoid external fragmentation from variable-length text records, each 4096-byte page divides space between a forward-growing slot directory and backward-growing record payloads:

<MemoryMap
  title="4KB Slotted-Page Physical Spatial Map"
  subheader="PageID: 1 · Fixed 4096-Byte Block Allocation"
  :totalBytes="4096"
  footerNote="Slot array grows downward from header offset 24; dynamic record payloads grow upward from byte 4096. Compaction reclaims gaps."
  :segments="[
    { id: 'hdr', kind: 'header', bytes: 24, label: 'Page Header (24B)', sublabel: 'QPG1 · slot_count · free_start · free_end · version · next_page' },
    { id: 'slots', kind: 'slot', bytes: 8, label: 'Slot Array (2 slots · 8B)', sublabel: 'Slot 0 {offset: 4020, len: 76} · Slot 1 {offset: 3950, len: 70}', growth: 'down' },
    { id: 'free', kind: 'free', bytes: 3918, label: 'Unallocated Free Space (3,918B)', sublabel: 'free_start (0x0020) .. free_end (0x0F6E)' },
    { id: 'records', kind: 'record', bytes: 146, label: 'Record Payloads (146B)', sublabel: 'Record 1 @3950 (len: 70B) · Record 0 @4020 (len: 76B)', growth: 'up' }
  ]"
/>

### Page Header Layout (24 Bytes)

The header occupies byte offsets 0 through 23 of every table data page:

| Byte Range | Field | Type | Description |
| :--- | :--- | :--- | :--- |
| `00 .. 03` | `magic_number` | `uint32_t` | Constant `0x31475051` ("QPG1" in little-endian). |
| `04 .. 05` | `slot_count` | `uint16_t` | Count of active and tombstone slots in slot array. |
| `06 .. 07` | `free_start` | `uint16_t` | Byte offset where the next slot entry can be written (starts at 24). |
| `08 .. 09` | `free_end` | `uint16_t` | Byte offset where the next record payload can be placed (starts at 4096). |
| `10 .. 11` | `page_version` | `uint16_t` | Format version (currently `1`). |
| `12 .. 19` | `next_page` | `uint64_t` | Singly-linked list pointer to the next table data page (`NO_PAGE` if last). |
| `20 .. 21` | `fragmented_bytes` | `uint16_t` | Gap bytes freed by updates or deletions pending compaction. |
| `22 .. 23` | `reserved` | `uint16_t` | Word alignment padding. |

---

## Interactive 4KB Page Hex & Structure Inspector

Inspect the raw 4096-byte memory dump and decoded structure in real-time. Hover over **Slot 0** to trace the exact 4 directory bytes to the record payload at byte 4020, or simulate insertions, deletions, and page compaction:

<HexInspector />

---

## The RecordID Pointer

In QuartzDB, records are never addressed by raw physical byte offsets. Instead, every record is uniquely and permanently referenced by a 64-bit **`RecordID`**:

```cpp
struct RecordID {
    PageID   page_id;     // 64-bit ID of the 4KB data page
    uint16_t slot_index;  // 16-bit index into the page's slot array
};
```

### Logical Indirection
When records are defragmented or compacted within a page, their byte offsets shift. However, **their slot index in the array does not change**. 

Because B+ Tree leaf nodes reference the stable `RecordID`, table compaction never requires updating or invalidating index pointers.

---

## Insert Path & Two-Way Growth

When a new tuple is inserted into a table:

1. **Serialization**: `RecordCodec::serialize()` packs values into a compact byte slice.
2. **Space Check**: The table checks whether the page has contiguous free capacity:
   ```text
   required = sizeof(Slot) + payload_length ≤ free_end - free_start
   ```
3. **Slot Allocation**: The slot at offset `free_start` is populated:
   ```cpp
   Slot slot { .offset = free_end - payload_length, .length = payload_length };
   ```
4. **Payload Placement**: Bytes are copied into `page.data[slot.offset]`.
5. **Pointer Adjustment**: `free_start += sizeof(Slot)` grows downward; `free_end -= payload_length` grows upward.

---

## Page Fill & Compaction

When records are updated with smaller values or deleted (`length = 0`), the unused space is recorded as `fragmented_bytes`:

<ExcalidrawDiagram
  name="slotted-pages-compaction"
  title="Page Fill and Compaction Sequence"
  alt="Sequence diagram of compact_page() sorting active slots, packing payloads contiguous to byte 4096, and resetting fragmented_bytes to 0"
/>

### When Compaction Triggers
1. **Automatic Insert Pass**: If `free_end - free_start < required`, but `(free_end - free_start) + fragmented_bytes >= required`, `compact_page()` runs before allocating a new block.
2. **Page Linking**: If total free space is insufficient even after compaction, `pager::allocate_page()` fetches a new 4KB page and links it to `next_page`.

---

## Serialization Mechanics (RecordCodec)

Records serialize values into typed tags:

| Value Type | Tag ID | Representation |
| :--- | :---: | :--- |
| `std::monostate` (NULL) | `0` | Single 1-byte tag |
| `std::int64_t` | `1` | 1-byte tag + 8-byte little-endian payload |
| `std::string` | `2` | 1-byte tag + 4-byte uint32 length + UTF-8 bytes |
