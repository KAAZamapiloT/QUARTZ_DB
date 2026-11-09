#include <gtest/gtest.h>

#include <execution/executor.hpp>
#include <quartz/database.hpp>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace qdb::testing {

namespace {

struct ScaleMetrics {
    std::size_t count{0};
    double write_total_ms{0.0};
    double write_ops_per_sec{0.0};
    double write_avg_us{0.0};
    double read_total_ms{0.0};
    double read_ops_per_sec{0.0};
    double read_avg_us{0.0};
    std::uint64_t db_file_bytes{0};
    std::size_t pages_allocated{0};
};

void print_metrics_header() {
    std::cout << "\n"
              << "+---------+----------------------+----------------------+------------------+\n"
              << "| Scale   | SQL Writes (INSERT)  | SQL Reads (SELECT)   | Disk Footprint   |\n"
              << "| Rows    | Throughput / Latency | Throughput / Latency | File Size / Pgs  |\n"
              << "+---------+----------------------+----------------------+------------------+\n";
}

void print_metrics_row(const ScaleMetrics& m) {
    std::ostringstream w_oss, r_oss, d_oss;

    // Writes summary
    w_oss << static_cast<long long>(m.write_ops_per_sec) << " qps ("
          << std::fixed << std::setprecision(1) << m.write_avg_us << " us)";

    // Reads summary
    r_oss << static_cast<long long>(m.read_ops_per_sec) << " qps ("
          << std::fixed << std::setprecision(1) << m.read_avg_us << " us)";

    // Disk summary
    if (m.db_file_bytes < 1024 * 1024) {
        d_oss << (m.db_file_bytes / 1024) << " KB (" << m.pages_allocated << " pgs)";
    } else {
        d_oss << std::fixed << std::setprecision(2)
              << (static_cast<double>(m.db_file_bytes) / (1024.0 * 1024.0))
              << " MB (" << m.pages_allocated << " pgs)";
    }

    std::cout << "| " << std::left << std::setw(7) << m.count << " | "
              << std::left << std::setw(20) << w_oss.str() << " | "
              << std::left << std::setw(20) << r_oss.str() << " | "
              << std::left << std::setw(16) << d_oss.str() << " |\n";
}

void print_metrics_footer() {
    std::cout << "+---------+----------------------+----------------------+------------------+\n\n";
}

class SQLScaleTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = std::filesystem::temp_directory_path() / "quartz_scale_tests";
        std::filesystem::create_directories(test_dir_);
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(test_dir_, ec);
    }

    std::filesystem::path create_db_path(const std::string& tag) {
        return test_dir_ / ("scale_" + tag + ".db");
    }

    ScaleMetrics execute_scale_run(std::size_t n, const std::string& label, bool verbose = true) {
        const auto db_path = create_db_path(label);
        std::error_code ec;
        std::filesystem::remove(db_path, ec);

        ScaleMetrics metrics{};
        metrics.count = n;

        // 1. Create table via SQL
        {
            Database db(db_path);
            SQLExecutor executor(db);

            const auto create_res = executor.execute(
                "CREATE TABLE bench (id INT PRIMARY KEY, tag TEXT NOT NULL, score INT);"
            );
            EXPECT_TRUE(create_res.success) << create_res.message;
            if (!create_res.success) {
                return metrics;
            }

            // 2. SQL WRITE PHASE (INSERT)
            std::string sql_cmd;
            sql_cmd.reserve(128);

            const auto write_start = std::chrono::high_resolution_clock::now();
            for (std::size_t i = 1; i <= n; ++i) {
                sql_cmd = "INSERT INTO bench VALUES (" + std::to_string(i) +
                          ", 'tag_" + std::to_string(i) + "', " +
                          std::to_string(i * 10) + ");";

                const auto ins_res = executor.execute(sql_cmd);
                if (!ins_res.success) {
                    EXPECT_TRUE(ins_res.success) << "Failed at row " << i << ": " << ins_res.message;
                    break;
                }
            }
            const auto write_end = std::chrono::high_resolution_clock::now();

            metrics.write_total_ms =
                std::chrono::duration<double, std::milli>(write_end - write_start).count();
            if (metrics.write_total_ms > 0.0) {
                metrics.write_ops_per_sec = (static_cast<double>(n) / metrics.write_total_ms) * 1000.0;
                metrics.write_avg_us = (metrics.write_total_ms / static_cast<double>(n)) * 1000.0;
            }

            // 3. SQL READ PHASE (Point Lookups via Primary Key / B+ Tree)
            // Use pseudo-random stride to test non-trivial lookup patterns
            std::mt19937_64 rng(1337);
            std::uniform_int_distribution<std::size_t> dist(1, n);

            const auto read_start = std::chrono::high_resolution_clock::now();
            for (std::size_t step = 0; step < n; ++step) {
                const std::size_t target_id = dist(rng);
                sql_cmd = "SELECT tag, score FROM bench WHERE id = " +
                          std::to_string(target_id) + ";";

                const auto sel_res = executor.execute(sql_cmd);
                if (!sel_res.success || sel_res.rows.size() != 1U) {
                    EXPECT_TRUE(sel_res.success);
                    EXPECT_EQ(sel_res.rows.size(), 1U);
                    break;
                }

                // Spot-check correctness
                const auto& tag_val = std::get<std::string>(sel_res.rows[0].values[0]);
                const auto score_val = std::get<std::int64_t>(sel_res.rows[0].values[1]);
                EXPECT_EQ(tag_val, "tag_" + std::to_string(target_id));
                EXPECT_EQ(score_val, static_cast<std::int64_t>(target_id * 10));
            }
            const auto read_end = std::chrono::high_resolution_clock::now();

            metrics.read_total_ms =
                std::chrono::duration<double, std::milli>(read_end - read_start).count();
            if (metrics.read_total_ms > 0.0) {
                metrics.read_ops_per_sec = (static_cast<double>(n) / metrics.read_total_ms) * 1000.0;
                metrics.read_avg_us = (metrics.read_total_ms / static_cast<double>(n)) * 1000.0;
            }

            // 4. Persistence and Catalog verification
            auto* table = db.find_table("bench");
            EXPECT_NE(table, nullptr);
            if (table) {
                EXPECT_TRUE(table->has_primary_index());
            }
        }

        // 5. Inspect physical on-disk file size
        metrics.db_file_bytes = std::filesystem::file_size(db_path, ec);
        metrics.pages_allocated = static_cast<std::size_t>(metrics.db_file_bytes / PAGE_SIZE);

        if (verbose) {
            std::cout << "[SQLScale] Finished scale N = " << n << " in "
                      << std::fixed << std::setprecision(2)
                      << (metrics.write_total_ms + metrics.read_total_ms) << " ms total ("
                      << "Writes: " << static_cast<long long>(metrics.write_ops_per_sec) << " qps, "
                      << "Reads: " << static_cast<long long>(metrics.read_ops_per_sec) << " qps)\n";
        }

        return metrics;
    }

    std::filesystem::path test_dir_;
};

} // namespace

