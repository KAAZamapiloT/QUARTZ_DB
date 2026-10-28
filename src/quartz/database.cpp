#include <quartz/database.hpp>
#include <storage/page.hpp>
#include <storage/pager.hpp>
#include <storage/table.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_set>
#include <utility>

namespace qdb {

    namespace {
        constexpr std::uint32_t DB_MAGIC = 0x31424451u; // "QDB1" in little-endian
        constexpr std::uint16_t DB_VERSION = 1;

        constexpr std::size_t CATALOG_HEADER_SIZE = 12;
        constexpr std::size_t ENTRY_FIXED_SIZE = 10;
        constexpr std::size_t MAX_TABLE_NAME_BYTES = 255;

        void write_u16(Page& page, std::size_t offset, std::uint16_t value) {
            for (unsigned i = 0; i < 2; ++i) {
                page.data[offset + i] =
                    static_cast<std::byte>((value >> (i * 8)) & 0xFFu);
            }
        }

        std::uint16_t read_u16(const Page& page, std::size_t offset) {
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

        void write_u32(Page& page, std::size_t offset, std::uint32_t value) {
            for (unsigned i = 0; i < 4; ++i) {
                page.data[offset + i] =
                    static_cast<std::byte>((value >> (i * 8)) & 0xFFu);
            }
        }

        std::uint32_t read_u32(const Page& page, std::size_t offset) {
            std::uint32_t value = 0;
            for (unsigned i = 0; i < 4; ++i) {
                value |= static_cast<std::uint32_t>(
                    std::to_integer<unsigned char>(page.data[offset + i])
                ) << (i * 8);
            }
            return value;
        }

        void write_u64(Page& page, std::size_t offset, std::uint64_t value) {
            for (unsigned i = 0; i < 8; ++i) {
                page.data[offset + i] =
                    static_cast<std::byte>((value >> (i * 8)) & 0xFFULL);
            }
        }

        std::uint64_t read_u64(const Page& page, std::size_t offset) {
            std::uint64_t value = 0;
            for (unsigned i = 0; i < 8; ++i) {
                value |= static_cast<std::uint64_t>(
                    std::to_integer<unsigned char>(page.data[offset + i])
                ) << (i * 8);
            }
            return value;
        }

        void append_name(Page& page, std::size_t offset, const std::string& name) {
            for (std::size_t i = 0; i < name.size(); ++i) {
                page.data[offset + i] = static_cast<std::byte>(name[i]);
            }
        }

        std::string read_name(const Page& page,
            std::size_t offset,
            std::size_t length) {
            std::string name;
            name.reserve(length);
            for (std::size_t i = 0; i < length; ++i) {
                name.push_back(static_cast<char>(page.data[offset + i]));
            }
            return name;
        }
    } // namespace

    Database::Database(const std::filesystem::path& file_path) : pager_(file_path) {
        std::error_code ec;
        const auto file_bytes = std::filesystem::file_size(file_path, ec);

        if (ec) {
            throw std::runtime_error("Failed to determine database file size");
        }

        if (file_bytes == 0) {
            initialize_catalog();
        } else {
            if (file_bytes % PAGE_SIZE != 0) {
                throw std::runtime_error("Database file size is not a multiple of PAGE_SIZE");
            }
            load_catalog();
        }
    }

    void Database::initialize_catalog() {
        Page page{};

        write_u32(page, 0, DB_MAGIC);
        write_u16(page, 4, DB_VERSION);
        write_u16(page, 6, 0);
        write_u32(page, 8, static_cast<std::uint32_t>(PAGE_SIZE));

        if (!pager_.write_page(0, page)) {
            throw std::runtime_error("Failed to initialize database catalog");
        }
    }

