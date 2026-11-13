#include <gtest/gtest.h>
#include <quartz/quartz.h>

#include <filesystem>
#include <string>
#include <vector>

class CAPITest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        db_path_ = std::filesystem::temp_directory_path() / (std::string("qdb_capi_") + info->name() + ".db");
        std::error_code ec;
        std::filesystem::remove(db_path_, ec);
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove(db_path_, ec);
    }

    std::filesystem::path db_path_;
};

TEST_F(CAPITest, LifecycleOpenClose) {
    qdb_database_t* db = nullptr;
    const std::string path_str = db_path_.string();

    // 1. Valid open
    int rc = qdb_open(path_str.c_str(), &db);
    ASSERT_EQ(rc, QDB_OK);
    ASSERT_NE(db, nullptr);

    // 2. Safe close
    qdb_close(db);

    // 3. Null pointer guards
    EXPECT_EQ(qdb_open(nullptr, &db), QDB_NULL_POINTER);
    EXPECT_EQ(qdb_open(path_str.c_str(), nullptr), QDB_NULL_POINTER);
    EXPECT_NO_THROW(qdb_close(nullptr));
}

TEST_F(CAPITest, TableCreationAndInsertion) {
    qdb_database_t* db = nullptr;
    const std::string path_str = db_path_.string();
    ASSERT_EQ(qdb_open(path_str.c_str(), &db), QDB_OK);

    // 1. CREATE TABLE
    qdb_result_t* res = nullptr;
    int rc = qdb_execute(db, "CREATE TABLE inventory (id INT PRIMARY KEY, item TEXT NOT NULL, qty INT);", &res);
    ASSERT_EQ(rc, QDB_OK);
    ASSERT_NE(res, nullptr);
    EXPECT_EQ(qdb_result_is_success(res), 1);
    EXPECT_EQ(qdb_result_rows_affected(res), 0U);
    qdb_result_free(res);

    // 2. INSERT records
    rc = qdb_execute(db, "INSERT INTO inventory VALUES (1, 'Apples', 50);", &res);
    ASSERT_EQ(rc, QDB_OK);
    EXPECT_EQ(qdb_result_rows_affected(res), 1U);
    qdb_result_free(res);

    rc = qdb_execute(db, "INSERT INTO inventory VALUES (2, 'Oranges', 30);", &res);
    ASSERT_EQ(rc, QDB_OK);
    EXPECT_EQ(qdb_result_rows_affected(res), 1U);
    qdb_result_free(res);

    rc = qdb_execute(db, "INSERT INTO inventory VALUES (3, 'Bananas', NULL);", &res);
    ASSERT_EQ(rc, QDB_OK);
    EXPECT_EQ(qdb_result_rows_affected(res), 1U);
    qdb_result_free(res);

    qdb_close(db);
}

TEST_F(CAPITest, SelectAndTypedColumnAccess) {
    qdb_database_t* db = nullptr;
    const std::string path_str = db_path_.string();
    ASSERT_EQ(qdb_open(path_str.c_str(), &db), QDB_OK);

    ASSERT_EQ(qdb_execute(db, "CREATE TABLE inventory (id INT PRIMARY KEY, item TEXT NOT NULL, qty INT);", nullptr), QDB_OK);
    ASSERT_EQ(qdb_execute(db, "INSERT INTO inventory VALUES (1, 'Apples', 50);", nullptr), QDB_OK);
    ASSERT_EQ(qdb_execute(db, "INSERT INTO inventory VALUES (2, 'Oranges', 30);", nullptr), QDB_OK);
    ASSERT_EQ(qdb_execute(db, "INSERT INTO inventory VALUES (3, 'Bananas', NULL);", nullptr), QDB_OK);

    qdb_result_t* res = nullptr;
    int rc = qdb_execute(db, "SELECT id, item, qty FROM inventory ORDER BY id ASC;", &res);
    ASSERT_EQ(rc, QDB_OK);
    ASSERT_NE(res, nullptr);

    // Column metadata
    EXPECT_EQ(qdb_result_column_count(res), 3U);
    EXPECT_STREQ(qdb_result_column_name(res, 0), "id");
    EXPECT_STREQ(qdb_result_column_name(res, 1), "item");
    EXPECT_STREQ(qdb_result_column_name(res, 2), "qty");

    // Row count
    ASSERT_EQ(qdb_result_row_count(res), 3U);

    // Row 0: 1 | Apples | 50
    EXPECT_EQ(qdb_column_type(res, 0, 0), QDB_TYPE_INT);
    EXPECT_EQ(qdb_column_int(res, 0, 0), 1);
    EXPECT_EQ(qdb_column_type(res, 0, 1), QDB_TYPE_TEXT);
    EXPECT_STREQ(qdb_column_text(res, 0, 1), "Apples");
    EXPECT_EQ(qdb_column_type(res, 0, 2), QDB_TYPE_INT);
    EXPECT_EQ(qdb_column_int(res, 0, 2), 50);
    EXPECT_EQ(qdb_column_is_null(res, 0, 2), 0);

    // Row 1: 2 | Oranges | 30
    EXPECT_EQ(qdb_column_int(res, 1, 0), 2);
    EXPECT_STREQ(qdb_column_text(res, 1, 1), "Oranges");
    EXPECT_EQ(qdb_column_int(res, 1, 2), 30);

    // Row 2: 3 | Bananas | NULL
    EXPECT_EQ(qdb_column_int(res, 2, 0), 3);
    EXPECT_STREQ(qdb_column_text(res, 2, 1), "Bananas");
    EXPECT_EQ(qdb_column_type(res, 2, 2), QDB_TYPE_NULL);
    EXPECT_EQ(qdb_column_is_null(res, 2, 2), 1);
    EXPECT_EQ(qdb_column_text(res, 2, 2), nullptr);

    // Cross-type access: int column via text accessor
    EXPECT_STREQ(qdb_column_text(res, 0, 0), "1");

    // Out-of-bounds safety
    EXPECT_EQ(qdb_column_type(res, 99, 99), QDB_TYPE_NULL);
    EXPECT_EQ(qdb_column_is_null(res, 99, 99), 1);
    EXPECT_EQ(qdb_column_int(res, 99, 99), 0);
    EXPECT_EQ(qdb_column_text(res, 99, 99), nullptr);

    qdb_result_free(res);
    qdb_close(db);
}

