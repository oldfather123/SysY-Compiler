#include "lexer.hpp"

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <utility>

#include "util.hpp"

namespace frontend::token_matcher {
MatchResult Matcher::try_match_reserved(std::string_view text) {
    auto keywords = token_type::RESERVED_KEYWORDS();
    for (const auto &[keyword, token_type] : keywords) {
        auto keyword_len = keyword.size();
        if (keyword_len < text.size() && text.substr(0, keyword_len) == keyword &&
            (text[keyword_len] != '_' && !std::isalnum(text[keyword_len]))) {
            return std::make_pair(keyword_len, token_type);
        }
    }
    return std::nullopt;
}

MatchResult Matcher::try_match_identifier(std::string_view text) {
    if (text.empty() || (!std::isalpha(text[0]) && text[0] != '_')) {
        return std::nullopt;
    }
    size_t res_index = 1;
    while (res_index < text.size() && (std::isalnum(text[res_index]) || text[res_index] == '_')) {
        res_index++;
    }
    return std::make_pair(res_index, TokenType::IDENFR);
}

MatchResult Matcher::try_match_str_const(std::string_view text) {
    if (text.empty() || text[0] != '\"') {
        return std::nullopt;
    }
    size_t res_index = 1;
    while (res_index < text.size() && text[res_index] != '\"') {
        res_index++;
    }
    return std::make_pair(res_index + 1, TokenType::STRCON);
}

MatchResult Matcher::try_match_int_const(std::string_view text) {
    if (text.empty() || !std::isdigit(text[0])) {
        return std::nullopt;
    }
    size_t res;
    if (text[0] == '0' && text.size() > 1 && (text[1] == 'x' || text[1] == 'X')) {
        // hexadecimal constant
        res = 2;
        while (res < text.size() && std::isxdigit(text[res])) {
            res++;
        }
    } else if (text[0] == '0') {
        // octal constant
        res = 1;
        while (res < text.size() && std::isdigit(text[res]) && text[res] <= '7') {
            res++;
        }
    } else {
        // decimal constant
        res = 1;
        while (res < text.size() && std::isdigit(text[res])) {
            res++;
        }
    }
    return std::make_pair(res, TokenType::INTCON);
}

MatchResult Matcher::try_match_float_const(std::string_view text) {
    if (text.empty() || (!std::isdigit(text[0]) && text[0] != '.')) {
        return std::nullopt;
    }
    auto cur_loc = text;
    bool has_hex = false;
    if (cur_loc[0] == '0' && (cur_loc[1] == 'x' || cur_loc[1] == 'X')) {
        cur_loc.remove_prefix(2);
        has_hex = true;
    }
    cur_loc = std::find_if(cur_loc.cbegin(), cur_loc.cend(), [has_hex](char ch) {
        return !::isdigit(ch) && ch != '.' && (!has_hex || ::tolower(ch) < 'a' || ::tolower(ch) > 'f');
    });
    if (!cur_loc.empty() && (std::tolower(cur_loc[0]) == 'e' || std::tolower(cur_loc[0]) == 'p')) {
        cur_loc.remove_prefix(1);
        if (cur_loc[0] == '+' || cur_loc[0] == '-') {
            cur_loc.remove_prefix(1);
        }
        cur_loc = std::find_if_not(cur_loc.cbegin(), cur_loc.cend(), ::isdigit);
    }

    // determine if it can downgrade to a pure int like `1`
    auto content = text.substr(0, cur_loc.cbegin() - text.cbegin());
    if (can_parse_to_int(content)) {
        // TODO return INTCON here, and rm `try_match_int_const`
        return std::nullopt;
    }

    return std::make_pair(cur_loc.cbegin() - text.cbegin(), TokenType::FLOATCON);
}

MatchResult Matcher::try_match_delimiter(std::string_view text) {
    auto keywords = token_type::DELIMITER_KEYWORDS();
    for (const auto &[keyword, token_type] : keywords) {
        auto keyword_len = keyword.size();
        if (keyword_len <= text.size() && text.substr(0, keyword_len) == keyword) {
            return std::make_pair(keyword_len, token_type);
        }
    }
    return std::nullopt;
}

}  // namespace frontend::token_matcher

namespace frontend::lexer {
using namespace frontend::token_matcher;
std::optional<Token> Lexer::next_token() {
    while (skip_whitespace() || skip_comment()) {
    }
    if (cur_loc.empty()) {
        return std::nullopt;
    }
    if (auto result = match_one_token(cur_loc); result.has_value()) {
        auto [len, token_type] = result.value();
        auto token = Token(token_type, std::string(cur_loc.substr(0, len)), cur_line);
        cur_loc.remove_prefix(len);
        logger::DEBUG("[Lexer::next_token] token: ", token, " at line ", cur_line);
        return token;
    }
    return std::nullopt;
}

bool Lexer::skip_whitespace() {
    bool has_skipped = false;
    while (!cur_loc.empty() && std::isspace(cur_loc[0])) {
        if (cur_loc[0] == '\n') {
            cur_line++;
        }
        has_skipped = true;
        cur_loc.remove_prefix(1);
    }
    return has_skipped;
}

bool Lexer::skip_comment() {
    if (cur_loc.empty() || cur_loc[0] != '/') {
        return false;
    }
    if (cur_loc.substr(0, 2) == "//") {
        if (const auto end_of_line = cur_loc.find('\n'); end_of_line == std::string_view::npos) {
            // no end of line, means the comment is the last line
            cur_loc.remove_prefix(cur_loc.size());
        } else {
            cur_loc.remove_prefix(end_of_line + 1);
            cur_line++;
        }
        return true;
    }
    if (cur_loc.substr(0, 2) == "/*") {
        const auto end_index = cur_loc.find("*/", 2);
        if (end_index == std::string_view::npos) {
            logger::ERROR("[Lexer::skip_comment] error: missing '*/' in comment block at line ", cur_line);
            return false;
        }
        const auto comment = cur_loc.substr(0, end_index);
        cur_line += std::count(comment.cbegin(), comment.cend(), '\n');
        cur_loc.remove_prefix(end_index + 2);
        return true;
    }
    return false;
}

void Lexer::branch() {
    branches.push({cur_loc, cur_line});
    // logger::DEBUG("[Lexer::branch] branch at line ", cur_line, ", cur stack size: ", branches.size());
}

void Lexer::rollback() {
    if (branches.empty()) {
        logger::ERROR("[Lexer::rollback] error: no branch to rollback");
        return;
    }
    auto &[saved_loc, saved_line] = branches.top();
    cur_loc = saved_loc;
    cur_line = saved_line;
    branches.pop();
    // logger::DEBUG("[Lexer::rollback] rollback to line ", cur_line, ", cur stack size: ", branches.size());
}

void Lexer::commit() {
    if (branches.empty()) {
        logger::ERROR("[Lexer::commit] error: no branch to commit");
        return;
    }
    branches.pop();
    logger::DEBUG("[Lexer::commit] commit, cur stack size: ", branches.size());
}

bool Lexer::is_end() {
    while (skip_comment() || skip_whitespace()) {
    }
    return branches.empty() &&
           std::all_of(cur_loc.begin(), cur_loc.end(), [](unsigned char c) { return std::isspace(c); });
}

}  // namespace frontend::lexer
