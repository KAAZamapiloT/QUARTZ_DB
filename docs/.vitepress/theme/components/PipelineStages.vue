<template>
  <div class="pipeline-stages-card diagram-full">
    <!-- Component Header -->
    <div class="card-header">
      <div class="header-titles">
        <span class="header-tag">INTERACTIVE PIPELINE TRACER</span>
        <h3 class="header-title">Detailed Query Execution Pipeline</h3>
        <p class="header-sub">
          Trace query lifecycle transformations through each layer of the QuartzDB engine — from ASCII text to binary disk pages.
        </p>
      </div>

      <!-- Statement Selector Tabs -->
      <div class="stmt-tabs">
        <button 
          v-for="s in statements" 
          :key="s.id"
          :class="['stmt-tab', { active: activeStmtId === s.id }]"
          @click="activeStmtId = s.id"
        >
          <span :class="['stmt-dot', s.color]"></span>
          {{ s.label }}
        </button>
      </div>
    </div>

    <!-- Active Query Example Banner -->
    <div class="active-query-banner">
      <div class="query-sql-wrap">
        <span class="prompt-symbol">quartz&gt;</span>
        <code class="query-sql-text">{{ currentStmt.sql }}</code>
      </div>
      <div class="query-strategy-badge" :class="currentStmt.strategyType">
        {{ currentStmt.strategyLabel }}
      </div>
    </div>

    <!-- Strategy Switcher (For SELECT/UPDATE/DELETE) -->
    <div v-if="currentStmt.hasStrategyToggle" class="strategy-toggle-bar">
      <span class="strat-label">Execution Strategy:</span>
      <button 
        :class="['strat-btn', { active: executionMode === 'fast' }]"
        @click="executionMode = 'fast'"
      >
        ⚡ B+ Tree Index Fast-Path (O(log N))
      </button>
      <button 
        :class="['strat-btn', { active: executionMode === 'scan' }]"
        @click="executionMode = 'scan'"
      >
        🔄 Slotted-Page Linear Scan (O(N))
      </button>
    </div>

    <!-- Pipeline Stages Sequence -->
    <div class="stages-timeline">
      <div 
        v-for="(stage, idx) in activeStages" 
        :key="stage.step"
        class="stage-card"
        :class="[stage.theme, { 'is-last': idx === activeStages.length - 1 }]"
      >
        <!-- Stage Step Indicator & Connector -->
        <div class="timeline-gutter">
          <div class="step-circle">{{ stage.step }}</div>
          <div v-if="idx < activeStages.length - 1" class="step-stem"></div>
        </div>

        <!-- Stage Content Body -->
        <div class="stage-content">
          <div class="stage-top-meta">
            <div class="title-group">
              <span class="stage-subsystem">{{ stage.subsystem }}</span>
              <h4 class="stage-name">{{ stage.title }}</h4>
            </div>
            <div class="meta-badges">
              <span class="file-badge"><code>{{ stage.file }}</code></span>
              <span :class="['layer-pill', stage.theme]">{{ stage.layer }}</span>
            </div>
          </div>

          <p class="stage-summary">{{ stage.summary }}</p>

          <!-- Input / Output Data Transformation Strip -->
          <div class="io-strip">
            <div class="io-block input-block">
              <span class="io-lbl">IN:</span>
              <span class="io-val">{{ stage.input }}</span>
            </div>
            <div class="io-arrow">&rarr;</div>
            <div class="io-block output-block">
              <span class="io-lbl">OUT:</span>
              <span class="io-val">{{ stage.output }}</span>
            </div>
          </div>

          <!-- Internal Mechanics List -->
          <div class="mechanics-grid">
            <div v-for="(m, mIdx) in stage.mechanics" :key="mIdx" class="mechanic-item">
              <span class="mech-bullet">&bull;</span>
              <span class="mech-text"><strong>{{ m.label }}:</strong> {{ m.desc }}</span>
            </div>
          </div>

          <!-- Stage Code / Data Preview -->
          <div v-if="stage.code" class="stage-code-box">
            <div class="code-box-header">
              <span class="code-box-title">{{ stage.codeTitle }}</span>
              <span class="code-box-lang">C++23</span>
            </div>
            <pre class="code-snippet"><code>{{ stage.code }}</code></pre>
          </div>
        </div>
      </div>
    </div>

    <!-- Pipeline Footer Summary -->
    <div class="pipeline-footer">
      <div class="footer-left">
        <span class="footer-stat-lbl">End-to-End Latency:</span>
        <span class="footer-stat-val">{{ currentStmt.latency }}</span>
      </div>
      <div class="footer-right">
        <span class="footer-note">{{ currentStmt.engineNote }}</span>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed } from 'vue'

const activeStmtId = ref('select')
const executionMode = ref('fast')

const statements = [
  { id: 'select', label: 'SELECT Query', color: 'blue' },
  { id: 'insert', label: 'INSERT Tuple', color: 'emerald' },
  { id: 'update', label: 'UPDATE Row', color: 'amber' },
  { id: 'delete', label: 'DELETE Record', color: 'rose' },
  { id: 'create', label: 'CREATE TABLE', color: 'purple' },
  { id: 'drop', label: 'DROP TABLE', color: 'teal' }
]

