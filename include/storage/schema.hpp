#pragma once
#include<vector>
#include<variant>
#include<string>

namespace qdb {
	enum class DATA_TYPE :uint8_t {
		INTEGER = 1,
		TEXT=2
		
	};
	

	struct Column {
		std::string name;
		DATA_TYPE type;
		bool nullable = false;
		bool primary_key = false;
	};

	struct Schema {
		std::vector<Column> columns;
	};
}
