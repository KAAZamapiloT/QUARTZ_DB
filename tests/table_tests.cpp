#include <storage/pager.hpp>
#include <storage/record.hpp>
#include <storage/table.hpp>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
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
            std::string(80 + (i % 31), static_cast<char>('a' + i % 26))
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
                "Record contents mismatch"
            );
        }
    }

} // namespace

void run_table_tests() {
    const auto path =
        std::filesystem::temp_directory_path() / "quartzdb_table_test.db";

    std::error_code ec;
    std::filesystem::remove(path, ec);
    check(!ec, "Could not remove previous test database");

    constexpr std::size_t RECORD_COUNT = 120;

    std::vector<qdb::Record> expected;
    expected.reserve(RECORD_COUNT);

    // Test 1: Fresh database, insert, scan, and force page growth.
    {
        qdb::pager storage(path);
        qdb::Table table(storage);

        check(table.scan().empty(),
            "New table should initially be empty");

        for (std::size_t i = 0; i < RECORD_COUNT; ++i) {
            auto record = make_record(i);
            table.insert(record);
            expected.push_back(std::move(record));
        }

        const auto actual = table.scan();
        check_records(actual, expected);

        check(storage.page_count() > 2,
            "Test did not allocate multiple data pages");

        std::cout << "[PASS] Insert and scan across multiple pages\n";
    }

    // Test 2: Destroy and reopen the pager and table.
    {
        qdb::pager storage(path);
        qdb::Table table(storage);

        const auto actual = table.scan();
        check_records(actual, expected);

        std::cout << "[PASS] Records survive database reopen\n";
    }

    std::filesystem::remove(path, ec);
    check(!ec, "Could not clean up test database");

    std::cout << "All table tests passed!\n";
}