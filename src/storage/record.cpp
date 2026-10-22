

#include"limits"
#include <stdexcept>
#include <utility>

#include <storage/record.hpp>
namespace qdb {

	namespace {
		
		void write_u32(std::vector<std::byte>& out, std::uint32_t value) {

			for (unsigned i = 0;i < 4;++i) {
				out.push_back(static_cast<std::byte>((value >> (i * 8))));
			}
		}

		std::uint32_t read_u32(const std::byte*data) {
			std::uint32_t value = 0;


			for (unsigned i = 0;i < 4;++i) {
				value|=static_cast<std::uint32_t>(std::to_integer<unsigned char>(data[i]))<<(i*8);
			}
			return value;
		}
	}


	std::vector<std::byte> RecordCodec::serialize(const Record&record) {

		if (record.name.size() > std::numeric_limits<std::uint32_t>::max()) {
			throw std::length_error("Record name is too large");
		}

		std::vector<std::byte> out;

		write_u32(out, static_cast<std::uint32_t>(record.id));

		write_u32(out, static_cast<std::uint32_t>(record.name.size()));

		for (unsigned char ch : record.name) {
			out.push_back(static_cast<std::byte>(ch));
		}
		return out;
	}

	std::optional<Record> RecordCodec::deserialize(
		const std::byte* data,
		std::size_t size
	) {
		
		Record out;

		if (data == nullptr || size <= 0) {
			return std::nullopt;
		}

		const std::uint32_t raw_id = read_u32(data);
		const std::uint32_t name_len = read_u32(data + 4);
		
		if (name_len > size - 8) {
			return std::nullopt;
		}
		const std::uint32_t id = static_cast<std::uint32_t>(raw_id);

		std::string rec;
		rec.reserve(name_len);

		for (unsigned i = 0;i < name_len;++i) {
			rec.push_back(static_cast<char>(std::to_integer<unsigned char>(data[8+i])));
		}

		return Record({ id,std::move(rec)});
	}
	

}