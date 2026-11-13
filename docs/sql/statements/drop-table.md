# DROP TABLE

The `DROP TABLE` statement deletes a registered table from the database, deallocates its slotted data pages, tears down its B+ Tree index, and reclaims all pages into the pager's free-list.

---

## Syntax Diagram

<SyntaxRailroad title="DROP TABLE Railroad Grammar" viewBox="0 0 520 80">
  <path d="M 20 40 L 50 40" class="track" />

  <!-- DROP TABLE -->
  <rect x="50" y="24" width="120" height="32" rx="4" class="token-box keyword" />
  <text x="110" y="40" class="token-text keyword">DROP TABLE</text>
  <path d="M 170 40 L 200 40" class="track" />

  <!-- Table Name -->
  <rect x="200" y="24" width="90" height="32" rx="4" class="token-box" />
  <text x="245" y="40" class="token-text">table</text>
  <path d="M 290 40 L 320 40" class="track" />

  <!-- Semicolon -->
  <rect x="320" y="25" width="25" height="30" rx="4" class="token-box keyword" />
  <text x="332" y="40" class="token-text keyword">;</text>
  <path d="M 345 40 L 380 40" class="track" />
</SyntaxRailroad>

---

## Deallocation Workflow

When a table is dropped:
1. **Catalog Unregistration**: The table entry is removed from Page 0, updating `table_count`.
2. **Data Page Freeing**: The engine traverses the `next_page` linked list, passing each `PageID` to `pager::deallocate_page()`.
3. **Index Page Freeing**: `BplusTree::deallocate_all_pages()` traverses internal and leaf nodes in post-order DFS, recycling all B+ tree pages into the `free_pages` list.
4. **Immediate Space Reuse**: Deallocated page IDs are immediately reused for subsequent `CREATE TABLE` or `INSERT` operations without requiring file compaction.

---

## Examples

```sql
DROP TABLE orders;
```
