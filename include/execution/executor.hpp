#pragma once

#include <quartz/database.hpp>
#include <quartz/status.hpp>
#include <sql/ast.hpp>
#include <sql/parser.hpp>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace qdb {

struct QueryResult {
    bool success{true};
    std::string message;
    std::vector<std::string> column_names;
    std::vector<Record> rows;
    std::size_t rows_affected{0};
};

class SQLExecutor {
public:
    explicit SQLExecutor(Database& db) noexcept;

    // Parses and executes a raw SQL statement.
    [[nodiscard]] QueryResult execute(std::string_view sql);

    // Executes a pre-parsed Statement AST node.
    [[nodiscard]] QueryResult execute(const sql::Statement& statement);

private:
    [[nodiscard]] QueryResult execute_create_table(const sql::CreateTableStatement& stmt);
    [[nodiscard]] QueryResult execute_drop_table(const sql::DropTableStatement& stmt);
    [[nodiscard]] QueryResult execute_insert(const sql::InsertStatement& stmt);
    [[nodiscard]] QueryResult execute_select(const sql::SelectStatement& stmt);
    [[nodiscard]] QueryResult execute_update(const sql::UpdateStatement& stmt);
    [[nodiscard]] QueryResult execute_delete(const sql::DeleteStatement& stmt);

    Database& db_;
};

} // namespace qdb
