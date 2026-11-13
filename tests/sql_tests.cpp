#include <gtest/gtest.h>

#include <execution/executor.hpp>
#include <quartz/database.hpp>
#include <sql/ast.hpp>
#include <sql/lexer.hpp>
#include <sql/parser.hpp>
#include <sql/token.hpp>

#include <filesystem>

using namespace qdb::sql;

// ============================================================================
// LEXER TESTS
// ============================================================================

TEST(LexerTest, TokenizeCreateTable) {
    const std::string sql = "CREATE TABLE users (id INT PRIMARY KEY, name TEXT NOT NULL, active BOOL);";
    Lexer lexer(sql);
    auto res = lexer.tokenize();
    ASSERT_TRUE(res.has_value()) << res.error().message();

    const auto& tokens = *res;
    std::vector<TokenType> expected = {
        TokenType::KeywordCreate,
        TokenType::KeywordTable,
        TokenType::Identifier, // users
        TokenType::LeftParen,
        TokenType::Identifier, // id
        TokenType::KeywordInteger,
        TokenType::KeywordPrimary,
        TokenType::KeywordKey,
        TokenType::Comma,
        TokenType::Identifier, // name
        TokenType::KeywordText,
        TokenType::KeywordNot,
        TokenType::KeywordNull,
        TokenType::Comma,
        TokenType::Identifier, // active
        TokenType::KeywordBoolean,
        TokenType::RightParen,
        TokenType::Semicolon,
        TokenType::EndOfFile
    };

    ASSERT_EQ(tokens.size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(tokens[i].type, expected[i]) << "Mismatch at index " << i << ": got " << tokens[i].type_name();
    }
}

TEST(LexerTest, TokenizeInsertInto) {
    const std::string sql = "INSERT INTO users VALUES (42, 'Alice', true, 3.14, NULL);";
    Lexer lexer(sql);
    auto res = lexer.tokenize();
    ASSERT_TRUE(res.has_value());

    const auto& tokens = *res;
    ASSERT_EQ(tokens[0].type, TokenType::KeywordInsert);
    ASSERT_EQ(tokens[1].type, TokenType::KeywordInto);
    ASSERT_EQ(tokens[2].type, TokenType::Identifier);
    EXPECT_EQ(tokens[2].text, "users");
    ASSERT_EQ(tokens[3].type, TokenType::KeywordValues);
    ASSERT_EQ(tokens[4].type, TokenType::LeftParen);
    ASSERT_EQ(tokens[5].type, TokenType::IntegerLiteral);
    EXPECT_EQ(tokens[5].text, "42");
    ASSERT_EQ(tokens[6].type, TokenType::Comma);
    ASSERT_EQ(tokens[7].type, TokenType::StringLiteral);
    EXPECT_EQ(tokens[7].text, "Alice");
    ASSERT_EQ(tokens[8].type, TokenType::Comma);
    ASSERT_EQ(tokens[9].type, TokenType::BooleanLiteral);
    EXPECT_EQ(tokens[9].text, "true");
    ASSERT_EQ(tokens[10].type, TokenType::Comma);
    ASSERT_EQ(tokens[11].type, TokenType::FloatLiteral);
    EXPECT_EQ(tokens[11].text, "3.14");
    ASSERT_EQ(tokens[12].type, TokenType::Comma);
    ASSERT_EQ(tokens[13].type, TokenType::KeywordNull);
    ASSERT_EQ(tokens[14].type, TokenType::RightParen);
    ASSERT_EQ(tokens[15].type, TokenType::Semicolon);
    ASSERT_EQ(tokens[16].type, TokenType::EndOfFile);
}

TEST(LexerTest, TokenizeSelectWhereAndOperators) {
    const std::string sql = "SELECT id, name FROM users WHERE id >= 10 AND score != 0;";
    Lexer lexer(sql);
    auto res = lexer.tokenize();
    ASSERT_TRUE(res.has_value());

    const auto& tokens = *res;
    std::vector<TokenType> expected = {
        TokenType::KeywordSelect,
        TokenType::Identifier, // id
        TokenType::Comma,
        TokenType::Identifier, // name
        TokenType::KeywordFrom,
        TokenType::Identifier, // users
        TokenType::KeywordWhere,
        TokenType::Identifier, // id
        TokenType::GreaterThanOrEqual,
        TokenType::IntegerLiteral,
        TokenType::KeywordAnd,
        TokenType::Identifier, // score
        TokenType::NotEqual,
        TokenType::IntegerLiteral,
        TokenType::Semicolon,
        TokenType::EndOfFile
    };

    ASSERT_EQ(tokens.size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(tokens[i].type, expected[i]) << "Mismatch at index " << i;
    }
}

TEST(LexerTest, TokenizeUpdateSet) {
    const std::string sql = "UPDATE users SET name = 'Bob', active = false WHERE id = 1;";
    Lexer lexer(sql);
    auto res = lexer.tokenize();
    ASSERT_TRUE(res.has_value());

    const auto& tokens = *res;
    EXPECT_EQ(tokens[0].type, TokenType::KeywordUpdate);
    EXPECT_EQ(tokens[1].type, TokenType::Identifier);
    EXPECT_EQ(tokens[2].type, TokenType::KeywordSet);
    EXPECT_EQ(tokens[3].type, TokenType::Identifier);
    EXPECT_EQ(tokens[4].type, TokenType::Equal);
    EXPECT_EQ(tokens[5].type, TokenType::StringLiteral);
    EXPECT_EQ(tokens[5].text, "Bob");
    EXPECT_EQ(tokens[6].type, TokenType::Comma);
    EXPECT_EQ(tokens[7].type, TokenType::Identifier);
    EXPECT_EQ(tokens[8].type, TokenType::Equal);
    EXPECT_EQ(tokens[9].type, TokenType::BooleanLiteral);
    EXPECT_EQ(tokens[10].type, TokenType::KeywordWhere);
    EXPECT_EQ(tokens[11].type, TokenType::Identifier);
    EXPECT_EQ(tokens[12].type, TokenType::Equal);
    EXPECT_EQ(tokens[13].type, TokenType::IntegerLiteral);
    EXPECT_EQ(tokens[14].type, TokenType::Semicolon);
    EXPECT_EQ(tokens[15].type, TokenType::EndOfFile);
}

TEST(LexerTest, TokenizeDeleteFrom) {
    const std::string sql = "DELETE FROM users WHERE id = 99;";
    Lexer lexer(sql);
    auto res = lexer.tokenize();
    ASSERT_TRUE(res.has_value());

    const auto& tokens = *res;
    EXPECT_EQ(tokens[0].type, TokenType::KeywordDelete);
    EXPECT_EQ(tokens[1].type, TokenType::KeywordFrom);
    EXPECT_EQ(tokens[2].type, TokenType::Identifier);
    EXPECT_EQ(tokens[3].type, TokenType::KeywordWhere);
    EXPECT_EQ(tokens[4].type, TokenType::Identifier);
    EXPECT_EQ(tokens[5].type, TokenType::Equal);
    EXPECT_EQ(tokens[6].type, TokenType::IntegerLiteral);
    EXPECT_EQ(tokens[7].type, TokenType::Semicolon);
    EXPECT_EQ(tokens[8].type, TokenType::EndOfFile);
}

TEST(LexerTest, CaseInsensitiveKeywords) {
    const std::string sql = "cReAtE tAbLe SeLeCt InTo WhErE fRoM";
    Lexer lexer(sql);
    auto res = lexer.tokenize();
    ASSERT_TRUE(res.has_value());

    const auto& tokens = *res;
    EXPECT_EQ(tokens[0].type, TokenType::KeywordCreate);
    EXPECT_EQ(tokens[1].type, TokenType::KeywordTable);
    EXPECT_EQ(tokens[2].type, TokenType::KeywordSelect);
    EXPECT_EQ(tokens[3].type, TokenType::KeywordInto);
    EXPECT_EQ(tokens[4].type, TokenType::KeywordWhere);
    EXPECT_EQ(tokens[5].type, TokenType::KeywordFrom);
}

