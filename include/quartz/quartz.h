#ifndef QUARTZ_H
#define QUARTZ_H

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32) || defined(__CYGWIN__)
  #if defined(QUARTZ_BUILDING_DLL)
    #define QDB_API __declspec(dllexport)
  #elif defined(QUARTZ_USING_DLL)
    #define QDB_API __declspec(dllimport)
  #else
    #define QDB_API
  #endif
#else
  #if defined(__GNUC__) && __GNUC__ >= 4
    #define QDB_API __attribute__((visibility("default")))
  #else
    #define QDB_API
  #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Status / Return Codes
 */
typedef enum qdb_status {
    QDB_OK           = 0,
    QDB_ERROR        = 1,
    QDB_MISUSE       = 2,
    QDB_NULL_POINTER = 3
} qdb_status_t;

/*
 * Data Types in Query Results
 */
typedef enum qdb_type {
    QDB_TYPE_NULL = 0,
    QDB_TYPE_INT  = 1,
    QDB_TYPE_TEXT = 2
} qdb_type_t;

/*
 * Opaque Handles
 */
typedef struct qdb_database qdb_database_t;
typedef struct qdb_result   qdb_result_t;

/*
 * Callback signature for qdb_exec() (SQLite-compatible)
 *   user_data: pointer passed to qdb_exec()
 *   col_count: number of columns in row
 *   col_values: array of column string values (NULL if column is SQL NULL)
 *   col_names: array of column names
 * Return 0 to continue iteration, non-zero to abort.
 */
typedef int (*qdb_callback_t)(
    void* user_data,
    int col_count,
    char** col_values,
    char** col_names
);

/*
 * Database Lifecycle
 */
QDB_API int         qdb_open(const char* filepath, qdb_database_t** out_db);
QDB_API void        qdb_close(qdb_database_t* db);
QDB_API const char* qdb_errmsg(const qdb_database_t* db);

/*
 * Query Execution (Object API)
 * Executes an SQL query. If out_result is not NULL, allocates a result object.
 * Returns QDB_OK on query success, QDB_ERROR on syntax/runtime error.
 */
QDB_API int         qdb_execute(qdb_database_t* db, const char* sql, qdb_result_t** out_result);
QDB_API void        qdb_result_free(qdb_result_t* res);

/*
 * Result Inspection
 */
QDB_API int         qdb_result_is_success(const qdb_result_t* res);
QDB_API const char* qdb_result_message(const qdb_result_t* res);
QDB_API size_t      qdb_result_rows_affected(const qdb_result_t* res);
QDB_API size_t      qdb_result_row_count(const qdb_result_t* res);
QDB_API size_t      qdb_result_column_count(const qdb_result_t* res);
QDB_API const char* qdb_result_column_name(const qdb_result_t* res, size_t col_idx);

/*
 * Cell Value Access
 */
QDB_API qdb_type_t  qdb_column_type(const qdb_result_t* res, size_t row_idx, size_t col_idx);
QDB_API int         qdb_column_is_null(const qdb_result_t* res, size_t row_idx, size_t col_idx);
QDB_API int64_t     qdb_column_int(const qdb_result_t* res, size_t row_idx, size_t col_idx);
QDB_API const char* qdb_column_text(const qdb_result_t* res, size_t row_idx, size_t col_idx);

/*
 * Callback Execution (SQLite-style convenience API)
 */
QDB_API int         qdb_exec(
    qdb_database_t* db,
    const char* sql,
    qdb_callback_t callback,
    void* user_data,
    char** out_errmsg
);

/*
 * Memory Utility
 */
QDB_API void        qdb_free(void* ptr);

/*
 * Engine Information
 */
QDB_API const char* qdb_version(void);

#ifdef __cplusplus
}
#endif

#endif /* QUARTZ_H */
