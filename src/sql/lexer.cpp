#include <sql/lexer.hpp>

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <unordered_map>

namespace qdb::sql {

std::string_view token_type_name(TokenType type) noexcept {
    switch (type) {
        case TokenType::KeywordCreate:        return "CREATE";
        case TokenType::KeywordTable:         return "TABLE";
        case TokenType::KeywordDrop:          return "DROP";
        case TokenType::KeywordSelect:        return "SELECT";
        case TokenType::KeywordInsert:        return "INSERT";
        case TokenType::KeywordInto:          return "INTO";
        case TokenType::KeywordValues:        return "VALUES";
        case TokenType::KeywordUpdate:        return "UPDATE";
        case TokenType::KeywordSet:           return "SET";
        case TokenType::KeywordDelete:        return "DELETE";
        case TokenType::KeywordFrom:          return "FROM";
        case TokenType::KeywordWhere:         return "WHERE";
        case TokenType::KeywordInteger:       return "INTEGER";
        case TokenType::KeywordText:          return "TEXT";
        case TokenType::KeywordBoolean:       return "BOOLEAN";
        case TokenType::KeywordFloat:         return "FLOAT";
        case TokenType::KeywordPrimary:       return "PRIMARY";
        case TokenType::KeywordKey:           return "KEY";
        case TokenType::KeywordNot:           return "NOT";
        case TokenType::KeywordNull:          return "NULL";
        case TokenType::KeywordAnd:           return "AND";
        case TokenType::KeywordOr:            return "OR";
        case TokenType::KeywordOrder:         return "ORDER";
        case TokenType::KeywordBy:            return "BY";
        case TokenType::KeywordLimit:         return "LIMIT";
        case TokenType::KeywordJoin:          return "JOIN";
        case TokenType::IntegerLiteral:       return "IntegerLiteral";
        case TokenType::FloatLiteral:         return "FloatLiteral";
        case TokenType::StringLiteral:        return "StringLiteral";
        case TokenType::BooleanLiteral:       return "BooleanLiteral";
        case TokenType::NullLiteral:          return "NullLiteral";
        case TokenType::Identifier:           return "Identifier";
        case TokenType::Comma:                return ",";
        case TokenType::Semicolon:            return ";";
        case TokenType::Dot:                  return ".";
        case TokenType::Star:                 return "*";
        case TokenType::Plus:                 return "+";
        case TokenType::Minus:                return "-";
        case TokenType::LeftParen:            return "(";
        case TokenType::RightParen:           return ")";
        case TokenType::Equal:                return "=";
        case TokenType::NotEqual:             return "!=";
        case TokenType::LessThan:             return "<";
        case TokenType::LessThanOrEqual:      return "<=";
        case TokenType::GreaterThan:          return ">";
        case TokenType::GreaterThanOrEqual:   return ">=";
        case TokenType::EndOfFile:            return "EOF";
        case TokenType::Invalid:              return "INVALID";
    }
    return "UNKNOWN";
}

std::string_view Token::type_name() const noexcept {
    return token_type_name(type);
}

namespace {

std::string to_upper(std::string_view str) {
    std::string upper;
    upper.reserve(str.size());
    for (const char c : str) {
        upper.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    }
    return upper;
}

const std::unordered_map<std::string, TokenType>& keyword_map() {
    static const std::unordered_map<std::string, TokenType> map = {
        {"CREATE",   TokenType::KeywordCreate},
        {"TABLE",    TokenType::KeywordTable},
        {"DROP",     TokenType::KeywordDrop},
        {"SELECT",   TokenType::KeywordSelect},
        {"INSERT",   TokenType::KeywordInsert},
        {"INTO",     TokenType::KeywordInto},
        {"VALUES",   TokenType::KeywordValues},
        {"UPDATE",   TokenType::KeywordUpdate},
        {"SET",      TokenType::KeywordSet},
        {"DELETE",   TokenType::KeywordDelete},
        {"FROM",     TokenType::KeywordFrom},
        {"WHERE",    TokenType::KeywordWhere},
        {"INT",      TokenType::KeywordInteger},
        {"INTEGER",  TokenType::KeywordInteger},
        {"TEXT",     TokenType::KeywordText},
        {"VARCHAR",  TokenType::KeywordText},
        {"STRING",   TokenType::KeywordText},
        {"BOOL",     TokenType::KeywordBoolean},
        {"BOOLEAN",  TokenType::KeywordBoolean},
        {"FLOAT",    TokenType::KeywordFloat},
        {"DOUBLE",   TokenType::KeywordFloat},
        {"REAL",     TokenType::KeywordFloat},
        {"PRIMARY",  TokenType::KeywordPrimary},
        {"KEY",      TokenType::KeywordKey},
        {"NOT",      TokenType::KeywordNot},
        {"NULL",     TokenType::KeywordNull},
        {"AND",      TokenType::KeywordAnd},
        {"OR",       TokenType::KeywordOr},
        {"ORDER",    TokenType::KeywordOrder},
        {"BY",       TokenType::KeywordBy},
        {"LIMIT",    TokenType::KeywordLimit},
        {"JOIN",     TokenType::KeywordJoin},
        {"TRUE",     TokenType::BooleanLiteral},
        {"FALSE",    TokenType::BooleanLiteral}
    };
    return map;
}

} // namespace

Lexer::Lexer(std::string_view source) noexcept
    : source_(source) {}

Result<std::vector<Token>> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (true) {
        Token token = next_token();
        if (token.is(TokenType::Invalid)) {
            return std::unexpected(Status{
                StatusCode::SyntaxError,
                "Syntax error at line " + std::to_string(token.line) +
                ", column " + std::to_string(token.column) + ": " + token.text
            });
        }
        tokens.push_back(token);
        if (token.is(TokenType::EndOfFile)) {
            break;
        }
    }
    return tokens;
}

