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

        record.values.emplace_back(
            std::string(
                80 + (i % 31),
                static_cast<char>('a' + (i % 26))
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

void run_persistence_allocation_tests() {
    const auto path =
        std::filesystem::temp_directory_path()
        / "quartzdb_persistence_allocation_test.db";

    std::error_code ec;
    std::filesystem::remove(path, ec);
    check(!ec, "Could not remove previous test database");

    constexpr std::size_t RECORDS_PER_SESSION = 120;
    constexpr std::size_t TOTAL_RECORDS =
        RECORDS_PER_SESSION * 2;

    std::vector<qdb::Record> expected;
    expected.reserve(TOTAL_RECORDS);

    // Retain the table's starting page ID across pager lifetimes.
    qdb::PageID first_page{};

    // Session 1: create the table and insert the first batch.
    {
        qdb::pager storage(path);

        qdb::Table table = qdb::Table::create(storage);
        first_page = table.first_page();

        check(first_page != 0,
            "Table received reserved page ID 0");

        for (std::size_t i = 0;
            i < RECORDS_PER_SESSION;
            ++i) {
            auto record = make_record(i);

            table.insert(record);
            expected.push_back(std::move(record));
        }

        check_records(table.scan(), expected);

        std::cout
            << "[PASS] Initial insertion across multiple pages\n";
    }

    // Session 2: reopen, verify old data, and insert more records.
    {
        qdb::pager storage(path);
        qdb::Table table(storage, first_page);

        check_records(table.scan(), expected);

        for (std::size_t i = RECORDS_PER_SESSION;
            i < TOTAL_RECORDS;
            ++i) {
            auto record = make_record(i);

            table.insert(record);
            expected.push_back(std::move(record));
        }

        check_records(table.scan(), expected);

        std::cout
            << "[PASS] Insert after reopen preserves existing records\n";
    }

    // Session 3: reopen again and verify all records.
    {
        qdb::pager storage(path);
        qdb::Table table(storage, first_page);

        check_records(table.scan(), expected);

        std::cout
            << "[PASS] All " << TOTAL_RECORDS
            << " records survive the second reopen\n";
    }

    std::filesystem::remove(path, ec);
    check(!ec, "Could not remove test database after test");

    std::cout << "All persistence allocation tests passed!\n";
}