#include <quartz/quartz.h>
#include <quartz/database.hpp>
#include <execution/executor.hpp>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

struct qdb_database {
    std::unique_ptr<qdb::Database> db;
    std::unique_ptr<qdb::SQLExecutor> executor;
    std::string last_error;
};

struct qdb_result {
    qdb::QueryResult result;
    mutable std::unordered_map<std::uint64_t, std::string> text_cache;
};

namespace {

char* qdb_strdup(const char* s) {
    if (!s) return nullptr;
    const std::size_t len = std::strlen(s);
    auto* copy = static_cast<char*>(std::malloc(len + 1));
    if (copy) {
        std::memcpy(copy, s, len + 1);
    }
    return copy;
}

const qdb::Value* get_cell_value(const qdb_result_t* res, std::size_t row_idx, std::size_t col_idx) noexcept {
    if (!res) return nullptr;
    if (row_idx >= res->result.rows.size()) return nullptr;
    const auto& row = res->result.rows[row_idx];
    if (col_idx >= row.values.size()) return nullptr;
    return &row.values[col_idx];
}

} // namespace

extern "C" {

int qdb_open(const char* filepath, qdb_database_t** out_db) {
    if (!out_db) return QDB_NULL_POINTER;
    *out_db = nullptr;
    if (!filepath) return QDB_NULL_POINTER;

    try {
        auto db_obj = std::make_unique<qdb::Database>(filepath);
        auto exec_obj = std::make_unique<qdb::SQLExecutor>(*db_obj);

        auto* handle = new qdb_database();
        handle->db = std::move(db_obj);
        handle->executor = std::move(exec_obj);
        *out_db = handle;
        return QDB_OK;
    } catch (const std::exception&) {
        return QDB_ERROR;
    } catch (...) {
        return QDB_ERROR;
    }
}

void qdb_close(qdb_database_t* db) {
    if (db) {
        delete db;
    }
}

const char* qdb_errmsg(const qdb_database_t* db) {
    if (!db) return "Invalid database handle";
    return db->last_error.c_str();
}

int qdb_execute(qdb_database_t* db, const char* sql, qdb_result_t** out_result) {
    if (out_result) {
        *out_result = nullptr;
    }
    if (!db || !sql) {
        return QDB_NULL_POINTER;
    }

    try {
        qdb::QueryResult qr = db->executor->execute(sql);
        const int status = qr.success ? QDB_OK : QDB_ERROR;

        if (!qr.success) {
            db->last_error = qr.message;
        } else {
            db->last_error.clear();
        }

        if (out_result) {
            auto* res = new qdb_result();
            res->result = std::move(qr);
            *out_result = res;
        }
        return status;
    } catch (const std::exception& e) {
        db->last_error = e.what();
        if (out_result) {
            auto* res = new qdb_result();
            res->result.success = false;
            res->result.message = e.what();
            *out_result = res;
        }
        return QDB_ERROR;
    } catch (...) {
        db->last_error = "Unknown error occurred";
        return QDB_ERROR;
    }
}

void qdb_result_free(qdb_result_t* res) {
    if (res) {
        delete res;
    }
}

int qdb_result_is_success(const qdb_result_t* res) {
    if (!res) return 0;
    return res->result.success ? 1 : 0;
}

const char* qdb_result_message(const qdb_result_t* res) {
    if (!res) return "Null result handle";
    return res->result.message.c_str();
}

size_t qdb_result_rows_affected(const qdb_result_t* res) {
    if (!res) return 0;
    return res->result.rows_affected;
}

size_t qdb_result_row_count(const qdb_result_t* res) {
    if (!res) return 0;
    return res->result.rows.size();
}

size_t qdb_result_column_count(const qdb_result_t* res) {
    if (!res) return 0;
    return res->result.column_names.size();
}

const char* qdb_result_column_name(const qdb_result_t* res, size_t col_idx) {
    if (!res || col_idx >= res->result.column_names.size()) {
        return "";
    }
    return res->result.column_names[col_idx].c_str();
}

qdb_type_t qdb_column_type(const qdb_result_t* res, size_t row_idx, size_t col_idx) {
    const auto* val = get_cell_value(res, row_idx, col_idx);
    if (!val) return QDB_TYPE_NULL;

    if (std::holds_alternative<std::int64_t>(*val)) {
        return QDB_TYPE_INT;
    }
    if (std::holds_alternative<std::string>(*val)) {
        return QDB_TYPE_TEXT;
    }
    return QDB_TYPE_NULL;
}

int qdb_column_is_null(const qdb_result_t* res, size_t row_idx, size_t col_idx) {
    const auto* val = get_cell_value(res, row_idx, col_idx);
    if (!val) return 1;
    return std::holds_alternative<std::monostate>(*val) ? 1 : 0;
}

int64_t qdb_column_int(const qdb_result_t* res, size_t row_idx, size_t col_idx) {
    const auto* val = get_cell_value(res, row_idx, col_idx);
    if (!val) return 0;

    if (std::holds_alternative<std::int64_t>(*val)) {
        return std::get<std::int64_t>(*val);
    }
    if (std::holds_alternative<std::string>(*val)) {
        try {
            return std::stoll(std::get<std::string>(*val));
        } catch (...) {
            return 0;
        }
    }
    return 0;
}

const char* qdb_column_text(const qdb_result_t* res, size_t row_idx, size_t col_idx) {
    const auto* val = get_cell_value(res, row_idx, col_idx);
    if (!val) return nullptr;

    if (std::holds_alternative<std::string>(*val)) {
        return std::get<std::string>(*val).c_str();
    }
    if (std::holds_alternative<std::int64_t>(*val)) {
        const std::uint64_t key = (static_cast<std::uint64_t>(row_idx) << 32) | static_cast<std::uint64_t>(col_idx);
        auto it = res->text_cache.find(key);
        if (it != res->text_cache.end()) {
            return it->second.c_str();
        }
        auto [inserted, _] = res->text_cache.emplace(key, std::to_string(std::get<std::int64_t>(*val)));
        return inserted->second.c_str();
    }
    return nullptr;
}

int qdb_exec(
    qdb_database_t* db,
    const char* sql,
    qdb_callback_t callback,
    void* user_data,
    char** out_errmsg
) {
    if (out_errmsg) {
        *out_errmsg = nullptr;
    }
    if (!db || !sql) {
        if (out_errmsg) {
            *out_errmsg = qdb_strdup("Database handle or SQL string is null");
        }
        return QDB_NULL_POINTER;
    }

    qdb_result_t* res = nullptr;
    const int rc = qdb_execute(db, sql, &res);

    if (rc != QDB_OK || !qdb_result_is_success(res)) {
        if (out_errmsg) {
            const char* msg = qdb_result_message(res);
            *out_errmsg = qdb_strdup(msg ? msg : "Query execution failed");
        }
        qdb_result_free(res);
        return QDB_ERROR;
    }

    if (callback && res) {
        const std::size_t rows = qdb_result_row_count(res);
        const std::size_t cols = qdb_result_column_count(res);

        std::vector<char*> col_names(cols);
        for (std::size_t c = 0; c < cols; ++c) {
            col_names[c] = const_cast<char*>(qdb_result_column_name(res, c));
        }

        std::vector<char*> col_values(cols);
        for (std::size_t r = 0; r < rows; ++r) {
            for (std::size_t c = 0; c < cols; ++c) {
                if (qdb_column_is_null(res, r, c)) {
                    col_values[c] = nullptr;
                } else {
                    col_values[c] = const_cast<char*>(qdb_column_text(res, r, c));
                }
            }
            const int cb_rc = callback(user_data, static_cast<int>(cols), col_values.data(), col_names.data());
            if (cb_rc != 0) {
                qdb_result_free(res);
                return QDB_ERROR;
            }
        }
    }

    qdb_result_free(res);
    return QDB_OK;
}

void qdb_free(void* ptr) {
    if (ptr) {
        std::free(ptr);
    }
}

const char* qdb_version(void) {
    return "0.1.0";
}

} // extern "C"
