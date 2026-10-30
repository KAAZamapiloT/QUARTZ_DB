#include"index/primary_index.hpp"



namespace qdb {
	
	PrimaryIndex::PrimaryIndex(
		pager& storage,
		PageID metadata_page
	)
		: tree_(storage, metadata_page) {
	}

	
	Result<std::unique_ptr<PrimaryIndex>> PrimaryIndex::create(pager& storage)
	{
		try {
			const PageID meta_data_page = BplusTree::Create(storage);

			return std::unique_ptr<PrimaryIndex>(new PrimaryIndex(storage,meta_data_page));
		}
		catch (const std::runtime_error& error) {
		
			return std::unexpected(Status{StatusCode::IndexError,error.what()});
		
		}
	}

	Result<std::unique_ptr<PrimaryIndex>> PrimaryIndex::open(pager& storage, PageID metadata_page)
	{
		try {
			

			return std::unique_ptr<PrimaryIndex>(new PrimaryIndex(storage, metadata_page));
		}
		catch (const std::runtime_error& error) {

			return std::unexpected(Status{ StatusCode::IndexError,error.what() });

		}
	}

	Result<void> PrimaryIndex::insert(std::int64_t key, RecordID id)
	{
		try {

			if (!tree_.insert(key, id)) {
				return std::unexpected(Status{StatusCode::DuplicatePrimaryKey, "Primary key already exists" });
			}
		}
		catch (const std::runtime_error& error) {

			return std::unexpected(Status{ StatusCode::IndexError,error.what() });
		}
	}

	Result<std::optional<RecordID>> PrimaryIndex::find(std::int64_t key)
	{
		try {
			return tree_.find(key);
			
		}
		catch (const std::runtime_error& error) {

			return std::unexpected(Status{ StatusCode::IndexError,error.what() });
		}
	}

}