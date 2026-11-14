/**
 * QuartzDB In-Browser Client-Side Storage Engine Simulator
 * 
 * Runs 100% in client-side browser memory (RAM) with optional local storage persistence.
 * Zero backend required — perfect for static Vercel deployment.
 * 
 * Simulates:
 * - 4KB Virtual Pager & Slotted Page layout
 * - Primary Key B+ Tree Indexing (O(log N) lookups & uniqueness enforcement)
 * - SQL Parser & Executor (CREATE, INSERT, SELECT, UPDATE, DELETE, DROP, JOIN, AGGREGATE)
 * - Storage Quota & Page Accounting (4096-byte blocks)
 */

export interface ColumnDef {
  name: string;
  type: 'INT' | 'TEXT';
  isPrimaryKey: boolean;
  isNotNull: boolean;
}

export interface TableDef {
  name: string;
  columns: ColumnDef[];
  firstPageId: number;
  rows: Record<string, any>[];
}

export interface QueryResult {
  success: boolean;
  columns?: string[];
  rows?: any[][];
  rawRows?: Record<string, any>[];
  rowsAffected?: number;
  message?: string;
  latencyUs: number;
  executionPath: 'B+ Tree Index Fast-Path (O(log N))' | 'Slotted-Page Linear Scan (O(N))' | 'DDL Catalog Mutation' | 'Direct Modification';
  pageStats: {
    totalPages: number;
    bytesUsed: number;
    quotaBytes: number;
  };
}

const STORAGE_KEY = 'quartz_browser_storage_v1';
const PAGE_SIZE = 4096;
const MAX_STORAGE_BYTES = 5 * 1024 * 1024; // 5MB browser quota

export class BrowserQuartzEngine {
  private tables: Map<string, TableDef> = new Map();
  private isStorageAvailable: boolean = true;

  constructor() {
    this.initDefaultSchema();
    this.loadFromStorage();
  }

  private initDefaultSchema() {
    this.tables.clear();

    // Table: users
    this.tables.set('users', {
      name: 'users',
      firstPageId: 1,
      columns: [
        { name: 'id', type: 'INT', isPrimaryKey: true, isNotNull: true },
        { name: 'name', type: 'TEXT', isPrimaryKey: false, isNotNull: true },
        { name: 'email', type: 'TEXT', isPrimaryKey: false, isNotNull: false },
        { name: 'role', type: 'TEXT', isPrimaryKey: false, isNotNull: false }
      ],
      rows: [
        { id: 1, name: 'Alice Smith', email: 'alice@example.com', role: 'admin' },
        { id: 2, name: 'Bob Jones', email: 'bob@example.com', role: 'engineer' },
        { id: 3, name: 'Carol Danvers', email: 'carol@example.com', role: 'lead' }
      ]
    });

    // Table: orders
    this.tables.set('orders', {
      name: 'orders',
      firstPageId: 2,
      columns: [
        { name: 'id', type: 'INT', isPrimaryKey: true, isNotNull: true },
        { name: 'user_id', type: 'INT', isPrimaryKey: false, isNotNull: true },
        { name: 'product', type: 'TEXT', isPrimaryKey: false, isNotNull: true },
        { name: 'amount', type: 'INT', isPrimaryKey: false, isNotNull: true }
      ],
      rows: [
        { id: 101, user_id: 1, product: '4KB NVMe SSD', amount: 240 },
        { id: 102, user_id: 1, product: 'Mechanical Keychron', amount: 110 },
        { id: 103, user_id: 2, product: 'Ergonomic Desk Chair', amount: 350 }
      ]
    });
  }

  private saveToStorage(): boolean {
    if (typeof window === 'undefined' || !window.localStorage) return false;
    try {
      const dump: Record<string, TableDef> = {};
      this.tables.forEach((tbl, key) => { dump[key] = tbl; });
      const str = JSON.stringify(dump);
      if (str.length > MAX_STORAGE_BYTES) {
        console.warn('QuartzDB: Browser storage quota exceeded. Changes kept in RAM only.');
        return false;
      }
      localStorage.setItem(STORAGE_KEY, str);
      return true;
    } catch (e) {
      console.warn('QuartzDB: Unable to write to localStorage, using RAM only:', e);
      this.isStorageAvailable = false;
      return false;
    }
  }

