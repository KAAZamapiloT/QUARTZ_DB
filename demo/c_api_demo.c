#include <quartz/quartz.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

/* Callback function for SQLite-style streaming query execution */
static int print_row_callback(void* user_data, int col_count, char** col_values, char** col_names) {
    int* row_counter = (int*)user_data;
    (*row_counter)++;

    printf("  [Row %d] ", *row_counter);
    for (int i = 0; i < col_count; ++i) {
        printf("%s: %s%s",
               col_names[i],
               col_values[i] ? col_values[i] : "NULL",
               (i + 1 < col_count) ? " | " : "");
    }
    printf("\n");
    return 0; /* Return 0 to continue reading rows */
}

int main(void) {
    printf("===================================================================\n");
    printf("  QuartzDB Embedded C Library (libquartz) Demonstration\n");
    printf("  Engine Version: %s\n", qdb_version());
    printf("===================================================================\n\n");

    const char* db_file = "demo_embedded.db";
    qdb_database_t* db = NULL;

    /* 1. Open / Initialize Database File */
    printf("[1/6] Opening database file: %s ...\n", db_file);
    int rc = qdb_open(db_file, &db);
    if (rc != QDB_OK || !db) {
        fprintf(stderr, "Failed to open database: %s\n", qdb_errmsg(db));
        return 1;
    }
    printf("      Database opened successfully.\n\n");

    /* 2. Create Schema */
    printf("[2/6] Creating 'products' table...\n");
    qdb_result_t* res = NULL;
    rc = qdb_execute(db,
        "CREATE TABLE products ("
        "  id INT PRIMARY KEY,"
        "  name TEXT NOT NULL,"
        "  price INT,"
        "  stock INT"
        ");",
        &res);

    if (rc != QDB_OK) {
        printf("      Notice: %s (table may already exist from previous run).\n", qdb_result_message(res));
    } else {
        printf("      Table 'products' created successfully.\n");
    }
    qdb_result_free(res);
    printf("\n");

    /* 3. Insert Records */
    printf("[3/6] Inserting sample records...\n");
    const char* inserts[] = {
        "INSERT INTO products VALUES (1, 'Mechanical Keyboard', 120, 45);",
        "INSERT INTO products VALUES (2, 'Ergonomic Mouse', 65, 80);",
        "INSERT INTO products VALUES (3, '4K Ultrawide Monitor', 450, 15);",
        "INSERT INTO products VALUES (4, 'USB-C Docking Station', 90, 60);",
        "INSERT INTO products VALUES (5, 'Noise-Cancelling Headset', 180, NULL);"
    };
    const size_t num_inserts = sizeof(inserts) / sizeof(inserts[0]);

    for (size_t i = 0; i < num_inserts; ++i) {
        res = NULL;
        rc = qdb_execute(db, inserts[i], &res);
        if (rc == QDB_OK) {
            printf("      Inserted product #%zu (rows affected: %zu)\n", i + 1, qdb_result_rows_affected(res));
        } else {
            printf("      Insert note: %s\n", qdb_result_message(res));
        }
        qdb_result_free(res);
    }
    printf("\n");

    /* 4. Query with Typed Column Accessors (Object API) */
    printf("[4/6] Querying with Typed Column Accessors (qdb_column_*)...\n");
    res = NULL;
    rc = qdb_execute(db, "SELECT id, name, price, stock FROM products ORDER BY price DESC;", &res);

    if (rc == QDB_OK && qdb_result_is_success(res)) {
        size_t rows = qdb_result_row_count(res);
        size_t cols = qdb_result_column_count(res);
        printf("      Result set contains %zu row(s) and %zu column(s):\n", rows, cols);

        for (size_t r = 0; r < rows; ++r) {
            int64_t id = qdb_column_int(res, r, 0);
            const char* name = qdb_column_text(res, r, 1);
            int64_t price = qdb_column_int(res, r, 2);

            printf("      [#%" PRId64 "] %-25s | $%" PRId64 " | Stock: ", id, name, price);
            if (qdb_column_is_null(res, r, 3)) {
                printf("OUT OF STOCK / NULL\n");
            } else {
                printf("%" PRId64 " units\n", qdb_column_int(res, r, 3));
            }
        }
    } else {
        fprintf(stderr, "Query failed: %s\n", qdb_result_message(res));
    }
    qdb_result_free(res);
    printf("\n");

    /* 5. SQL Aggregate Functions */
    printf("[5/6] Computing SQL Aggregates (COUNT, SUM, AVG, MIN, MAX)...\n");
    res = NULL;
    rc = qdb_execute(db, "SELECT COUNT(*), COUNT(stock), AVG(price), SUM(price), MIN(price), MAX(price) FROM products;", &res);

    if (rc == QDB_OK && qdb_result_is_success(res)) {
        printf("      Total Products    : %" PRId64 "\n", qdb_column_int(res, 0, 0));
        printf("      Products in Stock : %" PRId64 "\n", qdb_column_int(res, 0, 1));
        printf("      Average Price     : $%" PRId64 "\n", qdb_column_int(res, 0, 2));
        printf("      Total Inventory Val: $%" PRId64 "\n", qdb_column_int(res, 0, 3));
        printf("      Min Price         : $%" PRId64 "\n", qdb_column_int(res, 0, 4));
        printf("      Max Price         : $%" PRId64 "\n", qdb_column_int(res, 0, 5));
    }
    qdb_result_free(res);
    printf("\n");

    /* 6. SQLite-Style Callback Execution (qdb_exec) */
    printf("[6/6] Streaming Query with Callback API (qdb_exec)...\n");
    int row_count = 0;
    char* errmsg = NULL;
    rc = qdb_exec(db, "SELECT name, price FROM products WHERE price < 150 ORDER BY price ASC;",
                  print_row_callback, &row_count, &errmsg);

    if (rc != QDB_OK) {
        fprintf(stderr, "qdb_exec error: %s\n", errmsg ? errmsg : "Unknown");
        qdb_free(errmsg);
    } else {
        printf("      Streamed %d records successfully.\n", row_count);
    }
    printf("\n");

    /* Cleanup & Shutdown */
    printf("Closing database connection...\n");
    qdb_close(db);
    printf("Done! Demo completed successfully.\n");

    return 0;
}
