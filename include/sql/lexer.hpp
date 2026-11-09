#pragma once

#include <quartz/status.hpp>
#include <sql/token.hpp>

#include <cstddef>
#include <string_view>
#include <vector>

namespace qdb::sql {

class Lexer {
public:
    explicit Lexer(std::string_view source) noexcept;

    // Tokenizes the entire source and returns all tokens ending with EndOfFile.
    [[nodiscard]] Result<std::vector<Token>> tokenize();

    // Stream-based: returns the next token.
    [[nodiscard]] Token next_token();

private:
    [[nodiscard]] char peek() const noexcept;
    [[nodiscard]] char peek_next() const noexcept;
    char advance() noexcept;
    [[nodiscard]] bool is_at_end() const noexcept;
    bool match(char expected) noexcept;

    void skip_whitespace_and_comments();
    Token scan_token();
    Token scan_identifier_or_keyword();
    Token scan_number();
    Token scan_string(char quote);

    std::string_view source_;
    std::size_t start_{0};
    std::size_t current_{0};
    std::size_t line_{1};
    std::size_t column_{1};
    std::size_t token_column_{1};
};

} // namespace qdb::sql
