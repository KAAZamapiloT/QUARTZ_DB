
#include <storage/table.hpp>
#include <storage/schema.hpp>
#include <index/primary_index.hpp>

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


		class StorageFailure final : public std::runtime_error {
		public:
			using std::runtime_error::runtime_error;
		};

		class CorruptionFailure final : public std::runtime_error {
		public:
			using std::runtime_error::runtime_error;
		};


		struct PageHeader {
			std::uint16_t slot_count;
			std::size_t free_start;
			std::size_t free_end;
			PageID next_page;
			std::uint16_t fragmented_bytes;
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
			write_u16(page, 20, 0);
		}

		PageHeader load_header(const Page& page) {
			if (read_u32(page, 0) != PAGE_MAGIC) {
				throw CorruptionFailure("Invalid table page magic");
			}

			if (read_u16(page, 10) != PAGE_VERSION) {
				throw CorruptionFailure("Unsupported table page version");
			}

			const auto slot_count = read_u16(page, 4);
			const std::size_t free_start = read_u16(page, 6);
			const std::size_t free_end = read_u16(page, 8);

			const std::size_t expected_free_start =
				HEADER_SIZE + static_cast<std::size_t>(slot_count) * SLOT_SIZE;
			const std::uint16_t fragmented_bytes = read_u16(page, 20);
			if (free_start != expected_free_start ||
				free_start > free_end ||
				free_end > PAGE_SIZE) {
				throw CorruptionFailure("Corrupt table page header");
			}

			return {
				slot_count,
				free_start,
				free_end,
				read_u64(page, 12),
				fragmented_bytes
			};
		}

		void compact_page(Page& page,PageHeader& header) {

			Page rebuilt{};

			initialize_page(rebuilt);

			write_u16(rebuilt, 4, header.slot_count);
			write_u16(rebuilt, 6, static_cast<std::uint16_t>(header.free_start));
			write_u64(rebuilt, 12, header.next_page);

			std::size_t new_free_end = PAGE_SIZE;

			for (std::size_t i = 0;i < header.slot_count;++i) {
				const std::size_t slot_offset = HEADER_SIZE + static_cast<std::size_t>(i) * SLOT_SIZE;
				const std::uint16_t record_offset = read_u16(page, slot_offset);

				const std::size_t record_length = read_u16(page, slot_offset + 2);

				if (record_length == 0) {
					write_u16(rebuilt, slot_offset, 0);
					write_u16(rebuilt, slot_offset + 2, 0);
					continue;
				}
				if (record_offset < header.free_end ||
					static_cast<std::size_t>(record_offset) + record_length > PAGE_SIZE) {
					throw CorruptionFailure("Corrupt record slot during compaction");
				}

				new_free_end -= record_length;

				std::copy_n(page.data.data() + record_offset, record_length, rebuilt.data.data() + new_free_end);
				write_u16(rebuilt, slot_offset, static_cast<std::uint16_t>(new_free_end));
				write_u16(rebuilt, slot_offset + 2, record_length);
			}

			write_u16(rebuilt, 8, static_cast<std::uint16_t>(new_free_end));
			write_u16(rebuilt, 20, 0); 

			page = rebuilt;
			header.free_end = new_free_end;
			header.fragmented_bytes = 0;
		}
		}

		Page read_page_or_throw(pager& p, PageID id) {
			if (id > static_cast<PageID>(
				std::numeric_limits<std::size_t>::max())) {
				throw StorageFailure("Page ID exceeds platform limits");
			}

			Page page{};

			if (!p.read_page(static_cast<std::size_t>(id), page)) {
				throw StorageFailure("Failed to read table page");
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
				throw StorageFailure("Page ID exceeds platform limits");
			}

			if (!p.write_page(static_cast<std::size_t>(id), page)) {
				throw StorageFailure("Failed to write table page");
			}
		}

		Result<std::optional<std::size_t>> validate_schema(const Schema& schema) {
			if (schema.columns.empty()) {
				return std::unexpected(Status{
					StatusCode::InvalidSchema,
					"Table schema must contain at least one column"
					});
			}

			std::unordered_set<std::string> column_names;
			std::optional<std::size_t> primary_key_column;

			for (std::size_t i = 0; i < schema.columns.size(); ++i) {
				const Column& column = schema.columns[i];

				if (column.name.empty()) {
					return std::unexpected(Status{
						StatusCode::InvalidSchema,
						"Column name cannot be empty"
						});
				}

				if (!column_names.insert(column.name).second) {
					return std::unexpected(Status{
						StatusCode::InvalidSchema,
						"Duplicate column name: " + column.name
						});
				}

				if (column.type != DataType::Integer && column.type != DataType::Text) {
					return std::unexpected(Status{
						StatusCode::InvalidSchema,
						"Unsupported column data type: " + column.name
						});
				}

				if (column.primary_key) {
					if (primary_key_column.has_value()) {
						return std::unexpected(Status{
							StatusCode::InvalidSchema,
							"Composite primary keys are not supported"
							});
					}

					if (column.nullable) {
						return std::unexpected(Status{
							StatusCode::InvalidSchema,
							"Primary key column cannot be nullable: " + column.name
							});
					}

					if (column.type != DataType::Integer) {
						return std::unexpected(Status{
							StatusCode::InvalidSchema,
							"Primary key must have INTEGER type: " + column.name
							});
					}

					primary_key_column = i;
				}
			}

			return primary_key_column;
		}

		Result<void> validate_record(const Schema& schema, const Record& record) {
			if (record.values.size() != schema.columns.size()) {
				return std::unexpected(Status{
					StatusCode::InvalidRecord,
					"Record field count does not match table schema"
					});
			}

			for (std::size_t i = 0; i < schema.columns.size(); ++i) {
				const Column& column = schema.columns[i];
				const Value& value = record.values[i];

				if (std::holds_alternative<std::monostate>(value)) {
					if (!column.nullable) {
						return std::unexpected(Status{
							StatusCode::NullConstraintViolation,
							"NULL is not allowed for column: " + column.name
							});
					}
					continue;
				}

				switch (column.type) {
				case DataType::Integer:
					if (!std::holds_alternative<std::int64_t>(value)) {
						return std::unexpected(Status{
							StatusCode::TypeMismatch,
							"Expected INTEGER for column: " + column.name
							});
					}
					break;

				case DataType::Text:
					if (!std::holds_alternative<std::string>(value)) {
						return std::unexpected(Status{
							StatusCode::TypeMismatch,
							"Expected TEXT for column: " + column.name
							});
					}
					break;
				}
			}

			return {};
		}

		Result<void> validate_record_storage(const Schema& schema, const Record& record) {
			auto result = validate_record(schema, record);
			if (!result) {
				return std::unexpected(Status{
					StatusCode::Corruption,
					"Stored record violates its schema: " + std::string(result.error().message())
					});
			}
			return {};
		}
	}



	Result<RecordID> Table::update_by_primary_key(std::int64_t key, const Record& updated_record)
	{
		auto find_result = primary_index_->find(key);

		if (!find_result) {
			return std::unexpected(find_result.error());
		}
		if (!find_result->has_value())
			return std::unexpected(Status{
				StatusCode::RecordNotFound,
				"Record not found"
				});

		RecordID ID = **find_result;

		auto validation = validate_record(schema_, updated_record);

		if (!validation) {
			return std::unexpected(validation.error());
		}

		auto update_result = update_record(ID, updated_record);

		if (!update_result){
			return std::unexpected(update_result.error());
     	}
	   
		return ID;

	}

	Result<RecordID> Table::update_record(
		RecordID id,
		const Record& updated_record)
	{
		auto result = validate_record(schema_, updated_record);

		if (!result) {
			return std::unexpected(result.error());
		}

		std::vector<std::byte> updated_bytes;
		try {
			updated_bytes = RecordCodec::serialize(updated_record);
		}
		catch (const std::length_error& error) {
			return std::unexpected(Status{
				StatusCode::RecordTooLarge,
				error.what()
				});
		}

		if (id.page_id == 0 || id.page_id >= pager_.page_count()) {
			return std::unexpected(Status{
				StatusCode::Corruption,
				"RecordID references an invalid page"
				});
		}

		Page page{};
	      
		if (!pager_.read_page(static_cast<std::size_t>(id.page_id), page)) {
			return std::unexpected(Status{
				StatusCode::StorageError,
				"Failed to read table page"
				});
		}

		PageHeader header;

		try {
			header = load_header(page);
		}
		catch (const std::invalid_argument& error) {
			return std::unexpected(Status{
				StatusCode::Corruption,
				error.what()
				});
		}

		if (id.slot_index >= header.slot_count) {
			return std::unexpected(Status{
				StatusCode::Corruption,
				"RecordID references an invalid slot"
				});
		}

		std::vector< std::vector<std::byte>> existing_bytes;
		existing_bytes.reserve(header.slot_count);

		for (std::size_t i = 0;i < header.slot_count;++i) {
			const std::size_t slot_offset = HEADER_SIZE + static_cast<std::size_t>(i) * SLOT_SIZE;

			const std::size_t record_offset = read_u16(page, slot_offset);

			const std::size_t record_length = read_u16(page, slot_offset + 2);

			if (record_length == 0 || record_offset<header.free_end || record_offset > PAGE_SIZE || 
				record_length > PAGE_SIZE - record_offset) {
				return std::unexpected(Status{
					StatusCode::Corruption,
					"Corrupt record slot"
					});
			}
			
			if (i == id.slot_index && primary_key_column_) {

				auto existing = RecordCodec::deserialize(
					page.data.data() + record_offset,
					record_length
				);

				if (!existing) {
					return std::unexpected(Status{
						StatusCode::Corruption,
						"Failed to decode existing record"
						});
				}

				const auto* old_key =
					std::get_if<std::int64_t>(
						&existing->values[*primary_key_column_]
					);

				const auto* new_key =
					std::get_if<std::int64_t>(
						&updated_record.values[*primary_key_column_]
					);

				if (!old_key || !new_key || *old_key != *new_key) {
					return std::unexpected(Status{
						StatusCode::InvalidRecord,
						"Changing primary keys through update is not supported"
						});
				}
			}
			if(i==id.slot_index) {
				existing_bytes.push_back(updated_bytes);
			}
			else {
				existing_bytes.emplace_back(page.data.data() + record_offset, page.data.data() + record_offset + record_length);
			}
		}

		std::size_t new_free_start = HEADER_SIZE + header.slot_count * SLOT_SIZE;

		std::size_t total_record_bytes = 0;

		for (const auto& bytes : existing_bytes) {
			total_record_bytes += bytes.size();
		}

		if (total_record_bytes > PAGE_SIZE - new_free_start) {
			return std::unexpected(Status{
				StatusCode::RecordTooLarge,
				"Updated record data exceeds page capacity"
				});
		}

		Page rebuilt_page{};

		initialize_page(rebuilt_page);

		write_u16(rebuilt_page, 4, static_cast<std::uint16_t>(header.slot_count));

		write_u16(rebuilt_page, 6, static_cast<std::uint16_t>(new_free_start));

		write_u64(rebuilt_page, 12, header.next_page);

		std::size_t free_end = PAGE_SIZE;

		for (std::size_t i = 0; i < existing_bytes.size(); ++i) {
			const auto& bytes = existing_bytes[i];
			
			free_end -= bytes.size();
			
			std::copy(bytes.begin(), bytes.end(), rebuilt_page.data.begin() + static_cast<std::ptrdiff_t>(free_end));

			const std::size_t slot_offset = HEADER_SIZE + i * SLOT_SIZE;

			write_u16(rebuilt_page, slot_offset, static_cast<std::uint16_t>(free_end));

			write_u16(rebuilt_page, slot_offset + 2, static_cast<std::uint16_t>(bytes.size()));

			write_u16(rebuilt_page, 6, static_cast<std::uint16_t>(new_free_start));

			if (!pager_.write_page(static_cast<std::size_t>(id.page_id),rebuilt_page)) {
				return std::unexpected(Status{
		  StatusCode::StorageError,
		  "Failed to persist updated record"
					});
			}
		}


		return id;
	}

	

	Result<bool> Table::delete_by_primary_key(std::int64_t key)
	{

		if (!primary_index_ || !primary_key_column_) {
			return std::unexpected(Status{
			StatusCode::NoPrimaryKey,
			"Table does not have a primary-key index"
				});
		}
		auto result = primary_index_->find(key);

		if (!result) {
			return std::unexpected(result.error());
		}

		if (!result->has_value()) {
			return std::unexpected(Status{
				StatusCode::RecordNotFound,
				"Record not found"
				});
		}

		const RecordID id = **result;

		auto delete_result = delete_record(id);

		if (!delete_result) {
			return std::unexpected(delete_result.error());
		}

		if (!*delete_result) {
			return std::unexpected(Status{
				StatusCode::Corruption,
				"Primary index references an already deleted record"
				});
		}

		auto erase_result = primary_index_->erase(key);

		if (!erase_result) {
			return std::unexpected(erase_result.error());
		}


		return true;


	}

	Result<bool> Table::delete_record(RecordID id)
	{
		Page page{};

		auto read_result = pager_.read_page(static_cast<std::size_t>(id.page_id), page);

		if (!read_result) {
			return std::unexpected(Status{
				StatusCode::StorageError,
				"Failed to read table page"
				});
		}

		auto header = load_header(page);

		if (id.slot_index >= header.slot_count)	 {
			return std::unexpected(Status{
				StatusCode::Corruption,
				"RecordID references an invalid slot"
				});
		}
		const auto slot_offset = HEADER_SIZE + static_cast<std::size_t>(id.slot_index) * SLOT_SIZE;

		const auto record_offset = read_u16(page, slot_offset);
		const auto record_length = read_u16(page, slot_offset + 2);

		if (record_length == 0) {
			return false;
		}
		if (record_offset < header.free_end ||
			static_cast<std::size_t>(record_offset) + record_length > PAGE_SIZE) {
			return std::unexpected(Status{
				StatusCode::Corruption,
				"Record slot contains invalid boundaries"
				});
		}
		write_u16(page, slot_offset + 2, 0);

		auto write_result = pager_.write_page(
			static_cast<std::size_t>(id.page_id), page
		);

		if (!write_result) {
			return std::unexpected(Status{
				StatusCode::StorageError,
				"Failed to write table page"
				});
		}

		return true;
	}

	
	Table::Table(
		pager& storage,
		PageID first_page,
		Schema schema,
		std::unique_ptr<PrimaryIndex>&& primary_index,
		std::optional<std::size_t> primary_key_column,
		AdoptIndexTag
	) noexcept
		: pager_(storage),
		first_page_(first_page),
		schema_(std::move(schema)),
		primary_index_(std::move(primary_index)),
		primary_key_column_(primary_key_column) {



	} 

	Table Table::create(pager& storage,Schema schema) {
		PageID id{};

		auto schema_result = validate_schema(schema);

		if (!schema_result) {
			throw std::invalid_argument(std::string(schema_result.error().message()));
		}

		if (!storage.allocate_page(id)) {
			throw StorageFailure("Failed to allocate table's first page");
		}

		if (id == 0 ||
			id == std::numeric_limits<PageID>::max()) {
			throw CorruptionFailure("Allocator returned an invalid page ID");
		}

		Page page{};
		initialize_page(page);
		write_page_or_throw(storage, id, page);

		std::unique_ptr<PrimaryIndex> primary_index;
		if (schema_result->has_value()) {
			auto index_result = PrimaryIndex::create(storage);
			if (!index_result) {
				throw StorageFailure(
					"Failed to create table primary index: " +
					std::string(index_result.error().message())
				);
			}
			primary_index = std::move(*index_result);
		}
		return Table(storage, id,std::move(schema),std::move(primary_index),*schema_result,AdoptIndexTag{});
	}

	Table::Table(pager& storage, PageID first_page, Schema schema, std::optional<PageID> primary_index_metadata_page)
		: pager_(storage), first_page_(first_page), schema_(std::move(schema))
	{
		auto schema_result = validate_schema(schema_);

		if (!schema_result) {
			throw std::invalid_argument(std::string(schema_result.error().message()));
		}

		primary_key_column_ = *schema_result;

		if (first_page_ == 0 || first_page_ == NO_PAGE) {
			throw std::invalid_argument("Invalid table first page ID");
		}

		const Page page = read_page_or_throw(pager_, first_page_);
		(void)load_header(page);

		if (primary_key_column_ && primary_index_metadata_page) {
			auto index_result = PrimaryIndex::open(pager_, *primary_index_metadata_page);

			if (!index_result) {
				throw StorageFailure(
					"Failed to open table primary index: " +
					std::string(index_result.error().message())
				);
			}
			primary_index_ = std::move(*index_result);
		}
		else if (!primary_key_column_ && primary_index_metadata_page) {
			throw std::invalid_argument(
				"Catalog contains primary-index metadata for a table without a primary key"
			);
		}
	}

	Result<RecordID> Table::insert(const Record& record)
	{

		auto validation = validate_record(schema_, record);

		if (!validation) {
			return std::unexpected(validation.error());
		}

		std::optional<std::int64_t> primary_key;

		if (primary_key_column_) {

			const auto* key = std::get_if<std::int64_t>(&record.values[*primary_key_column_]);

			if (key == nullptr) {
				return std::unexpected(Status{
					StatusCode::InvalidRecord,
					"Primary-key value is not an INTEGER"
					});
			}

			primary_key = *key;

			if (primary_index_) {
				auto existing = primary_index_->find(*primary_key);

			if (!existing) {
				return std::unexpected(existing.error());
			}

			if (existing->has_value()) {
				return std::unexpected(Status{ StatusCode::DuplicatePrimaryKey,
				"Primary key already exists" });
			}
			}

		}
		auto record_id_result = insert_record_data(record);

		if (!record_id_result) {
			return std::unexpected(record_id_result.error());
		}
		if (primary_key && primary_index_) {
			auto index_insert_result = primary_index_->insert(*primary_key, *record_id_result);

			if (!index_insert_result) {
				return std::unexpected(index_insert_result.error());
			}
		}

		return *record_id_result;
	}

	Result<std::optional<Record>> Table::find_by_primary_key(std::int64_t key)
	{
		if (!primary_index_ || !primary_key_column_) {
			return std::unexpected(Status{
		   StatusCode::NoPrimaryKey,
		   "Table does not have a primary-key index"
				});
		}

		auto index_result = primary_index_->find(key);

		if (!index_result) {
			return std::unexpected(index_result.error());
		}
		if (!index_result->has_value()) {
			return std::optional<Record>{};
		}

		auto record_result = fetch_record_data(**index_result);

		if (!record_result) {
			return std::unexpected(record_result.error());
		}

		const auto* stored_key = std::get_if<std::int64_t>(&record_result->values[*primary_key_column_]);

		if (stored_key == nullptr || *stored_key != key) {
			return std::unexpected(Status{
		   StatusCode::Corruption,
		   "Primary index entry does not match the referenced record"
				});
		}

		return std::optional<Record>{std::move(*record_result)};
	}


	std::vector<Record> Table::scan()
	{

		std::vector<Record> records;

		PageID current_id = first_page_;

		std::unordered_set<PageID> visited;

		while (current_id!=NO_PAGE)
		{
			if (!visited.insert(current_id).second) {
				throw CorruptionFailure("Cycle in table page chain");
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
					throw CorruptionFailure("Corrupt record slot");
				}

				const auto decoded = RecordCodec::deserialize(page.data.data()+record_offset, record_length);

				if (!decoded) {
					throw CorruptionFailure(
						"Failed to decode record from table page"
					);
				}
				auto record_validation = validate_record_storage(schema_, *decoded);
				if (!record_validation) {
					throw CorruptionFailure(
						std::string(record_validation.error().message())
					);
				}
				records.push_back(*decoded);
			}
			current_id = header.next_page;

		}
		
		return records;
	}


	Result<RecordID> qdb::Table::insert_record_data(const Record& record)
	{
		std::vector<std::byte> bytes;

		try {
			bytes = RecordCodec::serialize(record);
		}
		catch (const std::length_error& error) {
			return std::unexpected(Status{StatusCode::RecordTooLarge,error.what()});
		}
		if (bytes.size() > PAGE_SIZE - HEADER_SIZE - SLOT_SIZE) {
			return std::unexpected(Status{
		   StatusCode::RecordTooLarge,
		   "Record is too large to fit in a table page"
				});
		}

		try {
			PageID current_page = first_page_;
			std::unordered_set<PageID> visited;

			while (true) {
				if (!visited.insert(current_page).second) {
					throw CorruptionFailure("Cycle in table page chain");
				}

				Page page = read_page_or_throw(pager_, current_page);

				const PageHeader header = load_header(page);

				const std::size_t needed = bytes.size() + SLOT_SIZE;
				const std::size_t avalible = header.free_end - header.free_start;

				if (avalible >= needed) {
					if (header.slot_count == std::numeric_limits<std::uint16_t>::max()) {
						throw CorruptionFailure("Table page slot count overflow");
					}

					const std::size_t record_offset = header.free_end - bytes.size();
					const std::size_t slot_offset = header.free_start;
					const std::uint16_t slot_index = header.slot_count;

					std::copy(bytes.begin(), bytes.end(), page.data.begin() + static_cast<std::ptrdiff_t>(record_offset));

					write_u16(page, slot_offset, static_cast<std::uint16_t>(record_offset));

					write_u16(page, slot_offset + 2, static_cast<std::uint16_t>(bytes.size()));

					write_u16(page, 4, static_cast<std::uint16_t>(header.slot_count + 1));

					write_u16(page, 6, static_cast<std::uint16_t>(slot_offset + SLOT_SIZE));

					write_u16(page, 8, static_cast<std::uint16_t>(record_offset));

					write_page_or_throw(pager_, current_page, page);

					return RecordID{ current_page, slot_index };
				}
				if (header.next_page != NO_PAGE) {
					current_page = header.next_page;
					continue;
				}

				PageID newID{};

				if (!pager_.allocate_page(newID)) {
					throw StorageFailure("Failed to allocate table page");
				}

				if (newID == 0 || newID == NO_PAGE || visited.contains(newID)) {
					throw CorruptionFailure(
						"Allocator returned an invalid or already-linked page"
					);
				}
				Page newPage{};

				initialize_page(newPage);

				write_page_or_throw(pager_, newID, newPage);

				write_u64(page, 12, newID);
				write_page_or_throw(pager_, current_page, page);

				current_page = newID;
			}
		}
		catch (const StorageFailure& error) {

			return std::unexpected(Status{
				StatusCode::StorageError,
				error.what()
				});

		}
		catch (const CorruptionFailure& error) {
			return std::unexpected(Status{
				StatusCode::Corruption,
				error.what()
				});
		}
		
	}
	Result<Record> qdb::Table::fetch_record_data(RecordID id)
	{
		try {
			if (id.page_id == 0 || id.page_id >= pager_.page_count()) {
				throw CorruptionFailure("RecordID references an invalid page");
			}

			const Page page = read_page_or_throw(pager_,id.page_id);
			const PageHeader header = load_header(page);

			if (id.slot_index >= header.slot_count) {
				throw CorruptionFailure("RecordID references an invalid slot");
			}

			const std::size_t slot_offset = HEADER_SIZE + static_cast<std::size_t>(id.slot_index) * SLOT_SIZE;

			const std::size_t record_offset = read_u16(page, slot_offset);
			const std::size_t record_length = read_u16(page, slot_offset + 2);

			if (record_length == 0 || record_offset<header.free_end || record_offset>PAGE_SIZE 
				|| record_length > PAGE_SIZE - record_offset) {
				throw CorruptionFailure("Corrupt record slot");
			}

			auto decoded = RecordCodec::deserialize(page.data.data() + record_offset, record_length);

			if (!decoded) {
				throw CorruptionFailure("Failed to decode record from table page");
			}

			auto record_validation = validate_record_storage(schema_, *decoded);
           
			if (!record_validation) {
				throw CorruptionFailure(
					std::string(record_validation.error().message())
				);
			}
			return *decoded;

		}
		catch (const StorageFailure& error) {
			return std::unexpected(Status{
				StatusCode::StorageError,
				error.what()
				});
		}
		catch (const CorruptionFailure& error) {
			return std::unexpected(Status{
				StatusCode::Corruption,
				error.what()
				});
		}
	}
}