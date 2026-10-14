
cmake_minimum_required(VERSION 3.25)

project(QuartzDB
    VERSION 0.1.0
    DESCRIPTION "A minimal embedded SQL database engine"
    LANGUAGES CXX
)

include(CTest)

option(QUARTZ_BUILD_TESTS "Build QuartzDB tests" ON)

# Shared project settings
add_library(quartz_options INTERFACE)

target_compile_features(quartz_options
    INTERFACE cxx_std_23
)

if(MSVC)
    target_compile_options(quartz_options INTERFACE
        /W4
        /permissive-
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
    src/sql/lexer.cpp
    src/sql/parser.cpp
    src/execution/executor.cpp
)

target_include_directories(quartz_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
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
    add_executable(quartz_tests
        tests/main.cpp
        tests/pager_tests.cpp
        tests/table_tests.cpp
        tests/sql_tests.cpp
        tests/index_tests.cpp
        tests/integration_tests.cpp
    )

    target_link_libraries(quartz_tests
        PRIVATE quartz_core
    )

    add_test(
        NAME quartz_tests
        COMMAND quartz_tests
    )
endif()
