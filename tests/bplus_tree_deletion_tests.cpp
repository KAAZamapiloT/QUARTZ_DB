#include <gtest/gtest.h>

#include <index/bplus_tree.hpp>
#include <index/primary_index.hpp>
#include <quartz/database.hpp>
#include <quartz/status.hpp>
#include <storage/page.hpp>
#include <storage/pager.hpp>
#include <storage/RecordID.hpp>
#include <storage/record.hpp>
#include <storage/schema.hpp>
#include <storage/table.hpp>

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <numeric>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace qdb::testing {

namespace {

// On-disk magic constants from database.cpp, bplus_tree.cpp, and table.cpp
constexpr std::uint32_t kDbMagic   = 0x31424451u; // "QDB1"
constexpr std::uint32_t kMetaMagic = 0x314D5042u; // "BPM1"
constexpr std::uint32_t kNodeMagic = 0x31545042u; // "BPT1"
constexpr std::uint32_t kTblMagic  = 0x31475051u; // "QPG1"

constexpr std::uint8_t kLeafNode     = 1;
constexpr std::uint8_t kInternalNode = 2;
constexpr std::size_t  kMaxKeys      = 128;
constexpr std::size_t  kMinKeys      = kMaxKeys / 2;
constexpr PageID       kNoPage       = std::numeric_limits<PageID>::max();

std::uint16_t raw_u16(const Page& page, std::size_t offset) {
    std::uint16_t value = 0;
    for (unsigned i = 0; i < 2; ++i) {
        value |= static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(
                std::to_integer<unsigned char>(page.data[offset + i])
            ) << (i * 8)
        );
    }
    return value;
}

std::uint32_t raw_u32(const Page& page, std::size_t offset) {
    std::uint32_t value = 0;
    for (unsigned i = 0; i < 4; ++i) {
        value |= static_cast<std::uint32_t>(
            static_cast<std::uint32_t>(
                std::to_integer<unsigned char>(page.data[offset + i])
            ) << (i * 8)
        );
    }
    return value;
}

std::uint64_t raw_u64(const Page& page, std::size_t offset) {
    std::uint64_t value = 0;
    for (unsigned i = 0; i < 8; ++i) {
        value |= static_cast<std::uint64_t>(
            static_cast<std::uint64_t>(
                std::to_integer<unsigned char>(page.data[offset + i])
            ) << (i * 8)
        );
    }
    return value;
}

struct DiskInspectionReport {
    std::size_t total_pages{0};
    std::size_t catalog_pages{0};
    std::size_t bpt_meta_pages{0};
    std::size_t bpt_leaf_pages{0};
    std::size_t bpt_intern_pages{0};
    std::size_t table_pages{0};
    std::size_t zero_or_free_pages{0};
};

DiskInspectionReport inspect_raw_db_file(const std::filesystem::path& path) {
    DiskInspectionReport report{};
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in.is_open()) {
        return report;
    }

    const auto file_len = in.tellg();
    if (file_len <= 0 || file_len % static_cast<std::streamoff>(PAGE_SIZE) != 0) {
        return report;
    }

    report.total_pages = static_cast<std::size_t>(file_len / static_cast<std::streamoff>(PAGE_SIZE));
    in.seekg(0, std::ios::beg);

    Page page{};
    for (std::size_t pid = 0; pid < report.total_pages; ++pid) {
        in.read(reinterpret_cast<char*>(page.data.data()), PAGE_SIZE);
        if (!in) {
            break;
        }

        const auto magic = raw_u32(page, 0);
        if (magic == kDbMagic) {
            ++report.catalog_pages;
            EXPECT_EQ(pid, 0U);
        } else if (magic == kMetaMagic) {
            ++report.bpt_meta_pages;
            EXPECT_GT(pid, 0U);
            EXPECT_EQ(raw_u16(page, 4), 1U);
            const PageID root_pid = raw_u64(page, 8);
            EXPECT_GT(root_pid, 0U);
            EXPECT_LT(root_pid, static_cast<PageID>(report.total_pages));
            EXPECT_EQ(raw_u32(page, 16), PAGE_SIZE);
        } else if (magic == kNodeMagic) {
            EXPECT_GT(pid, 0U);
            EXPECT_EQ(raw_u16(page, 4), 1U);
            const auto node_type = std::to_integer<std::uint8_t>(page.data[6]);
            const std::uint16_t key_count = raw_u16(page, 8);
            const PageID next_leaf = raw_u64(page, 16);
            EXPECT_LE(key_count, kMaxKeys);

            if (node_type == kLeafNode) {
                ++report.bpt_leaf_pages;
                if (next_leaf != kNoPage) {
                    EXPECT_GT(next_leaf, 0U);
                    EXPECT_LT(next_leaf, static_cast<PageID>(report.total_pages));
                }
                // Verify raw binary keys on disk are strictly increasing
                std::optional<std::int64_t> prev_key;
                std::size_t off = 24U;
                for (std::uint16_t i = 0; i < key_count; ++i) {
                    const auto k = std::bit_cast<std::int64_t>(raw_u64(page, off));
                    if (prev_key.has_value()) {
                        EXPECT_LT(*prev_key, k) << "Unsorted leaf keys on disk page " << pid;
                    }
                    prev_key = k;
                    off += 18U;
                }
            } else if (node_type == kInternalNode) {
                ++report.bpt_intern_pages;
                EXPECT_EQ(next_leaf, kNoPage);
                std::optional<std::int64_t> prev_key;
                std::size_t off = 32U; // Skip header (24B) + first child (8B)
                for (std::uint16_t i = 0; i < key_count; ++i) {
                    const auto k = std::bit_cast<std::int64_t>(raw_u64(page, off));
                    if (prev_key.has_value()) {
                        EXPECT_LT(*prev_key, k) << "Unsorted internal keys on disk page " << pid;
                    }
                    prev_key = k;
                    off += 16U;
                }
            }
        } else if (magic == kTblMagic) {
            ++report.table_pages;
            EXPECT_GT(pid, 0U);
            const std::uint16_t slot_count = raw_u16(page, 4);
            const std::uint16_t free_start = raw_u16(page, 6);
            const std::uint16_t free_end   = raw_u16(page, 8);
            const PageID        next_page  = raw_u64(page, 12);
            const std::uint16_t frag_bytes = raw_u16(page, 20);

            EXPECT_GE(free_start, 24U);
            EXPECT_LE(free_start, free_end);
            EXPECT_LE(free_end, PAGE_SIZE);
            EXPECT_LE(frag_bytes, static_cast<std::uint16_t>(PAGE_SIZE - free_end));

            if (next_page != kNoPage) {
                EXPECT_GT(next_page, 0U);
                EXPECT_LT(next_page, static_cast<PageID>(report.total_pages));
            }

            for (std::uint16_t s = 0; s < slot_count; ++s) {
                const std::size_t slot_off = 24U + static_cast<std::size_t>(s) * 4U;
                const std::uint16_t rec_off = raw_u16(page, slot_off);
                const std::uint16_t rec_len = raw_u16(page, slot_off + 2U);
                if (rec_len > 0) {
                    EXPECT_GE(rec_off, free_end);
                    EXPECT_LE(static_cast<std::size_t>(rec_off) + rec_len, PAGE_SIZE);
                }
            }
        } else {
            ++report.zero_or_free_pages;
        }
    }

    return report;
}

class BplusTreeDeletionTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto* test_info =
            ::testing::UnitTest::GetInstance()->current_test_info();
        const std::string filename =
            std::string("qdb_bpt_") + test_info->test_suite_name() + "_" +
            test_info->name() + ".db";
        db_path_ = std::filesystem::temp_directory_path() / filename;
        std::error_code ec;
        std::filesystem::remove(db_path_, ec);
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove(db_path_, ec);
    }

    // Allocates dedicated record data pages so every RecordID.page_id is valid
    // (0 < page_id < pager.page_count()) regardless of tree splits or collapses.
    static std::vector<PageID> allocate_record_pages(pager& storage,
                                                     std::size_t count = 4) {
        std::vector<PageID> pages;
        pages.reserve(count);
        for (std::size_t i = 0; i < count; ++i) {
            PageID pid{};
            EXPECT_TRUE(storage.allocate_page(pid));
            pages.push_back(pid);
        }
        return pages;
    }

    static RecordID make_rid(const std::vector<PageID>& record_pages,
                             std::int64_t key,
                             std::uint64_t payload_seed = 0) {
        const auto ukey = static_cast<std::uint64_t>(key) ^ (payload_seed * 0x9E3779B97F4A7C15ULL);
        const PageID page_id = record_pages[ukey % record_pages.size()];
        const auto slot = static_cast<std::uint16_t>((ukey * 31ULL + 7ULL) % 500ULL);
        return RecordID{page_id, slot};
    }

    std::filesystem::path db_path_;
};

using BplusTreeHighFrequencyTest = BplusTreeDeletionTest;
using BplusDBTest                = BplusTreeDeletionTest;

// ============================================================================
// PART 1: Granular Unit & Deletion Invariant Tests (BplusTreeDeletionTest)
// ============================================================================

// 1. Empty Tree, Single Element, and Nonexistent Key Deletions
TEST_F(BplusTreeDeletionTest, EraseFromEmptyTreeAndSingleKeyLifecycle) {
    pager storage(db_path_);
    const auto rec_pages = allocate_record_pages(storage);
    const PageID meta = BplusTree::Create(storage);
    BplusTree tree(storage, meta);

    EXPECT_TRUE(tree.validate());
    EXPECT_FALSE(tree.erase(42));
    EXPECT_FALSE(tree.erase(0));
    EXPECT_FALSE(tree.erase(-999));
    EXPECT_TRUE(tree.validate());

    const RecordID rid = make_rid(rec_pages, 42);
    EXPECT_TRUE(tree.insert(42, rid));
    EXPECT_EQ(tree.find(42), rid);
    EXPECT_TRUE(tree.validate());

    // Delete nonexistent neighbor keys
    EXPECT_FALSE(tree.erase(41));
    EXPECT_FALSE(tree.erase(43));
    EXPECT_EQ(tree.find(42), rid);
    EXPECT_TRUE(tree.validate());

    // Delete existing key and verify idempotency
    EXPECT_TRUE(tree.erase(42));
    EXPECT_EQ(tree.find(42), std::nullopt);
    EXPECT_TRUE(tree.validate());

    EXPECT_FALSE(tree.erase(42));
    EXPECT_TRUE(tree.validate());

    // Re-insert after emptying root leaf
    const RecordID rid2{rec_pages[0], 99};
    EXPECT_TRUE(tree.insert(42, rid2));
    EXPECT_EQ(tree.find(42), rid2);
    EXPECT_TRUE(tree.validate());
}

