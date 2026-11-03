#include <gtest/gtest.h>

#include <quartz/database.hpp>
#include <quartz/status.hpp>
#include <storage/record.hpp>
#include <storage/schema.hpp>
#include <storage/table.hpp>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <string>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

namespace fs = std::filesystem;

namespace {

qdb::Schema make_users_schema() {
    return qdb::Schema{
        .columns = {
            qdb::Column{"id", qdb::DataType::Integer, false, true},
            qdb::Column{"name", qdb::DataType::Text, false, false},
            qdb::Column{"nickname", qdb::DataType::Text, true, false}
        }
    };
}

qdb::Record make_user(
    std::int64_t id,
    std::string name,
    qdb::Value nickname = std::monostate{}
) {
    return qdb::Record{
        .values = {
            qdb::Value{id},
            qdb::Value{std::move(name)},
            std::move(nickname)
        }
    };
}

class SchemaTest : public ::testing::Test {
protected:
    void SetUp() override {
        static std::atomic<unsigned> counter{0};
        const auto* info =
            ::testing::UnitTest::GetInstance()->current_test_info();

        path_ = fs::temp_directory_path() /
            ("quartzdb_schema_" + std::string(info->name()) + "_" +
             std::to_string(counter++) + ".db");

        std::error_code ec;
        fs::remove(path_, ec);
        ASSERT_FALSE(ec) << "Could not remove old test database: "
                         << ec.message();
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove(path_, ec);
    }

    fs::path path_;
};

} // namespace

TEST_F(SchemaTest, AcceptsRecordMatchingSchema) {
    qdb::Database db(path_);
    qdb::Table& users = db.create_table("users", make_users_schema());

    auto result1 = users.insert(make_user(1, "Uday", std::string{"ud"}));
    ASSERT_TRUE(result1.has_value()) << result1.error().message();

    auto result2 = users.insert(make_user(2, "Alice"));
    ASSERT_TRUE(result2.has_value()) << result2.error().message();

    const auto records = users.scan();
    ASSERT_EQ(records.size(), 2u);
    EXPECT_EQ(records[0].values[0], qdb::Value{std::int64_t{1}});
    EXPECT_EQ(records[0].values[1], qdb::Value{std::string{"Uday"}});
    EXPECT_EQ(records[0].values[2], qdb::Value{std::string{"ud"}});
    EXPECT_EQ(records[1].values[2], qdb::Value{std::monostate{}});
}

TEST_F(SchemaTest, RejectsWrongFieldCountWithoutWritingRecord) {
    qdb::Database db(path_);
    qdb::Table& users = db.create_table("users", make_users_schema());

    auto first = users.insert(make_user(1, "Uday"));
    ASSERT_TRUE(first.has_value()) << first.error().message();

    const qdb::Record invalid{
        .values = {
            qdb::Value{std::int64_t{2}},
            qdb::Value{std::string{"Alice"}}
        }
    };

    auto result = users.insert(invalid);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), qdb::StatusCode::InvalidRecord);
    EXPECT_EQ(users.scan().size(), 1u);
}

TEST_F(SchemaTest, RejectsTypeMismatchWithoutWritingRecord) {
    qdb::Database db(path_);
    qdb::Table& users = db.create_table("users", make_users_schema());

    auto first = users.insert(make_user(1, "Uday"));
    ASSERT_TRUE(first.has_value()) << first.error().message();

    const qdb::Record invalid{
        .values = {
            qdb::Value{std::string{"not-an-integer"}},
            qdb::Value{std::string{"Alice"}},
            qdb::Value{std::monostate{}}
        }
    };

    auto result = users.insert(invalid);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), qdb::StatusCode::TypeMismatch);
    EXPECT_EQ(users.scan().size(), 1u);
}

TEST_F(SchemaTest, RejectsNullInNonNullableColumn) {
    qdb::Database db(path_);
    qdb::Table& users = db.create_table("users", make_users_schema());

    const qdb::Record invalid{
        .values = {
            qdb::Value{std::monostate{}},
            qdb::Value{std::string{"Alice"}},
            qdb::Value{std::monostate{}}
        }
    };

    auto result = users.insert(invalid);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), qdb::StatusCode::NullConstraintViolation);
    EXPECT_TRUE(users.scan().empty());
}

TEST_F(SchemaTest, AcceptsNullInNullableColumn) {
    qdb::Database db(path_);
    qdb::Table& users = db.create_table("users", make_users_schema());

    auto result = users.insert(make_user(1, "Uday", std::monostate{}));
    ASSERT_TRUE(result.has_value()) << result.error().message();

    const auto records = users.scan();
    ASSERT_EQ(records.size(), 1u);
    EXPECT_EQ(records[0].values[2], qdb::Value{std::monostate{}});
}

TEST_F(SchemaTest, RejectsEmptySchema) {
    qdb::Database db(path_);
    const qdb::Schema invalid{};

    EXPECT_THROW({
        [[maybe_unused]] auto& table = db.create_table("empty", invalid);
    }, std::invalid_argument);
    EXPECT_EQ(db.table_count(), 0u);
}

TEST_F(SchemaTest, RejectsDuplicateColumnNames) {
    qdb::Database db(path_);
    const qdb::Schema invalid{
        .columns = {
            qdb::Column{"id", qdb::DataType::Integer, false, true},
            qdb::Column{"id", qdb::DataType::Text, false, false}
        }
    };

    EXPECT_THROW({
        [[maybe_unused]] auto& table = db.create_table("bad_columns", invalid);
    }, std::invalid_argument);
    EXPECT_EQ(db.table_count(), 0u);
}

TEST_F(SchemaTest, RejectsNullablePrimaryKey) {
    qdb::Database db(path_);
    const qdb::Schema invalid{
        .columns = {
            qdb::Column{"id", qdb::DataType::Integer, true, true}
        }
    };

    EXPECT_THROW({
        [[maybe_unused]] auto& table = db.create_table("bad_primary_key", invalid);
    }, std::invalid_argument);
    EXPECT_EQ(db.table_count(), 0u);
}

TEST_F(SchemaTest, SchemaAndValidationSurviveDatabaseReopen) {
    {
        qdb::Database db(path_);
        qdb::Table& users = db.create_table("users", make_users_schema());
        auto result = users.insert(make_user(1, "Uday"));
        ASSERT_TRUE(result.has_value()) << result.error().message();
    }

    {
        qdb::Database db(path_);
        qdb::Table* users = db.find_table("users");
        ASSERT_NE(users, nullptr);

        const auto records = users->scan();
        ASSERT_EQ(records.size(), 1u);
        EXPECT_EQ(records[0].values[1], qdb::Value{std::string{"Uday"}});

        const qdb::Record invalid{
            .values = {
                qdb::Value{std::string{"wrong-type"}},
                qdb::Value{std::string{"Alice"}},
                qdb::Value{std::monostate{}}
            }
        };

        auto result = users->insert(invalid);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error().code(), qdb::StatusCode::TypeMismatch);
        EXPECT_EQ(users->scan().size(), 1u);
    }
}