TEST(LexerTest, StringEscapingAndQuotes) {
    const std::string sql = "'O''Reilly' \"hello world\" 'escaped\\nnewline'";
    Lexer lexer(sql);
    auto res = lexer.tokenize();
    ASSERT_TRUE(res.has_value());

    const auto& tokens = *res;
    ASSERT_GE(tokens.size(), 3U);
    EXPECT_EQ(tokens[0].type, TokenType::StringLiteral);
    EXPECT_EQ(tokens[0].text, "O'Reilly");

    EXPECT_EQ(tokens[1].type, TokenType::StringLiteral);
    EXPECT_EQ(tokens[1].text, "hello world");

    EXPECT_EQ(tokens[2].type, TokenType::StringLiteral);
    EXPECT_EQ(tokens[2].text, "escaped\nnewline");
}

TEST(LexerTest, CommentsSkipped) {
    const std::string sql = 
        "-- This is a comment\n"
        "SELECT * /* block comment */ FROM users; -- trailing comment";
    Lexer lexer(sql);
    auto res = lexer.tokenize();
    ASSERT_TRUE(res.has_value());

    const auto& tokens = *res;
    ASSERT_EQ(tokens.size(), 6U);
    EXPECT_EQ(tokens[0].type, TokenType::KeywordSelect);
    EXPECT_EQ(tokens[1].type, TokenType::Star);
    EXPECT_EQ(tokens[2].type, TokenType::KeywordFrom);
    EXPECT_EQ(tokens[3].type, TokenType::Identifier);
    EXPECT_EQ(tokens[4].type, TokenType::Semicolon);
    EXPECT_EQ(tokens[5].type, TokenType::EndOfFile);
}

TEST(LexerTest, SyntaxErrorOnUnterminatedString) {
    const std::string sql = "SELECT * FROM users WHERE name = 'unterminated";
    Lexer lexer(sql);
    auto res = lexer.tokenize();
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().code(), qdb::StatusCode::SyntaxError);
}

// ============================================================================
// PARSER TESTS
// ============================================================================

TEST(ParserTest, ParseCreateTable) {
    const std::string sql = 
        "CREATE TABLE accounts ("
        "  id INT PRIMARY KEY,"
        "  owner TEXT NOT NULL,"
        "  bio TEXT"
        ");";

    Parser parser(sql);
    auto res = parser.parse();
    ASSERT_TRUE(res.has_value()) << res.error().message();

    ASSERT_TRUE(std::holds_alternative<CreateTableStatement>(*res));
    const auto& stmt = std::get<CreateTableStatement>(*res);

    EXPECT_EQ(stmt.table_name, "accounts");
    ASSERT_EQ(stmt.columns.size(), 3U);

    EXPECT_EQ(stmt.columns[0].name, "id");
    EXPECT_EQ(stmt.columns[0].type, qdb::DataType::Integer);
    EXPECT_FALSE(stmt.columns[0].nullable);
    EXPECT_TRUE(stmt.columns[0].primary_key);

    EXPECT_EQ(stmt.columns[1].name, "owner");
    EXPECT_EQ(stmt.columns[1].type, qdb::DataType::Text);
    EXPECT_FALSE(stmt.columns[1].nullable);
    EXPECT_FALSE(stmt.columns[1].primary_key);

    EXPECT_EQ(stmt.columns[2].name, "bio");
    EXPECT_EQ(stmt.columns[2].type, qdb::DataType::Text);
    EXPECT_TRUE(stmt.columns[2].nullable);
    EXPECT_FALSE(stmt.columns[2].primary_key);
}

TEST(ParserTest, ParseDropTable) {
    const std::string sql = "DROP TABLE old_logs;";
    Parser parser(sql);
    auto res = parser.parse();
    ASSERT_TRUE(res.has_value()) << res.error().message();

    ASSERT_TRUE(std::holds_alternative<DropTableStatement>(*res));
    const auto& stmt = std::get<DropTableStatement>(*res);
    EXPECT_EQ(stmt.table_name, "old_logs");
}

TEST(ParserTest, ParseInsertWithAndWithoutColumns) {
    // 1. With column names
    {
        const std::string sql = "INSERT INTO users (id, name) VALUES (1, 'Alice');";
        Parser parser(sql);
        auto res = parser.parse();
        ASSERT_TRUE(res.has_value()) << res.error().message();

        ASSERT_TRUE(std::holds_alternative<InsertStatement>(*res));
        const auto& stmt = std::get<InsertStatement>(*res);
        EXPECT_EQ(stmt.table_name, "users");
        ASSERT_EQ(stmt.column_names.size(), 2U);
        EXPECT_EQ(stmt.column_names[0], "id");
        EXPECT_EQ(stmt.column_names[1], "name");
        ASSERT_EQ(stmt.values.size(), 2U);
        EXPECT_EQ(std::get<std::int64_t>(stmt.values[0]), 1);
        EXPECT_EQ(std::get<std::string>(stmt.values[1]), "Alice");
    }

    // 2. Without column names and with negative integer and NULL
    {
        const std::string sql = "INSERT INTO data VALUES (-42, 'Neg', NULL);";
        Parser parser(sql);
        auto res = parser.parse();
        ASSERT_TRUE(res.has_value()) << res.error().message();

        ASSERT_TRUE(std::holds_alternative<InsertStatement>(*res));
        const auto& stmt = std::get<InsertStatement>(*res);
        EXPECT_EQ(stmt.table_name, "data");
        EXPECT_TRUE(stmt.column_names.empty());
        ASSERT_EQ(stmt.values.size(), 3U);
        EXPECT_EQ(std::get<std::int64_t>(stmt.values[0]), -42);
        EXPECT_EQ(std::get<std::string>(stmt.values[1]), "Neg");
        EXPECT_TRUE(std::holds_alternative<std::monostate>(stmt.values[2]));
    }
}

TEST(ParserTest, ParseSelectStarAndColumns) {
    // 1. SELECT *
    {
        const std::string sql = "SELECT * FROM users;";
        Parser parser(sql);
        auto res = parser.parse();
        ASSERT_TRUE(res.has_value());

        ASSERT_TRUE(std::holds_alternative<SelectStatement>(*res));
        const auto& stmt = std::get<SelectStatement>(*res);
        EXPECT_EQ(stmt.table_name, "users");
        EXPECT_TRUE(stmt.columns.empty());
        EXPECT_EQ(stmt.where_clause, nullptr);
    }

    // 2. SELECT specific columns with WHERE
    {
        const std::string sql = "SELECT id, name FROM users WHERE id = 100;";
        Parser parser(sql);
        auto res = parser.parse();
        ASSERT_TRUE(res.has_value());

        ASSERT_TRUE(std::holds_alternative<SelectStatement>(*res));
        const auto& stmt = std::get<SelectStatement>(*res);
        EXPECT_EQ(stmt.table_name, "users");
        ASSERT_EQ(stmt.columns.size(), 2U);
        EXPECT_EQ(stmt.columns[0], "id");
        EXPECT_EQ(stmt.columns[1], "name");

        ASSERT_NE(stmt.where_clause, nullptr);
        ASSERT_TRUE(stmt.where_clause->is_binary());
        const auto* bin = stmt.where_clause->as_binary();
        ASSERT_NE(bin, nullptr);
        EXPECT_EQ(bin->column_name, "id");
        EXPECT_EQ(bin->op, BinaryOp::Equal);
        EXPECT_EQ(std::get<std::int64_t>(bin->literal_value), 100);
    }
}

TEST(ParserTest, ParseUpdateStatement) {
    const std::string sql = "UPDATE users SET name = 'Charlie' WHERE id = 5;";
    Parser parser(sql);
    auto res = parser.parse();
    ASSERT_TRUE(res.has_value()) << res.error().message();

    ASSERT_TRUE(std::holds_alternative<UpdateStatement>(*res));
    const auto& stmt = std::get<UpdateStatement>(*res);
    EXPECT_EQ(stmt.table_name, "users");
    EXPECT_EQ(stmt.column_name, "name");
    EXPECT_EQ(std::get<std::string>(stmt.new_value), "Charlie");

    ASSERT_NE(stmt.where_clause, nullptr);
    ASSERT_TRUE(stmt.where_clause->is_binary());
    const auto* bin = stmt.where_clause->as_binary();
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->column_name, "id");
    EXPECT_EQ(bin->op, BinaryOp::Equal);
    EXPECT_EQ(std::get<std::int64_t>(bin->literal_value), 5);
}

