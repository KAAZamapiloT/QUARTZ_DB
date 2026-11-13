# Database Tables Index 🗄️

This directory indexes all tables managed by QuartzDB. Each table maintains a persistent 4KB slotted-page record chain and an on-disk B+ Tree primary key index.

---

## Registered Tables Summary

| Table Name | Module | Rows | Est. Size | Primary Key | Description |
| :--- | :--- | :--- | :--- | :--- | :--- |
| [**`orders`**](/tables/orders) | Commerce | 450,000 | ~18.4 MB | `id` (INT64 🔑) | Customer purchase orders, total gross transaction amounts, and lifecycle states. |
| [**`users`**](/tables/users) | Identity | 120,000 | ~12.1 MB | `id` (INT64 🔑) | User account credentials, email addresses (PII ⚠️), roles, and registration metadata. |
| [**`products`**](/tables/products) | Catalog | 8,500 | ~2.4 MB | `id` (INT64 🔑) | Product SKU inventory, pricing in integer cents, stock counts, and descriptions. |
| [**`line_items`**](/tables/orders#related-tables) | Commerce | 1,200,000 | ~36.8 MB | `id` (INT64 🔑) | Itemized purchase line records linking orders to product SKUs. |

---

## Table Cards

<div class="stat-card-row">
  <div class="stat-card">
    <div class="stat-card-label">Commerce Module</div>
    <div class="stat-card-val"><a href="/tables/orders" style="color: inherit; text-decoration: none;">orders</a></div>
    <p style="font-size: 0.85rem; color: var(--vp-c-text-2); margin: 8px 0 0;">450,000 rows · 5 columns · 1 B+ Tree index</p>
  </div>

  <div class="stat-card">
    <div class="stat-card-label">Identity Module</div>
    <div class="stat-card-val"><a href="/tables/users" style="color: inherit; text-decoration: none;">users</a></div>
    <p style="font-size: 0.85rem; color: var(--vp-c-text-2); margin: 8px 0 0;">120,000 rows · 5 columns (PII protected)</p>
  </div>

  <div class="stat-card">
    <div class="stat-card-label">Inventory Module</div>
    <div class="stat-card-val"><a href="/tables/products" style="color: inherit; text-decoration: none;">products</a></div>
    <p style="font-size: 0.85rem; color: var(--vp-c-text-2); margin: 8px 0 0;">8,500 rows · 5 columns · Fast SKU point reads</p>
  </div>
</div>

---

## Schema Inspection via CLI

You can inspect the catalog schema directly in the terminal:

```sql
quartz> .tables
+--------------------+
| Tables in quartz   |
+--------------------+
| orders             |
| users              |
| products           |
| line_items         |
+--------------------+

quartz> .schema orders
+-------------+---------+----------+-------------+
| Column      | Type    | Nullable | Primary Key |
+-------------+---------+----------+-------------+
| id          | INT64   | NO       | YES         |
| user_id     | INT64   | NO       | NO          |
| total_cents | INT64   | NO       | NO          |
| status      | STRING  | NO       | NO          |
| placed_at   | INT64   | NO       | NO          |
+-------------+---------+----------+-------------+
```
