#pragma once

#include<array>
#include<cstddef>
#include<stdint.h>
#include<limits>

// a simple page
namespace qdb{
inline constexpr std::size_t  PAGE_SIZE = 4096;
struct Page{
std::array<std::byte,std::size_t(PAGE_SIZE)> data{};
};

static_assert(sizeof(Page) == PAGE_SIZE, "Page size is not 4096 bytes");


}


