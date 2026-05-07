#pragma once

#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <variant>
#include <cstdarg>

extern "C" {
#include "syntax_tree.h"
extern syntax_tree * parse(const char * input);
}
#include "User.hpp"


enum SysYType { TYPE_INT, TYPE_FLOAT, TYPE_VOID };

enum EqOp {
    // ==
    OP_EQ,
    // !=
    OP_NEQ
};

enum RelOp {
    // <=
    OP_LE,
    // <
    OP_LT,
    // >
    OP_GT,
    // >=
    OP_GE
};

enum UnaryOp {
    // +
    OP_POS,
    // -
    OP_NEG,
    // !
    OP_NOT
};

enum AddOp {
    // +
    OP_PLUS,
    // -
    OP_MINUS
};

enum MulOp {
    // *
    OP_MUL,
    // /
    OP_DIV,
    // %
    OP_MOD
};

class AST;

struct ASTNode;
struct ASTProgram;

struct ASTAddExp;
struct ASTAssignStmt;
struct ASTBlockItem;
struct ASTBlockStmt;
struct ASTBreakStmt;
struct ASTCall;
struct ASTCompUnit;
struct ASTCond;
struct ASTContinueStmt;
struct ASTDecl;
struct ASTDef;
struct ASTEqExp;
struct ASTExp;
struct ASTExpStmt;
struct ASTFuncDef;
struct ASTFuncParam;
struct ASTInitVal;
struct ASTIterationStmt;
struct ASTLAndExp;
struct ASTLOrExp;
struct ASTLVal;
struct ASTMulExp;
struct ASTNum;
struct ASTPrimaryExp;
struct ASTRelExp;
struct ASTReturnStmt;
struct ASTSelectionStmt;
struct ASTStmt;
struct ASTUnaryExp;

class ASTVisitor;

class AST {
  public:
    AST() = delete;
    AST(syntax_tree *);
    AST(AST && tree) {
        root = tree.root;
        tree.root = nullptr;
    };
    ASTProgram * get_root() { return root.get(); }
    void run_visitor(ASTVisitor & visitor);

  private:
    ASTNode * transform_node_iter(syntax_tree_node *);
    std::shared_ptr<ASTProgram> root = nullptr;
};

struct ASTNode {
    virtual Value * accept(ASTVisitor &) = 0;
    virtual ~ASTNode() = default;
};

struct ASTProgram : ASTNode {
    virtual Value * accept(ASTVisitor &) override final;
    std::vector<std::shared_ptr<ASTCompUnit>> comp_units;
};

struct ASTStmt : ASTNode {
    virtual Value * accept(ASTVisitor &) = 0;
    virtual ~ASTStmt() = default;
};

struct ASTAssignStmt : ASTStmt {
    virtual Value * accept(ASTVisitor &) override final;
    std::shared_ptr<ASTLVal> lval;
    std::shared_ptr<ASTExp> exp;
};

struct ASTBlockItem : ASTNode {
    virtual Value * accept(ASTVisitor &) override final;
    bool is_decl;
    // usage: std::get<std::share_ptr<T>>(item)
    std::variant<std::shared_ptr<ASTDecl>, std::shared_ptr<ASTStmt>> item;
};

struct ASTBlockStmt : ASTStmt {
    virtual Value * accept(ASTVisitor &) override final;
    std::vector<std::shared_ptr<ASTBlockItem>> block_items;
};

struct ASTBreakStmt : ASTStmt {
    virtual Value * accept(ASTVisitor &) override final;
};

struct ASTCompUnit : ASTNode {
    virtual Value * accept(ASTVisitor &) = 0;
    virtual ~ASTCompUnit() = default;
    SysYType type;
};

struct ASTContinueStmt : ASTStmt {
    virtual Value * accept(ASTVisitor &) override final;
};

struct ASTDecl : ASTCompUnit {
    virtual Value * accept(ASTVisitor &) override final;
    bool is_const = false;
    std::vector<std::shared_ptr<ASTDef>> defs;
};

struct ASTInitVal : ASTNode {
    virtual Value * accept(ASTVisitor &) override final;
    // determined dynamically
    bool is_const = false;
    SysYType type = TYPE_VOID;
    unsigned depth = 0;

