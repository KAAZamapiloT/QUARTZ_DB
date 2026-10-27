// tests/database_test.cpp
#include <quartz/database.hpp>
#include <storage/record.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Adapter layer: the ONLY place that depends on the Table API.
// Adjust these three functions to match your actual Table interface.
// ---------------------------------------------------------------------------
namespace {

    // Returns the table's first page ID (used to verify tables don't share pages).
    std::uint32_t first_page_id(const qdb::Table& t) {
        return static_cast<std::uint32_t>(t.first_page());
    }

    // Inserts one record containing `payload` into the table.
    void insert_record(qdb::Table& t, const std::string& payload) {
        t.insert(qdb::Record{ .values = { payload } });
    }

    // Returns every record in the table as a string (order-independent compare).
    std::vector<std::string> read_all(qdb::Table& t) {
        std::vector<std::string> out;
        for (const auto& rec : t.scan()) {
            if (!rec.values.empty()) {
                if (const auto* str = std::get_if<std::string>(&rec.values[0])) {
                    out.push_back(*str);
                }
            }
        }
        std::sort(out.begin(), out.end());
        return out;
    }

    std::vector<std::string> sorted(std::vector<std::string> v) {
        std::sort(v.begin(), v.end());
        return v;
    }

} // namespace

// ---------------------------------------------------------------------------
// Fixture: gives each test a unique temp file and cleans it up afterwards.
// ---------------------------------------------------------------------------
class DatabaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        static std::atomic<int> counter{ 0 };
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        path_ = fs::temp_directory_path() /
            ("qdb_" + std::string(info->name()) + "_" +
                std::to_string(counter++) + ".db");
        fs::remove(path_);
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove(path_, ec);
    }

    std::unique_ptr<qdb::Database> open() {
        return std::make_unique<qdb::Database>(path_);
    }

    fs::path path_;
};

// ---------------------------------------------------------------------------
// 1. Open a new database -> valid catalog, zero tables
// ---------------------------------------------------------------------------
TEST_F(DatabaseTest, NewDatabaseHasEmptyValidCatalog) {
    ASSERT_NO_THROW({
        qdb::Database db(path_);
        EXPECT_EQ(db.table_count(), 0u);
        });
    EXPECT_TRUE(fs::exists(path_));
}

// ---------------------------------------------------------------------------
// 2. Create two tables -> different first-page IDs
// ---------------------------------------------------------------------------
TEST_F(DatabaseTest, TablesHaveDistinctFirstPages) {
    qdb::Database db(path_);

    auto& a = db.create_table("users");
    auto& b = db.create_table("orders");

    EXPECT_EQ(db.table_count(), 2u);
    EXPECT_NE(first_page_id(a), first_page_id(b));
}

// ---------------------------------------------------------------------------
// 3. Insert different records into each table -> no cross-table leakage
// ---------------------------------------------------------------------------
TEST_F(DatabaseTest, RecordsDoNotLeakBetweenTables) {
    qdb::Database db(path_);

    auto& a = db.create_table("users");
    auto& b = db.create_table("orders");

    insert_record(a, "alice");
    insert_record(a, "bob");
    insert_record(b, "order-1");
    insert_record(b, "order-2");
    insert_record(b, "order-3");

    EXPECT_EQ(read_all(a), sorted({ "alice", "bob" }));
    EXPECT_EQ(read_all(b), sorted({ "order-1", "order-2", "order-3" }));
}

// ---------------------------------------------------------------------------
// 4. Find tables by name -> correct table returned
// ---------------------------------------------------------------------------
TEST_F(DatabaseTest, FindTableReturnsCorrectTable) {
    qdb::Database db(path_);

    auto& a = db.create_table("users");
    auto& b = db.create_table("orders");

    EXPECT_EQ(db.find_table("users"), &a);
    EXPECT_EQ(db.find_table("orders"), &b);
    EXPECT_NE(db.find_table("users"), db.find_table("orders"));
}

TEST_F(DatabaseTest, FindTableWorksThroughConstReference) {
    qdb::Database db(path_);
    auto& a = db.create_table("users");

    const qdb::Database& cdb = db;
    EXPECT_EQ(cdb.find_table("users"), &a);
}

// ---------------------------------------------------------------------------
// 5. Look up a nonexistent table -> nullptr
// ---------------------------------------------------------------------------
TEST_F(DatabaseTest, FindNonexistentTableReturnsNullptr) {
    qdb::Database db(path_);
    EXPECT_EQ(db.find_table("ghost"), nullptr);

    (void)db.create_table("users");
    EXPECT_EQ(db.find_table("ghost"), nullptr);

    const qdb::Database& cdb = db;
    EXPECT_EQ(cdb.find_table("ghost"), nullptr);
}

