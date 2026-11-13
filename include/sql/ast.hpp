#pragma once

#include <storage/record.hpp>
#include <storage/schema.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace qdb::sql {

// ============================================================================
// Comparison operators for column vs literal conditions
// ============================================================================

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

// ============================================================================
// Expression tree for WHERE clauses with AND / OR support
//
// Grammar:
//   Expr   ::= OrExpr
//   OrExpr ::= AndExpr  ( 'OR'  AndExpr )*
//   AndExpr::= Atom     ( 'AND' Atom    )*
//   Atom   ::= column_name op literal
//
// Tree node types:
//   BinaryExpression  — a leaf: col op value  (e.g. id = 5)
//   AndExpr           — logical AND of two sub-expressions
//   OrExpr            — logical OR  of two sub-expressions
// ============================================================================

// Leaf: a single column comparison  (id > 10)
struct BinaryExpression {
    std::string column_name;
    BinaryOp    op{BinaryOp::Equal};
    Value       literal_value;
};

// Forward-declare the variant so And/Or nodes can hold it by pointer
struct Expr;

enum class LogicalOp { And, Or };

// Internal node: left <AND|OR> right
struct LogicalExpression {
    LogicalOp               op;
    std::unique_ptr<Expr>   left;
    std::unique_ptr<Expr>   right;
};

// The variant that is either a leaf or an internal AND/OR node
struct Expr {
    std::variant<BinaryExpression, LogicalExpression> node;

    // Convenience constructors
    explicit Expr(BinaryExpression b)   : node(std::move(b)) {}
    explicit Expr(LogicalExpression l)  : node(std::move(l)) {}

    [[nodiscard]] bool is_binary() const noexcept {
        return std::holds_alternative<BinaryExpression>(node);
    }
    [[nodiscard]] bool is_logical() const noexcept {
        return std::holds_alternative<LogicalExpression>(node);
    }
    [[nodiscard]] const BinaryExpression* as_binary() const noexcept {
        return std::get_if<BinaryExpression>(&node);
    }
    [[nodiscard]] const LogicalExpression* as_logical() const noexcept {
        return std::get_if<LogicalExpression>(&node);
    }
};

// ============================================================================
// SQL Statement AST nodes
// ============================================================================

enum class JoinType {
    Inner,
    Left
};

struct JoinClause {
    JoinType    join_type{JoinType::Inner};
    std::string table_name;
    std::string left_column;
    std::string right_column;
};

enum class OrderDirection {
    Ascending,
    Descending
};

struct OrderByClause {
    std::string    column_name;
    OrderDirection direction{OrderDirection::Ascending};
};

enum class AggregateType {
    Count,
    Sum,
    Avg,
    Min,
    Max
};

struct AggregateCall {
    AggregateType type;
    std::string   column_name;
    bool          is_star{false};
};

struct CreateTableStatement {
    std::string         table_name;
    std::vector<Column> columns;
};

struct DropTableStatement {
    std::string table_name;
};

struct InsertStatement {
    std::string              table_name;
    std::vector<std::string> column_names; // Empty means all columns in schema order
    std::vector<Value>       values;
};

struct SelectStatement {
    std::string                  table_name;
    std::vector<std::string>     columns;      // Empty means "*" (all columns)
    std::vector<AggregateCall>   aggregates;   // Aggregate function calls
    std::vector<JoinClause>      joins;        // Zero or more JOIN clauses
    std::unique_ptr<Expr>        where_clause; // nullptr = no WHERE
    std::optional<OrderByClause> order_by;     // std::nullopt = no ORDER BY
    std::optional<std::size_t>   limit;        // std::nullopt = no LIMIT
    std::optional<std::size_t>   offset;       // std::nullopt = no OFFSET
};

struct UpdateStatement {
    std::string           table_name;
    std::string           column_name;
    Value                 new_value;
    std::unique_ptr<Expr> where_clause; // nullptr = no WHERE
};

struct DeleteStatement {
    std::string           table_name;
    std::unique_ptr<Expr> where_clause; // nullptr = no WHERE
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
