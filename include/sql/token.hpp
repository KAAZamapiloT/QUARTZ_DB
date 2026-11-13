#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace qdb::sql {

enum class TokenType : std::uint16_t {
    // Keywords - DDL
    KeywordCreate,
    KeywordTable,
    KeywordDrop,

    // Keywords - DML & Query
    KeywordSelect,
    KeywordInsert,
    KeywordInto,
    KeywordValues,
    KeywordUpdate,
    KeywordSet,
    KeywordDelete,
    KeywordFrom,
    KeywordWhere,

    // Keywords - Data Types
    KeywordInteger,
    KeywordText,
    KeywordBoolean,
    KeywordFloat,

    // Keywords - Constraints & Logic
    KeywordPrimary,
    KeywordKey,
    KeywordNot,
    KeywordNull,
    KeywordAnd,
    KeywordOr,
    KeywordOrder,
    KeywordBy,
    KeywordLimit,
    KeywordJoin,
    KeywordInner,
    KeywordLeft,
    KeywordRight,
    KeywordOn,

    // Literals
    IntegerLiteral,
    FloatLiteral,
    StringLiteral,
    BooleanLiteral,
    NullLiteral,

    // Identifiers
    Identifier,

    // Operators & Delimiters
    Comma,               // ,
    Semicolon,           // ;
    Dot,                 // .
    Star,                // *
    Plus,                // +
    Minus,               // -
    LeftParen,           // (
    RightParen,          // )
    Equal,               // =
    NotEqual,            // != or <>
    LessThan,            // <
    LessThanOrEqual,     // <=
    GreaterThan,         // >
    GreaterThanOrEqual,  // >=

    // Special
    EndOfFile,
    Invalid
};

struct Token {
    TokenType type{TokenType::Invalid};
    std::string text;
    std::size_t line{1};
    std::size_t column{1};

    [[nodiscard]] constexpr bool is(TokenType expected) const noexcept {
        return type == expected;
    }

    [[nodiscard]] constexpr bool is_keyword() const noexcept {
        return type >= TokenType::KeywordCreate && type <= TokenType::KeywordJoin;
    }

    [[nodiscard]] constexpr bool is_literal() const noexcept {
        return type >= TokenType::IntegerLiteral && type <= TokenType::NullLiteral;
    }

    [[nodiscard]] std::string_view type_name() const noexcept;
};

[[nodiscard]] std::string_view token_type_name(TokenType type) noexcept;

} // namespace qdb::sql