// 2. Exhaustive Permutations (All N! Deletion Orders)
TEST_F(BplusTreeDeletionTest, AllDeletionPermutationsSmallTree) {
    pager storage(db_path_);
    const auto rec_pages = allocate_record_pages(storage);
    const PageID meta = BplusTree::Create(storage);
    BplusTree tree(storage, meta);

    std::vector<std::int64_t> keys = {-20, -5, 0, 7, 19, 100};
    std::vector<std::int64_t> perm = keys;

    do {
        for (const auto k : keys) {
            EXPECT_TRUE(tree.insert(k, make_rid(rec_pages, k)));
        }
        EXPECT_TRUE(tree.validate());

        for (std::size_t i = 0; i < perm.size(); ++i) {
            const auto target = perm[i];
            EXPECT_TRUE(tree.erase(target));
            EXPECT_EQ(tree.find(target), std::nullopt);
            EXPECT_FALSE(tree.erase(target));

            for (std::size_t j = i + 1; j < perm.size(); ++j) {
                EXPECT_EQ(tree.find(perm[j]), make_rid(rec_pages, perm[j]));
            }
            EXPECT_TRUE(tree.validate());
        }
    } while (std::next_permutation(perm.begin(), perm.end()));
}

// 3. Single-Leaf Boundary Orders (Forward, Reverse, Alternating, Random)
TEST_F(BplusTreeDeletionTest, SingleLeafMaxCapacityAllDeletionPatterns) {
    pager storage(db_path_);
    const auto rec_pages = allocate_record_pages(storage);
    const PageID meta = BplusTree::Create(storage);
    BplusTree tree(storage, meta);

    std::vector<std::int64_t> keys(kMaxKeys);
    std::iota(keys.begin(), keys.end(), 1);

    // Pattern A: Forward deletion
    for (const auto k : keys) {
        EXPECT_TRUE(tree.insert(k, make_rid(rec_pages, k)));
    }
    for (const auto k : keys) {
        EXPECT_TRUE(tree.erase(k));
        EXPECT_EQ(tree.find(k), std::nullopt);
        EXPECT_TRUE(tree.validate());
    }

    // Pattern B: Reverse deletion
    for (const auto k : keys) {
        EXPECT_TRUE(tree.insert(k, make_rid(rec_pages, k)));
    }
    for (auto it = keys.rbegin(); it != keys.rend(); ++it) {
        EXPECT_TRUE(tree.erase(*it));
        EXPECT_EQ(tree.find(*it), std::nullopt);
        EXPECT_TRUE(tree.validate());
    }

    // Pattern C: Outside-in alternating deletion
    for (const auto k : keys) {
        EXPECT_TRUE(tree.insert(k, make_rid(rec_pages, k)));
    }
    std::size_t left = 0;
    std::size_t right = keys.size() - 1;
    while (left <= right) {
        EXPECT_TRUE(tree.erase(keys[left]));
        EXPECT_TRUE(tree.validate());
        if (left < right) {
            EXPECT_TRUE(tree.erase(keys[right]));
            EXPECT_TRUE(tree.validate());
        }
        ++left;
        if (right > 0) {
            --right;
        } else {
            break;
        }
    }
}

// 4. Borrow From Right Sibling & Parent Separator Update
TEST_F(BplusTreeDeletionTest, BorrowFromRightSiblingAndUpdateSeparator) {
    pager storage(db_path_);
    const auto rec_pages = allocate_record_pages(storage);
    const PageID meta = BplusTree::Create(storage);
    BplusTree tree(storage, meta);

    constexpr std::int64_t kTotal = static_cast<std::int64_t>(kMaxKeys + 2);
    for (std::int64_t k = 1; k <= kTotal; ++k) {
        EXPECT_TRUE(tree.insert(k, make_rid(rec_pages, k)));
    }
    const PageID root_before = tree.RootPage();
    EXPECT_TRUE(tree.validate());

    // Delete key 1 from left leaf: left drops to 63 (< 64), borrows 65 from right leaf.
    EXPECT_TRUE(tree.erase(1));
    EXPECT_EQ(tree.find(1), std::nullopt);
    EXPECT_EQ(tree.RootPage(), root_before);
    EXPECT_TRUE(tree.validate());

    // Delete key 2 from left leaf: left drops to 63 again, borrows 66 from right leaf.
    EXPECT_TRUE(tree.erase(2));
    EXPECT_EQ(tree.find(2), std::nullopt);
    EXPECT_EQ(tree.RootPage(), root_before);
    EXPECT_TRUE(tree.validate());

    for (std::int64_t k = 3; k <= kTotal; ++k) {
        EXPECT_EQ(tree.find(k), make_rid(rec_pages, k));
    }
}

// 5. Borrow From Left Sibling & Parent Separator Update
TEST_F(BplusTreeDeletionTest, BorrowFromLeftSiblingAndUpdateSeparator) {
    pager storage(db_path_);
    const auto rec_pages = allocate_record_pages(storage);
    const PageID meta = BplusTree::Create(storage);
    BplusTree tree(storage, meta);

    for (std::int64_t k = 100; k <= 228; ++k) {
        EXPECT_TRUE(tree.insert(k, make_rid(rec_pages, k)));
    }
    for (std::int64_t k = 10; k <= 14; ++k) {
        EXPECT_TRUE(tree.insert(k, make_rid(rec_pages, k)));
    }
    const PageID root_before = tree.RootPage();
    EXPECT_TRUE(tree.validate());

    // Delete 164 (first key of right leaf): separator updates to 165
    EXPECT_TRUE(tree.erase(164));
    EXPECT_EQ(tree.find(164), std::nullopt);
    EXPECT_TRUE(tree.validate());

    // Delete 228 from right leaf: borrows 163 from left sibling
    EXPECT_TRUE(tree.erase(228));
    EXPECT_EQ(tree.find(228), std::nullopt);
    EXPECT_EQ(tree.RootPage(), root_before);
    EXPECT_TRUE(tree.validate());

    for (std::int64_t k = 10; k <= 14; ++k) {
        EXPECT_EQ(tree.find(k), make_rid(rec_pages, k));
    }
    for (std::int64_t k = 100; k <= 227; ++k) {
        if (k == 164) continue;
        EXPECT_EQ(tree.find(k), make_rid(rec_pages, k));
    }
}

