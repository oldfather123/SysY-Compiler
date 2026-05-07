#include <unordered_map>

#include "ast.hpp"
#include "token.hpp"

namespace frontend::grammar {
constexpr auto OPTION = CollectorOperator::OPTION;
constexpr auto SEVERAL = CollectorOperator::SEVERAL;

#define NODE(type) collector<NodeType::type>()
#define TOKEN(token) collector<TokenType::token>()

const std::unordered_map<NodeType, GrammarNodeCollector> COLLECTORS{
    // CompUnit -> {Decl | FuncDef}
    {NodeType::COMP_UNIT, (NODE(DECL) | NODE(FUNC_DEF)) * SEVERAL},

    // Decl -> ConstDecl | VarDecl
    {NodeType::DECL, NODE(CONST_DECL) | NODE(VAR_DECL)},

    // FuncDef -> FuncType Ident '(' [FuncFParams] ')' Block
    {NodeType::FUNC_DEF,
     NODE(FUNC_TYPE) + TOKEN(IDENFR) + TOKEN(LPARENT) + (NODE(FUNC_F_PARAMS)) * OPTION + TOKEN(RPARENT) + NODE(BLOCK)},

    // ConstDecl -> 'const' BType ConstDef {',' ConstDef} ';'
    {NodeType::CONST_DECL,
     TOKEN(CONSTTK) + NODE(B_TYPE) + NODE(CONST_DEF) + (TOKEN(COMMA) + NODE(CONST_DEF)) * SEVERAL + TOKEN(SEMICN)},

    // BType -> 'int' | 'float'
    {NodeType::B_TYPE, TOKEN(INTTK) | TOKEN(FLOATTK)},

    // ConstDef -> Ident {'[' ConstExp ']'} '=' ConstInitVal
    {NodeType::CONST_DEF,
     TOKEN(IDENFR) + (TOKEN(LBRACK) + NODE(CONST_EXP) + TOKEN(RBRACK)) * SEVERAL + TOKEN(ASSIGN) +
         NODE(CONST_INIT_VAL)},

    // ConstInitVal -> ConstExp | '{' [ConstInitVal {',' ConstInitVal}] '}'
    {NodeType::CONST_INIT_VAL,
     NODE(CONST_EXP) |
         (TOKEN(LBRACE) + (NODE(CONST_INIT_VAL) + (TOKEN(COMMA) + NODE(CONST_INIT_VAL)) * SEVERAL) * OPTION +
          TOKEN(RBRACE))},

    // VarDecl -> BType VarDef {',' VarDef} ';'
    {NodeType::VAR_DECL, NODE(B_TYPE) + NODE(VAR_DEF) + (TOKEN(COMMA) + NODE(VAR_DEF)) * SEVERAL + TOKEN(SEMICN)},

    // VarDef -> Ident {'[' ConstExp ']'} ['=' InitVal]
    {NodeType::VAR_DEF,
     TOKEN(IDENFR) + (TOKEN(LBRACK) + NODE(CONST_EXP) + TOKEN(RBRACK)) * SEVERAL +
         (TOKEN(ASSIGN) + NODE(INIT_VAL)) * OPTION},

    // InitVal -> Exp | '{' [InitVal {',' InitVal}] '}'
    {NodeType::INIT_VAL,
     NODE(EXP) |
         (TOKEN(LBRACE) + (NODE(INIT_VAL) + (TOKEN(COMMA) + NODE(INIT_VAL)) * SEVERAL) * OPTION + TOKEN(RBRACE))},

    // FuncType -> 'void' | 'int' | 'float'
    {NodeType::FUNC_TYPE, TOKEN(VOIDTK) | TOKEN(INTTK) | TOKEN(FLOATTK)},

    // FuncFParams -> FuncFParam {',' FuncFParam}
    {NodeType::FUNC_F_PARAMS, NODE(FUNC_F_PARAM) + (TOKEN(COMMA) + NODE(FUNC_F_PARAM)) * SEVERAL},

    // FuncFParam -> BType Ident ['[' ']' {'[' Exp ']'}]
    {NodeType::FUNC_F_PARAM,
     NODE(B_TYPE) + TOKEN(IDENFR) +
         (TOKEN(LBRACK) + TOKEN(RBRACK) + (TOKEN(LBRACK) + NODE(EXP) + TOKEN(RBRACK)) * SEVERAL) * OPTION},

    // Block -> '{' {BlockItem} '}'
    {NodeType::BLOCK, TOKEN(LBRACE) + (NODE(BLOCK_ITEM)) * SEVERAL + TOKEN(RBRACE)},

    // BlockItem -> Decl | Stmt
    {NodeType::BLOCK_ITEM, NODE(DECL) | NODE(STMT)},

    // Stmt -> LValStmt | ExpStmt | Block | IfStmt | WhileStmt | BreakStmt | ContinueStmt | ReturnStmt
    {NodeType::STMT,
     NODE(ASSIGN_STMT) | NODE(EXP_STMT) | NODE(BLOCK) | NODE(IF_STMT) | NODE(WHILE_STMT) | NODE(BREAK_STMT) |
         NODE(CONTINUE_STMT) | NODE(RETURN_STMT)},

    // AssignStmt -> LVal '=' Exp ';'
    {NodeType::ASSIGN_STMT, NODE(LVAL) + TOKEN(ASSIGN) + NODE(EXP) + TOKEN(SEMICN)},

    // ExpStmt -> [Exp] ';'
    {NodeType::EXP_STMT, (NODE(EXP)) * OPTION + TOKEN(SEMICN)},

    // IfStmt -> 'if' '(' Cond ')' Stmt [ 'else' Stmt ]
    {NodeType::IF_STMT,
     TOKEN(IFTK) + TOKEN(LPARENT) + NODE(COND) + TOKEN(RPARENT) + NODE(STMT) + (TOKEN(ELSETK) + NODE(STMT)) * OPTION},

    // WhileStmt -> 'while' '(' Cond ')' Stmt
    {NodeType::WHILE_STMT, TOKEN(WHILETK) + TOKEN(LPARENT) + NODE(COND) + TOKEN(RPARENT) + NODE(STMT)},

    // BreakStmt -> 'break' ';'
    {NodeType::BREAK_STMT, TOKEN(BREAKTK) + TOKEN(SEMICN)},

    // ContinueStmt -> 'continue' ';'
    {NodeType::CONTINUE_STMT, TOKEN(CONTINUETK) + TOKEN(SEMICN)},

    // ReturnStmt -> 'return' [Exp] ';'
    {NodeType::RETURN_STMT, TOKEN(RETURNTK) + (NODE(EXP)) * OPTION + TOKEN(SEMICN)},

    // Exp -> AddExp
    {NodeType::EXP, NODE(ADD_EXP)},

    // Cond -> LOrExp
    {NodeType::COND, NODE(LOR_EXP)},

    // LVal -> Ident {'[' Exp ']'}
    {NodeType::LVAL, TOKEN(IDENFR) + (TOKEN(LBRACK) + NODE(EXP) + TOKEN(RBRACK)) * SEVERAL},

    // PrimaryExp -> '(' Exp ')' | LVal | Number
    {NodeType::PRIMARY_EXP, (TOKEN(LPARENT) + NODE(EXP) + TOKEN(RPARENT)) | NODE(LVAL) | NODE(NUMBER)},

    // Number -> IntConst | floatConst
    {NodeType::NUMBER, TOKEN(INTCON) | TOKEN(FLOATCON)},

    // UnaryExp -> Ident '(' [FuncRParams] ')' | PrimaryExp | UnaryOp UnaryExp
    {NodeType::UNARY_EXP,
     (TOKEN(IDENFR) + TOKEN(LPARENT) + NODE(FUNC_R_PARAMS) * OPTION + TOKEN(RPARENT)) | NODE(PRIMARY_EXP) |
         (NODE(UNARY_OP) + NODE(UNARY_EXP))},

    // UnaryOp -> '+' | '-' | '!'
    {NodeType::UNARY_OP, TOKEN(PLUS) | TOKEN(MINU) | TOKEN(NOT)},

    // FuncRParams -> Exp {',' Exp}
    {NodeType::FUNC_R_PARAMS, NODE(EXP) + (TOKEN(COMMA) + NODE(EXP)) * SEVERAL},

    // MulExp -> UnaryExp {('*' | '/' | '%') UnaryExp}
    {NodeType::MUL_EXP, NODE(UNARY_EXP) + ((TOKEN(MULT) | TOKEN(DIV) | TOKEN(MOD)) + NODE(UNARY_EXP)) * SEVERAL},

    // AddExp -> MulExp {('+' | '-') MulExp}
    {NodeType::ADD_EXP, NODE(MUL_EXP) + ((TOKEN(PLUS) | TOKEN(MINU)) + NODE(MUL_EXP)) * SEVERAL},

    // RelExp -> AddExp {('<' | '>' | '<=' | '>=') AddExp}
    {NodeType::REL_EXP,
     NODE(ADD_EXP) + ((TOKEN(LSS) | TOKEN(GRE) | TOKEN(LEQ) | TOKEN(GEQ)) + NODE(ADD_EXP)) * SEVERAL},

    // EqExp -> RelExp {('==' | '!=') RelExp}
    {NodeType::EQ_EXP, NODE(REL_EXP) + ((TOKEN(EQL) | TOKEN(NEQ)) + NODE(REL_EXP)) * SEVERAL},

    // LAndExp -> EqExp {'&&' EqExp}
    {NodeType::LAND_EXP, NODE(EQ_EXP) + (TOKEN(AND) + NODE(EQ_EXP)) * SEVERAL},

    // LOrExp -> LAndExp {'||' LAndExp}
    {NodeType::LOR_EXP, NODE(LAND_EXP) + (TOKEN(OR) + NODE(LAND_EXP)) * SEVERAL},

    // ConstExp -> AddExp
    {NodeType::CONST_EXP, NODE(ADD_EXP)},

};
}  // namespace frontend::grammar

