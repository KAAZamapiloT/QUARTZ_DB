import fs from 'fs';

const replacements = [
  {
    file: 'docs/diagrams.md',
    name: 'erd-physical-page-layout',
    title: 'Physical Page Layout ERD (Catalog, Slotted Pages & B+ Tree Blocks)',
    alt: 'Entity relationship diagram of QuartzDB physical page layout: Catalog Page 0, Table Data Page, B+ Meta Page, B+ Root Page, B+ Internal Page, B+ Leaf Page, and Slotted Page Record'
  },
  {
    file: 'docs/operations.md',
    name: 'operations-compaction-runbook',
    title: 'Storage Compaction Runbook Flow',
    alt: 'Six-stage offline storage compaction runbook flow: Stop App, Scan DB, Rebuild Data, Re-index B+ Tree, Rename File, and Verify Reopen'
  },
  {
    file: 'docs/architecture/bplus-tree.md',
    name: 'bplus-tree-topology',
    title: 'Complete B+ Tree Topology (Metadata, Internal Routing & Leaf Chain)',
    alt: 'Three-level on-disk B+ Tree hierarchy showing Metadata Page 2, Root Page 3, Internal routing pages, Leaf nodes 10-13, and horizontal next_leaf chaining'
  },
  {
    file: 'docs/architecture/bplus-tree.md',
    name: 'bplus-tree-point-lookup',
    title: 'Point Lookup Traversal (find(key) O(log N))',
    alt: 'Binary search routing workflow descending from B+ Tree root page through internal nodes to leaf node and resolving RecordID'
  },
  {
    file: 'docs/architecture/bplus-tree.md',
    name: 'bplus-tree-node-split',
    title: 'Node Splitting and Promotion Sequence (allocate_page & next_leaf Re-link)',
    alt: 'Sequence diagram showing leaf node splitting at index 64 upon exceeding MAX_KEYS (128), allocating Page B, chaining next_leaf, and promoting separator to parent'
  },
  {
    file: 'docs/architecture/overview.md',
    name: 'overview-system-architecture',
    title: 'Complete End-to-End System Architecture',
    alt: 'End-to-end multi-tier architecture of QuartzDB: Client Layer, Query Compiler Engine, Relational Execution Engine, Paged Storage Subsystem, and OS File System'
  },
  {
    file: 'docs/architecture/overview.md',
    name: 'overview-dual-query-paths',
    title: 'Dual Query Execution Paths: Index Fast-Path vs Table Scan',
    alt: 'Sequence diagram comparing Path A (B+ Tree O(log N) point lookup) versus Path B (O(N) slotted-page table scan)'
  },
  {
    file: 'docs/architecture/pager.md',
    name: 'pager-physical-partitioning',
    title: 'Physical File Partitioning (Fixed 4,096-Byte Blocks)',
    alt: 'Physical 4096-byte database file block partitioning showing Page 0 System Catalog, Table Data Roots, B+ Tree Meta & Nodes, and Freelist Recycling Pages'
  },
  {
    file: 'docs/architecture/pager.md',
    name: 'pager-allocation-recycling',
    title: 'Page Allocation & Recycling Workflow (allocate_page & deallocate_page)',
    alt: 'Dual workflow diagram showing O(1) freelist recycling via free_pages stack versus file extension on disk, and deallocate_page recycling'
  },
  {
    file: 'docs/architecture/slotted-pages.md',
    name: 'slotted-pages-compaction',
    title: 'Page Fill and Compaction Sequence',
    alt: 'Sequence diagram of compact_page() sorting active slots, packing payloads contiguous to byte 4096, and resetting fragmented_bytes to 0'
  },
  {
    file: 'docs/concepts/architecture.md',
    name: 'concepts-multi-layer',
    title: 'Complete Multi-Layer Architecture Diagram',
    alt: 'Multi-layer architectural model of QuartzDB: SQL Compilation, Relational Execution, Catalog & Table Engine, Physical Paged Structures, and I/O Block Engine'
  },
  {
    file: 'docs/concepts/architecture.md',
    name: 'concepts-query-pipeline',
    title: 'End-to-End Query Execution Pipeline',
    alt: 'Six-stage linear query pipeline from SQL Text through Lexer, Parser, Executor, Storage Engine, and Disk File'
  },
  {
    file: 'docs/concepts/indexing.md',
    name: 'concepts-indexing-topology',
    title: 'Topology & Node Fan-Out (Root, Internal, Leaves, and Slotted Data)',
    alt: 'B+ Tree indexing topology diagram showing Root Node Page 3, Internal Nodes 5 and 6, Leaf Pages 10-13, and RecordID links to Table Pages 1 and 4'
  },
  {
    file: 'docs/guide/what-is-quartz.md',
    name: 'what-is-quartz-architecture',
    title: 'High-Level System Architecture',
    alt: 'High-level architecture of QuartzDB showing Query Processing, Catalog & Table Manager, and Physical Storage Engine'
  },
  {
    file: 'docs/operations/backup-recovery.md',
    name: 'backup-recovery-compaction',
    title: 'Compaction Mechanics & Gap Elimination',
    alt: 'Flowchart of slotted-page compaction triggering on fragmented bytes > 20%, copying active slots, sliding payloads, and resetting fragmentation'
  },
  {
    file: 'docs/sql/aggregations.md',
    name: 'sql-aggregations-workflow',
    title: 'How Aggregations Work Under the Hood in QuartzDB',
    alt: 'Aggregation execution flowchart: WHERE filtering, LIMIT checks, accumulator initialization, and reduction passes for COUNT, SUM, AVG, MIN, and MAX'
  },
  {
    file: 'docs/sql/joins.md',
    name: 'sql-joins-workflow',
    title: 'How JOIN Works Under the Hood in QuartzDB',
    alt: 'Relational JOIN execution flowchart: scanning Table A, evaluating ON predicate, selecting INLJ Fast-Path vs Block Scan, and formatting LEFT/INNER rows'
  },
  {
    file: 'docs/sql/order-by-limit.md',
    name: 'sql-order-by-limit-workflow',
    title: 'How Sorting & Windowing Work in QuartzDB',
    alt: 'Sorting and pagination workflow: evaluating ORDER BY via std::stable_sort, evaluating OFFSET to skip K records, and evaluating LIMIT to truncate buffer'
  },
  {
    file: 'docs/sql/statements/create-table.md',
    name: 'create-table-workflow',
    title: 'How CREATE TABLE Works Under the Hood in QuartzDB',
    alt: 'DDL execution flowchart for CREATE TABLE: validating schema, checking catalog duplicate names, allocating root data page and B+ Tree pages, and persisting catalog Page 0'
  },
  {
    file: 'docs/sql/statements/create-table.md',
    name: 'create-table-allocation-sequence',
    title: 'Allocation Flow Sequence (Page 1 Data Root & Pages 2-3 B+ Tree Index)',
    alt: 'Sequence diagram showing DDL Query Executor, Catalog Page 0, Pager, and PrimaryIndex initializing Page 1, Page 2 Meta, Page 3 Root, and writing to Page 0'
  },
  {
    file: 'docs/sql/statements/delete.md',
    name: 'delete-workflow',
    title: 'How DELETE Works Under the Hood in QuartzDB',
    alt: 'DELETE query execution pipeline: checking fast-path vs scan, slot tombstoning, B+ Tree key erasure, and underflow rebalancing (borrow, merge, root collapse)'
  },
  {
    file: 'docs/sql/statements/drop-table.md',
    name: 'drop-table-workflow',
    title: 'How DROP TABLE Works Under the Hood in QuartzDB',
    alt: 'DROP TABLE teardown flowchart: walking data page chain, walking B+ Tree index pages in post-order DFS, recycling all PageIDs to free_pages stack, and catalog compaction'
  },
  {
    file: 'docs/sql/statements/insert.md',
    name: 'insert-workflow',
    title: 'How INSERT Works Under the Hood in QuartzDB',
    alt: 'INSERT query execution pipeline: schema type validation, primary key uniqueness check, contiguous space check, slot allocation, and B+ Tree index insertion'
  },
  {
    file: 'docs/sql/statements/update.md',
    name: 'update-workflow',
    title: 'How UPDATE Works Under the Hood in QuartzDB',
    alt: 'UPDATE query execution pipeline: immutable primary key guard, point fast-path vs scan, in-place overwrite vs out-of-place re-slotting, and slot_index stability'
  },
  {
    file: 'docs/tables/orders.md',
    name: 'erd-orders-related',
    title: 'Related Tables Entity-Relationship Diagram',
    alt: 'ER diagram showing users table (1:N) orders table (1:N) line_items table with primary and foreign key relations'
  }
];

// Group by file to replace sequentially
const byFile = {};
replacements.forEach(r => {
  if (!byFile[r.file]) byFile[r.file] = [];
  byFile[r.file].push(r);
});

let totalReplaced = 0;
for (const [file, items] of Object.entries(byFile)) {
  let content = fs.readFileSync(file, 'utf8');
  items.forEach(item => {
    const regex = /```mermaid\r?\n[\s\S]*?```/;
    const component = `<ExcalidrawDiagram\n  name="${item.name}"\n  title="${item.title}"\n  alt="${item.alt}"\n/>`;
    if (regex.test(content)) {
      content = content.replace(regex, component);
      totalReplaced++;
      console.log(`✓ Replaced in ${file} -> ${item.name}`);
    } else {
      console.error(`✗ Failed to match mermaid in ${file} for ${item.name}`);
    }
  });
  fs.writeFileSync(file, content, 'utf8');
}

console.log(`\n🎉 Total diagrams replaced: ${totalReplaced} of ${replacements.length}`);