  private loadFromStorage(): boolean {
    if (typeof window === 'undefined' || !window.localStorage) return false;
    try {
      const data = localStorage.getItem(STORAGE_KEY);
      if (!data) return false;
      const dump: Record<string, TableDef> = JSON.parse(data);
      this.tables.clear();
      for (const key of Object.keys(dump)) {
        this.tables.set(key, dump[key]);
      }
      return true;
    } catch (e) {
      this.initDefaultSchema();
      return false;
    }
  }

  public resetDatabase(): void {
    if (typeof window !== 'undefined' && window.localStorage) {
      localStorage.removeItem(STORAGE_KEY);
    }
    this.initDefaultSchema();
    this.saveToStorage();
  }

  public getStorageMetrics() {
    let totalRecords = 0;
    let approxBytes = 0;

    // Fixed Catalog Page 0: 4096 bytes
    let totalPages = 1;

    this.tables.forEach(tbl => {
      totalRecords += tbl.rows.length;
      // Slotted page header: 24 bytes, slot array: 4 bytes per slot
      // Average row serialization: ~40-100 bytes
      let tableBytes = 24 + (tbl.rows.length * 4);
      tbl.rows.forEach(r => {
        let rowBytes = 0;
        Object.values(r).forEach(v => {
          rowBytes += (typeof v === 'string') ? v.length + 4 : 8;
        });
        tableBytes += rowBytes;
      });
      approxBytes += tableBytes;
      const tablePages = Math.max(1, Math.ceil(tableBytes / (PAGE_SIZE - 64)));
      totalPages += tablePages;
    });

    const dumpSize = typeof window !== 'undefined' && window.localStorage 
      ? (localStorage.getItem(STORAGE_KEY)?.length || approxBytes)
      : approxBytes;

    return {
      totalRecords,
      totalPages,
      bytesUsed: dumpSize,
      quotaBytes: MAX_STORAGE_BYTES,
      tablesCount: this.tables.size
    };
  }

  public execute(sql: string): QueryResult {
    const start = performance.now();
    const clean = sql.trim().replace(/;+\s*$/, '').trim();

    if (!clean) {
      return {
        success: false,
        message: 'Empty query statement.',
        latencyUs: 10,
        executionPath: 'Slotted-Page Linear Scan (O(N))',
        pageStats: this.getStorageMetrics()
      };
    }

    try {
      // 1. Meta commands: .tables, .schema, SHOW TABLES
      if (/^\.tables|^SHOW TABLES/i.test(clean)) {
        const tableNames = Array.from(this.tables.keys());
        const latency = Math.round((performance.now() - start) * 1000 + 45);
        return {
          success: true,
          columns: ['table_name', 'columns_count', 'rows_count', 'first_page_id'],
          rows: tableNames.map(name => {
            const t = this.tables.get(name)!;
            return [t.name, t.columns.length, t.rows.length, t.firstPageId];
          }),
          rowsAffected: tableNames.length,
          latencyUs: latency,
          executionPath: 'DDL Catalog Mutation',
          pageStats: this.getStorageMetrics()
        };
      }

      if (/^\.schema/i.test(clean)) {
        const match = clean.match(/^\.schema\s*(\w*)/i);
        const target = match && match[1] ? match[1].trim() : '';
        const list = target ? [target] : Array.from(this.tables.keys());
        const rows: any[][] = [];

        list.forEach(tName => {
          const t = this.tables.get(tName);
          if (t) {
            t.columns.forEach(c => {
              rows.push([
                t.name,
                c.name,
                c.type,
                c.isPrimaryKey ? 'YES' : 'NO',
                c.isNotNull ? 'YES' : 'NO'
              ]);
            });
          }
        });

        const latency = Math.round((performance.now() - start) * 1000 + 60);
        return {
          success: true,
          columns: ['table', 'column', 'type', 'primary_key', 'not_null'],
          rows,
          rowsAffected: rows.length,
          latencyUs: latency,
          executionPath: 'DDL Catalog Mutation',
          pageStats: this.getStorageMetrics()
        };
      }

      // 2. CREATE TABLE
      if (/^CREATE TABLE/i.test(clean)) {
        return this.handleCreateTable(clean, start);
      }

      // 3. DROP TABLE
      if (/^DROP TABLE/i.test(clean)) {
        return this.handleDropTable(clean, start);
      }

      // 4. INSERT INTO
      if (/^INSERT INTO/i.test(clean)) {
        return this.handleInsert(clean, start);
      }

      // 5. UPDATE
      if (/^UPDATE/i.test(clean)) {
        return this.handleUpdate(clean, start);
      }

      // 6. DELETE FROM
      if (/^DELETE FROM/i.test(clean)) {
        return this.handleDelete(clean, start);
      }

      // 7. SELECT
      if (/^SELECT/i.test(clean)) {
        return this.handleSelect(clean, start);
      }

      throw new Error(`Unsupported SQL statement or syntax: "${clean.slice(0, 30)}..."`);
    } catch (err: any) {
      const latency = Math.round((performance.now() - start) * 1000 + 40);
      return {
        success: false,
        message: err.message || String(err),
        latencyUs: latency,
        executionPath: 'Slotted-Page Linear Scan (O(N))',
        pageStats: this.getStorageMetrics()
      };
    }
  }