TEST(ParserTest, ParseDeleteStatement) {
    const std::string sql = "DELETE FROM users WHERE age >= 21;";
    Parser parser(sql);
    auto res = parser.parse();
    ASSERT_TRUE(res.has_value()) << res.error().message();

    ASSERT_TRUE(std::holds_alternative<DeleteStatement>(*res));
    const auto& stmt = std::get<DeleteStatement>(*res);
    EXPECT_EQ(stmt.table_name, "users");

    ASSERT_NE(stmt.where_clause, nullptr);
    ASSERT_TRUE(stmt.where_clause->is_binary());
    const auto* bin = stmt.where_clause->as_binary();
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->column_name, "age");
    EXPECT_EQ(bin->op, BinaryOp::GreaterThanOrEqual);
    EXPECT_EQ(std::get<std::int64_t>(bin->literal_value), 21);
}

TEST(ParserTest, SyntaxErrorHandling) {
    // Missing table name
    {
        Parser parser("CREATE TABLE (id INT);");
        auto res = parser.parse();
        ASSERT_FALSE(res.has_value());
        EXPECT_EQ(res.error().code(), qdb::StatusCode::SyntaxError);
    }

    // Missing closing paren
    {
        Parser parser("INSERT INTO users VALUES (1, 'Alice';");
        auto res = parser.parse();
        ASSERT_FALSE(res.has_value());
        EXPECT_EQ(res.error().code(), qdb::StatusCode::SyntaxError);
    }

    // Invalid keyword
    {
        Parser parser("FOOBAR * FROM users;");
        auto res = parser.parse();
        ASSERT_FALSE(res.has_value());
        EXPECT_EQ(res.error().code(), qdb::StatusCode::SyntaxError);
    }
}

TEST(ParserTest, ParseOrderByAndLimit) {
    // 1. ORDER BY default ASC
    {
        Parser parser("SELECT * FROM users ORDER BY age;");
        auto res = parser.parse();
        ASSERT_TRUE(res.has_value()) << res.error().message();
        const auto& stmt = std::get<SelectStatement>(*res);
        ASSERT_TRUE(stmt.order_by.has_value());
        EXPECT_EQ(stmt.order_by->column_name, "age");
        EXPECT_EQ(stmt.order_by->direction, OrderDirection::Ascending);
        EXPECT_FALSE(stmt.limit.has_value());
        EXPECT_FALSE(stmt.offset.has_value());
    }

    // 2. ORDER BY DESC
    {
        Parser parser("SELECT id, name FROM users ORDER BY score DESC;");
        auto res = parser.parse();
        ASSERT_TRUE(res.has_value());
        const auto& stmt = std::get<SelectStatement>(*res);
        ASSERT_TRUE(stmt.order_by.has_value());
        EXPECT_EQ(stmt.order_by->column_name, "score");
        EXPECT_EQ(stmt.order_by->direction, OrderDirection::Descending);
    }

    // 3. LIMIT with OFFSET
    {
        Parser parser("SELECT * FROM users LIMIT 15 OFFSET 30;");
        auto res = parser.parse();
        ASSERT_TRUE(res.has_value());
        const auto& stmt = std::get<SelectStatement>(*res);
        EXPECT_FALSE(stmt.order_by.has_value());
        ASSERT_TRUE(stmt.limit.has_value());
        EXPECT_EQ(*stmt.limit, 15U);
        ASSERT_TRUE(stmt.offset.has_value());
        EXPECT_EQ(*stmt.offset, 30U);
    }

    // 4. Combined WHERE, ORDER BY, LIMIT, and OFFSET
    {
        Parser parser("SELECT id FROM users WHERE active = 1 ORDER BY created_at DESC LIMIT 5 OFFSET 10;");
        auto res = parser.parse();
        ASSERT_TRUE(res.has_value());
        const auto& stmt = std::get<SelectStatement>(*res);
        ASSERT_NE(stmt.where_clause, nullptr);
        ASSERT_TRUE(stmt.order_by.has_value());
        EXPECT_EQ(stmt.order_by->column_name, "created_at");
        EXPECT_EQ(stmt.order_by->direction, OrderDirection::Descending);
        ASSERT_TRUE(stmt.limit.has_value());
        EXPECT_EQ(*stmt.limit, 5U);
        ASSERT_TRUE(stmt.offset.has_value());
        EXPECT_EQ(*stmt.offset, 10U);
    }
}

TEST(ParserTest, ParseAggregateFunctions) {
    // 1. COUNT(*)
    {
        Parser parser("SELECT COUNT(*) FROM users;");
        auto res = parser.parse();
        ASSERT_TRUE(res.has_value()) << res.error().message();
        const auto& stmt = std::get<SelectStatement>(*res);
        EXPECT_EQ(stmt.table_name, "users");
        EXPECT_TRUE(stmt.columns.empty());
        ASSERT_EQ(stmt.aggregates.size(), 1U);
        EXPECT_EQ(stmt.aggregates[0].type, AggregateType::Count);
        EXPECT_TRUE(stmt.aggregates[0].is_star);
        EXPECT_EQ(stmt.aggregates[0].column_name, "*");
    }

    // 2. Multiple aggregates: COUNT(id), SUM(score), AVG(score), MIN(score), MAX(score)
    {
        Parser parser("SELECT COUNT(id), SUM(score), AVG(score), MIN(score), MAX(score) FROM scores;");
        auto res = parser.parse();
        ASSERT_TRUE(res.has_value()) << res.error().message();
        const auto& stmt = std::get<SelectStatement>(*res);
        ASSERT_EQ(stmt.aggregates.size(), 5U);
        EXPECT_EQ(stmt.aggregates[0].type, AggregateType::Count);
        EXPECT_FALSE(stmt.aggregates[0].is_star);
        EXPECT_EQ(stmt.aggregates[0].column_name, "id");
        EXPECT_EQ(stmt.aggregates[1].type, AggregateType::Sum);
        EXPECT_EQ(stmt.aggregates[1].column_name, "score");
        EXPECT_EQ(stmt.aggregates[2].type, AggregateType::Avg);
        EXPECT_EQ(stmt.aggregates[2].column_name, "score");
        EXPECT_EQ(stmt.aggregates[3].type, AggregateType::Min);
        EXPECT_EQ(stmt.aggregates[3].column_name, "score");
        EXPECT_EQ(stmt.aggregates[4].type, AggregateType::Max);
        EXPECT_EQ(stmt.aggregates[4].column_name, "score");
    }

    // 3. MIN and MAX on text column
    {
        Parser parser("SELECT MIN(player), MAX(player) FROM scores;");
        auto res = parser.parse();
        ASSERT_TRUE(res.has_value());
        const auto& stmt = std::get<SelectStatement>(*res);
        ASSERT_EQ(stmt.aggregates.size(), 2U);
        EXPECT_EQ(stmt.aggregates[0].type, AggregateType::Min);
        EXPECT_EQ(stmt.aggregates[0].column_name, "player");
        EXPECT_EQ(stmt.aggregates[1].type, AggregateType::Max);
        EXPECT_EQ(stmt.aggregates[1].column_name, "player");
    }

    // 4. Reject '*' in non-COUNT aggregates
    {
        Parser p1("SELECT SUM(*) FROM scores;");
        auto r1 = p1.parse();
        ASSERT_FALSE(r1.has_value());

        Parser p2("SELECT AVG(*) FROM scores;");
        auto r2 = p2.parse();
        ASSERT_FALSE(r2.has_value());

        Parser p3("SELECT MIN(*) FROM scores;");
        auto r3 = p3.parse();
        ASSERT_FALSE(r3.has_value());

        Parser p4("SELECT MAX(*) FROM scores;");
        auto r4 = p4.parse();
        ASSERT_FALSE(r4.has_value());
    }

    // 5. Reject mixing plain columns with aggregates without GROUP BY
    {
        Parser p1("SELECT id, COUNT(*) FROM scores;");
        auto r1 = p1.parse();
        ASSERT_FALSE(r1.has_value());

        Parser p2("SELECT COUNT(*), id FROM scores;");
        auto r2 = p2.parse();
        ASSERT_FALSE(r2.has_value());
    }
}

