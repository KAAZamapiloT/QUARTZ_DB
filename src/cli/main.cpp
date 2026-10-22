

#include <cassert>
#include <iostream>

#include"storage/record.hpp"
#include "storage/pager.hpp"
int main() {


    const qdb::Record original{ 42, "QuartzDB" };

    const auto bytes = qdb::RecordCodec::serialize(original);

    const auto decoded = qdb::RecordCodec::deserialize(
        bytes.data(),
        bytes.size()
    );

    assert(decoded.has_value());
    assert(decoded->id == original.id);
    assert(decoded->name == original.name);

    std::cout << "Record serialization test passed!\n";
}