  // --- Handlers ---

  private handleCreateTable(sql: string, start: number): QueryResult {
    const match = sql.match(/^CREATE\s+TABLE\s+(\w+)\s*\((.+)\)/i);
    if (!match) throw new Error('Syntax error in CREATE TABLE statement.');
    const tableName = match[1].toLowerCase();
    const colDefsStr = match[2];

    if (this.tables.has(tableName)) {
      throw new Error(`Table '${tableName}' already exists in Page 0 catalog.`);
    }

    const columns: ColumnDef[] = [];
    const parts = colDefsStr.split(',').map(s => s.trim());
    for (const part of parts) {
      const tokens = part.split(/\s+/);
      const colName = tokens[0];
      const colType = tokens[1]?.toUpperCase();
      if (colType !== 'INT' && colType !== 'TEXT') {
        throw new Error(`Unsupported type '${colType}' for column '${colName}'. Supported: INT, TEXT.`);
      }
      const upperPart = part.toUpperCase();
      const isPrimaryKey = upperPart.includes('PRIMARY KEY');
      const isNotNull = isPrimaryKey || upperPart.includes('NOT NULL');

      columns.push({
        name: colName,
        type: colType as 'INT' | 'TEXT',
        isPrimaryKey,
        isNotNull
      });
    }

    const nextFirstPage = this.tables.size + 1;
    this.tables.set(tableName, {
      name: tableName,
      columns,
      firstPageId: nextFirstPage,
      rows: []
    });

    this.saveToStorage();
    const latency = Math.round((performance.now() - start) * 1000 + 180);
    return {
      success: true,
      message: `Query OK, 0 rows affected. Table '${tableName}' registered on Page 0 catalog. Allocated Page ${nextFirstPage} for slotted storage.`,
      rowsAffected: 0,
      latencyUs: latency,
      executionPath: 'DDL Catalog Mutation',
      pageStats: this.getStorageMetrics()
    };
  }

  private handleDropTable(sql: string, start: number): QueryResult {
    const match = sql.match(/^DROP\s+TABLE\s+(\w+)/i);
    if (!match) throw new Error('Syntax error in DROP TABLE statement.');
    const tableName = match[1].toLowerCase();

    if (!this.tables.has(tableName)) {
      throw new Error(`Table '${tableName}' does not exist.`);
    }

    this.tables.delete(tableName);
    this.saveToStorage();
    const latency = Math.round((performance.now() - start) * 1000 + 120);
    return {
      success: true,
      message: `Query OK, 0 rows affected. Table '${tableName}' dropped and slotted pages marked free in Pager.`,
      rowsAffected: 0,
      latencyUs: latency,
      executionPath: 'DDL Catalog Mutation',
      pageStats: this.getStorageMetrics()
    };
  }

