#include <execution/executor.hpp>

#include <algorithm>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace qdb {

using sql::JoinType;
using sql::BinaryOp;
using sql::LogicalOp;
using sql::OrderDirection;
using sql::AggregateType;
using sql::AggregateCall;

namespace {

std::optional<std::size_t> find_column_index(const Schema& schema, std::string_view name) {
    // 1. Exact match (e.g. "users.id" or "id")
    for (std::size_t i = 0; i < schema.columns.size(); ++i) {
        if (schema.columns[i].name == name) {
            return i;
        }
    }

    // 2. Unqualified match (if name does not contain '.')
    if (name.find('.') == std::string_view::npos) {
        std::optional<std::size_t> found;
        for (std::size_t i = 0; i < schema.columns.size(); ++i) {
            const std::string_view col_name = schema.columns[i].name;
            const auto dot_pos = col_name.find('.');
            if (dot_pos != std::string_view::npos) {
                if (col_name.substr(dot_pos + 1) == name) {
                    if (found) {
                        return std::nullopt; // Ambiguous column name
                    }
                    found = i;
                }
            }
        }
        if (found) {
            return found;
        }
    }

    // 3. Qualified query column on unqualified schema (e.g. searching "users.id" in schema that has "id")
    const auto dot_pos = name.find('.');
    if (dot_pos != std::string_view::npos) {
        const std::string_view suffix = name.substr(dot_pos + 1);
        for (std::size_t i = 0; i < schema.columns.size(); ++i) {
            if (schema.columns[i].name.find('.') == std::string_view::npos &&
                schema.columns[i].name == suffix) {
                return i;
            }
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

bool order_by_compare(const Value& a, const Value& b, OrderDirection dir) {
    const bool a_null = std::holds_alternative<std::monostate>(a);
    const bool b_null = std::holds_alternative<std::monostate>(b);

    if (a_null && b_null) return false;
    if (a_null) return dir == OrderDirection::Ascending;
    if (b_null) return dir != OrderDirection::Ascending;

    const int cmp = compare_values(a, b);
    if (cmp != -2) {
        return (dir == OrderDirection::Ascending) ? (cmp < 0) : (cmp > 0);
    }

    return (dir == OrderDirection::Ascending) ? (a.index() < b.index()) : (a.index() > b.index());
}

QueryResult evaluate_aggregates(
    const std::vector<AggregateCall>& aggregates,
    const Schema& schema,
    const std::vector<Record>& rows,
    const std::optional<std::size_t>& limit
) {
    if (limit.has_value() && *limit == 0) {
        std::vector<std::string> names;
        names.reserve(aggregates.size());
        for (const auto& agg : aggregates) {
            std::string header;
            switch (agg.type) {
                case AggregateType::Count: header = agg.is_star ? "COUNT(*)" : ("COUNT(" + agg.column_name + ")"); break;
                case AggregateType::Sum:   header = "SUM(" + agg.column_name + ")"; break;
                case AggregateType::Avg:   header = "AVG(" + agg.column_name + ")"; break;
                case AggregateType::Min:   header = "MIN(" + agg.column_name + ")"; break;
                case AggregateType::Max:   header = "MAX(" + agg.column_name + ")"; break;
            }
            names.push_back(std::move(header));
        }
        return QueryResult{
            .success = true,
            .message = "0 row(s) selected.",
            .column_names = std::move(names),
            .rows = {},
            .rows_affected = 0
        };
    }

    std::vector<std::string> names;
    names.reserve(aggregates.size());
    Record agg_row;
    agg_row.values.reserve(aggregates.size());

    for (const auto& agg : aggregates) {
        std::string header;
        switch (agg.type) {
            case AggregateType::Count: header = agg.is_star ? "COUNT(*)" : ("COUNT(" + agg.column_name + ")"); break;
            case AggregateType::Sum:   header = "SUM(" + agg.column_name + ")"; break;
            case AggregateType::Avg:   header = "AVG(" + agg.column_name + ")"; break;
            case AggregateType::Min:   header = "MIN(" + agg.column_name + ")"; break;
            case AggregateType::Max:   header = "MAX(" + agg.column_name + ")"; break;
        }
        names.push_back(std::move(header));

        if (agg.type == AggregateType::Count && agg.is_star) {
            agg_row.values.push_back(static_cast<std::int64_t>(rows.size()));
            continue;
        }

        auto col_idx = find_column_index(schema, agg.column_name);
        if (!col_idx) {
            return QueryResult{
                .success = false,
                .message = "Column '" + agg.column_name + "' does not exist in table schema."
            };
        }
        const std::size_t idx = *col_idx;

        if (agg.type == AggregateType::Count) {
            std::int64_t count = 0;
            for (const auto& row : rows) {
                if (idx < row.values.size() && !std::holds_alternative<std::monostate>(row.values[idx])) {
                    ++count;
                }
            }
            agg_row.values.push_back(count);
        } else if (agg.type == AggregateType::Sum || agg.type == AggregateType::Avg) {
            if (idx >= schema.columns.size() || schema.columns[idx].type != DataType::Integer) {
                return QueryResult{
                    .success = false,
                    .message = "Aggregate function requires an integer column: '" + agg.column_name + "'."
                };
            }

            std::int64_t sum = 0;
            std::int64_t count = 0;
            for (const auto& row : rows) {
                if (idx < row.values.size() && std::holds_alternative<std::int64_t>(row.values[idx])) {
                    sum += std::get<std::int64_t>(row.values[idx]);
                    ++count;
                }
            }

            if (count == 0) {
                agg_row.values.push_back(std::monostate{});
            } else if (agg.type == AggregateType::Sum) {
                agg_row.values.push_back(sum);
            } else {
                agg_row.values.push_back(sum / count);
            }
        } else if (agg.type == AggregateType::Min) {
            std::optional<Value> min_val;
            for (const auto& row : rows) {
                if (idx < row.values.size() && !std::holds_alternative<std::monostate>(row.values[idx])) {
                    const auto& v = row.values[idx];
                    if (!min_val.has_value() || order_by_compare(v, *min_val, OrderDirection::Ascending)) {
                        min_val = v;
                    }
                }
            }
            if (min_val.has_value()) {
                agg_row.values.push_back(std::move(*min_val));
            } else {
                agg_row.values.push_back(std::monostate{});
            }
        } else if (agg.type == AggregateType::Max) {
            std::optional<Value> max_val;
            for (const auto& row : rows) {
                if (idx < row.values.size() && !std::holds_alternative<std::monostate>(row.values[idx])) {
                    const auto& v = row.values[idx];
                    if (!max_val.has_value() || order_by_compare(v, *max_val, OrderDirection::Descending)) {
                        max_val = v;
                    }
                }
            }
            if (max_val.has_value()) {
                agg_row.values.push_back(std::move(*max_val));
            } else {
                agg_row.values.push_back(std::monostate{});
            }
        }
    }

    std::vector<Record> result_rows;
    result_rows.push_back(std::move(agg_row));

    return QueryResult{
        .success = true,
        .message = "1 row(s) selected.",
        .column_names = std::move(names),
        .rows = std::move(result_rows),
        .rows_affected = 0
    };
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

bool evaluate_expr(const sql::Expr* expr, const Schema& schema, const Record& record) {
    if (!expr) {
        return true;
    }

    return std::visit([&](const auto& n) -> bool {
        using T = std::decay_t<decltype(n)>;
        if constexpr (std::is_same_v<T, sql::BinaryExpression>) {
            return evaluate_condition(n, schema, record);
        } else if constexpr (std::is_same_v<T, sql::LogicalExpression>) {
            if (n.op == sql::LogicalOp::And) {
                return evaluate_expr(n.left.get(), schema, record) &&
                       evaluate_expr(n.right.get(), schema, record);
            } else if (n.op == sql::LogicalOp::Or) {
                return evaluate_expr(n.left.get(), schema, record) ||
                       evaluate_expr(n.right.get(), schema, record);
            }
            return false;
        }
        return false;
    }, expr->node);
}

const sql::BinaryExpression* as_single_binary_condition(const sql::Expr* expr) {
    if (!expr) return nullptr;
    return expr->as_binary();
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

Record build_joined_record(const Record& lrow, const Record* rrow,
                           std::size_t left_col_count, std::size_t total_col_count) {
    Record joined_rec;
    joined_rec.values.reserve(total_col_count);
    for (std::size_t i = 0; i < left_col_count; ++i) {
        if (i < lrow.values.size()) {
            joined_rec.values.push_back(lrow.values[i]);
        } else {
            joined_rec.values.push_back(std::monostate{});
        }
    }
    if (rrow) {
        const std::size_t right_col_count = (total_col_count > left_col_count) ? (total_col_count - left_col_count) : 0;
        for (std::size_t i = 0; i < right_col_count; ++i) {
            if (i < rrow->values.size()) {
                joined_rec.values.push_back(rrow->values[i]);
            } else {
                joined_rec.values.push_back(std::monostate{});
            }
        }
    } else {
        joined_rec.values.resize(total_col_count, std::monostate{});
    }
    return joined_rec;
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
        (void)db_.create_table(stmt.table_name, Schema{stmt.columns});
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
    if (!db_.drop_table(stmt.table_name)) {
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
    if (!stmt.joins.empty()) {
        return execute_select_join(stmt);
    }

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
        proj_names.reserve(stmt.columns.size());
        proj_indices.reserve(stmt.columns.size());
        for (const auto& col_name : stmt.columns) {
            if (col_name.ends_with(".*")) {
                for (std::size_t i = 0; i < schema.columns.size(); ++i) {
                    proj_names.push_back(schema.columns[i].name);
                    proj_indices.push_back(i);
                }
                continue;
            }
            auto idx = find_column_index(schema, col_name);
            if (!idx) {
                return QueryResult{
                    .success = false,
                    .message = "Column '" + col_name + "' does not exist in table '" + stmt.table_name + "'."
                };
            }
            proj_names.push_back(col_name);
            proj_indices.push_back(*idx);
        }
    }

    std::vector<Record> matched_rows;
    const auto pk_idx = find_primary_key_column(schema);

    // Fast-path: O(log N) point lookup via B+ tree primary index
    const auto* single_cond = as_single_binary_condition(stmt.where_clause.get());
    const auto cond_col_idx = single_cond ? find_column_index(schema, single_cond->column_name) : std::nullopt;
    if (single_cond != nullptr &&
        single_cond->op == sql::BinaryOp::Equal &&
        pk_idx.has_value() &&
        cond_col_idx.has_value() &&
        *cond_col_idx == *pk_idx &&
        std::holds_alternative<std::int64_t>(single_cond->literal_value) &&
        table->has_primary_index())
    {
        const std::int64_t pk_val = std::get<std::int64_t>(single_cond->literal_value);
        auto find_res = table->find_by_primary_key(pk_val);
        if (!find_res) {
            return QueryResult{
                .success = false,
                .message = std::string(find_res.error().message())
            };
        }
        if (find_res->has_value()) {
            matched_rows.push_back(std::move(**find_res));
        }
    } else {
        // General table scan path
        auto all_rows = table->scan();
        for (auto& row : all_rows) {
            if (!stmt.where_clause || evaluate_expr(stmt.where_clause.get(), schema, row)) {
                matched_rows.push_back(std::move(row));
            }
        }
    }

    if (!stmt.aggregates.empty()) {
        return evaluate_aggregates(stmt.aggregates, schema, matched_rows, stmt.limit);
    }

    // 1. ORDER BY
    if (stmt.order_by.has_value()) {
        auto sort_col_idx = find_column_index(schema, stmt.order_by->column_name);
        if (!sort_col_idx) {
            return QueryResult{
                .success = false,
                .message = "Column '" + stmt.order_by->column_name + "' for ORDER BY does not exist in table '" + stmt.table_name + "'."
            };
        }
        const std::size_t s_idx = *sort_col_idx;
        const OrderDirection dir = stmt.order_by->direction;
        std::stable_sort(matched_rows.begin(), matched_rows.end(),
            [s_idx, dir](const Record& r1, const Record& r2) {
                const auto& v1 = (s_idx < r1.values.size()) ? r1.values[s_idx] : Value{std::monostate{}};
                const auto& v2 = (s_idx < r2.values.size()) ? r2.values[s_idx] : Value{std::monostate{}};
                return order_by_compare(v1, v2, dir);
            });
    }

    // 2. OFFSET
    std::size_t start_idx = 0;
    if (stmt.offset.has_value()) {
        start_idx = std::min(*stmt.offset, matched_rows.size());
    }

    // 3. LIMIT
    std::size_t end_idx = matched_rows.size();
    if (stmt.limit.has_value()) {
        end_idx = std::min(start_idx + *stmt.limit, matched_rows.size());
    }

    // 4. Project slice
    std::vector<Record> result_rows;
    result_rows.reserve(end_idx > start_idx ? end_idx - start_idx : 0);
    for (std::size_t i = start_idx; i < end_idx; ++i) {
        result_rows.push_back(project_record(matched_rows[i], proj_indices));
    }

    return QueryResult{
        .success = true,
        .message = std::to_string(result_rows.size()) + " row(s) selected.",
        .column_names = std::move(proj_names),
        .rows = std::move(result_rows),
        .rows_affected = 0
    };
}

QueryResult SQLExecutor::execute_select_join(const sql::SelectStatement& stmt) {
    auto* left_table = db_.find_table(stmt.table_name);
    if (!left_table) {
        return QueryResult{.success = false, .message = "Table '" + stmt.table_name + "' not found."};
    }

    const Schema left_schema = left_table->get_schema();

    const auto& join = stmt.joins.front();
    auto* right_table = db_.find_table(join.table_name);
    if (!right_table) {
        return QueryResult{.success = false, .message = "Joined table '" + join.table_name + "' not found."};
    }

    const Schema right_schema = right_table->get_schema();

    // Build joined schema with qualified column names: left_table.col, right_table.col
    Schema joined_schema;
    joined_schema.columns.reserve(left_schema.columns.size() + right_schema.columns.size());
    for (const auto& col : left_schema.columns) {
        Column c = col;
        c.name = stmt.table_name + "." + col.name;
        joined_schema.columns.push_back(std::move(c));
    }
    for (const auto& col : right_schema.columns) {
        Column c = col;
        c.name = join.table_name + "." + col.name;
        joined_schema.columns.push_back(std::move(c));
    }

    // Resolve join keys
    auto left_idx = find_column_index(joined_schema, join.left_column);
    auto right_idx = find_column_index(joined_schema, join.right_column);

    if (!left_idx || !right_idx) {
        return QueryResult{
            .success = false,
            .message = "Cannot resolve JOIN ON columns '" + join.left_column + "' and '" + join.right_column + "'."
        };
    }

    std::size_t left_key_pos = *left_idx;
    std::size_t right_key_pos = *right_idx;
    if (left_key_pos >= left_schema.columns.size()) {
        std::swap(left_key_pos, right_key_pos);
    }

    if (left_key_pos >= left_schema.columns.size() || right_key_pos < left_schema.columns.size()) {
        return QueryResult{
            .success = false,
            .message = "JOIN condition must link a column from '" + stmt.table_name + "' with a column from '" + join.table_name + "'."
        };
    }

    const std::size_t right_col_in_right_schema = right_key_pos - left_schema.columns.size();

    const auto right_pk_idx = find_primary_key_column(right_schema);
    const bool can_use_right_index = (right_pk_idx.has_value() &&
                                      *right_pk_idx == right_col_in_right_schema &&
                                      right_table->has_primary_index());

    // Resolve projected columns
    std::vector<std::string> proj_names;
    std::vector<std::size_t> proj_indices;

    if (stmt.columns.empty()) {
        proj_names.reserve(joined_schema.columns.size());
        proj_indices.reserve(joined_schema.columns.size());
        for (std::size_t i = 0; i < joined_schema.columns.size(); ++i) {
            proj_names.push_back(joined_schema.columns[i].name);
            proj_indices.push_back(i);
        }
    } else {
        proj_names.reserve(stmt.columns.size());
        proj_indices.reserve(stmt.columns.size());
        for (const auto& col_name : stmt.columns) {
            if (col_name.ends_with(".*")) {
                const std::string prefix = col_name.substr(0, col_name.size() - 2);
                bool matched_any = false;
                for (std::size_t i = 0; i < joined_schema.columns.size(); ++i) {
                    if (joined_schema.columns[i].name.starts_with(prefix + ".")) {
                        proj_names.push_back(joined_schema.columns[i].name);
                        proj_indices.push_back(i);
                        matched_any = true;
                    }
                }
                if (!matched_any) {
                    return QueryResult{
                        .success = false,
                        .message = "Table prefix '" + prefix + "' does not match any columns in join."
                    };
                }
                continue;
            }

            auto idx = find_column_index(joined_schema, col_name);
            if (!idx) {
                return QueryResult{
                    .success = false,
                    .message = "Column '" + col_name + "' does not exist in joined tables."
                };
            }
            proj_names.push_back(col_name);
            proj_indices.push_back(*idx);
        }
    }

    std::vector<Record> joined_rows;
    const auto left_rows = left_table->scan();

    if (can_use_right_index) {
        // Index Nested Loop Join: O(N * log M)
        for (const auto& lrow : left_rows) {
            const Value lval = (left_key_pos < lrow.values.size()) ? lrow.values[left_key_pos] : Value{std::monostate{}};
            bool matched = false;

            if (std::holds_alternative<std::int64_t>(lval)) {
                const auto pk_key = std::get<std::int64_t>(lval);
                auto find_res = right_table->find_by_primary_key(pk_key);
                if (find_res && find_res->has_value()) {
                    matched = true;
                    Record joined_rec = build_joined_record(lrow, &(**find_res), left_schema.columns.size(), joined_schema.columns.size());
                    if (!stmt.where_clause || evaluate_expr(stmt.where_clause.get(), joined_schema, joined_rec)) {
                        joined_rows.push_back(std::move(joined_rec));
                    }
                }
            }

            if (!matched && join.join_type == sql::JoinType::Left) {
                Record joined_rec = build_joined_record(lrow, nullptr, left_schema.columns.size(), joined_schema.columns.size());
                if (!stmt.where_clause || evaluate_expr(stmt.where_clause.get(), joined_schema, joined_rec)) {
                    joined_rows.push_back(std::move(joined_rec));
                }
            }
        }
    } else {
        // Nested Loop Join: O(N * M)
        const auto right_rows = right_table->scan();
        for (const auto& lrow : left_rows) {
            const Value lval = (left_key_pos < lrow.values.size()) ? lrow.values[left_key_pos] : Value{std::monostate{}};
            bool matched = false;

            if (!std::holds_alternative<std::monostate>(lval)) {
                for (const auto& rrow : right_rows) {
                    if (right_col_in_right_schema >= rrow.values.size()) continue;
                    const auto& rval = rrow.values[right_col_in_right_schema];

                    if (compare_values(lval, rval) == 0) {
                        matched = true;
                        Record joined_rec = build_joined_record(lrow, &rrow, left_schema.columns.size(), joined_schema.columns.size());
                        if (!stmt.where_clause || evaluate_expr(stmt.where_clause.get(), joined_schema, joined_rec)) {
                            joined_rows.push_back(std::move(joined_rec));
                        }
                    }
                }
            }

            if (!matched && join.join_type == sql::JoinType::Left) {
                Record joined_rec = build_joined_record(lrow, nullptr, left_schema.columns.size(), joined_schema.columns.size());
                if (!stmt.where_clause || evaluate_expr(stmt.where_clause.get(), joined_schema, joined_rec)) {
                    joined_rows.push_back(std::move(joined_rec));
                }
            }
        }
    }

    if (!stmt.aggregates.empty()) {
        return evaluate_aggregates(stmt.aggregates, joined_schema, joined_rows, stmt.limit);
    }

    // 1. ORDER BY
    if (stmt.order_by.has_value()) {
        auto sort_col_idx = find_column_index(joined_schema, stmt.order_by->column_name);
        if (!sort_col_idx) {
            return QueryResult{
                .success = false,
                .message = "Column '" + stmt.order_by->column_name + "' for ORDER BY does not exist in joined tables."
            };
        }
        const std::size_t s_idx = *sort_col_idx;
        const OrderDirection dir = stmt.order_by->direction;
        std::stable_sort(joined_rows.begin(), joined_rows.end(),
            [s_idx, dir](const Record& r1, const Record& r2) {
                const auto& v1 = (s_idx < r1.values.size()) ? r1.values[s_idx] : Value{std::monostate{}};
                const auto& v2 = (s_idx < r2.values.size()) ? r2.values[s_idx] : Value{std::monostate{}};
                return order_by_compare(v1, v2, dir);
            });
    }

    // 2. OFFSET
    std::size_t start_idx = 0;
    if (stmt.offset.has_value()) {
        start_idx = std::min(*stmt.offset, joined_rows.size());
    }

    // 3. LIMIT
    std::size_t end_idx = joined_rows.size();
    if (stmt.limit.has_value()) {
        end_idx = std::min(start_idx + *stmt.limit, joined_rows.size());
    }

    // 4. Project slice
    std::vector<Record> result_rows;
    result_rows.reserve(end_idx > start_idx ? end_idx - start_idx : 0);
    for (std::size_t i = start_idx; i < end_idx; ++i) {
        result_rows.push_back(project_record(joined_rows[i], proj_indices));
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
    const auto* single_cond = as_single_binary_condition(stmt.where_clause.get());
    const auto cond_col_idx = single_cond ? find_column_index(schema, single_cond->column_name) : std::nullopt;
    if (single_cond != nullptr &&
        single_cond->op == sql::BinaryOp::Equal &&
        pk_idx.has_value() &&
        cond_col_idx.has_value() &&
        *cond_col_idx == *pk_idx &&
        std::holds_alternative<std::int64_t>(single_cond->literal_value) &&
        table->has_primary_index())
    {
        const std::int64_t pk_val = std::get<std::int64_t>(single_cond->literal_value);
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
    } else if (pk_idx.has_value() && table->has_primary_index()) {
        // Scan path with primary index
        auto all_rows = table->scan();
        for (const auto& row : all_rows) {
            if (!stmt.where_clause || evaluate_expr(stmt.where_clause.get(), schema, row)) {
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
    } else {
        // Scan path without primary index
        auto all_rows = table->scan_with_ids();
        for (const auto& [rid, row] : all_rows) {
            if (!stmt.where_clause || evaluate_expr(stmt.where_clause.get(), schema, row)) {
                Record updated = row;
                updated.values[*target_idx] = stmt.new_value;
                auto upd_res = table->update_record(rid, updated);
                if (!upd_res && upd_res.error().code() == StatusCode::RecordTooLarge) {
                    auto del_res = table->delete_record(rid);
                    if (!del_res) {
                        return QueryResult{.success = false, .message = std::string(del_res.error().message())};
                    }
                    auto ins_res = table->insert(updated);
                    if (!ins_res) {
                        return QueryResult{.success = false, .message = std::string(ins_res.error().message())};
                    }
                    upd_res = *ins_res;
                }
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
    const auto* single_cond = as_single_binary_condition(stmt.where_clause.get());
    const auto cond_col_idx = single_cond ? find_column_index(schema, single_cond->column_name) : std::nullopt;
    if (single_cond != nullptr &&
        single_cond->op == sql::BinaryOp::Equal &&
        pk_idx.has_value() &&
        cond_col_idx.has_value() &&
        *cond_col_idx == *pk_idx &&
        std::holds_alternative<std::int64_t>(single_cond->literal_value) &&
        table->has_primary_index())
    {
        const std::int64_t pk_val = std::get<std::int64_t>(single_cond->literal_value);
        auto del_res = table->delete_by_primary_key(pk_val);
        if (!del_res) {
            return QueryResult{.success = false, .message = std::string(del_res.error().message())};
        }
        if (*del_res) {
            rows_affected = 1;
        }
    } else if (pk_idx.has_value() && table->has_primary_index()) {
        // Scan path with primary index
        auto all_rows = table->scan();
        std::vector<std::int64_t> pks_to_delete;
        for (const auto& row : all_rows) {
            if (!stmt.where_clause || evaluate_expr(stmt.where_clause.get(), schema, row)) {
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
    } else {
        // Scan path without primary index
        auto all_rows = table->scan_with_ids();
        std::vector<RecordID> rids_to_delete;
        for (const auto& [rid, row] : all_rows) {
            if (!stmt.where_clause || evaluate_expr(stmt.where_clause.get(), schema, row)) {
                rids_to_delete.push_back(rid);
            }
        }

        for (RecordID rid : rids_to_delete) {
            auto del_res = table->delete_record(rid);
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
