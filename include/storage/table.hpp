#pragma once

#include <storage/pager.hpp>
#include <storage/record.hpp>
#include <storage/schema.hpp>
#include<storage/RecordID.hpp>
#include<quartz/status.hpp>
#include <vector>

class PrimaryIndex;

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

        Table(pager& pager, PageID first_page, Schema schema = default_schema());

        void insert(const Record& record);

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

    private:
        static constexpr PageID FIRST_DATA_PAGE = 1;

        pager& pager_;
        PageID first_page_;
        Schema schema_;


        std::unique_ptr<PrimaryIndex> primary_index_;

        std::optional<std::size_t> primary_key_column_;

        [[nodiscard]]
        Result<RecordID> insert_record_data(const Record& record);

        [[nodiscard]]
        Result<Record> fetch_record_data(RecordID id);
    };

} // namespace qdb