// ============================================================================
// EXECUTOR TESTS (END-TO-END SQL)
// ============================================================================

class SQLExecutorTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        db_path_ = std::filesystem::temp_directory_path() / (std::string("qdb_sql_") + info->name() + ".db");
        std::error_code ec;
        std::filesystem::remove(db_path_, ec);
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove(db_path_, ec);
    }

    std::filesystem::path db_path_;
};

TEST_F(SQLExecutorTest, EndToEndCreateTableInsertSelect) {
    qdb::Database db(db_path_);
    qdb::SQLExecutor executor(db);

    // 1. CREATE TABLE
    auto res1 = executor.execute("CREATE TABLE employees (id INT PRIMARY KEY, name TEXT NOT NULL, department TEXT);");
    ASSERT_TRUE(res1.success) << res1.message;

    // 2. INSERT records
    auto res2 = executor.execute("INSERT INTO employees VALUES (1, 'Alice', 'Engineering');");
    ASSERT_TRUE(res2.success) << res2.message;
    EXPECT_EQ(res2.rows_affected, 1U);

    auto res3 = executor.execute("INSERT INTO employees VALUES (2, 'Bob', 'Marketing');");
    ASSERT_TRUE(res3.success) << res3.message;

    auto res4 = executor.execute("INSERT INTO employees VALUES (3, 'Charlie', 'Engineering');");
    ASSERT_TRUE(res4.success) << res4.message;

    // 3. SELECT * (all rows)
    auto select_all = executor.execute("SELECT * FROM employees;");
    ASSERT_TRUE(select_all.success) << select_all.message;
    EXPECT_EQ(select_all.column_names.size(), 3U);
    EXPECT_EQ(select_all.rows.size(), 3U);

    // 4. Point lookup using B+ tree index (WHERE id = 2)
    auto point_select = executor.execute("SELECT name, department FROM employees WHERE id = 2;");
    ASSERT_TRUE(point_select.success) << point_select.message;
    ASSERT_EQ(point_select.rows.size(), 1U);
    EXPECT_EQ(point_select.column_names[0], "name");
    EXPECT_EQ(point_select.column_names[1], "department");
    EXPECT_EQ(std::get<std::string>(point_select.rows[0].values[0]), "Bob");
    EXPECT_EQ(std::get<std::string>(point_select.rows[0].values[1]), "Marketing");
}

TEST_F(SQLExecutorTest, EndToEndUpdateAndDelete) {
    qdb::Database db(db_path_);
    qdb::SQLExecutor executor(db);

    ASSERT_TRUE(executor.execute("CREATE TABLE users (id INT PRIMARY KEY, name TEXT);").success);
    ASSERT_TRUE(executor.execute("INSERT INTO users VALUES (1, 'User1');").success);
    ASSERT_TRUE(executor.execute("INSERT INTO users VALUES (2, 'User2');").success);

    // UPDATE
    auto upd = executor.execute("UPDATE users SET name = 'UpdatedUser1' WHERE id = 1;");
    ASSERT_TRUE(upd.success) << upd.message;
    EXPECT_EQ(upd.rows_affected, 1U);

    auto verify_upd = executor.execute("SELECT name FROM users WHERE id = 1;");
    ASSERT_TRUE(verify_upd.success);
    ASSERT_EQ(verify_upd.rows.size(), 1U);
    EXPECT_EQ(std::get<std::string>(verify_upd.rows[0].values[0]), "UpdatedUser1");

    // DELETE
    auto del = executor.execute("DELETE FROM users WHERE id = 2;");
    ASSERT_TRUE(del.success) << del.message;
    EXPECT_EQ(del.rows_affected, 1U);

    auto verify_del = executor.execute("SELECT * FROM users WHERE id = 2;");
    ASSERT_TRUE(verify_del.success);
    EXPECT_EQ(verify_del.rows.size(), 0U);

    auto scan = executor.execute("SELECT * FROM users;");
    ASSERT_TRUE(scan.success);
    EXPECT_EQ(scan.rows.size(), 1U);
}

TEST_F(SQLExecutorTest, DuplicatePrimaryKeyIsRejected) {
    qdb::Database db(db_path_);
    qdb::SQLExecutor executor(db);

    ASSERT_TRUE(executor.execute("CREATE TABLE items (id INT PRIMARY KEY, title TEXT);").success);
    ASSERT_TRUE(executor.execute("INSERT INTO items VALUES (10, 'Item A');").success);

    auto dup = executor.execute("INSERT INTO items VALUES (10, 'Item Duplicate');");
    ASSERT_FALSE(dup.success);
}

TEST_F(SQLExecutorTest, DropTableLifecycleAndPersistence) {
    // Session 1: Create, insert, verify, and drop
    {
        qdb::Database db(db_path_);
        qdb::SQLExecutor executor(db);

        ASSERT_TRUE(executor.execute("CREATE TABLE sample (id INT PRIMARY KEY, name TEXT);").success);
        ASSERT_TRUE(executor.execute("INSERT INTO sample VALUES (1, 'Mechanical Keyboard');").success);

        // Verify table exists and row is queryable
        auto sel1 = executor.execute("SELECT * FROM sample;");
        ASSERT_TRUE(sel1.success);
        ASSERT_EQ(sel1.rows.size(), 1U);
        EXPECT_EQ(db.table_count(), 1U);
        EXPECT_NE(db.find_table("sample"), nullptr);

        // Drop table
        auto drop_res = executor.execute("DROP TABLE sample;");
        ASSERT_TRUE(drop_res.success) << drop_res.message;
        EXPECT_EQ(drop_res.message, "Table 'sample' dropped.");

        // Verify table is no longer in catalog
        EXPECT_EQ(db.table_count(), 0U);
        EXPECT_EQ(db.find_table("sample"), nullptr);
        EXPECT_TRUE(db.list_tables().empty());

        // SELECT on dropped table fails
        auto sel_after = executor.execute("SELECT * FROM sample;");
        ASSERT_FALSE(sel_after.success);

        // Second DROP TABLE fails
        auto drop_again = executor.execute("DROP TABLE sample;");
        ASSERT_FALSE(drop_again.success);
    }

    // Session 2: Reopen from disk and verify dropped table remains deleted
    {
        qdb::Database db(db_path_);
        qdb::SQLExecutor executor(db);

        EXPECT_EQ(db.table_count(), 0U);
        EXPECT_EQ(db.find_table("sample"), nullptr);

        auto sel = executor.execute("SELECT * FROM sample;");
        ASSERT_FALSE(sel.success);

        // Can recreate table with same name cleanly
        ASSERT_TRUE(executor.execute("CREATE TABLE sample (id INT PRIMARY KEY, name TEXT);").success);
        ASSERT_TRUE(executor.execute("INSERT INTO sample VALUES (2, 'New Keyboard');").success);
        auto sel_new = executor.execute("SELECT * FROM sample;");
        ASSERT_TRUE(sel_new.success);
        ASSERT_EQ(sel_new.rows.size(), 1U);
        EXPECT_EQ(std::get<std::int64_t>(sel_new.rows[0].values[0]), 2);
    }
}

TEST(ParserTest, ParseCompoundWhereWithAndOr) {
    // 1. Simple AND
    {
        Parser parser("SELECT * FROM users WHERE id > 10 AND score <= 100;");
        auto res = parser.parse();
        ASSERT_TRUE(res.has_value()) << res.error().message();
        const auto& stmt = std::get<SelectStatement>(*res);
        ASSERT_NE(stmt.where_clause, nullptr);
        ASSERT_TRUE(stmt.where_clause->is_logical());
        const auto* log = stmt.where_clause->as_logical();
        EXPECT_EQ(log->op, LogicalOp::And);
        ASSERT_TRUE(log->left->is_binary());
        ASSERT_TRUE(log->right->is_binary());
        EXPECT_EQ(log->left->as_binary()->column_name, "id");
        EXPECT_EQ(log->right->as_binary()->column_name, "score");
    }

    // 2. OR with parentheses
    {
        Parser parser("SELECT * FROM users WHERE (role = 'admin' OR role = 'mod') AND active = 1;");
        auto res = parser.parse();
        ASSERT_TRUE(res.has_value()) << res.error().message();
        const auto& stmt = std::get<SelectStatement>(*res);
        ASSERT_NE(stmt.where_clause, nullptr);
        ASSERT_TRUE(stmt.where_clause->is_logical());
        const auto* log = stmt.where_clause->as_logical();
        EXPECT_EQ(log->op, LogicalOp::And);
        ASSERT_TRUE(log->left->is_logical());
        EXPECT_EQ(log->left->as_logical()->op, LogicalOp::Or);
    }
}

