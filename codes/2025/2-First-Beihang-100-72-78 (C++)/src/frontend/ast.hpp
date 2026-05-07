#ifndef GEECEECEE_FRONTEND_AST_HPP
#define GEECEECEE_FRONTEND_AST_HPP

#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>

#include "lexer.hpp"
#include "token.hpp"
namespace frontend::ast_type {
// clang-format off
enum class NodeType {
    COMP_UNIT, DECL, CONST_DECL, VAR_DECL, B_TYPE, CONST_DEF, CONST_INIT_VAL, VAR_DEF,
    FUNC_DEF, MAIN_FUNC_DEF, INIT_VAL, FUNC_TYPE, FUNC_F_PARAMS, FUNC_F_PARAM, BLOCK,
    BLOCK_ITEM, STMT, ASSIGN_STMT, EXP_STMT, IF_STMT, WHILE_STMT, BREAK_STMT,
    CONTINUE_STMT, RETURN_STMT, EXP, COND, LVAL, PRIMARY_EXP, NUMBER, UNARY_EXP, UNARY_OP,
    FUNC_R_PARAMS, MUL_EXP, ADD_EXP, REL_EXP, EQ_EXP, LAND_EXP, LOR_EXP, CONST_EXP
};
// clang-format on

inline std::ostream &operator<<(std::ostream &os, const NodeType &type) {
    const static std::unordered_map<NodeType, std::string> node_type_to_string_map{
        {NodeType::COMP_UNIT, "CompUnit"},
        {NodeType::FUNC_DEF, "FuncDef"},
        {NodeType::MAIN_FUNC_DEF, "MainFuncDef"},
        {NodeType::DECL, "Decl"},
        {NodeType::CONST_DECL, "ConstDecl"},
        {NodeType::VAR_DECL, "VarDecl"},
        {NodeType::B_TYPE, "BType"},
        {NodeType::CONST_DEF, "ConstDef"},
        {NodeType::CONST_INIT_VAL, "ConstInitVal"},
        {NodeType::VAR_DEF, "VarDef"},
        {NodeType::CONST_EXP, "ConstExp"},
        {NodeType::INIT_VAL, "InitVal"},
        {NodeType::FUNC_TYPE, "FuncType"},
        {NodeType::FUNC_F_PARAMS, "FuncFParams"},
        {NodeType::FUNC_F_PARAM, "FuncFParam"},
        {NodeType::BLOCK, "Block"},
        {NodeType::BLOCK_ITEM, "BlockItem"},
        {NodeType::STMT, "Stmt"},
        {NodeType::ASSIGN_STMT, "AssignStmt"},
        {NodeType::EXP_STMT, "ExpStmt"},
        {NodeType::IF_STMT, "IfStmt"},
        {NodeType::WHILE_STMT, "WhileStmt"},
        {NodeType::BREAK_STMT, "BreakStmt"},
        {NodeType::CONTINUE_STMT, "ContinueStmt"},
        {NodeType::RETURN_STMT, "ReturnStmt"},
        {NodeType::EXP, "Exp"},
        {NodeType::COND, "Cond"},
        {NodeType::LVAL, "LVal"},
        {NodeType::PRIMARY_EXP, "PrimaryExp"},
        {NodeType::NUMBER, "Number"},
        {NodeType::UNARY_EXP, "UnaryExp"},
        {NodeType::UNARY_OP, "UnaryOp"},
        {NodeType::FUNC_R_PARAMS, "FuncRParams"},
        {NodeType::MUL_EXP, "MulExp"},
        {NodeType::ADD_EXP, "AddExp"},
        {NodeType::REL_EXP, "RelExp"},
        {NodeType::EQ_EXP, "EqExp"},
        {NodeType::LAND_EXP, "LandExp"},
        {NodeType::LOR_EXP, "LorExp"}};
    if (const auto it = node_type_to_string_map.find(type); it != node_type_to_string_map.end()) {
        os << it->second;
        return os;
    }
    logger::ERROR("[NodeType::operator<<] unknown node type");
    return os;
}

}  // namespace frontend::ast_type

namespace frontend::ast {
class ASTNode;
using NodeType = ast_type::NodeType;
using Token = token::Token;
using ASTNodePtr = std::unique_ptr<ASTNode>;
using TokenPtr = std::unique_ptr<Token>;
using GrammarNode = std::variant<ASTNodePtr, TokenPtr>;
using GrammarNodeIterator = std::vector<GrammarNode>::const_iterator;

class ASTNode {
public:
    explicit ASTNode(NodeType type) : type(type) {}
    inline NodeType get_type() const { return type; }
    inline void set_children(std::vector<GrammarNode> &&children) { this->children = std::move(children); }
    std::string print_tree(int indent = 0, bool is_last = true, const std::vector<bool> &last_stack = {}) const;

    template <typename Father>
    void for_each_child(Father &&father, size_t from = 0) {
        assert(from < children.size());
        for (size_t i = from; i < children.size(); ++i) {
            std::invoke(std::forward<Father>(father), children[i]);
        }
    }

    std::vector<GrammarNode> &get_children() { return children; }

private:
    NodeType type;
    std::vector<GrammarNode> children;
};
}  // namespace frontend::ast

namespace frontend::grammar {
using namespace frontend::ast;
using namespace frontend::lexer;
using GrammarNodeCollector = std::function<std::optional<std::vector<GrammarNode>>(Lexer *)>;
using GrammarNodeGenerator = std::function<std::optional<ASTNodePtr>(Lexer *)>;

extern const std::unordered_map<NodeType, GrammarNodeCollector> COLLECTORS;

enum class CollectorOperator { OPTION, SEVERAL };

inline GrammarNodeCollector operator+(const GrammarNodeCollector &lhs, const GrammarNodeCollector &rhs);
inline GrammarNodeCollector operator|(const GrammarNodeCollector &lhs, const GrammarNodeCollector &rhs);
inline GrammarNodeCollector operator*(const GrammarNodeCollector &gen, const CollectorOperator &op);

template <NodeType type>
GrammarNodeGenerator generator() {
    return [](Lexer *lexer) -> std::optional<ASTNodePtr> {
        auto it = COLLECTORS.find(type);
        if (it == COLLECTORS.end()) {
            logger::ERROR("[GrammarNodeGenerator::generator] node type ", type, " not implemented");
            return std::nullopt;
        }
        auto col = it->second;
        auto children = col(lexer);
        if (!children.has_value()) {
            return std::nullopt;
        }
        auto node = std::make_unique<ASTNode>(type);
        node->set_children(std::move(children.value()));
        return std::move(node);
    };
}

template <TokenType type>
GrammarNodeCollector collector() {
    return [](Lexer *lexer) -> std::optional<std::vector<GrammarNode>> {
        auto token = lexer->next_assert(type);
        if (!token.has_value()) {
            return std::nullopt;
        }
        std::vector<GrammarNode> result;
        result.emplace_back(std::make_unique<Token>(std::move(token.value())));
        return result;
    };
}

template <NodeType type>
GrammarNodeCollector collector() {
    return [](Lexer *lexer) -> std::optional<std::vector<GrammarNode>> {
        logger::DEBUG("[GrammarNodeCollector::collector] collecting node type ", type);
        auto node = generator<type>()(lexer);
        if (!node.has_value()) {
            return std::nullopt;
        }
        std::vector<GrammarNode> result;
        result.emplace_back(std::move(node.value()));
        return result;
    };
}

}  // namespace frontend::grammar
#endif