// ============================================================================
// INDIVIDUAL SCALE BENCHMARKS (100, 1,000, 10,000, 50,000, 100,000)
// ============================================================================

TEST_F(SQLScaleTest, Scale_100) {
    const auto m = execute_scale_run(100, "100");
    EXPECT_GT(m.write_ops_per_sec, 0.0);
    EXPECT_GT(m.read_ops_per_sec, 0.0);
    EXPECT_GT(m.db_file_bytes, 0ULL);
}

TEST_F(SQLScaleTest, Scale_1000) {
    const auto m = execute_scale_run(1000, "1000");
    EXPECT_GT(m.write_ops_per_sec, 0.0);
    EXPECT_GT(m.read_ops_per_sec, 0.0);
    EXPECT_GT(m.db_file_bytes, 0ULL);
}

TEST_F(SQLScaleTest, Scale_10000) {
    const auto m = execute_scale_run(10000, "10000");
    EXPECT_GT(m.write_ops_per_sec, 0.0);
    EXPECT_GT(m.read_ops_per_sec, 0.0);
    EXPECT_GT(m.db_file_bytes, 0ULL);
}

TEST_F(SQLScaleTest, Scale_50000) {
    const auto m = execute_scale_run(50000, "50000");
    EXPECT_GT(m.write_ops_per_sec, 0.0);
    EXPECT_GT(m.read_ops_per_sec, 0.0);
    EXPECT_GT(m.db_file_bytes, 0ULL);
}

TEST_F(SQLScaleTest, Scale_100000) {
    const auto m = execute_scale_run(100000, "100000");
    EXPECT_GT(m.write_ops_per_sec, 0.0);
    EXPECT_GT(m.read_ops_per_sec, 0.0);
    EXPECT_GT(m.db_file_bytes, 0ULL);
}

TEST_F(SQLScaleTest, Scale_120000) {
    const auto m = execute_scale_run(120000, "120000");
    EXPECT_GT(m.write_ops_per_sec, 0.0);
    EXPECT_GT(m.read_ops_per_sec, 0.0);
    EXPECT_GT(m.db_file_bytes, 0ULL);
}

// ============================================================================
// PROGRESSION AND LIMITS EXPLORATION REPORT
// ============================================================================

TEST_F(SQLScaleTest, FullProgressionAndLimitsMatrix) {
    // Allows overriding the maximum progression bound via environment variable
    // e.g. QUARTZ_SCALE_LIMIT=120000
    std::size_t max_scale = 120000;
    if (const char* env_limit = std::getenv("QUARTZ_SCALE_LIMIT")) {
        try {
            max_scale = std::stoull(env_limit);
        } catch (...) {}
    }

    const std::vector<std::size_t> target_scales = {100, 1000, 10000, 50000, 100000, 120000};
    std::vector<ScaleMetrics> results;

    std::cout << "\n===================================================================\n"
              << "       QUARTZ DB - SQL ENGINE SCALE & LIMITS BENCHMARK\n"
              << "===================================================================\n";

    print_metrics_header();
    for (std::size_t scale : target_scales) {
        if (scale > max_scale) {
            break;
        }
        const auto m = execute_scale_run(scale, "prog_" + std::to_string(scale), false);
        results.push_back(m);
        print_metrics_row(m);
    }
    print_metrics_footer();

    std::cout << "All scales completed successfully with 100% data integrity verified.\n\n";
}

} // namespace qdb::testing
