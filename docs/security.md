# Security, Roles & PII Map 🔒

QuartzDB provides role-based access patterns, data governance guidelines, and a comprehensive Personally Identifiable Information (PII) map.

> [!NOTE] Example Schema — Not Engine-Specific
> This page describes the **example sample schema** shipped with QuartzDB for demonstration purposes (`users`, `orders`, `products`). QuartzDB itself does not enforce any predefined business schema, roles, or PII policy. These tables exist solely to illustrate engine capabilities.

---

## 1. Role-Based Access Control (RBAC)

Because QuartzDB runs as an embedded engine within the host application process, authentication and authorization are validated at the API and executor boundary:

| Role Name | Table DDL (`CREATE` / `DROP`) | Read Operations (`SELECT`) | Write Operations (`INSERT` / `UPDATE` / `DELETE`) | Administrative (.open, .drop) |
| :--- | :---: | :---: | :---: | :---: |
| **`Admin`** | ✅ | ✅ | ✅ | ✅ |
| **`AppService`** | ❌ | ✅ | ✅ | ❌ |
| **`Auditor`** | ❌ | ✅ (Read-Only) | ❌ | ❌ |
| **`Restricted`** | ❌ | ⚠️ Non-PII Only | ❌ | ❌ |

---

## 2. Personally Identifiable Information (PII) Map

Attributes marked <span class="badge-pii">PII ⚠️</span> contain personal customer data subject to global compliance regulations (GDPR, CCPA, SOC2).

| Table | Column | PII Classification | Sensitivity | Retention Policy | Compliance Requirement |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **`users`** | `email` | Direct Identifier | High (Tier 1) | Account Lifetime + 30 Days | Encrypted at rest; purge upon deletion. |
| **`users`** | `full_name` | Direct Identifier | Medium (Tier 2) | Account Lifetime + 30 Days | Purge upon GDPR deletion request. |

### GDPR "Right to be Forgotten" Workflow
When a customer requests account deletion:
```sql
-- 1. Remove customer identity from users table
DELETE FROM users WHERE id = 1042;

-- 2. Trigger slotted page compaction to overwrite physical bytes
-- The engine reclaims slot gaps and compacts the 4KB data page.
```

---

## 3. Storage Security & File Permissions

- **Operating System File Permissions**: Database `.db` files should be provisioned with POSIX `0600` permissions (read/write only by the application service owner).
- **Embedded Boundary**: No open TCP sockets or network listening ports exist, eliminating remote network injection and port scanning vectors.