    void Database::load_catalog() {
        Page page{};

        if (!pager_.read_page(0, page)) {
            throw std::runtime_error("Failed to read database catalog");
        }

        if (read_u32(page, 0) != DB_MAGIC) {
            throw std::runtime_error("Invalid database magic");
        }

        if (read_u16(page, 4) != DB_VERSION) {
            throw std::runtime_error("Unsupported database format version");
        }

        const std::uint16_t count = read_u16(page, 6);

        if (read_u32(page, 8) != PAGE_SIZE) {
            throw std::runtime_error("Database page-size mismatch");
        }

        std::size_t offset = CATALOG_HEADER_SIZE;
        std::unordered_set<PageID> used_first_pages;

        for (std::uint16_t i = 0; i < count; ++i) {
            if (offset > PAGE_SIZE || PAGE_SIZE - offset < ENTRY_FIXED_SIZE) {
                throw std::runtime_error("Truncated catalog entry");
            }

            const std::uint16_t name_length = read_u16(page, offset);
            offset += 2;

            const PageID first_page = read_u64(page, offset);
            offset += 8;

            if (name_length == 0 ||
                name_length > MAX_TABLE_NAME_BYTES ||
                name_length > PAGE_SIZE - offset) {
                throw std::runtime_error("Invalid table name in catalog");
            }

            std::string name = read_name(page, offset, name_length);
            offset += name_length;

            if (first_page == 0 ||
                first_page >= static_cast<PageID>(pager_.page_count())) {
                throw std::runtime_error("Catalog references an invalid table page");
            }

            if (tables_.contains(name)) {
                throw std::runtime_error("Duplicate table name in catalog");
            }

            if (!used_first_pages.insert(first_page).second) {
                throw std::runtime_error("Multiple tables reference the same first page");
            }

            // Read schema if available
            Schema schema;
            if (offset + 2 <= PAGE_SIZE) {
                const std::uint16_t col_count = read_u16(page, offset);
                offset += 2;

                std::vector<Column> columns;
                columns.reserve(col_count);

                for (std::uint16_t c = 0; c < col_count; ++c) {
                    if (offset + 2 > PAGE_SIZE) {
                        throw std::runtime_error("Truncated column definition");
                    }
                    const std::uint16_t col_name_len = read_u16(page, offset);
                    offset += 2;

                    if (offset + col_name_len + 3 > PAGE_SIZE) {
                        throw std::runtime_error("Truncated column details");
                    }

                    std::string col_name = read_name(page, offset, col_name_len);
                    offset += col_name_len;

                    const auto col_type = static_cast<DataType>(std::to_integer<std::uint8_t>(page.data[offset++]));
                    const bool nullable = std::to_integer<std::uint8_t>(page.data[offset++]) != 0;
                    const bool primary_key = std::to_integer<std::uint8_t>(page.data[offset++]) != 0;

                    columns.push_back(Column{
                        .name = std::move(col_name),
                        .type = col_type,
                        .nullable = nullable,
                        .primary_key = primary_key
                    });
                }
                schema.columns = std::move(columns);
            }

            if (schema.columns.empty()) {
                schema = default_schema();
            }

            auto table = std::make_unique<Table>(pager_, first_page, std::move(schema));
            tables_.emplace(name, std::move(table));
        }

        // Catalog pages are written from a zero-initialized Page.
        // Reject unexpected trailing bytes instead of silently ignoring them.
        for (; offset < PAGE_SIZE; ++offset) {
            if (page.data[offset] != std::byte{ 0 }) {
                throw std::runtime_error("Unexpected data after catalog entries");
            }
        }
    }

    void Database::persist_catalog() {
        Page page{};

        write_u32(page, 0, DB_MAGIC);
        write_u16(page, 4, DB_VERSION);
        write_u16(page, 6, static_cast<std::uint16_t>(tables_.size()));
        write_u32(page, 8, static_cast<std::uint32_t>(PAGE_SIZE));

        std::size_t offset = CATALOG_HEADER_SIZE;

        for (const auto& [name, table] : tables_) {
            const auto name_len = name.size();
            const auto& schema = table->get_schema();
            const auto col_count = schema.columns.size();

            if (name_len > MAX_TABLE_NAME_BYTES ||
                offset + ENTRY_FIXED_SIZE + name_len > PAGE_SIZE) {
                throw std::runtime_error("Catalog exceeds page capacity");
            }

            write_u16(page, offset, static_cast<std::uint16_t>(name_len));
            offset += 2;

            write_u64(page, offset, table->first_page());
            offset += 8;

            append_name(page, offset, name);
            offset += name_len;

            write_u16(page, offset, static_cast<std::uint16_t>(col_count));
            offset += 2;

            for (const auto& col : schema.columns) {
                const auto col_name_len = col.name.size();
                if (offset + 2 + col_name_len + 3 > PAGE_SIZE) {
                    throw std::runtime_error("Catalog exceeds page capacity");
                }

                write_u16(page, offset, static_cast<std::uint16_t>(col_name_len));
                offset += 2;

                append_name(page, offset, col.name);
                offset += col_name_len;

                page.data[offset++] = static_cast<std::byte>(col.type);
                page.data[offset++] = static_cast<std::byte>(col.nullable ? 1 : 0);
                page.data[offset++] = static_cast<std::byte>(col.primary_key ? 1 : 0);
            }
        }

        if (!pager_.write_page(0, page)) {
            throw std::runtime_error("Failed to persist database catalog");
        }
    }

    Table& Database::create_table(const std::string& name, Schema schema) {
        if (name.empty() || name.size() > MAX_TABLE_NAME_BYTES) {
            throw std::invalid_argument("Invalid table name: " + name);
        }

        if (tables_.contains(name)) {
            throw std::invalid_argument("Table already exists: " + name);
        }

        Table table = Table::create(pager_, std::move(schema));
        auto ptr = std::make_unique<Table>(std::move(table));
        auto [it, inserted] = tables_.emplace(name, std::move(ptr));

        if (!inserted) {
            throw std::runtime_error("Failed to register table");
        }

        try {
            persist_catalog();
        }
        catch (...) {
            tables_.erase(it);
            throw;
        }

        return *it->second;
    }

    Table* Database::find_table(const std::string& name) {
        const auto it = tables_.find(name);
        return it == tables_.end() ? nullptr : it->second.get();
    }

    const Table* Database::find_table(const std::string& name) const {
        const auto it = tables_.find(name);
        return it == tables_.end() ? nullptr : it->second.get();
    }

} // namespace qdb
