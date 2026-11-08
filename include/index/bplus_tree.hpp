#pragma once

#include<storage/pager.hpp>
#include<storage/RecordID.hpp>

namespace qdb {

	struct BplusTree_Node {
		std::vector<std::int64_t> keys;
		std::vector<RecordID> values;
		std::vector<PageID> children;
		bool is_leaf = false;
		PageID next_leaf = std::numeric_limits<PageID>::max();
	};

	struct Split {
		std::int64_t seprator_key;
		PageID rightPage;
	};

	struct InsertResult {
		bool inserted = false;
		std::optional<Split> split;
	};

	struct EraseResult {
		bool erased = false;
		bool underflow = false;
		std::optional<std::int64_t> new_min;
	};
	class BplusTree {
	public:

		[[nodiscard]] static PageID Create(pager& storage);
		[[nodiscard]] static PageID create(pager& storage) { return Create(storage); }
		explicit BplusTree(pager& storage, PageID metadata_Page);

		[[nodiscard]]
		std::optional<RecordID> find(std::int64_t id);

		bool insert(std::int64_t key, RecordID value);

		[[nodiscard]]
		PageID MetaDataPage() const {
			return metadata_page_;
		}

		[[nodiscard]]
		PageID RootPage() const {
			return root_page_;
		}

		[[nodiscard]]
		bool erase(std::int64_t key);


		bool validate() const;

	private:
		static constexpr std::size_t MAX_KEYS = 128;
		static constexpr std::size_t MIN_KEYS = MAX_KEYS / 2;

		pager& pager_;
		PageID metadata_page_;
		PageID root_page_;


		[[nodiscard]] BplusTree_Node read_node(PageID page_id) const;

		void write_node(PageID page_id, const BplusTree_Node& node);
		[[nodiscard]] PageID allocate_node(const BplusTree_Node& node);
		[[nodiscard]] InsertResult insert_recursive(
			PageID page_id,
			std::int64_t key,
			RecordID value
		);
		[[nodiscard]]
		EraseResult erase_recursive(
			PageID page_id,
			std::int64_t key,
			bool is_root
		);


		
		void persist_root();
	};
	using BPlusTree = BplusTree;

}
