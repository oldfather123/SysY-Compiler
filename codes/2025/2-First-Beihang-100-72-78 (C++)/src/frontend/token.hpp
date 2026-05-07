#ifndef GEECEECEE_FRONTEND_TOKEN_HPP
#define GEECEECEE_FRONTEND_TOKEN_HPP

#include <algorithm>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "log.hpp"

namespace frontend::token_type {
// clang-format off
enum class TokenType {
    // identifier
    IDENFR,

    // const value
    INTCON, FLOATCON, STRCON,

    // reserved
    CONSTTK, INTTK, FLOATTK, BREAKTK, CONTINUETK, IFTK,
    ELSETK, WHILETK, RETURNTK, VOIDTK,

    // delimiter
    NOT, AND, OR, PLUS, MINU, MULT, DIV,
    MOD, LSS, LEQ, GRE, GEQ, EQL, NEQ,
    ASSIGN, SEMICN, COMMA, LPARENT, RPARENT, LBRACK, RBRACK,
    LBRACE, RBRACE
};
// clang-format on

// overload << operator
inline std::ostream &operator<<(std::ostream &os, const TokenType &type) {
    const static std::unordered_map<TokenType, std::string> token_type_to_string_map{
        {TokenType::IDENFR, "IDENFR"},     {TokenType::INTCON, "INTCON"},   {TokenType::FLOATCON, "FLOATCON"},
        {TokenType::STRCON, "STRCON"},     {TokenType::CONSTTK, "CONSTTK"}, {TokenType::INTTK, "INTTK"},
        {TokenType::FLOATTK, "FLOATTK"},   {TokenType::BREAKTK, "BREAKTK"}, {TokenType::CONTINUETK, "CONTINUETK"},
        {TokenType::IFTK, "IFTK"},         {TokenType::ELSETK, "ELSETK"},   {TokenType::WHILETK, "WHILETK"},
        {TokenType::RETURNTK, "RETURNTK"}, {TokenType::VOIDTK, "VOIDTK"},   {TokenType::NOT, "NOT"},
        {TokenType::AND, "AND"},           {TokenType::OR, "OR"},           {TokenType::PLUS, "PLUS"},
        {TokenType::MINU, "MINU"},         {TokenType::MULT, "MULT"},       {TokenType::DIV, "DIV"},
        {TokenType::MOD, "MOD"},           {TokenType::LSS, "LSS"},         {TokenType::LEQ, "LEQ"},
        {TokenType::GRE, "GRE"},           {TokenType::GEQ, "GEQ"},         {TokenType::EQL, "EQL"},
        {TokenType::NEQ, "NEQ"},           {TokenType::ASSIGN, "ASSIGN"},   {TokenType::SEMICN, "SEMICN"},
        {TokenType::COMMA, "COMMA"},       {TokenType::LPARENT, "LPARENT"}, {TokenType::RPARENT, "RPARENT"},
        {TokenType::LBRACK, "LBRACK"},     {TokenType::RBRACK, "RBRACK"},   {TokenType::LBRACE, "LBRACE"},
        {TokenType::RBRACE, "RBRACE"},
    };
    if (const auto it = token_type_to_string_map.find(type); it != token_type_to_string_map.end()) {
        os << it->second;
        return os;
    }
    logger::ERROR("[TokenType::operator<<] unknown token type");
    return os;
}

constexpr const static auto RESERVED_KEYWORDS = []() {
    auto keywords = std::vector<std::pair<std::string, TokenType>>{
        {"const", TokenType::CONSTTK},
        {"int", TokenType::INTTK},
        {"float", TokenType::FLOATTK},
        {"break", TokenType::BREAKTK},
        {"continue", TokenType::CONTINUETK},
        {"if", TokenType::IFTK},
        {"else", TokenType::ELSETK},
        {"while", TokenType::WHILETK},
        {"return", TokenType::RETURNTK},
        {"void", TokenType::VOIDTK},
    };
    return keywords;
};

constexpr const static auto DELIMITER_KEYWORDS = []() {
    auto keywords = std::vector<std::pair<std::string, TokenType>>{
        {"!", TokenType::NOT},    {"&&", TokenType::AND},    {"||", TokenType::OR},     {"+", TokenType::PLUS},
        {"-", TokenType::MINU},   {"*", TokenType::MULT},    {"/", TokenType::DIV},     {"%", TokenType::MOD},
        {"<", TokenType::LSS},    {"<=", TokenType::LEQ},    {">", TokenType::GRE},     {">=", TokenType::GEQ},
        {"==", TokenType::EQL},   {"!=", TokenType::NEQ},    {"=", TokenType::ASSIGN},  {";", TokenType::SEMICN},
        {",", TokenType::COMMA},  {"(", TokenType::LPARENT}, {")", TokenType::RPARENT}, {"[", TokenType::LBRACK},
        {"]", TokenType::RBRACK}, {"{", TokenType::LBRACE},  {"}", TokenType::RBRACE},
    };
    // clang-format off
    // sort to match long word whith priority
    std::sort(keywords.begin(), keywords.end(), [](const auto &a, const auto &b) {
        return a.first.size() > b.first.size();
    });
    // clang-format on
    return keywords;
};

}  // namespace frontend::token_type

namespace frontend::token {
using TokenType = token_type::TokenType;

class Token {
public:
    explicit Token(TokenType type, std::string content, size_t line)
        : type(type), content(std::move(content)), line(line) {}
    [[nodiscard]] inline bool is_type(const TokenType &t) const { return type == t; }
    [[nodiscard]] inline TokenType get_type() const { return type; }
    [[nodiscard]] inline const std::string &get_content() const { return content; }
    [[nodiscard]] inline size_t get_line() const { return line; }

    friend std::ostream &operator<<(std::ostream &os, const Token &token);

private:
    TokenType type;
    std::string content;
    size_t line;
};

inline std::ostream &operator<<(std::ostream &os, const Token &token) {
    os << "Token(" << token.type << ", " << token.content << ", " << token.line << ")";
    return os;
}
}  // namespace frontend::token

#endif  // GEECEECEE_FRONTEND_TOKEN_HPP
