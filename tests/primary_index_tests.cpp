#include <gtest/gtest.h>

#include <index/primary_index.hpp>
#include <storage/pager.hpp>
#include <storage/RecordID.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace qdb::test {
namespace {

class PrimaryIndexTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto stamp = std::chrono::steady_clock::now()
                               .time_since_epoch()
                               .count();
        database_path_ = std::filesystem::temp_directory_path() /
            ("quartzdb_primary_index_test_" + std::to_string(stamp) + ".db");

        // Ensure a stale file from an interrupted test cannot affect this run.
        std::error_code ec;
        std::filesystem::remove(database_path_, ec);
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove(database_path_, ec);
    }

    [[nodiscard]] PageID allocate_record_page(pager& storage) {
        PageID page_id{};
        if (!storage.allocate_page(page_id)) {
            throw std::runtime_error("Test setup failed to allocate record page");
        }
        return page_id;
    }

    std::filesystem::path database_path_;
};

TEST_F(PrimaryIndexTest, CreateAndInsertThenFind) {
    pager storage(database_path_);

    auto create_result = PrimaryIndex::create(storage);
    ASSERT_TRUE(create_result.has_value())
        << create_result.error().message();
    auto index = std::move(*create_result);
    ASSERT_NE(index, nullptr);

    const PageID record_page = allocate_record_page(storage);
    const RecordID expected_id{record_page, 7};

    auto insert_result = index->insert(42, expected_id);
    ASSERT_TRUE(insert_result.has_value())
        << insert_result.error().message();

    auto find_result = index->find(42);
    ASSERT_TRUE(find_result.has_value())
        << find_result.error().message();
    ASSERT_TRUE(find_result->has_value());
    EXPECT_EQ(**find_result, expected_id);
}

TEST_F(PrimaryIndexTest, FindMissingKeyReturnsEmptyOptional) {
    pager storage(database_path_);

    auto create_result = PrimaryIndex::create(storage);
    ASSERT_TRUE(create_result.has_value())
        << create_result.error().message();
    auto index = std::move(*create_result);

    auto find_result = index->find(999);
    ASSERT_TRUE(find_result.has_value())
        << find_result.error().message();
    EXPECT_FALSE(find_result->has_value());
}

TEST_F(PrimaryIndexTest, DuplicateKeyReturnsDuplicatePrimaryKeyStatus) {
    pager storage(database_path_);

    auto create_result = PrimaryIndex::create(storage);
    ASSERT_TRUE(create_result.has_value())
        << create_result.error().message();
    auto index = std::move(*create_result);

    const PageID record_page = allocate_record_page(storage);
    const RecordID first_id{record_page, 1};
    const RecordID second_id{record_page, 2};

    auto first_insert = index->insert(100, first_id);
    ASSERT_TRUE(first_insert.has_value())
        << first_insert.error().message();

    auto duplicate_insert = index->insert(100, second_id);
    ASSERT_FALSE(duplicate_insert.has_value());
    EXPECT_EQ(
        duplicate_insert.error().code(),
        StatusCode::DuplicatePrimaryKey
    );

    // The original mapping must remain unchanged after duplicate rejection.
    auto find_result = index->find(100);
    ASSERT_TRUE(find_result.has_value())
        << find_result.error().message();
    ASSERT_TRUE(find_result->has_value());
    EXPECT_EQ(**find_result, first_id);
}

TEST_F(PrimaryIndexTest, SupportsNegativeZeroAndIntegerBoundaries) {
    pager storage(database_path_);

    auto create_result = PrimaryIndex::create(storage);
    ASSERT_TRUE(create_result.has_value())
        << create_result.error().message();
    auto index = std::move(*create_result);

    const PageID record_page = allocate_record_page(storage);
    const std::vector<std::int64_t> keys{
        std::numeric_limits<std::int64_t>::min(),
        -1,
        0,
        1,
        std::numeric_limits<std::int64_t>::max()
    };

    for (std::size_t i = 0; i < keys.size(); ++i) {
        const RecordID id{
            record_page,
            static_cast<std::uint16_t>(i)
        };
        auto result = index->insert(keys[i], id);
        ASSERT_TRUE(result.has_value())
            << "Failed to insert key " << keys[i]
            << ": " << result.error().message();
    }

    for (std::size_t i = 0; i < keys.size(); ++i) {
        auto result = index->find(keys[i]);
        ASSERT_TRUE(result.has_value())
            << result.error().message();
        ASSERT_TRUE(result->has_value());
        EXPECT_EQ(
            **result,
            (RecordID{record_page, static_cast<std::uint16_t>(i)})
        );
    }
}

