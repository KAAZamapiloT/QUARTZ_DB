import { DiagramBuilder } from './diagram-renderer.mjs';

export function buildAllDiagrams() {
  const builders = [];

  // =========================================================================
  // 1. erd-physical-page-layout (docs/diagrams.md)
  // =========================================================================
  {
    const d = new DiagramBuilder('erd-physical-page-layout', 'Physical Page Layout ERD', 960, 620);
    
    // Entity Cards
    d.addEntityCard({
      id: 'cat', x: 40, y: 40, w: 260,
      title: 'CATALOG_PAGE_0',
      fields: ['uint32 magic_QDB1 [0x31424451]', 'uint16 version', 'uint16 table_count', 'entries table_registry'],
      type: 'entry'
    });

    d.addEntityCard({
      id: 'tab', x: 350, y: 40, w: 260,
      title: 'TABLE_DATA_PAGE',
      fields: ['uint32 magic_QPG1 [0x31475051]', 'uint16 slot_count', 'uint16 free_start', 'uint16 free_end', 'uint64 next_page'],
      type: 'disk'
    });

    d.addEntityCard({
      id: 'record', x: 670, y: 40, w: 250,
      title: 'SLOTTED_PAGE_RECORD',
      fields: ['RecordID {page_id, slot_index}', 'uint16 offset', 'uint16 length', 'bytes serialized_payload'],
      type: 'success'
    });

    d.addEntityCard({
      id: 'meta', x: 40, y: 240, w: 260,
      title: 'BPLUS_META_PAGE',
      fields: ['uint32 magic_BPM1', 'uint64 root_page_id', 'uint32 total_keys', 'uint16 tree_height'],
      type: 'shared'
    });

    d.addEntityCard({
      id: 'root', x: 350, y: 240, w: 260,
      title: 'BPLUS_ROOT_PAGE',
      fields: ['uint32 magic_BPT1', 'uint8 type (INTERNAL=2)', 'uint16 key_count', 'PageID child_page_pointers[]'],
      type: 'entry'
    });

    d.addEntityCard({
      id: 'internal', x: 40, y: 430, w: 260,
      title: 'BPLUS_INTERNAL_PAGE',
      fields: ['uint32 magic_BPT1', 'uint8 type (INTERNAL=2)', 'uint16 key_count', 'PageID child_page_pointers[]'],
      type: 'disk'
    });

    d.addEntityCard({
      id: 'leaf', x: 350, y: 430, w: 260,
      title: 'BPLUS_LEAF_PAGE',
      fields: ['uint32 magic_BPT1', 'uint8 type (LEAF=1)', 'uint64 next_leaf', 'entries key_and_record_id[]'],
      type: 'success'
    });

    // Relationships
    d.addArrow({ points: [[300, 100], [350, 100]], label: 'first_page pointer (1:N)' });
    d.addArrow({ points: [[610, 100], [670, 100]], label: 'slot directory' });
    d.addArrow({ points: [[170, 180], [170, 240]], label: 'index_meta (1:1)' });
    d.addArrow({ points: [[300, 300], [350, 300]], label: 'root_page pointer (1:1)' });
    d.addArrow({ points: [[480, 380], [170, 430]], label: 'child pointers (1:N)' });
    d.addArrow({ points: [[300, 490], [350, 490]], label: 'child pointers (1:N)' });
    d.addArrow({ points: [[610, 490], [795, 490], [795, 180]], label: 'RecordID {P, S}' });
    d.addArrow({ points: [[480, 180], [480, 210], [580, 210], [580, 180]], label: 'next_page list', dashed: true });

    builders.push(d);
  }

  // =========================================================================
  // 2. operations-compaction-runbook (docs/operations.md)
  // =========================================================================
  {
    const d = new DiagramBuilder('operations-compaction-runbook', 'Storage Compaction Runbook Flow', 900, 220);

    d.addBox({ id: 's1', x: 20, y: 70, w: 130, h: 75, title: '1. Stop App', subtitle: 'Verify Quiescence', type: 'entry' });
    d.addBox({ id: 's2', x: 175, y: 70, w: 130, h: 75, title: '2. Scan DB', subtitle: 'Find fragmented > 0', type: 'disk' });
    d.addBox({ id: 's3', x: 330, y: 70, w: 135, h: 75, title: '3. Rebuild Data', subtitle: 'Pack active slots', type: 'shared' });
    d.addBox({ id: 's4', x: 490, y: 70, w: 135, h: 75, title: '4. Re-index BPT', subtitle: 'Reconstruct tree', type: 'shared' });
    d.addBox({ id: 's5', x: 650, y: 70, w: 125, h: 75, title: '5. Rename File', subtitle: 'Atomic file swap', type: 'disk' });
    d.addBox({ id: 's6', x: 800, y: 70, w: 85, h: 75, title: '6. Verify', subtitle: 'Reopen DB', type: 'success' });

    d.addArrow({ points: [[150, 107], [175, 107]] });
    d.addArrow({ points: [[305, 107], [330, 107]] });
    d.addArrow({ points: [[465, 107], [490, 107]] });
    d.addArrow({ points: [[625, 107], [650, 107]] });
    d.addArrow({ points: [[775, 107], [800, 107]] });

    builders.push(d);
  }

  // =========================================================================
  // 3. bplus-tree-topology (docs/architecture/bplus-tree.md)
  // =========================================================================
  {
    const d = new DiagramBuilder('bplus-tree-topology', 'Complete B+ Tree Topology', 940, 480);

    d.addBox({ id: 'meta', x: 30, y: 40, w: 180, h: 70, title: 'Metadata Page 2', subtitle: 'Magic: BPM1 · Root: Page 3', type: 'shared' });

    d.addContainer({ id: 'hier', x: 240, y: 20, w: 680, h: 440, title: 'On-Disk Multi-Level Tree Hierarchy (4096-Byte Pages)' });

    d.addBox({ id: 'root', x: 470, y: 50, w: 220, h: 75, title: 'Root Page 3 (Internal)', subtitle: 'Separator: [ 100 ] · Fan-out ≤ 128', type: 'entry' });

    d.addBox({ id: 'intL', x: 330, y: 175, w: 200, h: 70, title: 'Internal Page 5', subtitle: 'Keys < 100 · Sep: [ 50 ]', type: 'disk' });
    d.addBox({ id: 'intR', x: 630, y: 175, w: 200, h: 70, title: 'Internal Page 6', subtitle: 'Keys ≥ 100 · Sep: [ 150 ]', type: 'disk' });

    d.addBox({ id: 'l1', x: 260, y: 310, w: 135, h: 80, title: 'Leaf Page 10', subtitle: 'Keys: [ 10 .. 49 ]\nPayload: RecordID', type: 'success' });
    d.addBox({ id: 'l2', x: 420, y: 310, w: 135, h: 80, title: 'Leaf Page 11', subtitle: 'Keys: [ 50 .. 99 ]\nPayload: RecordID', type: 'success' });
    d.addBox({ id: 'l3', x: 580, y: 310, w: 135, h: 80, title: 'Leaf Page 12', subtitle: 'Keys: [ 100 .. 149 ]\nPayload: RecordID', type: 'success' });
    d.addBox({ id: 'l4', x: 740, y: 310, w: 135, h: 80, title: 'Leaf Page 13', subtitle: 'Keys: [ 150 .. 200 ]\nPayload: RecordID', type: 'success' });

    d.addBox({ id: 'term', x: 740, y: 410, w: 135, h: 36, title: 'End of Index (NO_PAGE)', subtitle: '', type: 'guard' });

    // Connections
    d.addArrow({ points: [[210, 75], [470, 75]], label: 'root_page_id', dashed: true });
    d.addArrow({ points: [[520, 125], [440, 175]], label: 'Child 0: key < 100' });
    d.addArrow({ points: [[640, 125], [720, 175]], label: 'Child 1: key ≥ 100' });

    d.addArrow({ points: [[380, 245], [330, 310]], label: 'key < 50' });
    d.addArrow({ points: [[470, 245], [490, 310]], label: 'key ≥ 50' });
    d.addArrow({ points: [[680, 245], [650, 310]], label: 'key < 150' });
    d.addArrow({ points: [[770, 245], [800, 310]], label: 'key ≥ 150' });

    // Leaf horizontal chain
    d.addArrow({ points: [[395, 350], [420, 350]], label: 'next_leaf' });
    d.addArrow({ points: [[555, 350], [580, 350]], label: 'next_leaf' });
    d.addArrow({ points: [[715, 350], [740, 350]], label: 'next_leaf' });
    d.addArrow({ points: [[807, 390], [807, 410]], dashed: true });

    builders.push(d);
  }

  // =========================================================================
  // 4. bplus-tree-point-lookup (docs/architecture/bplus-tree.md)
  // =========================================================================
  {
    const d = new DiagramBuilder('bplus-tree-point-lookup', 'Point Lookup Traversal (find)', 880, 480);

    d.addBox({ id: 's1', x: 40, y: 40, w: 180, h: 65, title: '1. Call find(key)', subtitle: 'O(log N) Point Query', type: 'entry' });
    d.addBox({ id: 's2', x: 260, y: 40, w: 200, h: 65, title: '2. Read root_page_', subtitle: 'Pager::read_page(root)', type: 'disk' });
    d.addDiamond({ id: 'd1', x: 500, y: 35, w: 170, h: 75, text: 'Node Type == LEAF?', type: 'guard' });

    // Internal path (No)
    d.addBox({ id: 'int_bin', x: 690, y: 150, w: 180, h: 65, title: '3a. Binary Search Keys', subtitle: 'Find separator index', type: 'entry' });
    d.addBox({ id: 'int_fetch', x: 690, y: 260, w: 180, h: 65, title: '4a. Pager::read_page()', subtitle: 'Follow downward pointer', type: 'disk' });

    // Leaf path (Yes)
    d.addBox({ id: 'leaf_bin', x: 495, y: 170, w: 180, h: 65, title: '3b. Binary Search Leaf', subtitle: 'Find exact key match', type: 'entry' });
    d.addDiamond({ id: 'd2', x: 500, y: 270, w: 170, h: 75, text: 'Key exists in Leaf?', type: 'guard' });

    d.addBox({ id: 'ok', x: 370, y: 380, w: 170, h: 65, title: '4b. Return RecordID', subtitle: '{page_id, slot_index}', type: 'success' });
    d.addBox({ id: 'err', x: 630, y: 380, w: 170, h: 65, title: '4c. Return std::nullopt', subtitle: 'Record Not Found', type: 'error' });

    d.addArrow({ points: [[220, 72], [260, 72]] });
    d.addArrow({ points: [[460, 72], [500, 72]] });

    d.addArrow({ points: [[670, 72], [780, 72], [780, 150]], label: 'No (Internal)' });
    d.addArrow({ points: [[780, 215], [780, 260]] });
    d.addArrow({ points: [[690, 292], [470, 292], [470, 72]], label: 'Loop down' });

    d.addArrow({ points: [[585, 110], [585, 170]], label: 'Yes (Leaf)' });
    d.addArrow({ points: [[585, 235], [585, 270]] });
    d.addArrow({ points: [[530, 335], [455, 380]], label: 'Yes (Match)' });
    d.addArrow({ points: [[640, 335], [715, 380]], label: 'No (Missing)' });

    builders.push(d);
  }

  // =========================================================================
  // 5. bplus-tree-node-split (docs/architecture/bplus-tree.md)
  // =========================================================================
  {
    const d = new DiagramBuilder('bplus-tree-node-split', 'Node Splitting and Promotion Sequence', 880, 420);

    const xL = 120, xR = 360, xP = 600, xPg = 800;
    d.addLifeline({ id: 'l_leaf', x: xL, topY: 30, height: 350, name: 'Left Leaf', role: 'Page A (Full)' });
    d.addLifeline({ id: 'l_right', x: xR, topY: 30, height: 350, name: 'New Right Leaf', role: 'Page B (Allocated)' });
    d.addLifeline({ id: 'l_parent', x: xP, topY: 30, height: 350, name: 'Parent Node', role: 'Internal Page' });
    d.addLifeline({ id: 'l_pager', x: xPg, topY: 30, height: 350, name: 'Pager', role: 'qdb::pager' });

    d.addSequenceNote({ x: 30, y: 90, w: 180, h: 32, text: 'Leaf has 129 keys (>MAX_KEYS)', type: 'error' });
    d.addSequenceMessage({ fromX: xL, toX: xPg, y: 140, num: 1, text: 'allocate_page() -> Page B' });
    d.addSequenceMessage({ fromX: xPg, toX: xR, y: 175, num: 2, text: 'Initialize Page B Header', dashed: true });

    d.addSequenceNote({ x: 190, y: 205, w: 220, h: 32, text: 'Split keys at middle (index 64)', type: 'guard' });
    d.addSequenceMessage({ fromX: xL, toX: xR, y: 255, num: 3, text: 'Right.next_leaf = Leaf.next_leaf' });
    d.addSequenceMessage({ fromX: xL, toX: xL + 40, y: 290, num: 4, text: 'Leaf.next_leaf = Page B' });
    d.addSequenceMessage({ fromX: xL, toX: xP, y: 330, num: 5, text: 'Push Separator Key (middle) & Page B' });

    d.addSequenceNote({ x: 480, y: 365, w: 240, h: 32, text: 'alt: Parent splits recursively / new root', type: 'shared' });

    builders.push(d);
  }

  // =========================================================================
  // 6. overview-system-architecture (docs/architecture/overview.md)
  // =========================================================================
  {
    const d = new DiagramBuilder('overview-system-architecture', 'Complete End-to-End System Architecture', 960, 680);

    d.addContainer({ id: 'c1', x: 30, y: 30, w: 260, h: 120, title: '1. Client Interface Layer' });
    d.addBox({ id: 'sql_in', x: 50, y: 70, w: 100, h: 55, title: 'Raw SQL', subtitle: 'Query String', type: 'entry' });
    d.addBox({ id: 'cpp_in', x: 165, y: 70, w: 110, h: 55, title: 'C++ API', subtitle: 'qdb::Database', type: 'entry' });

    d.addContainer({ id: 'c2', x: 320, y: 30, w: 320, h: 120, title: '2. Query Compiler Engine' });
    d.addBox({ id: 'lex', x: 340, y: 70, w: 130, h: 55, title: 'qdb::Lexer', subtitle: 'Token Stream', type: 'entry' });
    d.addBox({ id: 'parse', x: 490, y: 70, w: 130, h: 55, title: 'qdb::Parser', subtitle: 'AST Statement Nodes', type: 'entry' });

    d.addContainer({ id: 'c3', x: 660, y: 30, w: 270, h: 120, title: '3. Relational Execution Engine' });
    d.addBox({ id: 'exec', x: 680, y: 70, w: 230, h: 55, title: 'qdb::Executor', subtitle: 'Relational Operators & Plan Routing', type: 'guard' });

    d.addContainer({ id: 'c4', x: 30, y: 190, w: 900, h: 300, title: '4. Paged Storage Subsystem' });
    d.addBox({ id: 'cat', x: 60, y: 240, w: 220, h: 65, title: 'System Catalog (Page 0)', subtitle: 'Tables registry & column schemas', type: 'shared' });
    d.addBox({ id: 'tab', x: 320, y: 240, w: 240, h: 65, title: 'Table Engine (qdb::Table)', subtitle: 'Slotted-page scan & tuple codec', type: 'guard' });
    d.addBox({ id: 'idx', x: 600, y: 240, w: 300, h: 65, title: 'Primary Index (qdb::PrimaryIndex)', subtitle: 'B+ Tree key to RecordID mapping', type: 'guard' });

    d.addBox({ id: 'slotted', x: 260, y: 345, w: 200, h: 60, title: 'Slotted Pages (4KB)', subtitle: 'Tombstones & gap tracking', type: 'disk' });
    d.addBox({ id: 'bpt', x: 500, y: 345, w: 200, h: 60, title: 'Disk B+ Tree', subtitle: 'Leaf chain & node rebalance', type: 'disk' });
    d.addBox({ id: 'pager', x: 380, y: 435, w: 200, h: 45, title: 'qdb::Pager', subtitle: '4KB block I/O & freelist', type: 'entry' });

    d.addContainer({ id: 'c5', x: 250, y: 520, w: 460, h: 120, title: '5. Operating System File System' });
    d.addCylinder({ id: 'disk_f', x: 350, y: 555, w: 260, h: 65, title: 'Unified Database File (.db)', subtitle: 'Contiguous 4096-Byte Page Frames', type: 'disk' });

    // Arrows
    d.addArrow({ points: [[150, 97], [340, 97]] });
    d.addArrow({ points: [[470, 97], [490, 97]] });
    d.addArrow({ points: [[620, 97], [680, 97]] });
    d.addArrow({ points: [[275, 110], [680, 110]], dashed: true });

    d.addArrow({ points: [[795, 125], [795, 240]], label: 'Execute plan' });
    d.addArrow({ points: [[680, 272], [280, 272]], label: 'Catalog lookup' });
    d.addArrow({ points: [[750, 240], [440, 240]], label: 'Table Scan / Modify' });

    d.addArrow({ points: [[420, 305], [360, 345]] });
    d.addArrow({ points: [[750, 305], [600, 345]] });
    d.addArrow({ points: [[360, 405], [450, 435]] });
    d.addArrow({ points: [[600, 405], [510, 435]] });
    d.addArrow({ points: [[170, 305], [380, 455]], label: 'Catalog sync' });

    d.addArrow({ points: [[480, 480], [480, 555]], label: 'Direct read/write seek' });

    builders.push(d);
  }

  // =========================================================================
  // 7. overview-dual-query-paths (docs/architecture/overview.md)
  // =========================================================================
  {
    const d = new DiagramBuilder('overview-dual-query-paths', 'Dual Query Execution Paths', 960, 520);

    const xU = 80, xEx = 220, xIdx = 380, xBpt = 530, xTab = 680, xPg = 820;
    d.addLifeline({ id: 'u', x: xU, topY: 30, height: 460, name: 'Client', role: 'SQL Query' });
    d.addLifeline({ id: 'ex', x: xEx, topY: 30, height: 460, name: 'Executor', role: 'Planner' });
    d.addLifeline({ id: 'idx', x: xIdx, topY: 30, height: 460, name: 'PrimaryIndex', role: 'Key Resolver' });
    d.addLifeline({ id: 'bpt', x: xBpt, topY: 30, height: 460, name: 'B+ Tree', role: 'O(log N)' });
    d.addLifeline({ id: 'tab', x: xTab, topY: 30, height: 460, name: 'Table', role: 'Slotted Page' });
    d.addLifeline({ id: 'pg', x: xPg, topY: 30, height: 460, name: 'Pager', role: 'Disk File' });

    // Path A: Index Fast-Path
    d.addSequenceNote({ x: 250, y: 80, w: 320, h: 28, text: 'Path A: Primary Key Fast-Path (WHERE id = 42)', type: 'success' });
    d.addSequenceMessage({ fromX: xU, toX: xEx, y: 125, num: 1, text: 'SELECT * WHERE id = 42' });
    d.addSequenceMessage({ fromX: xEx, toX: xIdx, y: 155, num: 2, text: 'find_by_primary_key(42)' });
    d.addSequenceMessage({ fromX: xIdx, toX: xBpt, y: 185, num: 3, text: 'find(42)' });
    d.addSequenceMessage({ fromX: xBpt, toX: xPg, y: 215, num: 4, text: 'read_page(root & leaf)' });
    d.addSequenceMessage({ fromX: xBpt, toX: xIdx, y: 245, num: 5, text: 'Return RecordID {Page 14, Slot 3}', dashed: true });
    d.addSequenceMessage({ fromX: xEx, toX: xTab, y: 275, num: 6, text: 'get_record({Page 14, Slot 3})' });
    d.addSequenceMessage({ fromX: xEx, toX: xU, y: 305, num: 7, text: 'QueryResult: 1 row', dashed: true, type: 'success' });

    // Path B: Full Table Scan
    d.addSequenceNote({ x: 250, y: 345, w: 320, h: 28, text: 'Path B: Full Table Scan (WHERE status = "PAID")', type: 'disk' });
    d.addSequenceMessage({ fromX: xU, toX: xEx, y: 390, num: 8, text: 'SELECT * WHERE status = "PAID"' });
    d.addSequenceMessage({ fromX: xEx, toX: xTab, y: 420, num: 9, text: 'scan_table() (Iterate slotted page chain)' });
    d.addSequenceMessage({ fromX: xTab, toX: xPg, y: 450, num: 10, text: 'read_page(chain_id)' });
    d.addSequenceMessage({ fromX: xEx, toX: xU, y: 480, num: 11, text: 'QueryResult: N rows', dashed: true });

    builders.push(d);
  }

  // =========================================================================
  // 8. pager-physical-partitioning (docs/architecture/pager.md)
  // =========================================================================
  {
    const d = new DiagramBuilder('pager-physical-partitioning', 'Physical File Partitioning (Fixed 4,096-Byte Blocks)', 960, 480);

    d.addContainer({ id: 'c1', x: 30, y: 30, w: 900, h: 120, title: 'Base Allocations (Pages 0 - 2)' });
    d.addBox({ id: 'p0', x: 50, y: 65, w: 260, h: 70, title: 'Page 0: System Catalog', subtitle: 'Offset: 0x0000 · Magic: QDB1\nTables registry & schema root', type: 'entry' });
    d.addBox({ id: 'p1', x: 350, y: 65, w: 260, h: 70, title: 'Page 1: Table Data Root', subtitle: 'Offset: 0x1000 · Magic: QPG1\nFirst slotted page of Table', type: 'disk' });
    d.addBox({ id: 'p2', x: 650, y: 65, w: 260, h: 70, title: 'Page 2: B+ Tree Meta', subtitle: 'Offset: 0x2000 · Magic: BPM1\nIndex root pointer & fan-out', type: 'shared' });

    d.addContainer({ id: 'c2', x: 30, y: 175, w: 900, h: 120, title: 'Index & Data Expansion (Pages 3 - 5)' });
    d.addBox({ id: 'p3', x: 50, y: 210, w: 260, h: 70, title: 'Page 3: B+ Tree Root Node', subtitle: 'Offset: 0x3000 · Magic: BPT1\nRouting keys & child pointers', type: 'shared' });
    d.addBox({ id: 'p4', x: 350, y: 210, w: 260, h: 70, title: 'Page 4: Next Table Page', subtitle: 'Offset: 0x4000 · Linked next_page\nSlotted records & compaction', type: 'disk' });
    d.addBox({ id: 'p5', x: 650, y: 210, w: 260, h: 70, title: 'Page 5: B+ Tree Leaf Node', subtitle: 'Offset: 0x5000 · Sorted keys\nChained via next_leaf', type: 'success' });

    d.addContainer({ id: 'c3', x: 30, y: 320, w: 900, h: 120, title: 'Freelist Recycling Pool (Pages 6+)' });
    d.addBox({ id: 'p6', x: 50, y: 355, w: 260, h: 70, title: 'Page 6: Recycled Page', subtitle: 'Offset: 0x6000 · IN FREELIST\nReady for zero-cost reuse', type: 'guard' });
    d.addBox({ id: 'pn', x: 350, y: 355, w: 260, h: 70, title: 'Page N: File Extension', subtitle: 'Offset: N * 4096 Bytes\nDynamically appended on demand', type: 'disk' });

    d.addArrow({ points: [[310, 100], [350, 100]] });
    d.addArrow({ points: [[610, 100], [650, 100]] });
    d.addArrow({ points: [[780, 135], [780, 175], [180, 175], [180, 210]] });
    d.addArrow({ points: [[310, 245], [350, 245]] });
    d.addArrow({ points: [[610, 245], [650, 245]] });
    d.addArrow({ points: [[780, 280], [780, 320], [180, 320], [180, 355]], dashed: true });
    d.addArrow({ points: [[310, 390], [350, 390]], dashed: true });

    builders.push(d);
  }

  // =========================================================================
  // 9. pager-allocation-recycling (docs/architecture/pager.md)
  // =========================================================================
  {
    const d = new DiagramBuilder('pager-allocation-recycling', 'Page Allocation & Recycling Workflow', 940, 520);

    // Central Shared Resource
    d.addCylinder({ id: 'pool', x: 370, y: 30, w: 200, h: 75, title: 'Freelist LIFO Stack', subtitle: 'std::vector<PageID> free_pages', type: 'shared' });

    // Allocation Subgraph
    d.addContainer({ id: 'alloc_c', x: 30, y: 130, w: 420, h: 360, title: 'Allocation Workflow: allocate_page()' });
    d.addBox({ id: 'a_req', x: 130, y: 165, w: 220, h: 55, title: '1. Request Page', subtitle: 'allocate_page() called', type: 'entry' });
    d.addDiamond({ id: 'a_chk', x: 150, y: 240, w: 180, h: 65, text: 'free_pages empty?', type: 'guard' });
    d.addBox({ id: 'a_pop', x: 50, y: 330, w: 170, h: 65, title: '3a. O(1) Freelist Pop', subtitle: 'PageID = free_pages.back()', type: 'success' });
    d.addBox({ id: 'a_grow', x: 250, y: 330, w: 170, h: 65, title: '3b. Extend File', subtitle: 'PageID = total_pages_++', type: 'disk' });
    d.addBox({ id: 'a_ret1', x: 50, y: 415, w: 170, h: 55, title: 'Return Recycled ID', subtitle: 'Zero file growth!', type: 'success' });
    d.addBox({ id: 'a_ret2', x: 250, y: 415, w: 170, h: 55, title: 'Return Appended ID', subtitle: 'File grows by 4KB', type: 'disk' });

    // Deallocation Subgraph
    d.addContainer({ id: 'dealloc_c', x: 490, y: 130, w: 420, h: 360, title: 'Deallocation Workflow: deallocate_page(page_id)' });
    d.addBox({ id: 'd_req', x: 590, y: 175, w: 220, h: 55, title: '1. Free Page', subtitle: 'deallocate_page(page_id)', type: 'entry' });
    d.addBox({ id: 'd_push', x: 590, y: 260, w: 220, h: 65, title: '2. Push to Freelist Stack', subtitle: 'free_pages.push_back(page_id)', type: 'shared' });
    d.addBox({ id: 'd_ready', x: 590, y: 360, w: 220, h: 55, title: '3. Marked for Reuse', subtitle: 'Available for immediate recycle', type: 'success' });

    // Arrows
    d.addArrow({ points: [[240, 220], [240, 240]] });
    d.addArrow({ points: [[180, 290], [135, 330]], label: 'No (Available)' });
    d.addArrow({ points: [[300, 290], [335, 330]], label: 'Yes (Empty)' });
    d.addArrow({ points: [[135, 395], [135, 415]] });
    d.addArrow({ points: [[335, 395], [335, 415]] });

    d.addArrow({ points: [[370, 70], [135, 70], [135, 330]], label: 'Pop PageID', dashed: true });

    d.addArrow({ points: [[700, 230], [700, 260]] });
    d.addArrow({ points: [[700, 325], [700, 360]] });
    d.addArrow({ points: [[700, 260], [700, 70], [570, 70]], label: 'Push PageID', dashed: true });

    builders.push(d);
  }

  // =========================================================================
  // 10. slotted-pages-compaction (docs/architecture/slotted-pages.md)
  // =========================================================================
  {
    const d = new DiagramBuilder('slotted-pages-compaction', 'Page Fill and Compaction Sequence', 880, 420);

    const xT = 160, xP = 440, xC = 720;
    d.addLifeline({ id: 'l_tab', x: xT, topY: 30, height: 350, name: 'Table Engine', role: 'Caller' });
    d.addLifeline({ id: 'l_page', x: xP, topY: 30, height: 350, name: 'SlottedPage (4KB)', role: 'Storage Page' });
    d.addLifeline({ id: 'l_codec', x: xC, topY: 30, height: 350, name: 'RecordCodec', role: 'Deserializer' });

    d.addSequenceMessage({ fromX: xT, toX: xP, y: 90, num: 1, text: 'compact_page()' });
    d.addSequenceNote({ x: 340, y: 115, w: 200, h: 30, text: 'fragmented_bytes > 0 detected', type: 'guard' });

    d.addSequenceMessage({ fromX: xP, toX: xP + 50, y: 165, num: 2, text: 'Collect active slots (length > 0)' });
    d.addSequenceMessage({ fromX: xP, toX: xP + 50, y: 200, num: 3, text: 'Sort active slots by offset ascending' });

    d.addSequenceMessage({ fromX: xP, toX: xC, y: 240, num: 4, text: 'Read payload bytes' });
    d.addSequenceMessage({ fromX: xP, toX: xP + 50, y: 275, num: 5, text: 'Pack payload contiguous at free_end' });
    d.addSequenceMessage({ fromX: xP, toX: xP + 50, y: 310, num: 6, text: 'Update slot.offset & free_start' });

    d.addSequenceNote({ x: 340, y: 335, w: 200, h: 30, text: 'fragmented_bytes = 0 reset', type: 'success' });
    d.addSequenceMessage({ fromX: xP, toX: xT, y: 375, num: 7, text: 'Contiguous Free Space Restored', dashed: true, type: 'success' });

    builders.push(d);
  }

  // =========================================================================
  // 11. concepts-multi-layer (docs/concepts/architecture.md)
  // =========================================================================
  {
    const d = new DiagramBuilder('concepts-multi-layer', 'Complete Multi-Layer Architecture Diagram', 960, 680);

    d.addContainer({ id: 'l1', x: 30, y: 30, w: 420, h: 180, title: '1. Parsing & SQL Compilation Layer' });
    d.addBox({ id: 'sql', x: 50, y: 70, w: 160, h: 55, title: 'SQL Query String', subtitle: 'Raw Input', type: 'entry' });
    d.addBox({ id: 'lex', x: 250, y: 70, w: 170, h: 55, title: 'Lexical Analyzer', subtitle: 'qdb::Lexer', type: 'entry' });
    d.addBox({ id: 'parse', x: 150, y: 140, w: 180, h: 55, title: 'Recursive Descent Parser', subtitle: 'AST Statement Nodes', type: 'entry' });

    d.addContainer({ id: 'l2', x: 490, y: 30, w: 440, h: 180, title: '2. Relational Execution Layer' });
    d.addBox({ id: 'plan', x: 510, y: 70, w: 180, h: 55, title: 'Query Router & Plan', subtitle: 'Fast-Path vs Scan', type: 'guard' });
    d.addBox({ id: 'exec', x: 720, y: 70, w: 180, h: 55, title: 'Relational Executor', subtitle: 'Volcano Pipeline', type: 'guard' });
    d.addBox({ id: 'eval', x: 615, y: 140, w: 190, h: 55, title: 'Expression Evaluator', subtitle: 'WHERE / Aggregates', type: 'guard' });

    d.addContainer({ id: 'l3', x: 30, y: 240, w: 900, h: 110, title: '3. Catalog & Relational Engine' });
    d.addBox({ id: 'db', x: 60, y: 275, w: 240, h: 60, title: 'Catalog Manager (Page 0)', subtitle: 'qdb::Database Registry', type: 'shared' });
    d.addBox({ id: 'tab_mgr', x: 360, y: 275, w: 240, h: 60, title: 'Table Storage Manager', subtitle: 'qdb::Table Heap', type: 'disk' });
    d.addBox({ id: 'pri_idx', x: 660, y: 275, w: 240, h: 60, title: 'Primary Key Index', subtitle: 'qdb::PrimaryIndex', type: 'shared' });

    d.addContainer({ id: 'l4', x: 30, y: 375, w: 900, h: 110, title: '4. Physical Paged Data Structures' });
    d.addBox({ id: 'slot_p', x: 100, y: 410, w: 220, h: 60, title: 'Slotted Page Engine', subtitle: '4096-Byte Frames & Gaps', type: 'disk' });
    d.addBox({ id: 'bpt_p', x: 370, y: 410, w: 220, h: 60, title: 'Disk B+ Tree Index', subtitle: 'Balanced Fan-out & Chain', type: 'shared' });
    d.addBox({ id: 'codec', x: 640, y: 410, w: 220, h: 60, title: 'Record Codec', subtitle: 'Binary Serialization', type: 'entry' });

    d.addContainer({ id: 'l5', x: 30, y: 510, w: 900, h: 130, title: '5. I/O Block Engine & Storage Medium' });
    d.addBox({ id: 'pager', x: 160, y: 550, w: 260, h: 65, title: 'Pager Engine (qdb::Pager)', subtitle: 'Uniform 4096-Byte Block I/O', type: 'entry' });
    d.addCylinder({ id: 'file_db', x: 540, y: 550, w: 280, h: 65, title: 'Single File Storage (.db)', subtitle: 'Zero External Daemons', type: 'disk' });

    // Connections
    d.addArrow({ points: [[210, 97], [250, 97]] });
    d.addArrow({ points: [[335, 125], [240, 140]] });
    d.addArrow({ points: [[330, 167], [510, 97]] });
    d.addArrow({ points: [[690, 97], [720, 97]] });
    d.addArrow({ points: [[810, 125], [710, 140]] });
    d.addArrow({ points: [[710, 195], [780, 275]] });
    d.addArrow({ points: [[710, 195], [480, 275]] });
    d.addArrow({ points: [[300, 305], [360, 305]] });
    d.addArrow({ points: [[600, 305], [660, 305]] });
    d.addArrow({ points: [[480, 335], [210, 410]] });
    d.addArrow({ points: [[780, 335], [480, 410]] });
    d.addArrow({ points: [[210, 470], [290, 550]] });
    d.addArrow({ points: [[480, 470], [350, 550]] });
    d.addArrow({ points: [[420, 582], [540, 582]] });

    builders.push(d);
  }

  // =========================================================================
  // 12. concepts-query-pipeline (docs/concepts/architecture.md)
  // =========================================================================
  {
    const d = new DiagramBuilder('concepts-query-pipeline', 'End-to-End Query Execution Pipeline', 920, 180);

    d.addBox({ id: 'p1', x: 20, y: 55, w: 130, h: 70, title: '1. SQL Text', subtitle: 'SELECT * FROM ...', type: 'entry' });
    d.addBox({ id: 'p2', x: 175, y: 55, w: 130, h: 70, title: '2. Lexer', subtitle: 'Tokens Stream', type: 'entry' });
    d.addBox({ id: 'p3', x: 330, y: 55, w: 130, h: 70, title: '3. Parser', subtitle: 'Statement AST', type: 'entry' });
    d.addBox({ id: 'p4', x: 485, y: 55, w: 130, h: 70, title: '4. Executor', subtitle: 'Query Plan Routing', type: 'guard' });
    d.addBox({ id: 'p5', x: 640, y: 55, w: 130, h: 70, title: '5. Storage Engine', subtitle: '4KB Slotted & BPT', type: 'shared' });
    d.addBox({ id: 'p6', x: 795, y: 55, w: 105, h: 70, title: '6. Disk File', subtitle: '4KB Block I/O', type: 'disk' });

    d.addArrow({ points: [[150, 90], [175, 90]] });
    d.addArrow({ points: [[305, 90], [330, 90]] });
    d.addArrow({ points: [[460, 90], [485, 90]] });
    d.addArrow({ points: [[615, 90], [640, 90]] });
    d.addArrow({ points: [[770, 90], [795, 90]] });

    builders.push(d);
  }

  // =========================================================================
  // 13. concepts-indexing-topology (docs/concepts/indexing.md)
  // =========================================================================
  {
    const d = new DiagramBuilder('concepts-indexing-topology', 'Topology and Node Fan-Out', 940, 480);

    d.addContainer({ id: 'c_tree', x: 30, y: 20, w: 880, h: 290, title: 'B+ Tree 3-Level Topology (Fixed 4KB Blocks)' });
    d.addBox({ id: 'root', x: 370, y: 50, w: 200, h: 65, title: 'Root Node (Page 3)', subtitle: 'Separator Key: [ 100 ]', type: 'entry' });

    d.addBox({ id: 'int1', x: 200, y: 140, w: 190, h: 60, title: 'Internal Node (Page 5)', subtitle: 'Separator: [ 50 ]', type: 'disk' });
    d.addBox({ id: 'int2', x: 550, y: 140, w: 190, h: 60, title: 'Internal Node (Page 6)', subtitle: 'Separator: [ 150 ]', type: 'disk' });

    d.addBox({ id: 'l10', x: 50, y: 230, w: 170, h: 60, title: 'Leaf Page 10', subtitle: 'Keys: [ 10 .. 49 ]', type: 'success' });
    d.addBox({ id: 'l11', x: 260, y: 230, w: 170, h: 60, title: 'Leaf Page 11', subtitle: 'Keys: [ 50 .. 99 ]', type: 'success' });
    d.addBox({ id: 'l12', x: 510, y: 230, w: 170, h: 60, title: 'Leaf Page 12', subtitle: 'Keys: [ 100 .. 149 ]', type: 'success' });
    d.addBox({ id: 'l13', x: 720, y: 230, w: 170, h: 60, title: 'Leaf Page 13', subtitle: 'Keys: [ 150 .. 200 ]', type: 'success' });

    d.addContainer({ id: 'c_data', x: 150, y: 340, w: 640, h: 110, title: 'Physical Table Heap (Slotted 4KB Pages)' });
    d.addBox({ id: 'd1', x: 200, y: 375, w: 220, h: 55, title: 'Table Page 1', subtitle: 'Slotted Records {P1, S0..Sn}', type: 'disk' });
    d.addBox({ id: 'd2', x: 520, y: 375, w: 220, h: 55, title: 'Table Page 4', subtitle: 'Slotted Records {P4, S0..Sn}', type: 'disk' });

    // Connections
    d.addArrow({ points: [[430, 115], [300, 140]], label: 'Child 0 (<100)' });
    d.addArrow({ points: [[510, 115], [640, 140]], label: 'Child 1 (>=100)' });

    d.addArrow({ points: [[260, 200], [150, 230]] });
    d.addArrow({ points: [[320, 200], [330, 230]] });
    d.addArrow({ points: [[610, 200], [590, 230]] });
    d.addArrow({ points: [[680, 200], [780, 230]] });

    d.addArrow({ points: [[220, 260], [260, 260]], label: 'next' });
    d.addArrow({ points: [[430, 260], [510, 260]], label: 'next' });
    d.addArrow({ points: [[680, 260], [720, 260]], label: 'next' });

    d.addArrow({ points: [[135, 290], [280, 375]], label: 'RecordID', dashed: true });
    d.addArrow({ points: [[345, 290], [320, 375]], label: 'RecordID', dashed: true });
    d.addArrow({ points: [[595, 290], [600, 375]], label: 'RecordID', dashed: true });
    d.addArrow({ points: [[805, 290], [650, 375]], label: 'RecordID', dashed: true });

    builders.push(d);
  }

  // =========================================================================
  // 14. what-is-quartz-architecture (docs/guide/what-is-quartz.md)
  // =========================================================================
  {
    const d = new DiagramBuilder('what-is-quartz-architecture', 'High-Level System Architecture', 920, 480);

    d.addContainer({ id: 'f_end', x: 30, y: 30, w: 860, h: 100, title: '1. Query Processing Layer' });
    d.addBox({ id: 'sql', x: 50, y: 60, w: 160, h: 50, title: 'SQL Query String', subtitle: 'REPL or C++ API', type: 'entry' });
    d.addBox({ id: 'lex', x: 260, y: 60, w: 160, h: 50, title: 'Lexer Scanner', subtitle: 'Token Stream', type: 'entry' });
    d.addBox({ id: 'parse', x: 470, y: 60, w: 160, h: 50, title: 'AST Parser', subtitle: 'Statement AST', type: 'entry' });
    d.addBox({ id: 'exec', x: 680, y: 60, w: 190, h: 50, title: 'Query Executor', subtitle: 'Execution Engine', type: 'guard' });

    d.addContainer({ id: 'cat_l', x: 30, y: 155, w: 860, h: 120, title: '2. Catalog & Table Manager' });
    d.addBox({ id: 'cat', x: 60, y: 190, w: 230, h: 65, title: 'Database Catalog (Page 0)', subtitle: 'Schema & Root Pointers', type: 'shared' });
    d.addBox({ id: 'tab', x: 340, y: 190, w: 230, h: 65, title: 'Table Manager', subtitle: 'Slotted Page Records', type: 'guard' });
    d.addBox({ id: 'idx', x: 620, y: 190, w: 230, h: 65, title: 'Primary Key Index', subtitle: 'B+ Tree Routing', type: 'guard' });

    d.addContainer({ id: 'stor_l', x: 30, y: 300, w: 860, h: 150, title: '3. Physical Storage Engine' });
    d.addBox({ id: 'bpt', x: 100, y: 335, w: 200, h: 55, title: 'Disk B+ Tree Index', subtitle: 'Multi-level Fan-out', type: 'shared' });
    d.addBox({ id: 'slotted', x: 360, y: 335, w: 200, h: 55, title: 'Slotted Page Engine', subtitle: '4KB Pages & Gaps', type: 'disk' });
    d.addBox({ id: 'pager', x: 620, y: 335, w: 200, h: 55, title: '4KB Pager Engine', subtitle: 'Direct Block I/O', type: 'entry' });
    d.addCylinder({ id: 'disk', x: 360, y: 405, w: 200, h: 36, title: 'Unified .db File on Disk', subtitle: '', type: 'disk' });

    // Connections
    d.addArrow({ points: [[210, 85], [260, 85]] });
    d.addArrow({ points: [[420, 85], [470, 85]] });
    d.addArrow({ points: [[630, 85], [680, 85]] });

    d.addArrow({ points: [[775, 110], [775, 155], [175, 155], [175, 190]] });
    d.addArrow({ points: [[290, 222], [340, 222]] });
    d.addArrow({ points: [[570, 222], [620, 222]] });

    d.addArrow({ points: [[735, 255], [200, 335]] });
    d.addArrow({ points: [[455, 255], [460, 335]] });
    d.addArrow({ points: [[300, 362], [360, 362]] });
    d.addArrow({ points: [[560, 362], [620, 362]] });
    d.addArrow({ points: [[720, 390], [720, 423], [560, 423]] });

    builders.push(d);
  }

  // =========================================================================
  // 15. backup-recovery-compaction (docs/operations/backup-recovery.md)
  // =========================================================================
  {
    const d = new DiagramBuilder('backup-recovery-compaction', 'Compaction Mechanics and Gap Elimination', 880, 260);

    d.addBox({ id: 'scan', x: 30, y: 95, w: 180, h: 70, title: '1. Pager Scans 4KB Pages', subtitle: 'Inspect slotted header state', type: 'entry' });
    d.addDiamond({ id: 'chk', x: 260, y: 80, w: 190, h: 100, text: 'Fragmented Bytes > 20%?', type: 'guard' });

    d.addBox({ id: 'rebuild', x: 490, y: 30, w: 180, h: 65, title: '2. Rebuild In-Memory Page', subtitle: 'Copy active slots (length > 0)', type: 'disk' });
    d.addBox({ id: 'slide', x: 690, y: 30, w: 160, h: 65, title: '3. Slide Payloads', subtitle: 'Contiguous to byte 4096', type: 'shared' });
    d.addBox({ id: 'flush', x: 690, y: 140, w: 160, h: 65, title: '4. Write Back Page', subtitle: 'fragmented_bytes = 0', type: 'disk' });

    d.addBox({ id: 'retain', x: 490, y: 170, w: 170, h: 55, title: 'Optimal Density Preserved', subtitle: 'No compaction needed', type: 'success' });

    d.addArrow({ points: [[210, 130], [260, 130]] });
    d.addArrow({ points: [[355, 80], [355, 62], [490, 62]], label: 'Yes (> 20%)' });
    d.addArrow({ points: [[670, 62], [690, 62]] });
    d.addArrow({ points: [[770, 95], [770, 140]] });
    d.addArrow({ points: [[355, 180], [355, 197], [490, 197]], label: 'No (Optimal)' });

    builders.push(d);
  }

  // =========================================================================
  // 16. sql-aggregations-workflow (docs/sql/aggregations.md)
  // =========================================================================
  {
    const d = new DiagramBuilder('sql-aggregations-workflow', 'How Aggregations Work Under the Hood', 940, 520);

    d.addBox({ id: 'start', x: 40, y: 30, w: 220, h: 60, title: '1. SELECT with Aggregates', subtitle: 'SELECT COUNT(*), SUM(cents)...', type: 'entry' });
    d.addBox({ id: 'filter', x: 300, y: 30, w: 220, h: 60, title: '2. Filter Phase (WHERE)', subtitle: 'Generates matched_rows vector', type: 'shared' });
    d.addDiamond({ id: 'chk_lim', x: 560, y: 25, w: 150, h: 70, text: 'LIMIT == 0?', type: 'guard' });
    d.addBox({ id: 'empty', x: 750, y: 30, w: 160, h: 60, title: 'Return 0 Rows', subtitle: 'Empty set immediately', type: 'success' });

    d.addBox({ id: 'init', x: 350, y: 130, w: 240, h: 55, title: '4. Initialize Accumulator Record', subtitle: 'Reserve slots for AggregateCalls', type: 'shared' });
    d.addBox({ id: 'loop', x: 370, y: 215, w: 200, h: 50, title: '5. For Each AggregateCall', subtitle: 'Column reduction loop', type: 'entry' });

    d.addBox({ id: 'do_star', x: 40, y: 295, w: 180, h: 55, title: 'COUNT(*)', subtitle: 'Append matched_rows.size()', type: 'success' });
    d.addBox({ id: 'do_cnt', x: 245, y: 295, w: 195, h: 55, title: 'COUNT(col)', subtitle: 'Increment if != NULL', type: 'success' });
    d.addDiamond({ id: 'chk_int', x: 465, y: 285, w: 170, h: 75, text: 'Col type == INT?', type: 'guard' });
    d.addBox({ id: 'do_ext', x: 670, y: 295, w: 220, h: 55, title: 'MIN(col) / MAX(col)', subtitle: 'Track min/max via compare()', type: 'success' });

    d.addBox({ id: 'err_type', x: 370, y: 395, w: 180, h: 50, title: 'Abort: Invalid Type', subtitle: 'SUM/AVG requires integer', type: 'error' });
    d.addBox({ id: 'do_sum', x: 580, y: 395, w: 200, h: 55, title: 'SUM / AVG Accumulation', subtitle: 'AVG = sum / count (trunc)', type: 'success' });

    d.addBox({ id: 'pkg', x: 350, y: 465, w: 240, h: 45, title: '6. Package QueryResult', subtitle: '1 row with metrics', type: 'success' });

    // Connections
    d.addArrow({ points: [[260, 60], [300, 60]] });
    d.addArrow({ points: [[520, 60], [560, 60]] });
    d.addArrow({ points: [[710, 60], [750, 60]], label: 'Yes' });
    d.addArrow({ points: [[635, 95], [635, 157], [590, 157]], label: 'No' });
    d.addArrow({ points: [[470, 185], [470, 215]] });

    d.addArrow({ points: [[370, 240], [130, 240], [130, 295]], label: 'COUNT(*)' });
    d.addArrow({ points: [[420, 265], [342, 295]], label: 'COUNT(col)' });
    d.addArrow({ points: [[510, 265], [550, 285]], label: 'SUM / AVG' });
    d.addArrow({ points: [[570, 240], [780, 240], [780, 295]], label: 'MIN / MAX' });

    d.addArrow({ points: [[510, 360], [460, 395]], label: 'No (Text)' });
    d.addArrow({ points: [[590, 360], [680, 395]], label: 'Yes (Int)' });

    d.addArrow({ points: [[130, 350], [130, 487], [350, 487]] });
    d.addArrow({ points: [[342, 350], [342, 475], [350, 475]] });
    d.addArrow({ points: [[680, 450], [680, 475], [590, 475]] });
    d.addArrow({ points: [[780, 350], [780, 487], [590, 487]] });

    builders.push(d);
  }

  // =========================================================================
  // 17. sql-joins-workflow (docs/sql/joins.md)
  // =========================================================================
  {
    const d = new DiagramBuilder('sql-joins-workflow', 'How JOIN Works Under the Hood in QuartzDB', 940, 520);

    d.addBox({ id: 'scan_a', x: 40, y: 40, w: 220, h: 65, title: '1. Scan Outer Table A', subtitle: 'Iterate candidate slotted tuples', type: 'entry' });
    d.addBox({ id: 'loop_r', x: 300, y: 40, w: 180, h: 65, title: '2. For Each Left Row', subtitle: 'Extract join column value', type: 'entry' });
    d.addDiamond({ id: 'chk_pk', x: 520, y: 25, w: 180, h: 95, text: 'ON pred: FK = PK?', type: 'guard' });

    d.addBox({ id: 'inlj', x: 740, y: 15, w: 180, h: 55, title: 'INLJ Fast-Path: O(log M)', subtitle: 'find_by_primary_key(fk)', type: 'success' });
    d.addBox({ id: 'scan_b', x: 740, y: 80, w: 180, h: 55, title: 'Linear Block Scan: O(M)', subtitle: 'Iterate right slotted pages', type: 'disk' });

    d.addDiamond({ id: 'found', x: 530, y: 160, w: 160, h: 80, text: 'Match Found?', type: 'guard' });

    d.addBox({ id: 'emit_both', x: 740, y: 172, w: 180, h: 55, title: 'Emit Joined Row', subtitle: '[ Row A | Row B ]', type: 'success' });

    d.addDiamond({ id: 'chk_left', x: 320, y: 160, w: 160, h: 80, text: 'Is LEFT JOIN?', type: 'guard' });
    d.addBox({ id: 'emit_null', x: 80, y: 172, w: 200, h: 55, title: 'Emit Outer Row', subtitle: '[ Row A | NULL ... ]', type: 'shared' });
    d.addBox({ id: 'skip', x: 310, y: 280, w: 180, h: 50, title: 'Skip Unmatched Row', subtitle: 'INNER JOIN drops row', type: 'error' });

    d.addBox({ id: 'filter_p', x: 320, y: 370, w: 300, h: 60, title: '4. Apply WHERE, Projections & ORDER BY', subtitle: 'Relational post-processing pipeline', type: 'entry' });
    d.addBox({ id: 'res', x: 370, y: 460, w: 200, h: 45, title: '5. Return QueryResult', subtitle: 'Formatted table output', type: 'success' });

    // Connections
    d.addArrow({ points: [[260, 72], [300, 72]] });
    d.addArrow({ points: [[480, 72], [520, 72]] });
    d.addArrow({ points: [[700, 55], [740, 42]], label: 'Yes (Indexed)' });
    d.addArrow({ points: [[700, 90], [740, 107]], label: 'No (Scan)' });

    d.addArrow({ points: [[830, 70], [830, 140], [610, 140], [610, 160]] });
    d.addArrow({ points: [[740, 107], [660, 107], [660, 160]] });

    d.addArrow({ points: [[690, 200], [740, 200]], label: 'Yes' });
    d.addArrow({ points: [[530, 200], [480, 200]], label: 'No' });

    d.addArrow({ points: [[320, 200], [280, 200]], label: 'Yes (LEFT)' });
    d.addArrow({ points: [[400, 240], [400, 280]], label: 'No (INNER)' });

    d.addArrow({ points: [[180, 227], [180, 400], [320, 400]] });
    d.addArrow({ points: [[830, 227], [830, 400], [620, 400]] });
    d.addArrow({ points: [[470, 430], [470, 460]] });

    builders.push(d);
  }

  // =========================================================================
  // 18. sql-order-by-limit-workflow (docs/sql/order-by-limit.md)
  // =========================================================================
  {
    const d = new DiagramBuilder('sql-order-by-limit-workflow', 'How Sorting and Windowing Work in QuartzDB', 940, 420);

    d.addBox({ id: 'cand', x: 30, y: 35, w: 200, h: 65, title: '1. Candidate Tuples', subtitle: 'Slotted Scan or BPT Seek', type: 'entry' });
    d.addBox({ id: 'filt', x: 270, y: 35, w: 180, h: 65, title: '2. Filter Pass', subtitle: 'Evaluate WHERE clauses', type: 'entry' });
    d.addDiamond({ id: 'has_sort', x: 490, y: 25, w: 160, h: 85, text: 'Has ORDER BY?', type: 'guard' });

    d.addBox({ id: 'sort', x: 690, y: 25, w: 220, h: 85, title: 'std::stable_sort()', subtitle: 'order_by_compare()\nSort INT64, String, NULLs', type: 'shared' });

    d.addBox({ id: 'win', x: 490, y: 155, w: 160, h: 50, title: '4. Windowing Pass', subtitle: 'OFFSET & LIMIT', type: 'entry' });

    d.addDiamond({ id: 'has_off', x: 280, y: 240, w: 160, h: 75, text: 'Has OFFSET K?', type: 'guard' });
    d.addBox({ id: 'slice_off', x: 50, y: 250, w: 190, h: 55, title: 'Skip First K Rows', subtitle: 'Advance memory buffer', type: 'shared' });

    d.addDiamond({ id: 'has_lim', x: 500, y: 240, w: 160, h: 75, text: 'Has LIMIT M?', type: 'guard' });
    d.addBox({ id: 'slice_lim', x: 710, y: 250, w: 190, h: 55, title: 'Truncate to M Rows', subtitle: 'Resize memory buffer', type: 'shared' });

    d.addBox({ id: 'emit', x: 480, y: 355, w: 200, h: 45, title: '5. Return QueryResult', subtitle: 'Formatted result set', type: 'success' });

    // Connections
    d.addArrow({ points: [[230, 67], [270, 67]] });
    d.addArrow({ points: [[450, 67], [490, 67]] });
    d.addArrow({ points: [[650, 67], [690, 67]], label: 'Yes' });
    d.addArrow({ points: [[800, 110], [800, 180], [650, 180]] });
    d.addArrow({ points: [[570, 110], [570, 155]], label: 'No' });

    d.addArrow({ points: [[490, 180], [360, 180], [360, 240]] });
    d.addArrow({ points: [[280, 277], [240, 277]], label: 'Yes' });
    d.addArrow({ points: [[145, 305], [145, 335], [440, 335], [440, 277], [500, 277]] });
    d.addArrow({ points: [[440, 277], [500, 277]], label: 'No' });

    d.addArrow({ points: [[660, 277], [710, 277]], label: 'Yes' });
    d.addArrow({ points: [[805, 305], [805, 377], [680, 377]] });
    d.addArrow({ points: [[580, 315], [580, 355]], label: 'No' });

    builders.push(d);
  }

  // =========================================================================
  // 19. create-table-workflow (docs/sql/statements/create-table.md)
  // =========================================================================
  {
    const d = new DiagramBuilder('create-table-workflow', 'How CREATE TABLE Works Under the Hood', 920, 380);

    d.addBox({ id: 'start', x: 30, y: 35, w: 220, h: 65, title: '1. Execute CREATE TABLE', subtitle: 'CREATE TABLE users (id INT PK..)', type: 'entry' });
    d.addBox({ id: 'ast', x: 290, y: 35, w: 200, h: 65, title: '2. AST Parser & Validate', subtitle: 'Verify exactly 1 INT PK', type: 'entry' });
    d.addDiamond({ id: 'chk_dup', x: 530, y: 25, w: 170, h: 85, text: 'Table Name Exists?', type: 'guard' });
    d.addBox({ id: 'abort', x: 740, y: 35, w: 150, h: 65, title: 'Abort: Collision', subtitle: 'TableAlreadyExists', type: 'error' });

    d.addBox({ id: 'alloc_p1', x: 120, y: 170, w: 200, h: 65, title: '4. Allocate Page 1', subtitle: 'Pager::allocate_page()\nTable Data Root', type: 'disk' });
    d.addBox({ id: 'alloc_bpt', x: 370, y: 170, w: 200, h: 65, title: '5. Allocate B+ Tree Pages', subtitle: 'Page 2: Meta · Page 3: Root', type: 'shared' });
    d.addBox({ id: 'reg_cat', x: 620, y: 170, w: 220, h: 65, title: '6. Register in Catalog Page 0', subtitle: 'Persist schema & root IDs', type: 'disk' });

    d.addBox({ id: 'ok', x: 360, y: 290, w: 220, h: 50, title: '7. Return QueryResult', subtitle: 'Table Created Successfully', type: 'success' });

    // Connections
    d.addArrow({ points: [[250, 67], [290, 67]] });
    d.addArrow({ points: [[490, 67], [530, 67]] });
    d.addArrow({ points: [[700, 67], [740, 67]], label: 'Yes' });
    d.addArrow({ points: [[615, 110], [615, 140], [220, 140], [220, 170]], label: 'No (Valid)' });

    d.addArrow({ points: [[320, 202], [370, 202]] });
    d.addArrow({ points: [[570, 202], [620, 202]] });
    d.addArrow({ points: [[730, 235], [730, 315], [580, 315]] });

    builders.push(d);
  }

  // =========================================================================
  // 20. create-table-allocation-sequence (docs/sql/statements/create-table.md)
  // =========================================================================
  {
    const d = new DiagramBuilder('create-table-allocation-sequence', 'Allocation Flow Sequence', 880, 440);

    const xS = 120, xC = 340, xP = 560, xB = 760;
    d.addLifeline({ id: 'l_sql', x: xS, topY: 30, height: 380, name: 'DDL Executor', role: 'qdb::Executor' });
    d.addLifeline({ id: 'l_cat', x: xC, topY: 30, height: 380, name: 'Catalog (Page 0)', role: 'Metadata Manager' });
    d.addLifeline({ id: 'l_pg', x: xP, topY: 30, height: 380, name: 'Pager', role: '4KB Block Engine' });
    d.addLifeline({ id: 'l_bpt', x: xB, topY: 30, height: 380, name: 'B+ Tree Index', role: 'qdb::PrimaryIndex' });

    d.addSequenceMessage({ fromX: xS, toX: xP, y: 95, num: 1, text: 'allocate_page() -> Page 1' });
    d.addSequenceMessage({ fromX: xP, toX: xS, y: 130, num: 2, text: 'Page 1 Initialized (Table Root)', dashed: true });

    d.addSequenceMessage({ fromX: xS, toX: xB, y: 170, num: 3, text: 'Initialize Primary Key Index' });
    d.addSequenceMessage({ fromX: xB, toX: xP, y: 205, num: 4, text: 'allocate_page() -> Page 2 (Meta)' });
    d.addSequenceMessage({ fromX: xB, toX: xP, y: 240, num: 5, text: 'allocate_page() -> Page 3 (Root)' });
    d.addSequenceMessage({ fromX: xP, toX: xB, y: 275, num: 6, text: 'Index Pages Initialized', dashed: true });

    d.addSequenceMessage({ fromX: xS, toX: xC, y: 315, num: 7, text: 'Register Table {name, first_page=1, root=3}' });
    d.addSequenceMessage({ fromX: xC, toX: xP, y: 350, num: 8, text: 'write_page(0, catalog_bytes)' });
    d.addSequenceMessage({ fromX: xC, toX: xS, y: 385, num: 9, text: 'Table Active in Catalog', dashed: true, type: 'success' });

    builders.push(d);
  }

  // =========================================================================
  // 21. delete-workflow (docs/sql/statements/delete.md)
  // =========================================================================
  {
    const d = new DiagramBuilder('delete-workflow', 'How DELETE Works Under the Hood in QuartzDB', 940, 520);

    d.addBox({ id: 'start', x: 30, y: 35, w: 220, h: 65, title: '1. Execute DELETE', subtitle: 'DELETE FROM orders WHERE id=1042', type: 'entry' });
    d.addDiamond({ id: 'chk_rt', x: 290, y: 25, w: 170, h: 85, text: 'WHERE on PK?', type: 'guard' });

    d.addBox({ id: 'bpt_seek', x: 500, y: 20, w: 200, h: 55, title: '3a. B+ Tree Seek: O(log N)', subtitle: 'Find RecordID {page, slot}', type: 'success' });
    d.addBox({ id: 'scan_seek', x: 500, y: 85, w: 200, h: 55, title: '3b. Slotted Scan: O(N)', subtitle: 'Evaluate WHERE filter on pages', type: 'disk' });

    d.addBox({ id: 'fetch', x: 740, y: 50, w: 170, h: 65, title: '4. Fetch Data Page', subtitle: 'Pager::read_page(page_id)', type: 'disk' });

    d.addBox({ id: 'tomb', x: 740, y: 175, w: 170, h: 65, title: '5. Slot Tombstone', subtitle: 'slot.record_length = 0\nGap added to fragmented', type: 'guard' });
    d.addBox({ id: 'erase', x: 500, y: 175, w: 190, h: 65, title: '6. B+ Tree Erasure', subtitle: 'BplusTree::erase(pk)\nRemove key from leaf', type: 'disk' });

    d.addDiamond({ id: 'chk_under', x: 280, y: 165, w: 170, h: 85, text: 'Keys < MIN_KEYS (64)?', type: 'guard' });

    d.addBox({ id: 'rebal', x: 40, y: 175, w: 200, h: 75, title: '8. Tree Rebalancing', subtitle: '• Borrow from sibling\n• Merge nodes & recycle page\n• Collapse root if 1 child', type: 'shared' });

    d.addBox({ id: 'ok', x: 190, y: 350, w: 200, h: 50, title: '9. Return QueryResult', subtitle: '"N row(s) deleted."', type: 'success' });

    // Connections
    d.addArrow({ points: [[250, 67], [290, 67]] });
    d.addArrow({ points: [[460, 50], [500, 42]], label: 'Yes (Fast-Path)' });
    d.addArrow({ points: [[460, 85], [500, 107]], label: 'No (Scan)' });

    d.addArrow({ points: [[700, 42], [740, 65]] });
    d.addArrow({ points: [[700, 107], [740, 90]] });

    d.addArrow({ points: [[825, 115], [825, 175]] });
    d.addArrow({ points: [[740, 207], [690, 207]] });
    d.addArrow({ points: [[500, 207], [450, 207]] });

    d.addArrow({ points: [[280, 207], [240, 207]], label: 'Yes' });
    d.addArrow({ points: [[365, 250], [365, 350]], label: 'No' });
    d.addArrow({ points: [[140, 250], [140, 375], [190, 375]] });

    builders.push(d);
  }

  // =========================================================================
  // 22. drop-table-workflow (docs/sql/statements/drop-table.md)
  // =========================================================================
  {
    const d = new DiagramBuilder('drop-table-workflow', 'How DROP TABLE Works Under the Hood in QuartzDB', 940, 360);

    d.addBox({ id: 'start', x: 30, y: 50, w: 180, h: 65, title: '1. DROP TABLE orders', subtitle: 'Initiate teardown', type: 'entry' });
    d.addBox({ id: 'val', x: 240, y: 50, w: 190, h: 65, title: '2. Validate in Catalog', subtitle: 'Lookup table in Page 0', type: 'guard' });
    d.addBox({ id: 'free_data', x: 460, y: 40, w: 220, h: 85, title: '3. Walk Data Page Chain', subtitle: 'Follow next_page pointers\nPush IDs to free_pages stack', type: 'disk' });
    d.addBox({ id: 'free_idx', x: 710, y: 40, w: 200, h: 85, title: '4. Free B+ Tree Pages', subtitle: 'Post-order DFS traversal\nRecycle all nodes + meta', type: 'shared' });

    d.addBox({ id: 'erase_cat', x: 550, y: 190, w: 240, h: 70, title: '5. Unregister from Catalog', subtitle: 'Decrement table_count in Page 0\nPersist compacted catalog', type: 'disk' });
    d.addBox({ id: 'reuse', x: 180, y: 190, w: 320, h: 70, title: '6. Instant Zero-Cost Page Reuse', subtitle: 'All PageIDs in pager.free_pages stack\nSubsequent inserts reuse with O(1) efficiency', type: 'success' });

    d.addArrow({ points: [[210, 82], [240, 82]] });
    d.addArrow({ points: [[430, 82], [460, 82]] });
    d.addArrow({ points: [[680, 82], [710, 82]] });
    d.addArrow({ points: [[810, 125], [810, 225], [790, 225]] });
    d.addArrow({ points: [[550, 225], [500, 225]] });

    builders.push(d);
  }

  // =========================================================================
  // 23. insert-workflow (docs/sql/statements/insert.md)
  // =========================================================================
  {
    const d = new DiagramBuilder('insert-workflow', 'How INSERT Works Under the Hood in QuartzDB', 940, 480);

    d.addBox({ id: 'start', x: 30, y: 40, w: 200, h: 60, title: '1. Execute INSERT', subtitle: 'INSERT INTO users VALUES (42, ..)', type: 'entry' });
    d.addBox({ id: 'val', x: 260, y: 40, w: 190, h: 60, title: '2. Schema Validation', subtitle: 'Verify types & NOT NULL rules', type: 'entry' });
    d.addDiamond({ id: 'chk_pk', x: 480, y: 25, w: 170, h: 90, text: 'PK Unique in BPT?', type: 'guard' });
    d.addBox({ id: 'abort_pk', x: 690, y: 40, w: 180, h: 60, title: 'Abort: Collision', subtitle: 'DuplicatePrimaryKey error', type: 'error' });

    d.addBox({ id: 'codec', x: 480, y: 160, w: 170, h: 55, title: '4. RecordCodec Pack', subtitle: 'Serialize to binary slice', type: 'shared' });
    d.addDiamond({ id: 'chk_space', x: 475, y: 250, w: 180, h: 80, text: 'Page Space Available?', type: 'guard' });

    d.addBox({ id: 'alloc_page', x: 220, y: 260, w: 190, h: 60, title: '6a. Allocate New Page', subtitle: 'Pager::allocate_page()\nLink via next_page', type: 'disk' });
    d.addBox({ id: 'write_slot', x: 470, y: 360, w: 190, h: 65, title: '6b. Write Slot & Payload', subtitle: 'Slot at free_start\nPayload at free_end', type: 'disk' });

    d.addBox({ id: 'idx_bpt', x: 700, y: 360, w: 190, h: 65, title: '7. Update B+ Tree Index', subtitle: 'Insert {pk, RecordID}\nSplit leaf if full', type: 'shared' });
    d.addBox({ id: 'ok', x: 700, y: 260, w: 190, h: 55, title: '8. Return QueryResult', subtitle: '1 row inserted', type: 'success' });

    // Connections
    d.addArrow({ points: [[230, 70], [260, 70]] });
    d.addArrow({ points: [[450, 70], [480, 70]] });
    d.addArrow({ points: [[650, 70], [690, 70]], label: 'No (Duplicate)' });
    d.addArrow({ points: [[565, 115], [565, 160]], label: 'Yes (Unique)' });

    d.addArrow({ points: [[565, 215], [565, 250]] });
    d.addArrow({ points: [[475, 290], [410, 290]], label: 'No (Full)' });
    d.addArrow({ points: [[315, 320], [315, 392], [470, 392]] });
    d.addArrow({ points: [[565, 330], [565, 360]], label: 'Yes' });

    d.addArrow({ points: [[660, 392], [700, 392]] });
    d.addArrow({ points: [[795, 360], [795, 315]] });

    builders.push(d);
  }

  // =========================================================================
  // 24. update-workflow (docs/sql/statements/update.md)
  // =========================================================================
  {
    const d = new DiagramBuilder('update-workflow', 'How UPDATE Works Under the Hood in QuartzDB', 940, 520);

    d.addBox({ id: 'start', x: 30, y: 40, w: 190, h: 60, title: '1. Execute UPDATE', subtitle: 'UPDATE users SET email=..', type: 'entry' });
    d.addDiamond({ id: 'chk_pk', x: 250, y: 25, w: 170, h: 90, text: 'Modifies PRIMARY KEY?', type: 'guard' });
    d.addBox({ id: 'abort_pk', x: 450, y: 40, w: 190, h: 60, title: 'Abort: PK Immutable', subtitle: 'Cannot modify primary key', type: 'error' });

    d.addDiamond({ id: 'chk_rt', x: 250, y: 155, w: 170, h: 80, text: 'WHERE on PK?', type: 'guard' });
    d.addBox({ id: 'bpt_seek', x: 460, y: 140, w: 190, h: 50, title: '4a. B+ Tree Point Fetch', subtitle: 'O(log N) -> RecordID {P, S}', type: 'success' });
    d.addBox({ id: 'scan_seek', x: 460, y: 200, w: 190, h: 50, title: '4b. Slotted-Page Scan', subtitle: 'O(N) -> Scan data pages', type: 'disk' });

    d.addDiamond({ id: 'chk_sz', x: 700, y: 145, w: 190, h: 100, text: 'New Length <= Old Length?', type: 'guard' });

    d.addBox({ id: 'inplace', x: 690, y: 285, w: 210, h: 65, title: 'In-Place Overwrite', subtitle: 'Write bytes at existing offset\nZero fragmentation created', type: 'success' });
    d.addBox({ id: 'reslot', x: 430, y: 285, w: 220, h: 65, title: 'Out-of-Place Re-Slotting', subtitle: 'Mark old bytes as fragmented\nWrite new payload at free_end', type: 'disk' });

    d.addBox({ id: 'safe', x: 500, y: 410, w: 320, h: 65, title: '6. RecordID Pointer Preserved', subtitle: 'slot_index never changes!\nB+ Tree index pointers remain 100% valid', type: 'shared' });

    // Connections
    d.addArrow({ points: [[220, 70], [250, 70]] });
    d.addArrow({ points: [[420, 70], [450, 70]], label: 'Yes' });
    d.addArrow({ points: [[335, 115], [335, 155]], label: 'No' });

    d.addArrow({ points: [[420, 175], [460, 165]], label: 'Yes (Fast-Path)' });
    d.addArrow({ points: [[420, 210], [460, 225]], label: 'No (Scan)' });

    d.addArrow({ points: [[650, 165], [700, 185]] });
    d.addArrow({ points: [[650, 225], [700, 205]] });

    d.addArrow({ points: [[795, 245], [795, 285]], label: 'Yes (Fits)' });
    d.addArrow({ points: [[700, 215], [540, 215], [540, 285]], label: 'No (Larger)' });

    d.addArrow({ points: [[795, 350], [795, 410]] });
    d.addArrow({ points: [[540, 350], [540, 410]] });

    builders.push(d);
  }

  // =========================================================================
  // 25. erd-orders-related (docs/tables/orders.md)
  // =========================================================================
  {
    const d = new DiagramBuilder('erd-orders-related', 'Related Tables Entity-Relationship Diagram', 920, 360);

    d.addEntityCard({
      id: 'e_users', x: 40, y: 50, w: 250,
      title: 'users',
      fields: ['int64 id [PK]', 'string email', 'string full_name'],
      type: 'entry'
    });

    d.addEntityCard({
      id: 'e_orders', x: 340, y: 50, w: 250,
      title: 'orders',
      fields: ['int64 id [PK]', 'int64 user_id [FK]', 'int64 total_cents', 'string status'],
      type: 'shared'
    });

    d.addEntityCard({
      id: 'e_items', x: 640, y: 50, w: 250,
      title: 'line_items',
      fields: ['int64 id [PK]', 'int64 order_id [FK]', 'int64 product_id [FK]'],
      type: 'disk'
    });

    d.addArrow({ points: [[290, 110], [340, 110]], label: 'places (1:N)' });
    d.addArrow({ points: [[590, 110], [640, 110]], label: 'contains (1:N)' });

    builders.push(d);
  }

  return builders;
}