// 6. Merge With Right Sibling & Root Collapse
TEST_F(BplusTreeDeletionTest, MergeWithRightSiblingAndCollapseRoot) {
    pager storage(db_path_);
    const auto rec_pages = allocate_record_pages(storage);
    const PageID meta = BplusTree::Create(storage);
    BplusTree tree(storage, meta);

    for (std::int64_t k = 1; k <= 129; ++k) {
        EXPECT_TRUE(tree.insert(k, make_rid(rec_pages, k)));
    }
    const PageID split_root = tree.RootPage();
    EXPECT_TRUE(tree.validate());

    EXPECT_TRUE(tree.erase(129));
    EXPECT_EQ(tree.RootPage(), split_root);
    EXPECT_TRUE(tree.validate());

    // Delete from left leaf: merges with right leaf and collapses root
    EXPECT_TRUE(tree.erase(1));
    EXPECT_NE(tree.RootPage(), split_root);
    EXPECT_TRUE(tree.validate());

    for (std::int64_t k = 2; k <= 128; ++k) {
        EXPECT_EQ(tree.find(k), make_rid(rec_pages, k));
    }
}

// 7. Merge With Left Sibling & Root Collapse
TEST_F(BplusTreeDeletionTest, MergeWithLeftSiblingAndCollapseRoot) {
    pager storage(db_path_);
    const auto rec_pages = allocate_record_pages(storage);
    const PageID meta = BplusTree::Create(storage);
    BplusTree tree(storage, meta);

    for (std::int64_t k = 1; k <= 129; ++k) {
        EXPECT_TRUE(tree.insert(k, make_rid(rec_pages, k)));
    }
    const PageID split_root = tree.RootPage();
    EXPECT_TRUE(tree.validate());

    EXPECT_TRUE(tree.erase(65));
    EXPECT_EQ(tree.RootPage(), split_root);
    EXPECT_TRUE(tree.validate());

    EXPECT_TRUE(tree.erase(66));
    EXPECT_NE(tree.RootPage(), split_root);
    EXPECT_TRUE(tree.validate());

    for (std::int64_t k = 1; k <= 129; ++k) {
        if (k == 65 || k == 66) {
            EXPECT_EQ(tree.find(k), std::nullopt);
        } else {
            EXPECT_EQ(tree.find(k), make_rid(rec_pages, k));
        }
    }
}

// 8. Sibling Borrows and Merges with Root Collapse
TEST_F(BplusTreeDeletionTest, SiblingBorrowsAndMergesWithRootCollapse) {
    pager storage(db_path_);
    const auto rec_pages = allocate_record_pages(storage);
    const PageID meta = BplusTree::Create(storage);
    BplusTree tree(storage, meta);

    // Insert 130 keys [1..130]: splits into [1..64] (64 keys) and [65..130] (66 keys)
    for (std::int64_t k = 1; k <= 130; ++k) {
        ASSERT_TRUE(tree.insert(k, make_rid(rec_pages, k)));
    }
    const PageID split_root = tree.RootPage();
    ASSERT_TRUE(tree.validate());

    // 1. Delete from left leaf (1, 2): borrows 65 and 66 from right leaf
    ASSERT_TRUE(tree.erase(1));
    ASSERT_TRUE(tree.erase(2));
    EXPECT_EQ(tree.RootPage(), split_root);
    ASSERT_TRUE(tree.validate());

    // 2. Delete 3 from left leaf: merges with right leaf and collapses the root!
    ASSERT_TRUE(tree.erase(3));
    EXPECT_NE(tree.RootPage(), split_root);
    ASSERT_TRUE(tree.validate());
}

// 9. Multi-Level (3-Level) Tree: Internal Borrow, Internal Merge & Leftmost Min Propagation
TEST_F(BplusTreeDeletionTest, ThreeLevelTreeInternalBorrowMergeAndMinPropagation) {
    pager storage(db_path_);
    const auto rec_pages = allocate_record_pages(storage);
    const PageID meta = BplusTree::Create(storage);
    BplusTree tree(storage, meta);

    constexpr std::int64_t kNumKeys = 9000;
    for (std::int64_t k = 1; k <= kNumKeys; ++k) {
        ASSERT_TRUE(tree.insert(k, make_rid(rec_pages, k)));
    }
    ASSERT_TRUE(tree.validate());

    // Phase 1: Delete keys around the middle [4000..4300]
    for (std::int64_t k = 4000; k <= 4300; ++k) {
        ASSERT_TRUE(tree.erase(k));
        ASSERT_EQ(tree.find(k), std::nullopt);
        if (k % 25 == 0) {
            ASSERT_TRUE(tree.validate());
        }
    }
    ASSERT_TRUE(tree.validate());

    // Phase 2: Delete from the high end [4301..9000]
    for (std::int64_t k = kNumKeys; k >= 4301; --k) {
        ASSERT_TRUE(tree.erase(k));
        if (k % 250 == 0) {
            ASSERT_TRUE(tree.validate());
        }
    }
    ASSERT_TRUE(tree.validate());

    // Phase 3: Delete remaining keys [1..3999] in randomized order
    std::vector<std::int64_t> remaining(3999);
    std::iota(remaining.begin(), remaining.end(), 1);
    std::mt19937_64 rng(0xC0FFEE123456ULL);
    std::shuffle(remaining.begin(), remaining.end(), rng);

    for (std::size_t i = 0; i < remaining.size(); ++i) {
        ASSERT_TRUE(tree.erase(remaining[i]));
        if (i % 200 == 0 || i + 1 == remaining.size()) {
            ASSERT_TRUE(tree.validate());
        }
    }

    for (std::int64_t k = 1; k <= 100; ++k) {
        EXPECT_EQ(tree.find(k), std::nullopt);
    }
}

