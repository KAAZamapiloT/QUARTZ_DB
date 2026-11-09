#pragma once

#include <storage/pager.hpp>
#include <storage/record.hpp>
#include <storage/schema.hpp>
#include <storage/RecordID.hpp>
#include <quartz/status.hpp>
#include <index/primary_index.hpp>
#include <vector>
#include <optional>
#include <memory>

namespace qdb {

    inline Schema default_schema() {
        return Schema{
            .columns = {
                Column{
                    .name = "val",
                    .type = DataType::Text,
                    .nullable = false,
                    .primary_key = false
                }
            }
        };
    }

    class Table {
    public:
        static Table create(pager& storage, Schema schema = default_schema());

        Table(
            pager& storage,
            PageID first_page,
            Schema schema = default_schema(),
            std::optional<PageID> primary_index_metadata_page = std::nullopt
        );

        [[nodiscard]]
        Result<RecordID> insert(const Record& record);

        [[nodiscard]]
        Result<std::optional<Record>> find_by_primary_key(std::int64_t key);

        [[nodiscard]]
        std::vector<Record> scan();

        [[nodiscard]]
        PageID first_page() const noexcept {
            return first_page_;
        }

        [[nodiscard]]
        Schema get_schema() const noexcept {
            return schema_;
        }

        [[nodiscard]]
        bool has_primary_index() const noexcept {
            return primary_index_ != nullptr;
        }

        [[nodiscard]]
        std::optional<PageID> primary_index_metadata_page() const noexcept {
            if (!primary_index_) {
                return std::nullopt;
            }
            return primary_index_->metadata_page();
        }

        // update

        Result<RecordID> update_by_primary_key(std::int64_t key, const Record&updated_record);

		Result<RecordID> update_record(RecordID id, const Record& updated_record);
        // deletion
         
        Result<bool> delete_by_primary_key(std::int64_t key);

        
        Result<bool> delete_record(RecordID ID);

    private:
        struct AdoptIndexTag {};

        Table(
            pager& storage,
            PageID first_page,
            Schema schema,
            std::unique_ptr<PrimaryIndex>&& primary_index,
            std::optional<std::size_t> primary_key_column,
            AdoptIndexTag
        ) noexcept;

        pager& pager_;
        PageID first_page_;
        PageID last_page_{first_page_};
        Schema schema_;

        std::unique_ptr<PrimaryIndex> primary_index_;

        std::optional<std::size_t> primary_key_column_;

        [[nodiscard]]
        Result<RecordID> insert_record_data(const Record& record);

        [[nodiscard]]
        Result<Record> fetch_record_data(RecordID id);
    };

} // namespace qdb
