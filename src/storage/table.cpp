
#include "storage/table.hpp"
#include"storage/schema.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>
namespace  qdb {

	namespace {

		constexpr std::size_t HEADER_SIZE = 24;
		constexpr std::size_t SLOT_SIZE = 4;

		constexpr std::uint32_t PAGE_MAGIC = 0x31475051u;
		constexpr std::uint16_t PAGE_VERSION = 1;

		constexpr PageID NO_PAGE =
			std::numeric_limits<PageID>::max();

		struct PageHeader {
			std::uint16_t slot_count;
			std::size_t free_start;
			std::size_t free_end;
			PageID next_page;
		};

		void write_u16(Page& page, std::size_t offset,
			std::uint16_t value) {

			for (int i = 0;i < 2;++i) {
				page.data[offset + i] = static_cast<std::byte>(value >> (i*8) & 0xFFu);
			}
		}

		std::uint16_t read_u16(const Page& page, std::size_t offset) {
			std::uint16_t value = 0;
			for (int i = 0;i < 2;++i) {
				value |= static_cast<std::uint16_t>(std::to_integer<unsigned char>(page.data[offset + i])<<(i*8));
			}
			return value;
		}

		void write_u32(Page& page, std::size_t offset,
			std::uint32_t value) {
			for (unsigned i = 0; i < 4; ++i) {
				page.data[offset + i] =
					static_cast<std::byte>((value >> (i * 8)) & 0xFFu);
			}
		}

		std::uint32_t read_u32(const Page& page, std::size_t offset) {
			std::uint32_t value = 0;
			for (unsigned i = 0; i < 4; ++i) {
				value |= static_cast<std::uint32_t>(
					std::to_integer<unsigned char>(page.data[offset + i])
					) << (i * 8);
			}

			return value;
		}

		void write_u64(Page& page, std::size_t offset, std::uint64_t value) {
			for (unsigned i = 0; i < 8; ++i) {
				page.data[offset + i] =
					static_cast<std::byte>((value >> (i * 8)) & 0xFFULL);
			}
		}

		std::uint64_t read_u64(const Page& page, std::size_t offset) {
			std::uint64_t value = 0;

			for (unsigned i = 0; i < 8; ++i) {
				value |= static_cast<std::uint64_t>(
					std::to_integer<unsigned char>(page.data[offset + i])
					) << (i * 8);
			}

			return value;
		}

		void initialize_page(Page& page) {
			page = Page{};

			write_u32(page, 0, PAGE_MAGIC);
			write_u16(page, 4, 0);
			write_u16(page, 6, static_cast<std::uint16_t>(HEADER_SIZE));
			write_u16(page, 8, static_cast<std::uint16_t>(PAGE_SIZE));
			write_u16(page, 10, PAGE_VERSION);
			write_u64(page, 12, NO_PAGE);
		}

		PageHeader load_header(const Page& page) {
			if (read_u32(page, 0) != PAGE_MAGIC) {
				throw std::runtime_error("Invalid table page magic");
			}

			if (read_u16(page, 10) != PAGE_VERSION) {
				throw std::runtime_error("Unsupported table page version");
			}

			const auto slot_count = read_u16(page, 4);
			const std::size_t free_start = read_u16(page, 6);
			const std::size_t free_end = read_u16(page, 8);

			const std::size_t expected_free_start =
				HEADER_SIZE + static_cast<std::size_t>(slot_count) * SLOT_SIZE;

			if (free_start != expected_free_start ||
				free_start > free_end ||
				free_end > PAGE_SIZE) {
				throw std::runtime_error("Corrupt table page header");
			}

			return {
				slot_count,
				free_start,
				free_end,
				read_u64(page, 12)
			};
		}

		Page read_page_or_throw(pager& p, PageID id) {
			if (id > static_cast<PageID>(
				std::numeric_limits<std::size_t>::max())) {
				throw std::runtime_error("Page ID exceeds platform limits");
			}

			Page page{};

			if (!p.read_page(static_cast<std::size_t>(id), page)) {
				throw std::runtime_error("Failed to read table page");
			}

			return page;
		}

		void write_page_or_throw(
			pager& p,
			PageID id,
			const Page& page
		) {
			if (id > static_cast<PageID>(
				std::numeric_limits<std::size_t>::max())) {
				throw std::runtime_error("Page ID exceeds platform limits");
			}

			if (!p.write_page(static_cast<std::size_t>(id), page)) {
				throw std::runtime_error("Failed to write table page");
			}
		}

		void validate_schema(const Schema& schema) {
			if (schema.columns.empty()) {
				throw std::invalid_argument(
					"Table schema must contain at least one column"
				);
			}

			std::unordered_set<std::string> column_names;
			std::size_t primary_key_count = 0;

			for (const Column& column : schema.columns) {
				if (column.name.empty()) {
					throw std::invalid_argument(
						"Column name cannot be empty"
					);
				}

				if (!column_names.insert(column.name).second) {
					throw std::invalid_argument(
						"Duplicate column name: " + column.name
					);
				}

				switch (column.type) {
				case  DATA_TYPE::INTEGER:
				case  DATA_TYPE::TEXT:
					break;

				default:
					throw std::invalid_argument(
						"Unsupported column data type"
					);
				}

				if (column.primary_key) {
					++primary_key_count;

					if (column.nullable) {
						throw std::invalid_argument(
							"Primary key column cannot be nullable"
						);
					}
				}
			}

			// This initial implementation supports a single-column primary key.
			if (primary_key_count > 1) {
				throw std::invalid_argument(
					"Composite primary keys are not supported yet"
				);
			}
		}

