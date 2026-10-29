#include<index/bplus_tree.hpp>




namespace qdb {


	namespace {
		constexpr std::uint32_t NODE_MAGIC = 0x31545042u; // bytes: "BPT1"
		constexpr std::uint32_t META_MAGIC = 0x314D5042u; // bytes: "BPM1"
		constexpr std::uint16_t FORMAT_VERSION = 1;
		constexpr std::uint8_t LEAF_NODE = 1;
		constexpr std::uint8_t INTERNAL_NODE = 2;
		constexpr std::size_t NODE_HEADER_SIZE = 24;
		constexpr std::size_t META_HEADER_SIZE = 16;
		constexpr PageID NO_PAGE = std::numeric_limits<PageID>::max();

	void check_page_id(pager & storage, PageID id, const char* what) 
	{
				if (id == 0 || id == NO_PAGE ||
					id >= static_cast<PageID>(storage.page_count())) {
					throw std::runtime_error(what);
				}
	 }

	std::size_t page_index(PageID id)
	{
		if (id > std::numeric_limits<std::size_t>::max()) {
			throw std::runtime_error("Page ID exceeds platform limits");
		}

		return static_cast<std::size_t>(id);
	}

    void write_u16(Page& page, std::size_t offset, std::uint16_t value) {
        for (unsigned i = 0; i < 2; ++i) {
            page.data[offset + i] = static_cast<std::byte>(
                (value >> (i * 8)) & 0xFFu
                );
        }
    }

    std::uint16_t read_u16(const Page& page, std::size_t offset) {
        std::uint16_t value = 0;
        for (unsigned i = 0; i < 2; ++i) {
            value |= static_cast<std::uint16_t>(
                static_cast<std::uint16_t>(
                    std::to_integer<unsigned char>(page.data[offset + i])
                    ) << (i * 8)
                );
        }
        return value;
    }

    void write_u32(Page& page, std::size_t offset, std::uint32_t value) {
        for (unsigned i = 0; i < 4; ++i) {
            page.data[offset + i] = static_cast<std::byte>(
                (value >> (i * 8)) & 0xFFu
                );
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
            page.data[offset + i] = static_cast<std::byte>(
                (value >> (i * 8)) & 0xFFULL
                );
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

    void write_metadata_page(
        pager& storage,
        PageID metadata_page,
        PageID root_page
    ) {
        Page page{};
        write_u32(page, 0, META_MAGIC);
        write_u16(page, 4, FORMAT_VERSION);
        write_u16(page, 6, 0); // Reserved flags.
        write_u64(page, 8, root_page);
        write_u32(page, 16, static_cast<std::uint32_t>(PAGE_SIZE));

        if (!storage.write_page(page_index(metadata_page), page)) {
            throw std::runtime_error("Failed to write B+ tree metadata page");
        }
    }

    PageID read_metadata_page(pager& storage, PageID metadata_page) {
        check_page_id(storage, metadata_page,
            "B+ tree metadata page ID is invalid");

        Page page{};
        if (!storage.read_page(page_index(metadata_page), page)) {
            throw std::runtime_error("Failed to read B+ tree metadata page");
        }

        if (read_u32(page, 0) != META_MAGIC) {
            throw std::runtime_error("Invalid B+ tree metadata magic");
        }
        if (read_u16(page, 4) != FORMAT_VERSION) {
            throw std::runtime_error("Unsupported B+ tree format version");
        }
        if (read_u32(page, 16) != PAGE_SIZE) {
            throw std::runtime_error("B+ tree page-size mismatch");
        }

        const PageID root_page = read_u64(page, 8);
        check_page_id(storage, root_page, "B+ tree root page ID is invalid");
        return root_page;
    }

	}


    PageID BplusTree::Create() {

    }

	BplusTree::BplusTree(pager& storage, PageID metadata_Page) :pager_(storage), metadata_page_(metadata_Page)
	{

	}

	std::optional<RecordID> BplusTree::find(std::uint64_t id)
	{
		return std::optional<RecordID>();
	}

	void BplusTree::insert(std::uint64_t key, RecordID value)
	{
	}

	BplusTree_Node BplusTree::read_node(PageID page_id)
	{
		return BplusTree_Node();
	}

	void BplusTree::write_node(PageID page_id, const BplusTree_Node& node)
	{
	}

	InsertResult BplusTree::insert_recursive(PageID page_id, std::int64_t key, RecordID value)
	{
		return InsertResult();
	}

	void BplusTree::persist_root()
	{
	}

	





}