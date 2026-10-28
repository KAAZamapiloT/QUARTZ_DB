#include <storage/pager.hpp>
#include <storage/record.hpp>
#include <storage/schema.hpp>
#include <storage/table.hpp>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

    void check(bool condition, const char* message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }

} // namespace

int main() {
    const std::filesystem::path path = "table_test.db";
    std::filesystem::remove(path);

    {
        qdb::pager pager(path);

        const qdb::Schema schema{
            .columns = {
                qdb::Column{
                    .name = "id",
                    .type = qdb::DataType::Integer,
                    .nullable = false,
                    .primary_key = true
                },
                qdb::Column{
                    .name = "name",
                    .type = qdb::DataType::Text,
                    .nullable = false,
                    .primary_key = false
                }
            }
        };

        // Create and initialize a new table.
        qdb::Table table = qdb::Table::create(pager, schema);

        check(table.scan().empty(),
            "New table should initially be empty");

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

        check(records.size() == 3,
            "Expected three records");

        check(records[0].values[0] == qdb::Value{ std::int64_t{ 1 } },
            "First record ID mismatch");

        check(records[1].values[1] == qdb::Value{ std::string{ "Uday" } },
            "Second record name mismatch");

        check(records[2].values[1] == qdb::Value{ std::string{ "Ravi" } },
            "Third record name mismatch");
    }

    std::filesystem::remove(path);

    std::cout << "Table insert and scan tests passed!\n";
}
