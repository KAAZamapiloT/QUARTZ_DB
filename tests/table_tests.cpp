#include <storage/pager.hpp>
#include <storage/record.hpp>
#include <storage/table.hpp>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

namespace {

    void check(bool condition, const char* message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }

    qdb::Record make_record(std::size_t i) {
        qdb::Record record;

        record.values.emplace_back(static_cast<std::int64_t>(i));

        // Variable-length text exercises the slotted-page layout.
        record.values.emplace_back(
            std::string(
                80 + (i % 31),
                static_cast<char>('a' + i % 26)
            )
        );

        if (i % 2 == 0) {
            record.values.emplace_back(std::monostate{});
        }
        else {
            record.values.emplace_back(
                static_cast<std::int64_t>(i * 10)
            );
        }

        return record;
    }

    void check_records(
        const std::vector<qdb::Record>& actual,
        const std::vector<qdb::Record>& expected
    ) {
        check(actual.size() == expected.size(),
            "Record count mismatch");

        for (std::size_t i = 0; i < expected.size(); ++i) {
            check(
                actual[i].values == expected[i].values,
                "Record contents/order mismatch"
            );
        }
    }

} // namespace

void run_table_tests() {
    const auto path =
        std::filesystem::temp_directory_path()
        / "quartzdb_table_test.db";

    std::error_code ec;
    std::filesystem::remove(path, ec);
    check(!ec, "Could not remove previous test database");

    constexpr std::size_t RECORD_COUNT = 120;

    std::vector<qdb::Record> expected;
    expected.reserve(RECORD_COUNT);

    // Retain the starting page ID across pager lifetimes.
    qdb::PageID first_page{};

    // Test 1: Create a new table, insert records, and force page growth.
    {
        qdb::pager storage(path);
        qdb::Table table = qdb::Table::create(storage);

        first_page = table.first_page();

        check(first_page != 0,
            "Table's first page cannot be reserved page 0");

        check(table.scan().empty(),
            "New table should initially be empty");

        for (std::size_t i = 0; i < RECORD_COUNT; ++i) {
            auto record = make_record(i);
            table.insert(record);
            expected.push_back(std::move(record));
        }

        check_records(table.scan(), expected);

        check(
            storage.page_count() > 2,
            "Test did not allocate multiple data pages"
        );

        std::cout
            << "[PASS] Insert and scan across multiple pages\n";
    }

    // Test 2: Reopen the pager and attach to the existing table.
    {
        qdb::pager storage(path);
        qdb::Table table(storage, first_page);

        check_records(table.scan(), expected);

        std::cout
            << "[PASS] Records survive database reopen\n";
    }

    std::filesystem::remove(path, ec);
    check(!ec, "Could not clean up test database");

    std::cout << "All table tests passed!\n";
}