Token Lexer::next_token() {
    skip_whitespace_and_comments();

    start_ = current_;
    token_column_ = column_;

    if (is_at_end()) {
        return Token{TokenType::EndOfFile, "", line_, token_column_};
    }

    const char c = advance();

    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
        return scan_identifier_or_keyword();
    }

    if (std::isdigit(static_cast<unsigned char>(c))) {
        return scan_number();
    }

    switch (c) {
        case '(': return Token{TokenType::LeftParen, "(", line_, token_column_};
        case ')': return Token{TokenType::RightParen, ")", line_, token_column_};
        case ',': return Token{TokenType::Comma, ",", line_, token_column_};
        case ';': return Token{TokenType::Semicolon, ";", line_, token_column_};
        case '.': return Token{TokenType::Dot, ".", line_, token_column_};
        case '*': return Token{TokenType::Star, "*", line_, token_column_};
        case '+': return Token{TokenType::Plus, "+", line_, token_column_};
        case '-': return Token{TokenType::Minus, "-", line_, token_column_};
        case '=': return Token{TokenType::Equal, "=", line_, token_column_};

        case '!':
            if (match('=')) {
                return Token{TokenType::NotEqual, "!=", line_, token_column_};
            }
            return Token{TokenType::Invalid, "Unexpected character '!' (did you mean '!=')", line_, token_column_};

        case '<':
            if (match('=')) {
                return Token{TokenType::LessThanOrEqual, "<=", line_, token_column_};
            }
            if (match('>')) {
                return Token{TokenType::NotEqual, "<>", line_, token_column_};
            }
            return Token{TokenType::LessThan, "<", line_, token_column_};

        case '>':
            if (match('=')) {
                return Token{TokenType::GreaterThanOrEqual, ">=", line_, token_column_};
            }
            return Token{TokenType::GreaterThan, ">", line_, token_column_};

        case '\'':
        case '"':
            return scan_string(c);

        default:
            return Token{
                TokenType::Invalid,
                std::string("Unexpected character '") + c + "'",
                line_,
                token_column_
            };
    }
}

char Lexer::peek() const noexcept {
    if (is_at_end()) return '\0';
    return source_[current_];
}

