# products

*Represents warehouse product catalog inventory, pricing models, and SKU descriptions.*  
**Last updated:** `2026-11-13` · **Owner:** `catalog-logistics`

---

<!-- Stat Card Wireframe Banner -->
<div class="stat-card-row">
  <div class="stat-card">
    <div class="stat-card-label">Estimated Rows</div>
    <div class="stat-card-val">📊 8,500</div>
  </div>
  <div class="stat-card">
    <div class="stat-card-label">Disk Footprint</div>
    <div class="stat-card-val">📅 est. 2.4 MB</div>
  </div>
  <div class="stat-card">
    <div class="stat-card-label">4KB Pages Allocated</div>
    <div class="stat-card-val">📄 615 pgs</div>
  </div>
  <div class="stat-card">
    <div class="stat-card-label">Access Pattern</div>
    <div class="stat-card-val">⚡ High Read / Low Write</div>
  </div>
</div>

---

## Columns

| Column Name | Data Type | Nullable | Badges | Description |
| :--- | :--- | :--- | :--- | :--- |
| <span class="click-copy" onclick="navigator.clipboard.writeText('id')">`id`</span> | `INT64` | `NO` | <span class="badge-pk">PK 🔑</span> | Unique numerical product SKU identifier. |
| <span class="click-copy" onclick="navigator.clipboard.writeText('title')">`title`</span> | `TEXT` | `NO` | — | Customer-facing product name. |
| <span class="click-copy" onclick="navigator.clipboard.writeText('price_cents')">`price_cents`</span> | `INT64` | `NO` | — | Unit retail price stored in integer cents (USD). |
| <span class="click-copy" onclick="navigator.clipboard.writeText('stock_quantity')">`stock_quantity`</span> | `INT64` | `NO` | — | Current physical warehouse stock balance. |
| <span class="click-copy" onclick="navigator.clipboard.writeText('description')">`description`</span> | `TEXT` | `YES` | <span class="badge-null">NULL</span> | Extended product technical specification and Markdown overview. |

---

## Keys & Constraints

- **Primary Key Constraint**: Unique primary key on `id` backed by a dedicated B+ Tree index.
- **Stock Invariant**: `stock_quantity >= 0` enforced by business transaction layer.

---

## Related Tables

- **Child Table**: `line_items` via `line_items.product_id = products.id`.

```sql
-- Query low stock products
SELECT id, title, stock_quantity, price_cents
FROM products
WHERE stock_quantity < 10;
```