const currentStmt = computed(() => {
  switch (activeStmtId.value) {
    case 'select':
      return {
        sql: executionMode.value === 'fast' 
          ? "SELECT users.name, orders.total_cents FROM users JOIN orders ON users.id = orders.user_id WHERE users.id = 42;"
          : "SELECT name, email FROM users WHERE email != '' AND role = 'admin';",
        strategyLabel: executionMode.value === 'fast' ? "⚡ Index Nested-Loop Join (O(log N))" : "🔄 Linear Slotted-Page Scan (O(N))",
        strategyType: executionMode.value === 'fast' ? "fast" : "scan",
        hasStrategyToggle: true,
        latency: executionMode.value === 'fast' ? "~120 μs (Point lookup + INLJ)" : "~350 μs (Multi-page scan)",
        engineNote: "In-memory reduction and index traversal avoid materializing intermediate disk tables."
      }
    case 'insert':
      return {
        sql: "INSERT INTO users (id, name, email) VALUES (1042, 'Grace Hopper', 'grace@navy.mil');",
        strategyLabel: "Two-Way Slotted-Page Allocation + B+ Tree Split",
        strategyType: "insert",
        hasStrategyToggle: false,
        latency: "~280 μs (Direct 4KB page seek + B+ tree insert)",
        engineNote: "Contiguous free space is checked before modifying memory; leaves split automatically at MAX_KEYS = 128."
      }
    case 'update':
      return {
        sql: executionMode.value === 'fast'
          ? "UPDATE users SET email = 'grace.hopper@defense.gov' WHERE id = 1042;"
          : "UPDATE orders SET status = 'SHIPPED' WHERE status = 'PAID';",
        strategyLabel: executionMode.value === 'fast' ? "⚡ Fast-Path In-Place Overwrite" : "🔄 Scan & Out-of-Place Re-Slotting",
        strategyType: executionMode.value === 'fast' ? "fast" : "scan",
        hasStrategyToggle: true,
        latency: "~240 μs (Point overwrite without index modification)",
        engineNote: "RecordID slot_index remains constant, preserving B+ tree leaf pointer validity."
      }
    case 'delete':
      return {
        sql: executionMode.value === 'fast'
          ? "DELETE FROM users WHERE id = 1042;"
          : "DELETE FROM orders WHERE status = 'CANCELLED';",
        strategyLabel: executionMode.value === 'fast' ? "⚡ Slot Tombstoning + B+ Tree Erase" : "🔄 Filtered Multi-Page Deletion",
        strategyType: executionMode.value === 'fast' ? "fast" : "scan",
        hasStrategyToggle: true,
        latency: "~290 μs (Tombstone set + B+ tree underflow rebalance)",
        engineNote: "Deletions zero the slot length and track fragmented_bytes until compact_page() runs."
      }
    case 'create':
      return {
        sql: "CREATE TABLE accounts (id INT PRIMARY KEY, balance INT NOT NULL, owner TEXT);",
        strategyLabel: "Atomic Catalog Registration (Page 0) + Tree Init",
        strategyType: "create",
        hasStrategyToggle: false,
        latency: "~450 μs (Allocates Page 1 Data, Page 2 Meta, Page 3 Root)",
        engineNote: "All table metadata persists into Page 0 (DB_MAGIC 0x31424451) for immediate recovery."
      }
    case 'drop':
      return {
        sql: "DROP TABLE staging_events;",
        strategyLabel: "DFS Page Recycling into Pager Freelist",
        strategyType: "drop",
        hasStrategyToggle: false,
        latency: "~180 μs (Immediate zero-copy freelist reclamation)",
        engineNote: "Deallocated pages are pushed directly to free_pages for instant zero-fragmentation reuse."
      }
    default:
      return {}
  }
})