// 10. Page Deallocation & Recycling Verification
TEST_F(BplusTreeDeletionTest, MergedAndCollapsedPagesAreDeallocatedAndReused) {
    pager storage(db_path_);
    const auto rec_pages = allocate_record_pages(storage);
    const PageID meta = BplusTree::Create(storage);
    BplusTree tree(storage, meta);

    constexpr std::int64_t kBatchSize = 2500;
    for (std::int64_t k = 1; k <= kBatchSize; ++k) {
        ASSERT_TRUE(tree.insert(k, make_rid(rec_pages, k)));
    }
    ASSERT_TRUE(tree.validate());

    const std::size_t peak_pages_first_pass = storage.page_count();
    EXPECT_GT(peak_pages_first_pass, 30U);

    for (std::int64_t k = 1; k <= kBatchSize; ++k) {
        ASSERT_TRUE(tree.erase(k));
    }
    ASSERT_TRUE(tree.validate());

    // Re-insert the same number of keys: pager must recycle freed pages without growing the file.
    for (std::int64_t k = 10001; k <= 10000 + kBatchSize; ++k) {
        ASSERT_TRUE(tree.insert(k, make_rid(rec_pages, k)));
    }
    ASSERT_TRUE(tree.validate());

    const std::size_t peak_pages_second_pass = storage.page_count();
    EXPECT_LE(peak_pages_second_pass, peak_pages_first_pass);
}

// 11. Persistence Across Pager Reopens (Root Collapse, Merges, Separators)
TEST_F(BplusTreeDeletionTest, PersistenceAcrossMultipleReopens) {
    PageID meta_page{};
    std::vector<PageID> rec_pages;

    // Session 1: Create tree, insert 600 keys, delete half
    {
        pager storage(db_path_);
        rec_pages = allocate_record_pages(storage);
        meta_page = BplusTree::Create(storage);
        BplusTree tree(storage, meta_page);

        for (std::int64_t k = 1; k <= 600; ++k) {
            ASSERT_TRUE(tree.insert(k, make_rid(rec_pages, k)));
        }
        for (std::int64_t k = 2; k <= 600; k += 2) {
            ASSERT_TRUE(tree.erase(k));
        }
        ASSERT_TRUE(tree.validate());
    }

    // Session 2: Reopen from disk, delete until root collapses
    PageID collapsed_root{};
    {
        pager storage(db_path_);
        BplusTree tree(storage, meta_page);
        ASSERT_TRUE(tree.validate());

        for (std::int64_t k = 1; k <= 600; ++k) {
            if (k % 2 == 0) {
                EXPECT_EQ(tree.find(k), std::nullopt);
            } else {
                EXPECT_EQ(tree.find(k), make_rid(rec_pages, k));
            }
        }

        for (std::int64_t k = 51; k <= 599; k += 2) {
            ASSERT_TRUE(tree.erase(k));
        }
        ASSERT_TRUE(tree.validate());
        collapsed_root = tree.RootPage();
    }

    // Session 3: Reopen again and verify the persisted collapsed root
    {
        pager storage(db_path_);
        BplusTree tree(storage, meta_page);
        ASSERT_TRUE(tree.validate());
        EXPECT_EQ(tree.RootPage(), collapsed_root);

        for (std::int64_t k = 1; k <= 49; k += 2) {
            EXPECT_EQ(tree.find(k), make_rid(rec_pages, k));
            EXPECT_TRUE(tree.erase(k));
        }
        EXPECT_TRUE(tree.validate());
    }
}

// 12. Extreme Signed 64-Bit Keys & Randomized Differential Stress Test
TEST_F(BplusTreeDeletionTest, Int64ExtremesAndRandomizedDifferentialStress) {
    pager storage(db_path_);
    const auto rec_pages = allocate_record_pages(storage);
    const PageID meta = BplusTree::Create(storage);
    BplusTree tree(storage, meta);

    std::map<std::int64_t, RecordID> oracle;

    const std::vector<std::int64_t> extremes = {
        std::numeric_limits<std::int64_t>::min(),
        std::numeric_limits<std::int64_t>::min() + 1,
        -1000000000000LL,
        -1,
        0,
        1,
        1000000000000LL,
        std::numeric_limits<std::int64_t>::max() - 1,
        std::numeric_limits<std::int64_t>::max()
    };

    for (const auto k : extremes) {
        const auto rid = make_rid(rec_pages, k);
        EXPECT_TRUE(tree.insert(k, rid));
        oracle[k] = rid;
    }
    EXPECT_TRUE(tree.validate());

    std::mt19937_64 rng(0xBADC0FFEE0DDF00DULL);
    std::uniform_int_distribution<std::int64_t> key_dist(-1500, 1500);
    std::uniform_int_distribution<int> op_dist(0, 99);

    for (int step = 0; step < 4000; ++step) {
        const std::int64_t k = key_dist(rng);
        const int op = op_dist(rng);

        if (op < 55) {
            const RecordID rid = make_rid(rec_pages, k + step);
            const bool expected = oracle.find(k) == oracle.end();
            EXPECT_EQ(tree.insert(k, rid), expected);
            if (expected) {
                oracle.emplace(k, rid);
            }
        } else if (op < 90) {
            const auto it = oracle.find(k);
            const bool expected = (it != oracle.end());
            EXPECT_EQ(tree.erase(k), expected);
            if (expected) {
                oracle.erase(it);
            }
        } else {
            const auto it = oracle.find(k);
            if (it == oracle.end()) {
                EXPECT_EQ(tree.find(k), std::nullopt);
            } else {
                EXPECT_EQ(tree.find(k), it->second);
            }
        }

        if (step % 200 == 0) {
            ASSERT_TRUE(tree.validate());
        }
    }

    ASSERT_TRUE(tree.validate());

    for (const auto& [k, rid] : oracle) {
        ASSERT_EQ(tree.find(k), rid);
    }
    for (const auto& [k, _] : oracle) {
        ASSERT_TRUE(tree.erase(k));
        ASSERT_EQ(tree.find(k), std::nullopt);
    }
    ASSERT_TRUE(tree.validate());
}

