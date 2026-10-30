#pragma once

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <utility>

namespace qdb {

    enum class StatusCode : std::uint16_t {
        InvalidArgument,
        InvalidSchema,
        InvalidRecord,
        TypeMismatch,
        NullConstraintViolation,

        DuplicatePrimaryKey,
        NoPrimaryKey,

        StorageError,
        Corruption,
        IndexError,
        CatalogError
    };

    class Status {
    public:
        Status(
            StatusCode code,
            std::string message = {}
        )
            : code_(code),
            message_(std::move(message)) {
        }

        [[nodiscard]]
        StatusCode code() const noexcept {
            return code_;
        }

        [[nodiscard]]
        std::string_view message() const noexcept {
            return message_;
        }

    private:
        StatusCode code_;
        std::string message_;
    };

    template<typename T>
    using Result = std::expected<T, Status>;

}