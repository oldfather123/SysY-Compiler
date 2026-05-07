#ifndef GEECEECEE_PARSER_HPP
#define GEECEECEE_PARSER_HPP

#include "ast.hpp"
#include "lexer.hpp"

namespace frontend::parser {
using namespace frontend::grammar;
using Lexer = frontend::lexer::Lexer;

class Parser {
public:
    explicit Parser(Lexer *lexer) : lexer(lexer) {}

    GrammarNode parse();

private:
    Lexer *lexer;
};
}  // namespace frontend::parser

#endif