TEST_F(CAPITest, AggregateQueries) {
    qdb_database_t* db = nullptr;
    const std::string path_str = db_path_.string();
    ASSERT_EQ(qdb_open(path_str.c_str(), &db), QDB_OK);

    ASSERT_EQ(qdb_execute(db, "CREATE TABLE inventory (id INT PRIMARY KEY, item TEXT, qty INT);", nullptr), QDB_OK);
    ASSERT_EQ(qdb_execute(db, "INSERT INTO inventory VALUES (1, 'Apples', 50);", nullptr), QDB_OK);
    ASSERT_EQ(qdb_execute(db, "INSERT INTO inventory VALUES (2, 'Oranges', 30);", nullptr), QDB_OK);
    ASSERT_EQ(qdb_execute(db, "INSERT INTO inventory VALUES (3, 'Bananas', NULL);", nullptr), QDB_OK);

    qdb_result_t* res = nullptr;
    int rc = qdb_execute(db, "SELECT COUNT(*), COUNT(qty), SUM(qty), AVG(qty), MIN(qty), MAX(qty) FROM inventory;", &res);
    ASSERT_EQ(rc, QDB_OK);
    ASSERT_NE(res, nullptr);
    ASSERT_EQ(qdb_result_row_count(res), 1U);
    ASSERT_EQ(qdb_result_column_count(res), 6U);

    EXPECT_EQ(qdb_column_int(res, 0, 0), 3); // COUNT(*)
    EXPECT_EQ(qdb_column_int(res, 0, 1), 2); // COUNT(qty) excluding NULL
    EXPECT_EQ(qdb_column_int(res, 0, 2), 80); // SUM(qty) = 50 + 30
    EXPECT_EQ(qdb_column_int(res, 0, 3), 40); // AVG(qty) = 80 / 2
    EXPECT_EQ(qdb_column_int(res, 0, 4), 30); // MIN(qty)
    EXPECT_EQ(qdb_column_int(res, 0, 5), 50); // MAX(qty)

    qdb_result_free(res);
    qdb_close(db);
}

TEST_F(CAPITest, ErrorHandling) {
    qdb_database_t* db = nullptr;
    const std::string path_str = db_path_.string();
    ASSERT_EQ(qdb_open(path_str.c_str(), &db), QDB_OK);

    // Query non-existent table
    qdb_result_t* res = nullptr;
    int rc = qdb_execute(db, "SELECT * FROM imaginary_table;", &res);
    EXPECT_EQ(rc, QDB_ERROR);
    ASSERT_NE(res, nullptr);
    EXPECT_EQ(qdb_result_is_success(res), 0);
    EXPECT_NE(qdb_errmsg(db), nullptr);
    EXPECT_STRNE(qdb_errmsg(db), "");

    qdb_result_free(res);
    qdb_close(db);
}

TEST_F(CAPITest, CallbackExecAPI) {
    qdb_database_t* db = nullptr;
    const std::string path_str = db_path_.string();
    ASSERT_EQ(qdb_open(path_str.c_str(), &db), QDB_OK);

    ASSERT_EQ(qdb_execute(db, "CREATE TABLE users (id INT PRIMARY KEY, name TEXT);", nullptr), QDB_OK);
    ASSERT_EQ(qdb_execute(db, "INSERT INTO users VALUES (1, 'Alice');", nullptr), QDB_OK);
    ASSERT_EQ(qdb_execute(db, "INSERT INTO users VALUES (2, 'Bob');", nullptr), QDB_OK);

    struct Accumulator {
        std::vector<std::vector<std::string>> rows;
    } acc;

    auto callback = [](void* user_data, int col_count, char** col_values, char** /*col_names*/) -> int {
        auto* data = static_cast<Accumulator*>(user_data);
        std::vector<std::string> row;
        for (int i = 0; i < col_count; ++i) {
            row.push_back(col_values[i] ? col_values[i] : "NULL");
        }
        data->rows.push_back(std::move(row));
        return 0; // continue
    };

    char* errmsg = nullptr;
    int rc = qdb_exec(db, "SELECT id, name FROM users ORDER BY id ASC;", callback, &acc, &errmsg);
    EXPECT_EQ(rc, QDB_OK);
    EXPECT_EQ(errmsg, nullptr);
    ASSERT_EQ(acc.rows.size(), 2U);
    EXPECT_EQ(acc.rows[0][0], "1");
    EXPECT_EQ(acc.rows[0][1], "Alice");
    EXPECT_EQ(acc.rows[1][0], "2");
    EXPECT_EQ(acc.rows[1][1], "Bob");

    // Abort test: return 1 from callback
    auto abort_callback = [](void*, int, char**, char**) -> int {
        return 1; // abort immediately
    };
    rc = qdb_exec(db, "SELECT * FROM users;", abort_callback, nullptr, &errmsg);
    EXPECT_EQ(rc, QDB_ERROR);

    // Free test error
    qdb_free(errmsg);
    qdb_close(db);
}

TEST_F(CAPITest, VersionInfo) {
    const char* ver = qdb_version();
    ASSERT_NE(ver, nullptr);
    EXPECT_STREQ(ver, "0.1.0");
}