TEST_F(SQLExecutorTest, MultiConditionWhereAndOr) {
    qdb::Database db(db_path_);
    qdb::SQLExecutor executor(db);

    ASSERT_TRUE(executor.execute("CREATE TABLE products (id INT PRIMARY KEY, category TEXT, price INT, in_stock INT);").success);
    ASSERT_TRUE(executor.execute("INSERT INTO products VALUES (1, 'Electronics', 100, 1);").success);
    ASSERT_TRUE(executor.execute("INSERT INTO products VALUES (2, 'Electronics', 500, 0);").success);
    ASSERT_TRUE(executor.execute("INSERT INTO products VALUES (3, 'Books', 20, 1);").success);
    ASSERT_TRUE(executor.execute("INSERT INTO products VALUES (4, 'Books', 45, 1);").success);
    ASSERT_TRUE(executor.execute("INSERT INTO products VALUES (5, 'Clothing', 80, 0);").success);

    // 1. Test AND
    {
        auto res = executor.execute("SELECT * FROM products WHERE category = 'Electronics' AND price < 200;");
        ASSERT_TRUE(res.success) << res.message;
        ASSERT_EQ(res.rows.size(), 1U);
        EXPECT_EQ(std::get<std::int64_t>(res.rows[0].values[0]), 1);
    }

    // 2. Test OR
    {
        auto res = executor.execute("SELECT * FROM products WHERE category = 'Books' OR category = 'Clothing';");
        ASSERT_TRUE(res.success) << res.message;
        ASSERT_EQ(res.rows.size(), 3U);
    }

    // 3. Test Parenthesized AND / OR
    {
        auto res = executor.execute("SELECT * FROM products WHERE (category = 'Electronics' OR category = 'Books') AND in_stock = 1;");
        ASSERT_TRUE(res.success) << res.message;
        ASSERT_EQ(res.rows.size(), 3U); // ids 1, 3, 4
    }

    // 4. Test UPDATE with AND
    {
        auto upd = executor.execute("UPDATE products SET in_stock = 1 WHERE category = 'Clothing' AND price = 80;");
        ASSERT_TRUE(upd.success) << upd.message;
        EXPECT_EQ(upd.rows_affected, 1U);

        auto check = executor.execute("SELECT in_stock FROM products WHERE id = 5;");
        ASSERT_TRUE(check.success);
        ASSERT_EQ(check.rows.size(), 1U);
        EXPECT_EQ(std::get<std::int64_t>(check.rows[0].values[0]), 1);
    }

    // 5. Test DELETE with OR
    {
        auto del = executor.execute("DELETE FROM products WHERE price > 400 OR id = 1;");
        ASSERT_TRUE(del.success) << del.message;
        EXPECT_EQ(del.rows_affected, 2U); // ids 1 and 2 deleted

        auto check = executor.execute("SELECT * FROM products;");
        ASSERT_TRUE(check.success);
        EXPECT_EQ(check.rows.size(), 3U); // ids 3, 4, 5 remain
    }
}

TEST_F(SQLExecutorTest, PersistenceAcrossReopen) {
    // Session 1: Create and insert
    {
        qdb::Database db(db_path_);
        qdb::SQLExecutor executor(db);
        ASSERT_TRUE(executor.execute("CREATE TABLE notes (id INT PRIMARY KEY, body TEXT);").success);
        ASSERT_TRUE(executor.execute("INSERT INTO notes VALUES (1, 'Remember the milk');").success);
        ASSERT_TRUE(executor.execute("INSERT INTO notes VALUES (2, 'Call Alice');").success);
    }

    // Session 2: Reopen from disk and query through SQL
    {
        qdb::Database db(db_path_);
        qdb::SQLExecutor executor(db);

        auto res = executor.execute("SELECT * FROM notes WHERE id = 2;");
        ASSERT_TRUE(res.success) << res.message;
        ASSERT_EQ(res.rows.size(), 1U);
        EXPECT_EQ(std::get<std::string>(res.rows[0].values[1]), "Call Alice");

        auto scan = executor.execute("SELECT * FROM notes;");
        ASSERT_TRUE(scan.success);
        EXPECT_EQ(scan.rows.size(), 2U);
    }
}

TEST(ParserTest, ParseJoinStatements) {
    // 1. Basic INNER JOIN with ON
    {
        Parser parser("SELECT users.name, orders.amount FROM users INNER JOIN orders ON users.id = orders.user_id;");
        auto res = parser.parse();
        ASSERT_TRUE(res.has_value()) << res.error().message();
        const auto& stmt = std::get<SelectStatement>(*res);
        EXPECT_EQ(stmt.table_name, "users");
        ASSERT_EQ(stmt.columns.size(), 2U);
        EXPECT_EQ(stmt.columns[0], "users.name");
        EXPECT_EQ(stmt.columns[1], "orders.amount");
        ASSERT_EQ(stmt.joins.size(), 1U);
        EXPECT_EQ(stmt.joins[0].join_type, JoinType::Inner);
        EXPECT_EQ(stmt.joins[0].table_name, "orders");
        EXPECT_EQ(stmt.joins[0].left_column, "users.id");
        EXPECT_EQ(stmt.joins[0].right_column, "orders.user_id");
    }

    // 2. JOIN with WHERE
    {
        Parser parser("SELECT * FROM users JOIN orders ON users.id = orders.user_id WHERE orders.amount > 100;");
        auto res = parser.parse();
        ASSERT_TRUE(res.has_value()) << res.error().message();
        const auto& stmt = std::get<SelectStatement>(*res);
        EXPECT_TRUE(stmt.columns.empty()); // SELECT *
        ASSERT_EQ(stmt.joins.size(), 1U);
        ASSERT_NE(stmt.where_clause, nullptr);
    }

    // 3. LEFT JOIN
    {
        Parser parser("SELECT users.name, orders.product FROM users LEFT JOIN orders ON users.id = orders.user_id;");
        auto res = parser.parse();
        ASSERT_TRUE(res.has_value()) << res.error().message();
        const auto& stmt = std::get<SelectStatement>(*res);
        ASSERT_EQ(stmt.joins.size(), 1U);
        EXPECT_EQ(stmt.joins[0].join_type, JoinType::Left);
    }
}

