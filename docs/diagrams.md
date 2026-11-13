# Relational Diagrams & Interactive ERD 🗺️

This section provides visual entity-relationship diagrams (ERD) mapping the relational models and foreign key chains in QuartzDB.

---

## Interactive Relational ERD

Click on any table or hover to isolate relationships, primary keys 🔑, and foreign key pointers 🔗:

<InteractiveErd />

---

## Physical Page Layout ERD

Below is the low-level physical layout demonstrating how tables link into slotted pages and B+ trees across 4096-byte database blocks:

```mermaid
erDiagram
    CATALOG_PAGE_0 ||--|{ TABLE_DATA_PAGE : "first_page pointer"
    CATALOG_PAGE_0 ||--o| BPLUS_META_PAGE : "index_metadata_page pointer"
    BPLUS_META_PAGE ||--|| BPLUS_ROOT_PAGE : "root_page pointer"
    BPLUS_ROOT_PAGE ||--|{ BPLUS_INTERNAL_PAGE : "child_page pointers"
    BPLUS_INTERNAL_PAGE ||--|{ BPLUS_LEAF_PAGE : "child_page pointers"
    BPLUS_LEAF_PAGE ||--|{ SLOTTED_PAGE_RECORD : "RecordID {page_id, slot_index}"
    TABLE_DATA_PAGE ||--o| TABLE_DATA_PAGE : "next_page linked list"

    CATALOG_PAGE_0 {
        uint32 magic_QDB1
        uint16 version
        uint16 table_count
        entries table_registry
    }
    TABLE_DATA_PAGE {
        uint32 magic_QPG1
        uint16 slot_count
        uint16 free_start
        uint16 free_end
        uint64 next_page
    }
    BPLUS_LEAF_PAGE {
        uint32 magic_BPT1
        uint8 type_LEAF
        uint64 next_leaf
        entries key_and_record_id
    }
```
