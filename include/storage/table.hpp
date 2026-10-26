#pragma once

#include <storage/pager.hpp>
#include <storage/record.hpp>

#include <vector>
namespace qdb {


	class Table {
	public:
		explicit Table(pager& pager);

		void insert(const Record& record);

		[[nodiscard]]
		std::vector<Record> scan();

	private:
		static constexpr PageID FIRST_DATA_PAGE = 1;

		pager& pager_;
	};
}