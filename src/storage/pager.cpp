#pragma once
#include"page.hpp"

#include<filesystem>
#include<fstream>






explicit qdb::pager::pager(const std::filesystem::path& path){


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