const activeStages = computed(() => {
  if (activeStmtId.value === 'select') {
    if (executionMode.value === 'fast') {
      return [
        {
          step: '01',
          title: 'Lexical Token Scanner',
          layer: 'FRONTEND',
          subsystem: 'qdb::sql::Lexer',
          file: 'src/sql/lexer.cpp',
          theme: 'amber',
          summary: 'Scans the raw SQL input string into a typed vector of tokens, stripping whitespace and parsing literals.',
          input: 'std::string_view ("SELECT users.name...")',
          output: 'std::vector<Token> (19 tokens: KEYWORD, IDENT, DOT, EQUAL, NUMBER)',
          mechanics: [
            { label: 'Keyword Recognition', desc: 'Case-insensitive lookup identifies SELECT, JOIN, ON, WHERE' },
            { label: 'Identifier Qualification', desc: 'Scans multipart names (users.name, orders.total_cents)' },
            { label: 'Numeric Parsing', desc: 'Parses 42 into a signed 64-bit integer literal (std::int64_t)' }
          ],
          codeTitle: 'Token Structure',
          code: 'Token { type: TokenType::KeywordSelect, lexeme: "SELECT" }\nToken { type: TokenType::Identifier,    lexeme: "users.name" }\nToken { type: TokenType::Number,        lexeme: "42", value: 42LL }'
        },
        {
          step: '02',
          title: 'Recursive-Descent AST Construction',
          layer: 'PARSER',
          subsystem: 'qdb::sql::Parser',
          file: 'src/sql/parser.cpp',
          theme: 'purple',
          summary: 'Constructs the strongly-typed SelectStatement AST containing projections, JOIN clause, and binary expression tree.',
          input: 'std::vector<Token> token_stream',
          output: 'qdb::sql::SelectStatement AST node variant',
          mechanics: [
            { label: 'Projections AST', desc: 'Captures explicit projections ["users.name", "orders.total_cents"]' },
            { label: 'JoinClause Node', desc: 'Stores join_type: Inner, table: "orders", left: "users.id", right: "orders.user_id"' },
            { label: 'Where Expression', desc: 'Builds BinaryExpression { col: "users.id", op: Equal, val: 42 }' }
          ],
          codeTitle: 'Parsed AST Structure',
          code: 'SelectStatement {\n  table_name: "users",\n  columns: ["users.name", "orders.total_cents"],\n  joins: [ JoinClause { table: "orders", left: "users.id", right: "orders.user_id" } ],\n  where_clause: Expr(BinaryExpression("users.id", BinaryOp::Equal, 42))\n}'
        },
        {
          step: '03',
          title: 'Plan Routing & Fast-Path Optimization',
          layer: 'PLANNER',
          subsystem: 'qdb::execution::SQLExecutor',
          file: 'src/execution/executor.cpp',
          theme: 'emerald',
          summary: 'Inspects AST predicates to select the O(log N) index route and plans an Index Nested-Loop Join (INLJ).',
          input: 'SelectStatement & Catalog Schema',
          output: 'Physical Execution Plan: B+ Tree Descent + INLJ',
          mechanics: [
            { label: 'Fast-Path Detection', desc: 'Filter condition (users.id = 42) matches the primary key of users' },
            { label: 'Bypasses Table Scan', desc: 'Skips iterating all pages of users; directly queries PrimaryIndex' },
            { label: 'INLJ Optimization', desc: 'Join target orders.user_id is evaluated against candidate matches' }
          ]
        },
        {
          step: '04',
          title: 'B+ Tree Index Point Lookup',
          layer: 'STORAGE & INDEX',
          subsystem: 'qdb::PrimaryIndex & BplusTree',
          file: 'src/index/bplus_tree.cpp',
          theme: 'coral',
          summary: 'Navigates the disk-backed B+ tree root and internal pages to find key 42 in a leaf page with O(log N) reads.',
          input: 'Primary Key Search: 42 (std::int64_t)',
          output: 'RecordID { page_id: 1, slot_index: 0 }',
          mechanics: [
            { label: 'Root Read', desc: 'Reads Page 3 (Root); tests separator key (100) -> branches left to Page 5' },
            { label: 'Internal Descent', desc: 'Reads Page 5; tests separator key (50) -> branches left to Leaf Page 10' },
            { label: 'Leaf Extraction', desc: 'Binary search inside Leaf Page 10 locates key 42 -> returns RecordID {1, 0}' }
          ],
          codeTitle: 'Traversal Path',
          code: 'Page 3 (Root) -> key < 100 -> Page 5 (Internal) -> key < 50 -> Page 10 (Leaf)\nResult: Found key 42 -> RecordID { page_id: 1, slot_index: 0 }'
        },
        {
          step: '05',
          title: 'Slotted-Page Record Hydration & Join',
          layer: 'STORAGE & JOIN',
          subsystem: 'qdb::Table & RecordCodec',
          file: 'src/storage/table.cpp',
          theme: 'teal',
          summary: 'Hydrates the user record from Page 1, performs the index-assisted join with orders, and projects columns.',
          input: 'RecordID {1, 0} & orders table scan',
          output: 'std::vector<Record> (Joined output rows)',
          mechanics: [
            { label: 'Slot Dereference', desc: 'Reads slot 0 at byte offset 24 in Page 1 -> offset: 4020, len: 76' },
            { label: 'RecordCodec Unpack', desc: 'Unpacks name: "Grace Hopper", email: "grace@navy.mil"' },
            { label: 'Joined Tuple Emit', desc: 'Pairs with matching order row -> [{ "Grace Hopper", 15400 }]' }
          ]
        },
        {
          step: '06',
          title: 'QueryResult Buffer Packaging',
          layer: 'OUTPUT',
          subsystem: 'qdb::QueryResult',
          file: 'src/cli/main.cpp',
          theme: 'blue',
          summary: 'Encapsulates projected column names, matching rows, and elapsed execution telemetry into the final response.',
          input: 'Final Record vectors',
          output: 'qdb::QueryResult { success: true, rows: 1, columns: 2 }',
          mechanics: [
            { label: 'Column Headers', desc: 'Sets headers: ["users.name", "orders.total_cents"]' },
            { label: 'Formatting', desc: 'Interactive REPL prints tabular ASCII grid or delivers C++ vector' }
          ]
        }
      ]
    } else {
      // Linear Table Scan mode
      return [
        {
          step: '01',
          title: 'Lexical Token Scanner',
          layer: 'FRONTEND',
          subsystem: 'qdb::sql::Lexer',
          file: 'src/sql/lexer.cpp',
          theme: 'amber',
          summary: 'Scans query tokens, detecting comparison operators and string literals.',
          input: 'std::string_view ("SELECT name, email...")',
          output: 'std::vector<Token> (16 tokens)',
          mechanics: [
            { label: 'Operator Tokens', desc: 'Parses !=, AND, =' },
            { label: 'String Literals', desc: 'Parses \'admin\' and empty string \'\'' }
          ]
        },
        {
          step: '02',
          title: 'Recursive AST & Predicate Tree',
          layer: 'PARSER',
          subsystem: 'qdb::sql::Parser',
          file: 'src/sql/parser.cpp',
          theme: 'purple',
          summary: 'Builds LogicalExpression (AND) with two BinaryExpression leaves.',
          input: 'std::vector<Token>',
          output: 'SelectStatement with Expr(LogicalExpression)',
          mechanics: [
            { label: 'Precedence Parsing', desc: 'Binds AND expression between email filter and role filter' },
            { label: 'Variant Leaf Packaging', desc: 'Constructs BinaryExpression(email != \'\') & BinaryExpression(role == \'admin\')' }
          ]
        },
        {
          step: '03',
          title: 'Multi-Page Slotted Scan (O(N))',
          layer: 'STORAGE ENGINE',
          subsystem: 'qdb::Table::scan()',
          file: 'src/storage/table.cpp',
          theme: 'coral',
          summary: 'Traverses the singly-linked list of 4KB pages starting at first_page_, iterating all active slots.',
          input: 'first_page_ (Page 1) pointer',
          output: 'std::vector<Record> (All uncompacted table rows)',
          mechanics: [
            { label: 'Linked Page Walk', desc: 'Reads Page 1 -> next_page (Page 4) -> next_page (NO_PAGE)' },
            { label: 'Slot Array Unpacking', desc: 'Iterates slots 0..slot_count-1; ignores tombstones (length == 0)' },
            { label: 'Record Deserialization', desc: 'RecordCodec::deserialize() decodes typed values into Record structs' }
          ]
        },
        {
          step: '04',
          title: 'In-Memory Predicate Reduction',
          layer: 'EXECUTOR',
          subsystem: 'qdb::evaluate_expr()',
          file: 'src/execution/executor.cpp',
          theme: 'emerald',
          summary: 'Evaluates the recursive boolean expression tree against each candidate record in memory.',
          input: 'Candidate rows from table scan',
          output: 'matched_rows vector',
          mechanics: [
            { label: 'Short-Circuiting', desc: 'If left side of AND is false, right branch is skipped' },
            { label: 'Null Safety', desc: 'NULL values evaluated against comparisons return false' }
          ]
        },
        {
          step: '05',
          title: 'Projection Slicing & QueryResult',
          layer: 'OUTPUT',
          subsystem: 'qdb::QueryResult',
          file: 'src/execution/executor.cpp',
          theme: 'blue',
          summary: 'Slices matching rows to requested projection columns (name, email) and returns formatted output.',
          input: 'matched_rows vector',
          output: 'QueryResult with filtered tabular records',
          mechanics: [
            { label: 'Attribute Slicing', desc: 'Extracts indices [1, 2] from each row' },
            { label: 'Telemetry', desc: 'Records total scanned vs matched rows' }
          ]
        }
      ]
    }
  }

  if (activeStmtId.value === 'insert') {
    return [
      {
        step: '01',
        title: 'Tokenization & Column Reordering',
        layer: 'FRONTEND & PARSER',
        subsystem: 'qdb::sql::Parser',
        file: 'src/sql/parser.cpp',
        theme: 'amber',
        summary: 'Parses target table, optional column list, and VALUES tuple. Reorders fields into canonical schema sequence.',
        input: 'INSERT statement string',
        output: 'qdb::sql::InsertStatement',
        mechanics: [
          { label: 'Column Mapping', desc: 'Maps provided columns (id, name, email) to schema positions [0, 1, 2]' },
          { label: 'Value Validation', desc: 'Checks values count matches column count' }
        ]
      },
      {
        step: '02',
        title: 'Primary Key Uniqueness Verification',
        layer: 'INDEX GUARD',
        subsystem: 'qdb::PrimaryIndex',
        file: 'src/index/primary_index.cpp',
        theme: 'purple',
        summary: 'Performs pre-write uniqueness check against the disk B+ Tree before allocating any 4KB data pages.',
        input: 'Key: 1042 (std::int64_t)',
        output: 'Uniqueness confirmed (Key does not exist)',
        mechanics: [
          { label: 'Zero Dirty Pages', desc: 'If key already existed, aborts immediately with QERR_PK_DUPLICATE' },
          { label: 'Integrity Check', desc: 'Asserts primary key value is not NULL (std::monostate)' }
        ]
      },
      {
        step: '03',
        title: 'Binary Serialization (RecordCodec)',
        layer: 'STORAGE CODEC',
        subsystem: 'qdb::RecordCodec',
        file: 'src/storage/record.cpp',
        theme: 'emerald',
        summary: 'Packs typed variant values into a contiguous little-endian byte slice.',
        input: 'Record { id: 1042, name: "Grace Hopper", email: "grace@navy.mil" }',
        output: 'std::vector<std::byte> (72 Bytes binary payload)',
        mechanics: [
          { label: 'INT64 Encoding', desc: 'Tag 0x01 + 8 bytes little-endian integer' },
          { label: 'TEXT Encoding', desc: 'Tag 0x02 + 4 bytes length + UTF-8 string bytes' },
          { label: 'Payload Bound Guard', desc: 'Verifies payload length <= 4068 bytes' }
        ]
      },
      {
        step: '04',
        title: 'Two-Way Slotted-Page Allocation',
        layer: 'STORAGE ENGINE',
        subsystem: 'qdb::Table::insert()',
        file: 'src/storage/table.cpp',
        theme: 'coral',
        summary: 'Allocates space in the active 4KB data page using the two-way spatial growth layout.',
        input: '72 Bytes serialized payload',
        output: 'RecordID { page_id: 1, slot_index: 2 }',
        mechanics: [
          { label: 'Capacity Check', desc: 'Tests free_end - free_start >= 4B slot + 72B payload' },
          { label: 'Down-Growing Slot', desc: 'Writes Slot { offset: 3882, length: 72 } at free_start (offset 32)' },
          { label: 'Up-Growing Payload', desc: 'Copies 72 bytes backward from byte 3954 -> free_end becomes 3882' }
        ]
      },
      {
        step: '05',
        title: 'B+ Tree Index Insertion & Node Split',
        layer: 'INDEX ENGINE',
        subsystem: 'qdb::BplusTree::insert()',
        file: 'src/index/bplus_tree.cpp',
        theme: 'teal',
        summary: 'Inserts the new key and RecordID into the leaf page, splitting and promoting separator keys if capacity is reached.',
        input: 'Key 1042 & RecordID {1, 2}',
        output: 'B+ Tree updated on disk',
        mechanics: [
          { label: 'Sorted Insertion', desc: 'Maintains sorted order of keys inside the leaf' },
          { label: 'Leaf Split Check', desc: 'If keys > MAX_KEYS (128), allocates new leaf page and promotes median key' },
          { label: 'Horizontal Chaining', desc: 'Updates next_leaf pointer to link adjacent leaf blocks' }
        ]
      }
    ]
  }

  if (activeStmtId.value === 'update') {
    return [
      {
        step: '01',
        title: 'Target Column & Immutability Check',
        layer: 'FRONTEND',
        subsystem: 'qdb::execution::SQLExecutor',
        file: 'src/execution/executor.cpp',
        theme: 'amber',
        summary: 'Verifies target table and column exist. Validates that the primary key is not being modified.',
        input: 'UPDATE statement AST',
        output: 'Validated target column index',
        mechanics: [
          { label: 'Primary Key Protection', desc: 'Rejects updates to the primary key column (primary keys are immutable)' },
          { label: 'Type Compatibility', desc: 'Ensures new literal value matches target column data type' }
        ]
      },
      {
        step: '02',
        title: 'Record Location (Fast-Path vs Scan)',
        layer: 'PLANNER & ROUTER',
        subsystem: 'qdb::PrimaryIndex',
        file: 'src/index/primary_index.cpp',
        theme: 'purple',
        summary: 'Finds candidate row(s) via O(log N) B+ tree descent or linear table scan.',
        input: 'WHERE predicate',
        output: 'RecordID { page_id: 1, slot_index: 0 }',
        mechanics: [
          { label: 'Fast-Path Check', desc: 'WHERE id = 1042 fetches exact slot directory entry in 3 page reads' },
          { label: 'Slot Access', desc: 'Reads existing slot offset and length' }
        ]
      },
      {
        step: '03',
        title: 'In-Place vs Out-of-Place Re-Slotting',
        layer: 'STORAGE ENGINE',
        subsystem: 'qdb::Table::update_by_primary_key()',
        file: 'src/storage/table.cpp',
        theme: 'coral',
        summary: 'Decides physical rewrite strategy based on updated tuple byte length.',
        input: 'New serialized tuple bytes',
        output: 'Updated slotted 4KB page',
        mechanics: [
          { label: 'In-Place Overwrite', desc: 'If new_len <= old_len, overwrites existing memory directly' },
          { label: 'Re-Slotting Strategy', desc: 'If larger, writes to free_end, marks old bytes as fragmented_bytes' },
          { label: 'Index Stability', desc: 'slot_index never changes; RecordID remains permanently stable!' }
        ]
      },
      {
        step: '04',
        title: 'Disk Page Flush & QueryResult',
        layer: 'PAGER & OUTPUT',
        subsystem: 'qdb::pager',
        file: 'src/storage/pager.cpp',
        theme: 'emerald',
        summary: 'Flushes modified 4KB block back to disk and reports affected row count.',
        input: 'Page buffer',
        output: 'QueryResult: "1 row(s) updated."',
        mechanics: [
          { label: 'Direct Block Seek', desc: 'Writes 4096 bytes at seek offset page_id * 4096' },
          { label: 'Result Packaging', desc: 'Returns rows_affected: 1' }
        ]
      }
    ]
  }

  if (activeStmtId.value === 'delete') {
    return [
      {
        step: '01',
        title: 'Query Routing & Target Search',
        layer: 'ROUTING',
        subsystem: 'qdb::execution::SQLExecutor',
        file: 'src/execution/executor.cpp',
        theme: 'amber',
        summary: 'Evaluates WHERE clause to locate targeted record(s) via index fast-path or scan.',
        input: 'DELETE FROM users WHERE id = 1042',
        output: 'Target RecordID {1, 0}',
        mechanics: [
          { label: 'Fast-Path Point Lookup', desc: 'Primary index descends root-to-leaf in O(log N)' }
        ]
      },
      {
        step: '02',
        title: 'Slotted-Page Tombstoning',
        layer: 'STORAGE ENGINE',
        subsystem: 'qdb::Table::delete_by_primary_key()',
        file: 'src/storage/table.cpp',
        theme: 'rose',
        summary: 'Zeroes the slot length, creating an indirection tombstone without invalidating slot array positions.',
        input: 'Slot 0 in Page 1',
        output: 'slot.length = 0 (Tombstone)',
        mechanics: [
          { label: 'Tombstone Flag', desc: 'Sets length = 0; future scans immediately skip this slot' },
          { label: 'Fragment Tracking', desc: 'Adds old record length to page header fragmented_bytes' },
          { label: 'Array Preservation', desc: 'Slot remains in array to preserve RecordID addressing for other tuples' }
        ]
      },
      {
        step: '03',
        title: 'B+ Tree Index Key Erasure',
        layer: 'INDEX ENGINE',
        subsystem: 'qdb::BplusTree::erase()',
        file: 'src/index/bplus_tree.cpp',
        theme: 'purple',
        summary: 'Erases the key from the B+ tree leaf page and checks for node underflow.',
        input: 'Key: 1042',
        output: 'Key removed from leaf',
        mechanics: [
          { label: 'Underflow Check', desc: 'Tests if leaf keys count < MIN_KEYS (64)' },
          { label: 'Sibling Borrowing', desc: 'Borrows adjacent key from sibling if available' },
          { label: 'Node Merging', desc: 'If siblings cannot spare keys, merges adjacent nodes and frees page to freelist' }
        ]
      },
      {
        step: '04',
        title: 'Completion & Telemetry',
        layer: 'OUTPUT',
        subsystem: 'qdb::QueryResult',
        file: 'src/execution/executor.cpp',
        theme: 'emerald',
        summary: 'Flushes modified pages and returns execution summary.',
        input: 'Affected tuple counts',
        output: 'QueryResult: "1 row(s) deleted."',
        mechanics: [
          { label: 'Zero-Leak Reclaim', desc: 'Space is eligible for compaction on next compact_page() run' }
        ]
      }
    ]
  }

  if (activeStmtId.value === 'create') {
    return [
      {
        step: '01',
        title: 'Schema Invariant Validation',
        layer: 'DDL VALIDATOR',
        subsystem: 'qdb::Schema',
        file: 'src/storage/schema.cpp',
        theme: 'amber',
        summary: 'Validates table name uniqueness, ensures exactly one INT primary key is specified, and checks column types.',
        input: 'CREATE TABLE accounts (id INT PRIMARY KEY, balance INT NOT NULL, owner TEXT);',
        output: 'Validated Schema object',
        mechanics: [
          { label: 'Primary Key Rule', desc: 'Asserts exactly 1 column has primary_key = true (must be INT)' },
          { label: 'Duplicate Guard', desc: 'Rejects tables with duplicate column names or empty schemas' }
        ]
      },
      {
        step: '02',
        title: 'Page 1 Allocation: Table Data Root',
        layer: 'BLOCK PAGER',
        subsystem: 'qdb::pager::allocate_page()',
        file: 'src/storage/table.cpp',
        theme: 'emerald',
        summary: 'Allocates a new 4KB database page and initializes the slotted-page header.',
        input: 'Pager allocation request',
        output: 'PageID 1 (Table Data Root)',
        mechanics: [
          { label: 'Header Initialization', desc: 'Writes Magic: 0x31475051 (QPG1), version: 1' },
          { label: 'Pointers Setup', desc: 'free_start = 24, free_end = 4096, slot_count = 0, next_page = NO_PAGE' }
        ]
      },
      {
        step: '03',
        title: 'Page 2 & 3 Allocation: B+ Tree Hierarchy',
        layer: 'INDEX ENGINE',
        subsystem: 'qdb::BplusTree::create()',
        file: 'src/index/bplus_tree.cpp',
        theme: 'purple',
        summary: 'Allocates index metadata page (Page 2) and root node page (Page 3).',
        input: 'Pager allocation requests',
        output: 'Index Metadata & Root Node Initialized',
        mechanics: [
          { label: 'Metadata Page (Page 2)', desc: 'Writes Magic: 0x314D5042 (BPM1) and root pointer (3)' },
          { label: 'Root Page (Page 3)', desc: 'Writes Magic: 0x31545042 (BPT1), node_type: LEAF (1), key_count = 0' }
        ]
      },
      {
        step: '04',
        title: 'Catalog Page 0 Serialization',
        layer: 'SYSTEM CATALOG',
        subsystem: 'qdb::Database::persist_catalog()',
        file: 'src/quartz/database.cpp',
        theme: 'teal',
        summary: 'Serializes the table name, schema columns, data root (1), and index meta (2) into Catalog Page 0.',
        input: 'Table descriptor & Page 0 buffer',
        output: 'Page 0 flushed to disk (Magic 0x31424451)',
        mechanics: [
          { label: 'Catalog Compaction', desc: 'Appends entry after existing tables contiguous to offset 12' },
          { label: 'Table Count', desc: 'Increments catalog header table_count' },
          { label: 'Atomic Mount', desc: 'Survives database close and reload cycles permanently' }
        ]
      }
    ]
  }

  if (activeStmtId.value === 'drop') {
    return [
      {
        step: '01',
        title: 'Catalog Lookup & Table Deregistration',
        layer: 'CATALOG MANAGER',
        subsystem: 'qdb::Database::drop_table()',
        file: 'src/quartz/database.cpp',
        theme: 'amber',
        summary: 'Locates table entry in Page 0 catalog. Erases table descriptor and decrements table_count.',
        input: 'DROP TABLE staging_events;',
        output: 'Catalog entry removed',
        mechanics: [
          { label: 'Catalog Compaction', desc: 'Shifts subsequent table entries forward in Page 0 buffer' },
          { label: 'Safety Guard', desc: 'Returns QERR_TABLE_NOT_FOUND if table does not exist' }
        ]
      },
      {
        step: '02',
        title: 'Data Page Chain Deallocation',
        layer: 'STORAGE ENGINE',
        subsystem: 'qdb::Table::deallocate_all_pages()',
        file: 'src/storage/table.cpp',
        theme: 'rose',
        summary: 'Traverses the next_page linked list from first_page_, returning each PageID to the pager freelist.',
        input: 'first_page_ linked list',
        output: 'Data pages recycled',
        mechanics: [
          { label: 'Chain Walk', desc: 'Reads next_page pointer from header before deallocating block' },
          { label: 'Freelist Push', desc: 'Appends each PageID into pager.free_pages vector' }
        ]
      },
      {
        step: '03',
        title: 'B+ Tree Index DFS Page Recycling',
        layer: 'INDEX ENGINE',
        subsystem: 'qdb::BplusTree::deallocate_all_pages()',
        file: 'src/index/bplus_tree.cpp',
        theme: 'purple',
        summary: 'Executes post-order depth-first search to recycle all internal, leaf, and metadata index pages.',
        input: 'Index Root & Meta PageIDs',
        output: 'All index pages pushed to freelist',
        mechanics: [
          { label: 'Post-Order DFS', desc: 'Ensures child nodes are freed before parents to avoid dangling pointers' },
          { label: 'Metadata Deallocation', desc: 'Deallocates the BPM1 metadata page' }
        ]
      },
      {
        step: '04',
        title: 'Freelist Ready for Zero-Cost Reuse',
        layer: 'PAGER FREELIST',
        subsystem: 'qdb::pager',
        file: 'src/storage/pager.cpp',
        theme: 'emerald',
        summary: 'All freed pages are immediately available for subsequent CREATE TABLE or INSERT queries.',
        input: 'pager.free_pages vector',
        output: 'QueryResult: "Table dropped successfully."',
        mechanics: [
          { label: 'Zero File Inflation', desc: 'Subsequent allocations pop from freelist with O(1) efficiency' },
          { label: 'Zero Fragmentation', desc: 'Eliminates need for offline vacuuming' }
        ]
      }
    ]
  }

  return []
})
</script>

