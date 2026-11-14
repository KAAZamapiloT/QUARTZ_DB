# users

*Represents registered user identities, authentication references, and authorization profiles.*  
**Last updated:** `2026-11-13` · **Owner:** `identity-security`

> [!NOTE] Example Schema — Not Engine-Specific
> This page describes the **example sample schema** shipped with QuartzDB for demonstration purposes (`users`, `orders`, `products`). QuartzDB itself does not enforce any predefined business schema, roles, or PII policy. These tables exist solely to illustrate engine capabilities.

---

<!-- Stat Card Wireframe Banner -->
<div class="stat-card-row">
  <div class="stat-card">
    <div class="stat-card-label">Estimated Rows</div>
    <div class="stat-card-val">📊 120,000</div>
  </div>
  <div class="stat-card">
    <div class="stat-card-label">Disk Footprint</div>
    <div class="stat-card-val">📅 est. 12.1 MB</div>
  </div>
  <div class="stat-card">
    <div class="stat-card-label">4KB Pages Allocated</div>
    <div class="stat-card-val">📄 3,094 pgs</div>
  </div>
  <div class="stat-card">
    <div class="stat-card-label">Data Sensitivity</div>
    <div class="stat-card-val">⚠️ PII Tier 1</div>
  </div>
</div>

---

## Columns

| Column Name | Data Type | Nullable | Badges | Description |
| :--- | :--- | :--- | :--- | :--- |
| <span class="click-copy" onclick="navigator.clipboard.writeText('id')">`id`</span> | `INT64` | `NO` | <span class="badge-pk">PK 🔑</span> | Unique 64-bit integer identifier and B+ Tree primary key. |
| <span class="click-copy" onclick="navigator.clipboard.writeText('email')">`email`</span> | `TEXT` | `NO` | <span class="badge-pii">PII ⚠️</span> | Login email address. Subject to GDPR Right to be Forgotten. |
| <span class="click-copy" onclick="navigator.clipboard.writeText('full_name')">`full_name`</span> | `TEXT` | `NO` | <span class="badge-pii">PII ⚠️</span> | Legal personal customer name for billing and shipping. |
| <span class="click-copy" onclick="navigator.clipboard.writeText('created_at')">`created_at`</span> | `INT64` | `NO` | — | Epoch millisecond timestamp of user registration. |
| <span class="click-copy" onclick="navigator.clipboard.writeText('role')">`role`</span> | `TEXT` | `YES` | <span class="badge-null">NULL</span> | Authorization tier: `admin`, `engineer`, `customer`. |

> [!CAUTION]
> Columns marked <span class="badge-pii">PII ⚠️</span> contain Personally Identifiable Information. Refer to the [Security & PII Map](/security) for compliance and access policies.

---

## Keys & Constraints

- **Primary Key Constraint**: Unique primary key on `id`. Enforced by `PrimaryIndex` B+ Tree index.
- **NotNull Constraint**: `email`, `full_name`, and `created_at` reject `NULL` insertions.
- **Nullable Constraint**: `role` allows `NULL` (`std::monostate`), defaulting to standard user privileges.

---

## Indexes & Access Paths

| Index Name | Column | Structure | Latency | Access Method |
| :--- | :--- | :--- | :--- | :--- |
| **`idx_users_pk`** | `id` | Disk B+ Tree | ~100 μs | Point reads (`WHERE id = ?`). Direct slot fetch without page scans. |
| **Sequential Scan** | `email`, `role` | Slotted Pages | ~450 μs / 1k rows | Slotted page linear scan with `RecordCodec::deserialize`. |

---

## Related Tables

- **Child Table**: [**`orders`**](/tables/orders) via `orders.user_id = users.id` (1:N relationship).

```sql
-- Query customer with order history
SELECT users.full_name, orders.id AS order_id, orders.total_cents
FROM users
JOIN orders ON users.id = orders.user_id
WHERE users.id = 1;
```
