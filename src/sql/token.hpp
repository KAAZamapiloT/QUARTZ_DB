#pragma once
#include<cstdint>
#include<vector>
namespace qdb {
	enum class ExecTokens:uint8_t {
		SELECT,
		FIND,
		CREATE,
		DROP,
	    TRUNCATE,
		JOIN,
		WHERE
	};

	// conceptutally eachg exec token gets mapped to a function and then executed in executioner

}