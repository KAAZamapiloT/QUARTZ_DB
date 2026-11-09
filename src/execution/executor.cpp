#include <execution/executor.hpp>

#include <algorithm>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace qdb {

namespace {

std::optional<std::size_t> find_column_index(const Schema& schema, std::string_view name) {
    for (std::size_t i = 0; i < schema.columns.size(); ++i) {
        if (schema.columns[i].name == name) {
            return i;
        }
    }
    return std::nullopt;
}

std::optional<std::size_t> find_primary_key_column(const Schema& schema) {
    for (std::size_t i = 0; i < schema.columns.size(); ++i) {
        if (schema.columns[i].primary_key) {
            return i;
        }
    }
    return std::nullopt;
}

int compare_values(const Value& a, const Value& b) {
    if (std::holds_alternative<std::int64_t>(a) && std::holds_alternative<std::int64_t>(b)) {
        const auto va = std::get<std::int64_t>(a);
        const auto vb = std::get<std::int64_t>(b);
        if (va < vb) return -1;
        if (va > vb) return 1;
        return 0;
    }
    if (std::holds_alternative<std::string>(a) && std::holds_alternative<std::string>(b)) {
        const auto& sa = std::get<std::string>(a);
        const auto& sb = std::get<std::string>(b);
        if (sa < sb) return -1;
        if (sa > sb) return 1;
        return 0;
    }
    return -2; // Incomparable or NULL
}

bool evaluate_condition(const sql::BinaryExpression& expr, const Schema& schema, const Record& record) {
    auto col_idx = find_column_index(schema, expr.column_name);
    if (!col_idx || *col_idx >= record.values.size()) {
        return false;
    }

    const Value& rec_val = record.values[*col_idx];
    const Value& lit_val = expr.literal_value;

    if (std::holds_alternative<std::monostate>(rec_val) || std::holds_alternative<std::monostate>(lit_val)) {
        return false;
    }

    const int cmp = compare_values(rec_val, lit_val);
    if (cmp == -2) {
        return false;
    }

    switch (expr.op) {
        case sql::BinaryOp::Equal:              return cmp == 0;
        case sql::BinaryOp::NotEqual:           return cmp != 0;
        case sql::BinaryOp::LessThan:           return cmp < 0;
        case sql::BinaryOp::LessThanOrEqual:    return cmp <= 0;
        case sql::BinaryOp::GreaterThan:        return cmp > 0;
        case sql::BinaryOp::GreaterThanOrEqual: return cmp >= 0;
    }
    return false;
}

Record project_record(const Record& rec, const std::vector<std::size_t>& indices) {
    Record out;
    out.values.reserve(indices.size());
    for (std::size_t idx : indices) {
        if (idx < rec.values.size()) {
            out.values.push_back(rec.values[idx]);
        } else {
            out.values.push_back(std::monostate{});
        }
    }
    return out;
}

} // namespace

SQLExecutor::SQLExecutor(Database& db) noexcept
    : db_(db) {}

QueryResult SQLExecutor::execute(std::string_view sql) {
    sql::Parser parser(sql);
    auto parse_res = parser.parse();
    if (!parse_res) {
        return QueryResult{
            .success = false,
            .message = std::string(parse_res.error().message())
        };
    }
    return execute(*parse_res);
}

QueryResult SQLExecutor::execute(const sql::Statement& statement) {
    return std::visit([this](const auto& stmt) -> QueryResult {
        using T = std::decay_t<decltype(stmt)>;
        if constexpr (std::is_same_v<T, sql::CreateTableStatement>) {
            return execute_create_table(stmt);
        } else if constexpr (std::is_same_v<T, sql::DropTableStatement>) {
            return execute_drop_table(stmt);
        } else if constexpr (std::is_same_v<T, sql::InsertStatement>) {
            return execute_insert(stmt);
        } else if constexpr (std::is_same_v<T, sql::SelectStatement>) {
            return execute_select(stmt);
        } else if constexpr (std::is_same_v<T, sql::UpdateStatement>) {
            return execute_update(stmt);
        } else if constexpr (std::is_same_v<T, sql::DeleteStatement>) {
            return execute_delete(stmt);
        }
    }, statement);
}

QueryResult SQLExecutor::execute_create_table(const sql::CreateTableStatement& stmt) {
    try {
        db_.create_table(stmt.table_name, Schema{stmt.columns});
        return QueryResult{
            .success = true,
            .message = "Table '" + stmt.table_name + "' created successfully.",
            .rows_affected = 0
        };
    } catch (const std::exception& e) {
        return QueryResult{
            .success = false,
            .message = e.what()
        };
    }
}