// ============================================================================
// PART 2: High-Frequency Tree Churn & Raw .db File Verification
// ============================================================================

TEST_F(BplusTreeHighFrequencyTest, SlidingWindowHighFrequencyChurnAndDiskBound) {
    constexpr std::int64_t kTotalInserts = 18000;
    constexpr std::int64_t kWindowSize   = 1500;

    PageID meta_page{};
    std::vector<PageID> rec_pages;
    std::size_t steady_state_page_bound = 0;

    {
        pager storage(db_path_);
        rec_pages = allocate_record_pages(storage, 8);
        meta_page = BplusTree::Create(storage);
        BplusTree tree(storage, meta_page);

        for (std::int64_t k = 1; k <= kTotalInserts; ++k) {
            ASSERT_TRUE(tree.insert(k, make_rid(rec_pages, k)));

            if (k > 10) {
                const std::int64_t probe = k - (k % 7);
                if (probe > k - kWindowSize) {
                    ASSERT_EQ(tree.find(probe), make_rid(rec_pages, probe));
                }
            }

            if (k > kWindowSize) {
                const std::int64_t evict_key = k - kWindowSize;
                ASSERT_TRUE(tree.erase(evict_key));
                ASSERT_EQ(tree.find(evict_key), std::nullopt);
            }

            if (k == kWindowSize * 2) {
                steady_state_page_bound = storage.page_count();
            }

            if (k % 3000 == 0) {
                ASSERT_TRUE(tree.validate());
            }
        }

        ASSERT_TRUE(tree.validate());
        EXPECT_LE(storage.page_count(), steady_state_page_bound + 6U);
    }

    const DiskInspectionReport report = inspect_raw_db_file(db_path_);
    EXPECT_EQ(report.bpt_meta_pages, 1U);
    EXPECT_GT(report.bpt_leaf_pages, 10U);

    {
        pager storage(db_path_);
        BplusTree tree(storage, meta_page);
        ASSERT_TRUE(tree.validate());

        for (std::int64_t k = 1; k <= kTotalInserts - kWindowSize; k += 97) {
            EXPECT_EQ(tree.find(k), std::nullopt);
        }
        for (std::int64_t k = kTotalInserts - kWindowSize + 1; k <= kTotalInserts; ++k) {
            ASSERT_EQ(tree.find(k), make_rid(rec_pages, k));
        }
    }
}

TEST_F(BplusTreeHighFrequencyTest, MultiWaveSawtoothExpansionCollapseAndFileReuse) {
    pager storage(db_path_);
    const auto rec_pages = allocate_record_pages(storage, 8);
    const PageID meta_page = BplusTree::Create(storage);
    BplusTree tree(storage, meta_page);

    std::map<std::int64_t, RecordID> oracle;
    std::mt19937_64 rng(0x5A170074BEEFULL);
    std::size_t first_wave_peak_pages = 0;

    for (std::uint64_t wave = 0; wave < 3; ++wave) {
        const std::int64_t base_key = static_cast<std::int64_t>(wave) * 50000LL - 25000LL;

        std::vector<std::int64_t> wave_keys(10000);
        std::iota(wave_keys.begin(), wave_keys.end(), base_key);
        std::shuffle(wave_keys.begin(), wave_keys.end(), rng);

        for (std::size_t i = 0; i < wave_keys.size(); ++i) {
            const std::int64_t k = wave_keys[i];
            const RecordID rid = make_rid(rec_pages, k, wave);
            ASSERT_TRUE(tree.insert(k, rid));
            oracle[k] = rid;

            if (i % 5 == 0) {
                const std::int64_t check_k = wave_keys[i / 2];
                ASSERT_EQ(tree.find(check_k), oracle[check_k]);
            }
        }

        ASSERT_TRUE(tree.validate());

        if (wave == 0) {
            first_wave_peak_pages = storage.page_count();
            EXPECT_GT(first_wave_peak_pages, 100U);
        } else {
            EXPECT_LE(storage.page_count(), first_wave_peak_pages + 5U);
        }

        std::shuffle(wave_keys.begin(), wave_keys.end(), rng);
        for (std::size_t i = 0; i < wave_keys.size() - 20U; ++i) {
            const std::int64_t k = wave_keys[i];
            ASSERT_TRUE(tree.erase(k));
            oracle.erase(k);

            if (i % 10 == 0) {
                ASSERT_EQ(tree.find(k), std::nullopt);
            }
        }

        ASSERT_TRUE(tree.validate());
    }

    EXPECT_EQ(oracle.size(), 60U);
    for (const auto& [k, rid] : oracle) {
        EXPECT_EQ(tree.find(k), rid);
    }

    const auto report = inspect_raw_db_file(db_path_);
    EXPECT_EQ(report.total_pages, storage.page_count());
}

