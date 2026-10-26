#include <storage/pager.hpp>
#include <storage/record.hpp>
#include <storage/table.hpp>

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>

int main() {
    const std::filesystem::path path = "table_test.db";
    std::filesystem::remove(path);

    {
        qdb::pager pager(path);
        qdb::Table table(pager);

        table.insert(qdb::Record{
            .values = {
                std::int64_t{ 1 },
                std::string{ "Aman" }
            }
            });

        table.insert(qdb::Record{
            .values = {
                std::int64_t{ 2 },
                std::string{ "Uday" }
            }
            });

        table.insert(qdb::Record{
            .values = {
                std::int64_t{ 3 },
                std::string{ "Ravi" }
            }
            });

        const auto records = table.scan();

        assert(records.size() == 3);
        assert(records[0].values[0] == qdb::Value{ std::int64_t{ 1 } });
        assert(records[1].values[1] == qdb::Value{ std::string{ "Uday" } });
        assert(records[2].values[1] == qdb::Value{ std::string{ "Ravi" } });
    }

    std::cout << "Table insert and scan tests passed!\n";
}