---
layout: home

hero:
  name: "QuartzDB"
  text: "Embedded Relational Engine in Modern C++23"
  tagline: "A lightweight, disk-backed SQL database built from scratch with slotted pages, B+ tree indexing, and zero external dependencies."
  actions:
    - theme: brand
      text: Get Started
      link: /guide/what-is-quartz
    - theme: alt
      text: Architecture Deep Dive
      link: /architecture/overview
    - theme: alt
      text: View on GitHub
      link: https://github.com/KAAZamapiloT/QUARTZ_DB

features:
  - icon: 🗄️
    title: Slotted-Page Storage
    details: Low-level 4KB page allocator with slot arrays growing forward and dynamic record payloads growing backward for minimal fragmentation.
  - icon: 🌲
    title: Disk-Backed B+ Tree
    details: Fully persistent multi-level B+ tree index with automatic leaf/internal node splitting, root promotion, and O(log N) primary-key lookups.
  - icon: ⚡
    title: Handcrafted SQL Pipeline
    details: Zero-dependency recursive-descent AST parser and custom lexer powering SELECT, INSERT, UPDATE, DELETE, and relational JOINs.
  - icon: 💻
    title: Production REPL CLI
    details: Interactive command-line shell with multi-database support, dot commands (.schema, .tables, .open), and formatted table visualization.
  - icon: 🚀
    title: High Sustained Throughput
    details: Benchmark-proven 3,000+ QPS across 120,000+ records with sub-millisecond point queries on a compact 12MB disk footprint.
  - icon: 🧩
    title: Clean Modern C++23
    details: Strict schema validation, std::variant dynamic values, and strong type safety designed for educational depth and embeddability.
---

<div style="margin-top: 3rem; text-align: center;">
  <h2>Quick Terminal Preview</h2>
</div>

```sql
quartz> CREATE DATABASE shop;
Database 'shop.db' created and active.

quartz> CREATE TABLE users (id INT PRIMARY KEY, name TEXT NOT NULL, email TEXT);
Table 'users' created successfully.

quartz> INSERT INTO users VALUES (1, 'Alice', 'alice@example.com');
Query OK, 1 row affected.

quartz> SELECT * FROM users WHERE id = 1;
+----+-------+-------------------+
| id | name  | email             |
+----+-------+-------------------+
| 1  | Alice | alice@example.com |
+----+-------+-------------------+
1 row in set (0.000142 sec)
```