TEST_F(PrimaryIndexTest, ReopenPreservesEntries) {
    PageID metadata_page{};
    PageID record_page{};

    {
        pager storage(database_path_);

        auto create_result = PrimaryIndex::create(storage);
        ASSERT_TRUE(create_result.has_value())
            << create_result.error().message();
        auto index = std::move(*create_result);

        metadata_page = index->metadata_page();
        record_page = allocate_record_page(storage);

        for (std::int64_t key = -20; key <= 20; ++key) {
            const auto slot = static_cast<std::uint16_t>(key + 20);
            auto result = index->insert(key, RecordID{record_page, slot});
            ASSERT_TRUE(result.has_value())
                << result.error().message();
        }
    } // Destroy the index and close the Pager before reopening.

    {
        pager storage(database_path_);

        auto open_result = PrimaryIndex::open(storage, metadata_page);
        ASSERT_TRUE(open_result.has_value())
            << open_result.error().message();
        auto index = std::move(*open_result);

        for (std::int64_t key = -20; key <= 20; ++key) {
            auto result = index->find(key);
            ASSERT_TRUE(result.has_value())
                << result.error().message();
            ASSERT_TRUE(result->has_value()) << "Missing key: " << key;
            EXPECT_EQ(
                **result,
                (RecordID{
                    record_page,
                    static_cast<std::uint16_t>(key + 20)
                })
            );
        }

        auto missing_result = index->find(21);
        ASSERT_TRUE(missing_result.has_value())
            << missing_result.error().message();
        EXPECT_FALSE(missing_result->has_value());
    }
}

TEST_F(PrimaryIndexTest, InsertAfterReopeningIndex) {
    PageID metadata_page{};
    PageID record_page{};

    {
        pager storage(database_path_);
        auto create_result = PrimaryIndex::create(storage);
        ASSERT_TRUE(create_result.has_value())
            << create_result.error().message();
        auto index = std::move(*create_result);

        metadata_page = index->metadata_page();
        record_page = allocate_record_page(storage);

        auto result = index->insert(10, RecordID{record_page, 10});
        ASSERT_TRUE(result.has_value()) << result.error().message();
    }

    {
        pager storage(database_path_);
        auto open_result = PrimaryIndex::open(storage, metadata_page);
        ASSERT_TRUE(open_result.has_value())
            << open_result.error().message();
        auto index = std::move(*open_result);

        auto insert_result = index->insert(20, RecordID{record_page, 20});
        ASSERT_TRUE(insert_result.has_value())
            << insert_result.error().message();

        auto first = index->find(10);
        ASSERT_TRUE(first.has_value()) << first.error().message();
        ASSERT_TRUE(first->has_value());
        EXPECT_EQ(**first, (RecordID{record_page, 10}));

        auto second = index->find(20);
        ASSERT_TRUE(second.has_value()) << second.error().message();
        ASSERT_TRUE(second->has_value());
        EXPECT_EQ(**second, (RecordID{record_page, 20}));
    }
}

TEST_F(PrimaryIndexTest, LeafSplitsPreserveAllMappings) {
    pager storage(database_path_);

    auto create_result = PrimaryIndex::create(storage);
    ASSERT_TRUE(create_result.has_value())
        << create_result.error().message();
    auto index = std::move(*create_result);

    const PageID record_page = allocate_record_page(storage);
    constexpr std::int64_t KEY_COUNT = 300; // Exceeds the B+ tree's 128-key leaf capacity.

    // Insert in ascending order to exercise repeated rightmost-leaf splits.
    for (std::int64_t key = 0; key < KEY_COUNT; ++key) {
        const RecordID id{
            record_page,
            static_cast<std::uint16_t>(key)
        };
        auto result = index->insert(key, id);
        ASSERT_TRUE(result.has_value())
            << "Failed to insert key " << key
            << ": " << result.error().message();
    }

    for (std::int64_t key = 0; key < KEY_COUNT; ++key) {
        auto result = index->find(key);
        ASSERT_TRUE(result.has_value())
            << result.error().message();
        ASSERT_TRUE(result->has_value()) << "Missing key: " << key;
        EXPECT_EQ(
            **result,
            (RecordID{
                record_page,
                static_cast<std::uint16_t>(key)
            })
        );
    }
}

TEST_F(PrimaryIndexTest, ReopenAfterLeafSplitsPreservesAllMappings) {
    PageID metadata_page{};
    PageID record_page{};
    constexpr std::int64_t KEY_COUNT = 300;

    {
        pager storage(database_path_);
        auto create_result = PrimaryIndex::create(storage);
        ASSERT_TRUE(create_result.has_value())
            << create_result.error().message();
        auto index = std::move(*create_result);

        metadata_page = index->metadata_page();
        record_page = allocate_record_page(storage);

        for (std::int64_t key = 0; key < KEY_COUNT; ++key) {
            auto result = index->insert(
                key,
                RecordID{record_page, static_cast<std::uint16_t>(key)}
            );
            ASSERT_TRUE(result.has_value())
                << "Failed to insert key " << key
                << ": " << result.error().message();
        }
    }

    {
        pager storage(database_path_);
        auto open_result = PrimaryIndex::open(storage, metadata_page);
        ASSERT_TRUE(open_result.has_value())
            << open_result.error().message();
        auto index = std::move(*open_result);

        for (std::int64_t key = 0; key < KEY_COUNT; ++key) {
            auto result = index->find(key);
            ASSERT_TRUE(result.has_value())
                << result.error().message();
            ASSERT_TRUE(result->has_value()) << "Missing key: " << key;
            EXPECT_EQ(
                **result,
                (RecordID{
                    record_page,
                    static_cast<std::uint16_t>(key)
                })
            );
        }
    }
}

} // namespace
} // namespace qdb::test