// ---------------------------------------------------------------------------
// 6. Duplicate table name -> rejected, catalog unchanged
// ---------------------------------------------------------------------------
TEST_F(DatabaseTest, DuplicateTableNameIsRejectedWithoutModifyingCatalog) {
    {
        qdb::Database db(path_);
        auto& original = db.create_table("users");
        insert_record(original, "alice");
        const auto original_page = first_page_id(original);

        EXPECT_ANY_THROW((void)db.create_table("users"));

        // In-memory state unchanged
        EXPECT_EQ(db.table_count(), 1u);
        ASSERT_EQ(db.find_table("users"), &original);
        EXPECT_EQ(first_page_id(*db.find_table("users")), original_page);
        EXPECT_EQ(read_all(*db.find_table("users")), sorted({ "alice" }));
    }

    // Persisted state unchanged
    qdb::Database reopened(path_);
    EXPECT_EQ(reopened.table_count(), 1u);
    ASSERT_NE(reopened.find_table("users"), nullptr);
    EXPECT_EQ(read_all(*reopened.find_table("users")), sorted({ "alice" }));
}

// ---------------------------------------------------------------------------
// 7. Close and reopen -> tables and records recovered from path alone
// ---------------------------------------------------------------------------
TEST_F(DatabaseTest, CloseAndReopenRecoversTablesAndRecords) {
    std::uint32_t users_page = 0, orders_page = 0;

    {
        auto db = open();
        auto& a = db->create_table("users");
        auto& b = db->create_table("orders");
        users_page = first_page_id(a);
        orders_page = first_page_id(b);

        insert_record(a, "alice");
        insert_record(a, "bob");
        insert_record(b, "order-1");
    } // db destroyed -> file closed

    auto db = open();
    ASSERT_EQ(db->table_count(), 2u);

    auto* a = db->find_table("users");
    auto* b = db->find_table("orders");
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);

    EXPECT_EQ(first_page_id(*a), users_page);
    EXPECT_EQ(first_page_id(*b), orders_page);
    EXPECT_EQ(read_all(*a), sorted({ "alice", "bob" }));
    EXPECT_EQ(read_all(*b), sorted({ "order-1" }));
}

// ---------------------------------------------------------------------------
// 8. Create another table after reopening -> existing data intact
// ---------------------------------------------------------------------------
TEST_F(DatabaseTest, CreateTableAfterReopenKeepsExistingData) {
    {
        auto db = open();
        auto& a = db->create_table("users");
        insert_record(a, "alice");
    }

    std::uint32_t users_page = 0, products_page = 0;
    {
        auto db = open();
        auto* users = db->find_table("users");
        ASSERT_NE(users, nullptr);
        users_page = first_page_id(*users);

        auto& products = db->create_table("products");
        products_page = first_page_id(products);
        insert_record(products, "widget");

        EXPECT_NE(users_page, products_page);
        EXPECT_EQ(db->table_count(), 2u);
        EXPECT_EQ(read_all(*db->find_table("users")), sorted({ "alice" }));
    }

    // Final reopen: everything still there
    auto db = open();
    ASSERT_EQ(db->table_count(), 2u);
    ASSERT_NE(db->find_table("users"), nullptr);
    ASSERT_NE(db->find_table("products"), nullptr);
    EXPECT_EQ(first_page_id(*db->find_table("users")), users_page);
    EXPECT_EQ(first_page_id(*db->find_table("products")), products_page);
    EXPECT_EQ(read_all(*db->find_table("users")), sorted({ "alice" }));
    EXPECT_EQ(read_all(*db->find_table("products")), sorted({ "widget" }));
}

// ---------------------------------------------------------------------------
// 9. Malformed database file -> error, not silently accepted
// ---------------------------------------------------------------------------
TEST_F(DatabaseTest, GarbageFileIsRejected) {
    {
        std::ofstream f(path_, std::ios::binary);
        const std::string junk(8192, '\xAB');
        f.write(junk.data(), static_cast<std::streamsize>(junk.size()));
    }
    EXPECT_ANY_THROW({ qdb::Database db(path_); });
}

TEST_F(DatabaseTest, TruncatedFileIsRejected) {
    {
        qdb::Database db(path_);
        (void)db.create_table("users");
    }
    // Chop the file down to a few bytes
    fs::resize_file(path_, 7);

    EXPECT_ANY_THROW({ qdb::Database db(path_); });
}

TEST_F(DatabaseTest, CorruptedCatalogHeaderIsRejected) {
    {
        qdb::Database db(path_);
        (void)db.create_table("users");
    }
    // Overwrite the first bytes (magic/header) with garbage
    {
        std::fstream f(path_, std::ios::in | std::ios::out | std::ios::binary);
        ASSERT_TRUE(f.is_open());
        const std::string junk(16, '\xFF');
        f.seekp(0);
        f.write(junk.data(), static_cast<std::streamsize>(junk.size()));
    }
    EXPECT_ANY_THROW({ qdb::Database db(path_); });
}