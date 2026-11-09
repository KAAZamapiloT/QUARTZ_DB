#pragma once

#include <quartz/status.hpp>
#include <sql/ast.hpp>
#include <sql/lexer.hpp>
#include <sql/token.hpp>

#include <string_view>
#include <vector>

namespace qdb::sql {

class Parser {
public:
    explicit Parser(std::vector<Token> tokens) noexcept;
    explicit Parser(std::string_view sql);

    [[nodiscard]] Result<Statement> parse();

    [[nodiscard]] Result<CreateTableStatement> parse_create_table();
    [[nodiscard]] Result<DropTableStatement> parse_drop_table();
    [[nodiscard]] Result<InsertStatement> parse_insert();
    [[nodiscard]] Result<SelectStatement> parse_select();
    [[nodiscard]] Result<UpdateStatement> parse_update();
    [[nodiscard]] Result<DeleteStatement> parse_delete();

private:
    [[nodiscard]] Result<Column> parse_column_definition();
    [[nodiscard]] Result<DataType> parse_data_type();
    [[nodiscard]] Result<Value> parse_literal_value();
    [[nodiscard]] Result<BinaryExpression> parse_where_clause();
    [[nodiscard]] Result<BinaryOp> parse_binary_op();

    [[nodiscard]] const Token& peek() const noexcept;
    [[nodiscard]] const Token& previous() const noexcept;
    [[nodiscard]] bool is_at_end() const noexcept;
    const Token& advance() noexcept;
    [[nodiscard]] bool check(TokenType type) const noexcept;
    bool match(TokenType type) noexcept;
    [[nodiscard]] Result<Token> consume(TokenType type, std::string_view error_message);

    [[nodiscard]] Status syntax_error(std::string_view message, const Token& token) const;

    std::vector<Token> tokens_;
    std::size_t current_{0};
    std::optional<Status> lexer_error_;
};

} // namespace qdb::sql