TEST_F(SQLExecutorTest, JoinTwoTables) {
    qdb::Database db(db_path_);
    qdb::SQLExecutor executor(db);

    // Setup users table
    ASSERT_TRUE(executor.execute("CREATE TABLE users (id INT PRIMARY KEY, name TEXT);").success);
    ASSERT_TRUE(executor.execute("INSERT INTO users VALUES (1, 'Alice');").success);
    ASSERT_TRUE(executor.execute("INSERT INTO users VALUES (2, 'Bob');").success);
    ASSERT_TRUE(executor.execute("INSERT INTO users VALUES (3, 'Charlie');").success);

    // Setup orders table
    ASSERT_TRUE(executor.execute("CREATE TABLE orders (order_id INT PRIMARY KEY, user_id INT, product TEXT, amount INT);").success);
    ASSERT_TRUE(executor.execute("INSERT INTO orders VALUES (101, 1, 'Laptop', 1200);").success);
    ASSERT_TRUE(executor.execute("INSERT INTO orders VALUES (102, 1, 'Mouse', 25);").success);
    ASSERT_TRUE(executor.execute("INSERT INTO orders VALUES (103, 2, 'Keyboard', 75);").success);
    ASSERT_TRUE(executor.execute("INSERT INTO orders VALUES (104, 99, 'Orphan', 10);").success); // No matching user

    // 1. INNER JOIN (all columns: SELECT *)
    {
        auto res = executor.execute("SELECT * FROM users INNER JOIN orders ON users.id = orders.user_id;");
        ASSERT_TRUE(res.success) << res.message;
        ASSERT_EQ(res.rows.size(), 3U);
        EXPECT_EQ(res.column_names.size(), 6U); // users.id, users.name, orders.order_id, orders.user_id, orders.product, orders.amount
    }

    // 2. INNER JOIN with specific projected columns
    {
        auto res = executor.execute("SELECT users.name, orders.product, orders.amount FROM users JOIN orders ON users.id = orders.user_id;");
        ASSERT_TRUE(res.success) << res.message;
        ASSERT_EQ(res.rows.size(), 3U);
        ASSERT_EQ(res.column_names.size(), 3U);

        // Spot check row values
        EXPECT_EQ(std::get<std::string>(res.rows[0].values[0]), "Alice");
        EXPECT_EQ(std::get<std::string>(res.rows[0].values[1]), "Laptop");
        EXPECT_EQ(std::get<std::int64_t>(res.rows[0].values[2]), 1200);

        EXPECT_EQ(std::get<std::string>(res.rows[1].values[0]), "Alice");
        EXPECT_EQ(std::get<std::string>(res.rows[1].values[1]), "Mouse");
        EXPECT_EQ(std::get<std::int64_t>(res.rows[1].values[2]), 25);

        EXPECT_EQ(std::get<std::string>(res.rows[2].values[0]), "Bob");
        EXPECT_EQ(std::get<std::string>(res.rows[2].values[1]), "Keyboard");
        EXPECT_EQ(std::get<std::int64_t>(res.rows[2].values[2]), 75);
    }

    // 3. INNER JOIN with WHERE filter
    {
        auto res = executor.execute("SELECT users.name, orders.product FROM users JOIN orders ON users.id = orders.user_id WHERE orders.amount > 50;");
        ASSERT_TRUE(res.success) << res.message;
        ASSERT_EQ(res.rows.size(), 2U); // Alice (Laptop), Bob (Keyboard)
        EXPECT_EQ(std::get<std::string>(res.rows[0].values[1]), "Laptop");
        EXPECT_EQ(std::get<std::string>(res.rows[1].values[1]), "Keyboard");
    }

    // 4. Reversible ON condition: ON orders.user_id = users.id
    {
        auto res = executor.execute("SELECT users.name, orders.product FROM users JOIN orders ON orders.user_id = users.id;");
        ASSERT_TRUE(res.success) << res.message;
        ASSERT_EQ(res.rows.size(), 3U);
    }

    // 5. LEFT JOIN (Charlie has no orders, should appear with NULLs)
    {
        auto res = executor.execute("SELECT users.name, orders.product FROM users LEFT JOIN orders ON users.id = orders.user_id;");
        ASSERT_TRUE(res.success) << res.message;
        ASSERT_EQ(res.rows.size(), 4U); // Alice (Laptop), Alice (Mouse), Bob (Keyboard), Charlie (NULL)

        // Find Charlie's row
        bool found_charlie = false;
        for (const auto& row : res.rows) {
            if (std::get<std::string>(row.values[0]) == "Charlie") {
                found_charlie = true;
                EXPECT_TRUE(std::holds_alternative<std::monostate>(row.values[1])); // NULL product
            }
        }
        EXPECT_TRUE(found_charlie);
    }

    // 6. Table wildcard projection in JOIN (users.*)
    {
        auto res = executor.execute("SELECT users.*, orders.product FROM users JOIN orders ON users.id = orders.user_id;");
        ASSERT_TRUE(res.success) << res.message;
        ASSERT_EQ(res.rows.size(), 3U);
        EXPECT_EQ(res.column_names.size(), 3U); // users.id, users.name, orders.product
        EXPECT_EQ(res.column_names[0], "users.id");
        EXPECT_EQ(res.column_names[1], "users.name");
        EXPECT_EQ(res.column_names[2], "orders.product");
    }

    // 7. Qualified column names in single table query
    {
        auto res = executor.execute("SELECT users.name FROM users WHERE users.id = 2;");
        ASSERT_TRUE(res.success) << res.message;
        ASSERT_EQ(res.rows.size(), 1U);
        EXPECT_EQ(std::get<std::string>(res.rows[0].values[0]), "Bob");
    }
}

TEST_F(SQLExecutorTest, NonPrimaryKeyUpdateAndDelete) {
    {
        qdb::Database db(db_path_);
        qdb::SQLExecutor executor(db);

        // 1. Create table without a primary key
        auto create_res = executor.execute("CREATE TABLE audit_logs (service TEXT NOT NULL, message TEXT, severity TEXT);");
        ASSERT_TRUE(create_res.success) << create_res.message;

        // 2. Insert records
        ASSERT_TRUE(executor.execute("INSERT INTO audit_logs VALUES ('auth', 'login success', 'INFO');").success);
        ASSERT_TRUE(executor.execute("INSERT INTO audit_logs VALUES ('auth', 'bad password attempt', 'WARN');").success);
        ASSERT_TRUE(executor.execute("INSERT INTO audit_logs VALUES ('billing', 'card charged', 'INFO');").success);
        ASSERT_TRUE(executor.execute("INSERT INTO audit_logs VALUES ('billing', 'charge declined', 'WARN');").success);
        ASSERT_TRUE(executor.execute("INSERT INTO audit_logs VALUES ('infra', 'high cpu utilization detected', 'WARN');").success);

        auto sel_all = executor.execute("SELECT * FROM audit_logs;");
        ASSERT_TRUE(sel_all.success);
        EXPECT_EQ(sel_all.rows.size(), 5U);

        // 3. Update multiple non-PK rows matching WHERE clause
        auto upd_res = executor.execute("UPDATE audit_logs SET severity = 'ALERT' WHERE severity = 'WARN';");
        ASSERT_TRUE(upd_res.success) << upd_res.message;
        EXPECT_EQ(upd_res.rows_affected, 3U);

        auto check_alerts = executor.execute("SELECT * FROM audit_logs WHERE severity = 'ALERT';");
        ASSERT_TRUE(check_alerts.success);
        EXPECT_EQ(check_alerts.rows.size(), 3U);

        // 4. Update with larger string payload (tests page capacity and in-place/rebuild handling)
        auto upd_large = executor.execute("UPDATE audit_logs SET message = 'expanded message with substantial additional explanatory detail for the incident report' WHERE service = 'infra';");
        ASSERT_TRUE(upd_large.success) << upd_large.message;
        EXPECT_EQ(upd_large.rows_affected, 1U);

        auto check_infra = executor.execute("SELECT message FROM audit_logs WHERE service = 'infra';");
        ASSERT_TRUE(check_infra.success);
        ASSERT_EQ(check_infra.rows.size(), 1U);
        EXPECT_EQ(std::get<std::string>(check_infra.rows[0].values[0]),
                  "expanded message with substantial additional explanatory detail for the incident report");

        // 5. Delete specific non-PK rows
        auto del_res = executor.execute("DELETE FROM audit_logs WHERE severity = 'INFO';");
        ASSERT_TRUE(del_res.success) << del_res.message;
        EXPECT_EQ(del_res.rows_affected, 2U);

        auto check_remaining = executor.execute("SELECT * FROM audit_logs;");
        ASSERT_TRUE(check_remaining.success);
        EXPECT_EQ(check_remaining.rows.size(), 3U);

        // 6. Delete all remaining rows unconditionally
        auto del_all = executor.execute("DELETE FROM audit_logs;");
        ASSERT_TRUE(del_all.success) << del_all.message;
        EXPECT_EQ(del_all.rows_affected, 3U);

        auto check_empty = executor.execute("SELECT * FROM audit_logs;");
        ASSERT_TRUE(check_empty.success);
        EXPECT_EQ(check_empty.rows.size(), 0U);
    }

    // 7. Verify persistence across database reopen
    {
        qdb::Database db(db_path_);
        qdb::SQLExecutor executor(db);

        auto check_after_reopen = executor.execute("SELECT * FROM audit_logs;");
        ASSERT_TRUE(check_after_reopen.success);
        EXPECT_EQ(check_after_reopen.rows.size(), 0U);

        // Insert new record into reopened non-PK table
        auto ins_reopen = executor.execute("INSERT INTO audit_logs VALUES ('kernel', 'boot complete', 'NOTICE');");
        ASSERT_TRUE(ins_reopen.success);

        auto final_sel = executor.execute("SELECT * FROM audit_logs;");
        ASSERT_TRUE(final_sel.success);
        EXPECT_EQ(final_sel.rows.size(), 1U);
    }
}

