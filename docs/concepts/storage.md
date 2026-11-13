# Slotted-Page Storage & Pager 📦

QuartzDB uses the **Slotted-Page Architecture** inside fixed 4096-byte blocks to prevent external fragmentation from variable-length text records.

---

## 4KB Slotted-Page Memory Layout

Each data page utilizes a two-way growth pattern:

```mermaid
graph TD
    subgraph PageBlock ["4096-Byte Slotted Page"]
        direction TB
        H["Page Header (24 Bytes)<br>Magic: QPG1 | Slot Count | Free Start | Free End | Version | Next Page"]
        
        subgraph Slots ["Slot Array (Grows Downward ↓)"]
            S0["Slot 0: { offset: 4020, length: 76 }"]
            S1["Slot 1: { offset: 3950, length: 70 }"]
        end
        
        FREE["Unallocated Free Space (free_start .. free_end)"]
        
        subgraph Payloads ["Record Payloads (Grows Upward ↑)"]
            R1["Record 1 Payload (3950 .. 4019)"]
            R0["Record 0 Payload (4020 .. 4095)"]
        end

        H --> S0 --> S1 --> FREE --> R1 --> R0
    end
```

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