<style scoped>
.pipeline-stages-card {
  margin: 2.5rem 0;
  border-radius: 12px;
  border: 1px solid var(--vp-c-divider);
  background: var(--vp-c-bg-soft);
  box-shadow: 0 8px 30px rgba(0, 0, 0, 0.08);
  overflow: hidden;
  font-family: var(--vp-font-family-base);
}

.card-header {
  padding: 1.5rem 1.75rem 1.25rem;
  border-bottom: 1px solid var(--vp-c-divider);
  background: var(--vp-c-bg);
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  flex-wrap: wrap;
  gap: 1rem;
}

.header-titles {
  max-width: 650px;
}

.header-tag {
  display: inline-block;
  font-family: var(--vp-font-family-mono);
  font-size: 0.72rem;
  font-weight: 700;
  letter-spacing: 0.08em;
  color: #b83a24;
  background: rgba(184, 58, 36, 0.08);
  padding: 3px 8px;
  border-radius: 4px;
  margin-bottom: 0.4rem;
}

.dark .header-tag {
  color: #f87171;
  background: rgba(248, 113, 113, 0.12);
}

.header-title {
  margin: 0 0 0.4rem;
  font-family: 'Newsreader', Georgia, serif;
  font-size: 1.5rem;
  font-weight: 700;
  color: var(--vp-c-text-1);
}