char Lexer::peek_next() const noexcept {
    if (current_ + 1 >= source_.size()) return '\0';
    return source_[current_ + 1];
}

char Lexer::advance() noexcept {
    const char c = source_[current_++];
    ++column_;
    return c;
}

bool Lexer::is_at_end() const noexcept {
    return current_ >= source_.size();
}

bool Lexer::match(char expected) noexcept {
    if (is_at_end() || source_[current_] != expected) {
        return false;
    }
    ++current_;
    ++column_;
    return true;
}

void Lexer::skip_whitespace_and_comments() {
    while (!is_at_end()) {
        const char c = peek();
        switch (c) {
            case ' ':
            case '\t':
            case '\r':
                advance();
                break;
            case '\n':
                advance();
                ++line_;
                column_ = 1;
                break;
            case '-':
                if (peek_next() == '-') {
                    // Single-line comment: skip until newline or EOF
                    advance(); // skip '-'
                    advance(); // skip '-'
                    while (!is_at_end() && peek() != '\n') {
                        advance();
                    }
                } else {
                    return;
                }
                break;
            case '/':
                if (peek_next() == '*') {
                    // Multi-line comment: skip until '*/' or EOF
                    advance(); // skip '/'
                    advance(); // skip '*'
                    while (!is_at_end()) {
                        if (peek() == '*' && peek_next() == '/') {
                            advance(); // skip '*'
                            advance(); // skip '/'
                            break;
                        }
                        if (peek() == '\n') {
                            ++line_;
                            column_ = 0; // will be incremented by advance
                        }
                        advance();
                    }
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

Token Lexer::scan_identifier_or_keyword() {
    while (!is_at_end() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
        advance();
    }

    const std::string_view raw_text = source_.substr(start_, current_ - start_);
    const std::string upper = to_upper(raw_text);

    const auto& kmap = keyword_map();
    const auto it = kmap.find(upper);
    if (it != kmap.end()) {
        return Token{it->second, std::string(raw_text), line_, token_column_};
    }

    return Token{TokenType::Identifier, std::string(raw_text), line_, token_column_};
}

Token Lexer::scan_number() {
    bool is_float = false;

    while (!is_at_end() && std::isdigit(static_cast<unsigned char>(peek()))) {
        advance();
    }

    if (!is_at_end() && peek() == '.' && std::isdigit(static_cast<unsigned char>(peek_next()))) {
        is_float = true;
        advance(); // consume '.'
        while (!is_at_end() && std::isdigit(static_cast<unsigned char>(peek()))) {
            advance();
        }
    }

    const std::string_view num_text = source_.substr(start_, current_ - start_);
    return Token{
        is_float ? TokenType::FloatLiteral : TokenType::IntegerLiteral,
        std::string(num_text),
        line_,
        token_column_
    };
}

Token Lexer::scan_string(char quote) {
    std::string value;

    while (!is_at_end()) {
        const char c = advance();

        if (c == quote) {
            // Check for escaped quote (e.g. '' in SQL)
            if (peek() == quote) {
                value.push_back(quote);
                advance(); // consume second quote
            } else {
                // Closing quote found
                return Token{TokenType::StringLiteral, std::move(value), line_, token_column_};
            }
        } else if (c == '\\' && !is_at_end()) {
            // C-style escape sequence support
            const char escaped = advance();
            switch (escaped) {
                case 'n':  value.push_back('\n'); break;
                case 't':  value.push_back('\t'); break;
                case 'r':  value.push_back('\r'); break;
                case '\\': value.push_back('\\'); break;
                case '\'': value.push_back('\''); break;
                case '"':  value.push_back('"');  break;
                default:
                    value.push_back(escaped);
                    break;
            }
        } else {
            if (c == '\n') {
                ++line_;
                column_ = 1;
            }
            value.push_back(c);
        }
    }

    return Token{
        TokenType::Invalid,
        "Unterminated string literal",
        line_,
        token_column_
    };
}

} // namespace qdb::sql
