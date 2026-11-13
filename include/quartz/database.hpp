#pragma once

#include <storage/pager.hpp>
#include <storage/schema.hpp>
#include <storage/table.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

namespace qdb {

    class Database {
    public:
        explicit Database(const std::filesystem::path& path);

        Database(const Database&) = delete;
        Database& operator=(const Database&) = delete;

        Database(Database&&) = delete;
        Database& operator=(Database&&) = delete;

        [[nodiscard]]
        Table& create_table(const std::string& name, Schema schema = default_schema());

        bool drop_table(const std::string& name);

        [[nodiscard]]
        Table* find_table(const std::string& name);

        [[nodiscard]]
        const Table* find_table(const std::string& name) const;

        [[nodiscard]]
        std::size_t table_count() const noexcept {
            return tables_.size();
        }

        [[nodiscard]]
        const std::unordered_map<std::string, std::unique_ptr<Table>>& tables() const noexcept {
            return tables_;
        }

        [[nodiscard]]
        std::vector<std::string> list_tables() noexcept {
            std::vector<std::string> tables;

            for (auto& tab : tables_) {
                tables.push_back(tab.first);
            }
            return tables;
        }

    private:
        pager pager_;

        void initialize_catalog();
        void load_catalog();
        void persist_catalog();

        std::unordered_map<std::string, std::unique_ptr<Table>> tables_;
    };

    using DataBase = Database;

} // namespace qdb
