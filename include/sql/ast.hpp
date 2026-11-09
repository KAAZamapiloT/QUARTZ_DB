#pragma once

#include <storage/record.hpp>
#include <storage/schema.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace qdb::sql {

enum class BinaryOp {
    Equal,
    NotEqual,
    LessThan,
    LessThanOrEqual,
    GreaterThan,
    GreaterThanOrEqual
};

[[nodiscard]] constexpr std::string_view binary_op_to_string(BinaryOp op) noexcept {
    switch (op) {
        case BinaryOp::Equal:              return "=";
        case BinaryOp::NotEqual:           return "!=";
        case BinaryOp::LessThan:           return "<";
        case BinaryOp::LessThanOrEqual:    return "<=";
        case BinaryOp::GreaterThan:        return ">";
        case BinaryOp::GreaterThanOrEqual: return ">=";
    }
    return "UNKNOWN";
}

struct BinaryExpression {
    std::string column_name;
    BinaryOp op{BinaryOp::Equal};
    Value literal_value;
};

struct CreateTableStatement {
    std::string table_name;
    std::vector<Column> columns;
};

struct DropTableStatement {
    std::string table_name;
};

struct InsertStatement {
    std::string table_name;
    std::vector<std::string> column_names; // Empty means all columns in schema order
    std::vector<Value> values;
};

struct SelectStatement {
    std::string table_name;
    std::vector<std::string> columns;      // Empty means "*" (all columns)
    std::optional<BinaryExpression> where_clause;
};

struct UpdateStatement {
    std::string table_name;
    std::string column_name;
    Value new_value;
    std::optional<BinaryExpression> where_clause;
};

struct DeleteStatement {
    std::string table_name;
    std::optional<BinaryExpression> where_clause;
};

using Statement = std::variant<
    CreateTableStatement,
    DropTableStatement,
    InsertStatement,
    SelectStatement,
    UpdateStatement,
    DeleteStatement
>;

} // namespace qdb::sql