		void validate_record(const Schema& schema, const Record& record) {
			if (record.values.size() != schema.columns.size()) {
				throw std::invalid_argument(
					"Record field count does not match table schema"
				);
			}

			for (std::size_t i = 0; i < schema.columns.size(); ++i) {
				const Column& column = schema.columns[i];
				const Value& value = record.values[i];

				if (std::holds_alternative<std::monostate>(value)) {
					if (!column.nullable) {
						throw std::invalid_argument(
							"NULL is not allowed for column: " + column.name
						);
					}

					continue;
				}

				switch (column.type) {
					case DATA_TYPE::INTEGER:
					if (!std::holds_alternative<std::int64_t>(value)) {
						throw std::invalid_argument(
							"Expected INTEGER for column: " + column.name
						);
					}
					break;

				case DATA_TYPE::TEXT:
					if (!std::holds_alternative<std::string>(value)) {
						throw std::invalid_argument(
							"Expected TEXT for column: " + column.name
						);
					}
					break;

				default:
					throw std::invalid_argument(
						"Unsupported column data type"
					);
				}
			}
		}
	}






	Table Table::create(pager& storage,Schema schema) {
		PageID id{};

		validate_schema(schema);

		if (!storage.allocate_page(id)) {
			throw std::runtime_error("Failed to allocate table's first page");
		}

		if (id == 0 ||
			id == std::numeric_limits<PageID>::max()) {
			throw std::runtime_error("Allocator returned an invalid page ID");
		}

		Page page{};
		initialize_page(page);
		write_page_or_throw(storage, id, page);

		return Table(storage, id,schema);
	}

	Table::Table(pager& storage, PageID first_page,Schema schema)
		: pager_(storage), first_page_(first_page),schema_(std::move(schema)) {


		validate_schema(schema_);
		if (first_page_ == 0 ||
			first_page_ == std::numeric_limits<PageID>::max()) {
			throw std::invalid_argument("Invalid table first page ID");
		}

		Page page = read_page_or_throw(pager_, first_page_);
		(void)load_header(page);
	}

	void Table::insert(const Record& record)
	{

		validate_record(schema_, record);
		const std::vector<std::byte> bytes = RecordCodec::serialize(record);
		
		if (bytes.size() > PAGE_SIZE - HEADER_SIZE - SLOT_SIZE) {
			throw std::length_error(
				"Record is too large to fit in a table page"
			);
		}

		PageID current_id = first_page_;

		std::unordered_set<PageID> visited;

		while (true) {

			if (!visited.insert(current_id).second) {
				throw std::runtime_error("Cycle in table page chain");
			}

			Page page = read_page_or_throw(pager_, current_id);
			PageHeader header = load_header(page);

			const std::size_t needed = bytes.size() + SLOT_SIZE;
			const std::size_t avalible = header.free_end - header.free_start;

			if (avalible >= needed) {
				const std::size_t record_offset = header.free_end - bytes.size();

				const std::size_t slot_offset = header.free_start;

				std::copy(bytes.begin(), bytes.end(),
					page.data.begin() + static_cast<std::ptrdiff_t>(record_offset));


				write_u16(
					page, slot_offset,
					static_cast<std::uint16_t>(record_offset)
				);

				write_u16(
					page, slot_offset + 2,
					static_cast<std::uint16_t>(bytes.size())
				);

				write_u16(
					page, 4,
					static_cast<std::uint16_t>(header.slot_count + 1)
				);

				write_u16(
					page, 6,
					static_cast<std::uint16_t>(slot_offset + SLOT_SIZE)
				);

				write_u16(
					page, 8,
					static_cast<std::uint16_t>(record_offset)
				);

				write_page_or_throw(pager_, current_id, page);
				return;
			   }

			if (header.next_page != NO_PAGE) {
				current_id = header.next_page;
				continue;
			}

			
			PageID new_id{};

			if (!pager_.allocate_page(new_id)) {
				throw std::runtime_error("Failed to allocate table page");
			}

			if (new_id == 0 || visited.contains(new_id)) {
				throw std::runtime_error(
					"Allocator returned an invalid or already-linked page"
				);
			}

			Page new_page{};
			initialize_page(new_page);
			write_page_or_throw(pager_, new_id, new_page);

		
			write_u64(page, 12, new_id);
			write_page_or_throw(pager_, current_id, page);

			current_id = new_id;
		}


	}


	std::vector<Record> Table::scan()
	{

		std::vector<Record> records;

		PageID current_id = first_page_;

		std::unordered_set<PageID> visited;

		while (current_id!=NO_PAGE)
		{
			if (!visited.insert(current_id).second) {
				throw std::runtime_error("Cycle in table page chain");
			}
			const Page page = read_page_or_throw(pager_, current_id);
			const PageHeader header = load_header(page);

			for (uint16_t i = 0;i < header.slot_count;++i) {
				const std::size_t slot_offset = HEADER_SIZE + static_cast<std::size_t>(i) * SLOT_SIZE;

				const std::size_t record_offset = read_u16(page, slot_offset);

				const std::size_t record_length = read_u16(page, slot_offset + 2);

				if (record_length == 0 ||
					record_offset < header.free_end ||
					record_offset > PAGE_SIZE ||
					record_length > PAGE_SIZE - record_offset) {
					throw std::runtime_error("Corrupt record slot");
				}

				const auto decoded = RecordCodec::deserialize(page.data.data()+record_offset, record_length);

				if (!decoded) {
					throw std::runtime_error(
						"Failed to decode record from table page"
					);
				}
				validate_record(schema_, *decoded);
				records.push_back(*decoded);
			}
			current_id = header.next_page;

		}
		
		return records;
	}
}