TEST_F(SQLExecutorTest, OrderByAndLimitPagination) {
    qdb::Database db(db_path_);
    qdb::SQLExecutor executor(db);

    // 1. Create table and insert test dataset
    ASSERT_TRUE(executor.execute("CREATE TABLE scores (id INT PRIMARY KEY, player TEXT NOT NULL, score INT, note TEXT);").success);
    ASSERT_TRUE(executor.execute("INSERT INTO scores VALUES (1, 'Alice', 450, 'pro');").success);
    ASSERT_TRUE(executor.execute("INSERT INTO scores VALUES (2, 'Bob', 120, NULL);").success);
    ASSERT_TRUE(executor.execute("INSERT INTO scores VALUES (3, 'Charlie', 890, 'expert');").success);
    ASSERT_TRUE(executor.execute("INSERT INTO scores VALUES (4, 'David', 300, NULL);").success);
    ASSERT_TRUE(executor.execute("INSERT INTO scores VALUES (5, 'Eve', 750, 'champion');").success);
    ASSERT_TRUE(executor.execute("INSERT INTO scores VALUES (6, 'Frank', 120, 'rookie');").success);

    // 2. ORDER BY score ASC (integer ascending)
    {
        auto res = executor.execute("SELECT player, score FROM scores ORDER BY score ASC;");
        ASSERT_TRUE(res.success) << res.message;
        ASSERT_EQ(res.rows.size(), 6U);
        EXPECT_EQ(std::get<std::int64_t>(res.rows[0].values[1]), 120);
        EXPECT_EQ(std::get<std::int64_t>(res.rows[1].values[1]), 120);
        EXPECT_EQ(std::get<std::int64_t>(res.rows[2].values[1]), 300);
        EXPECT_EQ(std::get<std::int64_t>(res.rows[3].values[1]), 450);
        EXPECT_EQ(std::get<std::int64_t>(res.rows[4].values[1]), 750);
        EXPECT_EQ(std::get<std::int64_t>(res.rows[5].values[1]), 890);
    }

    // 3. ORDER BY score DESC (integer descending)
    {
        auto res = executor.execute("SELECT player, score FROM scores ORDER BY score DESC;");
        ASSERT_TRUE(res.success) << res.message;
        ASSERT_EQ(res.rows.size(), 6U);
        EXPECT_EQ(std::get<std::int64_t>(res.rows[0].values[1]), 890);
        EXPECT_EQ(std::get<std::string>(res.rows[0].values[0]), "Charlie");
        EXPECT_EQ(std::get<std::int64_t>(res.rows[1].values[1]), 750);
        EXPECT_EQ(std::get<std::string>(res.rows[1].values[0]), "Eve");
        EXPECT_EQ(std::get<std::int64_t>(res.rows[2].values[1]), 450);
        EXPECT_EQ(std::get<std::string>(res.rows[2].values[0]), "Alice");
    }

    // 4. ORDER BY player ASC / DESC (text column)
    {
        auto res_asc = executor.execute("SELECT player FROM scores ORDER BY player ASC;");
        ASSERT_TRUE(res_asc.success);
        ASSERT_EQ(res_asc.rows.size(), 6U);
        EXPECT_EQ(std::get<std::string>(res_asc.rows[0].values[0]), "Alice");
        EXPECT_EQ(std::get<std::string>(res_asc.rows[5].values[0]), "Frank");

        auto res_desc = executor.execute("SELECT player FROM scores ORDER BY player DESC;");
        ASSERT_TRUE(res_desc.success);
        ASSERT_EQ(res_desc.rows.size(), 6U);
        EXPECT_EQ(std::get<std::string>(res_desc.rows[0].values[0]), "Frank");
        EXPECT_EQ(std::get<std::string>(res_desc.rows[5].values[0]), "Alice");
    }

    // 5. ORDER BY with NULL values (NULLs sort first in ASC, last in DESC)
    {
        auto res_null_asc = executor.execute("SELECT player, note FROM scores ORDER BY note ASC;");
        ASSERT_TRUE(res_null_asc.success);
        ASSERT_EQ(res_null_asc.rows.size(), 6U);
        EXPECT_TRUE(std::holds_alternative<std::monostate>(res_null_asc.rows[0].values[1]));
        EXPECT_TRUE(std::holds_alternative<std::monostate>(res_null_asc.rows[1].values[1]));

        auto res_null_desc = executor.execute("SELECT player, note FROM scores ORDER BY note DESC;");
        ASSERT_TRUE(res_null_desc.success);
        ASSERT_EQ(res_null_desc.rows.size(), 6U);
        EXPECT_TRUE(std::holds_alternative<std::monostate>(res_null_desc.rows[4].values[1]));
        EXPECT_TRUE(std::holds_alternative<std::monostate>(res_null_desc.rows[5].values[1]));
    }

    // 6. ORDER BY unprojected column
    {
        auto res = executor.execute("SELECT player FROM scores ORDER BY score DESC;");
        ASSERT_TRUE(res.success);
        ASSERT_EQ(res.rows.size(), 6U);
        ASSERT_EQ(res.column_names.size(), 1U);
        EXPECT_EQ(std::get<std::string>(res.rows[0].values[0]), "Charlie");
        EXPECT_EQ(std::get<std::string>(res.rows[1].values[0]), "Eve");
    }

    // 7. LIMIT alone
    {
        auto res = executor.execute("SELECT * FROM scores LIMIT 3;");
        ASSERT_TRUE(res.success);
        EXPECT_EQ(res.rows.size(), 3U);
    }

    // 8. ORDER BY with LIMIT (Top-N query)
    {
        auto res = executor.execute("SELECT player, score FROM scores ORDER BY score DESC LIMIT 3;");
        ASSERT_TRUE(res.success);
        ASSERT_EQ(res.rows.size(), 3U);
        EXPECT_EQ(std::get<std::string>(res.rows[0].values[0]), "Charlie");
        EXPECT_EQ(std::get<std::string>(res.rows[1].values[0]), "Eve");
        EXPECT_EQ(std::get<std::string>(res.rows[2].values[0]), "Alice");
    }

    // 9. LIMIT with OFFSET (Pagination)
    {
        // Page 1: 2 items
        auto page1 = executor.execute("SELECT player, score FROM scores ORDER BY score DESC LIMIT 2 OFFSET 0;");
        ASSERT_TRUE(page1.success);
        ASSERT_EQ(page1.rows.size(), 2U);
        EXPECT_EQ(std::get<std::string>(page1.rows[0].values[0]), "Charlie");
        EXPECT_EQ(std::get<std::string>(page1.rows[1].values[0]), "Eve");

        // Page 2: 2 items (offset 2)
        auto page2 = executor.execute("SELECT player, score FROM scores ORDER BY score DESC LIMIT 2 OFFSET 2;");
        ASSERT_TRUE(page2.success);
        ASSERT_EQ(page2.rows.size(), 2U);
        EXPECT_EQ(std::get<std::string>(page2.rows[0].values[0]), "Alice");
        EXPECT_EQ(std::get<std::string>(page2.rows[1].values[0]), "David");

        // Page 3: 2 items (offset 4)
        auto page3 = executor.execute("SELECT player, score FROM scores ORDER BY score DESC LIMIT 2 OFFSET 4;");
        ASSERT_TRUE(page3.success);
        ASSERT_EQ(page3.rows.size(), 2U);
        EXPECT_EQ(std::get<std::int64_t>(page3.rows[0].values[1]), 120);
        EXPECT_EQ(std::get<std::int64_t>(page3.rows[1].values[1]), 120);
    }

    // 10. OFFSET exceeding row count
    {
        auto res = executor.execute("SELECT * FROM scores LIMIT 10 OFFSET 50;");
        ASSERT_TRUE(res.success);
        EXPECT_EQ(res.rows.size(), 0U);
    }

    // 11. Combined WHERE, ORDER BY, and LIMIT
    {
        auto res = executor.execute("SELECT player, score FROM scores WHERE score > 200 ORDER BY score ASC LIMIT 2;");
        ASSERT_TRUE(res.success);
        ASSERT_EQ(res.rows.size(), 2U);
        EXPECT_EQ(std::get<std::string>(res.rows[0].values[0]), "David");
        EXPECT_EQ(std::get<std::int64_t>(res.rows[0].values[1]), 300);
        EXPECT_EQ(std::get<std::string>(res.rows[1].values[0]), "Alice");
        EXPECT_EQ(std::get<std::int64_t>(res.rows[1].values[1]), 450);
    }

    // 12. Non-existent column in ORDER BY returns error
    {
        auto res = executor.execute("SELECT * FROM scores ORDER BY imaginary_col;");
        ASSERT_FALSE(res.success);
    }
}

