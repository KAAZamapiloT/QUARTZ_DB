#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>


namespace qdb {

	struct Record {
		std::uint32_t id;
		std::string name;
	};


	class RecordCodec {

	public:
		static std::vector<std::byte> serialize(const Record& record);

		static std::optional<Record> deserialize(const std::byte* data, std::size_t size);

	private:


	};
}