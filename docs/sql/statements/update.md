# UPDATE

The `UPDATE` statement modifies existing records satisfying a predicate filter.

---

## Syntax Diagram

<SyntaxRailroad title="UPDATE Statement Railroad Grammar" viewBox="0 0 680 80">
  <path d="M 20 40 L 50 40" class="track" />

  <!-- UPDATE -->
  <rect x="50" y="24" width="80" height="32" rx="4" class="token-box keyword" />
  <text x="90" y="40" class="token-text keyword">UPDATE</text>
  <path d="M 130 40 L 150 40" class="track" />

  <!-- Table -->
  <rect x="150" y="24" width="75" height="32" rx="4" class="token-box" />
  <text x="187" y="40" class="token-text">table</text>
  <path d="M 225 40 L 250 40" class="track" />

  <!-- SET -->
  <rect x="250" y="24" width="55" height="32" rx="4" class="token-box keyword" />
  <text x="277" y="40" class="token-text keyword">SET</text>
  <path d="M 305 40 L 325 40" class="track" />

  <!-- Assignments -->
  <rect x="325" y="24" width="160" height="32" rx="4" class="token-box" />
  <text x="405" y="40" class="token-text">col = val, ...</text>
  <path d="M 485 40 L 505 40" class="track" />

  <!-- WHERE Clause -->
  <rect x="505" y="24" width="105" height="32" rx="4" class="token-box" />
  <text x="557" y="40" class="token-text">WHERE expr</text>
  <path d="M 610 40 L 630 40" class="track" />

  <!-- Semicolon -->
  <rect x="630" y="25" width="25" height="30" rx="4" class="token-box keyword" />
  <text x="642" y="40" class="token-text keyword">;</text>
  <path d="M 655 40 L 675 40" class="track" />
</SyntaxRailroad>

---

## Behavior & Storage Mechanics

1. **In-Place vs Re-Slotting**: If the updated record payload fits within the original slot length, the payload is overwritten in-place. If the updated record is larger, the slot is marked fragmented, and the new payload is written to the end of free space (`free_end`).
2. **Primary Key Modification**: Updating the primary key column will update the `PrimaryIndex` B+ Tree index to point to the record. If the new key collides with an existing key, the transaction aborts with `QERR_PK_DUPLICATE`.

---

## Examples

```sql
UPDATE users 
SET email = 'alice.new@company.com' 
WHERE id = 1;

UPDATE orders 
SET status = 'SHIPPED' 
WHERE status = 'PAID' AND id < 5000;
```
