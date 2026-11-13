# Slotted-Page Storage 📦

QuartzDB uses the **Slotted-Page Architecture** (`include/storage/table.hpp`, `src/storage/table.cpp`) for storing records within 4096-byte data pages.

---

## Slotted-Page Memory Anatomy

Each data page uses a **two-way growth layout**:
1. **Header & Slot Array**: Grow **forward** (top-down) starting at offset 0.
2. **Record Payloads**: Grow **backward** (bottom-up) starting at byte 4096.

```mermaid
graph TD
    subgraph PageLayout ["4096-Byte Slotted Page Layout (PageID: N)"]
        direction TB
        H["Page Header (24 Bytes)<br>Magic: QPG1 | Slot Count | Free Start | Free End | Version: 1 | Next Page | Frag Bytes"]
        
        subgraph Slots ["Slot Array (Grows Downward ↓)"]
            S0["Slot 0: { offset: 4020, length: 76 }"]
            S1["Slot 1: { offset: 3950, length: 70 }"]
            S2["Slot 2: { offset: 0, length: 0 } (Tombstone Deleted)"]
        end
        
        FREE["Unallocated Free Space<br>(free_start .. free_end)"]
        
        subgraph Payloads ["Record Payloads (Grows Upward ↑)"]
            R1["Record 1 Payload (Bytes 3950 .. 4019)"]
            R0["Record 0 Payload (Bytes 4020 .. 4095)"]
        end
        
        H --> S0 --> S1 --> S2 --> FREE --> R1 --> R0
    end
```

---

## Page Header Specification (24 Bytes)

The header occupies the first 24 bytes of every table data page:

```mermaid
classDiagram
    class SlottedPageHeader {
        +00..03: uint32 PAGE_MAGIC = 0x31475051 ("QPG1")
        +04..05: uint16 slot_count
        +06..07: uint16 free_start (Header end + slot_count * 4)
        +08..09: uint16 free_end (Bottom of lowest record payload)
        +10..11: uint16 PAGE_VERSION = 1
        +12..19: uint64 next_page (PageID of next linked page)
        +20..21: uint16 fragmented_bytes
        +22..23: uint16 reserved_padding
    }
```

- **`free_start`**: Offset where the next slot entry can be written. Initialized to `24`.
- **`free_end`**: Offset where the next record payload can be placed. Initialized to `4096`.
- **`fragmented_bytes`**: Accumulates unusable gap bytes created when records are deleted or updated with smaller payloads.

---

## Slot Array & RecordID Indirection

In QuartzDB, records are never referenced by raw byte offsets. Instead, they are referenced by a persistent **`RecordID`**:

```mermaid
graph LR
    RID["RecordID<br>page_id: 1<br>slot_index: 1"] -->|1. Seek to Page 1| PAGE["Page 1<br>(4096 Bytes)"]
    
    PAGE -->|2. Index into Slot Array at Header + 1*4| SLOT["Slot 1<br>offset: 3950<br>length: 70"]
    
    SLOT -->|3. Read Byte Slice 3950 .. 4019| PAYLOAD["Record Payload<br>[RecordCodec::deserialize]"]
```

### Why Logical Indirection Matters
When a page is defragmented (compacted), records are moved to new byte offsets. However, **the slot index remains unchanged**. As a result, B+ Tree index pointers (`RecordID`) remain permanently valid without needing re-indexing!

---

## Record Binary Serialization (`RecordCodec`)

Records are serialized into compact byte vectors with typed tag discriminators:

```mermaid
flowchart LR
    subgraph SerializedRecord ["Serialized Record Byte Format"]
        direction LR
        N["Num Values<br>(4 Bytes uint32)"]
        
        V0["Col 0 Tag: 1 (INT64)<br>+ 8 Bytes (int64_t)"]
        V1["Col 1 Tag: 2 (TEXT)<br>+ 4 Bytes len<br>+ Raw string bytes"]
        V2["Col 2 Tag: 0 (NULL)<br>(1 Byte tag only)"]
        
        N --> V0 --> V1 --> V2
    end
```

| Tag ID | C++ Type | Byte Size | Serialization Scheme |
| :--- | :--- | :--- | :--- |
| `0` (`NULL_TAG`) | `std::monostate` | 1 byte | Single tag byte |
| `1` (`INTEGER_TAG`) | `std::int64_t` | 9 bytes | 1 byte tag + 8 bytes little-endian integer |
| `2` (`TEXT_TAG`) | `std::string` | $5 + L$ bytes | 1 byte tag + 4 bytes length ($L$) + UTF-8 string bytes |

---

## Linked List Table Chain

When a page fills up (`free_start + 4 > free_end - record_size`), a new page is allocated and linked via `next_page`:

```mermaid
graph LR
    T["Table Object<br>first_page_: 1"] --> P1["Data Page 1<br>next_page: 4"]
    P1 --> P4["Data Page 4<br>next_page: 9"]
    P4 --> P9["Data Page 9<br>next_page: NO_PAGE (0xFFFFFFFFFFFFFFFF)"]
```

---

## Page Compaction Algorithm (`compact_page`)

When updates or deletions leave fragmentation (`fragmented_bytes > 0`), the engine rebuilds the page cleanly:

```mermaid
sequenceDiagram
    participant P as Fragmented Page
    participant R as Rebuilt Page Buffer
    participant H as PageHeader

    Note over P: Slots 0 and 2 active; Slot 1 deleted (length: 0). Free space split by gaps.
    P->>R: initialize_page(R)
    P->>R: Copy slot_count and next_page
    loop For each active slot i (0 .. slot_count - 1)
        alt record_length == 0
            Note over R: Write empty slot tombstone {0, 0}
        else record_length > 0
            Note over R: Decrement new_free_end by record_length
            P->>R: Copy bytes from old offset to new_free_end
            R->>R: Update Slot[i] with new_free_end and length
        end
    end
    R->>H: Set free_end = new_free_end, fragmented_bytes = 0
    Note over P: Page now fully compacted with 0 fragmented bytes
```
