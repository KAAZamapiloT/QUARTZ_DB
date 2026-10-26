#pragma once
#include<table.hpp>


namespace qdb{
	class DataBase {
	public:
		virtual Table& CreateTable() = 0;
		virtual Table* findTable() = 0;

	private:
		pager pager_;

	};
	
}