    bool is_default = false;
    unsigned level = 0;
    // should be nullptr if no init value exists
    std::shared_ptr<ASTExp> exp;
    // should be nullptr if no init value exists
    std::vector<std::shared_ptr<ASTInitVal>> init_vals;
};

struct ASTDef : ASTNode {
    virtual Value * accept(ASTVisitor &) override final;
    std::string id;
    bool is_const = false;
    SysYType type = TYPE_VOID;
    std::vector<std::shared_ptr<ASTExp>> dims;
    // should be nullptr if no init value
    std::shared_ptr<ASTInitVal> init_val;
};

struct ASTFuncDef : ASTCompUnit {
    virtual Value * accept(ASTVisitor &) override final;
    std::string id;
    std::vector<std::shared_ptr<ASTFuncParam>> params;
    std::vector<std::shared_ptr<ASTBlockItem>> block_items;
};

struct ASTFuncParam : ASTNode {
    virtual Value * accept(ASTVisitor &) override final;
    SysYType type;
    std::string id;
    bool is_array;
    std::vector<std::shared_ptr<ASTExp>> dims;
};

struct ASTIterationStmt : ASTStmt {
    virtual Value * accept(ASTVisitor &) override final;
    std::shared_ptr<ASTCond> cond;
    std::shared_ptr<ASTStmt> stmt;
};

struct ASTReturnStmt : ASTStmt {
    virtual Value * accept(ASTVisitor &) override final;
    // should be nullptr if return void
    std::shared_ptr<ASTExp> exp;
};

struct ASTSelectionStmt : ASTStmt {
    virtual Value * accept(ASTVisitor &) override final;
    std::shared_ptr<ASTCond> cond;
    std::shared_ptr<ASTStmt> if_stmt;
    // should be nullptr if no else structure exists
    std::shared_ptr<ASTStmt> else_stmt;
};

struct ASTExpStmt : ASTStmt {
    virtual Value * accept(ASTVisitor &) override final;
    std::shared_ptr<ASTExp> exp;
};
/***********************************************************************************/

struct ASTCond : ASTNode {
    virtual Value * accept(ASTVisitor &) = 0;
    virtual ~ASTCond() = default;
};

struct ASTExp : ASTNode {
    virtual Value * accept(ASTVisitor &) = 0;
    virtual ~ASTExp() = default;
};

struct ASTLOrExp : ASTCond {
    virtual Value * accept(ASTVisitor &) override final;
    std::vector<std::shared_ptr<ASTLAndExp>> and_exps;
};

struct ASTLAndExp : ASTNode {
    virtual Value * accept(ASTVisitor &) override final;
    std::vector<std::shared_ptr<ASTEqExp>> eq_exps;
};

struct ASTEqExp : ASTNode {
    virtual Value * accept(ASTVisitor &) override final;
    std::shared_ptr<ASTRelExp> left;
    std::vector<std::pair<EqOp, std::shared_ptr<ASTRelExp>>> right;
};

struct ASTRelExp : ASTNode {
    virtual Value * accept(ASTVisitor &) override final;
    std::shared_ptr<ASTAddExp> left;
    std::vector<std::pair<RelOp, std::shared_ptr<ASTAddExp>>> right;
};

struct ASTAddExp : ASTExp {
    virtual Value * accept(ASTVisitor &) override final;
    std::shared_ptr<ASTMulExp> left;
    std::vector<std::pair<AddOp, std::shared_ptr<ASTMulExp>>> right;
};

struct ASTMulExp : ASTNode {
    virtual Value * accept(ASTVisitor &) override final;
    std::shared_ptr<ASTUnaryExp> left;
    std::vector<std::pair<MulOp, std::shared_ptr<ASTUnaryExp>>> right;
};

struct ASTUnaryExp : ASTNode {
    virtual Value * accept(ASTVisitor &) override final;
    // simplify UnaryOp list
    std::vector<UnaryOp> ops;
    std::shared_ptr<ASTPrimaryExp> primary_exp;
};

struct ASTPrimaryExp : ASTNode {
    virtual Value * accept(ASTVisitor &) = 0;
    virtual ~ASTPrimaryExp() = default;
};

