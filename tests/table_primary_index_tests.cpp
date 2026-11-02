#include <gtest/gtest.h>

#include <storage/table.hpp>
#include <storage/pager.hpp>
#include <storage/record.hpp>
#include <storage/schema.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <stdexcept>
#include <utility>
#include <system_error>

namespace qdb::test {
namespace {

class TablePrimaryIndexTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto stamp = std::chrono::steady_clock::now()
                               .time_since_epoch()
                               .count();
        database_path_ = std::filesystem::temp_directory_path() /
            ("quartzdb_table_primary_index_test_" +
             std::to_string(stamp) + ".db");
        std::error_code ec;
        std::filesystem::remove(database_path_, ec);
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove(database_path_, ec);
    }

    static Schema users_schema() {
        return Schema{
            .columns = {
                Column{
                    .name = "id",
                    .type = DataType::Integer,
                    .nullable = false,
                    .primary_key = true
                },
                Column{
                    .name = "name",
                    .type = DataType::Text,
                    .nullable = false,
                    .primary_key = false
                }
            }
        };
    }

    static Record user(std::int64_t id, std::string name) {
        return Record{
            .values = {Value{id}, Value{std::move(name)}}
        };
    }

    std::filesystem::path database_path_;
};

TEST_F(TablePrimaryIndexTest, CreateTableBuildsPrimaryIndex) {
    pager storage(database_path_);
    Table table = Table::create(storage, users_schema());

    EXPECT_TRUE(table.has_primary_index());
    EXPECT_TRUE(table.primary_index_metadata_page().has_value());
}

TEST_F(TablePrimaryIndexTest, InsertReturnsRecordIdAndIndexedLookupFindsRecord) {
    pager storage(database_path_);
    Table table = Table::create(storage, users_schema());

    auto insert_result = table.insert(user(42, "Ada"));
    ASSERT_TRUE(insert_result.has_value())
        << insert_result.error().message();

    auto find_result = table.find_by_primary_key(42);
    ASSERT_TRUE(find_result.has_value())
        << find_result.error().message();
    ASSERT_TRUE(find_result->has_value());

    const Record& found = **find_result;
    ASSERT_EQ(found.values.size(), 2u);
    EXPECT_EQ(std::get<std::int64_t>(found.values[0]), 42);
    EXPECT_EQ(std::get<std::string>(found.values[1]), "Ada");
}

TEST_F(TablePrimaryIndexTest, DuplicatePrimaryKeyDoesNotInsertSecondRecord) {
    pager storage(database_path_);
    Table table = Table::create(storage, users_schema());

    auto first = table.insert(user(7, "first"));
    ASSERT_TRUE(first.has_value());

    auto duplicate = table.insert(user(7, "second"));
    ASSERT_FALSE(duplicate.has_value());
    EXPECT_EQ(duplicate.error().code(), StatusCode::DuplicatePrimaryKey);

    const auto records = table.scan();
    ASSERT_EQ(records.size(), 1u);
    EXPECT_EQ(std::get<std::string>(records[0].values[1]), "first");
}

TEST_F(TablePrimaryIndexTest, MissingKeyReturnsEmptyOptional) {
    pager storage(database_path_);
    Table table = Table::create(storage, users_schema());

    auto result = table.find_by_primary_key(999);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_FALSE(result->has_value());
}

TEST_F(TablePrimaryIndexTest, TablesWithoutPrimaryKeyHaveNoIndex) {
    pager storage(database_path_);
    Table table = Table::create(storage);

    EXPECT_FALSE(table.has_primary_index());

    Record record{
        .values = {Value{std::string{"log entry"}}}
    };
    auto insert_result = table.insert(record);
    ASSERT_TRUE(insert_result.has_value())
        << insert_result.error().message();

    auto lookup_result = table.find_by_primary_key(1);
    ASSERT_FALSE(lookup_result.has_value());
    EXPECT_EQ(lookup_result.error().code(), StatusCode::NoPrimaryKey);
}

TEST_F(TablePrimaryIndexTest, ReopeningTableRestoresPrimaryIndex) {
    PageID first_page{};
    PageID index_metadata_page{};

    {
        pager storage(database_path_);
        Table table = Table::create(storage, users_schema());
        first_page = table.first_page();
        ASSERT_TRUE(table.primary_index_metadata_page().has_value());
        index_metadata_page = *table.primary_index_metadata_page();

        auto inserted = table.insert(user(-123, "Grace"));
        ASSERT_TRUE(inserted.has_value()) << inserted.error().message();
    }

    {
        pager storage(database_path_);
        Table table(
            storage,
            first_page,
            users_schema(),
            index_metadata_page
        );

        auto result = table.find_by_primary_key(-123);
        ASSERT_TRUE(result.has_value()) << result.error().message();
        ASSERT_TRUE(result->has_value());
        EXPECT_EQ(
            std::get<std::string>((**result).values[1]),
            "Grace"
        );
    }
}

TEST_F(TablePrimaryIndexTest, PrimaryKeyMustBeInteger) {
    pager storage(database_path_);

    Schema invalid_schema{
        .columns = {
            Column{
                .name = "name",
                .type = DataType::Text,
                .nullable = false,
                .primary_key = true
            }
        }
    };

    EXPECT_THROW(
        (void)Table::create(storage, std::move(invalid_schema)),
        std::invalid_argument
    );
}

} // namespace
} // namespace qdb::test
