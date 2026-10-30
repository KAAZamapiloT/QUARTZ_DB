#include <index/bplus_tree.hpp>

#include <gtest/gtest.h>

#include <filesystem>
#include <cstdint>
#include <system_error>
#include <vector>

namespace {

class BPlusTreeTest : public ::testing::Test {
protected:
    void SetUp() override {
        path_ = std::filesystem::temp_directory_path() /
            "quartzdb_bplus_tree_test.db";
        std::error_code ec;
        std::filesystem::remove(path_, ec);
        ASSERT_FALSE(ec);
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove(path_, ec);
    }

    std::filesystem::path path_;
};

TEST_F(BPlusTreeTest, FindsKeysAndRejectsDuplicates) {
    qdb::pager storage(path_);
    qdb::PageID record_page{};
    ASSERT_TRUE(storage.allocate_page(record_page));
    const qdb::PageID metadata = qdb::BPlusTree::create(storage);
    qdb::BPlusTree tree(storage, metadata);

    EXPECT_TRUE(tree.insert(10, qdb::RecordId{record_page, 0}));
    EXPECT_TRUE(tree.insert(-5, qdb::RecordId{record_page, 1}));
    EXPECT_TRUE(tree.insert(70, qdb::RecordId{record_page, 2}));
    EXPECT_FALSE(tree.insert(10, qdb::RecordId{record_page, 3}));

    ASSERT_TRUE(tree.find(10).has_value());
    EXPECT_EQ(*tree.find(10), (qdb::RecordId{record_page, 0}));
    EXPECT_EQ(*tree.find(-5), (qdb::RecordId{record_page, 1}));
    EXPECT_FALSE(tree.find(999).has_value());
}

TEST_F(BPlusTreeTest, FindsAllKeysAfterLeafAndInternalSplits) {
    qdb::pager storage(path_);
    qdb::PageID record_page{};
    ASSERT_TRUE(storage.allocate_page(record_page));
    const qdb::PageID metadata = qdb::BPlusTree::create(storage);
    qdb::BPlusTree tree(storage, metadata);

    // More than 128 keys forces leaf splits; many leaves force internal splits.
    constexpr std::int64_t count = 20000;
    for (std::int64_t key = 0; key < count; ++key) {
        ASSERT_TRUE(tree.insert(key, qdb::RecordId{record_page, static_cast<std::uint16_t>(key % 65536)}));
    }

    for (std::int64_t key = 0; key < count; ++key) {
        const auto value = tree.find(key);
        ASSERT_TRUE(value.has_value()) << "Missing key " << key;
        EXPECT_EQ(value->page_id, record_page);
        EXPECT_EQ(value->slot_index,
                  static_cast<std::uint16_t>(key % 65536));
    }

    EXPECT_FALSE(tree.find(-1).has_value());
    EXPECT_FALSE(tree.find(count).has_value());
}

TEST_F(BPlusTreeTest, RootAndEntriesSurviveReopen) {
    qdb::PageID metadata{};
    qdb::PageID record_page{};
    {
        qdb::pager storage(path_);
        ASSERT_TRUE(storage.allocate_page(record_page));
        metadata = qdb::BPlusTree::create(storage);
        qdb::BPlusTree tree(storage, metadata);
        for (std::int64_t key = 0; key < 1000; ++key) {
            ASSERT_TRUE(tree.insert(key * 3, qdb::RecordId{record_page, static_cast<std::uint16_t>(key)}));
        }
    }

    {
        qdb::pager storage(path_);
        qdb::BPlusTree tree(storage, metadata);
        for (std::int64_t key = 0; key < 1000; ++key) {
            const auto value = tree.find(key * 3);
            ASSERT_TRUE(value.has_value());
            EXPECT_EQ(value->slot_index, static_cast<std::uint16_t>(key));
        }
        EXPECT_FALSE(tree.find(1).has_value());
    }
}

} // namespace