QueryResult SQLExecutor::execute_drop_table(const sql::DropTableStatement& stmt) {
    auto* tab = db_.find_table(stmt.table_name);
    if (!tab) {
        return QueryResult{
            .success = false,
            .message = "Table '" + stmt.table_name + "' does not exist."
        };
    }
    return QueryResult{
        .success = true,
        .message = "Table '" + stmt.table_name + "' dropped."
    };
}

QueryResult SQLExecutor::execute_insert(const sql::InsertStatement& stmt) {
    auto* table = db_.find_table(stmt.table_name);
    if (!table) {
        return QueryResult{
            .success = false,
            .message = "Table '" + stmt.table_name + "' not found."
        };
    }

    const Schema schema = table->get_schema();
    Record rec;

    if (stmt.column_names.empty()) {
        if (stmt.values.size() != schema.columns.size()) {
            return QueryResult{
                .success = false,
                .message = "Column count mismatch: expected " + std::to_string(schema.columns.size()) +
                           " values, but got " + std::to_string(stmt.values.size()) + "."
            };
        }
        rec.values = stmt.values;
    } else {
        if (stmt.column_names.size() != stmt.values.size()) {
            return QueryResult{
                .success = false,
                .message = "Column count does not match values count."
            };
        }

        rec.values.resize(schema.columns.size(), std::monostate{});
        for (std::size_t i = 0; i < stmt.column_names.size(); ++i) {
            auto idx = find_column_index(schema, stmt.column_names[i]);
            if (!idx) {
                return QueryResult{
                    .success = false,
                    .message = "Column '" + stmt.column_names[i] + "' does not exist in table '" + stmt.table_name + "'."
                };
            }
            rec.values[*idx] = stmt.values[i];
        }
    }

    auto ins_res = table->insert(rec);
    if (!ins_res) {
        return QueryResult{
            .success = false,
            .message = std::string(ins_res.error().message())
        };
    }

    return QueryResult{
        .success = true,
        .message = "1 row inserted.",
        .rows_affected = 1
    };
}

QueryResult SQLExecutor::execute_select(const sql::SelectStatement& stmt) {
    auto* table = db_.find_table(stmt.table_name);
    if (!table) {
        return QueryResult{
            .success = false,
            .message = "Table '" + stmt.table_name + "' not found."
        };
    }

    const Schema schema = table->get_schema();
    std::vector<std::string> proj_names;
    std::vector<std::size_t> proj_indices;

    if (stmt.columns.empty()) {
        proj_names.reserve(schema.columns.size());
        proj_indices.reserve(schema.columns.size());
        for (std::size_t i = 0; i < schema.columns.size(); ++i) {
            proj_names.push_back(schema.columns[i].name);
            proj_indices.push_back(i);
        }
    } else {
        proj_names = stmt.columns;
        proj_indices.reserve(stmt.columns.size());
        for (const auto& col_name : stmt.columns) {
            auto idx = find_column_index(schema, col_name);
            if (!idx) {
                return QueryResult{
                    .success = false,
                    .message = "Column '" + col_name + "' does not exist in table '" + stmt.table_name + "'."
                };
            }
            proj_indices.push_back(*idx);
        }
    }

    std::vector<Record> result_rows;
    const auto pk_idx = find_primary_key_column(schema);

    // Fast-path: O(log N) point lookup via B+ tree primary index
    if (stmt.where_clause.has_value() &&
        stmt.where_clause->op == sql::BinaryOp::Equal &&
        pk_idx.has_value() &&
        stmt.where_clause->column_name == schema.columns[*pk_idx].name &&
        std::holds_alternative<std::int64_t>(stmt.where_clause->literal_value) &&
        table->has_primary_index())
    {
        const std::int64_t pk_val = std::get<std::int64_t>(stmt.where_clause->literal_value);
        auto find_res = table->find_by_primary_key(pk_val);
        if (!find_res) {
            return QueryResult{
                .success = false,
                .message = std::string(find_res.error().message())
            };
        }
        if (find_res->has_value()) {
            result_rows.push_back(project_record(**find_res, proj_indices));
        }
    } else {
        // General table scan path
        auto all_rows = table->scan();
        for (const auto& row : all_rows) {
            if (!stmt.where_clause.has_value() || evaluate_condition(*stmt.where_clause, schema, row)) {
                result_rows.push_back(project_record(row, proj_indices));
            }
        }
    }

    return QueryResult{
        .success = true,
        .message = std::to_string(result_rows.size()) + " row(s) selected.",
        .column_names = std::move(proj_names),
        .rows = std::move(result_rows),
        .rows_affected = 0
    };
}

