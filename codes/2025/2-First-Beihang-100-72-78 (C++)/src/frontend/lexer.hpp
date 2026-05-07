#ifndef GEECEECEE_FRONTEND_LEXER_HPP
#define GEECEECEE_FRONTEND_LEXER_HPP

#include <cstddef>
#include <functional>
#include <optional>
#include <stack>
#include <string_view>
#include <utility>

#include "token.hpp"

namespace frontend::token_matcher {
using TokenType = token_type::TokenType;
using MatchResult = std::optional<std::pair<size_t, TokenType>>;

class Matcher {
public:
    static MatchResult try_match_reserved(std::string_view text);
    static MatchResult try_match_identifier(std::string_view text);
    static MatchResult try_match_str_const(std::string_view text);
    static MatchResult try_match_int_const(std::string_view text);
    static MatchResult try_match_float_const(std::string_view text);
    static MatchResult try_match_delimiter(std::string_view text);
};

const std::function<MatchResult(std::string_view)> MACHERS[] = {
    Matcher::try_match_reserved,
    Matcher::try_match_identifier,
    Matcher::try_match_str_const,
    Matcher::try_match_float_const,
    Matcher::try_match_int_const,
    Matcher::try_match_delimiter,
};

inline MatchResult match_one_token(std::string_view text) {
    for (const auto &matcher : MACHERS) {
        auto result = matcher(text);
        if (result.has_value()) {
            return result;
        }
    }
    return std::nullopt;
}
}  // namespace frontend::token_matcher

namespace frontend::lexer {
using namespace frontend::token;

class Lexer {
    class Iterator;

public:
    explicit Lexer(std::string_view text) : text(text), cur_loc(text), cur_line(1) {
#ifdef ENABLE_LOG
        // output token list
        std::ostringstream oss;
        for (const auto &token : *this) {
            oss << token << std::endl;
        }
        auto path = logger::log_to_file("token_list.txt", oss.str());
        // reset cur loc and line
        cur_loc = text;
        cur_line = 1;
        logger::INFO("token list is saved to ", path);
#endif
    }

    // Iterator interface
    [[nodiscard]] inline Iterator begin() { return Iterator(this, next_token()); }
    [[nodiscard]] inline Iterator end() { return Iterator(this, std::nullopt); }

    inline std::optional<Token> next() { return next_token(); }
    inline std::optional<Token> next_assert(TokenType type) {
        branch();
        auto token = next();
        if (!token.has_value() || !token.value().is_type(type)) {
            logger::DEBUG("Expected token type: ", type, " , but failed");
            rollback();
            return std::nullopt;
        }
        commit();
        return token;
    }

    // branch parsing
    void branch();
    // rollback to the last branch, if any, and remove it from the stack
    void rollback();
    // commit the current branch, if any, and remove it from the stack
    void commit();

    // call when parse finished, check if lexer reach the end
    [[nodiscard]] bool is_end();

private:
    [[nodiscard]] std::optional<Token> next_token();
    [[nodiscard]] bool skip_whitespace();
    [[nodiscard]] bool skip_comment();

private:
    class Iterator {
    public:
        explicit Iterator(Lexer *self, std::optional<Token> cur_token) : self(self), cur_token(cur_token) {}
        const Token &operator*() const { return cur_token.value(); }
        bool operator==(const Iterator &other) const { return self == other.self && !cur_token && !other.cur_token; }
        bool operator!=(const Iterator &other) const { return !(*this == other); }
        Iterator &operator++() {
            cur_token = self->next_token();
            return *this;
        }

    private:
        Lexer *self;
        std::optional<Token> cur_token;
    };

private:
    std::string_view text;
    std::string_view cur_loc;
    size_t cur_line;
    std::stack<std::pair<std::string_view, size_t>> branches;  // <loc, line>, for backtracking
};
}  // namespace frontend::lexer

#endif  // GEECEECEE_FRONTEND_LEXER_HPP
