
#pragma once

#include "page.hpp"

#include <cstddef>
#include <filesystem>
#include <fstream>

namespace qdb {

class pager {
public:
    explicit pager(const std::filesystem::path& path);

    bool read_page(std::size_t page_number, Page& page);
    bool write_page(std::size_t page_number, const Page& page);

private:
    std::fstream file_;
};

} // namespace qdb
