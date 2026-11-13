# DELETE

The `DELETE` statement removes records from a table matching a `WHERE` predicate filter.

---

## Syntax Diagram

<SyntaxRailroad title="DELETE Statement Railroad Grammar" viewBox="0 0 620 80">
  <path d="M 20 40 L 50 40" class="track" />

  <!-- DELETE FROM -->
  <rect x="50" y="24" width="130" height="32" rx="4" class="token-box keyword" />
  <text x="115" y="40" class="token-text keyword">DELETE FROM</text>
  <path d="M 180 40 L 205 40" class="track" />

  <!-- Table -->
  <rect x="205" y="24" width="80" height="32" rx="4" class="token-box" />
  <text x="245" y="40" class="token-text">table</text>
  <path d="M 285 40 L 310 40" class="track" />

  <!-- WHERE Clause -->
  <rect x="310" y="24" width="220" height="32" rx="4" class="token-box" />
  <text x="420" y="40" class="token-text">WHERE condition_expr</text>
  <path d="M 530 40 L 555 40" class="track" />

  <!-- Semicolon -->
  <rect x="555" y="25" width="25" height="30" rx="4" class="token-box keyword" />
  <text x="567" y="40" class="token-text keyword">;</text>
  <path d="M 580 40 L 610 40" class="track" />
</SyntaxRailroad>

---

## Storage & Deletion Mechanics

1. **Slotted Page Tombstones**: The slot length is zeroed (`length = 0`), and the byte count is added to `fragmented_bytes`.
2. **B+ Tree Key Erasure**: If the primary key is deleted, `BplusTree::erase()` is invoked to remove the key from leaf nodes, executing underflow rebalancing or page merging if key count drops below `MIN_KEYS = 64`.
3. **Space Reclamation**: The physical space occupied by the deleted payload is reclaimed on the next `compact_page()` pass.

---

## Examples

### Delete by Primary Key ($O(\log N)$)
```sql
DELETE FROM users WHERE id = 1042;
```

### Delete with Boolean Predicate
```sql
DELETE FROM orders WHERE status = 'CANCELLED' OR total_cents = 0;
```