TEST_F(SQLExecutorTest, AggregateFunctions) {
    qdb::Database db(db_path_);
    qdb::SQLExecutor executor(db);

    // Setup: scores table with various scores and NULL notes
    ASSERT_TRUE(executor.execute(
        "CREATE TABLE scores (id INT PRIMARY KEY, player TEXT NOT NULL, score INT, note TEXT);"
    ).success);

    ASSERT_TRUE(executor.execute("INSERT INTO scores VALUES (1, 'Alice', 100, 'first');").success);
    ASSERT_TRUE(executor.execute("INSERT INTO scores VALUES (2, 'Bob', 200, 'second');").success);
    ASSERT_TRUE(executor.execute("INSERT INTO scores VALUES (3, 'Charlie', 300, NULL);").success);
    ASSERT_TRUE(executor.execute("INSERT INTO scores VALUES (4, 'David', 400, 'fourth');").success);
    ASSERT_TRUE(executor.execute("INSERT INTO scores VALUES (5, 'Eve', NULL, NULL);").success);

    // 1. COUNT(*) vs COUNT(column) with NULLs
    {
        auto res = executor.execute("SELECT COUNT(*), COUNT(score), COUNT(note) FROM scores;");
        ASSERT_TRUE(res.success);
        ASSERT_EQ(res.rows.size(), 1U);
        ASSERT_EQ(res.column_names.size(), 3U);
        EXPECT_EQ(res.column_names[0], "COUNT(*)");
        EXPECT_EQ(res.column_names[1], "COUNT(score)");
        EXPECT_EQ(res.column_names[2], "COUNT(note)");
        EXPECT_EQ(std::get<std::int64_t>(res.rows[0].values[0]), 5);
        EXPECT_EQ(std::get<std::int64_t>(res.rows[0].values[1]), 4);
        EXPECT_EQ(std::get<std::int64_t>(res.rows[0].values[2]), 3);
    }

    // 2. SUM and AVG (skips NULLs)
    {
        // Scores are 100, 200, 300, 400 (Eve is NULL). Sum = 1000, Count = 4, Avg = 250
        auto res = executor.execute("SELECT SUM(score), AVG(score) FROM scores;");
        ASSERT_TRUE(res.success);
        ASSERT_EQ(res.rows.size(), 1U);
        EXPECT_EQ(std::get<std::int64_t>(res.rows[0].values[0]), 1000);
        EXPECT_EQ(std::get<std::int64_t>(res.rows[0].values[1]), 250);
    }

    // 3. MIN and MAX on integer column
    {
        auto res = executor.execute("SELECT MIN(score), MAX(score) FROM scores;");
        ASSERT_TRUE(res.success);
        ASSERT_EQ(res.rows.size(), 1U);
        EXPECT_EQ(std::get<std::int64_t>(res.rows[0].values[0]), 100);
        EXPECT_EQ(std::get<std::int64_t>(res.rows[0].values[1]), 400);
    }

    // 4. MIN and MAX on text column
    {
        auto res = executor.execute("SELECT MIN(player), MAX(player) FROM scores;");
        ASSERT_TRUE(res.success);
        ASSERT_EQ(res.rows.size(), 1U);
        EXPECT_EQ(std::get<std::string>(res.rows[0].values[0]), "Alice");
        EXPECT_EQ(std::get<std::string>(res.rows[0].values[1]), "Eve");
    }

    // 5. Aggregates with WHERE clause
    {
        auto res = executor.execute("SELECT COUNT(*), SUM(score), AVG(score) FROM scores WHERE score >= 200;");
        ASSERT_TRUE(res.success);
        ASSERT_EQ(res.rows.size(), 1U);
        EXPECT_EQ(std::get<std::int64_t>(res.rows[0].values[0]), 3); // 200, 300, 400
        EXPECT_EQ(std::get<std::int64_t>(res.rows[0].values[1]), 900);
        EXPECT_EQ(std::get<std::int64_t>(res.rows[0].values[2]), 300);
    }

    // 6. Aggregates on empty result set (COUNT returns 0, SUM/AVG/MIN/MAX return NULL)
    {
        auto res = executor.execute("SELECT COUNT(*), COUNT(score), SUM(score), AVG(score), MIN(score), MAX(score) FROM scores WHERE score > 9999;");
        ASSERT_TRUE(res.success);
        ASSERT_EQ(res.rows.size(), 1U);
        EXPECT_EQ(std::get<std::int64_t>(res.rows[0].values[0]), 0);
        EXPECT_EQ(std::get<std::int64_t>(res.rows[0].values[1]), 0);
        EXPECT_TRUE(std::holds_alternative<std::monostate>(res.rows[0].values[2]));
        EXPECT_TRUE(std::holds_alternative<std::monostate>(res.rows[0].values[3]));
        EXPECT_TRUE(std::holds_alternative<std::monostate>(res.rows[0].values[4]));
        EXPECT_TRUE(std::holds_alternative<std::monostate>(res.rows[0].values[5]));
    }

    // 7. Type safety: SUM / AVG on non-integer columns fail
    {
        auto res_sum = executor.execute("SELECT SUM(player) FROM scores;");
        ASSERT_FALSE(res_sum.success);

        auto res_avg = executor.execute("SELECT AVG(player) FROM scores;");
        ASSERT_FALSE(res_avg.success);
    }

    // 8. Aggregates with LIMIT 0 / LIMIT 1
    {
        auto res_zero = executor.execute("SELECT COUNT(*) FROM scores LIMIT 0;");
        ASSERT_TRUE(res_zero.success);
        EXPECT_EQ(res_zero.rows.size(), 0U);

        auto res_one = executor.execute("SELECT COUNT(*) FROM scores LIMIT 1;");
        ASSERT_TRUE(res_one.success);
        ASSERT_EQ(res_one.rows.size(), 1U);
        EXPECT_EQ(std::get<std::int64_t>(res_one.rows[0].values[0]), 5);
    }

    // 9. Aggregates over JOIN
    {
        ASSERT_TRUE(executor.execute(
            "CREATE TABLE teams (team_id INT PRIMARY KEY, player_id INT, division TEXT);"
        ).success);
        ASSERT_TRUE(executor.execute("INSERT INTO teams VALUES (1, 1, 'West');").success);
        ASSERT_TRUE(executor.execute("INSERT INTO teams VALUES (2, 2, 'West');").success);
        ASSERT_TRUE(executor.execute("INSERT INTO teams VALUES (3, 3, 'East');").success);

        auto res = executor.execute(
            "SELECT COUNT(*), SUM(scores.score) FROM scores JOIN teams ON scores.id = teams.player_id WHERE teams.division = 'West';"
        );
        ASSERT_TRUE(res.success);
        ASSERT_EQ(res.rows.size(), 1U);
        EXPECT_EQ(std::get<std::int64_t>(res.rows[0].values[0]), 2); // Alice and Bob
        EXPECT_EQ(std::get<std::int64_t>(res.rows[0].values[1]), 300); // 100 + 200
    }
}