namespace frontend::grammar {
GrammarNodeCollector operator|(const GrammarNodeCollector &lhs, const GrammarNodeCollector &rhs) {
    return [lhs, rhs](Lexer *lexer) -> std::optional<std::vector<GrammarNode>> {
        lexer->branch();
        if (auto result = lhs(lexer)) {
            lexer->commit();
            return {std::move(result.value())};
        } else {
            lexer->rollback();
            lexer->branch();
            if (auto result = rhs(lexer)) {
                lexer->commit();
                return {std::move(result.value())};
            }
            lexer->rollback();
            return std::nullopt;
        }
    };
}

GrammarNodeCollector operator+(const GrammarNodeCollector &lhs, const GrammarNodeCollector &rhs) {
    return [lhs, rhs](Lexer *lexer) -> std::optional<std::vector<GrammarNode>> {
        auto tmp1 = lhs(lexer);
        if (!tmp1.has_value()) {
            return std::nullopt;
        }
        auto tmp2 = rhs(lexer);
        if (!tmp2.has_value()) {
            return std::nullopt;
        }
        auto res1 = std::move(tmp1.value());
        auto res2 = std::move(tmp2.value());
        res1.insert(res1.end(), std::make_move_iterator(res2.begin()), std::make_move_iterator(res2.end()));
        return res1;
    };
}

GrammarNodeCollector operator*(const GrammarNodeCollector &gen, const CollectorOperator &op) {
    return [gen, op](Lexer *lexer) -> std::optional<std::vector<GrammarNode>> {
        switch (op) {
            case CollectorOperator::OPTION: {
                if (auto result = gen(lexer)) {
                    return {std::move(result.value())};
                }
                return std::vector<GrammarNode>();
            }
            case CollectorOperator::SEVERAL: {
                std::vector<GrammarNode> res;
                while (auto result = gen(lexer)) {
                    res.insert(res.end(),
                               std::make_move_iterator(result.value().begin()),
                               std::make_move_iterator(result.value().end()));
                }
                return std::move(res);
            }
            default:
                __builtin_unreachable();
        }
    };
}

}  // namespace frontend::grammar

