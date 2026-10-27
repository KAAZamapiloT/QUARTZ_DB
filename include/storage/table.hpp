#pragma once

#include <storage/pager.hpp>
#include <storage/record.hpp>
#include<storage/schema.hpp>

#include <vector>
namespace qdb {


	class Table {
	public:


		static Table create(pager& storage,Schema schema);

	    Table(pager& pager,PageID first_page,Schema schema);

		void insert(const Record& record);

		[[nodiscard]]
		std::vector<Record> scan();

		[[nodiscard]]
		PageID first_page() const noexcept {
			return first_page_;
		}
		[[nodiscard]]
		Schema get_schema() const noexcept{
			return schema_;
		}
	private:
		static constexpr PageID FIRST_DATA_PAGE = 1;

		pager& pager_;
		PageID first_page_;
		Schema schema_;
	};
}