QueryResult SQLExecutor::execute_update(const sql::UpdateStatement& stmt) {
    auto* table = db_.find_table(stmt.table_name);
    if (!table) {
        return QueryResult{
            .success = false,
            .message = "Table '" + stmt.table_name + "' not found."
        };
    }

    const Schema schema = table->get_schema();
    auto target_idx = find_column_index(schema, stmt.column_name);
    if (!target_idx) {
        return QueryResult{
            .success = false,
            .message = "Column '" + stmt.column_name + "' does not exist in table '" + stmt.table_name + "'."
        };
    }

    const auto pk_idx = find_primary_key_column(schema);
    if (pk_idx.has_value() && *pk_idx == *target_idx) {
        return QueryResult{
            .success = false,
            .message = "Updating primary key column '" + stmt.column_name + "' is not supported."
        };
    }

    std::size_t rows_affected = 0;

    // Fast-path: where pk = X
    if (stmt.where_clause.has_value() &&
        stmt.where_clause->op == sql::BinaryOp::Equal &&
        pk_idx.has_value() &&
        stmt.where_clause->column_name == schema.columns[*pk_idx].name &&
        std::holds_alternative<std::int64_t>(stmt.where_clause->literal_value) &&
        table->has_primary_index())
    {
        const std::int64_t pk_val = std::get<std::int64_t>(stmt.where_clause->literal_value);
        auto find_res = table->find_by_primary_key(pk_val);
        if (!find_res) {
            return QueryResult{.success = false, .message = std::string(find_res.error().message())};
        }
        if (find_res->has_value()) {
            Record updated = **find_res;
            updated.values[*target_idx] = stmt.new_value;
            auto upd_res = table->update_by_primary_key(pk_val, updated);
            if (!upd_res) {
                return QueryResult{.success = false, .message = std::string(upd_res.error().message())};
            }
            rows_affected = 1;
        }
    } else {
        // Scan path
        auto all_rows = table->scan();
        for (const auto& row : all_rows) {
            if (!stmt.where_clause.has_value() || evaluate_condition(*stmt.where_clause, schema, row)) {
                if (!pk_idx.has_value()) {
                    return QueryResult{
                        .success = false,
                        .message = "Table does not have a primary key for update operation."
                    };
                }
                const auto pk_val = std::get<std::int64_t>(row.values[*pk_idx]);
                Record updated = row;
                updated.values[*target_idx] = stmt.new_value;
                auto upd_res = table->update_by_primary_key(pk_val, updated);
                if (!upd_res) {
                    return QueryResult{.success = false, .message = std::string(upd_res.error().message())};
                }
                ++rows_affected;
            }
        }
    }

    return QueryResult{
        .success = true,
        .message = std::to_string(rows_affected) + " row(s) updated.",
        .rows_affected = rows_affected
    };
}

QueryResult SQLExecutor::execute_delete(const sql::DeleteStatement& stmt) {
    auto* table = db_.find_table(stmt.table_name);
    if (!table) {
        return QueryResult{
            .success = false,
            .message = "Table '" + stmt.table_name + "' not found."
        };
    }

    const Schema schema = table->get_schema();
    const auto pk_idx = find_primary_key_column(schema);
    std::size_t rows_affected = 0;

    // Fast-path: where pk = X
    if (stmt.where_clause.has_value() &&
        stmt.where_clause->op == sql::BinaryOp::Equal &&
        pk_idx.has_value() &&
        stmt.where_clause->column_name == schema.columns[*pk_idx].name &&
        std::holds_alternative<std::int64_t>(stmt.where_clause->literal_value) &&
        table->has_primary_index())
    {
        const std::int64_t pk_val = std::get<std::int64_t>(stmt.where_clause->literal_value);
        auto del_res = table->delete_by_primary_key(pk_val);
        if (!del_res) {
            return QueryResult{.success = false, .message = std::string(del_res.error().message())};
        }
        if (*del_res) {
            rows_affected = 1;
        }
    } else {
        // Scan path
        auto all_rows = table->scan();
        std::vector<std::int64_t> pks_to_delete;
        for (const auto& row : all_rows) {
            if (!stmt.where_clause.has_value() || evaluate_condition(*stmt.where_clause, schema, row)) {
                if (!pk_idx.has_value()) {
                    return QueryResult{
                        .success = false,
                        .message = "Table does not have a primary key for delete operation."
                    };
                }
                pks_to_delete.push_back(std::get<std::int64_t>(row.values[*pk_idx]));
            }
        }

        for (std::int64_t pk : pks_to_delete) {
            auto del_res = table->delete_by_primary_key(pk);
            if (!del_res) {
                return QueryResult{.success = false, .message = std::string(del_res.error().message())};
            }
            if (*del_res) {
                ++rows_affected;
            }
        }
    }

    return QueryResult{
        .success = true,
        .message = std::to_string(rows_affected) + " row(s) deleted.",
        .rows_affected = rows_affected
    };
}

} // namespace qdb
