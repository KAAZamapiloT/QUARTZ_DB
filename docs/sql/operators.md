# Operators & Expressions ⚙️

QuartzDB's execution engine supports standard comparison operators and boolean logical composition.

---

## Comparison Operators

| Operator | Meaning | Example |
| :---: | :--- | :--- |
| `=` | Equality comparison | `WHERE id = 42` |
| `!=` | Inequality comparison | `WHERE status != 'CANCELLED'` |
| `<` | Less than | `WHERE amount < 1000` |
| `<=` | Less than or equal to | `WHERE total_cents <= 5000` |
| `>` | Greater than | `WHERE id > 100` |
| `>=` | Greater than or equal to | `WHERE created_at >= 1700000000000` |

---

## Logical Operators & Precedence

| Operator | Precedence | Example |
| :---: | :---: | :--- |
| `NOT` | High | `WHERE NOT (status = 'DELETED')` |
| `AND` | Medium | `WHERE id > 10 AND status = 'PAID'` |
| `OR` | Low | `WHERE role = 'admin' OR role = 'lead'` |

Parentheses `(` and `)` can be nested to override default operator precedence.
