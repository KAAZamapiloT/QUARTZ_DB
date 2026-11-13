#include <execution/executor.hpp>
#include <quartz/database.hpp>

#include <cctype>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string value_to_string(const qdb::Value& val) {
    if (std::holds_alternative<std::monostate>(val)) {
        return "NULL";
    }
    if (std::holds_alternative<std::int64_t>(val)) {
        return std::to_string(std::get<std::int64_t>(val));
    }
    if (std::holds_alternative<std::string>(val)) {
        return std::get<std::string>(val);
    }
    return "";
}

void print_table_grid(const std::vector<std::string>& headers,
                      const std::vector<qdb::Record>& rows) {
    if (headers.empty()) {
        return;
    }

    const std::size_t num_cols = headers.size();
    std::vector<std::size_t> col_widths(num_cols);

    for (std::size_t c = 0; c < num_cols; ++c) {
        col_widths[c] = headers[c].size();
    }

    for (const auto& row : rows) {
        for (std::size_t c = 0; c < num_cols; ++c) {
            if (c < row.values.size()) {
                const std::string s = value_to_string(row.values[c]);
                col_widths[c] = std::max(col_widths[c], s.size());
            }
        }
    }

    // Border builder
    auto print_border = [&]() {
        std::cout << "+";
        for (std::size_t c = 0; c < num_cols; ++c) {
            std::cout << std::string(col_widths[c] + 2, '-') << "+";
        }
        std::cout << "\n";
    };

    // Print Header
    print_border();
    std::cout << "|";
    for (std::size_t c = 0; c < num_cols; ++c) {
        std::cout << " " << std::left << std::setw(static_cast<int>(col_widths[c])) << headers[c] << " |";
    }
    std::cout << "\n";
    print_border();

    // Print Rows
    for (const auto& row : rows) {
        std::cout << "|";
        for (std::size_t c = 0; c < num_cols; ++c) {
            std::string s;
            if (c < row.values.size()) {
                s = value_to_string(row.values[c]);
            }
            std::cout << " " << std::left << std::setw(static_cast<int>(col_widths[c])) << s << " |";
        }
        std::cout << "\n";
    }
    print_border();
}

void print_help() {
    std::cout << "\nQuartzDB Interactive Terminal\n"
              << "===================================================================\n"
              << "Database Selection & Creation:\n"
              << "  CREATE DATABASE <name>;  Create and switch to a new database file\n"
              << "  DROP DATABASE <name>;    Delete a database file (e.g. DROP DATABASE test;)\n"
              << "  USE <name>;              Switch active database (e.g. USE shop;)\n"
              << "  SHOW DATABASES;          List all database files in the directory\n"
              << "  SHOW TABLES;             List all tables in the current database\n"
              << "  SELECT DATABASE();       Display currently active database\n\n"
              << "Dot Commands:\n"
              << "  .open <path>       Open or create a database file (e.g. .open mydb.db)\n"
              << "  .drop <name>       Drop/delete a database file\n"
              << "  .db                Show current database file and location\n"
              << "  .tables            List all tables in the current database\n"
              << "  .schema [table]    Show column definitions for a table (or all tables)\n"
              << "  .help              Show this help menu\n"
              << "  .exit / .quit      Exit QuartzDB\n\n"
              << "SQL Query Examples (end commands with ';'):\n"
              << "  CREATE TABLE users (id INT PRIMARY KEY, name TEXT NOT NULL, email TEXT);\n"
              << "  INSERT INTO users VALUES (1, 'Alice', 'alice@example.com');\n"
              << "  SELECT * FROM users;\n"
              << "  SELECT name, email FROM users WHERE id = 1;\n"
              << "  SELECT * FROM users WHERE id > 0 AND (name = 'Alice' OR email != '');\n"
              << "  SELECT users.name, orders.product FROM users JOIN orders ON users.id = orders.user_id;\n"
              << "  UPDATE users SET email = 'alice@new.com' WHERE id = 1;\n"
              << "  DELETE FROM users WHERE id = 1 OR name = 'Bob';\n"
              << "  DROP TABLE users;\n"
              << "===================================================================\n\n";
}

