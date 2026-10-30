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


    PageID BplusTree::Create(pager&storage) {
        PageID MetaDataPage{};
        if (!storage.allocate_page(MetaDataPage)){
            throw std::runtime_error("Failed to allocate B+ tree metadata page");
        }

        if (MetaDataPage == 0 || MetaDataPage == NO_PAGE) {
            throw std::runtime_error("Allocator returned invalid metadata page ID");
        }

        PageID RootPage{};
        if (!storage.allocate_page(RootPage)) {
            throw std::runtime_error("Failed to allocate B+ tree root page");
        }
        if (RootPage == 0 || RootPage == NO_PAGE || RootPage == MetaDataPage) {
            throw std::runtime_error("Allocator returned invalid root page ID");
        }

        Page root{};
       
        write_u32(root, 0, NODE_MAGIC);

        write_u16(root, 4, FORMAT_VERSION);

        root.data[6] = static_cast<std::byte>(LEAF_NODE);

        write_u16(root, 8, 0);
        write_u64(root, 16, NO_PAGE);

        if (!storage.write_page(page_index(RootPage), root)) {
            throw std::runtime_error("Failed to initialize B+ tree root page");
        }

        write_metadata_page(storage, MetaDataPage, RootPage);

        return MetaDataPage;
    }

	BplusTree::BplusTree(pager& storage, PageID metadata_Page) :pager_(storage), metadata_page_(metadata_Page),
        root_page_(read_metadata_page(storage,metadata_Page))
	{
        (void)read_node(root_page_);
	}

	std::optional<RecordID> BplusTree::find(std::int64_t id)
	{
        PageID current_page = root_page_;

        while (true) {
            BplusTree_Node node = read_node(current_page);

            if (node.is_leaf) {
                const auto it = std::lower_bound(node.keys.begin(), node.keys.end(), id);

                if (it == node.keys.end()||*it!=id) {
                    return std::nullopt;
                }

                const std::size_t index = static_cast<std::size_t>(it - node.keys.begin());
                return node.values[index];
            }

            const auto it = std::upper_bound(node.keys.begin(), node.keys.end(), id);

            const std::size_t child_index = static_cast<std::size_t>(it - node.keys.begin());

            current_page = node.children[child_index];


        }
	}



    InsertResult BplusTree::insert_recursive(PageID page_id, std::int64_t key, RecordID value)
    {

        BplusTree_Node node = read_node(page_id);

        // -------------------------------------------------------------------------
       // Case 1: Leaf Node
      // -------------------------------------------------------------------------
        if (node.is_leaf) {
            const auto it = std::lower_bound(node.keys.begin(), node.keys.end(), key);

            const std::size_t position = static_cast<std::size_t>(it - node.keys.begin());

            if (it != node.keys.end() && *it == key) {
                return{ false,std::nullopt};
            }

            node.keys.insert(it, key);
            node.values.insert(node.values.begin() + static_cast<std::ptrdiff_t>(position), value);

            if (node.keys.size() <= MAX_KEYS) {
                write_node(page_id, node);
                return { true,std::nullopt };
            }

            const std::size_t middle = node.keys.size() / 2;

            BplusTree_Node right;

            right.is_leaf = true;
            right.next_leaf = node.next_leaf;

            const auto mid_keys = node.keys.begin() + static_cast<std::ptrdiff_t>(middle);
            
            const auto mid_vals = node.values.begin() + static_cast<std::ptrdiff_t>(middle);
            
            right.keys.assign(mid_keys, node.keys.end());
            
            right.values.assign(mid_vals, node.values.end());
            
            node.keys.erase(mid_keys, node.keys.end());
            
            node.values.erase(mid_vals, node.values.end());
            const PageID right_page = allocate_node(right);

            node.next_leaf = right_page;
            write_node(page_id, node);

            // In a leaf split, the separator pushed up is copied from the first key of the right leaf
            
            return { true, Split{ right.keys.front(), right_page } };

        
        }

        // -------------------------------------------------------------------------
       // Case 2: Internal Node
      // -------------------------------------------------------------------------
        const auto child_it = std::upper_bound(node.keys.begin(), node.keys.end(), key);
        const std::size_t child_index = static_cast<std::size_t>(child_it - node.keys.begin());

        InsertResult child_result = insert_recursive(node.children[child_index], key, value);
        if (!child_result.inserted || !child_result.split) {
            return child_result;
        }

        const Split child_split = *child_result.split;

        node.keys.insert(node.keys.begin() + static_cast<std::ptrdiff_t>(child_index),
            child_split.seprator_key);

        node.children.insert(node.children.begin() + static_cast<std::ptrdiff_t>(child_index + 1),
            child_split.rightPage);

        if (node.keys.size() <= MAX_KEYS) {
            write_node(page_id, node);
            return { true ,std::nullopt };
        }

        const std::size_t middle = node.keys.size() / 2;
        const std::int64_t promoted_key = node.keys[middle];

        BplusTree_Node right;
        right.is_leaf = false;
        right.keys.assign(
            node.keys.begin() + static_cast<std::ptrdiff_t>(middle + 1),
            node.keys.end()
        );
        right.children.assign(
            node.children.begin() + static_cast<std::ptrdiff_t>(middle + 1),
            node.children.end()
        );

        node.keys.erase(
            node.keys.begin() + static_cast<std::ptrdiff_t>(middle),
            node.keys.end()
        );
        node.children.erase(
            node.children.begin() + static_cast<std::ptrdiff_t>(middle + 1),
            node.children.end()
        );

        const PageID right_page = allocate_node(right);
        write_node(page_id, node);

        return { true, Split{ promoted_key, right_page } };

    }

	bool BplusTree::insert(std::int64_t key, RecordID value)
	{

        check_page_id(pager_, value.page_id,
            "Cannot index a record with an invalid page ID");

        InsertResult result = insert_recursive(root_page_, key, value);

        if (!result.inserted) {
            return false;
        }


        if (result.split) {
            BplusTree_Node new_root;

            new_root.is_leaf = false;
            new_root.keys.push_back(result.split->seprator_key);

            new_root.children = { root_page_, result.split->rightPage };
            const PageID new_root_page = allocate_node(new_root);
            root_page_ = new_root_page;
            persist_root();

        }
        return true;
	}

	BplusTree_Node BplusTree::read_node(PageID page_id)
	{
        check_page_id(pager_, page_id, "B+ tree node page ID is invalid");

        Page page{};

        if (!pager_.read_page(page_index(page_id), page)) {
            throw std::runtime_error("Failed to read B+ tree node page");
        }

        if (read_u32(page, 0) != NODE_MAGIC) {
            throw std::runtime_error("Invalid B+ tree node magic");
        }

        if (read_u16(page, 4) != FORMAT_VERSION) {
            throw std::runtime_error("Unsupported B+ tree node version");
        }

        const auto type = std::to_integer<std::uint8_t>(page.data[6]);
        const std::size_t key_count = read_u16(page, 8);
        if (key_count > MAX_KEYS) {
            throw std::runtime_error("B+ tree node exceeds maximum key count");
        }

        BplusTree_Node node;

        node.is_leaf = (type == LEAF_NODE);
        if (!node.is_leaf && type != INTERNAL_NODE) {
            throw std::runtime_error("Invalid B+ tree node type");
        }

        node.next_leaf = read_u64(page, 16);
        node.keys.reserve(key_count);

        std::size_t offset = NODE_HEADER_SIZE;

        if (node.is_leaf) {
            node.values.reserve(key_count);

            for (std::size_t i = 0;i < key_count;++i) {
                const auto raw_key = read_u64(page, offset);
                offset += 8;

                node.keys.push_back(std::bit_cast<std::int64_t>(raw_key));

                RecordID value{};
                value.page_id = read_u64(page, offset);
                offset += 8;

                value.slot_index = read_u16(page, offset);
                offset += 2;
                check_page_id(pager_, value.page_id,
                    "B+ tree leaf contains invalid record page ID");
                node.values.push_back(value);
            }

            if (node.next_leaf != NO_PAGE) {
                check_page_id(pager_, node.next_leaf,
                    "B+ tree leaf has invalid next-leaf page ID");
            }
        }
        else {

            node.children.reserve(key_count + 1);
            node.children.push_back(read_u64(page, offset));
            offset += 8;

            check_page_id(pager_, node.children.front(),
                "B+ tree internal node has invalid child page ID");

            for (std::size_t i = 0;i < key_count;++i) {
                const auto raw_key = read_u64(page, offset);
                offset += 8;

                node.keys.push_back(std::bit_cast<std::int64_t>(raw_key));

                const PageID children = read_u64(page, offset);
                offset += 8;
                check_page_id(pager_, children,
                    "B+ tree internal node has invalid child page ID");
                node.children.push_back(children);
            }

        }


        if (!std::is_sorted(node.keys.begin(), node.keys.end()) ||
            std::adjacent_find(node.keys.begin(), node.keys.end()) != node.keys.end()) {
            throw std::runtime_error("B+ tree node keys are not strictly ordered");
        }

        if (node.is_leaf && node.values.size() != node.keys.size()) {
            throw std::runtime_error("B+ tree leaf key/value count mismatch");
        }
        if (!node.is_leaf && node.children.size() != node.keys.size() + 1) {
            throw std::runtime_error("B+ tree internal key/child count mismatch");
        }

        return node;
	}

	void BplusTree::write_node(PageID page_id, const BplusTree_Node& node)
	{
        if (node.keys.size() > MAX_KEYS) {
            throw std::length_error("B+ tree node has too many keys to serialize");
        }
        if (!std::is_sorted(node.keys.begin(), node.keys.end()) ||
            std::adjacent_find(node.keys.begin(), node.keys.end()) != node.keys.end()) {
            throw std::logic_error("Attempted to write unordered B+ tree keys");
        }
        if (node.is_leaf && node.values.size() != node.keys.size()) {
            throw std::logic_error("B+ tree leaf key/value count mismatch");
        }
        if (!node.is_leaf && node.children.size() != node.keys.size() + 1) {
            throw std::logic_error("B+ tree internal key/child count mismatch");
        }

        Page page{};
        write_u32(page, 0, NODE_MAGIC);
        write_u32(page, 4, FORMAT_VERSION);

        page.data[6] = static_cast<std::byte>(node.is_leaf ? LEAF_NODE : INTERNAL_NODE);

        write_u16(page, 8, static_cast<std::uint16_t>(node.keys.size()));
        write_u64(page, 16, node.is_leaf ? node.next_leaf : NO_PAGE);

        std::size_t offset = NODE_HEADER_SIZE;

        if (node.is_leaf) {

            for (std::size_t i = 0;i < node.keys.size();++i) {
                write_u64(page, offset, std::bit_cast<std::int64_t>(node.keys[i]));

                offset += 8;

                write_u64(page, offset, node.values[i].page_id);

                offset += 8;

                write_u16(page, offset, node.values[i].slot_index);

                offset += 2;
            }
        }
        else {
            write_u64(page, offset, node.children.front());
            offset += 8;
            for (std::size_t i = 0;i < node.keys.size();++i) {
                write_u64(page, offset,
                    std::bit_cast<std::uint64_t>(node.keys[i]));
                offset += 8;
                write_u64(page, offset, node.children[i + 1]);
                offset += 8;
            }
        }
        if (offset > PAGE_SIZE) {
            throw std::length_error("Serialized B+ tree node exceeds page size");
        }
        if (!pager_.write_page(page_index(page_id), page)) {
            throw std::runtime_error("Failed to write B+ tree node page");
        }
	}

    PageID BplusTree::allocate_node(const BplusTree_Node& node)
    {
        PageID id{};

        if (!pager_.allocate_page(id)) {
            throw std::runtime_error("Failed to allocate B+ tree node page");
        }
        if (id == 0 || id == NO_PAGE || id == metadata_page_) {
            throw std::runtime_error("Allocator returned invalid B+ tree node page ID");
        }
        write_node(id, node);
        return id;
    }


	void BplusTree::persist_root()
	{
        write_metadata_page(pager_, metadata_page_, root_page_);
	}

	





}