TEST_F(BplusTreeHighFrequencyTest, SixtyThousandOpAdversarialReadWriteDeleteBlitz) {
    PageID meta_page{};
    std::vector<PageID> rec_pages;
    std::unordered_map<std::int64_t, RecordID> oracle;
    oracle.reserve(8192);

    std::mt19937_64 rng(0xDEC0DED123456789ULL);
    std::uniform_int_distribution<std::int64_t> hot_dist(-4000, 4000);
    std::uniform_int_distribution<int> op_dist(0, 99);

    {
        pager storage(db_path_);
        rec_pages = allocate_record_pages(storage, 10);
        meta_page = BplusTree::Create(storage);
        BplusTree tree(storage, meta_page);

        for (std::int64_t k = -3500; k < 3500; k += 2) {
            const RecordID rid = make_rid(rec_pages, k, 1);
            ASSERT_TRUE(tree.insert(k, rid));
            oracle.emplace(k, rid);
        }
        ASSERT_TRUE(tree.validate());

        for (std::uint64_t step = 0; step < 60000; ++step) {
            const std::int64_t k = hot_dist(rng);
            const int op = op_dist(rng);

            if (op < 40) {
                const auto it = oracle.find(k);
                if (it == oracle.end()) {
                    ASSERT_EQ(tree.find(k), std::nullopt);
                } else {
                    ASSERT_EQ(tree.find(k), it->second);
                }
            } else if (op < 72) {
                const auto it = oracle.find(k);
                const RecordID rid = make_rid(rec_pages, k, step);
                if (it == oracle.end()) {
                    ASSERT_TRUE(tree.insert(k, rid));
                    oracle.emplace(k, rid);
                } else {
                    ASSERT_FALSE(tree.insert(k, rid));
                    ASSERT_TRUE(tree.erase(k));
                    ASSERT_TRUE(tree.insert(k, rid));
                    it->second = rid;
                }
            } else {
                const auto it = oracle.find(k);
                if (it == oracle.end()) {
                    ASSERT_FALSE(tree.erase(k));
                } else {
                    ASSERT_TRUE(tree.erase(k));
                    oracle.erase(it);
                }
            }

            if (step % 15000 == 0 && step > 0) {
                ASSERT_TRUE(tree.validate());
            }
        }

        ASSERT_TRUE(tree.validate());
    }

    const auto report = inspect_raw_db_file(db_path_);
    EXPECT_EQ(report.bpt_meta_pages, 1U);

    {
        pager storage(db_path_);
        BplusTree tree(storage, meta_page);
        ASSERT_TRUE(tree.validate());

        for (std::int64_t k = -4000; k <= 4000; ++k) {
            const auto it = oracle.find(k);
            if (it == oracle.end()) {
                ASSERT_EQ(tree.find(k), std::nullopt) << "Ghost key on disk: " << k;
            } else {
                ASSERT_EQ(tree.find(k), it->second) << "Corrupted RecordID for key: " << k;
            }
        }
    }
}

// ============================================================================
// PART 3: Bplus_DB Full-Stack Integration (Database + Table + PrimaryIndex + BplusTree)
// ============================================================================

