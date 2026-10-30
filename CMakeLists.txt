cmake_minimum_required(VERSION 3.25)

project(QuartzDB
    VERSION 0.1.0
    DESCRIPTION "A minimal embedded SQL database engine"
    LANGUAGES CXX
)

option(QUARTZ_BUILD_TESTS "Build QuartzDB tests" ON)

# Shared project settings
add_library(quartz_options INTERFACE)

target_compile_features(quartz_options
    INTERFACE
        cxx_std_23
)

if(MSVC)
    target_compile_options(quartz_options INTERFACE
        /W4
        /permissive-
        /utf-8
    )
else()
    target_compile_options(quartz_options INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
    )
endif()

# Database engine
add_library(quartz_core STATIC
    src/storage/pager.cpp
    src/storage/table.cpp
    src/storage/record.cpp
    src/sql/lexer.cpp
    src/sql/parser.cpp
    src/execution/executor.cpp
    src/quartz/database.cpp
    src/index/bplus_tree.cpp
)

target_include_directories(quartz_core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(quartz_core
    PUBLIC quartz_options
)

# Interactive CLI
add_executable(quartz
    src/cli/main.cpp
)

target_link_libraries(quartz
    PRIVATE quartz_core
)

# Tests
if(QUARTZ_BUILD_TESTS)
    enable_testing()

    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
        GIT_SHALLOW TRUE
    )

    # Avoid MSVC runtime-library mismatch on Windows.
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(googletest)

    add_executable(quartz_tests
        tests/main.cpp
        tests/table_tests.cpp
        tests/persistence_allocation_tests.cpp
        tests/database_tests.cpp
        tests/schema_tests.cpp
        tests/bplus_tree_tests.cpp
    )

    target_link_libraries(quartz_tests
        PRIVATE
            quartz_core
            GTest::gtest
    )

    include(GoogleTest)
    gtest_discover_tests(quartz_tests
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        DISCOVERY_TIMEOUT 30
    )
endif()
