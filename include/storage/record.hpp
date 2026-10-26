#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <variant>
#include <string>
#include <vector>

namespace qdb {

	// values supported 
	using Value = std::variant<std::monostate,
		std::int64_t,
		std::string>;
	// a row of values
	struct Record {
		std::vector<Value> values;
	};


	class RecordCodec {

	public:
	
		[[nodiscard]]  static std::vector<std::byte> serialize(const Record& record);
		
		[[nodiscard]]  static std::optional<Record> deserialize(const std::byte* data, std::size_t size);


	};
}