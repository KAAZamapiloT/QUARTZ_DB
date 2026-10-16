#pragma once

#include<array>
#include<cstddef>
#include<stdint.h>


// a simple page
namespace qdb{
    inline constexpr std::size_t  PAGE_SIZE = 4096;
struct Page{
std::array<std::byte,std::size_t(PAGE_SIZE)> data{};
};

static_assert(sizeof(Page) == PAGE_SIZE, "Page size is not 4096 bytes");



 class pager{
public:
explicit pager(const std::filesystem::path& path);

bool read_page(std::size_t page_number, Page& page);

bool write_page(std::size_t page_number, const Page& page);

private:
std::fstream file_;

    };
}


