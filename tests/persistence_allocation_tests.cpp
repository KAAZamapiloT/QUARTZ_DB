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
        std::string(80 + (i % 31), static_cast<char>('a' + (i % 26)))
    );

    if (i % 2 == 0) {
        record.values.emplace_back(std::monostate{});
    } else {
        record.values.emplace_back(static_cast<std::int64_t>(i * 10));
    }

    return record;
}

void check_records(
    const std::vector<qdb::Record>& actual,
    const std::vector<qdb::Record>& expected
) {
    check(actual.size() == expected.size(), "Record count mismatch");

    for (std::size_t i = 0; i < expected.size(); ++i) {
        check(
            actual[i].values == expected[i].values,
            "Record contents/order mismatch"
        );
    }
}

} // namespace

// Call this from tests/main.cpp alongside the other test groups.
void run_persistence_allocation_tests() {
    const auto path = std::filesystem::temp_directory_path()
        / "quartzdb_persistence_allocation_test.db";

    std::error_code ec;
    std::filesystem::remove(path, ec);
    check(!ec, "Could not remove previous test database");

    constexpr std::size_t RECORDS_PER_SESSION = 120;
    constexpr std::size_t TOTAL_RECORDS = RECORDS_PER_SESSION * 2;

    std::vector<qdb::Record> expected;
    expected.reserve(TOTAL_RECORDS);

    // Session 1: create a database and insert enough rows to use multiple pages.
    {
        qdb::pager storage(path);
        qdb::Table table(storage);

        for (std::size_t i = 0; i < RECORDS_PER_SESSION; ++i) {
            auto record = make_record(i);
            table.insert(record);
            expected.push_back(std::move(record));
        }

        check_records(table.scan(), expected);
    }

    // Session 2: reopen, verify old rows, then allocate/write more pages.
    {
        qdb::pager storage(path);
        qdb::Table table(storage);

        check_records(table.scan(), expected);

        for (std::size_t i = RECORDS_PER_SESSION; i < TOTAL_RECORDS; ++i) {
            auto record = make_record(i);
            table.insert(record);
            expected.push_back(std::move(record));
        }

        check_records(table.scan(), expected);
    }

    // Session 3: reopen again and verify all records survived.
    {
        qdb::pager storage(path);
        qdb::Table table(storage);
        check_records(table.scan(), expected);
    }

    std::filesystem::remove(path, ec);
    check(!ec, "Could not remove test database after test");

    std::cout
        << "[PASS] Insert after reopen preserves existing pages and records\n"
        << "[PASS] All " << TOTAL_RECORDS
        << " records survive the second reopen\n";
}
