# Slotted-Page Storage & Pager 📦

QuartzDB uses the **Slotted-Page Architecture** inside fixed 4096-byte blocks to prevent external fragmentation from variable-length text records.

---

## Memory Layout

A slotted page is a spatial memory structure, not a sequential stream. Each 4096-byte page divides space between a forward-growing slot directory and backward-growing record payloads:

<MemoryMap
  title="4KB Slotted-Page Physical Spatial Map"
  subheader="PageID: 1 · Fixed 4096-Byte Block Allocation"
  :totalBytes="4096"
  footerNote="Slots grow downward from header offset 24; dynamic record payloads grow upward from byte 4096. Compaction reclaims gaps."
  :segments="[
    { id: 'hdr', kind: 'header', bytes: 24, label: 'Page Header (24B)', sublabel: 'QPG1 · slot_count · free_start · free_end · version · next_page' },
    { id: 'slots', kind: 'slot', bytes: 8, label: 'Slot Array (2 slots · 8B)', sublabel: 'Slot 0 {offset: 4020, len: 76} · Slot 1 {offset: 3950, len: 70}', growth: 'down' },
    { id: 'free', kind: 'free', bytes: 3918, label: 'Unallocated Free Space (3,918B)', sublabel: 'free_start (0x0020) .. free_end (0x0F6E)' },
    { id: 'records', kind: 'record', bytes: 146, label: 'Record Payloads (146B)', sublabel: 'Record 1 @3950 (len: 70B) · Record 0 @4020 (len: 76B)', growth: 'up' }
  ]"
/>

---

## Interactive 4KB Page Hex & Structure Inspector

Inspect the raw 4096-byte memory dump and decoded structure in real-time. Hover over **Slot 0** to trace the exact 4 directory bytes to the record payload at byte 4020:

<HexInspector />

---

## The RecordID Pointer

Records are addressed by a stable logical tuple:

```cpp
struct RecordID {
    PageID   page_id;     // 64-bit ID of the 4KB data page
    uint16_t slot_index;  // Index into the page's slot array
};
```

Because B+ tree leaf nodes reference `RecordID` rather than physical byte offsets, pages can defragment and relocate records via `compact_page()` without breaking index pointers.

---

## Insert Path & Two-Way Growth

When a new tuple is inserted:
1. `RecordCodec::serialize()` packs values into a compact byte slice.
2. Contiguous free space is checked: `free_end - free_start >= required_bytes`.
3. The slot at offset `free_start` is allocated and grows **downward**.
4. The record payload is placed at offset `free_end - len` and grows **upward**.

---

## Page Fill & Compaction

When records are deleted or updated with smaller values, empty gaps are recorded as `fragmented_bytes`. When space runs short, `compact_page()` shifts active records contiguous to byte 4096, resetting `fragmented_bytes = 0`.