struct ASTParenthesizedExp : ASTPrimaryExp{
    // 括号表达式, e.g. (a + b), (c * d)
    virtual Value* accept(ASTVisitor &) override final;
    std::shared_ptr<ASTExp> exp;
};

struct ASTCall : ASTPrimaryExp {
    // 函数调用, e.g. f(a, b, c)
    virtual Value* accept(ASTVisitor &) override final;
    std::string id; // function name
    std::vector<std::shared_ptr<ASTExp>> params; // function arguments
};

struct ASTLVal : ASTPrimaryExp {
    virtual Value * accept(ASTVisitor &) override final;
    bool is_lval = false;
    std::string id;
    std::vector<std::shared_ptr<ASTExp>> indices;
};

struct ASTNum : ASTPrimaryExp {
    virtual Value * accept(ASTVisitor &) override final;
    SysYType type;
    union {
        int i_val;
        float f_val;
    };
};

/***********************************************************************************/

class ASTVisitor {
  public:
    virtual Value * visit(ASTProgram &) = 0;
    virtual Value * visit(ASTAddExp &) = 0;
    virtual Value * visit(ASTAssignStmt &) = 0;
    virtual Value * visit(ASTBlockItem &) = 0;
    virtual Value * visit(ASTBlockStmt &) = 0;
    virtual Value * visit(ASTBreakStmt &) = 0;
    virtual Value * visit(ASTCall &) = 0;
    virtual Value * visit(ASTContinueStmt &) = 0;
    virtual Value * visit(ASTDecl &) = 0;
    virtual Value * visit(ASTDef &) = 0;
    virtual Value * visit(ASTEqExp &) = 0;
    virtual Value * visit(ASTFuncDef &) = 0;
    virtual Value * visit(ASTFuncParam &) = 0;
    virtual Value * visit(ASTInitVal &) = 0;
    virtual Value * visit(ASTIterationStmt &) = 0;
    virtual Value * visit(ASTLAndExp &) = 0;
    virtual Value * visit(ASTLOrExp &) = 0;
    virtual Value * visit(ASTLVal &) = 0;
    virtual Value * visit(ASTMulExp &) = 0;
    virtual Value * visit(ASTNum &) = 0;
    virtual Value * visit(ASTParenthesizedExp &) = 0;
    virtual Value * visit(ASTRelExp &) = 0;
    virtual Value * visit(ASTReturnStmt &) = 0;
    virtual Value * visit(ASTSelectionStmt &) = 0;
    virtual Value * visit(ASTUnaryExp &) = 0;
    virtual Value * visit(ASTExpStmt &) = 0;
};

class ASTPrinter : public ASTVisitor {
  public:
    virtual Value * visit(ASTProgram &) override final;
    virtual Value * visit(ASTAddExp &) override final;
    virtual Value * visit(ASTAssignStmt &) override final;
    virtual Value * visit(ASTBlockItem &) override final;
    virtual Value * visit(ASTBlockStmt &) override final;
    virtual Value * visit(ASTBreakStmt &) override final;
    virtual Value * visit(ASTCall &) override final;
    virtual Value * visit(ASTContinueStmt &) override final;
    virtual Value * visit(ASTDecl &) override final;
    virtual Value * visit(ASTDef &) override final;
    virtual Value * visit(ASTEqExp &) override final;
    virtual Value * visit(ASTFuncDef &) override final;
    virtual Value * visit(ASTFuncParam &) override final;
    virtual Value * visit(ASTInitVal &) override final;
    virtual Value * visit(ASTIterationStmt &) override final;
    virtual Value * visit(ASTLAndExp &) override final;
    virtual Value * visit(ASTLOrExp &) override final;
    virtual Value * visit(ASTLVal &) override final;
    virtual Value * visit(ASTMulExp &) override final;
    virtual Value * visit(ASTNum &) override final;
    virtual Value * visit(ASTParenthesizedExp &) override final;
    virtual Value * visit(ASTRelExp &) override final;
    virtual Value * visit(ASTReturnStmt &) override final;
    virtual Value * visit(ASTSelectionStmt &) override final;
    virtual Value * visit(ASTUnaryExp &) override final;
    virtual Value * visit(ASTExpStmt &) override final;
    void add_depth() { depth += 2; }
    void remove_depth() { depth -= 2; }

  private:
    int depth = 0;
};
