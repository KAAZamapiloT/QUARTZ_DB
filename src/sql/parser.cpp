#include <sql/parser.hpp>

#include <string>
#include <string_view>
#include <utility>

namespace qdb::sql {

Parser::Parser(std::vector<Token> tokens) noexcept
    : tokens_(std::move(tokens)) {}

Parser::Parser(std::string_view sql) {
    Lexer lexer(sql);
    auto token_res = lexer.tokenize();
    if (!token_res) {
        lexer_error_ = token_res.error();
    } else {
        tokens_ = std::move(*token_res);
    }
}

Result<Statement> Parser::parse() {
    if (lexer_error_) {
        return std::unexpected(*lexer_error_);
    }

    if (tokens_.empty() || peek().is(TokenType::EndOfFile)) {
        return std::unexpected(Status{
            StatusCode::SyntaxError,
            "Empty SQL statement"
        });
    }

    Result<Statement> result = std::unexpected(Status{
        StatusCode::SyntaxError,
        "Unknown syntax error"
    });

    if (check(TokenType::KeywordCreate)) {
        auto stmt = parse_create_table();
        if (!stmt) return std::unexpected(stmt.error());
        result = Statement{std::move(*stmt)};
    } else if (check(TokenType::KeywordDrop)) {
        auto stmt = parse_drop_table();
        if (!stmt) return std::unexpected(stmt.error());
        result = Statement{std::move(*stmt)};
    } else if (check(TokenType::KeywordInsert)) {
        auto stmt = parse_insert();
        if (!stmt) return std::unexpected(stmt.error());
        result = Statement{std::move(*stmt)};
    } else if (check(TokenType::KeywordSelect)) {
        auto stmt = parse_select();
        if (!stmt) return std::unexpected(stmt.error());
        result = Statement{std::move(*stmt)};
    } else if (check(TokenType::KeywordUpdate)) {
        auto stmt = parse_update();
        if (!stmt) return std::unexpected(stmt.error());
        result = Statement{std::move(*stmt)};
    } else if (check(TokenType::KeywordDelete)) {
        auto stmt = parse_delete();
        if (!stmt) return std::unexpected(stmt.error());
        result = Statement{std::move(*stmt)};
    } else {
        return std::unexpected(syntax_error(
            "Unexpected token '" + peek().text + "', expected statement keyword (CREATE, SELECT, INSERT, UPDATE, DELETE, DROP)",
            peek()
        ));
    }

    // Optional trailing semicolon
    match(TokenType::Semicolon);

    if (!is_at_end() && !peek().is(TokenType::EndOfFile)) {
        return std::unexpected(syntax_error(
            "Unexpected token '" + peek().text + "' after end of statement",
            peek()
        ));
    }

    return result;
}

Result<CreateTableStatement> Parser::parse_create_table() {
    auto create_tok = consume(TokenType::KeywordCreate, "Expected 'CREATE'");
    if (!create_tok) return std::unexpected(create_tok.error());

    auto table_tok = consume(TokenType::KeywordTable, "Expected 'TABLE' after 'CREATE'");
    if (!table_tok) return std::unexpected(table_tok.error());

    auto name_tok = consume(TokenType::Identifier, "Expected table name after 'CREATE TABLE'");
    if (!name_tok) return std::unexpected(name_tok.error());

    auto lparen = consume(TokenType::LeftParen, "Expected '(' after table name");
    if (!lparen) return std::unexpected(lparen.error());

    std::vector<Column> columns;
    while (!check(TokenType::RightParen) && !is_at_end()) {
        auto col = parse_column_definition();
        if (!col) return std::unexpected(col.error());
        columns.push_back(std::move(*col));

        if (match(TokenType::Comma)) {
            continue;
        }
        break;
    }

    auto rparen = consume(TokenType::RightParen, "Expected ')' after column definitions");
    if (!rparen) return std::unexpected(rparen.error());

    if (columns.empty()) {
        return std::unexpected(syntax_error("Table must have at least one column definition", previous()));
    }

    return CreateTableStatement{
        .table_name = name_tok->text,
        .columns = std::move(columns)
    };
}

Result<Column> Parser::parse_column_definition() {
    auto name_tok = consume(TokenType::Identifier, "Expected column name");
    if (!name_tok) return std::unexpected(name_tok.error());

    auto type_res = parse_data_type();
    if (!type_res) return std::unexpected(type_res.error());

    bool nullable = true;
    bool primary_key = false;

    while (!check(TokenType::Comma) && !check(TokenType::RightParen) && !is_at_end()) {
        if (match(TokenType::KeywordPrimary)) {
            auto key_tok = consume(TokenType::KeywordKey, "Expected 'KEY' after 'PRIMARY'");
            if (!key_tok) return std::unexpected(key_tok.error());
            primary_key = true;
            nullable = false;
        } else if (match(TokenType::KeywordNot)) {
            auto null_tok = consume(TokenType::KeywordNull, "Expected 'NULL' after 'NOT'");
            if (!null_tok) return std::unexpected(null_tok.error());
            nullable = false;
        } else if (match(TokenType::KeywordNull)) {
            nullable = true;
        } else {
            return std::unexpected(syntax_error("Unexpected token in column definition: '" + peek().text + "'", peek()));
        }
    }

    return Column{
        .name = name_tok->text,
        .type = *type_res,
        .nullable = nullable,
        .primary_key = primary_key
    };
}

Result<DataType> Parser::parse_data_type() {
    if (match(TokenType::KeywordInteger)) {
        return DataType::Integer;
    }
    if (match(TokenType::KeywordText)) {
        return DataType::Text;
    }
    if (match(TokenType::KeywordBoolean)) {
        // Boolean values map directly to integers (0 or 1) in storage
        return DataType::Integer;
    }
    if (match(TokenType::KeywordFloat)) {
        return std::unexpected(syntax_error("FLOAT type is not supported in this version", previous()));
    }
    return std::unexpected(syntax_error(
        "Expected data type (INT, INTEGER, TEXT, VARCHAR, STRING)",
        peek()
    ));
}

Result<DropTableStatement> Parser::parse_drop_table() {
    auto drop_tok = consume(TokenType::KeywordDrop, "Expected 'DROP'");
    if (!drop_tok) return std::unexpected(drop_tok.error());

    auto table_tok = consume(TokenType::KeywordTable, "Expected 'TABLE' after 'DROP'");
    if (!table_tok) return std::unexpected(table_tok.error());

    auto name_tok = consume(TokenType::Identifier, "Expected table name after 'DROP TABLE'");
    if (!name_tok) return std::unexpected(name_tok.error());

    return DropTableStatement{.table_name = name_tok->text};
}

Result<InsertStatement> Parser::parse_insert() {
    auto ins_tok = consume(TokenType::KeywordInsert, "Expected 'INSERT'");
    if (!ins_tok) return std::unexpected(ins_tok.error());

    auto into_tok = consume(TokenType::KeywordInto, "Expected 'INTO' after 'INSERT'");
    if (!into_tok) return std::unexpected(into_tok.error());

    auto name_tok = consume(TokenType::Identifier, "Expected table name after 'INSERT INTO'");
    if (!name_tok) return std::unexpected(name_tok.error());

    std::vector<std::string> column_names;
    if (match(TokenType::LeftParen)) {
        while (!check(TokenType::RightParen) && !is_at_end()) {
            auto col = consume(TokenType::Identifier, "Expected column name in column list");
            if (!col) return std::unexpected(col.error());
            column_names.push_back(col->text);

            if (match(TokenType::Comma)) {
                continue;
            }
            break;
        }
        auto rparen = consume(TokenType::RightParen, "Expected ')' after column list");
        if (!rparen) return std::unexpected(rparen.error());
    }

    auto val_kw = consume(TokenType::KeywordValues, "Expected 'VALUES' in INSERT statement");
    if (!val_kw) return std::unexpected(val_kw.error());

    auto lparen = consume(TokenType::LeftParen, "Expected '(' before values list");
    if (!lparen) return std::unexpected(lparen.error());

    std::vector<Value> values;
    while (!check(TokenType::RightParen) && !is_at_end()) {
        auto val = parse_literal_value();
        if (!val) return std::unexpected(val.error());
        values.push_back(std::move(*val));

        if (match(TokenType::Comma)) {
            continue;
        }
        break;
    }

    auto rparen = consume(TokenType::RightParen, "Expected ')' after values list");
    if (!rparen) return std::unexpected(rparen.error());

    if (values.empty()) {
        return std::unexpected(syntax_error("VALUES list cannot be empty", previous()));
    }

    return InsertStatement{
        .table_name = name_tok->text,
        .column_names = std::move(column_names),
        .values = std::move(values)
    };
}

Result<SelectStatement> Parser::parse_select() {
    auto sel_tok = consume(TokenType::KeywordSelect, "Expected 'SELECT'");
    if (!sel_tok) return std::unexpected(sel_tok.error());

    std::vector<std::string> columns;
    std::vector<AggregateCall> aggregates;

    if (match(TokenType::Star)) {
        columns.clear(); // Empty means all columns
    } else {
        while (!check(TokenType::KeywordFrom) && !is_at_end()) {
            if (match(TokenType::KeywordCount)) {
                auto lparen = consume(TokenType::LeftParen, "Expected '(' after 'COUNT'");
                if (!lparen) return std::unexpected(lparen.error());

                if (match(TokenType::Star)) {
                    auto rparen = consume(TokenType::RightParen, "Expected ')' after '*'");
                    if (!rparen) return std::unexpected(rparen.error());
                    aggregates.push_back(AggregateCall{
                        .type = AggregateType::Count,
                        .column_name = "*",
                        .is_star = true
                    });
                } else {
                    auto col = parse_column_reference();
                    if (!col) return std::unexpected(col.error());
                    auto rparen = consume(TokenType::RightParen, "Expected ')' after column in COUNT");
                    if (!rparen) return std::unexpected(rparen.error());
                    aggregates.push_back(AggregateCall{
                        .type = AggregateType::Count,
                        .column_name = std::move(*col),
                        .is_star = false
                    });
                }
            } else if (match(TokenType::KeywordSum) || match(TokenType::KeywordAvg) ||
                       match(TokenType::KeywordMin) || match(TokenType::KeywordMax)) {
                const TokenType func_type = previous().type;
                AggregateType agg_type = AggregateType::Sum;
                std::string func_name = "SUM";
                if (func_type == TokenType::KeywordAvg) {
                    agg_type = AggregateType::Avg;
                    func_name = "AVG";
                } else if (func_type == TokenType::KeywordMin) {
                    agg_type = AggregateType::Min;
                    func_name = "MIN";
                } else if (func_type == TokenType::KeywordMax) {
                    agg_type = AggregateType::Max;
                    func_name = "MAX";
                }

                auto lparen = consume(TokenType::LeftParen, "Expected '(' after '" + func_name + "'");
                if (!lparen) return std::unexpected(lparen.error());

                if (check(TokenType::Star)) {
                    return std::unexpected(syntax_error("'*' is not allowed in " + func_name + "()", peek()));
                }

                auto col = parse_column_reference();
                if (!col) return std::unexpected(col.error());

                auto rparen = consume(TokenType::RightParen, "Expected ')' after column in " + func_name);
                if (!rparen) return std::unexpected(rparen.error());

                aggregates.push_back(AggregateCall{
                    .type = agg_type,
                    .column_name = std::move(*col),
                    .is_star = false
                });
            } else {
                auto col = parse_column_reference();
                if (!col) return std::unexpected(col.error());
                columns.push_back(std::move(*col));
            }

            if (match(TokenType::Comma)) {
                continue;
            }
            break;
        }

        if (!columns.empty() && !aggregates.empty()) {
            return std::unexpected(syntax_error(
                "Mixing aggregate functions and non-aggregate columns is not allowed without GROUP BY",
                peek()
            ));
        }
    }

    auto from_tok = consume(TokenType::KeywordFrom, "Expected 'FROM' after SELECT columns");
    if (!from_tok) return std::unexpected(from_tok.error());

    auto name_tok = consume(TokenType::Identifier, "Expected table name after 'FROM'");
    if (!name_tok) return std::unexpected(name_tok.error());

    std::vector<JoinClause> joins;
    while (check(TokenType::KeywordJoin) || check(TokenType::KeywordInner) || check(TokenType::KeywordLeft)) {
        JoinType jtype = JoinType::Inner;
        if (match(TokenType::KeywordInner)) {
            auto join_tok = consume(TokenType::KeywordJoin, "Expected 'JOIN' after 'INNER'");
            if (!join_tok) return std::unexpected(join_tok.error());
        } else if (match(TokenType::KeywordLeft)) {
            auto join_tok = consume(TokenType::KeywordJoin, "Expected 'JOIN' after 'LEFT'");
            if (!join_tok) return std::unexpected(join_tok.error());
            jtype = JoinType::Left;
        } else {
            advance(); // consume KeywordJoin
        }

        auto right_tbl = consume(TokenType::Identifier, "Expected table name after JOIN");
        if (!right_tbl) return std::unexpected(right_tbl.error());

        auto on_tok = consume(TokenType::KeywordOn, "Expected 'ON' after JOIN table name");
        if (!on_tok) return std::unexpected(on_tok.error());

        auto left_col = parse_column_reference();
        if (!left_col) return std::unexpected(left_col.error());

        auto eq_tok = consume(TokenType::Equal, "Expected '=' in JOIN ON condition");
        if (!eq_tok) return std::unexpected(eq_tok.error());

        auto right_col = parse_column_reference();
        if (!right_col) return std::unexpected(right_col.error());

        joins.push_back(JoinClause{
            .join_type = jtype,
            .table_name = right_tbl->text,
            .left_column = std::move(*left_col),
            .right_column = std::move(*right_col)
        });
    }

    std::unique_ptr<Expr> where_clause;
    if (match(TokenType::KeywordWhere)) {
        auto where_res = parse_where_expr();
        if (!where_res) return std::unexpected(where_res.error());
        where_clause = std::move(*where_res);
    }

    std::optional<OrderByClause> order_by;
    if (match(TokenType::KeywordOrder)) {
        auto by_tok = consume(TokenType::KeywordBy, "Expected 'BY' after 'ORDER'");
        if (!by_tok) return std::unexpected(by_tok.error());

        auto col_res = parse_column_reference();
        if (!col_res) return std::unexpected(col_res.error());

        OrderDirection dir = OrderDirection::Ascending;
        if (match(TokenType::KeywordDesc)) {
            dir = OrderDirection::Descending;
        } else if (match(TokenType::KeywordAsc)) {
            dir = OrderDirection::Ascending;
        }

        order_by = OrderByClause{
            .column_name = std::move(*col_res),
            .direction = dir
        };
    }

    std::optional<std::size_t> limit;
    std::optional<std::size_t> offset;
    if (match(TokenType::KeywordLimit)) {
        auto lim_tok = consume(TokenType::IntegerLiteral, "Expected integer count after 'LIMIT'");
        if (!lim_tok) return std::unexpected(lim_tok.error());

        try {
            long long val = std::stoll(lim_tok->text);
            if (val < 0) {
                return std::unexpected(syntax_error("LIMIT count cannot be negative", *lim_tok));
            }
            limit = static_cast<std::size_t>(val);
        } catch (...) {
            return std::unexpected(syntax_error("Invalid integer in LIMIT", *lim_tok));
        }

        if (match(TokenType::KeywordOffset)) {
            auto off_tok = consume(TokenType::IntegerLiteral, "Expected integer count after 'OFFSET'");
            if (!off_tok) return std::unexpected(off_tok.error());

            try {
                long long oval = std::stoll(off_tok->text);
                if (oval < 0) {
                    return std::unexpected(syntax_error("OFFSET count cannot be negative", *off_tok));
                }
                offset = static_cast<std::size_t>(oval);
            } catch (...) {
                return std::unexpected(syntax_error("Invalid integer in OFFSET", *off_tok));
            }
        }
    } else if (match(TokenType::KeywordOffset)) {
        auto off_tok = consume(TokenType::IntegerLiteral, "Expected integer count after 'OFFSET'");
        if (!off_tok) return std::unexpected(off_tok.error());

        try {
            long long oval = std::stoll(off_tok->text);
            if (oval < 0) {
                return std::unexpected(syntax_error("OFFSET count cannot be negative", *off_tok));
            }
            offset = static_cast<std::size_t>(oval);
        } catch (...) {
            return std::unexpected(syntax_error("Invalid integer in OFFSET", *off_tok));
        }
    }

    return SelectStatement{
        .table_name = name_tok->text,
        .columns = std::move(columns),
        .aggregates = std::move(aggregates),
        .joins = std::move(joins),
        .where_clause = std::move(where_clause),
        .order_by = std::move(order_by),
        .limit = limit,
        .offset = offset
    };
}

Result<UpdateStatement> Parser::parse_update() {
    auto upd_tok = consume(TokenType::KeywordUpdate, "Expected 'UPDATE'");
    if (!upd_tok) return std::unexpected(upd_tok.error());

    auto name_tok = consume(TokenType::Identifier, "Expected table name after 'UPDATE'");
    if (!name_tok) return std::unexpected(name_tok.error());

    auto set_tok = consume(TokenType::KeywordSet, "Expected 'SET' after table name");
    if (!set_tok) return std::unexpected(set_tok.error());

    auto col_tok = consume(TokenType::Identifier, "Expected column name to update");
    if (!col_tok) return std::unexpected(col_tok.error());

    auto eq_tok = consume(TokenType::Equal, "Expected '=' after column name");
    if (!eq_tok) return std::unexpected(eq_tok.error());

    auto val_res = parse_literal_value();
    if (!val_res) return std::unexpected(val_res.error());

    std::unique_ptr<Expr> where_clause;
    if (match(TokenType::KeywordWhere)) {
        auto where_res = parse_where_expr();
        if (!where_res) return std::unexpected(where_res.error());
        where_clause = std::move(*where_res);
    }

    return UpdateStatement{
        .table_name = name_tok->text,
        .column_name = col_tok->text,
        .new_value = std::move(*val_res),
        .where_clause = std::move(where_clause)
    };
}

Result<DeleteStatement> Parser::parse_delete() {
    auto del_tok = consume(TokenType::KeywordDelete, "Expected 'DELETE'");
    if (!del_tok) return std::unexpected(del_tok.error());

    auto from_tok = consume(TokenType::KeywordFrom, "Expected 'FROM' after 'DELETE'");
    if (!from_tok) return std::unexpected(from_tok.error());

    auto name_tok = consume(TokenType::Identifier, "Expected table name after 'DELETE FROM'");
    if (!name_tok) return std::unexpected(name_tok.error());

    std::unique_ptr<Expr> where_clause;
    if (match(TokenType::KeywordWhere)) {
        auto where_res = parse_where_expr();
        if (!where_res) return std::unexpected(where_res.error());
        where_clause = std::move(*where_res);
    }

    return DeleteStatement{
        .table_name = name_tok->text,
        .where_clause = std::move(where_clause)
    };
}

// ============================================================================
// WHERE expression tree parser
// Precedence (lowest to highest): OR < AND < atom comparison
// ============================================================================

Result<std::unique_ptr<Expr>> Parser::parse_where_expr() {
    return parse_or_expr();
}

Result<std::unique_ptr<Expr>> Parser::parse_or_expr() {
    auto left = parse_and_expr();
    if (!left) return std::unexpected(left.error());

    while (match(TokenType::KeywordOr)) {
        auto right = parse_and_expr();
        if (!right) return std::unexpected(right.error());

        auto node = std::make_unique<Expr>(LogicalExpression{
            .op    = LogicalOp::Or,
            .left  = std::move(*left),
            .right = std::move(*right)
        });
        left = std::move(node);
    }

    return left;
}

Result<std::unique_ptr<Expr>> Parser::parse_and_expr() {
    auto left = parse_atom_expr();
    if (!left) return std::unexpected(left.error());

    while (match(TokenType::KeywordAnd)) {
        auto right = parse_atom_expr();
        if (!right) return std::unexpected(right.error());

        auto node = std::make_unique<Expr>(LogicalExpression{
            .op    = LogicalOp::And,
            .left  = std::move(*left),
            .right = std::move(*right)
        });
        left = std::move(node);
    }

    return left;
}

Result<std::unique_ptr<Expr>> Parser::parse_atom_expr() {
    // Parenthesised sub-expression:  ( expr )
    if (match(TokenType::LeftParen)) {
        auto inner = parse_or_expr();
        if (!inner) return std::unexpected(inner.error());
        auto rparen = consume(TokenType::RightParen, "Expected ')' after expression");
        if (!rparen) return std::unexpected(rparen.error());
        return inner;
    }

    // Leaf: column op literal
    auto col_res = parse_column_reference();
    if (!col_res) return std::unexpected(col_res.error());

    auto op_res = parse_binary_op();
    if (!op_res) return std::unexpected(op_res.error());

    auto val_res = parse_literal_value();
    if (!val_res) return std::unexpected(val_res.error());

    return std::make_unique<Expr>(BinaryExpression{
        .column_name  = std::move(*col_res),
        .op           = *op_res,
        .literal_value = std::move(*val_res)
    });
}

Result<std::string> Parser::parse_column_reference() {
    auto id_tok = consume(TokenType::Identifier, "Expected column name");
    if (!id_tok) return std::unexpected(id_tok.error());

    std::string ref = id_tok->text;
    if (match(TokenType::Dot)) {
        if (match(TokenType::Star)) {
            ref += ".*";
        } else {
            auto field_tok = consume(TokenType::Identifier, "Expected column name after '.'");
            if (!field_tok) return std::unexpected(field_tok.error());
            ref += "." + field_tok->text;
        }
    }
    return ref;
}

Result<BinaryOp> Parser::parse_binary_op() {
    if (match(TokenType::Equal))              return BinaryOp::Equal;
    if (match(TokenType::NotEqual))           return BinaryOp::NotEqual;
    if (match(TokenType::LessThan))           return BinaryOp::LessThan;
    if (match(TokenType::LessThanOrEqual))    return BinaryOp::LessThanOrEqual;
    if (match(TokenType::GreaterThan))        return BinaryOp::GreaterThan;
    if (match(TokenType::GreaterThanOrEqual)) return BinaryOp::GreaterThanOrEqual;

    return std::unexpected(syntax_error(
        "Expected comparison operator (=, !=, <>, <, <=, >, >=)",
        peek()
    ));
}

Result<Value> Parser::parse_literal_value() {
    if (match(TokenType::Minus)) {
        if (check(TokenType::IntegerLiteral)) {
            auto tok = advance();
            try {
                return Value{-std::stoll(tok.text)};
            } catch (...) {
                return std::unexpected(syntax_error("Integer literal out of range: " + tok.text, tok));
            }
        }
        return std::unexpected(syntax_error("Expected integer literal after '-'", peek()));
    }

    if (match(TokenType::IntegerLiteral)) {
        try {
            return Value{std::stoll(previous().text)};
        } catch (...) {
            return std::unexpected(syntax_error("Integer literal out of range: " + previous().text, previous()));
        }
    }

    if (match(TokenType::StringLiteral)) {
        return Value{previous().text};
    }

    if (match(TokenType::BooleanLiteral)) {
        const std::string& txt = previous().text;
        bool b = (txt == "true" || txt == "TRUE" || txt == "True");
        return Value{std::int64_t{b ? 1 : 0}};
    }

    if (match(TokenType::KeywordNull) || match(TokenType::NullLiteral)) {
        return Value{std::monostate{}};
    }

    return std::unexpected(syntax_error(
        "Expected literal value (integer, string, boolean, or NULL)",
        peek()
    ));
}

const Token& Parser::peek() const noexcept {
    if (current_ >= tokens_.size()) {
        static const Token eof_token{TokenType::EndOfFile, "", 0, 0};
        return eof_token;
    }
    return tokens_[current_];
}

const Token& Parser::previous() const noexcept {
    if (current_ == 0) {
        return peek();
    }
    return tokens_[current_ - 1];
}

bool Parser::is_at_end() const noexcept {
    return peek().is(TokenType::EndOfFile);
}

const Token& Parser::advance() noexcept {
    if (!is_at_end()) {
        ++current_;
    }
    return previous();
}

bool Parser::check(TokenType type) const noexcept {
    if (is_at_end()) return false;
    return peek().is(type);
}

bool Parser::match(TokenType type) noexcept {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

Result<Token> Parser::consume(TokenType type, std::string_view error_message) {
    if (check(type)) {
        return advance();
    }
    return std::unexpected(syntax_error(error_message, peek()));
}

Status Parser::syntax_error(std::string_view message, const Token& token) const {
    return Status{
        StatusCode::SyntaxError,
        std::string(message) + " at line " + std::to_string(token.line) +
        ", column " + std::to_string(token.column) +
        (token.text.empty() ? "" : (" (got '" + token.text + "')"))
    };
}

} // namespace qdb::sql
