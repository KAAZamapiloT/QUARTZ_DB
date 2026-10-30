#pragma once

#include<storage/pager.hpp>
#include<storage/RecordID.hpp>
namespace qdb {

	struct BplusTree_Node {
		std::vector<std::uint64_t> keys;
		std::vector<RecordID> values;
		std::vector<PageID> children;
		bool is_leaf = false;
		PageID next_leaf = std::numeric_limits<PageID>::max();
	};

	struct Split {
		std::uint64_t seprator_key;
		PageID rightPage;
	};

	struct InsertResult {
		bool inserted = false;
		std::vector<Split> split;
	};
	class BplusTree {
	public:

		[[nodiscard]] static PageID Create(pager& storage);
		explicit BplusTree(pager& storage, PageID metadata_Page);

		[[nodiscard]]
		std::optional<RecordID> find(std::uint64_t id);

		void insert(std::uint64_t key, RecordID value);

		[[nodiscard]]
		PageID MetaDataPage() const {
			return metadata_page_;
		}

		[[nodiscard]]
		PageID RootPage() const {
			return root_page_;
		}
	private:
		static constexpr std::size_t MAX_KEYS = 128;

		



		pager& pager_;
		PageID metadata_page_;
		PageID root_page_;


		[[nodiscard]] BplusTree_Node read_node(PageID page_id);
		void write_node(PageID page_id, const BplusTree_Node& node);
		[[nodiscard]] PageID allocate_node(const BplusTree_Node& node);
		[[nodiscard]] InsertResult insert_recursive(
			PageID page_id,
			std::int64_t key,
			RecordID value
		);



		
		void persist_root();
	};
}