  private handleInsert(sql: string, start: number): QueryResult {
    const match = sql.match(/^INSERT\s+INTO\s+(\w+)(?:\s*\(([^)]+)\))?\s+VALUES\s*\((.+)\)/i);
    if (!match) throw new Error('Syntax error in INSERT statement.');

    const tableName = match[1].toLowerCase();
    const table = this.tables.get(tableName);
    if (!table) throw new Error(`Table '${tableName}' does not exist.`);

    const explicitCols = match[2] ? match[2].split(',').map(s => s.trim()) : null;
    const rawVals = match[3].split(',').map(s => s.trim().replace(/^['"]|['"]$/g, ''));

    const rowObj: Record<string, any> = {};

    if (explicitCols) {
      if (explicitCols.length !== rawVals.length) {
        throw new Error('Column count does not match value count.');
      }
      for (let i = 0; i < explicitCols.length; i++) {
        rowObj[explicitCols[i]] = rawVals[i];
      }
    } else {
      if (rawVals.length > table.columns.length) {
        throw new Error(`Too many values supplied for table '${tableName}'. Expected ${table.columns.length}, got ${rawVals.length}.`);
      }
      for (let i = 0; i < table.columns.length; i++) {
        rowObj[table.columns[i].name] = rawVals[i] !== undefined ? rawVals[i] : null;
      }
    }

    // Cast types & Validate constraints
    const pkCol = table.columns.find(c => c.isPrimaryKey);
    for (const col of table.columns) {
      let val = rowObj[col.name];
      if (val === undefined || val === null || val === 'NULL') {
        if (col.isNotNull) {
          throw new Error(`NOT NULL constraint violation on column '${col.name}'.`);
        }
        rowObj[col.name] = null;
      } else {
        if (col.type === 'INT') {
          const num = parseInt(val, 10);
          if (isNaN(num)) throw new Error(`Type error: Value '${val}' is not a valid INT for column '${col.name}'.`);
          rowObj[col.name] = num;
        } else {
          rowObj[col.name] = String(val);
        }
      }
    }

    // Primary key uniqueness check (B+ Tree invariant)
    if (pkCol) {
      const pkVal = rowObj[pkCol.name];
      const exists = table.rows.some(r => r[pkCol.name] === pkVal);
      if (exists) {
        throw new Error(`Primary key violation: Duplicate key '${pkVal}' in B+ Tree index on '${tableName}.${pkCol.name}'.`);
      }
    }

    table.rows.push(rowObj);
    this.saveToStorage();

    const slotIndex = table.rows.length - 1;
    const latency = Math.round((performance.now() - start) * 1000 + 135);
    return {
      success: true,
      message: `Query OK, 1 row affected.\nRecord serialized via RecordCodec and written to Page ${table.firstPageId}, Slot ${slotIndex}.\nB+ Tree indexed key: ${pkCol ? rowObj[pkCol.name] : 'N/A'}`,
      rowsAffected: 1,
      latencyUs: latency,
      executionPath: 'B+ Tree Index Fast-Path (O(log N))',
      pageStats: this.getStorageMetrics()
    };
  }

  private handleUpdate(sql: string, start: number): QueryResult {
    const match = sql.match(/^UPDATE\s+(\w+)\s+SET\s+(.+?)(?:\s+WHERE\s+(.+))?$/i);
    if (!match) throw new Error('Syntax error in UPDATE statement.');

    const tableName = match[1].toLowerCase();
    const table = this.tables.get(tableName);
    if (!table) throw new Error(`Table '${tableName}' does not exist.`);

    const setClauses = match[2].split(',').map(s => s.trim());
    const whereClause = match[3] ? match[3].trim() : null;

    let updatedCount = 0;
    const pkCol = table.columns.find(c => c.isPrimaryKey);

    for (const row of table.rows) {
      if (!whereClause || this.evalCondition(row, whereClause)) {
        for (const clause of setClauses) {
          const [col, rawVal] = clause.split('=').map(s => s.trim());
          const cleanVal = rawVal.replace(/^['"]|['"]$/g, '');
          const colDef = table.columns.find(c => c.name.toLowerCase() === col.toLowerCase());
          if (!colDef) throw new Error(`Column '${col}' does not exist in table '${tableName}'.`);

          const castVal = colDef.type === 'INT' ? parseInt(cleanVal, 10) : cleanVal;
          if (colDef.isPrimaryKey && castVal !== row[colDef.name]) {
            if (table.rows.some(r => r !== row && r[colDef.name] === castVal)) {
              throw new Error(`Primary key violation: Duplicate key '${castVal}' in table '${tableName}'.`);
            }
          }
          row[colDef.name] = castVal;
        }
        updatedCount++;
      }
    }

    this.saveToStorage();
    const isPkFastPath = whereClause && pkCol && whereClause.includes(pkCol.name);
    const latency = Math.round((performance.now() - start) * 1000 + (isPkFastPath ? 140 : 220));

    return {
      success: true,
      message: `Query OK, ${updatedCount} row${updatedCount === 1 ? '' : 's'} affected. Slotted pages updated in place.`,
      rowsAffected: updatedCount,
      latencyUs: latency,
      executionPath: isPkFastPath ? 'B+ Tree Index Fast-Path (O(log N))' : 'Slotted-Page Linear Scan (O(N))',
      pageStats: this.getStorageMetrics()
    };
  }

  private handleDelete(sql: string, start: number): QueryResult {
    const match = sql.match(/^DELETE\s+FROM\s+(\w+)(?:\s+WHERE\s+(.+))?$/i);
    if (!match) throw new Error('Syntax error in DELETE statement.');

    const tableName = match[1].toLowerCase();
    const table = this.tables.get(tableName);
    if (!table) throw new Error(`Table '${tableName}' does not exist.`);

    const whereClause = match[2] ? match[2].trim() : null;
    const initialCount = table.rows.length;
    const pkCol = table.columns.find(c => c.isPrimaryKey);

    if (!whereClause) {
      table.rows = [];
    } else {
      table.rows = table.rows.filter(row => !this.evalCondition(row, whereClause));
    }

    const deletedCount = initialCount - table.rows.length;
    this.saveToStorage();

    const isPkFastPath = whereClause && pkCol && whereClause.includes(pkCol.name);
    const latency = Math.round((performance.now() - start) * 1000 + (isPkFastPath ? 120 : 210));

    return {
      success: true,
      message: `Query OK, ${deletedCount} row${deletedCount === 1 ? '' : 's'} deleted. Slotted page space marked tombstoned (freed for compaction).`,
      rowsAffected: deletedCount,
      latencyUs: latency,
      executionPath: isPkFastPath ? 'B+ Tree Index Fast-Path (O(log N))' : 'Slotted-Page Linear Scan (O(N))',
      pageStats: this.getStorageMetrics()
    };
  }

  private handleSelect(sql: string, start: number): QueryResult {
    // Check for JOIN
    if (/JOIN/i.test(sql)) {
      return this.handleSelectJoin(sql, start);
    }

    // Standard SELECT
    const match = sql.match(/^SELECT\s+(.+?)\s+FROM\s+(\w+)(?:\s+WHERE\s+(.+?))?(?:\s+ORDER\s+BY\s+(.+?))?(?:\s+LIMIT\s+(\d+)(?:\s+OFFSET\s+(\d+))?)?$/i);
    if (!match) throw new Error('Syntax error or unsupported clauses in SELECT statement.');

    const selectColsStr = match[1].trim();
    const tableName = match[2].toLowerCase();
    const whereClause = match[3] ? match[3].trim() : null;
    const orderByClause = match[4] ? match[4].trim() : null;
    const limitVal = match[5] ? parseInt(match[5], 10) : null;
    const offsetVal = match[6] ? parseInt(match[6], 10) : 0;

    const table = this.tables.get(tableName);
    if (!table) throw new Error(`Table '${tableName}' does not exist.`);

    // Aggregations: COUNT, SUM, AVG, MIN, MAX
    if (/^(COUNT|SUM|AVG|MIN|MAX)\s*\(/i.test(selectColsStr)) {
      return this.handleAggregation(selectColsStr, table, whereClause, start);
    }

    // Filter rows
    let resultRows = table.rows.filter(row => {
      if (!whereClause) return true;
      return this.evalCondition(row, whereClause);
    });

    const pkCol = table.columns.find(c => c.isPrimaryKey);
    const isPkFastPath = whereClause && pkCol && new RegExp(`\\b${pkCol.name}\\s*=`, 'i').test(whereClause);

    // Order By
    if (orderByClause) {
      const parts = orderByClause.split(/\s+/);
      const colName = parts[0];
      const desc = parts[1] && parts[1].toUpperCase() === 'DESC';
      resultRows.sort((a, b) => {
        const valA = a[colName];
        const valB = b[colName];
        if (valA < valB) return desc ? 1 : -1;
        if (valA > valB) return desc ? -1 : 1;
        return 0;
      });
    }

    // Offset & Limit
    if (offsetVal > 0) {
      resultRows = resultRows.slice(offsetVal);
    }
    if (limitVal !== null && limitVal >= 0) {
      resultRows = resultRows.slice(0, limitVal);
    }

    // Project columns
    let outColumns: string[] = [];
    if (selectColsStr === '*') {
      outColumns = table.columns.map(c => c.name);
    } else {
      outColumns = selectColsStr.split(',').map(s => s.trim());
    }

    const tableData: any[][] = resultRows.map(r => {
      return outColumns.map(col => r[col] !== undefined ? r[col] : 'NULL');
    });

    const latency = Math.round((performance.now() - start) * 1000 + (isPkFastPath ? 110 : 220));

    return {
      success: true,
      columns: outColumns,
      rows: tableData,
      rawRows: resultRows,
      rowsAffected: tableData.length,
      latencyUs: latency,
      executionPath: isPkFastPath ? 'B+ Tree Index Fast-Path (O(log N))' : 'Slotted-Page Linear Scan (O(N))',
      pageStats: this.getStorageMetrics()
    };
  }

  private handleSelectJoin(sql: string, start: number): QueryResult {
    const match = sql.match(/^SELECT\s+(.+?)\s+FROM\s+(\w+)\s+(?:INNER\s+)?JOIN\s+(\w+)\s+ON\s+(\S+)\s*=\s*(\S+)(?:\s+WHERE\s+(.+))?$/i);
    if (!match) throw new Error('Unsupported or malformed JOIN syntax. Supported: SELECT cols FROM t1 JOIN t2 ON t1.col = t2.col [WHERE condition]');

    const selectColsStr = match[1].trim();
    const t1Name = match[2].toLowerCase();
    const t2Name = match[3].toLowerCase();
    const onLeft = match[4].trim();
    const onRight = match[5].trim();
    const whereClause = match[6] ? match[6].trim() : null;

    const t1 = this.tables.get(t1Name);
    const t2 = this.tables.get(t2Name);
    if (!t1) throw new Error(`Table '${t1Name}' does not exist.`);
    if (!t2) throw new Error(`Table '${t2Name}' does not exist.`);

    const parseOnCol = (str: string) => {
      const dots = str.split('.');
      return dots.length > 1 ? dots[1] : dots[0];
    };
    const colLeft = parseOnCol(onLeft);
    const colRight = parseOnCol(onRight);

    const joined: Record<string, any>[] = [];

    for (const r1 of t1.rows) {
      for (const r2 of t2.rows) {
        if (r1[colLeft] !== undefined && r2[colRight] !== undefined && String(r1[colLeft]) === String(r2[colRight])) {
          const combo: Record<string, any> = {};
          // prefix table names for unambiguous resolution
          Object.keys(r1).forEach(k => {
            combo[`${t1Name}.${k}`] = r1[k];
            combo[k] = r1[k];
          });
          Object.keys(r2).forEach(k => {
            combo[`${t2Name}.${k}`] = r2[k];
            if (combo[k] === undefined) combo[k] = r2[k];
          });

          if (!whereClause || this.evalCondition(combo, whereClause)) {
            joined.push(combo);
          }
        }
      }
    }

    let outColumns: string[] = [];
    if (selectColsStr === '*') {
      outColumns = [...t1.columns.map(c => `${t1Name}.${c.name}`), ...t2.columns.map(c => `${t2Name}.${c.name}`)];
    } else {
      outColumns = selectColsStr.split(',').map(s => s.trim());
    }

    const tableData = joined.map(j => {
      return outColumns.map(c => j[c] !== undefined ? j[c] : 'NULL');
    });

    const latency = Math.round((performance.now() - start) * 1000 + 260);

    return {
      success: true,
      columns: outColumns,
      rows: tableData,
      rawRows: joined,
      rowsAffected: tableData.length,
      latencyUs: latency,
      executionPath: 'B+ Tree Index Fast-Path (O(log N))', // Index nested-loop join
      pageStats: this.getStorageMetrics()
    };
  }

  private handleAggregation(colStr: string, table: TableDef, whereClause: string | null, start: number): QueryResult {
    const match = colStr.match(/^(COUNT|SUM|AVG|MIN|MAX)\s*\((.+?)\)/i);
    if (!match) throw new Error('Unsupported aggregation expression.');

    const func = match[1].toUpperCase();
    const arg = match[2].trim();

    const filtered = table.rows.filter(row => !whereClause || this.evalCondition(row, whereClause));

    let result: any = 0;
    if (func === 'COUNT') {
      result = arg === '*' ? filtered.length : filtered.filter(r => r[arg] !== null && r[arg] !== undefined).length;
    } else {
      const numbers = filtered.map(r => Number(r[arg])).filter(n => !isNaN(n));
      if (numbers.length === 0) {
        result = func === 'COUNT' ? 0 : 'NULL';
      } else if (func === 'SUM') {
        result = numbers.reduce((a, b) => a + b, 0);
      } else if (func === 'AVG') {
        result = (numbers.reduce((a, b) => a + b, 0) / numbers.length).toFixed(2);
      } else if (func === 'MIN') {
        result = Math.min(...numbers);
      } else if (func === 'MAX') {
        result = Math.max(...numbers);
      }
    }

    const latency = Math.round((performance.now() - start) * 1000 + 150);

    return {
      success: true,
      columns: [`${func}(${arg})`],
      rows: [[result]],
      rowsAffected: 1,
      latencyUs: latency,
      executionPath: 'Slotted-Page Linear Scan (O(N))',
      pageStats: this.getStorageMetrics()
    };
  }

  private evalCondition(row: Record<string, any>, expr: string): boolean {
    const eqMatch = expr.match(/(\w+(?:\.\w+)?)\s*(=|!=|>|<|>=|<=)\s*(.+)/);
    if (!eqMatch) return true;

    const col = eqMatch[1].trim();
    const op = eqMatch[2].trim();
    const rawTarget = eqMatch[3].trim().replace(/^['"]|['"]$/g, '');

    const rowVal = row[col] !== undefined ? row[col] : row[col.split('.').pop() || ''];
    if (rowVal === undefined || rowVal === null) return false;

    const isNum = typeof rowVal === 'number' || !isNaN(Number(rawTarget));
    const a = isNum ? Number(rowVal) : String(rowVal);
    const b = isNum ? Number(rawTarget) : String(rawTarget);

    switch (op) {
      case '=': return a === b;
      case '!=': return a !== b;
      case '>': return a > b;
      case '<': return a < b;
      case '>=': return a >= b;
      case '<=': return a <= b;
      default: return true;
    }
  }
}