.header-sub {
  margin: 0;
  font-size: 0.9rem;
  color: var(--vp-c-text-2);
  line-height: 1.45;
}

/* Statement Tabs */
.stmt-tabs {
  display: flex;
  flex-wrap: wrap;
  gap: 0.4rem;
}

.stmt-tab {
  display: inline-flex;
  align-items: center;
  gap: 0.4rem;
  padding: 6px 12px;
  font-family: var(--vp-font-family-mono);
  font-size: 0.78rem;
  font-weight: 600;
  border-radius: 6px;
  border: 1px solid var(--vp-c-divider);
  background: var(--vp-c-bg-soft);
  color: var(--vp-c-text-1);
  cursor: pointer;
  transition: all 0.15s ease;
}

.stmt-tab:hover {
  border-color: #b83a24;
  transform: translateY(-1px);
}

.stmt-tab.active {
  background: var(--vp-c-text-1);
  color: var(--vp-c-bg);
  border-color: var(--vp-c-text-1);
}

.dark .stmt-tab.active {
  background: #f4f4f5;
  color: #18181b;
  border-color: #f4f4f5;
}

.stmt-dot {
  width: 7px;
  height: 7px;
  border-radius: 50%;
}

.stmt-dot.blue { background: #38bdf8; }
.stmt-dot.emerald { background: #34d399; }
.stmt-dot.amber { background: #fbbf24; }
.stmt-dot.rose { background: #fb7185; }
.stmt-dot.purple { background: #c084fc; }
.stmt-dot.teal { background: #2dd4bf; }

/* Active Query Banner */
.active-query-banner {
  padding: 0.85rem 1.75rem;
  background: #18181b;
  border-bottom: 1px solid #27272a;
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 1rem;
}

.query-sql-wrap {
  display: flex;
  align-items: center;
  gap: 0.6rem;
  flex: 1;
  min-width: 300px;
}

.prompt-symbol {
  font-family: var(--vp-font-family-mono);
  font-size: 0.8rem;
  font-weight: 700;
  color: #fbbf24;
}

.query-sql-text {
  font-family: var(--vp-font-family-mono);
  font-size: 0.85rem;
  color: #f4f4f5;
  background: transparent;
  padding: 0;
}

.query-strategy-badge {
  font-family: var(--vp-font-family-mono);
  font-size: 0.72rem;
  font-weight: 600;
  padding: 4px 10px;
  border-radius: 6px;
  white-space: nowrap;
}

.query-strategy-badge.fast {
  background: rgba(52, 211, 153, 0.15);
  color: #34d399;
  border: 1px solid rgba(52, 211, 153, 0.3);
}

.query-strategy-badge.scan {
  background: rgba(251, 191, 36, 0.15);
  color: #fbbf24;
  border: 1px solid rgba(251, 191, 36, 0.3);
}

.query-strategy-badge.insert,
.query-strategy-badge.create,
.query-strategy-badge.drop {
  background: rgba(96, 165, 250, 0.15);
  color: #60a5fa;
  border: 1px solid rgba(96, 165, 250, 0.3);
}

/* Strategy Switcher Bar */
.strategy-toggle-bar {
  padding: 0.6rem 1.75rem;
  background: var(--vp-c-bg-mute);
  border-bottom: 1px solid var(--vp-c-divider);
  display: flex;
  align-items: center;
  gap: 0.6rem;
  flex-wrap: wrap;
}

.strat-label {
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  font-weight: 600;
  color: var(--vp-c-text-2);
}

.strat-btn {
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  padding: 3px 10px;
  border-radius: 5px;
  border: 1px solid var(--vp-c-divider);
  background: var(--vp-c-bg);
  color: var(--vp-c-text-2);
  cursor: pointer;
  transition: all 0.15s ease;
}

.strat-btn.active {
  background: #064e3b;
  color: #ecfdf5;
  border-color: #34d399;
  font-weight: 600;
}

/* Stages Timeline */
.stages-timeline {
  padding: 1.75rem;
  display: flex;
  flex-direction: column;
  gap: 1.25rem;
}

.stage-card {
  display: flex;
  gap: 1.25rem;
  position: relative;
}

.timeline-gutter {
  display: flex;
  flex-direction: column;
  align-items: center;
  width: 36px;
  flex-shrink: 0;
}

.step-circle {
  width: 34px;
  height: 34px;
  border-radius: 50%;
  background: var(--vp-c-bg);
  border: 2px solid var(--vp-c-divider);
  display: flex;
  align-items: center;
  justify-content: center;
  font-family: var(--vp-font-family-mono);
  font-size: 0.78rem;
  font-weight: 700;
  color: var(--vp-c-text-1);
  box-shadow: 0 2px 6px rgba(0, 0, 0, 0.08);
  z-index: 2;
}

.stage-card.amber .step-circle { border-color: #f59e0b; color: #d97706; }
.stage-card.purple .step-circle { border-color: #a855f7; color: #7c3aed; }
.stage-card.emerald .step-circle { border-color: #10b981; color: #059669; }
.stage-card.coral .step-circle { border-color: #f43f5e; color: #e11d48; }
.stage-card.teal .step-circle { border-color: #14b8a6; color: #0d9488; }
.stage-card.blue .step-circle { border-color: #3b82f6; color: #2563eb; }

.dark .stage-card.amber .step-circle { border-color: #fbbf24; color: #fbbf24; }
.dark .stage-card.purple .step-circle { border-color: #c084fc; color: #c084fc; }
.dark .stage-card.emerald .step-circle { border-color: #34d399; color: #34d399; }
.dark .stage-card.coral .step-circle { border-color: #fb7185; color: #fb7185; }
.dark .stage-card.teal .step-circle { border-color: #2dd4bf; color: #2dd4bf; }
.dark .stage-card.blue .step-circle { border-color: #60a5fa; color: #60a5fa; }

.step-stem {
  width: 2px;
  flex: 1;
  background: var(--vp-c-divider);
  margin-top: 6px;
  margin-bottom: -10px;
}

/* Stage Content */
.stage-content {
  flex: 1;
  border: 1px solid var(--vp-c-divider);
  border-radius: 8px;
  padding: 1.15rem 1.35rem;
  background: var(--vp-c-bg);
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.03);
  transition: all 0.15s ease;
}

.stage-content:hover {
  border-color: #b83a24;
  box-shadow: 0 4px 16px rgba(0, 0, 0, 0.06);
}

.dark .stage-content:hover {
  border-color: #f87171;
}

.stage-top-meta {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  flex-wrap: wrap;
  gap: 0.5rem;
  margin-bottom: 0.5rem;
}

.stage-subsystem {
  display: block;
  font-family: var(--vp-font-family-mono);
  font-size: 0.72rem;
  font-weight: 600;
  color: var(--vp-c-text-2);
  margin-bottom: 2px;
}

.stage-name {
  margin: 0;
  font-family: 'Newsreader', Georgia, serif;
  font-size: 1.2rem;
  font-weight: 700;
  color: var(--vp-c-text-1);
}

.meta-badges {
  display: flex;
  align-items: center;
  gap: 0.4rem;
}

.file-badge {
  font-size: 0.72rem;
  background: var(--vp-c-bg-soft);
  padding: 2px 6px;
  border-radius: 4px;
  border: 1px solid var(--vp-c-divider);
}

.layer-pill {
  font-family: var(--vp-font-family-mono);
  font-size: 0.65rem;
  font-weight: 700;
  padding: 2px 6px;
  border-radius: 4px;
}

.layer-pill.amber { background: rgba(245, 158, 11, 0.15); color: #d97706; }
.layer-pill.purple { background: rgba(168, 85, 247, 0.15); color: #7c3aed; }
.layer-pill.emerald { background: rgba(16, 185, 129, 0.15); color: #059669; }
.layer-pill.coral { background: rgba(244, 63, 94, 0.15); color: #e11d48; }
.layer-pill.teal { background: rgba(20, 184, 166, 0.15); color: #0d9488; }
.layer-pill.blue { background: rgba(59, 130, 246, 0.15); color: #2563eb; }

.stage-summary {
  margin: 0 0 0.85rem;
  font-size: 0.88rem;
  color: var(--vp-c-text-2);
  line-height: 1.45;
}

/* IO Strip */
.io-strip {
  display: flex;
  align-items: center;
  gap: 0.75rem;
  padding: 0.5rem 0.85rem;
  border-radius: 6px;
  background: var(--vp-c-bg-mute);
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  margin-bottom: 0.85rem;
  flex-wrap: wrap;
}

.io-block {
  display: flex;
  align-items: center;
  gap: 0.4rem;
}

.io-lbl {
  font-weight: 700;
  color: var(--vp-c-text-2);
}

.io-val {
  color: var(--vp-c-text-1);
}

.io-arrow {
  font-weight: 700;
  color: #b83a24;
}

.dark .io-arrow {
  color: #f87171;
}

/* Mechanics Grid */
.mechanics-grid {
  display: flex;
  flex-direction: column;
  gap: 0.35rem;
  margin-bottom: 0.85rem;
}

.mechanic-item {
  font-size: 0.82rem;
  color: var(--vp-c-text-2);
  display: flex;
  align-items: baseline;
  gap: 0.4rem;
  line-height: 1.4;
}

.mech-bullet {
  color: #b83a24;
  font-size: 1rem;
}

.dark .mech-bullet {
  color: #f87171;
}

.mech-text strong {
  color: var(--vp-c-text-1);
}

/* Code Snippet Box */
.stage-code-box {
  border-radius: 6px;
  overflow: hidden;
  border: 1px solid var(--vp-c-divider);
  background: #18181b;
}

.code-box-header {
  padding: 4px 10px;
  background: #27272a;
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-family: var(--vp-font-family-mono);
  font-size: 0.68rem;
  color: #a1a1aa;
}

.code-snippet {
  margin: 0;
  padding: 0.65rem 1rem;
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  line-height: 1.5;
  color: #f4f4f5;
  background: transparent;
  overflow-x: auto;
}

/* Footer Bar */
.pipeline-footer {
  padding: 0.85rem 1.75rem;
  background: var(--vp-c-bg);
  border-top: 1px solid var(--vp-c-divider);
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 0.75rem;
}

.footer-left {
  display: flex;
  align-items: center;
  gap: 0.5rem;
}

.footer-stat-lbl {
  font-family: var(--vp-font-family-mono);
  font-size: 0.75rem;
  font-weight: 700;
  color: #b83a24;
}

.dark .footer-stat-lbl {
  color: #f87171;
}

.footer-stat-val {
  font-family: var(--vp-font-family-mono);
  font-size: 0.8rem;
  font-weight: 600;
  color: var(--vp-c-text-1);
}

.footer-note {
  font-size: 0.78rem;
  color: var(--vp-c-text-2);
}
</style>
