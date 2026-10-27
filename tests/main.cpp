#include <gtest/gtest.h>

void run_table_tests();
void run_persistence_allocation_tests();

TEST(LegacyTests, TableTests) {
    run_table_tests();
}

TEST(LegacyTests, PersistenceAllocationTests) {
    run_persistence_allocation_tests();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