TEST_F(BplusDBTest, FullStackHighFrequencyReadWriteUpdateDeleteAndLazyCompaction) {
    Schema schema{
        .columns = {
            Column{.name = "id",      .type = DataType::Integer, .nullable = false, .primary_key = true},
            Column{.name = "version", .type = DataType::Integer, .nullable = false, .primary_key = false},
            Column{.name = "payload", .type = DataType::Text,    .nullable = false, .primary_key = false}
        }
    };

    auto make_payload = [](std::int64_t key, std::int64_t ver) -> std::string {
        const std::size_t len =
            20U + static_cast<std::size_t>((key * 37 + ver * 13) & 0x7F);
        std::string s = "row_" + std::to_string(key) + "_v" + std::to_string(ver) + "_";
        if (s.size() < len) {
            const auto ch = static_cast<char>('A' + (std::abs(key + ver) % 26));
            s.append(len - s.size(), ch);
        }
        return s;
    };

    struct ExpectedRow {
        std::int64_t version;
        std::string payload;
    };

    std::map<std::int64_t, ExpectedRow> row_oracle;
    PageID table_first_page{};
    PageID index_meta_page{};

    // Session 1: 1,200 initial inserts + 10,000 mixed OLTP operations
    {
        pager storage(db_path_);
        Table table = Table::create(storage, schema);
        ASSERT_TRUE(table.has_primary_index());
        ASSERT_TRUE(table.primary_index_metadata_page().has_value());

        table_first_page = table.first_page();
        index_meta_page  = *table.primary_index_metadata_page();

        for (std::int64_t k = 1; k <= 1200; ++k) {
            const std::string text = make_payload(k, 1);
            Record rec{{Value{k}, Value{std::int64_t{1}}, Value{text}}};
            auto ins_res = table.insert(rec);
            ASSERT_TRUE(ins_res.has_value()) << ins_res.error().message();
            row_oracle[k] = ExpectedRow{1, text};

            auto dup_res = table.insert(rec);
            ASSERT_FALSE(dup_res.has_value());
            EXPECT_EQ(dup_res.error().code(), StatusCode::DuplicatePrimaryKey);
        }

        std::mt19937_64 rng(0xDB2026B715ULL);
        std::uniform_int_distribution<std::int64_t> key_dist(1, 1600);
        std::uniform_int_distribution<int> op_dist(0, 99);

        for (int step = 0; step < 10000; ++step) {
            const std::int64_t k = key_dist(rng);
            const int op = op_dist(rng);
            const auto it = row_oracle.find(k);

            if (op < 35) {
                auto find_res = table.find_by_primary_key(k);
                ASSERT_TRUE(find_res.has_value()) << find_res.error().message();
                if (it == row_oracle.end()) {
                    ASSERT_FALSE(find_res->has_value());
                } else {
                    ASSERT_TRUE(find_res->has_value());
                    const Record& r = **find_res;
                    EXPECT_EQ(std::get<std::int64_t>(r.values[0]), k);
                    EXPECT_EQ(std::get<std::int64_t>(r.values[1]), it->second.version);
                    EXPECT_EQ(std::get<std::string>(r.values[2]), it->second.payload);
                }
            } else if (op < 60) {
                const std::int64_t next_ver =
                    (it == row_oracle.end()) ? 1 : (it->second.version + 1);
                const std::string new_text = make_payload(k, next_ver);
                Record updated{{Value{k}, Value{next_ver}, Value{new_text}}};

                auto upd_res = table.update_by_primary_key(k, updated);
                if (it == row_oracle.end()) {
                    ASSERT_FALSE(upd_res.has_value());
                    EXPECT_EQ(upd_res.error().code(), StatusCode::RecordNotFound);
                } else {
                    ASSERT_TRUE(upd_res.has_value()) << upd_res.error().message();
                    it->second = ExpectedRow{next_ver, new_text};
                }
            } else if (op < 82) {
                auto del_res = table.delete_by_primary_key(k);
                if (it == row_oracle.end()) {
                    ASSERT_FALSE(del_res.has_value());
                    EXPECT_EQ(del_res.error().code(), StatusCode::RecordNotFound);
                } else {
                    ASSERT_TRUE(del_res.has_value()) << del_res.error().message();
                    EXPECT_TRUE(*del_res);
                    row_oracle.erase(it);
                }
            } else {
                const std::string text = make_payload(k, 1);
                Record rec{{Value{k}, Value{std::int64_t{1}}, Value{text}}};
                auto ins_res = table.insert(rec);
                if (it == row_oracle.end()) {
                    ASSERT_TRUE(ins_res.has_value()) << ins_res.error().message();
                    row_oracle[k] = ExpectedRow{1, text};
                } else {
                    ASSERT_FALSE(ins_res.has_value());
                    EXPECT_EQ(ins_res.error().code(), StatusCode::DuplicatePrimaryKey);
                }
            }
        }

        BplusTree index_validator(storage, index_meta_page);
        ASSERT_TRUE(index_validator.validate());
    }

    const DiskInspectionReport mid_report = inspect_raw_db_file(db_path_);
    EXPECT_GT(mid_report.table_pages, 0U);
    EXPECT_GT(mid_report.bpt_leaf_pages, 0U);

    // Session 2: Crash/reopen recovery test
    {
        pager storage(db_path_);
        Table table(storage, table_first_page, schema, index_meta_page);
        ASSERT_TRUE(table.has_primary_index());

        BplusTree index_validator(storage, index_meta_page);
        ASSERT_TRUE(index_validator.validate());

        for (const auto& [k, expected] : row_oracle) {
            auto find_res = table.find_by_primary_key(k);
            ASSERT_TRUE(find_res.has_value()) << find_res.error().message();
            ASSERT_TRUE(find_res->has_value()) << "Missing row for key: " << k;
            const Record& r = **find_res;
            EXPECT_EQ(std::get<std::int64_t>(r.values[0]), k);
            EXPECT_EQ(std::get<std::int64_t>(r.values[1]), expected.version);
            EXPECT_EQ(std::get<std::string>(r.values[2]), expected.payload);
        }

        const auto full_scan = table.scan();
        EXPECT_EQ(full_scan.size(), row_oracle.size());

        for (const auto& [k, _] : row_oracle) {
            auto del_res = table.delete_by_primary_key(k);
            ASSERT_TRUE(del_res.has_value()) << del_res.error().message();
            EXPECT_TRUE(*del_res);
        }

        EXPECT_TRUE(table.scan().empty());
        ASSERT_TRUE(BplusTree(storage, index_meta_page).validate());
    }
}

TEST_F(BplusDBTest, DatabaseCatalogMultiSessionTableAndBplusTreeIntegrity) {
    Schema users_schema{
        .columns = {
            Column{.name = "uid",   .type = DataType::Integer, .nullable = false, .primary_key = true},
            Column{.name = "uname", .type = DataType::Text,    .nullable = false, .primary_key = false}
        }
    };

    PageID users_index_meta{};

    {
        Database db(db_path_);
        Table& users = db.create_table("users", users_schema);
        ASSERT_TRUE(users.has_primary_index());
        users_index_meta = *users.primary_index_metadata_page();

        for (std::int64_t id = 1; id <= 600; ++id) {
            Record rec{{Value{id}, Value{"user_" + std::to_string(id)}}};
            ASSERT_TRUE(users.insert(rec).has_value());
        }

        for (std::int64_t id = 1; id <= 420; ++id) {
            ASSERT_TRUE(users.delete_by_primary_key(id).has_value());
        }

        for (std::int64_t id = 421; id <= 600; ++id) {
            Record upd{{Value{id}, Value{"updated_user_" + std::to_string(id)}}};
            ASSERT_TRUE(users.update_by_primary_key(id, upd).has_value());
        }

        EXPECT_EQ(users.scan().size(), 180U);
    }

    const DiskInspectionReport report = inspect_raw_db_file(db_path_);
    EXPECT_EQ(report.catalog_pages, 1U);
    EXPECT_EQ(report.bpt_meta_pages, 1U);
    EXPECT_GE(report.bpt_leaf_pages, 1U);
    EXPECT_GE(report.table_pages, 1U);

    {
        Database db(db_path_);
        EXPECT_EQ(db.table_count(), 1U);
        Table* users = db.find_table("users");
        ASSERT_NE(users, nullptr);
        EXPECT_EQ(users->scan().size(), 180U);

        pager direct_pager(db_path_);
        BplusTree tree(direct_pager, users_index_meta);
        EXPECT_TRUE(tree.validate());

        for (std::int64_t id = 1; id <= 600; ++id) {
            if (id <= 420) {
                EXPECT_EQ(tree.find(id), std::nullopt);
            } else {
                EXPECT_TRUE(tree.find(id).has_value());
            }
        }
    }
}

} // namespace
} // namespace qdb::testing
