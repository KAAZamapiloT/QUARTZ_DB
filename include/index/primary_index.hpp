#pragma once

#include <cstdint>
#include <memory>
#include <optional>

#include <index/bplus_tree.hpp>
#include <quartz/status.hpp>

namespace qdb {

    class PrimaryIndex {
    public:

        [[nodiscard]]
        static Result<std::unique_ptr<PrimaryIndex>>
            create(pager& storage);

        [[nodiscard]]
        static Result<std::unique_ptr<PrimaryIndex>>
            open(pager& storage, PageID metadata_page);

        [[nodiscard]]
        Result<void> insert(
            std::int64_t key,
            RecordID id
        );

        [[nodiscard]]
        Result<std::optional<RecordID>> find(
            std::int64_t key
        );

        [[nodiscard]]
        PageID metadata_page() const noexcept {
            return tree_.MetaDataPage();
        }

    private:

        PrimaryIndex(
            pager& storage,
            PageID metadata_page
        );

        BplusTree tree_;
    };

}