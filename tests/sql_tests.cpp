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
        EXPECT_FALSE(stmt.where_clause.has_value());
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

        ASSERT_TRUE(stmt.where_clause.has_value());
        EXPECT_EQ(stmt.where_clause->column_name, "id");
        EXPECT_EQ(stmt.where_clause->op, BinaryOp::Equal);
        EXPECT_EQ(std::get<std::int64_t>(stmt.where_clause->literal_value), 100);
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

    ASSERT_TRUE(stmt.where_clause.has_value());
    EXPECT_EQ(stmt.where_clause->column_name, "id");
    EXPECT_EQ(stmt.where_clause->op, BinaryOp::Equal);
    EXPECT_EQ(std::get<std::int64_t>(stmt.where_clause->literal_value), 5);
}

TEST(ParserTest, ParseDeleteStatement) {
    const std::string sql = "DELETE FROM users WHERE age >= 21;";
    Parser parser(sql);
    auto res = parser.parse();
    ASSERT_TRUE(res.has_value()) << res.error().message();

    ASSERT_TRUE(std::holds_alternative<DeleteStatement>(*res));
    const auto& stmt = std::get<DeleteStatement>(*res);
    EXPECT_EQ(stmt.table_name, "users");

    ASSERT_TRUE(stmt.where_clause.has_value());
    EXPECT_EQ(stmt.where_clause->column_name, "age");
    EXPECT_EQ(stmt.where_clause->op, BinaryOp::GreaterThanOrEqual);
    EXPECT_EQ(std::get<std::int64_t>(stmt.where_clause->literal_value), 21);
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
