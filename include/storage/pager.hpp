
#pragma once

#include "storage/page.hpp"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include<vector>
namespace qdb {

    using PageID = std::uint64_t;     
class pager {
public:
    explicit pager(const std::filesystem::path& path);

    bool read_page(std::size_t page_number, Page& page);
    bool write_page(std::size_t page_number, const Page& page);

    bool allocate_page(PageID& out_id);
    std::size_t page_count() const;

    bool deallocate_page(PageID id);
private:
    std::fstream file_;
    std::size_t num_pages{1};
    std::vector<PageID> free_pages;
};

} // namespace qdb