namespace frontend::ast {
std::string ASTNode::print_tree(int indent, bool is_last, const std::vector<bool> &last_stack) const {
    std::ostringstream oss;

    // Print indentation
    for (int i = 0; i < indent; ++i) {
        if (static_cast<size_t>(i) < last_stack.size()) {
            oss << (last_stack[i] ? "    " : "│   ");
        }
    }

    // Print current node
    if (indent > 0) {
        oss << (is_last ? "└── " : "├── ");
    }
    oss << type << "\n";

    // Prepare new stack for children
    auto child_stack = last_stack;
    child_stack.push_back(is_last);

    // Print children
    for (size_t i = 0; i < children.size(); ++i) {
        bool child_is_last = (i == children.size() - 1);
        std::visit(
            [&](auto &&arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, ASTNodePtr>) {
                    if (arg) {
                        oss << arg->print_tree(indent + 1, child_is_last, child_stack);
                    }
                } else if constexpr (std::is_same_v<T, TokenPtr>) {
                    if (arg) {
                        // Print indentation for token
                        for (int j = 0; j < indent + 1; ++j) {
                            if (static_cast<size_t>(j) < child_stack.size()) {
                                oss << (child_stack[j] ? "    " : "│   ");
                            }
                        }
                        oss << (child_is_last ? "└── " : "├── ") << arg->get_content() << "\n";
                    }
                }
            },
            children[i]);
    }

    return oss.str();
}
}  // namespace frontend::ast
