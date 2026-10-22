
#include"storage/pager.hpp"

#include<filesystem>
#include<fstream>
#include<algorithm>





 qdb::pager::pager(const std::filesystem::path& path){


    if(!std::filesystem::exists(path)){

        std::ofstream create(path,std::ios::binary);
        if(!create){     
        throw std::runtime_error("Cannot create database file");
       }
     }

    file_.open(path,std::ios::in|std::ios::out|std::ios::binary);
     
    if(!file_.is_open()){
        throw std::runtime_error("Cannot open database file");
    }
}


bool qdb::pager::read_page(std::size_t page_number, Page& page){


    const auto offset=static_cast<std::streamoff>(page_number)
    *static_cast<std::streamoff>(PAGE_SIZE);

    file_.clear();

    file_.seekg(offset,std::ios::beg);

    if(!file_){
        return false;
    }
     file_.read(
        reinterpret_cast<char*>(page.data.data()),
        static_cast<std::streamsize>(PAGE_SIZE)
    );

    return file_.gcount()==static_cast<std::streamsize>(PAGE_SIZE);
}



bool qdb::pager::write_page(std::size_t page_number, const Page& page){
   const auto offset=static_cast<std::streamoff>(page_number)
    *static_cast<std::streamoff>(PAGE_SIZE);

    file_.clear();

    file_.seekg(offset,std::ios::beg);

    if(!file_){
        return false;
    }
     file_.write(
        reinterpret_cast<const char*>(page.data.data()),
        static_cast<std::streamsize>(PAGE_SIZE)
    );

      if (!file_) {
        return false;
    }

    file_.flush();
    return static_cast<bool>(file_);
}


bool qdb::pager::allocate_page(qdb::PageID& out_id){
file_.clear();

const bool reuse = !free_pages.empty();
    const PageID id = reuse
        ? free_pages.back()
        : static_cast<PageID>(num_pages);

    if (id >= static_cast<PageID>(
                  std::numeric_limits<std::streamoff>::max()
              ) / PAGE_SIZE) {
        return false;
    }

    Page page{};

    if (!write_page(static_cast<std::size_t>(id), page)) {
        return false;
    }

    if (reuse) {
        free_pages.pop_back();
    } else {
        ++num_pages;
    }

    out_id = id;
    return true;

}


std::size_t qdb::pager::page_count()const{
    return num_pages;
}


bool qdb::pager::deallocate_page(PageID id) {
    // Page 0 is reserved for database metadata.
    if (id == 0) {
        return false;
    }

    if (id >= num_pages) {
        return false;
    }

    if (std::find(free_pages.begin(), free_pages.end(), id)
        != free_pages.end()) {
        return false;
    }

    free_pages.push_back(id);
    return true;
}