#pragma once



#include <cstdint>
#include <storage/pager.hpp>
namespace qdb {
	struct RecordID {
		PageID page_id;
		std::uint16_t slot_index;

		bool operator==(const RecordID&) const = default;
	};
}