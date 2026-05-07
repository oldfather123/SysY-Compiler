#include "parser.hpp"

namespace frontend::parser {
GrammarNode Parser::parse() {
    auto gen = grammar::generator<NodeType::COMP_UNIT>();
    auto res = gen(this->lexer);
    if (!res.has_value()) {
        logger::ERROR("[Parser::parse] parse failed");
        throw std::runtime_error("parse failed");
    }
    if (!lexer->is_end()) {
        logger::ERROR("[Parser::parse] parse finished, but lexer does not reach the end");
        throw std::runtime_error("lexer not end");
    }
#ifdef ENABLE_LOG
    auto content = res.value()->print_tree();
    auto path = logger::log_to_file("ast.txt", content);
    logger::INFO("ast structure is saved to ", path);
#endif
    return std::move(res.value());
}
}  // namespace frontend::parser
