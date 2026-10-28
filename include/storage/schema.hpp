#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace qdb {

    enum class DataType : std::uint8_t {
        Integer = 1,
        Text = 2,
        INTEGER = 1,
        TEXT = 2
    };

    using DATA_TYPE = DataType;

    struct Column {
        std::string name;
        DataType type;
        bool nullable = false;
        bool primary_key = false;
    };

    struct Schema {
        std::vector<Column> columns;
    };

} // namespace qdb