bool iequals_prefix(std::string_view str, std::string_view prefix) {
    if (str.size() < prefix.size()) return false;
    for (std::size_t i = 0; i < prefix.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(str[i])) !=
            std::tolower(static_cast<unsigned char>(prefix[i]))) {
            return false;
        }
    }
    return true;
}

std::string clean_db_name(std::string name) {
    const auto first = name.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const auto last = name.find_last_not_of(" \t\r\n;");
    name = name.substr(first, last - first + 1);

    while (!name.empty() && (name.front() == '\'' || name.front() == '"' || name.front() == '`')) {
        name.erase(0, 1);
    }
    while (!name.empty() && (name.back() == '\'' || name.back() == '"' || name.back() == '`' || name.back() == ';')) {
        name.pop_back();
    }
    if (!name.empty() && name.find('.') == std::string::npos) {
        name += ".db";
    }
    return name;
}

} // namespace

int main(int argc, char* argv[]) {
    std::filesystem::path current_db_path = "quartz.db";
    if (argc > 1) {
        current_db_path = argv[1];
    }

    std::cout << "===================================================================\n"
              << "  QuartzDB v1.0 - Embedded High-Performance Relational Engine\n"
              << "===================================================================\n";

    std::unique_ptr<qdb::Database> db;
    std::unique_ptr<qdb::SQLExecutor> executor;

    auto open_database = [&](const std::filesystem::path& path) -> bool {
        try {
            db = std::make_unique<qdb::Database>(path);
            executor = std::make_unique<qdb::SQLExecutor>(*db);
            current_db_path = path;
            std::cout << "Connected to database: " << std::filesystem::absolute(path).string() << "\n";
            std::cout << "Type SQL statements ending with ';' or '.help' for commands.\n\n";
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to open database '" << path.string() << "': " << e.what() << "\n\n";
            return false;
        }
    };

    if (!open_database(current_db_path)) {
        return 1;
    }

    std::string sql_buffer;

    while (true) {
        if (sql_buffer.empty()) {
            std::cout << "quartz> ";
        } else {
            std::cout << "   ...> ";
        }

        std::string line;
        if (!std::getline(std::cin, line)) {
            // EOF (Ctrl+Z / Ctrl+D)
            std::cout << "\nGoodbye!\n";
            break;
        }

        // Trim leading and trailing whitespace
        const auto first_non_space = line.find_first_not_of(" \t\r\n");
        if (first_non_space == std::string::npos) {
            continue;
        }
        std::string trimmed = line.substr(first_non_space);

        // Dot commands (only valid at prompt start)
        if (sql_buffer.empty() && trimmed.front() == '.') {
            std::istringstream iss(trimmed);
            std::string cmd;
            iss >> cmd;

            if (cmd == ".exit" || cmd == ".quit") {
                std::cout << "Goodbye!\n";
                break;
            } else if (cmd == ".help") {
                print_help();
            } else if (cmd == ".db" || cmd == ".database") {
                std::cout << "Current Database: "
                          << std::filesystem::absolute(current_db_path).string()
                          << " (" << db->table_count() << " table(s))\n";
            } else if (cmd == ".open") {
                std::string new_path;
                iss >> new_path;
                if (new_path.empty()) {
                    std::cout << "Usage: .open <path/to/database.db>\n";
                } else {
                    open_database(new_path);
                }
            } else if (cmd == ".drop") {
                std::string target;
                iss >> target;
                std::string db_file = clean_db_name(target);
                if (db_file.empty()) {
                    std::cout << "Usage: .drop <name>\n";
                } else if (!std::filesystem::exists(db_file)) {
                    std::cout << "Database '" << db_file << "' does not exist.\n";
                } else {
                    std::error_code ec;
                    const bool is_current = std::filesystem::equivalent(current_db_path, db_file, ec);
                    ec.clear();
                    if (is_current) {
                        db.reset();
                        executor.reset();
                        if (!std::filesystem::remove(db_file, ec)) {
                            std::cout << "Failed to drop active database '" << db_file << "': " << ec.message() << "\n";
                            open_database(db_file);
                        } else {
                            std::cout << "Active database '" << db_file << "' dropped.\n";
                            open_database("quartz.db");
                        }
                    } else {
                        if (!std::filesystem::remove(db_file, ec)) {
                            std::cout << "Failed to drop database '" << db_file << "': " << ec.message() << "\n";
                        } else {
                            std::cout << "Database '" << db_file << "' dropped.\n";
                        }
                    }
                }
            } else if (cmd == ".tables") {
                auto tables = db->list_tables();
                if (tables.empty()) {
                    std::cout << "(No tables in database)\n";
                } else {
                    for (const auto& tbl_name : tables) {
                        std::cout << "  " << tbl_name << "\n";
                    }
                }
            } else if (cmd == ".schema") {
                std::string target_table;
                iss >> target_table;

                auto print_schema_for = [&](const std::string& name) {
                    auto* t = db->find_table(name);
                    if (!t) {
                        std::cout << "Table '" << name << "' not found.\n";
                        return;
                    }
                    std::cout << "TABLE " << name << " (\n";
                    const auto s = t->get_schema();
                    for (std::size_t i = 0; i < s.columns.size(); ++i) {
                        const auto& col = s.columns[i];
                        std::cout << "    " << col.name << " "
                                  << (col.type == qdb::DataType::Integer ? "INT" : "TEXT");
                        if (col.primary_key) {
                            std::cout << " PRIMARY KEY";
                        }
                        if (!col.nullable) {
                            std::cout << " NOT NULL";
                        }
                        if (i + 1 < s.columns.size()) {
                            std::cout << ",";
                        }
                        std::cout << "\n";
                    }
                    std::cout << ");\n";
                };

                if (!target_table.empty()) {
                    print_schema_for(target_table);
                } else {
                    auto tables = db->list_tables();
                    if (tables.empty()) {
                        std::cout << "(No tables in database)\n";
                    } else {
                        for (const auto& tname : tables) {
                            print_schema_for(tname);
                            std::cout << "\n";
                        }
                    }
                }
            } else {
                std::cout << "Unknown command: '" << cmd << "'. Type .help for a list of commands.\n";
            }
            continue;
        }

        // Accumulate SQL query
        if (!sql_buffer.empty()) {
            sql_buffer += " ";
        }
        sql_buffer += trimmed;

        // Check if query is terminated by semicolon
        if (sql_buffer.back() == ';') {
            std::string query_to_run = sql_buffer;
            sql_buffer.clear();

            // 1. CREATE DATABASE / CREATE SCHEMA
            if (iequals_prefix(query_to_run, "CREATE DATABASE ") || iequals_prefix(query_to_run, "CREATE SCHEMA ")) {
                const auto first_space = query_to_run.find(' ');
                const std::string after_first = query_to_run.substr(first_space + 1);
                const auto second_space = after_first.find(' ');
                const std::string raw_name = after_first.substr(second_space + 1);
                const std::string db_file = clean_db_name(raw_name);

                if (db_file.empty()) {
                    std::cout << "Error: Database name required. Usage: CREATE DATABASE <name>;\n\n";
                } else {
                    const bool existed = std::filesystem::exists(db_file);
                    if (open_database(db_file)) {
                        if (existed) {
                            std::cout << "Database '" << db_file << "' already exists. Switched to database.\n\n";
                        } else {
                            std::cout << "Database '" << db_file << "' created successfully. Switched to database.\n\n";
                        }
                    }
                }
                continue;
            }

            // 2. DROP DATABASE / DROP SCHEMA
            if (iequals_prefix(query_to_run, "DROP DATABASE ") || iequals_prefix(query_to_run, "DROP SCHEMA ")) {
                const auto first_space = query_to_run.find(' ');
                const std::string after_first = query_to_run.substr(first_space + 1);
                const auto second_space = after_first.find(' ');
                std::string raw_name = after_first.substr(second_space + 1);

                bool if_exists = false;
                if (iequals_prefix(raw_name, "IF EXISTS ")) {
                    if_exists = true;
                    raw_name = raw_name.substr(10);
                }

                const std::string db_file = clean_db_name(raw_name);
                if (db_file.empty()) {
                    std::cout << "Error: Database name required. Usage: DROP DATABASE [IF EXISTS] <name>;\n\n";
                } else if (!std::filesystem::exists(db_file)) {
                    if (if_exists) {
                        std::cout << "Database '" << db_file << "' does not exist (skipped).\n\n";
                    } else {
                        std::cout << "Error: Database '" << db_file << "' does not exist.\n\n";
                    }
                } else {
                    std::error_code ec;
                    const bool is_current = std::filesystem::equivalent(current_db_path, db_file, ec);
                    ec.clear();

                    if (is_current) {
                        db.reset();
                        executor.reset();

                        if (!std::filesystem::remove(db_file, ec)) {
                            std::cout << "Error: Failed to drop active database '" << db_file << "': "
                                      << ec.message() << "\n\n";
                            open_database(db_file);
                        } else {
                            std::cout << "Active database '" << db_file << "' dropped.\n";
                            open_database("quartz.db");
                        }
                    } else {
                        if (!std::filesystem::remove(db_file, ec)) {
                            std::cout << "Error: Failed to drop database '" << db_file << "': "
                                      << ec.message() << "\n\n";
                        } else {
                            std::cout << "Database '" << db_file << "' dropped successfully.\n\n";
                        }
                    }
                }
                continue;
            }

            // 3. USE <db>
            if (iequals_prefix(query_to_run, "USE ")) {
                const std::string raw_name = query_to_run.substr(4);
                const std::string db_file = clean_db_name(raw_name);

                if (db_file.empty()) {
                    std::cout << "Error: Database name required. Usage: USE <name>;\n\n";
                } else if (!std::filesystem::exists(db_file)) {
                    std::cout << "Error: Database '" << db_file << "' not found. Use 'CREATE DATABASE "
                              << db_file << ";' or '.open " << db_file << "' to create it.\n\n";
                } else {
                    if (open_database(db_file)) {
                        std::cout << "Database changed to '" << db_file << "'.\n\n";
                    }
                }
                continue;
            }

            // 4. SHOW DATABASES / SHOW SCHEMAS
            if (iequals_prefix(query_to_run, "SHOW DATABASES") || iequals_prefix(query_to_run, "SHOW SCHEMAS")) {
                std::vector<std::string> db_names;
                try {
                    for (const auto& entry : std::filesystem::directory_iterator(".")) {
                        if (entry.is_regular_file() && entry.path().extension() == ".db") {
                            db_names.push_back(entry.path().filename().string());
                        }
                    }
                } catch (...) {}

                if (db_names.empty()) {
                    std::cout << "(No .db files found in current directory)\n\n";
                } else {
                    std::vector<qdb::Record> rows;
                    for (const auto& d : db_names) {
                        qdb::Record rec;
                        rec.values.push_back(d);
                        rows.push_back(rec);
                    }
                    print_table_grid({"Database"}, rows);
                    std::cout << rows.size() << " row(s) in set\n\n";
                }
                continue;
            }

            // 5. SHOW TABLES
            if (iequals_prefix(query_to_run, "SHOW TABLES")) {
                auto tables = db->list_tables();
                std::vector<qdb::Record> rows;
                for (const auto& t : tables) {
                    qdb::Record rec;
                    rec.values.push_back(t);
                    rows.push_back(rec);
                }
                const std::string header = "Tables_in_" + current_db_path.filename().string();
                print_table_grid({header}, rows);
                std::cout << rows.size() << " row(s) in set\n\n";
                continue;
            }

            // 6. SELECT DATABASE()
            if (iequals_prefix(query_to_run, "SELECT DATABASE()") || iequals_prefix(query_to_run, "SELECT DATABASE ()")) {
                qdb::Record rec;
                rec.values.push_back(current_db_path.filename().string());
                print_table_grid({"DATABASE()"}, {rec});
                std::cout << "1 row in set\n\n";
                continue;
            }

            // Standard SQL statement execution via SQLExecutor
            const auto start_time = std::chrono::high_resolution_clock::now();
            auto result = executor->execute(query_to_run);
            const auto end_time = std::chrono::high_resolution_clock::now();
            const double elapsed_ms =
                std::chrono::duration<double, std::milli>(end_time - start_time).count();

            if (!result.success) {
                std::cout << "Error: " << result.message << "\n";
            } else {
                if (!result.column_names.empty()) {
                    print_table_grid(result.column_names, result.rows);
                    std::cout << result.rows.size() << " row(s) in set ("
                              << std::fixed << std::setprecision(2) << elapsed_ms << " ms)\n\n";
                } else {
                    std::cout << result.message << " ("
                              << std::fixed << std::setprecision(2) << elapsed_ms << " ms)\n\n";
                }
            }
        }
    }

    return 0;
}
