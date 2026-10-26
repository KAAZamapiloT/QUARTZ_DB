

#include<limits>
#include<bit>
#include <stdexcept>
#include <utility>

#include <storage/record.hpp>
namespace qdb {

	namespace {
		constexpr std::uint8_t NULL_TAG = 0;
		constexpr std::uint8_t INTEGER_TAG = 1;
		constexpr std::uint8_t TEXT_TAG = 2;

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


		void write_u64(std::vector<std::byte>& out, std::uint64_t value) {

			for (unsigned i = 0;i < 8;++i) {
				out.push_back(static_cast<std::byte>((value >> (i * 8))));
			}
		}

		std::uint64_t read_u64(const std::byte* data) {
			std::uint64_t value = 0;


			for (unsigned i = 0;i < 8;++i) {
				value |= static_cast<std::uint64_t>(std::to_integer<unsigned char>(data[i])) << (i * 8);
			}
			return value;
		}

	}


	std::vector<std::byte> RecordCodec::serialize(const Record&record) {

		if (record.values.size() > std::numeric_limits<std::uint32_t>::max()) {
			throw std::length_error("Record name is too large");
		}

		std::vector<std::byte> out;

		write_u32(out, static_cast<std::uint32_t>(record.values.size()));

		for (const Value& value : record.values) {

			if (std::holds_alternative<std::monostate>(value)){
				out.push_back(static_cast<std::byte>(NULL_TAG));
			}
			else if (const auto* integer =
				std::get_if<std::int64_t>(&value)) {
				out.push_back(static_cast<std::byte>(INTEGER_TAG));

				write_u64(out, std::bit_cast<std::uint64_t>(*integer));
			}
			else if (const auto* text =
				std::get_if<std::string>(&value)) {
				if (text->size() >
					std::numeric_limits<std::uint32_t>::max()) {
					throw std::length_error("Text field is too large");
				}
				out.push_back(static_cast<std::byte>(TEXT_TAG));

				write_u32(out, static_cast<std::uint32_t>(text->size()));

				for (unsigned char ch : *text) {
					out.push_back(static_cast<std::byte>(ch));
				}
			}
			
			
		}
		return out;
	}

	std::optional<Record> RecordCodec::deserialize(
		const std::byte* data,
		std::size_t size
	) {
		
		if (data == nullptr || size < 4) {
			return std::nullopt;
		}

		const std::uint32_t field_count = read_u32(data);
		std::size_t offset = 4;


		if (field_count > size - offset) {
			return std::nullopt;
		}

		Record record;
		record.values.reserve(field_count);

		for (std::uint32_t i = 0; i < field_count; ++i) {
			const auto tag =
				std::to_integer<std::uint8_t>(data[offset++]);

			switch (tag) {
			case NULL_TAG:
				record.values.emplace_back(std::monostate{});
				break;

			case INTEGER_TAG: {
				if (size - offset < 8) {
					return std::nullopt;
				}

				const std::uint64_t raw = read_u64(data + offset);
				offset += 8;

				record.values.emplace_back(
					std::bit_cast<std::int64_t>(raw)
				);
				break;
			}

			case TEXT_TAG: {
				if (size - offset < 4) {
					return std::nullopt;
				}

				const std::uint32_t length = read_u32(data + offset);
				offset += 4;

				if (length > size - offset) {
					return std::nullopt;
				}

				std::string text(
					reinterpret_cast<const char*>(data + offset),
					length
				);

				offset += length;
				record.values.emplace_back(std::move(text));
				break;
			}

			default:
				
				return std::nullopt;
			}
		}

		if (offset != size) {
			return std::nullopt;
		}

		return record;
	}
	

}