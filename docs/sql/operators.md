# Operators & Expression Evaluation ⚙️

QuartzDB provides an expression evaluation engine supporting relational comparisons, logical composition, and operator precedence hierarchies within `WHERE` filters and `JOIN` predicates.

---

## Relational Comparison Operators

All comparison operators operate on matching data types (`INT` vs `INT`, `TEXT` vs `TEXT`):

| Operator | Semantic Description | Type Support | Example Query |
| :---: | :--- | :--- | :--- |
| **`=`** | Exact equality | `INT`, `TEXT` | `SELECT * FROM users WHERE id = 42;` |
| **`!=`** | Inequality comparison | `INT`, `TEXT` | `SELECT * FROM orders WHERE status != 'CANCELLED';` |
| **`<`** | Strictly less than | `INT`, `TEXT` (lexicographic) | `SELECT * FROM products WHERE price_cents < 2500;` |
| **`<=`** | Less than or equal to | `INT`, `TEXT` (lexicographic) | `SELECT * FROM users WHERE age <= 65;` |
| **`>`** | Strictly greater than | `INT`, `TEXT` (lexicographic) | `SELECT * FROM orders WHERE id > 1000;` |
| **`>=`** | Greater than or equal to | `INT`, `TEXT` (lexicographic) | `SELECT * FROM logs WHERE timestamp >= 1700000000;` |

> [!NOTE]
> When equality (`=`) is evaluated against the table's `PRIMARY KEY` (e.g. `WHERE id = 42`), the query planner selects the **$O(\log N)$ B+ Tree fast-path** instead of executing a full table scan.

---

## Boolean Logical Operators

Logical operators combine multiple relational predicates into complex filter expressions:

| Operator | Precedence | Associativity | Example |
| :---: | :---: | :---: | :--- |
| **`NOT`** | 1 (Highest) | Right-to-Left | `WHERE NOT status = 'ARCHIVED'` |
| **`AND`** | 2 (Medium) | Left-to-Right | `WHERE active = 1 AND score >= 90` |
| **`OR`** | 3 (Lowest) | Left-to-Right | `WHERE role = 'admin' OR role = 'staff'` |

### Operator Precedence & Grouping
Because `AND` has higher precedence than `OR`, parentheses `(` and `)` are required to group alternate predicates:

```sql
-- Evaluates as: (dept = 'Engineering') OR (dept = 'Research' AND level >= 5)
SELECT * FROM employees 
WHERE dept = 'Engineering' OR dept = 'Research' AND level >= 5;

-- Explicit grouping forces OR evaluation first:
SELECT * FROM employees 
WHERE (dept = 'Engineering' OR dept = 'Research') AND level >= 5;
```

---

## Three-Valued Logic (`NULL` Handling)

When comparing against nullable columns:
- Comparing any value with `NULL` using `=`, `!=`, `<`, or `>` yields `UNKNOWN` (falsy in `WHERE` filtering).
- To check for nullability, standard ANSI SQL patterns should ensure columns are populated or declared `NOT NULL`.

---

## Practical Examples

### Combined Filter with Range & Status
```sql
SELECT id, customer_name, total_cents 
FROM orders 
WHERE total_cents >= 10000 
  AND (status = 'CONFIRMED' OR status = 'SHIPPED');
```

### Relational Join with Filter Predicates
```sql
SELECT users.full_name, orders.total_cents, orders.status
FROM users
JOIN orders ON users.id = orders.user_id
WHERE users.active = 1 
  AND orders.status != 'REFUNDED';
```
